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

; TODO: replace these with the ones from vcinclude/common.inc
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

.ifdef RGB888
.define RGB_ROFF, 0
.define RGB_BOFF, 2
.else
.define RGB_ROFF, 2
.define RGB_BOFF, 0
.endif
.define RGB_GOFF, 1


;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_rgb565_to_rgb888
;;;
;;; SYNOPSIS
;;;   void vc_genconv_rgb565_to_rgb888(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines);     r5
;;;
;;; FUNCTION
;;;   Convert n (<= 16) lines of rgb565 to rgb888

function vc_genconv_rgb565_to_rgb888 ; {{{
            stm      r6-r9, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9}

            mul      r8, r4, 3
            mov      r9, r5
            and      r4, r0, 31
            and      r5, r2, 63
            sub      r0, r4
            sub      r2, r5
            lsr      r5, 1

            ;  r4 = dst offset
            ;  r5 = src offset
            ;  r8 = columns
            ;  r9 = lines

            cbclr
            add      r5, 32-1

            addcmpblt r8, 0, 32, .leftedge
            addcmpbeq r4, 0, 0, .begin
            ; pre-load left edge so we don't damage it
.leftedge:  vld      H(48++,0)*, (r0+=r1)          REP 16
            vld      H(48++,16)*, (r0+16+=r1)      REP 16
.begin:     add      r7, r8, r4
            cmp      r7, 64
            bge      .loopent

            ; pre-load right edge so we don't damage it
.rightedge: vld      H(48++,32)*, (r0+32+=r1)      REP 16
            vld      H(48++,48)*, (r0+48+=r1)      REP 16
.loopent:   sub      r8, 3 ; fiddly bodge
.loop:      addcmpblt r5, 1, 32, .noload
.load:      vld      HX(0++,0), (r2+=r3)           REP 16
            vld      HX(0++,32), (r2+32+=r3)       REP 16
            bmask    r5, 5
            add      r2, 64
.noload:    vand     HX(16,0), VX(0,0)+r5, 0xF81F
            vand     HX(16,32), VX(0,0)+r5, 0x07E0
            vmulm    V(48,RGB_ROFF)+r4*, H(16,16), 0x108
            vmulhd.su V(48,RGB_GOFF)+r4*, HX(16,32), 0x2080
            vmulm    V(48,RGB_BOFF)+r4*, H(16,0), 0x840
            addcmpblt r4, 3, 32, .nostore
.store:     addcmpbge r9, 0, 16, .easystore
            mov      r6, 0
            shl      r9, 6
            mov      r7, r0
.repn:      vst      H(48,0)+r6*, (r0)
            vst      H(48,16)+r6*, (r0+16)
            add      r0, r1
            add      r6, 64
            addcmpbne r6, 0, r9, .repn
            lsr      r9, 6
            mov      r0, r7
            b        .stored
.easystore: vst      H(48++,0)*, (r0+=r1)          REP 16
            vst      H(48++,16)*, (r0+16+=r1)      REP 16
.stored:    add      r0, 32
            bmask    r4, 5
            cbadd2
            addcmpbgt r8, -3, 63, .loop
            addcmpbgt r8, 3, 32, .rightedge
.nostore:   addcmpbge r8, -3, 0, .loop
            addcmpbeq r4, 0, 0, .end

            mov      r4, 0
            b        .store
.end:       ldm      r6-r9, (sp++)
            .cfa_pop {%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_3d32_to_rgb888
;;;
;;; SYNOPSIS
;;;   void vc_genconv_3d32_to_rgb888(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines,      r5
;;;      unsigned short const *cmap);     (sp) -> (sp+20)
;;;
;;; FUNCTION
;;;   Convert n (<= 16) lines of rgb565 to rgb888

function vc_genconv_3d32_to_rgb888 ; {{{
            stm      r6-r10, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9,%r10}
            ld       r10, (sp+20)

            mul      r8, r4, 3
            mov      r9, r5
            and      r4, r0, 31
            and      r5, r2, 31
            sub      r0, r4
            sub      r2, r5
            lsr      r5, 1
            mov      r6, r10

            ;  r4 = dst offset
            ;  r5 = src offset
            ;  r8 = columns
            ;  r9 = lines

            cbclr
            add      r5, 32-1

            addcmpblt r8, 0, 32, .leftedge
            addcmpbeq r4, 0, 0, .begin
            ; pre-load left edge so we don't damage it
.leftedge:  vld      H(48++,0)*, (r0+=r1)          REP 16
            vld      H(48++,16)*, (r0+16+=r1)      REP 16
.begin:     add      r7, r8, r4
            cmp      r7, 64
            bge      .loopent

            ; pre-load right edge so we don't damage it
.rightedge: vld      H(48++,32)*, (r0+32+=r1)      REP 16
            vld      H(48++,48)*, (r0+48+=r1)      REP 16
.loopent:   sub      r8, 3 ; fiddly bodge
.loop:      addcmpblt r5, 1, 32, .noload
.load:      vld      HX(0++,0), (r2+=r3)           REP 16
            vld      HX(0++,32), (r2+64+=r3)       REP 16

            bmask    r5, 5
            add      r2, 128
.noload:    vand     -, VX(0,0)+r5, 0xFF           CLRA ACC ; can't use V(0,0) because we need the funky indexing
            vlookupm HX(32,32), (r6)
            vlsr     H(33,32), VX(0,0)+r5, 8       ; still need funky indexing

            vand     HX(32,0), HX(32,32), 0xF81F
            vand     HX(33,0), HX(32,32), 0x07E0
            vmulm    H(34,0), H(32,16), 0x108
            vmulhd.su H(33,0), HX(33,0), 0x2080
            vmulm    H(32,0), H(32,0), 0x840

;            vmov     H(33,32), 0xff
            vmulm    V(48,RGB_ROFF)+r4*, H(34,0), H(33,32)
            vmulm    V(48,RGB_GOFF)+r4*, H(33,0), H(33,32)
            vmulm    V(48,RGB_BOFF)+r4*, H(32,0), H(33,32)

            addcmpblt r4, 3, 32, .nostore
.store:     addcmpbge r9, 0, 16, .easystore
            mov      r6, 0
            shl      r9, 6
            mov      r7, r0
.repn:      vst      H(48,0)+r6*, (r0)
            vst      H(48,16)+r6*, (r0+16)
            add      r0, r1
            add      r6, 64
            addcmpbne r6, 0, r9, .repn
            mov      r6, r10
            lsr      r9, 6
            mov      r0, r7
            b        .stored
.easystore: vst      H(48++,0)*, (r0+=r1)          REP 16
            vst      H(48++,16)*, (r0+16+=r1)      REP 16
.stored:    add      r0, 32
            bmask    r4, 5
            cbadd2
            addcmpbgt r8, -3, 63, .loop
            addcmpbgt r8, 3, 32, .rightedge
.nostore:   addcmpbge r8, -3, 0, .loop
            addcmpbeq r4, 0, 0, .end

            mov      r4, 0
            b        .store
.end:       ldm      r6-r10, (sp++)
            .cfa_pop {%r10,%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_3d32b_to_rgb888
;;;
;;; SYNOPSIS
;;;   void vc_genconv_3d32b_to_rgb888(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines);     r5
;;;
;;; FUNCTION
;;;   Convert n (<= 16) lines of rgb565 to rgb888

function vc_genconv_3d32b_to_rgb888 ; {{{
            stm      r6-r9, (--sp)
            .cfa_push {%r6,%r7,%r8,%r9}

            mul      r8, r4, 3
            mov      r9, r5
            and      r4, r0, 31
            and      r5, r2, 31
            sub      r0, r4
            sub      r2, r5
            lsr      r5, 1

            ;  r4 = dst offset
            ;  r5 = src offset
            ;  r8 = columns
            ;  r9 = lines

            cbclr
            add      r5, 32-1

            addcmpblt r8, 0, 32, .leftedge
            addcmpbeq r4, 0, 0, .begin
            ; pre-load left edge so we don't damage it
.leftedge:  vld      H(48++,0)*, (r0+=r1)          REP 16
            vld      H(48++,16)*, (r0+16+=r1)      REP 16
.begin:     add      r7, r8, r4
            cmp      r7, 64
            bge      .loopent

            ; pre-load right edge so we don't damage it
.rightedge: vld      H(48++,32)*, (r0+32+=r1)      REP 16
            vld      H(48++,48)*, (r0+48+=r1)      REP 16
.loopent:   sub      r8, 3 ; fiddly bodge
.loop:      addcmpblt r5, 1, 32, .noload
.load:      vld      HX(0++,0), (r2+=r3)           REP 16
            vld      HX(0++,32), (r2+64+=r3)       REP 16
            bmask    r5, 5
            add      r2, 128
.noload:    vand     HX(16,0), VX(0,0)+r5, 0xF81F
            vand     HX(16,32), VX(0,0)+r5, 0x07E0
            vmulm    V(48,RGB_ROFF)+r4*, H(16,16), 0x108
            vmulhd.su V(48,RGB_GOFF)+r4*, HX(16,32), 0x2080
            vmulm    V(48,RGB_BOFF)+r4*, H(16,0), 0x840
            addcmpblt r4, 3, 32, .nostore
.store:     addcmpbge r9, 0, 16, .easystore
            mov      r6, 0
            shl      r9, 6
            mov      r7, r0
.repn:      vst      H(48,0)+r6*, (r0)
            vst      H(48,16)+r6*, (r0+16)
            add      r0, r1
            add      r6, 64
            addcmpbne r6, 0, r9, .repn
            lsr      r9, 6
            mov      r0, r7
            b        .stored
.easystore: vst      H(48++,0)*, (r0+=r1)          REP 16
            vst      H(48++,16)*, (r0+16+=r1)      REP 16
.stored:    add      r0, 32
            bmask    r4, 5
            cbadd2
            addcmpbgt r8, -3, 63, .loop
            addcmpbgt r8, 3, 32, .rightedge
.nostore:   addcmpbge r8, -3, 0, .loop
            addcmpbeq r4, 0, 0, .end

            mov      r4, 0
            b        .store
.end:       ldm      r6-r9, (sp++)
            .cfa_pop {%r9,%r8,%r7,%r6}
            b        lr
endfunc ; }}}
.FORGET


   ; R = 1.164*(Y-16)                 + 1.596*(V-128)
   ; G = 1.164*(Y-16) - 0.391*(U-128) - 0.813*(V-128)
   ; B = 1.164*(Y-16) + 2.018*(U-128)

   ; so

   ; R = 1.164*Y           + 1.596*V  - (1.164*16             + 1.596*128)
   ; G = 1.164*Y - 0.391*U - 0.813*V  - (1.164*16 - 0.391*128 - 0.813*128)
   ; B = 1.164*Y + 2.018*U            - (1.164*16 + 2.018*128            )

.define YSCALE,    9539  ;;  256 * 32 * 1.1643835
.define bUSCALE,  16525  ;;  256 * 32 * 2.0172321
.define gUSCALE,  -3209  ;; -256 * 32 * 0.3917623
.define rVSCALE,  13075  ;;  256 * 32 * 1.5960267
.define gVSCALE,  -6659  ;; -256 * 32 * 0.8129676


.define RBIAS, -223     ;; -(YSCALE * 16                 + rVSCALE * 128) / 256
.define GBIAS,  135     ;; -(YSCALE * 16 - gUSCALE * 128 - gVSCALE * 128) / 256
.define BBIAS, -277     ;; -(YSCALE * 16 + rUSCALE * 128                ) / 256

   ; R = (YSCALE*Y             + rVSCALE*V) / 256 + RBIAS
   ; G = (YSCALE*Y - gUSCALE*U - gVSCALE*V) / 256 + GBIAS
   ; B = (YSCALE*Y + bUSCALE*U            ) / 256 + BBIAS

;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_yuv420_to_rgb888
;;;
;;; SYNOPSIS
;;;   void vc_genconv_yuv420_to_rgb888(
;;;      unsigned char       *dest888,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned char const *src420,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height,     /* r5 * in pixels */
;;;      unsigned char const *src_u,      /* (sp) */
;;;      unsigned char const *src_v,      /* (sp+4) */
;;;      int                  uv_pitch,   /* (sp+8) */
;;;      int                  mode);      /* (sp+12) */
;;;
;;; FUNCTION
;;;   Overlay an arbitrarily-aligned part of an RGBA565 image onto an
;;;   arbitrarily-aligned part of an RGB888 image.

function vc_genconv_yuv420_to_rgb888 ; {{{
         stm      r6-r14,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14}

         ld       r6, (sp+40)
         ld       r11, (sp+40+4)
         ld       r12, (sp+40+8)
         ld       r14, (sp+40+12)
         add      r13, pc, pcrel(.doload)
         btest    r14, 0
         beq      .evenuv
         add      r13, pc, pcrel(.doloado)
.evenuv:

         sub      r11, r6
         eor      r12, r1

         cbclr
         add      r10, r4, 1
         shl      r7, r5, 6

         and      r4, r0, 31
         and      r5, r2, 31
         bic      r0, 31 ; only necessary while we use H() accesses to store 32-byte chunks
         ror      r5, 1

         cmp      r7, 16 << 6  ; flags used to choose a store method
         add      lr, pc, pcrel(.stored)
         add      r5, 16-1

         vld      H(48++,0)*, (r0+=r1)             REP 16
         vld      H(48++,16)*, (r0+16+=r1)         REP 16
         addcmpbpl r5,0,0,.lpent ; r5 is negative if it used to be odd

         addcmpbge r10, 0, 24, .notailload ; TODO: make this correct
         vld      H(48++,32)*, (r0+32+=r1)        REP 16
         vld      H(48++,48)*, (r0+48+=r1)        REP 16
.notailload:

         add      r5, 1
         bmask    r5, 4
         eor      r1, r12
         mov      r14, r5
         add      r5, r11, r6
         bl       r13 ; load some YUV stuff
         eor      r1, r12
         mov      r5, r14
         add      r6, 16
         add      r2, 32
         add      lr, pc, pcrel(.stored) ; restore lr, corrupted above

         vadds    V(48,RGB_ROFF)+r4*, VX(32,0)+r5, RBIAS  ;; even red
         vadds    V(48,RGB_GOFF)+r4*, VX(16,0)+r5, GBIAS  ;; even green
         vadds    V(48,RGB_BOFF)+r4*, VX(0,0)+r5,  BBIAS  ;; even blue

         sub      r4, 3
         sub      r10, 1
         b        .lpent2
.align 32
.loop:   vadds    V(48,RGB_ROFF)+r4*, VX(32,0)+r5, RBIAS  ;; even red
         vadds    V(48,RGB_GOFF)+r4*, VX(16,0)+r5, GBIAS  ;; even green
         vadds    V(48,RGB_BOFF)+r4*, VX(0,0)+r5,  BBIAS  ;; even blue

         vadds    V(48,RGB_ROFF+3)+r4*, VX(32,32)+r5, RBIAS ;; odd red
         vadds    V(48,RGB_GOFF+3)+r4*, VX(16,32)+r5, GBIAS ;; odd green
         vadds    V(48,RGB_BOFF+3)+r4*, VX(0,32)+r5,  BBIAS ;; odd blue

.lpent2: addcmpblt r4, 6, 32, .nostore
         blo      .slowstore
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.stored: cbadd2
         add      r0, 32
         bmask    r4, 5
.lpent:  addcmpbge r10, 0, 24, .nostore ; TODO: make this correct
         vld      H(48++,32)*, (r0+32+=r1)        REP 16
         vld      H(48++,48)*, (r0+48+=r1)        REP 16
.nostore:

         addcmpblo r5, 1, 16, .noload
         mov      r14, r5
         add      r5, r11, r6
         eor      r1, r12
         bl       r13 ; load some YUV stuff
         eor      r1, r12
         bmask    r5, r14, 4
         add      r6, 16
         add      r2, 32
         add      lr, pc, pcrel(.stored) ; restore lr, corrupted above
.noload: 

         addcmpbgt r10, -2, 0, .loop

         addcmpbne r10, 0, 0, .noroundoff
         vadds    V(48,RGB_ROFF)+r4*, VX(32,0)+r5, RBIAS  ;; even red
         vadds    V(48,RGB_GOFF)+r4*, VX(16,0)+r5, GBIAS  ;; even green
         vadds    V(48,RGB_BOFF)+r4*, VX(0,0)+r5,  BBIAS  ;; even blue
         addcmpblt r4, 3, 32, .noroundoff
         add      lr, pc, pcrel(.stored2) ; restore lr, corrupted above
         blo      .slowstore
         vst      H(48++,0)*, (r0+=r1)             REP 16
         vst      H(48++,16)*, (r0+16+=r1)         REP 16
.stored2: cbadd2
         add      r0, 32
         bmask    r4, 5
.noroundoff:

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
         vst      H(48++,0)*, (r0+=r1)            REP 16
         vst      H(48++,16)*, (r0+16+=r1)        REP 16
.done:   ldm      r6-r14,pc, (sp++)

.align 32
.doload: vld      HX(0++,0), (r2+=r3) REP 16             ;; Y (split into odds and evens)
         vld      H(0++,32), (r6+=r1) REP 8              ;; U
         vld      H(0++,48), (r5+=r1) REP 8              ;; V

         vmulm    -,         H(0,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(24,32), H(0,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(1,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(25,32), H(1,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(2,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(26,32), H(2,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(3,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(27,32), H(3,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(4,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(28,32), H(4,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(5,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(29,32), H(5,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(6,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(30,32), H(6,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(7,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(31,32), H(7,48), gVSCALE ACC      ;; gV = V * 0.813

         vmulm    HX(32,32), H(0,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(33,32), H(0,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(34,32), H(1,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(35,32), H(1,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(36,32), H(2,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(37,32), H(2,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(38,32), H(3,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(39,32), H(3,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(40,32), H(4,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(41,32), H(4,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(42,32), H(5,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(43,32), H(5,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(44,32), H(6,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(45,32), H(6,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(46,32), H(7,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(47,32), H(7,48), -rVSCALE  ACC      ;; rV = V * 1.596

         vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
         vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

         ;; even greens
         vadd     HX(16++,0), HX(0++,0),  HX(24,32) REP 2
         vadd     HX(18++,0), HX(2++,0),  HX(25,32) REP 2
         vadd     HX(20++,0), HX(4++,0),  HX(26,32) REP 2
         vadd     HX(22++,0), HX(6++,0),  HX(27,32) REP 2
         vadd     HX(24++,0), HX(8++,0),  HX(28,32) REP 2
         vadd     HX(26++,0), HX(10++,0), HX(29,32) REP 2
         vadd     HX(28++,0), HX(12++,0), HX(30,32) REP 2
         vadd     HX(30++,0), HX(14++,0), HX(31,32) REP 2
         ;; even blues
         vadd     HX(0++,0),  HX(0++,0),  HX(32,32) REP 2
         vadd     HX(2++,0),  HX(2++,0),  HX(34,32) REP 2
         vadd     HX(4++,0),  HX(4++,0),  HX(36,32) REP 2
         vadd     HX(6++,0),  HX(6++,0),  HX(38,32) REP 2
         vadd     HX(8++,0),  HX(8++,0),  HX(40,32) REP 2
         vadd     HX(10++,0), HX(10++,0), HX(42,32) REP 2
         vadd     HX(12++,0), HX(12++,0), HX(44,32) REP 2
         vadd     HX(14++,0), HX(14++,0), HX(46,32) REP 2
         ;; even reds
         vsub     HX(32++,0), HX(0++,0),  HX(33,32) REP 2
         vsub     HX(34++,0), HX(2++,0),  HX(35,32) REP 2
         vsub     HX(36++,0), HX(4++,0),  HX(37,32) REP 2
         vsub     HX(38++,0), HX(6++,0),  HX(39,32) REP 2
         vsub     HX(40++,0), HX(8++,0),  HX(41,32) REP 2
         vsub     HX(42++,0), HX(10++,0), HX(43,32) REP 2
         vsub     HX(44++,0), HX(12++,0), HX(45,32) REP 2
         vsub     HX(46++,0), HX(14++,0), HX(47,32) REP 2

         ;; odd greens
         vadd     HX(16++,32), HX(0++,32),  HX(24,32) REP 2
         vadd     HX(18++,32), HX(2++,32),  HX(25,32) REP 2
         vadd     HX(20++,32), HX(4++,32),  HX(26,32) REP 2
         vadd     HX(22++,32), HX(6++,32),  HX(27,32) REP 2
         vadd     HX(24++,32), HX(8++,32),  HX(28,32) REP 2
         vadd     HX(26++,32), HX(10++,32), HX(29,32) REP 2
         vadd     HX(28++,32), HX(12++,32), HX(30,32) REP 2
         vadd     HX(30++,32), HX(14++,32), HX(31,32) REP 2
         ;; odd blues
         vadd     HX(0++,32),  HX(0++,32),  HX(32,32) REP 2
         vadd     HX(2++,32),  HX(2++,32),  HX(34,32) REP 2
         vadd     HX(4++,32),  HX(4++,32),  HX(36,32) REP 2
         vadd     HX(6++,32),  HX(6++,32),  HX(38,32) REP 2
         vadd     HX(8++,32),  HX(8++,32),  HX(40,32) REP 2
         vadd     HX(10++,32), HX(10++,32), HX(42,32) REP 2
         vadd     HX(12++,32), HX(12++,32), HX(44,32) REP 2
         vadd     HX(14++,32), HX(14++,32), HX(46,32) REP 2
         ;; odd reds
         vsub     HX(32++,32), HX(0++,32),  HX(33,32) REP 2
         vsub     HX(34++,32), HX(2++,32),  HX(35,32) REP 2
         vsub     HX(36++,32), HX(4++,32),  HX(37,32) REP 2
         vsub     HX(38++,32), HX(6++,32),  HX(39,32) REP 2
         vsub     HX(40++,32), HX(8++,32),  HX(41,32) REP 2
         vsub     HX(42++,32), HX(10++,32), HX(43,32) REP 2
         vsub     HX(44++,32), HX(12++,32), HX(45,32) REP 2
         vsub     HX(46++,32), HX(14++,32), HX(47,32) REP 2

         vasr     HX(0++,0), HX(0++,0), 5       REP 16
         vasr     HX(0++,32), HX(0++,32), 5     REP 16
         vasr     HX(16++,0), HX(16++,0), 5     REP 32
         vasr     HX(16++,32), HX(16++,32), 5   REP 32

         b        lr

.align 32
.doloado:vld      HX(0++,0), (r2+=r3) REP 16             ;; Y (split into odds and evens)
         vld      H(0++,32), (r6+=r1) REP 8              ;; U
         vld      H(0++,48), (r5+=r1) REP 8              ;; V

         vmulm    -,         H(0,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(24,32), H(0,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(1,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(25,32), H(1,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(2,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(26,32), H(2,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(3,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(27,32), H(3,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(4,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(28,32), H(4,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(5,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(29,32), H(5,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(6,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(30,32), H(6,48), gVSCALE ACC      ;; gV = V * 0.813
         vmulm    -,         H(7,32), gUSCALE CLRA ACC ;; gU = U * 0.391
         vmulm    HX(31,32), H(7,48), gVSCALE ACC      ;; gV = V * 0.813

         vmulm    HX(32,32), H(0,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(33,32), H(0,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(34,32), H(1,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(35,32), H(1,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(36,32), H(2,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(37,32), H(2,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(38,32), H(3,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(39,32), H(3,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(40,32), H(4,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(41,32), H(4,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(42,32), H(5,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(43,32), H(5,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(44,32), H(6,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(45,32), H(6,48), -rVSCALE  ACC      ;; rV = V * 1.596
         vmulm    HX(46,32), H(7,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(47,32), H(7,48), -rVSCALE  ACC      ;; rV = V * 1.596

         vmulm    HX(0++,32), H(0++,16), YSCALE REP 16   ;; Y *= 1.164
         vmulm    HX(0++,0),  H(0++,0),  YSCALE REP 16   ;; Y *= 1.164

         ;; even greens
         vadd     HX(16  ,0), HX(0  ,0),  HX(24,32)
         vadd     HX(17++,0), HX(1++,0),  HX(25,32) REP 2
         vadd     HX(19++,0), HX(3++,0),  HX(26,32) REP 2
         vadd     HX(21++,0), HX(5++,0),  HX(27,32) REP 2
         vadd     HX(23++,0), HX(7++,0),  HX(28,32) REP 2
         vadd     HX(25++,0), HX(9++,0),  HX(28,32) REP 2
         vadd     HX(27++,0), HX(11++,0), HX(30,32) REP 2
         vadd     HX(29++,0), HX(13++,0), HX(31,32) REP 2
         ;; even blues
         vadd     HX(0  ,0),  HX(0  ,0),  HX(32,32)
         vadd     HX(1++,0),  HX(1++,0),  HX(34,32) REP 2
         vadd     HX(3++,0),  HX(3++,0),  HX(36,32) REP 2
         vadd     HX(5++,0),  HX(5++,0),  HX(38,32) REP 2
         vadd     HX(7++,0),  HX(7++,0),  HX(40,32) REP 2
         vadd     HX(9++,0),  HX(9++,0),  HX(42,32) REP 2
         vadd     HX(11++,0), HX(11++,0), HX(44,32) REP 2
         vadd     HX(13++,0), HX(13++,0), HX(46,32) REP 2
         ;; even reds
         vsub     HX(32  ,0), HX(0  ,0),  HX(33,32)
         vsub     HX(33++,0), HX(1++,0),  HX(35,32) REP 2
         vsub     HX(35++,0), HX(3++,0),  HX(37,32) REP 2
         vsub     HX(37++,0), HX(5++,0),  HX(39,32) REP 2
         vsub     HX(39++,0), HX(7++,0),  HX(41,32) REP 2
         vsub     HX(41++,0), HX(9++,0),  HX(43,32) REP 2
         vsub     HX(43++,0), HX(11++,0), HX(45,32) REP 2
         vsub     HX(45++,0), HX(13++,0), HX(47,32) REP 2

         ;; odd greens
         vadd     HX(16  ,32), HX(0  ,32),  HX(24,32)
         vadd     HX(17++,32), HX(1++,32),  HX(25,32) REP 2
         vadd     HX(19++,32), HX(3++,32),  HX(26,32) REP 2
         vadd     HX(21++,32), HX(5++,32),  HX(27,32) REP 2
         vadd     HX(23++,32), HX(7++,32),  HX(28,32) REP 2
         vadd     HX(25++,32), HX(9++,32),  HX(28,32) REP 2
         vadd     HX(27++,32), HX(11++,32), HX(30,32) REP 2
         vadd     HX(29++,32), HX(13++,32), HX(31,32) REP 2
         ;; odd blues
         vadd     HX(0  ,32),  HX(0  ,32),  HX(32,32)
         vadd     HX(1++,32),  HX(1++,32),  HX(34,32) REP 2
         vadd     HX(3++,32),  HX(3++,32),  HX(36,32) REP 2
         vadd     HX(5++,32),  HX(5++,32),  HX(38,32) REP 2
         vadd     HX(7++,32),  HX(7++,32),  HX(40,32) REP 2
         vadd     HX(9++,32),  HX(9++,32),  HX(42,32) REP 2
         vadd     HX(11++,32), HX(11++,32), HX(44,32) REP 2
         vadd     HX(13++,32), HX(13++,32), HX(46,32) REP 2
         ;; odd reds
         vsub     HX(32  ,32), HX(0  ,32),  HX(33,32)
         vsub     HX(33++,32), HX(1++,32),  HX(35,32) REP 2
         vsub     HX(35++,32), HX(3++,32),  HX(37,32) REP 2
         vsub     HX(37++,32), HX(5++,32),  HX(39,32) REP 2
         vsub     HX(39++,32), HX(7++,32),  HX(41,32) REP 2
         vsub     HX(41++,32), HX(9++,32),  HX(43,32) REP 2
         vsub     HX(43++,32), HX(11++,32), HX(45,32) REP 2
         vsub     HX(45++,32), HX(13++,32), HX(47,32) REP 2

         addscale r5, r1 << 3
         addscale r6, r1 << 3
         shl      r1, 3

         vld      H(31,32), (r6)                         ;; U
         vld      H(47,32), (r5)                         ;; V

         sub      r6, r1
         lsr      r1, 3

         vmulm    -,        H(31,32), gUSCALE   CLRA ACC ;; gU = U * 0.391
         vmulm    HX(31,0), H(47,32), gVSCALE   ACC      ;; gUV = gU + V * 0.813

         vmulm    HX(31,32), H(31,32), bUSCALE   CLRA ACC ;; bU = U * 2.018
         vmulm    HX(47,32), H(47,32), -rVSCALE  ACC      ;; rV = V * 1.596

         ;; odd green calculate
         vadd     -,           HX(15  ,32), HX(31,0) CLRA ACC

         ;; even green
         vadd     HX(31  ,0), HX(15  ,0), HX(31, 0)
         ;; even blue
         vadd     HX(15  ,0), HX(15  ,0), HX(31,32)
         ;; even red
         vsub     HX(47  ,0), HX(15  ,0), HX(47,32)

         ;; odd blue
         vadd     HX(15  ,32), HX(15  ,32), HX(31,32)
         ;; odd reds
         vsub     HX(47  ,32), HX(15  ,32), HX(47,32)

         ;; odd green store
         vmov     HX(31  ,32), 0 ACC

         vasr     HX(0++,0), HX(0++,0), 5       REP 16
         vasr     HX(0++,32), HX(0++,32), 5     REP 16
         vasr     HX(16++,0), HX(16++,0), 5     REP 32
         vasr     HX(16++,32), HX(16++,32), 5   REP 32

         b        lr

         .cfa_pop {%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
endfunc ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_8bpp_to_rgb888
;;;
;;; SYNOPSIS
;;;   void vc_genconv_8bpp_to_rgb888(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines,      r5
;;;      short      *vc_image_palette);  (sp)
;;;
;;; FUNCTION
;;;   Convert n (<= 16) lines of 8bpp to rgb888

function vc_genconv_8bpp_to_rgb888 ; {{{
            stm      r6-r9,(--sp)
            .cfa_push {%r6,%r7,%r8,%r9}

            mul      r8, r4, 3
            mov      r9, r5
            and      r4, r0, 31
            and      r5, r2, 31
            sub      r0, r4
            sub      r2, r5

            ;  r4 = dst offset
            ;  r5 = src offset
            ;  r8 = columns
            ;  r9 = lines

            cbclr
            add      r5, 32-1

            addcmpblt r8, 0, 32, .leftedge
            addcmpbeq r4, 0, 0, .begin
            ; pre-load left edge so we don't damage it
.leftedge:  vld      H(48++,0)*, (r0+=r1)          REP 16
            vld      H(48++,16)*, (r0+16+=r1)      REP 16
.begin:     add      r7, r8, r4
            cmp      r7, 64
            bge      .loopent

            ; pre-load right edge so we don't damage it
.rightedge: vld      H(48++,32)*, (r0+32+=r1)      REP 16
            vld      H(48++,48)*, (r0+48+=r1)      REP 16
.loopent:   sub      r8, 3 ; fiddly bodge
.loop:      addcmpblt r5, 1, 32, .noload
.load:      
            vld      H(0++,0), (r2+=r3)            REP 16
            vld      H(0++,32), (r2+16+=r3)        REP 16
            ld       r6, (sp+16) ; palette
.irep index, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
            vmov -, H(index,0) CLRA ACC
            vlookupm HX(index,0), (r6)
            vmov -, H(index,32) CLRA ACC
            vlookupm HX(index,32), (r6)
.endr
            bmask    r5, 5
            add      r2, 32
.noload:    vand     HX(16,0), VX(0,0)+r5, 0xF81F
            vand     HX(16,32), VX(0,0)+r5, 0x07E0
            vmulm    V(48,RGB_ROFF)+r4*, H(16,16), 0x108
            vmulhd.su V(48,RGB_GOFF)+r4*, HX(16,32), 0x2080
            vmulm    V(48,RGB_BOFF)+r4*, H(16,0), 0x840
            addcmpblt r4, 3, 32, .nostore
.store:     addcmpbge r9, 0, 16, .easystore
            mov      r6, 0
            shl      r9, 6
            mov      r7, r0
.repn:      vst      H(48,0)+r6*, (r0)
            vst      H(48,16)+r6*, (r0+16)
            add      r0, r1
            add      r6, 64
            addcmpbne r6, 0, r9, .repn
            lsr      r9, 6
            mov      r0, r7
            b        .stored
.easystore: vst      H(48++,0)*, (r0+=r1)          REP 16
            vst      H(48++,16)*, (r0+16+=r1)      REP 16
.stored:    add      r0, 32
            bmask    r4, 5
            cbadd2
            addcmpbgt r8, -3, 63, .loop
            addcmpbgt r8, 3, 32, .rightedge
.nostore:   addcmpbge r8, -3, 0, .loop
            addcmpbeq r4, 0, 0, .end

            mov      r4, 0
            b        .store
.end:       ldm      r6-r9,(sp++)
            .cfa_pop {%r9,%r8,%r7,%r6}
            b lr
endfunc ; }}}
.FORGET

.define Rsrc, r2
.define Rwidth, r4
.define Rsrcpitch, r3
.define Rdst, r0
.define Rdstpitch, r1
.define Rtemp, r6         ; general use by vector instructions
.define Rshift, r7
.define Rorigheight, r8
.define Rsrcinc, r9
.define Rdstinc, r10
.define Rlookup, r11
.define Rheight, r12

.define N_src, 0
.define N_dst, 1
.define N_trans, 2
.define N_colortab, 3
.define N_enable, 4
.define N_shift, 5
.define N_colorbase, 6
.define N_pmask, 7
.define N_temp, 8
.define N_one, 9
.define N_thirtyone, 10

.define STACK_ARG_0, 28

;;; *****************************************************************************
;;; NAME
;;;   vc_genconv_8bpp_to_rgb565
;;;
;;; SYNOPSIS
;;;   void vc_genconv_8bpp_to_rgb565(
;;;      unsigned char       *dest,       r0
;;;      int                  dpitch,     r1
;;;      unsigned char const *src,        r2
;;;      int                  spitch,     r3
;;;      int                  columns,    r4
;;;      int                  lines,      r5
;;;      short      *vc_image_palette);  (sp)
;;;
;;; FUNCTION
;;;   Convert n (<= 16) lines of 8bpp to rgb565

function vc_genconv_8bpp_to_rgb565 ; {{{
   stm r6-r12, (--sp)
   .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12}
   mov Rheight, r5
   mov Rorigheight, Rheight
   mul Rsrcinc, Rheight, Rsrcpitch
   mul Rdstinc, Rheight, Rdstpitch
   rsub Rsrcinc, 16
   rsub Rdstinc, 16
   add Rdstinc, 16
1:
   mov Rtemp, -1
   min Rshift, Rwidth, 16
   shl Rtemp, Rshift
   vbitplanes H(N_enable,0), Rtemp
2:
   veor -, H(N_enable,0), 0xff SETF          ; now Z for ignored
   vldleft  H(N_src,0), (Rsrc)
;   vldright H(N_src,0), (Rsrc)
   vmov -, H(N_src,0) CLRA ACC
   ld Rtemp, (sp+STACK_ARG_0) ; paldata
   vlookupm HX(N_src,0), (Rtemp) IFNZ
   nop
   nop
   vstleft  HX(N_src,0), (Rdst) IFNZ
;   vstright HX(N_src,0), (Rdst) IFNZ
   add Rsrc, Rsrcpitch
   add Rdst, Rdstpitch
   addcmpbgt Rheight, -1, 0, 2b

   add Rsrc, Rsrcinc
   add Rdst, Rdstinc
   mov Rheight, Rorigheight
   sub Rwidth, 16
   addcmpbgt Rwidth, 0, 0, 1b

   ldm r6-r12, (sp++)
   .cfa_pop {%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   b lr
endfunc ; }}}
.FORGET


function vc_genconv_3d32_to_rgb565 ; {{{
   stm r6-r12, (--sp)
   .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12}
   mov Rheight, r5
   mov Rorigheight, Rheight
   mul Rsrcinc, Rheight, Rsrcpitch
   mul Rdstinc, Rheight, Rdstpitch
   rsub Rsrcinc, 16
   add Rsrcinc, 48
   rsub Rdstinc, 16
   add Rdstinc, 16
1:
   mov Rtemp, -1
   min Rshift, Rwidth, 16
   shl Rtemp, Rshift
   vbitplanes H(N_enable,0), Rtemp
2:
   veor -, H(N_enable,0), 0xff SETF          ; now Z for ignored
   vldleft  HX(N_src,0), (Rsrc)
;   vldright HX(N_src,0), (Rsrc)
   vmov -, H(N_src,0) CLRA ACC
   ld Rtemp, (sp+STACK_ARG_0) ; paldata
   vlookupm HX(N_src,0), (Rtemp) IFNZ
   nop
   nop
   vstleft  HX(N_src,0), (Rdst) IFNZ
;   vstright HX(N_src,0), (Rdst) IFNZ
   add Rsrc, Rsrcpitch
   add Rdst, Rdstpitch
   addcmpbgt Rheight, -1, 0, 2b

   add Rsrc, Rsrcinc
   add Rdst, Rdstinc
   mov Rheight, Rorigheight
   sub Rwidth, 16
   addcmpbgt Rwidth, 0, 0, 1b

   ldm r6-r12, (sp++)
   .cfa_pop {%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   b lr
endfunc ; }}}
.FORGET

function vc_genconv_3d32b_to_rgb565 ; {{{
   stm r6-r12, (--sp)
   .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12}
   mov Rheight, r5
   mov Rorigheight, Rheight
   mul Rsrcinc, Rheight, Rsrcpitch
   mul Rdstinc, Rheight, Rdstpitch
   rsub Rsrcinc, 16
   add Rsrcinc, 48
   rsub Rdstinc, 16
   add Rdstinc, 16
1:
   mov Rtemp, -1
   min Rshift, Rwidth, 16
   shl Rtemp, Rshift
   vbitplanes H(N_enable,0), Rtemp
2:
   veor -, H(N_enable,0), 0xff SETF          ; now Z for ignored
   vldleft  HX(N_src,0), (Rsrc)
;   vldright HX(N_src,0), (Rsrc)
   vmov -, H(N_src,0) CLRA ACC
   nop
   nop
   vstleft  HX(N_src,0), (Rdst) IFNZ
;   vstright HX(N_src,0), (Rdst) IFNZ
   add Rsrc, Rsrcpitch
   add Rdst, Rdstpitch
   addcmpbgt Rheight, -1, 0, 2b

   add Rsrc, Rsrcinc
   add Rdst, Rdstinc
   mov Rheight, Rorigheight
   sub Rwidth, 16
   addcmpbgt Rwidth, 0, 0, 1b

   ldm r6-r12, (sp++)
   .cfa_pop {%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   b lr
endfunc ; }}}
.FORGET

