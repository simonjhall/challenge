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

.define RGBA565_TRANSPARENTBITS, 0x18DF
.define RGBA565_TRANSPARENTKEY, 0xE700
.define RGBA565_ALPHABITS, 0x001C

;;; *****************************************************************************

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
;;;   rgba565_from_rgba32_blocky
;;;
;;; SYNOPSIS
;;;   void rgba565_from_rgba32_blocky(
;;;      unsigned short      *desta565,   /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long const *srca32,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgba32 image to rgba565

function rgba565_from_rgba32_blocky ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}

         add      r4, 15
         add      r5, 15

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

         assertne r6
         assertne r7

.vloop:  add      r9, r0, r6

.hloop:  vld      HX(0++,0), (r2+=r3)              REP 16
         vld      HX(0++,32), (r2+32+=r3)          REP 16

         mov      r4, 0
         mov      r5, 0

.loop:   vand     VX(16,0), VX(0,0)+r5, 0xFCF8     ; green mask / blue mask
         vand     VX(16,1), VX(0,1)+r5, 0xE0F8     ; alpha mask / red mask

         vlsr     -, V(16,0), 3                    CLRA UACC ; blue
         vshl     -, V(16,1), (11-3)               UACC      ; red
         vshl     VX(48,0)+r4, V(16,16), (5-2)     UACC      ; green

         vsub     -, V(0,17)+r5, 0xF0              SETF  ; alpha test
         vand     VX(16,0), VX(48,0)+r4, 0xC618    IFN
         vor      VX(16,0), VX(16,0), V(16,17)     IFN
         vlsr     VX(16,0), VX(16,0), 3            IFN
         vor      VX(48,0)+r4, VX(16,0), RGBA565_TRANSPARENTKEY IFN


         vand     VX(16,0), VX(0,32)+r5, 0xFCF8    ; green mask / blue mask
         vand     VX(16,1), VX(0,33)+r5, 0xE0F8    ; alpha mask / red mask

         vlsr     -, V(16,0), 3                    CLRA UACC ; blue
         vshl     -, V(16,1), (11-3)               UACC      ; red
         vshl     VX(48,8)+r4, V(16,16), (5-2)     UACC      ; green

         vsub     -, V(0,49)+r5, 0xF0              SETF  ; alpha test
         vand     VX(16,0), VX(48,8)+r4, 0xC618    IFN
         vor      VX(16,0), VX(16,0), V(16,17)     IFN
         vlsr     VX(16,0), VX(16,0), 3            IFN
         vor      VX(48,8)+r4, VX(16,0), RGBA565_TRANSPARENTKEY IFN

         add      r5, 2
         addcmpbne r4, 1, 8, .loop

         vst      HX(48++,0), (r0+=r1)             REP 16

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_from_rgba32
;;;
;;; SYNOPSIS
;;;   void rgba565_from_rgba32(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an arbitrarily-aligned part of an RGBA32 image into an
;;;   arbitrarily-aligned part of an RGBA565 image.

function rgba565_from_rgba32 ; {{{
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
         add      r5, 16-2
         b        .lpent

.loop:   vand     VX(16,0), VX(0,0)+r5, 0xFCF8     ; green mask / blue mask
         vand     VX(16,1), VX(0,1)+r5, 0xE0F8     ; alpha mask / red mask

         vlsr     -, V(16,0), 3                    CLRA UACC ; blue
         vshl     -, V(16,1), (11-3)               UACC      ; red
         vshl     VX(48,0)+r4, V(16,16), (5-2)     UACC      ; green

         vsub     -, V(0,17)+r5, 0xF0              SETF  ; alpha test
         vand     VX(16,0), VX(48,0)+r4, 0xC618    IFN
         vor      VX(16,0), VX(16,0), V(16,17)     IFN
         vlsr     VX(16,0), VX(16,0), 3            IFN
         vor      VX(48,0)+r4, VX(16,0), RGBA565_TRANSPARENTKEY IFN

         addcmpbne r4, 1, 16, .nostore
         blo      .slowstore
         vst      HX(48++,0), (r0+=r1)             REP 16
.stored: add      r0, 32
         mov      r4, 0
.lpent:  vld      HX(48++,0), (r0+=r1)             REP 16
.nostore:

         addcmpblo r5, 2, 16, .noload
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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_to_rgba32_blocky
;;;
;;; SYNOPSIS
;;;   void rgba565_to_rgba32_blocky(
;;;      unsigned short      *desta32,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long const *srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgba565 image to rgba32

function rgba565_to_rgba32_blocky ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

         assertne r6
         assertne r7

.vloop:  add      r9, r2, r6

.hloop:  vld      HX(0++,0), (r2+=r3)              REP 16

         mov      r4, 0
         mov      r5, 0

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vmov     V(48,17)+r4, 0xFF
         vshl     V(16,0), V(0,0)+r5, 3            IFZ
         vand     V(48,17)+r4, V(16,0), 0xE0       IFZ
         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,0)+r5, VX(16,0), 10         IFZ

         vand     VX(16,0), VX(0,0)+r5, ~0x07E0
         vand     VX(16,1), VX(0,0)+r5, 0x7E0
         vmulm    V(48,0)+r4, V(16,0), 0x840
         vmulm    V(48,1)+r4, V(16,16), 0x108
         vmulhd.su V(48,16)+r4, VX(16,1), 0x2080


         vand     VX(16,0), VX(0,8)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vmov     V(48,49)+r4, 0xFF
         vshl     V(16,0), V(0,8)+r5, 3            IFZ
         vand     V(48,49)+r4, V(16,0), 0xE0       IFZ
         vand     VX(16,0), VX(0,8)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,8)+r5, VX(16,0), 10         IFZ

         vand     VX(16,0), VX(0,8)+r5, ~0x07E0
         vand     VX(16,1), VX(0,8)+r5, 0x7E0
         vmulm    V(48,32)+r4, V(16,0), 0x840
         vmulm    V(48,33)+r4, V(16,16), 0x108
         vmulhd.su V(48,48)+r4, VX(16,1), 0x2080

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_from_rgb565_blocky
;;;
;;; SYNOPSIS
;;;   void rgba565_from_rgb565_blocky(
;;;      unsigned short      *desta565,   /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*src565,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Moves all the unsafe colours in an RGB565 image to nearby safe colours

function rgba565_from_rgb565_blocky ; {{{
         stm      r6-r8, (--sp)
         .cfa_push {%r6,%r7,%r8}

         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

         assertne r6
         assertne r7

.vloop:  add      r5, r0, r6

.hloop:  vld      HX(0++,0), (r2+=r3)              REP 16

         mov      r4, 0
.loop:   vand     VX(16,0), VX(0,0)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(0,0)+r4, VX(0,0)+r4, 0x0020   IFZ
         vand     VX(16,0), VX(0,1)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(0,1)+r4, VX(0,1)+r4, 0x0020   IFZ
         vand     VX(16,0), VX(0,2)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(0,2)+r4, VX(0,2)+r4, 0x0020   IFZ
         vand     VX(16,0), VX(0,3)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(0,3)+r4, VX(0,3)+r4, 0x0020   IFZ
         addcmpbne r4, 4, 16, .loop

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba32_overlay_to_rgba565
;;;
;;; SYNOPSIS
;;;   void rgba32_overlay_to_rgba565(
;;;      unsigned short      *desta565,   /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca32,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      int                  ascale);    /* (sp) -> (sp+16) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA32 image onto an
;;;   arbitrarily-aligned part of an RGBA565 image.
;;;
;;; TODO
;;;   Optimise -- this is an un-optimised composite of other functions

function rgba32_overlay_to_rgba565 ; {{{
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
         add      r5, 16-2
         b        .lpent

.loop:   vand     VX(16,0), VX(48,0)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         vmov     VX(16,7), 0x100
         vand     VX(16,7), V(48,0)+r4, RGBA565_ALPHABITS IFZ
         vshl     VX(16,7), V(16,7), 3                   IFZ
         vand     VX(16,0), VX(48,0)+r4, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(48,0)+r4, VX(16,0), 10              IFZ

         vmulm    VX(16,6), V(0,17)+r5, VX(16,8) ; alpha weight adjust

         vsub     -, VX(16,6), VX(16,7)            SETF

         vand     VX(16,4), VX(48,0)+r4, 0x07E0
         vand     VX(16,5), VX(48,0)+r4, ~0x07E0
         vmulhd.su VX(16,4), VX(16,4), 0x2080 ; green weight adjust
         vmulm    V(16,5+16), V(16,5+16), 0x108 ; red weight adjust
         vmulm    V(16,5), V(16,5), 0x840 ; blue weight adjust

         ; red
         vmov     VX(16,0), V(16,5+16)             IFC CLRA ACC
         vmov     VX(16,0), V(0,1)+r5              IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(16,5+16)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(0,1)+r5
         vmulm    V(16,5+16), VX(16,6), VX(16,0)   ACC
         ; green
         vmov     VX(16,0), VX(16,4)               IFC CLRA ACC
         vmov     VX(16,0), V(0,16)+r5             IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), VX(16,4)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(0,16)+r5
         vmulm    VX(16,4), VX(16,6), VX(16,0)     ACC
         ; blue
         vmov     VX(16,0), V(16,5)                IFC CLRA ACC
         vmov     VX(16,0), V(0,0)+r5              IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(16,5)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(0,0)+r5
         vmulm    VX(16,0), VX(16,6), VX(16,0)     ACC
         vlsr     V(16,5), VX(16,0), 3
         vshl     VX(16,4), VX(16,4), 3
         vand     -, VX(16,4), 0x07E0              CLRA ACC
         vand     VX(48,0)+r4, VX(16,5), ~0x07E0   ACC

         ; move result avay from unsafe colours
         vand     VX(16,0), VX(48,0)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(48,0)+r4, VX(48,0)+r4, 0x20   IFZ

         ; calculate new alpha
         vrsub    VX(16,6), VX(16,6), 0x100
         vrsub    VX(16,7), VX(16,7), 0x100
         vmulm    VX(16,6), VX(16,6), VX(16,7)
         vrsub    VX(16,6), VX(16,6), 0x100+0x10

         ; put new alpha back into result
         vsub     -, VX(16,6), 0xF0+0x10           SETF
         vand     -, VX(16,6), 0xE0                IFN CLRA ACC
         vand     VX(16,0), VX(48,0)+r4, (RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS) << 3 IFN ACC
         vlsr     VX(16,0), VX(16,0), 3            IFN
         vor      VX(48,0)+r4, VX(16,0), RGBA565_TRANSPARENTKEY IFN

         addcmpbne r4, 1, 16, .nostore
         blo      .slowstore
         vst      HX(48++,0), (r0+=r1)             REP 16
.stored: add      r0, 32
         mov      r4, 0
.lpent:  vld      HX(48++,0), (r0+=r1)             REP 16
.nostore:

         addcmpblo r5, 2, 16, .noload
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
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   rgba565_overlay_to_rgba32
;;;
;;; SYNOPSIS
;;;   void rgba565_overlay_to_rgba32(
;;;      unsigned short      *desta32,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      int                  ascale);    /* (sp) -> (sp+16) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGBA32 image.
;;;
;;; TODO
;;;   Optimise -- this is an un-optimised composite of other functions

function rgba565_overlay_to_rgba32 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         vmov     VX(16,6), 0x100
         vand     VX(16,6), V(0,0)+r5, RGBA565_ALPHABITS IFZ
         vshl     VX(16,6), V(16,6), 3                   IFZ
         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,0)+r5, VX(16,0), 10              IFZ

         vmulm    VX(16,7), V(48,17)+r4, 0x102 ; alpha weight adjust
         vmulm    VX(16,6), VX(16,6), VX(16,8) ; src alpha weight adjust

         vsub     -, VX(16,6), VX(16,7)            SETF

         vand     VX(16,4), VX(0,0)+r5, 0x07E0
         vand     VX(16,5), VX(0,0)+r5, ~0x07E0
         vmulhd.su VX(16,4), VX(16,4), 0x2080 ; green weight adjust
         vmulm    V(16,5+16), V(16,5+16), 0x108 ; red weight adjust
         vmulm    V(16,5), V(16,5), 0x840 ; blue weight adjust

         ; red
         vmov     VX(16,0), V(48,1)+r4             IFC CLRA ACC
         vmov     VX(16,0), V(16,5+16)             IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(48,1)+r4
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(16,5+16)
         vmulm    V(48,1)+r4, VX(16,6), VX(16,0)   ACC
         ; green
         vmov     VX(16,0), V(48,16)+r4            IFC CLRA ACC
         vmov     VX(16,0), VX(16,4)               IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(48,16)+r4
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), VX(16,4)
         vmulm    V(48,16)+r4, VX(16,6), VX(16,0)  ACC
         ; blue
         vmov     VX(16,0), V(48,0)+r4             IFC CLRA ACC
         vmov     VX(16,0), V(16,5)                IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(48,0)+r4
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(16,5)
         vmulm    V(48,0)+r4, VX(16,6), VX(16,0)   ACC

         ; calculate new alpha
         vrsub    VX(16,6), VX(16,6), 0x100
         vrsub    VX(16,7), VX(16,7), 0x100
         vmulm    VX(16,6), VX(16,6), VX(16,7)
         vrsubs   V(48,17)+r4, VX(16,6), 0x100

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
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   rgba565_overlay_to_rgba565
;;;
;;; SYNOPSIS
;;;   void rgba565_overlay_to_rgba565(
;;;      unsigned short      *desta565,   /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGBA565 image.
;;;
;;; TODO
;;;   Optimise this.

function rgba565_overlay_to_rgba565 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         vmov     VX(16,6), 0x100
         vand     VX(16,6), VX(0,0)+r5, RGBA565_ALPHABITS IFZ
         vshl     VX(16,6), VX(16,6), 3                  IFZ
         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,0)+r5, VX(16,0), 10               IFZ

         vand     VX(16,0), VX(48,0)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         vmov     VX(16,7), 0x100
         vand     VX(16,7), VX(48,0)+r4, RGBA565_ALPHABITS IFZ
         vshl     VX(16,7), VX(16,7), 3                  IFZ
         vand     VX(16,0), VX(48,0)+r4, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(48,0)+r4, VX(16,0), 10              IFZ

         vsub     -, VX(16,6), VX(16,7)            SETF

         vand     VX(16,2), VX(0,0)+r5, 0x7E0
         vand     VX(16,3), VX(0,0)+r5, ~0x7E0
         vand     VX(16,4), VX(48,0)+r4, 0x7E0
         vand     VX(16,5), VX(48,0)+r4, ~0x07E0

         ; red
         vmov     VX(16,0), V(16,5+16)             IFC CLRA ACC
         vmov     VX(16,0), V(16,3+16)             IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(16,5+16)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(16,3+16)
         vmulm    VX(16,0), VX(16,6), VX(16,0)     ACC
         vand     V(16,5+16), VX(16,0), 0xF8
         ; green
         vmov     VX(16,0), VX(16,4)               IFC CLRA ACC
         vmov     VX(16,0), VX(16,2)               IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), VX(16,4)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), VX(16,2)
         vmulm    VX(16,0), VX(16,6), VX(16,0)     ACC
         vand     VX(16,4), VX(16,0), 0x07E0
         ; blue
         vmov     VX(16,0), V(16,5)                IFC CLRA ACC
         vmov     VX(16,0), V(16,3)                IFNC CLRA ACC
         vrsub    VX(16,0), VX(16,0), V(16,5)
         vmulm    VX(16,0), VX(16,7), VX(16,0)     ACC
         vrsub    VX(16,0), VX(16,0), V(16,3)
         vmulm    V(16,5), VX(16,6), VX(16,0)      ACC
         ; stall from V/VX crossover
         vor      VX(48,0)+r4, VX(16,4), VX(16,5)

         ; move result avay from unsafe colours
         vand     VX(16,0), VX(48,0)+r4, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(48,0)+r4, VX(48,0)+r4, 0x20   IFZ

         ; calculate new alpha
         vrsub    VX(16,6), VX(16,6), 0x100
         vrsub    VX(16,7), VX(16,7), 0x100
         vmulm    VX(16,6), VX(16,6), VX(16,7)
         vrsub    VX(16,6), VX(16,6), 0x100+0x10

         ; put new alpha back into result
         vsub     -, VX(16,6), 0xF0+0x10           SETF
         vand     -, VX(16,6), 0xE0                IFN CLRA ACC
         vand     VX(16,0), VX(48,0)+r4, (RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS) << 3 IFN ACC
         vlsr     VX(16,0), VX(16,0), 3            IFN
         vor      VX(48,0)+r4, VX(16,0), RGBA565_TRANSPARENTKEY IFN

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgb565_overlay_to_rgba565
;;;
;;; SYNOPSIS
;;;   void rgb565_overlay_to_rgba565(
;;;      unsigned short      *desta565,   /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*src565,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      int                  transparent_colour); /* (sp) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of a single-transparent-colour
;;;   RGB565 image onto an arbitrarily-aligned part of an RGBA565 image.

function rgb565_overlay_to_rgba565 ; {{{
         stm      r6-r9,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9}

         ldh      r6, (sp+20) ; transparent_colour
         add      r9, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         lsr      r4, 1
         lsr      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1
         b        .lpent

.loop:   vmov     VX(16,0), VX(0,0)+r5
         vand     VX(16,1), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,1), RGBA565_TRANSPARENTKEY    SETF
         veor     VX(0,0)+r5, VX(0,0)+r5, 0x0020   IFZ

         veor     -, VX(16,0), r6                  SETF
         vmov     VX(48,0)+r4, VX(0,0)+r5          IFNZ

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

         addcmpbgt r9, -1, 0, .loop

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
.done:   ldm      r6-r9,pc, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_to_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba565_to_rgb565(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an arbitrarily-aligned part of an RGBA565 image into an
;;;   arbitrarily-aligned part of an RGB565 image.

function rgba565_to_rgb565 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vmov     VX(48,0)+r4, VX(0,0)+r5
         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(48,0)+r4, VX(16,0), 10        IFZ

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_overlay_to_rgb565
;;;
;;; SYNOPSIS
;;;   void rgba565_overlay_to_rgb565(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB565 image.
;;;
;;; TODO
;;;   Optimise this.

function rgba565_overlay_to_rgb565 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vshl     VX(16,0), VX(0,0)+r5, 3          IFZ
         vand     VX(16,4), VX(16,0), 0xE0         IFZ
         vrsub    VX(16,5), VX(16,4), 0x100        IFZ
         vand     VX(16,0), VX(16,0), (RGBA565_TRANSPARENTBITS << 3) & ~0xE0   IFZ
         vmulm    V(16,0), V(16,0), 320            IFZ ; optional bit-extension
         vmulm    V(16,16), V(16,16), 320          IFZ ; workaround for sign-extended mulm

         vand     VX(16,1), VX(16,0), 0x07E0       IFZ
         vand     VX(16,0), VX(16,0), ~0x07E0      IFZ
         vmulm    V(16,0), V(16,0), VX(16,4)       IFZ
         vmulm    V(16,16), V(16,16), VX(16,4)     IFZ ; workaround for sign-extended mulm
         vmulm    VX(16,1), VX(16,1), VX(16,4)     IFZ
         vand     -, VX(16,0), ~0x07E0             IFZ CLRA UACC
         vand     -, VX(16,1), 0x07E0              IFZ UACC

         vand     VX(16,0), VX(48,0)+r4, ~0x07E0   IFZ
         vand     VX(16,1), VX(48,0)+r4, 0x07E0    IFZ
         vmulm    V(16,0), V(16,0), VX(16,5)       IFZ
         vmulm    V(16,16), V(16,16), VX(16,5)     IFZ ; workaround for sign-extended mulm
         vmulm    VX(16,1), VX(16,1), VX(16,5)     IFZ
         vand               -, VX(16,0), ~0x07E0   IFZ UACC
         vand     VX(48,0)+r4, VX(16,1), 0x07E0    IFZ UACC

         vmov     VX(48,0)+r4, VX(0,0)+r5          IFNZ

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_to_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba565_to_rgb888(
;;;      unsigned char       *dest888,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB888 image.

function rgba565_to_rgb888 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,0)+r5, VX(16,0), 10         IFZ

         vand     VX(16,0), VX(0,0)+r5, ~0x07E0
         vand     VX(16,1), VX(0,0)+r5, 0x7E0

         vmulm    V(48,0)+r4*, V(16,0), 0x840      ; blue
         vmulhd.su V(48,1)+r4*, VX(16,1), 0x2080   ; green
         vmulm    V(48,2)+r4*, V(16,16), 0x108     ; red

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
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   rgba565_overlay_to_rgb888
;;;
;;; SYNOPSIS
;;;   void rgba565_overlay_to_rgb888(
;;;      unsigned char       *dest888,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned short const*srca565,    /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB888 image.

function rgba565_overlay_to_rgb888 ; {{{
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

.loop:   vand     VX(16,0), VX(0,0)+r5, ~RGBA565_TRANSPARENTBITS
         veor     -, VX(16,0), RGBA565_TRANSPARENTKEY    SETF

         vmov     VX(16,2), 0x100
         vshl     V(16,0), V(0,0)+r5, 3            IFZ
         vand     VX(16,2), V(16,0), 0xE0          IFZ
         vand     VX(16,0), VX(0,0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
         vmul     VX(0,0)+r5, VX(16,0), 10         IFZ

         vrsub    VX(16,2), VX(16,2), 0x100
         vand     VX(16,0), VX(0,0)+r5, ~0x07E0
         vand     VX(16,1), VX(0,0)+r5, 0x7E0

         vmulm    V(16,3), V(16,0), 0x840          CLRA ACC ; blue
         vsub     VX(16,3), V(48,0)+r4*, V(16,3)   ; suffer 1 cycle stall, save one add and three VRF accesses
         vmulm    V(48,0)+r4*, VX(16,3), VX(16,2)  ACC

         vmulhd.su V(16,3), VX(16,1), 0x2080       CLRA ACC ; green
         vsub     VX(16,3), V(48,1)+r4*, V(16,3)   ; suffer 1 cycle stall, save one add and three VRF accesses
         vmulm    V(48,1)+r4*, VX(16,3), VX(16,2)  ACC

         vmulm    V(16,3), V(16,16), 0x108         CLRA ACC ; red
         vsub     VX(16,3), V(48,2)+r4*, V(16,3)   ; suffer 1 cycle stall, save one add and three VRF accesses
         vmulm    V(48,2)+r4*, VX(16,3), VX(16,2)  ACC

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
endfunc ; }}}
.FORGET


;******************************************************************************
;NAME
;   vc_deflicker_rgba565
;
;SYNOPSIS
;   void vc_deflicker_rgba565(unsigned char *data, int pitch, unsigned char *context);
;
;FUNCTION
;   Deflicker a 16x16 RGBA block using a simple low pass filter
;   [1/4 1/2 1/4] vertically. Uses context as a two line buffer between calls.
;
;RETURNS
;   -
;******************************************************************************

function vc_deflicker_rgba565 ; {{{
   stm r6-r9,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9}
   add r3, r0, r1
   mov r4, 32
alignedloop:
   vld   HX(2++, 0), (r0+=r1) REP 16
   vld   HX(0++,0), (r2+=r1) REP 2
   vst   HX(16++, 0), (r2+=r1) REP 2

   mov r5, 0
   mov r6, 64*18
   mov r7, 64
1:
   vand HX(16, 32), HX(0, 0)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(16, 32), RGBA565_TRANSPARENTKEY SETF
   vmov H(32, 48)+r5, 0x100>>3                  ; alpha >> 3
   vand H(32, 48)+r5, H(0, 0)+r5, RGBA565_ALPHABITS IFZ

   vand HX(16, 32), HX(0, 0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
   vmul HX(0, 0)+r5, HX(16, 32), 10 IFZ

   vand H(32, 0)+r5,  HX(0, 0)+r5,  0x1f    ; blue
   vlsr H(32, 16)+r5, HX(0, 0)+r5,  5       ;
   vand H(32, 16)+r5, H(32, 16)+r5, 0x3f    ; green
   vlsr H(32, 32)+r5, HX(0, 0)+r5,  11      ; red

   addcmpblt r5, r7, r6, 1b
; blue: H(32++,0), green: H(32++, 16), red: H(32++, 32), alpha: H(32++, 48)

; apply deflicker filter: blue, green, red, alpha
.if 1
   mov r5, 4
   cbclr
1:
   vadd  HX(0++, 32), H(32++,  0)*, H(33++,  0)* REP 16
   vadd  HX(0++, 32), HX(0++, 32),  H(33++,  0)* REP 16
   vadd  HX(0++, 32), HX(0++, 32),  H(34++,  0)* REP 16
   vlsr  H(32++,  0)*, HX(0++, 32), 2 REP 16
   cbadd1
   addcmpbgt r5, -1, 0, 1b
.endif   
   mov r5, 0
1:
   ; combine RGB
   vmov -, H(32, 0)+r5                 CLRA ACC ; blue
   vshl -, H(32, 16)+r5, 5             ACC      ; green
   vshl HX(0, 32)+r5, H(32, 32)+r5, 11 ACC      ; red

   ; protect
   vand HX(20, 32), HX(0, 32)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(20, 32), RGBA565_TRANSPARENTKEY       SETF
   veor HX(0, 32)+r5, HX(0, 32)+r5, 0x20            IFZ

   ; replace alpha
   vsub     -, H(32, 48)+r5, (0xF0+0x10)>>3                SETF
   vand     -, H(32, 48)+r5, 0xE0>>3                       IFN CLRA ACC
   vlsr     HX(20, 32), HX(0, 32)+r5, 3                  IFN
   vand     HX(20, 32), HX(20, 32), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
   vor      HX(0, 32)+r5, HX(20, 32), RGBA565_TRANSPARENTKEY IFN
   addcmpblt r5, r7, r6, 1b

   vst   HX(0++, 32), (r0+=r1) REP 16
   add r2, r4
   addcmpblt r0, r4, r3, alignedloop
   ldm      r6-r9,pc, (sp++)
   .cfa_pop {%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}


