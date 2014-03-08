;;; ===========================================================================
;;; Copyright (c) 2005-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

.define N_cx,  0
.define N_cy,  1
.define N_ascale, 2
.define N_src,    3

.define N_srcred,   16
.define N_srcgreen, 17
.define N_srcblue,  18
.define N_red,      19
.define N_green,    20
.define N_blue,     21
.define N_frac,     22
.define N_ifrac,    23
.define N0to15,     25 ; H(25,0)
.define N_src1,     26
.define N_gradient, 28
.define N_temp,    29
.define N_col,     30
.define N_width,   31
.define N_height,  32
.define N_pitch,   33
.define N_incxx_0to15, 37
.define N_incxy_0to15, 39

.define Rsrc, r0
.define Rinc, r1
.define Rend, r2
.define Rtemp, r5

; void vc_image_line_x (VC_IMAGE_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour);
.function vc_image_line_x ; {{{
   vmov HX(N_cx, 32), r1
   vmov HX(N_cy, 0), 0
   vmov HX(N_cy, 32), r3

   VMOV_H32 N_gradient, r4
   VMOV_H32 N_col, r5

   vc_image_width Rtemp, r0
   vmov HX(N_width, 0), Rtemp
   vc_image_height Rtemp, r0
   vmov HX(N_height, 0), Rtemp
   vmin HX(N_width, 0), HX(N_width, 0), r2

   vc_image_pitch Rtemp, r0
   vmov HX(N_pitch, 0), Rtemp

   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)

   vadd HX(N_cx, 32), HX(N_cx, 32), H(N0to15, 0)

   vmov -, HX(N_cy, 0) CLRA UACC
   vmul HX(N_cy, 0), HX(N_gradient, 0), H(N0to15, 0) UACC
   vmov -, HX(N_cy, 32) ACC
   vmulhd.uu -, HX(N_gradient, 0), H(N0to15, 0) ACC
   vmul HX(N_cy, 32), HX(N_gradient, 32), H(N0to15, 0) ACC

   vmul HX(N_incxy_0to15, 0), HX(N_gradient, 0), 16 CLRA UACC
   vmulhd.uu -, HX(N_gradient, 0), 16 ACC
   vmul HX(N_incxy_0to15, 32), HX(N_gradient, 32), 16 ACC

   vc_image_bpp_switch Rtemp, r0, .vc_image_line_x_1bpp, .vc_image_line_x_2bpp, .vc_image_line_x_3bpp, .vc_image_line_x_default, .vc_image_line_x_default
.vc_image_line_x_1bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC
   vindexwritem H(N_col,0), (Rsrc) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr

.vc_image_line_x_3bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmul      -, HX(N_cx, 32), 3              UACC
   vindexwritem H(N_col, 0), (Rsrc+0) UACC IFN
   vindexwritem H(N_col,16), (Rsrc+1) UACC IFN
   vindexwritem H(N_col,32), (Rsrc+2) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr

.vc_image_line_x_default:
   bkpt
   b lr
.vc_image_line_x_2bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
   vlsr HX(N_pitch, 0), HX(N_pitch, 0), 1 ; lookup is in shorts
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC
   vindexwritem HX(N_col,0), (Rsrc) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr
.endfn ; }}}

; void vc_image_line_y (VC_IMAGE_T *dest, int x_start, int x_end, int y_start, int gradient, int fg_colour);
.function vc_image_line_y ; {{{
   vmov HX(N_cx, 32), r1
   vmov HX(N_cy, 0), 0
   vmov HX(N_cy, 32), r3

   VMOV_H32 N_gradient, r4
   VMOV_H32 N_col, r5

   vc_image_width Rtemp, r0
   vmov HX(N_height, 0), Rtemp
   vc_image_height Rtemp, r0
   vmov HX(N_width, 0), Rtemp
   vmin HX(N_width, 0), HX(N_width, 0), r2

   vc_image_pitch Rtemp, r0
   vmov HX(N_pitch, 0), Rtemp

   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)

   vadd HX(N_cx, 32), HX(N_cx, 32), H(N0to15, 0)

   vmov -, HX(N_cy, 0) CLRA UACC
   vmul HX(N_cy, 0), HX(N_gradient, 0), H(N0to15, 0) UACC
   vmov -, HX(N_cy, 32) ACC
   vmulhd.uu -, HX(N_gradient, 0), H(N0to15, 0) ACC
   vmul HX(N_cy, 32), HX(N_gradient, 32), H(N0to15, 0) ACC

   vmul HX(N_incxy_0to15, 0), HX(N_gradient, 0), 16 CLRA UACC
   vmulhd.uu -, HX(N_gradient, 0), 16 ACC
   vmul HX(N_incxy_0to15, 32), HX(N_gradient, 32), 16 ACC

   vc_image_bpp_switch Rtemp, r0, .vc_image_line_y_1bpp, .vc_image_line_y_2bpp, .vc_image_line_y_3bpp, .vc_image_line_y_4bpp, .vc_image_line_y_default
.vc_image_line_y_1bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cy, 32)            UACC
   vindexwritem H(N_col,0), (Rsrc) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr

.vc_image_line_y_3bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmul      -, HX(N_cy, 32), 3              UACC
   vindexwritem H(N_col, 0), (Rsrc+0) UACC IFN
   vindexwritem H(N_col,16), (Rsrc+1) UACC IFN
   vindexwritem H(N_col,32), (Rsrc+2) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr

.vc_image_line_y_default:
   bkpt
   b lr
.vc_image_line_y_2bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
   vlsr HX(N_pitch, 0), HX(N_pitch, 0), 1 ; lookup is in shorts
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cy, 32)            UACC
   vindexwritem HX(N_col,0), (Rsrc) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr

