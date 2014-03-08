/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - ThreadX implementation code
=============================================================================*/

#define VCOS_INLINE_BODIES
#define VCOS_WANT_IMPL              /* we want the explicitly exported functions as regular funcs */
#define VCOS_ASSERT_LOGGING_DISABLE 1

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include "vcfw/logging/logging.h"
#include <stdarg.h>
#include "helpers/sysman/sysman.h"
#include <stdio.h>
#include <string.h>

static void hisr_wrapper(void *);

static SYSMAN_HANDLE_T vcos_acquire_vpu1( void )
{
   SYSMAN_HANDLE_T sysmanh;
   sysman_register_user(&sysmanh);
   if ( sysmanh )
      sysman_set_user_request( sysmanh, SYSMAN_BLOCK_VPU1, 1, SYSMAN_WAIT_BLOCKING );
   return sysmanh;
}

static void vcos_release_vpu1( SYSMAN_HANDLE_T sysmanh )
{
   if ( sysmanh ) {
      sysman_set_user_request(sysmanh, SYSMAN_BLOCK_VPU1, 0, SYSMAN_WAIT_BLOCKING);
      sysman_deregister_user(sysmanh);
   }
}

VCOS_STATUS_T vcos_threadx_map_error(UINT st)
{
   switch (st)
   {
   case TX_SUCCESS: return VCOS_SUCCESS;
   case TX_NOT_AVAILABLE:
   case TX_NO_INSTANCE:
   case TX_NO_MEMORY:
   case TX_NO_EVENTS:
   case TX_QUEUE_FULL:
   case TX_QUEUE_EMPTY:
       return VCOS_EAGAIN;
   default: return VCOS_EINVAL;
   }
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
   UINT st;
#if VCOS_THREAD_PRI_MIN
   vcos_assert(priority >= VCOS_THREAD_PRI_MIN);
#endif
   vcos_assert(priority <= VCOS_THREAD_PRI_MAX);
   vcos_assert((affinity & ~VCOS_AFFINITY_MASK) == 0);
   vcos_assert((stacksz & 3) == 0);
   vcos_assert(timeslice < 1024);

   if (affinity & VCOS_AFFINITY_CPU1)
   {
      thread->sysmanh = vcos_acquire_vpu1();
      if (!thread->sysmanh)
         return VCOS_ENXIO;
   }
   else
   {
      thread->sysmanh = NULL;
   }

   /* Leave priority zero for the system timer tasks */
   priority += 1;

   /* Ensure that legacy HISRs are higher priority than regular threads */
   if (entry != hisr_wrapper)
   {
      /* Map a regular thread priority of <n> to an actual priority of 1 + VCOS_HISR_PRI_MAX + 1 + <n> */
      priority += (VCOS_HISR_PRI_MAX + 1);
   }

   /* Copy the name, because ThreadX just stores the pointer */
   strncpy(thread->name, name, sizeof(thread->name));
   thread->name[sizeof(thread->name) - 1] = '\0';

   st = tx_thread_create(&thread->thread, (CHAR*)thread->name, (TX_ENTRY_FN)entry, (ULONG)arg, stack, stacksz,
                         priority | affinity, priority, timeslice, autostart);
   if (st != TX_SUCCESS)
   {
      vcos_release_vpu1(thread->sysmanh);
   }
   return vcos_threadx_map_error(st);
}

void vcos_llthread_exit(void) {
   UINT ret = tx_thread_terminate(tx_thread_identify());
   vcos_assert(ret == TX_SUCCESS);
}

void vcos_llthread_delete(VCOS_LLTHREAD_T *thread) {
   UINT ret = tx_thread_terminate(&thread->thread);
   vcos_assert(ret == TX_SUCCESS);
   ret = tx_thread_delete(&thread->thread);
   vcos_assert(ret == TX_SUCCESS);
   if (thread->sysmanh)
   {
      vcos_release_vpu1(thread->sysmanh);
      thread->sysmanh = NULL;
   }
}

void vcos_threadx_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity)
{
#ifdef WANT_SMP
   UINT pri;
   UINT ret = tx_thread_info_get(&thread->thread.thread, 0, 0, 0, &pri, 0, 0, 0, 0);
   if (affinity != (pri & _VCOS_AFFINITY_MASK))
   {
      UINT ret, old_pri;
      int release_cpu1 = 0;
      if (affinity & _VCOS_AFFINITY_CPU1)
      {
         // acquire cpu1 if not already acquired
         if (thread->thread.sysmanh == NULL)
            thread->thread.sysmanh = vcos_acquire_vpu1();
      }
      else
      {
         // need to release cpu1 if already acquired, once the thread has migrated
         if (thread->thread.sysmanh != NULL)
            release_cpu1 = 1;
      }
      ret = tx_thread_priority_change(&thread->thread.thread, (pri & ~_VCOS_AFFINITY_MASK) | affinity, &old_pri);
      vcos_assert(ret == TX_SUCCESS);
      if (release_cpu1)
      {
         vcos_release_vpu1(thread->thread.sysmanh);
         thread->thread.sysmanh = NULL;
      }
   }
#else // Non-SMP case
   // Do nothing
#endif
}

