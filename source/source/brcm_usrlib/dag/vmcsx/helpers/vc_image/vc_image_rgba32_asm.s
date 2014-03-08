;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.FORGET

;;; *****************************************************************************

.include "vcinclude/common.inc"

.macro function, name
.define funcname, name
.pushsect .text
.section .text$\&funcname,"text"
.globl funcname
funcname:
.cfa_bf funcname
.endm

.macro endfunc
.cfa_ef
.size funcname,.-funcname
.popsect
.undef funcname
.endm

.macro asserteq, reg, val=0
.ifndef NDEBUG
         cmp reg, val
         beq 9f
         bkpt
.endif
9:
.endm

.macro assertne, reg, val=0
.ifndef NDEBUG
         cmp reg, val
         bne 9f
         bkpt
.endif
9:
.endm


;;; *****************************************************************************
;;; NAME
;;;   rgba32_from_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba32_from_rgb888(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src888,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha);     r4
;;;
;;; FUNCTION
;;;   Convert an rgb888 16x16 cell to rgba32

function rgba32_from_rgb888 ; {{{
      vld      H(0++,0), (r2+=r3)            REP 16
      vld      H(0++,16), (r2+16+=r3)        REP 16
      vld      H(0++,32), (r2+32+=r3)        REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmov     V(48,0)+r2, V(0,0)+r3
      vmov     V(48,1)+r2, V(0,2)+r3
      vmov     V(48,16)+r2, V(0,1)+r3
      vmov     V(48,17)+r2, r4

      vmov     V(48,32)+r2, V(0,24+0)+r3
      vmov     V(48,33)+r2, V(0,24+2)+r3
      vmov     V(48,48)+r2, V(0,24+1)+r3
      vmov     V(48,49)+r2, r4

      add      r3, 3
      addcmpbne r2, 2, 16, .loop

      vst      HX(48++,0), (r0+=r1)          REP 16
      vst      HX(48++,32), (r0+32+=r1)      REP 16
      b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   rgba32_from_bgr888
;;;
;;; SYNOPSIS
;;;   void rgba32_from_rgb888(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src888,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha);     r4
;;;
;;; FUNCTION
;;;   Convert an bgr888 16x16 cell to rgba32 (same as above but with swapping 
;;;   of r & b components on the fly

function rgba32_from_bgr888 ; {{{
      vld      H(0++,0), (r2+=r3)            REP 16
      vld      H(0++,16), (r2+16+=r3)        REP 16
      vld      H(0++,32), (r2+32+=r3)        REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmov     V(48,0)+r2, V(0,2)+r3         ; swap 
      vmov     V(48,1)+r2, V(0,0)+r3         ; swap
      vmov     V(48,16)+r2, V(0,1)+r3
      vmov     V(48,17)+r2, r4

      vmov     V(48,32)+r2, V(0,24+2)+r3     ; swap
      vmov     V(48,33)+r2, V(0,24+0)+r3     ; swap
      vmov     V(48,48)+r2, V(0,24+1)+r3
      vmov     V(48,49)+r2, r4

      add      r3, 3
      addcmpbne r2, 2, 16, .loop

      vst      HX(48++,0), (r0+=r1)          REP 16
      vst      HX(48++,32), (r0+32+=r1)      REP 16
      b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   rgba32_from_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba32_from_rgb565(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned short const*src565,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha);     r4
;;;
;;; FUNCTION
;;;   Convert an rgb565 16x16 cell to rgba32

function rgba32_from_rgb565 ; {{{
      vld      HX(0++,0), (r2+=r3)           REP 16

      vand     HX(0++,32), HX(0++,0), 0x07E0 REP 16
      vand     HX(0++,0), HX(0++,0), 0xF81F  REP 16

      mov      r2, 0
.loop:
      vmulm    V(48,0)+r2, V(0,16)+r2, 0x108 ; red
      vmulhd.su V(48,16)+r2, VX(0,32)+r2, 0x2080 ; green
      vmulm    V(48,32)+r2, V(0,0)+r2, 0x840 ; blue
      vmov     V(48,48)+r2, r4               ; alpha
      addcmpbne r2, 1, 16, .loop

      vst      H32(48++,0), (r0+=r1)         REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_from_rgb565_transparent
;;;
;;; SYNOPSIS
;;;   void rgba32_from_transparent_rgb565(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned short const*src565,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      int         transparent_colour); r5
;;;
;;; FUNCTION
;;;   Convert an rgb565 16x16 cell to rgba32

function rgba32_from_transparent_rgb565 ; {{{
      cmp      r5, -1
      beq      rgba32_from_rgb565

      vld      HX(16++,0), (r2+=r3)                REP 16

      vand     HX(0++,32), HX(16++,0), 0x07E0      REP 16
      vand     HX(0++,0), HX(16++,0), 0xF81F       REP 16

      mov      r2, 0
.loop:
      veor     -, VX(16,0)+r2, r5                  SETF
      vmulm    V(48,0)+r2, V(0,16)+r2, 0x108 ; red
      vmulhd.su V(48,16)+r2, VX(0,32)+r2, 0x2080 ; green
      vmulm    V(48,32)+r2, V(0,0)+r2, 0x840 ; blue
      vmov     V(48,48)+r2, 0                      IFZ  ; alpha
      vmov     V(48,48)+r2, r4                     IFNZ ; alpha
      addcmpbne r2, 1, 16, .loop

      vst      H32(48++,0), (r0+=r1)               REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_from_transparent_4bpp
;;;
;;; SYNOPSIS
;;;   void rgba32_from_transparent_4bpp(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned short const*src565,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      int         transparent_colour,  r5
;;;      unsigned short const*cmap);      (sp)
;;;
;;; FUNCTION
;;;   Convert a 4bpp 16x16 cell to rgba32

function rgba32_from_transparent_4bpp ; {{{
      vldleft  H(0++,0), (r2+=r3)                  REP 16
      stm      r6, (--sp)
      .cfa_push {%r6}
      ld       r6, (sp+4)

      vlsr     V(0,8++), V(0,0++), 4               REP 8
      vand     V(0,0++), V(0,0++), 0x0F            REP 8

      mov      r2, 0
      mov      r3, 0
.loop:
      veor     -, V(0,0)+r2, r5                    SETF
      vmov     V(48,48)+r3, r4
      vmov     -, V(0,0)+r2                        CLRA ACC
      vlookupm VX(16,0), (r6)
      vand     VX(16,32), VX(16,0), 0x07E0
      vand     VX(16,0),  VX(16,0), 0xF81C
      vmulm    V(48,0)+r3,  V(16,16),   0x108
      vmulhd.su V(48,16)+r3, VX(16,32), 0x2080
      vmulm    V(48,32)+r3,   V(16,0),    0x840
      vmov     V(48,48)+r3, 0                      IFZ

      veor     -, V(0,8)+r2, r5                    SETF
      vmov     V(48,49)+r3, r4
      vmov     -, V(0,8)+r2                        CLRA ACC
      vlookupm VX(16,0), (r6)
      vand     VX(16,32), VX(16,0), 0x07E0
      vand     VX(16,0),  VX(16,0), 0xF81C
      vmulm    V(48,1)+r3,  V(16,16),   0x108
      vmulhd.su V(48,17)+r3, VX(16,32), 0x2080
      vmulm    V(48,33)+r3,   V(16,0),    0x840
      vmov     V(48,49)+r3, 0                      IFZ

      add      r3, 2
      addcmpbne r2, 1, 8, .loop

      VST_H32_REP  <(48++,0)>, r0, r1, 16

      ldm      r6, (sp++)
      .cfa_pop {%r6}
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgba32
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgba32(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      unsigned long        opmask,     r4
;;;      int                  ascale);    r5
;;;
;;; FUNCTION
;;;   Overlay one rgba32 16x16 cell onto another such that the new output can be
;;;   applied in a single action with the result of the two having been applied
;;;   one after the other.
;;;
;;;   expects destination image to be aligned.

function rgba32_overlay_to_rgba32 ; {{{
      vld      HX(0++,0), (r0+=r1)                 REP 16
      vld      HX(0++,32), (r0+32+=r1)             REP 16

      vldleft  HX(16++,0), (r2+=r3)                REP 16
;      vldright HX(16++,0), (r2+=r3)                REP 16
      vldleft  HX(16++,32), (r2+32+=r3)            REP 16
;      vldright HX(16++,32), (r2+32+=r3)            REP 16

      mov      r2, 0
.loop:
      vmulm    VX(32,0), V(0,17)+r2, 0x102      ;; VX(32,0) == DstAlpha
      vrsub    VX(32,1), VX(32,0), 256          ;; VX(32,1) == 1-DstAlpha
      vmulm    VX(32,2), V(16,17)+r2, r5        ;; VX(32,2) == SrcAlpha
      vrsub    VX(32,3), VX(32,2), 256          ;; VX(32,3) == 1-SrcAlpha
      vmulm    VX(32,4), VX(32,1), VX(32,3)     ;; VX(33,4) == 1-NewAlpha

      vsub     -, VX(32,2), VX(32,0)               SETF ; set C if dstalpha > srcalpha
      vrsubs   V(0,17)+r2, VX(32,4), 256

      vmov     VX(32,32), V(0,0)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,0)+r2               IFNC CLRA ACC
      vsub     VX(32,32), V(0,0)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,0)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,0)+r2, VX(32,32), 0

      vmov     VX(32,32), V(0,1)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,1)+r2               IFNC CLRA ACC
      vsub     VX(32,32), V(0,1)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,1)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,1)+r2, VX(32,32), 0

      vmov     VX(32,32), V(0,16)+r2               IFC CLRA ACC
      vmov     VX(32,32), V(16,16)+r2              IFNC CLRA ACC
      vsub     VX(32,32), V(0,16)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,16)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,16)+r2, VX(32,32), 0

      addcmpbne r2, 0, 14, .donthop
      add      r2, 16
.donthop:
      addcmpbne r2, 2, 48, .loop

      vbitplanes -, r4                             SETF
      ror      r4, 16
      vst      HX(0++,0), (r0+=r1)                 IFNZ REP 16
      vbitplanes -, r4                             SETF
      vst      HX(0++,32), (r0+32+=r1)             IFNZ REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgba32_n
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgba32_n(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  lines,      r4
;;;      unsigned long        opmask,     r5
;;;      int                  ascale);    (sp)
;;;
;;; FUNCTION
;;;   Overlay one rgba32 16x16 cell onto another such that the new output can be
;;;   applied in a single action with the result of the two having been applied
;;;   one after the other.
;;;
;;;   expects destination image to be aligned.

function rgba32_overlay_to_rgba32_n ; {{{
      stm      r6-r8, (--sp)
      .cfa_push {%r6,%r7,%r8}
      mov      r7, 64
      shl      r8, r4, 6

      mov      r6, 0
      mov      r4, r0
.loadloop:
      vld      HX(0,0)+r6, (r4)
      vld      HX(0,32)+r6, (r4+32)

      vldleft  HX(16,0)+r6, (r2)
;      vldright HX(16,0)+r6, (r2)
      vldleft  HX(16,32)+r6, (r2+32)
;      vldright HX(16,32)+r6, (r2+32)
      add      r4, r1
      add      r2, r3
      addcmpbne r6, r7, r8, .loadloop

      ld       r6, (sp+12)

      mov      r2, 0
.loop:
      vmulm    VX(32,0), V(0,17)+r2, 0x102      ;; VX(32,0) == DstAlpha
      vrsub    VX(32,1), VX(32,0), 256          ;; VX(32,1) == 1-DstAlpha
      vmulm    VX(32,2), V(16,17)+r2, r6        ;; VX(32,2) == SrcAlpha
      vrsub    VX(32,3), VX(32,2), 256          ;; VX(32,3) == 1-SrcAlpha
      vmulm    VX(32,4), VX(32,1), VX(32,3)     ;; VX(33,4) == 1-NewAlpha

      vsub     -, VX(32,2), VX(32,0)               SETF ; set C if dstalpha > srcalpha
      vrsubs   V(0,17)+r2, VX(32,4), 256

      vmov     VX(32,32), V(0,0)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,0)+r2               IFNC CLRA ACC
      vsub     VX(32,32), V(0,0)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,0)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,0)+r2, VX(32,32), 0

      vmov     VX(32,32), V(0,1)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,1)+r2               IFNC CLRA ACC
      vsub     VX(32,32), V(0,1)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,1)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,1)+r2, VX(32,32), 0

      vmov     VX(32,32), V(0,16)+r2               IFC CLRA ACC
      vmov     VX(32,32), V(16,16)+r2              IFNC CLRA ACC
      vsub     VX(32,32), V(0,16)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,16)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,16)+r2, VX(32,32), 0

      addcmpbne r2, 0, 14, .donthop
      add      r2, 16
.donthop:
      addcmpbne r2, 2, 48, .loop

      lsr      r4, r5, 16
      mov      r6, 0
.storeloop:
      vbitplanes -, r5                             SETF
      vst      HX(0,0)+r6, (r0)                    IFNZ
      vbitplanes -, r4                             SETF
      vst      HX(0,32)+r6, (r0+32)                IFNZ
      add      r0, r1
      addcmpbne r6, r7, r8, .storeloop

      ldm      r6-r8, (sp++)
      .cfa_pop {%r8,%r7,%r6}
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb888_blocky
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb888_blocky(
;;;      unsigned char       *dest888,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha);     r4
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB888 image.
;;;
;;;   expects everything to be aligned

function rgba32_overlay_to_rgb888_blocky ; {{{
      vld      H(0++,0), (r0+=r1)            REP 16
      vld      H(0++,16), (r0+16+=r1)        REP 16
      vld      H(0++,32), (r0+32+=r1)        REP 16

      vld      HX(16++,0), (r2+=r3)          REP 16
      vld      HX(16++,32), (r2+32+=r3)      REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),    V(16,0)+r2, V( 0,0)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3, VX(32,1),   V( 0,0)+r3

      vsub     VX(32,1),    V(16,16)+r2, V( 0,1)+r3
      vmulm    VX(32,1),   VX(32,1),    VX(32,0)
      vadds    V(48,1)+r3, VX(32,1),    V( 0,1)+r3

      vsub     VX(32,1),    V(16,1)+r2, V( 0,2)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,2)+r3, VX(32,1),    V( 0,2)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,0+24)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,0+24)+r3, VX(32,1),       V( 0,0+24)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,1+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,1+24)+r3, VX(32,1),        V( 0,1+24)+r3

      vsub     VX(32,1),       V(16,1+32)+r2,  V( 0,2+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,2+24)+r3, VX(32,1),        V( 0,2+24)+r3

      add      r2, 2
      addcmpbne r3, 3, 24, .loop

      vst      H(48++,0), (r0+=r1)           REP 16
      vst      H(48++,16), (r0+16+=r1)       REP 16
      vst      H(48++,32), (r0+32+=r1)       REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb888(
;;;      unsigned char       *dest888,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      unsigned long long   opmask);    r5+(sp)
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB888 image.
;;;
;;;   expects source image to be aligned.

function rgba32_overlay_to_rgb888 ; {{{
      vldleft  H(0++,0), (r0+=r1)            REP 16
      vldleft  H(0++,16), (r0+16+=r1)        REP 16
      vldleft  H(0++,32), (r0+32+=r1)        REP 16
;      vldright H(0++,0), (r0+=r1)            REP 16
;      vldright H(0++,16), (r0+16+=r1)        REP 16
;      vldright H(0++,32), (r0+32+=r1)        REP 16

      vld      HX(16++,0), (r2+=r3)          REP 16
      vld      HX(16++,32), (r2+32+=r3)      REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),    V(16,0)+r2, V( 0,0)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3, VX(32,1),   V( 0,0)+r3

      vsub     VX(32,1),    V(16,16)+r2, V( 0,1)+r3
      vmulm    VX(32,1),   VX(32,1),    VX(32,0)
      vadds    V(48,1)+r3, VX(32,1),    V( 0,1)+r3

      vsub     VX(32,1),    V(16,1)+r2, V( 0,2)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,2)+r3, VX(32,1),    V( 0,2)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,0+24)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,0+24)+r3, VX(32,1),       V( 0,0+24)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,1+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,1+24)+r3, VX(32,1),        V( 0,1+24)+r3

      vsub     VX(32,1),       V(16,1+32)+r2,  V( 0,2+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,2+24)+r3, VX(32,1),        V( 0,2+24)+r3

      add      r2, 2
      addcmpbne r3, 3, 24, .loop

      vbitplanes -, r5                       SETF
      ror      r5, 16
      vstleft  H(48++,0), (r0+=r1)           IFNZ REP 16
;      vstright H(48++,0), (r0+=r1)           IFNZ REP 16
      vbitplanes -, r5                       SETF
      ldh      r5, (sp)
      vstleft  H(48++,16), (r0+16+=r1)       IFNZ REP 16
;      vstright H(48++,16), (r0+16+=r1)       IFNZ REP 16
      vbitplanes -, r5                       SETF
      vstleft  H(48++,32), (r0+32+=r1)       IFNZ REP 16
;      vstright H(48++,32), (r0+32+=r1)       IFNZ REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb888_n
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb888_n(
;;;      unsigned char       *dest888,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      int                  lines,      r5
;;;      unsigned long long   opmask);    (sp)
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB888 image.

function rgba32_overlay_to_rgb888_n ; {{{
      stm      r6-r8, (--sp)
      .cfa_push {%r6,%r7,%r8}

      shl      r8, r5, 6
      mov      r7, 64

      mov      r6, 0
      mov      r5, r0
.loadloop:
      vldleft  H(0,0)+r6, (r5)
;      vldright H(0,0)+r6, (r5)
      vldleft  H(0,16)+r6, (r5+16)
;      vldright H(0,16)+r6, (r5+16)
      vldleft  H(0,32)+r6, (r5+32)
;      vldright H(0,32)+r6, (r5+32)

      vld      HX(16,0)+r6, (r2)
      vld      HX(16,32)+r6, (r2+32)
      add      r5, r1
      add      r2, r3
      addcmpbne r6, r7, r8, .loadloop

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),    V(16,0)+r2, V( 0,0)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3, VX(32,1),   V( 0,0)+r3

      vsub     VX(32,1),    V(16,16)+r2, V( 0,1)+r3
      vmulm    VX(32,1),   VX(32,1),    VX(32,0)
      vadds    V(48,1)+r3, VX(32,1),    V( 0,1)+r3

      vsub     VX(32,1),    V(16,1)+r2, V( 0,2)+r3
      vmulm    VX(32,1),   VX(32,1),   VX(32,0)
      vadds    V(48,2)+r3, VX(32,1),    V( 0,2)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,0+24)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,0+24)+r3, VX(32,1),       V( 0,0+24)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,1+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,1+24)+r3, VX(32,1),        V( 0,1+24)+r3

      vsub     VX(32,1),       V(16,1+32)+r2,  V( 0,2+24)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,2+24)+r3, VX(32,1),        V( 0,2+24)+r3

      add      r2, 2
      addcmpbne r3, 3, 24, .loop

      ld       r3, (sp+12)
      ldh      r5, (sp+12+4)
      lsr      r4, r3, 16

      mov      r6, 0
