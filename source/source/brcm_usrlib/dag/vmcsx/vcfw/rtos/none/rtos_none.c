/* ============================================================================
Copyright (c) 2005-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "interface/vcos/vcos.h"
#include "vcinclude\hardware.h"

//drivers
#include "intctrl.h"

#ifdef WANT_VCGF
#   include "middleware/game/game.h"
#endif

#include "vcfw/rtos/rtos.h"
#include "rtos_common_priv.h"
#include "vcfw/rtos/none/rtos_malloc.h"
#include "vcfw/drivers/chip/systimer.h" // Systimer, for (deprecated) direct access to STC

#include "drivers/chip/intctrl.h"
#include "drivers/chip/core.h"

#include "middleware/rpc/rpc.h"

/******************************************************************************
Local defines
******************************************************************************/

/******************************************************************************
Private defs
******************************************************************************/

typedef struct
{
   //a secure function to register asm lisrs
   RTOS_SECURE_FUNC_HANDLE_T rtos_register_asm_lisr_secure_handle;

   //the intctrl driver and handle
   const INTCTRL_DRIVER_T *intctrl_driver;
   DRIVER_HANDLE_T intctrl_handle;
   volatile uint32_t sleep_time[2];

} RTOS_NONE_STATE_T;

/******************************************************************************
Extern functions.
******************************************************************************/

extern int32_t main( int argc, char *argv[] );

extern void rtos_interrupt_handler( void );

extern int32_t rtos_register_asm_lisr_secure( const uint32_t vector, RTOS_LISR_T new_lisr, RTOS_LISR_T *old_lisr );

extern void rtos_common_secure_software_interrupt( void );
extern void rtos_common_stack_smash_exception( void );

extern void exception_zero( void );
extern void exception_misaligned( void );
extern void exception_dividebyzero( void );
extern void exception_undefinedinstruction( void );
extern void exception_forbiddeninstruction( void );
extern void exception_illegalmemory( void );
extern void exception_buserror( void );
extern void exception_floatingpoint( void );
extern void exception_isp( void );
extern void exception_dummy( void );
extern void exception_icache( void );
extern void exception_veccore( void );
extern void exception_badl2alias( void );
extern void exception_breakpoint( void );
extern void exception_unknown( void );

extern void exception_rundomainaccess( void );

/******************************************************************************
Static data
******************************************************************************/

//state for this module
static RTOS_NONE_STATE_T rtos_none_state;

RTOS_LISR_T rtos_interrupt_functions[ INTERRUPT_HW_NUM ];
uint32_t rtos_interrupt_depth[2]; /* One per core */


//The hardware interrupt vectors
#pragma Data(DATA, ".isr_vectors")
const uint32_t _isr_vectors[ 128 ] =
{
#ifdef __VIDEOCORE4__
   (uint32_t)&exception_zero,
   (uint32_t)&exception_misaligned,
   (uint32_t)&exception_dividebyzero,
   (uint32_t)&exception_undefinedinstruction,
   (uint32_t)&exception_forbiddeninstruction,
   (uint32_t)&exception_illegalmemory,
   (uint32_t)&exception_buserror,
   (uint32_t)&exception_floatingpoint,
   (uint32_t)&exception_isp, 
   (uint32_t)&exception_dummy,
   (uint32_t)&exception_icache,
   (uint32_t)&exception_veccore,
   (uint32_t)&exception_badl2alias,
#ifndef __BCM2708A0__
   (uint32_t)&exception_breakpoint,
#else
   (uint32_t)&exception_unknown,
#endif
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
#else
   (uint32_t)&exception_zero,
   (uint32_t)&exception_misaligned,
   (uint32_t)&exception_dividebyzero,
   (uint32_t)&exception_undefinedinstruction,
   (uint32_t)&exception_forbiddeninstruction,
   (uint32_t)&exception_illegalmemory,
   (uint32_t)&exception_buserror,
   (uint32_t)&exception_floatingpoint,
   (uint32_t)&exception_isp, 
   (uint32_t)&exception_rundomainaccess,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
   (uint32_t)&exception_unknown,
#endif
   (uint32_t)&rtos_common_secure_software_interrupt,
#ifdef STACKPROTECT
   (uint32_t)&rtos_common_stack_smash_exception,
#endif
};

#pragma Data()

/******************************************************************************
Global functions.
******************************************************************************/