static RTOS_LATCH_T lock = rtos_latch_unlocked();
static int inited;
VCOS_STATUS_T vcos_init(void)
{
   VCOS_STATUS_T st = VCOS_SUCCESS;

   rtos_latch_get(&lock);

   if (!inited)
   {
#ifdef VCOS_USE_SPINLOCK_SEMAPHORES
      vcos_spinlock_init();
#endif

      vcos_logging_init();

      st = vcos_tls_init();
      vcos_assert(st == VCOS_SUCCESS);

      if (st == VCOS_SUCCESS)
         st = _vcos_named_semaphore_init();
      vcos_assert(st == VCOS_SUCCESS);

      st = vcos_msgq_init();
      vcos_assert(st == VCOS_SUCCESS);

      st = vcos_thread_reaper_init();
      vcos_assert(st == VCOS_SUCCESS);

      inited = 1;
   }

   rtos_latch_put(&lock);

   return st;
}

void vcos_deinit(void)
{
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
   vlogging_message(LOGGING_VCOS, fmt, ap);
   va_end(ap);
}

void vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args)
{
   vlogging_message(LOGGING_VCOS, fmt, args);
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

int vcos_llthread_running(VCOS_LLTHREAD_T *thread)
{
   UINT state;
   UINT st;

   st = tx_thread_info_get(&thread->thread,
                           NULL, &state, NULL, NULL,
                           NULL, NULL, NULL, NULL);

   vcos_assert(st == TX_SUCCESS);

   if ((state == TX_READY) || (state == TX_SLEEP))
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

static void hisr_wrapper(void *cxt)
{
   VCOS_HISR_T *hisr = cxt;
   while (1)
   {
      vcos_semaphore_wait(&hisr->sem);
      if (hisr->quit)
         break;
      else
      {
         hisr->calls++;
         hisr->entry();
      }
   }
   vcos_semaphore_post(&hisr->exitsem);
   vcos_llthread_exit();
}

VCOS_STATUS_T vcos_threadx_legacy_hisr_create(
   VCOS_HISR_T *hisr,
   const char *name,
   void (*entry)(void),
   VCOS_UNSIGNED pri,
   void *stack,
   VCOS_UNSIGNED stack_size)
{
   VCOS_STATUS_T st;
#if VCOS_HISR_PRI_MIN
   vcos_assert(pri >= VCOS_HISR_PRI_MIN);
#endif
   vcos_assert(pri <= VCOS_HISR_PRI_MAX);
   memset(hisr, 0, sizeof(*hisr));

   /* The name will be filled in later when the thread is created. */
   hisr->thread.name[0] = '\0';

   hisr->stack = stack;
   hisr->entry = entry;

   st = vcos_semaphore_create(&hisr->sem, hisr->thread.name, 0);
   if (st != VCOS_SUCCESS)
      goto fail_sem;

   st = vcos_semaphore_create(&hisr->exitsem, hisr->thread.name, 0);
   if (st != VCOS_SUCCESS)
      goto fail_exitsem;
   
   st = vcos_llthread_create(&hisr->thread,
                             name,
                             hisr_wrapper,
                             hisr,
                             stack,
                             stack_size,
                             pri,
                             pri & _VCOS_AFFINITY_MASK, TX_NO_TIME_SLICE, VCOS_START);
   if (st != VCOS_SUCCESS)
      goto fail_thread;

   return VCOS_SUCCESS;

fail_thread:
   vcos_semaphore_delete(&hisr->exitsem);
fail_exitsem:
   vcos_semaphore_delete(&hisr->sem);
fail_sem:
   return st;
}

void vcos_threadx_legacy_hisr_delete(VCOS_HISR_T *hisr)
{
   hisr->quit = 1;
   vcos_semaphore_post(&hisr->sem);
   vcos_semaphore_wait(&hisr->exitsem);
   vcos_llthread_delete(&hisr->thread);

   vcos_semaphore_delete(&hisr->sem);
   vcos_semaphore_delete(&hisr->exitsem);
   return;
}

int vcos_current_thread_is_fake_hisr_thread(VCOS_HISR_T *hisr)
{
   VCOS_LLTHREAD_T *current = vcos_llthread_current();
   return current == &hisr->thread;
}

int vcos_have_rtos(void)
{
   return 1;
}

VCOS_UNSIGNED vcos_threadx_thread_get_priority(VCOS_THREAD_T *thread)
{
   UINT priority;
   UINT ret = tx_thread_info_get(&thread->thread.thread, 0, 0, 0, &priority, 0, 0, 0, 0);
   /* Undo the mapping of a regular thread priority of <n> to an actual priority of
      1 + VCOS_HISR_PRI_MAX + 1 + <n> */
   vcos_assert(ret == TX_SUCCESS);
   priority -= 1 + VCOS_HISR_PRI_MAX + 1;
   priority &= ~_VCOS_AFFINITY_MASK;

#if VCOS_THREAD_PRI_MIN
   vcos_assert((priority & ~_VCOS_AFFINITY_MASK) >= VCOS_THREAD_PRI_MIN);
#endif
   vcos_assert((priority & ~_VCOS_AFFINITY_MASK) <= VCOS_THREAD_PRI_MAX);
   return priority;
}

void vcos_threadx_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED priority)
{
   UINT oldp;
   UINT ret;
#if VCOS_THREAD_PRI_MIN
   vcos_assert((priority & ~_VCOS_AFFINITY_MASK) >= VCOS_THREAD_PRI_MIN);
#endif
   vcos_assert((priority & ~_VCOS_AFFINITY_MASK) <= VCOS_THREAD_PRI_MAX);
   /* Map a regular thread priority of <n> to an actual priority of 1 + VCOS_HISR_PRI_MAX + 1 + <n> */
   priority += 1 + VCOS_HISR_PRI_MAX + 1;
   ret = tx_thread_priority_change(&thread->thread.thread, priority, &oldp);
   vcos_assert(ret == TX_SUCCESS);
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

