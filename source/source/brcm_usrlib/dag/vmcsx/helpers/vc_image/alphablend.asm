;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

#include "vcinclude/common.inc"

; =============================================================================
; NAME: make_alpha_gradient

;  void make_alpha_gradient( unsigned char* buffer, int start_alpha, int end_alpha, 
;                             int width, int height, int pitch)

; ARGUMENTS: pointer to buffer,
;            starting alpha value,
;            ending alpha value,
;            width,
;            height,
;            pitch,

; FUNCTION: Make a horizontal alpha gradient mask for alpha blending

; RETURN:   the gradient mask is written to the passed pointer
; =============================================================================

; VRF Usage:
; H(0,0) - i for multiplying step size 
; HX(1,0) - constant vector b-a
; HX(2,0) - i*step
; H(3,0) - final results

; Register usage
; r2 - reused to store the difference (end alpha value - start alpha value) and the pitch
; r3 - remaining width to be written
; r5 - reused to store the step size
; r6 - point to the top row of buffer
; r7 - tmp value for counting no. of rows to output
; 

.const zero_to_fifteen, 32 ; constants 0-15
   .byte 0
   .byte 1
   .byte 2
   .byte 3
   .byte 4
   .byte 5
   .byte 6
   .byte 7
   .byte 8
   .byte 9
   .byte 10
   .byte 11
   .byte 12
   .byte 13
   .byte 14
   .byte 15
.endconst

.function make_alpha_gradient

   stm r6-r7, lr, (--sp)

   add r6, pc, pcrel(zero_to_fifteen)
   vld H(0,0), (r6)   
   sub r2, r1        ; r2 = b-a
   vmov HX(1,0), r2  ; HX(1,0) is now the difference bewteen the two alpha values
   mov r2, r5        ; r2 is now the pitch
   mov r5, 0xffff
   sub r6, r3, 1    
   div.uu r5, r5, r6 ;step size scaled up by 65536

   mov r6, r0
start:
   ; compute (i * step * (b-a) >> 16) + a
   vmul HX(2,0), H(0,0), r5   ; i*step
   vmulhn.su HX(3,0), HX(1,0), HX(2,0) ; i*step*(b-a) >> 16
   vadds H(3,0), HX(3,0), r1           ; H(3,0) now has the answer

   ;Store out a column of 16 alpha values
   mov r0, r6
   mov r7, r4
.pragma offwarn      ; The assembler doesn't like a REP modifier without a ++.  Here += is used instead.
0:
   vst H(3,0), (r0 += r2) REP 16     
.pragma popwarn   
   addscale r0, r2 << 4
   sub r7, 16
   addcmpbgt r7, 0, 0, 0b
   
   ;move on to the column of next 16 values
   add r6, 16
   sub r3, 16
   vadd H(0,0), H(0,0), 16
   addcmpbgt r3, 0, 0, start
   
   ldm r6-r7, pc, (sp++)
   
.endfn


; =============================================================================
; NAME: make_alpha_gradient2

;  void make_alpha_gradient2( unsigned char* buffer, int start_alpha, int end_alpha, 
;                             int width, int height, int pitch, unsigned char *buffer2)

; ARGUMENTS: pointer to buffer,
;            starting alpha value,
;            ending alpha value,
;            width,
;            height,
;            pitch,
;            pointer to second buffer

; FUNCTION: Make two horizontal alpha gradient masks for alpha blending, one is of
;           size width x height, the other is of (width/2) x (height/2)

; RETURN:   the gradient mask is written to the passed pointer
; =============================================================================

; Register usage
; r2 - reused to store the difference (end alpha value - start alpha value) and the pitch
; r3 - remaining width to be written
; r4 - remaining height to be written
; r5 - reused to store the step size
; r6 - point to the top row of buffer
; r7 - tmp value for counting no. of rows to output
; r8 - pointer to second buffer
; r9 - tmp storage for height


