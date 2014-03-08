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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vcinclude\hardware.h"

#include "vcfw/rtos/rtos.h"
#include "rtos_common_priv.h"
#include "rtos_common_mem.h"

#include "vcfw/drivers/chip/systimer.h"
#include "vcfw/drivers/chip/intctrl.h"
#include "vcfw/drivers/chip/core.h"
#include "vcfw/drivers/chip/resetctrl.h"

/******************************************************************************
Local defines
******************************************************************************/

/******************************************************************************
Static functions.
******************************************************************************/

#ifndef FORSIM
//Callback for the timer wait function
static void rtos_wait_done( RTOS_TIMER_T *timer, void *param);
#endif

/******************************************************************************
Static data
******************************************************************************/

//Static pointers to the drivers
static const SYSTIMER_DRIVER_T *systimer_driver = NULL;
static const INTCTRL_DRIVER_T *intctrl_driver = NULL;
static const CORE_DRIVER_T *core_driver = NULL;
static const RESETCTRL_DRIVER_T *reset_driver = NULL;

//Handles from the opened drivers
static DRIVER_HANDLE_T systimer_handle = NULL;
static DRIVER_HANDLE_T intctrl_handle = NULL;
static DRIVER_HANDLE_T core_handle = NULL;
static DRIVER_HANDLE_T reset_handle = NULL;

/******************************************************************************
Global data.
******************************************************************************/

int rtos_argc = 0;
char **rtos_argv = NULL;

/******************************************************************************
Global functions.
******************************************************************************/

/***********************************************************
 * Name: rtos_common_init
 *
 * Arguments:
 *       void
 *
 * Description: Initialise the common RTOS functions, including any sub modules
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_common_init( void )
{
   int32_t success = -1; //fail by default

   //init the secure library
#if 0
   success = rtos_common_secure_init();
   assert( success >= 0 );
#endif

   //init the timer library
   success = rtos_timer_lib_init();
   assert( success >= 0 );

   //get a handle to the systimer driver
   systimer_driver = systimer_get_func_table();
   assert( NULL != systimer_driver );

   //open the sys timer and get a handle
   success = systimer_driver->open( NULL, &systimer_handle );
   assert( success >= 0 );

   //get a handle to the intctrl driver
   intctrl_driver = intctrl_get_func_table();
   assert( NULL != intctrl_driver );

   //open the intctrlr and get a handle
   success = intctrl_driver->open( NULL, &intctrl_handle );
   assert( success >= 0 );

   //get a handle to the core driver
   core_driver = core_get_func_table();
   assert( NULL != core_driver );

   //open the core driver and get a handle
   success = core_driver->open( NULL, &core_handle );
   assert( success >= 0 );
   
   // Get reset driver
   reset_driver = resetctrl_get_func_table();
   assert( NULL != reset_driver);
   
   //get handle
   success = reset_driver->open( NULL, &reset_handle );
   assert( success >= 0 );

   //initialise the relocatable heap
   success = rtos_common_mem_init();
   assert( success >= 0 );

   return success;
}


/***********************************************************
 * Name: rtos_delay
 *
 * Arguments:
 *       const uint32_t usecs
 *       const uint32_t suspend
 *
 * Description: Delay the calling function by 'usecs'. Optionally allow the RTOS to suspend
 *                that task (as specified by the suspend flag)
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_delay( const uint32_t usecs, const uint32_t suspend )
{
   int32_t success = -1; //fail by default

   //don't delay in the simulator
#ifndef FORSIM
   //only allow suspending behaviour if:
   // * the suspend flag is set
   // * the time we want to suspend for is more than 100usecs
   // * interrupts are enable
   // * the interrupt priority is valid
   if ( suspend && (usecs > 100) && rtos_interrupts_enabled() )
   {
      RTOS_TIMER_T timer;

      RTOS_LATCH_T latch = rtos_latch_locked();

      success = rtos_timer_init( &timer, rtos_wait_done, &latch );
      assert( success >= 0 );

      if ( success >= 0 )
      {
         rtos_timer_set( &timer, usecs );

         rtos_latch_get(&latch);
      }
   }
   else
   {
      //call the system timer driver
      assert( systimer_driver && systimer_driver->wait_for_usecs );
      success = systimer_driver->wait_for_usecs( systimer_handle, usecs);
   }
#else
   success = 0;
#endif

   return success;
}

/***********************************************************
 * Name: rtos_getmicrosecs()
 *
 * Arguments:
 *
 * Description: Get the current time in usecs
 *
 * Returns: uint32_t time
 *
 ***********************************************************/

unsigned int rtos_getmicrosecs(void)
{
   return systimer_driver->get_time_in_usecs( systimer_handle );
}

/***********************************************************
 * Name: rtos_get_cpu_num
 *
 * Arguments:
 *       void
 *
 * Description: Returns the number of the current CPU
 *
 *
 * Returns: uint (cannot fail)
 *
 ***********************************************************/
uint32_t rtos_get_cpu_number( void )
{
   return core_driver->get_core_num( core_handle );
}

/***********************************************************
 * Name: rtos_reset
 *
 * Arguments:
 *       void
 *
 * Description: Resets the processor (does not return)
 *
 *
 * Returns: Doesn't!
 *
 ***********************************************************/
void rtos_reset( void )
{
   reset_driver->set_watchdog( reset_handle, 10);
   
   // This function will never return
   for(;;);
}


/******************************************************************************
Static functions.
******************************************************************************/

/***********************************************************
 * Name: rtos_wait_done
 *
 * Arguments:
 *       RTOS_TIMER_T *timer - the timer being called back
 *       void *param - the param that was passed when the timer was launched
 *
 * Description: Callback used with the 'wait_for_usecs' API when blocking for time 'x'
 *
 * Returns: void
 *
 ***********************************************************/
#ifndef FORSIM
static void rtos_wait_done( RTOS_TIMER_T *timer, void *param )
{
   //cast the param to a latch
   RTOS_LATCH_T *latch = (RTOS_LATCH_T *)param;

   assert( NULL != latch );

   //release the latch
   rtos_latch_put( latch );
}
#endif

/* ************************************ The End ***************************************** */
