/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - Mobcom RTOS types
=============================================================================*/

/* Do not include this file directly - instead include it via vcos.h */

/** @file
  *
  * Mobcom RTOS implementation of VCOS.
  *
  */

#ifndef VCOS_PLATFORM_H
#define VCOS_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "mobcom_types.h"
#include "consts.h"
#include "ostask.h"
#include "ostimer.h"
#include "ossemaphore.h"
#include "osevent.h"
#include "osinterrupt.h"
#include "irqctrl.h"
#include "timer.h"
#include "vcinclude/hardware.h"
#include "vcfw/drivers/chip/v3d.h"
#include <errno.h>
#include "assert.h"

/* Using critical sections for VCOS_MUTEX_T seems to break
 * VCHI. So for now use semaphores with a count of 1.
 */
typedef struct VCOS_SEMAPHORE_T
{
    Semaphore_t sem;
} VCOS_SEMAPHORE_T;

//typedef VCOS_SEMAPHORE_T VCOS_NAMED_SEMAPHORE_T;

typedef struct VCOS_LLTHREAD_T
{
    Task_t      thread;
    void*       tls;
} VCOS_LLTHREAD_T;	 

typedef struct VCOS_TIMER_T
{
    Timer_t     h;            /**< Handle to timer queue timer */
} VCOS_TIMER_T;

typedef struct VCOS_EVENT_T
{
    Event_t     h;
} VCOS_EVENT_T;

typedef uint32_t                        VCOS_TLS_KEY_T;
typedef uint32_t                        VCOS_UNSIGNED;
typedef uint32_t                        VCOS_OPTION;
typedef VCOS_EVENT_T                    VCOS_EVENT_FLAGS_T;

/* Event flag operations */
#define VCOS_OR							OSEVENT_OPTION_OR
#define VCOS_AND						OSEVENT_OPTION_AND
#define VCOS_AND_CONSUME				OSEVENT_OPTION_AND_CLEAR
#define VCOS_OR_CONSUME					OSEVENT_OPTION_OR_CLEAR

#define VCOS_HAVE_RTOS					1
#define VCOS_HAVE_TIMER					0
#define VCOS_HAVE_SEMAPHORE				1
#define VCOS_HAVE_EVENT					1

#define VCOS_SUSPEND                    -1
#define VCOS_NO_SUSPEND                 0

#define VCOS_THREAD_PRI_MIN             THREAD_PRIORITY_IDLE
#define VCOS_THREAD_PRI_MAX             THREAD_PRIORITY_TIME_CRITICAL

// Task priorities
#define VCOS_THREAD_PRI_MIN             1
#define VCOS_THREAD_PRI_MAX             15
#define VCOS_THREAD_PRI_NORMAL          10
#define VCOS_THREAD_PRI_BELOW_NORMAL    12
#define VCOS_THREAD_PRI_ABOVE_NORMAL    8
#define VCOS_THREAD_PRI_HIGHEST         VCOS_THREAD_PRI_MIN
#define VCOS_THREAD_PRI_LOWEST          VCOS_THREAD_PRI_MAX
#define VCOS_THREAD_PRI_INCREASE        -1


#define _VCOS_AFFINITY_DEFAULT          0
#define _VCOS_AFFINITY_MASK             0xf0000000

#define VCOS_TICKS_PER_SECOND           1000

#define VCOS_START                      1
#define VCOS_NO_START                   0
#define VCOS_DEFAULT_STACK_SIZE         8192

/* We can't set the stack address to be used by a newly created thread on this platform */
#define VCOS_CAN_SET_STACK_ADDR         0

typedef void (*VCOS_ISR_HANDLER_T)(VCOS_UNSIGNED vecnum);

VCOS_INLINE_DECL void*  _vcos_tls_thread_ptr_get(void);
VCOS_INLINE_DECL void   _vcos_tls_thread_ptr_set(void *p);
VCOS_INLINE_DECL void	vcos_set_interrupt_mask(VCOS_UNSIGNED vec, VCOS_UNSIGNED mask);

/*
 * spinlock stuff (for the spinlock semaphore and atomic flags)
 */
VCOS_INLINE_DECL int vcos_mobcom_rtos_spinlock_acquire(void);
VCOS_INLINE_DECL void vcos_mobcom_rtos_spinlock_release(int);

