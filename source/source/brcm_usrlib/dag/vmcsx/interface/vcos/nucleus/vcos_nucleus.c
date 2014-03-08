/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Nucleus implementation code
=============================================================================*/

#define VCOS_KEEP_NUCLEUS_NAMES     /* don't #undef NU_Create_XXX; we need them! */
#define VCOS_INLINE_BODIES          /* we wan't the inline bodies here, not just the decls */
#define VCOS_WANT_IMPL              /* we want the explicitly exported functions as regular funcs */
#define VCOS_ASSERT_LOGGING_DISABLE 1

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include "vcfw/logging/logging.h"
#include "vcfw/rtos/nucleus/rtos_nucleus.h"
#include "helpers/sysman/sysman.h"
#include <stdio.h>
#include <string.h>

static void vcos_nucleus_thread_wrapper(UNSIGNED argc, void *argv)
{
   VCOS_THREAD_ENTRY_FN_T fn = (VCOS_THREAD_ENTRY_FN_T)argc;
   fn(argv);
}

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

VCOS_STATUS_T vcos_nu_map_status(STATUS st)
{
   switch (st)
   {
   case NU_SUCCESS: return VCOS_SUCCESS;
   case NU_TIMEOUT: return VCOS_EAGAIN;
   default:
                    return VCOS_EINVAL;
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
   STATUS st;
   vcos_assert(priority >= VCOS_THREAD_PRI_MIN);
   vcos_assert(priority <= VCOS_THREAD_PRI_MAX);
   vcos_assert((affinity & ~NU_AFFINITY_MASK) == 0);
   vcos_assert((stacksz & 3) == 0);
   vcos_assert(timeslice < 256);

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

   st = NU_Create_Task(&thread->thread, (char*)name, vcos_nucleus_thread_wrapper,
                       (UNSIGNED)entry, arg, stack, stacksz, priority | affinity,
                       timeslice, NU_PREEMPT, autostart ? NU_START : NU_NO_START);
   if (st != NU_SUCCESS)
   {
      vcos_release_vpu1(thread->sysmanh);
   }
   return vcos_nu_map_status(st);
}

void vcos_llthread_exit(void)
{
   STATUS st = NU_Terminate_Task(NU_Current_Task_Pointer());
   vcos_assert(st==NU_SUCCESS);
}

void vcos_llthread_delete(VCOS_LLTHREAD_T *thread)
{
   STATUS st = NU_Terminate_Task(&thread->thread);
   vcos_assert(st==NU_SUCCESS);
   st = NU_Delete_Task(&thread->thread);
   vcos_assert(st==NU_SUCCESS);
   if (thread->sysmanh)
   {
      vcos_release_vpu1(thread->sysmanh);
      thread->sysmanh = NULL;
   }
}

/* Ensure vcos_init() only does initialization once, regardless of how many
 * times it gets called.
 */
static RTOS_LATCH_T lock = rtos_latch_unlocked();
static int inited;

VCOS_STATUS_T vcos_init(void)
{
   VCOS_STATUS_T st = VCOS_SUCCESS;

   rtos_latch_get(&lock);

   if (!inited)
   {
      vcos_logging_init();

      st = vcos_tls_init();
      vcos_assert(st == VCOS_SUCCESS);

      if (st == VCOS_SUCCESS)
         st = _vcos_named_semaphore_init();
      vcos_assert(st == VCOS_SUCCESS);

      vcos_msgq_init();

      inited = 1;
   }

   rtos_latch_put(&lock);

   return st;
}

void vcos_deinit(void)
{
   vcos_tls_deinit();
}

void vcos_global_lock(void)
{
   rtos_latch_get(&lock);
}

void vcos_global_unlock(void)
{
   rtos_latch_put(&lock);
}

void vcos_log(const char *fmt, ...)
{
   va_list ap;
   va_start(ap,fmt);
   vlogging_message(LOGGING_VMCS, fmt, ap);
   va_end(ap);
}

void vcos_vlog(const char *fmt, va_list args)
{
   vlogging_message(LOGGING_VMCS, fmt, args);
}

/* For now, logging is done on this platform using the VCFW
 * logging facility, externally.
 */
void vcos_logging_init(void)
{
}


void vcos_nucleus_hisr_activate(VCOS_HISR_T *hisr)
{
   STATUS st = NU_Activate_HISR(hisr);
   vcos_assert(st==NU_SUCCESS);
   if (!NU_In_LISR() && (NU_Current_Task_Pointer() != NULL)) {
      /* Force a reschedule ; Nucleus does not do this for us */
      TCT_System_Protect();
      TCT_Control_To_System();
      TCT_Unprotect();
   }
}

VCOS_STATUS_T vcos_create_shared_stack_hisr( VCOS_HISR_T *hisr_control,
         const char *hisr_name,
         void (*hisr)(void))
{
   STATUS st = rtos_create_shared_stack_hisr(hisr_control, (char*)hisr_name, hisr);
   return (st == NU_SUCCESS ? VCOS_SUCCESS: VCOS_ENOMEM);
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

void vcos_nucleus_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity)
{
#ifdef WANT_SMP
   int release_cpu1 = 0;
   VCOS_UNSIGNED pri = vcos_thread_get_priority(thread);
   pri &= ~_VCOS_AFFINITY_MASK;

   if (affinity & _VCOS_AFFINITY_CPU1)
   {
      /* need to acquire CPU1 if not already acquired */
      if (thread->thread.sysmanh == NULL)
      {
         thread->thread.sysmanh = vcos_acquire_vpu1();
      }
   }
   else
   {
      /* need to release CPU1 if already acquired once the thread has migrated */
      if (thread->thread.sysmanh != NULL)
      {
         release_cpu1 = 1;
      }
   }

   NU_Change_Priority(&thread->thread.thread, pri | affinity);

   if (release_cpu1)
   {
      vcos_release_vpu1(thread->thread.sysmanh);
      thread->thread.sysmanh = NULL;
   }

#else /* Non-SMP case */
   /* Do nothing */
#endif
}

int vcos_llthread_running(VCOS_LLTHREAD_T *thread)
{
   CHAR          name[ 8 ];
   DATA_ELEMENT  task_status;
   UNSIGNED      scheduled_count;
   OPTION        priority;
   OPTION        preempt;
   UNSIGNED      time_slice;
   VOID          *stack_base;
   UNSIGNED      stack_size;
   UNSIGNED      minimum_stack;

   STATUS        ret = NU_Task_Information( &thread->thread,
                                            name,
                                            &task_status,
                                            &scheduled_count,
                                            &priority,
                                            &preempt,
                                            &time_slice,
                                            &stack_base,
                                            &stack_size,
                                            &minimum_stack );

   if ( ( NU_SUCCESS == ret ) &&
        ( ( NU_READY == task_status ) || ( NU_SLEEP_SUSPEND == task_status ) ) )
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

extern VCOS_STATUS_T vcos_nucleus_register_legacy_lisr(VCOS_UNSIGNED vecnum,
                                                       void (*lisr)(VCOS_INT),
                                                       void (**old_lisr)(VCOS_INT))
{

   STATUS st = NU_Register_LISR(vecnum, lisr, old_lisr);
   if (st == NU_SUCCESS)
      return VCOS_SUCCESS;
   else if (st == NU_INVALID_VECTOR)
      return VCOS_EINVAL;
   else if (st == NU_NO_MORE_LISRS)
      return VCOS_ENOSPC;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

int vcos_have_rtos(void)
{
   return 1;
}

