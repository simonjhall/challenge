/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  osal
File     :  $RCSfile: $
Revision :  $Revision$

FILE DESCRIPTION
VideoCore OS Abstraction Layer - ThreadX types
=============================================================================*/

// Do not include this file directly - instead include it via vcos.h

/** @file
  *
  * Threadx implementation of OSAL.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#define VCOS_USE_SPINLOCK_SEMAPHORES
#define VCOS_HAVE_RTOS          1
#define VCOS_HAVE_SEMAPHORE     1
#define VCOS_HAVE_TIMER         1
#define VCOS_HAVE_ISR           1
#define VCOS_HAVE_LEGACY_ISR    1
#define VCOS_HAVE_QUEUE         1
#define VCOS_HAVE_EVENT         1
#define VCOS_HAVE_DLFCN         1
#define VCOS_HAVE_ATOMIC_FLAGS  1

/**< The legacy Nucleus types CHAR, UNSIGNED, etc are defined, and polluting the global namespace */
#define VCOS_HAVE_TYPE_VOID
#define VCOS_HAVE_TYPE_CHAR
#define VCOS_HAVE_TYPE_UCHAR
#define VCOS_HAVE_TYPE_UINT
#define VCOS_HAVE_TYPE_INT
#define VCOS_HAVE_TYPE_LONG
#define VCOS_HAVE_TYPE_ULONG
#define VCOS_HAVE_TYPE_SHORT
#define VCOS_HAVE_TYPE_USHORT

#define VCOS_WANT_TLS_EMULATION
#define VCOS_SO_EXT  ".vll"

#include "thirdparty/vcfw/rtos/threadx/kernel/tx_api.h"
#include "thirdparty/vcfw/rtos/threadx/kernel/tx_thread.h"
#include "thirdparty/vcfw/rtos/threadx/kernel/tx_event_flags.h"

#if defined(VCOS_INLINE_BODIES)
#include "vcfw/rtos/rtos.h"
#endif

#define VCOS_BLOCKING_UNSAFE tx_blocking_unsafe

typedef TX_EVENT_FLAGS_GROUP  VCOS_EVENT_FLAGS_T;
typedef TX_QUEUE              VCOS_QUEUE_T;
typedef TX_BYTE_POOL          VCOS_MEMPOOL_T;
typedef TX_TIMER              VCOS_TIMER_T;

typedef struct VCOS_LLTHREAD_T
{
   TX_THREAD thread;
   char name[16];
   struct SYSMAN_HANDLE_S *sysmanh;
} VCOS_LLTHREAD_T;

typedef ULONG                 VCOS_UNSIGNED;
typedef ULONG                 VCOS_OPTION;
typedef INT                   VCOS_INT;

#ifdef VCOS_USE_SPINLOCK_SEMAPHORES
#include "vcfw/vcos/vcos_spinlock_semaphore.h"
#else
typedef TX_SEMAPHORE          VCOS_SEMAPHORE_T;
#endif /* VCOS_USE_SPINLOCK_SEMAPHORES */

#include "vcfw/vcos/vcos_rtos_latch.h"

typedef struct vcos_opaque_quickslow_mutex_t* VCOS_QUICKSLOW_MUTEX_T;


typedef struct {
   void (*entry)(void);
   int quit;
   uint8_t *stack;
   unsigned activations;
   unsigned calls;
   VCOS_SEMAPHORE_T sem;
   VCOS_SEMAPHORE_T exitsem;
   VCOS_LLTHREAD_T thread;
} VCOS_HISR_T;

/* Event flag operations */
#define VCOS_OR            TX_OR
#define VCOS_AND           TX_EVENT_FLAGS_AND_MASK
#define VCOS_AND_CONSUME   (TX_EVENT_FLAGS_AND_MASK | TX_EVENT_FLAGS_CLEAR_MASK)
#define VCOS_OR_CONSUME    TX_OR_CLEAR

#define VCOS_SUSPEND       TX_WAIT_FOREVER
#define VCOS_NO_SUSPEND    0

#define VCOS_NO_START      0
#define VCOS_START         TX_AUTO_START

#define VCOS_TICKS_PER_SECOND 100

#define VCOS_PREEMPT       0xff
#define VCOS_NO_PREEMPT    0

/* We don't need a thread timer on this platform */
#define _VCOS_THREAD_TIMER_UNUSED

/*
 * CPU affinity
 */
