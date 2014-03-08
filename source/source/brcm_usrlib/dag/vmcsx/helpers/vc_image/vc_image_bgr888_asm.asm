; ===========================================================================
; Copyright (c) 2007-2014, Broadcom Corporation
; All rights reserved.
; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
; ===========================================================================

; Assembly implementation of swapping
#include "vcinclude/common.inc"

; ================================================================
; NAME vc_rgb_swap
; SYNOPSIS void vc_rgbswaprgb(uint8_t* dest, uint8_t* src, uint32_t width, uint32_t height, uint32_t src_pitch, uint32_t dest_pitch)
; FUNCTION: swapping the r and b component of a rgb 24bit image
; ARGUMENT: destination, source, width, height, src pitch, dest pitch (allow the flexibility of not having pitch != width*3
; pixels are assumed to be in consecutive memory location
; This function only supports 24bpp value
; ================================================================

; register usage
; r0 - current destination pointer
; r1 - current source pointer
; r2 - width left to write for the current batch of 16 rows
; r3 - height left to write
; r4 - pitch for each source batch
; r5 - pitch for each destination batch
; r6 - for calculating mask and row indexing counter in VRF
; r7 - start of destination pointer for the next batch
; r8 - start of source pointer for the next batch
; r9 - reset width after each batch
; r10 - scratch
.function vc_rgbswaprgb
   stm r6-r10, lr, (--sp)
   mov r7, r0
   mov r8, r1
   mov r9, r2
   
   vbitplanes -,0x7fff SETF ;remember only 15 out of 16 values are valid
   
loop:
   ; Load two blocks of 15x16 values, so loading 10 pixels x 16 rows each time
   ; V(0,15) and V(0,31) are rubbish
   vldleft  H(0++,0), (r1+=r4) REP 16
   vldright H(0++,0), (r1+=r4) REP 16
   vldleft  H(0++,16), (r1+15+=r4) REP 16
   vldright H(0++,16), (r1+15+=r4) REP 16

   ;swap 0 with 2, 3 with 5, etc
   ;but operate as 16-bit pairs so we are processing the two blocks in parallel
   ;swapping a & b: a=a^b b=a^b a=a^b
   veor VX(0,0), VX(0,0), VX(0,2)
   veor VX(0,2), VX(0,0), VX(0,2)
   veor VX(0,0), VX(0,0), VX(0,2)

   veor VX(0,3), VX(0,3), VX(0,5)
   veor VX(0,5), VX(0,3), VX(0,5)
   veor VX(0,3), VX(0,3), VX(0,5)

   veor VX(0,6), VX(0,6), VX(0,8)
   veor VX(0,8), VX(0,6), VX(0,8)
   veor VX(0,6), VX(0,6), VX(0,8)

   veor VX(0,9), VX(0,9), VX(0,11)
   veor VX(0,11), VX(0,9), VX(0,11)
   veor VX(0,9), VX(0,9), VX(0,11)

   veor VX(0,12), VX(0,12), VX(0,14)
   veor VX(0,14), VX(0,12), VX(0,14)
   veor VX(0,12), VX(0,12), VX(0,14)

   ; Now write the values back
   addcmpblt r2, 0, 10, less_than_10_col  ; we are on the right of the image
   addcmpblt r3, 0, 16, less_than_16_row  ; we are at the bottom of the image
   
   ; Normal case: write back 10pixels x 16 rows   
   vstleft H(0++,0), (r0+=r5) REP 16 IFN
   vstright H(0++,0), (r0+=r5) REP 16 IFN
   vstleft H(0++,16), (r0+15+=r5) REP 16 IFN
   vstright H(0++,16), (r0+15+=r5) REP 16 IFN

   ; adjust the pointers
   sub r2, 10
   addcmpble r2, 0, 0, next_batch
   add r0, 30
   add r1, 30
   b loop ; next block of 10 pixels x 16 rows
   
less_than_16_row: ; but full 10 pixels
   mov r10, 0
   mov r6, 0
0:
   vstleft H(0,0)+r6, (r0) IFN
   vstright H(0,0)+r6, (r0) IFN
   vstleft H(0,16)+r6, (r0+15) IFN
   vstright H(0,16)+r6, (r0+15) IFN
   add r0, r5
   add r6, 64  ;mov to the next row of the VRF
   addcmpblt r10, 1, r3, 0b
   sub r2, 10
   addcmpble r2, 0, 0, next_batch
   mul r10, r5
   sub r0, r10  ;reset r0 for next lot of 10 pixels
   add r0, 30
   add r1, 30
   b loop ; next block of 10 pixels x n rows
   
less_than_10_col: ;Less than 10 col
   ; we need two masks for writing out, the first one is 0x7fff if col >= 5 and 2^col-1 otherwise 
   ; the second one is 0 if col <= 5, and 2^n-1 where n is col-5 otherwise
   addcmpblt r2, 0, 5, less_than_5_col
   ; we only write out the second group of 5 pixels if we have more than 5 col left
   sub r10, r2, 5
   mul r10, 3
   mov r6, 1
   shl r6, r10
   sub r6, 1   ; r6 is now our mask for the second group of 5 pixels
   vbitplanes -, r6 SETF

   ; check if we have < 16 rows (the bottom right hand corner  
   addcmpblt r3, 0, 16, less_than_16_row2
   vstleft H(0++,16), (r0+15+=r5) IFN REP 16
   vstright H(0++,16), (r0+15+=r5) IFN REP 16

   vbitplanes -, 0x7fff SETF

   b 2f  ; write the first batch of 5 pixels

less_than_16_row2:
   mov r10, 0
   mov r6, 0
1:
   vstleft H(0,16)+r6, (r0+15) IFN
   vstright H(0,16)+r6, (r0+15) IFN
   add r0, r5
   add r6, 64
   addcmpblt r10, 1, r3, 1b
   mul r10, r5
   sub r0, r10  ;reset r0 for writing the first 5 pixels
   vbitplanes -, 0x7fff SETF
   b 2f

less_than_5_col:
   mul r10, r2, 3
   mov r6, 1
   shl r6, r10
   sub r6, 1   ; r6 is now our mask for the first group of 5 pixels
   vbitplanes -, r6 SETF

2:
   ; check if we have < 16 rows (the bottom right hand corner  
   addcmpblt r3, 0, 16, less_than_16_row3
   vstleft H(0++,0), (r0+=r5) IFN REP 16
   vstright H(0++,0), (r0+=r5) IFN REP 16
   vbitplanes -,0x7fff SETF 

next_batch:
   ;We are done for the current 16 rows, adjust the pointer in destination and source
   sub r3, 16
   addcmpble r3, 0, 0, done ; if we have no rows left too, we are done
   addscale r7, r4 << 4
   addscale r8, r5 << 4
   mov r0, r7
   mov r1, r8
   mov r2, r9  ;reset width counter
   b loop
   
less_than_16_row3:
   mov r10, 0
   mov r6, 0
1:
   vstleft H(0,0)+r6, (r0) IFN
   vstright H(0,0)+r6, (r0) IFN
   add r0, r5
   add r6, 64
   addcmpblt r10, 1, r3, 1b
   ; When we get here we must have done, so return
done:
   ldm r6-r10, pc, (sp++)
.endfn
.FORGET
