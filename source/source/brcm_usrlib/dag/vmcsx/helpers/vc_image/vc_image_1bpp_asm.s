;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.section .text$vc_image_endian_translate_1bpp,"text"
.globl vc_image_endian_translate_1bpp
vc_image_endian_translate_1bpp:
      mov r4, r2
      asr r4, 5            ; no. of 32-byte wide blocks
vc_image_endian_translate_1bpp_loop:
      vld HX(0++,0), (r1+=r2) REP 16
      vbitplanes HX(0++,0), HX(0++,0) REP 16
      vmov VX(16,0), VX(0,7)
      vmov VX(16,1), VX(0,6)
      vmov VX(16,2), VX(0,5)
      vmov VX(16,3), VX(0,4)
      vmov VX(16,4), VX(0,3)
      vmov VX(16,5), VX(0,2)
      vmov VX(16,6), VX(0,1)
      vmov VX(16,7), VX(0,0)
      vmov VX(16,8), VX(0,15)
      vmov VX(16,9), VX(0,14)
      vmov VX(16,10), VX(0,13)
      vmov VX(16,11), VX(0,12)
      vmov VX(16,12), VX(0,11)
      vmov VX(16,13), VX(0,10)
      vmov VX(16,14), VX(0,9)
      vmov VX(16,15), VX(0,8)
      vbitplanes HX(16++,0), HX(16++,0) REP 16
      vst HX(16++,0), (r0+=r3) REP 16
      add r0, 32
      add r1, 32
      addcmpbne r4, -1, 0, vc_image_endian_translate_1bpp_loop
      b lr
.stabs "vc_image_endian_translate_1bpp:F1",36,0,0,vc_image_endian_translate_1bpp

;;; NAME
;;;    vc_1bpp_vflip
;;;
;;; SYNOPSIS
;;;    void vc_1bpp_vflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes)
;;;
;;; FUNCTION
;;;    Flip image vertically.
;;;
;;; RETURNS
;;;    -

.section .text$vc_1bpp_vflip,"text"
.globl vc_1bpp_vflip
vc_1bpp_vflip:
      stm r6-r9, lr, (--sp)
      mov r9, r5
      mov r6, 32
      ;; It is easiest just to copy over 1 row at a time, and not desparately inefficient.
vc_1bpp_vflip_vloop:
      mov r5, r4
      shl r5, 5
      add r5, r0           ; target address for r0
      mov r7, r0
      mov r8, r1
vc_1bpp_vflip_hloop:
      vld HX(0,0), (r1)
      vst HX(0,0), (r0)
      add r1, r6           
      addcmpbne r0, r6, r5, vc_1bpp_vflip_hloop
      add r0, r7, r9
      sub r1, r8, r2
      addcmpbne r3, -1, 0, vc_1bpp_vflip_vloop     
      ldm r6-r9, pc, (sp++)
.stabs "vc_1bpp_vflip:F1",36,0,0,vc_1bpp_vflip

;;; NAME
;;;    vc_1bpp_hflip
;;;
;;; SYNOPSIS
;;;    void vc_1bpp_hflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int ncols, int offset, int dest_pitch_bytes)
;;;
;;; FUNCTION
;;;    Horizontal flip ("mirror").
;;;
;;; RETURNS
;;;    -

.section .text$vc_1bpp_hflip,"text"
.globl vc_1bpp_hflip
vc_1bpp_hflip:
      stm r6-r6, lr, (--sp)
      cbclr
      mov r6, 0

      addcmpbeq r4, 0, 0, vc_1bpp_hflip_loop
      vldleft HX(0++,32), (r1+=r2) REP 16
      vbitplanes HX(0++,32), HX(0++,32) REP 16
vc_1bpp_hflip_loop:
      sub r1, 2
      vldleft HX(0++,0)*, (r1+=r2) REP 16
      vbitplanes HX(0++,0)*, HX(0++,0)* REP 16
      ;; VX addressing bug in core means we have to copy them out first before reversing them
      vmov VX(16,0++), VX(0,0++)+r4* REP 16
      vmov VX(32,0), VX(16,15)
      vmov VX(32,1), VX(16,14)
      vmov VX(32,2), VX(16,13)
      vmov VX(32,3), VX(16,12)
      vmov VX(32,4), VX(16,11)
      vmov VX(32,5), VX(16,10)
      vmov VX(32,6), VX(16,9)
      vmov VX(32,7), VX(16,8)
      vmov VX(32,8), VX(16,7)
      vmov VX(32,9), VX(16,6)
      vmov VX(32,10), VX(16,5)
      vmov VX(32,11), VX(16,4)
      vmov VX(32,12), VX(16,3)
      vmov VX(32,13), VX(16,2)
      vmov VX(32,14), VX(16,1)
      vmov VX(32,15), VX(16,0)
      ;; now repack them, and the left most column contributes the next output column
      vbitplanes HX(32++,0), HX(32++,0) REP 16
      vmov VX(32,32)+r6, VX(32,0)
      cbadd2
      addcmpbeq r3, -1, 0, vc_1bpp_hflip_done
      addcmpbne r6, 1, 16, vc_1bpp_hflip_loop
      vst HX(32++,32), (r0+=r5) REP 16
      add r0, 32
      mov r6, 0
      b vc_1bpp_hflip_loop
vc_1bpp_hflip_done:
      vst HX(32++,32), (r0+=r5) REP 16
      ldm r6-r6, pc, (sp++)

.stabs "vc_1bpp_hflip:F1",36,0,0,vc_1bpp_hflip

;;; NAME
;;;    vc_1bpp_transpose
;;;
;;; SYNOPSIS
;;;    void vc_1bpp_transpose(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int block_height)
;;;
;;; FUNCTION
;;;    Transpose image. Process a 16-*bit* wide column from src, turning it into 16-deep stripe
;;;    in the destination.
;;;
;;; RETURNS
;;;    -

.section .text$vc_1bpp_transpose,"text"
.globl vc_1bpp_transpose
vc_1bpp_transpose:
      mov r5,0
vc_1bpp_transpose_loop:
      ;; Load 16 rows of src.
      vldleft HX(0++,0), (r1+=r3) REP 16
      addscale r1, r3 << 4
      ;; Bitplane and transpose
      vbitplanes VX(0,32++), HX(0++,0) REP 16
      vbitplanes HX(0++,0), HX(0++,32) REP 16
      ;; now grab the column we want and place into 16x16 output block
      vmov VX(16,0)+r5, VX(0,0)
      ;; If either we have processed the entire column, or we have done 16 16-deep
      ;; blocks, then we must output a block.
      addcmpbeq r5, 1, r4, vc_1bpp_transpose_done
      addcmpbne r5, 0, 16, vc_1bpp_transpose_loop
      vst HX(16++,0), (r0+=r2) REP 16
      add r0, 32
      sub r4, 16
      b vc_1bpp_transpose
vc_1bpp_transpose_done:
      vst HX(16++,0), (r0+=r2) REP 16
      b lr     
.stabs "vc_1bpp_transpose:F1",36,0,0,vc_1bpp_transpose