#define _VCOS_AFFINITY_CPU0 TX_AFFINITY_CPU0
#define _VCOS_AFFINITY_CPU1 TX_AFFINITY_CPU1
#define _VCOS_AFFINITY_MASK TX_AFFINITY_MASK
#define _VCOS_AFFINITY_DEFAULT TX_AFFINITY_THISCPU
#define _VCOS_AFFINITY_THISCPU TX_AFFINITY_THISCPU

/*
 * Thread priority
 */
#define VCOS_THREAD_PRI_MIN      0
#define VCOS_THREAD_PRI_MAX_REAL 31
#define VCOS_HISR_PRI_MIN        0
#define VCOS_HISR_PRI_MAX        2
/* Reserve priority zero for system timers */
#define VCOS_THREAD_PRI_MAX      (VCOS_THREAD_PRI_MAX_REAL - (VCOS_HISR_PRI_MAX + 1) - 1)

#define VCOS_THREAD_PRI_NORMAL         10
#define VCOS_THREAD_PRI_BELOW_NORMAL   12
#define VCOS_THREAD_PRI_ABOVE_NORMAL   8
#define VCOS_THREAD_PRI_HIGHEST        VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_LOWEST         VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_INCREASE       -1

#define VCOS_APPLICATION_INITIALIZE tx_application_define
#define VCOS_CAN_SET_STACK_ADDR  1

#define VCOS_APPLICATION_ARGC          rtos_argc
#define VCOS_APPLICATION_ARGV          rtos_argv

#define _vcos_tls_thread_ptr_get() tx_thread_tls_ptr_get()
#define _vcos_tls_thread_ptr_set(p) tx_thread_tls_ptr_set(p)

#define VCOS_LEGACY_HISR_NEEDS_STACK   1

/** The priority of HISRs created via the old (un)shared stack HISR scheme
  */
#define VCOS_LEGACY_HISR_PRI           1

/** The size of the stack needed to emulate the old legacy HISR scheme on ThreadX.
  */
#define VCOS_LEGACY_HISR_STACK_SIZE    512

/*
 * This nasty code is here to be able to atomically unlock and suspend. It turns
 * out that the more obvious ways of doing this end up deadlocking sometimes.
 *
 */
#define _VCOS_OS_LOCK_STATE               TX_INTERRUPT_SAVE_AREA
#define _VCOS_OS_DISABLE()                TX_DISABLE
#define _VCOS_OS_RESTORE()                TX_RESTORE

#define _VCOS_OS_RESTORE_AND_SUSPEND()    do {                             \
   TX_THREAD *cur = tx_thread_identify();                                  \
   cur -> tx_thread_state =    TX_SUSPENDED;                               \
   cur -> tx_thread_suspending =  TX_TRUE;                                 \
   cur -> tx_thread_timer.tx_timer_internal_remaining_ticks =  0;          \
   TX_PREEMPT_DISABLE                                                      \
   TX_RESTORE                                                              \
   _tx_thread_system_suspend(cur);                                         \
} while (0)

typedef void (*TX_ENTRY_FN)(ULONG);


/** C function to register as an IRQ. Called in interrupt context obviously.
  */
typedef void (*VCOS_ISR_HANDLER_T)(VCOS_UNSIGNED vecnum);

VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_threadx_map_error(UINT e);

/* Do not include this file directly - instead include it via vcos.h */

#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"
#include "interface/vcos/generic/vcos_thread_reaper.h"

#ifdef __VIDEOCORE__
#include "vcfw/vcos/vcos_rtos_latch_event.h"
#endif

#include "vcfw/vcos/vcos_spinlock_atomic_flags.h"

#if defined(VCOS_INLINE_BODIES)

VCOSPRE_ void VCOSPOST_ vcos_threadx_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity);
VCOSPRE_ VCOS_UNSIGNED VCOSPOST_ vcos_threadx_thread_get_priority(VCOS_THREAD_T *thread);
VCOSPRE_ void VCOSPOST_ vcos_threadx_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED pri);

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1

