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
;;;   vc_image_tfsub2ut_4444
;;;
;;; DESCRIPTION
;;;   Downsample 4 or 8 uTiles of RGBA4444, to make one or two uTiles
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

.function vc_image_tfsub2ut_16load ; {{{
        ;; Load 4 or 8 uTiles, using one H32 access for each uTile
        ;; (this segretgates the horizontally even and odd pixels).
        ;; We load them in a slightly weird order. Considering their
        ;; *final* post-decimation positions, as {Row,Column} coords:
        ;;    00 01 02 03 00 01 02 03 10 11 12 13 10 11 12 13
        ;;    20 21 22 23 20 21 22 23 30 31 32 33 30 31 32 33
        ;;    08 09 0A 0B 08 09 0A 0B 18 19 1A 1B 18 19 1A 1B } if req'd
        ;;    28 29 2A 2B 28 29 2A 2B 38 39 3A 3B 38 39 3A 3B }
        ;;    04 05 06 07 04 05 06 07 14 15 16 17 14 15 16 17
        ;;    24 25 26 27 24 25 26 27 34 35 36 37 34 35 36 37
        ;;    0C 0D 0E 0F 0C 0D 0E 0F 1C 1D 1E 1F 1C 1D 1E 1F } if req'd
        ;;    2C 2D 2E 2F 2C 2D 2E 2F 3C 3D 3E 3F 3C 3D 3E 3F }
        v32ld  H32(32,0), (r2)
        v32ld  H32(36,0), (r2+64)
        v32ld  H32(33,0), (r3)
        v32ld  H32(37,0), (r3+64)
        addcmpble r0, 0, 1, 1f
        v32ld  H32(34,0), (r2+128)
        v32ld  H32(38,0), (r2+192)
        v32ld  H32(35,0), (r3+128)
        v32ld  H32(39,0), (r3+192)
1:        
        b r4  ; NB weird calling convention!
.endfn ; }}}

.function vc_image_tfsub2ut_4444 ;  {{{
        add r4, pc, pcrel(4f)
        b vc_image_tfsub2ut_16load ; NB weird calling convention!
4:
        ;; Split data into 2 "components": these are the upper and
        ;; lower nybbles of each byte to allow arithmetic headroom.
        ;; Each component occupies 8 rows of VRF (on both sides).
        ;; Meanwhile set r0 to be twice the number of uTiles required.
        mov r4, 0x0F0F0F0F
        mov r5, 32
        v16mov -, 0x0202 CLRA UACC
        max r0, 0
        vbitplanes -, 0xF0F0 SETF
        min r0, 2
        v32lsr H32(8++,0), H32(32++,0), 4 REP 8
        shl r0, 1
        v32and H32(0++,0), H32(32++,0), r4 REP 8    ; lower nybbles
        v32and H32(8++,0), H32(8++,0),  r4 REP 8    ; upper nybbles >> 4
        
        ;; Sum quads of pixels, in both components. Remember that each of
        ;; our "components" is actually two colour components, one per byte.
        ;; Add 0x0202 for rounding offset using accumulators and no-writeback.
        v16add H16(0++,0), H16(0++,0),   H16(0++,32) REP 16
        v16add H16(0++,0), H16(0++,4),   H16(0++,0)  UACC NO_WB REP r0
        v16add H16(0++,0), H16(4++,44),  H16(4++,0)  UACC NO_WB REP r0 IFNZ
        v16add H16(8++,0), H16(8++,4),   H16(8++,0)  UACC NO_WB REP r0
        v16add H16(8++,0), H16(12++,44), H16(12++,0) UACC NO_WB REP r0 IFNZ

        ;; Shift desired results from bits [13:10],[5:2] to final places.
        ;; Mask and reassemble the 16-bit pixel values. Store output.
        v16lsr H16(0++,0),  H16(0++,0), 2 REP r0
        v16shl H16(8++,0),  H16(8++,0), 2 REP r0
        v16and H16(0++,0),  H16(0++,0), 0x0F0F REP r0
        v16and H16(8++,0),  H16(8++,0), 0xF0F0 REP r0
        v16or  H16(16++,0), H16(0++,0), H16(8++,0) REP r0
.ifdef __BCM2707B0__
        v16add -, V16(16,0), 0  ; force VRF round-trip (core bug)
.endif
        v16st  H16(16++,0), (r1+=r5) REP r0   ; one or two uTiles
.ifdef __BCM2707B0__
        v16add H16(16++,0), H16(16++,0), 0 REP r0 ; sync vst
.endif
        b lr
.endfn ; }}}


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; FUNCTION
;;;   vc_image_tfsub2ut_565
;;;
;;; DESCRIPTION
;;;   Downsample 4 or 8 uTiles of RGB565, to make one or two uTiles
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

