/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Pthreads implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#define __USE_POSIX199309
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>

#define VCOS_HAVE_RTOS         1
#define VCOS_HAVE_SEMAPHORE    1
#define VCOS_HAVE_EVENT        1
#define VCOS_HAVE_QUEUE        0
#define VCOS_HAVE_LEGACY_ISR   0
#define VCOS_HAVE_TIMER        1
#define VCOS_HAVE_MEMPOOL      0
#define VCOS_HAVE_ISR          0
#define VCOS_HAVE_ATOMIC_FLAGS 1
#define VCOS_HAVE_THREAD_AT_EXIT        1

#ifdef __linux__
#ifndef ANDROID
#define VCOS_HAVE_BACKTRACE    1
#endif
#endif

#define VCOS_SO_EXT  ".so"

/* Linux/pthreads seems to have different timer characteristics */
#define VCOS_TIMER_MARGIN_EARLY 0
#define VCOS_TIMER_MARGIN_LATE 15

typedef sem_t                 VCOS_SEMAPHORE_T;
typedef sem_t                 VCOS_EVENT_T;
typedef pthread_mutex_t       VCOS_MUTEX_T;

typedef uint32_t              VCOS_UNSIGNED;
typedef uint32_t              VCOS_OPTION;
typedef pthread_key_t         VCOS_TLS_KEY_T;

#ifdef __arm__
#ifndef ANDROID
typedef __timer_t timer_t;
#endif
#endif
typedef struct VCOS_TIMER_T
{
   timer_t timer_id;       /**< id of timer */
   void (*pfn)(void*);  /**< Call this on expiration */
   void *context;       /**< Context for callback */
} VCOS_TIMER_T;

/** Thread attribute structure. Don't use pthread_attr directly, as
  * the calls can fail, and inits must match deletes.
  */
typedef struct VCOS_THREAD_ATTR_T
{
   void *ta_stackaddr;
   VCOS_UNSIGNED ta_stacksz;
   VCOS_UNSIGNED ta_priority;
   VCOS_UNSIGNED ta_affinity;
   VCOS_UNSIGNED ta_timeslice;
   VCOS_UNSIGNED legacy;
} VCOS_THREAD_ATTR_T;

/** Called at thread exit.
  */
typedef struct VCOS_THREAD_EXIT_T
{
   void (*pfn)(void *);
   void *cxt;
} VCOS_THREAD_EXIT_T;
#define VCOS_MAX_EXIT_HANDLERS  4

typedef struct VCOS_THREAD_T
{
   pthread_t thread;             /**< The thread itself */
   VCOS_THREAD_ENTRY_FN_T entry; /**< The thread entry point */
   void *arg;                    /**< The argument to be passed to entry */
   VCOS_SEMAPHORE_T suspend;     /**< For support event groups and similar - a per thread semaphore */
   VCOS_TIMER_T task_timer;
   VCOS_UNSIGNED legacy;
   char name[16];                 /**< Record the name of this thread, for diagnostics */

   /** Callback invoked at thread exit time */
   VCOS_THREAD_EXIT_T at_exit[VCOS_MAX_EXIT_HANDLERS];
} VCOS_THREAD_T;

#ifdef VCOS_PTHREADS_WANT_HISR_EMULATION

typedef struct
{
   VCOS_THREAD_T thread;
   char stack[1024];
   VCOS_SEMAPHORE_T waitsem;
} VCOS_HISR_T;

#endif

#define VCOS_SUSPEND          -1
#define VCOS_NO_SUSPEND       0

#define VCOS_START 1
#define VCOS_NO_START 0

#define VCOS_THREAD_PRI_MIN   (sched_get_priority_min(SCHED_OTHER))
#define VCOS_THREAD_PRI_MAX   (sched_get_priority_max(SCHED_OTHER))

#define VCOS_THREAD_PRI_INCREASE (1)
#define VCOS_THREAD_PRI_HIGHEST  VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_LOWEST   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_NORMAL ((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2)
#define VCOS_THREAD_PRI_ABOVE_NORMAL (((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2) + 1)
#define VCOS_THREAD_PRI_REALTIME VCOS_THREAD_PRI_MAX

#define _VCOS_AFFINITY_DEFAULT 0
#define _VCOS_AFFINITY_CPU0    0x100
#define _VCOS_AFFINITY_CPU1    0x200
#define _VCOS_AFFINITY_MASK    0x300
#define VCOS_CAN_SET_STACK_ADDR  0

#define VCOS_TICKS_PER_SECOND _vcos_get_ticks_per_second()

#if VCOS_HAVE_ATOMIC_FLAGS
#define _VCOS_OS_LOCK_STATE int vcos_pthreads_spinlock_ei
#define _VCOS_OS_DISABLE()
#define _VCOS_OS_RESTORE()
#define _VCOS_OS_RESTORE_AND_SUSPEND()
#include "vcfw/vcos/vcos_spinlock_atomic_flags.h"
#endif