#define _VCOS_OS_LOCK_STATE				VCOS_UNSIGNED vcos_mobcom_rtos_spinlock_ei
#define _VCOS_OS_DISABLE()				vcos_mobcom_rtos_spinlock_ei = vcos_mobcom_rtos_spinlock_acquire()
#define _VCOS_OS_RESTORE()				vcos_mobcom_rtos_spinlock_release(vcos_mobcom_rtos_spinlock_ei)
#define _VCOS_OS_RESTORE_AND_SUSPEND()	do {																	\
											VCOS_LLTHREAD_T *thread = vcos_llthread_current();					\
											thread->suspended = 1;												\
											vcos_mobcom_rtos_spinlock_release(vcos_mobcom_rtos_spinlock_ei);	\
										while (thread->suspended);

#include "interface/vcos/generic/vcos_latch_from_sem.h"
#include "interface/vcos/generic/vcos_joinable_thread_from_plain.h"
#include "interface/vcos/generic/vcos_generic_named_sem.h"
#include "interface/vcos/generic/vcos_generic_reentrant_mtx.h"
#include "vcfw/vcos/vcos_spinlock_atomic_flags.h"

#ifdef VCOS_INLINE_BODIES

/***********************************************************
 *
 * TLS
 *
 ***********************************************************/
VCOS_INLINE_IMPL
void* _vcos_tls_thread_ptr_get(void)
{
    return OSTASK_TlsPtrGet();
}

VCOS_INLINE_IMPL
void _vcos_tls_thread_ptr_set(void *p)
{
    OSTASK_TlsPtrSet(p);
}

/***********************************************************
 *
 * Counted Semaphores
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_wait(
    VCOS_SEMAPHORE_T*   sem) 
{
    if (OSSTATUS_SUCCESS != OSSEMAPHORE_Obtain(sem->sem, TICKS_FOREVER))
        return VCOS_EINVAL; 
    else
        return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_trywait(
    VCOS_SEMAPHORE_T*   sem) 
{
    if (OSSTATUS_SUCCESS != OSSEMAPHORE_Obtain(sem->sem, TICKS_FOREVER))
        return VCOS_EINVAL; 
    else
        return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_create(
    VCOS_SEMAPHORE_T*   sem,
    const char*         name,
    VCOS_UNSIGNED       initial_count) 
{
   sem->sem = OSSEMAPHORE_Create(initial_count, OSSUSPEND_PRIORITY);
    
   if (sem->sem)
      return VCOS_SUCCESS;
   else
      return VCOS_EINVAL;
}

VCOS_INLINE_IMPL
void vcos_semaphore_delete(
    VCOS_SEMAPHORE_T*   sem) 
{
    OSSEMAPHORE_Destroy(sem->sem);
    
    sem->sem = NULL;
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_semaphore_post(
    VCOS_SEMAPHORE_T*   sem)
{
   if (!sem->sem)
      vcos_assert(0);

   vcos_assert(OSSTATUS_SUCCESS == OSSEMAPHORE_Release(sem->sem));

   return VCOS_SUCCESS;
}

/***********************************************************
 *
 * Threads
 *
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_LLTHREAD_T *vcos_llthread_current(void) 
{
    return (VCOS_LLTHREAD_T*)OSTASK_GetCurrentTask();
}

VCOS_INLINE_IMPL
void vcos_llthread_resume(
    VCOS_LLTHREAD_T*    thread) 
{
    OSTASK_Resume(&thread->thread);
}

VCOS_INLINE_IMPL
void vcos_sleep(
    uint32_t        ms) 
{
    OSTASK_Sleep(ms);
}

VCOS_INLINE_IMPL
void vcos_thread_set_priority(
    VCOS_THREAD_T*  thread, 
    VCOS_UNSIGNED   p) 
{
    OSTASK_ChangePriority(&thread->thread.thread, (TPriority_t)p);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_priority(
    VCOS_THREAD_T*  thread) 
{
    return OSTASK_GetPriority(&thread->thread);
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_thread_get_affinity(
    VCOS_THREAD_T*  thread) 
{
    return _VCOS_AFFINITY_DEFAULT;
}

VCOS_INLINE_IMPL
void vcos_thread_set_affinity(
    VCOS_THREAD_T*  thread, 
    VCOS_UNSIGNED   affinity) 
{
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_process_id_current(void) 
{
#ifdef CLIENT_THREAD_IS_PROCESS
	void *tls = _vcos_tls_thread_ptr_get();  //use as pid 
	(uint64_t) tls;
#else
    return 0;
#endif
}

VCOS_INLINE_IMPL
void vcos_thread_relinquish(void) 
{
    OSTASK_Relinquish();
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_change_preemption(
    VCOS_UNSIGNED   pe) 
{
	if (pe)
		OSTASK_RestorePreemption();
	else
		OSTASK_DisablePreemption();

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
void _vcos_thread_sem_wait(void) 
{
    VCOS_THREAD_T *t = vcos_thread_current();
   
    vcos_semaphore_wait(&t->suspend);
}

VCOS_INLINE_IMPL
void _vcos_thread_sem_post(
    VCOS_THREAD_T*  thread) 
{
    vcos_semaphore_post(&thread->suspend);
}

VCOS_INLINE_IMPL
int vcos_mutex_is_locked(
    VCOS_MUTEX_T*   mutex) 
{
	assert(0);
}

VCOS_INLINE_IMPL
int vcos_in_interrupt(void)
{
	return 0;
}

VCOS_INLINE_IMPL
int vcos_strcasecmp(
    const char* s1, 
    const char* s2) 
{
	return strcmp(s1,s2);
}

/***********************************************************
 *
 * Timers
 *
 * Note: Using Windows timer queues you don't seem to be able
 * to create a timer (which might fail due to lack of resources)
 * and then later start it,
 ***********************************************************/
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_timer_create(
    VCOS_TIMER_T*   timer,
    const char*     name,
    void            (*expiration_routine)(void *context),
    void*           context) 
{
    vcos_assert(timer);
    vcos_assert(expiration_routine);

    timer->h = OSTIMER_Create((TimerEntry_t)expiration_routine, 0, TICKS_ONE_SECOND, 0);

    if (timer->h)
		return VCOS_SUCCESS;
    else
		return VCOS_EINVAL;
}

