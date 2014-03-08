/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <assert.h>
#include <string.h>

#include "vcinclude/hardware.h"
#include "vcfw/rtos/rtos.h"

#include "vcfw/drivers/chip/systimer.h"
#include "vcfw/drivers/chip/intctrl.h"
#include "vcfw/drivers/chip/multicore_sync.h"

/******************************************************************************
  Typedefs
 *****************************************************************************/

//#define RTOS_TIMER_SANITY_CHECK

#define PREFER_TIMER_INT                2
/* Nucleus uses timer 0
   Univ button presses use timer 1
   univ_create_timer used timer 2
   MetaWare profiling uses timer 3
*/

typedef struct
{
   //multicore semaphore to protect this API over 2 cores
   MULTICORE_SYNC_SEMAPHORE_T semaphore;

   //the actual timer queue
   RTOS_TIMER_T *queue;

   //Static pointers to the drivers
   const INTCTRL_DRIVER_T *intctrl_driver;
   const MULTICORE_SYNC_DRIVER_T *multicore_sync_driver;
   const SYSTIMER_DRIVER_T *systimer_driver;

   //Handles from the opened drivers
   DRIVER_HANDLE_T intctrl_handle;
   DRIVER_HANDLE_T multicore_sync_handle;
   DRIVER_HANDLE_T systimer_handle;
   
   //Cancelled interrupts must be cleared from the interrupt handler
   int cancelled_timer;

} RTOS_TIMER_STATE_T;


//#define DO_TIMER_LOGGING
#ifdef DO_TIMER_LOGGING

   #define LOG_SIZE (1<<5)
   #define LOG_QUEUE_DEPTH (8)

   typedef enum {
      FUNCTION_RESET,
      FUNCTION_SET,
      FUNCTION_CANCEL,
      FUNCTION_EAT,
   } TIMER_LOGGING_FN_T;

   typedef struct {
      TIMER_LOGGING_FN_T func;
      int alarm;
      int delay;
      int core;
      RTOS_TIMER_T *timer;
      int next;
      RTOS_TIMER_T *queue[LOG_QUEUE_DEPTH];
   } TIMER_LOG_RECORD_T;

   struct _temp {
      int current;
      TIMER_LOG_RECORD_T rec[LOG_SIZE];
   } timer_log;

   void do_log(int function, int time, int delay, RTOS_TIMER_T *timer, RTOS_TIMER_T *next)
   {
      TIMER_LOG_RECORD_T *rec = timer_log.rec+timer_log.current;
      int i, d = 0;

      rec->func = function;
      rec->alarm = time + delay;
      rec->delay = delay;
      rec->core = 0;//rtos_get_cpu_number();
      rec->timer = timer;
      rec->next = next ? next->ticks : 0;

      for(i=0; i<LOG_QUEUE_DEPTH; i++)
      {
         rec->queue[i] = next;
         if(next)
         {
            d++;
            next = next->next;
         }
      }
      timer_log.current = (timer_log.current + 1) & (LOG_SIZE-1);

      {
         // try and spot if the list has got much longer or shorter than
         // last time.
         static int last;
         if(d-last < -1 || d-last>1)
            assert(0);
         last = d;
      }
   }

#endif // DO_TIMER_LOGGING

#ifdef DO_TIMER_LOGGING
#define TIMER_LOGGING(func, time, delay, timer, next) do_log((func), (time), (delay), (timer), (next))
#else
#define TIMER_LOGGING(func, time, delay, timer, next)
#endif

/******************************************************************************
  Static function prototypes
 *****************************************************************************/

static void rtos_timer_interrupt( const uint32_t isr_num );

static void rtos_timer_clear_interrupt( void );

static uint32_t rtos_timer_interrupt_pending( void );

static void rtos_timer_force_interrupt( void );

static int rtos_timer_overdue_( void );

#ifdef RTOS_TIMER_SANITY_CHECK
static void rtos_timer_list_check_( void );

static void rtos_timer_check_(void );
#endif

static void rtos_timer_launch_( void );

static RTOS_TIMER_T * rtos_timer_eat_queue( void );

static int32_t rtos_timer_set_( RTOS_TIMER_T *timer, const RTOS_TIMER_TIME_T delay );