/***********************************************************
 * Name: rtos_init
 *
 * Arguments:
 *       void
 *
 * Description: Initialise the RTOS. In the case of 'no' RTOS, it does nothing
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_init( void )
{
   int32_t success = 0; //success by default

   if (core_get_func_table()->get_core_num(NULL) == 0)
   {
      //reset the state
      memset( &rtos_none_state, 0, sizeof( RTOS_NONE_STATE_T ) );

      //reset the rtos interrupt functions
      memset( rtos_interrupt_functions, 0, sizeof( rtos_interrupt_functions ) );

      //reset the rtos interrupt depth
      memset( rtos_interrupt_depth, 0, sizeof( rtos_interrupt_depth ) );

      //init the malloc pool
      success = rtos_malloc_init();
      if (!vcos_verify( success >= 0 ))
         return success;

      success = rtos_common_secure_init();
      if (!vcos_verify( success >= 0 ))
         return success;

      //register the secure function
      success = rtos_secure_function_register(  (RTOS_SECURE_FUNC_T)&rtos_register_asm_lisr_secure,
                &rtos_none_state.rtos_register_asm_lisr_secure_handle );

      //init the latch library
      success = rtos_none_latch_init();
      if (!vcos_verify( success >= 0 ))
         return success;

      //next, init the common rtos functions
      success = rtos_common_init();
      if (!vcos_verify( success >= 0 ))
         return success;

      //get a handle to the intctrl driver.  note that ->init will have already
      //been called in vcfw startup at this point, via driver_base_init()
      rtos_none_state.intctrl_driver = intctrl_get_func_table();
      if (!vcos_verify( NULL != rtos_none_state.intctrl_driver ))
         return -1;

      //open the intctrlr and get a handle
      success = rtos_none_state.intctrl_driver->open( NULL, &rtos_none_state.intctrl_handle );
      if (!vcos_verify( success >= 0 ))
         return success;
   }

   return success;
}

#pragma weak dispmanx_init
int dispmanx_init () { return 0; }

/***********************************************************
 * Call main with ARGC and ARGV when we don't have an RTOS.
 * Initialise the display here.  For the RTOS case, the display is initialised
 * as part of the task init.
 * Returns: int ( < 0 is fail)
 ***********************************************************/
int32_t
rtos_start (int argc, char *argv[])
{
  int32_t status = -1;

  _ei ();  /* enable interrupts */
  if (core_get_func_table()->get_core_num(NULL) == 0)
    {
      rtos_argc = argc;
      rtos_argv = argv;

      //init the games framework?!
#ifdef WANT_VCGF
      game_main (0, NULL);
#endif
    }
   else
   {
      // For the moment, don't get the second core running anything
      rtos_none_state.sleep_time[1] = 1;        // The 1 is to say it's awake...
      while(1)
      {
         _sleep();
      }
   }

  return main (argc, argv);
}


/***********************************************************
 * Name: rtos_sleep
 *
 * Arguments:
 *       void
 *
 * Description: Routine to put the current task (which is single threaded) to sleep
 *
 * Returns: void
 *
 ***********************************************************/
int32_t rtos_sleep( void )
{
   uint32_t sr = 0;

   //disable interrupts
   sr = rtos_none_state.intctrl_driver->disable_interrupts( rtos_none_state.intctrl_handle );

   uint32_t sleep_start = STC;

   //do the actual sleep!
   _sleep();

   rtos_none_state.sleep_time[rtos_get_cpu_number()] += STC - sleep_start;
   //restore interrupts
   return rtos_none_state.intctrl_driver->restore_interrupts( rtos_none_state.intctrl_handle, sr );
}

/***********************************************************
 * Name: rtos_register_asm_lisr
 *
 * Arguments:
 *       const uint32_t vector - the vector number
 *       RTOS_LISR_T new_lisr - the new lisr to register
 *       RTOS_LISR_T *old_lisr - the old lisr (which must be called when the new lisr is activated)
 *
 * Description: Register a asm lisr (a low level isr routine) with the RTOS
 *
 * Returns: int32_t - (0 == success)
 *
 ***********************************************************/
int32_t rtos_register_asm_lisr( const uint32_t vector, RTOS_LISR_ASM_T new_lisr, RTOS_LISR_ASM_T *old_lisr )
{
   return rtos_secure_function_call(   rtos_none_state.rtos_register_asm_lisr_secure_handle,
                                       vector,
                                       (uint32_t)new_lisr,
                                       (uint32_t)old_lisr,
                                       0,
                                       0 );
}


