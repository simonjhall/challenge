; /*=============================================================================
; Copyright (c) 2009-2014, Broadcom Corporation
; All rights reserved.
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
; =============================================================================*/

; The ".endfn" macro forgets any symbols created using ".set", so to avoid
; the warning about references not being position-independent (i.e. unable
; to resolve at compile time), we place values here which are used in multiple
; functions in vcsuspend_asm_vc4.s, and include this header above each one.

.set INTERRUPT_CPG_ADDR,	(MEM_SDRAM_DIRECT + (4 * INTERRUPT_CPG))

; Scratch area definitions

; Total size should be a multiple of 32, so that memerase can erase it all without
; any part being left out.

.set SCRATCH_MAGIC_OFFSET,      0
.set SCRATCH_MAGIC_SZ,          64
.set SCRATCH_BOOTRAM_OFFSET,	(SCRATCH_MAGIC_OFFSET + SCRATCH_MAGIC_SZ)
.set SCRATCH_BOOTRAM_SZ, 		BOOTROM_RAM_LENGTH
.set SCRATCH_STACK_OFFSET,		(SCRATCH_BOOTRAM_OFFSET + SCRATCH_BOOTRAM_SZ)
.set SCRATCH_STACK_SZ,			128 	; 31 words used + 4 bytes padding

.set SCRATCH_STACK_END,			(SCRATCH_STACK_OFFSET + SCRATCH_STACK_SZ)
.set SCRATCH_SZ,				SCRATCH_STACK_END

; Verify that we haven't run out of space (SCRATCHPAD_RESERVED_BYTES is defined
;	in vcsuspend_asm_internal.inc).
.assert SCRATCH_SZ <= SCRATCHPAD_RESERVED_BYTES

;-------------------------------------------------------------------------------


