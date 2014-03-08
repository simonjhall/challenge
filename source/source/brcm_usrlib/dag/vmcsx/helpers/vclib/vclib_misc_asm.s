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
;;;   vclib_extend_24to32
;;;
;;; SYNOPSIS
;;;   void vclib_extend_24to32(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Sign-extend some memory from 24-bit words to to 32-bit words.

.function vclib_extend_24to32 ; {{{
         lsr      r3, r2, 4
         bmask    r4, r2, 4
         addscale r2, r0, r3 * 64
         addscale r1, r3 * 32
         addscale r1, r3 * 16

         mov      r0, -1
         cmp      r4, 0
         shl      r0, r4
         vbitplanes -, r0                          SETF
         mov      r0, 1
         add.ne   r3, 1
         bne      .entry

.oloop:  min      r0, r3, 16
         subscale r1, r0 * 32
         subscale r1, r0 * 16
         subscale r2, r0 * 64
         vbitplanes -, 0                           SETF
.entry:  mov      r4, 48
         v8ld     H8(0++,0), (r1+=r4)              REP r0
         v8ld     H8(0++,16), (r1+16+=r4)          REP r0
         v8ld     H8(0++,32), (r1+32+=r4)          REP r0

         mov      r4, 0
         mov      r5, 0
.iloop:  v32shl   -,            V8(0,0)+r5, 8      CLRA UACC IFZ
         v32shl   -,            V8(0,1)+r5, 16          UACC IFZ
         v32shl   V32(16,0)+r4, V8(0,2)+r5, 24          SACC IFZ
         add      r5, 3
         addcmpbne r4, 1, 16, .iloop

         v32asr   H32(16++,0), H32(16++,0), 8      REP r0

         mov      r4, 64
         v32st    H32(16++,0), (r2+=r4)            REP r0 IFZ
         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_extend_24to32hi
;;;
;;; SYNOPSIS
;;;   void vclib_extend_24to32hi(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Sign-extend some memory from 24-bit words to to 32-bit words.

.function vclib_extend_24to32hi ; {{{
         lsr      r3, r2, 4
         bmask    r4, r2, 4
         addscale r2, r0, r3 * 64
         addscale r1, r3 * 32
         addscale r1, r3 * 16

         mov      r0, -1
         cmp      r4, 0
         shl      r0, r4
         vbitplanes -, r0                          SETF
         mov      r0, 1
         add.ne   r3, 1
         bne      .entry

.oloop:  min      r0, r3, 16
         subscale r1, r0 * 32
         subscale r1, r0 * 16
         subscale r2, r0 * 64
         vbitplanes -, 0                           SETF
.entry:  mov      r4, 48
         v8ld     H8(0++,0), (r1+=r4)              REP r0
         v8ld     H8(0++,16), (r1+16+=r4)          REP r0
         v8ld     H8(0++,32), (r1+32+=r4)          REP r0

         mov      r4, 0
         mov      r5, 0
.iloop:  v32shl   -,            V8(0,0)+r5, 8      CLRA UACC IFZ
         v32shl   -,            V8(0,1)+r5, 16          UACC IFZ
         v32shl   V32(16,0)+r4, V8(0,2)+r5, 24          SACC IFZ
         add      r5, 3
         addcmpbne r4, 1, 16, .iloop

         mov      r4, 64
         v32st    H32(16++,0), (r2+=r4)            REP r0 IFZ
         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_extend_u8to16hi
;;;
;;; SYNOPSIS
;;;   void vclib_extend_u8to16hi(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Re-bias and scale some memory from 8-bit up to 16-bit words.

.function vclib_extend_u8to16hi ; {{{
         lsr      r3, r2, 4
         bmask    r4, r2, 4
         addscale r2, r0, r3 * 32
         addscale r1, r3 * 16

         mov      r0, -1
         cmp      r4, 0
         shl      r0, r4
         vbitplanes -, r0                          SETF
         mov      r4, 16
         mov      r5, 32
         mov      r0, 1
         add.ne   r3, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r3, 16
         subscale r1, r0 * 16
         subscale r2, r0 * 32
.entry:  v8ld     H8(0++,0), (r1+=r4)              REP r0

         v16sub   H16(16++,0), H8(0++,0), 0x80     REP r0
         v16shl   H16(16++,0), H16(16++,0), 8      REP r0

         v16st    H16(16++,0), (r2+=r5)            REP r0 IFZ
         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_extend_alawto16
;;;
;;; SYNOPSIS
;;;   void vclib_extend_alawto16(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Unpack some alaw bytes to 16-bit words.

.function vclib_extend_alawto16 ; {{{
         lsr      r3, r2, 4
         bmask    r4, r2, 4
         addscale r2, r0, r3 * 32
         addscale r1, r3 * 16

         mov      r0, -1
         cmp      r4, 0
         shl      r0, r4
         vbitplanes -, r0                          SETF
         mov      r4, 16
         mov      r5, 32
         vmov     -, 8                             CLRA UACC
         mov      r0, 1
         add.ne   r3, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r3, 16
         subscale r1, r0 * 16
         subscale r2, r0 * 32
.entry:  v8ld     H8(0++,16), (r1+=r4)                REP r0

         veor     H8(0++,16), H8(0++,16), 0xD5        REP r0
         vshl     H8(16++,0), H8(0++,16), 4           REP r0 UACC NO_WB ; mantissa with rounding bias
         vasr     H16(32++,0), H16(0++,0), 15         REP r0 ; collect sign bits
         vlsr     H8(0++,0), H8(0++,16), 4            REP r0 ; exponent
         vand     H8(0++,0), H8(0++,0), 7             REP r0 ; without sign bit
         vmin     H8(16++,16), H8(0++,0), 1           REP r0 ; assert implicit 1 for non-denormals
         vsub     H8(0++,0), H8(0++,0), H8(16++,16)   REP r0
         vshl     H16(16++,0), H16(16++,0), H8(0++,0) REP r0

         veor     H16(16++,0), H16(16++,0), H16(32++,0) REP r0
         vsub     H16(16++,0), H16(16++,0), H16(32++,0) REP r0

         v16st    H16(16++,0), (r2+=r5)               REP r0 IFZ
         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_extend_ulawto16
;;;
;;; SYNOPSIS
;;;   void vclib_extend_ulawto16(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Unpack some ulaw bytes to 16-bit words.

.function vclib_extend_ulawto16 ; {{{
         lsr      r3, r2, 4
         bmask    r4, r2, 4
         addscale r2, r0, r3 * 32
         addscale r1, r3 * 16

         mov      r0, -1
         cmp      r4, 0
         shl      r0, r4
         vbitplanes -, r0                          SETF
         mov      r4, 16
         mov      r5, 32
         vmov     -, 0x84                          CLRA UACC
         mov      r0, 1
         add.ne   r3, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r3, 16
         subscale r1, r0 * 16
         subscale r2, r0 * 32
.entry:  v8ld     H8(0++,16), (r1+=r4)                REP r0

         veor     H8(0++,16), H8(0++,16), 0x7f        REP r0

         vand     H8(16++,0), H8(0++,16), 15          REP r0
         vshl     H16(16++,0), H8(16++,0), 3          REP r0 SACC NO_WB ; mantissa with rounding bias
         vasr     H16(32++,0), H16(0++,0), 15         REP r0 ; collect sign bits
         vlsr     H8(0++,0), H8(0++,16), 4            REP r0 ; exponent
         vand     H8(0++,0), H8(0++,0), 7             REP r0 ; without sign bit
         vshl     H16(16++,0), H16(16++,0), H8(0++,0) REP r0 NEG SACC NO_WB

         veor     H16(16++,0), H16(16++,0), H16(32++,0) REP r0
         vsub     H16(16++,0), H16(16++,0), H16(32++,0) REP r0

         v16st    H16(16++,0), (r2+=r5)               REP r0 IFZ
         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_unextend_32to24
;;;
;;; SYNOPSIS
;;;   void vclib_extend_32to24(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Contract some memory from 32-bit words to to 24-bit words.

.function vclib_unextend_32to24 ; {{{
         stm      r6-r7,lr,(--sp)
         .cfa_push{%lr,%r6,%r7}
         lsr      r3, r2, 4
         bmask    r6, r2, 4
         mov      r2, r0

         vbitplanes -, 0x0FFF                      SETF
         mov      r7, 0x00FFFFFF

.oloop:  min      r0, r3, 16
         mov      r4, 64

         v32ld    H32(0++,0), (r1+=r4)             REP r0
         nop

         mov      r4, 0
         mov      r5, 0
.iloop:  vand     -,             V32(0,0)+r5, r7   CLRA UACC
         vshl     V32(16,0)+r4,  V8(0,1)+r5, 24         SACC
         v32lsr   V16(16,1)+r4,  V32(0,1)+r5, 8
         vmov     V16(16,33)+r4, V16(0,2)+r5
         vmov     -,             V8(0,34)+r5       CLRA UACC
         vshl     V32(16,2)+r4,  V32(0,3)+r5, 8         SACC
         add      r4, 3
         addcmpbne r5, 4, 16, .iloop

         mov      r4, 48
         v32st    H32(16++,0), (r2+=r4)            REP r0 IFNZ

         addscale r1, r0 * 64
         addscale r2, r0 * 32
         addscale r2, r0 * 16

         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop

         addcmpbne r6, 0, 0, .eloop ; predict not-taken
         ldm r6-r7,pc,(sp++)

.eloop:  ld       r0, (r1++)
         nop
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         addcmpbne r6, -1, 0, .eloop

         ldm      r6-r7,pc,(sp++)
         .cfa_pop{%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_unextend_32to24s
;;;
;;; SYNOPSIS
;;;   void vclib_extend_32to24s(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Contract some memory from 32-bit words to to 24-bit words, signed-saturating.

.function vclib_unextend_32to24s ; {{{
         stm      r6-r7,lr,(--sp)
         .cfa_push{%lr,%r6,%r7}
         lsr      r3, r2, 4
         bmask    r6, r2, 4
         mov      r2, r0

         vbitplanes -, 0x0FFF                      SETF
         mov      r7, 0xFFFFFF00

.oloop:  min      r0, r3, 16
         mov      r4, 64

         v32ld    H32(0++,0), (r1+=r4)             REP r0
         v32shls  H32(0++,0), H32(0++,0), 8        REP r0

         mov      r4, 0
         mov      r5, 0
.iloop:  vlsr     -,             V32(0,0)+r5, 8    CLRA UACC
         vshl     V32(16,0)+r4,  V8(0,17)+r5, 24        SACC
         vmov     V16(16,1)+r4,  V16(0,33)+r5
         v32lsr   V16(16,33)+r4, V32(0,2)+r5, 8
         vmov     -,             V8(0,50)+r5       CLRA UACC
         vand     V32(16,2)+r4,  V32(0,3)+r5, r7        SACC
         add      r4, 3
         addcmpbne r5, 4, 16, .iloop

         mov      r4, 48
         v32st    H32(16++,0), (r2+=r4)            REP r0 IFNZ

         addscale r1, r0 * 64
         addscale r2, r0 * 32
         addscale r2, r0 * 16

         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop

         addcmpbne r6, 0, 0, .eloop ; predict not-taken
         ldm r6-r7,pc,(sp++)

.eloop:  ld       r0, (r1++)
         shls     r0, 8
         ror      r0, 8
         nop
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         addcmpbne r6, -1, 0, .eloop

         ldm      r6-r7,pc,(sp++)
         .cfa_pop{%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_unextend_32hito24
;;;
;;; SYNOPSIS
;;;   void vclib_extend_32hito24(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   Contract some memory from 32-bit words to to 24-bit words.

.function vclib_unextend_32hito24 ; {{{
         stm      r6-r7,lr,(--sp)
         .cfa_push{%lr,%r6,%r7}
         lsr      r3, r2, 4
         bmask    r6, r2, 4
         mov      r2, r0

         vbitplanes -, 0x0FFF                      SETF
         mov      r7, 0xFFFFFF00

.oloop:  min      r0, r3, 16
         mov      r4, 64

         v32ld    H32(0++,0), (r1+=r4)             REP r0
         nop

         mov      r4, 0
         mov      r5, 0
.iloop:  vlsr     -,             V32(0,0)+r5, 8    CLRA UACC
         vshl     V32(16,0)+r4,  V8(0,17)+r5, 24        SACC
         vmov     V16(16,1)+r4,  V16(0,33)+r5
         v32lsr   V16(16,33)+r4, V32(0,2)+r5, 8
         vmov     -,             V8(0,50)+r5       CLRA UACC
         vand     V32(16,2)+r4,  V32(0,3)+r5, r7        SACC
         add      r4, 3
         addcmpbne r5, 4, 16, .iloop

         mov      r4, 48
         v32st    H32(16++,0), (r2+=r4)            REP r0 IFNZ

         addscale r1, r0 * 64
         addscale r2, r0 * 32
         addscale r2, r0 * 16

         sub      r3, r0
         addcmpbne r3, 0, 0, .oloop

         addcmpbne r6, 0, 0, .eloop ; predict not-taken
         ldm r6-r7,pc,(sp++)

.eloop:  ld       r0, (r1++)
         ror      r0, 8
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         ror      r0, 8
         stb      r0, (r2++)
         addcmpbne r6, -1, 0, .eloop

         ldm      r6-r7,pc,(sp++)
         .cfa_pop{%r7,%r6,%pc}
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_signasl_32
;;;
;;; SYNOPSIS
;;;   void vclib_signasl_32(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  shift,   /* r2 */
;;;      int                  count);  /* r3 */
;;;
;;; FUNCTION
;;;   Read \a count 32-bit signed words, shift left by \a shift (signed), and
;;;   write them back to \a dst.

.function vclib_signasl_32 ; {{{
.set CHUNK, 16
         lsr      r4, r3, 4
         bmask    r5, r3, 4
         addscale r1, r4 * (CHUNK * 4)
         addscale r3, r0, r4 * (CHUNK * 4)

         mov      r0, -1
         cmp      r5, 0
         shl      r0, r5
         vbitplanes -, r0                          SETF
         mov      r5, 64
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 4)
         subscale r3, r0 * (CHUNK * 4)
.entry:  v32ld    H32(0++,0), (r1+=r5)             REP r0
         v32signasl H32(0++,0), H32(0++,0), r2     REP r0
         v32st    H32(0++,0), (r3+=r5)             REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_signasls_32
;;;
;;; SYNOPSIS
;;;   void vclib_signasls_32(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  shift,   /* r2 */
;;;      int                  count);  /* r3 */
;;;
;;; FUNCTION
;;;   Read \a count 32-bit signed words, shift left by \a shift (signed,
;;;   saturating), and write them back to \a dst.

.function vclib_signasls_32 ; {{{
.set CHUNK, 16
         lsr      r4, r3, 4
         bmask    r5, r3, 4
         addscale r1, r4 * (CHUNK * 4)
         addscale r3, r0, r4 * (CHUNK * 4)

         mov      r0, -1
         cmp      r5, 0
         shl      r0, r5
         vbitplanes -, r0                          SETF
         mov      r5, 64
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 4)
         subscale r3, r0 * (CHUNK * 4)
.entry:  v32ld    H32(0++,0), (r1+=r5)             REP r0
         v32signasls H32(0++,0), H32(0++,0), r2    REP r0
         v32st    H32(0++,0), (r3+=r5)             REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_swab
;;;
;;; SYNOPSIS
;;;   void vclib_swab(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   swab() with SIMD acceleration

.function vclib_swab ; {{{
.set CHUNK, 16
         asr      r3, r2, 1
         addcmpbmi r2, 0, 0, .exit
         lsr      r4, r3, 4
         bmask    r5, r3, 4
         addscale r1, r4 * (CHUNK * 2)
         addscale r3, r0, r4 * (CHUNK * 2)

         mov      r0, -1
         cmp      r5, 0
         shl      r0, r5
         vbitplanes -, r0                          SETF
         mov      r5, 32
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 2)
         subscale r3, r0 * (CHUNK * 2)
.entry:  v16ld    H32(0++,0), (r1+=r5)             REP r0
         v16ror   H16(0++,0), H16(0++,0), 8        REP r0
         v16st    H32(0++,0), (r3+=r5)             REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
.exit:   b        lr
.endfn ; }}}


.FORGET
;;; *****************************************************************************
;;; NAME
;;;   vclib_swab2
;;;
;;; SYNOPSIS
;;;   void vclib_swab2(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      int                  count);  /* r2 */
;;;
;;; FUNCTION
;;;   swab() with SIMD acceleration, and 4-byte reversal

.function vclib_swab2 ; {{{
.set CHUNK, 16
         asr      r3, r2, 2
         addcmpbmi r2, 0, 0, .exit
         lsr      r4, r3, 4
         bmask    r5, r3, 4
         addscale r1, r4 * (CHUNK * 4)
         addscale r3, r0, r4 * (CHUNK * 4)

         mov      r0, -1
         cmp      r5, 0
         shl      r0, r5
         vbitplanes -, r0                          SETF
         mov      r5, 64
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 4)
         subscale r3, r0 * (CHUNK * 4)
.entry:  v32ld    H32(0++,0), (r1+=r5)             REP r0
         v16ror   H16(32++,0), H16(0++,32), 8      REP r0
         v16ror   H16(32++,32), H16(0++,0), 8      REP r0
         v32st    H32(32++,0), (r3+=r5)            REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
.exit:   b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_reorder
;;;
;;; SYNOPSIS
;;;   void vclib_reorder(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      signed char   const *indices, /* r2 */
;;;      int                  period,  /* r3 */
;;;      int                  count);  /* r4 */
;;;
;;; FUNCTION
;;;   Reorder bytes of an array

.function vclib_reorder ; {{{
         stm      r6-r23,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18,%r19,%r20,%r21,%r22,%r23}

         vmov     V8(16,0++), 0 REP 16 ; handle the -1 indices here (16 of them to trap the unrolling arithmetic below)
         mov      r5, 64 * 16 

         ldsb     r8, (r2)
         cmp      r8, 0
         mov.mi   r8, r5
         addcmpbls r3, 0, 1, .unrl1
         ldsb     r9, (r2+1)
         cmp      r9, 0
         mov.mi   r9, r5
         addcmpbls r3, 0, 2, .unrl2
         ldsb     r10, (r2+2)
         nop
         ldsb     r11, (r2+3)
         cmp      r10, 0
         mov.mi   r10, r5
         cmp      r11, 0
         mov.mi   r11, r5
         addcmpbls r3, 0, 4, .unrl4
         ldsb     r12, (r2+4)
         nop
         ldsb     r13, (r2+5)
         cmp      r12, 0
         ldsb     r14, (r2+6)
         mov.mi   r12, r5
         cmp      r13, 0
         ldsb     r15, (r2+7)
         mov.mi   r13, r5
         cmp      r14, 0
         mov.mi   r14, r5
         cmp      r15, 0
         mov.mi   r15, r5
         addcmpbls r3, 0, 8, .unrl8
         ldsb     r16, (r2+8)
         nop
         ldsb     r17, (r2+9)
         cmp      r16, 0
         ldsb     r18, (r2+10)
         mov.mi   r16, r5
         cmp      r17, 0
         ldsb     r19, (r2+11)
         mov.mi   r17, r5
         cmp      r18, 0
         ldsb     r20, (r2+12)
         mov.mi   r18, r5
         cmp      r19, 0
         ldsb     r21, (r2+13)
         mov.mi   r19, r5
         cmp      r20, 0
         ldsb     r22, (r2+14)
         mov.mi   r20, r5
         cmp      r21, 0
         ldsb     r23, (r2+15)
         mov.mi   r21, r5
         cmp      r22, 0
         mov.mi   r22, r5
         cmp      r23, 0
         mov.mi   r23, r5
         b        .nounrl
.unrl1:  add      r9, r8, 1
         nop
.unrl2:  add      r10, r8, 2
         add      r11, r9, 2
.unrl4:  add      r12, r8, 4
         add      r13, r9, 4
         add      r14, r10, 4
         add      r15, r11, 4
.unrl8:  add      r16, r8, 8
         add      r17, r9, 8
         add      r18, r10, 8
         add      r19, r11, 8
         add      r20, r12, 8
         add      r21, r13, 8
         add      r22, r14, 8
         add      r23, r15, 8
.nounrl:

.set CHUNK, 16
         asr      r3, r4, 0 ; TODO: clean up
         addcmpbmi r4, 0, 0, .exit
         lsr      r4, r3, 4
         bmask    r2, r3, 4
         addscale r1, r4 * (CHUNK * 1)
         addscale r3, r0, r4 * (CHUNK * 1)

         mov      r0, -1
         cmp      r2, 0
         shl      r0, r2
         vbitplanes -, r0                          SETF
         mov      r2, 16
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 1)
         subscale r3, r0 * (CHUNK * 1)
.entry:  v8ld     H8(0++,0), (r1+=r2)             REP r0
         vmov     V8(32, 0), V8(0,0)+r8
         vmov     V8(32, 1), V8(0,0)+r9
         vmov     V8(32, 2), V8(0,0)+r10
         vmov     V8(32, 3), V8(0,0)+r11
         vmov     V8(32, 4), V8(0,0)+r12
         mov      r5, r15
         vmov     V8(32, 5), V8(0,0)+r13
         mov      r6, r16
         vmov     V8(32, 6), V8(0,0)+r14
         mov      r7, r17
         vmov     V8(32, 7), V8(0,0)+r5
         mov      r5, r18
         vmov     V8(32, 8), V8(0,0)+r6
         mov      r6, r19
         vmov     V8(32, 9), V8(0,0)+r7
         mov      r7, r20
         vmov     V8(32,10), V8(0,0)+r5
         mov      r5, r21
         vmov     V8(32,11), V8(0,0)+r6
         mov      r6, r22
         vmov     V8(32,12), V8(0,0)+r7
         mov      r7, r23
         vmov     V8(32,13), V8(0,0)+r5
         vmov     V8(32,14), V8(0,0)+r6
         vmov     V8(32,15), V8(0,0)+r7

         v8st     H8(32++,0), (r3+=r2)            REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
.exit:   ldm      r6-r23,pc,(sp++)
         .cfa_pop{%r23,%r22,%r21,%r20,%r19,%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vclib_reorder2
;;;
;;; SYNOPSIS
;;;   void vclib_reorder2(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      signed char   const *indices, /* r2 */
;;;      int                  period,  /* r3 */
;;;      int                  count);  /* r4 */
;;;
;;; FUNCTION
;;;   Reorder halfwords of an array

.function vclib_reorder2 ; {{{
         stm      r6-r23,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18,%r19,%r20,%r21,%r22,%r23}

         vmov     V16(16,0++), 0 REP 16 ; handle the -1 indices here (16 of them to trap the unrolling arithmetic below)
         mov      r5, 64 * 16 

         ldsb     r8, (r2)
         cmp      r8, 0
         mov.mi   r8, r5
         addcmpbls r3, 0, 1, .unrl1
         ldsb     r9, (r2+1)
         cmp      r9, 0
         mov.mi   r9, r5
         addcmpbls r3, 0, 2, .unrl2
         ldsb     r10, (r2+2)
         nop
         ldsb     r11, (r2+3)
         cmp      r10, 0
         mov.mi   r10, r5
         cmp      r11, 0
         mov.mi   r11, r5
         addcmpbls r3, 0, 4, .unrl4
         ldsb     r12, (r2+4)
         nop
         ldsb     r13, (r2+5)
         cmp      r12, 0
         ldsb     r14, (r2+6)
         mov.mi   r12, r5
         cmp      r13, 0
         ldsb     r15, (r2+7)
         mov.mi   r13, r5
         cmp      r14, 0
         mov.mi   r14, r5
         cmp      r15, 0
         mov.mi   r15, r5
         addcmpbls r3, 0, 8, .unrl8
         ldsb     r16, (r2+8)
         nop
         ldsb     r17, (r2+9)
         cmp      r16, 0
         ldsb     r18, (r2+10)
         mov.mi   r16, r5
         cmp      r17, 0
         ldsb     r19, (r2+11)
         mov.mi   r17, r5
         cmp      r18, 0
         ldsb     r20, (r2+12)
         mov.mi   r18, r5
         cmp      r19, 0
         ldsb     r21, (r2+13)
         mov.mi   r19, r5
         cmp      r20, 0
         ldsb     r22, (r2+14)
         mov.mi   r20, r5
         cmp      r21, 0
         ldsb     r23, (r2+15)
         mov.mi   r21, r5
         cmp      r22, 0
         mov.mi   r22, r5
         cmp      r23, 0
         mov.mi   r23, r5
         b        .nounrl
.unrl1:  add      r9, r8, 1
         nop
.unrl2:  add      r10, r8, 2
         add      r11, r9, 2
.unrl4:  add      r12, r8, 4
         add      r13, r9, 4
         add      r14, r10, 4
         add      r15, r11, 4
.unrl8:  add      r16, r8, 8
         add      r17, r9, 8
         add      r18, r10, 8
         add      r19, r11, 8
         add      r20, r12, 8
         add      r21, r13, 8
         add      r22, r14, 8
         add      r23, r15, 8
.nounrl:

.set CHUNK, 16
         asr      r3, r4, 0 ; TODO: clean up
         addcmpbmi r4, 0, 0, .exit
         lsr      r4, r3, 4
         bmask    r2, r3, 4
         addscale r1, r4 * (CHUNK * 2)
         addscale r3, r0, r4 * (CHUNK * 2)

         mov      r0, -1
         cmp      r2, 0
         shl      r0, r2
         vbitplanes -, r0                          SETF
         mov      r2, 32
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 2)
         subscale r3, r0 * (CHUNK * 2)
.entry:  v16ld    H16(0++,0), (r1+=r2)             REP r0
         vmov     V16(32, 0), V16(0,0)+r8
         vmov     V16(32, 1), V16(0,0)+r9
         vmov     V16(32, 2), V16(0,0)+r10
         vmov     V16(32, 3), V16(0,0)+r11
         vmov     V16(32, 4), V16(0,0)+r12
         mov      r5, r15
         vmov     V16(32, 5), V16(0,0)+r13
         mov      r6, r16
         vmov     V16(32, 6), V16(0,0)+r14
         mov      r7, r17
         vmov     V16(32, 7), V16(0,0)+r5
         mov      r5, r18
         vmov     V16(32, 8), V16(0,0)+r6
         mov      r6, r19
         vmov     V16(32, 9), V16(0,0)+r7
         mov      r7, r20
         vmov     V16(32,10), V16(0,0)+r5
         mov      r5, r21
         vmov     V16(32,11), V16(0,0)+r6
         mov      r6, r22
         vmov     V16(32,12), V16(0,0)+r7
         mov      r7, r23
         vmov     V16(32,13), V16(0,0)+r5
         vmov     V16(32,14), V16(0,0)+r6
         vmov     V16(32,15), V16(0,0)+r7

         v16st    H16(32++,0), (r3+=r2)            REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
.exit:   ldm      r6-r23,pc,(sp++)
         .cfa_pop{%r23,%r22,%r21,%r20,%r19,%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   vclib_reorder4
;;;
;;; SYNOPSIS
;;;   void vclib_reorder4(
;;;      void                *dst,     /* r0 */
;;;      void          const *src,     /* r1 */
;;;      signed char   const *indices, /* r2 */
;;;      int                  period,  /* r3 */
;;;      int                  count);  /* r4 */
;;;
;;; FUNCTION
;;;   Reorder words of an array

.function vclib_reorder4 ; {{{
         stm      r6-r23,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18,%r19,%r20,%r21,%r22,%r23}

         vmov     V32(16,0++), 0 REP 16 ; handle the -1 indices here (16 of them to trap the unrolling arithmetic below)
         mov      r5, 64 * 16 

         ldsb     r8, (r2)
         cmp      r8, 0
         mov.mi   r8, r5
         addcmpbls r3, 0, 1, .unrl1
         ldsb     r9, (r2+1)
         cmp      r9, 0
         mov.mi   r9, r5
         addcmpbls r3, 0, 2, .unrl2
         ldsb     r10, (r2+2)
         nop
         ldsb     r11, (r2+3)
         cmp      r10, 0
         mov.mi   r10, r5
         cmp      r11, 0
         mov.mi   r11, r5
         addcmpbls r3, 0, 4, .unrl4
         ldsb     r12, (r2+4)
         nop
         ldsb     r13, (r2+5)
         cmp      r12, 0
         ldsb     r14, (r2+6)
         mov.mi   r12, r5
         cmp      r13, 0
         ldsb     r15, (r2+7)
         mov.mi   r13, r5
         cmp      r14, 0
         mov.mi   r14, r5
         cmp      r15, 0
         mov.mi   r15, r5
         addcmpbls r3, 0, 8, .unrl8
         ldsb     r16, (r2+8)
         nop
         ldsb     r17, (r2+9)
         cmp      r16, 0
         ldsb     r18, (r2+10)
         mov.mi   r16, r5
         cmp      r17, 0
         ldsb     r19, (r2+11)
         mov.mi   r17, r5
         cmp      r18, 0
         ldsb     r20, (r2+12)
         mov.mi   r18, r5
         cmp      r19, 0
         ldsb     r21, (r2+13)
         mov.mi   r19, r5
         cmp      r20, 0
         ldsb     r22, (r2+14)
         mov.mi   r20, r5
         cmp      r21, 0
         ldsb     r23, (r2+15)
         mov.mi   r21, r5
         cmp      r22, 0
         mov.mi   r22, r5
         cmp      r23, 0
         mov.mi   r23, r5
         b        .nounrl
.unrl1:  add      r9, r8, 1
         nop
.unrl2:  add      r10, r8, 2
         add      r11, r9, 2
.unrl4:  add      r12, r8, 4
         add      r13, r9, 4
         add      r14, r10, 4
         add      r15, r11, 4
.unrl8:  add      r16, r8, 8
         add      r17, r9, 8
         add      r18, r10, 8
         add      r19, r11, 8
         add      r20, r12, 8
         add      r21, r13, 8
         add      r22, r14, 8
         add      r23, r15, 8
.nounrl:

.set CHUNK, 16
         asr      r3, r4, 0 ; TODO: clean up
         addcmpbmi r4, 0, 0, .exit
         lsr      r4, r3, 4
         bmask    r2, r3, 4
         addscale r1, r4 * (CHUNK * 4)
         addscale r3, r0, r4 * (CHUNK * 4)

         mov      r0, -1
         cmp      r2, 0
         shl      r0, r2
         vbitplanes -, r0                          SETF
         mov      r2, 64
         mov      r0, 1
         add.ne   r4, 1
         bne      .entry

.oloop:  vbitplanes -, 0                           SETF
         min      r0, r4, CHUNK
         subscale r1, r0 * (CHUNK * 4)
         subscale r3, r0 * (CHUNK * 4)
.entry:  v32ld    H32(0++,0), (r1+=r2)             REP r0
         vmov     V32(32, 0), V32(0,0)+r8
         vmov     V32(32, 1), V32(0,0)+r9
         vmov     V32(32, 2), V32(0,0)+r10
         vmov     V32(32, 3), V32(0,0)+r11
         vmov     V32(32, 4), V32(0,0)+r12
         mov      r5, r15
         vmov     V32(32, 5), V32(0,0)+r13
         mov      r6, r16
         vmov     V32(32, 6), V32(0,0)+r14
         mov      r7, r17
         vmov     V32(32, 7), V32(0,0)+r5
         mov      r5, r18
         vmov     V32(32, 8), V32(0,0)+r6
         mov      r6, r19
         vmov     V32(32, 9), V32(0,0)+r7
         mov      r7, r20
         vmov     V32(32,10), V32(0,0)+r5
         mov      r5, r21
         vmov     V32(32,11), V32(0,0)+r6
         mov      r6, r22
         vmov     V32(32,12), V32(0,0)+r7
         mov      r7, r23
         vmov     V32(32,13), V32(0,0)+r5
         vmov     V32(32,14), V32(0,0)+r6
         vmov     V32(32,15), V32(0,0)+r7

         v32st    H32(32++,0), (r3+=r2)            REP r0 IFZ
         sub      r4, r0
         addcmpbne r4, 0, 0, .oloop
.exit:   ldm      r6-r23,pc,(sp++)
         .cfa_pop{%r23,%r22,%r21,%r20,%r19,%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}
.FORGET