/***********************************************************
 * Name: rtos_register_lisr
 *
 * Arguments:
 *       const uint32_t vector - the vector number
 *       RTOS_LISR_T new_lisr - the new lisr to register
 *       RTOS_LISR_T *old_lisr - the old lisr (which must be called when the new lisr is activated)
 *
 * Description: Register a lisr (a low level isr routine) with the RTOS
 *
 * Returns: int32_t - (0 == success)
 *
 ***********************************************************/
int32_t rtos_register_lisr( const uint32_t vector, RTOS_LISR_T new_lisr, RTOS_LISR_T *old_lisr )
{
   int32_t success = -1; //fail by default

   if ( (vector >= INTERRUPT_HW_OFFSET) && (vector < (INTERRUPT_HW_OFFSET+INTERRUPT_HW_NUM)) )
   {
      if ( NULL != old_lisr )
      {
         *old_lisr = (RTOS_LISR_T)rtos_interrupt_functions[ vector - INTERRUPT_HW_OFFSET ];
      }

      //store the isr
      rtos_interrupt_functions[ vector - INTERRUPT_HW_OFFSET ] = new_lisr;

      //now register the asm handler
      success = rtos_register_asm_lisr( vector, rtos_interrupt_handler, NULL );
   }

   return success;
}


/***********************************************************
 * Name: rtos_disable_interrupts
 *
 * Arguments:
 *       void
 *
 * Description: Routine to disable interrupts
 *
 * Returns: uint32_t - previous isr status
 *
 ***********************************************************/
uint32_t rtos_disable_interrupts( void )
{
   uint32_t status = 0;

   status = rtos_none_state.intctrl_driver->disable_interrupts( rtos_none_state.intctrl_handle );

   return status;
}


/***********************************************************
 * Name: rtos_restore_interrupts
 *
 * Arguments:
 *       const uint32_t previous_state
 *
 * Description: Routine to re-enable interrupts
 *
 * Returns:
 *       None
 *
 ***********************************************************/
void rtos_restore_interrupts( const uint32_t previous_state )
{
   //restore interrupts
   rtos_none_state.intctrl_driver->restore_interrupts( rtos_none_state.intctrl_handle, previous_state );
}


/***********************************************************
 * Name: rtos_get_cpu_count
 *
 * Arguments:     None
 *
 * Description:   Returns a count of the CPUs controlled by the RTOS
 *
 * Returns:    uint32_t
 *
 ***********************************************************/
uint32_t rtos_get_cpu_count( void )
{
   return 0;
}

/***********************************************************
 * Name: rtos_get_thread_id
 *
 * Arguments:     None
 *
 * Description:   Returns the id of the current thread
 *
 * Returns:       uint32_t
 *
 ***********************************************************/
uint32_t rtos_get_thread_id( void )
{
   return (uint32_t)rtos_get_cpu_number();
}

static int have_thread;
static RTOS_LATCH_T thread_exit_latch;

static void thread_exit_callback(int dummy)
{
   rtos_latch_put(&thread_exit_latch);
}

static void need_rpc()
{
   static uint32_t initialised = 0;

   if (!initialised)
   {
      rpc_init();
      thread_exit_latch = rtos_latch_locked();
      have_thread = 0;
      rpc_callback = thread_exit_callback;
   }
}

/***********************************************************
 * Name: rtos_cpu_migrate
 *
 * Arguments:     const uint32_t cpu - the CPU to migrate to
 *
 * Description:   Routine to migrate the current thread to the other cpu.
 *                Does nothing if already on the requested cpu.
 *                Please note that rtos_get_thread_id may not return consistent
 *                values before and after cpu migration.
 *                In rtos_none, fails if something is already running on the
 *                other cpu.
 *
 * Returns:       None
 *
 ***********************************************************/
void rtos_cpu_migrate( const uint32_t cpu )
{
   need_rpc();
   rpc_migrate( cpu );
}

/***********************************************************
 * Name: rtos_cpu_launch
 *
 * Arguments:     const uint32_t cpu - the CPU to launch on
 *                RTOS_LAUNCH_FUNC_T launch_func - the function to launch
 *                uint32_t *argc - data to pass to the function
 *                void *argv - data to pass to the function
 *                const uint32_t stack_size - minimum required stack size
 *                for the task we are launching
 *
 * Description:   Launch the given function asynchronously on the other cpu.
 *                Do not wait for it to return.
 *                The rtos_none version of this function is implemented using rpc.
 *                We do not create the stack here, so we fail if the requested
 *                stack size is bigger than the RPC stack size.
 *                Fails if the current CPU is not 0 or the target CPU is not 1.
 *
 * Returns:       The thread identifier of the task we've just created. This is
 *                consistent with a rtos_get_thread_id() call from the new
 *                task.
 *                Returns RTOS_INVALID_THREAD_ID on failure.
 *
 ***********************************************************/
