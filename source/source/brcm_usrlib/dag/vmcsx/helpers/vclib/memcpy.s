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
   
   ;; r0 is dst
   ;; r1 is src
   ;; r2 is n

.if _VC_VERSION < 3
.function vclib_memcpy ; {{{
   
   ;; Apparently conditional stores can cause a bug when they
   ;; interact with the cache and external memory, so we cannot use them.

   ;; The case for optimisation is in fact the internal to
   ;; internal case, as most memcpys to or from external memory 
   ;; should already be using the DMA processor, and definitely are 
   ;; when doing MPEG4 decode and display.

   ;; Case VLD HX, VST HX:   8 bytes per cycle
   ;; Case VLD HX, VSTLEFT VSTRIGHT HX: 6.4 bytes per cycle
   ;; Case VLDLEFT VLDRIGHT HX, VST HX: 6.4 bytes per cycle
   ;; Case VLDLEFT VLDRIGHT HX, VSTLEFT VSTRIGHT HX: 5.3 bytes per cycle
   ;; Case VLD H, VST H: 4 bytes per cycle (not worth doing!)
   ;; Case VLD H, VSTLEFT VSTRIGHT H: 3.2 bytes per cycle
   ;; Case VLDLEFT VLDRIGHT H, VST H: 3.2 bytes per cycle
   ;; Case VLDLEFT VLDRIGHT H, VSTLEFT VSTRIGHT H:   2.7 bytes per cycle

   ;; Note that loads take a lot longer than stores on external
   ;; memory. For this reason we prefer to line up the source
   ;; address.
   
   addcmpblo r2,0,32,.finish

   ;; Alignment   src   dst
   ;;     32   32   aligned32 (8 bytes/cycle)
   ;;     >=2   >=2   aligned2 (5.3 bytes/cycle)

   ;; Otherwise line up src to 16 bytes and go to

   ;;     16   1   src_aligned16 (5.3 bytes/cycle)

   or r4,r1,r0
   mov r3,-32

   btest r4,0
   bmask r4,5

   addcmpbeq r4,0,0,.aligned32

   beq .aligned2

   bmask r5,r1,4

   addcmpbne r5,0,0,.line_up_src 

   ;; We need to pre-load H(0,0) before jumping into the loop.
   
   ;; Also there have to be two cycles taken up to avoid the 
   ;; vstleft/vstright bug.
   
   vld H(0,0),(r1)

   mov r3,-16
   
   b .src_aligned16_loop


.line_up_src:
   ;; Want r2 <- 16-r5
   ;; in terms of r3 = -16
   ;; i.e. r2 <- -(r3+r5)
   ;; Want r3 <- r2-(16-r5)
   ;; i.e. r3 <- r2+r5+r3
   add r5,r3
   mov r4,lr

   add r3,r2,r5
   neg r2,r5

   bl .finish_scalar

   mov r2,r3
   mov lr,r4
     
   addcmpblo r2,0,32,.finish


   ;; We need to pre-load the H(0,0) register before jumping into
   ;; the loop. Also there have to be two cycles wasted to avoid
   ;; the vstleft/vstright bug
   vld H(0,0),(r1)

   mov r3,-16
     
   b .src_aligned16_loop

   
   ;; r3 = -32
   .align 16
.aligned2:   
   vldleft HX(0,0),(r1)
   
   vldright HX(0,0),(r1)

   nop        ; waste a cycle to avoid the vstleft bug
   
   b .aligned2_loop   
   
   .align 16

.aligned2_loop:
   vstleft HX(0,0),(r0)
   
   vstright HX(0,0),(r0)

   vldleft HX(0,0),(r1+32)
   
   vldright HX(0,0),(r1+32)
   
   add r1,32
   add r0,32

   addcmpbhs r2,r3,32,.aligned2_loop

   b .finish

   ;; r3 = -32
   .align 32
.aligned32:
   vld HX(0,0),(r1)     

   vst HX(0,0),(r0)

   add r1,32
   add r0,32
   
   addcmpbhs r2,r3,32,.aligned32

   b .finish


.if 0   
   .align 16
   ;; r3 = -32
.src_aligned32_dst_aligned2:
   vld HX(0,0),(r1)
   
   add r1,32
   add r0,32     ; insert register stall
   
   vstleft HX(0,0),(r0-32)
   
   vstright HX(0,0),(r0-32)
   
   addcmpbhs r2,r3,32,.src_aligned32_dst_aligned2
   
   b .finish
.endif



   ;; r3 = -16
   
   ;; H(0,0) already loaded from (r1) and two cycles taken since
   ;; load.

   .align 16
.src_aligned16_loop:   
   
   vstleft H(0,0),(r0)
   
   vstright H(0,0),(r0)

   vld H(0,0),(r1+16)

   add r1,16
   add r0,16
     
   addcmpbhs r2,r3,16,.src_aligned16_loop
   
   b .finish


   ;; may not use any register except r0,r1,r2,r5 because called
   ;; from line_up_src
.finish:
   addcmpbeq r2,0,0,.b_lr
   
   addcmpblo r2,0,16,.finish_scalar

   mov r5,-16

.aligned1:        ; costs a stall if non-aligned
         ; if the loop is ever repeated, 
         ; but that should never happen 
   vldleft H(0,0),(r1)
   
   vldright H(0,0),(r1)

   add r1,16
   add r0,16     ; insert register stall
   
   vstleft H(0,0),(r0-16)
   
   vstright H(0,0),(r0-16)

   addcmpbhs r2,r5,16,.aligned1
   
.finish_scalar:
   or r5,r0,r1
   
   bmask r5,2

   ;; a switch statement is slower
   addcmpbeq r5,0,0,.finish_scalar_4
   
   addcmpbeq r5,0,2,.finish_scalar_2

   b .finish_scalar_1
   
.finish_scalar_4:
   addcmpblo r2,0,4,.finish_scalar_2

.finish_scalar_4_loop:
   ld r5,(r1++)
   
   st r5,(r0++)
   
   addcmpbge r2,-4,4,.finish_scalar_4_loop
   
   addcmpbeq r2,0,0,.b_lr

.finish_scalar_2:
   addcmpblo r2,0,2,.finish_scalar_1

.finish_scalar_2_loop:   
   ldh r5,(r1++)
   
   sth r5,(r0++)
   
   addcmpbge r2,-2,2,.finish_scalar_2_loop
     
.finish_scalar_1:
   addcmpbeq r2,0,0,.b_lr

.finish_scalar_1_loop:     
   ldb r5,(r1++)
   
   stb r5,(r0++)
   
   addcmpbne r2,-1,0,.finish_scalar_1_loop
.b_lr:   
   b lr
   
.endfn   ; }}}
.else
.function vclib_memcpy ; {{{
         or    r3, r0, r1
         and   r3, 3
         addcmpbne r3, 0, 0, .goslowly

.goquickly:
         bmask r5, r2, 6
         lsr   r2, 6

         mov   r4, 64
         mov   r3, r0
         min   r0, r2, r4
         nop
.loop64: vld   H32(0++,0), (r1+=r4)          REP r0
         sub   r2, r0
         addscale r1, r0 * 64
         cmp   r2, 0
.ifdef __BCM2707B0__
.pragma offwarn
         v32add -, V32(0,0), 0
.pragma popwarn
.endif
         vst   H32(0++,0), (r3+=r4)          REP r0
.ifdef __BCM2707B0__
         v32add H32(0,0), H32(0,0), 0
.endif
         addscale r3, r0 * 64
         min   r0, r2, r4
         bgt   .loop64

         vmov  H16(0,0), -1
         addcmpbeq r5, 0, 0, .finish      ; predict not taken

         mov   r0, -1
         lsr   r2, r5, 2
         shl   r0, r2
         bmask r5, 2
         vbitplanes -, r0                    SETF
         vld   H32(0,0), (r1)                IFZ
         addscale r1, r2 * 4
.ifdef __BCM2707B0__
.pragma offwarn
         v32add -, V32(0,0), 0
.pragma popwarn
.endif
         vst   H32(0,0), (r3)                IFZ
.ifdef __BCM2707B0__
         v32add H32(0,0), H32(0,0), 0
.endif
         addscale r3, r2 * 4

         addcmpbne r5, 0, 0, .lastbytes   ; predict not taken
         b     lr
.lastbytes:
         ldb   r0, (r1++)
         stb   r0, (r3++)
         addcmpbeq r5, 0, 1, .finish
         ldb   r0, (r1++)
         stb   r0, (r3++)
         addcmpbeq r5, 0, 2, .finish
         ldb   r0, (r1++)
         stb   r0, (r3++)
.finish: b     lr


; TODO: if ((r0 ^ r1) & 3) == 0, we can crawl through the first couple of bytes
; and then hop back to .goquickly

.goslowly:
         bmask r5, r2, 4
         lsr   r2, 4

         mov   r4, 16
         mov   r3, r0
         min   r0, r2, 16
         nop
.loop16: vld   H8(0++,0), (r1+=r4)           REP r0
         sub   r2, r0
         addscale r1, r0 * 16
         cmp   r2, 0
.ifdef __BCM2707B0__
.pragma offwarn
         v32add -, V32(0,0), 0
.pragma popwarn
.endif
         vst   H8(0++,0), (r3+=r4)           REP r0
.ifdef __BCM2707B0__
         v32add H32(0,0), H32(0,0), 0
.endif
         addscale r3, r0 * 16
         min   r0, r2, 16
         bgt   .loop16

         mov   r0, -1
         shl   r0, r5
         vbitplanes -, r0                    SETF
         ;; don't load conditional because of B0 bug, just store conditional
         vld   H8(0,0), (r1)
.ifdef __BCM2707B0__
.pragma offwarn
         v32add -, V32(0,0), 0
.pragma popwarn
.endif
         vst   H8(0,0), (r3)                 IFZ
.ifdef __BCM2707B0__
         v16add H8(0,0), H8(0,0), 0
.endif
         b     lr
.endfn ; }}}
.endif
.FORGET


.if _VC_VERSION >= 3
.function vclib_memcpy64
         mov   r4, 64
         mov   r3, r0
         min   r0, r2, r4
         nop
.loop:   vld   H32(0++,0), (r1+=r4)          REP r0
         sub   r2, r0
         addscale r1, r0 * 64
         cmp   r2, 0
         vst   H32(0++,0), (r3+=r4)          REP r0
         addscale r3, r0 * 64
         min   r0, r2, r4
         bgt   .loop

         b     lr
.endfn
.endif
