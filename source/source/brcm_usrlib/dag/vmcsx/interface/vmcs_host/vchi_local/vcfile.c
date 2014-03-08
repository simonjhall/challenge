/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
Copyright (c) 2002 Alphamosaic Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Host-specific functions
File     :  $RCSfile: vcfile.c,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION
Operating system depending host-side functions.
File system functions.
=============================================================================*/

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <vcfilesys.h>

#include "vchost.h"
#include "filesystem/filesys.h"

/******************************************************************************
Global data.
******************************************************************************/
#define TOP_LEVEL_PATH "/mfs/sd"
#ifndef MAX_PATH
#define MAX_PATH 256
#endif

static char szpath[MAX_PATH];
#define IS_SLASH(c) ((c)=='\\' || (c)=='/')
#define SLASH_S "/"
#define SLASH_C '/'
#define SLASH (SLASH_S[0])

static void convert_slashes(char *p) {
   if (!p) return;
   for (; *p; p++) {
      if (IS_SLASH(*p)) *p = SLASH;
   }
}


/******************************************************************************
Local types and defines.
******************************************************************************/

/******************************************************************************
NAME
   vc_hostfs_init

SYNOPSIS
   void vc_hostfs_init(void)

FUNCTION
   Initialises the host to accept requests from VC01

RETURNS
   void
******************************************************************************/

void vc_hostfs_init (void) {
}


/******************************************************************************
NAME
   vc_hostfs_exit

SYNOPSIS
   void vc_hostfs_exit(void)

FUNCTION
   De-initialises the host from accepting requests from VC01

RETURNS
   void
******************************************************************************/

void vc_hostfs_exit (void) {
}


/******************************************************************************
NAME
   vc_hostfs_close

SYNOPSIS
   int vc_hostfs_close(int fildes)

FUNCTION
   Deallocates the file descriptor to a file.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_close (int fildes) {
   /* Nothing to do here either */
  
   
  return(close(fildes));   
}


/******************************************************************************
NAME
   vc_hostfs_lseek

SYNOPSIS
   long vc_hostfs_lseek(int fildes, long offset, int whence)

FUNCTION
   Sets the file pointer associated with the open file specified by fildes.

RETURNS
   Successful completion: offset
   Otherwise: -1
******************************************************************************/

/*
long vc_hostfs_lseek (int fildes, long offset, int whence) {
   int retval = lseek(fildes, offset, whence);
   return (!retval)? offset : -1;
   
}
*/

long vc_hostfs_lseek (int fildes, long offset, int whence) {
   int retval = lseek(fildes, offset, whence);
   return(retval);
   //return (!retval)? offset : -1;
   
}
/******************************************************************************
NAME
   vc_hostfs_lseek64

SYNOPSIS
   int64_t vc_hostfs_lseek64(int fildes, int64_t offset, int whence)

FUNCTION
   Sets the file pointer associated with the open file specified by fildes.

RETURNS
   Successful completion: offset
   Otherwise: -1
******************************************************************************/

int64_t vc_hostfs_lseek64 (int fildes, int64_t offset, int whence) {
   int retval = lseek64(fildes, offset, whence);
   return(retval);
   //return (!retval)? offset : -1;
}


/******************************************************************************
NAME
   vc_hostfs_open

SYNOPSIS
   int vc_hostfs_open(const char *path, int vc_oflag)

FUNCTION
   Establishes a connection between a file and a file descriptor.

RETURNS
   Successful completion: file descriptor
   Otherwise: -1
******************************************************************************/