.storeloop:
      vbitplanes -, r3                       SETF
      vstleft  H(48,0)+r6, (r0)              IFNZ
;      vstright H(48,0)+r6, (r0)              IFNZ
      vbitplanes -, r4                       SETF
      vstleft  H(48,16)+r6, (r0+16)          IFNZ
;      vstright H(48,16)+r6, (r0+16)          IFNZ
      vbitplanes -, r5                       SETF
      vstleft  H(48,32)+r6, (r0+32)          IFNZ
;      vstright H(48,32)+r6, (r0+32)          IFNZ
      add      r0, r1
      addcmpbne r6, r7, r8, .storeloop

      ldm      r6-r8, (sp++)
      .cfa_pop {%r8,%r7,%r6}
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb565_blocky
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb565_blocky(
;;;      unsigned char       *dest565,    r0
;;;      int                  dpitch,     r1
;;;      unsigned char       *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha);     r4
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB565 image.

function rgba32_overlay_to_rgb565_blocky ; {{{
      vld      HX(0++,0), (r0+=r1)           REP 16

      vld      HX(16++,0), (r2+=r3)          REP 16
      vld      HX(16++,32), (r2+32+=r3)      REP 16

      vand     HX(0++,32), HX(0++,0), 0x07E0 REP 16
      vand     HX(0++,0), HX(0++,0), 0xF81F  REP 16

      vmulm    V(0,0++),  V(0,0++), 0x840    REP 16 ; blue
      vmulm    V(0,16++), V(0,16++), 0x108   REP 16 ; red
      vmulhd.su V(0,32++), VX(0,32++), 0x2080 REP 16 ; green

      mov      r2, 0
      mov      r3, 0
.loop:

      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),     V(16,1)+r2, V( 0,0)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3,  VX(32,1),    V( 0,0)+r3

      vsub     VX(32,1),     V(16,16)+r2, V( 0,32)+r3
      vmulm    VX(32,1),    VX(32,1),    VX(32,0)
      vadds    V(48,32)+r3, VX(32,1),     V( 0,32)+r3

      vsub     VX(32,1),     V(16,0)+r2, V( 0,16)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,16)+r3, VX(32,1),    V( 0,16)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),      V(16,1+32)+r2, V( 0,0+8)+r3
      vmulm    VX(32,1),     VX(32,1),      VX(32,0)
      vadds    V(48,0+8)+r3, VX(32,1),       V( 0,0+8)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,32+8)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,32+8)+r3, VX(32,1),        V( 0,32+8)+r3

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,16+8)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,16+8)+r3, VX(32,1),       V( 0,16+8)+r3

      add      r2, 2
      addcmpbne r3, 1, 8, .loop

      vlsr     V(48,0++),   V(48,0++), 3     REP 16
      vlsr     V(48,32++),  V(48,32++), 2    REP 16
      vand     V(48,16++),  V(48,16++), 0xF8 REP 16
      vshl     VX(48,32++), V(48,32++), 5    REP 16
      vor      VX(48,0++), VX(48,0++), VX(48,32++) REP 16

      vst      HX(48++,0), (r0+=r1)          REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb565(
;;;      unsigned short      *dest565,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      unsigned short       opmask);    r5
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB565 image.
;;;
;;;   expects source to be aligned