static int32_t rtos_timer_cancel_( RTOS_TIMER_T *timer );

/******************************************************************************
  Static data
 *****************************************************************************/

//the rtos state
static RTOS_TIMER_STATE_T rtos_timer_state;

/******************************************************************************
  Global functions
 *****************************************************************************/

/***********************************************************
 * Name: rtos_timer_lib_init
 *
 * Arguments:
 *       void
 *
 * Description: Initialises this module
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_timer_lib_init( void )
{
   int32_t success = -1; //fail by default
   RTOS_LISR_T old_lisr = NULL;

   //reset the global state
   memset( &rtos_timer_state, 0, sizeof( rtos_timer_state ) );

   //Register the isr
   rtos_register_lisr( INTERRUPT_TIMER0 + PREFER_TIMER_INT, rtos_timer_interrupt, &old_lisr );

   //get a handle to the intctrl driver
   rtos_timer_state.intctrl_driver = intctrl_get_func_table();
   assert( NULL != rtos_timer_state.intctrl_driver );

   rtos_timer_state.multicore_sync_driver = multicore_sync_get_func_table();
   assert( NULL != rtos_timer_state.multicore_sync_driver );   

   rtos_timer_state.systimer_driver = systimer_get_func_table();
   assert( NULL != rtos_timer_state.systimer_driver );

   //open the intctrlr and get a handle
   success = rtos_timer_state.intctrl_driver->open( NULL, &rtos_timer_state.intctrl_handle );
   assert( success >= 0 );

   //if the previous isr wasn't this library,
   if (old_lisr != rtos_timer_interrupt)
   {
      //clear the isr (incase there was some already pending updates)
      rtos_timer_clear_interrupt();
   }

   //open the multicore_sync and get a handle
   success = rtos_timer_state.multicore_sync_driver->open( NULL, &rtos_timer_state.multicore_sync_handle );
   assert( success >= 0 );

   //enable the isr
   success = rtos_timer_state.intctrl_driver->set_imask( rtos_timer_state.intctrl_handle, INTERRUPT_TIMER0 + PREFER_TIMER_INT, 1 );
   assert( success >= 0 );

   //allocate a multicore_sync semaphore
   success = rtos_timer_state.multicore_sync_driver->create_semaphore( &rtos_timer_state.semaphore );   
   assert( success >= 0 );

   success = rtos_timer_state.systimer_driver->open( NULL, &rtos_timer_state.systimer_handle );   
   assert( success >= 0 );

   return success;
}


/***********************************************************
 * Name: rtos_timer_init
 *
 * Arguments:
 *       RTOS_TIMER_T *timer - timer structure to init
 *       RTOS_TIMER_DONE_OPERATION_T done - done function to callback when the timer expires
 *       void *priv - private data to pass the callback when the timer expires
 *
 * Description: Initialises TIMER with the given parameters. Returns zero on success
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_timer_init( RTOS_TIMER_T *timer, RTOS_TIMER_DONE_OPERATION_T done, void *priv)
{
   int32_t success = -1; //default is fail

   if ( (NULL != timer) && (NULL != done) )
   {
      memset( timer, 0, sizeof(*timer) );

      timer->done = done;
      timer->priv = priv;

      //success!
      success = 0;
   }
   else
   {
      assert( 0 );
   }

   return success;
}


/***********************************************************
 * Name: rtos_timer_is_running
 *
 * Arguments:
 *       RTOS_TIMER_T *timer - timer structure
 *
 * Description: Returns non-zero if TIMER is in the queue
 *
 * Returns: int (0 == timer is not in the qyey)
 *
 ***********************************************************/
