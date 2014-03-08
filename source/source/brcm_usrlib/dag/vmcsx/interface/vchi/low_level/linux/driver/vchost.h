/*****************************************************************************
*  Copyright 2006 - 2007 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/


#ifndef VCHOST_H
#define VCHOST_H

#include "vchost_config.h"
#include "vc3host-port.h"

#define UNUSED_PARAMETER(x) ((void)(x))/* macro to suppress not use warning */

/*---------------------------------------------------------------------------*/
/* Host port related functions */
/*---------------------------------------------------------------------------*/

/* Perform any platform specific initialisations. */

int vc_host_init(void);

int vc_host_reg(void *hostReg);

/* Wait for an interrupt from VideoCore. This can return immediately if applications
   are happy to busy-wait. */

int vc_host_wait_interrupt(void);

/* Tell the host to act on or ignore interrupts. */

void vc_host_interrupts(int on);

/* Function called when there is some kind of internal error. Breakpoints can be set on
   this for debugging. */

void vc_error(void);

void vc_host_read_VC02_STC_reg (void) ;


/*---------------------------------------------------------------------------*/
/* Event (interrupt) related functions */
/*---------------------------------------------------------------------------*/

// Minimum number of event objects an implementation should support.
// Sufficient for 2 per 8 interfaces/services + 4 others
#define VC_EVENT_MAX_NUM  20

#define VC_EVENT_DEBUG  1

#if VC_EVENT_DEBUG

#define vc_event_create()       vc_event_create_dbg( __FILE__, __LINE__ )

void *vc_event_create_dbg( const char *fileName, int lineNum );

#define vc_event_wait(sig)      vc_event_wait_dbg( sig, __FILE__, __LINE__ )

void vc_event_wait_dbg( void *sig, const char *fileName, int lineNum );

#else

#define vc_event_create_dbg( fileName, lineNum )    vc_event_create()
/* Create (and clear) an event.  Returns a pointer to the event object. */
void *vc_event_create(void);

#define vc_event_wait_dbg( sig, fileName, lineNum ) vc_event_wait( sig )

/* Wait for an event to be set, blocking until it is set.
   Only one thread may be waiting at any one time.
   The event is automatically cleared on leaving this function. */
void vc_event_wait(void *sig);

#endif  // VC_EVENT_DEBUG


/* Reads the state of an event (for polling systems) */
int vc_event_status(void *sig);

/* Forcibly clears any pending event */
void vc_event_clear(void *sig);

/* Sets an event - can be called from any thread */
void vc_event_set(void *sig);

/* Register the calling task to be notified of an event. */
void vc_event_register(void *ievent);
/* Set events to block, stopping polling mode. */
void vc_event_blocking(void);

/*---------------------------------------------------------------------------*/
/* Semaphore related functions */
/*---------------------------------------------------------------------------*/

// Minimum number of locks an implementation should support.

#define VC_LOCK_MAX_NUM 32

#define VC_LOCK_DEBUG   1

// Create a lock. Returns a pointer to the lock object. A lock is initially available
// just once.

void *vc_lock_create(void);

#if VC_LOCK_DEBUG

#define vc_lock_obtain(lock)    vc_lock_obtain_dbg( lock, __FILE__, __LINE__ )

void vc_lock_obtain_dbg(void *lock, const char *fileName, int lineNum );

#else

#define vc_lock_obtain_dbg( lock, fileName, lineNum )   vc_lock_obtain( lock )

// Obtain a lock. Block until we have it. Locks are not re-entrant for the same thread.

void vc_lock_obtain(void *lock);

#endif  // VC_LOCK_DEBUG

// Release a lock. Anyone can call this, even if they didn't obtain the lock first.

void vc_lock_release(void *lock);

/*---------------------------------------------------------------------------*/
/* File system related functions */
/*---------------------------------------------------------------------------*/

#if defined(USE_FILESYS) && USE_FILESYS

#include "vcfilesys_defs.h"

// Initialises the host dependent file system functions for use
void vc_hostfs_init(void);

// Low level file system functions equivalent to close(), lseek(), open(), read() and write()
int vc_hostfs_close(int fildes);
long vc_hostfs_lseek(int fildes, long offset, int whence);
int64_t vc_hostfs_lseek64(int fildes, int64_t offset, int whence);
int vc_hostfs_open(const char *path, int vc_oflag);
int vc_hostfs_read(int fildes, void *buf, unsigned int nbyte);
int vc_hostfs_write(int fildes, const void *buf, unsigned int nbyte);

// Ends a directory listing iteration
int vc_hostfs_closedir(void *dhandle);

// Formats the drive that contains the given path
int vc_hostfs_format(const char *path);

// Returns the amount of free space on the drive that contains the given path
int vc_hostfs_freespace(const char *path);

// Gets the attributes of the named file
int vc_hostfs_get_attr(const char *path, fattributes_t *attr);

// Creates a new directory
int vc_hostfs_mkdir(const char *path);

// Starts a directory listing iteration
void *vc_hostfs_opendir(const char *dirname);

// Directory listing iterator
struct dirent *vc_hostfs_readdir_r(void *dhandle, struct dirent *result);

// Deletes a file or (empty) directory
int vc_hostfs_remove(const char *path);

// Renames a file, provided the new name is on the same file system as the old
int vc_hostfs_rename(const char *oldfile, const char *newfile);

// Sets the attributes of the named file
int vc_hostfs_set_attr(const char *path, fattributes_t attr);

// Truncates a file at its current position
int vc_hostfs_setend(int fildes);

// Returns the total amount of space on the drive that contains the given path
int vc_hostfs_totalspace(const char *path);

// Return millisecond resolution system time, only used for differences
int vc_millitime(void);

/*---------------------------------------------------------------------------*/
/* These functions only need to be implemented for the test system. */
/*---------------------------------------------------------------------------*/

// Open a log file.
void vc_log_open(const char *fname);

// Flush any pending data to the log file.
void vc_log_flush(void);

// Close the log file.
void vc_log_close(void);

// Log an error.
void vc_log_error(const char *format, ...);

// Log a warning.
void vc_log_warning(const char *format, ...);

// Write a message to the log.
void vc_log_msg(const char *format, ...);

// Flush the log.
void vc_log_flush(void);

// Return the total number of warnings and errors logged.
void vc_log_counts(int *warnings, int *errors);

// Wait for the specified number of microseconds. Used in test system only.
void vc_sleep(int ms);

// Get a time value in milliseconds. Used for measuring time differences
uint32_t vc_time(void);

// Check timing functions are available. Use in calibrating tests.
int calibrate_sleep (const char *data_dir);

#endif  // USE_FILESYS
#endif
