/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - pthreads types
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Pthreads implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifndef _WIN32_WINNT
#define  _WIN32_WINNT 0x0500
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#include <sys/types.h>
#include <errno.h>

#define VCOS_HAVE_RTOS          1
#define VCOS_HAVE_TIMER         0
#define VCOS_HAVE_SEMAPHORE     1
#define VCOS_HAVE_EVENT         1
#define VCOS_HAVE_ATOMIC_FLAGS  1

/* Using critical sections for VCOS_MUTEX_T seems to break
 * VCHI. So for now use semaphores with a count of 1.
 */
//#define VCOS_WIN32_USE_CRITSEC

typedef struct VCOS_SEMAPHORE_T
{
   HANDLE sem;
} VCOS_SEMAPHORE_T;

typedef VCOS_SEMAPHORE_T VCOS_NAMED_SEMAPHORE_T;

typedef struct VCOS_LLTHREAD_T
{
   HANDLE thread;
   void (*entry)(void*);
   void *arg;
   unsigned magic;
} VCOS_LLTHREAD_T;

#define _VCOS_WIN32_THREAD_MAGIC 0x56435354

typedef struct VCOS_MUTEX_T
{
   BOOL locked;
   HANDLE h;
} VCOS_MUTEX_T;

typedef struct VCOS_TIMER_T
{
   HANDLE h;            /**< Handle to timer queue timer */
   HANDLE wait;         /**< Wait on this after cancelling a timer */
   void (*pfn)(void*);  /**< Call this on expiration */
   void *context;       /**< Context for callback */
} VCOS_TIMER_T;

typedef struct VCOS_EVENT_T
{
   HANDLE h;
} VCOS_EVENT_T;

typedef DWORD                 VCOS_TLS_KEY_T;
typedef uint32_t              VCOS_UNSIGNED;
typedef uint32_t              VCOS_OPTION;

#define VCOS_SUSPEND          -1
#define VCOS_NO_SUSPEND       0

#define VCOS_THREAD_PRI_MIN   0
#define VCOS_THREAD_PRI_MAX   6

#define VCOS_THREAD_PRI_INCREASE (1)
#define VCOS_THREAD_PRI_HIGHEST  VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_LOWEST   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_NORMAL   ((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2)
#define VCOS_THREAD_PRI_REALTIME VCOS_THREAD_PRI_MAX
extern int vcos_thread_priorities[(VCOS_THREAD_PRI_MAX - VCOS_THREAD_PRI_MIN) + 1];

#define _VCOS_PRE

#define _VCOS_AFFINITY_DEFAULT 0
#define _VCOS_AFFINITY_CPU0    0  // NOT core-locked, just aids compilation
#define _VCOS_AFFINITY_CPU1    0
#define _VCOS_AFFINITY_MASK    0xf0000000

#define VCOS_TICKS_PER_SECOND 1000

#define VCOS_START 1
#define VCOS_NO_START 0
#define VCOS_DEFAULT_STACK_SIZE 8192

/* We can't set the stack address to be used by a newly created thread on this platform */
#define VCOS_CAN_SET_STACK_ADDR  0

typedef LONG VCOS_ATOMIC_FLAGS_T;

#define VCOS_WIN32_REPORT_ERROR(ok) if (!ok) vcos_win32_report_error()