/***********************************************************
 *
 * Event groups
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_create(VCOS_EVENT_FLAGS_T *flags, const char *name) {
   UINT st = tx_event_flags_create(flags, (CHAR*)name);
   return vcos_threadx_map_error(st);
}

VCOS_INLINE_IMPL
void vcos_event_flags_delete(VCOS_EVENT_FLAGS_T *flags) {
   UINT st = tx_event_flags_delete(flags);
   vcos_assert(st == VCOS_SUCCESS);
}

VCOS_EXPORT
void vcos_event_flags_set(VCOS_EVENT_FLAGS_T *flags,
                                            VCOS_UNSIGNED events,
                                            VCOS_OPTION op) {
   UINT st = tx_event_flags_set(flags, events, op);
   vcos_assert(st==TX_SUCCESS);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_get(VCOS_EVENT_FLAGS_T *flags,
                                                     VCOS_UNSIGNED requested_events,
                                                     VCOS_OPTION op,
                                                     VCOS_UNSIGNED ms_suspend,
                                                     VCOS_UNSIGNED *retrieved_events) {
   if (ms_suspend != 0 && ms_suspend != VCOS_SUSPEND)
      ms_suspend = _VCOS_MS_TO_TICKS(ms_suspend);
   UINT ret = tx_event_flags_get(flags, requested_events, op, retrieved_events, ms_suspend);
   if (ret == TX_NO_EVENTS) {
      *retrieved_events = 0;
       return VCOS_EAGAIN;
   }
   else if (ret == TX_SUCCESS) return VCOS_SUCCESS;
   else {
      vcos_assert(0);
      *retrieved_events = 0;
      return VCOS_EINVAL;
   }
}


/***********************************************************
 *
 * Semaphores
 *
 ***********************************************************/

#ifndef VCOS_USE_SPINLOCK_SEMAPHORES

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   UINT ret = tx_semaphore_get(sem, TX_WAIT_FOREVER);
   vcos_assert(ret == TX_SUCCESS);
   return VCOS_SUCCESS; /* FIXME: should be void */
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait_timeout(VCOS_SEMAPHORE_T *sem, VCOS_UNSIGNED timeout) {
   UINT ret = tx_semaphore_get(sem, timeout);
   if (ret == TX_NO_INSTANCE)
      return VCOS_EAGAIN;
   else if (ret == TX_SUCCESS)
      return VCOS_SUCCESS;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   UINT ret = tx_semaphore_get(sem, TX_NO_WAIT);
   if (ret == TX_NO_INSTANCE) return VCOS_EAGAIN;
   else if (ret == TX_SUCCESS) return VCOS_SUCCESS;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   UINT ret = tx_semaphore_put(sem);
   vcos_assert(ret == TX_SUCCESS);
   return VCOS_SUCCESS; /* FIXME: should be void */
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                                      const char *name,
                                                      VCOS_UNSIGNED initial_count) {
   UINT ret = tx_semaphore_create(sem, (CHAR*)name, initial_count);
   return vcos_threadx_map_error(ret);
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   UINT ret = tx_semaphore_delete(sem);
   vcos_assert(ret == TX_SUCCESS);
}

#endif /* VCOS_USE_SPINLOCK_SEMAPHORES */

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   return vcos_threadx_thread_get_priority(thread);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
   UINT aff;
#ifdef WANT_SMP
   UINT ret = tx_thread_info_get(&thread->thread.thread, 0, 0, 0, &aff, 0, 0, 0, 0);
   vcos_assert(ret == TX_SUCCESS);
   aff &= _VCOS_AFFINITY_MASK;
#else // Non-SMP case
   aff = _VCOS_AFFINITY_CPU0;
#endif
   return aff;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
   vcos_threadx_thread_set_affinity(thread, affinity);
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
#ifdef __VIDEOCORE__
   rtos_delay(ms*1000, 1 /* suspend */);
#else
   tx_thread_sleep(_VCOS_MS_TO_TICKS(ms));
#endif
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return tx_in_interrupt();
}

