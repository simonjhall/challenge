;;; ===========================================================================
;;; Copyright (c) 2010-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================
;;;
;;; A [hopefully] cute attempt at CRCs through the vector unit
;;;

;;;
;;; nsb: the "_rev" versions of the functions work as though the crc register is
;;;      shifting right, the normal functions shift the crc register left
;;; 
   
.include "vcinclude/common.inc"

;;; *****************************************************************************
;;; NAME
;;;   vclib_crc_core
;;;
;;; SYNOPSIS
;;;   void vclib_crc_core(
;;;      uint32_t          polytab[32][16],/* r0 */
;;;      uint32_t          work,          /* r1 */
;;;      uint32_t   const *data,          /* r2 */
;;;      int               length,        /* r3 */
;;;      int               poly_words,    /* r4 */
;;;      int               chomp_words);  /* r5 */
;;;   
;;;
;;; FUNCTION
;;;   Do awesome stuff

.function vclib_crc_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

         vsub     H(36,0), 0, r4

         mov      r6, 64
;         vld      H32(0++,0), (r0+=r6)             REP 32

         mov      r0, -1
         shl      r0, r4
         vbitplanes H(36,16), r0                    SETF
;         vld      H32(32,0), (r1)                  IFZ
         mov      r0, -1
         shl      r0, r5
         vbitplanes H16(36,32), r0

         mov      r0, -1
         and      r7, r3, 15
         asr      r3, 4
         shl      r0, r7
         addcmpbeq r7, 0, 0, .ldtest
         vbitplanes -, r0                          SETF
         rsub     r8, r7, 16
         rsub     r0, r5, 0
         vmov     H32(48,0), 0
         vld      H32(48,0)+r8, (r2)               IFZ
         vmov     H32(32,0)+r8, H32(32,0)
         addscale r2, r7 * 4
         and      r8, r0
         mov      r9, 0
         mov      r10, 64
         b        .entry

.load:   vld      H32(48++,0), (r2+=r6)            REP r0
         addscale r2, r0 * 64
         sub      r3, r0
         mov      r9, 0
         shl      r10, r0, 6
.outer:  mov      r8, 0
.entry:  veor     H32(32,0), H32(32,0), H32(48,0)+r9
.inner:  vmov     H32(33,0), H32(32,0)+r8
         msb      r0, r4
.pragma offwarn
         vinterl  H32(33,0), H32(33,0), H32(33,0)  REP r0
.pragma popwarn
         vand     H32(32,0)+r8, H32(32,0)+r8, H16(36,32)
         add      r8, r5

         vmov     H32(34,0), 0
.irep i, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30
         vshl     -,         H32(33,0), 31-i       SETF
         veor     H32(34,0), H32(34,0), H32(i,0)   IFN
         veor     H32(34,0), H32(34,0), H32(i+1,0) IFC
.endr

         v16shl   -, H(36,0), 13                   SETF
         veor     H32(34,0), H32(34,8), H32(34,0)  IFC
         veor     H32(34,0), H32(34,4), H32(34,0)  IFN
         v16shl   -, H(36,0), 15                   SETF
         veor     H32(34,0), H32(34,2), H32(34,0)  IFC
         veor     H32(34,0), H32(34,1), H32(34,0)  IFN
         vmov     -, H8(36,16)                     SETF
         veor     H32(32,0)+r8, H32(32,0)+r8, H32(34,0) IFZ
         
         addcmpbne r8, 0, 16, .inner
         addcmpbne r9, r6, r10, .outer
.ldtest: min      r0, r3, 16
         addcmpbgt r3, 0, 0, .load

;         mov      r0, -1
;         shl      r0, r4
;         vbitplanes -, r0                          SETF
;         vst      H32(32,0), (r1)                  IFZ
         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET

.if _VC_VERSION >= 3