int32_t rtos_timer_is_running( const RTOS_TIMER_T *timer )
{
   int32_t is_running = 0;
   RTOS_TIMER_T *timer_list_ptr = NULL;   

   //disable interrupts
   uint32_t sr = rtos_timer_state.intctrl_driver->disable_interrupts( rtos_timer_state.intctrl_handle );

   //get the semaphore
   rtos_timer_state.multicore_sync_driver->get_semaphore( rtos_timer_state.semaphore );

   //interate over the list
   for ( timer_list_ptr = rtos_timer_state.queue; timer_list_ptr != NULL ; timer_list_ptr = timer_list_ptr->next )
   {
      if (timer == timer_list_ptr)
      {
         is_running = 1;
         break;
      }
   }

   //release the semaphore
   rtos_timer_state.multicore_sync_driver->put_semaphore( rtos_timer_state.semaphore );

   //restore interrupts
   rtos_timer_state.intctrl_driver->restore_interrupts( rtos_timer_state.intctrl_handle, sr );   

   return is_running;
}



/***********************************************************
 * Name: rtos_timer_set
 *
 * Arguments:
 *       RTOS_TIMER_T *timer - ptr to the timer structure
         const RTOS_TIMER_TIME_T delay - time to delay in usecs
 *
 * Description: Adds TIMER to the queue to run TIMER->done(TIMER->priv) after DELAY microseconds
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_timer_set( RTOS_TIMER_T *timer, const RTOS_TIMER_TIME_T delay )
{
   int32_t success = -1; //fail by default
   uint32_t sr = 0;

   //disable interrupts
   sr = rtos_timer_state.intctrl_driver->disable_interrupts( rtos_timer_state.intctrl_handle );

   //get the semaphore
   rtos_timer_state.multicore_sync_driver->get_semaphore( rtos_timer_state.semaphore );
   
   success = rtos_timer_set_( timer, delay );

   //release the semaphore
   rtos_timer_state.multicore_sync_driver->put_semaphore( rtos_timer_state.semaphore );

   //restore interrupts
   rtos_timer_state.intctrl_driver->restore_interrupts( rtos_timer_state.intctrl_handle, sr );

   return success;
}


/***********************************************************
 * Name: rtos_timer_reset
 *
 * Arguments:
 *       void
 *
 * Description: TIMER is cancelled and then set.
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_timer_reset( RTOS_TIMER_T *timer, const RTOS_TIMER_TIME_T delay )
{
   int32_t success = -1;
   uint32_t sr = 0;

   //disable interrupts
   sr = rtos_timer_state.intctrl_driver->disable_interrupts( rtos_timer_state.intctrl_handle );

   //get the semaphore
   rtos_timer_state.multicore_sync_driver->get_semaphore( rtos_timer_state.semaphore );

   TIMER_LOGGING(FUNCTION_RESET, rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle), delay, timer, rtos_timer_state.queue);

   //cancel the timer
   success = rtos_timer_cancel_( timer );

   if ( success == 0 )
   {
      //restart the timer
      success = rtos_timer_set_( timer, delay );
   }

   //release the semaphore
   rtos_timer_state.multicore_sync_driver->put_semaphore( rtos_timer_state.semaphore );

   //restore interrupts
   rtos_timer_state.intctrl_driver->restore_interrupts( rtos_timer_state.intctrl_handle, sr );

   return success;
}



/***********************************************************
 * Name: rtos_timer_cancel
 *
 * Arguments:
 *       void
 *
 * Description: Cancel a timer that has already been added into the queue
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_timer_cancel( RTOS_TIMER_T *timer )
{
   int32_t success = 0; // can't fail
   uint32_t sr = 0;

   //disable interrupts
   sr = rtos_timer_state.intctrl_driver->disable_interrupts( rtos_timer_state.intctrl_handle );      

   //get the semaphore
   rtos_timer_state.multicore_sync_driver->get_semaphore( rtos_timer_state.semaphore );

   success = rtos_timer_cancel_( timer );

   //release the semaphore
   rtos_timer_state.multicore_sync_driver->put_semaphore( rtos_timer_state.semaphore );

   //restore interrupts
   rtos_timer_state.intctrl_driver->restore_interrupts( rtos_timer_state.intctrl_handle, sr );

   return success;
}

/******************************************************************************
 Static Functions
 *****************************************************************************/

