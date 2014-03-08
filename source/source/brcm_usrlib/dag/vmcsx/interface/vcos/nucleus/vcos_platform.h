/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Nucleus types
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */


/** @file
  *
  * Nucleus implementation of OSAL.
  *
  * Since the OSAL API is deliberately intended to be close to the original
  * Nucleus API (to avoid massive rework) there is no benefit to be had by
  * using a minimal set of APIs.
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#define VCOS_HAVE_RTOS        1
#define VCOS_HAVE_SEMAPHORE   1
#define VCOS_HAVE_TIMER       1
#define VCOS_HAVE_LEGACY_ISR  1
#define VCOS_HAVE_MEMPOOL     1
#define VCOS_HAVE_QUEUE       1
#define VCOS_HAVE_EVENT       1
#define VCOS_HAVE_ISR         1
#define VCOS_LISRS_NEED_HISRS 1

/**< The legacy Nucleus types CHAR, UNSIGNED, etc are defined, and polluting the global namespace */
#define VCOS_HAVE_TYPE_VOID
#define VCOS_HAVE_TYPE_CHAR
#define VCOS_HAVE_TYPE_UNSIGNED_CHAR
#define VCOS_HAVE_TYPE_UNSIGNED
#define VCOS_HAVE_TYPE_SIGNED
#define VCOS_HAVE_TYPE_UINT8
#define VCOS_HAVE_TYPE_INT8
#define VCOS_HAVE_TYPE_UINT16
#define VCOS_HAVE_TYPE_INT16
#define VCOS_HAVE_TYPE_UINT32
#define VCOS_HAVE_TYPE_INT32
#define VCOS_HAVE_TYPE_UINT64
#define VCOS_HAVE_TYPE_INT64
#define VCOS_HAVE_TYPE_BOOLEAN

/* Nucleus does not have TLS */
#define VCOS_WANT_TLS_EMULATION

#define COMPILING_IN_VCOS
#include "vcfw/rtos/nucleus/rtos_nucleus.h"
#include "nucleus.h"
#undef COMPILING_IN_VCOS

#ifndef NU_Get_TLS_Pointer
#error Old version of Nucleus API
#endif

#if defined(VCOS_INLINE_BODIES)
#include "vcfw/rtos/rtos.h" /* for rtos_getmicrosecs */
#include <string.h>         /* for strcasecmp */
#endif

typedef NU_SEMAPHORE          VCOS_SEMAPHORE_T;
typedef NU_EVENT_GROUP        VCOS_EVENT_FLAGS_T;
typedef NU_QUEUE              VCOS_QUEUE_T;
typedef NU_HISR               VCOS_HISR_T;
typedef NU_MEMORY_POOL        VCOS_MEMPOOL_T;

typedef struct VCOS_LLTHREAD_T
{
   NU_TASK thread;
   struct SYSMAN_HANDLE_S *sysmanh;
} VCOS_LLTHREAD_T;

typedef struct {
   NU_TIMER timer;
   void (*fn)(void *);
   void *context;
} VCOS_TIMER_T;

typedef UNSIGNED              VCOS_UNSIGNED;
typedef OPTION                VCOS_OPTION;
typedef INT                   VCOS_INT;
typedef UNSIGNED_CHAR         VCOS_BYTE;

typedef void (*VCOS_ISR_HANDLER_T)(VCOS_UNSIGNED vecnum);

#define VCOS_SUSPEND       NU_SUSPEND
#define VCOS_NO_SUSPEND    NU_NO_SUSPEND

#define VCOS_OR            NU_OR
#define VCOS_AND           NU_AND
#define VCOS_AND_CONSUME   NU_AND_CONSUME
#define VCOS_OR_CONSUME    NU_OR_CONSUME

#define VCOS_OR_CONSUME    NU_OR_CONSUME

#define VCOS_START         1
#define VCOS_NO_START      0

#define VCOS_TICKS_PER_SECOND NU_PLUS_Ticks_Per_Second

