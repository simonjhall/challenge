/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <string.h>

#include "interface/vcos/vcos.h"
#include "vcfw/rtos/rtos.h"

#include "drivers/chip/intctrl.h"
#include "drivers/chip/multicore_sync.h"

/******************************************************************************
  Local typedefs
 *****************************************************************************/

typedef struct
{
   const MULTICORE_SYNC_DRIVER_T *multicore_sync_driver;   //Ptr to the chip multicore_sync driver
   DRIVER_HANDLE_T multicore_sync_handle; //Handle to an opened chip driver

   const INTCTRL_DRIVER_T *intctrl_driver;   //Ptr to the chip intctrl driver
   DRIVER_HANDLE_T intctrl_handle; //Handle to an opened chip driver

   //semaphore
   MULTICORE_SYNC_SEMAPHORE_T multicore_sync_semaphore;

} RTOS_LATCH_STATE_T;

/******************************************************************************
  Static data
 *****************************************************************************/

static RTOS_LATCH_STATE_T rtos_latch_state;

/******************************************************************************
  Global functions
 *****************************************************************************/

/***********************************************************
 * Name: rtos_none_latch_init
 *
 * Arguments:
 *       void
 *
 * Description: Routine to init the latch library
 *
 * Returns: void
 *
 ***********************************************************/
int32_t rtos_none_latch_init( void )
{
   int32_t success = -1;

   //reset the state
   memset( &rtos_latch_state, 0, sizeof( RTOS_LATCH_STATE_T ) );

   //get the multicore_sync driver
   rtos_latch_state.multicore_sync_driver = multicore_sync_get_func_table();
   if (!vcos_verify( NULL != rtos_latch_state.multicore_sync_driver ))
      return success;

   //open an instance of the multicore_sync
   success = rtos_latch_state.multicore_sync_driver->open( NULL, &rtos_latch_state.multicore_sync_handle );
   if (!vcos_verify( success >= 0 ))
      return success;

   //create a semaphore to use to protect the latches
   success = rtos_latch_state.multicore_sync_driver->create_semaphore( &rtos_latch_state.multicore_sync_semaphore );
   if (!vcos_verify( success >= 0 ))
      return success;

   //get the inctrl driver
   rtos_latch_state.intctrl_driver = intctrl_get_func_table();
   if (!vcos_verify( NULL != rtos_latch_state.intctrl_driver ))
      return success;

   //open an instance of the intctrl
   success = rtos_latch_state.intctrl_driver->open( NULL, &rtos_latch_state.intctrl_handle );
   if (!vcos_verify( success >= 0 ))
      return success;
   
#ifdef LATCH_LOGGING_ENABLED
   rtos_latch_logging_init();
#endif

   return success;
}


/***********************************************************
* Name: rtos_latch_get_real
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to get
*
* Description: Routine to get / obtain a latch
*
* Returns: void
*
***********************************************************/
void rtos_latch_get_real( RTOS_LATCH_T *latch )
{
   while ( 1 )
   {
      //disable interrupts
      int32_t old_int_state = rtos_latch_state.intctrl_driver->disable_interrupts( rtos_latch_state.intctrl_handle );

      //grab the multicore sync semaphore
      if ( 0 == rtos_latch_state.multicore_sync_driver->get_semaphore( rtos_latch_state.multicore_sync_semaphore ) )
      {
         //is the latch unlocked?
         if (rtos_latch_unlocked() == *latch)
         {
            //yes, lock the latch
            *latch = rtos_latch_locked();

            //release the semaphore
            rtos_latch_state.multicore_sync_driver->put_semaphore( rtos_latch_state.multicore_sync_semaphore );

            //return
            rtos_latch_state.intctrl_driver->restore_interrupts( rtos_latch_state.intctrl_handle, old_int_state );

            return;
         }

         //release the semaphore
         rtos_latch_state.multicore_sync_driver->put_semaphore( rtos_latch_state.multicore_sync_semaphore );
      }
      else
      {
         //this should never go off
         vcos_assert( 0 );
      }

      //reenable interrupts
      rtos_latch_state.intctrl_driver->restore_interrupts( rtos_latch_state.intctrl_handle, old_int_state );

      vcos_assert( rtos_latch_state.intctrl_driver->are_interrupts_enabled( rtos_latch_state.intctrl_handle ) );
   }
}


/***********************************************************
* Name: rtos_latch_put_real
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to put
*
* Description: Routine to put / release a latch
*
* Returns: void
*
***********************************************************/
void rtos_latch_put_real( RTOS_LATCH_T *latch )
{
   //disable interrupts
   int32_t old_int_state = rtos_latch_state.intctrl_driver->disable_interrupts( rtos_latch_state.intctrl_handle );

   //grab the multicore sync semaphore
   if ( 0 == rtos_latch_state.multicore_sync_driver->get_semaphore( rtos_latch_state.multicore_sync_semaphore ) )
   {
      //Allow latch to be put when already in the unlocked state, to be
      //consistent with rtos_nucleus_latch
      //vcos_assert(rtos_latch_unlocked() != *latch);
      //vcos_assert(rtos_latch_locked() == *latch);

      *latch = rtos_latch_unlocked();

      //release the semaphore
      rtos_latch_state.multicore_sync_driver->put_semaphore( rtos_latch_state.multicore_sync_semaphore );
   }
   else
   {
      vcos_assert( 0 ); //what has gone wrong here?!
   }

   //reenable interrupts
   rtos_latch_state.intctrl_driver->restore_interrupts( rtos_latch_state.intctrl_handle, old_int_state );
}

/***********************************************************
* Name: rtos_latch_try_real
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to get
*
* Description: Routine to attempt to get a latch.
*
* Returns: success = 0
*
***********************************************************/
int32_t rtos_latch_try_real( RTOS_LATCH_T *latch )
{
   int32_t success = -1;

   //cheap check to see if the latch is available?
   if (rtos_latch_unlocked() == *latch)
   {
      //disable interrupts
      int32_t old_int_state = old_int_state = rtos_latch_state.intctrl_driver->disable_interrupts( rtos_latch_state.intctrl_handle );

      rtos_latch_state.multicore_sync_driver->get_semaphore( rtos_latch_state.multicore_sync_semaphore );

      if (rtos_latch_unlocked() == *latch)
      {
         *latch = rtos_latch_locked();

         //success!
         success = 0;
      }

      //release the semaphore
      rtos_latch_state.multicore_sync_driver->put_semaphore( rtos_latch_state.multicore_sync_semaphore );

      rtos_latch_state.intctrl_driver->restore_interrupts( rtos_latch_state.intctrl_handle, old_int_state );
   }

   return success;
}

/********************************** End of file ******************************************/