/***********************************************************
 * Name: rtos_timer_interrupt
 *
 * Arguments:
 *       int isr_num - ths number of this isr
 *
 * Description: The isr that fires when the timer expires
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
static void rtos_timer_interrupt( const uint32_t isr_num )
{
   uint32_t sr = 0;
   RTOS_TIMER_T *expired_timer = NULL;

   //check that interrupts are not enabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   //get the semaphore
   rtos_timer_state.multicore_sync_driver->get_semaphore( rtos_timer_state.semaphore );

   //check that the isr is the expected number
   assert(INTERRUPT_TIMER0+PREFER_TIMER_INT == isr_num);

   //check that the timer is pending
   if(rtos_timer_interrupt_pending())
   {
      //clear the interrupt
      rtos_timer_clear_interrupt();

      // Occasionally we can cancel a timer too late and it goes off, therefore we want to ignore these
      // unless the next timer in the list is overdue (we can have multiple timers at the same time!)
      if(!rtos_timer_state.cancelled_timer || rtos_timer_overdue_())
      {
         //eat item(s) from the queue (i.e. dispatch it)
         expired_timer = rtos_timer_eat_queue();
      }
      else
      {
         rtos_timer_state.cancelled_timer = 0;
      }
   }

   //release the semaphore
   rtos_timer_state.multicore_sync_driver->put_semaphore( rtos_timer_state.semaphore );

   //if we have found a timer, expire it now
   if ( NULL != expired_timer )
   {
      expired_timer->next = 0;

      //call the timer callback
      expired_timer->done(expired_timer, expired_timer->priv);
   }   
}


/***********************************************************
 * Name: rtos_timer_clear_interrupt
 *
 * Arguments:
 *       void
 *
 * Description: Routine to clear the timer interrupt
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
static void rtos_timer_clear_interrupt( void )
{
   STCS = 1 << PREFER_TIMER_INT;

   //hack for VCIII as it requires the isr status flag to be read back first
   #if _VC_VERSION > 2
      {
         volatile long t = STCS; }
   #endif
   
   // Clear any forced interrupt as well
   rtos_timer_state.intctrl_driver->clear_interrupt( 
      rtos_timer_state.intctrl_handle, 
      0, 
      INTERRUPT_TIMER0 + PREFER_TIMER_INT);
   
}

/***********************************************************
 * Name: rtos_timer_interrupt_pending
 *
 * Arguments:
 *       void
 *
 * Description: Function to return if there is an isr already pending
 *
 * Returns: uint32_t - (1 == pending)
 *
 ***********************************************************/
static uint32_t rtos_timer_interrupt_pending( void )
{
   return (1 << PREFER_TIMER_INT) & STCS | rtos_timer_state.intctrl_driver->is_forced( 
                                             rtos_timer_state.intctrl_handle, 
                                             0, 
                                             INTERRUPT_TIMER0 + PREFER_TIMER_INT);
}


/***********************************************************
 * Name: rtos_timer_force_interrupt
 *
 * Arguments:
 *       void
 *
 * Description: Force the time interrupt to fire
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
static void rtos_timer_force_interrupt( void )
{
   // Force the interrupt in the interrupt controller
   rtos_timer_state.intctrl_driver->force_interrupt( 
      rtos_timer_state.intctrl_handle, 
      0, 
      INTERRUPT_TIMER0 + PREFER_TIMER_INT);
}


/***********************************************************
 * Name: rtos_timer_overdue_
 *
 * Arguments:
 *       void
 *
 * Description: Figure out if there is a timer that is overdue to fire
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
static int rtos_timer_overdue_(void)
{
   uint32_t overdue = 0;

   //check that interrupts are not enabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   //if there is no queue, there is no point looking any further
   if (NULL != rtos_timer_state.queue)
   {
      RTOS_TIMER_TICKS_T now = rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle );

      //the timer is overdue if the current queue has an expiry time in the past
      overdue = (rtos_timer_state.queue->ticks - rtos_timer_state.queue->offset) <= (now - rtos_timer_state.queue->offset);
   }

   return overdue;
}

#ifdef RTOS_TIMER_SANITY_CHECK
/***********************************************************
* Name: rtos_timer_list_check_
*
* Arguments:
*       void
*
* Description: Routine to sanity check the timer queue
*
* Returns: int ( < 0 is fail)
*
***********************************************************/
static void rtos_timer_list_check_( void )
{
   RTOS_TIMER_T *timer= rtos_timer_state.queue;
   RTOS_TIMER_T *x2 = timer;

   RTOS_TIMER_TICKS_T start = rtos_timer_state.queue ? rtos_timer_state.queue->ticks : 0;
   RTOS_TIMER_TICKS_T last = 0;

   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   //iterate over the timers in the queue and check that
   //they are in order and that there params are ok
   while (timer)
   {
      RTOS_TIMER_TICKS_T cur = 0;

      if (x2)
      {
         x2 = x2->next;
      }
      assert(timer->next != timer);
      assert(timer->next != rtos_timer_state.queue);
      assert(x2 != timer);

      cur = timer->ticks - start;

      if (last > cur)
      {
         //fire an assert if there is an issue
         assert( 0 );
      }

      last = cur;

      timer = timer->next;

      if (x2)
      {
         x2 = x2->next;
      }
   }
}


