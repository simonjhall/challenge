/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision$

FILE DESCRIPTION:
=============================================================================*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vcinclude/common.h"

#include "interface/vchi/os/os.h"
#include "interface/vcos/vcos.h"

/******************************************************************************
Private typedefs
******************************************************************************/

struct {
   int sem;
   int sem_cnt;
   int sem_glb;
   int event;
   int cond;
   int alloc;
} stats_obtain, stats_release;

#define STATS(type, obtain_release) stats_##obtain_release.type++

void os_print_stats(void)
{
   printf("S:%u %u SC:%u %u SG:%u %u E:%u %u C:%u %u A:%u %u\n", stats_obtain.sem, stats_release.sem, 
   stats_obtain.sem_cnt, stats_release.sem_cnt, 
   stats_obtain.sem_glb, stats_release.sem_glb, 
   stats_obtain.event, stats_release.event, 
   stats_obtain.cond, stats_release.cond,
   stats_obtain.alloc, stats_release.alloc);
}


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
              const char *format - printf-style formatting string
              ... - varargs
 *
 * Description: Routine to write message to log
 *
 * Returns: void
 *
 ***********************************************************/

void os_logging_message(const char *format, ...)
{
#ifndef NDEBUG
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);

   putc('\n', stderr);
}

/***********************************************************
 * Name: os_assert_failure
 * 
 * Arguments: const char *msg - message for log (normally condition text)
 *            const char *file - filename of translation unit
 *            const char *func - name of function (NULL if not available)
 *            int line - line number
 */
void os_assert_failure(const char *msg, const char *file, const char *func, int line)
{
   fprintf(stderr, "In %s::%s(%d)\n%s\n", file, func, line, msg);
#if defined(_MSC_VER)                     // Visual C define equivalent types
   __asm { int 3 }
#endif //#if defined(_MSC_VER)                     // Visual C define equivalent types
   abort();
}

int32_t os_halt( void )
{
   os_assert(0); // shouldn't reach here
   return 0;
}

int32_t os_suspend( void )
{
   os_assert(0); // shouldn't reach here
   return 0;
}


/****************************** End of file ***********************************/