function rgba32_overlay_to_rgb565 ; {{{
      vldleft  HX(0++,0), (r0+=r1)           REP 16
;      vldright HX(0++,0), (r0+=r1)           REP 16

      vld      HX(16++,0), (r2+=r3)          REP 16
      vld      HX(16++,32), (r2+32+=r3)      REP 16

      vand     HX(0++,32), HX(0++,0), 0x07E0 REP 16
      vand     HX(0++,0), HX(0++,0), 0xF81F  REP 16

      vmulm    H(0++,0),  H(0++,0), 0x840    REP 16
      vmulm    H(0++,16), H(0++,16), 0x108   REP 16
      vmulhd.su H(0++,32), HX(0++,32), 0x2080 REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),     V(16,1)+r2, V( 0,0)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3,  VX(32,1),    V( 0,0)+r3

      vsub     VX(32,1),     V(16,16)+r2, V( 0,32)+r3
      vmulm    VX(32,1),    VX(32,1),    VX(32,0)
      vadds    V(48,32)+r3, VX(32,1),     V( 0,32)+r3

      vsub     VX(32,1),     V(16,0)+r2, V( 0,16)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,16)+r3, VX(32,1),    V( 0,16)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),      V(16,1+32)+r2, V( 0,0+8)+r3
      vmulm    VX(32,1),     VX(32,1),      VX(32,0)
      vadds    V(48,0+8)+r3, VX(32,1),       V( 0,0+8)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,32+8)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,32+8)+r3, VX(32,1),        V( 0,32+8)+r3

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,16+8)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,16+8)+r3, VX(32,1),       V( 0,16+8)+r3

      add      r2, 2
      addcmpbne r3, 1, 8, .loop

      vlsr     H(48++,0),   H(48++,0), 3     REP 16
      vlsr     H(48++,32),  H(48++,32), 2    REP 16
      vand     H(48++,16),  H(48++,16), 0xF8 REP 16
      vshl     HX(48++,32), H(48++,32), 5    REP 16
      vor      HX(48++,0), HX(48++,0), HX(48++,32) REP 16

      vbitplanes -, r5                       SETF
      vstleft  HX(48++,0), (r0+=r1)          IFNZ REP 16
