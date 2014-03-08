/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

FILE DESCRIPTION
VideoCore file daemon
=============================================================================*/

/**
  * @file
  *
  * Daemon serving files to VideoCore from the host filing system.
  *
  */

#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <getopt.h>

#include "interface/vchiq_arm/vchiq.h"
#include "interface/vchi/vchi.h"
#include "interface/vcos/vcos.h"
#include "interface/vmcs_host/vc_vchi_filesys.h"
#include "interface/usbdk/vmcs_rpc_client/message_dispatch.h"
#include "vchost.h"
#include "platform.h"

static int log_stderr;              /** Debug output to stderr? */
static int foreground;              /** Don't fork - run in foreground? */
static const char *work_dir = "/";  /** Working directory */
static const char *progname;

VCHI_INSTANCE_T global_initialise_instance;
VCHI_CONNECTION_T *global_connection;


void vc_host_get_vchi_state(VCHI_INSTANCE_T *initialise_instance, VCHI_CONNECTION_T **connection)
{
   *initialise_instance = global_initialise_instance;
   *connection = global_connection;
}

static void logmsg(int level, const char *fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);

   if (!log_stderr)
   {
      vsyslog(level | LOG_DAEMON, fmt, ap);
   }
   else
   {
      vfprintf(stderr, fmt, ap);
   }
   va_end(ap);
}

static void usage(const char *progname)
{

   fprintf(stderr,"usage: %s [-debug] [-foreground] [-root dir]\n",
           progname);
   fprintf(stderr,"  -debug      - debug to stderr rather than syslog\n");
   fprintf(stderr,"  -foreground - do not fork, stay in foreground\n");
   fprintf(stderr,"  -root dir   - chdir to this directory first\n");
}

enum { OPT_DEBUG, OPT_FG, OPT_ROOT };

static void parse_args(int argc, char *const *argv)
{
   int finished = 0;
   static struct option long_options[] =
   {
      {"debug", no_argument, &log_stderr, 1},
      {"foreground", no_argument, &foreground, 1},
      {"root", required_argument, NULL, OPT_ROOT},
      {0, 0, 0, 0},
   };
   while (!finished)
   {
      int option_index = 0;
      int c = getopt_long_only(argc, argv, "", long_options, &option_index);

      switch (c)
      {
      case 0:
         // debug or foreground options, just sets flags directly
         break;
      case OPT_ROOT:
         work_dir = optarg;
         // sanity check directory
         if (chdir(work_dir) < 0)
         {
            fprintf(stderr,"cannot chdir to %s: %s\n", work_dir, strerror(errno));
            exit(-1);
         }
         break;

      default:
      case '?':
         usage(argv[0]);
         exit(-1);
         break;

      case -1:
         finished = 1;
         break;
      }

   }
}

// fork and exit.

static void daemonize(void)
{
   pid_t sid, pid = fork();
   if (pid > 0)
   {
      // we're the parent
      exit(0);
   }
   else if (pid < 0)
   {
      // we're still the parent, fork failed
      logmsg(LOG_CRIT, "failed to fork: %s\n", strerror(errno));
      exit(-1);
   }
   else
   {
      int fd;
      // we're the child
      sid = setsid();
      if (sid < 0)
      {
         logmsg(LOG_CRIT, "could not get new session id: %s\n", strerror(errno));
         exit(-1);
      }

      umask(0);
      if (chdir(work_dir) < 0)
      {
         logmsg(LOG_CRIT, "could not cd to %s\n", work_dir);
         exit(-1);
      }
      // reconnect standard descriptors to /dev/null to discourage
      // gratuitous output
      close(STDIN_FILENO);
      close(STDOUT_FILENO);
      close(STDERR_FILENO);

      fd = open("/dev/null", O_RDWR);
      if (fd >= 0)
      {
        (void)dup(fd);  // stdout
        (void)dup(fd);  // stderr
      }
      else
      {
         logmsg(LOG_CRIT, "could not open /dev/null:%s\n", strerror(errno));
         exit(-1);
      }
   }
}

// attempt to create a lock file or exit if already locked

static void vcfile_lock(void)
{
   const char *lockdir = "/var/run/vcfiled";
   char buf[128];
   int rc, fd;

   if (mkdir(lockdir, 0644) < 0)
   {
      if (errno != EEXIST)
      {
         logmsg(LOG_CRIT, "could not create %s:%s\n", lockdir,strerror(errno));
         exit(-1);
      }
   }
   snprintf(buf, sizeof(buf), "%s/%s", lockdir, progname);
   fd = open(buf, O_RDWR | O_CREAT | O_EXCL, 0640);
   if (fd<0)
   {
      if (errno != EEXIST)
      {
         logmsg(LOG_CRIT, "could not create lockfile %s:%s\n", buf, strerror(errno));
         exit(-1);
      }
      else
      {
         // it already exists - reopen it and try to lock it
         fd = open(buf, O_RDWR);
         if (fd<0)
         {
            logmsg(LOG_CRIT, "could not re-open lockfile %s:%s\n", buf, strerror(errno));
            exit(-1);
         }
      }
   }
   // at this point, we have opened the file, and can use discretionary locking, 
   // which should work even over NFS

   if (lockf(fd,F_TLOCK,0)<0)
   {
      // if we failed to lock, then it might mean we're already running, or
      // something else bad.
      if (errno == EACCES || errno == EAGAIN)
      {
         // already running
         int pid = 0, rc = read(fd, buf, sizeof(buf));
         if (rc)
            pid = atoi(buf);
         logmsg(LOG_CRIT, "already running at pid %d\n", pid);
         exit(0);
      }
      else
      {
         logmsg(LOG_CRIT, "could not lock %s:%s\n", buf, strerror(errno));
         exit(-1);
      }
   }
   snprintf(buf,sizeof(buf),"%d", getpid());
   rc = write(fd, buf, strlen(buf)+1);
   if (rc<0)
   {
      logmsg(LOG_CRIT, "could not write pid:%s\n", strerror(errno));
      exit(-1);
   }
}

int main(int argc, char *const *argv)
{
   VCHIQ_INSTANCE_T vchiq_instance;
   int success;

   progname = argv[0];
   const char *sep = strrchr(progname, '/');
   if (sep)
      progname = sep+1;

   parse_args(argc, argv);

   if (!foreground)
   {
      log_stderr = 0;           // must log to syslog if we're a daemon
      daemonize();
   }
   vcfile_lock();

   vcos_init();

   if (vchiq_initialise(&vchiq_instance) != VCHIQ_SUCCESS)
   {
      logmsg(LOG_ERR, "%s: failed to open vchiq instance\n", argv[0]);
      return -1;
   }

   // initialise the OS abstraction layer
   os_init();

   success = vchi_initialise( &global_initialise_instance);
   assert(success == 0);
   vchiq_instance = (VCHIQ_INSTANCE_T)global_initialise_instance;

   global_connection = vchi_create_connection(single_get_func_table(),
                                              vchi_mphi_message_driver_func_table());

   vchi_connect(&global_connection, 1, global_initialise_instance);
  
   vc_vchi_filesys_init (global_initialise_instance, &global_connection, 1);

   for (;;)
   {
      sleep(INT_MAX);
   }

   vcos_deinit ();

   return 0;
}

