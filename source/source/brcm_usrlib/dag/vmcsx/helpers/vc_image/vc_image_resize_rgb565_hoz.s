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

; Imported symbols.
.globl vc_image_const_0_15
.globl vc_image_recip_0_16

; UTILITY FUNCTION
;
; void vcir16_margin(unsigned short * dest, const unsigned short * src,
;                   int width_pels, int height, int dest_pitch, int src_pitch)
;
; For each line, repeatedly copy the pixel at src to a horizontal scan at dest.

.section .text$vcir16_margin,"text"
.globl vcir16_margin
vcir16_margin:
   addcmpbgt r3, 0, 0, mgn16_nonzh
   b lr
mgn16_nonzh:
   stm r6-r7,lr,(--sp)
   mul r3, r4, r3
   add r3, r0  ; destination end pointer
   addcmpbeq r2, 0, 1, mgn16_width1short
   shl r2, 1
   sub r4, r2 

mgn16_widthn:
   ldh r6, (r1)
   add r7, r0, r2
mgn16_loop:
   sth r6, (r0)
   addcmpbne r0, 2, r7, mgn16_loop
   add r1, r5
   addcmpblo r0, r4, r3, mgn16_widthn
   ldm r6-r7,pc,(sp++)

mgn16_width1short:
   ldh r6, (r1)
   add r1, r5
   sth r6, (r0)
   addcmpblo r0, r4, r3, mgn16_width1short
   ldm r6-r7,pc,(sp++)


vcir16_interp_init:
   add r6, r0, r2     ; r6 points to end of destination array
   mov r7, 64
   sub r8, r6, r7     ; r8 points 32 PIXELS before end of destination array
   mov r7, 32         ; r7 is constant 32
   mov r10, r4        ; r10 is step
   ld r11, (sp+32)    ; r11 is source pitch
   mov r12, r5        ; r12 is dest pitch

   add r2, pc, pcrel(vc_image_const_0_15)
   vld H(63,0), (r2)           ; magic number sequence 0..15
   vshl H(63,0), H(63,0), 1    ; double it
   
   mov r2, r0
   bmask r2, 5                 ; Misaligned starting destination?
   vsub -, H(63,0), r2 SETF    ; N for pixels off the start. NN for valid ones.
   sub r0, r2                  ; Backtrack destination to be aligned
   asr r2, 1
   mul r2, r4
   sub r3, r2                  ; Backtrack .16 source position to compensate
   
   mov r2, r1
   bmask r2, 5                 ; Misaligned source pointer?
   sub r1, r2                  ; Backtrack source to be aligned
   shl r2, 15
   add r3, r2                  ; Advance source position to compensate

   ; Note: there's a chance r3 is negative. We'll get undefined pixels at
   ; -ve positions. This is not a bug: such pixels should only arise due
   ; to alignment corrections and so will be N-flagged anyway. r3 may also
   ; have grown enough to take us into the second block of source.

   mov r9, r1                  ; r9 is source start
   cmp r6, r8                  ; Shortcut (BLS) will fail initially
 
   b lr  ; We now return you to our main feature

; ==== HORIZONTAL RESIZE: NEAREST NEIGHBOUR METHOD ===
; Based on the above, with some simplifications. Source coordinates in .16
; form are always rounded down to determine the "nearest neightbour".

; void vcir16_nnbr_16(unsigned char * dest,      /*  r0  */
;                     const unsigned char * src, /*  r1  */
;                     int dest_width_bytes,      /*  r2  */
;                     int pos,                   /*  r3  */
;                     int step,                  /*  r4  */
;                     int dest_pitch_bytes,      /*  r5  */
;                     int src_pitch_bytes);      /* (sp) */

.section .text$vcir16_nnbr_16,"text"
.globl vcir16_nnbr_16
vcir16_nnbr_16:
   stm r6-r12,lr,(--sp)
   
   bl vcir16_interp_init ; Initialize as above
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   [short index to next 16 columns]
   ; r2   [temporary]
   ; r3   source position in .16 format
   ; r4   [inner loop counter]
   ; r5   initially dest_pitch [later also src_pitch]
   ; r6   end of destination top line
   ; r7   constant #32
   ; r8   32 before end of destination top line
   ; r9   source start pointer (aligned)
   ; r10  step size in .16 format
   ; r11  source_pitch
   ; r12  dest_pitch
   ; BLS branch will fail at first. [Succeeds when all 16 columns valid.]
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains twice the "magic" byte sequence (0..30)

   mov r1, 0   ; Nothing fetched yet
   