#define VCOS_PREEMPT       NU_PREEMPT
#define VCOS_NO_PREEMPT    NU_NO_PREEMPT

/* We don't need a thread timer on this platform */
#define _VCOS_THREAD_TIMER_UNUSED

// Task priorities
#define VCOS_THREAD_PRI_MIN   1
#define VCOS_THREAD_PRI_MAX   15
#define VCOS_THREAD_PRI_NORMAL         10
#define VCOS_THREAD_PRI_BELOW_NORMAL   12
#define VCOS_THREAD_PRI_ABOVE_NORMAL   8
#define VCOS_THREAD_PRI_HIGHEST        VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_LOWEST         VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_INCREASE       -1

#ifdef WANT_SMP
#define _VCOS_AFFINITY_CPU0 NU_AFFINITY_CPU0
#define _VCOS_AFFINITY_CPU1 NU_AFFINITY_CPU1
#define _VCOS_AFFINITY_MASK NU_AFFINITY_MASK
#define _VCOS_AFFINITY_THISCPU NU_AFFINITY_THISCPU
#else
#define _VCOS_AFFINITY_CPU0 0
#define _VCOS_AFFINITY_CPU1 0
#define _VCOS_AFFINITY_MASK 0
#define _VCOS_AFFINITY_THISCPU 0
#endif
#define _VCOS_AFFINITY_DEFAULT 0

#define _vcos_tls_thread_ptr_get() NU_Get_TLS_Pointer()
#define _vcos_tls_thread_ptr_set(p) NU_Set_TLS_Pointer(p)

#ifdef __VIDEOCORE__
#include "vcfw/vcos/vcos_rtos_latch.h"
#include "vcfw/vcos/vcos_rtos_latch_event.h"
#else
#include "interface/vcos/generic/vcos_latch_from_sem.h"
#endif

#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"
#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"

/* System startup */

/** On Nucleus and ThreadX, the OS calls into this function to
  * start things going. It gets passed a single argument which
  * is the start of usable memory.
  */
#define VCOS_APPLICATION_INITIALIZE Application_Initialize

#define VCOS_APPLICATION_ARGC          rtos_argc
#define VCOS_APPLICATION_ARGV          rtos_argv
#define VCOS_CAN_SET_STACK_ADDR  1

/** The Nucleus HISR implementation requires a separate stack for each
  * HISR. Callers must allocate it and pass it in.
  */
#define VCOS_LEGACY_HISR_NEEDS_STACK   1

/** The priority of HISRs created via the old (un)shared stack HISR scheme
  */
#define VCOS_LEGACY_HISR_PRI           1

/** The size of the stack created via the old (un)shared stack HISR scheme */
#define VCOS_LEGACY_HISR_STACK_SIZE    1024

extern VCOS_STATUS_T vcos_nu_map_status(STATUS st);
extern void vcos_nucleus_hisr_activate(VCOS_HISR_T *hisr);
extern void vcos_nucleus_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity);
extern VCOS_STATUS_T vcos_nucleus_register_legacy_lisr(VCOS_UNSIGNED vecnum,
                                                       void (*lisr)(VCOS_INT),
                                                       void (**old_lisr)(VCOS_INT));

#if defined(VCOS_INLINE_BODIES)

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1

