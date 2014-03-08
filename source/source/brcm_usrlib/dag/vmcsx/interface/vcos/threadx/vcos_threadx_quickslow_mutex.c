/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Latch
Module   :  ThreadX
File     :  $RCSfile: rtos_nucleus_latch.c,v $
Revision :  $Revision: #1 $

FILE DESCRIPTION
Modified version of rtos_threadx_latch to have quick and slow modes.
=============================================================================*/

#include <stddef.h>
#include <string.h>

#include "vcinclude/common.h"
#include "tx_api.h"
#include "tx_thread.h"

#include "interface/vcos/vcos.h"
#include "interface/vcos/threadx/vcos_platform.h"

VCOS_INLINE_DECL
void vcos_quickslow_mutex_internal_spin_lock(VCOS_QUICKSLOW_MUTEX_T *m);

VCOS_INLINE_DECL
void vcos_quickslow_mutex_internal_spin_unlock(VCOS_QUICKSLOW_MUTEX_T *m);

/******************************************************************************
  Local typedefs
 *****************************************************************************/

#define LATCH_WOKEN_VALUE __LINE__

typedef struct VCOS_QUICKSLOW_MUTEX_WAITING VCOS_QUICKSLOW_MUTEX_WAITING_T;

struct VCOS_QUICKSLOW_MUTEX_WAITING
{
   VCOS_QUICKSLOW_MUTEX_WAITING_T* next;
   void *task;
   uint32_t woken;
};

#define quickslow_mutex_locked() ((VCOS_QUICKSLOW_MUTEX_T)1)
#define quickslow_mutex_unlocked() ((VCOS_QUICKSLOW_MUTEX_T)0)

/******************************************************************************
  Static data
 *****************************************************************************/

/******************************************************************************
  Externs TX Functions
 *****************************************************************************/

/******************************************************************************
  Static Functions
 *****************************************************************************/

/******************************************************************************
  Global Functions
 *****************************************************************************/
extern void vcos_quickslow_mutex_wait_impl( VCOS_QUICKSLOW_MUTEX_T *m );

/***********************************************************
* Name: vcos_quickslow_mutex_lock_impl
*
* Arguments:
*       VCOS_QUICKSLOW_MUTEX_T *m - mutex to lock
*
* Description: Routine to lock a mutex
*
* Returns: void
*
***********************************************************/
void vcos_quickslow_mutex_lock_impl( VCOS_QUICKSLOW_MUTEX_T *m )
{
   vcos_quickslow_mutex_internal_spin_lock(m);

   //is the mutex unlocked?
   if (quickslow_mutex_unlocked() == *m)
   {
      //yes, lock the mutex
      *m = quickslow_mutex_locked();

      vcos_quickslow_mutex_internal_spin_unlock(m);
      return;
   }

   //wait for the mutex
   vcos_quickslow_mutex_wait_impl( m );
}


/***********************************************************
* Name: vcos_quickslow_mutex_unlock_impl
*
* Arguments:
*       VCOS_QUICKSLOW_MUTEX_T *m - mutex to unlock
*
* Description: Routine to unlock a mutex
*
* Returns: void
*
***********************************************************/
void vcos_quickslow_mutex_unlock_impl( VCOS_QUICKSLOW_MUTEX_T *m )
{
   VCOS_QUICKSLOW_MUTEX_WAITING_T *waiting;
   UINT status;

   vcos_assert(quickslow_mutex_unlocked() != *m);     //Was commented out for rtos_latch

   if (quickslow_mutex_locked() == *m)
   {
      *m = quickslow_mutex_unlocked();

      vcos_quickslow_mutex_internal_spin_unlock(m);
      return;
   }

   //store the wait struct
   waiting = (VCOS_QUICKSLOW_MUTEX_WAITING_T*)*m;
   *m = (VCOS_QUICKSLOW_MUTEX_T)waiting->next;

   vcos_assert(!waiting->woken);

   //if we are in an isr, store the woken value for debug
   vcos_assert(!tx_in_interrupt());
   if (tx_in_interrupt())
   {
      waiting->woken=LATCH_WOKEN_VALUE;

      tx_thread_resume(waiting->task);

      vcos_quickslow_mutex_internal_spin_unlock(m);
      return;
   }

   waiting->woken=LATCH_WOKEN_VALUE;

   vcos_quickslow_mutex_internal_spin_unlock(m);

   vcos_assert((NULL == tx_thread_identify()) || (waiting->task != tx_thread_identify()));
   status = tx_thread_resume((TX_THREAD *)waiting->task);
   vcos_assert(TX_SUCCESS == status);
}

/***********************************************************
* Name: vcos_quickslow_mutex_wait
*
* Arguments:
*       VCOS_QUICKSLOW_MUTEX_T *m - mutex to wait for
*
* Description: Routine to suspend a task
*
* Returns: void
*
***********************************************************/
void vcos_quickslow_mutex_wait_impl( VCOS_QUICKSLOW_MUTEX_T *m )
{
   TX_THREAD *cur = tx_thread_identify();
   VCOS_QUICKSLOW_MUTEX_WAITING_T *next = NULL;
   VCOS_QUICKSLOW_MUTEX_WAITING_T *last = NULL;
   VCOS_QUICKSLOW_MUTEX_WAITING_T waiting = { (VCOS_QUICKSLOW_MUTEX_WAITING_T*)quickslow_mutex_locked(),
                                    cur,0 };

   vcos_assert(!tx_in_interrupt());
   vcos_assert(cur);/* Cannot suspend if no task available */

   for ( next = (VCOS_QUICKSLOW_MUTEX_WAITING_T*)*m; quickslow_mutex_locked() != (VCOS_QUICKSLOW_MUTEX_T)next; next = next->next )
   {
      last = next;
   }

   if (last)
   {
      last->next = &waiting;
   }
   else
   {
      *m = (VCOS_QUICKSLOW_MUTEX_T)&waiting;
   }

   /* N.B. This is a partial expansion of tx_thread_suspend, so this
      implementation straddles the API boundary. */

   /* Set the state to suspended.  */
   cur -> tx_thread_state =    TX_SUSPENDED;

   /* Set the suspending flag. */
   cur -> tx_thread_suspending =  TX_TRUE;

   /* Setup for no timeout period.  */
   cur -> tx_thread_timer.tx_timer_internal_remaining_ticks =  0;

   /* Temporarily disable preemption.  */
   TX_PREEMPT_DISABLE

   /* Restore interrupts.  */
   vcos_quickslow_mutex_internal_spin_unlock(m);

   /* Call actual thread suspension routine.  */
   _tx_thread_system_suspend(cur);

   vcos_assert(waiting.woken);
}

/********************************** End of file ******************************************/
