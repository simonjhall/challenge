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
;;;   vclib_spread_core
;;;
;;; SYNOPSIS
;;;   void vclib_spread_core(
;;;      int               r0,         /* r0 */
;;;      int               count,      /* r1 */
;;;      void             *dst,        /* r2 */
;;;      int               storepitch, /* r3 */
;;;      void       const *src,        /* r4 */
;;;      int               loadpitch,  /* r5 */
;;;      int               dcount,     /* (sp+0) */
;;;      int               scount);    /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   Take a continuous run of bytes and copy small chunks and add zero-padding

.function vclib_spread_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

.alias   r1, count
.alias   r2, dptr
.alias   r3, dpitch
.alias   r4, sptr
.alias   r5, spitch

.alias   r6, dcount
.alias   r7, scount

.alias   r8, didx
.alias   r9, sidx

         ld       dcount, (sp+24)
         ld       scount, (sp+28)

.oloop:  min      r0, count, 16
         cmp      spitch, 16
         vld      V(0,0++), (sptr+=spitch)            REP r0
         bls      .loaded
         cmp      spitch, 32
         vld      V(16,0++), (sptr+16+=spitch)        REP r0
         bls      .loaded
         cmp      spitch, 48
         vld      V(32,0++), (sptr+32+=spitch)        REP r0
         bls      .loaded
         vld      V(48,0++), (sptr+48+=spitch)        REP r0
.loaded: addscale sptr, spitch * 16
         nop

         mov      sidx, 0
         mov      didx, 0
         mov      r10, 0
         mov      r0, scount
.iloop:  vmov     H(0++,32)+didx, H(0++,0)+sidx       REP r0
         addscale didx, r0 * 64
         addscale sidx, r0 * 64
         sub      r0, dcount, scount
         vmov     H(0++,32)+didx, 0                   REP r0
         addscale didx, r0 * 64
         mov      r0, scount
         addcmpblo r10, dcount, dpitch, .iloop

         sub      r0, dpitch, 1
         mov      r10, -2
         ; we rely on using only the bottom 5 bits for shift index (bit 6 is known to be 1) 
         shl      r10, r0

         min      r0, count, 16
         vst      V(0,32++), (dptr+=dpitch)           REP r0
         vst      V(16,32++), (dptr+16+=dpitch)       REP r0
         cmp      dpitch, 48

         vbitplanes -, r10                            SETF
         ror      r10, 16
         vst      V(32,32++), (dptr+32+=dpitch)       REP r0 IFZ
         bls      .stored
         vbitplanes -, r10                            SETF
         vst      V(48,32++), (dptr+48+=dpitch)       REP r0 IFZ
.stored: addscale dptr, dpitch * 16

         sub      count, r0
         addcmpbne count, 0, 0, .oloop

         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}

.free$dptr
.free$dpitch
.free$sptr
.free$spitch
.free$dcount
.free$scount
.free$count
.free$didx
.free$sidx

.endfn ; }}}


.FORGET
;;; *****************************************************************************
;;; NAME
;;;   vclib_spread2_core
;;;
;;; SYNOPSIS
;;;   void vclib_spread2_core(
;;;      int               r0,         /* r0 */
;;;      int               count,      /* r1 */
;;;      void             *dst,        /* r2 */
;;;      int               storepitch, /* r3 */
;;;      void       const *src,        /* r4 */
;;;      int               loadpitch,  /* r5 */
;;;      int               dcount,     /* (sp+0) */
;;;      int               scount);    /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   Take a continuous run of bytes and copy small chunks and add zero-padding

.function vclib_spread2_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

.alias   r1, count
.alias   r2, dptr
.alias   r3, dpitch
.alias   r4, sptr
.alias   r5, spitch

.alias   r6, dcount
.alias   r7, scount

.alias   r8, didx
.alias   r9, sidx

         ld       dcount, (sp+24)
         ld       scount, (sp+28)

.oloop:  min      r0, count, 16
         cmp      spitch, 32
         vld      V16(0,0++), (sptr+=spitch)          REP r0
         bls      .loaded
         cmp      spitch, 64
         vld      V16(16,0++), (sptr+32+=spitch)      REP r0
         bls      .loaded
         cmp      spitch, 96
         vld      V16(32,0++), (sptr+64+=spitch)      REP r0
         bls      .loaded
         vld      V16(48,0++), (sptr+96+=spitch)      REP r0
.loaded: addscale sptr, spitch * 16
         lsr      dpitch, 1

         mov      sidx, 0
         mov      didx, 0
         mov      r10, 0
         mov      r0, scount