static int launcher( RTOS_LAUNCH_FUNC_T launch_func, uint32_t argc, void *argv )
{
   // TODO: rpc seems to leave interrupts disabled when it calls its function
   intctrl_get_func_table()->enable_interrupts( 0 );
   launch_func(argc, argv);
   return 0;
}

uint32_t rtos_cpu_launch( const uint32_t cpu, RTOS_LAUNCH_FUNC_T launch_func, uint32_t argc, void *argv, const uint32_t stack_size )
{
   if (vcos_verify( rtos_get_cpu_number() == 0 && cpu == 1 && stack_size <= RPC_STACK_SIZE ))
   {
      need_rpc();
      have_thread = 1;
      rpc_function = (int(*)())launcher;
      rpc_execute( launch_func, argc, argv );
      return cpu;
   }
   else
   {
      return RTOS_INVALID_THREAD_ID;
   }
}

/***********************************************************
 * Name: rtos_thread_join
 *
 * Arguments:     const uint32_t thread_id - the id of the thread we
 *                wish to wait for
 *
 * Description:   Wait for the specified thread to terminate. This should
 *                be called exactly once for each thread created. After it
 *                is called, thread_id may identify a new thread.
 *
 * Returns:       -
 *
 ***********************************************************/

void rtos_thread_join( const uint32_t thread_id )
{
   vcos_assert( have_thread && rtos_get_cpu_number() == 0 && thread_id == 1 );
   have_thread = 0;
   rtos_latch_get( &thread_exit_latch );
}

/***********************************************************
 * Name: rtos_check_stack
 *
 * Arguments:     -
 *
 * Description:   Return the amount of free space on the stack (in bytes).
 *                Useful for making recursive algorithms safe.
 *
 * Returns:       uint32_t
 *
 ***********************************************************/

uint32_t rtos_check_stack( void )
{
   // TODO: calculate this properly on rtos_none?
   return 1234567;
}

/***********************************************************
 * Name: rtos_get_sleep_time
 *
 * Arguments:     const uint32_t cpu - the CPU in question
 *
 * Description:   Returns a count of the usecs the specified
 *                CPU has spent sleeping
 *
 * Returns:       uint32_t
 *
 ***********************************************************/
uint32_t rtos_get_sleep_time( const uint32_t cpu )
{
   return rtos_none_state.sleep_time[cpu];
}

/***********************************************************
 * Name: rtos_reset_sleep_time
 *
 * Arguments:     uint32_t cpu - the CPU in question
 *
 * Description:   Resets the sleep time for that CPU
 *
 * Returns:       Nothing
 *
 ***********************************************************/

extern void rtos_reset_sleep_time( uint32_t cpu )
{
   rtos_none_state.sleep_time[cpu] = 0;
}


/***********************************************************
 * Name: rtos_get_lisr_time
 *
 * Arguments:     const uint32_t cpu - the CPU in question
 *
 * Description:   Returns a count of the usecs the specified
 *                CPU has spent in LISRs
 *
 * Returns: uint32_t
 *
 ***********************************************************/
uint32_t rtos_get_lisr_time( const uint32_t cpu )
{
   return 0;
}


/***********************************************************
 * Name: rtos_set_lisr_max_time
 *
 * Arguments:     const uint32_t cpu - the CPU in question
 *
 * Description:   Sets the upper limit on the number of usecs an LISR can
 *                take. Exceeding this limit will trigger a breakpoint on
 *                completion.
 *
 * Returns:       uint32_t - the previous limit
 *
 ***********************************************************/
uint32_t rtos_set_lisr_max_time( const uint32_t cpu, const uint32_t limit )
{
   return RTOS_MAX_TIME_UNLIMITED;
}


/***********************************************************
 * Name: rtos_in_interrupt()
 *
 * Arguments:     None.
 *
 * Description:   Returns an indication of whether the caller is within
 *                interrupt context.
 *
 * Returns:       int - non-zero if the caller is within interrupt
 *                context, otherwise returns zero.
 *
 ***********************************************************/
int rtos_in_interrupt()
{
   return (rtos_interrupt_depth[rtos_get_cpu_number()] != 0);
}

/* ************************************ The End ***************************************** */