.vc_image_line_y_4bpp:
   vc_image_image_data Rsrc, r0
   mov Rtemp, 16
   vlsr HX(N_pitch, 0), HX(N_pitch, 0), 1 ; lookup is in shorts
1:
; check for clipping with source limits
   vsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vrsub  -, HX(N_cx, 32), -1            SETF IFN
   vsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFN
   vrsub  -, HX(N_cy, 32), -1            SETF IFN

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cy, 32)            UACC
   vindexwritem H32(N_col,0), (Rsrc) UACC IFN

; increment x & y index
   vadd  HX(N_cx, 32), HX(N_cx, 32), 16

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpble r1, Rtemp, r2, 1b
   b lr
.endfn ; }}}
.undef Rtemp, Rloops, Rsrc, Rend

.define Rsrc, r0
.define Rstartoffset, r1
.define Rnblocks, r2
.define Rnextracols, r3
.define Rtemp, r4
; void vc_image_line_horiz(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
.function vc_image_line_horiz ; {{{
; check y pos on screen
   vc_image_height Rtemp, r0
   addcmpblo r2, 0, Rtemp, 1f
   b lr
1:
; order start end endpoints
   min Rtemp, r1, r3
   max r3, r1
   mov r1, Rtemp
; check if completely offscreen
   addcmpbge r3, 0, 0, 1f
   b lr
1:
   vc_image_width Rtemp, r0
   addcmpblt r1, 0, Rtemp, 1f
   b lr
1:
; clip to screen width
   max r1, 0
   sub Rtemp, 1
   max r3, 0
   min r1, Rtemp
   min r3, Rtemp
   add Rtemp, pc, pcrel(vc_image_const_0_15)
   VMOV_H32 N_col, r5
   
   vld H(N0to15,0), (Rtemp)
   
   vc_image_pitch Rtemp, r0
   mul r2, Rtemp

   vc_image_bpp_switch Rtemp, r0, .vc_image_line_horiz_1bpp, .vc_image_line_horiz_2bpp, .vc_image_line_horiz_3bpp, .vc_image_line_horiz_4bpp, .vc_image_line_horiz_default

.vc_image_line_horiz_default:
   bkpt
   b lr

.vc_image_line_horiz_3bpp:
   vc_image_image_data Rsrc, r0
   sub Rtemp, r3, r1 ; draw this + 1 pixels 

   mul r1, 3      ; pixels are 24bpp
   add Rsrc, r2

   add Rsrc, r1
   lsr r1, r5, 8
   lsr r3, r5, 16
1:
   stb r5, (Rsrc++)
   stb r1, (Rsrc++)
   stb r3, (Rsrc++)
   addcmpbge Rtemp, -1, 0, 1b
   b lr

.vc_image_line_horiz_1bpp:
   vc_image_image_data Rsrc, r0
   add Rsrc, r2

   add Rnextracols, r3, 1
   add Rnblocks, r1, 15

   bic Rtemp, Rnextracols, 15
   bic Rnblocks, 15

   and Rnextracols, 15
   rsub Rnblocks, Rtemp

   asr Rnblocks, 4

   add Rsrc, r1   ; pixels are chars
   and Rstartoffset, r1, 15
.if _VC_VERSION > 2
   bic Rsrc, 15
.endif
   addcmpbge Rnblocks, 0, 0, 1f
   ;; The region to fill lies wholly within a 16-wide block.
   vsub H(62,0), H(N0to15,0), Rstartoffset SETF
   vmov H(62,0), 16 IFN
   sub Rnextracols, Rstartoffset
   vsub -, H(62,0), Rnextracols SETF
   vst H(N_col, 0), (Rsrc) IFN
   b lr

1:
   ;; Test whether the start_offset was unaligned. If so, we need to load that block and change the entries only from start_offset onwards.
   addcmpbeq Rstartoffset, 0, 0, 1f
   vsub -, H(N0to15,0), Rstartoffset SETF
   vst H(N_col, 0), (Rsrc) IFNN
   add Rsrc, 16

1:   
   addcmpbeq Rnblocks, 0, 0, 2f
   mov Rtemp, 16
   shl Rnblocks, 4            ; compute terminating address for Rsrc
   add Rnblocks, Rsrc

   ;; This loop simple blasts 16 pixels with the constant value.
1:
   vst H(N_col, 0), (Rsrc)
   addcmpbne Rsrc, Rtemp, Rnblocks, 1b
   
2:
   ;; There may be some trailing columns too. Load up the block that contains
   ;; them and modify the necessary entries.
   vsub -, H(N0to15,0), Rnextracols SETF
   vst H(N_col, 0), (Rsrc) IFN
   b lr

.vc_image_line_horiz_2bpp:
   vc_image_image_data Rsrc, r0
   add Rsrc, r2

   add Rnextracols, r3, 1
   add Rnblocks, r1, 15

   bic Rtemp, Rnextracols, 15
   bic Rnblocks, 15

   and Rnextracols, 15
   rsub Rnblocks, Rtemp

   asr Rnblocks, 4

   addscale Rsrc, r1<<1   ; pixels are shorts
   and Rstartoffset, r1, 15
.if _VC_VERSION > 2
   bic Rsrc, 31
.endif
   addcmpbge Rnblocks, 0, 0, 1f
   ;; The region to fill lies wholly within a 16-wide block.
   vsub H(62,0), H(N0to15,0), Rstartoffset SETF
   vmov H(62,0), 16 IFN
   sub Rnextracols, Rstartoffset
   vsub -, H(62,0), Rnextracols SETF
   vst HX(N_col, 0), (Rsrc) IFN
   b lr

1:
   ;; Test whether the start_offset was unaligned. If so, we need to load that block and change the entries only from start_offset onwards.
   addcmpbeq Rstartoffset, 0, 0, 1f
   vsub -, H(N0to15,0), Rstartoffset SETF
   vst HX(N_col, 0), (Rsrc) IFNN
   add Rsrc, 32

1:   
   addcmpbeq Rnblocks, 0, 0, 2f
   mov Rtemp, 32
   shl Rnblocks, 5            ; compute terminating address for Rsrc
   add Rnblocks, Rsrc

   ;; This loop simple blasts 16 pixels with the constant value.
1:
   vst HX(N_col,0), (Rsrc)
   addcmpbne Rsrc, Rtemp, Rnblocks, 1b
   
2:
   ;; There may be some trailing columns too. Load up the block that contains
   ;; them and modify the necessary entries.
   vsub -, H(N0to15,0), Rnextracols SETF
   vst HX(N_col, 0), (Rsrc) IFN
   b lr

.vc_image_line_horiz_4bpp:
   vc_image_image_data Rsrc, r0
   add Rsrc, r2

   add Rnextracols, r3, 1
   add Rnblocks, r1, 15

   bic Rtemp, Rnextracols, 15
   bic Rnblocks, 15

   and Rnextracols, 15
   rsub Rnblocks, Rtemp

   asr Rnblocks, 4

   bic Rtemp, r1, 15
   addscale Rsrc, Rtemp<<2   ; pixels are words
   and Rstartoffset, r1, 15
   addcmpbge Rnblocks, 0, 0, 1f
   ;; The region to fill lies wholly within a 16-wide block.
   vsub H(62,0), H(N0to15,0), Rstartoffset SETF
   vmov H(62,0), 16 IFN
   sub Rnextracols, Rstartoffset
   vsub -, H(62,0), Rnextracols SETF
   vst H32(N_col, 0), (Rsrc) IFN
   b lr

1:
   ;; Test whether the start_offset was unaligned. If so, we need to load that block and change the entries only from start_offset onwards.
   addcmpbeq Rstartoffset, 0, 0, 1f
   vsub -, H(N0to15,0), Rstartoffset SETF
   vst H32(N_col, 0), (Rsrc) IFNN
   add Rsrc, 64

1:   
   addcmpbeq Rnblocks, 0, 0, 2f
   mov Rtemp, 64
   shl Rnblocks, 6            ; compute terminating address for Rsrc
   add Rnblocks, Rsrc

   ;; This loop simple blasts 16 pixels with the constant value.
1:
   vst H32(N_col,0), (Rsrc)
   addcmpbne Rsrc, Rtemp, Rnblocks, 1b
   
2:
   ;; There may be some trailing columns too. Load up the block that contains
   ;; them and modify the necessary entries.
   vsub -, H(N0to15,0), Rnextracols SETF
   vst H32(N_col, 0), (Rsrc) IFN
   b lr
.endfn ; }}}
.undef Rsrc, Rstartoffset, Rnblocks, Rnextracols, Rtemp

.define Rsrc, r2
.define Rtemp, r3
.define Rend, r4
.define Rpitch, r3
; void vc_image_line_vert(VC_IMAGE_T *dest, int x_start, int y_start, int x_end, int y_end, int fg_colour);
.function vc_image_line_vert ; {{{
; check x pos on screen
   vc_image_width Rtemp, r0
   addcmpblo r1, 0, Rtemp, 1f
   b lr
1:
; order start end endpoints
   min Rtemp, r2, r4
   max r4, r2
   mov r2, Rtemp
; check if completely offscreen
   addcmpbge r4, 0, 0, 1f
   b lr
1:
   vc_image_height Rtemp, r0
   addcmpblt r2, 0, Rtemp, 1f
   b lr
1:
; clip to screen width
   max r2, 0
   sub Rtemp, 1
   max r4, 0
   min r2, Rtemp
   min r4, Rtemp

   vc_image_pitch Rpitch, r0
   mul r4, Rpitch
   mul r2, Rpitch

   vc_image_image_data Rtemp, r0
   add Rend, Rtemp, r4
   add Rsrc, Rtemp, r2

   vc_image_pitch Rpitch, r0

   vc_image_bpp_switch r0, r0, .vc_image_line_vert_1bpp, .vc_image_line_vert_2bpp, .vc_image_line_vert_3bpp, .vc_image_line_vert_4bpp, .vc_image_line_vert_default
.vc_image_line_vert_default:
   bkpt
   b lr
.vc_image_line_vert_1bpp:
   add Rsrc, r1
   add Rend, r1
1:
   stb r5, (Rsrc)
   addcmpble Rsrc, Rpitch, Rend, 1b
   b lr
.vc_image_line_vert_2bpp:
   addscale Rsrc, r1<<1
   addscale Rend, r1<<1
1:
   sth r5, (Rsrc)
   addcmpble Rsrc, Rpitch, Rend, 1b
   b lr
.vc_image_line_vert_3bpp:
   addscale Rsrc, r1<<1
   addscale Rend, r1<<1
   add Rsrc, r1
   add Rend, r1
   lsr r0, r5, 8
   lsr r1, r5, 16
1:
   stb r5, (Rsrc+0)
   stb r0, (Rsrc+1)
   stb r1, (Rsrc+2)
   addcmpble Rsrc, Rpitch, Rend, 1b
   b lr
.vc_image_line_vert_4bpp:
   addscale Rsrc, r1<<2
   addscale Rend, r1<<2
1:
   st r5, (Rsrc)
   addcmpble Rsrc, Rpitch, Rend, 1b
   b lr
.endfn ; }}}
.undef Rsrc, Rtemp, Rend, Rpitch

.define Rsrc, r0
.define Rloops, r1
.define Rpitch, r2
.define Rtemp, r3
.define Rthick, r4
.define Rthickness, r5

; void vc_plotline_aa_x(VC_IMAGE_T *im, short firstx, short lastx, int intery, int gradient, int thickness, int col);
.function vc_plotline_aa_x ; {{{
   vmov HX(N_cx, 0), 0
   vmov HX(N_cx, 32), r1

   VMOV_H32 N_cy, r3
   VMOV_H32 N_gradient, r4

   ld Rtemp, (sp)
   vmov HX(N_src, 0), Rtemp
   vmov HX(N_col, 0), Rtemp
   vmov HX(N_ascale, 0), Rthickness
   lsr Rthickness, 16

   sub Rloops, r2, r1
   add Rloops, 16
   asr Rloops, 4

   vc_image_width Rtemp, r0
   vmov HX(N_width, 0), Rtemp-1
   vc_image_height Rtemp, r0
   sub Rtemp, Rthickness ; to allow for extra line thickness
   vmov HX(N_height, 0), Rtemp-2 ; to allow for extra antialias pixel
   vmin HX(N_width, 0), HX(N_width, 0), r2

   vc_image_pitch Rpitch, r0
   asr Rtemp, Rpitch, 1 ; lookup is in shorts
   vmov HX(N_pitch, 0), Rtemp
   vc_image_image_data Rsrc, r0

; get components
   vshl HX(N_blue, 0), HX(N_src,0), 11
   vand HX(N_green, 0), HX(N_src , 0), 0x7e0
   vshl HX(N_green, 0), HX(N_green,0), 5
   vand HX(N_red, 0), HX(N_src ,0), 0xf800

   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)

   vadd HX(N_cx, 32), HX(N_cx, 32), H(N0to15, 0)

   vmov -, HX(N_cy, 0) CLRA UACC
   vmul HX(N_cy, 0), HX(N_gradient, 0), H(N0to15, 0) UACC
   vmov -, HX(N_cy, 32) ACC
   vmulhd.uu -, HX(N_gradient, 0), H(N0to15, 0) ACC
   vmul HX(N_cy, 32), HX(N_gradient, 32), H(N0to15, 0) ACC

   vmov HX(N_incxx_0to15, 0), 0
   vmov HX(N_incxx_0to15, 32), 16

   vmul HX(N_incxy_0to15, 0), HX(N_gradient, 0), 16 CLRA UACC
   vmulhd.uu -, HX(N_gradient, 0), 16 ACC
   vmul HX(N_incxy_0to15, 32), HX(N_gradient, 32), 16 ACC
.lineloop_aa:
   add Rtemp, Rsrc, Rpitch
   mov Rthick, Rthickness
; check for clipping with source limits
   vrsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vsub  -, HX(N_cx, 32), 0               SETF IFNN
   vrsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFNN
   vsub  -, HX(N_cy, 32), 0               SETF IFNN

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC
; lookup pixel
   vlookupm  HX(N_src,0), (Rsrc) UACC IFNN
1:
   addcmpblt Rthick, -1, 0, 2f
   vindexwritem HX(N_col,0), (Rtemp) UACC IFNN
   add Rtemp, Rpitch
   b 1b
2:
   vlookupm  HX(N_src1,0), (Rtemp) UACC IFNN

; get components
   vshl HX(N_srcblue, 0), HX(N_src, 0), 11
   vand HX(N_srcgreen, 0), HX(N_src, 0), 0x7e0
   vshl HX(N_srcgreen, 0), HX(N_srcgreen,0), 5
   vand HX(N_srcred, 0), HX(N_src, 0), 0xf800

; calc scaled fractions
   veor HX(N_frac, 0),  HX(N_cy, 0), 0xffff
   vmulhd.uu HX(N_frac, 0), HX(N_frac, 0), HX(N_ascale, 0)
   veor HX(N_ifrac, 0), HX(N_frac, 0), 0xffff

; average
   vmulhd.uu -, HX(N_srcblue, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcblue, 0), HX(N_blue, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcgreen, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcgreen, 0), HX(N_green, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcred, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcred, 0), HX(N_red, 0), HX(N_frac, 0) UACC

; combine again
   vmulhn.uu HX(N_srcred, 0), HX(N_srcred, 0), 1<<5
   vmulhn.uu HX(N_srcgreen, 0), HX(N_srcgreen, 0), 1<<6
   vmulhn.uu HX(N_srcblue, 0), HX(N_srcblue, 0), 1<<5 CLRA ACC
   vshl -, HX(N_srcgreen, 0), 5 ACC
   vshl HX(N_src, 0), HX(N_srcred, 0), 11 ACC

; get components
   vshl HX(N_srcblue, 0), HX(N_src1,0), 11
   vand HX(N_srcgreen, 0), HX(N_src1, 0), 0x7e0
   vshl HX(N_srcgreen, 0), HX(N_srcgreen,0), 5
   vand HX(N_srcred, 0), HX(N_src1, 0), 0xf800

; calc scaled fractions
   veor HX(N_frac, 0),  HX(N_cy, 0), 0
   vmulhd.uu HX(N_frac, 0), HX(N_frac, 0), HX(N_ascale, 0)
   veor HX(N_ifrac, 0), HX(N_frac, 0), 0xffff

; average
   vmulhd.uu -, HX(N_srcblue, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcblue, 0), HX(N_blue, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcgreen, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcgreen, 0), HX(N_green, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcred, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcred, 0), HX(N_red, 0), HX(N_frac, 0) UACC

; combine again
   vmulhn.uu HX(N_srcred, 0), HX(N_srcred, 0), 1<<5
   vmulhn.uu HX(N_srcgreen, 0), HX(N_srcgreen, 0), 1<<6
   vmulhn.uu HX(N_srcblue, 0), HX(N_srcblue, 0), 1<<5 CLRA ACC
   vshl -, HX(N_srcgreen, 0), 5 ACC
   vshl HX(N_src1, 0), HX(N_srcred, 0), 11 ACC

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC
; writeback pixel
   vindexwritem HX(N_src,0), (Rsrc) UACC IFNN
   vindexwritem HX(N_src1,0), (Rtemp) UACC IFNN
.if __VCCOREVER__ <= 0x04000100
; HW-1297 workaround - stall until the vindexwritem completes
   ldh   Rthick, (Rtemp)
   mov   Rthick, Rthick
.endif

; increment x & y index
   vmov  -, HX(N_cx, 0) CLRA UACC
   vmov  HX(N_cx, 0), HX(N_incxx_0to15, 0) UACC
   vadd  HX(N_cx, 32), HX(N_cx, 32), HX(N_incxx_0to15, 32) ACC

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpbgt Rloops, -1, 0, .lineloop_aa

   b lr
.endfn ; }}}

