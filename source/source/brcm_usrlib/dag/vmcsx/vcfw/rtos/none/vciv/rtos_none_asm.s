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
.include "vcinclude/hardware.inc"

.extern _isr_vectors

;/***********************************************************
; * Name: rtos_interrupt_handler
; *
; * Arguments:
; *       void
; *
; * Description: The actual interrupt handler
; *
; * Returns: int32_t - (0 == success)
; *
; ***********************************************************/
.function rtos_interrupt_handler
   .weak _0
   .cfa_push {%pc, %sr}
   stm r0-r5,lr,(--sp)
   .cfa_push {%lr,%r0,%r1,%r2,%r3,%r4,%r5}

   version r0
   btest r0, 16

   ; first get interrupt vector number from interrupt status register
   mov r0, (IC_0)                ; interrupt controller
   mov r1, (IC_1)                ; interrupt controller

   mov.ne r0, r1

   ld r0, (r0+IS-IC)                       ; get contents of IS
   bmask r0, 7                       ; r0 = interrupt vector number
   addcmpbeq r0, 0, 0, int_now_zero

   ; Increment the interrupt depth
   add r4, gp, gprel27(rtos_interrupt_depth)
   ld r5, (r4 + tp<<2)
   add r5, 1
   st r5, (r4 + tp<<2)

   sub r2, r0, INTERRUPT_HW_OFFSET       ; r2 = hardware int number
;mov p15, r2
   ; look up in handler table
   add r1, gp, gprel27(rtos_interrupt_functions)
   ld r1, (r1+r2 << 2)
   addcmpbeq r1, 0, 0, int_now_zero

   bl r1                    ; call C function

   ; Decrement the interrupt depth
   add r4, gp, gprel27(rtos_interrupt_depth)
   ld r5, (r4 + tp<<2)
   sub r5, 1
   st r5, (r4 + tp<<2)

   b 1f
int_now_zero:
   bkpt
1:
   ldm r0-r5,(sp++)
   .cfa_pop  {%r5,%r4,%r3,%r2,%r1,%r0}
   ld  lr, (sp++)
   .cfa_pop  {%lr}
   rti
   .cfa_pop  {%sr, %pc}

   .align      4
   .word       _isr_vectors    ; ensure _isr_vectors is linked in
.endfn ; }}}


; /******************************************************************************
; NAME
;   Exceptions
;
; SYNOPSIS
; ******************************************************************************/
.align 4
.function exception_zero
   .cfa_push {%pc, %sr}
   bkpt
   ; shouldn't occur, but can when interrupts go away outside of isr
   ; can be when core code clears interrupt pending state, or
   ; isr returns after writing to register that clears state without blocking until write is complete (e.g. with ld and mov)
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_misaligned
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_dividebyzero
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_undefinedinstruction
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_forbiddeninstruction
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_illegalmemory
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_buserror
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_floatingpoint
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_isp
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_dummy
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_icache
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_veccore
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_badl2alias
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_breakpoint
   .cfa_push {%pc, %sr}
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

.function exception_unknown
   .cfa_push {%pc, %sr}
   bkpt
   rti
   .cfa_pop  {%sr, %pc}
.endfn ; }}}

; /******************************************************************************
; NAME
;   rtos_register_asm_lisr_secure
;
; SYNOPSIS
;    int32_t rtos_register_asm_lisr_secure ( int vector, void (*new_lisr)(int), void (**old_lisr)(int) )
;
; FUNCTION
;
;    Registers an assembler routine with a given interrupt vector.
;    If an assembler routine has previously been registered, it is returned in old_lisr -
;    It is polite to call this routine if the interrupt was not for you.
;
; RETURNS
;    0 on success, all other values are failures
; ******************************************************************************/
.secure_api_function rtos_register_asm_lisr_secure ; {{{
   .weak _0
   cmp r0, INTERRUPT_HW_OFFSET
   blt 2f
   cmp r0, INTERRUPT_HW_OFFSET + INTERRUPT_HW_NUM
   bge 2f
   shl r0, 2
   add r0, __INTERRUPT_VECTORS_START-_0    ; vector base
   addcmpbeq r2, 0, 0, 1f
   ld r3, (r0)
   st r3, (r2)
1:
   st r1, (r0)
   mov r0, 0
   b lr
2:
   mov r0, -1
   b lr
.endfn ; }}}


;/**************************************** End of file ************************************************************/
