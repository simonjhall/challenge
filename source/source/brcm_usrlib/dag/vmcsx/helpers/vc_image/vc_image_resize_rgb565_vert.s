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

; void vcir16_rowinterp(unsigned short * dest,      /* r0 */
;                       const unsigned short * src, /* r1 */
;                       int width_bytes,            /* r2 */
;                       int src_pitch_bytes,        /* r3 */
;                       int weight /* weighting in bottom 16 bits, r4 */ );
;
; Effect: dest[i=0..width-1] = src[i] + weight * (src[src_pitch+i] - src[i]) / 65536
;
; Runs faster when dest, src and src_pitch are 32-byte aligned, but not compulsory.

.globl vc_image_const_0_15
.globl vclib_memcpy

.section .text$vcir16_rowinterp,"text"
.globl vcir16_rowinterp
vcir16_rowinterp:
   addcmpbne r2, r0, r0, ri16_nonz  ; r2 marks the end of destination array
   b lr
ri16_nonz:
   bmask r4, 16
vcir16_rowinterp_entry_from_mixrow:
   addcmpbgt r4, 0, 63, ri16_notjustmemcpy
   sub r2, r0
   b vclib_memcpy              ; If weight is zero, just memcpy the upper row
ri16_notjustmemcpy:
   stm r6-r7,lr,(--sp)
   mov r6, 32                  ; r6 is constant 32
   sub r7, r2, r6              ; r7 is 32 behind the end
   add r5, pc, pcrel(vc_image_const_0_15)
   vld H(63,0), (r5)           ; magic number sequence 0..15
   vshl H(63,0), H(63,0), 1
   mov r5, r0
   bmask r5, 5                 ; Misaligned starting destination?
   vsub -, H(63,0), r5 SETF    ; N for pixels off the start. NN for valid ones.
   sub r0, r5                  ; Backtrack destination to be aligned
   sub r1, r5                  ; Backtrack source pointer to compensate

ri16_loop:
   sub r5, r2, r0
   vrsub -, H(63,0), r5+-2 IFNN SETF ; NN for valid pixels. N off the end.
   vld HX(4,0), (r0) IFN             ; Load in the pixels *not* to be modified.
ri16_loop_allvalid:
;   vldright HX(0++,0), (r1+=r3) REP 2
   vldleft HX(0++,0), (r1+=r3) REP 2
   vlsr HX(8++,0), HX(0++,0), 11 REP 2
   vlsr HX(10++,0), HX(0++,0), 5 REP 2
   vand HX(12++,0), HX(0++,0), 31 REP 2
   vand HX(10++,0), HX(10++,0), 63 REP 2
   vsub HX(14,0), HX(9,0), HX(8,0) 
   vsub HX(15,0), HX(11,0), HX(10,0)
   vsub HX(13,0), HX(13,0), HX(12,0)
   vmulhn.su HX(14++,0), HX(14++,0), r4 REP 2
   vmulhn.su HX(13,0), HX(13,0), r4
   vadd HX(8,0), HX(8,0), HX(14,0)
   vadd HX(9,0), HX(10,0), HX(15,0)
   vshl HX(8,0), HX(8,0), 11 CLRA ACC
   vshl HX(9,0), HX(9,0), 5 ACC
   vadd HX(4,0), HX(12,0), HX(13,0) ACC IFNN
   add r1, r6
   vst HX(4,0), (r0)
   vmov -, 0 SETF                    ; Clear the vector N flags
   addcmpbls r0, r6, r7, ri16_loop_allvalid
   addcmpblo r0, 0, r2, ri16_loop
   ldm r6-r7,pc,(sp++)
   .stabs "vcir16_rowinterp:F1",36,0,0,vcir16_rowinterp



; void vcir16_rowmix(unsigned short * dest,       r0
;                   const unsigned short * src,   r1
;                   int width_bytes,              r2
;                   int src_pitch_bytes,          r3
;                   int nrows);                   r4
;
; Effect: dest[i=0..width-1] = SUM(j=0..nrows-1)src[i+j*src_increment] / nrows

