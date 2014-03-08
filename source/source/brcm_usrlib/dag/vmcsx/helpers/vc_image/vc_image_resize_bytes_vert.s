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

; void vcir8_rowinterp(unsigned char * dest,       /* r0 */
;                      const unsigned char * src,  /* r1 */
;                      int width,                  /* r2 */
;                      int src_pitch,              /* r3 */
;                      int weight /* weighting factor in bottom 16 bits, r4 */ );
;
; Effect: dest[i=0..width-1] = src[i] + weight * (src[src_pitch+i] - src[i]) / 65536
;
; Runs faster when dest, src and src_pitch are 16-byte aligned, but not compulsory.

.globl vc_image_const_0_15
.globl vclib_memcpy

.section .text$vcir8_rowinterp,"text"
.globl vcir8_rowinterp
vcir8_rowinterp:
   addcmpbne r2, r0, r0, ri8_nonz  ; r2 marks the end of destination array
   b lr
ri8_nonz:
   bmask r4, 16
vcir8_rowinterp_entry_from_mixrow:
   ;addcmpbgt r4, 0, 63, ri8_notjustmemcpy
   ;sub r2, r0
   ;b vclib_memcpy              ; If weight is zero, just memcpy the upper row
ri8_notjustmemcpy:
   stm r6-r8,lr,(--sp)
   mov r6, 16                  ; r6 is constant 16
   mov r7, 32                  ; r7 is constant 32
   sub r8, r2, r7              ; r8 is 32 before the end of dest
   add r5, pc, pcrel(vc_image_const_0_15)
   vld H(63,0), (r5)           ; magic number sequence 0..15
   mov r5, r0
   bmask r5, 4                 ; Misaligned starting destination?
   vsub -, H(63,0), r5 SETF    ; N for pixels off the start. NN for valid ones.
   sub r0, r5                  ; Backtrack destination to be aligned
   sub r1, r5                  ; Backtrack source pointer to compensate
   or r5, r1, r3
   bmask r5, 4
   cmp r5, 0                   ; Scalar Z flag set when source is aligned   

ri8_loop:
   sub r5, r2, r0                    ; Number of pixels remaining
   vrsub -, H(63,0), r5+-1 IFNN SETF ; NN for valid pixels. N off the end.
   vld H(4,0), (r0) IFN              ; Load in the pixels *not* to be modified.
ri8_loop_allvalid:
   vldleft H(0++,0), (r1+=r3) REP 2
;   vldright H(0++,0), (r1+=r3) REP 2
   vsub HX(1,32), H(1,0), H(0,0)     ; NB: hard to use accumulators here due
   vmulhn.su HX(1,0), HX(1,32), r4   ; to saturation; doing it this way gives
   vadd H(4,0), H(0,0), HX(1,0) IFNN ; us maximal accuracy but is a bit slow...
   add r1, r6
   vst H(4,0), (r0)
   vmov -, 0 SETF                    ; Clear the vector N flags
   addcmpbls r0, r6, r8, ri8_32remain
   addcmpblo r0, 0, r2, ri8_loop
   ldm r6-r8,pc,(sp++)

ri8_32remain:
   ; If at least 32 to do and source rows are aligned, go to the fast version
   bne ri8_loop_allvalid  ; source was not aligned (scalar Z clear)
   add r3, r1             ; within fastloop, r3 is address of lower row
   mov r5, 16             ; within fastloop, r5 is constant 16
   
ri8_fastloop:
   vld H(0++,0), (r1+=r5) REP 2
   vld H(2++,0), (r3+=r5) REP 2
   add r1, r7
   vsub HX(2++,32), H(2++,0), H(0++,0) REP 2
   vmulhn.su HX(2++,0), HX(2++,32), r4 REP 2
   vadd H(4++,0), H(0++,0), HX(2++,0) REP 2
   add r3, r7
   vst H(4++,0), (r0+=r5) REP 2
   addcmpbls r0, r7, r8, ri8_fastloop
   sub r3, r1             ; r3 is an offset again
   addcmpblo r0, 0, r2, ri8_loop
   ldm r6-r8,pc,(sp++)


.stabs "vcir8_rowinterp:F1",36,0,0,vcir8_rowinterp




; void vcir8_rowmix(unsigned char * dest,         r0
;                   const unsigned char * src,    r1
;                   int width,                    r2
;                   int src_pitch,                r3
;                   int nrows);                   r4
;
; Effect: dest[i=0..width-1] = SUM(j=0..nrows-1)src[i+j*src_increment] / nrows

.section .text$vcir8_rowmix,"text"
.globl vcir8_rowmix
vcir8_rowmix:
   ; First we catch some degenerate cases, and special cases for
   ; when the number of rows to be mixed is equal to two or unity.
   addcmpbne r2, r0, r0, vcir8_rowmix_nonzwidth
   b lr
vcir8_rowmix_nonzwidth:
   addcmpbgt r4, -1, 1, vcir8_rowmix_general  ; NB: r4 is one lower
   shl r4, 15  ; 2 rows -> weight=32768,  1 row -> weight=0
   b vcir8_rowinterp_entry_from_mixrow
vcir8_rowmix_general:
   stm r6-r8,lr,(--sp)
   mov r6, 16                  ; r6 is constant 16
   sub r8, r2, r6              ; r8 is 16 before the end of dest
   add r5, pc, pcrel(vc_image_const_0_15)
   vld H(63,0), (r5)           ; magic number sequence 0..15
   mov r5, r0
   bmask r5, 4                 ; Misaligned starting destination?
   vsub -, H(63,0), r5 SETF    ; N for pixels off the start. NN for valid ones.
   sub r0, r5                  ; Backtrack destination to be aligned
   sub r1, r5                  ; Backtrack source pointer to compensate
   addcmpble r4, 1, 16, rm8g_three_to_sixteen
rm8g_hack_loop:                ; Hack: reduce number of rows to 16 or less 
   shl r3, 1
   asr r4, 1
   addcmpbgt r4, 0, 16, rm8g_hack_loop
rm8g_three_to_sixteen:
   add r5, pc, pcrel(vc_image_recip_0_16)
   ld r5, (r5 + r4 << 2)       ; r5 is 65536 / nrows
   vmov HX(0,32), r5

rm8g_loop:
   sub r5, r2, r0                    ; Number of pixels remaining
   vrsub -, H(63,0), r5+-1 IFNN SETF ; NN for valid pixels. N off the end.
   vld H(4,0), (r0) IFN              ; Load in the pixels *not* to be modified.
rm8g_loop_allvalid:
   ; Load in the first 2 rows of source. We know there are at least 3.
   vldleft H(0++,0), (r1+=r3) REP 2
;   vldright H(0++,0), (r1+=r3) REP 2
   vadd HX(3,0), H(0,0), H(1,0)
   addscale r5, r1, r3 << 1
   mov r7, r4
rm8g_sumloop:
   vldleft H(0,0), (r5)
;   vldright H(0,0), (r5)
   vadd HX(3,0), HX(3,0), H(0,0)
   add r5, r3
   addcmpbgt r7, -1, 2, rm8g_sumloop
   vmulhn.su H(4,0), HX(3,0), HX(0,32) IFNN ; Divide by number of rows
   vmov -, 0 SETF
   add r1, r6
   vst H(4,0), (r0)
   addcmpbls r0, r6, r8, rm8g_loop_allvalid
   addcmpblo r0, 0, r2, rm8g_loop
   ldm r6-r8,pc,(sp++)

.stabs "vcir8_rowmix:F1",36,0,0,vcir8_rowmix

.FORGET
