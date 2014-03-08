/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Linux kernel (partial) implementation.
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Linux kernel (partial) implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <asm/bitops.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>
#include <linux/jiffies.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/random.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/time.h>  // for time_t


#define VCOS_HAVE_RTOS         1
#define VCOS_HAVE_SEMAPHORE    1
#define VCOS_HAVE_EVENT        1
#define VCOS_HAVE_QUEUE        0
#define VCOS_HAVE_LEGACY_ISR   0
#define VCOS_HAVE_TIMER        0
#define VCOS_HAVE_MEMPOOL      0
#define VCOS_HAVE_ISR          0
#define VCOS_HAVE_ATOMIC_FLAGS 0

/* Exclude many VCOS classes which don't have predicates */
#define VCOS_EVENT_FLAGS_H
#define VCOS_TLS_H
#define VCOS_NAMED_MUTEX_H
#define VCOS_REENTRANT_MUTEX_H
#define VCOS_NAMED_SEMAPHORE_H

typedef struct semaphore      VCOS_SEMAPHORE_T;
typedef struct semaphore      VCOS_EVENT_T;
typedef struct mutex          VCOS_MUTEX_T;

typedef unsigned int          VCOS_UNSIGNED;
typedef unsigned int          VCOS_OPTION;
//typedef pthread_key_t         VCOS_TLS_KEY_T;

/** Thread attribute structure. Don't use pthread_attr directly, as
  * the calls can fail, and inits must match deletes.
  */
typedef struct VCOS_THREAD_ATTR_T
{
   VCOS_UNSIGNED ta_stacksz;
   VCOS_UNSIGNED ta_priority;
   VCOS_UNSIGNED ta_affinity;
   VCOS_UNSIGNED ta_timeslice;
   VCOS_UNSIGNED legacy;
} VCOS_THREAD_ATTR_T;

typedef struct task_struct *VCOS_THREAD_T;

#define VCOS_SUSPEND          -1
#define VCOS_NO_SUSPEND       0

#define VCOS_START 1
#define VCOS_NO_START 0

#define VCOS_THREAD_PRI_MIN   0
#define VCOS_THREAD_PRI_MAX   0

#define VCOS_THREAD_PRI_INCREASE 0
#define VCOS_THREAD_PRI_HIGHEST  VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_LOWEST   VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_NORMAL ((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2)
#define VCOS_THREAD_PRI_ABOVE_NORMAL (((VCOS_THREAD_PRI_MAX+VCOS_THREAD_PRI_MIN)/2) + VCOS_THREAD_PRI_INCREASE)
#define VCOS_THREAD_PRI_REALTIME VCOS_THREAD_PRI_MAX

#define _VCOS_AFFINITY_DEFAULT 0
#define _VCOS_AFFINITY_CPU0 0
#define _VCOS_AFFINITY_CPU1 0
#define _VCOS_AFFINITY_MASK 0
#define VCOS_CAN_SET_STACK_ADDR  0

#define VCOS_TICKS_PER_SECOND HZ

typedef unsigned long VCOS_ATOMIC_FLAGS_T; /* TODO: dummy type - improve */

#if defined(VCOS_INLINE_BODIES)

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 1

/*
 * Counted Semaphores
 */
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(VCOS_SEMAPHORE_T *sem) {
   if (down_interruptible(sem) != 0)
      return VCOS_EAGAIN;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(VCOS_SEMAPHORE_T *sem) {
   if (down_trylock(sem) != 0)
      return VCOS_EAGAIN;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(VCOS_SEMAPHORE_T *sem,
                                    const char *name,
                                    VCOS_UNSIGNED initial_count) {
   sema_init(sem, initial_count);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(VCOS_SEMAPHORE_T *sem) {
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(VCOS_SEMAPHORE_T *sem) {
   up(sem);
   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/

VCOS_INLINE_IMPL
VCOS_THREAD_T *vcos_thread_current(void) {
   /* Not implemented - the types don't work */
   vcos_assert(0);
   return NULL;
}


VCOS_INLINE_IMPL
void vcos_sleep(uint32_t ms) {
   msleep(ms);
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
VCOS_STATUS_T vcos_mutex_create(VCOS_MUTEX_T *m, const char *name) {
   mutex_init(m);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_delete(VCOS_MUTEX_T *m) {
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_lock(VCOS_MUTEX_T *m) {
   if (mutex_lock_interruptible(m) != 0)
      return VCOS_EAGAIN;
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_mutex_unlock(VCOS_MUTEX_T *m) {
   mutex_unlock(m);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(VCOS_MUTEX_T *m) {
   if (mutex_trylock(m) != 0)
      return 1; /* it was locked */
   mutex_unlock(m);
   /* it wasn't locked */
   return 0;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_mutex_trylock(VCOS_MUTEX_T *m) {
   if (mutex_trylock(m) == 0)
      return VCOS_SUCCESS;
   else
      return VCOS_EAGAIN;
}


/*
 * Events
 */

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(VCOS_EVENT_T *event, const char *debug_name)
{
   sema_init(event, 0);
   return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_signal(VCOS_EVENT_T *event)
{
   up(event);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(VCOS_EVENT_T *event)
{
   if (down_interruptible(event) != 0)
      return VCOS_EAGAIN;
   // Emulate a maximum count of 1 by removing any extra upness
   while (down_trylock(event) == 0) continue;
   return VCOS_SUCCESS;
}

VCOS_INLINE_DECL
VCOS_STATUS_T vcos_event_try(VCOS_EVENT_T *event)
{
   return (down_trylock(event) == 0) ? VCOS_SUCCESS : VCOS_EAGAIN;
}

VCOS_INLINE_IMPL
void vcos_event_delete(VCOS_EVENT_T *event)
{
}


VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) {
   return (VCOS_UNSIGNED)current->pid;
}


VCOS_INLINE_IMPL
int vcos_in_interrupt(void) {
   return in_interrupt();
}

#undef VCOS_ASSERT_LOGGING_DISABLE
#define VCOS_ASSERT_LOGGING_DISABLE 0

#endif /* VCOS_INLINE_BODIES */

#include "interface/vcos/generic/vcos_common.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"

#endif /* VCOS_PLATFORM_H */