/***********************************************************
 *
 * Event groups - optional.
 *
 * Event groups can be fabricated from a semaphore and a uint32_t. However, since Nucleus
 * has a event groups already (along with ThreadX, probably the only ones that do) there
 * is little point in doing this.
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_create(VCOS_EVENT_FLAGS_T *flags, const char *name) {
   STATUS st = NU_Create_Event_Group(flags, name ? (CHAR*)name:"");
   return vcos_nu_map_status(st);
}

VCOS_INLINE_IMPL
void vcos_event_flags_delete(VCOS_EVENT_FLAGS_T *flags) {
   STATUS st = NU_Delete_Event_Group(flags);
   vcos_assert(st==NU_SUCCESS);
}

VCOS_EXPORT
void vcos_event_flags_set(VCOS_EVENT_FLAGS_T *flags,
                                                       VCOS_UNSIGNED events,
                                                       VCOS_OPTION op) {
   STATUS st = NU_Set_Events(flags, events, op);
   vcos_assert(st==NU_SUCCESS);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_get(VCOS_EVENT_FLAGS_T *flags,
                                                           VCOS_UNSIGNED requested_events,
                                                           VCOS_OPTION op,
                                                           VCOS_UNSIGNED ms_suspend,
                                                           VCOS_UNSIGNED *retrieved_events) {
   if (ms_suspend != 0 && ms_suspend != VCOS_SUSPEND)
      ms_suspend = _VCOS_MS_TO_TICKS(ms_suspend);
   STATUS st = NU_Retrieve_Events(flags, requested_events, op, retrieved_events,
                                  ms_suspend);
   if (st == NU_SUCCESS) return VCOS_SUCCESS;
   else if (st == NU_TIMEOUT) return VCOS_EAGAIN;
   else if (st == NU_NOT_PRESENT) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}


/***********************************************************
 *
 * Semaphores
 *
 ***********************************************************/

VCOS_INLINE_IMPL VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   STATUS st = NU_Obtain_Semaphore(sem, NU_SUSPEND);
   vcos_assert(st==NU_SUCCESS);
   return VCOS_SUCCESS; /* Should be void */
}


VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait_timeout(VCOS_SEMAPHORE_T *sem, VCOS_UNSIGNED timeout) {
   STATUS st = NU_Obtain_Semaphore(sem, timeout);
   if (st == NU_TIMEOUT)
      return VCOS_EAGAIN;
   else if (st == NU_SUCCESS)
      return VCOS_SUCCESS;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   STATUS st = NU_Obtain_Semaphore(sem, NU_NO_SUSPEND);
   if (st == NU_SUCCESS) return VCOS_SUCCESS;
   else if (st == NU_UNAVAILABLE) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   STATUS st = NU_Release_Semaphore(sem);
   vcos_assert(st==NU_SUCCESS);
   return VCOS_SUCCESS; /* Should be void */
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                                        const char *name,
                                                        VCOS_UNSIGNED initial_count) {
   STATUS st;
   st = NU_Create_Semaphore(sem, name ? (CHAR*)name:"", initial_count, NU_FIFO);
   return vcos_nu_map_status(st);
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   STATUS st = NU_Delete_Semaphore(sem);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   CHAR name[8];
   DATA_ELEMENT st, pri;
   UNSIGNED sched, ts, sz, minstack;
   OPTION preempt;
   void *base;
   STATUS ret = NU_Task_Information(&thread->thread.thread, name, &st, &sched, &pri, &preempt,
                                   &ts, &base, &sz, &minstack);
   vcos_assert(ret == NU_SUCCESS);
   return pri;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
   UNSIGNED aff;
#ifdef WANT_SMP
   DATA_ELEMENT cpu, homecpu;
   STATUS st = NU_Task_Affinity_Information(&thread->thread.thread, &aff, &cpu, &homecpu);
   vcos_assert(st == NU_SUCCESS);
#else /* Non-SMP case */
   aff = _VCOS_AFFINITY_CPU0;
#endif
   return aff;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
   vcos_nucleus_thread_set_affinity(thread, affinity);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_change_preemption(VCOS_UNSIGNED pe) {
   return NU_Change_Preemption(pe);
}

VCOS_EXPORT
void vcos_sleep(uint32_t ms) {
   NU_Sleep(_VCOS_MS_TO_TICKS(ms));
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return NU_In_LISR() || NU_Current_HISR_Pointer();
}

VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   return (VCOS_LLTHREAD_T*)NU_Current_Task_Pointer();
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED pri) {
   NU_Change_Priority(&thread->thread.thread, pri);
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) {
   NU_Relinquish();
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(VCOS_LLTHREAD_T *thread) {
   NU_Resume_Task(&thread->thread);
}

#define vcos_llthread_counted_suspend(thread) do {             \
   TCT_Protect( &TCD_System_Protect ); /* system protection */ \
   TCC_Suspend_Task( &thread->thread, NU_PURE_SUSPEND, 0, 0, NU_SUSPEND ); \
   TCT_Unprotect(); } while (0)