;      vstright HX(48++,0), (r0+=r1)          IFNZ REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgb565_n
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgb565_n(
;;;      unsigned short      *dest565,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch,     r3
;;;      int                  alpha,      r4
;;;      int                  lines,      r5
;;;      unsigned short       opmask);    (sp)
;;;
;;; FUNCTION
;;;   Overlay an RGBA32 image onto an RGB565 image.
;;;
;;;   expects source to be aligned

function rgba32_overlay_to_rgb565_n ; {{{
      stm      r6-r8, (--sp)
      .cfa_push {%r6,%r7,%r8}

      shl      r8, r5, 6
      mov      r7, 64

      mov      r6, 0
      mov      r5, r0
.loadloop:
      vldleft  HX(0,0)+r6, (r5)
;      vldright HX(0,0)+r6, (r5)

      vld      HX(16,0)+r6, (r2)
      vld      HX(16,32)+r6, (r2+32)
      add      r5, r1
      add      r2, r3
      addcmpbne r6, r7, r8, .loadloop

      ld       r5, (sp+12)

      vand     HX(0++,32), HX(0++,0), 0x07E0 REP 16
      vand     HX(0++,0), HX(0++,0), 0xF81F  REP 16

      vmulm    H(0++,0),  H(0++,0), 0x840    REP 16
      vmulm    H(0++,16), H(0++,16), 0x108   REP 16
      vmulhd.su H(0++,32), HX(0++,32), 0x2080 REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulm    VX(32,0), V(16,17)+r2, r4

      vsub     VX(32,1),     V(16,1)+r2, V( 0,0)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,0)+r3,  VX(32,1),    V( 0,0)+r3

      vsub     VX(32,1),     V(16,16)+r2, V( 0,32)+r3
      vmulm    VX(32,1),    VX(32,1),    VX(32,0)
      vadds    V(48,32)+r3, VX(32,1),     V( 0,32)+r3

      vsub     VX(32,1),     V(16,0)+r2, V( 0,16)+r3
      vmulm    VX(32,1),    VX(32,1),   VX(32,0)
      vadds    V(48,16)+r3, VX(32,1),    V( 0,16)+r3

      vmulm    VX(32,0), V(16,17+32)+r2, r4

      vsub     VX(32,1),      V(16,1+32)+r2, V( 0,0+8)+r3
      vmulm    VX(32,1),     VX(32,1),      VX(32,0)
      vadds    V(48,0+8)+r3, VX(32,1),       V( 0,0+8)+r3

      vsub     VX(32,1),       V(16,16+32)+r2, V( 0,32+8)+r3
      vmulm    VX(32,1),      VX(32,1),       VX(32,0)
      vadds    V(48,32+8)+r3, VX(32,1),        V( 0,32+8)+r3

      vsub     VX(32,1),       V(16,0+32)+r2, V( 0,16+8)+r3
      vmulm    VX(32,1),      VX(32,1),      VX(32,0)
      vadds    V(48,16+8)+r3, VX(32,1),       V( 0,16+8)+r3

      add      r2, 2
      addcmpbne r3, 1, 8, .loop

      vlsr     H(48++,0),   H(48++,0), 3     REP 16
      vlsr     H(48++,32),  H(48++,32), 2    REP 16
      vand     H(48++,16),  H(48++,16), 0xF8 REP 16
      vshl     HX(48++,32), H(48++,32), 5    REP 16
      vor      HX(48++,0), HX(48++,0), HX(48++,32) REP 16

      vbitplanes -, r5                       SETF
      mov      r6, 0
.storeloop:
      vstleft  HX(48,0)+r6, (r0)             IFNZ
;      vstright HX(48,0)+r6, (r0)             IFNZ
      add      r0, r1
      addcmpbne r6, r7, r8, .storeloop

      ldm      r6-r8, (sp++)
      .cfa_pop {%r8,%r7,%r6}
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgb565_overlay_to_rgba32_n
;;;
;;; SYNOPSIS
;;;   void rgb565_overlay_to_rgba32_n(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned short const*src565,     r2
;;;      int                  spitch,     r3
;;;      int                  lines,      r4
;;;      unsigned long        opmask,     r5
;;;      int                  transparent, (sp)
;;;      int                  alpha);     (sp+4)
;;;
;;; FUNCTION
;;;   Overlay one rgba32 16x16 cell onto another such that the new output can be
;;;   applied in a single action with the result of the two having been applied
;;;   one after the other.
;;;
;;;   expects destination image to be aligned.

function rgb565_overlay_to_rgba32_n ; {{{
      stm      r6-r8, (--sp)
      .cfa_push {%r6,%r7,%r8}
      mov      r7, 64
      shl      r8, r4, 6

      mov      r6, 0
      mov      r4, r0
.loadloop:
      vld      HX(0,0)+r6, (r4)
      vld      HX(0,32)+r6, (r4+32)

      vldleft  HX(16,0)+r6, (r2)
;      vldright HX(16,0)+r6, (r2)
      add      r4, r1
      add      r2, r3
      addcmpbne r6, r7, r8, .loadloop

      ld       r4, (sp+12)
      ld       r6, (sp+16)
      vbitplanes -, r5                             SETF
      vmov     HX(16++,0), r4                      REP 16 IFZ

      vmov     VX(32,2), r6
      vmulm    VX(32,2), V(32,2), 0x102         ;; VX(32,2) == SrcAlpha
      vrsub    VX(32,3), VX(32,2), 256          ;; VX(32,3) == 1-SrcAlpha

      mov      r3, 0
      mov      r2, 0
.loop:
      vand     VX(16,32), VX(16,0)+r3, 0xF81F
      vand     VX(16,33), VX(16,0)+r3, (~0xF81F & 0xFFFF)

      vmulm    V(16,32), V(16,32), 0x840        ;; blue
      vmulhd.su V(16,33), VX(16,33), 0x2080     ;; green
      vmulm    V(16,34), V(16,48), 0x108        ;; red

      vmulm    VX(32,0), V(0,17)+r2, 0x102      ;; VX(32,0) == DstAlpha
      vrsub    VX(32,1), VX(32,0), 256          ;; VX(32,1) == 1-DstAlpha
      vmulm    VX(32,4), VX(32,1), VX(32,3)     ;; VX(33,4) == 1-NewAlpha

      vsub     -, VX(32,2), VX(32,0)               SETF ; set C if dstalpha > srcalpha
      veor     -, VX(16,0)+r3, r4                  SETF ; set Z if transparent (or out-of-range)
      vrsubs   V(0,17)+r2, VX(32,4), 256           IFNZ

      vmov     VX(32,32), V(0,0)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,32)                 IFNC CLRA ACC
      vsub     VX(32,32), V(0,0)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,32), VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,0)+r2, VX(32,32), 0             IFNZ

      vmov     VX(32,32), V(0,1)+r2                IFC CLRA ACC
      vmov     VX(32,32), V(16,34)                 IFNC CLRA ACC
      vsub     VX(32,32), V(0,1)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,34), VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,1)+r2, VX(32,32), 0             IFNZ

      vmov     VX(32,32), V(0,16)+r2               IFC CLRA ACC
      vmov     VX(32,32), V(16,33)                 IFNC CLRA ACC
      vsub     VX(32,32), V(0,16)+r2, VX(32,32)
      vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
      vsub     VX(32,32), V(16,33), VX(32,32)
      vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
      vadds    V(0,16)+r2, VX(32,32), 0            IFNZ

      addcmpbne r2, 0, 14, .donthop
      add      r2, 16
