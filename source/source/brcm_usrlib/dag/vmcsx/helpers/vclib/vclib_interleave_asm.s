;;; ===========================================================================
;;; Copyright (c) 2009-2014, Broadcom Corporation
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
;;;   vclib_interleave2_2
;;;
;;; SYNOPSIS
;;;   void vclib_interleave2_2(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      int                  count);  /* r3 */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave2_2 ; {{{
         stm      r6-r7,(--sp)
         .cfa_push{%r6,%r7}

         v32mov   H32(0++,0), 0                    REP 16

         lsr      r7, r3, 4
         bmask    r5, r3, 4
         mov      r6, 0

         cmp      r1, 0
         bitset.ne r6, 0
         cmp      r2, 0
         bitset.ne r6, 1

         addscale r3, r2, r7 * 32      ; src1
         addscale r2, r1, r7 * 32      ; src0
         addscale r1, r0, r7 * 64      ; dst

         mov      r0, -1
         cmp      r5, 0
         shl      r0, r5
         vbitplanes -, r0                          SETF
         mov      r0, 1
         mov      r4, 32 
         mov      r5, 64
         add.ne   r7, 1
         bne      .entry

.oloop:  min      r0, r7, 16
         subscale r1, r0 * 64
         subscale r2, r0 * 32
         subscale r3, r0 * 32
         vbitplanes -, 0                           SETF
.entry:  btest    r6, 0
         beq      .skip0
         v16ld    H16(0++,0), (r2+=r4)             REP r0 IFZ
.skip0:  btest    r6, 1
         beq      .skip1
         v16ld    H16(0++,32), (r3+=r4)            REP r0 IFZ
.skip1:  v32st    H32(0++,0), (r1+=r5)             REP r0 IFZ
         sub      r7, r0
         addcmpbne r7, 0, 0, .oloop

         ldm      r6-r7,(sp++)
         .cfa_pop{%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_interleave4_2
;;;
;;; SYNOPSIS
;;;   void vclib_interleave4_2(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      int                 count);   /* r3 */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave4_2 ; {{{
         stm      r6-r9,(--sp)
         .cfa_push{%r6,%r7,%r8,%r9}

         v32mov   H32(0++,0), 0                    REP 16

         lsr      r9, r3, 4
         bmask    r7, r3, 4
         mov      r8, 0

         cmp      r1, 0
         bitset.ne r8, 0
         cmp      r2, 0
         bitset.ne r8, 1

         addscale r3, r2, r9 * 64      ; src1
         addscale r2, r1, r9 * 64      ; src0
         addscale r1, r0, r9 * 128     ; dst

         mov      r0, -1
         cmp      r7, 0
         shl      r0, r7
         mov      r6, 64
         vbitplanes -, r0                          SETF
         mov      r0, 1
         bne      .entry
         vbitplanes -, 0                           SETF
         nop
.oloop:  min      r0, r9, 8
         subscale r1, r0 * 128
         subscale r2, r0 * 64
         subscale r3, r0 * 64
.entry:  btest    r8, 0
         beq      .skip0
         v32ld    H32(0++,0), (r2+=r6)             REP r0 IFZ
.skip0:  btest    r8, 1
         beq      .skip1
         v32ld    H32(8++,0), (r3+=r6)             REP r0 IFZ
.skip1:  vinterl  H32(48,0), H32(0,0), H32(8,0)
         vinterh  H32(49,0), H32(0,0), H32(8,0)
         vinterl  H32(50,0), H32(1,0), H32(9,0)
         vinterh  H32(51,0), H32(1,0), H32(9,0)
         vinterl  H32(52,0), H32(2,0), H32(10,0)
         vinterh  H32(53,0), H32(2,0), H32(10,0)
         vinterl  H32(54,0), H32(3,0), H32(11,0)
         vinterh  H32(55,0), H32(3,0), H32(11,0)
         vinterl  H32(56,0), H32(4,0), H32(12,0)
         vinterh  H32(57,0), H32(4,0), H32(12,0)
         vinterl  H32(58,0), H32(5,0), H32(13,0)
         vinterh  H32(59,0), H32(5,0), H32(13,0)
         vinterl  H32(60,0), H32(6,0), H32(14,0)
         vinterh  H32(61,0), H32(6,0), H32(14,0)
         vinterl  H32(62,0), H32(7,0), H32(15,0)
         vinterh  H32(63,0), H32(7,0), H32(15,0)

         addcmpbne r7, 0, 0, .tailstore

         sub      r9, r0
         shl      r0, 1

         v32st    H32(48++,0), (r1+=r6)            REP r0
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr

         .cfa_push{%r6,%r7,%r8,%r9}
.tailstore:
         lsr      r0, r7, 3
         and      r7, 7
         v32st    H32(48++,0), (r1+=r6)            REP r0
         shl      r7, 1
         mov      r6, -1
         shl      r0, 6
         shl      r6, r7
         mov      r7, 0
         vbitplanes -, r6                          SETF
         add      r6, r1, r0
         v32st    H32(48,0)+r0, (r6)               IFZ
         mov      r6, 64
         vbitplanes -, 0                           SETF
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_interleave2_4
;;;
;;; SYNOPSIS
;;;   void vclib_interleave2_4(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      void          const *src2,    /* r3 */
;;;      void          const *src3,    /* r4 */
;;;      int                 count);   /* r5 */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave2_4 ; {{{
         stm      r6-r9,(--sp)
         .cfa_push{%r6,%r7,%r8,%r9}

         v32mov   H32(0++,0), 0                    REP 16

         lsr      r9, r5, 4
         bmask    r7, r5, 4
         mov      r8, 0

         cmp      r1, 0
         bitset.ne r8, 0
         cmp      r2, 0
         bitset.ne r8, 1
         cmp      r3, 0
         bitset.ne r8, 2
         cmp      r4, 0
         bitset.ne r8, 3

         addscale r5, r4, r9 * 32      ; src3
         addscale r4, r3, r9 * 32      ; src2
         addscale r3, r2, r9 * 32      ; src1
         addscale r2, r1, r9 * 32      ; src0
         addscale r1, r0, r9 * 128     ; dst

         mov      r0, -1
         cmp      r7, 0
         shl      r0, r7
         mov      r6, 32
         vbitplanes -, r0                          SETF
         mov      r0, 1
         bne      .entry
         vbitplanes -, 0                           SETF
         nop
.oloop:  min      r0, r9, 8
         mov      r6, 32
         subscale r1, r0 * 128
         subscale r2, r0 * 32
         subscale r3, r0 * 32
         subscale r4, r0 * 32
         subscale r5, r0 * 32
.entry:  btest    r8, 0
         beq      .skip0
         v16ld    H16(0++,0), (r2+=r6)             REP r0 IFZ
.skip0:  btest    r8, 1
         beq      .skip1
         v16ld    H16(0++,32), (r3+=r6)            REP r0 IFZ
.skip1:  btest    r8, 2
         beq      .skip2
         v16ld    H16(8++,0), (r4+=r6)             REP r0 IFZ
.skip2:  btest    r8, 3
         beq      .skip3
         v16ld    H16(8++,32), (r5+=r6)            REP r0 IFZ
.skip3:  vinterl  H32(48,0), H32(0,0), H32(8,0)
         vinterh  H32(49,0), H32(0,0), H32(8,0)
         vinterl  H32(50,0), H32(1,0), H32(9,0)
         vinterh  H32(51,0), H32(1,0), H32(9,0)
         vinterl  H32(52,0), H32(2,0), H32(10,0)
         vinterh  H32(53,0), H32(2,0), H32(10,0)
         vinterl  H32(54,0), H32(3,0), H32(11,0)
         vinterh  H32(55,0), H32(3,0), H32(11,0)
         vinterl  H32(56,0), H32(4,0), H32(12,0)
         vinterh  H32(57,0), H32(4,0), H32(12,0)
         vinterl  H32(58,0), H32(5,0), H32(13,0)
         vinterh  H32(59,0), H32(5,0), H32(13,0)
         vinterl  H32(60,0), H32(6,0), H32(14,0)
         vinterh  H32(61,0), H32(6,0), H32(14,0)
         vinterl  H32(62,0), H32(7,0), H32(15,0)
         vinterh  H32(63,0), H32(7,0), H32(15,0)

         mov      r6, 64
         addcmpbne r7, 0, 0, .tailstore

         sub      r9, r0
         shl      r0, 1

         v32st    H32(48++,0), (r1+=r6)            REP r0
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr

         .cfa_push{%r6,%r7,%r8,%r9}
.tailstore:
         lsr      r0, r7, 3
         and      r7, 7
         v32st    H32(48++,0), (r1+=r6)            REP r0
         shl      r7, 1
         mov      r6, -1
         shl      r0, 6
         shl      r6, r7
         mov      r7, 0
         vbitplanes -, r6                          SETF
         add      r6, r1, r0
         v32st    H32(48,0)+r0, (r6)               IFZ
         vbitplanes -, 0                           SETF
         mov      r6, 32
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_interleave4_4
;;;
;;; SYNOPSIS
;;;   void vclib_interleave4_4(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      void          const *src2,    /* r3 */
;;;      void          const *src3,    /* r4 */
;;;      int                 count);   /* r5 */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave4_4 ; {{{
         stm      r6-r9,(--sp)
         .cfa_push{%r6,%r7,%r8,%r9}

         v32mov   H32(0++,0), 0                    REP 16

         lsr      r9, r5, 4
         bmask    r7, r5, 4
         mov      r8, 0

         cmp      r1, 0
         bitset.ne r8, 0
         cmp      r2, 0
         bitset.ne r8, 1
         cmp      r3, 0
         bitset.ne r8, 2
         cmp      r4, 0
         bitset.ne r8, 3

         addscale r5, r4, r9 * 64      ; src3
         addscale r4, r3, r9 * 64      ; src2
         addscale r3, r2, r9 * 64      ; src1
         addscale r2, r1, r9 * 64      ; src0
         addscale r1, r0, r9 * 256     ; dst

         mov      r0, -1
         cmp      r7, 0
         shl      r0, r7
         mov      r6, 32
         vbitplanes -, r0                          SETF
         mov      r0, 1
         mov      r6, 64
         bne      .entry
         vbitplanes -, 0                           SETF
         nop
.oloop:  min      r0, r9, 4
         subscale r1, r0 * 256
         subscale r2, r0 * 64
         subscale r3, r0 * 64
         subscale r4, r0 * 64
         subscale r5, r0 * 64
.entry:  btest    r8, 0
         beq      .skip0
         v32ld    H32(0++,0), (r2+=r6)            REP r0 IFZ
.skip0:  btest    r8, 1
         beq      .skip1
         v32ld    H32(4++,0), (r3+=r6)            REP r0 IFZ
.skip1:  btest    r8, 2
         beq      .skip2
         v32ld    H32(8++,0), (r4+=r6)            REP r0 IFZ
.skip2:  btest    r8, 3
         beq      .skip3
         v32ld    H32(12++,0), (r5+=r6)           REP r0 IFZ
.skip3:  veven    V32(32,0),  V32(0,0),  V32(0,1)
         veven    V32(32,8),  V32(0,2),  V32(0,3)
         veven    V32(32,1),  V32(0,4),  V32(0,5)
         veven    V32(32,9),  V32(0,6),  V32(0,7)
         veven    V32(32,2),  V32(0,8),  V32(0,9)
         veven    V32(32,10), V32(0,10), V32(0,11)
         veven    V32(32,3),  V32(0,12), V32(0,13)
         veven    V32(32,11), V32(0,14), V32(0,15)
         vodd     V32(32,4),  V32(0,0),  V32(0,1)
         vodd     V32(32,12), V32(0,2),  V32(0,3)
         vodd     V32(32,5),  V32(0,4),  V32(0,5)
         vodd     V32(32,13), V32(0,6),  V32(0,7)
         vodd     V32(32,6),  V32(0,8),  V32(0,9)
         vodd     V32(32,14), V32(0,10), V32(0,11)
         vodd     V32(32,7),  V32(0,12), V32(0,13)
         vodd     V32(32,15), V32(0,14), V32(0,15)

         veven    H32(48++,0),  V32(32,0++),  V32(32,8++) REP 8
         vodd     H32(56++,0),  V32(32,0++),  V32(32,8++) REP 8
         addcmpbne r7, 0, 0, .tailstore

         sub      r9, r0
         shl      r0, 2

         v32st    H32(48++,0), (r1+=r6)           REP r0
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr

         .cfa_push{%r6,%r7,%r8,%r9}
.tailstore:
         lsr      r0, r7, 2
         and      r7, 3
         v32st    H32(48++,0), (r1+=r6)           REP r0
         shl      r7, 2
         mov      r6, -1
         shl      r0, 6
         shl      r6, r7
         mov      r7, 0
         vbitplanes -, r6                         SETF
         add      r6, r1, r0
         v32st    H32(48,0)+r0, (r6)              IFZ
         vbitplanes -, 0                           SETF
         mov      r6, 64
         addcmpbne r9, 0, 0, .oloop
         ldm      r6-r9,(sp++)
         .cfa_pop{%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_interleave2_8
;;;
;;; SYNOPSIS
;;;   void vclib_interleave2_8(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      void          const *src2,    /* r3 */
;;;      void          const *src3,    /* r4 */
;;;      void          const *src4,    /* r5 */
;;;      void          const *src5,    /* (sp)     -> (sp+32) */
;;;      void          const *src6,    /* (sp+4)   -> (sp+36) */
;;;      void          const *src7,    /* (sp+8)   -> (sp+40) */
;;;      int                  count);  /* (sp+12)  -> (sp+44) */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave2_8 ; {{{
         stm      r6-r13,(--sp)
         .cfa_push{%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}

         v32mov   H32(0++,0), 0                    REP 16

         ld       r12, (sp+44)
         ld       r6, (sp+32)
         ld       r7, (sp+36)
         ld       r8, (sp+40)
         lsr      r13, r12, 4
         bmask    r11, r12, 4
         mov      r12, 0

         cmp      r1, 0
         bitset.ne r12, 0
         cmp      r2, 0
         bitset.ne r12, 1
         cmp      r3, 0
         bitset.ne r12, 2
         cmp      r4, 0
         bitset.ne r12, 3
         cmp      r5, 0
         bitset.ne r12, 4
         cmp      r6, 0
         bitset.ne r12, 5
         cmp      r7, 0
         bitset.ne r12, 6
         cmp      r8, 0
         bitset.ne r12, 7

         addscale r9, r8, r13 * 32      ; src7
         addscale r8, r7, r13 * 32      ; src6
         addscale r7, r6, r13 * 32      ; src5
         addscale r6, r5, r13 * 32      ; src4
         addscale r5, r4, r13 * 32      ; src3
         addscale r4, r3, r13 * 32      ; src2
         addscale r3, r2, r13 * 32      ; src1
         addscale r2, r1, r13 * 32      ; src0
         addscale r1, r0, r13 * 256     ; dst

         mov      r0, -1
         cmp      r11, 0
         shl      r0, r11
         mov      r10, 32
         vbitplanes -, r0                          SETF
         mov      r0, 1
         bne      .entry
         vbitplanes -, 0                           SETF
         nop
.oloop:  min      r0, r13, 4
         mov      r10, 32
         subscale r1, r0 * 256
         subscale r2, r0 * 32
         subscale r3, r0 * 32
         subscale r4, r0 * 32
         subscale r5, r0 * 32
         subscale r6, r0 * 32
         subscale r7, r0 * 32
         subscale r8, r0 * 32
         subscale r9, r0 * 32
.entry:  btest    r12, 0
         beq      .skip0
         v16ld    H16(0++,0), (r2+=r10)            REP r0 IFZ
.skip0:  btest    r12, 1
         beq      .skip1
         v16ld    H16(0++,32), (r3+=r10)           REP r0 IFZ
.skip1:  btest    r12, 2
         beq      .skip2
         v16ld    H16(4++,0), (r4+=r10)            REP r0 IFZ
.skip2:  btest    r12, 3
         beq      .skip3
         v16ld    H16(4++,32), (r5+=r10)           REP r0 IFZ
.skip3:  btest    r12, 4
         beq      .skip4
         v16ld    H16(8++,0), (r6+=r10)            REP r0 IFZ
.skip4:  btest    r12, 5
         beq      .skip5
         v16ld    H16(8++,32), (r7+=r10)           REP r0 IFZ
.skip5:  btest    r12, 6
         beq      .skip6
         v16ld    H16(12++,0), (r8+=r10)           REP r0 IFZ
.skip6:  btest    r12, 7
         beq      .skip7
         v16ld    H16(12++,32), (r9+=r10)          REP r0 IFZ
.skip7:  veven    V32(32,0),  V32(0,0),  V32(0,1)
         veven    V32(32,8),  V32(0,2),  V32(0,3)
         veven    V32(32,1),  V32(0,4),  V32(0,5)
         veven    V32(32,9),  V32(0,6),  V32(0,7)
         veven    V32(32,2),  V32(0,8),  V32(0,9)
         veven    V32(32,10), V32(0,10), V32(0,11)
         veven    V32(32,3),  V32(0,12), V32(0,13)
         veven    V32(32,11), V32(0,14), V32(0,15)
         vodd     V32(32,4),  V32(0,0),  V32(0,1)
         vodd     V32(32,12), V32(0,2),  V32(0,3)
         vodd     V32(32,5),  V32(0,4),  V32(0,5)
         vodd     V32(32,13), V32(0,6),  V32(0,7)
         vodd     V32(32,6),  V32(0,8),  V32(0,9)
         vodd     V32(32,14), V32(0,10), V32(0,11)
         vodd     V32(32,7),  V32(0,12), V32(0,13)
         vodd     V32(32,15), V32(0,14), V32(0,15)

         veven    H32(48++,0),  V32(32,0++),  V32(32,8++) REP 8
         vodd     H32(56++,0),  V32(32,0++),  V32(32,8++) REP 8
         mov      r10, 64
         addcmpbne r11, 0, 0, .tailstore

         sub      r13, r0
         shl      r0, 2

         v32st    H32(48++,0), (r1+=r10)           REP r0
         addcmpbne r13, 0, 0, .oloop
         ldm      r6-r13,(sp++)
         .cfa_pop{%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
         b        lr

         .cfa_push{%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
.tailstore:
         lsr      r0, r11, 2
         and      r11, 3
         v32st    H32(48++,0), (r1+=r10)           REP r0
         shl      r11, 2
         mov      r10, -1
         shl      r0, 6
         shl      r10, r11
         mov      r11, 0
         vbitplanes -, r10                         SETF
         add      r10, r1, r0
         v32st    H32(48,0)+r0, (r10)              IFZ
         vbitplanes -, 0                           SETF
         mov      r10, 32
         addcmpbne r13, 0, 0, .oloop
         ldm      r6-r13,(sp++)
         .cfa_pop{%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_interleave4_8
;;;
;;; SYNOPSIS
;;;   void vclib_interleave2_8(
;;;      void                *dst,     /* r0 */
;;;      void          const *src0,    /* r1 */
;;;      void          const *src1,    /* r2 */
;;;      void          const *src2,    /* r3 */
;;;      void          const *src3,    /* r4 */
;;;      void          const *src4,    /* r5 */
;;;      void          const *src5,    /* (sp)     -> (sp+32) */
;;;      void          const *src6,    /* (sp+4)   -> (sp+36) */
;;;      void          const *src7,    /* (sp+8)   -> (sp+40) */
;;;      int                  count);  /* (sp+12)  -> (sp+44) */
;;;
;;; FUNCTION
;;;   

.function vclib_interleave4_8 ; {{{
         stm      r6-r13,(--sp)
         .cfa_push{%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}

         v32mov   H32(0++,0), 0                    REP 16

         ld       r12, (sp+44)
         ld       r6, (sp+32)
         ld       r7, (sp+36)
         ld       r8, (sp+40)
         lsr      r13, r12, 4
         bmask    r11, r12, 4
         mov      r12, 0

         cmp      r1, 0
         bitset.ne r12, 0
         cmp      r2, 0
         bitset.ne r12, 1
         cmp      r3, 0
         bitset.ne r12, 2
         cmp      r4, 0
         bitset.ne r12, 3
         cmp      r5, 0
         bitset.ne r12, 4
         cmp      r6, 0
         bitset.ne r12, 5
         cmp      r7, 0
         bitset.ne r12, 6
         cmp      r8, 0
         bitset.ne r12, 7

         addscale r9, r8, r13 * 64      ; src7
         addscale r8, r7, r13 * 64      ; src6
         addscale r7, r6, r13 * 64      ; src5
         addscale r6, r5, r13 * 64      ; src4
         addscale r5, r4, r13 * 64      ; src3
         addscale r4, r3, r13 * 64      ; src2
         addscale r3, r2, r13 * 64      ; src1
         addscale r2, r1, r13 * 64      ; src0
         addscale r1, r0, r13 * 256     ; dst
         addscale r1, r1, r13 * 256     ; dst

         mov      r0, -1
         cmp      r11, 0
         shl      r0, r11
         mov      r10, 64 
         vbitplanes -, r0                          SETF
         mov      r0, 1
         bne      .entry
         vbitplanes -, 0                           SETF
         nop
.oloop:  min      r0, r13, 2
         subscale r1, r0 * 256
         subscale r1, r0 * 256
         subscale r2, r0 * 64
         subscale r3, r0 * 64
         subscale r4, r0 * 64
         subscale r5, r0 * 64
         subscale r6, r0 * 64
         subscale r7, r0 * 64
         subscale r8, r0 * 64
         subscale r9, r0 * 64
.entry:  btest    r12, 0
         beq      .skip0
         v32ld    H32(0++,0), (r2+=r10)            REP r0 IFZ
         nop
.skip0:  btest    r12, 1
         beq      .skip1
         v32ld    H32(2++,0), (r3+=r10)            REP r0 IFZ
         nop
.skip1:  btest    r12, 2
         beq      .skip2
         v32ld    H32(4++,0), (r4+=r10)            REP r0 IFZ
         nop
.skip2:  btest    r12, 3
         beq      .skip3
         v32ld    H32(6++,0), (r5+=r10)            REP r0 IFZ
         nop
.skip3:  btest    r12, 4
         beq      .skip4
         v32ld    H32(8++,0), (r6+=r10)            REP r0 IFZ
         nop
.skip4:  btest    r12, 5
         beq      .skip5
         v32ld    H32(10++,0), (r7+=r10)           REP r0 IFZ
         nop
.skip5:  btest    r12, 6
         beq      .skip6
         v32ld    H32(12++,0), (r8+=r10)           REP r0 IFZ
         nop
.skip6:  btest    r12, 7
         beq      .skip7
         v32ld    H32(14++,0), (r9+=r10)           REP r0 IFZ
         nop
.skip7:  veven    H32(48,0), V32(0,0),  V32(0,1)
         veven    H32(49,0), V32(0,2),  V32(0,3)
         veven    H32(50,0), V32(0,4),  V32(0,5)
         veven    H32(51,0), V32(0,6),  V32(0,7)
         veven    H32(52,0), V32(0,8),  V32(0,9)
         veven    H32(53,0), V32(0,10), V32(0,11)
         veven    H32(54,0), V32(0,12), V32(0,13)
         veven    H32(55,0), V32(0,14), V32(0,15)

         addcmpbne r11, 0, 0, .tailstore

         vodd     H32(56,0), V32(0,0),  V32(0,1)
         vodd     H32(57,0), V32(0,2),  V32(0,3)
         vodd     H32(58,0), V32(0,4),  V32(0,5)
         vodd     H32(59,0), V32(0,6),  V32(0,7)
         vodd     H32(60,0), V32(0,8),  V32(0,9)
         vodd     H32(61,0), V32(0,10), V32(0,11)
         sub      r13, r0
         vodd     H32(62,0), V32(0,12), V32(0,13)
         shl      r0, 3
         vodd     H32(63,0), V32(0,14), V32(0,15)

         v32st    H32(48++,0), (r1+=r10)           REP r0
         addcmpbne r13, 0, 0, .oloop
         ldm      r6-r13,(sp++)
         .cfa_pop{%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
         b        lr

         .cfa_push{%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
.tailstore:
         lsr      r0, r11, 1
         and      r11, 1
         v32st    H32(48++,0), (r1+=r10)           REP r0
         shl      r11, 3
         mov      r10, -1
         shl      r0, 6
         shl      r10, r11
         mov      r11, 0
         vbitplanes -, r10                         SETF
         add      r10, r1, r0
         v32st    H32(48,0)+r0, (r10)              IFZ
         vbitplanes -, 0                           SETF
         mov      r10, 64
         addcmpbne r13, 0, 0, .oloop
         ldm      r6-r13,(sp++)
         .cfa_pop{%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