#define vcos_llthread_counted_resume(thread) do {              \
   NU_Resume_Task(&thread->thread);                            \
} while (0)

VCOS_INLINE_IMPL
void vcos_register_isr(VCOS_UNSIGNED vec,
                       VCOS_ISR_HANDLER_T handler,
                       VCOS_ISR_HANDLER_T *old_handler) {
   int ret = rtos_register_lisr(vec, (RTOS_LISR_T)handler, (RTOS_LISR_T*)old_handler);
   vcos_assert(ret == 0);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_int_disable(void) {
   return NU_Local_Control_Interrupts(NU_DISABLE_INTERRUPTS);
}

VCOS_INLINE_IMPL
void vcos_int_restore(VCOS_UNSIGNED previous) {
   NU_Local_Control_Interrupts(previous);
}

/***********************************************************
 *
 * Fixed size message queues
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_create(VCOS_QUEUE_T *queue,
                                const char *name,
                                VCOS_UNSIGNED message_size,
                                void *queue_start,
                                VCOS_UNSIGNED queue_size) {
   STATUS st;

   // Nucleus message & queue size params are in a different order to threadx equivalent
   // params.
   vcos_assert(message_size <= queue_size);

   st = NU_Create_Queue(queue, name ? (char*)name:"", queue_start, queue_size,
                        NU_FIXED_SIZE, message_size, NU_FIFO);
   return vcos_nu_map_status(st);
}

VCOS_INLINE_IMPL
void vcos_queue_delete(VCOS_QUEUE_T *queue) {
   STATUS st = NU_Delete_Queue(queue);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_send(VCOS_QUEUE_T *queue, const void *src, VCOS_UNSIGNED wait) {
   STATUS st;
   vcos_assert(queue->qu_fixed_size);
   st = NU_Send_To_Queue(queue, (void*)src, queue->qu_message_size, wait);
   if (st == NU_SUCCESS) return VCOS_SUCCESS;
   else if (st == NU_QUEUE_FULL) return VCOS_ENOSPC;
   else if (st == NU_TIMEOUT) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_receive(VCOS_QUEUE_T *queue, void *dst, VCOS_UNSIGNED wait) {
   STATUS st;
   UNSIGNED actual;
   vcos_assert(queue->qu_fixed_size);
   st = NU_Receive_From_Queue(queue, dst, queue->qu_message_size, &actual, wait);
   if (st == NU_SUCCESS) return VCOS_SUCCESS;
   else if (st == NU_QUEUE_EMPTY) return VCOS_EAGAIN;
   else if (st == NU_TIMEOUT) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

/***********************************************************
 *
 * Legacy interrupt handling
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_register_legacy_lisr(VCOS_UNSIGNED vecnum,
                                                          void (*lisr)(VCOS_INT),
                                                          void (**old_lisr)(VCOS_INT)) {
   return vcos_nucleus_register_legacy_lisr(vecnum, lisr, old_lisr);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_legacy_hisr_create(VCOS_HISR_T *hisr, const char *name,
                                      void (*entry)(void), VCOS_UNSIGNED pri,
                                      void *stack, VCOS_UNSIGNED stack_size) {
   STATUS st = NU_Create_HISR(hisr, name?(CHAR*)name:"", entry, pri, stack, stack_size);
   return vcos_nu_map_status(st);
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_activate(VCOS_HISR_T *hisr) {
   vcos_nucleus_hisr_activate(hisr);
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_delete(VCOS_HISR_T *hisr) {
   STATUS st = NU_Delete_HISR(hisr);
   vcos_assert(st==NU_SUCCESS);
}

VCOS_INLINE_IMPL
int vcos_in_legacy_lisr(void) {
   return NU_In_LISR();
}

/***********************************************************
 *
 * Timers
 *
 ***********************************************************/

typedef void (*_nu_expiry_fn)(UNSIGNED);

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_timer_create(VCOS_TIMER_T *timer,
                                const char *name,
                                void (*expiration_routine)(void *context),
                                void *context) {
   STATUS st;
   vcos_assert(sizeof(void*) == sizeof(UNSIGNED));
   timer->fn = expiration_routine;
   timer->context = context;

   st = NU_Create_Timer(&timer->timer,
                        name ? (CHAR*)name:"",
                        (_nu_expiry_fn)(expiration_routine),
                        (UNSIGNED)context,
                        1, 0, NU_DISABLE_TIMER);

   return vcos_nu_map_status(st);
}

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   STATUS st = NU_Reset_Timer(&timer->timer,
                              (_nu_expiry_fn)timer->fn,
                              _VCOS_MS_TO_TICKS(delay_ms), 0, NU_ENABLE_TIMER);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   STATUS st = NU_Control_Timer(&timer->timer, NU_DISABLE_TIMER);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay_ms);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   STATUS st = NU_Delete_Timer(&timer->timer);
   vcos_assert(st == NU_SUCCESS);
}