;;;
;;; NAME
;;;   vclib_crc32_combine(_rev)
;;;
;;; SYNOPSIS
;;;   uint32_t vclib_crc32_combine(uint32_t* crc_vec, uint32_t* len_vec, uint32_t* combine_table)
;;;
;;; FUNCTION
;;;   Given a vector containing 16 crc and a second vector listing the number of zeros that
;;;   should be appended to the corresponding CRC, calculate modified CRCs and then combine
;;;   and return them to the caller (via r0).
;;;
.function vclib_crc32_combine   ; {{{
   vld         H32(33,0),     (r1)
   mov         r4, 64
   vld         H32(32,0),     (r0)
   vld         H32(34++,0),   (r2+=r4)                      REP 2
   vmsb        -,             H32(33,0),     0              MAX r3
   addcmpble   r3, 1, 0, 2f
1: mov         r5, 0
   vmov        H32(0++,0),    0                             REP 32
   vbitrev     H32(36,0),     H32(32,0),     0
   vbitplanes  H16(36,0),     H16(36,0)
   vbitplanes  H16(36,32),    H16(36,32)
3: vlsr        H16(36,0),     H16(36,0),     1              SETF
   vmov        V32(0,0)+r5,   H32(34,0)                     IFC
   vlsr        H16(36,32),    H16(36,32),    1              SETF
   vmov        V32(16,0)+r5,  H32(35,0)                     IFC
   addcmpblt   r5, 1, 16, 3b
   vlsr        H32(33,0),     H32(33,0),     1              SETF
   vld         H32(34++,0),   (r2+128+=r4)                  REP 2
   vmov        H32(32,0),     0                             IFC
   veor        H32(1++,0),    H32(1++,0),    H32(0++,0)     IFC REP 32
   addscale    r2, r4 << 1
   addcmpbgt   r3, -1, 0, 1b
2: vbitplanes  -,             1                             SETF 
   vbitplanes  H16(34,0),     H16(32,0)
   vbitplanes  H16(35,0),     H16(32,32)
   vcount      H16(34++,0),   H16(34++,0),   0              REP 2 ; xor results into ppu 0
   vbitplanes  H16(34,0),     H16(34,0)
   vbitplanes  H16(34,32),    H16(35,0)
   vmov        -,             H32(34,0)                     IFNZ SUM r0
   b           lr
.endfn                          ; }}}

