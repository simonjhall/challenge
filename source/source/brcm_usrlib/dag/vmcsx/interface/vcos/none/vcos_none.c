/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - no-RTOS implementation code
=============================================================================*/

#define VCOS_INLINE_BODIES
#define VCOS_ASSERT_LOGGING_DISABLE 1
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "interface/vcos/vcos.h"
#include "middleware/rpc/rpc.h"
#include "drivers/chip/intctrl.h"
#include "vcfw/logging/logging.h"

VCOS_THREAD_T vcos_none_vpu0_thread;
VCOS_LLTHREAD_T *vcos_none_vpu1_thread;
RTOS_LATCH_T lock = rtos_latch_unlocked();

volatile int vcos_none_spinlock_claim[2], vcos_none_spinlock_decider;

VCOS_STATUS_T vcos_init(void)
{
   VCOS_STATUS_T status;

   status = _vcos_thread_create_attach(&vcos_none_vpu0_thread, "vpu0");
   if (status != VCOS_SUCCESS)
      return status;

   vcos_none_vpu1_thread = NULL;

   vcos_none_spinlock_claim[0] = 0;
   vcos_none_spinlock_claim[1] = 0;

   vcos_logging_init();

   return VCOS_SUCCESS;
}

void vcos_deinit(void)
{
   _vcos_thread_delete(&vcos_none_vpu0_thread);
}

void vcos_global_lock(void)
{
   rtos_latch_get(&lock);
}

void vcos_global_unlock(void)
{
   rtos_latch_put(&lock);
}

void vcos_log_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, ...)
{
   va_list ap;
   va_start(ap,fmt);
   vlogging_message(LOGGING_GENERAL, fmt, ap);
   va_end(ap);
}

void vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args)
{
   vlogging_message(LOGGING_GENERAL, fmt, args);
}

void vcos_none_timer_expiration_routine(RTOS_TIMER_T *timer, void *context)
{
   VCOS_TIMER_T *vcos_timer = (VCOS_TIMER_T *)context;

   vcos_assert(timer == &vcos_timer->timer);

   (vcos_timer->fn)(vcos_timer->context);
}

/* For now, logging is done on this platform using the VCFW
 * logging facility.
 */
void vcos_logging_init(void)
{
}

VCOS_STATUS_T vcos_timer_init(void)
{
   return VCOS_SUCCESS;
}

int vcos_snprintf(char *buf, size_t buflen, const char *fmt, ...)
{
   int ret;
   va_list ap;
   va_start(ap,fmt);
   ret = vsnprintf(buf, buflen, fmt, ap);
   va_end(ap);
   return ret;
}


/* This is a special-purpose implementation of llthread to replace rtos_cpu_launch */

static void thread_exit_callback(int dummy)
{
}

static void need_rpc()
{
   static uint32_t initialised = 0;

   if (!initialised)
   {
      rpc_init();
      rpc_callback = thread_exit_callback;
      initialised = 1;
   }
}

static int launcher( VCOS_LLTHREAD_ENTRY_FN_T entry, void *arg )
{
   // TODO: rpc seems to leave interrupts disabled when it calls its function
   intctrl_get_func_table()->enable_interrupts( 0 );
   while (vcos_none_vpu1_thread->suspended) ;
   entry(arg);
   return 0;
}

VCOS_STATUS_T vcos_llthread_create(VCOS_LLTHREAD_T *thread,
                                   const char *name,
                                   VCOS_LLTHREAD_ENTRY_FN_T entry,
                                   void *arg,
                                   void *stack,
                                   VCOS_UNSIGNED stacksz,
                                   VCOS_UNSIGNED priority,
                                   VCOS_UNSIGNED affinity,
                                   VCOS_UNSIGNED timeslice,
                                   VCOS_UNSIGNED autostart)
{
   vcos_assert(priority >= VCOS_THREAD_PRI_MIN);
   vcos_assert(priority <= VCOS_THREAD_PRI_MAX);
   vcos_assert((affinity & ~VCOS_AFFINITY_MASK) == 0);
   vcos_assert((stacksz & 3) == 0);
   vcos_assert(timeslice < 1024);
   vcos_assert((autostart == VCOS_START) || (autostart == VCOS_NO_START));

   /* Extra restrictions for the RPC-based implementation */
   vcos_assert(stack == NULL);
   if ( rtos_get_cpu_number() == 0 &&
        affinity == VCOS_AFFINITY_CPU1 &&
        vcos_none_vpu1_thread == NULL )
   {
      need_rpc();
      vcos_none_vpu1_thread = thread;
      thread->suspended = !autostart;
      rpc_function = (int(*)())launcher;
      rpc_execute( entry, arg );
      return VCOS_SUCCESS;
   }
   else
   {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

void vcos_llthread_exit(void) {
   /* vcos_llthread_delete might have been called by this point! */
   vcos_assert(rtos_get_cpu_number() == 1);
   rpc_exit(0);
}

void vcos_llthread_delete(VCOS_LLTHREAD_T *thread) {
   vcos_assert(thread == vcos_none_vpu1_thread);
   vcos_none_vpu1_thread = NULL;
}

VCOS_STATUS_T _vcos_llthread_create_attach(VCOS_LLTHREAD_T *thread)
{
   thread->suspended = 0;
   return VCOS_SUCCESS;
}

int vcos_have_rtos(void)
{
   return 0;
}

#define OPTION "--vcloglevel="

const char * _vcos_log_level(void)
{
   int i;
   for (i=0; i<VCOS_APPLICATION_ARGC; i++)
   {
      const char *arg = VCOS_APPLICATION_ARGV[i];
      if (strncmp(arg,OPTION,strlen(OPTION)) == 0)
      {
         return arg+strlen(OPTION);
      }
   }
   return NULL;
}