.donthop:
      add      r3, 1
      addcmpbne r2, 2, 48, .loop

      mov      r6, 0
.storeloop:
      vst      HX(0,0)+r6, (r0)
      vst      HX(0,32)+r6, (r0+32)
      add      r0, r1
      addcmpbne r6, r7, r8, .storeloop

      ldm      r6-r8, (sp++)
      .cfa_pop {%r8,%r7,%r6}
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgb565_overlay_to_rgba32_alpha
;;;
;;; SYNOPSIS
;;;   void rgb565_overlay_to_rgba32_alpha(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned short const*src565,     r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines,      r5
;;;      int                  alpha);     (sp)
;;;
;;; FUNCTION
;;;   Overlay one rgba32 16x16 cell onto another such that the new output can be
;;;   applied in a single action with the result of the two having been applied
;;;   one after the other.

;function rgb565_overlay_to_rgba32_alpha ; {{{
;            stm      r6-r10, (--sp)
;            .cfa_push {%r6,%r7,%r8,%r9,%r10}
;
;            shl      r8, r4, 2
;            mov      r9, r5
;            and      r4, r0, 31
;            and      r5, r2, 63
;            sub      r0, r4
;            sub      r2, r5
;            lsr      r5, 1
;
;            ldb      r10, (sp+20)
;
;            ;  r4 = dst offset
;            ;  r5 = src offset
;            ;  r8 = columns (in bytes)
;            ;  r9 = lines
;            ;  r10 = alpha
;
;
;            cbclr
;            add      r5, 32 - 1
;
;
;            vmov     VX(32,2), r10
;            vmulm    VX(32,2), V(32,2), 0x102         ;; VX(32,2) == SrcAlpha
;            vrsub    VX(32,3), VX(32,2), 256          ;; VX(32,3) == 1-SrcAlpha
;
;            vld      HX(48++,0), (r0+=r1)          REP 16
;
;.loop:      addcmpblt r5, 1, 32, .noload
;
;            vld      HX(48++,0), (r2+=r3)          REP 16
;            vld      HX(48++,32), (r2+32+=r3)      REP 16
;            bmask    r5, 5
;            add      r2, 64
;.noload:    vand     VX(16,32), VX(16,0)+r5, 0xF81F
;            vand     VX(16,33), VX(16,0)+r5, ~0xF81F
;
;            vmulm    V(16,32), V(16,32), 0x840        ;; blue
;            vmulhd.su V(16,33), VX(16,33), 0x2080     ;; green
;            vmulm    V(16,34), V(16,48), 0x108        ;; red
;
;            vmulm    VX(32,0), V(0,17)+r4, 0x102      ;; VX(32,0) == DstAlpha
;            vrsub    VX(32,1), VX(32,0), 256          ;; VX(32,1) == 1-DstAlpha
;            vmulm    VX(32,4), VX(32,1), VX(32,3)     ;; VX(33,4) == 1-NewAlpha
;
;            vsub     -, VX(32,2), VX(32,0)               SETF ; set C if dstalpha > srcalpha
;            veor     -, VX(16,0)+r5, r4                  SETF ; set Z if transparent (or out-of-range)
;            vrsubs   V(0,17)+r4, VX(32,4), 256           IFNZ
;
;            vmov     VX(32,32), V(0,0)+r4                IFC CLRA ACC
;            vmov     VX(32,32), V(16,32)                 IFNC CLRA ACC
;            vsub     VX(32,32), V(0,0)+r4, VX(32,32)
;            vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
;            vsub     VX(32,32), V(16,32), VX(32,32)
;            vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
;            vadds    V(0,0)+r4, VX(32,32), 0             IFNZ
;
;            vmov     VX(32,32), V(0,1)+r4                IFC CLRA ACC
;            vmov     VX(32,32), V(16,34)                 IFNC CLRA ACC
;            vsub     VX(32,32), V(0,1)+r4, VX(32,32)
;            vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
;            vsub     VX(32,32), V(16,34), VX(32,32)
;            vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
;            vadds    V(0,1)+r4, VX(32,32), 0             IFNZ
;
;            vmov     VX(32,32), V(0,16)+r4               IFC CLRA ACC
;            vmov     VX(32,32), V(16,33)                 IFNC CLRA ACC
;            vsub     VX(32,32), V(0,16)+r4, VX(32,32)
;            vmulm    VX(32,32), VX(32,0), VX(32,32)      ACC
;            vsub     VX(32,32), V(16,33), VX(32,32)
;            vmulm    VX(32,32), VX(32,2), VX(32,32)      ACC
;            vadds    V(0,16)+r4, VX(32,32), 0            IFNZ
;            
;...
;
;
;
;
;
;            ldm      r6-r10, (sp++)
;            .cfa_pop {%r10,%r9,%r8,%r7,%r6}
;            b        lr
;endfunc ; }}}
;.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_alpha_adjust8
;;;
;;; SYNOPSIS
;;;   void rgba32_alpha_adjust8(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char       *mask,       r2
;;;      int                  mpitch,     r3
;;;      int                  scale,      r4
;;;      int                  boost);     r5
;;;
;;; FUNCTION
;;;   Adjust the alpha channel of an RGBA32 image

