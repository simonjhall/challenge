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

;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_h_reduce2_y_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_reduce2_y_stripe(unsigned char *dest, unsigned int dpitch, unsigned char *src, unsigned int spitch,
;;;                                    unsigned int columns, unsigned int step)
;;; 
;;; FUNCTION
;;;   Reduce a horizontal stripe of yuvuv128 y data by a factor of 2.0 to 4.0

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rnblocks, r12
.define Rnum128, r13
.define Rdoffset, r17
.define Rsoffset, r18

.function yuvuv128_h_reduce2_y_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}
        
        lsr      r5, 1
        mov      r7, r5
        mov      Rdest, r0
        sub      Rdpitch, r1, 112
        mov      Rsrc, r2
        sub      Rspitch, r3, 96
        add      Rnblocks, r4, 15
        lsr      Rnblocks, 4
        mov      r16, 0

        lsr      r4, r7, 1
        sub      r4, 0x8000
        add      r5, r4, r7
        shl      r7, 1

        mov      Rdoffset, 0
        mov      Rsoffset, 0
        mov      Rnum128, 128

        vld      HX(48++,32),(Rsrc+=Rnum128) REP 16
        vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
        vlsr     H(0++,0), HX(48++,32), 1 REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc
        
.oloop:
        lsr      r0, r4, 16
        mov      r6, 0
.if _VC_VERSION > 2
        bic      r0, 15
.endif

        add      r2, Rsoffset, 1 ;block offset of next block
        btest    r2, 2
        add.eq   r3, Rsrc, 32
        add.ne   r3, Rsrc, Rspitch
        bmask    r2, 2
        add      r2, 1          ;block offset of next block
        btest    r2, 2
        add.eq   r2, r3, 32
        add.ne   r2, r3, Rspitch
        
        vld      HX(48++,32), (r3+=Rnum128) REP 16
        vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
        vlsr     H(0++,16)+r0, HX(48++,32), 1 REP 16

        vld      HX(48++,32), (r2+=Rnum128) REP 16
        vadd     HX(48++,32), H(48++,32), H(48++,48) REP 16
        vlsr     H(0++,32)+r0, HX(48++,32), 1 REP 16

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
        
        
        lsr      r14, r4, 16+4
        sub      r14, r16
        and      r14, 3
        add      r16, r14

        add      Rsoffset, r14
        btest    Rsoffset, 2    ;gone over a strip
        sub.ne   r14,1
        add.ne   Rsrc, Rspitch
        bmask    Rsoffset, 2
        addscale Rsrc, r14 << 5
        
        VST_REP_SAFE <H(48++,0)>, <(Rdest+=Rnum128) REP 16>
        add      Rdoffset, 1
        bmask    Rdoffset, 3
        cmp      Rdoffset, 0
        add.ne   Rdest, 16
        add.eq   Rdest, Rdpitch
        
        bmask    r4, 16+6
        bmask    r5, 16+6
        addcmpbne Rnblocks, -1, 0, .oloop

.end:
        .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rnblocks, Rnum128, Rdoffset, Rsoffset


;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_h_reduce2_uv_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_reduce2_uv_stripe(unsigned char *dest, unsigned int dpitch, unsigned char *src, unsigned int spitch,
;;;                                     unsigned int columns, unsigned int step)
;;; 
;;; FUNCTION
;;;   Reduce a horizontal stripe of yuvuv128 uv data by a factor of 2.0 to 4.0

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rnblocks, r12
.define Rnum128, r13
.define Rdoffset, r17
.define Rsoffset, r18

.function yuvuv128_h_reduce2_uv_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}

        lsr      r5, 1
        mov      r7, r5
        mov      Rdest, r0
        sub      Rdpitch, r1, 96
        mov      Rsrc, r2
        sub      Rspitch, r3, 64
        add      Rnblocks, r4, 15
        lsr      Rnblocks, 4
        mov      r16, 0

        lsr      r4, r7, 1
        sub      r4, 0x8000
        add      r5, r4, r7
        shl      r7, 1

        mov      Rdoffset, 0
        mov      Rsoffset, 0
        mov      Rnum128, 128

        vldw     H32(32++,0),(Rsrc+=Rnum128) REP 8
        vadd     H16(48++,32), H(32++,0), H(32++,32) REP 8 ;sum up u pairs
        vadd     H16(56++,32), H(32++,16), H(32++,48) REP 8 ;sum up v pairs
        vlsr     H(0++,0), HX(48++,32), 1 REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc
        
.oloop:
        lsr      r0, r4, 16
        mov      r6, 0
.if _VC_VERSION > 2
        bic      r0, 15
.endif

        btest    Rsoffset, 0
        add.eq   r3, Rsrc, 64
        add.ne   r3, Rsrc, Rspitch
        add.eq   r2, r3, Rspitch
        add.ne   r2, r3, 64
        
        vldw     H32(32++,0), (r3+=Rnum128) REP 8
        vadd     H16(48++,32), H(32++,0), H(32++,32) REP 8 ;sum up u pairs
        vadd     H16(56++,32), H(32++,16), H(32++,48) REP 8 ;sum up v pairs
        vlsr     H(0++,16)+r0, HX(48++,32), 1 REP 16

        vldw     H32(32++,0), (r2+=Rnum128) REP 8
        vadd     H16(48++,32), H(32++,0), H(32++,32) REP 8 ;sum up u pairs
        vadd     H16(56++,32), H(32++,16), H(32++,48) REP 8 ;sum up v pairs
        vlsr     H(0++,32)+r0, HX(48++,32), 1 REP 16

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
        
        lsr      r14, r4, 16+4
        sub      r14, r16
        and      r14, 3
        add      r16, r14

        btest    Rsoffset, 0
        asr      r0, r14, 1
        bmask    r1, r14, 1
        add      r1, r0
        mul.eq   r0, Rspitch
        addscale.eq r2, r0, r1 << 6
        mul.ne   r1, Rspitch
        addscale.ne r2, r1, r0 << 6
        add      Rsrc, r2
        add      Rsoffset, r14
        bmask    Rsoffset, 1

        vmov H(48++,16), H(56++,0) REP 8 ;move v alongside u
        VST_REP_SAFE <HX(48++,0)>, <(Rdest+=Rnum128) REP 8>

        add      Rdoffset, 1
        bmask    Rdoffset, 2
        cmp      Rdoffset, 0
        add.ne   Rdest, 32
        add.eq   Rdest, Rdpitch
        
        bmask    r4, 16+6
        bmask    r5, 16+6
        addcmpbne Rnblocks, -1, 0, .oloop

.end:
       .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
       ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rnblocks, Rnum128, Rdoffset, Rsoffset
        
        
;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_h_reduce_y_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_reduce_y_stripe(unsigned char *dest, unsigned int dest_pitch,
;;;                                   unsigned char *src, unsigned int src_pitch,
;;;                                   unsigned int columns, unsigned int step)
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of yuvuv128 y data

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rnblocks, r12
.define Rnum128, r13
.define Rdoffset, r17
.define Rsoffset, r18
        
.function yuvuv128_h_reduce_y_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}

        mov      r7, r5
        mov      Rdest, r0
        sub      Rdpitch, r1, 112
        mov      Rsrc, r2
        sub      Rspitch, r3, 112
        add      Rnblocks, r4, 15
        lsr      Rnblocks, 4
        mov      r16, 0

        lsr      r4, r7, 1
        sub      r4, 0x8000
        add      r5, r4, r7
        shl      r7, 1

        mov      Rdoffset, 0
        mov      Rsoffset, 0
        mov      Rnum128, 128
        
        vld      H(0++,0), (Rsrc+=Rnum128) REP 16

         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

.oloop:
        lsr      r0, r4, 16
        mov      r6, 0
.if _VC_VERSION > 2
        bic r0, 15
.endif
        add      r2, Rsoffset, 1 ;block offset of next block
        btest    r2, 3
        add.eq   r3, Rsrc, 16
        add.ne   r3, Rsrc, Rspitch
        bmask    r2, 3
        add      r2, 1          ;block offset of next block
        btest    r2, 3
        add.eq   r2, r3, 16
        add.ne   r2, r3, Rspitch
        
        vld      H(0++,16)+r0,(r3+=Rnum128)      REP 16
        vld      H(0++,32)+r0,(r2+=Rnum128)      REP 16

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
        
        lsr      r14, r4, 16+4
        sub      r14, r16
        and      r14, 3
        add      r16, r14

        add      Rsoffset, r14
        btest    Rsoffset, 3    ;gone over a strip
        sub.ne   r14,1
        add.ne   Rsrc, Rspitch
        bmask    Rsoffset, 3
        addscale Rsrc, r14 << 4
        
        VST_REP_SAFE <H(48++,0)>, <(Rdest+=Rnum128) REP 16>
        add      Rdoffset, 1
        bmask    Rdoffset, 3
        cmp      Rdoffset, 0
        add.ne   Rdest, 16
        add.eq   Rdest, Rdpitch
        
        bmask    r4, 16+6
        bmask    r5, 16+6
        addcmpbne Rnblocks, -1, 0, .oloop
        
.end:
        .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rnblocks, Rnum128, Rdoffset, Rsoffset
        
;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_h_reduce_uv_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_reduce_uv_stripe(unsigned char *dest, unsigned int dest_pitch,
;;;                                    unsigned char *src, unsigned int src_pitch,
;;;                                    unsigned int columns, unsigned int step)
;;;
;;; FUNCTION
;;;   Reduce a horizontal stripe of yuvuv128 uv data

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rnblocks, r12
.define Rnum128, r13
.define Rdoffset, r17
.define Rsoffset, r18
        
.function yuvuv128_h_reduce_uv_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}

        mov      r7, r5
        mov      Rdest, r0
        sub      Rdpitch, r1, 96
        mov      Rsrc, r2
        sub      Rspitch, r3, 96
        add      Rnblocks, r4, 15
        lsr      Rnblocks, 4
        mov      r16, 0

        lsr      r4, r7, 1
        sub      r4, 0x8000
        add      r5, r4, r7
        shl      r7, 1

        mov      Rdoffset, 0
        mov      Rsoffset, 0
        mov      Rnum128, 128
        
        vld      HX(0++,0), (Rsrc+=Rnum128) REP 8
        vmov     H(8++,0), H(0++,16) REP 8 ;move v to below the u
         ; r0 = src offset
         ; r2 = src ptr
         ; r3 = src pitch

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

.oloop:
        lsr      r0, r4, 16
        mov      r6, 0
.if _VC_VERSION > 2
        bic r0, 15
.endif
        add      r2, Rsoffset, 1 ;block offset of next block
        btest    r2, 2
        add.eq   r3, Rsrc, 32
        add.ne   r3, Rsrc, Rspitch
        bmask    r2, 2
        add      r2, 1          ;block offset of next block
        btest    r2, 2
        add.eq   r2, r3, 32
        add.ne   r2, r3, Rspitch
        
        vld      HX(32++,0),(r3+=Rnum128) REP 8
        vld      HX(40++,0),(r2+=Rnum128) REP 8

        vmov     H(0++,16)+r0, H(32++,0) REP 8 ;move u to the right place
        vmov     H(8++,16)+r0, H(32++,16) REP 8 ;move v to below the u
        vmov     H(0++,32)+r0, H(40++,0) REP 8 ;move u to the right place
        vmov     H(8++,32)+r0, H(40++,16) REP 8 ;move v to below the u

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
        
        lsr      r14, r4, 16+4
        sub      r14, r16
        and      r14, 3
        add      r16, r14

        add      Rsoffset, r14
        btest    Rsoffset, 2    ;gone over a strip
        sub.ne   r14,1
        add.ne   Rsrc, Rspitch
        bmask    Rsoffset, 2
        addscale Rsrc, r14 << 5

        vmov H(48++,16), H(56++,0) REP 8 ;move v alongside u
        VST_REP_SAFE <HX(48++,0)>, <(Rdest+=Rnum128) REP 8>
        add      Rdoffset, 1
        bmask    Rdoffset, 2
        cmp      Rdoffset, 0
        add.ne   Rdest, 32
        add.eq   Rdest, Rdpitch
        
        bmask    r4, 16+6
        bmask    r5, 16+6
        addcmpbne Rnblocks, -1, 0, .oloop
        
