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
;;; FUNCTION
;;;   vc_image_tfsub2ut_32
;;;
;;; DESCRIPTION
;;;   Downsample 4 or 8 uTiles of 32-bit TFormat, to make one or two uTiles
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

.function vc_image_tfsub2ut_32  ; {{{
        vmov H32(0++,0), 0 REP 64
        ;; Load 4 uTiles
        mov r5, 64
        mov r4, 0
        v32ld   H32(32++,0), (r2+=r5) REP 2
        v32ld   H32(34++,0), (r3+=r5) REP 2
        addcmpblt r0, 0, 2, 1f
        v32ld   H32(48++,0), (r2+128+=r5) REP 2
        v32ld   H32(50++,0), (r3+128+=r5) REP 2
1:
        ;; Set up some stuff
        mov r2, 0x00020002
        vbitplanes -, 0xCCCC SETF ; Pattern of Z's
2:
        ;; Split into evens and odds... (segregate horizontal neighbours)
        vmov -, r2 CLRA UACC      ; Rounding offsets
        mov r5, 0x00FF00FF        ; Component mask
        v32even H32(0++,0),  H32(32++,0)+r4, H32(34++,0)+r4 REP 2
        v32odd  H32(2++,0),  H32(32++,0)+r4, H32(34++,0)+r4 REP 2

        ;; Split the data into two "components", bytes -2-0 and -3-1 respectively
        v32lsr H32(4++,0),   H32(0++,0),    8  REP 4
        v32and H32(0++,0),   H32(0++,0),    r5 REP 8
        
        ;; Add quads of pixels having the same final position
        v32add  H32(16++,0), H32(0++,0),  H32(2++,0) REP 2 ; cpt1 horizontal pairs
        v32add  H32(18++,0), H32(4++,0),  H32(6++,0) REP 2 ; cpt2 horizontal pairs
        v32add  H32(0,0),    H32(16,2),   H32(16,0)  UACC NO_WB      ; cpt1 vert A
        v32add  H32(0,0),    H32(17,14),  H32(17,0)  UACC NO_WB IFNZ ; cpt1 vert B
        v32add  H32(1,0),    H32(18,2),   H32(18,0)  UACC NO_WB      ; cpt2 vert A
        v32add  H32(1,0),    H32(19,14),  H32(19,0)  UACC NO_WB IFNZ ; cpt2 vert B

        ;; Rescale and re-integrate the two components.
        v32lsr  H32(2,0),    H32(0,0),    2
        v32shl  H32(3,0),    H32(1,0),    6
        v32and  -,           H32(2,0),    r5 CLRA UACC
        v32bic  H32(4,0),    H32(3,0),    r5 SACC

        ;; Store the resulting uTile
.ifdef __BCM2707B0__
        v32mov   -, V32(0,0)
.endif
        v32st   H32(4,0),    (r1)
.ifdef __BCM2707B0__
        v32add  H32(4,0), H32(4,0), 0
.endif

        ;; Another uTile?
        add r1, 64
        addcmpblt r0, 0, 2, 9f
        mov r4, 16*64
        mov r0, 0
        b 2b
9:
        b lr
.endfn  ; }}}