.section .text$vcir16_rowmix,"text"
.globl vcir16_rowmix
vcir16_rowmix:
   ; First we catch some degenerate cases, and special cases for
   ; when the number of rows to be mixed is equal to two or unity.
   addcmpbne r2, r0, r0, vcir16_rowmix_nonzwidth
   b lr
vcir16_rowmix_nonzwidth:
   addcmpbgt r4, 0, 2, vcir16_rowmix_general
   sub r4, 1
   shl r4, 16  ; 2 rows -> weight=32768,  1 row -> weight=0
   b vcir16_rowinterp_entry_from_mixrow
vcir16_rowmix_general:
   stm r6-r9,lr,(--sp)
   mov r6, 32                  ; r6 is constant 32
   sub r8, r2, r6              ; r8 is 32 before the end of dest
   add r5, pc, pcrel(vc_image_const_0_15)
   vld H(63,0), (r5)           ; magic number sequence 0..15
   vshl H(63,0), H(63,0), 1
   mov r5, r0
   bmask r5, 5                 ; Misaligned starting destination?
   vsub -, H(63,0), r5 SETF    ; N for pixels off the start. NN for valid ones.
   sub r0, r5                  ; Backtrack destination to be aligned
   sub r1, r5                  ; Backtrack source pointer to compensate
rm16g_hack_loop:
   addcmpble r4, 0, 16, rm16g_three_to_sixteen
   shl r3, 1
   asr r4, 1
   b rm16g_hack_loop
rm16g_three_to_sixteen:
   add r9, pc, pcrel(vc_image_recip_0_16)

rm16g_loop:
   sub r5, r2, r0
   vrsub -, H(63,0), r5+-2 IFNN SETF ; NN for valid pixels. N off the end.
   vld HX(4,0), (r0) IFN             ; Load in the pixels *not* to be modified.
rm16g_loop_allvalid:
   ; Load in the first 2 rows of source. We know there are at least 3.
   mov r5, r1
   vldleft HX(0++,0), (r1+=r3) REP 2
;   vldright HX(0++,0), (r1+=r3) REP 2
   vlsr HX(13++,0), HX(0++,0), 11 REP 2 CLRA ACC
   vlsr HX(10++,0), HX(0++,0), 5 REP 2
   vand -, HX(0++,0), 31 REP 2 CLRA ACC
   vand HX(10++,0), HX(10++,0), 63 REP 2
   vadd HX(15,0), HX(11,0), HX(10,0)
   add r5, r3
   mov r7, 2
rm16g_mixloop:
   add r5, r3
   vldleft HX(1,0), (r5)
;   vldright HX(1,0), (r5)
   vlsr HX(8,0), HX(1,0), 11
   vlsr HX(9,0), HX(1,0), 5
   vand HX(10,0), HX(1,0), 31 ACC
   vand HX(9,0), HX(9,0), 63
   vadd HX(14++,0), HX(14++,0), HX(8++,0) REP 2
   addcmpbne r7, 1, r4, rm16g_mixloop
   ld r5, (r9 + r4 << 2)
   vmulhn.su HX(14++,0), HX(14++,0), r5 REP 2
   vmulhn.su HX(10,0), HX(10,0), r5 CLRA ACC
   vshl HX(14,0), HX(14,0), 11 ACC
   vshl HX(4,0), HX(15,0), 5 ACC IFNN
   vmov -, 0 SETF
   add r1, r6
   vst HX(4,0), (r0)
   addcmpbls r0, r6, r8, rm16g_loop_allvalid
   addcmpblo r0, 0, r2, rm16g_loop
   ldm r6-r9,pc,(sp++)

.stabs "vcir16_rowmix:F1",36,0,0,vcir16_rowmix

.FORGET