; void vc_plotline_aa_y(VC_IMAGE_T *im, short firstx, short lastx, int intery, int gradient, int thickness, int col);
.function vc_plotline_aa_y ; {{{
   vmov HX(N_cx, 0), 0
   vmov HX(N_cx, 32), r1

   VMOV_H32 N_cy, r3
   VMOV_H32 N_gradient, r4

   ld Rtemp, (sp)
   vmov HX(N_src, 0), Rtemp
   vmov HX(N_col, 0), Rtemp
   vmov HX(N_ascale, 0), Rthickness
   lsr Rthickness, 16

   sub Rloops, r2, r1
   add Rloops, 16
   asr Rloops, 4

   vc_image_width Rtemp, r0
   sub Rtemp, Rthickness ; to allow for extra line thickness
   vmov HX(N_width, 0), Rtemp-2 ; to allow for extra antialias pixel
   vc_image_height Rtemp, r0
   vmov HX(N_height, 0), Rtemp-1
   vmin HX(N_height, 0), HX(N_height, 0), r2

   vc_image_pitch Rpitch, r0
   asr Rtemp, Rpitch, 1 ; lookup is in shorts
   vmov HX(N_pitch, 0), Rtemp
   vc_image_image_data Rsrc, r0

; get components
   vshl HX(N_blue, 0), HX(N_src,0), 11
   vand HX(N_green, 0), HX(N_src , 0), 0x7e0
   vshl HX(N_green, 0), HX(N_green,0), 5
   vand HX(N_red, 0), HX(N_src ,0), 0xf800

   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)

   vadd HX(N_cx, 32), HX(N_cx, 32), H(N0to15, 0)

   vmov -, HX(N_cy, 0) CLRA UACC
   vmul HX(N_cy, 0), HX(N_gradient, 0), H(N0to15, 0) UACC
   vmov -, HX(N_cy, 32) ACC
   vmulhd.uu -, HX(N_gradient, 0), H(N0to15, 0) ACC
   vmul HX(N_cy, 32), HX(N_gradient, 32), H(N0to15, 0) ACC

   vmov HX(N_incxx_0to15, 0), 0
   vmov HX(N_incxx_0to15, 32), 16

   vmul HX(N_incxy_0to15, 0), HX(N_gradient, 0), 16 CLRA UACC
   vmulhd.uu -, HX(N_gradient, 0), 16 ACC
   vmul HX(N_incxy_0to15, 32), HX(N_gradient, 32), 16 ACC
.lineloop_aaY:
   add Rtemp, Rsrc, 2 ; Rpitch
   mov Rthick, Rthickness
; check for clipping with source limits
   vrsub -, HX(N_cx, 32), HX(N_height, 0) SETF
   vsub  -, HX(N_cx, 32), 0               SETF IFNN
   vrsub -, HX(N_cy, 32), HX(N_width, 0)  SETF IFNN
   vsub  -, HX(N_cy, 32), 0               SETF IFNN

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cy, 32)            UACC
; lookup pixel
   vlookupm  HX(N_src,0), (Rsrc) UACC IFNN
1:
   addcmpblt Rthick, -1, 0, 2f
   vindexwritem HX(N_col,0), (Rtemp) UACC IFNN
   add Rtemp, 2 ; Rpitch
   b 1b
2:
   vlookupm  HX(N_src1,0), (Rtemp) UACC IFNN

; get components
   vshl HX(N_srcblue, 0), HX(N_src, 0), 11
   vand HX(N_srcgreen, 0), HX(N_src, 0), 0x7e0
   vshl HX(N_srcgreen, 0), HX(N_srcgreen,0), 5
   vand HX(N_srcred, 0), HX(N_src, 0), 0xf800

; calc scaled fractions
   veor HX(N_frac, 0),  HX(N_cy, 0), 0xffff
   vmulhd.uu HX(N_frac, 0), HX(N_frac, 0), HX(N_ascale, 0)
   veor HX(N_ifrac, 0), HX(N_frac, 0), 0xffff

; average
   vmulhd.uu -, HX(N_srcblue, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcblue, 0), HX(N_blue, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcgreen, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcgreen, 0), HX(N_green, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcred, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcred, 0), HX(N_red, 0), HX(N_frac, 0) UACC

; combine again
   vmulhn.uu HX(N_srcred, 0), HX(N_srcred, 0), 1<<5
   vmulhn.uu HX(N_srcgreen, 0), HX(N_srcgreen, 0), 1<<6
   vmulhn.uu HX(N_srcblue, 0), HX(N_srcblue, 0), 1<<5 CLRA ACC
   vshl -, HX(N_srcgreen, 0), 5 ACC
   vshl HX(N_src, 0), HX(N_srcred, 0), 11 ACC

; get components
   vshl HX(N_srcblue, 0), HX(N_src1,0), 11
   vand HX(N_srcgreen, 0), HX(N_src1, 0), 0x7e0
   vshl HX(N_srcgreen, 0), HX(N_srcgreen,0), 5
   vand HX(N_srcred, 0), HX(N_src1, 0), 0xf800

; calc scaled fractions
   veor HX(N_frac, 0),  HX(N_cy, 0), 0x0
   vmulhd.uu HX(N_frac, 0), HX(N_frac, 0), HX(N_ascale, 0)
   veor HX(N_ifrac, 0), HX(N_frac, 0), 0xffff

; average
   vmulhd.uu -, HX(N_srcblue, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcblue, 0), HX(N_blue, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcgreen, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcgreen, 0), HX(N_green, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcred, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcred, 0), HX(N_red, 0), HX(N_frac, 0) UACC

; combine again
   vmulhn.uu HX(N_srcred, 0), HX(N_srcred, 0), 1<<5
   vmulhn.uu HX(N_srcgreen, 0), HX(N_srcgreen, 0), 1<<6
   vmulhn.uu HX(N_srcblue, 0), HX(N_srcblue, 0), 1<<5 CLRA ACC
   vshl -, HX(N_srcgreen, 0), 5 ACC
   vshl HX(N_src1, 0), HX(N_srcred, 0), 11 ACC

; get pixel offset
   vmul      -, HX(N_cx, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cx, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cy, 32)            UACC
; writeback pixel
   vindexwritem HX(N_src,0), (Rsrc) UACC IFNN
   vindexwritem HX(N_src1,0), (Rtemp) UACC IFNN
.if __VCCOREVER__ <= 0x04000100
; HW-1297 workaround - stall until the vindexwritem completes
   ldh   Rthick, (Rtemp)
   mov   Rthick, Rthick
.endif

; increment x & y index
   vmov  -, HX(N_cx, 0) CLRA UACC
   vmov  HX(N_cx, 0), HX(N_incxx_0to15, 0) UACC
   vadd  HX(N_cx, 32), HX(N_cx, 32), HX(N_incxx_0to15, 32) ACC

   vmov  -, HX(N_cy, 0) CLRA UACC
   vmov  HX(N_cy, 0), HX(N_incxy_0to15, 0) UACC
   vadd  HX(N_cy, 32), HX(N_cy, 32), HX(N_incxy_0to15, 32) ACC

   addcmpbgt Rloops, -1, 0, .lineloop_aaY

   b lr
.endfn ; }}}