;;;
;;; 'reverse' version (a-la zlib's routines)
;;; 
.function vclib_crc32_combine_rev ; {{{
   vld         H32(33,0),     (r1)
   mov         r4, 64
   vld         H32(32,0),     (r0)
   vld         H32(34++,0),   (r2+=r4)                      REP 2
   vmsb        -,             H32(33,0),     0              MAX r3
   addcmpble   r3, 1, 0, 2f
1: mov         r5, 0
   vmov        H32(0++,0),    0                             REP 32
   vbitplanes  H16(36,0),     H16(32,0)
   vbitplanes  H16(36,32),    H16(32,32)
3: vlsr        H16(36,0),     H16(36,0),     1              SETF
   vmov        V32(0,0)+r5,   H32(34,0)                     IFC
   vlsr        H16(36,32),    H16(36,32),    1              SETF
   vmov        V32(16,0)+r5,  H32(35,0)                     IFC
   addcmpblt   r5, 1, 16, 3b
   vlsr        H32(33,0),     H32(33,0),     1              SETF
   vld         H32(34++,0),   (r2+128+=r4)                  REP 2
   vmov        H32(32,0),     0                             IFC
   veor        H32(1++,0),    H32(1++,0),    H32(0++,0)     IFC REP 32
   addscale    r2, r4 << 1
   addcmpbgt   r3, -1, 0, 1b
2: vbitplanes  -,             1                             SETF 
   vbitplanes  H16(34,0),     H16(32,0)
   vbitplanes  H16(35,0),     H16(32,32)
   vcount      H16(34++,0),   H16(34++,0),   0              REP 2 ; xor results into ppu 0
   vbitplanes  H16(34,0),     H16(34,0)
   vbitplanes  H16(34,32),    H16(35,0)
   vmov        -,             H32(34,0)                     IFNZ SUM r0
   b           lr
.endfn                          ; }}}

;;;
;;; NAME
;;;   vclib_crc32_update
;;;
;;; SYNOPSIS
;;;   void vclib_crc32_update(int rows, uint8_t* base, int32_t calc_pitch,
;;;                           int32_t num, uint32_t poly)
;;;
;;; FUNCTION
;;;   VPU agnostic flavour of the parallel CRC-32 generator, does not use tables
;;;   (hence simple);  assumes that data to be CRC'ed consists of 16 independent
;;;   streams where data spaced at pitch bytes apart  and that 'n' bytes of each
;;;   stream are to be processed
;;;

;;; 
;;; append data to the top end of the register
;;; 
.function vclib_crc32_update    ; {{{
   ;; load CRCs, invert bits & update initial parameters
   vmov        H32(33,0),     r4
   bmask       r5, r1, 4
   bic         r1, 15
   mov         r4, r5
   shl         r5, 6
   rsub        r4, 16
   min         r4, r3
   vld         V8(0,0++),     (r1+=r2)                      REP r0
   cbclr
1: vldb        V8(0,16++)*,   (r1+16+=r2)                   REP r0
   sub         r3, r4
   cbadd1
   ;; crc byte update loop; extremely brain dead!
2: veor        H8(32,48),     H8(32,48),     H8(0,48)+r5*
   add         r5, (1 << 6)
.rep 8
   vshl        H32(32,0),     H32(32,0),     1              SETF
   veor        H32(32,0),     H32(32,0),     H32(33,0)      IFC
.endr
   addcmpbgt   r4, -1, 0, 2b
   add         r1, 16
   min         r4, r3, 16
   mov         r5, 0
   addcmpbgt   r3, 0, 0, 1b
   b           lr
.endfn                          ; }}}

;;; 
;;; append data to the lower end of the register
;;; 
.function vclib_crc32_update_rev   ; {{{
   ;; load CRCs, invert bits & update initial parameters
   vmov        H32(33,0),     r4
   bmask       r5, r1, 4
   bic         r1, 15
   mov         r4, r5
   shl         r5, 6
   rsub        r4, 16
   min         r4, r3
   vld         V8(0,0++),     (r1+=r2)                      REP r0
   cbclr
1: vldb        V8(0,16++)*,   (r1+16+=r2)                   REP r0
   sub         r3, r4
   cbadd1
   ;; crc byte update loop; extremely brain dead!
2: veor        H32(32,0),     H32(32,0),     H8(0,48)+r5*
   add         r5, (1 << 6)
.rep 8
   vlsr        H32(32,0),     H32(32,0),     1              SETF
   veor        H32(32,0),     H32(32,0),     H32(33,0)      IFC
.endr
   addcmpbgt   r4, -1, 0, 2b
   add         r1, 16
   min         r4, r3, 16
   mov         r5, 0
   addcmpbgt   r3, 0, 0, 1b
   b           lr
.endfn                          ; }}}
   
.endif   ; _VC_VERSION >= 3


;;; *****************************************************************************
;;; NAME
;;;   vclib_crc16_core
;;;
;;; SYNOPSIS
;;;   uint16_t vclib_crc16_core(
;;;      uint16_t      const *data);      /* r0                 */
;;;      uint16_t             initial,    /* r1                 */
;;;      void          const *buffer,     /* r2                 */
;;;      int                  length);    /* r3                 */
;;;
;;; FUNCTION
;;;   stuff