.function make_alpha_gradient2

   stm r6-r10, lr, (--sp)

   add r6, pc, pcrel(zero_to_fifteen)
   vld H(0,0), (r6)   
   sub r2, r1        ; r2 = b-a
   vmov HX(1,0), r2  ; HX(1,0) is now the difference bewteen the two alpha values
   mov r2, r5        ; r2 is now the pitch
   mov r5, 0xffff
   sub r6, r3, 1    
   div.uu r5, r5, r6 ;step size scaled up by 65536
   ld r8, (sp+24)    ;pointer to second buffer

   mov r7, r0
   asr r6, r2, 1     ; r6 is now half the pitch
   
   ;preset a mask for writing out 8 values at a time for U/V
   vbitplanes -, 0xff SETF
   mov r10, r1
start:
   vmul HX(2,0), H(0,0), r5   ; i*step
   vmulhn.su HX(3,0), HX(1,0), HX(2,0) ; i*step*(b-a) >> 16
   
   ; shift the value left by 1 element so
   ; HX(4,0) has the value (i+1)*step plus 
   ; rubbish in the last element
   ; This is for the U/V mask
   vadd H(4,0),  H(2,1), 0
   vadd H(4,16), H(2,17), 0
   vadd HX(2,0), HX(2,0), 1
   vlsr HX(2,0), HX(2,0), 1
   vadd HX(4,0), HX(4,0), 1
   vlsr HX(4,0), HX(4,0), 1 
   vadd HX(4,0), HX(2,0), HX(4,0) ;this is the value of i*step for U/V, computed as the average of the two values on either side of it from Y

   mov r1, r10
   ; compute (i * step * (b-a) >> 16) + a
   vadds H(3,0), HX(3,0), r1           ; H(3,0) now has the mask value for Y

   ; repeat this for U/V mask
   vmulhn.su HX(4,0), HX(1,0), HX(4,0) ; i*step*(b-a) >> 16
   vadds H(4,0), HX(4,0), r1           ; H(4,0) now has the mask value for U/V (but in positions 0, 2, ... 14)

   vfrom32l H(5,0), H(4,0), H(4,0)     ; now H(5,0) will have values from positions 0,2,4 ..14, 0,2,4..14 from H(4,0)
   
   ;Now write out both Y and U/V masks
   mov r0, r7
   mov r1, r8
   mov r9, r4
.pragma offwarn      ; The assembler doesn't like a REP modifier without a ++.  Here += is used instead.
0:
   vst H(3,0), (r0 += r2) REP 16     
   vstleft H(5,0), (r1 += r6) REP 8 IFN ; we only have 8 rows of U/V each time, 8 values for each row
.pragma popwarn
   addscale r0, r2 << 4
   addscale r1, r6 << 3
   sub r9, 16
   addcmpbgt r9, 0, 0, 0b
   
   ;move on to the column of next 16 values (for Y) and 8 values (for U/V)
   add r7, 16  ; next 16 column of Y
   add r8, 8   ; next 8 column of U/V
   sub r3, 16
   vadd H(0,0), H(0,0), 16
   addcmpbgt r3, 0, 0, start
   
   ldm r6-r10, pc, (sp++)


.endfn

; =============================================================================
; NAME: vc_image_alpha_blend_byte

;  void vc_image_alpha_blend_byte( unsigned char *dest, unsigned char *src, 
;                                  int width, int height
;                                  int dest_pitch, int source_pitch,
;                                  unsigned char *alpha_mask, int mask_pitch )          
;

; ARGUMENTS: 

; FUNCTION: Use the mask to alphablend between dest and src
;           dest buffer will be overwritten, an alpha value of 0 correspond to
;           pixel value in dest and alpha value of 255 correspond to pixel 
;           value in source. The mask is assumed to have dimensionw width x height
;           with pitch mask_pitch
;
;           The offsets in source and destination are assumed taken care of
;           in the caller so blend ALWAYS start from top left corner here.

; RETURN:   updated buffer in dest
; =============================================================================

; VRF Usage:
; H(0++,0) - current 16x16 samples of source image
; H(0++,16) - current 16x16 samples of target image (background)
; H(16++,0) - current 16x16 samples of alpha mask
; HX(0++,32) - target - source

; registers usage:
; r0 - pointer to destination
; r1 - pointer to source
; r2 - pointer to destination if we have fewer than 16 rows to save
; r3 - alpha pitch
; r4 - dest pitch
; r5 - source pitch
; r6 - pointer to alpha mask
; r7 - width
; r8 - height
; r9, r10, r11 - for temp storing the pointers to dest, src, mask
; r12 - temp storage for width
; r13 - tmp storage for height