n1616_loop:
   mov r4, 0  ; for each of the 16 pixels in destination block:
   
n1616_loop2:
   mov r2, r3
   asr r2, 16
   addcmpblt r2, 0, r1, n1616_noload

   ; Load the block of source pixels containing r2 in top line.
   ; Because we don't have to interpolate, only one block is ever
   ; required at a time, and we always put it at HX(0,0)..HX(15,0).
   mov r5, r11
   mov r1, r2
   lsr r1, 4
   shl r1, 5
   add r1, r9
   vld HX(0++,0), (r1+=r5) REP 16
   sub r1, r9
   add r1, r7
   asr r1, 1
   mov r5, r12

n1616_noload:
   bmask r2, 4           ; X offset into vector register file for nn pixel
   vadd VX(32,0)+r4, VX(0,0)+r2, 0
   add r3, r10
   addcmpblt r4, 1, 16, n1616_loop2

   ; WARNING: This code uses long-distance flags, assume unchaged by addcmpbxx
   bls n1616_shortcut
   sub r2, r6, r0                      ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld HX(32++,0), (r0+=r5) IFN REP 16 ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

n1616_shortcut:
   vst HX(32++,0), (r0+=r5) REP 16     ; Store this output block!
   cmp r0, r8                          ; If at least 16 remain, take shortcut
   addcmpblo r0, r7, r6, n1616_loop
   ldm r6-r12,pc,(sp++)


.stabs "vcir16_nnbr_16:F1",36,0,0,vcir16_nnbr_16


; void vcir16_nnbr_1(unsigned char * dest,        r0
;                    const unsigned char * src,   r1
;                    int dest_width_bytes,        r2
;                    int pos,                     r3
;                    int step)                    r4
;
; If we end up using this much, we're in trouble; it doesn't use
; the vector unit and is quite slow. XXX could try using vlookupm?

.section .text$vcir16_nnbr_1,"text"
.globl vcir16_nnbr_1
vcir16_nnbr_1:
n161_top:
   addcmpbgt r2,0,0,n161_nonz
   b lr
n161_nonz:
   stm r6-r7,lr,(--sp)
   or r5, r0, r2
   bmask r5, 5
   addcmpbeq r5, 0, 0, n161_aligned
   mov r6, 16
n161_loop:
   asr r5, r3, r6    ; r5 is source index
   ldh r5, (r1+r5<<1)
   add r3, r4
   sth r5, (r0++)
   addcmpbgt r2,-2,0,n161_loop
   ldm r6-r7,pc,(sp++)

n161_aligned:
   mov r6, 12
   mov r7, 32
   vmov HX(0,0), 0
   mov r5, r4
   asr r5, 9
   vbitplanes -, 0xff00 SETF
   vmov HX(0,0), r5 IFNZ
   asr r5, 1
   vbitplanes -, 0xf0f0 SETF
   vadd HX(0,0), HX(0,0), r5 IFNZ
   asr r5, 1
   vbitplanes -, 0xcccc SETF
   vadd HX(0,0), HX(0,0), r5 IFNZ
   asr r5, 1
   vbitplanes -, 0xaaaa SETF
   vadd HX(0,0), HX(0,0), r5 IFNZ
   add r2, r0
n161_aligned_loop:
   asr r5, r3, r6
   vadd HX(1,0), HX(0,0), r5
   vasr -, HX(1,0), 4 CLRA ACC
   addscale r3, r4<<4
   vlookupm HX(2,0), (r1)
   vst HX(2,0), (r0)
   addcmpbne r0, r7, r2, n161_aligned_loop
   ldm r6-r7,pc,(sp++)

.stabs "vcir16_nnbr_1:F1",36,0,0,vcir16_nnbr_1

.FORGET