function rgba32_alpha_adjust8 ; {{{
      vld      HX(0++,0), (r0+=r1)           REP 16
      vld      HX(0++,32), (r0+32+=r1)       REP 16
      vld      H(16++,0), (r2+=r3)           REP 16

      vmulms   HX(16++,0), H(16++,0), r4     REP 16
      vadds    HX(16++,0), HX(16++,0), r5    REP 16

      mov      r2, 0
      mov      r3, 0
.loop:
      vmulms   V(0,17)+r3, V(0,17)+r3, VX(16,0)+r2
      vmulms   V(0,19)+r3, V(0,19)+r3, VX(16,1)+r2

      vmulms   V(0,49)+r3, V(0,49)+r3, VX(16,8)+r2
      vmulms   V(0,51)+r3, V(0,51)+r3, VX(16,9)+r2

      add      r3, 4
      addcmpbne r2, 2, 8, .loop

      vst      HX(0++,0), (r0+=r1)          REP 16
      vst      HX(0++,32), (r0+32+=r1)      REP 16
      b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_hflip_in_place
;;;
;;; SYNOPSIS
;;;   void rgba32_hflip_in_place(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      int                  length);    r2
;;;
;;; FUNCTION
;;;   Perform a horizontal flip on a line of RGBA32 data.
;;;
;;;   Always operates on 16 lines.

function rgba32_hflip_in_place ; {{{
            stm      r6, (--sp)
            .cfa_push {%r6}
            sub      r5, r2, 1
            mov      r4, 0
            addscale r2, r0, r5 << 2
            shl      r5, 1
            bic      r2, 31
            bmask    r5, 4
            mov      r6, 32

            ; r4 = dst offset
            ; r5 = src offset
            ; r6 = 32

            vld      HX(0++,0), (r2+=r1)                 REP 16
            vld      HX(32++,0), (r0+=r1)                REP 16
.loop:      vmov     VX(16,0)+r4, VX(0,0)+r5
            vmov     VX(16,1)+r4, VX(0,1)+r5
            vmov     VX(48,0)+r5, VX(32,0)+r4
            vmov     VX(48,1)+r5, VX(32,1)+r4
            addcmpbge r5, -2, 0, .loaded
            vst      HX(48++,0), (r2+=r1)                REP 16
            vld      HX(0++,0), (r2-32+=r1)              REP 16
            sub      r2, r6
            bmask    r5, 4
            addcmpblt r2, 0, r0, .exit
.loaded:    addcmpbne r4, 2, 16, .loop
            vst      HX(16++,0), (r0+=r1)                REP 16
            vld      HX(32++,0), (r0+32+=r1)             REP 16
            bmask    r4, 4
            addcmpble r0, r6, r2, .loop
.exit:      ldm      r6, (sp++)
            .cfa_pop {%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_transpose_chunky
;;;
;;; SYNOPSIS
;;;   void rgba32_transpose_chunky(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch);    r3
;;;
;;; FUNCTION
;;;   Transpose 4kB of RGBA32 data.

function rgba32_transpose_chunky ; {{{
            vld      HX(1++,0), (r2+=r3)           REP 64
            vld      HX(1++,32), (r2+32+=r3)       REP 64

            mov      r4, -64*64
            mov      r5, 64*4
            vfrom32l -, HX(0,0), HX(0,32)          CLRA UACC
            vfrom32h -, HX(0,0), HX(0,32)          ACC

.loop:      vfrom32l HX(0,0)+r4,  HX(1,0)+r4, HX(1,32)+r4
            vfrom32h HX(0,32)+r4, HX(1,0)+r4, HX(1,32)+r4
            vfrom32l HX(1,0)+r4,  HX(2,0)+r4, HX(2,32)+r4
            vfrom32h HX(1,32)+r4, HX(2,0)+r4, HX(2,32)+r4
            vfrom32l HX(2,0)+r4,  HX(3,0)+r4, HX(3,32)+r4
            vfrom32h HX(2,32)+r4, HX(3,0)+r4, HX(3,32)+r4
            vfrom32l HX(3,0)+r4,  HX(4,0)+r4, HX(4,32)+r4
            vfrom32h HX(3,32)+r4, HX(4,0)+r4, HX(4,32)+r4
            addcmpbne r4, r5, 0, .loop

            vmov     HX(63,0), 0                   UACC
            vmov     HX(63,32), 0                  ACC

            VST_V32_REP <(0, 0++)>, r0, r1, 16
            VST_V32_REP <(16,0++)>, r0, r1, 16, 64
            VST_V32_REP <(32,0++)>, r0, r1, 16, 128
            VST_V32_REP <(48,0++)>, r0, r1, 16, 192

            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_transpose_chunky0
;;;
;;; SYNOPSIS
;;;   void rgba32_transpose_chunky0(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch);    r3
;;;
;;; FUNCTION
;;;   Transpose 4kB of RGBA32 data dropping all the odd pixels.

function rgba32_transpose_chunky0 ; {{{
            vld      HX(1++,0), (r2+=r3)           REP 32
            vld      HX(1++,32), (r2+32+=r3)       REP 32
            vld      HX(33++,0), (r2+64+=r3)       REP 32
            vld      HX(33++,32), (r2+96+=r3)      REP 32

            mov      r4, -64*64
            mov      r5, 64*4
            vfrom32l -, HX(0,0), HX(0,32)          CLRA UACC
            vfrom32h -, HX(0,0), HX(0,32)          ACC

.loop:      vfrom32l HX(0,0)+r4,  HX(1,0)+r4, HX(1,32)+r4
            vfrom32h HX(0,32)+r4, HX(1,0)+r4, HX(1,32)+r4
            vfrom32l HX(1,0)+r4,  HX(2,0)+r4, HX(2,32)+r4
            vfrom32h HX(1,32)+r4, HX(2,0)+r4, HX(2,32)+r4
            vfrom32l HX(2,0)+r4,  HX(3,0)+r4, HX(3,32)+r4
            vfrom32h HX(2,32)+r4, HX(3,0)+r4, HX(3,32)+r4
            vfrom32l HX(3,0)+r4,  HX(4,0)+r4, HX(4,32)+r4
            vfrom32h HX(3,32)+r4, HX(4,0)+r4, HX(4,32)+r4
            addcmpbne r4, r5, 0, .loop

            vmov     HX(63,0), 0                   UACC
            vmov     HX(63,32), 0                  ACC

            vfrom32l HX(0++,0), HX(0++,0), HX(32++,0) REP 32
            vfrom32l HX(0++,32), HX(0++,32), HX(32++,32) REP 32

            VST_V32_REP <(0, 0++)>, r0, r1, 16
            VST_V32_REP <(16,0++)>, r0, r1, 16, 64

            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_transpose_chunky1
;;;
;;; SYNOPSIS
;;;   void rgba32_transpose_chunky1(
;;;      unsigned long       *desta32,    r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *srca32,     r2
;;;      int                  spitch);    r3
;;;
;;; FUNCTION
;;;   Transpose 4kB of RGBA32 data dropping all the even pixels.

function rgba32_transpose_chunky1 ; {{{
            vld      HX(1++,0), (r2+=r3)           REP 32
            vld      HX(1++,32), (r2+32+=r3)       REP 32
            vld      HX(33++,0), (r2+64+=r3)       REP 32
            vld      HX(33++,32), (r2+96+=r3)      REP 32

            mov      r4, -64*64
            mov      r5, 64*4
            vfrom32l -, HX(0,0), HX(0,32)          CLRA UACC
            vfrom32h -, HX(0,0), HX(0,32)          ACC

.loop:      vfrom32l HX(0,0)+r4,  HX(1,0)+r4, HX(1,32)+r4
            vfrom32h HX(0,32)+r4, HX(1,0)+r4, HX(1,32)+r4
            vfrom32l HX(1,0)+r4,  HX(2,0)+r4, HX(2,32)+r4
            vfrom32h HX(1,32)+r4, HX(2,0)+r4, HX(2,32)+r4
            vfrom32l HX(2,0)+r4,  HX(3,0)+r4, HX(3,32)+r4
            vfrom32h HX(2,32)+r4, HX(3,0)+r4, HX(3,32)+r4
            vfrom32l HX(3,0)+r4,  HX(4,0)+r4, HX(4,32)+r4
            vfrom32h HX(3,32)+r4, HX(4,0)+r4, HX(4,32)+r4
            addcmpbne r4, r5, 0, .loop

            vmov     HX(63,0), 0                   UACC
            vmov     HX(63,32), 0                  ACC

            vfrom32h HX(0++,0), HX(0++,0), HX(32++,0) REP 32
            vfrom32h HX(0++,32), HX(0++,32), HX(32++,32) REP 32

            VST_V32_REP <(0, 0++)>, r0, r1, 16
            VST_V32_REP  <(16,0++)>, r0, r1, 16, 64

            b        lr
endfunc ; }}}
.FORGET

.if _VC_VERSION >= 3
;;;    vc_rgb888_font_alpha_blt
;;;
;;; SYNOPSIS
;;;    void vc_rgba32_font_alpha_blt(unsigned long *dest  /* r0 */,
;;;                                  unsigned char *src   /* r1 */,
;;;                                  int dest_pitch_bytes /* r2 */,
;;;                                  int src_pitch_bytes  /* r3 */,
;;;                                  int width            /* r4 */,
;;;                                  int height           /* r5 */,
;;;                                  int red,int green,int blue,int alpha);
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgba32_font_alpha_blt,"text"
.globl vc_rgba32_font_alpha_blt
vc_rgba32_font_alpha_blt:
      stm r6-r9, lr, (--sp)
.ifdef RGB888
      ld r6, (sp+20)       ; red
      vmov HX(60,0), r6
      ld r6, (sp+24)       ; green
      vmov HX(61,0), r6
      ld r6, (sp+28)       ; blue
      vmov HX(62,0), r6
.else
      ld r6, (sp+28)       ; blue (hereinafter called "red", the lowest byte)
      vmov HX(60,0), r6
      ld r6, (sp+24)       ; green
      vmov HX(61,0), r6
      ld r6, (sp+20)       ; red (hereinafter called "blue")
      vmov HX(62,0), r6
.endif
      ld r6, (sp+32)       ; alpha value

1:
      mov r7, r0
      mov r8, r1
      addscale r9, r0, r4 << 2
0:
      vld H8(1,0), (r1)    ; 16 of "4.4" (alpha/grey) values
      vld H32(0,0), (r0)
      add r1, 16
      add r0, 64
      vlsr H16(1,0), H8(1,0), 4
      vmul H16(1,0), H16(1,0), r6 ; scale by global alpha to range 0..3825
      vmulhn.uu H16(1,0), H16(1,0), 4386
      vclip     H16(1,0), H16(1,0), 256 SETF
      vadds     H8(5,0),  H16(1,0), 0
      vsub      H16(2,0), H16(60,0), H8(0,0)  IFNZ
      vsub      H16(3,0), H16(61,0), H8(0,16) IFNZ
      vsub      H16(4,0), H16(62,0), H8(0,32) IFNZ
      vmulm     H16(2++,0), H16(2++,0),  H16(1,0) IFNZ REP 2
      vmulm     H16(4,0), H16(4,0),  H16(1,0) IFNZ
      vadds     H8(0,0),  H8(0,0),   H16(2,0) IFNZ
      vadds     H8(0,16), H8(0,16),  H16(3,0) IFNZ
      vadds     H8(0,32), H8(0,32),  H16(4,0) IFNZ
      vmax      H8(0,48), H8(0,48),  H8(5,0)
      vst       H32(0,0), (r0-64) IFNZ
      addcmpblo r0, 0, r9, 0b
      add r0, r7, r2
      add r1, r8, r3
      addcmpbgt r5, -1, 0, 1b
      ldm r6-r9, pc, (sp++)  
.endif


;;; *****************************************************************************
;;; NAME
;;;   rgba32_copy_alpha_from_8bpp
;;;
;;; SYNOPSIS
;;;   void rgba32_copy_alpha_from_8bpp(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char       *mask,       r2
;;;      int                  mpitch);    r3
;;;
;;; FUNCTION
;;;     Force values into the alpha channel using an 8-bit all alpha 8bpp source.

function rgba32_copy_alpha_from_8bpp ; {{{
         vld      HX(0++,0), (r0+=r1)              REP 16   ; *dest+=dpitch
         vld      HX(0++,32), (r0+32+=r1)          REP 16   
         vld      H(16++,0), (r2+=r3)              REP 16   ; *mask+=mpitch

         mov       r5, 0
         mov      r4, 0
         mov      r2, 0
.loop:
         vadd   V(0,17)+r4, V(16,0)+r2, r5
         vadd   V(0,49)+r4, V(16,8)+r2, r5
         add      r4, 2  
         addcmpblt r2, 1, 8, .loop

         vst      HX(0++,0), (r0+=r1)              REP 16   ; *dest+=dpitch
         vst      HX(0++,32), (r0+32+=r1)          REP 16
        
         b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_premultiply
;;;
;;; SYNOPSIS
;;;   void rgba32_premultiply(
;;;      unsigned long       *dest,       /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long       *src,        /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  alphabits,  /* r4 */
;;;      int                  count);     /* r5 */
;;;
;;; FUNCTION
;;;    Load a line of 16x16 blocks and convert them from normalised to
;;;    premultiplied alpha.

function rgba32_premultiply ; {{{
         shl      r4, 1

         vmov     -, 0x80                                      CLRA UACC

         vld      H32(0++,0)+r4, (r2+=r3)                      REP 16

         addcmpble r5, 0, 1, .final

.loop:   vld      H32(16++,0)+r4, (r2+64+=r3)                  REP 16 ; pre-load the next block
         add      r2, 64

.final:  
.ifdef __BCM2707B0__
         ; H32(48++,0)+r4 is where we did our last store, but on B0 we can't
         ; perform any operations that leave a port free because the store may
         ; get in and break that operation... so we block here.
         vmov     H32(48,0)+r4, 0
.endif

         vmul     H16(32++,0)+r4, H8(0++,16)+r4, H8(0++,0)+r4  REP 16 UACC NO_WB
         vmulhd.uu H8(0++,16)+r4, H16(32++,0)+r4, 0x101        REP 16

         vmul     H16(32++,0)+r4, H8(0++,32)+r4, H8(0++,0)+r4  REP 16 UACC NO_WB
         vmulhd.uu H8(0++,32)+r4, H16(32++,0)+r4, 0x101        REP 16

         vmul     H16(32++,0)+r4, H8(0++,48)+r4, H8(0++,0)+r4  REP 16 UACC NO_WB
         vmulhd.uu H8(0++,48)+r4, H16(32++,0)+r4, 0x101        REP 16

         vst      H32(0++,0)+r4, (r0+=r1)                      REP 16
         add      r0, 64

         add      r4, 1024
         bmask    r4, 12

         addcmpbgt r5, -1, 1, .loop
         addcmpbgt r5, 0, 0, .final

         vmov     H32(48,0)+r4, 0
         b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_composite
;;;
;;; SYNOPSIS
;;;   void rgba32_composite(
;;;      unsigned long       *dest,       /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long       *src,        /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  alphabits,  /* r4 */
;;;      int                  width,      /* r5 */
;;;      int                  height,     /* (sp) */
;;;      int                  alpha);     /* (sp+4) */
;;;
;;; FUNCTION
;;;    blah
;;;
;;; TODO
;;;   Allow height to be any value.
;;;   Forget about the read/write pipelining and work stripes of 16 lines

function rgba32_composite ; {{{
         stm      r6-r10, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9,%r10}

         shl      r4, 1
         mov      r6, r0
         ld       r0, (sp+20) ; height
         min      r8, r5, 16
         ld       r7, (sp+24) ; alpha
         mov      r9, -1
         vld      H32(0++,0)+r4, (r6+=r1)                         REP r0
         shl      r9, r8
         vld      H32(16++,0)+r4, (r2+=r3)                        REP r0

         mov      r8, -16
         vbitplanes -, r9                                         SETF
         vmov     -, 0x80                                         CLRA UACC

         addcmpble r5, 0, 16, .final

.loop:   vld      H32(32++,0)+r4, (r6+64+=r1)                     REP r0 ; pre-load the next block
         vld      H32(48++,0)+r4, (r2+64+=r3)                     REP r0 ; pre-load the next block
         add      r2, 64

.final:  vmul     H16(24++,0)+r4, H8(16++,0)+r4, r7               REP r0 UACC NO_WB
         vmulhd.uu H8(16++,0)+r4, H16(24++,0)+r4, 0x101           REP r0

         veor     H8(0++,0)+r4, H8(0++,0)+r4, 255                 REP r0
         veor     H8(16++,0)+r4, H8(16++,0)+r4, 255               REP r0

         vmul     H16(8++,0)+r4,  H8(0++,16)+r4, H8(16++,0)+r4    REP r0
         vmul     H16(24++,0)+r4, H8(16++,16)+r4, r7              REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,16)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4,  H8(0++,32)+r4, H8(16++,0)+r4    REP r0
         vmul     H16(24++,0)+r4, H8(16++,32)+r4, r7              REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,32)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4,  H8(0++,48)+r4, H8(16++,0)+r4    REP r0
         vmul     H16(24++,0)+r4, H8(16++,48)+r4, r7              REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,48)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4, H8(0++,0)+r4, H8(16++,0)+r4      REP r0 UACC NO_WB
         vmulhd.uu H8(0++,0)+r4, H16(8++,0)+r4, 0x101             REP r0
         veor     H8(0++,0)+r4, H8(0++,0)+r4, 255                 REP r0

         vst      H32(0++,0)+r4, (r6+=r1)                         REP r0
         add      r6, 64

         bitflip  r4, 11

         addcmpbge r5, r8, 16, .loop

         mov      r9, -1
         shl      r9, r5
         vbitplanes  -, r9                                        SETF

         addcmpbgt r5, 0, 0, .final

         ldm      r6-r10, (sp++)
         .cfa_pop {%r10,%r9,%r8,%r7,%r6}
         b        lr
endfunc ; }}}
.FORGET



;;; *****************************************************************************
;;; NAME
;;;   rgba32_composite_normalised
;;;
;;; SYNOPSIS
;;;   void rgba32_composite_normalised(
;;;      unsigned long       *dest,       /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long       *src,        /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  alphabits,  /* r4 */
;;;      int                  width,      /* r5 */
;;;      int                  height,     /* (sp) */
;;;      int                  alpha);     /* (sp+4) */
;;;
;;; FUNCTION
;;;    blah
;;;
;;; TODO
;;;   Allow height to be any value.
;;;   Forget about the read/write pipelining and work stripes of 16 lines

function rgba32_composite_normalised ; {{{
         stm      r6-r10, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9,%r10}

         shl      r4, 1
         mov      r6, r0
         ld       r0, (sp+20) ; height
         min      r8, r5, 16
         ld       r7, (sp+24) ; alpha
         mov      r9, -1
         vld      H32(0++,0)+r4, (r6+=r1)                         REP r0
         shl      r9, r8
         vld      H32(16++,0)+r4, (r2+=r3)                        REP r0

         mov      r8, -16
         vbitplanes -, r9                                         SETF
         vmov     -, 0x80                                         CLRA UACC

         addcmpble r5, 0, 16, .final

.loop:   vld      H32(32++,0)+r4, (r6+64+=r1)                     REP r0 ; pre-load the next block
         vld      H32(48++,0)+r4, (r2+64+=r3)                     REP r0 ; pre-load the next block
         add      r2, 64

.final:  vmul     H16(24++,0)+r4, H8(16++,0)+r4, r7               REP r0 UACC NO_WB
         vmulhd.uu H8(16++,0)+r4, H16(24++,0)+r4, 0x101           REP r0

         veor     H8(0++,0)+r4, H8(0++,0)+r4, 255                 REP r0
         veor     H16(24++,32)+r4, H8(16++,0)+r4, 255             REP r0

         vmul     H16(8++,0)+r4,  H8(0++,16)+r4, H16(24++,32)+r4  REP r0
         vmul     H16(24++,0)+r4, H8(16++,16)+r4, H8(16++,0)+r4   REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,16)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4,  H8(0++,32)+r4, H16(24++,32)+r4  REP r0
         vmul     H16(24++,0)+r4, H8(16++,32)+r4, H8(16++,0)+r4   REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,32)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4,  H8(0++,48)+r4, H16(24++,32)+r4  REP r0
         vmul     H16(24++,0)+r4, H8(16++,48)+r4, H8(16++,0)+r4   REP r0
         vadd     H16(8++,0)+r4, H16(8++,0)+r4, H16(24++,0)+r4    REP r0 UACC NO_WB
         ; limit H16(8++,0)+r4 to <= 255*255+0x80 here...
         vmulhd.uu H8(0++,48)+r4, H16(8++,0)+r4, 0x101            REP r0

         vmul     H16(8++,0)+r4, H8(0++,0)+r4, H16(24++,32)+r4    REP r0 UACC NO_WB
         vmulhd.uu H8(0++,0)+r4, H16(8++,0)+r4, 0x101             REP r0
         veor     H8(0++,0)+r4, H8(0++,0)+r4, 255                 REP r0


         vst      H32(0++,0)+r4, (r6+=r1)                         IFZ REP r0
         add      r6, 64

         bitflip  r4, 11

         addcmpbge r5, r8, 16, .loop

         mov      r9, -1
         shl      r9, r5
         vbitplanes  -, r9                                        SETF

         addcmpbgt r5, 0, 0, .final

         ldm      r6-r10, (sp++)
         .cfa_pop {%r10,%r9,%r8,%r7,%r6}
         b        lr
endfunc ; }}}
.FORGET

;;; NAME
;;;    rgba32_transpose
;;;
;;; SYNOPSIS
;;;    void rgba32_transpose(void *dest, const void *src, int dest_pitch, int src_pitch, int src_height)
;;;
;;; FUNCTION
;;;    Transpose a 16-pixel wide vertical strip from src into a horizontal stripe in dest.
;;;
;;; RETURNS
;;;    -
function rgba32_transpose ; {{{
      v32ld          H32(0++,0), (r1+=r3) REP 16
      v32st          V32(0,0++), (r0+=r2) REP 16
.ifdef __BCM2707B0__
      v32add         V32(0,15), V32(0,15), 0 ; B0 workaround
.endif
      add            r0, 64
      addscale       r1, r3 << 4
      sub            r4, 16
      addcmpbgt      r4, 0, 0, rgba32_transpose
      b              lr
endfunc ; }}}
.FORGET
