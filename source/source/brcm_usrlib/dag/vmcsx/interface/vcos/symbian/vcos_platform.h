/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: #2 $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Symbian
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Symbian implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#include <kernel/kern_priv.h>

#define VCOS_HAVE_RTOS        1
#define VCOS_HAVE_MULTITHREAD 1
#define VCOS_HAVE_TIMER       0
#define VCOS_HAVE_SEMAPHORE   1
#define VCOS_HAVE_EVENT       0


typedef struct VCOS_SEMAPHORE_T
{
   DSemaphore *sem;
} VCOS_SEMAPHORE_T;

typedef VCOS_SEMAPHORE_T VCOS_NAMED_SEMAPHORE_T;

typedef struct VCOS_LLTHREAD_T
{
   struct VCOS_LLTHREAD_T *next;
   DThread *thread;
   void (*entry)(void*);
   void *arg;
   unsigned magic;
} VCOS_LLTHREAD_T;

//#define _VCOS_WIN32_THREAD_MAGIC 0x56435354

typedef struct VCOS_MUTEX_T
{
   TInt locked;
   DMutex *m;
} VCOS_MUTEX_T;

typedef struct VCOS_TIMER_T
{
   NTimer timer;
} VCOS_TIMER_T;

typedef VCOS_SEMAPHORE_T VCOS_EVENT_T;

typedef TUint32               VCOS_TLS_KEY_T;
typedef TUint32               VCOS_UNSIGNED;
typedef TUint32               VCOS_OPTION;

typedef struct VCOS_QUEUE_T
{
   VCOS_UNSIGNED *start;       //api uses words
   VCOS_UNSIGNED *end;
   char name[32];
   VCOS_SEMAPHORE_T producer;  //Producer waits on this when FIFO full
   VCOS_SEMAPHORE_T consumer;  //Consumer waits on this when FIFO empty
   volatile VCOS_UNSIGNED free_space;
   VCOS_UNSIGNED* read_ptr;
   VCOS_UNSIGNED* write_ptr;
   VCOS_UNSIGNED message_size;
   VCOS_UNSIGNED queue_size;
}VCOS_QUEUE_T;


#define VCOS_SUSPEND          -1
#define VCOS_NO_SUSPEND       0

#define VCOS_THREAD_PRI_MIN   0
#define VCOS_THREAD_PRI_MAX   63

#define VCOS_THREAD_PRI_INCREASE (1)
#define VCOS_THREAD_PRI_HIGHEST  VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_LOWEST   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_NORMAL   27
#define VCOS_THREAD_PRI_REALTIME 48

#define _VCOS_AFFINITY_MASK 0
#define _VCOS_AFFINITY_DEFAULT 0

#define _VCOS_PRE

#define VCOS_TICKS_PER_SECOND NKern::TimerTicks(1000)

#define VCOS_START 1
#define VCOS_NO_START 0
#define VCOS_DEFAULT_STACK_SIZE 8192

/* We can't set the stack address to be used by a newly created thread on this platform */
#define VCOS_CAN_SET_STACK_ADDR  0

/** Convert Symbian error values into the values recognized by vcos */
extern "C"
    {
VCOSPRE_ VCOS_STATUS_T VCOSPOST_ vcos_symbian_map_error(TInt err);
VCOSPRE_ VCOS_LLTHREAD_T * VCOSPOST_ _vcos_llthread_current(void);
    }

#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"
#include "interface/vcos/generic/vcos_mutexes_are_reentrant.h"


/***********************************************************
 *
 * Counted Semaphores
 *
 ***********************************************************/

#ifdef VCOS_INLINE_BODIES

