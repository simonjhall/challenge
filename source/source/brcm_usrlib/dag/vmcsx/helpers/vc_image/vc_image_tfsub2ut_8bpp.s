;;; ===========================================================================
;;; Copyright (c) 2008-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"
        
;;; T-Format Factor-of-2 decimation routines. In each case we read 8 uTiles
;;; and output 2 horizontally adjacent uTiles (unless only one is required).
;;; We assume that sources and destinations are suitable aligned, so that
;;; all the uTiles loaded/stored are horizontally contiguous in memory.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; FUNCTIONS
;;;   vc_image_tfsub2ut_[y|pal]8
;;;
;;; DESCRIPTION
;;;   Downsample 4 or 8 uTiles of 8-bit TFormat, to make one or two uTiles
;;;
;;; ARGUMENTS
;;;   r0 number of uTiles to generate, should be 1 or 2
;;;   r1 destination pointer
;;;   r2 source (lower row)
;;;   r3 source (upper row)
;;;
;;; RETURNS
;;;  void
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.section .text$vc_image_tfsub2ut_8, "text"

.globl vc_image_tfsub2ut_y8
.globl vc_image_tfsub2ut_pal8
.globl vc_image_tfsub2ut_8

vc_image_tfsub2ut_pal8:
        mov r4, 0
        b vc_image_tfsub2ut_8
vc_image_tfsub2ut_y8:
        mov r4, 1
vc_image_tfsub2ut_8: ; {{{
        min r0, 2
        mov r5, 32
        shl r0, 2  ; 1uT => 4, 2uT => 8
        vbitplanes -, 0xF0F0 SETF ; Pattern of Z's
        
        ;; Load 4 or 8 uTiles, using two H16 loads for each uTile
        ;; (this segregates the horizontally even and odd pixels).
        v16ld  H16(0++,0),  (r2+=r5) REP r0
        v16ld  H16(0++,32), (r3+=r5) REP r0

        addcmpbne r4, 0, 0, 3f
        mov r2, 0x00FF00FF
        v32and H32(0++,0), H32(0++,0), r2 REP r0
3:
        ;; Add the horizontal pairs which got segregated.
        ;; Meanwhile, shuffle the uTiles around.
        v16add H16(16++,0), H8(0++,0),  H8(0++,16) REP 2 ; lower row uTile 0
        v16add H16(18++,0), H8(0++,32), H8(0++,48) REP 2 ; upper row uTile 0
        v16add H16(20++,0), H8(4++,0),  H8(4++,16) REP 2 ; lower row uTile 2
        v16add H16(22++,0), H8(4++,32), H8(4++,48) REP 2 ; upper row uTile 2
        v16add H16(24++,0), H8(2++,0),  H8(2++,16) REP 2 ; lower row uTile 1
        v16add H16(26++,0), H8(2++,32), H8(2++,48) REP 2 ; upper row uTile 1
        v16add H16(28++,0), H8(6++,0),  H8(6++,16) REP 2 ; lower row uTile 3
        mov r5, 16
        v16add H16(30++,0), H8(6++,32), H8(6++,48) REP 2 ; upper row uTile 3

        addcmpbeq r4, 0, 0, 2f
        ;; Add vertical neighbours (using shifted-by-4 access). Re-interleave
        ;; to fill in the gaps. Downscale. We are now in H8 pixel order.
        v16add      H16(0++,0), H16(16++,4),  H16(16++,0) REP r0
        v16add      H16(0++,0), H16(24++,44), H16(24++,0) REP r0 IFNZ
        v16mulhn.uu H16(16++,0), H16(0++,0),  16384 REP r0
1:
        ;; Store one or two output uTiles
        v8st        H8(16++,0), (r1+=r5) REP r0
.ifdef __BCM2707B0__
        v16add      H8(16++,0), H8(16++,0), 0 REP r0 ; sync vst (B0 core bug)
.endif
        b lr

2:
        ;; Unsmoothed version. Still have to do the re-interleave.
        v16add H16(16++,0), H16(24++,44), 0 REP r0 IFNZ
        b 1b
; }}}

.text