.end:
        .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rnblocks, Rnum128, Rdoffset, Rsoffset

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_h_expand_y_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_expand_y_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
;;;                                   unsigned int dest_width, unsigned int step, unsigned int source_offset, unsigned int source_width)
;;;
;;; FUNCTION
;;;   horizontally expand 16 high stripe of yuvuv y data.  Can be used in place if the source
;;;   data is at the right hand end of the stripe (x offset rounded to multiple of 32).
;;;   source must be the top right of the yuvuv image, with the first used pixel being left
;;;   by source_offset pixels.

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rswidth, r12
.define Rnum128, r13
        
.define Rnblocks, r14
.define RnumM16, r15
.define Rpos1s, r16
.define Rsoffset, r17
.define Rdoffset, r18        
        
.function yuvuv128_h_expand_y_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}
        
        mov      r7, r5
        mov      Rdest, r0
        mov      Rdpitch, r1
        mov      Rspitch, r3
        add      Rnblocks, r4, 15
        mov      Rnum128, 128
        lsr      Rnblocks, 4
        
        ld       Rswidth, (sp+60)
        mov      RnumM16, -16

        ld       Rsoffset, (sp+56)
        asr      r4, Rsoffset, 7 ;number of complete strips
        mul      r4, Rspitch
        add      Rsrc, r2, r4
        bmask    Rsoffset, 7     ;offset within strip
        add      Rsrc, Rsoffset  ;actual src position
        asr      Rsoffset, 4     ;offset into strip in 16wide blocks
        sub      Rspitch, 112    ;offset between last 16wide column and next strip

        mov      Rdoffset, 0     ;offset into dest strip in 16width blocks
        sub      Rdpitch, 112    ;offset between last 16wide column and next strip
        
        lsr      r4, r7, 1
        sub      r4, 0x8000
        bmask    r4, 16+6
        add      r5, r4, r7
        shl      r7, 1
        
        mov      Rpos1s, r4
        vld      H(0++,0), (Rsrc+=Rnum128) REP 16
        add      Rsoffset, 1
        bmask    Rsoffset, 3
        cmp      Rsoffset, 0
        add.ne   Rsrc, 16
        add.eq   Rsrc, Rspitch
        
        vmov     V(0,63), V(0,0)
        addcmpbhs Rswidth, 0, 16, .oloop
        vmov     V(0,0)+Rswidth, V(0,63)+Rswidth
        
         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc

.oloop:
        lsr      r0, r4, 16
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
        eor      Rpos1s, r4
        btest    Rpos1s, 16+4
        mov      Rpos1s, r4
        beq      .noload
        
        lsr      r0, r4, 16
        bic      r0, 15
        vld      H(0++,16)+r0, (Rsrc+=Rnum128) REP 16
        add      Rsoffset, 1
        bmask    Rsoffset, 3
        cmp      Rsoffset, 0
        add.ne   Rsrc, 16
        add.eq   Rsrc, Rspitch
        
        addcmpbhs Rswidth, RnumM16, 16, .noload
        add      Rswidth, r0
        vmov     V(0,16)+Rswidth, V(0,15)+Rswidth
        sub      Rswidth, r0
        
.noload:
        VST_REP_SAFE <H(48++,0)>, <(Rdest+=Rnum128) REP 16>
        add      Rdoffset, 1
        bmask    Rdoffset, 3
        cmp      Rdoffset, 0
        add.ne   Rdest, 16
        add.eq   Rdest, Rdpitch
        
        addcmpbne Rnblocks, -1, 0, .oloop

.end:
        .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rswidth, Rnum128, Rnblocks, RnumM16, Rpos1s, Rsoffset, Rdoffset
        
;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_h_expand_uv_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_h_expand_uv_stripe(unsigned char *dest, unsigned int dest_pitch, unsigned char *src, unsigned int src_pitch,
;;;                                    unsigned int dest_width, unsigned int step, unsigned int source_offset, unsigned int source_width)
;;;
;;; FUNCTION
;;;   horizontally expand 8 high stripe of yuvuv uv data.  Can be used in place if the source
;;;   data is at the right hand end of the stripe (x offset rounded to multiple of 32).
;;;   source must be the top right of the yuvuv image, with the first used pixel being left
;;;   by source_offset pixels.

.define Rdest, r8
.define Rdpitch, r9
.define Rsrc, r10
.define Rspitch, r11
.define Rswidth, r12
.define Rnum128, r13
        
.define Rnblocks, r14
.define RnumM16, r15
.define Rpos1s, r16
.define Rsoffset, r17
.define Rdoffset, r18        
        
.function yuvuv128_h_expand_uv_stripe ; {{{
        stm      r6-r18, lr, (--sp)
       .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13,%r14,%r15,%r16,%r17,%r18}
        
        mov      r7, r5
        mov      Rdest, r0
        mov      Rdpitch, r1
        mov      Rspitch, r3
        add      Rnblocks, r4, 15
        mov      Rnum128, 128
        lsr      Rnblocks, 4
        
        ld       Rswidth, (sp+60)
        mov      RnumM16, -16

        ld       Rsoffset, (sp+56)
        asr      r4, Rsoffset, 6 ;number of complete strips
        mul      r4, Rspitch
        add      Rsrc, r2, r4
        bmask    Rsoffset, 6
        shl      Rsoffset, 1     ;offset within strip
        add      Rsrc, Rsoffset  ;actual src position
        asr      Rsoffset, 5     ;offset into strip in 32 byte wide blocks
        sub      Rspitch, 96     ;offset between last 32 byte wide column and next strip

        mov      Rdoffset, 0     ;offset into dest strip in 32 byte wide blocks
        sub      Rdpitch, 96     ;offset between last 32 byte wide column and next strip
        
        lsr      r4, r7, 1
        sub      r4, 0x8000
        bmask    r4, 16+6
        add      r5, r4, r7
        shl      r7, 1
        
        mov      Rpos1s, r4
        vld      HX(0++,0), (Rsrc+=Rnum128) REP 8
        vmov     H(8++,0), H(0++,16) REP 8 ;move the V data below the U data
        add      Rsoffset, 1
        bmask    Rsoffset, 2
        cmp      Rsoffset, 0
        add.ne   Rsrc, 32
        add.eq   Rsrc, Rspitch
        
        vmov     V(0,63), V(0,0)
        addcmpbhs Rswidth, 0, 16, .oloop
        vmov     V(0,0)+Rswidth, V(0,63)+Rswidth

         ; r4 = pos1
         ; r5 = pos2
         ; r7 = inc
.oloop:
        lsr      r0, r4, 16
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
        eor      Rpos1s, r4
        btest    Rpos1s, 16+4
        mov      Rpos1s, r4
        beq      .noload
        
        lsr      r0, r4, 16
        bic      r0, 15
        vld      HX(32++,0), (Rsrc+=Rnum128) REP 16
        vmov     H(0++,16)+r0, H(32++,0) REP 8 ; move the U data to the right place
        vmov     H(8++,16)+r0, H(32++,16) REP 8 ; move the V data to the right place
        
        add      Rsoffset, 1
        bmask    Rsoffset, 2
        cmp      Rsoffset, 0
        add.ne   Rsrc, 32
        add.eq   Rsrc, Rspitch
        
        addcmpbhs Rswidth, RnumM16, 16, .noload
        add      Rswidth, r0
        vmov     V(0,16)+Rswidth, V(0,15)+Rswidth
        sub      Rswidth, r0
        
.noload:
        vmov     H(48++,16), H(56++,0) REP 8 ; move the V data alongside the U
        VST_REP_SAFE <HX(48++,0)>, <(Rdest+=Rnum128) REP 8>
        add      Rdoffset, 1
        bmask    Rdoffset, 2
        cmp      Rdoffset, 0
        add.ne   Rdest, 32
        add.eq   Rdest, Rdpitch
        
        addcmpbne Rnblocks, -1, 0, .oloop

.end:
        .cfa_pop {%r18,%r17,%r16,%r15,%r14,%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm      r6-r18, pc, (sp++)
.endfn ; }}}

.undef Rdest, Rdpitch, Rsrc, Rspitch, Rswidth, Rnum128, Rnblocks, RnumM16, Rpos1s, Rsoffset, Rdoffset


;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_move_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_move_stripe(unsigned char *y, unsigned char *uv, unsigned int nstrips, unsigned int pitch, unsigned int offset)
;;;
;;; FUNCTION
;;;   move the data in a stripe right by offset pixels.  offset is a multiple of 32
;;;   and less than nstrips*128.

.define Ry, r0
.define Ruv, r1
.define Rnstrips, r2
.define Rpitch, r3
.define Roffset, r4        
.define Rstop, r4
        
.define Rnum128, r2
.define Rtmp, r2        
.define Rdec0, r5
.define Rdec1, r6
.define Rdec2, r7
.define Rdec3, r8        

.define Rwy, r9                 ;y write pointer
.define Rwuv, r10               ;uv write pointer
        
