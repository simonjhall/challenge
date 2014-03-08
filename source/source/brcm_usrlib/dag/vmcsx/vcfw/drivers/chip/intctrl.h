/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef INTCTRL_H_
#define INTCTRL_H_

#include "vcinclude/common.h"
#include "vcfw/drivers/driver.h"

/******************************************************************************
 API
 *****************************************************************************/

//routine to set the interrupt mask (operates on the core it
typedef int32_t (*INTCTRL_SET_IMASK)(  const DRIVER_HANDLE_T handle,
                                       const uint32_t vector,
                                       const uint32_t priority );

//routine to set the interrupt mask on a specific core
typedef int32_t (*INTCTRL_SET_IMASK_PER_CORE)(  const DRIVER_HANDLE_T handle,
                                                const uint32_t core,
                                                const uint32_t vector,
                                                const uint32_t priority );

//Routine to wakeup core1
typedef int32_t (*INTCTRL_WAKEUP_CORE1)(  const DRIVER_HANDLE_T handle,
                                          const uint32_t address );

//Routine to disable interrupts
typedef int32_t (*INTCTRL_DISABLE_INTERRUPTS)( const DRIVER_HANDLE_T handle );

//Routine to enable interrupts
typedef int32_t (*INTCTRL_ENABLE_INTERRUPTS)( const DRIVER_HANDLE_T handle );

//Routine to enable interrupts
typedef int32_t (*INTCTRL_RESTORE_INTERRUPTS)(  const DRIVER_HANDLE_T handle,
                                                const int32_t prevstate );

//Routine to query if interrupts are enabled or not
typedef uint32_t (*INTCTRL_ARE_INTERRUPTS_ENABLED)( const DRIVER_HANDLE_T handle );

//Routine to set interrupt vector address
typedef int32_t (*INTCTRL_SET_INTERRUPT_VECTOR_ADDRESS)( const DRIVER_HANDLE_T handle, const uint32_t core, const uint32_t vector_address );

//Routines to force/clear/check interrupt
typedef int32_t (*INTCTRL_FORCE_INTERRUPT)(const DRIVER_HANDLE_T handle, const uint32_t core, const uint32_t vector);
typedef int32_t (*INTCTRL_CLEAR_INTERRUPT)(const DRIVER_HANDLE_T handle, const uint32_t core, const uint32_t vector);
typedef int32_t (*INTCTRL_IS_FORCED)(const DRIVER_HANDLE_T handle, const uint32_t core, const uint32_t vector);


/******************************************************************************
 System driver struct
 *****************************************************************************/

typedef struct
{
   //include the common driver definitions - this is a macro
   COMMON_DRIVER_API(void const *unused)

   //routine to set the imask of a interrupt vector
   INTCTRL_SET_IMASK set_imask;

   //routine to set the imask of a interrupt vector (on a core by core basis)
   INTCTRL_SET_IMASK_PER_CORE set_imask_per_core;

   //Routine to wakeup core1
   INTCTRL_WAKEUP_CORE1 wakeup_core1;

   //routine to disable interrupts
   INTCTRL_DISABLE_INTERRUPTS disable_interrupts;

   //routine to enable interrupts
   INTCTRL_ENABLE_INTERRUPTS enable_interrupts;

   //routine to restore interrupts from a status register
   INTCTRL_RESTORE_INTERRUPTS restore_interrupts;

   //routine to detect if interrupts are enabled or not
   INTCTRL_ARE_INTERRUPTS_ENABLED are_interrupts_enabled;

   //Routine to set interrupt vector address
   INTCTRL_SET_INTERRUPT_VECTOR_ADDRESS set_vector_address;
   
   //Routine to force interrupt to be set
   INTCTRL_FORCE_INTERRUPT force_interrupt;
   //Routine to clear forced interrupt
   INTCTRL_CLEAR_INTERRUPT clear_interrupt;
   // Routine to check if the interrupt is forced
   INTCTRL_IS_FORCED is_forced;
   
} INTCTRL_DRIVER_T;

/******************************************************************************
 Global Functions
 *****************************************************************************/

const INTCTRL_DRIVER_T *intctrl_get_func_table( void );

#endif // INTCTRL_H_
