;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.FORGET

.include "vcinclude/common.inc"

; entry:
;   r0 = memory area to search
;   r1 = character to find
;   r2 = maximum number of bytes to search
; exit:
;   if r1 is found in the first r2 bytes of r0, r0 points to the first occurrence
;   of r1, if it is not found a null pointer is returned

.function vclib_memchr
      add         r5, r0, 15
      mov         r4, -16
      and         r4, r5
      addcmpblo   r2, r0, r4, .T
      addcmpbhs   r0,  0, r4, .V
1:    ldb         r5, (r0)
      cmp         r5, r1
      beq         .E
      addcmpblo   r0, 1, r4, 1b
.V:   mov         r5, 16
      mov         r4, -16
      and         r4, r2
      vld         H(0,0), (r0)
      cbclr
      addcmpbhs   r0, 0, r4, .T
2:    vdists      HX(1,0), H(0,0)*, r1 SETF
      vtestmag    -, HX(1,0), 1        IFZ IMIN r3 ; if all the same r3=-1
      vld         H(0,16)*, (r0+16)
      cbadd1
      addcmpbge   r3, 0, 0, .VE
      addcmpblo   r0, r5, r4, 2b
.T:   addcmpbhs   r0, 0, r2, .F
3:    ldb         r5, (r0)
      addcmpbeq   r5, 0, r1, .E
      addcmpblo   r0, 1, r2, 3b
.F:   mov         r0, 0
      b           lr
.VE:  add         r0, r3           ; exit sum for the vector code
.E:   b           lr
.endfn

.FORGET
