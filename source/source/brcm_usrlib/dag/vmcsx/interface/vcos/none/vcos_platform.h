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
  * Implementation of OSAL for the case where there is no RTOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#define VCOS_HAVE_RTOS          0
#define VCOS_HAVE_SEMAPHORE     1
#define VCOS_HAVE_TIMER         1
#define VCOS_HAVE_ISR           1
#define VCOS_HAVE_LEGACY_ISR    0
#define VCOS_HAVE_EVENT         1
#define VCOS_HAVE_ATOMIC_FLAGS  1

#define VCOS_CAN_SET_STACK_ADDR  0

#define VCOS_WANT_TLS_EMULATION

#if defined(VCOS_INLINE_BODIES)
#include <string.h>         /* for strcasecmp */
#endif

#include "vcfw/rtos/rtos.h"
#include "vcfw/rtos/common/dmalloc.h"
#include "vcfw/vclib/vclib.h"

typedef int VCOS_QUICKSLOW_MUTEX_T; /* don't actually need any storage -- just a global spinlock on RTOS=none */

typedef int                   VCOS_QUEUE_T;
typedef dmalloc_pool_t        VCOS_MEMPOOL_T;
typedef struct VCOS_LLTHREAD_T
{
   volatile int suspended; /* when suspended, a thread will just loop until this becomes 0 */
   void *tls;
} VCOS_LLTHREAD_T;

typedef struct {
   void (*entry)(void);
} VCOS_HISR_T;

typedef struct {
   RTOS_TIMER_T timer;
   void (*fn)(void *);
   void *context;
} VCOS_TIMER_T;


typedef unsigned long         VCOS_UNSIGNED;
typedef unsigned long         VCOS_OPTION;
typedef int                   VCOS_INT;

#define VCOS_SUSPEND       ((unsigned long)-1)
#define VCOS_NO_SUSPEND       0

#define VCOS_NO_START      0
#define VCOS_START         1

#define VCOS_TICKS_PER_SECOND 1000000

/*
 * CPU affinity
 */
#define _VCOS_AFFINITY_CPU0 0
#define _VCOS_AFFINITY_CPU1 1
#define _VCOS_AFFINITY_MASK 1
#define _VCOS_AFFINITY_DEFAULT 1 /* default to vpu1 -- we can't create threads on vpu0! */

/*
 * Thread priority
 */
#define VCOS_THREAD_PRI_MIN   1
#define VCOS_THREAD_PRI_MAX   15

#define VCOS_THREAD_PRI_NORMAL         ((VCOS_THREAD_PRI_MIN+VCOS_THREAD_PRI_MAX)/2)
#define VCOS_THREAD_PRI_BELOW_NORMAL   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_ABOVE_NORMAL   VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_HIGHEST        VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_LOWEST         VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_INCREASE       0

#define VCOS_APPLICATION_ARGC          rtos_argc
#define VCOS_APPLICATION_ARGV          rtos_argv
#define _VCOS_PRE

/** C function to register as an IRQ. Called in interrupt context obviously.
  */
typedef void (*VCOS_ISR_HANDLER_T)(const VCOS_UNSIGNED vecnum);

extern void vcos_none_timer_expiration_routine(RTOS_TIMER_T *timer, void *context);

/*
 * spinlock stuff (for the spinlock semaphore and atomic flags)
 */

VCOS_INLINE_DECL int vcos_none_spinlock_acquire(void);
VCOS_INLINE_DECL void vcos_none_spinlock_release(int);

#define _VCOS_OS_LOCK_STATE int vcos_none_spinlock_ei
#define _VCOS_OS_DISABLE() do { vcos_none_spinlock_ei = vcos_none_spinlock_acquire(); } while (0)
#define _VCOS_OS_RESTORE() do { vcos_none_spinlock_release(vcos_none_spinlock_ei); } while (0)
#define _VCOS_OS_RESTORE_AND_SUSPEND() do { \
   VCOS_LLTHREAD_T *thread = vcos_llthread_current(); \
   thread->suspended = 1; \
   vcos_none_spinlock_release(vcos_none_spinlock_ei); \
   while (thread->suspended) ; \
} while (0)

/*
   tls stuff
*/

VCOS_INLINE_DECL void *_vcos_tls_thread_ptr_get(void);
VCOS_INLINE_DECL void _vcos_tls_thread_ptr_set(void *p);