/***********************************************************
* Name: rtos_timer_check_
*
* Arguments:
*       void
*
* Description: Initialise the RTOS. In the case of 'no' RTOS, it does nothing
*
* Returns: int ( < 0 is fail)
*
***********************************************************/
static void rtos_timer_check_( void )
{
   //ensure interrupts are disabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   //check that a timer isn't overdue or if there is, make sure there is an interrupt pending
   assert(!rtos_timer_overdue_() || rtos_timer_interrupt_pending());

   //check the list
   rtos_timer_list_check_();
}

#endif //RTOS_TIMER_SANITY_CHECK

#ifdef RTOS_TIMER_SANITY_CHECK
   #define RTOS_TIMER_LIST_CHECK() rtos_timer_list_check_()
   #define RTOS_TIMER_CHECK()      rtos_timer_check_()
#else
   #define RTOS_TIMER_LIST_CHECK()
   #define RTOS_TIMER_CHECK()
#endif

/***********************************************************
 * Name: rtos_timer_launch_
 *
 * Arguments:
 *       void
 *
 * Description: Routine to launch the timer queue initially
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
static void rtos_timer_launch_( void )
{
   //ensure interrupts are disabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   RTOS_TIMER_LIST_CHECK();

   //check there is a queue...
   if ( NULL != rtos_timer_state.queue )
   {
      rtos_timer_state.systimer_driver->set_compare_match(rtos_timer_state.systimer_handle, PREFER_TIMER_INT, rtos_timer_state.queue->ticks);

      //if a timer is already overdue, force it to launch now
      if (rtos_timer_overdue_() && !rtos_timer_interrupt_pending())
      {
         rtos_timer_force_interrupt();
      }      
   }
}


/***********************************************************
 * Name: rtos_timer_eat_queue
 *
 * Arguments:
 *       void
 *
 * Description: Remove timers of the queue that have expired
 *
 * Returns: The expired timer, or NULL
 *
 ***********************************************************/
static RTOS_TIMER_T * rtos_timer_eat_queue( void )
{
   RTOS_TIMER_T *expired_timer;
   
   expired_timer = rtos_timer_state.queue;
   
   TIMER_LOGGING(FUNCTION_EAT, rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle ), 0, expired_timer, rtos_timer_state.queue);
   
   RTOS_TIMER_LIST_CHECK();

   //if we have found a timer, check it
   if ( NULL != expired_timer )
   {
      /* Check that the timer has not gone off early */
      assert( (rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle ) - expired_timer->offset) >= (expired_timer->ticks - expired_timer->offset) );
         
   }
   
   if (NULL != expired_timer)
   {
      rtos_timer_state.queue = expired_timer->next;
      rtos_timer_launch_();

      RTOS_TIMER_CHECK();
   }

   return expired_timer;

}