#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   TInt ret = Kern::SemaphoreWait(*sem->sem);
   vcos_assert(ret == KErrNone);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   vcos_assert(0);
   return VCOS_EAGAIN;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                                        const char *name,
                                                        VCOS_UNSIGNED initial_count) {
   NKern::ThreadEnterCS();
   TInt ret = Kern::SemaphoreCreate(sem->sem, TPtrC8((TUint8*)(name?name:"VCOS")), initial_count);
   NKern::ThreadLeaveCS();
   return vcos_symbian_map_error(ret);
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
   NKern::ThreadEnterCS();
   Kern::SafeClose((DObject*&)sem->sem, NULL);
   NKern::ThreadLeaveCS();
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   Kern::SemaphoreSignal(*sem->sem);
   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/


VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) {
   return _vcos_llthread_current();
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(VCOS_LLTHREAD_T *thread) {
   Kern::ThreadResume(*thread->thread);
}

VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   NKern::Sleep(NKern::TimerTicks(ms));
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(VCOS_THREAD_T *thread, VCOS_UNSIGNED p) {
   Kern::SetThreadPriority(p, thread->thread.thread);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(VCOS_THREAD_T *thread) {
   //return GetThreadPriority(thread->thread.thread);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(VCOS_THREAD_T *thread) {
   //return _VCOS_AFFINITY_DEFAULT;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(VCOS_THREAD_T *thread, VCOS_UNSIGNED affinity) {
   NKern::ThreadSetCpuAffinity(&thread->thread.thread->iNThread, affinity);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
   return (VCOS_UNSIGNED)NKern::CurrentThread();
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) {
   NKern::YieldTimeslice();
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
   NKern::ThreadEnterCS();
   TInt ret = Kern::MutexCreate(mutex->m, TPtrC8((TUint8*)(name?name:"VCOS")), KMutexOrdGeneral4);
   NKern::ThreadLeaveCS();
   return vcos_symbian_map_error(ret);
}

VCOS_INLINE_IMPL
void vcos_mutex_delete(VCOS_MUTEX_T *mutex) {
   NKern::ThreadEnterCS();
   Kern::SafeClose((DObject*&)mutex->m, NULL);
   NKern::ThreadLeaveCS();
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *mutex) {
   NKern::ThreadEnterCS();
   Kern::MutexWait(*mutex->m);
   mutex->locked = 1;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *mutex) {
   mutex->locked = 0;
   Kern::MutexSignal(*mutex->m);
   NKern::ThreadLeaveCS();
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *mutex) {
   return mutex->locked;
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void) { 
   return NKern::CurrentContext() != NKern::EThread;
}

//VCOS_INLINE_IMPL
//int vcos_strcasecmp(const char *s1, const char *s2) {
//   return _stricmp(s1,s2);
//}

//VCOS_INLINE_IMPL
//int vcos_strncasecmp(const char *s1, const char *s2, size_t n) {
//   return _strnicmp(s1,s2,n);;
//}

/*
 * Events
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *debug_name) {
   NKern::ThreadEnterCS();
   TInt ret = Kern::SemaphoreCreate(event->sem, TPtrC8((TUint8*)(debug_name?debug_name:"VCOSevent")), 0);
   NKern::ThreadLeaveCS();
   return vcos_symbian_map_error(ret);
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event) {
#if 0
   // This would be ideal, but DSemaphore::Signal isn't exported
   NKern::LockSystem();
   if (event->sem->iCount <= 0)
       event->sem->Signal();
   else
       NKern::UnlockSystem();
#else
   // This should be okay, as long as two threads aren't signalling simultaneously...
   if (event->sem->iCount <= 0)
       Kern::SemaphoreSignal(*event->sem);
#endif
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event) {
   TInt ret = Kern::SemaphoreWait(*event->sem);
   vcos_assert(ret == KErrNone);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event) {
   NKern::ThreadEnterCS();
   Kern::SafeClose((DObject*&)event->sem, NULL);
   NKern::ThreadLeaveCS();
}

/***********************************************************
 *
 * Fixed size message queues
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_create(VCOS_QUEUE_T *queue,
                                const char *name,
                                VCOS_UNSIGNED message_size /*in VCOS_UNSIGNED not bytes*/,
                                void *queue_start,
                                VCOS_UNSIGNED queue_size) {
   VCOS_STATUS_T status = VCOS_EINVAL;  //fail by default
   if(queue && queue_start && message_size && queue_size) {
      queue->message_size = message_size;
      queue->start = (VCOS_UNSIGNED*)queue_start;
      queue->end = queue->start + queue_size*message_size;
      queue->write_ptr = queue->read_ptr = queue->start;
      queue->queue_size = queue_size;
      //queue_size free slots for producer
      status = vcos_semaphore_create(&queue->producer, "prod", queue_size);
      queue->free_space = queue_size;
      if(status != VCOS_SUCCESS)
         return(status);
      status = vcos_semaphore_create(&queue->consumer, "cons", 0);
   }
   return(status);

}

VCOS_INLINE_IMPL
void vcos_queue_delete(VCOS_QUEUE_T *queue) {
   if(queue) {
      vcos_semaphore_delete(&queue->producer);
      vcos_semaphore_delete(&queue->consumer);
   }
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_send(VCOS_QUEUE_T *queue, const void *src, VCOS_UNSIGNED wait) {
   VCOS_STATUS_T status;
   //note we are always blocking for now...in reality I imagine the queue will be big enough so we never block
   status = vcos_semaphore_wait(&queue->producer);
   if(status != VCOS_SUCCESS)
      return(status);
   //add the msg
   memcpy((void*)queue->write_ptr, (void*)src, queue->message_size*sizeof(VCOS_UNSIGNED));
   queue->write_ptr += queue->message_size;
   queue->free_space--;
   //loop back to the start
   if(queue->write_ptr > queue->end)
      queue->write_ptr = queue->start;
   status = vcos_semaphore_post(&queue->consumer);
   return(status);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_queue_receive(VCOS_QUEUE_T *queue, void *dst, VCOS_UNSIGNED wait) {
   VCOS_STATUS_T status;
   //we wait for a signal from queue_send for the next msg
   status = vcos_semaphore_wait(&queue->consumer);
   if(status != VCOS_SUCCESS)
      return(status);
   memcpy(dst, (void*)queue->read_ptr, queue->message_size*sizeof(VCOS_UNSIGNED));
   queue->read_ptr += queue->message_size;
   //loop back to the start
   if(queue->read_ptr > queue->end)
      queue->read_ptr = queue->start;
   queue->free_space++;
   //signal producer
   status = vcos_semaphore_post(&queue->producer);
   return(status);
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
                                
   timer->timer.iFunction = expiration_routine;
   timer->timer.iPtr = context;
   
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_timer_set(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   TInt err = timer->timer.OneShot(NKern::TimerTicks(delay_ms), ETrue);
   vcos_assert(err == KErrNone);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(VCOS_TIMER_T *timer) {
   timer->timer.Cancel();
}

VCOS_INLINE_IMPL
void vcos_timer_reset(VCOS_TIMER_T *timer, VCOS_UNSIGNED delay_ms) {
   vcos_timer_cancel(timer);
   vcos_timer_set(timer, delay_ms);
}

VCOS_INLINE_IMPL
void vcos_timer_delete(VCOS_TIMER_T *timer) {
   vcos_timer_cancel(timer);
}


VCOS_INLINE_IMPL
uint32_t vcos_getmicrosecs(void) {
   vcos_assert(0);
   return 0;
}

VCOS_INLINE_IMPL
void *vcos_malloc( VCOS_UNSIGNED size, const char *name )
{
   NKern::ThreadEnterCS();
   void *p = Kern::Alloc(size);
   NKern::ThreadLeaveCS();
   return p;
}

VCOS_INLINE_IMPL
void *vcos_malloc_aligned( VCOS_UNSIGNED size, VCOS_UNSIGNED align, const char *name )
{
   /* if anyone is asking for alignment >8, they're probably trying to give this
    * buffer to hardware - bad idea, as we're not giving them contiguous memory.
    * Make an effort to catch this.
    */
   if (align > 8)
   {
      vcos_assert(align > 8);
      return 0;
   }
   return vcos_malloc( size, name );
}

VCOS_INLINE_IMPL
void vcos_free( void *ptr )
{
   if (ptr) {
      NKern::ThreadEnterCS();
      Kern::Free(ptr);
      NKern::ThreadLeaveCS();
   }
}
#ifdef __cplusplus
}
#endif

#endif /* VCOS_INLINE_BODIES */

#include "interface/vcos/generic/vcos_generic_event_flags.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"
#include "interface/vcos/generic/vcos_common.h"

#endif /* VCOS_PLATFORM_H */

