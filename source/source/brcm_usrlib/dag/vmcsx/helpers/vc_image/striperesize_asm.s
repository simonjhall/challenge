;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

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


;; calculate V() offsets:
.macro get_horiz_offset16 rd, ra, shift = 0
.if _VC_VERSION > 2
            lsr      rd, ra, 16 - shift
            and      rd, 63 & (63 << shift)
.else
            lsr      rd, ra, 16
.if shift > 0
            shl      rd, shift
.endif
.endif
.endm

;; calculate H() offsets:
.macro get_vert_offset16 rd, ra, shift = 0
.if _VC_VERSION > 2
            lsr      rd, ra, 10 - shift
            and      rd, 0xFC0 & (0xFC0 << shift)
.else
            lsr      rd, ra, 16
            shl      rd, 6 + shift
.endif
.endm

.macro VST_REP_SAFE vrfaddr, etc
.ifdef __BCM2707B0__
.pragma offwarn
            vadd     vrfaddr, vrfaddr, 0  REP 1
.pragma popwarn
.endif   
            vst      vrfaddr, etc
.ifdef __BCM2707B0__
.pragma offwarn
            vadd     vrfaddr, vrfaddr, 0  REP 1
.pragma popwarn
.endif   
.endm


;; YUV or anything byte oriented {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_bytes
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_bytes(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes

function vc_stripe_h_reduce_bytes ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      H(0++,0),(r2+=r3)             REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            mov      r6, 0
.if _VC_VERSION > 2
            bic r0, 15
.endif
            vld      H(0++,16)+r0,(r2+16+=r3)      REP 16
            vld      H(0++,32)+r0,(r2+32+=r3)      REP 16

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 16

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 4

            VST_REP_SAFE <H(48++,0)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_bytes2
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_bytes2(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes by a factor of 2.0 to 4.0

function vc_stripe_h_reduce_bytes2 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vldleft  HX(48++,32),(r2+=r3)          REP 16
;            vldright HX(48++,32),(r2+=r3)          REP 16
            vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
            vlsr     H(0++,0), HX(48++,32), 1      REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            mov      r6, 0
.if _VC_VERSION > 2
            bic      r0, 15
.endif
            vldleft  HX(48++,32),(r2+32+=r3)       REP 16
;            vldright HX(48++,32),(r2+32+=r3)       REP 16
            vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
            vlsr     H(0++,16)+r0, HX(48++,32), 1  REP 16
            vldleft  HX(48++,32),(r2+64+=r3)       REP 16
;            vldright HX(48++,32),(r2+64+=r3)       REP 16
            vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
            vlsr     H(0++,32)+r0, HX(48++,32), 1  REP 16

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
            lsr      r0, r4, 16
            lsr      r1, r5, 16
.endif

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 16

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 5

            VST_REP_SAFE <H(48++,0)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_expand_bytes
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_expand_bytes(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step,       r5
;;;      int                  s_width);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes
;;;
;;; TODO
;;;   Finish this

function vc_stripe_h_expand_bytes ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            add      r10, r2, 16
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)
            mov      r14, -16

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4
            vld      H(0++,0),(r2+=r3)             REP 16
            vmov     V(0,63), V(0,0)
            addcmpbhs r15, 0, 16, .oloop
            mov      r0, r15
            vmov     V(0,0)+r0, V(0,63)+r0


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6
            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16
            bic      r0, 15
            mov      r2, r10
            mov      r3, r11
            add      r10, 16
            vld      H(0++,16)+r0,(r2+=r3)         REP 16
            addcmpbhs r15, r14, 16, .noload
            add      r0, r15
            vmov     V(0,16)+r0, V(0,15)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            add      r8, 16

            VST_REP_SAFE <H(48++,0)>, <(r0+=r1) REP 16>

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_bytes
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_bytes(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,       r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of bytes

function vc_stripe_v_reduce_bytes ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      V(0,0++),(r2+=r3)             REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            bic      r0, 15
            mov      r6, 0
            addscale r2, r3 << 4
            vld      V(0,16++)+r0,(r2+=r3)         REP 32

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            VST_REP_SAFE <V(48,0++)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_bytes2
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_bytes2(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,       r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of bytes by a factor of 2.0 to 4.0

function vc_stripe_v_reduce_bytes2 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      V(32,0++), (r2+=r3)          REP 32
            vadd     VX(48,32), V(32, 0), V(32, 1)
            vadd     VX(48,33), V(32, 2), V(32, 3)
            vadd     VX(48,34), V(32, 4), V(32, 5)
            vadd     VX(48,35), V(32, 6), V(32, 7)
            vadd     VX(48,36), V(32, 8), V(32, 9)
            vadd     VX(48,37), V(32,10), V(32,11)
            vadd     VX(48,38), V(32,12), V(32,13)
            vadd     VX(48,39), V(32,14), V(32,15)
            vadd     VX(48,40), V(32,16), V(32,17)
            vadd     VX(48,41), V(32,18), V(32,19)
            vadd     VX(48,42), V(32,20), V(32,21)
            vadd     VX(48,43), V(32,22), V(32,23)
            vadd     VX(48,44), V(32,24), V(32,25)
            vadd     VX(48,45), V(32,26), V(32,27)
            vadd     VX(48,46), V(32,28), V(32,29)
            vadd     VX(48,47), V(32,30), V(32,31)
            vlsr     V(0,0++), VX(48,32++), 1      REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            bic      r0, 15
            mov      r6, 0
            addscale r2, r3 << 5
            vld      V(32,0++), (r2+=r3)           REP 64
            vadd     VX(48,32), V(32, 0), V(32, 1)
            vadd     VX(48,33), V(32, 2), V(32, 3)
            vadd     VX(48,34), V(32, 4), V(32, 5)
            vadd     VX(48,35), V(32, 6), V(32, 7)
            vadd     VX(48,36), V(32, 8), V(32, 9)
            vadd     VX(48,37), V(32,10), V(32,11)
            vadd     VX(48,38), V(32,12), V(32,13)
            vadd     VX(48,39), V(32,14), V(32,15)
            vadd     VX(48,40), V(32,16), V(32,17)
            vadd     VX(48,41), V(32,18), V(32,19)
            vadd     VX(48,42), V(32,20), V(32,21)
            vadd     VX(48,43), V(32,22), V(32,23)
            vadd     VX(48,44), V(32,24), V(32,25)
            vadd     VX(48,45), V(32,26), V(32,27)
            vadd     VX(48,46), V(32,28), V(32,29)
            vadd     VX(48,47), V(32,30), V(32,31)
            vlsr     V(0,16++)+r0, VX(48,32++), 1  REP 16
            vadd     VX(48,32), V(32,32), V(32,33)
            vadd     VX(48,33), V(32,34), V(32,35)
            vadd     VX(48,34), V(32,36), V(32,37)
            vadd     VX(48,35), V(32,38), V(32,39)
            vadd     VX(48,36), V(32,40), V(32,41)
            vadd     VX(48,37), V(32,42), V(32,43)
            vadd     VX(48,38), V(32,44), V(32,45)
            vadd     VX(48,39), V(32,46), V(32,47)
            vadd     VX(48,40), V(32,48), V(32,49)
            vadd     VX(48,41), V(32,50), V(32,51)
            vadd     VX(48,42), V(32,52), V(32,53)
            vadd     VX(48,43), V(32,54), V(32,55)
            vadd     VX(48,44), V(32,56), V(32,57)
            vadd     VX(48,45), V(32,58), V(32,59)
            vadd     VX(48,46), V(32,60), V(32,61)
            vadd     VX(48,47), V(32,62), V(32,63)
            vlsr     V(0,32++)+r0, VX(48,32++), 1  REP 16

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 5

            vst      V(48,0++), (r0+=r1)           REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_expand_bytes
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_expand_bytes(
;;;      unsigned char       *dest,       /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned char const *src,        /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  rows,       /* r4 */
;;;      int                  step,       /* r5 */
;;;      int                  s_height);  /* (sp) -> (sp+40) */
;;;
;;; FUNCTION
;;;   Expand a vertical stripe of bytes
;;;
;;; TODO
;;;   Finish this

function vc_stripe_v_expand_bytes ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            addscale r10, r2, r3 << 4
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)
            mov      r14, -16

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4

            vld      V(0,0++),(r2+=r3)             REP 16
            vmov     V(0,63), V(0,0)
            addcmpbhs r15, 0, 16, .oloop
            mov      r0, r15
            vmov     V(0,0)+r0, V(0,63)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif
            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6
            
            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16
            bic      r0, 15
            mov      r2, r10
            mov      r3, r11
            addscale r10, r11 << 4
            vld      V(0,16++)+r0, (r2+=r3)        REP 16
            addcmpbhs r15, r14, 16, .noload
            add      r0, r15
            vmov     V(0,16)+r0, V(0,15)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4
            vst      V(48,0++), (r0+=r1)           REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_uv
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_uv(
;;;      unsigned char       *dest,       r0
;;;      unsigned char       *dest2,      r1
;;;      int                  dpitch,     r2
;;;      unsigned char const *src,        r3
;;;      unsigned char const *src2,       r4
;;;      int                  spitch,     r5
;;;      int                  rows        (sp)
;;;      int                  step);      (sp+4)
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of U and V data

function vc_stripe_v_reduce_uv ; {{{
            stm      r6-r15,lr, (--sp)
            .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            vbitplanes -, 0xff SETF
            ld       r7, (sp+44+4) ; step
            mov      r8, r0 ; dest
            mov      r9, r2 ; dpitch
            mov      r10, r3 ; src
            mov      r11, r5 ; spitch
            ld       r12, (sp+44+0) ; columns
            add      r12, 7
            lsr      r12, 3
            mov      r13, 0
            sub      r15, r4, r3
            sub      lr, r1, r0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r2, r10
            mov      r3, r11

            vldleft  V(0,0++),(r2+=r3)             REP 16
            add      r2, r15
            vldleft  V(16,0++),(r2+=r3)            REP 16
            vmov     H(8++,0), H(16++,0)           REP 8
            sub      r2, r15


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            bic      r0, 15
            mov      r6, 0
            addscale r2, r3 << 4
            vldleft  V(0,16++)+r0,(r2+=r3)         REP 32
            add      r2, r15
            vldleft  V(16,0++),(r2+=r3)            REP 32
            vmov     H(8++,16)+r0, H(16++,0)       REP 8
            vmov     H(8++,32)+r0, H(16++,16)      REP 8

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            addcmpbne r12, -1, 0, .fullstore
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            b        .end
            .align 32
.fullstore: vstleft  V(48,0++), (r0+=r1)           IFN REP 16
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15,pc, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_uv2
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_uv2(
;;;      unsigned char       *dest,       r0
;;;      unsigned char       *dest2,      r1
;;;      int                  dpitch,     r2
;;;      unsigned char const *src,        r3
;;;      unsigned char const *src2,       r4
;;;      int                  spitch,     r5
;;;      int                  rows,       (sp)
;;;      int                  step);      (sp+4)
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of U and V data by a factor of 2.0 to 4.0

function vc_stripe_v_reduce_uv2 ; {{{
            stm      r6-r15,lr, (--sp)
            .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            vbitplanes -, 0xff SETF
            ld       r7, (sp+44+4) ; step
            lsr      r7, 1
            mov      r8, r0 ; dest
            mov      r9, r2 ; dpitch
            mov      r10, r3 ; src
            mov      r11, r5 ; spitch
            ld       r12, (sp+44+0) ; columns
            add      r12, 7
            lsr      r12, 3
            mov      r13, 0
            sub      r15, r4, r3
            sub      lr, r1, r0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r2, r10
            mov      r3, r11

            vldleft  V(32,0++), (r2+=r3)           REP 32
            add      r2, r15
            vldleft  V(16,0++), (r2+=r3)           REP 32
            vmov     HX(40++,0), HX(16++,0)        REP 8
            vadd     VX(48,32), V(32, 0), V(32, 1)
            vadd     VX(48,33), V(32, 2), V(32, 3)
            vadd     VX(48,34), V(32, 4), V(32, 5)
            vadd     VX(48,35), V(32, 6), V(32, 7)
            vadd     VX(48,36), V(32, 8), V(32, 9)
            vadd     VX(48,37), V(32,10), V(32,11)
            vadd     VX(48,38), V(32,12), V(32,13)
            vadd     VX(48,39), V(32,14), V(32,15)
            vadd     VX(48,40), V(32,16), V(32,17)
            vadd     VX(48,41), V(32,18), V(32,19)
            vadd     VX(48,42), V(32,20), V(32,21)
            vadd     VX(48,43), V(32,22), V(32,23)
            vadd     VX(48,44), V(32,24), V(32,25)
            vadd     VX(48,45), V(32,26), V(32,27)
            vadd     VX(48,46), V(32,28), V(32,29)
            vadd     VX(48,47), V(32,30), V(32,31)
            vlsr     V(0,0++), VX(48,32++), 1      REP 16
            sub      r2, r15

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            bic      r0, 15
            mov      r6, 0
            addscale r2, r3 << 5
            vldleft  V(32,0++), (r2+=r3)           REP 64
            add      r2, r15
            vldleft  V(16,0++), (r2+=r3)           REP 64
            vmov     HX(40++,0), HX(16++,0)        REP 8
            vmov     HX(40++,32), HX(16++,32)      REP 8
            vadd     VX(48,32), V(32, 0), V(32, 1)
            vadd     VX(48,33), V(32, 2), V(32, 3)
            vadd     VX(48,34), V(32, 4), V(32, 5)
            vadd     VX(48,35), V(32, 6), V(32, 7)
            vadd     VX(48,36), V(32, 8), V(32, 9)
            vadd     VX(48,37), V(32,10), V(32,11)
            vadd     VX(48,38), V(32,12), V(32,13)
            vadd     VX(48,39), V(32,14), V(32,15)
            vadd     VX(48,40), V(32,16), V(32,17)
            vadd     VX(48,41), V(32,18), V(32,19)
            vadd     VX(48,42), V(32,20), V(32,21)
            vadd     VX(48,43), V(32,22), V(32,23)
            vadd     VX(48,44), V(32,24), V(32,25)
            vadd     VX(48,45), V(32,26), V(32,27)
            vadd     VX(48,46), V(32,28), V(32,29)
            vadd     VX(48,47), V(32,30), V(32,31)
            vlsr     V(0,16++)+r0, VX(48,32++), 1  REP 16
            vadd     VX(48,32), V(32,32), V(32,33)
            vadd     VX(48,33), V(32,34), V(32,35)
            vadd     VX(48,34), V(32,36), V(32,37)
            vadd     VX(48,35), V(32,38), V(32,39)
            vadd     VX(48,36), V(32,40), V(32,41)
            vadd     VX(48,37), V(32,42), V(32,43)
            vadd     VX(48,38), V(32,44), V(32,45)
            vadd     VX(48,39), V(32,46), V(32,47)
            vadd     VX(48,40), V(32,48), V(32,49)
            vadd     VX(48,41), V(32,50), V(32,51)
            vadd     VX(48,42), V(32,52), V(32,53)
            vadd     VX(48,43), V(32,54), V(32,55)
            vadd     VX(48,44), V(32,56), V(32,57)
            vadd     VX(48,45), V(32,58), V(32,59)
            vadd     VX(48,46), V(32,60), V(32,61)
            vadd     VX(48,47), V(32,62), V(32,63)
            vlsr     V(0,32++)+r0, VX(48,32++), 1  REP 16

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

      vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 5

            addcmpbne r12, -1, 0, .fullstore
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            b        .end
            .align 32
.fullstore: vstleft  V(48,0++), (r0+=r1)           IFN REP 16
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15,pc, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_expand_uv
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_expand_uv(
;;;      unsigned char       *dest,       r0
;;;      unsigned char       *dest2,      r1
;;;      int                  dpitch,     r2
;;;      unsigned char const *src,        r3
;;;      unsigned char const *src2,       r4
;;;      int                  spitch,     r5
;;;      int                  rows        (sp)   -> (sp+44)
;;;      int                  step,       (sp+4) -> (sp+48)
;;;      int                  s_height);  (sp+8) -> (sp+52)
;;;
;;; FUNCTION
;;;   Expand a vertical stripe of u and V data

function vc_stripe_v_expand_uv ; {{{
            stm      r6-r15,lr, (--sp)
            .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            vbitplanes -, 0xff SETF
            ld       r7, (sp+44+4) ; step
            mov      r8, r0 ; dest
            mov      r9, r2 ; dpitch
            mov      r10, r3 ; src
            mov      r11, r5 ; spitch
            ld       r12, (sp+44+0) ; columns
            ld       r15, (sp+44+8) ; s_height

            add      r12, 7
            lsr      r12, 3
            sub      r14, r4, r3
            sub      lr, r1, r0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4

            mov      r2, r10
            mov      r3, r11
            addscale r10, r3 << 4

            vldleft  V(0,0++),(r2+=r3)             REP 16
            add      r2, r14
            vldleft  V(16,0++),(r2+=r3)            REP 16
            vmov     H(8++,0), H(16++,0)           REP 8
            vmov     V(0,63), V(0,0)
            addcmpbhs r15, 0, 16, .oloop
            mov      r0, r15
            vmov     V(0,0)+r0, V(0,63)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
      bmask    r2, 16+6
      bmask    r3, 16+6
      bmask    r4, 16+6
      bmask    r5, 16+6
      lsr      r0, r4, 16
      lsr      r1, r5, 16
.endif

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r4
            vadds    V(48,0)+r6, VX(16,0), V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r5
            vadds    V(48,1)+r6, VX(16,0), V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vsubs    VX(16,0), V(0,1)+r0, V(0,0)+r0
            vmulhn.su VX(16,0), VX(16,0), r2
            vadds    V(48,2)+r6, VX(16,0), V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vsubs    VX(16,0), V(0,1)+r1, V(0,0)+r1
            vmulhn.su VX(16,0), VX(16,0), r3
            vadds    V(48,3)+r6, VX(16,0), V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6

            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16
            bic      r0, 15
            mov      r2, r10
            mov      r3, r11
            addscale r10, r11 << 4
            vldleft  V(0,16++)+r0,(r2+=r3)         REP 16
            add      r2, r14
            sub      r15, 16
            vldleft  V(16,0++),(r2+=r3)            REP 16
            vmov     H(8++,16)+r0, H(16++,0)       REP 8
            addcmpbhs r15, 0, 16, .noload
            add      r0, r15
            vmov     V(0,16)+r0, V(0,15)+r0
.noload:    mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4
            addcmpbne r12, -1, 0, .fullstore
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 8
            b        .end
            .align 32
.fullstore: vstleft  V(48,0++), (r0+=r1)           IFN REP 16
            add      r0, lr
            vmov     H(48++,0), H(56++,0)          REP 8
            vstleft  V(48,0++), (r0+=r1)           IFN REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15,pc, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}
.FORGET

;; }}}

;; RGBA32 {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba32
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba32(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes

function vc_stripe_h_reduce_rgba32 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(48++,32), (r2+=r3)         REP 16
            vmov     H(0++,0), H(48++,32)          REP 16
            vmov     H(16++,0), H(48++,48)         REP 16


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4-1
            mov      r6, 0
            shl      r0, 4

            vld      HX(48++,32), (r2+32+=r3)      REP 16
            vmov     H(0++,16)+r0, H(48++,32)      REP 16
            vmov     H(16++,16)+r0, H(48++,48)     REP 16

            vld      HX(48++,32), (r2+64+=r3)      REP 16
            vmov     H(0++,32)+r0, H(48++,32)      REP 16
            vmov     H(16++,32)+r0, H(48++,48)     REP 16

            get_horiz_offset16 r0, r4, 1
            get_horiz_offset16 r1, r5, 1
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif

            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,0++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,16++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r4, r7 << 1
            get_horiz_offset16 r0, r2, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,2++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,18++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r5, r7 << 1
            get_horiz_offset16 r1, r3, 1

            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,4++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,20++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r2, r7 << 1
            get_horiz_offset16 r0, r4, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,6++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,22++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r3, r7 << 1
            get_horiz_offset16 r1, r5, 1

            add r6, 1
            addcmpbne r6, 7, 16, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 5

            VST_REP_SAFE <HX(48++,0)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba322
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba322(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes by a factor of 2.0 to 4.0

function vc_stripe_h_reduce_rgba322 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(48++,0), (r2+=r3)          REP 16
            vld      HX(48++,32), (r2+32+=r3)      REP 16
            vadd     VX(32,0++), V(48,0++), V(48,2++)     REP 2
            vadd     VX(32,2++), V(48,4++), V(48,6++)     REP 2
            vadd     VX(32,4++), V(48,8++), V(48,10++)    REP 2
            vadd     VX(32,6++), V(48,12++), V(48,14++)   REP 2
            vadd     VX(32,8++), V(48,32++), V(48,34++)   REP 2
            vadd     VX(32,10++), V(48,36++), V(48,38++)  REP 2
            vadd     VX(32,12++), V(48,40++), V(48,42++)  REP 2
            vadd     VX(32,14++), V(48,44++), V(48,46++)  REP 2
            vlsr     H(0++,0), HX(32++,0), 1       REP 16

            vadd     VX(32,0++), V(48,16++), V(48,18++)   REP 2
            vadd     VX(32,2++), V(48,20++), V(48,22++)   REP 2
            vadd     VX(32,4++), V(48,24++), V(48,26++)   REP 2
            vadd     VX(32,6++), V(48,28++), V(48,30++)   REP 2
            vadd     VX(32,8++), V(48,48++), V(48,50++)   REP 2
            vadd     VX(32,10++), V(48,52++), V(48,54++)  REP 2
            vadd     VX(32,12++), V(48,56++), V(48,58++)  REP 2
            vadd     VX(32,14++), V(48,60++), V(48,62++)  REP 2
            vlsr     H(16++,0), HX(32++,0), 1      REP 16


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4-1
            mov      r6, 0
            shl      r0, 4

            vld      HX(48++,0), (r2+64+=r3)       REP 16
            vld      HX(48++,32), (r2+96+=r3)      REP 16
            vadd     VX(32,0++), V(48,0++), V(48,2++)     REP 2
            vadd     VX(32,2++), V(48,4++), V(48,6++)     REP 2
            vadd     VX(32,4++), V(48,8++), V(48,10++)    REP 2
            vadd     VX(32,6++), V(48,12++), V(48,14++)   REP 2
            vadd     VX(32,8++), V(48,32++), V(48,34++)   REP 2
            vadd     VX(32,10++), V(48,36++), V(48,38++)  REP 2
            vadd     VX(32,12++), V(48,40++), V(48,42++)  REP 2
            vadd     VX(32,14++), V(48,44++), V(48,46++)  REP 2
            vlsr     H(0++,16)+r0, HX(32++,0), 1   REP 16
            vadd     VX(32,0++), V(48,16++), V(48,18++)   REP 2
            vadd     VX(32,2++), V(48,20++), V(48,22++)   REP 2
            vadd     VX(32,4++), V(48,24++), V(48,26++)   REP 2
            vadd     VX(32,6++), V(48,28++), V(48,30++)   REP 2
            vadd     VX(32,8++), V(48,48++), V(48,50++)   REP 2
            vadd     VX(32,10++), V(48,52++), V(48,54++)  REP 2
            vadd     VX(32,12++), V(48,56++), V(48,58++)  REP 2
            vadd     VX(32,14++), V(48,60++), V(48,62++)  REP 2
            vlsr     H(16++,16)+r0, HX(32++,0), 1  REP 16

            vld      HX(48++,0), (r2+128+=r3)      REP 16
            vld      HX(48++,32), (r2+160+=r3)     REP 16
            vadd     VX(32,0++), V(48,0++), V(48,2++)     REP 2
            vadd     VX(32,2++), V(48,4++), V(48,6++)     REP 2
            vadd     VX(32,4++), V(48,8++), V(48,10++)    REP 2
            vadd     VX(32,6++), V(48,12++), V(48,14++)   REP 2
            vadd     VX(32,8++), V(48,32++), V(48,34++)   REP 2
            vadd     VX(32,10++), V(48,36++), V(48,38++)  REP 2
            vadd     VX(32,12++), V(48,40++), V(48,42++)  REP 2
            vadd     VX(32,14++), V(48,44++), V(48,46++)  REP 2
            vlsr     H(0++,32)+r0, HX(32++,0), 1   REP 16
            vadd     VX(32,0++), V(48,16++), V(48,18++)   REP 2
            vadd     VX(32,2++), V(48,20++), V(48,22++)   REP 2
            vadd     VX(32,4++), V(48,24++), V(48,26++)   REP 2
            vadd     VX(32,6++), V(48,28++), V(48,30++)   REP 2
            vadd     VX(32,8++), V(48,48++), V(48,50++)   REP 2
            vadd     VX(32,10++), V(48,52++), V(48,54++)  REP 2
            vadd     VX(32,12++), V(48,56++), V(48,58++)  REP 2
            vadd     VX(32,14++), V(48,60++), V(48,62++)  REP 2
            vlsr     H(16++,32)+r0, HX(32++,0), 1  REP 16

            get_horiz_offset16 r0, r4, 1
            get_horiz_offset16 r1, r5, 1
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:
.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif
            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,0++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,16++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r4, r7 << 1
            get_horiz_offset16 r0, r2, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,2++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,18++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r5, r7 << 1
            get_horiz_offset16 r1, r3, 1

            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,4++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,20++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r2, r7 << 1
            get_horiz_offset16 r0, r4, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,6++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,22++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r3, r7 << 1
            get_horiz_offset16 r1, r5, 1

            add r6, 1
            addcmpbne r6, 7, 16, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 6

            VST_REP_SAFE <HX(48++,0)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_expand_rgba32
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_expand_rgba32(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step,       r5
;;;      int                  s_width);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a horizontal stripe of bytes

function vc_stripe_h_expand_rgba32 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            add      r10, r2, 32
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3

            mov      r14, -8
            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6-1
            add      r5, r4, r7
            shl      r7, 1
            mov      r13, r4

            vld      HX(48++,32), (r2+=r3)         REP 16
            vmov     H(0++,0), H(48++,32)          REP 16
            vmov     H(16++,0), H(48++,48)         REP 16
            vmov     V(0,62++), V(0,0++)           REP 2
            vmov     V(16,62++), V(16,0++)         REP 2
            addcmpbhs r15, 0, 8, .oloop
            shl      r0, r15, 1
            vmov     V(0,0++)+r0, V(0,62++)+r0     REP 2
            vmov     V(16,0++)+r0, V(16,62++)+r0   REP 2

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_horiz_offset16 r0, r4, 1
            get_horiz_offset16 r1, r5, 1
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:

.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif
            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,0++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(48,16++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r4, r7 << 1
            get_horiz_offset16 r0, r2, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,2++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(48,18++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r5, r7 << 1
            get_horiz_offset16 r1, r3, 1

            vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,4++)+r6, VX(32,0++), V(0,0++)+r0     REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r2               REP 2
            vadds    V(48,20++)+r6, VX(32,0++), V(16,0++)+r0   REP 2
            addscale r2, r7 << 1
            get_horiz_offset16 r0, r4, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,6++)+r6, VX(32,0++), V(0,0++)+r1     REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r3               REP 2
            vadds    V(48,22++)+r6, VX(32,0++), V(16,0++)+r1   REP 2
            addscale r3, r7 << 1
            get_horiz_offset16 r1, r5, 1

            add r6, 1
            addcmpbne r6, 7, 16, .loop

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1

            eor      r13, r4
            btest    r13, 16+3
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16
            bic      r0, 7
            shl      r0, 1
            mov      r2, r10
            mov      r3, r11
            add      r10, 32
            vld      HX(48++,32), (r2+=r3)         REP 16
            vmov     H(0++,16)+r0, H(48++,32)      REP 16
            vmov     H(16++,16)+r0, H(48++,48)     REP 16
            addcmpbhs r15, r14, 8, .noload
            addscale r0, r15 << 1
            vmov     V(0,16++)+r0, V(0,14++)+r0    REP 2
            vmov     V(16,16++)+r0, V(16,14++)+r0  REP 2

.noload:    mov      r0, r8
            mov      r1, r9
            add      r8, 32
        
            VST_REP_SAFE <HX(48++,0)>, <(r0+=r1) REP 16>

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;; }}}

;; RGB565 {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgb565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgb565(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of rgb565

function vc_stripe_h_reduce_rgb565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(0,0++),(r2+=r3)            REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
.if _VC_VERSION > 2
            bic r0, 15
.endif
            shl      r0, 6
            mov      r6, 64*48
            mov      r15, 64*64
            vld      VX(16,0++)+r0,(r2+32+=r3)     REP 16
            vld      VX(32,0++)+r0,(r2+64+=r3)     REP 16

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7
         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      
.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif
            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 5

            VST_REP_SAFE <VX(48,32++)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgb5652
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgb5652(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of rgb565 by a factor of 2.0 to 4.0

function vc_stripe_h_reduce_rgb5652 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(0,32++),(r2+=r3)           REP 16
            vld      VX(16,32++), (r2+32+=r3)      REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(0,0),  HX(0,32), HX(1,32)
            vadd     HX(1,0),  HX(2,32), HX(3,32)
            vadd     HX(2,0),  HX(4,32), HX(5,32)
            vadd     HX(3,0),  HX(6,32), HX(7,32)
            vadd     HX(4,0),  HX(8,32), HX(9,32)
            vadd     HX(5,0),  HX(10,32), HX(11,32)
            vadd     HX(6,0),  HX(12,32), HX(13,32)
            vadd     HX(7,0),  HX(14,32), HX(15,32)
            vadd     HX(8,0),  HX(16,32), HX(17,32)
            vadd     HX(9,0),  HX(18,32), HX(19,32)
            vadd     HX(10,0), HX(20,32), HX(21,32)
            vadd     HX(11,0), HX(22,32), HX(23,32)
            vadd     HX(12,0), HX(24,32), HX(25,32)
            vadd     HX(13,0), HX(26,32), HX(27,32)
            vadd     HX(14,0), HX(28,32), HX(29,32)
            vadd     HX(15,0), HX(30,32), HX(31,32)

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64

            vld      VX(0,32++),(r2+64+=r3)        REP 16
            vld      VX(16,32++), (r2+96+=r3)      REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(16+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(16+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(16+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(16+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(16+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(16+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(16+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(16+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(16+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(16+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(16+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(16+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(16+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(16+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(16+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(16+15,0)+r0, HX(30,32), HX(31,32)

            vld      VX(0,32++),(r2+128+=r3)       REP 16
            vld      VX(16,32++), (r2+160+=r3)     REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(32+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(32+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(32+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(32+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(32+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(32+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(32+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(32+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(32+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(32+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(32+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(32+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(32+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(32+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(32+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(32+15,0)+r0, HX(30,32), HX(31,32)

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:
.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif
            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 6

            VST_REP_SAFE <VX(48,32++)>, <(r0+=r1) REP 16>

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_expand_rgb565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_expand_rgb565(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step,       r5
;;;      int                  s_width);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a horizontal stripe of rgb565

function vc_stripe_h_expand_rgb565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4
            vld      VX(0,0++),(r2+=r3)            REP 16
            vmov     HX(63,0), HX(0,0)
            addcmpbhs r15, 0, 16, .oloop
            shl      r0, r15, 6
            vmov     HX(0,0)+r0, HX(63,0)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

            mov      r6, 64*48
            mov      r14, 64*64

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      
.if _VC_VERSION > 2
            bmask    r2, 16+6
            bmask    r3, 16+6
            bmask    r4, 16+6
            bmask    r5, 16+6
.endif
            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r14, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6

            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload
            
            mov      r14, -16
            lsr      r0, r4, 16+4
            shl      r0, 6+4

            mov      r2, r10
            mov      r3, r11
            add      r10, 32
            vld      VX(16,0++)+r0,(r2+32+=r3)     REP 16
            addcmpbhs r15, r14, 16, .noload
            addscale r0, r15 << 6
            vmov     HX(16,0)+r0, HX(15,0)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            add      r8, 32

            VST_REP_SAFE <VX(48,32++)>, <(r0+=r1) REP 16>

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgb565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgb565(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,       r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of rgb565

function vc_stripe_v_reduce_rgb565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(0++,0),(r2+=r3)            REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64
            addscale r2, r3 << 4
            vld      HX(16++,0)+r0,(r2+=r3)        REP 32

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            vst      HX(48++,32), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgb5652
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgb5652(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of rgb565 by a factor of 2.0 to 4.0

function vc_stripe_v_reduce_rgb5652 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(0,0),  HX(0,32), HX(1,32)
            vadd     HX(1,0),  HX(2,32), HX(3,32)
            vadd     HX(2,0),  HX(4,32), HX(5,32)
            vadd     HX(3,0),  HX(6,32), HX(7,32)
            vadd     HX(4,0),  HX(8,32), HX(9,32)
            vadd     HX(5,0),  HX(10,32), HX(11,32)
            vadd     HX(6,0),  HX(12,32), HX(13,32)
            vadd     HX(7,0),  HX(14,32), HX(15,32)
            vadd     HX(8,0),  HX(16,32), HX(17,32)
            vadd     HX(9,0),  HX(18,32), HX(19,32)
            vadd     HX(10,0), HX(20,32), HX(21,32)
            vadd     HX(11,0), HX(22,32), HX(23,32)
            vadd     HX(12,0), HX(24,32), HX(25,32)
            vadd     HX(13,0), HX(26,32), HX(27,32)
            vadd     HX(14,0), HX(28,32), HX(29,32)
            vadd     HX(15,0), HX(30,32), HX(31,32)

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64

            addscale r2, r3 << 5
            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(16+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(16+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(16+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(16+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(16+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(16+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(16+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(16+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(16+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(16+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(16+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(16+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(16+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(16+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(16+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(16+15,0)+r0, HX(30,32), HX(31,32)

            addscale r2, r3 << 5
            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7BEF REP 32

            vadd     HX(32+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(32+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(32+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(32+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(32+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(32+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(32+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(32+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(32+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(32+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(32+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(32+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(32+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(32+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(32+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(32+15,0)+r0, HX(30,32), HX(31,32)

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 5

            vst      HX(48++,32), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_expand_rgb565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_expand_rgb565(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step,       r5
;;;      int                  s_height);  (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a vertical stripe of rgb565

function vc_stripe_v_expand_rgb565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4
            vld      HX(0++,0),(r2+=r3)            REP 16
            vmov     HX(63,0), HX(0,0)
            addcmpbhs r15, 0, 16, .oloop
            shl      r0, r15, 6
            vmov     HX(0,0)+r0, HX(63,0)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

            mov      r6, 64*48
            mov      r14, 64*64

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r4         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r4
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(0,32)+r6, HX(3,32), 0xF81F ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r5         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r5
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(1,32)+r6, HX(3,32), 0xF81F ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r2         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r2
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(2,32)+r6, HX(3,32), 0xF81F ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, ~0xF81F & 0xFFFF  REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF81F            REP 2
            vsub     HX(1,32), HX(1,32), HX(0,32)
            vsub     HX(4,32), H(3,32), H(2,32)
            vsub     HX(5,32), H(3,48), H(2,48)
            vmulhn.su HX(4++,32), HX(4++,32), r3         REP 2
            vmulhn.su HX(1  ,32), HX(1  ,32), r3
            vadd     H(3,32), HX(4,32), H(2,32)
            vadd     H(3,48), HX(5,32), H(2,48)
            vadd     HX(1,32), HX(1,32), HX(0,32)
            vand     -, HX(1,32), ~0xF81F & 0xFFFF       CLRA ACC
            vand     HX(3,32)+r6, HX(3,32), 0xF81F ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r14, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6

            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload
            mov      r14, -16
            
            lsr      r0, r4, 16+4
            shl      r0, 6+4
            addscale r10, r11 << 4
            mov      r2, r10
            mov      r3, r11
            vld      HX(16++,0)+r0,(r2+=r3)        REP 16
            addcmpbhs r15, r14, 16, .noload
            addscale r0, r15 << 6
            vmov     HX(16,0)+r0, HX(15,0)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            vst      HX(48++,32), (r0+=r1)         REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;; }}}

;; RGBA565 {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba565(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes

function vc_stripe_h_reduce_rgba565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vldleft  HX(48++,32), (r2+=r3)         REP 16
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 8, .lloop0


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+3
            shl      r0, 1+3

            vldleft  HX(48++,32), (r2+16+=r3)      REP 16
;            vldright HX(48++,32), (r2+16+=r3)      REP 16
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,17)+r0, 0x100>>3
            vand     V(16,17)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,16)+r0, VX(48,32)+r6, 3
            vand     V(16,16)+r0, V(16,16)+r0, 0xFC
            vand     V(0,16)+r0, V(48,32)+r6, 0x1F
            vand     V(0,17)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 16, .lloop1
            mov      r6, 0

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1


            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1
            addcmpbne r6, 2, 8, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 16

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 4

            vstleft  HX(48++,0), (r0+=r1)          REP 16

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba5652
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba5652(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes by a factor of 2.0 to 4.0

function vc_stripe_h_reduce_rgba5652 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(48++,32), (r2+=r3)         REP 16
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8

            vand     VX(32,0), VX(48,33)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     VX(32,5), 0x100>>3
            vand     VX(32,5), V(48,33)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,33)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,33)+r6, VX(32,0), 10                IFZ
            vlsr     VX(32,4), VX(48,33)+r6, 3
            vand     VX(32,4), VX(32,4), 0xFC
            vand     VX(32,2), V(48,33)+r6, 0x1F
            vand     VX(32,3), V(48,49)+r6, 0xF8

            vadd     VX(32,2++), VX(32,2++), V(0,0++)+r0       REP 2
            vadd     VX(32,4++), VX(32,4++), V(16,0++)+r0      REP 2
            vlsr     V(0,0++)+r0, VX(32,2++), 1                REP 2
            vlsr     V(16,0++)+r0, VX(32,4++), 1               REP 2

            add      r0, 2
            addcmpbne r6, 2, 16, .lloop0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+3
            shl      r0, 1+3

            vld      HX(48++,32), (r2+32+=r3)      REP 16
            vld      HX(48++,0), (r2+64+=r3)       REP 16
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,17)+r0, 0x100>>3
            vand     V(16,17)+r0, VX(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,16)+r0, VX(48,32)+r6, 3
            vand     V(16,16)+r0, V(16,16)+r0, 0xFC
            vand     V(0,16)+r0, VX(48,32)+r6, 0x1F
            vlsr     V(0,17)+r0, VX(48,32)+r6, 8 ; because of the index mix-up for H/HX
            vand     V(0,17)+r0, V(0,17)+r0, 0xF8

            vand     VX(32,0), VX(48,33)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     VX(32,5), 0x100>>3
            vand     VX(32,5), VX(48,33)+r6, RGBA565_ALPHABITS  IFZ
            vand     VX(32,0), VX(48,33)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,33)+r6, VX(32,0), 10                IFZ
            vlsr     VX(32,4), VX(48,33)+r6, 3
            vand     VX(32,4), VX(32,4), 0xFC
            vand     VX(32,2), VX(48,33)+r6, 0x1F
            vlsr     VX(32,3), VX(48,33)+r6, 8 ; because of the index mix-up for H/HX
            vand     VX(32,3), V(32,3), 0xF8

            vadd     VX(32,2++), VX(32,2++), V(0,16++)+r0      REP 2
            vadd     VX(32,4++), VX(32,4++), V(16,16++)+r0     REP 2
            vlsr     V(0,16++)+r0, VX(32,2++), 1               REP 2
            vlsr     V(16,16++)+r0, VX(32,4++), 1              REP 2

            add      r0, 2
            addcmpble r6, 2, 32, .lloop1
            mov      r6, 0

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1


            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1
            addcmpbne r6, 2, 8, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 16

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 5

            vstleft  HX(48++,0), (r0+=r1)          REP 16

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_expand_rgba565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_expand_rgba565(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step,       r5
;;;      int                  s_width);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a horizontal stripe of bytes

function vc_stripe_h_expand_rgba565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            add      r10, r2, 32
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3

            mov      r14, -8
            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6-1
            add      r5, r4, r7
            shl      r7, 1
            mov      r13, r4

            vld      HX(48++,32), (r2+=r3)         REP 16
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 16, .lloop0
            vmov     V(0,62++), V(0,0++)           REP 2
            vmov     V(16,62++), V(16,0++)         REP 2
            addcmpbhs r15, 0, 8, .oloop
            shl      r0, r15, 1
            vmov     V(0,0++)+r0, V(0,62++)+r0     REP 2
            vmov     V(16,0++)+r0, V(16,62++)+r0   REP 2

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1

            addcmpbne r6, 2, 8, .loop

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1

            eor      r13, r4
            btest    r13, 16+3
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16+3
            shl      r0, 1+3
            mov      r2, r10
            mov      r3, r11
            add      r10, 16
            vldleft  HX(48++,32), (r2+=r3)         REP 16
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,33)+r0, 0x100>>3
            vand     V(16,33)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,32)+r0, VX(48,32)+r6, 3
            vand     V(16,32)+r0, V(16,32)+r0, 0xFC
            vand     V(0,32)+r0, V(48,32)+r6, 0x1F
            vand     V(0,33)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 16, .lloop1
            addcmpbhs r15, r14, 8, .noload
            sub      r0, 32
            addscale r0, r15 << 1
            vmov     V(0,16++)+r0, V(0,14++)+r0    REP 2
            vmov     V(16,16++)+r0, V(16,14++)+r0  REP 2

.noload:    mov      r0, r8
            mov      r1, r9
            add      r8, 16
            vstleft  HX(48++,0), (r0+=r1)          REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgba565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgba565(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of bytes

function vc_stripe_v_reduce_rgba565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(48,32++), (r2+=r3)         REP 8
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 8, .lloop0


         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+3
            shl      r0, 1+3

            addscale r2, r3 << 3
            vld      VX(48,32++), (r2+=r3)         REP 16
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,17)+r0, 0x100>>3
            vand     V(16,17)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,16)+r0, VX(48,32)+r6, 3
            vand     V(16,16)+r0, V(16,16)+r0, 0xFC
            vand     V(0,16)+r0, V(48,32)+r6, 0x1F
            vand     V(0,17)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 16, .lloop1
            mov      r6, 0

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1


            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1
            addcmpbne r6, 2, 8, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 3

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 3

            vst      VX(48,0++), (r0+=r1)          REP 8

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgba5652
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgba5652(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of bytes by a factor of 2.0 to 4.0

function vc_stripe_v_reduce_rgba5652 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(48,32++), (r2+=r3)         REP 16
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8

            vand     VX(32,0), VX(48,33)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     VX(32,5), 0x100>>3
            vand     VX(32,5), V(48,33)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,33)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,33)+r6, VX(32,0), 10                IFZ
            vlsr     VX(32,4), VX(48,33)+r6, 3
            vand     VX(32,4), VX(32,4), 0xFC
            vand     VX(32,2), V(48,33)+r6, 0x1F
            vand     VX(32,3), V(48,49)+r6, 0xF8

            vadd     VX(32,2++), VX(32,2++), V(0,0++)+r0       REP 2
            vadd     VX(32,4++), VX(32,4++), V(16,0++)+r0      REP 2
            vlsr     V(0,0++)+r0, VX(32,2++), 1                REP 2
            vlsr     V(16,0++)+r0, VX(32,4++), 1               REP 2

            add      r0, 2
            addcmpbne r6, 2, 16, .lloop0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+3
            shl      r0, 1+3

            addscale r2, r3 << 4
            vld      VX(48,32++), (r2+=r3)         REP 32
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,17)+r0, 0x100>>3
            vand     V(16,17)+r0, VX(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,16)+r0, VX(48,32)+r6, 3
            vand     V(16,16)+r0, V(16,16)+r0, 0xFC
            vand     V(0,16)+r0, VX(48,32)+r6, 0x1F
            vlsr     V(0,17)+r0, VX(48,32)+r6, 8 ; because of the index mix-up for H/HX
            vand     V(0,17)+r0, V(0,17)+r0, 0xF8

            vand     VX(32,0), VX(48,33)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     VX(32,5), 0x100>>3
            vand     VX(32,5), VX(48,33)+r6, RGBA565_ALPHABITS  IFZ
            vand     VX(32,0), VX(48,33)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,33)+r6, VX(32,0), 10                IFZ
            vlsr     VX(32,4), VX(48,33)+r6, 3
            vand     VX(32,4), VX(32,4), 0xFC
            vand     VX(32,2), VX(48,33)+r6, 0x1F
            vlsr     VX(32,3), VX(48,33)+r6, 8 ; because of the index mix-up for H/HX
            vand     VX(32,3), VX(32,3), 0xF8

            vadd     VX(32,2++), VX(32,2++), V(0,16++)+r0      REP 2
            vadd     VX(32,4++), VX(32,4++), V(16,16++)+r0     REP 2
            vlsr     V(0,16++)+r0, VX(32,2++), 1               REP 2
            vlsr     V(16,16++)+r0, VX(32,4++), 1              REP 2

            add      r0, 2
            addcmpble r6, 2, 32, .lloop1
            mov      r6, 0

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1


            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1
            addcmpbne r6, 2, 8, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 3

            lsr      r14, r4, 16+4-1
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            vstleft  VX(48,0++), (r0+=r1)          REP 8

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_expand_rgba565
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_expand_rgba565(
;;;      unsigned long       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned long const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step,       r5
;;;      int                  s_height);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a vertical stripe of bytes

function vc_stripe_v_expand_rgba565 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            addscale r10, r2, r3 << 3
            mov      r11, r3
            add      r12, r4, 7
            lsr      r12, 3

            mov      r14, -8
            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            bmask    r4, 16+6-1
            add      r5, r4, r7
            shl      r7, 1
            mov      r13, r4

            vld      VX(48,32++), (r2+=r3)         REP 8
            mov      r6, 0
            mov      r0, 0
.lloop0:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,1)+r0, 0x100>>3
            vand     V(16,1)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,0)+r0, VX(48,32)+r6, 3
            vand     V(16,0)+r0, V(16,0)+r0, 0xFC
            vand     V(0,0)+r0, V(48,32)+r6, 0x1F
            vand     V(0,1)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 8, .lloop0
            vmov     V(0,62++), V(0,0++)           REP 2
            vmov     V(16,62++), V(16,0++)         REP 2
            addcmpbhs r15, 0, 8, .oloop
            shl      r0, r15, 1
            vmov     V(0,0++)+r0, V(0,62++)+r0     REP 2
            vmov     V(16,0++)+r0, V(16,62++)+r0   REP 2

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            lsr      r1, r5, 16
            shl      r0, 1
            shl      r1, 1
            add      r2, r4, r7
            add      r3, r5, r7
            mov      r6, 0

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vsubs    VX(32,0++), V(0,2++)+r0, V(0,0++)+r0      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r0        REP 2
            vsubs    VX(32,0++), V(16,2++)+r0, V(16,0++)+r0    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r4               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r0       REP 2
            ; recombine
            vand     V(48,0)+r6, V(32,2), 0x1F
            vand     V(48,16)+r6, V(32,3), 0xF8
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vor      VX(48,0)+r6, VX(48,0)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,0)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,0)+r6, VX(48,0)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,0)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,0)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r4, r7
            lsr      r0, r4, 16
            shl      r0, 1

            vsubs    VX(32,0++), V(0,2++)+r1, V(0,0++)+r1      REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,2++), VX(32,0++), V(0,0++)+r1        REP 2
            vsubs    VX(32,0++), V(16,2++)+r1, V(16,0++)+r1    REP 2
            vmulhn.su VX(32,0++), VX(32,0++), r5               REP 2
            vadds    V(32,4++), VX(32,0++), V(16,0++)+r1       REP 2
            ; recombine
            vand     V(32,4), V(32,4), 0xFC
            vshl     VX(32,0), V(32,4), 3
            vand     V(48,1)+r6, V(32,2), 0x1F
            vand     V(48,17)+r6, V(32,3), 0xF8
            vor      VX(48,1)+r6, VX(48,1)+r6, VX(32,0)
            ; protect
            vand     VX(32,0), VX(48,1)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            veor     VX(48,1)+r6, VX(48,1)+r6, 0x20            IFZ
            ; replace alpha
            vsub     -, V(32,5), (0xF0+0x10)>>3                SETF
            vand     -, V(32,5), 0xE0>>3                       IFN CLRA ACC
            vlsr     VX(32,0), VX(48,1)+r6, 3                  IFN
            vand     VX(32,0), VX(32,0), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
            vor      VX(48,1)+r6, VX(32,0), RGBA565_TRANSPARENTKEY IFN
            add      r5, r7
            lsr      r1, r5, 16
            shl      r1, 1

            addcmpbne r6, 2, 8, .loop

            bmask    r4, 16+6-1
            bmask    r5, 16+6-1

            eor      r13, r4
            btest    r13, 16+3
            mov      r13, r4
            beq      .noload

            lsr      r0, r4, 16+3
            shl      r0, 1+3
            mov      r2, r10
            mov      r3, r11
            addscale r10, r11 << 3
            vld      VX(48,32++), (r2+=r3)         REP 8
            mov      r6, 0
.lloop1:    vand     VX(32,0), VX(48,32)+r6, ~RGBA565_TRANSPARENTBITS
            veor     -, VX(32,0), RGBA565_TRANSPARENTKEY       SETF
            vmov     V(16,17)+r0, 0x100>>3
            vand     V(16,17)+r0, V(48,32)+r6, RGBA565_ALPHABITS IFZ
            vand     VX(32,0), VX(48,32)+r6, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
            vmul     VX(48,32)+r6, VX(32,0), 10                IFZ
            vlsr     V(16,16)+r0, VX(48,32)+r6, 3
            vand     V(16,16)+r0, V(16,16)+r0, 0xFC
            vand     V(0,16)+r0, V(48,32)+r6, 0x1F
            vand     V(0,17)+r0, V(48,48)+r6, 0xF8
            add      r0, 2
            addcmpbne r6, 1, 16, .lloop1
            addcmpbhs r15, r14, 8, .noload
            sub      r0, 32
            addscale r0, r15 << 1
            vmov     V(0,16++)+r0, V(0,14++)+r0    REP 2
            vmov     V(16,16++)+r0, V(16,14++)+r0  REP 2

.noload:    mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 3
            vst      VX(48,0++), (r0+=r1)          REP 8

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;; }}}

;; RGBA16 {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba16
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba16(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of rgba16

function vc_stripe_h_reduce_rgba16 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(0,0++),(r2+=r3)            REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_vert_offset16 r0, r4
            mov      r6, 64*48
            mov      r15, 64*64
            vld      VX(16,0++)+r0,(r2+32+=r3)     REP 16
            vld      VX(32,0++)+r0,(r2+64+=r3)     REP 16

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 5

            vst      VX(48,32++), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_rgba162
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_rgba162(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of rgba16 by a factor of 2.0 to 4.0

function vc_stripe_h_reduce_rgba162 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      VX(0,32++),(r2+=r3)           REP 16
            vld      VX(16,32++), (r2+32+=r3)      REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(0,0),  HX(0,32), HX(1,32)
            vadd     HX(1,0),  HX(2,32), HX(3,32)
            vadd     HX(2,0),  HX(4,32), HX(5,32)
            vadd     HX(3,0),  HX(6,32), HX(7,32)
            vadd     HX(4,0),  HX(8,32), HX(9,32)
            vadd     HX(5,0),  HX(10,32), HX(11,32)
            vadd     HX(6,0),  HX(12,32), HX(13,32)
            vadd     HX(7,0),  HX(14,32), HX(15,32)
            vadd     HX(8,0),  HX(16,32), HX(17,32)
            vadd     HX(9,0),  HX(18,32), HX(19,32)
            vadd     HX(10,0), HX(20,32), HX(21,32)
            vadd     HX(11,0), HX(22,32), HX(23,32)
            vadd     HX(12,0), HX(24,32), HX(25,32)
            vadd     HX(13,0), HX(26,32), HX(27,32)
            vadd     HX(14,0), HX(28,32), HX(29,32)
            vadd     HX(15,0), HX(30,32), HX(31,32)

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64

            vld      VX(0,32++),(r2+64+=r3)        REP 16
            vld      VX(16,32++), (r2+96+=r3)      REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(16+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(16+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(16+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(16+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(16+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(16+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(16+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(16+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(16+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(16+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(16+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(16+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(16+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(16+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(16+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(16+15,0)+r0, HX(30,32), HX(31,32)

            vld      VX(0,32++),(r2+128+=r3)       REP 16
            vld      VX(16,32++), (r2+160+=r3)     REP 16
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(32+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(32+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(32+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(32+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(32+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(32+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(32+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(32+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(32+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(32+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(32+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(32+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(32+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(32+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(32+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(32+15,0)+r0, HX(30,32), HX(31,32)

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 32

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 6

            vst      VX(48,32++), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_expand_rgba16
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_expand_rgba16(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step,       r5
;;;      int                  s_width);   (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a horizontal stripe of rgba16

function vc_stripe_h_expand_rgba16 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4
            vld      VX(0,0++),(r2+=r3)            REP 16
            vmov     HX(63,0), HX(0,0)
            addcmpbhs r15, 0, 16, .oloop
            shl      r0, r15, 6
            vmov     HX(0,0)+r0, HX(63,0)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

            mov      r6, 64*48
            mov      r14, 64*64

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r14, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6

            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload
            mov      r14, -16
            
            lsr      r0, r4, 16
            bic      r0, 15
            shl      r0, 6

            mov      r2, r10
            mov      r3, r11
            add      r10, 32
            vld      VX(16,0++)+r0,(r2+32+=r3)     REP 16
            addcmpbhs r15, r14, 16, .noload
            addscale r0, r15 << 6
            vmov     HX(16,0)+r0, HX(15,0)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            add      r8, 32

            vst      VX(48,32++), (r0+=r1)         REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgba16
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgba16(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,       r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of rgba16

function vc_stripe_v_reduce_rgba16 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(0++,0),(r2+=r3)            REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64
            addscale r2, r3 << 4
            vld      HX(16++,0)+r0,(r2+=r3)        REP 32

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            vst      HX(48++,32), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_rgba162
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_rgba162(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of rgba16 by a factor of 2.0 to 4.0

function vc_stripe_v_reduce_rgba162 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            lsr      r5, 1
            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(0,0),  HX(0,32), HX(1,32)
            vadd     HX(1,0),  HX(2,32), HX(3,32)
            vadd     HX(2,0),  HX(4,32), HX(5,32)
            vadd     HX(3,0),  HX(6,32), HX(7,32)
            vadd     HX(4,0),  HX(8,32), HX(9,32)
            vadd     HX(5,0),  HX(10,32), HX(11,32)
            vadd     HX(6,0),  HX(12,32), HX(13,32)
            vadd     HX(7,0),  HX(14,32), HX(15,32)
            vadd     HX(8,0),  HX(16,32), HX(17,32)
            vadd     HX(9,0),  HX(18,32), HX(19,32)
            vadd     HX(10,0), HX(20,32), HX(21,32)
            vadd     HX(11,0), HX(22,32), HX(23,32)
            vadd     HX(12,0), HX(24,32), HX(25,32)
            vadd     HX(13,0), HX(26,32), HX(27,32)
            vadd     HX(14,0), HX(28,32), HX(29,32)
            vadd     HX(15,0), HX(30,32), HX(31,32)

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16+4
            shl      r0, 6+4
            mov      r6, 64*48
            mov      r15, 64*64

            addscale r2, r3 << 5
            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(16+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(16+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(16+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(16+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(16+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(16+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(16+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(16+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(16+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(16+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(16+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(16+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(16+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(16+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(16+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(16+15,0)+r0, HX(30,32), HX(31,32)

            addscale r2, r3 << 5
            vld      HX(0++,32),(r2+=r3)           REP 32
            vlsr     HX(0++,32), HX(0++,32), 1     REP 32
            vand     HX(0++,32), HX(0++,32), 0x7777 REP 32

            vadd     HX(32+0,0)+r0,  HX(0,32), HX(1,32)
            vadd     HX(32+1,0)+r0,  HX(2,32), HX(3,32)
            vadd     HX(32+2,0)+r0,  HX(4,32), HX(5,32)
            vadd     HX(32+3,0)+r0,  HX(6,32), HX(7,32)
            vadd     HX(32+4,0)+r0,  HX(8,32), HX(9,32)
            vadd     HX(32+5,0)+r0,  HX(10,32), HX(11,32)
            vadd     HX(32+6,0)+r0,  HX(12,32), HX(13,32)
            vadd     HX(32+7,0)+r0,  HX(14,32), HX(15,32)
            vadd     HX(32+8,0)+r0,  HX(16,32), HX(17,32)
            vadd     HX(32+9,0)+r0,  HX(18,32), HX(19,32)
            vadd     HX(32+10,0)+r0, HX(20,32), HX(21,32)
            vadd     HX(32+11,0)+r0, HX(22,32), HX(23,32)
            vadd     HX(32+12,0)+r0, HX(24,32), HX(25,32)
            vadd     HX(32+13,0)+r0, HX(26,32), HX(27,32)
            vadd     HX(32+14,0)+r0, HX(28,32), HX(29,32)
            vadd     HX(32+15,0)+r0, HX(30,32), HX(31,32)

            get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r15, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 5

            vst      HX(48++,32), (r0+=r1)         REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_expand_rgba16
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_expand_rgba16(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,    r4
;;;      int                  step,       r5
;;;      int                  s_height);  (sp) -> (sp+40)
;;;
;;; FUNCTION
;;;   Expand a vertical stripe of rgba16

function vc_stripe_v_expand_rgba16 ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4

            ld       r15, (sp+40)

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            mov      r13, r4
            vld      HX(0++,0),(r2+=r3)            REP 16
            vmov     HX(63,0), HX(0,0)
            addcmpbhs r15, 0, 16, .oloop
            shl      r0, r15, 6
            vmov     HX(0,0)+r0, HX(63,0)+r0

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     get_vert_offset16 r0, r4
            get_vert_offset16 r1, r5
            add      r2, r4, r7
            add      r3, r5, r7

            mov      r6, 64*48
            mov      r14, 64*64

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r4         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(0,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r4, r7 << 1
            get_vert_offset16 r0, r2

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r5         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(1,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r5, r7 << 1
            get_vert_offset16 r1, r3

            vand     HX(0++,32), HX(0++,0)+r0, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r0, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r2         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(2,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r2, r7 << 1
            get_vert_offset16 r0, r4

            vand     HX(0++,32), HX(0++,0)+r1, 0x0F0F    REP 2
            vand     HX(2++,32), HX(0++,0)+r1, 0xF0F0    REP 2
            vsub     HX(6,32), H(1,32), H(0,32)
            vsub     HX(7,32), H(3,32), H(2,32)
            vsub     HX(8,32), H(1,48), H(0,48)
            vsub     HX(9,32), H(3,48), H(2,48)
            vmulhn.su HX(6++,32), HX(6++,32), r3         REP 4
            vadd     H(0,32), H(0,32), HX(6,32)
            vadd     H(2,32), H(2,32), HX(7,32)
            vadd     H(0,48), H(0,48), HX(8,32)
            vadd     H(2,48), H(2,48), HX(9,32)
            vand               -, HX(0,32), 0x0F0F       CLRA ACC
            vand     HX(3,32)+r6, HX(2,32), 0xF0F0       ACC
            addscale r3, r7 << 1
            get_vert_offset16 r1, r5

            add      r6, 64*4
            addcmpbne r6, 0, r14, .loop

            bmask    r4, 16+6
            bmask    r5, 16+6

            eor      r13, r4
            btest    r13, 16+4
            mov      r13, r4
            beq      .noload
            mov      r14, -16
            
            lsr      r0, r4, 16+4
            shl      r0, 6+4
            addscale r10, r11 << 4
            mov      r2, r10
            mov      r3, r11
            vld      HX(16++,0)+r0,(r2+=r3)        REP 16
            addcmpbhs r15, r14, 16, .noload
            addscale r0, r15 << 6
            vmov     HX(16,0)+r0, HX(15,0)+r0

.noload:    mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            vst      HX(48++,32), (r0+=r1)         REP 16

            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;; }}}

;; Unsmoothed YUV or anything byte oriented {{{

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_h_reduce_bytes_unsmoothed
;;;
;;; SYNOPSIS
;;;   void vc_stripe_h_reduce_bytes_unsmoothed(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of bytes

function vc_stripe_h_reduce_bytes_unsmoothed ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      H(0++,0),(r2+=r3)             REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            mov      r6, 0
            vld      H(0++,16)+r0,(r2+16+=r3)      REP 16
            vld      H(0++,32)+r0,(r2+32+=r3)      REP 16

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vmov    V(48,0)+r6, V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vmov    V(48,1)+r6, V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vmov    V(48,2)+r6, V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vmov    V(48,3)+r6, V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            add      r8, 16

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            addscale r10, r14 << 4

            vst      H(48++,0), (r0+=r1)           REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vc_stripe_v_reduce_bytes_unsmoothed
;;;
;;; SYNOPSIS
;;;   void vc_stripe_v_reduce_bytes_unsmoothed(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  rows,       r4
;;;      int                  step);      r5
;;;
;;; FUNCTION
;;;   Reduce a vertical stripe of bytes

function vc_stripe_v_reduce_bytes_unsmoothed ; {{{
            stm      r6-r15, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

            mov      r7, r5
            mov      r8, r0
            mov      r9, r1
            mov      r10, r2
            mov      r11, r3
            add      r12, r4, 15
            lsr      r12, 4
            mov      r13, 0

            lsr      r4, r7, 1
            sub      r4, 0x8000
            add      r5, r4, r7
            shl      r7, 1

            vld      V(0,0++),(r2+=r3)             REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

         ; r8 = dst ptr
         ; r9 = dst pitch
         ; r10 = src ptr
         ; r11 = src pitch

.oloop:     lsr      r0, r4, 16
            bic      r0, 15
            mov      r6, 0
            addscale r2, r3 << 4
            vld      V(0,16++)+r0,(r2+=r3)         REP 32

            lsr      r0, r4, 16
            lsr      r1, r5, 16
            add      r2, r4, r7
            add      r3, r5, r7

         ; r0 = si1
         ; r1 = si2

         ; r2 = pos1+inc
         ; r3 = pos2+inc

         ; r4 = pos1
         ; r5 = pos2

         ; r6 = dest
         ; r7 = inc
            b .loop
            .align 32
.loop:      vmov    V(48,0)+r6, V(0,0)+r0
            addscale r4, r7 << 1
            lsr      r0, r2, 16

            vmov    V(48,1)+r6, V(0,0)+r1
            addscale r5, r7 << 1
            lsr      r1, r3, 16

            vmov    V(48,2)+r6, V(0,0)+r0
            addscale r2, r7 << 1
            lsr      r0, r4, 16

            vmov    V(48,3)+r6, V(0,0)+r1
            addscale r3, r7 << 1
            lsr      r1, r5, 16

            addcmpbne r6, 4, 16, .loop

            mov      r0, r8
            mov      r1, r9
            addscale r8, r9 << 4

            lsr      r14, r4, 16+4
            sub      r14, r13
            and      r14, 3
            add      r13, r14
            mul      r14, r11
            addscale r10, r14 << 4

            vst      V(48,0++), (r0+=r1)           REP 16

            bmask    r4, 16+6
            bmask    r5, 16+6
            mov      r2, r10
            mov      r3, r11
            addcmpbne r12, -1, 0, .oloop

.end:       ldm      r6-r15, (sp++)
            .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET



.define SMOOTH
.define SMOOTHUV
;.define ROUNDING
.define SMOOTHRGB

; void vc_decimate_vert16(void *image, int pitch, int offset, int step);
function vc_decimate_vert16 ; {{{
   stm r6-r10,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10}
   add r9, r0, r1 
   neg r4, r3
   mov r10, 32
   mov r7, 64
   mov r8, 64*16
1:
   vld HX(0++, 0), (r0+=r1) REP 32
   mov r5, 0
.ifdef SMOOTHRGB
   vand H(32++, 0),  HX(0++, 0),  0x1f  REP 32  ; blue
   vlsr H(32++, 16), HX(0++, 0),  5     REP 32  ;
   vand H(32++, 16), H(32++, 16), 0x3f  REP 32  ; green
   vlsr H(32++, 32), HX(0++, 0),  11    REP 32  ; red
.endif
2:
   lsr r6, r2, 16
   shl r6, 6
.ifdef SMOOTHRGB
   not r3, r2

   vmulhn.su -, H(32, 32)+r6, r3 CLRA ACC          ; red
   vmulhn.su H(32, 48), H(33, 32)+r6, r2 ACC

   vmulhn.su -, H(32, 16)+r6, r3 CLRA ACC          ; green
   vmulhn.su H(33, 48), H(33, 16)+r6, r2 ACC

   vmulhn.su -, H(32, 0)+r6, r3 CLRA ACC          ; blue
   vmulhn.su -, H(33, 0)+r6, r2 ACC

   vshl -, H(33, 48), 5            ACC     ; green
   vshl HX(0,32)+r5, H(32, 48), 11 ACC     ; red
.else
   vmov HX(0,32)+r5, HX(0,0)+r6
.endif
   sub r2, r4
   addcmpblt r5, r7, r8, 2b
   addscale r2, r4 << 4
   vst HX(0++, 32), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r10,pc,(sp++)
   .cfa_pop {%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_verta16(void *image, int pitch, int offset, int step);
function vc_decimate_verta16 ; {{{
.ifdef SMOOTHRGB
   stm r6-r10,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10}
   add r9, r0, r1 
   neg r4, r3
   mov r10, 32
   mov r7, 64
   mov r8, 64*16
1:
   vld HX(0++, 0), (r0+=r1) REP 32
   mov r5, 0
   vand HX(32++,0), HX(0++,0), 0x0F0F   REP 32  ; red/blue
   vand HX(32++,32), HX(0++,0), 0xF0F0  REP 32  ; alpha/green

   vshl HX(32++,0), HX(32++,0), 4 REP 32
   vadd HX(32++,0), HX(32++,0), 0x0808 REP 32
   vadd HX(32++,32), HX(32++,32), 0x0808 REP 32
        
2:
   lsr r6, r2, 16
   shl r6, 6
   not r3, r2

   vmulhn.su         -, H(32, 0)+r6, r3            CLRA ACC ; blue
   vmulhn.su H(17, 32), H(33, 0)+r6, r2            ACC
        
   vmulhn.su         -, H(32, 16)+r6, r3           CLRA ACC ; red
   vmulhn.su H(17, 48), H(33, 16)+r6, r2           ACC

   vmulhn.su         -, H(32, 32)+r6, r3           CLRA ACC ; green
   vmulhn.su H(18, 32), H(33, 32)+r6, r2           ACC

   vmulhn.su         -, H(32, 48)+r6, r3           CLRA ACC ; alpha
   vmulhn.su H(18, 48), H(33, 48)+r6, r2           ACC
        
   vlsr HX(17,32), HX(17,32), 4
   vand           -, HX(17,32), 0x0F0F             CLRA ACC ; red/blue
   vand HX(0,32)+r5, HX(18,32), 0xF0F0             ACC ; alpha/green
        
   sub r2, r4
   addcmpblt r5, r7, r8, 2b
   addscale r2, r4 << 4
   vst HX(0++, 32), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r10,pc,(sp++)
   .cfa_pop {%r10,%r9,%r8,%r7,%r6,%pc}
.else
   b     vc_decimate_vert16
.endif
endfunc ; }}}

; void vc_decimate_vert_rgba565(void *image, int pitch, int offset, int step);
function vc_decimate_vert_rgba565 ; {{{
   stm r6-r10,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10}
   add r9, r0, r1 
   neg r4, r3
   mov r10, 32
   mov r7, 64
   mov r8, 64*16
1:
   vld HX(0++, 0), (r0+=r1) REP 32
.ifdef SMOOTHRGB
   mov r5, 0
   mov r6, 64*32
2:
   vand HX(16, 32), HX(0, 0)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(16, 32), RGBA565_TRANSPARENTKEY SETF
   vmov H(32, 48)+r5, 0x100>>3                  ; alpha >> 3
   vand H(32, 48)+r5, H(0, 0)+r5, RGBA565_ALPHABITS IFZ

   vand HX(16, 32), HX(0, 0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
   vmul HX(0, 0)+r5, HX(16, 32), 10 IFZ
   addcmpblt r5, r7, r6, 2b
   
   vand H(32++, 0),  HX(0++, 0),  0x1f  REP 32  ; blue
   vlsr H(32++, 16), HX(0++, 0),  5     REP 32  ;
   vand H(32++, 16), H(32++, 16), 0x3f  REP 32  ; green
   vlsr H(32++, 32), HX(0++, 0),  11    REP 32  ; red
.endif
   mov r5, 0
2:
   lsr r6, r2, 16
   shl r6, 6
.ifdef SMOOTHRGB
   not r3, r2

   vmulhn.su -, H(32, 48)+r6, r3 CLRA ACC          ; alpha
   vmulhn.su H(16, 32), H(33, 48)+r6, r2 ACC

   vmulhn.su -, H(32, 32)+r6, r3 CLRA ACC          ; red
   vmulhn.su H(17, 32), H(33, 32)+r6, r2 ACC

   vmulhn.su -, H(32, 16)+r6, r3 CLRA ACC          ; green
   vmulhn.su H(18, 32), H(33, 16)+r6, r2 ACC

   vmulhn.su -, H(32, 0)+r6, r3 CLRA ACC          ; blue
   vmulhn.su H(19, 32), H(33, 0)+r6, r2 ACC

   vshl -, H(18, 32), 5             ACC     ; green
   vshl HX(0, 32)+r5, H(17, 32), 11 ACC     ; red

   ; protect
   vand HX(20, 32), HX(0, 32)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(20, 32), RGBA565_TRANSPARENTKEY       SETF
   veor HX(0, 32)+r5, HX(0, 32)+r5, 0x20            IFZ

   ; replace alpha
   vsub     -, H(16, 32), (0xF0+0x10)>>3                SETF
   vand     -, H(16, 32), 0xE0>>3                       IFN CLRA ACC
   vlsr     HX(20, 32), HX(0, 32)+r5, 3                  IFN
   vand     HX(20, 32), HX(20, 32), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
   vor      HX(0, 32)+r5, HX(20, 32), RGBA565_TRANSPARENTKEY IFN
   
.else
   vmov HX(0,32)+r5, HX(0,0)+r6
.endif
   sub r2, r4
   addcmpblt r5, r7, r8, 2b
   addscale r2, r4 << 4
   vst HX(0++, 32), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r10,pc,(sp++)
   .cfa_pop {%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

.define Nprefactor, 23          ; HX(23,32)
.define N0to15, 30 ; H(30, 32)
.define Nindex, 31 ; HX(31, 32)
.define Nmult, 32 ; HX(32, 32)
.define Ninvmult, 48 ; HX(48, 32)
.define Ntemp, 29 ; HX(47, 0)

.ifdef SMOOTH
.macro moveit, index, reg
.ifdef ROUNDING
   vmulm -, H(1,0)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulm HX(Ntemp, 32), H(0,0)+reg, VX(Ninvmult, 32+index) ACC
   vadds HX(Ntemp, 32), HX(Ntemp, 32), 64
   vasr HX(Ntemp, 32), HX(Ntemp, 32), 7
   vadds H(index, 32), HX(Ntemp, 32), 0

   vmulm -, H(1,16)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulm HX(Ntemp, 32), H(0,16)+reg, VX(Ninvmult, 32+index) ACC
   vadds HX(Ntemp, 32), HX(Ntemp, 32), 64
   vasr HX(Ntemp, 32), HX(Ntemp, 32), 7
   vadds H(index, 48), HX(Ntemp, 32), 0
.else
   vmulhn.su -, H(1,0)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulhn.su H(index,32), H(0,0)+reg, VX(Ninvmult, 32+index) ACC

   vmulhn.su -, H(1,16)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulhn.su H(index,48), H(0,16)+reg, VX(Ninvmult, 32+index) ACC
.endif
.endm
.else
.macro moveit, index, reg
   vmov HX(index, 32), HX(0, 0)+reg
.endm
.endif

; note this buffer causes thread safety issues, so we have N copies and can cope with N concurrent users.
; Currently N==2 is sufficient (one task plus one one ISR). The alloc function will assert on the N+1 user.
.section .sdata$display_indices,"data"
   .align 32
.globl display_indices
display_indices:
   .block 32
   .block 32
display_indices_ptr:
   .word display_indices_ptr

.define Rdest,   r0
.define Rdpitch, r1
.define Rsrcu,   r2
.define Rspitch, r3
.define Roffset, r4
.define Rstep,   r5

.define Rindex1, r4
.define Rindex2, r5
.define Rindex3, r6

.define Rindices,r7
.define Rtemphi, r7
.define Rtemphi2,r6
.define Rloopend,r8
.define Rloopinc,r9

.define Rdest_store, r10
.define Rvalid,      r11
.define Rprefactor,  r12
        
.define Rsrcv,   r13
.define Rdstv,   r10


; void vc_decimate_temp_buffer_alloc();
function vc_decimate_temp_buffer_alloc ; {{{
   mov Rindex3, sr 
   di
   ld Rindices, (pc+pcrel(display_indices_ptr))
   sub Rindices, 32
   st Rindices, (pc+pcrel(display_indices_ptr))
   mov sr, Rindex3
.ifndef NDEBUG
   add Rindex3, pc, pcrel(display_indices)
   addcmpbge Rindices, 0, Rindex3, 1f
   bkpt
1:
.endif
   b lr
endfunc ; }}}   

; void vc_decimate_temp_buffer_free();
function vc_decimate_temp_buffer_free ; {{{
   mov Rindex3, sr 
   di
   ld Rindices, (pc+pcrel(display_indices_ptr))
   add Rindices, 32
   st Rindices, (pc+pcrel(display_indices_ptr))
   mov sr, Rindex3
.ifndef NDEBUG
   add Rindex3, pc, pcrel(display_indices_ptr)
   addcmpble Rindices, 0, Rindex3, 1f
   bkpt
1:
.endif
   b lr
endfunc ; }}}   

; void vc_decimate_vert(void *destination, int destination_pitch, void *source, int source_pitch, int offset, int step, int valid);
function vc_decimate_vert ; {{{
   stm r6-r12,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12}
   ld Rvalid, (sp+32)

   msb Rprefactor, Rstep
   max Rprefactor, 16
   sub Rprefactor, 16
   asr Rstep, Rprefactor
        
; remove integer offsets
   addcmpbge Rstep, 0, 0, 1f
   mul Rtemphi, Rstep, 15
   add Rtemphi, Roffset
   asr Rtemphi, Rtemphi, 16
   mul Rtemphi2, Rtemphi, Rspitch
   add Rsrcu, Rtemphi2
   shl Rtemphi, 16
   sub Roffset, Rtemphi
   b 2f
1:
   asr Rtemphi, Roffset, 16
   mul Rtemphi2, Rtemphi, Rspitch
   add Rsrcu, Rtemphi2
   bmask Roffset, 16
2:
   asr Roffset, Rprefactor
   asr Rvalid, Rprefactor
   addcmpblt Rvalid, 0, 32, 3f
   mov Rvalid, 0
3:
   shl Rvalid, 6

        
; calculate source 16.16 offsets in accumulators
   add Rindex3, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 32), (Rindex3)
   vmul      -,              H(N0to15, 32), Rstep CLRA UACC
   vmov      HX(Nmult, 32),  Roffset              UACC
   vmulhd.uu -,              H(N0to15, 32), Rstep ACC
   asr Rstep, 16
   asr Roffset, 16
   vmov      -,              Roffset              ACC
   vmul      HX(Nindex, 32), H(N0to15, 32), Rstep ACC
   vshl      HX(Nindex, 32), HX(Nindex, 32), 6
; replicate the mult coefficients for VX accesses
   veor HX(Ninvmult, 32), HX(Nmult, 32), -1
.ifdef ROUNDING
   vlsr HX(Nmult, 32), HX(Nmult, 32), 1
   vlsr HX(Ninvmult, 32), HX(Ninvmult, 32), 1
.endif

   vmov HX(Nmult++, 32), HX(Nmult, 32) REP 16
   vmov HX(Ninvmult++, 32), HX(Ninvmult, 32) REP 16

   bl vc_decimate_temp_buffer_alloc
   mov Rindex3, Rindices
   VST_REP_SAFE <HX(Nindex, 32)>, <(Rindex3)>

   add Rloopend, Rdest, Rdpitch             ; end of line
   mov Rloopinc, 32                ; outer strip loop across image
1:
   addcmpbeq Rprefactor, 0, 0, 3f
   ;; do predecimation
   mov Rdest_store, Rdest       ;save, need r0
   mov Rindex1, 0               ;output line offset
   mov Rdest, 1
   shl Rdest, Rprefactor        ;number of input lines per output line
   mov Rindex3, 2048            ;32rows of output
   mov Rloopinc, Rsrcu          ;store this for later
5:      
   mov Rindex2, 0               ;input line offset
   vld HX(32++,0), (Rsrcu+=Rspitch) REP 32
   addscale Rsrcu, Rspitch << 5
6:
   vmov HX(Nprefactor,32), H(32++,0)+Rindex2 CLRA UACC REP Rdest
   vasr H(0,0)+Rindex1, HX(Nprefactor,32), Rprefactor
   vmov HX(Nprefactor,32), H(32++,16)+Rindex2 CLRA UACC REP Rdest
   vasr H(0,16)+Rindex1, HX(Nprefactor,32), Rprefactor

   addscale Rindex2, Rdest << 6
   add Rindex1, 64
   addcmpblt Rindex2, 0, Rindex3, 6b
   addcmpblt Rindex1, 0, Rindex3, 5b
   mov Rdest, Rdest_store
   mov Rsrcu, Rloopinc
   mov Rloopinc, 32        
   b 4f
3:      
   vld HX(0++,0), (Rsrcu+=Rspitch) REP 32

4:      
   addcmpbeq Rvalid, 0, 0, 3f
   vmov HX(0,0)+Rvalid, HX(63,0)+Rvalid
3:
        
   ;; now do the scaling of these input rows
   ld Rindex1, (Rindices+0)
        
.irep index, 0, 1, 2
   ld Rindex2, (Rindices+(3*index+1)*4)
   lsr Rindex3, Rindex1, 16
   moveit 6*index+0, Rindex1
   moveit 6*index+1, Rindex3
   
.if index < 2
   ld Rindex3, (Rindices+(3*index+2)*4)
.else
   add Rsrcu, Rloopinc
.endif
   lsr Rindex1, Rindex2, 16
   moveit 6*index+2, Rindex2
   moveit 6*index+3, Rindex1

.if index < 2
   ld Rindex1, (Rindices+(3*index+3)*4)
   lsr Rindex2, Rindex3, 16
   moveit 6*index+4, Rindex3
   moveit 6*index+5, Rindex2
.endif
.endr
   VST_REP_SAFE <HX(0++,32)>, <(Rdest+=Rdpitch) REP 16>
   addcmpblt Rdest, Rloopinc, Rloopend, 1b

   bl vc_decimate_temp_buffer_free

   ldm r6-r12,pc,(sp++)
   .cfa_pop {%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

.purgem moveit
.ifdef SMOOTHUV
.macro moveit, index, reg
   vmulhn.su -, H(1,0)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulhn.su H(index,32), H(0,0)+reg, VX(Ninvmult, 32+index) ACC

   vmulhn.su -, H(17,0)+reg, VX(Nmult, 32+index) CLRA ACC
   vmulhn.su H(index+8,32), H(16,0)+reg, VX(Ninvmult, 32+index) ACC
.endm
.else
.macro moveit, index, reg
   vmov H(index, 32), H(0, 0)+reg
   vmov H(index+8, 32), H(16, 0)+reg
.endm
.endif

; void vc_decimate_uv_vert(void *dest_u, void *dest_v, int dest_pitch, void *src_u, void *src_v, int source_pitch, int offset, int step, int valid)
function vc_decimate_uv_vert ; {{{
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}

; shuffle arguments
   mov Rdstv, r1
   mov Rsrcv, r4
   mov Rdpitch, r2
   mov Rsrcu, r3
   mov Rspitch, r5
        
   ld Roffset, (sp+36)
   ld Rstep, (sp+40)
   ld Rvalid, (sp+44)

   msb Rprefactor, Rstep
   max Rprefactor, 16
   sub Rprefactor, 16
   asr Rstep, Rprefactor
        
; remove integer offsets
   addcmpbge Rstep, 0, 0, 1f
   mul Rtemphi, Rstep, 7 ; 15
   add Rtemphi, Roffset
   asr Rtemphi, Rtemphi, 16
   mul Rtemphi2, Rtemphi, Rspitch
   add Rsrcu, Rsrcu, Rtemphi2
   add Rsrcv, Rsrcv, Rtemphi2
   shl Rtemphi, 16
   sub Roffset, Rtemphi
   b 2f
1:
   asr Rtemphi, Roffset, 16
   mul Rtemphi2, Rtemphi, Rspitch
   add Rsrcu, Rsrcu, Rtemphi2
   add Rsrcv, Rsrcv, Rtemphi2
   bmask Roffset, 16
2:

   asr Roffset, Rprefactor
   asr Rvalid, Rprefactor
        
   addcmpblt Rvalid, 0, 16, 3f
   mov Rvalid, 0
3:
   shl Rvalid, 6
        
; calculate source 16.16 offsets in accumulators
   add Rindex3, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 32), (Rindex3)
   vmul      -,              H(N0to15, 32), Rstep CLRA UACC
   vmov      HX(Nmult, 32),  Roffset              UACC
   vmulhd.uu -,              H(N0to15, 32), Rstep ACC
   asr Rstep, 16
   asr Roffset, 16
   vmov      -,              Roffset              ACC
   vmul      HX(Nindex, 32), H(N0to15, 32), Rstep ACC
   vshl      HX(Nindex, 32), HX(Nindex, 32), 6
; replicate the mult coefficients for VX accesses
   vmov HX(Nmult++, 32), HX(Nmult, 32) REP 16
   veor HX(Ninvmult++, 32), HX(Nmult, 32), -1 REP 16

   bl vc_decimate_temp_buffer_alloc
   mov Rindex3, Rindices
   VST_REP_SAFE <HX(Nindex, 32)>, <(Rindex3)>

   add Rloopend, Rdest, Rdpitch        ; end of line
   mov Rloopinc, 16; 32                ; outer strip loop across image
1:
   addcmpbeq Rprefactor, 0, 0, 3f

   ;; do predecimation
   mov Rloopinc, Rdest          ;save, need r0
   mov Rindex1, 0               ;output line offset
   mov Rindex3, Rprefactor      ;shift sum right by
   mov Rdest, 1
   shl Rdest, Rprefactor        ;number of input lines per output line
   mov Rprefactor, 1024         ;16rows of output
5:
   mov Rindex2, 0               ;input line offset
   vld H(32++,0), (Rsrcu+=Rspitch) REP 16
   addcmpbeq Rdstv, 0, 0, 7f
   vld H(32++,16), (Rsrcv+=Rspitch) REP 16
7:      
   addscale Rsrcv, Rspitch << 4
   addscale Rsrcu, Rspitch << 4
6:
   vmov HX(Nprefactor,32), H(32++,0)+Rindex2 CLRA UACC REP Rdest
   vasr H(0,0)+Rindex1, HX(Nprefactor,32), Rindex3
   vmov HX(Nprefactor,32), H(32++,16)+Rindex2 CLRA UACC REP Rdest
   vasr H(16,0)+Rindex1, HX(Nprefactor,32), Rindex3

   addscale Rindex2, Rdest << 6
   add Rindex1, 64
   addcmpblt Rindex2, 0, Rprefactor, 6b
   addcmpblt Rindex1, 0, Rprefactor, 5b
   mov Rprefactor, Rindex3
   mov Rdest, Rloopinc
   add Rloopinc, Rprefactor, 4
   shl Rloopinc, Rspitch, Rloopinc
   sub Rsrcu, Rloopinc
   sub Rsrcv, Rloopinc
   mov Rloopinc, 16             ;set back to previous value
   b 4f
3:      
   vld H(0++,0), (Rsrcu+=Rspitch) REP 16
   addcmpbeq Rdstv, 0, 0, 4f
   vld H(16++,0), (Rsrcv+=Rspitch) REP 16
   
4:      
   add Rsrcu, Rloopinc
   add Rsrcv, Rloopinc
   addcmpbeq Rvalid, 0, 0, 3f 
   vmov H(0,0)+Rvalid, H(63,0)+Rvalid
   vmov H(16,0)+Rvalid, H(15,0)+Rvalid
3:
        
   ;; now do the scaling of these input rows
   ld Rindex1, (Rindices+0)

.irep index, 0, 1
.if index < 1
   ld Rindex2, (Rindices+(3*index+1)*4)
.endif
   lsr Rindex3, Rindex1, 16
   moveit 6*index+0, Rindex1
   moveit 6*index+1, Rindex3
   
.if index < 1
   ld Rindex3, (Rindices+(3*index+2)*4)
   lsr Rindex1, Rindex2, 16
   moveit 6*index+2, Rindex2
   moveit 6*index+3, Rindex1
.endif

.if index < 1
   ld Rindex1, (Rindices+(3*index+3)*4)
   lsr Rindex2, Rindex3, 16
   moveit 6*index+4, Rindex3
   moveit 6*index+5, Rindex2
.endif
.endr
   VST_REP_SAFE <H(0++,32)>, <(Rdest+=Rdpitch) REP 8>
   addcmpbeq Rdstv, 0, 0, 7f
   VST_REP_SAFE <H(8++,32)>, <(Rdstv+=Rdpitch) REP 8>
   add Rdstv, Rloopinc
7:      
   addcmpblt Rdest, Rloopinc, Rloopend, 1b
   bl vc_decimate_temp_buffer_free
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

.undef Nprefactor, N0to15, Nindex, Ntemp, Rdest, Rdpitch, Rsrcu, Rspitch, Roffset, Rstep, Rindex1, Rindex2, Rindex3, Rindices, Rtemphi, Rtemphi2, Rloopend, Rloopinc, Rdest_store, Rvalid, Rprefactor, Rsrcv, Rdstv

        
; void vc_decimate_horiz32(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiz32 ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
   mov r5, 0
1:
   vld HX(0++, 0),  (r0+=r1) REP 16
   vld HX(0++, 32), (r0+32+=r1) REP 16
   vld HX(16++, 0),  (r0+64+=r1) REP 16
   vld HX(16++, 32), (r0+96+=r1) REP 16
   vfrom32l HX(32++, 0), HX(0++, 0), HX(0++, 32) REP 16
   vfrom32h HX(32++, 32), HX(0++, 0), HX(0++, 32) REP 16
   vfrom32l HX(48++, 0), HX(16++, 0), HX(16++, 32) REP 16
   vfrom32h HX(48++, 32), HX(16++, 0), HX(16++, 32) REP 16
   vmov H(0++, 0),  H(32++, 0) REP 16   ; R0-15
   vmov H(0++, 16), H(48++, 0) REP 16   ; R16-31
   vmov H(0++, 32), H(32++, 16) REP 16  ; G0-15
   vmov H(0++, 48), H(48++, 16) REP 16  ; G16-31
   vmov H(16++, 0), H(32++, 32) REP 16   ; B0-15
   vmov H(16++, 16), H(48++, 32) REP 16  ; B16-31
   vmov H(16++, 32), H(32++, 48) REP 16  ; A0-15
   vmov H(16++, 48), H(48++, 48) REP 16  ; A16-31
2:
   lsr r6, r2, 16
.ifdef SMOOTHRGB
   not r4, r2

   vmulhn.su -, V(0, 0)+r6, r4 CLRA ACC          ; red
   vmulhn.su V(48, 0)+r5, V(0, 1)+r6, r2 ACC

   vmulhn.su -, V(0, 32)+r6, r4 CLRA ACC          ; green
   vmulhn.su V(48,16)+r5, V(0, 33)+r6, r2 ACC

   vmulhn.su -, V(16, 0)+r6, r4 CLRA ACC          ; blue
   vmulhn.su V(48,32)+r5, V(16, 1)+r6, r2 ACC

   vmulhn.su -, V(16, 32)+r6, r4 CLRA ACC          ; alpha
   vmulhn.su V(48,48)+r5, V(16, 33)+r6, r2 ACC
.else
   vmov V(48, 0)+r5, V(0,  0)+r6
   vmov V(48,16)+r5, V(0, 32)+r6
   vmov V(48,32)+r5, V(16, 0)+r6
   vmov V(48,48)+r5, V(16,32)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   VST_H32_REP <(48++,0)>, r0, r1, 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_horiz16(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiz16 ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
   mov r5, 0
1:
   vld HX(0++, 0),  (r0+=r1) REP 16
   vld HX(0++, 32), (r0+32+=r1) REP 16
.ifdef SMOOTHRGB
   vlsr H(32++, 0),  HX(0++, 0),  11    REP 16  ; red
   vlsr H(32++, 16), HX(0++, 32), 11    REP 16  ; red

   vlsr H(16++, 0),  HX(0++, 0),  5     REP 16  ;
   vand H(16++, 0),  H(16++, 0),  0x3f  REP 16  ; green

   vlsr H(16++, 16), HX(0++, 32), 5     REP 16  ;
   vand H(16++, 16), H(16++, 16), 0x3f  REP 16  ; green

   vand H(0++, 0),   HX(0++, 0),  0x1f  REP 16  ; blue
   vand H(0++, 16),  HX(0++, 32), 0x1f  REP 16  ; blue
.endif

2:
   lsr r6, r2, 16
.ifdef SMOOTHRGB
   not r4, r2

   vmulhn.su -, V(32, 0)+r6, r4 CLRA ACC          ; red
   vmulhn.su H(48,32), V(32, 1)+r6, r2 ACC

   vmulhn.su -, V(16, 0)+r6, r4 CLRA ACC          ; green
   vmulhn.su H(49,32), V(16, 1)+r6, r2 ACC

   vmulhn.su -, V(0, 0)+r6, r4 CLRA ACC          ; blue
   vmulhn.su -, V(0, 1)+r6, r2 ACC

   vshl -, H(49, 32), 5            ACC     ; green
   vshl VX(48,0)+r5, H(48, 32), 11 ACC     ; red
.else
   vmov VX(48,0)+r5, VX(0,0)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   vst HX(48++,0), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_horiza16(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiza16 ; {{{
.ifdef SMOOTHRGB
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
   mov r5, 0
1:
   vld VX(0,0++),  (r0+=r1)                        REP 16
   vld VX(16,0++), (r0+32+=r1)                     REP 16
   vand HX(32++,0), HX(0++,0), 0x0F0F   REP 32  ; red/blue
   vand HX(32++,32), HX(0++,0), 0xF0F0  REP 32  ; alpha/green
2:
   lsr r6, r2, 16
   not r4, r2

   vmulhn.su        -, H(32, 0)+r6, r4             CLRA ACC ; red
   vmulhn.su H(48, 0), H(33, 0)+r6, r2             ACC

   vmulhn.su         -, H(32, 16)+r6, r4           CLRA ACC ; blue
   vmulhn.su H(48, 48), H(33, 16)+r6, r2           ACC

   vmulhn.su         -, H(32, 32)+r6, r4           CLRA ACC ; green
   vmulhn.su H(48, 32), H(33, 32)+r6, r2           ACC

   vmulhn.su         -, H(32, 48)+r6, r4           CLRA ACC ; blue
   vmulhn.su H(48, 48), H(33, 48)+r6, r2           ACC

   vand           -, HX(48,0), 0x0F0F              CLRA ACC ; red/blue
   vand HX(0,32)+r5, HX(49,0), 0xF0F0              ACC ; alpha/green
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   vst VX(48,0++), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
.else
   b vc_decimate_horiz16
.endif
endfunc ; }}}

; void vc_decimate_horiz_rgba565(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiz_rgba565 ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
1:
.ifdef SMOOTHRGB
   vld HX(0++, 0),  (r0+=r1) REP 16
   vld HX(16++, 0), (r0+32+=r1) REP 16
   mov r5, 0
   mov r6, 64*32
2:
   vand HX(16, 32), HX(0, 0)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(16, 32), RGBA565_TRANSPARENTKEY SETF
   vmov H(32, 48)+r5, 0x100>>3                  ; alpha >> 3
   vand H(32, 48)+r5, H(0, 0)+r5, RGBA565_ALPHABITS IFZ

   vand HX(16, 32), HX(0, 0)+r5, RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFZ
   vmul HX(0, 0)+r5, HX(16, 32), 10 IFZ
   add r5, 64
   addcmpblt r5, 0, r6, 2b
   ; we put alpha in 16x32 block - want 32x16
   vmov H(48++, 32), H(32++, 48) REP 16

   vlsr H(48++, 0),  HX(0++, 0),  11    REP 16  ; red
   vlsr H(48++, 16), HX(16++, 0), 11    REP 16  ; red

   vlsr H(32++, 32), HX(0++, 0),  5     REP 16  ;
   vand H(32++, 32), H(32++, 32), 0x3f  REP 16  ; green

   vlsr H(32++, 48), HX(16++, 0), 5     REP 16  ;
   vand H(32++, 48), H(32++, 48), 0x3f  REP 16  ; green

   vand H(32++, 0),  HX(0++, 0),  0x1f  REP 16  ; blue
   vand H(32++, 16), HX(16++, 0), 0x1f  REP 16  ; blue
.else
   vld HX(0++, 0),  (r0+=r1) REP 16
   vld HX(0++, 32), (r0+32+=r1) REP 16
.endif
   mov r5, 0

2:
   lsr r6, r2, 16
.ifdef SMOOTHRGB
   not r4, r2

   vmulhn.su -, V(48, 32)+r6, r4 CLRA ACC          ; alpha
   vmulhn.su H(16, 32), V(48, 33)+r6, r2 ACC

   vmulhn.su -, V(48, 0)+r6, r4 CLRA ACC          ; red
   vmulhn.su H(17, 32), V(48, 1)+r6, r2 ACC

   vmulhn.su -, V(32, 32)+r6, r4 CLRA ACC          ; green
   vmulhn.su H(18, 32), V(32, 33)+r6, r2 ACC

   vmulhn.su -, V(32, 0)+r6, r4 CLRA ACC          ; blue
   vmulhn.su H(19, 32), V(32, 1)+r6, r2 ACC

   vshl -, H(18, 32), 5             ACC     ; green
   vshl VX(0, 32)+r5, H(17, 32), 11 ACC     ; red

   ; protect
   vand HX(20, 32), VX(0, 32)+r5, ~RGBA565_TRANSPARENTBITS
   veor -, HX(20, 32), RGBA565_TRANSPARENTKEY       SETF
   veor VX(0, 32)+r5, VX(0, 32)+r5, 0x20            IFZ

   ; replace alpha
   vsub     -, H(16, 32), (0xF0+0x10)>>3                SETF
   vand     -, H(16, 32), 0xE0>>3                       IFN CLRA ACC
   vlsr     HX(20, 32), VX(0, 32)+r5, 3                  IFN
   vand     HX(20, 32), HX(20, 32), RGBA565_TRANSPARENTBITS & ~RGBA565_ALPHABITS IFN ACC
   vor      VX(0, 32)+r5, HX(20, 32), RGBA565_TRANSPARENTKEY IFN

.else
   vmov VX(32, 0)+r5, VX(0, 0)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
.ifdef SMOOTHRGB
   vst HX(0++, 32), (r0+=r1) REP 16
.else
   vst HX(32++, 0), (r0+=r1) REP 16
.endif
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}


; void vc_decimate_horiz(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiz ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
   mov r5, 0
1:
   vld H(0++,0),  (r0+=r1) REP 16
   vld H(0++,16), (r0+16+=r1) REP 16
   vld H(0++,32), (r0+32+=r1) REP 16
2:
   lsr r6, r2, 16
.ifdef SMOOTH
   not r4, r2
   vmulhn.su -, V(0,0)+r6, r4 CLRA ACC
   vmulhn.su V(48,0)+r5, V(0,1)+r6, r2 ACC
.else
   vmov V(48,0)+r5, V(0,0)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   vst H(48++,0), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_uv_horiz(void *image, int pitch, int offset, int step, int height);
function vc_decimate_uv_horiz ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*8
   shl r10, r1, 4
   mov r5, 0
   vbitplanes -, 0xff SETF
1:
   vldleft  H(0++,0),  (r0+=r1) REP 16
;   vldright H(0++,0),  (r0+=r1) REP 16
   vldleft  H(0++,16), (r0+16+=r1) REP 16
2:
   lsr r6, r2, 16
.ifdef SMOOTHUV
   not r4, r2
   vmulhn.su -, V(0,0)+r6, r4 CLRA ACC
   vmulhn.su V(48,0)+r5, V(0,1)+r6, r2 ACC
.else
   vmov V(48,0)+r5, V(0,0)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   vstleft  H(48++,0), (r0+=r1) IFN REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}
   
; void vc_decimate_2_horiz(void *image, int pitch, int height);
function vc_decimate_2_horiz ; {{{
   stm r6-r8,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8}
   mov r4, r0
   mov r6, r0
   mov r7, r4
   add r5, r0, r1
   shl r3, r1, 4
1: ; width loop
   mul r8, r1, r2
   add r8, r0
2: ; height loop
   vldleft  HX(0++,0), (r0+=r1) REP 16
;   vldright HX(0++,0), (r0+=r1) REP 16
.ifdef SMOOTH
   vadd HX(0++,0), H(0++,0), H(0++,16) REP 16
   vasr H(0++,0), HX(0++,0), 1 REP 16
.endif
   vstleft  H(0++,0), (r4+=r1) REP 16
;   vstright H(0++,0), (r4+=r1) REP 16
   add r4, r3
   addcmpblt r0, r3, r8, 2b
   add r6, 32
   add r7, 16
   mov r0, r6
   mov r4, r7
   addcmpblt r0, 0, r5, 1b
   ldm r6-r8,pc,(sp++)
   .cfa_pop {%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_2_horiz16(void *image, int pitch, int height);
function vc_decimate_2_horiz16 ; {{{
   stm r6-r8,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8}
   mov r4, r0
   mov r6, r0
   mov r7, r4
   add r5, r0, r1
   shl r3, r1, 4
1: ; width loop
   mul r8, r1, r2
   add r8, r0
2: ; height loop
.ifdef SMOOTHRGB
   vld      HX(32++, 0), (r0+=r1)    REP 16
   vld      HX(32++,32), (r0+32+=r1) REP 16

   vfrom32l HX(0++, 0), HX(32++,0), HX(32++,32) REP 16
   vfrom32h HX(0++,32), HX(32++,0), HX(32++,32) REP 16

   vlsr H(32++, 0),  HX(0++, 0),  11    REP 16  ; red
   vlsr H(32++, 16), HX(0++, 32), 11    REP 16  ; red

   vlsr H(16++, 0),  HX(0++, 0),  5     REP 16  ;
   vand H(16++, 16), H(16++, 0),  0x3f  REP 16  ; green

   vlsr H(16++, 16), HX(0++, 32), 5     REP 16  ;
   vand H(16++, 16), H(16++, 16), 0x3f  REP 16  ; green

   vand H(0++, 0),   HX(0++, 0),  0x1f  REP 16  ; blue
   vand H(0++, 16),  HX(0++, 32), 0x1f  REP 16  ; blue

.if _VC_VERSION > 2
   st r0, (--sp)
   mov r0, 48
   vadd H(  0++, 0), H( 0++, 0), H(0++, 16) REP r0
   vlsr HX( 0++, 0), H( 0++, 0), 1  REP r0 ; blue, green and red
   ld r0, (sp++)
.else
   vadd H(  0++, 0), H( 0++, 0), H(0++, 16) REP 48
   vlsr HX( 0++, 0), H( 0++, 0), 1  REP 48 ; blue, green and red
.endif
   vshl HX(16++, 0), H(16++, 0), 5  REP 16 ; green
   vshl HX(32++, 0), H(32++, 0), 11 REP 16 ; red

   vor HX(0++, 0), HX(0++, 0), HX(16++, 0) REP 16
   vor HX(0++, 0), HX(0++, 0), HX(32++, 0) REP 16
   vst HX(0++,0), (r4+=r1) REP 16
.else
   vld      HX(0++,0), (r0+=r1) REP 16
   vld      HX(0++,32), (r0+32+=r1) REP 16
   vfrom32l HX(0++,0), HX(0++,0), HX(0++,32) REP 16
   vst      HX(0++,0), (r4+=r1) REP 16
.endif
   add r4, r3
   addcmpblt r0, r3, r8, 2b
   add r6, 64
   add r7, 32
   mov r0, r6
   mov r4, r7
   addcmpblt r0, 0, r5, 1b
   ldm r6-r8,pc,(sp++)
   .cfa_pop {%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_2_horiza16(void *image, int pitch, int height);
function vc_decimate_2_horiza16 ; {{{
.ifdef SMOOTHRGB
   stm r6-r8,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8}
   mov r4, r0
   mov r6, r0
   mov r7, r4
   add r5, r0, r1
   shl r3, r1, 4
1: ; width loop
   mul r8, r1, r2
   add r8, r0
2: ; height loop
   vld      HX(32++, 0), (r0+=r1)                  REP 16
   vld      HX(32++,32), (r0+32+=r1)               REP 16

   vfrom32l HX(0++, 0), HX(32++,0), HX(32++,32)    REP 16
   vfrom32h HX(16++,0), HX(32++,0), HX(32++,32)    REP 16

   vand     HX(0++,32), HX(0++,0), 0xF0F0          REP 32
   vand     HX(0++,0), HX(0++,0), 0x0F0F           REP 32
   vlsr     HX(0++,32), HX(0++,32), 1              REP 32
   vadd     HX(0++,0), HX(0++,0), HX(16++,0)       REP 16
   vadd     HX(0++,32), HX(0++,32), HX(16++,32)    REP 16
   vlsr     HX(0++,0), HX(0++,0), 1                REP 16

   vand     HX(0++,0), HX(0++,0), 0x0F0F           REP 16
   vand     HX(0++,32), HX(0++,32), 0xF0F0         REP 16

   vor      HX(0++,0), HX(0++,0), HX(0++,32)       REP 16

   vst      HX(0++,0), (r4+=r1)                    REP 16
   add r4, r3
   addcmpblt r0, r3, r8, 2b
   add r6, 64
   add r7, 32
   mov r0, r6
   mov r4, r7
   addcmpblt r0, 0, r5, 1b
   ldm r6-r8,pc,(sp++)
   .cfa_pop {%r8,%r7,%r6,%pc}
.else
   b     vc_decimate_2_horiz16
.endif
endfunc ; }}}

; void vc_decimate_2_horiz32(void *image, int pitch, int height);
function vc_decimate_2_horiz32 ; {{{
   stm r6-r8,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8}
   mov r4, r0
   mov r6, r0
   mov r7, r4
   add r5, r0, r1
   shl r3, r1, 4
1: ; width loop
   mul r8, r1, r2
   add r8, r0
2: ; height loop
   vld HX(0++, 0),  (r0+=r1) REP 16
   vld HX(0++, 32), (r0+32+=r1) REP 16

.ifdef SMOOTHRGB
   vadd VX(16, 0++), V(0 , 0++), V(0, 2++) REP 2
   vadd VX(16, 2++), V(0 , 4++), V(0, 6++) REP 2
   vadd VX(16, 4++), V(0 , 8++), V(0,10++) REP 2
   vadd VX(16, 6++), V(0 ,12++), V(0,14++) REP 2
   vadd VX(16, 8++), V(0 ,32++), V(0,34++) REP 2
   vadd VX(16,10++), V(0 ,36++), V(0,38++) REP 2
   vadd VX(16,12++), V(0 ,40++), V(0,42++) REP 2
   vadd VX(16,14++), V(0 ,44++), V(0,46++) REP 2

   vadd VX(16,32++), V(0 ,16++), V(0,18++) REP 2
   vadd VX(16,34++), V(0 ,20++), V(0,22++) REP 2
   vadd VX(16,36++), V(0 ,24++), V(0,26++) REP 2
   vadd VX(16,38++), V(0 ,28++), V(0,30++) REP 2
   vadd VX(16,40++), V(0 ,48++), V(0,50++) REP 2
   vadd VX(16,42++), V(0 ,52++), V(0,54++) REP 2
   vadd VX(16,44++), V(0 ,56++), V(0,58++) REP 2
   vadd VX(16,46++), V(0 ,60++), V(0,62++) REP 2

   vlsr H(48++,  0), HX(16++,  0), 1 REP 16
   vlsr H(48++, 16), HX(16++, 32), 1 REP 16
.else
   vmov VX(48, 0++), VX(0 , 0++) REP 2
   vmov VX(48, 2++), VX(0 , 4++) REP 2
   vmov VX(48, 4++), VX(0 , 8++) REP 2
   vmov VX(48, 6++), VX(0 ,12++) REP 2
   vmov VX(48, 8++), VX(0 ,32++) REP 2
   vmov VX(48,10++), VX(0 ,36++) REP 2
   vmov VX(48,12++), VX(0 ,40++) REP 2
   vmov VX(48,14++), VX(0 ,44++) REP 2
.endif
   vst HX(48++, 0), (r4+=r1) REP 16

   add r4, r3
   addcmpblt r0, r3, r8, 2b
   add r6, 64
   add r7, 32
   mov r0, r6
   mov r4, r7
   addcmpblt r0, 0, r5, 1b
   ldm r6-r8,pc,(sp++)
   .cfa_pop {%r8,%r7,%r6,%pc}
endfunc ; }}}


; double A = -1.0;
; 0 <= t < 1. 0 <= Bn <= 1.
; #define B0(t) (A*t*t*t - 2*A*t*t + A*t)
; #define B1(t) ((A+2)*t*t*t - (A+3)*t*t + 1.0)
; #define B2(t) (-(A+2)*t*t*t + (2*A+3)*t*t - A*t)
; #define B3(t) (-A*t*t*t + A*t*t)
;
; with pixels P0,P1,P2,P3 and wanting pixel between P1 and P2
; output = B0 * P0 + B1 * P1 + B2 * P2 + B3 * P3
;
; i.e. t=0, B0=0, B1=1, B2=0, B3=0
; i.e. t=1, B0=0, B1=0, B2=1, B3=0
;
; VRF: [ Src  Src  B0 B0 ]
;      [ Src  Src  B1 B1 ]
;      [           B2 B2 ]
;      [ Dest Dest B3 B3 ]

.FORGET

.define NSrc, 0      ; H(0,0)
.define NDest, 48    ; H(48,0)
.define N0to15, 0    ; temp: H(0, 0)
.define Nindex, 1    ; temp: HX(1, 0)
.define Ntemp, 47    ; temp: HX(47, 0)

.define Nt,  2    ; temp: HX(2, 0)
.define Nt2, 3    ; temp: HX(3, 0)
.define Nt3, 4    ; temp: HX(4, 0)

.define NB0, 0       ; HX(0, 32)
.define NB1, 16      ; HX(16, 32)
.define NB2, 32      ; HX(32, 32)
.define NB3, 48      ; HX(48, 32)

.purgem moveit
.ifdef SMOOTH
.macro moveit, index, reg
.ifdef ROUNDING
   vmulm -,                 H(NSrc+3, 0)+reg, VX(NB3, 32+index) CLRA ACC
   vmulm -,                 H(NSrc+2, 0)+reg, VX(NB2, 32+index) ACC
   vmulm -,                 H(NSrc+1, 0)+reg, VX(NB1, 32+index) ACC
   vmulm HX(Ntemp, 0),      H(NSrc+0, 0)+reg, VX(NB0, 32+index) ACC
   vadds HX(Ntemp, 0), HX(Ntemp, 0), 64
   vasr HX(Ntemp, 0), HX(Ntemp, 0), 7
   vadds H(NDest+index, 0), HX(Ntemp, 0), 0

   vmulm -,                  H(NSrc+3, 16)+reg, VX(NB3, 32+index) CLRA ACC
   vmulm -,                  H(NSrc+2, 16)+reg, VX(NB2, 32+index) ACC
   vmulm -,                  H(NSrc+1, 16)+reg, VX(NB1, 32+index) ACC
   vmulm HX(Ntemp, 0),       H(NSrc+0, 16)+reg, VX(NB0, 32+index) ACC
   vadds HX(Ntemp, 0), HX(Ntemp, 0), 64
   vasr HX(Ntemp, 0), HX(Ntemp, 0), 7
   vadds H(NDest+index, 16), HX(Ntemp, 0), 0
.else
   vmulhn.ss -,                 H(NSrc+3, 0)+reg, VX(NB3, 32+index) CLRA ACC
   vmulhn.su -,                 H(NSrc+2, 0)+reg, VX(NB2, 32+index) ACC
   vmulhn.su -,                 H(NSrc+1, 0)+reg, VX(NB1, 32+index) ACC
   vmulhn.ss HX(Ntemp, 0),         H(NSrc+0, 0)+reg, VX(NB0, 32+index) ACC
   vadds H(NDest+index, 0), HX(Ntemp, 0), 0

   vmulhn.ss -,                  H(NSrc+3, 16)+reg, VX(NB3, 32+index) CLRA ACC
   vmulhn.su -,                  H(NSrc+2, 16)+reg, VX(NB2, 32+index) ACC
   vmulhn.su -,                  H(NSrc+1, 16)+reg, VX(NB1, 32+index) ACC
   vmulhn.ss HX(Ntemp, 0),          H(NSrc+0, 16)+reg, VX(NB0, 32+index) ACC
   vadds H(NDest+index, 16), HX(Ntemp, 0), 0
.endif
.endm
.else
.macro moveit, index, reg
   vmov HX(NDest+index, 0), HX(NSrc+0, 0)+reg
.endm
.endif

.define Rdest,   r0
.define Rpitch,  r1
.define Rsrcu,   r2
.define Rsrcv,   r3

.define Roffset, r4
.define Rstep,   r5

.define Rindex1, r4
.define Rindex2, r5
.define Rindex3, r6

.define Rindices,r7
.define Rtemphi, r7
.define Rtemphi2,r6
.define Rloopend,r8
.define Rloopinc,r9
; void vc_decimate_vert_cubic(void *image, int pitch, int offset, int step);
function vc_decimate_vert_cubic ; {{{
   stm r6-r9,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9}
; remove integer offsets
   addcmpbge Rstep, 0, 0, 1f
   mul Rtemphi, Rstep, 15
   add Rtemphi, Roffset
   asr Rtemphi, Rtemphi, 16
   mul Rtemphi2, Rtemphi, Rpitch
   add Rsrcu, Rdest, Rtemphi2
   shl Rtemphi, 16
   sub Roffset, Rtemphi
   b 2f
1:
   asr Rtemphi, Roffset, 16
   mul Rtemphi2, Rtemphi, Rpitch
   add Rsrcu, Rdest, Rtemphi2
   bmask Roffset, 16
2:

; calculate source 16.16 offsets in accumulators
   add Rindex3, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 0), (Rindex3)
   vmul      -,              H(N0to15, 0), Rstep CLRA UACC
   vmov      HX(Nt, 0),  Roffset              UACC
   vmulhd.uu -,              H(N0to15, 0), Rstep ACC
   asr Rstep, 16
   asr Roffset, 16
   vmov      -,              Roffset              ACC
   vmul      HX(Nindex, 0), H(N0to15, 0), Rstep ACC
   vshl      HX(Nindex, 0), HX(Nindex, 0), 6


   vmulhn.uu HX(Nt2, 0), HX(Nt, 0), HX(Nt, 0)
   vmulhn.uu HX(Nt3, 0), HX(Nt2, 0), HX(Nt, 0)
.set A, -(1<<15)
; #define B0(t) (A*t*t*t - 2*A*t*t + A*t)
   vmulhn.uu      -,           HX(Nt3, 0), (1*A)>>0   CLRA SACC
   vmul           -,           HX(Nt3, 0), (1*A)>>16  SACC
   vmulhn.uu      -,           HX(Nt2, 0), (-(2*A)>>0) & 0xfff SACC
   vmul           -,           HX(Nt2, 0), -(2*A)>>16 SACC
   vmulhn.uu      -,           HX(Nt, 0),  (1*A)>>0   SACC
   vmul           HX(NB0, 32), HX(Nt, 0),  (1*A)>>16  SACC

; #define B1(t) ((A+2)*t*t*t - (A+3)*t*t + 1.0)
   vmulhn.uu      -,           HX(Nt3, 0), ((A+(2<<16))>>0) & 0xffff  CLRA SACC
   vmul           -,           HX(Nt3, 0), (A+(2<<16))>>16  SACC
   vmulhn.uu      -,           HX(Nt2, 0), (-(A+(3<<16))>>0) & 0xffff  SACC
   vmul           -,           HX(Nt2, 0), -(A+(3<<16))>>16 SACC
   vmulhn.uu      -,           HX(Nt, 0),  (0)>>0     SACC
   vmul           HX(NB1, 32), HX(Nt, 0),  (0)>>16    SACC

   vmov           -, HX(Nt, 0) SETF
   vadd           HX(NB1, 32), HX(NB1, 32),  -1 IFNN

; #define B2(t) (-(A+2)*t*t*t + (2*A+3)*t*t - A*t)
   vmulhn.uu      -,           HX(Nt3, 0), (-(A+(2<<16))>>0) & 0xffff  CLRA SACC
   vmul           -,           HX(Nt3, 0), -(A+(2<<16))>>16  SACC
   vmulhn.uu      -,           HX(Nt2, 0), ((2*A+(3<<16))>>0) & 0xffff  SACC
   vmul           -,           HX(Nt2, 0), (2*A+(3<<16))>>16 SACC
   vmulhn.uu      -,           HX(Nt, 0),  -(A)>>0     SACC
   vmul           HX(NB2, 32), HX(Nt, 0),  -(A)>>16    SACC

   vadd           HX(NB2, 32), HX(NB2, 32),  -1 IFN

; #define B3(t) -(-A*t*t*t + A*t*t)
   vmulhn.uu      -,           HX(Nt3, 0), -(A)>>0   CLRA SACC
   vmul           -,           HX(Nt3, 0), -(A)>>16  SACC
   vmulhn.uu      -,           HX(Nt2, 0), (A)>>0    SACC
   vmul           -,           HX(Nt2, 0), (A)>>16   SACC
   vmulhn.uu      -,           HX(Nt, 0),  (0*A)>>0  SACC
   vmul           HX(NB3, 32), HX(Nt, 0),  (0*A)>>16 SACC

.ifdef ROUNDING
   vasr HX(NB0, 32), HX(NB0, 32), 1
   vlsr HX(NB1, 32), HX(NB1, 32), 1
   vlsr HX(NB2, 32), HX(NB2, 32), 1
   vasr HX(NB3, 32), HX(NB3, 32), 1
.endif
   
; replicate the mult coefficients for VX accesses
   vmov HX(NB0++, 32), HX(NB0, 32) REP 16
   vmov HX(NB1++, 32), HX(NB1, 32) REP 16
   vmov HX(NB2++, 32), HX(NB2, 32) REP 16
   vmov HX(NB3++, 32), HX(NB3, 32) REP 16

   bl vc_decimate_temp_buffer_alloc
   mov Rindex3, Rindices
   vst HX(Nindex, 0), (Rindex3)

   add Rloopend, Rdest, Rpitch             ; end of line
   mov Rloopinc, 32                ; outer strip loop across image
.align 32
1:
   ld Rindex1, (Rindices+0)
.if _VC_VERSION > 2
   st r0, (--sp)
   mov r0, 48
   vld HX(NSrc++,0), (Rsrcu+=Rpitch) REP r0
   ld r0, (sp++)
.else
   vld HX(NSrc++,0), (Rsrcu+=Rpitch) REP 48
.endif
.irep index, 0, 1, 2
   ld Rindex2, (Rindices+(3*index+1)*4)
   lsr Rindex3, Rindex1, 16
   moveit 6*index+0, Rindex1
   moveit 6*index+1, Rindex3
   
.if index < 2
   ld Rindex3, (Rindices+(3*index+2)*4)
.else
   add Rsrcu, Rloopinc
.endif
   lsr Rindex1, Rindex2, 16
   moveit 6*index+2, Rindex2
   moveit 6*index+3, Rindex1

.if index < 2
   ld Rindex1, (Rindices+(3*index+3)*4)
   lsr Rindex2, Rindex3, 16
   moveit 6*index+4, Rindex3
   moveit 6*index+5, Rindex2
.endif
.endr
   vst HX(NDest++, 0), (Rdest+=Rpitch) REP 16
   addcmpblt Rdest, Rloopinc, Rloopend, 1b
   bl vc_decimate_temp_buffer_free
   ldm r6-r9,pc,(sp++)
   .cfa_pop {%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

.purgem moveit
.macro moveit, index, reg
   vmov HX(index, 32), HX(0, 0)+reg
.endm

; void vc_decimate_vert_unsmoothed(void *image, int pitch, int offset, int step);
function vc_decimate_vert_unsmoothed ; {{{
   stm r6-r9,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9}
; remove integer offsets
   addcmpbge Rstep, 0, 0, 1f
   mul Rtemphi, Rstep, 15
   add Rtemphi, Roffset
   asr Rtemphi, Rtemphi, 16
   mul Rtemphi2, Rtemphi, Rpitch
   add Rsrcu, Rdest, Rtemphi2
   shl Rtemphi, 16
   sub Roffset, Rtemphi
   b 2f
1:
   asr Rtemphi, Roffset, 16
   mul Rtemphi2, Rtemphi, Rpitch
   add Rsrcu, Rdest, Rtemphi2
   bmask Roffset, 16
2:

; calculate source 16.16 offsets in accumulators
   add Rindex3, pc, pcrel(vc_image_const_0_15)
   vld H(N0to15, 32), (Rindex3)
   vmul      -,              H(N0to15, 32), Rstep CLRA UACC
   vmov      HX(Nmult, 32),  Roffset              UACC
   vmulhd.uu -,              H(N0to15, 32), Rstep ACC
   asr Rstep, 16
   asr Roffset, 16
   vmov      -,              Roffset              ACC
   vmul      HX(Nindex, 32), H(N0to15, 32), Rstep ACC
   vshl      HX(Nindex, 32), HX(Nindex, 32), 6
; replicate the mult coefficients for VX accesses
   veor HX(Ninvmult, 32), HX(Nmult, 32), -1
.ifdef ROUNDING
   vlsr HX(Nmult, 32), HX(Nmult, 32), 1
   vlsr HX(Ninvmult, 32), HX(Ninvmult, 32), 1
.endif

   vmov HX(Nmult++, 32), HX(Nmult, 32) REP 16
   vmov HX(Ninvmult++, 32), HX(Ninvmult, 32) REP 16

   bl vc_decimate_temp_buffer_alloc
   mov Rindex3, Rindices
   vst HX(Nindex, 32), (Rindex3)

   add Rloopend, Rdest, Rpitch             ; end of line
   mov Rloopinc, 32                ; outer strip loop across image
.align 32
1:
   ld Rindex1, (Rindices+0)
   vld HX(0++,0), (Rsrcu+=Rpitch) REP 32
.irep index, 0, 1, 2
   ld Rindex2, (Rindices+(3*index+1)*4)
   lsr Rindex3, Rindex1, 16
   moveit 6*index+0, Rindex1
   moveit 6*index+1, Rindex3
   
.if index < 2
   ld Rindex3, (Rindices+(3*index+2)*4)
.else
   add Rsrcu, Rloopinc
.endif
   lsr Rindex1, Rindex2, 16
   moveit 6*index+2, Rindex2
   moveit 6*index+3, Rindex1

.if index < 2
   ld Rindex1, (Rindices+(3*index+3)*4)
   lsr Rindex2, Rindex3, 16
   moveit 6*index+4, Rindex3
   moveit 6*index+5, Rindex2
.endif
.endr
   vst HX(0++,32), (Rdest+=Rpitch) REP 16
   addcmpblt Rdest, Rloopinc, Rloopend, 1b

   bl vc_decimate_temp_buffer_free

   ldm r6-r9,pc,(sp++)
   .cfa_pop {%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

; void vc_decimate_horiz_unsmoothed(void *image, int pitch, int offset, int step, int height);
function vc_decimate_horiz_unsmoothed ; {{{
   stm r6-r11,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
   mul r9, r4, r1
   add r9, r0
   mov r11, r2
   mov r7, 1
   mov r8, 1*16
   shl r10, r1, 4
   mov r5, 0
1:
   vld H(0++,0),  (r0+=r1) REP 16
   vld H(0++,16), (r0+16+=r1) REP 16
   vld H(0++,32), (r0+32+=r1) REP 16
2:
   lsr r6, r2, 16
.if 0 //def SMOOTH
   not r4, r2
   vmulhn.su -, V(0,0)+r6, r4 CLRA ACC
   vmulhn.su V(48,0)+r5, V(0,1)+r6, r2 ACC
.else
   vmov V(48,0)+r5, V(0,0)+r6
.endif
   add r2, r3
   addcmpblt r5, r7, r8, 2b
   mov r2, r11
   mov r5, 0
   vst H(48++,0), (r0+=r1) REP 16
   addcmpblt r0, r10, r9, 1b
   ldm r6-r11,pc,(sp++)
   .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}