.function vc_image_tfsub2ut_565 ;  {{{
        add r4, pc, pcrel(5f)
        b vc_image_tfsub2ut_16load ; NB weird calling convention!
5:
        ;; Split data into 3 components: Red, Green and Blue.
        ;; Each component occupies 8 rows of VRF (on both sides).
        ;; Meanwhile set r0 to be twice the number of uTiles required.
        mov r4, 0x001F001F
        v16sub -, 0, 1 SETF        ; Force C=1 for rounding
        mov r5, 0x003F003F
        vbitplanes -, 0xF0F0 SETF  ; Pattern of Z's [C unchanged]
        max r0, 0
        v32lsr H32(0++,0),  H32(32++,0), 11 REP 8
        min r0, 2
        v32lsr H32(8++,0),  H32(32++,0),  5 REP 8
        shl r0, 1
        v32and H32(0++,0),  H32(0++,0),  r4 REP 8 ; R
        mov r2, 2*8*64
        v32and H32(8++,0),  H32(8++,0),  r5 REP 8 ; G
        mov r3, -8*64
        v32and H32(16++,0), H32(32++,0), r4 REP 8 ; B
        mov r5, 32

        ;; Decimate all 3 components using a loop. Each component
        ;; ends up occupying just the first 2 or 4 rows of its 8 rows.
7:
        v16addc H16(0++,0)+r2, H16(0++,0)+r2,  H16(0++,32)+r2 REP 8
        v16add  H16(0++,0)+r2, H16(0++,4)+r2,  H16(0++,0)+r2  REP r0
        v16add  H16(0++,0)+r2, H16(4++,44)+r2, H16(4++,0)+r2  REP r0 IFNZ
        v16lsr  H16(0++,0)+r2, H16(0++,0)+r2,  2 REP r0
        addcmpbge r2, r3, 0, 7b

        ;; Reassemble the 16-bit pixel values. Store output.
        v16shl H16(0++,0), H16(0++,0), 11 REP r0
        v16shl -,         H16(8,0),    5 CLRA UACC
        v16add H16(16,0), H16(16,0),   H16(0,0) UACC
        v16shl -,         H16(9,0),    5 CLRA UACC
        v16add H16(17,0), H16(17,0),   H16(1,0) UACC
        v16shl -,         H16(10,0),   5 CLRA UACC
        v16add H16(18,0), H16(18,0),   H16(2,0) UACC
        v16shl -,         H16(11,0),   5 CLRA UACC
        v16add H16(19,0), H16(19,0),   H16(3,0) UACC
.ifdef __BCM2707B0__
        v16add -, V16(16,0), 0  ; force VRF round-trip (core bug)
.endif
        v16st  H16(16++,0), (r1+=r5) REP r0   ; one or two uTiles
.ifdef __BCM2707B0__
        v16add H16(16++,0), H16(16++,0), 0 REP r0 ; sync vst
.endif
        b lr
.endfn ; }}}


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; FUNCTION
;;;   vc_image_tfsub2ut_5551
;;;
;;; DESCRIPTION
;;;   Downsample 4 or 8 uTiles of RGBA5551, to make one or two uTiles
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