/***********************************************************
 *
 * Memory
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mempool_create(VCOS_MEMPOOL_T *pool,
                                  const char *name,
                                  void *start,
                                  VCOS_UNSIGNED size) {
   STATUS st = NU_Create_Memory_Pool(pool, name?(CHAR*)name:"", start, size, 0, NU_FIFO);
   vcos_assert(st == NU_SUCCESS);
   return VCOS_SUCCESS;
}


VCOS_INLINE_IMPL
void *vcos_mempool_alloc(VCOS_MEMPOOL_T *pool, VCOS_UNSIGNED len) {
   void *ret;
   STATUS st = NU_Allocate_Memory(pool, &ret, len, NU_NO_SUSPEND);
   return st == NU_SUCCESS? ret : 0;
}

VCOS_INLINE_IMPL
void vcos_mempool_free(VCOS_MEMPOOL_T *pool, void *mem) {
   STATUS st = NU_Deallocate_Memory(mem);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_mempool_delete(VCOS_MEMPOOL_T *pool) {
   STATUS st = NU_Delete_Memory_Pool(pool);
   vcos_assert(st == NU_SUCCESS);
}

VCOS_INLINE_IMPL
uint32_t vcos_getmicrosecs(void) {
   return rtos_getmicrosecs();
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
   return 0;
}

VCOS_INLINE_IMPL
int vcos_strcasecmp(const char *s1, const char *s2) {
   return strcasecmp(s1,s2);
}

VCOS_INLINE_IMPL
int vcos_strncasecmp(const char *s1, const char *s2, size_t n) {
   return strncasecmp(s1,s2,n);
}

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 0

#endif /* VCOS_INLINE_BODIES */

/*
 * Under VideoCore we always rtos.h for memory allocation
 */
#include "vcfw/vcos/vcos_malloc.h"

#include "interface/vcos/generic/vcos_common.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"

#if !defined(ALLOW_DIRECT_NUCLEUS_CALLS_AS_TEMPORARY_HACK) && !defined(VCOS_KEEP_NUCLEUS_NAMES)
#undef NU_Set_Events
#undef NU_Release_Semaphore
#undef NU_Create_Task
#undef NU_Create_Semaphore
#undef NU_Create_Event_Group
#undef NU_Create_HISR
#undef NU_Create_Timer
#undef NU_Create_Queue
#endif

#ifdef __cplusplus
}
#endif

#endif /* VCOS_PLATFORM_H */

