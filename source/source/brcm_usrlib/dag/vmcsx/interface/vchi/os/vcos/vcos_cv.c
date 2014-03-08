/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

File     :  $File$
Revision :  $Revision$

VCHI condition-variable "lite" in terms of VCOS
=============================================================================*/

/*
 * VCHI's condition variable "lite" implementation in terms of
 * VCOS primitives. These are not the same as POSIX condition
 * variables.
 */

#include "interface/vchi/os/os.h"
#include <stdio.h>

#define MAX_COND 200

typedef struct cond_waiter {
   struct cond_waiter *next;
   VCOS_EVENT_T latch;
} COND_WAITER_T;

struct opaque_os_cond_t {
   COND_WAITER_T *waiters;
   OS_SEMAPHORE_T *semaphore;
};

// sem_latch protects the os_cond structure and os_cond_next
static VCOS_MUTEX_T cond_latch;
static uint32_t os_cond_next = 0;
static struct opaque_os_cond_t os_cond[ MAX_COND ];

#ifndef NDEBUG
static int vcos_cv_inited;
#endif

void os_cond_init(void)
{
   vcos_assert(vcos_cv_inited++ == 0);
   vcos_demand(vcos_mutex_create(&cond_latch,NULL) == VCOS_SUCCESS);
}

/***********************************************************
 * Name: os_cond_create
 *
 * Arguments:  OS_COND_T *condition
 *             OS_SEMAPHORE_T *semaphore
 *
 * Description: Routine to create a condition variable
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_create( OS_COND_T *condition, OS_SEMAPHORE_T *semaphore )
{
   int32_t success = -1;
   int i;
   
   os_assert(vcos_cv_inited);

   if( condition && semaphore )
   {
      uint32_t n;
      vcos_mutex_lock(&cond_latch);

      // start searching from os_cond_next
      n = os_cond_next;
      for (i=0; i<MAX_COND; i++)
      {
         if ( os_cond[n].semaphore == NULL )
            break;
         if ( ++n == MAX_COND )
            n = 0;
      }

      if ( i < MAX_COND )
      {
         OS_COND_T cond = &os_cond[n];
         cond->waiters = NULL;
         cond->semaphore = semaphore;
         *condition = cond;
         success = 0;
         if ( ++n == MAX_COND )
            n = 0;
         os_cond_next = n;
      }

      os_assert( success == 0 );
      vcos_mutex_unlock( &cond_latch );
   }
   
   return success;
}

/***********************************************************
 * Name: os_cond_destroy
 *
 * Arguments:  OS_COND_T *cond
 *
 * Description: Routine to destroy a condition variable
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_destroy( OS_COND_T *cond )
{
   int32_t success = 0;
   
   if (cond)
   {
      os_assert((*cond)->waiters == NULL);
      
      vcos_mutex_lock( &cond_latch );

      (*cond)->semaphore = NULL;
      os_cond_next = (*cond) - os_cond;
      
      os_assert(os_cond_next < MAX_COND);
      success = 0;
      
      os_assert( success == 0 );
      vcos_mutex_unlock( &cond_latch );
   }

   return success;
}

/***********************************************************
 * Name: os_cond_signal
 *
 * Arguments:  OS_COND_T *cond
 *
 * Description: Routine to signal at least one thread
 *              waiting on a condition variable. The
 *              caller must say whether they have obtained
 *              the semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_signal( OS_COND_T *cond, bool_t sem_claimed )
{
   int32_t success = -1;
   
   if (cond)
   {
      COND_WAITER_T *w;
      
      // Ensure the condvar's semaphore is claimed for thread-safe access
      if (!sem_claimed)
      {
         success = os_semaphore_obtain((*cond)->semaphore);
         os_assert(success == 0);
      }
      w = (*cond)->waiters;
      if (w)
      {
         // Wake the first person waiting
         vcos_event_signal(&w->latch);
         (*cond)->waiters = w->next;
         
      }
      success = 0;
      if (!sem_claimed)
      {
         success = os_semaphore_release((*cond)->semaphore);
      }
      os_assert(success == 0);
   }

   return success;
}

/***********************************************************
 * Name: os_cond_broadcast
 *
 * Arguments:  OS_COND_T *cond
 *             bool_t sem_claimed
 *
 * Description: Routine to signal all threads waiting on
 *              a condition variable. The caller must
 *              say whether they have obtained the semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_broadcast( OS_COND_T *cond, bool_t sem_claimed )
{
   int32_t success = -1;
   
   if (cond)
   {
      COND_WAITER_T *w;
      
      // Ensure the condvar's semaphore is claimed for thread-safe access
      if (!sem_claimed)
      {
         success = os_semaphore_obtain((*cond)->semaphore);
         os_assert(success == 0);
      }
      for (w = (*cond)->waiters; w; w = w->next)
      {
         vcos_event_signal(&w->latch);
      }
      (*cond)->waiters = NULL;
      
      success = 0;
      if (!sem_claimed)
      {
         success = os_semaphore_release((*cond)->semaphore);
      }
      os_assert(success == 0);
   }
   
   return success;
}

/***********************************************************
 * Name: os_cond_wait
 *
 * Arguments:  OS_COND_T *cond,
 *             OS_SEMAPHORE_T *semaphore
 *
 * Description: Routine to wait for a condition variable
 *              to be signalled. Semaphore is released
 *              while waiting. The same semaphore must
 *              always be used.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_wait( OS_COND_T *cond, OS_SEMAPHORE_T *semaphore )
{
   int32_t success = -1;
    
   if (cond && semaphore)
   {
      COND_WAITER_T w;
      COND_WAITER_T *p, **prev;
       
      // Check the API is being followed
      os_assert((*cond)->semaphore == semaphore && os_semaphore_obtained(semaphore));
      
      // Fill in a new waiter structure allocated on our stack
      w.next = NULL;
      vcos_demand(vcos_event_create(&w.latch, NULL) == VCOS_SUCCESS);
      
      // Add it to the end of the condvar's wait queue (we wake first come, first served)
      prev = &(*cond)->waiters;
      p = (*cond)->waiters;
      while (p)
         prev = &p->next, p = p->next;
      *prev = &w;

      // Ready to go to sleep now
      success = os_semaphore_release(semaphore);
      os_assert(success == 0);

      vcos_event_wait(&w.latch);

      success = os_semaphore_obtain(semaphore);
      os_assert(success == 0);
   }
   
   return success;
}

