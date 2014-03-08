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
;;;    vc_8bpp_vflip_in_place
;;;
;;; SYNOPSIS
;;;    void vc_8bpp_vflip_in_place (unsigned short *start, unsigned short *end, int pitch)
;;;
;;; FUNCTION
;;;    Flip image vertically.
;;;
;;; RETURNS
;;;    -

.section .text$vc_8bpp_vflip_in_place,"text"
.globl vc_8bpp_vflip_in_place
vc_8bpp_vflip_in_place:
      stm r6-r7, lr, (--sp)

      mov r5, 16           ; size of a block

vc_8bpp_vflip_in_place_vloop:
      mov r4, r2           ; r4 = pitch   
      add r4, r0           ; r4 = target for r0 = start of row + pitch
      mov r6, r0           ; store current r0 as r6
      mov r7, r1           ; store current r1 as r7

vc_8bpp_vflip_in_place_hloop:
      vld H(0,0), (r1)     ; copy 16 pixels at r1 to the VRF
      vld H(1,0), (r0)     ; copy 16 pixels at r0 to the VRF
      vst H(0,0), (r0)     ; copy 16 pixels from the VRF to r0
      vst H(1,0), (r1)     ; copy 16 pixels from the VRF to r1
      add r1, r5           ; increment r1 by 16 
      ; increment r0 by 16, compare against r4     
      addcmpblt r0, r5, r4, vc_8bpp_vflip_in_place_hloop

      add r0, r6, r2       ; r0 = r6 + pitch;
      sub r1, r7, r2       ; r1 = r7 - pitch
      addcmpbgt r1, 0, r0, vc_8bpp_vflip_in_place_vloop

      ldm r6-r7, pc, (sp++)
.stabs "vc_8bpp_vflip_in_place:F1",36,0,0,vc_8bpp_vflip_in_place

