;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.section .text$vc_image_const_0_15,"text"
   .align 16
.globl vc_image_const_0_15
vc_image_const_0_15:
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

.section .text$vc_image_recip_0_16,"text"
   .align 4
.globl vc_image_recip_0_16
vc_image_recip_0_16:
   .word 0           ; Reciprocals of 1..16 (0 is fudged!), weighted by 65536
   .word 65536
   .word 32768
   .word 21845
   .word 16384
   .word 13107
   .word 10923
   .word 9362
   .word 8192
   .word 7282
   .word 6554
   .word 5958
   .word 5461
   .word 5041
   .word 4681
   .word 4369
   .word 4096

;;; NAME
;;;    vc_image_copy_stripe
;;;
;;; SYNOPSIS
;;;    void vc_image_copy_stripe(unsigned char *dest, int dest_pitch_bytes,
;;;                              unsigned char *src, int src_pitch_bytes, int nblocks)
;;;
;;; FUNCTION
;;;    Copy a 16-deep stripe.
;;;
;;; RETURNS
;;;    -

.section .text$vc_image_copy_stripe,"text"
.globl vc_image_copy_stripe
vc_image_copy_stripe:
      mov r5, 32
      mul r4, r5
      add r4, r2
vc_image_copy_stripe_loop:
      vld HX(0++,0), (r2+=r3) REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      add r0, r5
      addcmpbne r2, r5, r4, vc_image_copy_stripe_loop
      b lr
.stabs "vc_image_copy_stripe:F1",36,0,0,vc_image_copy_stripe

;;; NAME
;;;    vc_image_load_yuv_downsampled
;;;
;;; SYNOPSIS
;;;    void vc_image_load_yuv_downsampled(unsigned char *Y_ptr, unsigned char *U_ptr, unsigned char *V_ptr,
;;;                                       int src->pitch)
;;;
;;; FUNCTION
;;;    Load a 32x32 pixel block and downsample to 16x16, leaving Y in H(0++,0), U in
;;;    H(16++,0), V in H(24++,l0).
;;;
;;; RETURNS
;;;    -

.section .text$vc_image_load_yuv_downsampled,"text"
.globl vc_image_load_yuv_downsampled
vc_image_load_yuv_downsampled:
      ;; Load everything up. This might load more than is actually
      ;; in the image, but that is no great problem.
      vld H(0++,0), (r0+=r3) REP 32
      vld H(0++,16), (r0+16+=r3) REP 32
      asr r3, 1
      vldleft H(32++,0), (r1+=r3) REP 16
;      vldright H(32++,0), (r1+=r3) REP 16
      vldleft H(48++,0), (r2+=r3) REP 16
;      vldright H(48++,0), (r2+=r3) REP 16
      ;; done with r0, r1, r2, r3
      mov r0, 64
      mov r1, 128
      mov r2, 1024
      mov r3, 0
      mov r4, 0
yuv_downsampled_vertical:
      vadd HX(0,32)+r3, H(0,16)+r4, H(1,16)+r4
      vadd HX(16,32)+r3, H(0,0)+r4, H(1,0)+r4
      vadd HX(32,0)+r3, H(32,0)+r4, H(33,0)+r4
      add r4, r1
      addcmpblt r3, r0, r2, yuv_downsampled_vertical
      mov r3, 0
      mov r4, 0
yuv_downsampled_horizontal:
      vadd VX(48,32), VX(16,32)+r3, VX(16,33)+r3
      vasr V(0,0)+r4, VX(48,32), 2
      vadd VX(48,32), VX(0,32)+r3, VX(0,33)+r3
      vasr V(0,8)+r4, VX(48,32), 2
      vadd VX(48,32), VX(32,0)+r3, VX(32,1)+r3
      vasr V(16,0)+r4, VX(48,32), 2
      add r3, 2
      addcmpblt r4, 1, 8, yuv_downsampled_horizontal           
      b lr
.stabs "vc_rgb565_from_yuv_downsampled:F1",36,0,0,vc_rgb565_from_yuv_downsampled

;;; NAME
;;;    vc_image_unpack_row
;;;
;;; SYNOPSIS
;;;    void vc_image_unpack_row(unsigned char *dest, unsigned char *src, int bytes);
;;;
;;; FUNCTION
;;;    Unpack a single row. Actually a reverse memcpy of an unaligned src to an aligned dest.
;;;
;;; RETURNS
;;;    -

.section .text$vc_image_unpack_row,"text"
.globl vc_image_unpack_row
vc_image_unpack_row:
   sub r2, 1
   mov r3, -16
   lsr r2, 4
   shl r2, 4
   add r4, r0, r2
   add r1, r2
vc_image_unpack_row_loop:     
   vldleft H(0,0), (r1)
;   vldright H(0,0), (r1)
   vst H(0,0), (r4)
   add r1, r3
   addcmpbge r4, r3, r0, vc_image_unpack_row_loop
   b lr
.stabs "vc_image_unpack_row:F1",36,0,0,vc_image_unpack_row


.FORGET