.function vclib_crc16_core
         mov      r4, 32
         vld      H16(48++,32), (r0+=r4)           REP 16

      ; We'll process 16 hwords at once in the inner loop.  Calculate the
      ; remainder here, so that we can process them separately in the first
      ; iteration, and so that we know where to inject our initial CRC.
         neg      r0, r3
         bmask    r0, 4

         add      r3, r0      ; round up count
         subscale r2, r0 * 2  ; round down address
         lsr      r3, 4       ; count in 16s from now on

      ; inject the CRC precondition into our working space, and zero the rest
         vbitplanes -, 1                           SETF
         vmov     H16(33,0), 0
         vmov     H16(33,0)+r0, r1                 IFNZ

      ; Because the first iteration may not be a full 16 values, we mask the
      ; load so that we don't load anything illegal, and we zero the vector
      ; so we don't get garbage in the unused PPUs.
         mov      r1, -1
         shl      r1, r0
         vbitplanes -, r1                          SETF
         vmov     V16(0,0), 0

      ; If we have only a single line of 16 hwords then we want to skip
      ; directly to the finalisation round, which uses different coefficients
      ; for each PPU.
         addcmpble r3, -1, 0, .once

      ; If we have more than one line then we can do a burst of up to 16 lines
      ; at once; unless the first one isn't complete, in which case we can only
      ; do one on the first round so that the conditional load works correctly.
         cmp      r0, 0
         min      r0, r3, 16
         mov.ne   r0, 1

      ; we also need to make an alternative table of coefficients which are the
      ; same in every PPU; we point to this table in r1.
         vmov     V16(48,0++), V16(48,32)          REP 16
         mov      r1, 0

         b        .loop
.align    32 ; align loop entry to a 32-byte boundary
.loop:   vld      V16(0,0++), (r2+=r4)                IFNZ REP r0
         addscale r2, r2, r0 * 32

         mov      r5, 0
         nop

.iloop:  veor     H16(32,0), H16(33,0), V16(0,0)+r5
         vmov     H16(33,0), 0

      ; Here we break each CRC up into 16 bits and calculate new CRCs for each
      ; bit having been advanced by the appropriate distance.  In the main loop
      ; this is 256 bits (the distance between one line and the next), and on
      ; the last round it is dependent on its position in the vector.  r1
      ; selects which table to use for this calculation.
         v16shl   -, H16(32,0), 15                    SETF ; bit0->N, bit1->C
         veor     H16(33,0), H16(33,0), H16(48,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(49,0)+r1  IFC
         v16shl   -, H16(32,0), 13                    SETF ; bit2->N, bit3->C
         veor     H16(33,0), H16(33,0), H16(50,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(51,0)+r1  IFC
         v16shl   -, H16(32,0), 11                    SETF ; bit4->N, bit5->C
         veor     H16(33,0), H16(33,0), H16(52,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(53,0)+r1  IFC
         v16shl   -, H16(32,0), 9                     SETF ; etc...
         veor     H16(33,0), H16(33,0), H16(54,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(55,0)+r1  IFC
         v16shl   -, H16(32,0), 7                     SETF
         veor     H16(33,0), H16(33,0), H16(56,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(57,0)+r1  IFC
         v16shl   -, H16(32,0), 5                     SETF
         veor     H16(33,0), H16(33,0), H16(58,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(59,0)+r1  IFC
         v16shl   -, H16(32,0), 3                     SETF
         veor     H16(33,0), H16(33,0), H16(60,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(61,0)+r1  IFC
         v16shl   -, H16(32,0), 1                     SETF
         veor     H16(33,0), H16(33,0), H16(62,0)+r1  IFN
         veor     H16(33,0), H16(33,0), H16(63,0)+r1  IFC

         addcmpbne r5, 1, r0, .iloop

         vbitplanes -, -1                          SETF
         sub      r3, r0

         min      r0, r3, 16
         addcmpbgt r3, 0, 0, .loop
.once:   mov      r1, 16
         mov      r0, 1
         addcmpbge r3, 0, 0, .loop

         vbitplanes -, 1                           SETF
         vbitplanes H16(34,0), H16(33,0)
         vcount   H16(35,0), H16(34,0), 0
         vbitplanes -, H16(35,0)                   IFNZ USUM r0
         b        lr
.endfn
.FORGET