int vc_hostfs_open (const char *path, int vc_oflag) {
   int fd;
   int oflag;

   if (strstr(path, TOP_LEVEL_PATH) != path) {
      /* Path does not start with /mfs */
      sprintf(szpath, "%s", TOP_LEVEL_PATH);
      strncpy(szpath + sizeof(TOP_LEVEL_PATH) - 1 , path, MAX_PATH - sizeof(TOP_LEVEL_PATH));
   } else {
      strncpy(szpath, path, MAX_PATH - 1);
   }
   convert_slashes(szpath);
   
   // we are calling straight to vc so convert back to nucleus values
   oflag = O_RDONLY;
   if (vc_oflag & VC_O_WRONLY) {
      oflag  = O_WRONLY; }
   if (vc_oflag & VC_O_RDWR)   {
      oflag  = O_RDWR; }
   if (vc_oflag & VC_O_APPEND) {
      oflag |= O_APPEND; }
   if (vc_oflag & VC_O_CREAT)  {
      oflag |= O_CREAT; }
   if (vc_oflag & VC_O_TRUNC)  {
      oflag |= O_TRUNC; }
   if (vc_oflag & VC_O_EXCL)   {
      oflag |= O_EXCL; }
   

   fd = open(szpath, oflag);
   if (fd < 0)
      fd = open(szpath, oflag, O_CREAT);

   return fd;

}


/******************************************************************************
NAME
   vc_hostfs_read

SYNOPSIS
   int vc_hostfs_read(int fildes, void *buf, unsigned int nbyte)

FUNCTION
   Attempts to read nbyte bytes from the file associated with the file
   descriptor, fildes, into the buffer pointed to by buf.

RETURNS
   Successful completion: number of bytes read
   Otherwise: -1
******************************************************************************/

int vc_hostfs_read (int fildes, void *buf, unsigned int nbyte) {
   int b = read(fildes, (char *) buf, nbyte);
   //assert(b == (int)nbyte);
   return b;

}


/******************************************************************************
NAME
   vc_hostfs_write

SYNOPSIS
   int vc_hostfs_write(int fildes, const void *buf, unsigned int nbyte)

FUNCTION
   Attempts to write nbyte bytes from the buffer pointed to by buf to file
   associated with the file descriptor, fildes.

RETURNS
   Successful completion: number of bytes written
   Otherwise: -1
******************************************************************************/

int vc_hostfs_write (int fildes, const void *buf, unsigned int nbyte) {
   int b = write(fildes, (char *)buf, nbyte);
   assert(b == nbyte);
   assert(b>0);
   return b;

}

/******************************************************************************
NAME
   vc_hostfs_closedir

SYNOPSIS
   int vc_hostfs_closedir(void *dhandle)

FUNCTION
   Ends a directory list iteration.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_closedir (void *dhandle) {
   return filesys_closedir( (DIR *) dhandle);
}


/******************************************************************************
NAME
   vc_hostfs_format

SYNOPSIS
   int vc_hostfs_format(const char *path)

FUNCTION
   Formats the physical file system that contains path.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_format (const char *path) {
   return filesys_format(path);
}


/******************************************************************************
NAME
   vc_hostfs_freespace

SYNOPSIS
   int vc_hostfs_freespace(const char *path)

FUNCTION
   Returns the amount of free space on the physical file system that contains
   path.

RETURNS
   Successful completion: free space
   Otherwise: -1
******************************************************************************/

int vc_hostfs_freespace (const char *path) {
   return filesys_freespace(path);
}


/******************************************************************************
NAME
   vc_hostfs_freespace64

SYNOPSIS
   int64_t vc_hostfs_freespace64(const char *path)

FUNCTION
   Returns the amount of free space on the physical file system that contains
   path.

RETURNS
   Successful completion: free space
   Otherwise: -1
******************************************************************************/

int64_t vc_hostfs_freespace64 (const char *path) {
   return filesys_freespace64(path);
}


/******************************************************************************
NAME
   vc_hostfs_get_attr

SYNOPSIS
   int vc_hostfs_get_attr(const char *path, fattributes_t *attr)

FUNCTION
   Gets the file/directory attributes.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_get_attr (const char *path, fattributes_t *attr) {
   return filesys_get_attr(path, attr);
}


/******************************************************************************
NAME
   vc_hostfs_mkdir

SYNOPSIS
   int vc_hostfs_mkdir(const char *path)

FUNCTION
   Creates a new directory named by the pathname pointed to by path.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_mkdir (const char *path) {
   return filesys_mkdir(path, 0);
}


/******************************************************************************
NAME
   vc_hostfs_opendir

SYNOPSIS
   void *vc_hostfs_opendir(const char *dirname)

FUNCTION
   Starts a directory list iteration of sub-directories.

RETURNS
   Successful completion: dhandle (pointer)
   Otherwise: NULL
******************************************************************************/