#include "vcfw/vcos/vcos_spinlock_semaphore.h"
#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"

extern VCOS_THREAD_T vcos_none_vpu0_thread;
extern VCOS_LLTHREAD_T *vcos_none_vpu1_thread;

#include "vcfw/vcos/vcos_spinlock_atomic_flags.h"

/* Do not include this file directly - instead include it via vcos.h */

#if defined(VCOS_INLINE_BODIES)

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1

/***********************************************************
 *
 * Quick/Slow Mutexes
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_quickslow_mutex_create(VCOS_QUICKSLOW_MUTEX_T *, const char *)
{
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_delete(VCOS_QUICKSLOW_MUTEX_T *)
{
}

/*
   no point in disabling interrupts:
   - interrupts can't grab mutexes
   - RTOS=none doesn't do task switches
   - interrupts should be short-lived and infrequent enough such that they don't
     cause mutex holding time to increase significantly
   in fact, we probably want to avoid disabling interrupts so they don't get
   starved when we're waiting for a slow unlock
*/

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_lock(VCOS_QUICKSLOW_MUTEX_T *)
{
   // vcscreentest does memory manager stuff with interrupts disabled...
   // vcos_assert((_vasm("mov  %D, %%r30") >> 30) & 1); /* interrupts enabled */
   while (_vasm("mov  %D, %%p17")) ;
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_unlock(VCOS_QUICKSLOW_MUTEX_T *)
{
   _vasm("mov  %%p17, %r", 0);
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_lock_quick(VCOS_QUICKSLOW_MUTEX_T *m)
{
   vcos_quickslow_mutex_lock(m);
}

VCOS_INLINE_IMPL
void vcos_quickslow_mutex_unlock_quick(VCOS_QUICKSLOW_MUTEX_T *m)
{
   vcos_quickslow_mutex_unlock(m);
}

/***********************************************************
 *
 * Spinlock
 *
 ***********************************************************/

extern volatile int vcos_none_spinlock_claim[2], vcos_none_spinlock_decider;

VCOS_INLINE_IMPL
int vcos_none_spinlock_acquire(void)
{
   int i = rtos_get_cpu_number();
#ifndef __CC_ARM
   int ei = !!(_vasm("mov  %D, %sr") & (1 << 30));
#else
   assert(0);
   int ei = 0;
#endif
   _di();

   vcos_assert(!vcos_none_spinlock_claim[i]);
   vcos_none_spinlock_claim[i] = 1;
   i ^= 1;
   vcos_none_spinlock_decider = i;
   while (vcos_none_spinlock_claim[i] && (vcos_none_spinlock_decider == i)) ;

   return ei;
}

VCOS_INLINE_IMPL
void vcos_none_spinlock_release(int ei)
{
   int i = rtos_get_cpu_number();

   vcos_assert(vcos_none_spinlock_claim[i]);
   vcos_none_spinlock_claim[i] = 0;

   if (ei) _ei();
}

/***********************************************************
 *
 * TLS
 *
 ***********************************************************/

VCOS_INLINE_IMPL
void *_vcos_tls_thread_ptr_get(void)
{
   return vcos_llthread_current()->tls;
}

VCOS_INLINE_IMPL
void _vcos_tls_thread_ptr_set(void *p)
{
   vcos_llthread_current()->tls = p;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   vcos_assert((thread == &vcos_none_vpu0_thread) ||
      (&thread->thread == vcos_none_vpu1_thread));
   return VCOS_THREAD_PRI_NORMAL;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
   if (thread == &vcos_none_vpu0_thread) {
      return _VCOS_AFFINITY_CPU0;
   }
   vcos_assert(&thread->thread == vcos_none_vpu1_thread);
   return _VCOS_AFFINITY_CPU1;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
   vcos_assert(
      ((thread == &vcos_none_vpu0_thread) && (affinity == _VCOS_AFFINITY_CPU0)) ||
      ((&thread->thread == vcos_none_vpu1_thread) && (affinity == _VCOS_AFFINITY_CPU1)));
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   rtos_delay(ms*1000, 0);
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return rtos_in_interrupt();
}

/* For support event groups - per thread semaphore */
VCOS_INLINE_IMPL
void _vcos_thread_sem_wait(void) {
   VCOS_THREAD_T *t = vcos_thread_current();
   vcos_semaphore_wait(&t->suspend);
}

VCOS_INLINE_IMPL
void _vcos_thread_sem_post(VCOS_THREAD_T *target) {
   vcos_semaphore_post(&target->suspend);
}

VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   int i = rtos_get_cpu_number();
   if (i == 0)
      return &vcos_none_vpu0_thread.thread;
   vcos_assert(i == 1);
   return vcos_none_vpu1_thread;
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED pri) {
   vcos_assert((thread == &vcos_none_vpu0_thread) ||
      (&thread->thread == vcos_none_vpu1_thread));
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) {
   vcos_assert(0);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_change_preemption(VCOS_UNSIGNED pe) {
   return pe;
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(VCOS_LLTHREAD_T *thread) {
   vcos_assert(thread->suspended);
   thread->suspended = 0;
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
   vcos_assert(0);
   return VCOS_EINVAL;
}

VCOS_INLINE_IMPL
void vcos_queue_delete(VCOS_QUEUE_T *queue) {
   vcos_assert(0);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_send(VCOS_QUEUE_T *queue, const void *src, VCOS_UNSIGNED wait) {
   vcos_assert(0);
   return VCOS_EINVAL;
}


VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_receive(VCOS_QUEUE_T *queue, void *dst, VCOS_UNSIGNED wait) {
   vcos_assert(0);
   return VCOS_EINVAL;
}

/***********************************************************
 *
 * Timers
 *
 ***********************************************************/

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
   int32_t st;
   timer->fn = expiration_routine;
   timer->context = context;
   st = rtos_timer_init(&timer->timer, vcos_none_timer_expiration_routine, timer);
   if (st < 0)
      return VCOS_EINVAL;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay) {
   int32_t st = rtos_timer_set(&timer->timer, delay*1000);
   vcos_assert(st == 0);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   rtos_timer_cancel(&timer->timer); // Can't fail
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   // Nothing to do
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
   hisr->entry = entry;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_activate(VCOS_HISR_T *hisr) {
#ifndef NDEBUG
   int32_t t = rtos_getmicrosecs();
#endif
   hisr->entry();
#ifndef NDEBUG
   t = rtos_getmicrosecs()-t;
   /* if you hit this assert, the HISR took too long! Refactor into
    * a separate thread!
    */
   vcos_assert(t < 256);
#endif
}

VCOS_INLINE_IMPL
void vcos_legacy_hisr_delete(VCOS_HISR_T *hisr) {
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
   int ret = rtos_register_lisr(vec, (RTOS_LISR_T)handler, (RTOS_LISR_T*)old_handler);
   vcos_assert(ret == 0);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_int_disable(void) {
   return vclib_disableint();
}

VCOS_INLINE_IMPL
void vcos_int_restore(VCOS_UNSIGNED previous) {
   vclib_restoreint(previous);
}

/***********************************************************
 *
 * Memory
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mempool_create(VCOS_MEMPOOL_T *pool,
                                  const char *name, void *start, VCOS_UNSIGNED size) {
   *pool = dmalloc_create_pool(start, (char *)start + size);
   if (*pool == 0)
   {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void *vcos_mempool_alloc(VCOS_MEMPOOL_T *pool, VCOS_UNSIGNED len) {
   return dmalloc(*pool, (size_t)len);
}

VCOS_INLINE_IMPL
void vcos_mempool_free(VCOS_MEMPOOL_T *pool, void *mem) {
   dfree(*pool, mem);
}

VCOS_INLINE_IMPL
void vcos_mempool_delete(VCOS_MEMPOOL_T *pool) {
   int st = dmalloc_delete_pool(*pool, 0); // Don't force the deletion - check the pool is empty
   vcos_assert(st == 0);
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

VCOS_INLINE_DECL void _vcos_thread_sem_wait(void);
VCOS_INLINE_DECL void _vcos_thread_sem_post(VCOS_THREAD_T *);

#include "vcfw/vcos/vcos_malloc.h"
#include "vcfw/vcos/vcos_rtos_latch_event.h"
#include "vcfw/vcos/vcos_rtos_latch.h"
#include "interface/vcos/generic/vcos_common.h"
#include "interface/vcos/generic/vcos_generic_event_flags.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"

VCOSPRE_ const char * VCOSPOST_ _vcos_log_level(void);
#define _VCOS_LOG_LEVEL() _vcos_log_level()

#ifdef __cplusplus
}
#endif

#endif /* VCOS_PLATFORM_H */
