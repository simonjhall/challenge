#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "interface/vchiq_arm/vchiq.h"
#include "interface/vchi/vchi.h"
#include "interface/vcos/vcos.h"
#include "interface/vmcs_host/vc_dispmanx.h"
#include "interface/vmcs_host/vc_vchi_filesys.h"
#include "interface/vmcs_host/vc_tvservice.h"
#include "interface/vmcs_host/vc_cecservice.h"
#include "interface/vmcs_host/vc_vchi_bufman.h"
#include "interface/usbdk/vmcs_rpc_client/message_dispatch.h"
#include "host_applications/framework/host_app.h"
#include "vcgencmd.h"
#include "vchost.h"
#include "platform.h"


VCHI_INSTANCE_T global_initialise_instance;
VCHI_CONNECTION_T *global_connection;

/** Return non-zero (true) if filed is running.
  */
static int is_filed_running(void)
{
   const char *filename = "/var/run/vcfiled/vcfiled";
   int ret;
   
   int fd = open(filename, O_RDONLY);
   if (fd < 0)
   {
      // file not there, so filed not running
      ret = 0;
   }

   else
   {
      int rc = lockf(fd, F_TEST, 0);
      if ( rc == 0 )
      {
         // file is unlocked, so filed not running
         ret = 0;
      }
      else
      {
         switch (errno)
         {
         case EACCES:
         case EPERM:
            // file is locked, so filed is running
            ret = 1;
            break;
         default:
            printf("%s: Could not access lockfile %s: %s\n",
                  "vmcs_main", filename, strerror(errno));
            ret = 0;
         }
      }
   }
   return ret;
}
void vc_host_get_vchi_state(VCHI_INSTANCE_T *initialise_instance, VCHI_CONNECTION_T **connection)
{
   *initialise_instance = global_initialise_instance;
   *connection = global_connection;
}


int main (int argc, const char **argv)
{
   VCHIQ_INSTANCE_T vchiq_instance;

   int success = -1;
   char response[ 128 ];
   
   if (!is_filed_running())
   {
      printf("vcfiled not running\n");
      exit(-1);
   }
   vcos_init();
   vcos_set_args(argc, argv);

   if (vchiq_initialise(&vchiq_instance) != VCHIQ_SUCCESS)
   {
      printf("* failed to open vchiq instance\n");
      return -1;
   }

   // initialise the OS abstraction layer
   os_init();

   vcos_log("vchi_initialise");
   success = vchi_initialise( &global_initialise_instance);
   assert(success == 0);
   vchiq_instance = (VCHIQ_INSTANCE_T)global_initialise_instance;

   global_connection = vchi_create_connection(single_get_func_table(),
                                              vchi_mphi_message_driver_func_table());

   vcos_log("vchi_connect");
   vchi_connect(&global_connection, 1, global_initialise_instance);
  
   vc_vchi_gencmd_init (global_initialise_instance, &global_connection, 1);
   vc_vchi_dispmanx_init (global_initialise_instance, &global_connection, 1);
   vc_vchi_tv_init (global_initialise_instance, &global_connection, 1);
   vc_vchi_cec_init (global_initialise_instance, &global_connection, 1);
   vc_vchi_bufman_init (global_initialise_instance, &global_connection, 1);

   if ( success == 0 )
   {
      success = vc_gencmd( response, sizeof(response), "set_vll_dir /sd/vlls" );
      vcos_assert( success == 0 );
   }


   vcos_log("VMCS services initialized.");

   message_queue_init ();
   host_app_message_handler (PLATFORM_MSG_INIT, 0, 0);
   dispatch_messages ();

   vcos_deinit ();

   return 0;
}
