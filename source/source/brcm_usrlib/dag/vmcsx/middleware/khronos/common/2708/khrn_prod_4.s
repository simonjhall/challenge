;;; ===========================================================================
; Copyright (c) 2008-2014, Broadcom Corporation
; All rights reserved.
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/hardware.inc"

khrn_hw_full_memory_barrier::

   ; todo: are all of these loads necessary? are they sufficient? can i go back
   ; to using p14/p15?

   add  r0, pc, pcrel(khrn_hw_full_memory_barrier) ; any harmless address

   bitset  r0, 31
.ifdef __BCM2708__
   ; HW-2827 workaround
   ldb  r1, (r0) ; load from non-allocating alias
   mov  r1, r1 ; kill lazy load
.endif

   bitset  r0, 30
   ldb  r1, (r0) ; load from direct alias
   mov  r1, r1 ; kill lazy load

   ; todo: we're assuming v3d is turned on!
   mov  r0, V3D_IDENT0
   ld  r1, (r0) ; load from v3d
   mov  r1, r1 ; kill lazy load

   b  lr