VCOS_INLINE_IMPL
int vcos_in_legacy_lisr(void) {
   return tx_in_interrupt();
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED priority) {
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

VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   return (VCOS_LLTHREAD_T*)tx_thread_identify();
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) {
   tx_thread_relinquish();
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_change_preemption(VCOS_UNSIGNED pe) {
   UINT old_thresh;
   tx_thread_preemption_change(tx_thread_identify(), pe, &old_thresh);
   return old_thresh;
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(VCOS_LLTHREAD_T *thread) {
   tx_thread_resume(&thread->thread);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_int_disable(void) {
   return tx_interrupt_control(TX_INT_DISABLE);
}

VCOS_INLINE_IMPL
void vcos_int_restore(VCOS_UNSIGNED previous) {
   tx_interrupt_control(previous);
}

/***********************************************************
 *
 * Mutexes
 *
 ***********************************************************/

/* Use rtos_latches - ThreadX is fast, but these are faster. */

/***********************************************************
 *
 * Quick/Slow Mutexes
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_quickslow_mutex_create(VCOS_QUICKSLOW_MUTEX_T *m, const char *name)
{
   *m = (VCOS_QUICKSLOW_MUTEX_T)0;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_delete(VCOS_QUICKSLOW_MUTEX_T *m)
{
   /* Nothing to do */
}

extern void vcos_quickslow_mutex_lock_impl(VCOS_QUICKSLOW_MUTEX_T *m);
extern void vcos_quickslow_mutex_wait_impl( VCOS_QUICKSLOW_MUTEX_T *m );

VCOS_INLINE_DECL
void vcos_quickslow_mutex_internal_spin_lock(VCOS_QUICKSLOW_MUTEX_T *m);

VCOS_INLINE_DECL
void vcos_quickslow_mutex_lock(VCOS_QUICKSLOW_MUTEX_T *m)
{
   vcos_quickslow_mutex_lock_impl(m);
}

extern void vcos_quickslow_mutex_unlock_impl(VCOS_QUICKSLOW_MUTEX_T *m);

VCOS_INLINE_DECL
void vcos_quickslow_mutex_unlock(VCOS_QUICKSLOW_MUTEX_T *m)
{
   vcos_assert ( NULL != m );

   vcos_quickslow_mutex_internal_spin_lock(m);
   vcos_quickslow_mutex_unlock_impl(m);
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_internal_spin_lock(VCOS_QUICKSLOW_MUTEX_T *m)
{
   unsigned int sync;
   vcos_assert(!tx_in_interrupt());
   _di();
   do {
      sync = _vasm( "mov %D, %p17" );
   } while( sync );
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_lock_quick(VCOS_QUICKSLOW_MUTEX_T *m)
{
   vcos_quickslow_mutex_internal_spin_lock(m);

   if ((VCOS_QUICKSLOW_MUTEX_T)0 != *m)
   {
      //wait for the mutex
      vcos_quickslow_mutex_wait_impl( m );
      vcos_quickslow_mutex_internal_spin_lock(m);
   }
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_internal_spin_unlock(VCOS_QUICKSLOW_MUTEX_T *m)
{
   unsigned int temp = 0;
   _vasm( "mov %p17, %r", temp );
   _ei();
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_unlock_quick(VCOS_QUICKSLOW_MUTEX_T *m)
{
   if ((VCOS_QUICKSLOW_MUTEX_T)0 != *m)
   {
      vcos_quickslow_mutex_unlock_impl(m);
      return;
   }
   vcos_quickslow_mutex_internal_spin_unlock(m);
}

/***********************************************************
 *
 * Fixed size queues
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_create(VCOS_QUEUE_T *queue,
                                const char *name,
                                VCOS_UNSIGNED message_size,
                                void *queue_start,
                                VCOS_UNSIGNED queue_size) {
   UINT ret = tx_queue_create(queue, (char*)name, message_size, queue_start, queue_size * 4);
   return vcos_threadx_map_error(ret);
}

VCOS_INLINE_IMPL
void vcos_queue_delete(VCOS_QUEUE_T *queue) {
   UINT ret = tx_queue_delete(queue);
   vcos_assert(ret == TX_SUCCESS); // should always succeed unless semaphore corrupt
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_send(VCOS_QUEUE_T *queue, const void *src, VCOS_UNSIGNED wait) {
   UINT ret = tx_queue_send(queue, (void*)src, wait);
   if (ret == TX_SUCCESS) return VCOS_SUCCESS;
   else if (ret == TX_QUEUE_FULL) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}


VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_receive(VCOS_QUEUE_T *queue, void *dst, VCOS_UNSIGNED wait) {
   UINT ret = tx_queue_receive(queue, dst, wait);
   if (ret == TX_SUCCESS) return VCOS_SUCCESS;
   else if (ret == TX_QUEUE_EMPTY) return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

/***********************************************************
 *
 * Timers
 *
 ***********************************************************/

typedef void (*_tx_expiry_fn)(ULONG);

/** Create a timer.
  *
  * Note that we just cast the expiry function - this assumes that UNSIGNED
  * and VOID* are the same size.
  */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_timer_create(VCOS_TIMER_T *timer,
                                const char *name,
                                void (*expiration_routine)(void *context),
                                void *context) {
   UINT st;
   vcos_assert(sizeof(void*) == sizeof(ULONG));

   st = tx_timer_create(timer,
                        (CHAR*)name,
                        (_tx_expiry_fn)(expiration_routine),
                        (ULONG)context,
                        1, 0, 0);

   return vcos_threadx_map_error(st);
}

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   UINT st = tx_timer_change(timer, _VCOS_MS_TO_TICKS(delay_ms), 0);
   vcos_assert(st==TX_SUCCESS);
   st = tx_timer_activate(timer);
   vcos_assert(st==TX_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   UINT st = tx_timer_deactivate(timer);
   vcos_assert(st == TX_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   UINT st = tx_timer_delete(timer);
   vcos_assert(st == TX_SUCCESS);
}

/***********************************************************
 *
 * Legacy interrupt handling
 *
 ***********************************************************/

VCOSPRE_ int VCOSPOST_ vcos_current_thread_is_fake_hisr_thread(VCOS_HISR_T *);

VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_threadx_legacy_hisr_create(
   VCOS_HISR_T *, const char *,
   void (*entry)(void), VCOS_UNSIGNED pri,
   void *stack, VCOS_UNSIGNED stack_size);
VCOSPRE_ void VCOSPOST_ vcos_threadx_legacy_hisr_delete(VCOS_HISR_T *hisr);

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_register_legacy_lisr(VCOS_UNSIGNED vecnum,
                                        void (*lisr)(VCOS_INT),
                                        void (**old_lisr)(VCOS_INT)) {
   int32_t st = rtos_register_lisr(vecnum, (RTOS_LISR_T)lisr, (RTOS_LISR_T*)old_lisr);
   switch (st) {
   case 0 : return VCOS_SUCCESS;
   case -1: return VCOS_EACCESS;
   case -2: return VCOS_EINVAL;
   default: vcos_assert(0); return VCOS_EINVAL;
   }
}


VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_legacy_hisr_create(VCOS_HISR_T *hisr, const char *name,
                                      void (*entry)(void), VCOS_UNSIGNED pri,
                                      void *stack, VCOS_UNSIGNED stack_size) {
   return vcos_threadx_legacy_hisr_create(hisr, name, entry, pri, stack, stack_size);
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_activate(VCOS_HISR_T *hisr) {
   hisr->activations++;
   vcos_semaphore_post(&hisr->sem);
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_delete(VCOS_HISR_T *hisr) {
   vcos_threadx_legacy_hisr_delete(hisr);
}

/***********************************************************
 *
 * Interrupt handling
 *
 ***********************************************************/
VCOS_INLINE_IMPL
void vcos_register_isr(VCOS_UNSIGNED vec,
                       VCOS_ISR_HANDLER_T handler,
                       VCOS_ISR_HANDLER_T *old_handler) {
   int ret = rtos_register_lisr(vec, handler, old_handler);
   vcos_assert(ret == 0);
}

/***********************************************************
 *
 * Memory
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mempool_create(VCOS_MEMPOOL_T *pool,
                                  const char *name, void *start, VCOS_UNSIGNED size) {
   UINT st = tx_byte_pool_create(pool, (CHAR*)name, start, size);
   vcos_assert(st == TX_SUCCESS);
   return VCOS_SUCCESS;
}


VCOS_INLINE_IMPL
void *vcos_mempool_alloc(VCOS_MEMPOOL_T *pool, VCOS_UNSIGNED len) {
   void *ret;
   UINT st = tx_byte_allocate(pool, &ret, len, 0);
   return st == TX_SUCCESS? ret : 0;
}

VCOS_INLINE_IMPL
void vcos_mempool_free(VCOS_MEMPOOL_T *pool, void *mem) {
   UINT st = tx_byte_release(mem);
   vcos_assert(st == TX_SUCCESS);
}

VCOS_INLINE_IMPL
void vcos_mempool_delete(VCOS_MEMPOOL_T *pool) {
   UINT st = tx_byte_pool_delete(pool);
   vcos_assert(st == TX_SUCCESS);
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

#include "vcfw/vcos/vcos_malloc.h"
#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"

#include "interface/vcos/generic/vcos_common.h"

VCOSPRE_ const char * VCOSPOST_ _vcos_log_level(void);
#define _VCOS_LOG_LEVEL() _vcos_log_level()


#endif /* VCOS_PLATFORM_H */

