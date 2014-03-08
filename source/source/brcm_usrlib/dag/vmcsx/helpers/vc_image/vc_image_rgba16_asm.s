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


;;; *****************************************************************************
;;; NAME
;;;   rgba16_from_rgb565_blocky
;;;
;;; SYNOPSIS
;;;   void rgba16_from_rgb565_blocky(
;;;      unsigned short      *desta16,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*src565,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgb565 image to rgba16, completely opaque

.function rgba16_from_rgb565_blocky ; {{{
         stm      r6-r8, (--sp)
         .cfa_push {%r6,%r7,%r8}

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

.vloop:  add      r5, r0, r6

.hloop:  vld      HX(0++,0), (r2+=r3)              REP 16

         vbitplanes HX(0++,0), HX(0++,0)           REP 16

         ;vmov     VX(0,12++), VX(0,12++)           REP 4 ; red
         vmov     VX(0,11), VX(0,10)               ; green
         vmov     VX(0,10), VX(0,9)
         vmov     VX(0,9), VX(0,8)
         vmov     VX(0,8), VX(0,7)
         vmov     VX(0,7), VX(0,4)                 ; blue
         vmov     VX(0,6), VX(0,3)
         vmov     VX(0,5), VX(0,2)
         vmov     VX(0,4), VX(0,1)
         vmov     VX(0,0++), 0xFFFF                REP 4 ; alpha

         vbitplanes HX(0++,0), HX(0++,0)           REP 16

         vst      HX(0++,0), (r0+=r1)              REP 16

         add      r2, r8
         addcmpbne r0, r8, r5, .hloop
         sub      r0, r6
         sub      r2, r6

         addscale r0, r1 << 4
         addscale r2, r3 << 4

         addcmpbne r7, -1, 0, .vloop

         ldm      r6-r8, (sp++)
         .cfa_pop {%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_from_rgba32_blocky
;;;
;;; SYNOPSIS
;;;   void rgba16_from_rgba32_blocky(
;;;      unsigned short      *desta16,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long const *srca32,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgba32 image to rgba16

.function rgba16_from_rgba32_blocky ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

.vloop:  add      r9, r0, r6

.hloop:  vld      H32(8++,0), (r2+=r3)             REP 16

         mov      r4, 0
         mov      r5, 0

.loop:   vmulhn.uu H(0,0), H(8,0)+r5,  3855                    ; red
         vmulhn.uu H(1,0), H(8,16)+r5, 3855                    ; green
         vmulhn.uu H(2,0), H(8,32)+r5, 3855                    ; blue
         vmulhn.uu -,      H(8,48)+r5, 3855        CLRA UACC   ; alpha
         vshl     -,           H(0,0), 12               UACC
         vshl     -,           H(1,0), 8                UACC
         vshl     HX(40,0)+r5, H(2,0), 4                UACC

         add      r5, 64
         addcmpbne r4, 1, 16, .loop

         vst      HX(40++,0), (r0+=r1)             REP 16

         addscale r2, r8 << 1
         addcmpbne r0, r8, r9, .hloop
         sub      r2, r6
         sub      r0, r6
         sub      r2, r6

         addscale r0, r1 << 4
         addscale r2, r3 << 4

         addcmpbne r7, -1, 0, .vloop

         ldm      r6-r9, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_to_rgba32_blocky
;;;
;;; SYNOPSIS
;;;   void rgba16_to_rgba32_blocky(
;;;      unsigned short      *desta32,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long const *srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgba16 image to rgba32

.function rgba16_to_rgba32_blocky ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

.vloop:  add      r9, r2, r6

.hloop:  vld      HX(0++,0), (r2+=r3)              REP 16

         mov      r4, 0
         mov      r5, 0

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vmov     V(48,0)+r4, V(16,17)             ; red
         vmov     V(48,16)+r4, V(16,16)            ; green
         vmov     V(48,1)+r4, V(16,1)              ; blue
         vmov     V(48,17)+r4, V(16,0)             ; alpha

         vand     VX(16,0), VX(0,8)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,8)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vmov     V(48,32)+r4, V(16,17)
         vmov     V(48,48)+r4, V(16,16)
         vmov     V(48,33)+r4, V(16,1)
         vmov     V(48,49)+r4, V(16,0)
         add      r5, 1
         addcmpbne r4, 2, 16, .loop

         vst      HX(48++,0), (r0+=r1)             REP 16
         vst      HX(48++,32), (r0+32+=r1)         REP 16

         addscale r0, r8 << 1
         addcmpbne r2, r8, r9, .hloop
         sub      r0, r6
         sub      r2, r6
         sub      r0, r6

         addscale r2, r3 << 4
         addscale r0, r1 << 4

         addcmpbne r7, -1, 0, .vloop

         ldm      r6-r9, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_to_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba16_to_rgb565(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an arbitrarily-aligned part of an RGBA565 image into an
;;;   arbitrarily-aligned part of an RGB565 image.
;;;
;;; TODO
;;;   test

.function rgba16_to_rgb565 ; {{{
         stm      r6-r8,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8}

         add      r6, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         lsr      r4, 1
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vlsr     V(16,1), V(16,1), 3              ; blue
         vand     V(16,17), V(16,17), 0xF8         ; red
         vlsr     V(16,16), V(16,16), 2            ; green
         vshl     -, V(16,16), 5                   CLRA ACC
         vmov     VX(48,0)+r4, VX(16,0)            ACC

         addcmpbne r4, 1, 16, .nostore
         blo      .slowstore
         vst      HX(48++,0), (r0+=r1)             REP 16
.stored: add      r0, 32
         mov      r4, 0
.lpent:  vld      HX(48++,0), (r0+=r1)             REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         vld      HX(0++,0), (r2+=r3)              REP 16
         bmask    r5, 4
         add      r2, 32
.noload: 

         addcmpbgt r6, -1, 0, .loop

         addcmpbeq r4, 0, 0, .done
         bhs      .faststore
         add      lr, pc, pcrel(.done)

.slowstore:
         mov      r4, 0
         mov      r8, r0
.repn:   vst      HX(48,0)+r4, (r0)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         mov      r0, r8
         b        lr

.faststore:
         vst      HX(48++,0), (r0+=r1)             REP 16
.done:   ldm      r6-r8,pc, (sp++)
         .cfa_pop {%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_to_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba16_to_rgb888(
;;;      unsigned char       *dest888,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB888 image.
;;;
;;; TODO
;;;   test

.function rgba16_to_rgb888 ; {{{
         stm      r6-r9,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9}

         cbclr
         add      r6, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         bic      r0, 31         ; normally we ignore this and depend on vld rounding behaviour, but here we use H() accesses and want 32-byte alignment
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1

         vld      H(48++,0)*, (r0+=r1)              REP 16
         vld      H(48++,16)*, (r0+16+=r1)          REP 16
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vrsub    VX(16,2), V(16,17), 0x100

.ifdef RGB888
         vmov     V(48,0)+r4*, V(16,17)            ; red
         vmov     V(48,1)+r4*, V(16,16)            ; green
         vmov     V(48,2)+r4*, V(16,1)             ; blue
.else
         vmov     V(48,0)+r4*, V(16,1)             ; blue
         vmov     V(48,1)+r4*, V(16,16)            ; green
         vmov     V(48,2)+r4*, V(16,17)            ; red
.endif

         addcmpblt r4, 3, 32, .nostore
         blo      .slowstore
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.stored: cbadd2
         add      r0, 32
         bmask    r4, 5
.lpent:  vld      H(48++,32)*, (r0+32+=r1)         REP 16
         vld      H(48++,48)*, (r0+48+=r1)         REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         vld      HX(0++,0), (r2+=r3)              REP 16
         bmask    r5, 4
         add      r2, 32
.noload: 

         addcmpbgt r6, -1, 0, .loop

         addcmpbeq r4, 0, 0, .done
         bhs      .faststore
         add      lr, pc, pcrel(.done)

.slowstore:
         mov      r9, r4
         mov      r4, 0
         mov      r8, r0
.repn:   vst      H(48,0)+r4*, (r0)
         vst      H(48,16)+r4*, (r0+16)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         mov      r0, r8
         mov      r4, r9
         b        lr

.faststore:
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.done:   ldm      r6-r9,pc, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_overlay_to_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba16_overlay_to_rgb565(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB565 image.

.function rgba16_overlay_to_rgb565 ; {{{
         stm      r6-r8,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8}

         add      r6, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         lsr      r4, 1
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vrsub    VX(16,2), V(16,0), 0x100

         vand     VX(16,4), VX(48,0)+r4, ~0x07E0
         vand     VX(16,5), VX(48,0)+r4, 0x07E0

         vmulm    V(16,4), V(16,4), 0x840
         vmulm    V(16,20), V(16,20), 0x108
         vmulhd.uu V(16,5), VX(16,5), 0x2080

         vsub     VX(16,3), V(16,4), V(16,1)       ; blue
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(16,4), VX(16,3), V(16,1)

         vsub     VX(16,3), V(16,5), V(16,16)      ; green
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(16,5), VX(16,3), V(16,16)

         vsub     VX(16,3), V(16,20), V(16,17)     ; red
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(16,20), VX(16,3), V(16,17)

         vlsr     V(16,4), V(16,4), 3
         vand     V(16,20), V(16,20), 0xF8
         vlsr     V(16,5), V(16,5), 2
         vshl     -, V(16,5), 5                    CLRA ACC
         vmov     VX(48,0)+r4, VX(16,4)            ACC

         addcmpbne r4, 1, 16, .nostore
         blo      .slowstore
         vst      HX(48++,0), (r0+=r1)             REP 16
.stored: add      r0, 32
         mov      r4, 0
.lpent:  vld      HX(48++,0), (r0+=r1)             REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         vld      HX(0++,0), (r2+=r3)              REP 16
         bmask    r5, 4
         add      r2, 32
.noload: 

.if 0
         addcmpbgt r6, -1, 0, .loop
.else
         addcmpble r6, -1, 0, .notloop
         b        .loop
.notloop:
.endif

         addcmpbeq r4, 0, 0, .done
         bhs      .faststore
         add      lr, pc, pcrel(.done)

.slowstore:
         mov      r4, 0
         mov      r8, r0
.repn:   vst      HX(48,0)+r4, (r0)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         mov      r0, r8
         b        lr

.faststore:
         vst      HX(48++,0), (r0+=r1)             REP 16
.done:   ldm      r6-r8,pc, (sp++)
         .cfa_pop {%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_overlay_to_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba16_overlay_to_rgb888(
;;;      unsigned char       *dest888,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB888 image.

.function rgba16_overlay_to_rgb888 ; {{{
         stm      r6-r9,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9}

         cbclr
         add      r6, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         bic      r0, 31         ; normally we ignore this and depend on vld rounding behaviour, but here we use H() accesses and want 32-byte alignment
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1

         vld      H(48++,0)*, (r0+=r1)              REP 16
         vld      H(48++,16)*, (r0+16+=r1)          REP 16
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vrsub    VX(16,2), V(16,0), 0x100

         vsub     VX(16,3), V(48,2)+r4*, V(16,17)  ; red
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(48,0)+r4*, VX(16,3), V(16,17)

         vsub     VX(16,3), V(48,1)+r4*, V(16,16)  ; green
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(48,1)+r4*, VX(16,3), V(16,16)

         vsub     VX(16,3), V(48,0)+r4*, V(16,1)   ; blue
         vmulm    VX(16,3), VX(16,3), VX(16,2)
         vadds    V(48,2)+r4*, VX(16,3), V(16,1)

         addcmpblt r4, 3, 32, .nostore
         blo      .slowstore
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.stored: cbadd2
         add      r0, 32
         bmask    r4, 5
.lpent:  vld      H(48++,32)*, (r0+32+=r1)         REP 16
         vld      H(48++,48)*, (r0+48+=r1)         REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         vld      HX(0++,0), (r2+=r3)              REP 16
         bmask    r5, 4
         add      r2, 32
.noload: 

         addcmpbgt r6, -1, 0, .loop

         addcmpbeq r4, 0, 0, .done
         bhs      .faststore
         add      lr, pc, pcrel(.done)

.slowstore:
         mov      r9, r4
         mov      r4, 0
         mov      r8, r0
.repn:   vst      H(48,0)+r4*, (r0)
         vst      H(48,16)+r4*, (r0+16)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         mov      r0, r8
         mov      r4, r9
         b        lr

.faststore:
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.done:   ldm      r6-r9,pc, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba16_overlay_to_rgba32
;;;
;;; SYNOPSIS
;;;   void rgba16_overlay_to_rgba32(
;;;      unsigned short      *desta32,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca16,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      int                  ascale);    /* (sp) -> (sp+16) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGBA32 image.

.function rgba16_overlay_to_rgba32 ; {{{
         stm      r6-r8,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8}

         ld       r6, (sp+16)
         vmov     VX(16,8), r6

         add      r6, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         lsr      r4, 1
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0x0F0F     ; alpha -> V(16,0) green -> V(16,16)
         vand     VX(16,1), VX(0,0)+r5, 0xF0F0     ; blue -> V(16,1) red -> V(16,17)
         vmulm    VX(16,0), VX(16,0), 0x1100
         vmulm    VX(16,1), VX(16,1), 0x0110

         vmulm    VX(16,4), V(48,17)+r4, 0x102     ; dest alpha weight adjust
         vmulm    VX(16,5), V(16,0), VX(16,8)      ; src alpha weight adjust

         vsub     -, VX(16,5), VX(16,4)            SETF

         ; red
         vmov     VX(16,3), V(48,1)+r4             IFC CLRA ACC
         vmov     VX(16,3), V(16,17)               IFNC CLRA ACC
         vrsub    VX(16,3), VX(16,3), V(48,1)+r4
         vmulm    VX(16,3), VX(16,4), VX(16,3)     ACC
         vrsub    VX(16,3), VX(16,3), V(16,17)
         vmulm    V(48,0)+r4, VX(16,5), VX(16,3)   ACC
         ; green
         vmov     VX(16,3), V(48,16)+r4            IFC CLRA ACC
         vmov     VX(16,3), V(16,16)               IFNC CLRA ACC
         vrsub    VX(16,3), VX(16,3), V(48,16)+r4
         vmulm    VX(16,3), VX(16,4), VX(16,3)     ACC
         vrsub    VX(16,3), VX(16,3), V(16,16)
         vmulm    V(48,16)+r4, VX(16,5), VX(16,3)  ACC
         ; blue
         vmov     VX(16,3), V(48,0)+r4             IFC CLRA ACC
         vmov     VX(16,3), V(16,1)                IFNC CLRA ACC
         vrsub    VX(16,3), VX(16,3), V(48,0)+r4
         vmulm    VX(16,3), VX(16,4), VX(16,3)     ACC
         vrsub    VX(16,3), VX(16,3), V(16,1)
         vmulm    V(48,1)+r4, VX(16,5), VX(16,3)   ACC

         ; calculate new alpha
         vrsub    VX(16,5), VX(16,5), 0x100
         vrsub    VX(16,4), VX(16,4), 0x100
         vmulm    VX(16,5), VX(16,5), VX(16,4)
         vrsubs   V(48,17)+r4, VX(16,5), 0x100

         addcmpbne r4, 2, 16, .nostore
         blo      .slowstore
         vst      HX(48++,0), (r0+=r1)             REP 16
.stored: add      r0, 32
         mov      r4, 0
.lpent:  vld      HX(48++,0), (r0+=r1)             REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         vld      HX(0++,0), (r2+=r3)              REP 16
         bmask    r5, 4
         add      r2, 32
.noload: 

.if 0
         addcmpbgt r6, -1, 0, .loop
.else
         addcmpble r6, -1, 0, .notloop
         b        .loop
.notloop:
.endif

         addcmpbeq r4, 0, 0, .done
         bhs      .faststore
         add      lr, pc, pcrel(.done)

.slowstore:
         mov      r4, 0
         mov      r8, r0
.repn:   vst      HX(48,0)+r4, (r0)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         mov      r0, r8
         b        lr

.faststore:
         vst      HX(48++,0), (r0+=r1)             REP 16
.done:   ldm      r6-r8,pc, (sp++)
         .cfa_pop {%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgb_overlay_to_rgb
;;;
;;; SYNOPSIS
;;;   void rgb_overlay_to_rgb(
;;;      unsigned void       *dest,       /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned void  const*src,        /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      int                  transparent_colour,  /* (sp) -> (sp+20) (only for src=rgb565) */
;;;      int                  ascale);    /* (sp+4) -> (sp+24) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an rgb image onto an
;;;   arbitrarily-aligned part of an rgb image.
;;;
;;; Use helper functions to unpack and pack the data from the memory
;;; format to rgba32 in the VRF.  Source is loaded from r2+=r3 into the second
;;; row, destination is loaded and stored from r0+=r1 into the first row.
;;; Pixels are stored in planar format, columns from left to right are
;;; brga.
;;; Only pack/unpack functions references in vc_image_overlay_rgb_to_rgb
;;; are linked in.
        
.function unpack_src_rgba16
        vld HX(48++,0), (r2+=r3) REP 16
        v16ror HX(48++,0), HX(48++,0), 4 REP 16  ; RGBA -> ARGB
        vand HX(16++,0), HX(48++,0), 0x0F0F REP 16
        vand HX(16++,32), HX(48++,0), 0xF0F0 REP 16
        vmulm HX(16++,0), HX(16++,0), 0x1100 REP 16 ; blue red
        vmulm HX(16++,32), HX(16++,32), 0x0110 REP 16 ; green alpha
        add r2, 32
        b lr
.endfn

.function unpack_src_rgba32
        vld HX(48++,0), (r2+=r3) REP 16
        vld HX(48++,32), (r2+32+=r3) REP 16
        vfrom32l HX(16++,0), HX(48++,0), HX(48++,32) REP 16
        vfrom32h HX(16++,32), HX(48++,0), HX(48++,32) REP 16
.ifdef RGB888
        vmov H(48++,0), H(16++,0) REP 16
        vmov H(16++,0), H(16++,32) REP 16
        vmov H(16++,32), H(16++,16) REP 16
        vmov H(16++,16), H(48++,0) REP 16
.else
        vmov H(48++,0), H(16++,16) REP 16
        vmov H(16++,16), H(16++,32) REP 16
        vmov H(16++,32), H(48++,0) REP 16
.endif
        add r2, 64
        b lr
.endfn

.function unpack_src_rgb565
        vld HX(48++,0), (r2+=r3) REP 16

        vand HX(48++,32), HX(48++,0), 0x07e0 REP 16
        vmulhd.su H(16++,32), HX(48++,32), 0x2080 REP 16 ; green
        vand HX(48++,32), HX(48++,0), 0xf81f REP 16
        vmulm H(16++,16), H(48++,48), 0x108 REP 16 ; red
        vmulm H(16++,0), H(48++,32), 0x840 REP 16 ; blue
                
        vsub HX(48++,32), HX(48++,0), r6 REP 16
        vtestmag H(16++,48), HX(48++,32), 1 REP 16
        vmul H(16++,48), H(16++,48), 255 REP 16 ; alpha

        add r2, 32
        b lr
.endfn

.function unpack_dest_rgba16    
        vld HX(48++,0), (r0+=r1) REP 16
        v16ror HX(48++,0), HX(48++,0), 4 REP 16  ; RGBA -> ARGB
        vand HX(0++,0), HX(48++,0), 0x0F0F REP 16
        vand HX(0++,32), HX(48++,0), 0xF0F0 REP 16
        vmulm HX(0++,0), HX(0++,0), 0x1100 REP 16 ; blue red
        vmulm HX(0++,32), HX(0++,32), 0x0110 REP 16 ; green alpha
        b lr
.endfn

.function unpack_dest_rgba32
        vld HX(48++,0), (r0+=r1) REP 16
        vld HX(48++,32), (r0+32+=r1) REP 16
        vfrom32l HX(0++,0), HX(48++,0), HX(48++,32) REP 16
        vfrom32h HX(0++,32), HX(48++,0), HX(48++,32) REP 16
.ifdef RGB888
        vmov H(48++,0), H(0++,0) REP 16
        vmov H(0++,0), H(0++,32) REP 16
        vmov H(0++,32), H(0++,16) REP 16
        vmov H(0++,16), H(48++,0) REP 16
.else
        vmov H(48++,0), H(0++,16) REP 16
        vmov H(0++,16), H(0++,32) REP 16
        vmov H(0++,32), H(48++,0) REP 16
.endif
        b lr
.endfn

.function unpack_dest_rgb565
        vld HX(48++,0), (r0+=r1) REP 16

        vand HX(48++,32), HX(48++,0), 0x07e0 REP 16
        vmulhd.su H(0++,32), HX(48++,32), 0x2080 REP 16 ; green
        vand HX(48++,32), HX(48++,0), 0xf81f REP 16
        vmulm H(0++,16), H(48++,48), 0x108 REP 16 ; red
        vmulm H(0++,0), H(48++,32), 0x840 REP 16 ; blue
                
        vsub HX(48++,32), HX(48++,0), r6 REP 16
        vtestmag H(0++,48), HX(48++,32), 1 REP 16
        vmul H(0++,48), H(0++,48), 255 REP 16 ; alpha

        b lr
.endfn

.function pack_rgba16
        vmulm HX(0++,0), HX(0++,0), 0x010 REP 16
        vand  HX(0++,0), HX(0++,0), 0x0f0f REP 16
        vand  HX(0++,32), HX(0++,32), 0xf0f0 REP 16
        vor   HX(0++,0), HX(0++,0), HX(0++,32) REP 16
        v16ror HX(0++,0), HX(0++,0), 12 REP 16  ; ARGB -> RGBA
        blo slowstore_16bpp
        vst HX(0++,0), (r0+=r1) REP 16
        add r0, 32
        b lr
.endfn

        
.function pack_rgba32
.ifndef RGB888
        vmov H(48++,0), H(0++,0) REP 16
        vmov H(0++,0), H(0++,16) REP 16
        vmov H(0++,16), H(0++,32) REP 16
        vmov H(0++,32), H(48++,0) REP 16
.else
        vmov H(48++,0), H(0++,16) REP 16
        vmov H(0++,16), H(0++,32) REP 16
        vmov H(0++,32), H(48++,0) REP 16
.endif
        vto32l HX(48++,0), HX(0++,0), HX(0++,32) REP 16
        vto32h HX(48++,32), HX(0++,0), HX(0++,32) REP 16
        vst HX(48++,0), (r0+=r1) REP 16
        vst HX(48++,32), (r0+32+=r1) REP 16
        add r0, 64
        b lr
.endfn


.function pack_rgb565
        vlsr H(48++,0), H(0++,0), 3 REP 16 ; blue
        vand H(48++,16), H(0++,16), 0xF8 REP 16 ;  red
        vlsr H(0++,32), H(0++,32), 2 REP 16 ;  green
        vshl HX(48++,32), H(0++,32), 5 REP 16
        vor HX(0++,0), HX(48++,0), HX(48++,32) REP 16
        blo slowstore_16bpp
        vst HX(0++,0), (r0+=r1) REP 16
        add r0, 32
        b lr
.endfn

.function slowstore_16bpp
         mov      r4, 0
         mov      r8, r0
.repn:   vst      HX(0,0)+r4, (r0)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         add      r0, r8, 32
         b        lr
.endfn

.function slowstore_32bpp
         mov      r4, 0
         mov      r8, r0
.repn:   vst      HX(0,0)+r4, (r0)
         vst      HX(0,32)+r4, (r0+32)
         add      r0, r1
         add      r4, 64
         addcmpbne r4, 0, r7, .repn
         add      r0, r8, 64
         b        lr
.endfn

        
        ;; void rgb_overlay_to_rgb()
        ;; 
        ;; r0 destination pointer
        ;; r1 destination pitch
        ;; r2 source pointer
        ;; r3 source pitch
        ;; r4 destination column index
        ;; r5 source column index
        ;; r6 transparent colour
        ;; r7 height x 64 (vrf offset of store)
        ;; r8 temp for slow store routine
        ;; r9 width + 1 (columns remaining)
        ;; r10 source unpack fn 
        ;; r11 destination unpack fn
        ;; r12 destination pack fn
.function rgb_overlay_to_rgb
         stm      r6-r12,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12}

         ld       r6, (sp+36)   ; alpha value
         vmov     VX(32,8), r6

         ld       r6, (sp+32)   ; transparent colour
         ld       r10, (sp+40)  ; src unpack fn
         ld       r11, (sp+44)  ; dest unpack fn
         ld       r12, (sp+48)  ; dest pack fn
        
         add      r9, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         lsr      r4, 1        ; dest column index
         lsr      r5, 1        ; src column index

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      r5, 16-1
         b        .lpent

.loop:

         vmulm    VX(32,4), V(0,48)+r4,  0x102        ; da
         vmulm    VX(32,5), V(16,48)+r5, VX(32,8)     ; sa

         vsub     -, VX(32,5), VX(32,4)            SETF

         vmov     VX(32,3), V(0,0)+r4               IFC CLRA ACC
         vmov     VX(32,3), V(16,0)+r5                IFNC CLRA ACC
         vrsub    VX(32,3), VX(32,3), V(0,0)+r4
         vmulm    VX(32,3), VX(32,4), VX(32,3)     ACC
         vrsub    VX(32,3), VX(32,3), V(16,0)+r5
         vmulm    V(0,0)+r4, VX(32,5), VX(32,3)      ACC

         vmov     VX(32,3), V(0,16)+r4             IFC CLRA ACC
         vmov     VX(32,3), V(16,16)+r5            IFNC CLRA ACC
         vrsub    VX(32,3), VX(32,3), V(0,16)+r4
         vmulm    VX(32,3), VX(32,4), VX(32,3)     ACC
         vrsub    VX(32,3), VX(32,3), V(16,16)+r5
         vmulm    V(0,16)+r4, VX(32,5), VX(32,3)      ACC

         vmov     VX(32,3), V(0,32)+r4               IFC CLRA ACC
         vmov     VX(32,3), V(16,32)+r5               IFNC CLRA ACC
         vrsub    VX(32,3), VX(32,3), V(0,32)+r4
         vmulm    VX(32,3), VX(32,4), VX(32,3)     ACC
         vrsub    VX(32,3), VX(32,3), V(16,32)+r5
         vmulm    V(0,32)+r4, VX(32,5), VX(32,3)     ACC
        
         vrsub    VX(32,5), VX(32,5), 0x100
         vrsub    VX(32,4), VX(32,4), 0x100
         vmulm    VX(32,5), VX(32,5), VX(32,4)
         vrsubs   V(0,48)+r4, VX(32,5), 0x100

         addcmpbne r4, 1, 16, .nostore
         bl r12                 ; pack & store dest

.stored:
         mov      r4, 0
.lpent:
         bl r11                 ; load & unpack dest
        
.nostore:
         addcmpblo r5, 1, 16, .noload
         bl r10                 ; load & unpack source
         bmask    r5, 4
.noload: 

.if 0
         addcmpbgt r9, -1, 0, .loop
.else
         addcmpble r9, -1, 0, .notloop
         b        .loop
.notloop:
.endif

         addcmpbeq r4, 0, 0, .done
         bl r12                 ; pack & store dest

.done:   ldm      r6-r12,pc, (sp++)
         ;.cfa_pop {%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
        
         
.FORGET

;******************************************************************************
;NAME
;   vc_deflicker_rgba16
;
;SYNOPSIS
;   void vc_deflicker_rgba16(unsigned char *data, int pitch, unsigned char *context);
;
;FUNCTION
;   Deflicker a 16x16 RGBA block using a simple low pass filter
;   [1/4 1/2 1/4] vertically. Uses context as a two line buffer between calls.
;
;RETURNS
;   -
;******************************************************************************

.function vc_deflicker_rgba16
        add r3, r0, r1
        mov r4, 32
loop:
        vld HX(2++, 0), (r0+=r1) REP 16
        vld HX(0++,0), (r2+=r1) REP 2
        vst HX(16++, 0), (r2+=r1) REP 2

        vand HX(32++,0), HX(0++,0), 0x0F0F REP 16
        vand HX(48++,0), HX(16++,0), 0x0F0F REP 2
        vand HX(32++,32), HX(0++,0), 0xF0F0 REP 16
        vand HX(48++,32), HX(16++,0), 0xF0F0 REP 2
        
        mov r5, 4
        cbclr
next_component:      
        vadd  HX(0++,32), H(32++,0)*, H(33++,0)* REP 16
        vadd  HX(0++,32), HX(0++,32), H(33++,0)* REP 16
        vadd  HX(0++,32), HX(0++,32), H(34++,0)* REP 16
        vlsr  H(32++,0)*, HX(0++,32), 2 REP 16
        cbadd1
        addcmpbgt r5, -1, 0, next_component
        
        vand HX(32++,0), HX(32++,0), 0x0F0F REP 16
        vand HX(32++,32), HX(32++,32), 0xF0F0 REP 16
        vor HX(0++,0), HX(32++,0), HX(32++,32) REP 16
        vst HX(0++,0), (r0+=r1) REP 16

        add r2, r4
        addcmpblt r0, r4, r3, loop
        b lr
.endfn