#ifdef __cplusplus
extern "C" {
#endif

/** Convert errno values into the values recognized by vcos */
VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_win32_map_error(void);

VCOSPRE_ void VCOSPOST_ vcos_win32_report_error(void);

VCOSPRE_ void * VCOSPOST_ vcos_get_thread_data_tls(void);

VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_win32_named_semaphore_create(VCOS_NAMED_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count);

VCOSPRE_ uint32_t VCOSPOST_ vcos_win32_getmicrosecs(void);

#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"
#include "interface/vcos/generic/vcos_mutexes_are_reentrant.h"

/***********************************************************
 *
 * Counted Semaphores
 *
 ***********************************************************/

#ifdef VCOS_INLINE_BODIES

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1

#include <string.h>


VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   DWORD rc = WaitForSingleObject(sem->sem,INFINITE);
   vcos_assert(rc == WAIT_OBJECT_0);
   (void)rc;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   int ret = WaitForSingleObject(sem->sem,0);
   if (ret == 0)
      return VCOS_SUCCESS;
   else if (ret == WAIT_TIMEOUT)
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
   HANDLE h = CreateSemaphore(NULL, initial_count, 1<<16, NULL);
   if (h) {
      sem->sem = h;
      return VCOS_SUCCESS;
   }
   else
      return vcos_win32_map_error();
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   BOOL ok;
   vcos_assert(sem->sem != NULL);
   ok = CloseHandle(sem->sem);
   vcos_assert(ok);
   sem->sem = NULL;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   BOOL ok;
   if (!sem->sem)
      vcos_assert(0);
   ok = ReleaseSemaphore(sem->sem, 1, NULL);
   vcos_assert(ok);
   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/


VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   VCOS_LLTHREAD_T *ret = (VCOS_LLTHREAD_T *)vcos_get_thread_data_tls();
   vcos_assert(ret != (VCOS_LLTHREAD_T*)-1);
   return ret;
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(VCOS_LLTHREAD_T *thread) {
   ResumeThread(thread->thread);
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   Sleep(ms);
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED p) {
   vcos_assert((p >= VCOS_THREAD_PRI_MIN) && (p <= VCOS_THREAD_PRI_MAX));
   if (!SetThreadPriority(thread->thread.thread, vcos_thread_priorities[p - VCOS_THREAD_PRI_MIN]))
   {
      int err = GetLastError();
      vcos_assert(err == 0);
   }
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   int prio = GetThreadPriority(thread->thread.thread);
   int p;
   for (p = 0; p <= (VCOS_THREAD_PRI_MAX - VCOS_THREAD_PRI_MIN); p++)
   {
      if (vcos_thread_priorities[p] == prio)
         return p + VCOS_THREAD_PRI_MIN;
   }
   vcos_assert(0);
   return -1;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
   return _VCOS_AFFINITY_DEFAULT;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
   return GetCurrentProcessId();
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) {
   vcos_assert(0);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_change_preemption(VCOS_UNSIGNED pe) {
   vcos_assert(0);
   return pe;
}

/*
 * Joinable threads store an additional semaphore which is
 * used to let threads go to sleep/wakeup in a counted fashion.
 * The event group uses it.
 *
 * It's for internal use only, and is not needed on all platforms.
 */
VCOS_INLINE_IMPL
void _vcos_thread_sem_wait(void) {
   VCOS_THREAD_T *t = vcos_thread_current();
   vcos_semaphore_wait(&t->suspend);
}

VCOS_INLINE_IMPL
void _vcos_thread_sem_post(VCOS_THREAD_T *thread) {
   vcos_semaphore_post(&thread->suspend);
}

/***********************************************************
 *
 * Mutexes
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_create(VCOS_MUTEX_T *mutex, const char *name) {
   mutex->h = CreateMutex(NULL,FALSE,NULL);
   mutex->locked = 0;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_delete(VCOS_MUTEX_T *mutex) {
   //vcos_assert(!mutex->locked);
   CloseHandle(mutex->h);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *mutex) {
   vcos_assert(mutex->h);
   WaitForSingleObject(mutex->h,INFINITE);
   mutex->locked = 1;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *mutex) {
   mutex->locked = 0;
   ReleaseMutex(mutex->h);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *mutex) {
   return mutex->locked;
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return 0;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_trylock(VCOS_MUTEX_T *mutex) {
   DWORD ret = WaitForSingleObject(mutex->h, 0);
   if (ret == WAIT_OBJECT_0) {
      mutex->locked = 1;
      return VCOS_SUCCESS;
   }
   else {
      return VCOS_EAGAIN;
   }
}

VCOS_INLINE_IMPL
int vcos_strcasecmp(const char *s1, const char *s2) {
#ifdef _MSC_VER
   return _stricmp(s1,s2);
#else
   return stricmp(s1,s2);
#endif
}

VCOS_INLINE_IMPL
int vcos_strncasecmp(const char *s1, const char *s2, size_t n) {
#ifdef _MSC_VER
   return _strnicmp(s1,s2,n);
#else
   return strnicmp(s1,s2,n);
#endif
}

/***********************************************************
 *
 * Timers
 *
 * Note: Using Windows timer queues you don't seem to be able
 * to create a timer (which might fail due to lack of resources)
 * and then later start it,
 ***********************************************************/

VCOSPRE_ HANDLE VCOSPOST_ _vcos_timer_queue(void);

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_timer_create(VCOS_TIMER_T *timer,
                                const char *name,
                                void (*expiration_routine)(void *context),
                                void *context) {
   vcos_assert(timer);
   vcos_assert(expiration_routine);

   timer->h = NULL;
   timer->pfn = expiration_routine;
   timer->context = context;
   timer->wait = CreateEvent(NULL,FALSE,FALSE,NULL);
   if (!timer->wait)
      return VCOS_ENOMEM;

   return VCOS_SUCCESS;
}

extern void CALLBACK vcos_win32_timer_wrapper(void *cxt, BOOLEAN);

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   BOOL ok;
   HANDLE timer_queue = _vcos_timer_queue();

   vcos_assert(timer);
   vcos_assert(timer->pfn);
   vcos_assert(timer_queue); /* Has vcos_timer_init() been called? */
   ok = CreateTimerQueueTimer(&timer->h, timer_queue,
                              vcos_win32_timer_wrapper, timer,
                              delay_ms, 0, WT_EXECUTEINPERSISTENTTHREAD);
   vcos_assert(ok); /* FIXME: what if this fails? */
   vcos_assert(timer->h);
}

VCOS_INLINE_IMPL
void vcos_timer_set_ms(VCOS_TIMER_T *timer, VCOS_UNSIGNED ms) {
   vcos_timer_set(timer, ms);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   /* delete the timer, and wait for any ongoing timer function calls to complete */
   BOOL ok;
   vcos_assert(timer);
   vcos_assert(timer->wait);
   vcos_assert(timer->h);
   ok = DeleteTimerQueueTimer(_vcos_timer_queue(), timer->h, timer->wait);
   WaitForSingleObject(timer->wait,INFINITE);
   timer->h = NULL;
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay_ms);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   vcos_assert(timer);
   CloseHandle(timer->wait);
}

