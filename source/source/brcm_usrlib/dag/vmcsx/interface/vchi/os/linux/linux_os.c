/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION: OS interface for Linux
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <semaphore.h>
#include <stdlib.h>
#include "interface/vchi/os/os.h"

#define MAX_EVENTS_IN_GROUP 32


/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

/******************************************************************************
Static func forwards
******************************************************************************/

/******************************************************************************
Global functions
******************************************************************************/

/***********************************************************
 * Name: os_logging_message
 *
 * Arguments: int level - which logging channel to use
 *            const char *format - printf-style formatting string
 *            ... - varargs
 *
 * Description: Routine to write message to log
 *
 * Returns: void
 *
 ***********************************************************/

void os_logging_message(const char *format, ...)
{
/*  // No logging level - hence logging messages can't be turned off - disabling until fixed

   va_list args;
   va_start(args, format);
   vprintf(format, args);
   va_end(args);
*/
}

/***********************************************************
 * Name: os_yield
 *
 * Arguments:  void
 *
 * Description: Routine to yield to other threads
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_yield( void )
{
   sched_yield();

   return 0;
}

/****************************** End of file ***********************************/


