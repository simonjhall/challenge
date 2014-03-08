;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

;;; NAME
;;;    block_4bpp_to_rgb565
;;;
;;; SYNOPSIS
;;;    void block_4bpp_to_rgb565(unsigned char *dest, unsigned char *src, unsigned short *palette,
;;;                              int dest_pitch, int src_pitch)
;;;
;;; FUNCTION
;;;    Convert 16x16 block from 4bpp to rgb565 through the given palette.
;;;
;;; RETURNS
;;;    -

.section .text$block_4bpp_to_rgb565,"text"
.globl block_4bpp_to_rgb565
block_4bpp_to_rgb565:
   ;; r1 is 16-pixel aligned, so we get what we need from one load
   vldleft H(0++,0), (r1+=r4) REP 16
    ;; split out the separate 4bpp halves from each byte
   vand H(0++,16), H(0++,0), 15 REP 16
   vlsr H(0++,0), H(0++,0), 4 REP 16
   ;; need to go through memory to re-interleave the halves, but that's ok, we can use dest
   vst HX(0++,0), (r0+=r3) REP 16
   vld H(32++,0), (r0+=r3) REP 16; seems to be a core bug if we try and use row 0
   ;; ok, Now do a lookup on each to convert to rgb565
   mov r1, -64             ; don't need src any more
   mov r4, 960
block_4bpp_to_rgb565_loop:
   vmov -, H(32,0)+r4 CLRA ACC
   vlookupm HX(32,0)+r4, (r2)
   addcmpbge r4, r1, 0, block_4bpp_to_rgb565_loop
   ;; finally write back out
   vst HX(32++,0), (r0+=r3) REP 16
   b lr

;;; NAME
;;;    block_4bpp_to_rgb888
;;;
;;; SYNOPSIS
;;;    void block_4bpp_to_rgb888(unsigned char *dest, unsigned char *src, unsigned short *palette,
;;;                              int dest_pitch, int src_pitch)
;;;
;;; FUNCTION
;;;    Convert 16x16 block from 4bpp to rgb888 through the given palette.
;;;
;;; RETURNS
;;;    -

.section .text$block_4bpp_to_rgb888,"text"
.globl block_4bpp_to_rgb888
block_4bpp_to_rgb888:
   ;; r1 is 16-pixel aligned, so we get what we need from one load
   vldleft H(0++,0), (r1+=r4) REP 16
    ;; split out the separate 4bpp halves from each byte
   vand H(0++,16), H(0++,0), 15 REP 16
   vlsr H(0++,0), H(0++,0), 4 REP 16
   ;; can't re-interleave these through memory in the same way as above because
   ;; dest and the dest_pitch may only be 16-bit aligned, but we can find another address
   mov r5, r0              ; need to find a 32-bit aligned address
   bmask r5, 5
   add r5, r0, r5          ; this is 32-bit aligned now
   mov r1, -64             ; don't need src any more
   mov r4, 960
block_4bpp_to_rgb888_loop:
   vst HX(0,0)+r4, (r5)
   vld H(32,0)+r4, (r5)       ; can't have ACC with this (seems to be a core bug if we try and use row 0)
   vmov -, H(32,0)+r4 CLRA ACC
   vlookupm HX(32,0)+r4, (r2)
   addcmpbge r4, r1, 0, block_4bpp_to_rgb888_loop
   ;; now convert to 24bpp
   mov r1, 0
   mov r4, 0
block_4bpp_to_rgb888_conv_loop:  
   vshl V(16,2)+r4, VX(32,0)+r1, 3     ; B
   vlsr V(16,1)+r4, VX(32,0)+r1, 5  
   vshl V(16,1)+r4, V(16,1)+r4, 2      ; G
   vand V(16,0)+r4, V(32,16)+r1, 0xf8  ; R
   add r4, 3
   addcmpblt r1, 1, 16, block_4bpp_to_rgb888_conv_loop
   ;; finally write back out
   vst H(16++,0), (r0+=r3) REP 16
   vst H(16++,16), (r0+16+=r3) REP 16
   vst H(16++,32), (r0+32+=r3) REP 16
   b lr