/*
 * Events
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *debug_name)
{
   event->h = CreateEvent(NULL, FALSE, FALSE, NULL);
   return event->h ? VCOS_SUCCESS : VCOS_ENOMEM;
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event)
{
   BOOL ok = SetEvent(event->h);
   vcos_assert(ok);
   (void)ok;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event)
{
   DWORD ret = WaitForSingleObject(event->h, INFINITE);
   vcos_assert(ret == WAIT_OBJECT_0);
   (void)ret;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_try(VCOS_EVENT_T *event)
{
   DWORD ret = WaitForSingleObject(event->h, 0);
   switch (ret) {
   case WAIT_OBJECT_0:
      return VCOS_SUCCESS;
   case WAIT_TIMEOUT:
      return VCOS_EAGAIN;
   default:
      vcos_demand(0);
      return VCOS_EINVAL;
   }
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event)
{
   BOOL ok = CloseHandle(event->h);
   vcos_assert(ok);
   (void)ok;
}

/*
 * Named semaphores
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_named_semaphore_create(VCOS_NAMED_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count) {
   return vcos_win32_named_semaphore_create(sem, name, count);
}

VCOS_INLINE_IMPL
void vcos_named_semaphore_wait(VCOS_NAMED_SEMAPHORE_T *sem) {
   vcos_semaphore_wait(sem);
}

VCOS_INLINE_IMPL
void vcos_named_semaphore_post(VCOS_NAMED_SEMAPHORE_T *sem) {
   vcos_semaphore_post(sem);
}

VCOS_INLINE_IMPL
void vcos_named_semaphore_delete(VCOS_NAMED_SEMAPHORE_T *sem) {
   vcos_semaphore_delete(sem);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_named_semaphore_trywait(VCOS_NAMED_SEMAPHORE_T *sem) {
   return vcos_semaphore_trywait(sem);
}
/*
 * Thread local storage
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_tls_create(VCOS_TLS_KEY_T *key) {
   *key = TlsAlloc();
   if (*key == TLS_OUT_OF_INDEXES)
      return VCOS_ENOSPC;
   else
      return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_tls_delete(VCOS_TLS_KEY_T tls) {
   TlsFree(tls);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_tls_set(VCOS_TLS_KEY_T tls, void *v) {
   BOOL ok = TlsSetValue(tls, v);
   vcos_assert(ok);
   (void)ok;
   return VCOS_SUCCESS;
}


/** Get the value for the current thread.
  *
  * @param key    The key to update
  *
  * @return The current value for this thread.
  */
VCOS_INLINE_IMPL
void *vcos_tls_get(VCOS_TLS_KEY_T tls) {
   return TlsGetValue(tls);
}

/*
 * Atomic flags
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_atomic_flags_create(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   *atomic_flags = 0;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_or(VCOS_ATOMIC_FLAGS_T *atomic_flags, uint32_t flags)
{
   /* for some reason, InterlockedOr isn't defined for x86 */
   LONG x, y = 0;
   do {
      x = y;
      y = InterlockedCompareExchange(atomic_flags, x | flags, x);
   } while (x != y);
}

VCOS_INLINE_IMPL
uint32_t vcos_atomic_flags_get_and_clear(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
   return InterlockedExchange(atomic_flags, 0);
}

VCOS_INLINE_IMPL
void vcos_atomic_flags_delete(VCOS_ATOMIC_FLAGS_T *atomic_flags)
{
}

VCOS_INLINE_IMPL
unsigned long vcos_get_free_mem(void) {
   vcos_assert(0); /* not yet implemented */
   return 0;
}

VCOS_INLINE_IMPL
uint32_t vcos_getmicrosecs(void) {
   return vcos_win32_getmicrosecs();
}

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 0

#endif /* VCOS_INLINE_BODIES */

/**
  *
  * Return the underlying OS handle for the event. This
  * turns out to be useful, especially on Windows.
  */
static __inline
HANDLE vcos_event_get_handle(VCOS_EVENT_T *event) {
   return event->h;
}


VCOS_INLINE_DECL void _vcos_thread_sem_wait(void);
VCOS_INLINE_DECL void _vcos_thread_sem_post(VCOS_THREAD_T *thread);

#include "interface/vcos/generic/vcos_generic_event_flags.h"
#include "interface/vcos/generic/vcos_common.h"
#include "interface/vcos/generic/vcos_mem_from_malloc.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"

#define _VCOS_LOG_LEVEL() getenv("VC_LOGLEVEL")

#ifdef __cplusplus
}
#endif


#endif /* VCOS_PLATFORM_H */