.function vc_image_alpha_blend_byte

   stm r6-r13 ,lr, (--sp)

   ; pointer to alpha mask
   ld r6, (sp+36)
   mov r7, r2
   mov r8, r3
   ld r3, (sp+40) ; alpha pitch

   ; precompute the mask for the right side boundary of the image
   and r9, r7, 0xf   ; value of width % 16
   ; if the remainder is 15, we want a mask 0x7fff (if we use IFN flag) and so on
   mov r10, r1

   mov r1, 0xffff
   rsub r9, r9, 16
   asr r1, r9       ; r1 is now our mask
   vbitplanes -, r1 SETF  ; where the PPU has N flag, the values are valid

   mov r9, r0
   mov r11, r6
   mov r12, r7
   mov r1, r10

curr_row: ;loop for each 16x16 block over 16 rows
   ; load the source image
   vldleft H(0++,16), (r1 += r5) REP 16
;   vldright H(0++,16), (r1 += r5) REP 16
   ; load the background
   vldleft H(0++,0), (r0 += r4) REP 16
;   vldright H(0++,0), (r0 += r4) REP 16
   ; load alpha (this is always aligned
   vld H(16++,0), (r6 += r3) REP 16
   
   vsub HX(0++,32), H(0++,16), H(0++,0) REP 16
   vmulms HX(0++,32), HX(0++,32), H(16++,0) REP 16
   
   vadds H(0++,0), HX(0++,32), H(0++,0)  REP 16 ;These are now the final values

   ; check if we have 16 rows to save, if not
   ; do a simple for loop, otherwise just save 
   ; 16 rows as normal
   mov r13, 0
   mov r2, r0
   addcmpblt r8, 0, 16, less_than_16row
   
   ; check if this is the last batch of 16 samples,
   ; if the width is not a multiple of 16 and
   ; we are writing the last batch, not all 16 samples
   ; are valid
   addcmpblt r7, 0, 16, less_than_16col
   ; normal case: save all 16 values
   vstleft H(0++,0), (r0 += r4) REP 16
;   vstright H(0++,0), (r0 += r4) REP 16
   b cont
less_than_16col:
   ; we have already use set the mask outside the loop at the top, 
   ; so just use conditional store here
   vstleft H(0++,0), (r0 += r4) REP 16 IFN
;   vstright H(0++,0), (r0 += r4) REP 16 IFN
   b cont

less_than_16row:
   ; when we get here, we don't need r12 anymore because this is 
   ; the last stripe we are outputting, we use it for saving r0
   ; which is used in turn for calculating the offsets to VRF
   mov r12, r0 
0:
   shl r0, r13, 6
   addcmpblt r7, 0, 16, less_than_16colb
   vstleft H(0,0)+r0, (r2)
;   vstright H(0,0)+r0, (r2)

less_than_16colb:
   vstleft H(0,0)+r0, (r2) IFN
;   vstright H(0,0)+r0, (r2) IFN
   add r2, r4
   addcmpblt r13, 1, r8, 0b

   mov r0, r12 ; restore r0 pointer for the next x rows of 16 samples

cont:   
   add r6, 16
   add r0, 16
   add r1, 16
   sub r7, 16
   addcmpbgt r7, 0, 0, curr_row
   ; next 16 row, increment the pointers
   addscale r9, r9, r4 << 4
   addscale r10, r10, r5 << 4
   addscale r11, r11, r3 << 4
   mov r0, r9
   mov r1, r10
   mov r6, r11
   sub r8, 16
   mov r7, r12
   addcmpbgt r8, 0, 0, curr_row
   
   ldm r6-r13, pc, (sp++)
.endfn
.FORGET
; =============================================================================
; NAME: vc_image_alpha_blend_byte2

;  void vc_image_alpha_blend_byte2( unsigned char *dest, unsigned char *src, 
;                                  int width, int height
;                                  int dest_pitch, int source_pitch,
;                                  unsigned char *alpha_mask, int mask_pitch )          
;

; ARGUMENTS: 

; FUNCTION: Same as vc_image_alpha_blend_byte above, but this one is for U/V
;           component, so works on 8 rows/16 cols rather than 16 rows/16 cols at a time.

; RETURN:   updated buffer in dest
; =============================================================================

; VRF Usage:
; H(0++,0) - current 8x16 samples of source image
; H(0++,16) - current 8x16 samples of target image (background)
; H(16++,0) - current 8x16 samples of alpha mask
; HX(0++,32) - target - source

; registers usage:
; r0 - pointer to destination
; r1 - pointer to source
; r2 - pointer to destination if we have fewer than 16 rows to save
; r3 - alpha pitch
; r4 - dest pitch
; r5 - source pitch
; r6 - pointer to alpha mask
; r7 - width
; r8 - height
; r9, r10, r11 - for temp storing the pointers to dest, src, mask
; r12 - temp storage for width
; r13 - tmp storage for height

.function vc_image_alpha_blend_byte2

   stm r6-r13,lr, (--sp)

   ; pointer to alpha mask
   ld r6, (sp+36)
   mov r7, r2
   mov r8, r3
   ld r3, (sp+40) ; alpha pitch

   ; precompute the mask for the right side boundary of the image
   and r9, r7, 0xf   ; value of width % 16
   ; if the remainder is 15, we want a mask 0x7fff (if we use IFN flag) and so on
   mov r10, r1

   mov r1, 0xffff
   rsub r9, r9, 16
   asr r1, r9       ; r1 is now our mask
   vbitplanes -, r1 SETF

   mov r9, r0
   mov r11, r6
   mov r12, r7
   mov r1, r10
   
curr_row: ;loop for each 16x16 block over 16 rows
   ; load the source image
   vldleft H(0++,16), (r1 += r5) REP 8
;   vldright H(0++,16), (r1 += r5) REP 8
   ; load the background
   vldleft H(0++,0), (r0 += r4) REP 8
;   vldright H(0++,0), (r0 += r4) REP 8
   ; load alpha
   vldleft H(16++,0), (r6 += r3) REP 8
;   vldright H(16++,0), (r6 += r3) REP 8
   
   vsub HX(0++,32), H(0++,16), H(0++,0) REP 8
   vmulms HX(0++,32), HX(0++,32), H(16++,0) REP 8
   
   vadds H(0++,0), HX(0++,32), H(0++,0)  REP 8 ;These are now the final values

   ; check if we have 16 rows to save, if not
   ; do a simple for loop, otherwise just save 
   ; 16 rows as normal
   mov r13, 0
   mov r2, r0
   addcmpblt r8, 0, 8, less_than_8row
   
   ; check if this is the last batch of 16 samples,
   ; if the width is not a multiple of 16 and
   ; we are writing the last batch, not all 16 samples
   ; are valid
   addcmpblt r7, 0, 16, less_than_16col
   ; normal case: save all 16 values
   vstleft H(0++,0), (r0 += r4) REP 8
;   vstright H(0++,0), (r0 += r4) REP 8
   b cont
less_than_16col:
   ; we have already use set the mask outside the loop at the top, 
   ; so just use conditional store here
   vstleft H(0++,0), (r0 += r4) REP 8 IFN 
;   vstright H(0++,0), (r0 += r4) REP 8 IFN 
   b cont

less_than_8row:
   mov r12, r0 
0:
   shl r0, r13, 6
   addcmpblt r7, 0, 16, less_than_16colb
   vstleft H(0,0)+r0, (r2)
;   vstright H(0,0)+r0, (r2)

less_than_16colb:
   vstleft H(0,0)+r0, (r2) IFN
;   vstright H(0,0)+r0, (r2) IFN
   add r2, r4
   addcmpblt r13, 1, r8, 0b

   mov r0, r12 ; restore r0 pointer for the next x rows of 16 samples

cont:   
   add r6, 16
   add r0, 16
   add r1, 16
   sub r7, 16
   addcmpbgt r7, 0, 0, curr_row
   ; next 8 rows, increment the pointers
   addscale r9, r9, r4 << 3
   addscale r10, r10, r5 << 3
   addscale r11, r11, r3 << 3
   mov r0, r9
   mov r1, r10
   mov r6, r11
   sub r8, 8
   mov r7, r12
   addcmpbgt r8, 0, 0, curr_row
   
   ldm r6-r13, pc, (sp++)
.endfn