VCOS_INLINE_IMPL
void vcos_timer_set(
    VCOS_TIMER_T*   timer, 
    VCOS_UNSIGNED   delay_ms) 
{
    OSTIMER_Reconfig(timer->h, delay_ms, NO_REPEAT_TIME, TRUE);
}

VCOS_INLINE_IMPL
void vcos_timer_set_ms(
    VCOS_TIMER_T*   timer, 
    VCOS_UNSIGNED   ms) 
{
    vcos_timer_set(timer, ms);
}

VCOS_INLINE_IMPL
void vcos_timer_cancel(
    VCOS_TIMER_T*   timer) 
{
    /* delete the timer, and wait for any ongoing timer function calls to complete */
    vcos_assert(timer);
    vcos_assert(timer->h);

    OSTIMER_Destroy(timer->h);

    timer->h = NULL;
}

VCOS_INLINE_IMPL
void vcos_timer_reset(
    VCOS_TIMER_T*   timer, 
    VCOS_UNSIGNED   delay_ms) 
{
	vcos_timer_cancel(timer);
	vcos_timer_set(timer, _VCOS_MS_TO_TICKS(delay_ms));
}

VCOS_INLINE_IMPL
void vcos_timer_delete(
    VCOS_TIMER_T*   timer) 
{
    vcos_assert(timer);

    OSTIMER_Destroy(timer->h);

    timer->h = NULL;
}

/*
 * Events
 */
VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_create(
    VCOS_EVENT_T*   event, 
    const char*     debug_name)
{
    event->h = OSEVENT_Create((EName_t)debug_name);
    return event->h ? VCOS_SUCCESS : VCOS_ENOMEM;
}

