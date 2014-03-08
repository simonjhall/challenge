/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vcos_assert.h>

#include "vcinclude/hardware.h"
#include "vcfw/rtos/rtos.h"
#include "vcfw/drivers/chip/intctrl.h"

/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

/* There is one pre-registered secure function. */
const RTOS_SECURE_FUNC_HANDLE_T
   rtos_secure_function_lookup_handle = (RTOS_SECURE_FUNC_HANDLE_T)0;
/******************************************************************************
Extern functions
******************************************************************************/

extern void rtos_common_secure_software_interrupt( void );

/******************************************************************************
Global function definitions.
******************************************************************************/

/***********************************************************
 * Name: rtos_common_secure_init
 *
 * Arguments:
 *       void
 *
 * Description: Initialises this module
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_common_secure_init( void )
{
   int32_t success;

   //success!
   success = 0;

   return success;
}

/***********************************************************
 * Name: rtos_secure_function_register
 *
 * Arguments:
 *       RTOS_SECURE_FUNC_T secure_func,
         RTOS_SECURE_FUNC_HANDLE_T *secure_func_handle
 *
 * Description: Registers a secure function
 *
 * Returns: int ( < 0 is fail)
 *
 ***********************************************************/
int32_t rtos_secure_function_register(RTOS_SECURE_FUNC_T secure_func,
                                      RTOS_SECURE_FUNC_HANDLE_T *secure_func_handle )
{
   int32_t success = -1; //fail by default
   int32_t handle = rtos_secure_function_call(rtos_secure_function_lookup_handle, (uint32_t)secure_func, 0, 0, 0, 0);

   vcos_assert(handle >= 0);

   if (handle >= 0)
   {
      *secure_func_handle = handle;

      //success!
      success = 0;
   }

   return success;
}

/******************************************************************************
Static functions
******************************************************************************/


/************************************ End of file ******************************************************/