.function vc_image_tfsub2ut_5551 ;  {{{
        add r4, pc, pcrel(6f)
        b vc_image_tfsub2ut_16load ; NB weird calling convention!
6:
        ;; Split data into 4 components: Red, Green, Blue and Alpha
        ;; (on both sides of the VRF). Remember, each component is 8 rows.
        ;; Meanwhile set r0 to be twice the number of uTiles required.
        mov r4, 0x001F001F
        v32lsr H32(0++,0),  H32(32++,0), 11 REP 8
        max r0, 0
        v32lsr H32(8++,0),  H32(32++,0),  6 REP 8
        min r0, 2
        v32lsr H32(16++,0), H32(32++,0),  1 REP 8
        shl r0, 1
        v32and H32(0++,0),  H32(0++,0),  r4 REP 8 ; R
        mov r2, 7*64
        v32and H32(8++,0),  H32(8++,0),  r4 REP 8 ; G
        mov r3, -64
        v32and H32(16++,0), H32(16++,0), r4 REP 8 ; B
        mov r5, 32

        ;; Extract Alpha and perform the "weird dilation stage".
        ;; Pixels with zero alpha "inherit" the colour of their neighbour.
8:
        v16and H16(24,0)+r2,  H16(32,0)+r2, 1 SETF ; Alpha (left side)
        v16mov H16(0,0)+r2,   H16(0,32)+r2    IFZ
        v16mov H16(8,0)+r2,   H16(8,32)+r2    IFZ
        v16mov H16(16,0)+r2,  H16(16,32)+r2   IFZ
        v16and H16(24,32)+r2, H16(32,32)+r2, 1 SETF ; Alpha (right side)
        v16mov H16(0,32)+r2,  H16(0,0)+r2     IFZ
        v16mov H16(8,32)+r2,  H16(8,0)+r2     IFZ
        v16mov H16(16,32)+r2, H16(16,0)+r2    IFZ
        addcmpbge r2, r3, 0, 8b

        ;; Decimate 4 components using a loop. Each component
        ;; ends up using just the first 2 or 4 rows of its 8 rows.
        ;; Slightly different rounding this time (for obscure reasons).
        v16sub -, 0, 1 SETF        ; Force C=1
        mov r2, 3*8*64
        vbitplanes -, 0xF0F0 SETF  ; Pattern of Z's [C unchanged]
        shl r3, 3
9:
        v16add  H16(0++,0)+r2, H16(0++,0)+r2,  H16(0++,32)+r2 REP 8
        v16addc H16(0++,0)+r2, H16(0++,4)+r2,  H16(0++,0)+r2  REP r0
        v16addc H16(0++,0)+r2, H16(4++,44)+r2, H16(4++,0)+r2  REP r0 IFNZ
        v16lsr  H16(0++,0)+r2, H16(0++,0)+r2,  2 REP r0
        addcmpbge r2, r3, 0, 9b

        ;; Reassemble the 16-bit pixel values. IMPORTANT: because we rounded
        ;; by +1/4, only quads with 3 or more opaque pixels will get A=1.
        ;; The dilation stage ensures that colour will be properly normalized,
        ;; but we must force the output to be all zeroes when A==0.
        v16shl  -,         H16(0,0),   11         CLRA UACC
        v16shl  -,         H16(8,0),    6         UACC
        v16addc H16(16,0), H16(16,0),   H16(16,0) UACC       ; (R<<11)+(G<<6)+(B<<1)+1
        v16shl  -,         H16(1,0),   11         CLRA UACC
        v16shl  -,         H16(9,0),    6         UACC
        v16addc H16(17,0), H16(17,0),   H16(17,0) UACC
        v16shl  -,         H16(2,0),   11         CLRA UACC
        v16shl  -,         H16(10,0),   6         UACC
        v16addc H16(18,0), H16(18,0),   H16(18,0) UACC
        v16shl  -,         H16(3,0),   11         CLRA UACC
        v16shl  -,         H16(11,0),   6         UACC
        v16addc H16(19,0), H16(19,0),   H16(19,0) UACC
        v16mul  H16(16++,0), H16(16++,0), H16(24++,0) REP r0 ; A=0 -> zero
.ifdef __BCM2707B0__
        v16add  -, V16(16,0), 0 ; force VRF round-trip (core bug)
.endif
        v16st   H16(16++,0), (r1+=r5) REP r0
.ifdef __BCM2707B0__
        v16add H16(16++,0), H16(16++,0), 0 REP r0 ; sync vst
.endif
        b lr
.endfn ; }}}