VCOS_INLINE_IMPL
void vcos_event_signal(
    VCOS_EVENT_T*   event)
{
    OSEVENT_Set(event->h);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_wait(
    VCOS_EVENT_T*   event)
{
    OSEVENT_Obtain(event->h, TICKS_FOREVER);
	return VCOS_SUCCESS;
}

VCOS_INLINE_IMPL
void vcos_event_delete(
    VCOS_EVENT_T*   event)
{
    OSEVENT_Destroy(event->h);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_create(
	VCOS_EVENT_FLAGS_T*	flags, 
	const char*			name) 
{
    flags->h = OSEVENT_Create((EName_t)name);
    return flags->h ? VCOS_SUCCESS : VCOS_ENOMEM;
}

VCOS_INLINE_IMPL
void vcos_event_flags_delete(
	VCOS_EVENT_FLAGS_T*	flags) 
{
    OSEVENT_Destroy(flags->h);
}

VCOS_EXPORT
void vcos_event_flags_set(
	VCOS_EVENT_FLAGS_T*	flags,
    VCOS_UNSIGNED		events,
    VCOS_OPTION			op) 
{
	OSEVENT_SetEvents(flags->h, events, op);
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_event_flags_get(
	VCOS_EVENT_FLAGS_T*	flags,
	VCOS_UNSIGNED		requested_events,
	VCOS_OPTION			op,
	VCOS_UNSIGNED		ms_suspend,
	VCOS_UNSIGNED*		retrieved_events) 
{
	OSStatus_t  status;
	status = OSEVENT_ObtainEvents(
				flags->h, 
				requested_events, 
				op, 
				retrieved_events,
				ms_suspend); 
	return ((status == OSSTATUS_SUCCESS) ? VCOS_SUCCESS : VCOS_ENOMEM);				
}

/*
 * 	Others
 */
VCOS_INLINE_IMPL
unsigned long vcos_get_free_mem(void) 
{
    vcos_assert(0); /* not yet implemented */
    return 0;
}

VCOS_INLINE_IMPL
uint32_t vcos_getmicrosecs(void) 
{
    return OSTIMER_RetrieveClock()*1000;
}

VCOS_INLINE_IMPL
uint32_t vcos_getmillisecs(void) 
{
    return OSTIMER_RetrieveClock();
}

/***********************************************************
 *
 * Interrupt handling
 *
 ***********************************************************/
#define V3D_HISR_STACK_SIZE 1024

static  Interrupt_t         v3dHisr;
static  Event_t             v3dStateEvent;
static  void                (*v3d_isr_callback)(int) = NULL;

static void _v3d_lisr(void)
{
#if defined(_ATHENA_) && (CHIP_REVISION==20)
	// SW patch for Async APB bridge HW bug
    *((volatile unsigned int *)(0x8950438)) = 0;
    if (*((volatile unsigned int *)(0x8950438)) != 0) 
        *((volatile unsigned int *)(0x8950438)) = 0;
#endif
	
	// Disable interrupt
    IRQ_Clear(MM_V3D_IRQ);
    IRQ_Disable(MM_V3D_IRQ);
    
    OSINTERRUPT_Trigger(v3dHisr);
}

static void v3d_hisr(void)
{
    IRQ_Clear(MM_V3D_IRQ);

    if (v3d_isr_callback)
	{
        v3d_isr_callback(INTERRUPT_3D);
	}
    else
	{
        uint32_t flags = v3d_read(INTCTL);
        v3d_write(INTCTL, flags);			// Clear end of frame interrupt
	}

    IRQ_Enable(MM_V3D_IRQ);
}

VCOS_INLINE_IMPL
void vcos_set_interrupt_mask(
    VCOS_UNSIGNED   vec,
    VCOS_UNSIGNED   mask)
{
    if (vec == INTERRUPT_3D)
        v3d_write(INTENA,  mask);  
}

VCOS_INLINE_IMPL
VCOS_STATUS_T vcos_register_isr(
    VCOS_UNSIGNED       vec,
    VCOS_ISR_HANDLER_T  handler,
    VCOS_ISR_HANDLER_T* old_handler) 
{
    if (vec == INTERRUPT_3D)
    {
        uint32_t flags = v3d_read(INTCTL);
        v3d_write(INTCTL, flags);          // Clear end of frame interrupt

        // initialize interrupt
        IRQ_Register(MM_V3D_IRQ, _v3d_lisr);
        IRQ_Enable(MM_V3D_IRQ);

        // create HISR
        v3dHisr = OSINTERRUPT_Create(
                    (IEntry_t)(v3d_hisr), 
                    (IName_t)"VC4LiteV3DHISR", 
                    IPRIORITY_MIDDLE,
                    V3D_HISR_STACK_SIZE);

        if (old_handler)
            *old_handler = (VCOS_ISR_HANDLER_T)v3d_isr_callback;

        v3d_isr_callback = (void (*)(int))handler;

        return VCOS_SUCCESS;
    }
    else
        return VCOS_EINVAL;
}

VCOS_INLINE_IMPL
VCOS_UNSIGNED vcos_int_disable(void) 
{
    return (VCOS_UNSIGNED)OSINTERRUPT_DisableAll();
}

VCOS_INLINE_IMPL
void vcos_int_restore(
    VCOS_UNSIGNED   previous) 
{
    OSINTERRUPT_Restore(previous);
}

/***********************************************************
 *
 * Spinlock
 *
 ***********************************************************/
VCOS_INLINE_IMPL
int vcos_mobcom_rtos_spinlock_acquire(void)
{
   return vcos_int_disable();
}

VCOS_INLINE_IMPL
void vcos_mobcom_rtos_spinlock_release(int ei)
{
	vcos_int_restore(ei);
}

#endif /* VCOS_INLINE_BODIES */

VCOS_INLINE_DECL void _vcos_thread_sem_wait(void);
VCOS_INLINE_DECL void _vcos_thread_sem_post(VCOS_THREAD_T *thread);

#include "interface/vcos/generic/vcos_common.h"
#include "interface/vcos/generic/vcos_mem_from_malloc.h"
#include "interface/vcos/generic/vcos_generic_quickslow_mutex.h"

#ifdef __cplusplus
}
#endif

#endif /* VCOS_PLATFORM_H */