;extern void vc_plotpoint_alpha(VC_IMAGE_T *buffer, int x, int y, FLOAT_T a, int col);
.function vc_plotpoint_alpha ; {{{
   st r0, (pc+pcrel(pointimage))
   ld r0, (pc+pcrel(pointnum))
   add r5, pc, pcrel(pointbuffer)
   addscale r5, r0 << 5
   sth r1, (r5++)
   sth r2, (r5++)
   sth r3, (r5++)
   sth r4, (r5++)
   addcmpble r0, 1, 15, 1f
   st r0, (pc+pcrel(pointnum))
   ld r0, (pc+pcrel(pointimage))
   b vc_plotpoint_alpha_flush
1:
   st r0, (pc+pcrel(pointnum))
   b lr
.endfn ; }}}

.function vc_plotpoint_alpha_flush ; {{{
   add Rtemp, pc, pcrel(pointbuffer)
   mov r1, 32
   vld VX(0, 32++), (Rtemp+=r1) REP 16
   
   vc_image_width Rtemp, r0
   vmov HX(N_width, 0), Rtemp-1
   vc_image_height Rtemp, r0
   vmov HX(N_height, 0), Rtemp-1
   vc_image_pitch Rtemp, r0
   asr Rtemp, 1 ; lookup is in shorts
   vmov HX(N_pitch, 0), Rtemp
   vc_image_image_data Rsrc, r0

; check for clipping with source limits
   vrsub -, HX(N_cx, 32), HX(N_width, 0)  SETF
   vsub  -, HX(N_cx, 32), 0               SETF IFNN
   vrsub -, HX(N_cy, 32), HX(N_height, 0) SETF IFNN
   vsub  -, HX(N_cy, 32), 0               SETF IFNN

; only enable valid points
   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)
   ld Rtemp, (pc+pcrel(pointnum))
   vrsub -, H(N0to15, 0), Rtemp-1 SETF IFNN

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC

; get components
   vshl HX(N_blue, 0),  HX(N_src,  32), 11
   vand HX(N_green, 0), HX(N_src,  32), 0x7e0
   vshl HX(N_green, 0), HX(N_green, 0), 5
   vand HX(N_red, 0),   HX(N_src,  32), 0xf800

; lookup pixel
   vlookupm  HX(N_src,0), (Rsrc) UACC IFNN

; get components
   vshl HX(N_srcblue, 0), HX(N_src, 0), 11
   vand HX(N_srcgreen, 0), HX(N_src, 0), 0x7e0
   vshl HX(N_srcgreen, 0), HX(N_srcgreen,0), 5
   vand HX(N_srcred, 0), HX(N_src, 0), 0xf800

; calc scaled fractions
   vmov HX(N_frac, 0), HX(N_ascale, 32)
   veor HX(N_ifrac, 0), HX(N_frac, 0), 0xffff

; average
   vmulhd.uu -, HX(N_srcblue, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcblue, 0), HX(N_blue, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcgreen, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcgreen, 0), HX(N_green, 0), HX(N_frac, 0) UACC
   vmulhd.uu -, HX(N_srcred, 0), HX(N_ifrac, 0) CLRA UACC
   vmulhd.uu HX(N_srcred, 0), HX(N_red, 0), HX(N_frac, 0) UACC

; combine again
   vmulhn.uu HX(N_srcred, 0), HX(N_srcred, 0), 1<<5
   vmulhn.uu HX(N_srcgreen, 0), HX(N_srcgreen, 0), 1<<6
   vmulhn.uu HX(N_srcblue, 0), HX(N_srcblue, 0), 1<<5 CLRA ACC
   vshl -, HX(N_srcgreen, 0), 5 ACC
   vshl HX(N_src, 0), HX(N_srcred, 0), 11 ACC

; get pixel offset
   vmul      -, HX(N_cy, 32), HX(N_pitch, 0) CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), HX(N_pitch, 0) ACC
   vmov      -, HX(N_cx, 32)            UACC

; writeback pixel
   vindexwritem HX(N_src,0), (Rsrc) UACC IFNN
   
   mov r0, 0
   st r0, (pc+pcrel(pointnum))
   b lr
.endfn ; }}}