.iloop:  vmov     H16(0++,32)+didx, H16(0++,0)+sidx   REP r0
         addscale didx, r0 * 64
         addscale sidx, r0 * 64
         sub      r0, dcount, scount
         vmov     H16(0++,32)+didx, 0                 REP r0
         addscale didx, r0 * 64
         mov      r0, scount
         addcmpblo r10, dcount, dpitch, .iloop

         sub      r0, dpitch, 1
         mov      r10, -2
         shl      dpitch, 1
         ; we rely on using only the bottom 5 bits for shift index (bit 6 is known to be 1) 
         shl      r10, r0

         min      r0, count, 16
         vst      V16(0,32++), (dptr+=dpitch)         REP r0
         vst      V16(16,32++), (dptr+32+=dpitch)     REP r0
         cmp      dpitch, 96

         vbitplanes -, r10                            SETF
         ror      r10, 16
         vst      V16(32,32++), (dptr+64+=dpitch)     REP r0 IFZ
         bls      .stored
         vbitplanes -, r10                            SETF
         vst      V16(48,32++), (dptr+96+=dpitch)     REP r0 IFZ
.stored: addscale dptr, dpitch * 16

         sub      count, r0
         addcmpbne count, 0, 0, .oloop

         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}

.free$dptr
.free$dpitch
.free$sptr
.free$spitch
.free$dcount
.free$scount
.free$count
.free$didx
.free$sidx

.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   vclib_unspread_core
;;;
;;; SYNOPSIS
;;;   void vclib_unspread_core(
;;;      int               r0,         /* r0 */
;;;      int               count,      /* r1 */
;;;      void             *dst,        /* r2 */
;;;      int               storepitch, /* r3 */
;;;      void       const *src,        /* r4 */
;;;      int               loadpitch,  /* r5 */
;;;      int               dcount,     /* (sp+0) */
;;;      int               scount);    /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   Take a continuous run of bytes and copy small chunks and remove padding

.function vclib_unspread_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

.alias   r1, count
.alias   r2, dptr
.alias   r3, dpitch
.alias   r4, sptr
.alias   r5, spitch

.alias   r6, dcount
.alias   r7, scount

.alias   r8, didx
.alias   r9, sidx

         ld       dcount, (sp+24)
         ld       scount, (sp+28)

.oloop:  min      r0, count, 16

         vld      V(0,0++), (sptr+=spitch)            REP r0
         vld      V(16,0++), (sptr+16+=spitch)        REP r0
         cmp      spitch, 48
         vld      V(32,0++), (sptr+32+=spitch)        REP r0
         bls      .loaded
         vld      V(48,0++), (sptr+48+=spitch)        REP r0
.loaded: addscale sptr, spitch * 16
         nop

         mov      sidx, 0
         mov      didx, 0
         mov      r10, 0
         mov      r0, dcount
.iloop:  vmov     H(0++,0)+didx, H(0++,0)+sidx        REP r0
         addscale didx, dcount * 64
         addscale sidx, scount * 64
         addcmpblo r10, dcount, dpitch, .iloop

         sub      r0, dpitch, 1
         mov      r8, -2
         mov      r9, -2

         shl      r9, r0
         min      r0, 31
         shl      r8, r0

         min      r0, count, 16
         cmp      dpitch, 16
         vbitplanes -, r8                             SETF
         ror      r8, 16
         vst      V(0,0++), (dptr+=dpitch)            REP r0 IFZ
         bls      .stored
         vbitplanes -, r8                             SETF
         cmp      dpitch, 32
         vst      V(16,0++), (dptr+16+=dpitch)        REP r0 IFZ
         bls      .stored
         cmp      dpitch, 48
         vbitplanes -, r9                             SETF
         ror      r9, 16
         vst      V(32,0++), (dptr+32+=dpitch)        REP r0 IFZ
         bls      .stored
         vbitplanes -, r9                             SETF
         vst      V(48,0++), (dptr+48+=dpitch)        REP r0 IFZ
.stored: addscale dptr, dpitch * 16

         sub      count, r0
         addcmpbne count, 0, 0, .oloop

         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}

.free$dptr
.free$dpitch
.free$sptr
.free$spitch
.free$dcount
.free$scount
.free$count
.free$didx
.free$sidx

.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   vclib_unspread2_core
;;;
;;; SYNOPSIS
;;;   void vclib_unspread2_core(
;;;      int               r0,         /* r0 */
;;;      int               count,      /* r1 */
;;;      void             *dst,        /* r2 */
;;;      int               storepitch, /* r3 */
;;;      void       const *src,        /* r4 */
;;;      int               loadpitch,  /* r5 */
;;;      int               dcount,     /* (sp+0) */
;;;      int               scount);    /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   Take a continuous run of bytes and copy small chunks and remove padding

.function vclib_unspread2_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

.alias   r1, count
.alias   r2, dptr
.alias   r3, dpitch
.alias   r4, sptr
.alias   r5, spitch

.alias   r6, dcount
.alias   r7, scount

.alias   r8, didx
.alias   r9, sidx

         ld       dcount, (sp+24)
         ld       scount, (sp+28)

.oloop:  min      r0, count, 16
         vld      V16(0,0++), (sptr+=spitch)          REP r0
         vld      V16(16,0++), (sptr+32+=spitch)      REP r0
         cmp      spitch, 96
         vld      V16(32,0++), (sptr+64+=spitch)      REP r0
         bls      .loaded
         vld      V16(48,0++), (sptr+96+=spitch)      REP r0
