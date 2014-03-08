;;; ===========================================================================
;;; Copyright (c) 2008-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

.extern __RTOS_SECFNS_START
.extern __RTOS_SECFNS_COUNT

; /******************************************************************************
; NAME
;   rtos_secure_function_call
; 
; SYNOPSIS
;    int32_t rtos_secure_function_call( const RTOS_SECURE_FUNC_HANDLE_T secure_func_handle,
;                    const uint32_t r0,
;                    const uint32_t r1,
;                    const uint32_t r2,
;                    const uint32_t r3,
;                    const uint32_t r4 )
; 
; FUNCTION
;
;    Calls a secure function via a software interrupt. Interrupts are disabled
;    before entering secure mode, then re-enabled afterwards.
; 
; RETURNS
;    -
; ******************************************************************************/
.function rtos_secure_function_call ; {{{
   st r6, (--sp)
   .cfa_push {%r6}
   mov r6, sr
   di
   int 0
   mov sr, r6
   ld r6, (sp++)
   .cfa_pop {%r6}
   b lr
.endfn ; }}}

; /******************************************************************************
; NAME
;   rtos_common_secure_software_interrupt
; 
; SYNOPSIS
;    int32_t rtos_common_secure_software_interrupt( void )
; 
; FUNCTION
;
;    Dispatches a secure API
; 
; RETURNS
;    -
; ******************************************************************************/
.secure_function rtos_common_secure_software_interrupt ; {{{
   .weak __0
   .cfa_push {%pc, %sr}
   stm r6,lr,(--sp)
   .cfa_push {%lr,%r6}

   ;check it's within the legal range
   cmp r0, near(__RTOS_SECFNS_COUNT - __0)
   bge 1f

   ;index into the 'rtos_common_secure_state' by r0
   add r6, pc, pcrel(__RTOS_SECFNS_START)

   ;load this address
   ld r6, (r6+r0<<2)

   ;re-arrange r1-r5 to r0-r4
   mov r0, r1 
   mov r1, r2 
   mov r2, r3 
   mov r3, r4 
   mov r4, r5

   ; call the function!
   bl r6

   ; all done!
   b 2f
   
1:
   bkpt ; invalid secure function!
   mov r0, -1
   
2:
   ldm r6,(sp++)
   ld  lr, (sp++)   
   .cfa_pop   {%r6,%lr}
   rti
   .cfa_pop   {%sr, %pc}

   .word rtos_common_secure_function_lookup ; Force a reference to this 
.endfn ; }}}

; /******************************************************************************
; NAME
;   rtos_common_secure_function_lookup
; 
; SYNOPSIS
;    RTOS_SECURE_FUNC_HANDLE_T rtos_common_secure_function_lookup( RTOS_SECURE_FUNC_T )
; 
; FUNCTION
;
;    Retrieves a handle for a secure API function.
;    This is preregistered as handle zero.
;    Do not rename this function without also modifying memorymap.mk accordingly.
; 
; RETURNS
;    -
; ******************************************************************************/
.secure_api_function rtos_common_secure_function_lookup ; {{{

   add   r1, pc, pcrel(__RTOS_SECFNS_START)

   mov   r2, near(__RTOS_SECFNS_COUNT - __0)
   mov   r3, 0

.find_func:
   ld    r4, (r1++)
   addcmpbeq r4, 0, r0, .found_func
   addcmpbne r3, 1, r2, .find_func

   mov r3, -1 ; invalid secure function

.found_func:
   mov   r0, r3
   b     lr
.endfn ; }}}

;/**************************************** End of file ************************************************************/