.datablock pointnum, 4
.datablock pointimage, 4
.datablock pointbuffer, 16*32, 32

.undef Rsrc, Rloops, Rpitch, Rtemp, Rthick, Rthickness
.undef N_temp, N0to15

.define N_starty, 16
.define N_ints, 17
.define N_temp, 18
.define N_temp2, 19
.define N_x1, 20
.define N_y1, 21
.define N_x2, 22
.define N_y2, 23
.define N_polyoffsets, 24
.define N_grad, 25
.define N0to15, 31 ; H(31, 0)

.define Rpoly, r0
.define Rints, r1
.define Rpoints, r2
.define Rnumpoints, r3
.define Rendpoints, r3
.define Rtemp, r4
.define Rtemp2, r5
.define Rnum, r5
.define Rpointsinc, r6
.define Rx1, r7
.define Ry1, r8
.define Rx2, r9
.define Ry2, r10
.define SHIFT, 6  ; points range ~ 0-1023 or 10 bits, can handle an extra 6

; extern void vc_poly_intersects(short polyInts16[16][16], short ints[16], VC_IMAGE_POINT_T *p, int n, int starty)
.function vc_poly_intersects ; {{{
   stm r6-r10, lr, (--sp)
   .cfa_push {lr,r6,r7,r8,r9,r10}
   vmov HX(N_starty, 0), r4
   mov Rpointsinc, 8
   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rtemp)
   vadd HX(N_starty, 0), HX(N_starty, 0), H(N0to15, 0)
   vshl HX(N_starty, 0), HX(N_starty, 0), SHIFT
   vmul HX(N_polyoffsets, 0), H(N0to15, 0), 16
   addscale Rendpoints, Rpoints, Rnumpoints << 3 ; numpoints * (x and y) * sizeof(int)

   vmov HX(N_ints, 0), 0
   ld Rx2, (Rendpoints-8)
   ld Ry2, (Rendpoints-4)
   asr Rx2, 16-SHIFT
   asr Ry2, 16-SHIFT
1:
   mov Rx1, Rx2
   mov Ry1, Ry2
.jump_in:
   ld Rx2, (Rpoints+0)
   ld Ry2, (Rpoints+4)
   asr Rx2, 16-SHIFT
   asr Ry2, 16-SHIFT

; order the two point in smaller y in (x1,y1), larger y in (x2,y2)
   addcmpblt Ry1, 0, Ry2, 3f
   addcmpbeq Ry1, 0, Ry2, .skipin
   mov Rtemp, Rx2
   vmov HX(N_x1, 0), Rtemp
   mov Rtemp, Ry2
   vmov HX(N_y1, 0), Rtemp
;   mov Rtemp, Rx1
;   vmov HX(N_x2, 0), Rtemp
   mov Rtemp, Ry1
   vmov HX(N_y2, 0), Rtemp
   b 4f
3:
   mov Rtemp, Rx1
   vmov HX(N_x1, 0), Rtemp
   mov Rtemp, Ry1
   vmov HX(N_y1, 0), Rtemp
;   mov Rtemp, Rx2
;   vmov HX(N_x2, 0), Rtemp
   mov Rtemp, Ry2
   vmov HX(N_y2, 0), Rtemp
4:
; polyInts16[y-starty][ints[y-starty]++] = x1 + fmul(fdiv(y - y1, y2 - y1), x2 - x1);

; calc 16.16 gradient
   sub Rtemp,  Rx2, Rx1
   sub Rtemp2, Ry2, Ry1
   shl Rtemp, 16
   div.ss Rtemp, Rtemp2

   VMOV_H32 N_grad, Rtemp

   vsub HX(N_temp,0), HX(N_starty, 0), HX(N_y1, 0)     ; y-y1
   vmul -, HX(N_temp,0), HX(N_grad,0)                    CLRA UACC
   vmulhd.uu -, HX(N_temp,0), HX(N_grad,0)               ACC
   vmul      HX(N_temp2,0), HX(N_temp,0), HX(N_grad,32)  ACC
   vadd HX(N_temp2, 0), HX(N_temp2, 0), HX(N_x1, 0)

; if (y >= y1 && y < y2) {
   vsub HX(N_temp, 0), HX(N_y1, 0), 1
   vsub  -, HX(N_starty, 0), HX(N_y2, 0)   SETF ; N if y-y2 < 0
   vrsub -, HX(N_starty, 0), HX(N_temp, 0) IFN SETF ; N if 1+y1-y < 0 => y1-y <= 0 => y >= y1

   vadd -, HX(N_polyoffsets, 0), HX(N_ints, 0) CLRA ACC IFN
   vindexwritem HX(N_temp2, 0), (Rpoly) IFN
   vadd HX(N_ints, 0), HX(N_ints, 0), 1 IFN

.skipin:
   addcmpblt Rpoints, Rpointsinc, Rendpoints, 1b
2:   

; now sort the points into increasing x for each y row
   mov Rpointsinc, 32
   mov Rpoints, Rpoly
   add Rendpoints, Rpoints, 16*16*2
   vstleft  HX(N_ints, 0), (Rints)
;   vstright HX(N_ints, 0), (Rints)
   mov Rints, 0
1:
   mov Rpoly, Rpoints
   vldleft  HX(0,0), (Rpoints)
;   vldright HX(0,0), (Rpoints)
   vsub -, H(N0to15, 0), Rints SETF ; Z for valid
   vmov -, HX(N_ints, 0) IFZ SUM Rnum
   addcmpbeq Rnum, 0, 0, .skipout

   vsub -, H(N0to15, 0), Rnum SETF ; N for valid
   vmov HX(0,0), 0x7fff IFNN
2:
   vmov -, HX(0, 0) IMIN Rtemp
   vsub -, H(N0to15, 0), Rtemp SETF ; Z for valid
   vmov -, HX(0, 0) IFZ SUM Rtemp
   vmov HX(0,0), 0x7fff IFZ
   sth Rtemp, (Rpoly++)
   addcmpbgt Rnum, -1, 0, 2b
.skipout:
   add Rints, 1
   addcmpblt Rpoints, Rpointsinc, Rendpoints, 1b
   
   ldm r6-r10, pc, (sp++)
   .cfa_pop {r10,r9,r8,r7,r6,pc}
.endfn ; }}}