.loaded: addscale sptr, spitch * 16
         lsr      dpitch, 1

         mov      sidx, 0
         mov      didx, 0
         mov      r10, 0
         mov      r0, dcount
.iloop:  vmov     H16(0++,0)+didx, H16(0++,0)+sidx    REP r0
         addscale didx, dcount * 64
         addscale sidx, scount * 64
         addcmpblo r10, dcount, dpitch, .iloop

         sub      r0, dpitch, 1
         mov      r8, -2
         mov      r9, -2
         shl      dpitch, 1

         shl      r9, r0
         min      r0, 31
         shl      r8, r0

         min      r0, count, 16
         cmp      dpitch, 32
         vbitplanes -, r8                             SETF
         ror      r8, 16
         vst      V16(0,0++), (dptr+=dpitch)          REP r0 IFZ
         bls      .stored
         vbitplanes -, r8                             SETF
         cmp      dpitch, 64
         vst      V16(16,0++), (dptr+32+=dpitch)      REP r0 IFZ
         bls      .stored
         cmp      dpitch, 96
         vbitplanes -, r9                             SETF
         ror      r9, 16
         vst      V16(32,0++), (dptr+64+=dpitch)      REP r0 IFZ
         bls      .stored
         vbitplanes -, r9                             SETF
         vst      V16(48,0++), (dptr+96+=dpitch)      REP r0 IFZ
.stored: addscale dptr, dpitch * 16

         sub      count, r0
         addcmpbne count, 0, 0, .oloop

         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}

.free$dptr
.free$dpitch
.free$sptr
.free$spitch
.free$dcount
.free$scount
.free$count
.free$didx
.free$sidx

.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   vclib_unspread4_core
;;;
;;; SYNOPSIS
;;;   void vclib_unspread4_core(
;;;      int               r0,         /* r0 */
;;;      int               count,      /* r1 */
;;;      void             *dst,        /* r2 */
;;;      int               storepitch, /* r3 */
;;;      void       const *src,        /* r4 */
;;;      int               loadpitch,  /* r5 */
;;;      int               dcount,     /* (sp+0) */
;;;      int               scount);    /* (sp+4) */
;;;   
;;;
;;; FUNCTION
;;;   Take a continuous run of bytes and copy small chunks and remove padding

.function vclib_unspread4_core ; {{{
         stm      r6-r10,lr,(--sp)
         .cfa_push{%lr,%r6,%r7,%r8,%r9,%r10}

.alias   r1, count
.alias   r2, dptr
.alias   r3, dpitch
.alias   r4, sptr
.alias   r5, spitch

.alias   r6, dcount
.alias   r7, scount

.alias   r8, didx
.alias   r9, sidx

         ld       dcount, (sp+24)
         ld       scount, (sp+28)

.oloop:  min      r0, count, 16
         vld      V32(0,0++), (sptr+=spitch)          REP r0
         vld      V32(16,0++), (sptr+64+=spitch)      REP r0
         cmp      spitch, 192
         vld      V32(32,0++), (sptr+128+=spitch)     REP r0
         bls      .loaded
         vld      V32(48,0++), (sptr+192+=spitch)     REP r0
.loaded: addscale sptr, spitch * 16
         lsr      dpitch, 2

         mov      sidx, 0
         mov      didx, 0
         mov      r10, 0
         mov      r0, dcount
.iloop:  vmov     H32(0++,0)+didx, H32(0++,0)+sidx    REP r0
         addscale didx, dcount * 64
         addscale sidx, scount * 64
         addcmpblo r10, dcount, dpitch, .iloop

         sub      r0, dpitch, 1
         mov      r8, -2
         mov      r9, -2
         shl      dpitch, 2

         shl      r9, r0
         min      r0, 31
         shl      r8, r0

         min      r0, count, 16
         cmp      dpitch, 64
         vbitplanes -, r8                             SETF
         ror      r8, 16
         vst      V32(0,0++), (dptr+=dpitch)          REP r0 IFZ
         bls      .stored
         vbitplanes -, r8                             SETF
         cmp      dpitch, 128
         vst      V32(16,0++), (dptr+64+=dpitch)      REP r0 IFZ
         bls      .stored
         cmp      dpitch, 192
         vbitplanes -, r9                             SETF
         ror      r9, 16
         vst      V32(32,0++), (dptr+128+=dpitch)     REP r0 IFZ
         bls      .stored
         vbitplanes -, r9                             SETF
         vst      V32(48,0++), (dptr+192+=dpitch)     REP r0 IFZ
.stored: addscale dptr, dpitch * 16

         sub      count, r0
         addcmpbne count, 0, 0, .oloop

         ldm      r6-r10,pc,(sp++)
         .cfa_pop{%r10,%r9,%r8,%r7,%r6,%pc}

.free$dptr
.free$dpitch
.free$sptr
.free$spitch
.free$dcount
.free$scount
.free$count
.free$didx
.free$sidx

.endfn ; }}}