.function yuvuv128_move_stripe  ; {{{
        stm r6-r10, lr, (--sp)   
        .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10}

        addcmpbeq Roffset, 0, 0, move_stripe_done
        
        sub Rnstrips, 1
        mul Rwy, Rpitch, Rnstrips
        add Rwy, Rwy, 96              ;write pointer offset
        add Rwuv, Rwy, Ruv
        add Rwy, Ry

        asr Rdec0, Roffset, 7   ;whole numbers of strips
        mul Rdec0, Rpitch
        bmask Roffset, 7
        add Rdec0, Roffset      ;initial wptr-rwptr

        asr Rtmp, Roffset, 5
        
        mov Rstop, Ry           ;stop when yread<Rstop
        sub Ry, Rwy, Rdec0      ;y read pointer
        sub Ruv, Rwuv, Rdec0    ;uv read pointer

        mov Rdec0, -32
        mov Rdec1, -32
        mov Rdec2, -32
        mov Rdec3, -32
        rsub Rpitch, 96
        
        cmp Rtmp, 0
        mov.eq Rdec0, Rpitch
        cmp Rtmp, 1
        mov.eq Rdec1, Rpitch
        cmp Rtmp, 2
        mov.eq Rdec2, Rpitch
        cmp Rtmp, 3
        mov.eq Rdec3, Rpitch

        mov Rnum128, 128

move_stripe_loop:
        ;; write into the 4th column of the strip
        vldh H16(0++,0), (Ry+=Rnum128) REP 16
        vldh H16(16++,0), (Ruv+=Rnum128) REP 8
        VST_REP_SAFE <H16(0++,0)>, <(Rwy+=Rnum128) REP 16>
        VST_REP_SAFE <H16(16++,0)>, <(Rwuv+=Rnum128) REP 8>

        addcmpblt Ry, Rdec3, Rstop, move_stripe_done
        add Ruv, Rdec3
        sub Rwy, 32
        sub Rwuv, 32
        
        ;; write into the 3rd column of the strip
        vldh H16(0++,0), (Ry+=Rnum128) REP 16
        vldh H16(16++,0), (Ruv+=Rnum128) REP 8
        VST_REP_SAFE <H16(0++,0)>, <(Rwy+=Rnum128) REP 16>
        VST_REP_SAFE <H16(16++,0)>, <(Rwuv+=Rnum128) REP 8>

        addcmpblt Ry, Rdec2, Rstop, move_stripe_done
        add Ruv, Rdec2
        sub Rwy, 32
        sub Rwuv, 32

        ;; write into the 2nd column of the strip
        vldh H16(0++,0), (Ry+=Rnum128) REP 16
        vldh H16(16++,0), (Ruv+=Rnum128) REP 8
        VST_REP_SAFE <H16(0++,0)>, <(Rwy+=Rnum128) REP 16>
        VST_REP_SAFE <H16(16++,0)>, <(Rwuv+=Rnum128) REP 8>

        addcmpblt Ry, Rdec1, Rstop, move_stripe_done
        add Ruv, Rdec1
        sub Rwy, 32
        sub Rwuv, 32

        ;; write into the 1st column of the strip
        vldh H16(0++,0), (Ry+=Rnum128) REP 16
        vldh H16(16++,0), (Ruv+=Rnum128) REP 8
        VST_REP_SAFE <H16(0++,0)>, <(Rwy+=Rnum128) REP 16>
        VST_REP_SAFE <H16(16++,0)>, <(Rwuv+=Rnum128) REP 8>

        addcmpblt Ry, Rdec0, Rstop, move_stripe_done
        add Ruv, Rdec0
        add Rwy, Rpitch
        add Rwuv, Rpitch

        b move_stripe_loop
        
move_stripe_done:
        .cfa_pop {%r10,%r9,%r8,%r7,%r6,%pc}
        ldm r6-r10, pc, (sp++)
.endfn
        
.undef Ry, Ruv, Rnstrips, Rpitch, Roffset, Rstop, Rnum128, Rtmp, Rdec0, Rdec1, Rdec2, Rdec3, Rwy, Rwuv

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_move_stripe_left
;;;
;;; SYNOPSIS
;;;   void yuvuv128_move_stripe_left(unsigned char *y, unsigned char *uv, unsigned int nstrips, unsigned int pitch, unsigned int offset)
;;;
;;; FUNCTION
;;;   move the data in a stripe left by offset pixels.  offset is a multiple of 2
;;;

.define Ry, r0
.define Ruv, r1
.define Rnstrips, r2
.define Rpitch, r3
.define Roffset, r4        
        
.define Rinc0, r5
.define Rinc1, r6
.define Rinc2, r7
.define Rinc3, r8        

.define Rwy, r9                 ;y write pointer
.define Rwuv, r10               ;uv write pointer
        
.define Rnum128, r11
.define Rtmp, r11        

.function yuvuv128_move_stripe_left  ; {{{
        stm r6-r11, lr, (--sp)   
        .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}

        mov Rwy, Ry
        mov Rwuv, Ruv
        
        asr Rtmp, Roffset, 7      ; whole number of strips in offset
        bmask Roffset, 7          ; now offset into strip
        sub Rnstrips, Rtmp
        mul Rtmp, Rpitch
        add Ry, Rtmp              ; move over whole number of strips
        add Ruv, Rtmp

        asr Rtmp, Roffset, 5      ; which column of 32bytes
        bmask Roffset, 5          ; now offset into 32byte block
        asr Roffset, 1            ; now in 0..15, vrf offset (0..15,0)
        
        mov Rinc0, 32
        mov Rinc1, 32
        mov Rinc2, 32
        mov Rinc3, 32
        sub Rpitch, 96
        
        cmp Rtmp, 0
        mov.eq Rinc3, Rpitch
        cmp Rtmp, 1
        mov.eq Rinc2, Rpitch
        cmp Rtmp, 2
        mov.eq Rinc1, Rpitch
        cmp Rtmp, 3
        mov.eq Rinc0, Rpitch

        shl Rtmp, 5             ; byte offset into strip
        add Ry, Rtmp
        add Ruv, Rtmp

        mov Rnum128, 128

        ;; load the first column
        vld H16(0++,0), (Ry+=Rnum128) REP 16
        vld H16(16++,0), (Ruv+=Rnum128) REP 8
        cbclr

move_strip_left_loop:

        ;; first column
        add Ry, Rinc0
        add Ruv, Rinc0
        vld H16(0++,32)*, (Ry+=Rnum128) REP 16
        vld H16(16++,32)*, (Ruv+=Rnum128) REP 8

        vmov H16(32++,0), H16(0++,0)+Roffset* REP 16
        vmov H16(48++,0), H16(16++,0)+Roffset* REP 16
        VST_REP_SAFE <H16(32++,0)>, <Rwy+=Rnum128 REP 16>
        VST_REP_SAFE <H16(48++,0)>, <Rwuv+=Rnum128 REP 8>
        add Rwy, 32
        add Rwuv, 32
        cbadd2

        ;; second column
        add Ry, Rinc1
        add Ruv, Rinc1
        vld H16(0++,32)*, (Ry+=Rnum128) REP 16
        vld H16(16++,32)*, (Ruv+=Rnum128) REP 8

        vmov H16(32++,0), H16(0++,0)+Roffset* REP 16
        vmov H16(48++,0), H16(16++,0)+Roffset* REP 16
        VST_REP_SAFE <H16(32++,0)>, <Rwy+=Rnum128 REP 16>
        VST_REP_SAFE <H16(48++,0)>, <Rwuv+=Rnum128 REP 8>
        add Rwy, 32
        add Rwuv, 32
        cbadd2
        
        ;; third column
        add Ry, Rinc2
        add Ruv, Rinc2
        vld H16(0++,32)*, (Ry+=Rnum128) REP 16
        vld H16(16++,32)*, (Ruv+=Rnum128) REP 8

        vmov H16(32++,0), H16(0++,0)+Roffset* REP 16
        vmov H16(48++,0), H16(16++,0)+Roffset* REP 16
        VST_REP_SAFE <H16(32++,0)>, <Rwy+=Rnum128 REP 16>
        VST_REP_SAFE <H16(48++,0)>, <Rwuv+=Rnum128 REP 8>
        add Rwy, 32
        add Rwuv, 32
        cbadd2
        
        ;; fourth column
        add Ry, Rinc3
        add Ruv, Rinc3
        vld H16(0++,32)*, (Ry+=Rnum128) REP 16
        vld H16(16++,32)*, (Ruv+=Rnum128) REP 8

        vmov H16(32++,0), H16(0++,0)+Roffset* REP 16
        vmov H16(48++,0), H16(16++,0)+Roffset* REP 16
        VST_REP_SAFE <H16(32++,0)>, <Rwy+=Rnum128 REP 16>
        VST_REP_SAFE <H16(48++,0)>, <Rwuv+=Rnum128 REP 8>
        add Rwy, Rpitch
        add Rwuv, Rpitch
        cbadd2

        addcmpbgt Rnstrips, -1, 0, move_strip_left_loop

        .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        ldm r6-r11, pc, (sp++)
.endfn
        
.undef Ry, Ruv, Rnstrips, Rpitch, Roffset, Rnum128, Rtmp, Rinc0, Rinc1, Rinc2, Rinc3, Rwy, Rwuv
       

        
        
        
;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_vflip_strip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_vflip_strip(unsigned char *data, unsigned int height);
;;;
;;; FUNCTION
;;;   vertically flip a strip of data of a given height and a column pitch of 128.

.function yuvuv128_vflip_strip ; {{{
        mov r3, r0
        mov r4, -128
        mov r5, 128
        addscale r2, r3, r1 << 7
        ;; r0 number of rows in top and bottom block to read
        ;; r1 (block height-1)*128
        ;; r2 bottom pointer
        ;; r3 top pointer
        ;; r4 -128, for writing upwards in memory
        ;; r5 128, for reading downwards in memory
vflip_loop:
        ;; r3 points the start of the first row to be read in the top block
        ;; r2 points to the line beyond the last row to be read in the bottom block
        sub r0, r2, r3          ;work out how many rows we can read/write
        add r0, 128
        asr r0, 8
        min r0, 8               ;maximum of 8 rows in one iteration
        shl r1, r0, 7
        sub r1, 128
        subscale r2, r0 << 7    ;move bottom pointer up to top of bottom block
        
        vldw H32(0++,0), r3+=r5 REP r0
        vldw H32(8++,0), r3+64+=r5 REP r0
        vldw H32(16++,0), r2+=r5 REP r0
        vldw H32(24++,0), r2+64+=r5 REP r0

        add r3, r1             ;move to bottom row in block
        add r2, r1
        
        VST_REP_SAFE <H32(0++,0)>, <r2+=r4 REP r0>
        VST_REP_SAFE <H32(8++,0)>, <r2+64+=r4 REP r0>
        VST_REP_SAFE <H32(16++,0)>, <r3+=r4 REP r0>
        VST_REP_SAFE <H32(24++,0)>, <r3+64+=r4 REP r0>

        sub r2, r1
        addcmpblt r3, r5, r2, vflip_loop
        b lr
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_hflip_y_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_hflip_y_stripe(unsigned char *data, unsigned int nstrips, unsigned int pitch, unsigned int width);
;;;
;;; FUNCTION
;;;   horizontally flip in place a 16 high stripe of yuvuv y data with column pitch of 128

.define Rleft, r0               ;address of left (64 wide) strip
.define Rright, r1              ;address of right (64 wide) strip
.define Rpitch, r2              ;pitch between strips, minus 64
.define Rnum128, r3             ;fixed 128, column width
.define Rrnext, r4              ;next change to Rright
.define Rnum64, r5              ;fixed 64, half column width
.define Rlnext, r6              ;next change to Rleft
.define Rnum, r7                ;actual width of right most block
.define Rloop, r8               ;loop variable
.define Rloop2, r9              ;loop variable
.define Rnum32, r10             ;fixed 32, half column width in uv

.function yuvuv128_hflip_y_stripe ; {{{
        stm r6-r9, lr, (--sp)

        sub r3, 1
        mov Rnum64, 64
        btest r3, 6               ;which half of the right most strip we start on
        bmask Rnum, r3, 6         ;width of right most strip
        add Rnum, 1
        sub Rright, 1             ;nstrips-1
        mov Rnum128, 128
        mul Rright, Rpitch      ;pitch*(nstrips-1)
        add Rright, r0          ;addr of right most strip
        sub Rpitch, 64          ;now pitch-64, offset between mid strip and next strip

        ;; setup right to correct half of strip
        add.ne Rright, Rnum64
        mov.ne Rrnext, Rnum64
        mov.eq Rrnext, Rpitch
        
        mov Rlnext, 64

        ;; read right block
        vld H(16++,0), Rright+=Rnum128 REP 16
        vld H(16++,16), Rright+16+=Rnum128 REP 16
        vld H(16++,32), Rright+32+=Rnum128 REP 16
        vld H(16++,48), Rright+48+=Rnum128 REP 16
        
        addcmpbeq Rleft, 0, Rright, hflip_y_middle_strip_right

hflip_y_loop:
        ;; read left block
        vld H(0++,0), Rleft+=Rnum128 REP 16
        vld H(0++,16), Rleft+16+=Rnum128 REP 16
        vld H(0++,32), Rleft+32+=Rnum128 REP 16
        vld H(0++,48), Rleft+48+=Rnum128 REP 16

        ;; copy Rnum lines from the start of each input block
        ;; to the start of each output block
        mov Rloop, 0
        sub Rloop2, Rnum, 1
1:
        vmov V8(48,0)+Rloop2, V8(0,0)+Rloop
        vmov V8(32,0)+Rloop2, V8(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum, 1b

        ;; write out the right output block
        VST_REP_SAFE <H(48++,0)>, <Rright+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,16)>, <Rright+16+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,32)>, <Rright+32+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,48)>, <Rright+48+=Rnum128 REP 16>

        ;; move to the next right block - if the same
        ;; as the left block, go to the middle_left code
        sub Rright, Rrnext
        addcmpbeq Rright, 0, Rleft, hflip_y_middle_strip_left

        ;; read right block
        vld H(16++,0), Rright+=Rnum128 REP 16
        vld H(16++,16), Rright+16+=Rnum128 REP 16
        vld H(16++,32), Rright+32+=Rnum128 REP 16
        vld H(16++,48), Rright+48+=Rnum128 REP 16
        
        ;; swap Rrnext between Rnum64 and Rpitch
        cmp Rrnext, Rnum64
        mov.eq Rrnext, Rpitch
        mov.ne Rrnext, Rnum64

        ;; copy 64-Rnum lines from the end of each input block
        ;; to the end of each output block.  May involve no work.
        addcmpbeq Rnum, 0, Rnum64, 2f
        mov Rloop, Rnum
        sub Rloop2, Rnum64, 1
1:
        vmov V8(48,0)+Rloop2, V8(0,0)+Rloop
        vmov V8(32,0)+Rloop2, V8(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum64, 1b
2:
        
        ;; write out the left output block
        VST_REP_SAFE <H(32++,0)>, <Rleft+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,16)>, <Rleft+16+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,32)>, <Rleft+32+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,48)>, <Rleft+48+=Rnum128 REP 16>

        ;; go to next left block, if same as right block
        ;; go the middle_right code
        addcmpbeq Rleft, Rlnext, Rright, hflip_y_middle_strip_right

        ;; swap Rlnext between Rnum64 and Rpitch
        cmp Rlnext, Rnum64
        mov.eq Rlnext, Rpitch
        mov.ne Rlnext, Rnum64

        b hflip_y_loop
        
hflip_y_middle_strip_left:
        ;; swap over the last 64-Rnum lines in the left block
        ;; to the left output block.  May involve no work.
        addcmpbeq Rnum, 0, Rnum64, 2f
        mov Rloop, Rnum
        sub Rloop2, Rnum64, 1
1:
        vmov V8(32,0)+Rloop2, V8(0,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum64, 1b
2:
        
        ;; write out the left output block
        VST_REP_SAFE <H(32++,0)>, <Rleft+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,16)>, <Rleft+16+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,32)>, <Rleft+32+=Rnum128 REP 16>
        VST_REP_SAFE <H(32++,48)>, <Rleft+48+=Rnum128 REP 16>
        
        b hflip_y_done
hflip_y_middle_strip_right:     
        ;; swap over the first Rnum lines in the right block
        ;; to the right output block
        mov Rloop, 0
        sub Rloop2, Rnum, 1
1:
        vmov V8(48,0)+Rloop2, V8(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum, 1b

        ;; write out the right output block
        VST_REP_SAFE <H(48++,0)>, <Rright+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,16)>, <Rright+16+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,32)>, <Rright+32+=Rnum128 REP 16>
        VST_REP_SAFE <H(48++,48)>, <Rright+48+=Rnum128 REP 16>
        
hflip_y_done:     
        ldm r6-r9, pc, (sp++)
.endfn ; }}}
        
;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_hflip_uv_stripe
;;;
;;; SYNOPSIS
;;;   void yuvuv128_hflip_uv_stripe(unsigned char *data, unsigned int nstrips, unsigned int pitch, unsigned int width);
;;;
;;; FUNCTION
;;;   horizontally flip in place an 8 high stripe of yuvuv uv data with column pitch of 128
;;;   width must be even

.function yuvuv128_hflip_uv_stripe ; {{{
        stm r6-r10, lr, (--sp)

        sub r3, 1
        mov Rnum64, 64
        btest r3, 6               ;which half of the right most strip we start on
        bmask Rnum, r3, 6         ;width of right most y strip
        asr Rnum, 1               ;width of right most uv strip
        mov Rnum32, 32
        add Rnum, 1               ;now in 1..32
        sub Rright, 1             ;nstrips-1
        mov Rnum128, 128
        mul Rright, Rpitch        ;pitch*(nstrips-1)
        add Rright, r0            ;addr of right most strip
        sub Rpitch, 64            ;now pitch-64, offset between mid strip and next strip

        ;; setup right to correct half of strip
        add.ne Rright, Rnum64
        mov.ne Rrnext, Rnum64
        mov.eq Rrnext, Rpitch
        
        mov Rlnext, 64

        ;; read right block
        vldh H16(16++,0), Rright+=Rnum128 REP 8
        vldh H16(16++,32), Rright+32+=Rnum128 REP 8
        
        addcmpbeq Rleft, 0, Rright, hflip_uv_middle_strip_right

hflip_uv_loop:
        ;; read left block
        vldh H16(0++,0), Rleft+=Rnum128 REP 8
        vldh H16(0++,32), Rleft+32+=Rnum128 REP 8

        ;; copy Rnum lines from the start of each input block
        ;; to the start of each output block
        mov Rloop, 0
        sub Rloop2, Rnum, 1
1:
        vmov V16(48,0)+Rloop2, V16(0,0)+Rloop
        vmov V16(32,0)+Rloop2, V16(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum, 1b

        ;; write out the right output block
        VST_REP_SAFE <H16(48++,0)>, <Rright+=Rnum128 REP 8>
        VST_REP_SAFE <H16(48++,32)>, <Rright+32+=Rnum128 REP 8>

        ;; move to the next right block - if the same
        ;; as the left block, go to the middle_left code
        sub Rright, Rrnext
        addcmpbeq Rright, 0, Rleft, hflip_uv_middle_strip_left

        ;; read right block
        vldh H16(16++,0), Rright+=Rnum128 REP 8
        vldh H16(16++,32), Rright+32+=Rnum128 REP 8
        
        ;; swap Rrnext between Rnum64 and Rpitch
        cmp Rrnext, Rnum64
        mov.eq Rrnext, Rpitch
        mov.ne Rrnext, Rnum64

        ;; copy 32-Rnum lines from the end of each input block
        ;; to the end of each output block.  May involve no work.
        addcmpbeq Rnum, 0, Rnum32, 2f
        mov Rloop, Rnum
        sub Rloop2, Rnum32, 1
1:
        vmov V16(48,0)+Rloop2, V16(0,0)+Rloop
        vmov V16(32,0)+Rloop2, V16(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum32, 1b
2:
        
        ;; write out the left output block
        VST_REP_SAFE <H16(32++,0)>, <Rleft+=Rnum128 REP 8>
        VST_REP_SAFE <H16(32++,32)>, <Rleft+32+=Rnum128 REP 8>

        ;; go to next left block, if same as right block
        ;; go the middle_right code
        addcmpbeq Rleft, Rlnext, Rright, hflip_uv_middle_strip_right

        ;; swap Rlnext between Rnum64 and Rpitch
        cmp Rlnext, Rnum64
        mov.eq Rlnext, Rpitch
        mov.ne Rlnext, Rnum64

        b hflip_uv_loop
        
hflip_uv_middle_strip_left:
        ;; swap over the last 32-Rnum lines in the left block
        ;; to the left output block.  May involve no work.
        addcmpbeq Rnum, 0, Rnum32, 2f
        mov Rloop, Rnum
        sub Rloop2, Rnum32, 1
1:
        vmov V16(32,0)+Rloop2, V16(0,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum32, 1b
2:
        
        ;; write out the left output block
        VST_REP_SAFE <H16(32++,0)>, <Rleft+=Rnum128 REP 8>
        VST_REP_SAFE <H16(32++,32)>, <Rleft+32+=Rnum128 REP 8>
        
        b hflip_uv_done
hflip_uv_middle_strip_right:     
        ;; swap over the first Rnum lines in the right block
        ;; to the right output block
        mov Rloop, 0
        sub Rloop2, Rnum, 1
1:
        vmov V16(48,0)+Rloop2, V16(16,0)+Rloop
        sub Rloop2, 1
        addcmpblt Rloop, 1, Rnum, 1b

        ;; write out the right output block
        VST_REP_SAFE <H16(48++,0)>, <Rright+=Rnum128 REP 8>
        VST_REP_SAFE <H16(48++,32)>, <Rright+32+=Rnum128 REP 8>
        
hflip_uv_done:     
        ldm r6-r10, pc, (sp++)
.endfn ; }}}

.undef Rleft, Rright, Rpitch, Rnum128, Rrnext, Rnum64, Rlnext, Rnum, Rloop, Rloop2, Rnum32

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_transpose_y_block
;;;
;;; SYNOPSIS
;;;   void yuvuv128_transpose_y_block(unsigned char *dest, unsigned char *src, unsigned int src_width)
;;;
;;; FUNCTION
;;;   transpose from a 128x128 piece of memory to a different 128xheight piece of memory where
;;;   height is (src_width+15)&~15

.define Rdest, r0
.define Rsrc, r1        
.define Rsrcend, r2
.define Rnum128, r3
.define Rnum16, r4
        
.function yuvuv128_transpose_y_block ; {{{

        mov Rnum128, 128
        add Rsrcend, Rsrc
        mov Rnum16, 16

transpose_y_loop:       
        vldb H8(0++,0), Rsrc+=Rnum128 REP 64
        vldb H8(0++,16), Rsrc+8192+=Rnum128 REP 64

        VST_REP_SAFE <V8(0,0++)>, <Rdest+=Rnum128 REP 16>
        VST_REP_SAFE <V8(16,0++)>, <Rdest+16+=Rnum128 REP 16> 
        VST_REP_SAFE <V8(32,0++)>, <Rdest+32+=Rnum128 REP 16> 
        VST_REP_SAFE <V8(48,0++)>, <Rdest+48+=Rnum128 REP 16> 
        VST_REP_SAFE <V8(0,16++)>, <Rdest+64+=Rnum128 REP 16>
        VST_REP_SAFE <V8(16,16++)>, <Rdest+80+=Rnum128 REP 16> 
        VST_REP_SAFE <V8(32,16++)>, <Rdest+96+=Rnum128 REP 16> 
        VST_REP_SAFE <V8(48,16++)>, <Rdest+112+=Rnum128 REP 16> 

        add Rdest, 2048         ;16 rows of 128
        addcmpblt Rsrc, Rnum16, Rsrcend, transpose_y_loop

        b lr
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_transpose_uv_block
;;;
;;; SYNOPSIS
;;;   void yuvuv128_transpose_uv_block(unsigned char *dest, unsigned char *src, unsigned int src_width)
;;;
;;; FUNCTION
;;;   transpose from a 128x64 piece of memory to a different 128xheight piece of memory where
;;;   height is ((src_width+15)&~15)>>1.  Memory consists of 64x64 uv pairs.
;;;   Only guaranteed to write within uv height rounded up to 8, so stop and check
;;;   after writing out the first 8 high stripe whether we've finished.

.function yuvuv128_transpose_uv_block ; {{{

        mov Rnum128, 128
        add Rsrcend, Rsrc
        mov Rnum16, 16

transpose_uv_loop:
        vldh H16(0++,0), Rsrc+=Rnum128 REP 64

        VST_REP_SAFE <V16(0,0++)>, <Rdest+=Rnum128 REP 8>
        VST_REP_SAFE <V16(16,0++)>, <Rdest+32+=Rnum128 REP 8>
        VST_REP_SAFE <V16(32,0++)>, <Rdest+64+=Rnum128 REP 8>
        VST_REP_SAFE <V16(48,0++)>, <Rdest+96+=Rnum128 REP 8>

        add Rdest, 1024         ;8 rows of 128
        addcmpbge Rsrc, Rnum16, Rsrcend, transpose_uv_end

        VST_REP_SAFE <V16(0,8++)>, <Rdest+=Rnum128 REP 8>
        VST_REP_SAFE <V16(16,8++)>, <Rdest+32+=Rnum128 REP 8>
        VST_REP_SAFE <V16(32,8++)>, <Rdest+64+=Rnum128 REP 8>
        VST_REP_SAFE <V16(48,8++)>, <Rdest+96+=Rnum128 REP 8>

        add Rdest, 1024         ;8 rows of 128
        addcmpblt Rsrc, Rnum16, Rsrcend, transpose_uv_loop

transpose_uv_end:       
        b lr
.endfn ; }}}
        
.undef Rdest, Rsrc, Rsrcend, Rnum128, Rnum16
 
;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_transpose_block_y
;;;
;;; SYNOPSIS
;;;   void yuvuv_transpose_block_y(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  dst_height, /* r2 */
;;;      int                  src_height);/* r3 */
;;;
;;; FUNCTION
;;;   Another implementation of yuvuv128_transpose_y_block; this one operates in
;;;   an order that attempts to be sensitive to the SDRAM page thrashing costs.

.function yuvuv128_transpose_block_y ; {{{
         mov      r4, r0
         lsr      r2, 2
         mov      r0, 64
         nop
.loop:   mov      r5, 128
         min      r0, r3, r0

         vld      H32(0++,0), (r1+=r5)                REP r0

         mov      r5, 128 * 4
         min      r0, r2, 16

         vst      V8(0,0++),   (r4+(0)+=r5)           REP r0
         vst      V8(16,0++),  (r4+(16)+=r5)          REP r0
         vst      V8(32,0++),  (r4+(32)+=r5)          REP r0
         vst      V8(48,0++),  (r4+(48)+=r5)          REP r0
         vst      V8(0,16++),  (r4+(128)+=r5)         REP r0
         vst      V8(16,16++), (r4+(128+16)+=r5)      REP r0
         vst      V8(32,16++), (r4+(128+32)+=r5)      REP r0
         vst      V8(48,16++), (r4+(128+48)+=r5)      REP r0
         vst      V8(0,32++),  (r4+(256)+=r5)         REP r0
         vst      V8(16,32++), (r4+(256+16)+=r5)      REP r0
         vst      V8(32,32++), (r4+(256+32)+=r5)      REP r0
         vst      V8(48,32++), (r4+(256+48)+=r5)      REP r0
         vst      V8(0,48++),  (r4+(384)+=r5)         REP r0
         vst      V8(16,48++), (r4+(384+16)+=r5)      REP r0
         vst      V8(32,48++), (r4+(384+32)+=r5)      REP r0
         vst      V8(48,48++), (r4+(384+48)+=r5)      REP r0

         mov      r0, -64
         mov      r5, 128
         addcmpble r0, r3, 0, .skip

         vld      H32(0++,0), (r1+8192+=r5)           REP r0

         mov      r5, 128 * 4
         min      r0, r2, 16

         vst      V8(0,0++),   (r4+(64)+=r5)          REP r0
         vst      V8(16,0++),  (r4+(80)+=r5)          REP r0
         vst      V8(32,0++),  (r4+(96)+=r5)          REP r0
         vst      V8(48,0++),  (r4+(112)+=r5)         REP r0
         vst      V8(0,16++),  (r4+(128+64)+=r5)      REP r0
         vst      V8(16,16++), (r4+(128+80)+=r5)      REP r0
         vst      V8(32,16++), (r4+(128+96)+=r5)      REP r0
         vst      V8(48,16++), (r4+(128+112)+=r5)     REP r0
         vst      V8(0,32++),  (r4+(256+64)+=r5)      REP r0
         vst      V8(16,32++), (r4+(256+80)+=r5)      REP r0
         vst      V8(32,32++), (r4+(256+96)+=r5)      REP r0
         vst      V8(48,32++), (r4+(256+112)+=r5)     REP r0
         vst      V8(0,48++),  (r4+(384+64)+=r5)      REP r0
         vst      V8(16,48++), (r4+(384+80)+=r5)      REP r0
         vst      V8(32,48++), (r4+(384+96)+=r5)      REP r0
         vst      V8(48,48++), (r4+(384+112)+=r5)     REP r0
.skip:   sub      r2, 16
         add      r1, 64
         add      r4, 64*128
         mov      r0, 64
         addcmpbgt  r2, 0, 0, .loop

         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_transpose_block_uv
;;;
;;; SYNOPSIS
;;;   void yuvuv128_transpose_block_uv(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  dst_height, /* r2 */
;;;      int                  src_height);/* r3 */
;;;
;;; FUNCTION
;;;   Another implementation of yuvuv128_transpose_uv_block; this one operates
;;;   in an order that attempts to be sensitive to the SDRAM page thrashing
;;;   costs.

.function yuvuv128_transpose_block_uv ; {{{
         mov      r4, r0
         lsr      r2, 1
         mov      r0, 64
         nop
.loop:   mov      r5, 128
         min      r0, r3, r0

         vld      H32(0++,0), (r1+=r5)                REP r0

         mov      r5, 128 * 2
         min      r0, r2, 16

         vst      V16(0,0++),   (r4+(0)+=r5)           REP r0
         vst      V16(16,0++),  (r4+(32)+=r5)          REP r0
         vst      V16(32,0++),  (r4+(64)+=r5)          REP r0
         vst      V16(48,0++),  (r4+(96)+=r5)          REP r0
         vst      V16(0,32++),  (r4+(128)+=r5)         REP r0
         vst      V16(16,32++), (r4+(128+32)+=r5)      REP r0
         vst      V16(32,32++), (r4+(128+64)+=r5)      REP r0
         vst      V16(48,32++), (r4+(128+96)+=r5)      REP r0

         sub      r2, 16
         add      r1, 64
         add      r4, 32*128
         mov      r0, 64
         addcmpbgt  r2, 0, 0, .loop

         b        lr
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_subsample_128xn_y
;;;
;;; SYNOPSIS
;;;   void yuvuv128_subsample_128xn_y(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  src_height);/* r2 */
;;;
;;; FUNCTION
;;;   Another implementation of yuvuv_downsample_4mb which operates in larger
;;;   chunks.

.function yuvuv128_subsample_128xn_y ; {{{
         stm      r6-r7, (--sp)
         .cfa_push {%r6,%r7}
         mov      r4, r0
         mov      r7, 128
         mov      r3, 32

.loop:   min      r0, r2, r3 ; src height
         vld      H32(0++,0), (r1+=r7)                   REP r0
         vld      H32(32++,0), (r1+64+=r7)               REP r0
         addscale r1, r1, r0 * 128

         mov      r3, -64 
         mov      r5, 0
         shl      r6, r0, 6
         lsr      r0, 1
.dloop:  vadd     -,           H8(0,32)+r5, H8(0,48)+r5  CLRA SACCH
         add      r3, 64
         vadd     -,           H8(0,0)+r5,  H8(0,16)+r5       UACC
         vadd     -,           H8(1,32)+r5, H8(1,48)+r5       SACCH
         vadd     H32(0,0)+r3, H8(1,0)+r5,  H8(1,16)+r5       UACC
         vadd     -,           H8(32,32)+r5, H8(32,48)+r5 CLRA SACCH
         vadd     -,           H8(32,0)+r5,  H8(32,16)+r5     UACC
         vadd     -,           H8(33,32)+r5, H8(33,48)+r5     SACCH
         vadd     H32(32,0)+r3, H8(33,0)+r5, H8(33,16)+r5     UACC
         addcmpblt r5, r7, r6, .dloop

         vmulhn.uu H8(0++,0), H16(0++,0), 0x4000         REP r0
         vmulhn.uu H8(0++,16), H16(0++,32), 0x4000       REP r0
         vmulhn.uu H8(32++,0), H16(32++,0), 0x4000       REP r0
         vmulhn.uu H8(32++,16), H16(32++,32), 0x4000     REP r0

         mov      r3, 32
         cmp      r2, 32
         sub      r2, 32
         vst      H16(0++,0), (r4+=r7)                   REP r0
         vst      H16(32++,0), (r4+32+=r7)               REP r0
         addscale r4, r4, r0 * 128
         bgt      .loop

         mov      r0, r4 ; return new dst
         ldm      r6-r7, (sp++)
         .cfa_pop {%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_subsample_64xn_uv
;;;
;;; SYNOPSIS
;;;   void yuvuv128_subsample_64xn_uv(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  src_height);/* r2 */
;;;
;;; FUNCTION
;;;   Another implementation of yuvuv_downsample_4mb which operates in larger
;;;   chunks.


.function yuvuv128_subsample_64xn_uv ; {{{
         stm      r6-r7, (--sp)
         .cfa_push {%r6,%r7}
         mov      r4, r0
         mov      r7, 128
         mov      r3, 32

.loop:   min      r0, r2, r3 ; src height
         vld      H32(0++,0), (r1+=r7)                   REP r0
         vld      H32(32++,0), (r1+64+=r7)               REP r0
         addscale r1, r1, r0 * 128

         mov      r3, -64 
         mov      r5, 0
         shl      r6, r0, 6
         lsr      r0, 1
.dloop:  vadd     -,           H8(0,16)+r5, H8(0,48)+r5  CLRA SACCH
         add      r3, 64
         vadd     -,           H8(0,0)+r5,  H8(0,32)+r5       UACC
         vadd     -,           H8(1,16)+r5, H8(1,48)+r5       SACCH
         vadd     H32(0,0)+r3, H8(1,0)+r5,  H8(1,32)+r5       UACC
         vadd     -,           H8(32,16)+r5, H8(32,48)+r5 CLRA SACCH
         vadd     -,           H8(32,0)+r5,  H8(32,32)+r5     UACC
         vadd     -,           H8(33,16)+r5, H8(33,48)+r5     SACCH
         vadd     H32(32,0)+r3, H8(33,0)+r5, H8(33,32)+r5     UACC
         addcmpblt r5, r7, r6, .dloop

         vmulhn.uu H8(0++,0), H16(0++,0), 0x4000         REP r0
         vmulhn.uu H8(0++,16), H16(0++,32), 0x4000       REP r0
         vmulhn.uu H8(32++,0), H16(32++,0), 0x4000       REP r0
         vmulhn.uu H8(32++,16), H16(32++,32), 0x4000     REP r0

         mov      r3, 32
         cmp      r2, 32
         sub      r2, 32
         vst      H16(0++,0), (r4+=r7)                   REP r0
         vst      H16(32++,0), (r4+32+=r7)               REP r0
         addscale r4, r4, r0 * 128
         bgt      .loop

         mov      r0, r4 ; return new dst
         ldm      r6-r7, (sp++)
         .cfa_pop {%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_trans_sub_block_y
;;;
;;; SYNOPSIS
;;;   void yuvuv128_trans_sub_block_y(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  dst_height, /* r2 */
;;;      int                  src_height);/* r3 */
;;;
;;; FUNCTION
;;;   Core of the transpose_and_subsample routine.

.function yuvuv128_trans_sub_block_y ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}
         mov      r4, r0
         mov      r9, r3
         mov      r7, 128
         mov      r8, 0x0000200002

.loop:   mov      r0, 64
         min      r0, r9 ; src height
         vld      H32(0++,0), (r1+=r7)                   REP r0
         addscale r1, r1, r0 * 128

         mov      r3, -64
         mov      r5, 0
         shl      r6, r0, 6
         lsr      r0, 1
.dloop:  vmov     -, r8                                  CLRA SACC
         add      r3, 64
         vadd     -,           H8(0,32)+r5, H8(0,48)+r5       SACCH
         vadd     -,           H8(0,0)+r5,  H8(0,16)+r5       UACC
         vadd     -,           H8(1,32)+r5, H8(1,48)+r5       SACCH
         vadd     H32(0,0)+r3, H8(1,0)+r5,  H8(1,16)+r5       UACC
         addcmpblt r5, r7, r6, .dloop

         vlsr     H32(0++,0), H32(0++,0), 2              REP r0

         mov      r0, r2

         mov      r3, 0
         mov      r5, 0
.tloop:  vinterl  V8(32,0)+r3,  H8(0,0)+r5,  H8(0,32)+r5
         vinterh  V8(48,0)+r3,  H8(0,0)+r5,  H8(0,32)+r5
         vinterl  V8(32,16)+r3, H8(1,0)+r5,  H8(1,32)+r5
         vinterh  V8(48,16)+r3, H8(1,0)+r5,  H8(1,32)+r5
         vinterl  V8(32,8)+r3,  H8(16,0)+r5, H8(16,32)+r5
         vinterh  V8(48,8)+r3,  H8(16,0)+r5, H8(16,32)+r5
         vinterl  V8(32,24)+r3, H8(17,0)+r5, H8(17,32)+r5
         vinterh  V8(48,24)+r3, H8(17,0)+r5, H8(17,32)+r5
         add      r5, 128
         addcmpbne r3, 1, 8, .tloop

         cmp      r9, 64
         sub      r9, 64
         vst      H16(32++,0), (r4+=r7)                  REP r0
         add      r4, 32
         bgt      .loop

         mov      r0, r4 ; return new dst
         ldm      r6-r9, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET

;;; *****************************************************************************
;;; NAME
;;;   yuvuv128_trans_sub_block_uv
;;;
;;; SYNOPSIS
;;;   void yuvuv128_trans_sub_block_uv(
;;;      void                *dst,        /* r0 */
;;;      void          const *src,        /* r1 */
;;;      int                  dst_height, /* r2 */
;;;      int                  src_height);/* r3 */
;;;
;;; FUNCTION
;;;   Core of the transpose_and_subsample routine.

.function yuvuv128_trans_sub_block_uv ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}
         mov      r4, r0
         mov      r9, r3
         mov      r7, 128
         mov      r8, 0x0000200002

.loop:   mov      r0, 64
         min      r0, r9 ; src height
         vld      H32(0++,0), (r1+=r7)                   REP r0
         addscale r1, r1, r0 * 128

         mov      r3, -64
         mov      r5, 0
         shl      r6, r0, 6
         lsr      r0, 1
.dloop:  vmov     -, r8                                  CLRA SACC
         add      r3, 64
         vadd     -,           H8(0,16)+r5, H8(0,48)+r5       SACCH
         vadd     -,           H8(0,0)+r5,  H8(0,32)+r5       UACC
         vadd     -,           H8(1,16)+r5, H8(1,48)+r5       SACCH
         vadd     H32(0,0)+r3, H8(1,0)+r5,  H8(1,32)+r5       UACC
         addcmpblt r5, r7, r6, .dloop

         mov      r0, r2
         vlsr     H8(32++,0), V16(0,0++), 2              REP r0
         vlsr     H8(32++,16), V16(0,32++), 2            REP r0
         vlsr     H8(48++,0), V16(16,0++), 2             REP r0
         vlsr     H8(48++,16), V16(16,32++), 2           REP r0

         cmp      r9, 64
         sub      r9, 64
         vst      H16(32++,0), (r4+=r7)                  REP r0
         vst      H16(48++,0), (r4+32+=r7)               REP r0
         add      r4, 64
         bgt      .loop

         mov      r0, r4 ; return new dst
         ldm      r6-r9, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6}
         b        lr
.endfn ; }}}
.FORGET


;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_from_yuv420_uvstrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_from_yuv420_uvstrip(void *usrc, void *vsrc, void *dst, 
;;;                                       int height, int src_pitch);
;;;
;;; FUNCTION
;;;   move a strip of UV from YUV420 to YUV_UV, assume width of 128
;;;   it is assumed that the U and V planes have the same initial alignment
;;;   and the same pitch value so stay in sync with each other
;;;   no special treatment is given to last strip where useful width may
;;;   be less than 128. Garbage will be written to the padding region

.function yuvuv128_from_yuv420_uvstrip ; {{{
      stm r6-r7, lr, (--sp)

line_loop:
;calculate offset from 32 byte aligned position
			and r7,r0,0x1f
         mov r6, 0
         mov r5,r0
uv_loop:
;load a stripe line of UV, 64 bytes, allow for possible alignment offset 
;so load 96 bytes
			   sub r5,r7
            vld VX(0,0), (r5)
            vld VX(0,1), (r5+32)
			   addcmpbeq r7, 0, 0, offset0
            vld VX(0,2), (r5+64)
			   addcmpbeq r7, 0, 8, offset8
      		addcmpbeq r7, 0, 24, offset24
		   	b offset16
      
;align data and reorder ready for output      
offset0:
			   vto32l H(48,32)+r6, V(0,0), V(0,16)
   			vto32h H(49,32)+r6, V(0,0), V(0,16)
	   		vto32l H(50,32)+r6, V(0,1), V(0,17)
		   	vto32h H(51,32)+r6, V(0,1), V(0,17)
			   b finished
offset8:
   			vto32l H(32,0), V(0,0), V(0,16)
	   		vto32h H(32,16), V(0,0), V(0,16)
		   	vto32l H(32,32), V(0,1), V(0,17)
			   vto32h H(32,48), V(0,1), V(0,17)			
   			vto32l H(33,0), V(0,2), V(0,18)			
	   		b offset24_last8
offset16:
		   	vto32h H(48,32)+r6, V(0,0), V(0,16)
			   vto32l H(49,32)+r6, V(0,1), V(0,17)
   			vto32h H(50,32)+r6, V(0,1), V(0,17)
	   		vto32l H(51,32)+r6, V(0,2), V(0,18)			
		   	b finished
offset24:
   			vto32h H(32,0), V(0,0), V(0,16)
	   		vto32l H(32,16), V(0,1), V(0,17)
		   	vto32h H(32,32), V(0,1), V(0,17)
			   vto32l H(32,48), V(0,2), V(0,18)			
   			vto32h H(33,0), V(0,2), V(0,18)			
offset24_last8:			
;now move last 8 bytes over			
	   		vadd H(48,32)+r6, H(32,8), 0
		   	vadd H(49,32)+r6, H(32,24), 0
			   vadd H(50,32)+r6, H(32,40), 0
   			vadd H(51,32)+r6, H(32,56), 0
	   		vbitplanes -, 0xff00 SETF  ;dummy op to set PPU flags
		   	vadd H(51,32)+r6, H(33,56), 0 IFNZ  ;only move top 8 bytes
			
finished:
			   mov r5, r1
   			add r6, 16
	   		cmp r6, 32
		   	blt uv_loop  ;need to process U and V components
			
;now store merged UV line

		   mov r6,32
			vst HX(48++,32), (r2+=r6) REP 4
			
			add r0, r4
			add r1, r4
			add r2, 128
			addcmpbgt r3, -1, 0, line_loop
			
      ldm r6-r7, pc, (sp++)
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_from_yuv420_ystrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_from_yuv420_ystrip(void *ysrc, void *dst, int height,
;;;                                    int src_pitch);
;;;
;;; FUNCTION
;;;   move a strip of Y from YUV420 to YUV_UV, assume width of 128
;;;   it is assumed that the U and V planes have the same initial alignment
;;;   and the same pitch value so stay in sync with each other
;;;   no special treatment is given to last strip where useful width may
;;;   be less than 128. Garbage will be written to the padding region

.function yuvuv128_from_yuv420_ystrip ; {{{
      stm r6-r7, lr, (--sp)

line_loop:
;calculate offset from 32 byte aligned position
			and r7,r0,0x1f
         mov r6, 128
         mov r5,r0
         mov r4,32
;load a stripe line of Y, 128 bytes
			sub r5,r7
         vld HX(0++,0), (r5+=r4) REP 4
			addcmpbne r7, 0, 0, offset16
      
;align data and reorder ready for output      
offset0:
;0 offset, nothing to do but write it out again
         add r0, r3
			vst HX(0++,0), (r1+=r4) REP 4
			add r1, r6  
			addcmpbgt r2, -1, 0, line_loop

         ldm r6-r7, pc, (sp++)

offset16:
         vld HX(4,0), (r5+128)
         add r0, r3
			vbitplanes -, 0xff SETF  ;dummy op to set PPU flags
         vadd H(48++,32), H(0++,8), 0 REP 4 IFNZ
         vadd H(48++,48), H(0++,24), 0 REP 4 IFNZ
			vbitplanes -, 0xff00 SETF  ;dummy op to set PPU flags
         vadd H(48++,32), H(1++,56), 0 REP 4 IFNZ
         vadd H(48++,48), H(1++,8), 0 REP 4 IFNZ
			vst HX(48++,32), (r1+=r4) REP 4
			add r1, r6  
			addcmpbgt r2, -1, 0, line_loop
			
      ldm r6-r7, pc, (sp++)
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_to_yuv420_ylaststrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_to_yuv420_ylaststrip(void *ydest, void *src, int height,
;;;                                       int src_pitch, int width);
;;;
;;; FUNCTION
;;;   move a strip of Y from YUV420 to YUV_UV, assume strip width of 128
;;;   this version only writes the requested width of data as required for
;;;   cases where the line width is not a multiple of the strip width
;;;   however it will generally be slower than the version that always writes 
;;;   a fixed 128 bytes so only use where required
;;;   it is assumed that the U and V planes have the same initial alignment
;;;   and the same pitch value so stay in sync with each other
;;;   y offset can only be 16 (whereas uv can be 8)

.function yuvuv128_to_yuv420_ylaststrip ; {{{
      stm r6-r7, lr, (--sp)

line_loop:
;calculate offset from 32 byte aligned position
			add r5,r4,31
			lsr r5,5  ;number of 32 byte chunks			
			mov r6,0
;load required part of stripe line of Y, (up to 128 bytes)
			mov r7,r1
read_loop:
            vld HX(0,0)+r6, (r1+=r4)
            add r1,32
            add r6,64  ;move down one row
            addcmpbgt r5,-1,0,read_loop
      
            add r1,r7,128  ;increment input address by 128 
                           ;regardless of how many bytes used
      
;calculate offset from 32 byte aligned position
			and r7,r0,0x1f
			addcmpbne r7, 0, 0, offset16
      
;align data and reorder ready for output      
offset0:
;0 offset, write it out again, limit to line width
            add r7,r4,15
			lsr r7,4  ;number of 16 byte chunks			
			addcmpblt r7, 0, 2, last0
			vst HX(0,0), (r0)
			addcmpblt r7, -2, 2, last0
			vst HX(1,0), (r0+32)
			addcmpblt r7, -2, 2, last0
			vst HX(2,0), (r0+64)
			addcmpblt r7, -2, 2, last0
			vst HX(3,0), (r0+96)
			addcmpble r7,-2,0,loopcheck
last0:			
			addcmpble r7,0,0,loopcheck
;16 bytes left to write out
;need to shuffle ready for 16 byte write			
;get offset for line			
			lsr r5,r4,5  ;number of 32 byte chunks
			shl r5,6  ;VRF line offset required
			vto32l H(32,0), H(0,0)+r5, H(0,16)+r5  ;first 16 bytes
			lsr r5,1  ;memory offset required for write (32 byte chunks)
			add r5,r0			
			vst H(32,0), (r5)
			b loopcheck
			
offset16:
;first 16 bytes need to be shuffled and written out H
			vto32l H(32,0), H(0,0), H(0,16)  ;first 16 bytes

;middle 96 bytes written out as HX
			vbitplanes -, 0xff SETF  ;dummy op to set PPU flags
;NB doing one redundant operation to give allowable REP count
;should check speed implications vs extra instructions			
         vadd H(48++,32), H(0++,8), 0 REP 4 IFNZ
         vadd H(48++,48), H(0++,24), 0 REP 4 IFNZ
			vbitplanes -, 0xff00 SETF  ;dummy op to set PPU flags
         vadd H(48++,32), H(1++,56), 0 REP 4 IFNZ
         vadd H(48++,48), H(1++,8), 0 REP 4 IFNZ
         add r5,r4,15
         lsr r5,4  ;number of 16 byte chunks
         sub r7,r5,1  
         lsr r7,1  ;number of complete 32 byte chunks written before last 
                   ;16 byte write (if any)
;first 16 bytes will lways be written      
         addcmpblt r5,0,1, last8
         vst H(32,0), (r0)
         addcmpblt r5,-1,2, last8
			vst HX(48,32), (r0+16)
			addcmpblt r5,-2,2, last8
			vst HX(49,32), (r0+48)
			addcmpblt r5,-2,2, last8
			vst HX(50,32), (r0+80)
			addcmpble r5,-2,0,loopcheck
last8:
;last 16 bytes need to be shuffled and written out H
			shl r6,r7,6  ;convert to VRF line offset
         addcmpblt r5,0,1,loopcheck			
			vto32h H(33,0), H(0,0)+r6, H(0,16)+r6  ;last 16 bytes
			lsr r6,1  ;convert to memory offset of start of line
			add r6,r0
         vst H(33,0), (r6+16)

loopcheck:			
         add r0, r3
			addcmpbgt r2, -1, 0, line_loop
      ldm r6-r7, pc, (sp++)
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_to_yuv420_uvlaststrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_to_yuv420_uvlaststrip(void *udst, void *vdst, void *src, 
;;;                                      int height, int src_pitch, int width);
;;;
;;; FUNCTION
;;;   move a strip of UV from YUV420 to YUV_UV, assume strip width of 128
;;;   this version only writes the requested width of data as required for
;;;   cases where the line width is not a multiple of the strip width
;;;   however it will generally be slower than the version that always writes 
;;;   a fixed 128 bytes so only use where required
;;;   y offset can only be 16 (whereas uv can be 8)

.function yuvuv128_to_yuv420_uvlaststrip ; {{{
      stm r6-r7, lr, (--sp)

			add r7,r5,7  ;dont expect to have non multiple of 8 bytes 
			             ;so probably not needed
			asr r7,3  ;number of 8 bit chunks

line_loop:
;load a stripe line of UV, 64 bytes, allow for possible alignment offset
;so load 96 bytes
			mov r6,32
         vld HX(0++,0), (r2+=r6) REP 2
         vld HX(0++,32), (r2+64+=r6) REP 2
			add r2, 128

			mov r5,r7
			btest r0,3
			bne offset8
			
;multiple instructions seems to be most efficient otherwise there is a delay
;of 3 cycles for scalar value to be available to vector unit
offset0:
			mov r6,0  ;VRF offset to last eight bytes
			addcmpblt r5,0,2, last8bytes0
			vst H(0,0), (r0)
			vst H(0,16), (r1)
			mov r6,64  ;VRF offset to last eight bytes
			addcmpblt r5,-2,2, last8bytes0
			vst H(1,0), (r0+16)
			vst H(1,16), (r1+16)
			mov r6,32  ;VRF offset to last eight bytes
			addcmpblt r5,-2,2, last8bytes0
			vst H(0,32), (r0+32)
			vst H(0,48), (r1+32)
			mov r6,64+32  ;VRF offset to last eight bytes
			addcmpblt r5,-2,2, last8bytes0
			vst H(1,32), (r0+48)
			vst H(1,48), (r1+48)
;if we get here all 128 bytes have been written, nothing else to do			
			b finished

last8bytes0:
			addcmpble r5,0,0, finished
;there are a last set of 8 bytes to be written			
			lsr r5,r7,1  ;number of 16 byte chunks written
			shl r5,4  ;convert to memory offset
			add r0,r5
			add r1,r5
			vbitplanes -, 0xff SETF  ;dummy op to set PPU flags
			vst H(0,0)+r6, (r0)  IFNZ  ;only move top 8 bytes
			vst H(0,16)+r6, (r1)  IFNZ  ;only move top 8 bytes
         
			sub r0,r5
			sub r1,r5
			b finished
			
offset8:
;shuffle into two rows in correct order to use as circular buffer
			vmov H(2,16), H(0,16)
			vmov H(0,16), H(1,0)
			vmov H(1,0), H(2,16)
			vmov H(2,48), H(0,48)
			vmov H(0,48), H(1,32)
			vmov H(1,32), H(2,48)
;now write out
;first store odd 8 bytes      
			sub r6,r5,2  ;final write location offset if odd 8 bytes at end
			addcmpble r5,0,0, finished
			vbitplanes -, 0xff00 SETF  ;dummy op to set PPU flags
			vst H(0,56), (r0-8)  IFNZ  ;only move top 8 bytes;
			vst H(1,56), (r1-8)  IFNZ  ;only move top 8 bytes;
			addcmpblt r5,-1,2, last8bytes8
			
;multiple instructions seems to be most efficient otherwise there is a delay
;of 3 cycles for scalar value to be available to vector unit
			vst H(0,8), (r0+8)
			vst H(1,8), (r1+8)
			addcmpblt r5,-2,2, last8bytes8
			vst H(0,24), (r0+24)
			vst H(1,24), (r1+24)
			addcmpblt r5,-2,2, last8bytes8
			vst H(0,40), (r0+40)
			vst H(1,40), (r1+40)
			addcmpble r5,-2,0,finished

last8bytes8:
			shl r6,3
			addcmpble r5,0,0,finished		
		   add r0,r6
		   add r1,r6
		   add r6,8
			vbitplanes -, 0xff SETF  ;dummy op to set PPU flags
			vst H(0,0)+r6, (r0)  IFNZ  ;only move top 8 bytes;
			vst H(1,0)+r6, (r1)  IFNZ  ;only move top 8 bytes;
			sub r0,r6
			sub r1,r6			
			
finished:			
			add r0, r4
			add r1, r4
			addcmpbgt r3, -1, 0, line_loop
			
      ldm r6-r7, pc, (sp++)
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_to_yuv420_uvstrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_to_yuv420_uvstrip(void *udst, void *vdst, void *src,
;;;                                    int height, int src_pitch);
;;;
;;; FUNCTION
;;;   move a strip of UV from YUV420 to YUV_UV, assume strip width of 128
;;;   writes a full strip width (which may be more than the line width left)
;;;   use yuv420_to_yuvuv128_uvlaststrip128 where this matters
;;;   y offset can only be 16 (whereas uv can be 8)

.function yuvuv128_to_yuv420_uvstrip ; {{{
      stm r6-r7, lr, (--sp)

line_loop:
;load a stripe line of UV, 64 bytes, allow for possible alignment offset
;so load 96 bytes
			mov r6,32
         vld HX(0++,0), (r2+=r6) REP 4
			add r2, 128

;calculate offset from 32 byte aligned position
			and r7,r0,0x1f
; calculate 32 byte aligned U address before start of data
         mov r6, 0
         mov r5,r0
uv_loop:
;load a stripe line of UV, 64 bytes, allow for possible alignment offset
;so load 96 bytes
			
			addcmpbeq r7, 0, 0, offset0
			addcmpbeq r7, 0, 8, offset8
			addcmpbeq r7, 0, 24, offset24
			b offset16
      
;align data and reorder ready for output      
offset0:
			vfrom32l H(48,32), H(0,0)+r6, H(1,0)+r6
			vfrom32h H(48,48), H(0,0)+r6, H(1,0)+r6
			vfrom32l H(49,32), H(2,0)+r6, H(3,0)+r6
			vfrom32h H(49,48), H(2,0)+r6, H(3,0)+r6
			vst HX(48,32), (r5)
			vst HX(49,32), (r5+32)
			b finished
offset8:
;pick out odd bytes
			vfrom32l H(32,0), H(0,0)+r6, H(1,0)+r6
			vfrom32l H(32,16), H(2,0)+r6, H(3,0)+r6
;move with shift, ready to write out						
			vadd H(48,32), H(32,60), 0
			vadd H(49,32), H(32,12), 0
			vadd H(50,32), H(32,28), 0
;pick out even bytes
			vfrom32h H(32,0), H(0,0)+r6, H(1,0)+r6
			vfrom32h H(32,16), H(2,0)+r6, H(3,0)+r6
;move with shift, ready to write out						
			vadd H(48,48), H(32,60), 0
			vadd H(49,48), H(32,12), 0
			vadd H(50,48), H(32,28), 0
;now save
			sub r5,r7  ;move to 32 byte aligned offset
			vbitplanes -, 0xfff0 SETF  ;dummy op to set PPU flags
			vst HX(48,32), (r5) IFNZ
			vst HX(49,32), (r5+32)
			vbitplanes -, 0x000f SETF  ;dummy op to set PPU flags
			vst HX(50,32), (r5+64) IFNZ
			
			b finished
offset16:
;first 16 bytes can be written directly as u v already separated by HX load
			vst H(0,0)+r6, (r5)			
			vfrom32l H(48,32), H(1,0)+r6, H(2,0)+r6
			vfrom32h H(48,48), H(1,0)+r6, H(2,0)+r6
			vst HX(48,32), (r5+16)
;last 16 bytes can be written directly as u v already separated by HX load
			vst H(3,0)+r6, (r5+48)			
			
			b finished
offset24:
;pick out odd bytes
			vfrom32l H(32,0), H(0,0)+r6, H(1,0)+r6
			vfrom32l H(32,16), H(2,0)+r6, H(3,0)+r6
;move with shift, ready to write out						
			vadd H(48,32), H(32,52), 0
			vadd H(49,32), H(32,4), 0
			vadd H(50,32), H(32,20), 0
;pick out even bytes
			vfrom32h H(32,0), H(0,0)+r6, H(1,0)+r6
			vfrom32h H(32,16), H(2,0)+r6, H(3,0)+r6
;move with shift, ready to write out						
			vadd H(48,48), H(32,52), 0
			vadd H(49,48), H(32,4), 0
			vadd H(50,48), H(32,20), 0
;now save
			sub r5,r7  ;move to 32 byte aligned offset
			vbitplanes -, 0xf000 SETF  ;dummy op to set PPU flags
			vst HX(48,32), (r5) IFNZ
			vst HX(49,32), (r5+32)
			vbitplanes -, 0x0fff SETF  ;dummy op to set PPU flags
			vst HX(50,32), (r5+64) IFNZ
			
finished:
			mov r5, r1
			add r6, 16
			cmp r6, 32
			blt uv_loop  ;need to process U and V components
			
			add r0, r4
			add r1, r4
			addcmpbgt r3, -1, 0, line_loop
			
      ldm r6-r7, pc, (sp++)			
.endfn ; }}}

;;; ***************************************************************************
;;; NAME
;;;   yuvuv128_to_yuv420_ystrip
;;;
;;; SYNOPSIS
;;;   void yuvuv128_to_yuv420_ystrip(void *ydest, void *src, int height, 
;;;                                    int src_pitch);
;;;
;;; FUNCTION
;;;   move a strip of Y from YUV420 to YUV_UV, assume strip width of 128
;;;   writes a full strip width (which may be more than the line width left)
;;;   use yuv420_to_yuvuv128_ylaststrip128 where this matters
;;;   y offset can only be 16 (whereas uv can be 8)

.function yuvuv128_to_yuv420_ystrip ; {{{
      stm r6-r7, lr, (--sp)

line_loop:
;calculate offset from 32 byte aligned position
			and r7,r0,0x1f
; calculate 32 byte aligned Y address before start of data
         mov r6, 128
         mov r5,r0
         mov r4,32
;load a stripe line of Y, 64 bytes
         vld HX(0++,0), (r1+=r4) REP 4
			add r1, r6  
			addcmpbne r7, 0, 0, offset16
      
;align data and reorder ready for output      
fromyoffset0:
;0 offset, nothing to do but write it out again
			vst HX(0++,0), (r0+=r4) REP 4
         add r0, r3
			addcmpbgt r2, -1, 0, line_loop

      ldm r6-r7, pc, (sp++)

offset16:
;first and last 16 bytes need to be shuffled and written out H
			vto32l H(32,0), H(0,0), H(0,16)  ;first 16 bytes
			vto32h H(33,0), H(3,0), H(3,16)  ;last 16 bytes

;middle 96 bytes written out as HX
			vbitplanes -, 0xff SETF  ;dummy op to set PPU flags
;NB doing one redundant operation to give allowable REP count
;should check speed implications vs extra instructions			
         vadd H(48++,32), H(0++,8), 0 REP 4 IFNZ
         vadd H(48++,48), H(0++,24), 0 REP 4 IFNZ
			vbitplanes -, 0xff00 SETF  ;dummy op to set PPU flags
         vadd H(48++,32), H(1++,56), 0 REP 4 IFNZ
         vadd H(48++,48), H(1++,8), 0 REP 4 IFNZ
         vst H(32,0), (r0)
			vst HX(48++,32), (r0+16+=r4) REP 2
			vst HX(50,32), (r0+80)
         vst H(33,0), (r0+112)
         add r0, r3
			addcmpbgt r2, -1, 0, line_loop
			
      ldm r6-r7, pc, (sp++)
.endfn ; }}}



yuvuv_difference_4mb::
   stm r6-r7, lr, (--sp)

   ;; On entry we expect:
   ;;
   ;; Difference an aligned (up to)64 x r0 region of YUV_UV (4 Macroblocks)
   ;; r0 # {31-16} rows;  {15-0} bitfield of columns to use
   ;; r1 d source/destination
   ;; r2 a source
   ;; r3 address to store sum of squares
   ;; r4 address to store max. absolute difference

   ;; select columns and set r0 to number of rows
   vbitplanes      -,            r0                         SETF
   lsr             r0,           16

   ;; Load 4 macroblocks
   mov             r5,           128
   v32ld           H32(0++,0),   (r1+=r5)                   REP r0
   v32ld           H32(32++,0),  (r2+=r5)                   REP r0             ;; done with a source address, r2 now scratch register

   ;; difference and maximum difference
   vdist           H8(0++,0),    H8(0++,0),   H8(32++,0)    IFNZ REP r0
   vdist           H8(0++,16),   H8(0++,16),  H8(32++,16)   IFNZ REP r0
   vdist           H8(0++,32),   H8(0++,32),  H8(32++,32)   IFNZ REP r0
   vdist           H8(0++,48),   H8(0++,48),  H8(32++,48)   IFNZ REP r0

   vmov            H8(32,0),     0
   vmax            H8(32,0),     H8(0++,0),   H8(32,0)      IFNZ REP r0
   vmax            H8(32,0),     H8(0++,16),  H8(32,0)      IFNZ REP r0
   vmax            H8(32,0),     H8(0++,32),  H8(32,0)      IFNZ REP r0
   vmax            H8(32,0),     H8(0++,48),  H8(32,0)      IFNZ REP r0
   vmov            -,            H8(32,0)                   MAX r2
   st              r2,           (r4)

   v32st           H32(0++,0),   (r1+=r5)                   REP r0             ;; r5 holds 128

   ;; sum of squares
   vmul            H32(32,0),    H8(0++,0),   H8(0++,0)     IFNZ CLRA UACC REP r0
   vmul            H32(32,0),    H8(0++,16),  H8(0++,16)    IFNZ UACC REP r0
   vmul            H32(32,0),    H8(0++,32),  H8(0++,32)    IFNZ UACC REP r0
   vmul            H32(32,0),    H8(0++,48),  H8(0++,48)    IFNZ UACC REP r0
   vmov            -,            H32(32,0)                  IFNZ USUM r5
   st              r5,           (r3)

   ldm r6-r7, pc, (sp++)



yuvuv_downsample_4mb::
   ;; Downsample an aligned 64x16 region of YUV_UV to 32x8 (4 Macroblocks)
   ;; r0 destination Y
   ;; r1 destination UV
   ;; r2 destination pitch
   ;; r3 source Y
   ;; r4 source UV
   ;; r5 source pitch

   ;; Load 4 macroblocks (in an unusual byte order)
   v32ld     H32(0++,0),   (r3+=r5)                 REP 16      
   v32ld     H32(32++,0),  (r4+=r5)                 REP 8

   ;; Y component
   v16add    H16(16++,0),  H8(0++,0),   H8(0++,16)  REP 16
   v16add    H16(16++,32), H8(0++,32),  H8(0++,48)  REP 16
   v32add    H32(0,0),     H32(16,0),   H32(17,0)
   v32add    H32(1,0),     H32(18,0),   H32(19,0)
   v32add    H32(2,0),     H32(20,0),   H32(21,0)
   v32add    H32(3,0),     H32(22,0),   H32(23,0)
   v32add    H32(4,0),     H32(24,0),   H32(25,0)
   v32add    H32(5,0),     H32(26,0),   H32(27,0)
   v32add    H32(6,0),     H32(28,0),   H32(29,0)
   v32add    H32(7,0),     H32(30,0),   H32(31,0)
   vmulhn.uu H8(16++,0),   H16(0++,0),  0x4000      REP 8
   vmulhn.uu H8(16++,16),  H16(0++,32), 0x4000      REP 8
.ifdef __BCM2707B0__
   v16add    V16(16,0),    V16(16,0),   0 ;;; Jira  HW-1420 workaround
.endif
   v16st     H16(16++,0),  (r0+=r2)                 REP 8
.ifdef __BCM2707B0__
   v16add    H16(16,0),    H16(16,0),   0 ;;; Jira  HW-1420 workaround
.endif

   ;; UV components
   v16add    H16(0++,0),   H8(32++,0),  H8(32++,32) REP 8
   v16add    H16(0++,32),  H8(32++,16), H8(32++,48) REP 8
   v32add    H32(8,0),     H32(0,0),    H32(1,0)
   v32add    H32(9,0),     H32(2,0),    H32(3,0)
   v32add    H32(10,0),    H32(4,0),    H32(5,0)
   v32add    H32(11,0),    H32(6,0),    H32(7,0)
   vmulhn.uu H8(16++,0),   H16(8++,0),  0x4000      REP 4
   vmulhn.uu H8(16++,16),  H16(8++,32), 0x4000      REP 4
.ifdef __BCM2707B0__
   v16add    V16(16,0),    V16(16,0),   0 ;;; Jira  HW-1420 workaround
.endif
   v16st     H16(16++,0),  (r1+=r2)                 REP 4
.ifdef __BCM2707B0__
   v16add    H16(16,0),    H16(16,0),   0 ;;; Jira  HW-1420 workaround
.endif

   b lr



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; macro VERT_DBLK
;;;
;;; Perform vertical deblocking
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.macro VERT_DBLK, A, B, C, D, rstrng

   ;;; d
   v16sub          V16(16,0), V(0,C)*, V(0,B)*             ;;; C - B
   v16shl          V16(16,0), V16(16,0), 2                 ;;; x4
   v16add          V16(16,0), V16(16,0), V(0,A)*           ;;; + A
   v16sub          V16(16,0), V16(16,0), V(0,D)*           ;;; - D
   v16asr          V16(16,0), V16(16,0), 3                 ;;; / 8
   
   ;;; d1
   v16sign         V16(16,1), V16(16,0), 0                 ;;; store sign
   v16dist         V16(16,0), V16(16,0), 0                 ;;; abs
   v16dist         V16(16,0), V16(16,0), rstrng
   v16rsub         V16(16,0), V16(16,0), rstrng
   v16max          V16(16,0), V16(16,0), 0
   v16mul          V16(16,0), V16(16,0), V16(16,1)
   
   ;;; B1
   v16adds         V(0,B)*, V16(16,0), V(0,B)*

   ;;; C1
   v16rsubs        V(0,C)*, V16(16,0), V(0,C)*
   
   ;;; d2
   v16sub          V16(16,2), V(0,A)*, V(0,D)*             ;;; A - D
   v16asr          V16(16,2), V16(16,2), 2                 ;;; / 4
   v16dist         V16(16,0), V16(16,0), 0                 ;;; abs
   v16asr          V16(16,0), V16(16,0), 1                 ;;; abs( d1 ) / 2
   v32clip2        V16(16,2), V16(16,2), V16(16,0)

   ;;; A1
   v16rsubs        V(0,A)*, V16(16,2), V(0,A)*

   ;;; D1
   v16adds         V(0,D)*, V16(16,2), V(0,D)*

.endm ;;; VERT_DBLK



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; macro HORZ_DBLK
;;;
;;; Perform horizontal deblocking
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.macro HORZ_DBLK, A, B, C, D, X, rstrng

   ;;; d
   v16sub          H16(16,0), H(C,X)*, H(B,X)*             ;;; C - B
   v16shl          H16(16,0), H16(16,0), 2                 ;;; x4
   v16add          H16(16,0), H16(16,0), H(A,X)*           ;;; + A
   v16sub          H16(16,0), H16(16,0), H(D,X)*           ;;; - D
   v16asr          H16(16,0), H16(16,0), 3                 ;;; / 8
   
   ;;; d1
   v16sign         H16(17,0), H16(16,0), 0                 ;;; store sign
   v16dist         H16(16,0), H16(16,0), 0                 ;;; abs
   v16dist         H16(16,0), H16(16,0), rstrng
   v16rsub         H16(16,0), H16(16,0), rstrng
   v16max          H16(16,0), H16(16,0), 0
   v16mul          H16(16,0), H16(16,0), H16(17,0)
   
   ;;; B1
   v16adds         H(B,X)*, H16(16,0), H(B,X)*

   ;;; C1
   v16rsubs        H(C,X)*, H16(16,0), H(C,X)*
   
   ;;; d2
   v16sub          H16(18,0), H(A,X)*, H(D,X)*             ;;; A - D
   v16asr          H16(18,0), H16(18,0), 2                 ;;; / 4
   v16dist         H16(16,0), H16(16,0), 0                 ;;; abs
   v16asr          H16(16,0), H16(16,0), 1                 ;;; abs( d1 ) / 2
   v32clip2        H16(18,0), H16(18,0), H16(16,0)

   ;;; A1
   v16rsubs        H(A,X)*, H16(18,0), H(A,X)*

   ;;; D1
   v16adds         H(D,X)*, H16(18,0), H(D,X)*

.endm ;;; HORZ_DBLK



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void vc_dblk_y_4( unsigned             nrows,
;;;                   unsigned char        *dest,
;;;                   const unsigned char  *src,
;;;                   int                  strength );
;;;
;;; Perform vertical deblocking only on the image's first and last 4 luma rows.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.section .text$vc_dblk_y_4,"text"
.globl vc_dblk_y_4
vc_dblk_y_4:
.cfa_bf vc_dblk_y_4

   mov             r5, 128                                 ;;; pixel rows have a stripewidth of 128

   ;;;
   ;;; first column?
   ;;;
   test            r1
   bne             ._y_4_notfirst

   ;;; load luma
   vld             H(0++,0)*, (r2+=r5) REP 4               ;;; stripewidth

   v16mov          H16(16++,32), 0 REP 4                   ;;; initialise

   VERT_DBLK 6, 7, 8, 9, r3

   b lr                                                    ;;; done

._y_4_notfirst:

   ;;;
   ;;; last column?
   ;;;
   test            r2
   bne             ._y_4_notlast

   ;;; store luma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   b lr                                                    ;;; done

._y_4_notlast:

   ;;;
   ;;; neither first nor last
   ;;;

   ;;; load luma
   vld             H(0++,16)*, (r2+=r5) REP 4              ;;; stripewidth

   VERT_DBLK 14, 15, 16, 17, r3

   ;;; store luma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   VERT_DBLK 22, 23, 24, 25, r3

   cbadd1

   b lr                                                    ;;; done

.cfa_ef
.stabs "vc_dblk_y_4:F1",36,0,0,vc_dblk_y_4



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void vc_dblk_y_8( unsigned             nrows,
;;;                   unsigned char        *dest,
;;;                   const unsigned char  *src,
;;;                   int                  strength );
;;;
;;; Perform deblocking (horizontal then vertical) on the image's middle luma rows.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.section .text$vc_dblk_y_8,"text"
.globl vc_dblk_y_8
vc_dblk_y_8:
.cfa_bf vc_dblk_y_8

   mov             r5, 128                                 ;;; pixel rows have a stripewidth of 128

   ;;;
   ;;; first column?
   ;;;
   test            r1
   bne             ._y_8_notfirst

   ;;; load luma
   vld             H(0++,0)*, (r2+=r5) REP 8               ;;; stripewidth

   v16mov          H16(16++,32), 0 REP 4                   ;;; initialise

   HORZ_DBLK 2, 3, 4, 5, 0, r3
   VERT_DBLK 6, 7, 8, 9, r3

   b lr                                                    ;;; done

._y_8_notfirst:

   ;;;
   ;;; last column?
   ;;;
   test            r2
   bne             ._y_8_notlast

   ;;; store luma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   b lr                                                    ;;; done

._y_8_notlast:

   ;;;
   ;;; neither first nor last
   ;;;

   ;;; load luma
   vld             H(0++,16)*, (r2+=r5) REP 8              ;;; stripewidth

   HORZ_DBLK 2, 3, 4, 5, 16, r3
   VERT_DBLK 14, 15, 16, 17, r3
   
   ;;; store luma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   VERT_DBLK 22, 23, 24, 25, r3

   cbadd1

   b lr                                                    ;;; done

.cfa_ef
.stabs "vc_dblk_y_8:F1",36,0,0,vc_dblk_y_8



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void vc_dblk_uv_4( unsigned             nrows,
;;;                    unsigned char        *dest,
;;;                    const unsigned char  *src,
;;;                    int                  strength );
;;;
;;; Perform vertical deblocking only on the image's first and last 4 chroma rows.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.section .text$vc_dblk_uv_4,"text"
.globl vc_dblk_uv_4
vc_dblk_uv_4:
.cfa_bf vc_dblk_uv_4

   mov             r5, 128                                 ;;; pixel rows have a stripewidth of 128

   ;;;
   ;;; first column?
   ;;;
   test            r1
   bne             ._uv_4_notfirst

   ;;; load chroma
   vld             H(0++,0)*, (r2+=r5) REP 4               ;;; stripewidth
   v16mov          H16(16++,32), 0 REP 8                   ;;; initialise
   b lr                                                    ;;; done

._uv_4_notfirst:

   ;;;
   ;;; last column?
   ;;;
   test            r2
   bne             ._uv_4_notlast

   ;;; store chroma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth
   b lr                                                    ;;; done

._uv_4_notlast:

   ;;;
   ;;; neither first nor last
   ;;;

   ;;; load chroma
   vld             H(0++,16)*, (r2+=r5) REP 4              ;;; stripewidth

   VERT_DBLK 12, 14, 16, 18, r3                            ;;; u
   VERT_DBLK 13, 15, 17, 19, r3                            ;;; v
   
   ;;; store chroma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   cbadd1

   b lr                                                    ;;; done

.cfa_ef
.stabs "vc_dblk_uv_4:F1",36,0,0,vc_dblk_uv_4



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; void vc_dblk_uv_8( unsigned             nrows,
;;;                    unsigned char        *dest,
;;;                    const unsigned char  *src,
;;;                    int                  strength );
;;;
;;; Perform deblocking (horizontal then vertical) on the image's middle chroma rows.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.section .text$vc_dblk_uv_8,"text"
.globl vc_dblk_uv_8
vc_dblk_uv_8:
.cfa_bf vc_dblk_uv_8

   mov             r5, 128                                 ;;; pixel rows have a stripewidth of 128

   ;;;
   ;;; first column?
   ;;;
   test            r1
   bne             ._uv_8_notfirst

   ;;; load chroma
   vld             H(0++,0)*, (r2+=r5) REP 8               ;;; stripewidth

   v16mov          H16(16++,32), 0 REP 8                   ;;; initialise

   HORZ_DBLK 2, 3, 4, 5, 0, r3

   b lr                                                    ;;; done

._uv_8_notfirst:

   ;;;
   ;;; last column?
   ;;;
   test            r2
   bne             ._uv_8_notlast

   ;;; store chroma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth
   b lr                                                    ;;; done

._uv_8_notlast:

   ;;;
   ;;; neither first nor last
   ;;;

   ;;; load chroma
   vld             H(0++,16)*, (r2+=r5) REP 8              ;;; stripewidth

   HORZ_DBLK 2, 3, 4, 5, 16, r3
   VERT_DBLK 12, 14, 16, 18, r3                            ;;; u
   VERT_DBLK 13, 15, 17, 19, r3                            ;;; v
   
   ;;; store chroma
   vst             H(0++,0)*, (r1+=r5) REP r0              ;;; stripewidth

   cbadd1

   b lr                                                    ;;; done

.cfa_ef
.stabs "vc_dblk_uv_8:F1",36,0,0,vc_dblk_uv_8