/***********************************************************
 * Name: rtos_timer_cancel_
 *
 * Arguments:
 *       void
 *
 * Description: Cancel a timer that has already been added into the queue
 *              Does not disable interrupts or take semaphore
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
static int32_t rtos_timer_cancel_( RTOS_TIMER_T *timer )
{
   int32_t success = 0; // can't fail

   //ensure interrupts are disabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   if ( NULL != timer )
   {
      RTOS_TIMER_T *t = NULL;      

      RTOS_TIMER_CHECK();
      
      TIMER_LOGGING(FUNCTION_CANCEL, rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle ), 0, timer, rtos_timer_state.queue);

      //if the current timer is next to expire, clean up after the interrupt and launch the queue again
      if (rtos_timer_state.queue == timer)
      {
         rtos_timer_state.queue = rtos_timer_state.queue->next;

         rtos_timer_state.systimer_driver->set_compare_match(rtos_timer_state.systimer_handle, PREFER_TIMER_INT, rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle )-1);

         // Make a note of the timer that was cancelled, then we can ignore it in the
         // interrupt handler if appropriate
         if(rtos_timer_interrupt_pending() && !rtos_timer_overdue_() )
            rtos_timer_state.cancelled_timer = 1;

         //launch the queue again
         rtos_timer_launch_();
         
         timer->next = 0;
      }
      else
      {
         //if there is anything in the timer queue
         if (rtos_timer_state.queue)
         {
            //iterate over the queue until we find the timer we want
            for ( t = rtos_timer_state.queue; NULL != t->next; t =t->next )
            {
               if (timer == t->next)
               {
                  t->next = timer->next;
                  timer->next = 0;
                  break;
               }
            }
         }
      }

      assert(timer->next == 0);

      RTOS_TIMER_CHECK();
   }

   return success;
}

/***********************************************************
 * Name: rtos_timer_set_
 *
 * Arguments:
 *       RTOS_TIMER_T *timer - ptr to the timer structure
         const RTOS_TIMER_TIME_T delay - time to delay in usecs
 *
 * Description: Adds TIMER to the queue to run TIMER->done(TIMER->priv) after DELAY microseconds
 *              Does not take interrupts or semaphore
 *
 * Returns: int32_t ( < 0 is fail)
 *
 ***********************************************************/
static int32_t rtos_timer_set_( RTOS_TIMER_T *timer, const RTOS_TIMER_TIME_T delay )
{
   int32_t success = -1; //fail by default

   //ensure interrupts are disabled
   assert( !rtos_timer_state.intctrl_driver->are_interrupts_enabled( rtos_timer_state.intctrl_handle ) );

   //param check
   if ( ( NULL != timer) && ((signed)delay > 0 ) )
   {
      RTOS_TIMER_TICKS_T now = 0;
      RTOS_TIMER_T *t = NULL;
      RTOS_TIMER_T *list = rtos_timer_state.queue;

      //sanity checks
      assert(timer->done);
      assert(!timer->next);

      // check this timer is not in the list already
      // if we hit this assert then we will almost certainly end up with
      // a loop in our list.
      while(list && list != timer)
         list = list->next;
      assert(list == NULL);
      
      if(list != NULL)
         return success;

      //setup the timer offset
      now = rtos_timer_state.systimer_driver->get_time_in_usecs( rtos_timer_state.systimer_handle );
      timer->offset = now;
      //calculate the expire time
      timer->ticks = now + delay;

      TIMER_LOGGING(FUNCTION_SET, now, delay, timer, rtos_timer_state.queue);
      
      RTOS_TIMER_CHECK();

      /* Suppose we are running behind schedule: then the STC is actually ahead of
         the current time. */
      if ( rtos_timer_state.queue )
      {
         now = rtos_timer_state.queue->offset;
      }

      //if the timer queue is empty and the timer hasn't expired, start the timer queue here
      if ( (NULL == rtos_timer_state.queue) || ((timer->ticks - now) < (rtos_timer_state.queue->ticks - now)) )
      {
         assert(timer != rtos_timer_state.queue);
         
         //insert this timer at the start of the list
         timer->next = rtos_timer_state.queue;
         rtos_timer_state.queue = timer;

         //start the timer queue
         rtos_timer_launch_();         
         
      }
      else
      {
         //find the position in the linked list that this timer needs to be inserted
         for ( t = rtos_timer_state.queue; NULL != t->next ; t = t->next )
         {
            if ( (timer->ticks - now) < (t->next->ticks - now) )
               break;
         }

         //insert the timer
         timer->next = t->next;
         t->next = timer;

      }

      RTOS_TIMER_CHECK();

      
      //success!
      success = 0;
   }

   return success;
}

/********************************************* End of file ************************************************************/
