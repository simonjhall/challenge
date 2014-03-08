/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision$

FILE DESCRIPTION:
=============================================================================*/

#include <string.h>
#include <stdlib.h>

#include "vcinclude/common.h"

#include "vcfw/rtos/rtos.h"
#include "vcfw/logging/logging.h"

#include "interface/vchi/os/os.h"

#include "helpers/vcsuspend/vcsuspend.h"

#ifdef __CC_ARM
#define inline __inline
#endif

/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

/******************************************************************************
Static func forwards
******************************************************************************/

#define rtos_quick_get_cpu_number() ((_vasm("version %D") >> 16) & 1)
static inline int32_t rtos_quick_disable_interrupts(void)
{
#ifdef __CC_ARM
   assert(0);
   return 0;
#else
   int32_t sr = _vasm("mov %D, %sr");
   _nop(); 
   _di();
   return sr;
#endif
}
static inline void rtos_quick_restore_interrupts(int32_t sr)
{
   if (sr & (1<<30))
      _ei();
   //else
   //   _di();
}

/******************************************************************************
Global functions
******************************************************************************/


/***********************************************************
 * Name: os_halt
 *
 * Arguments: void
 *
 * Description: Routine to prepare VideoCore for shutdown -
 *              called when VCHI DISCONNECT message received
 *
 * Returns: does not return
 *
 ***********************************************************/
int32_t os_halt( void )
{
#ifdef ANDROID
   return 0;
#else
   int32_t r = vcsuspend_halt();
   os_assert(0); // shouldn't reach here
   return r;
#endif
}

/***********************************************************
 * Name: os_suspend
 *
 * Arguments: void
 *
 * Description: Routine to suspend VideoCore -
 *              called when VCHI DISCONNECT message received
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_suspend( void )
{
#ifdef ANDROID
   return 0;
#else
   int32_t r = vcsuspend_suspend();
   os_assert(r == 0);
   return r;
#endif
}

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
#ifndef WIN32
void os_logging_message(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vlogging_message(LOGGING_VCHI, format, args);
   va_end(args);
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
   logging_assert(file, func, line, msg);
}
#endif

/****************************** End of file ***********************************/