void *vc_hostfs_opendir (const char *dirname) {
   return filesys_opendir(dirname);
}


/******************************************************************************
NAME
   vc_hostfs_readdir_r

SYNOPSIS
   struct dirent *vc_hostfs_readdir_r(void *dhandle, struct dirent *result)

FUNCTION
   Fills in the passed result structure with details of the directory entry
   at the current psition in the directory stream specified by the argument
   dhandle, and positions the directory stream at the next entry. If the last
   sub-directory has been reached it ends the iteration and begins a new one
   for files in the directory.

RETURNS
   Successful completion: result
   End of directory stream: NULL
******************************************************************************/

struct dirent *vc_hostfs_readdir_r (void *dhandle, struct dirent *result) {
   return filesys_readdir_r((DIR *)dhandle, result);
}

/******************************************************************************
NAME
   vc_hostfs_remove

SYNOPSIS
   int vc_hostfs_remove(const char *path)

FUNCTION
   Removes a file or a directory. A directory must be empty before it can be
   deleted.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_remove (const char *path) {
	
	if (strstr(path, TOP_LEVEL_PATH) != path) {
      /* Path does not start with /mfs */
      sprintf(szpath, "%s", TOP_LEVEL_PATH);
      strncpy(szpath + sizeof(TOP_LEVEL_PATH) - 1 , path, MAX_PATH - sizeof(TOP_LEVEL_PATH));
   } else {
      strncpy(szpath, path, MAX_PATH - 1);
   }
   
   convert_slashes(szpath);
	
   return filesys_remove(szpath);
}


/******************************************************************************
NAME
   vc_hostfs_rename

SYNOPSIS
   int vc_hostfs_rename(const char *old, const char *new)

FUNCTION
   Changes the name of a file. The old and new pathnames must be on the same
   physical file system.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_rename (const char *old, const char *newf) {
   return filesys_rename(old, newf);
}


/******************************************************************************
NAME
   vc_hostfs_set_attr

SYNOPSIS
   int vc_hostfs_set_attr(const char *path, fattributes_t attr)

FUNCTION
   Sets file/directory attributes.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_set_attr (const char *path, fattributes_t attr) {

   return filesys_set_attr(path, attr);
}


/******************************************************************************
NAME
   vc_hostfs_setend

SYNOPSIS
   int vc_hostfs_setend(int fildes)

FUNCTION
   Truncates file at current position.

RETURNS
   Successful completion: 0
   Otherwise: -1
******************************************************************************/

int vc_hostfs_setend (int fildes) {
   return fs_setend(fildes);
}


/******************************************************************************
NAME
   vc_hostfs_totalspace

SYNOPSIS
   int vc_hostfs_totalspace(const char *path)

FUNCTION
   Returns the total amount of space on the physical file system that contains
   path.

RETURNS
   Successful completion: total space
   Otherwise: -1
******************************************************************************/

int vc_hostfs_totalspace(const char *path)
{
   return filesys_totalspace(path);
}


/******************************************************************************
NAME
   vc_hostfs_totalspace64

SYNOPSIS
   int64_t vc_hostfs_totalspace64(const char *path)

FUNCTION
   Returns the total amount of space on the physical file system that contains
   path.

RETURNS
   Successful completion: total space
   Otherwise: -1
******************************************************************************/

int64_t vc_hostfs_totalspace64(const char *path)
{
   return filesys_totalspace64(path);
}


/******************************************************************************
NAME
   vc_hostfs_scandisk

SYNOPSIS
   int vc_hostfs_scandisk(const char *path)

FUNCTION
   Fixes up any problems with the filesystem

RETURNS
   -
******************************************************************************/

void vc_hostfs_scandisk(const char *path)
{
   filesys_scandisk(path);
}


/******************************************************************************
NAME
   vc_hostfs_chkdsk

SYNOPSIS
   int vc_hostfs_chkdsk(const char *path, int fix_errors)

FUNCTION
   Fixes up any problems with the filesystem

RETURNS
   -1 if error, 0 if no errors, 1 if errors present
******************************************************************************/

int vc_hostfs_chkdsk(const char *path, int fix_errors)
{
   return filesys_chkdsk(path, fix_errors);
}
