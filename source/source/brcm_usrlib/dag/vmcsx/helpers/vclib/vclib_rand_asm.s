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

.define N, 624
.define M, 397
.define MATRIX_A, 0x9908b0df

;;; *****************************************************************************
;;; NAME
;;;   vclib_mt19937_update
;;;
;;; SYNOPSIS
;;;   void vclib_mt19937_update(
;;;      unsigned long const *bits);      /* r0 */
;;;
;;; FUNCTION
;;;   stuff

.function vclib_mt19937_update ; {{{
         mov      r5, r0
         mov      r0, 39 ; (N + 15) >> 4
         mov      r1, 64
         nop ; pipeline stabilisation

         vld      H32(25++,0), (r5+=r1)            REP r0

         mov      r2, 0x80000000
         mov      r3, 0x7fffffff
         mov      r4, MATRIX_A

         mov      r1, 0
         mov      r0, 15
.loop:   vand     -,           V32(64-39,1)+r1, r3 CLRA SACC
         nop ; pipeline stabilisation
         vand     V32(0,0)+r1, V32(64-39,0)+r1, r2      SACC
         vlsr     V32(0,0)+r1, V32(0,0)+r1, 1      SETF
         veor     V32(0,0)+r1, V32(0,0)+r1, r4     IFC
         addcmpbne r1, 1, r0, .loop

         vand     -,           V32(64-38,1)+r1, r3 CLRA SACC
         add      r0, 64 * 13
         vand     V32(0,0)+r1, V32(64-39,0)+r1, r2      SACC
         vlsr     V32(0,0)+r1, V32(0,0)+r1, 1      SETF
         veor     V32(0,0)+r1, V32(0,0)+r1, r4     IFC
         sub      r1, 15

         veor     V32(0,0++)+r1, V32(64-39+24,13++)+r1, V32(0,0++)+r1   REP 2
         veor     V32(0,2  )+r1, V32(64-39+24,15  )+r1, V32(0,2  )+r1   REP 1
         veor     V32(0,3  )+r1, V32(64-39+25,0  )+r1, V32(0,3  )+r1    REP 1
         veor     V32(0,4++)+r1, V32(64-39+25,1++)+r1, V32(0,4++)+r1    REP 4
         veor     V32(0,8++)+r1, V32(64-39+25,5++)+r1, V32(0,8++)+r1    REP 8
         add      r1, 64 * 13

         cmp      r1, 64 * 39
         blo      .loop

         mov      r0, 39
         mov      r1, 64
         vst      H32(0++,0), (r5+=r1)             REP r0
         b lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vclib_mt19937_temper_block
;;;
;;; SYNOPSIS
;;;   int vclib_mt19937_temper_block(
;;;      int                  lines,      /* r0 */
;;;      void                *dest,       /* r1 */
;;;      unsigned long const *bits);      /* r2 */
;;;
;;; FUNCTION
;;;   stuff
;;;
;;; RETURNS
;;;   lines

.function vclib_mt19937_temper_block ; {{{
         mov      r3, 64
         vld      H32(0++,0), (r2+=r3)             REP r0

         shl      r5, r0, 6
         mov      r2, 0
         mov      r4, 1024

         vbitplanes  H16(0++,0), H16(0++,0)                             REP r0
         vbitplanes  H16(0++,32), H16(0++,32)                           REP r0

.loop:   veor     V16(0, 0++)+r2, V16(0, 0++)+r2, V16(0,11++)+r2        REP 16
         veor     V16(0,32++)+r2, V16(0,32++)+r2, V16(0,43++)+r2        REP 4
         veor     V16(0,36  )+r2, V16(0,36  )+r2, V16(0,47  )+r2

         veor     V16(0,47  )+r2, V16(0,47  )+r2, V16(0,40  )+r2
         veor     V16(0,44  )+r2, V16(0,44  )+r2, V16(0,37  )+r2
         veor     V16(0,42++)+r2, V16(0,42++)+r2, V16(0,35++)+r2        REP 2
         veor     V16(0,40  )+r2, V16(0,40  )+r2, V16(0,33  )+r2
         veor     V16(0,37  )+r2, V16(0,37  )+r2, V16(0,14  )+r2
         veor     V16(0,34++)+r2, V16(0,34++)+r2, V16(0,11++)+r2        REP 2
         veor     V16(0,14  )+r2, V16(0,14  )+r2, V16(0, 7  )+r2
         veor     V16(0,12  )+r2, V16(0,12  )+r2, V16(0, 5  )+r2
         veor     V16(0, 9++)+r2, V16(0, 9++)+r2, V16(0, 2++)+r2        REP 2
         veor     V16(0, 7  )+r2, V16(0, 7  )+r2, V16(0, 0  )+r2

         veor     V16(0,47  )+r2, V16(0,47  )+r2, V16(0,32  )+r2
         veor     V16(0,45++)+r2, V16(0,45++)+r2, V16(0,14++)+r2        REP 2
         veor     V16(0,42++)+r2, V16(0,42++)+r2, V16(0,11++)+r2        REP 2
         veor     V16(0,38++)+r2, V16(0,38++)+r2, V16(0, 7++)+r2        REP 4
         veor     V16(0,33++)+r2, V16(0,33++)+r2, V16(0, 2++)+r2        REP 2

         veor     V16(0, 0++)+r2, V16(0, 0++)+r2, V16(0,34++)+r2        REP 8
         veor     V16(0, 8++)+r2, V16(0, 8++)+r2, V16(0,42++)+r2        REP 4
         veor     V16(0,12++)+r2, V16(0,12++)+r2, V16(0,46++)+r2        REP 2

         addcmpblo  r2, r4, r5, .loop

         vbitplanes  H16(0++,0), H16(0++,0)                             REP r0
         vbitplanes  H16(0++,32), H16(0++,32)                           REP r0

         vst      H32(0++,0), (r1+=r3)             REP r0
         b lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_mt19937_update_and_temper_core
;;;
;;; SYNOPSIS
;;;      r5             void *dest
;;;      r6             64
;;;      r7             lines
;;;      H32(25..63,0)  bits
;;;
;;; FUNCTION
;;;   stuff
;;;
;;; RETURNS
;;;      H32(0..38,0)+r1   bits

.function vclib_mt19937_update_and_temper_core
         mov      r2, 0x80000000
         mov      r3, 0x7fffffff
         mov      r4, MATRIX_A

         mov      r1, 0
         mov      r0, 15
         nop ; pipeline stabilisation
.loop:   vand     -,           V32(64-39,1)+r1, r3 CLRA SACC
         nop ; pipeline stabilisation
         vand     V32(0,0)+r1, V32(64-39,0)+r1, r2      SACC
         vlsr     V32(0,0)+r1, V32(0,0)+r1, 1      SETF
         veor     V32(0,0)+r1, V32(0,0)+r1, r4     IFC
         addcmpbne r1, 1, r0, .loop

         vand     -,           V32(64-38,1)+r1, r3 CLRA SACC
         cmp      r7, 0
         vand     V32(0,0)+r1, V32(64-39,0)+r1, r2      SACC
         vlsr     V32(0,0)+r1, V32(0,0)+r1, 1      SETF
         veor     V32(0,0)+r1, V32(0,0)+r1, r4     IFC
         sub      r1, 15

         veor     V32(0,0++)+r1, V32(64-39+24,13++)+r1, V32(0,0++)+r1   REP 2
         veor     V32(0,2  )+r1, V32(64-39+24,15  )+r1, V32(0,2  )+r1   REP 1
         veor     V32(0,3  )+r1, V32(64-39+25,0  )+r1, V32(0,3  )+r1    REP 1
         veor     V32(0,4++)+r1, V32(64-39+25,1++)+r1, V32(0,4++)+r1    REP 4
         veor     V32(0,8++)+r1, V32(64-39+25,5++)+r1, V32(0,8++)+r1    REP 8

         ble      .skip

         min      r0, r7, 13
         nop ; pipeline stabilisation

         vbitplanes  H16(16++,0)+r1, H16(0++,0)+r1                      REP r0
         vbitplanes  H16(16++,32)+r1, H16(0++,32)+r1                    REP r0

         veor     V16(16, 0++)+r1, V16(16, 0++)+r1, V16(16,11++)+r1     REP 16
         veor     V16(16,32++)+r1, V16(16,32++)+r1, V16(16,43++)+r1     REP 4
         veor     V16(16,36  )+r1, V16(16,36  )+r1, V16(16,47  )+r1

         veor     V16(16,47  )+r1, V16(16,47  )+r1, V16(16,40  )+r1
         veor     V16(16,44  )+r1, V16(16,44  )+r1, V16(16,37  )+r1
         veor     V16(16,42++)+r1, V16(16,42++)+r1, V16(16,35++)+r1     REP 2
         veor     V16(16,40  )+r1, V16(16,40  )+r1, V16(16,33  )+r1
         veor     V16(16,37  )+r1, V16(16,37  )+r1, V16(16,14  )+r1
         veor     V16(16,34++)+r1, V16(16,34++)+r1, V16(16,11++)+r1     REP 2
         veor     V16(16,14  )+r1, V16(16,14  )+r1, V16(16, 7  )+r1
         veor     V16(16,12  )+r1, V16(16,12  )+r1, V16(16, 5  )+r1
         veor     V16(16, 9++)+r1, V16(16, 9++)+r1, V16(16, 2++)+r1     REP 2
         veor     V16(16, 7  )+r1, V16(16, 7  )+r1, V16(16, 0  )+r1

         veor     V16(16,47  )+r1, V16(16,47  )+r1, V16(16,32  )+r1
         veor     V16(16,45++)+r1, V16(16,45++)+r1, V16(16,14++)+r1     REP 2
         veor     V16(16,42++)+r1, V16(16,42++)+r1, V16(16,11++)+r1     REP 2
         veor     V16(16,38++)+r1, V16(16,38++)+r1, V16(16, 7++)+r1     REP 4
         veor     V16(16,33++)+r1, V16(16,33++)+r1, V16(16, 2++)+r1     REP 2

         veor     V16(16, 0++)+r1, V16(16, 0++)+r1, V16(16,34++)+r1     REP 8
         veor     V16(16, 8++)+r1, V16(16, 8++)+r1, V16(16,42++)+r1     REP 4
         veor     V16(16,12++)+r1, V16(16,12++)+r1, V16(16,46++)+r1     REP 2
         sub      r7, r0

         vbitplanes  H16(16++,0)+r1, H16(16++,0)+r1                     REP r0
         vbitplanes  H16(16++,32)+r1, H16(16++,32)+r1                   REP r0

         vst      H32(16++,0)+r1, (r5+=r6)                              REP r0
         addscale r5, r0 * 64

.skip:   btest    r1, 17
         add      r1, 64 * 13 + 0x10000

         bic      r1, 0xF000
         nop ; pipeline stabilisation

         add      r0, r1, 15
         beq      .loop

         bmask    r1, 12
         add      r0, r1, 15

         cmp      r7, 0
         bgt      .loop

         add      r1, (64-39)*64
         bmask    r1, 12
         b lr
.endfn
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_mt19937_update_and_temper
;;;
;;; SYNOPSIS
;;;   void vclib_mt19937_update_and_temper(
;;;      void                *dest,       /* r0 */
;;;      int                  lines,      /* r1 */
;;;      unsigned long       *bits);      /* r2 */
;;;
;;; FUNCTION
;;;   stuff

.function vclib_mt19937_update_and_temper
         stm      r6-r8,lr, (--sp)
         .cfa_push {%lr,%r6,%r7,%r8}

         mov      r5, r0
         mov      r6, 64
         mov      r7, r1
         mov      r8, r2

         mov      r0, 39 ; (N + 15) >> 4

         vld      H32(25++,0), (r8+=r6)            REP r0

         bl       vclib_mt19937_update_and_temper_core

         mov      r0, 39
         vst      H32(0++,0)+r1, (r8+=r6)          REP r0

         ldm      r6-r8,pc, (sp++)
         .cfa_pop {%r8,%r7,%r6,%pc}
.endfn
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_mt19937_update_and_temper_stateless
;;;
;;; SYNOPSIS
;;;   void vclib_mt19937_update_and_temper_stateless(
;;;      void                *dest,       /* r0 */
;;;      int                  lines,      /* r1 */
;;;      unsigned long const *seed);      /* r2 */
;;;
;;; FUNCTION
;;;   stuff

.function vclib_mt19937_update_and_temper_stateless
         stm      r6-r7,lr, (--sp)
         .cfa_push {%lr,%r6,%r7}

         mov      r5, r0
         mov      r6, 64
         mov      r7, r1

         mov      r0, 39 ; (N + 15) >> 4

         vld      H32(24,0), (r2)

         mov      r1, 0
         mov      r2, 0x5d588B65
         mov      r3, 1
.vloop:  vlsr     H32(25,0)+r1, H32(24,0)+r1, 30
         veor     H32(25,0)+r1, H32(25,0)+r1, H32(24,0)+r1
         vmov     -, r3                               CLRA UACC
         vmul     -, H16(25,0)+r1, 0x5d58                  UACCH
         vmul     -, H16(25,32)+r1, r2                     UACCH
         v32mul.uu -, H32(25,0)+r1, r2                     UACC
         v32getacc H32(25,0)+r1, 0
         add      r1, 64
         addcmpbne r3, 1, 40, .vloop

         bl       vclib_mt19937_update_and_temper_core

         ldm      r6-r7,pc, (sp++)
         .cfa_pop {%r7,%r6,%pc}
.endfn
.FORGET
