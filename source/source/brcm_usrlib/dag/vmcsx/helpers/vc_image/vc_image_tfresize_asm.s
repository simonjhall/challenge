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

.include "vcinclude/common.inc"


;;; *****************************************************************************
;;; NAME
;;;   tformat_resize_stripe
;;;
;;; SYNOPSIS
;;;   void tformat_resize_stripe(
;;;      void                *dptr0,      /* r0                 */
;;;      int                  dpitch,     /* r1                 */
;;;      int                  x,          /* r2                 */
;;;      float                xinc,       /* r3                 */
;;;      int                  y,          /* r4                 */
;;;      float                yinc,       /* r5                 */
;;;      int                  tu2offset,  /* (sp)    -> (sp+44) */
;;;      int                  width,      /* (sp+4)  -> (sp+48) */
;;;      void                *dptr1);     /* (sp+8)  -> (sp+52) */
;;;
;;; FUNCTION
;;;   stuff

.function tformat_resize_stripe ; {{{
.ldefine TU_A, 0
.ldefine TU_B, 16
.ldefine READAHEAD, 4

         stm      r6-r15,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

.ifndef NDEBUG
         vmov     H32(0++,0), -1 REP 64
.endif

         add      r7, pc, pcrel(.onetwothree)
         vld      H16(31,0), (r7)

         inttofloat r4, r4, 0
         shl      r2, 1
         or       r2, 1
         vmov     H32(30,0), r2
         vinttofloat H32(30,0), H32(30,0), 1
         vfadd    H32(31,0), H16(31,0), r4

.alias r0, dptr0
.alias r1, pitch
.alias r2, dptr1
.alias r3, xinc
.alias r4, tu2offset
.alias r5, yinc
;.alias r6, arg1
;.alias r7, arg2
.alias r8, tu
.alias r9, width
.alias r10, outidx

         ld       tu2offset, (sp+44)
         ld       width, (sp+48)
;         ld       arg1, (sp+60)
;         ld       arg2, (sp+64)
         mov      tu, 0x2000

         ld       dptr1, (sp+52)
         inttofloat tu2offset, tu2offset, 0
         fmul     tu2offset, tu2offset, xinc

         bic      width, 15
         mov      r12, 0

         b .loop
         .align 32

.loop:   ; preconditioning
         vfmul    H32(0,0), H32(30,0), xinc
         vfmul    H32(4++,0), H32(31,0), yinc            REP 4 ; Y coordinates
         vfadd    H32(2,0), H32(0,0), tu2offset
         vfadd    H32(1,0), H32(0,0), xinc
         vfadd    H32(3,0), H32(2,0), xinc
         vfadd    H32(30,0), H32(30,0), FLOAT16(2.0)

.ifdef __BCM2707A0__
         ; texture lookups
         vinterl  H32(TU_A+1,0)+tu, H32(4,0), H32(5,0)  ; Y coordinates lines 0..7
         vinterl  H32(TU_A+0,0)+tu, H32(0,0), H32(1,0)  ; X coordinates lines 0..7
         vinterh  H32(TU_A+1,0)+tu, H32(4,0), H32(5,0)  ; Y coordinates lines 8..15
         vinterh  H32(TU_A+0,0)+tu, H32(0,0), H32(1,0)  ; X coordinates lines 8..15
         vinterl  H32(TU_B+1,0)+tu, H32(6,0), H32(7,0)  ; Y coordinates lines 0..7
         vinterl  H32(TU_B+0,0)+tu, H32(2,0), H32(3,0)  ; X coordinates lines 0..7
         vinterh  H32(TU_B+1,0)+tu, H32(6,0), H32(7,0)  ; Y coordinates lines 8..15
         vinterh  H32(TU_B+0,0)+tu, H32(2,0), H32(3,0)  ; X coordinates lines 8..15
.else
         ; texture lookups
         vinterl  H32(TU_A+0   ,0)+tu, H32(0,0), H32(1,0)  ; X coordinates lines 0..7
         vinterl  H32(TU_A+1+32,0)+tu, H32(4,0), H32(5,0)  ; Y coordinates lines 0..7
         vinterh  H32(TU_A+0   ,0)+tu, H32(0,0), H32(1,0)  ; X coordinates lines 8..15
         vinterh  H32(TU_A+1+32,0)+tu, H32(4,0), H32(5,0)  ; Y coordinates lines 8..15
         vinterl  H32(TU_B+0   ,0)+tu, H32(2,0), H32(3,0)  ; X coordinates lines 0..7
         vinterl  H32(TU_B+1+32,0)+tu, H32(6,0), H32(7,0)  ; Y coordinates lines 0..7
         vinterh  H32(TU_B+0   ,0)+tu, H32(2,0), H32(3,0)  ; X coordinates lines 8..15
         vinterh  H32(TU_B+1+32,0)+tu, H32(6,0), H32(7,0)  ; Y coordinates lines 8..15
.endif

         addcmpbls r12, 2, READAHEAD, .nostore ; predict not-taken
.ifdef __BCM2707A0__
.pickup: v32mov   H32(28,0), H32(TU_A+0,0)+tu
         mov      r14, outidx
         v32mov   H32(29,0), H32(TU_A+0,0)+tu
.else
.pickup: v32mov   H32(28,0), H32(TU_A+32,0)+tu
         mov      r14, outidx
         v32mov   H32(29,0), H32(TU_A+32,0)+tu
.endif
         add      outidx, 2
         v32even  V32(32,0)+r14, H32(28,0), H32(29,0)
         cmp      outidx, r13
         v32odd   V32(32,1)+r14, H32(28,0), H32(29,0)
.ifdef __BCM2707A0__
         v32mov   H32(28,0), H32(TU_B+0,0)+tu
         v32mov   H32(29,0), H32(TU_B+0,0)+tu
.else
         v32mov   H32(28,0), H32(TU_B+32,0)+tu
         v32mov   H32(29,0), H32(TU_B+32,0)+tu
.endif
         v32even  V32(48,0)+r14, H32(28,0), H32(29,0)
         v32odd   V32(48,1)+r14, H32(28,0), H32(29,0)

         bne      .loop

.store:  addcmpbls r13, 2, 16, .bounce ; predict not-taken

         vsubs    H8(0++,0), H8(32++,0), 0               REP 16
         vsubs    H8(0++,16), H8(32++,16), 0             REP 16
         vsubs    H8(0++,32), H8(32++,32), 0             REP 16
         vmov     H8(0++,48), H8(32++,48)                REP 16

      mov      r11, 0
      mov      r13, -1
.tf1loop:
      add      r13, 1
      v32interl   H32(20,0), V32(0,0)+r11, V32(0,2)+r11
      v32interl   H32(21,0), V32(0,1)+r11, V32(0,3)+r11
      v32interh   H32(22,0), V32(0,0)+r11, V32(0,2)+r11
      v32interh   H32(23,0), V32(0,1)+r11, V32(0,3)+r11
      add      r11, 4
      v32interl   V32(32,0)+r13, H32(20,0), H32(21,0)
      cmp      r11, 16
      v32interh   V32(32,4)+r13, H32(20,0), H32(21,0)
      v32interl   V32(32,8)+r13, H32(22,0), H32(23,0)
      v32interh   V32(32,12)+r13, H32(22,0), H32(23,0)
      bne      .tf1loop

         mov      r13, 64
         v32st    V32(32,0++), (dptr0+=r13)              REP 16
         add      dptr0, pitch

         vsubs    H8(0++,0), H8(48++,0), 0               REP 16
         vsubs    H8(0++,16), H8(48++,16), 0             REP 16
         vsubs    H8(0++,32), H8(48++,32), 0             REP 16
         vmov     H8(0++,48), H8(48++,48)                REP 16

      mov      r11, 0
      mov      r13, -1
.tf2loop:
      add      r13, 1
      v32interl   H32(20,0), V32(0,0)+r11, V32(0,2)+r11
      v32interl   H32(21,0), V32(0,1)+r11, V32(0,3)+r11
      v32interh   H32(22,0), V32(0,0)+r11, V32(0,2)+r11
      v32interh   H32(23,0), V32(0,1)+r11, V32(0,3)+r11
      add      r11, 4
      v32interl   V32(48,0)+r13, H32(20,0), H32(21,0)
      cmp      r11, 16
      v32interh   V32(48,4)+r13, H32(20,0), H32(21,0)
      v32interl   V32(48,8)+r13, H32(22,0), H32(23,0)
      v32interh   V32(48,12)+r13, H32(22,0), H32(23,0)
      bne      .tf2loop

         mov      r13, 64
         v32st    V32(48,0++), (dptr1+=r13)              REP 16
         add      dptr1, pitch
         eor      pitch, 2048

.nostore:sub      r13, width, r12
         mov      outidx, 0
         min      r13, 16
         addcmpbhs r12, 0, width, .exit ; predict not-taken
         b        .loop
.exit:   ldm      r6-r15,pc, (sp++)

.bounce: b .pickup
         .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}

.free$width
.free$outidx
.free$tu
;.free$arg2
;.free$arg1
.free$yinc
.free$tu2offset
.free$xinc
.free$dptr1
.free$pitch
.free$dptr0

         .align   32
.onetwothree:     .half FLOAT16(0.5), FLOAT16(1.5), FLOAT16(2.5), FLOAT16(3.5)
                  .half FLOAT16(4.5), FLOAT16(5.5), FLOAT16(6.5), FLOAT16(7.5)
                  .half FLOAT16(8.5), FLOAT16(9.5), FLOAT16(10.5), FLOAT16(11.5)
                  .half FLOAT16(12.5), FLOAT16(13.5), FLOAT16(14.5), FLOAT16(15.5)
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   tformat_resize_utile_stripe
;;;
;;; SYNOPSIS
;;;   void tformat_resize_utile_stripe(
;;;      void                *dptr0,      /* r0                 */
;;;      int                  dpitch,     /* r1                 */
;;;      int                  x,          /* r2                 */
;;;      float                xinc,       /* r3                 */
;;;      int                  y,          /* r4                 */
;;;      float                yinc,       /* r5                 */
;;;      int                  tu2offset,  /* (sp)    -> (sp+40) */
;;;      int                  width,      /* (sp+4)  -> (sp+44) */
;;;      void                *dptr1);     /* (sp+8)  -> (sp+48) */
;;;
;;; FUNCTION
;;;   stuff
;;;
;;; NOTES
;;;   apologies for the un-pipelined implementation

.function tformat_resize_utile_stripe ; {{{
.ldefine TU_A, 0
.ldefine TU_B, 16
.ldefine READAHEAD, 4

         stm      r6-r15, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15}

.ifndef NDEBUG
         vmov     H32(0++,0), -1 REP 64
.endif

         add      r7, pc, pcrel(.x_values)
         vld      H16(30,0), (r7)
         vld      H16(31,0), (r7+32)

         inttofloat r2, r2, 0
         inttofloat r4, r4, 0
         vfadd    H32(30,0), H16(30,0), r2
         vfadd    H32(31,0), H16(31,0), r4

.alias r0, dptr0
.alias r1, pitch
.alias r2, dptr1
.alias r3, xinc
.alias r4, tu2offset
.alias r5, yinc
;.alias r6, arg1
;.alias r7, arg2
.alias r8, tu
.alias r9, width
.alias r10, outidx

         ld       tu2offset, (sp+40)
         ld       width, (sp+44)
         mov      tu, 0x2000

         ld       dptr1, (sp+48)
         inttofloat tu2offset, tu2offset, 0
         fmul     tu2offset, tu2offset, xinc

         bic      width, 3
         mov      r12, 0

         b .loop
         .align 32

.loop:   ; preconditioning
         vfmul    H32(0,0), H32(30,0), xinc
         vfadd    H32(30,0), H32(30,0), FLOAT16(4.0)

.ifdef __BCM2707A0__
         ; texture lookups
         vfmul    H32(TU_A+1,0)+tu, H32(31,0), yinc
         vfadd    H32(TU_A+0,0)+tu, H32(0,0), 0
         vfmul    H32(TU_B+1,0)+tu, H32(31,0), yinc
         vfadd    H32(TU_B+0,0)+tu, H32(0,0), tu2offset

.pickup: v32mov   H32(28,0), H32(TU_A+0,0)+tu
         v32mov   H32(29,0), H32(TU_B+0,0)+tu
.else
         ; texture lookups
         vfadd    H32(TU_A+0   ,0)+tu, H32(0,0), 0
         vfmul    H32(TU_A+1+32,0)+tu, H32(31,0), yinc
         vfadd    H32(TU_B+0   ,0)+tu, H32(0,0), tu2offset
         vfmul    H32(TU_B+1+32,0)+tu, H32(31,0), yinc

.pickup: v32mov   H32(28,0), H32(TU_A+32,0)+tu
         v32mov   H32(29,0), H32(TU_B+32,0)+tu
.endif

         mov      r14, 1
         veven    H32(32,0), H32(28,0), H32(28,0)+r14
         veven    H32(48,0), H32(29,0), H32(29,0)+r14

         mov      r13, 4
         mov      r14, 8

         vbitplanes -, 0x000f                                  SETF
         veor     H32(32,0)+r13, H32(32,0)+r13, H32(32,0)+r14  IFNZ
         veor     H32(32,0)+r14, H32(32,0)+r14, H32(32,0)+r13  IFNZ
         veor     H32(32,0)+r13, H32(32,0)+r13, H32(32,0)+r14  IFNZ
         veor     H32(48,0)+r13, H32(48,0)+r13, H32(48,0)+r14  IFNZ
         veor     H32(48,0)+r14, H32(48,0)+r14, H32(48,0)+r13  IFNZ
         veor     H32(48,0)+r13, H32(48,0)+r13, H32(48,0)+r14  IFNZ

.if 0
         vbitplanes -, 0xCC33                                  SETF
         mov      r13, 0xFF0000FF
         mov      r14, 0xFFFF0000
         vor      H32(32,0), H32(32,0), r13                    IFNZ
         vor      H32(48,0), H32(48,0), r14                    IFNZ
.endif

         v32st    H32(32,0), (dptr0)
         add      dptr0, pitch

         v32st    H32(48,0), (dptr1)
         add      dptr1, pitch

         addcmpbhs r12, 4, width, .exit ; predict not-taken
         b        .loop
.exit:   ldm      r6-r15, (sp++)
         .cfa_pop {%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
         b        lr

.free$width
.free$outidx
.free$tu
;.free$arg2
;.free$arg1
.free$yinc
.free$tu2offset
.free$xinc
.free$dptr1
.free$pitch
.free$dptr0

         .align   32
.x_values:        .half FLOAT16(0.5), FLOAT16(0.5), FLOAT16(1.5), FLOAT16(1.5)
                  .half FLOAT16(2.5), FLOAT16(2.5), FLOAT16(3.5), FLOAT16(3.5)
                  .half FLOAT16(0.5), FLOAT16(0.5), FLOAT16(1.5), FLOAT16(1.5)
                  .half FLOAT16(2.5), FLOAT16(2.5), FLOAT16(3.5), FLOAT16(3.5)
.y_values:        .half FLOAT16(0.5), FLOAT16(1.5), FLOAT16(0.5), FLOAT16(1.5)
                  .half FLOAT16(0.5), FLOAT16(1.5), FLOAT16(0.5), FLOAT16(1.5)
                  .half FLOAT16(2.5), FLOAT16(3.5), FLOAT16(2.5), FLOAT16(3.5)
                  .half FLOAT16(2.5), FLOAT16(3.5), FLOAT16(2.5), FLOAT16(3.5)
.endfn ; }}}
.FORGET

;; TODO: clean out more cruft
;; TODO: make some vague attempt at optimising the code 
;; TODO: add tformat_resize_utile_stripe
;; TODO: add 565 versions of these functions
;; TODO: add 4444 versions of these functions
;; TODO: add 5551 versions of these functions
;; TODO: add any other useful versions of these functions