#include "interface/vcos/generic/vcos_generic_event_flags.h"
#include "interface/vcos/generic/vcos_mem_from_malloc.h"

extern pthread_key_t _vcos_thread_current_key;

/** Convert errno values into the values recognized by vcos */
VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_pthreads_map_errno(void);

extern void
vcos_pthreads_logging_assert(const char *file, const char *func, unsigned int line, const char *fmt, ...);

/** Register a function to be called when the current thread exits.
  */
extern VCOS_STATUS_T vcos_thread_at_exit(void (*pfn)(void*), void *cxt);

extern uint32_t _vcos_get_ticks_per_second(void);
/** C function to register as an IRQ. Called in interrupt context obviously.
  */
typedef void (*VCOS_ISR_HANDLER_T)(const VCOS_UNSIGNED vecnum);

#if defined(VCOS_INLINE_BODIES)

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1


/*
 * Counted Semaphores
 */
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   int ret;
   /* gdb causes sem_wait() to EINTR when a breakpoint is hit, retry here */
   while ((ret = sem_wait(sem)) == -1 && errno == EINTR)
      continue;
   vcos_assert(ret==0);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   int ret;
   while ((ret = sem_trywait(sem)) == -1 && errno == EINTR)
      continue;
   if (ret == 0)
      return VCOS_SUCCESS;
   else if (errno == EAGAIN)
      return VCOS_EAGAIN;
   else {
      vcos_assert(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                                        const char *name,
                                                        VCOS_UNSIGNED initial_count) {
   int rc = sem_init(sem, 0, initial_count);
   if (rc != -1) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   int rc = sem_destroy(sem);
   vcos_assert(rc != -1);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   int rc = sem_post(sem);
   vcos_assert(rc == 0);
   (void)rc;
   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_THREAD_T *vcos_thread_current(void) {
   void *ret = pthread_getspecific(_vcos_thread_current_key);
#ifdef __cplusplus
   return static_cast<VCOS_THREAD_T*>(ret);
#else
   return ret;
#endif
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   struct timespec ts;
   ts.tv_sec = ms/1000;
   ts.tv_nsec = ms % 1000 * (1000000);
   nanosleep(&ts, NULL);
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setstack(VCOS_THREAD_ATTR_T *attr, void *addr, VCOS_UNSIGNED sz) {
   attr->ta_stackaddr = addr;
   attr->ta_stacksz = sz;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setstacksize(VCOS_THREAD_ATTR_T *attr, VCOS_UNSIGNED sz) {
   attr->ta_stacksz = sz;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setpriority(VCOS_THREAD_ATTR_T *attr, VCOS_UNSIGNED pri) {
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED p) {
   /* not implemented */
}
VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   /* not implemented */
   return 0;
}


VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
	printf("vcos_thread_get_affinity %s %d\n", __FILE__, __LINE__);
  return _VCOS_AFFINITY_CPU0;
}


VCOS_INLINE_IMPL
void vcos_thread_attr_setaffinity(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED affinity) {
   attrs->ta_affinity = affinity;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_settimeslice(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED ts) {
   attrs->ta_timeslice = ts;
}

VCOS_INLINE_IMPL
void _vcos_thread_attr_setlegacyapi(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED legacy) {
   attrs->legacy = legacy;
}

VCOS_INLINE_IMPL
void vcos_thread_attr_setautostart(VCOS_THREAD_ATTR_T *attrs, VCOS_UNSIGNED autostart) {
   
}

/*
 * Mutexes
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_create(VCOS_MUTEX_T *latch, const char *name) {
   int rc = pthread_mutex_init(latch, NULL);
   if (rc == 0) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_mutex_delete(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_destroy(latch);
   (void)rc;
   vcos_assert(rc==0);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_lock(latch);
   vcos_assert(rc==0);
   (void)rc;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *latch) {
   int rc = pthread_mutex_unlock(latch);
   (void)rc;
   vcos_assert(rc==0);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *m) {
   int rc = pthread_mutex_trylock(m);
   if (rc == 0) {
      pthread_mutex_unlock(m);
      /* it wasn't locked */
      return 0;
   }
   else {
      return 1; /* it was locked */
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_trylock(VCOS_MUTEX_T *m) {
   int rc = pthread_mutex_trylock(m);
   (void)rc;
   return (rc == 0) ? VCOS_SUCCESS : VCOS_EAGAIN;
}


/***********************************************************
 *
 * Timers
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_timer_create(VCOS_TIMER_T *timer,
                                const char *name,
                                void (*expiration_routine)(void *context),
                                void *context) {
   sigevent_t event;
   int rc;

   vcos_assert(timer);
   vcos_assert(expiration_routine);

   timer->pfn = expiration_routine;
   timer->context = context;
   event.sigev_notify = SIGEV_THREAD;
   event.sigev_value.sival_ptr = context;
   event.sigev_notify_function = (void (*)(union sigval))expiration_routine;
   event.sigev_notify_attributes = NULL;
   rc = timer_create(CLOCK_REALTIME, &event, &timer->timer_id);
   if (rc == 0)
      return VCOS_SUCCESS;
   else
      return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   struct itimerspec value;
   int rc;
   vcos_assert(timer);
   vcos_assert(timer->pfn);
   value.it_interval.tv_sec = 0;
   value.it_interval.tv_nsec = 0;
   value.it_value.tv_sec = delay_ms / 1000;
   value.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
   rc = timer_settime(timer->timer_id, 0, &value, NULL);
   vcos_assert(rc == 0); /* FIXME: what if this fails? */
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   struct itimerspec value;
   int rc;
   vcos_assert(timer);
   vcos_assert(timer->pfn);
   value.it_interval.tv_sec = 0;
   value.it_interval.tv_nsec = 0;
   value.it_value.tv_sec = 0;
   value.it_value.tv_nsec = 0;
   rc = timer_settime(timer->timer_id, 0, &value, NULL);
   vcos_assert(rc == 0); /* FIXME: what if this fails? */
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay_ms);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   int rc;
   rc = timer_delete(timer->timer_id);
   vcos_assert(rc == 0);
   (void)rc;
}


/*
 * Events
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *debug_name)
{
   int rc = sem_init(event, 0, 0);
   if (rc != -1) return VCOS_SUCCESS;
   else return vcos_pthreads_map_errno();
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event)
{
   int rc = sem_post(event);
   vcos_assert(rc == 0);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event)
{
   int ret;
   /* gdb causes sem_wait() to EINTR when a breakpoint is hit, retry here */
   while ((ret = sem_wait(event)) == -1 && errno == EINTR)
      continue;
   vcos_assert(ret==0);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_try(VCOS_EVENT_T *event)
{
   int ret;
   while ((ret = sem_trywait(event)) == -1 && errno == EINTR)
      continue;

   if (ret == -1 && errno == EAGAIN)
      return VCOS_EAGAIN;
   else
      return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event)
{
   int rc = sem_destroy(event);
   vcos_assert(rc != -1);
   (void)rc;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
#ifdef BRCM_V3D_OPT
   return (VCOS_UNSIGNED) pthread_self();//gettid();
#else
   return (VCOS_UNSIGNED) getpid();
#endif
}

VCOS_INLINE_IMPL
int vcos_strcasecmp(const char *s1, const char *s2) {
   return strcasecmp(s1,s2);
}

VCOS_INLINE_IMPL
int vcos_strncasecmp(const char *s1, const char *s2, size_t n) {
   return strncasecmp(s1,s2,n);
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return 0;
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
VCOS_STATUS_T vcos_tls_create(VCOS_TLS_KEY_T *key) {
   int st = pthread_key_create(key, NULL);
   return st == 0 ? VCOS_SUCCESS: VCOS_ENOMEM;
}

VCOS_INLINE_IMPL
void vcos_tls_delete(VCOS_TLS_KEY_T tls) {
   pthread_key_delete(tls);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_tls_set(VCOS_TLS_KEY_T tls, void *v) {
   pthread_setspecific(tls, v);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void *vcos_tls_get(VCOS_TLS_KEY_T tls) {
   return pthread_getspecific(tls);
}

#ifdef linux

// not exactly the free memory, but a measure of it

VCOS_INLINE_IMPL
unsigned long vcos_get_free_mem(void) {
   return (unsigned long)sbrk(0);
}

#endif

#ifdef VCOS_PTHREADS_WANT_HISR_EMULATION
VCOS_STATUS_T vcos_legacy_hisr_create(VCOS_HISR_T *hisr, const char *name,
                                      void (*entry)(void),
                                      VCOS_UNSIGNED pri,
                                      void *stack, VCOS_UNSIGNED stack_size);

void vcos_legacy_hisr_activate(VCOS_HISR_T *hisr);

void vcos_legacy_hisr_delete(VCOS_HISR_T *hisr);

#endif

/***********************************************************
 *
 * Interrupt handling
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_register_isr(VCOS_UNSIGNED vec,
                       VCOS_ISR_HANDLER_T handler,
                       VCOS_ISR_HANDLER_T *old_handler) {
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_int_disable(void) {
   return 0;
}

VCOS_INLINE_IMPL
void vcos_int_restore(VCOS_UNSIGNED previous) {
}

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 0

#endif /* VCOS_INLINE_BODIES */

VCOS_INLINE_DECL void _vcos_thread_sem_wait(void);
VCOS_INLINE_DECL void _vcos_thread_sem_post(VCOS_THREAD_T *);

#define VCOS_APPLICATION_ARGC          vcos_get_argc()
#define VCOS_APPLICATION_ARGV          vcos_get_argv()

#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"
#include "interface/vcos/generic/vcos_common.h"

#define _VCOS_LOG_LEVEL() getenv("VC_LOGLEVEL")

#ifdef __cplusplus
}
#endif
#endif /* VCOS_PLATFORM_H */

