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

;;; vclib_memcpy_512( unsigned char *to, unsigned char *from, int numbytes );
;;;
;;; Copies <numbytes> bytes from address <from> to address <to> using the VRF.
;;; Assumes that data is 512 bit aligned
;;; 
.function vclib_memcpy_512
   mov   r3,32
   mov   r4,512
   add   r2,r0
   b  loop_512
   .align 32
loop_512:
   vld   HX(0++,0),(r1+=r3) REP 16
   vst   HX(0++,0),(r0+=r3) REP 16
   add   r1,r4
   addcmpblt   r0,r4,r2,loop_512
   b  lr
.endfn

;;; vclib_memmove( unsigned char *to, unsigned char *from, int numbytes );
;;;
;;; Copies <numbytes> bytes from address <from> to address <to> using the VRF.
;;; Faster if 32 byte aligned
;;; 

.function vclib_memmove
   addcmpbhi r0, 0, r1, copydown
   b vclib_memcpy
copydown:
      add r0, r2
      add r1, r2
      or r3,r0,r1
      or r3,r2
      rsub r2, r0
      bmask r3,5
      addcmpbeq r3,0,0,aligned32
      ;; if we were 16-byte aligned but not 32, then r3 must have the value 16 now
      addcmpbeq r3, 0, 16, aligned16
slowcopy:
      ;; byte copy until aligned
      mov r3, -16
      and r4, r0
      max r4, r2
      addcmpbls r0,0,r4, EL2
L2:
      ldb r5,(--r1)
      stb r5,(--r0)
      addcmpbhi r0,0,r4, L2
EL2:
      ;; now copy aligned region
      add r1, r3
      addcmpblo r0,r3,r2, J0
L0:
      vldleft  H(0,0),(r1)
;      vldright H(0,0),(r1)
      vst      H(0,0),(r0)
      add r1,r3
      addcmpbhs r0,r3,r2, L0
J0:
      sub r0, r3
      sub r1, r3
      addcmpbls r0,0,r2, EL3
L3:
      ldb r4,(--r1)
      stb r4,(--r0)
      addcmpbhi r0,0,r2, L3
EL3:
      b lr

aligned32:
      ;; src dst and len all aligned
      mov r3, -32
      add r1, r3
      addcmpbhs r0,r3,r2, loop32
      b lr

      ;; The alignment here stops the next instructions being spread
      ;; across cache lines which can dramatically reduce the number of
      ;; stalls.
   .align 32
loop32:
      vld   HX(0,0),(r1)
      vst   HX(0,0),(r0)
      add   r1,r3
      addcmpbhs   r0,r3,r2,loop32
      b  lr
aligned16:
      ;; src dst and len all aligned
      mov r3, -16
      add r1, r3
      addcmpbhs r0,r3,r2, loop16
      b lr

      ;; The alignment here stops the next instructions being spread
      ;; across cache lines which can dramatically reduce the number of
      ;; stalls.
   .align 32
loop16:
      vld   H(0,0),(r1)
      vst   H(0,0),(r0)
      add   r1,r3
      addcmpbhs r0,r3,r2,loop16
      b  lr
.endfn

;;; vclib_aligned_memset2( void *to, int value, int numhalfwords );
;;; 
;;; sets the halfwords from to to to+numhalfwords to value.
;;; to should be aligned to 32 bytes. numhalfwords should be a multiple of 16
.function vclib_aligned_memset2
; need numhalfwords*2 % 16*32 == 0
   addscale r4, r0, r2<<1  ; end pointer
   lsr r2, 8
   shl r2, 8               
   addscale r2, r0, r2<<1  ; rounded down end pointer

   vmov HX(0,0), r1
   mov r1, 32
   mov r3, 32*16

   addcmpbge r0, 0, r2, 2f
1:
.pragma offwarn ; turn off warnings for REP with no ++
   vst HX(0,0), (r0+=r1) REP 16
.pragma popwarn
   addcmpblt r0, r3, r2, 1b
2:
   addcmpbeq r2, 0, r4, 2f
1:
   vst HX(0,0), (r0+=r1)
   addcmpblt r0, r1, r4, 1b
2:
   b lr
.endfn

;;; vclib_memset2( void *to, int value, int numhalfwords );
;;; 
;;; sets the halfwords from to to to+numhalfwords to value.
;;; to should be aligned to half-word boundary.
.function vclib_memset2
      add r2,r2
      add r3,r0,31
      mov r4,-32
      and r4,r3
      addcmpblo r2,r0,r4, J2    ; r2=to+numbytes, finish off if r2<r4
      ;; enough bytes to copy up to alignment
      addcmpbhs r0,0,r4, EL7
L7:
      sth r1,(r0)
      addcmpblo r0,2,r4, L7
EL7:
      ;; now set aligned region
      mov r4,-32
      and r4,r2
      addcmpbhs r0,0,r4, J2
      vmov HX(0,0),r1
      mov r5,32
L8:
      vst HX(0,0),(r0)
      addcmpblo r0,r5,r4, L8
J2:
      addcmpbhs r0,0,r2, EL9
L9:
      sth r1,(r0)
      addcmpblo r0,2,r2, L9
EL9:
      b lr
.endfn

;;; vclib_memset4( void *to, int value, int numwords );
;;; 
;;; sets the halfwords from to to to+numwords to value.
;;; to should be aligned to word boundary.
.if _VC_VERSION < 3
.function vclib_memset4
      shl r2,2
      add r3,r0,63
      mov r4,-64
      and r4,r3
      addcmpblo r2,r0,r4, J2    ; r2=to+numbytes, finish off if r2<r4
      ;; enough bytes to copy up to alignment
      addcmpbhs r0,0,r4, EL7
L7:
      st r1,(r0)
      addcmpblo r0,4,r4, L7
EL7:
      ;; now set aligned region
      mov r4,-64
      and r4,r2
      addcmpbhs r0,0,r4, J2
.if _VC_VERSION >= 3
      v32mov H32(0,0),r1
.else
      lsr r5, r1, 16
      vmov HX(0,0), r1
      vmov HX(0,32),r5
.endif
      mov r5,64
L8:
      VST_H32 <(0,0)>, r0
      addcmpblo r0,r5,r4, L8
J2:
      addcmpbhs r0,0,r2, EL9
L9:
      st r1,(r0)
      addcmpblo r0,4,r2, L9
EL9:
      b lr
.endfn
.else

.function vclib_memset4         ; {{{
   ;; loop head, might be mis-aligned
   bmask       r4, r0, 6
   mov         r3, -1
   v32mov      H32(0,0),      r1
   sub         r1, r0, r4
   lsr         r4, 2
   add         r5, r4, r2
.ifdef __BCM2707B0__
.pragma offwarn
   v32add      -,             V32(0,0),      0
   nop
.pragma popwarn
.endif
   shl         r3, r4
   min         r5, 16
   add         r2, r4
   bmask       r3, r5
   v16bitplanes -,            r3                            SETF
   cmp         r2, 16
   v32st       H32(0,0),      (r1)                          IFNZ
   ble         7f
   ;; main body - setup
   sub         r2, 16           ; actual remaining count
   add         r1, 64           ; advance pointer
   lsr         r4, r2, 4        ; body iteration count
   mov         r5, 127          ; max per iteration
   min         r0, r4, r5
   mov         r3, 64           ; pitch
   ;; main body
.pragma offwarn
1: v32st       H32(0,0),      (r1+=r3)                      REP r0
.pragma popwarn
   cmp         r4, r0
   sub         r4, r0
   addscale    r1, r0 << 6
   min         r0, r4, r5
   bgt         1b
   ;; remaining tail
   bmask       r2, 4
   mov         r3, -1
   bmask       r3, r2
   addcmpbeq   r2, 0, 0, 7f
   v16bitplanes -,            r3                            SETF
   v32st       H32(0,0),      (r1)                          IFNZ
7:
.ifdef __BCM2707B0__
   v32add      H32(0,0),      H32(0,0),      0
.endif
   b           lr
.endfn                          ; }}}

.endif

;;; vclib_memset( unsigned char *to, int value, int numbytes );
;;;
;;; Sets <numbytes> bytes at address <to> to be equal to <value>
;;; Quicker if the <to> pointer is be 256 bit aligned

.function vclib_memset
      ;; uncomment this to check if the thread calling this has the VRF lock...
;;      mov r3, lr
;;      stm r0-r3, (--sp)
;;      bl vclib_check_VRF
;;      ldm r0-r3, (sp++)
;;      mov lr, r3

      add r3,r0,15
      mov r4,-16
      and r4,r3
      addcmpblo r2,r0,r4, J1    ; r2=to+numbytes, finish off if r2<r4
      ;; enough bytes to copy up to alignment
      addcmpbhs r0,0,r4, EL4
L4:
      stb r1,(r0)
      addcmpblo r0,1,r4, L4
EL4:
      ;; now set aligned region
      mov r4,-16
      and r4,r2
      addcmpbhs r0,0,r4, J1
      vmov H(0,0),r1
      mov r5,16
L5:
.ifdef __BCM2707B0__
.pragma offwarn
      v16add -, V8(0,0), 0
.pragma popwarn
.endif
      vst H(0,0),(r0)
.ifdef __BCM2707B0__
      v16add H8(0,0), H8(0,0), 0
.endif
      addcmpblo r0,r5,r4, L5
J1:
      addcmpbhs r0,0,r2, EL6
L6:
      stb r1,(r0)
      addcmpblo r0,1,r2, L6
EL6:
      b lr
.endfn


.ifndef WANT_VCFW
;;; NAME
;;;    vclib_disableint
;;;
;;; SYNOPSIS
;;;    int vclib_disableint(void)
;;;
;;; FUNCTION
;;;    Disables interrupts
;;;
;;; RETURNS
;;;    The old status register
.function vclib_disableint
   mov r0,sr
.if _VC_VERSION > 2
   nop
.endif
   di
   b lr
.endfn
.stabs "vclib_disableint:F1",36,0,0,vclib_disableint


;;; NAME
;;;    vclib_restoreint
;;;
;;; SYNOPSIS
;;;    void vclib_restoreint(int sr);
;;;
;;; FUNCTION
;;;    Restores the status register to the given value
;;;
;;; RETURNS
;;;    -

.function vclib_restoreint
   btest r0, 30
   bne 1f
   di
   b lr
1:
   ei
   b lr
.endfn

;;; NAME
;;;    vclib_changestack
;;;
;;; SYNOPSIS
;;;    int vclib_changestack(void)
;;;
;;; FUNCTION
;;;    Disables interrupts
;;;
;;; RETURNS
;;;    The old status register

.function vclib_changestack
   mov r1,sp
   mov sp, r0
   mov r0, r1   
   b lr
.endfn

;;; NAME
;;;    vclib_restorestack
;;;
;;; SYNOPSIS
;;;    void vclib_restoreint(int sr);
;;;
;;; FUNCTION
;;;    Restores the status register to the given value
;;;
;;; RETURNS
;;;    -

.function vclib_restorestack
   mov sp,r0
   b lr
.endfn
.endif

;; Put a test pattern in the VRF and accumulators,
;; and check that it is intact.

.function vrf_set_pattern
        mov r0, 3072
        mov r1, -1024
vrf_set_loop:
        vmov HX(0,0)+r0,  0x1204
        vmov VX(0,0)+r0,  0xeace
        vmov HX(0,32)+r0, 0x1204
        vmov VX(0,32)+r0, 0xeace CLRA ACC
        addcmpbge r0, r1, 0, vrf_set_loop
        b lr
.endfn

.function vrf_check_pattern
        stm r6,lr,(--sp)
        .cfa_push{%lr,%r6}
.if _VC_VERSION < 3
        veor -, HX(0,0), HX(0,0) ACC USUM r5
.else
        veor -, HX(0,0), HX(0,0) ACC 
        v16getaccs16 -, 16 USUM r5
.endif
        mov r2, 0x1f90a
        mov r3, 0xeace0
        addcmpbeq r5, 0, r3, vrf_check_ok0
vrf_check_bad:
        bkpt
        mov r0, 0
        b vrf_check_end
vrf_check_ok0:
        mov r0, 3072
        mov r1, -1024
vrf_check_loop:
        vadd -, HX(0,0)+r0, 0 USUM r4
        vadd -, VX(0,0)+r0, 0 USUM r5
        veor -, HX(0,32)+r0, HX(0,0)+r0 USUM r6
        addcmpbne r4, 0, r2, vrf_check_bad
        veor -, VX(0,32)+r0, VX(0,0)+r0 USUM r4
        addcmpbne r5, 0, r3, vrf_check_bad
        addcmpbne r6, r4, 0, vrf_check_bad
        addcmpbge r0, r1, 0, vrf_check_loop
        mov r0, 1
vrf_check_end:
        ldm r6,pc,(sp++)
        .cfa_pop{%r6,%pc}
.endfn



;;; NAME
;;;   vclib_bytereorder32
;;;
;;; SYNOPSIS
;;;   void vclib_bytereorder32
;;;            unsigned long       *out,     /* r0 */
;;;            unsigned long const *in,      /* r1 */
;;;            int                  count,   /* r2 */
;;;            int                  byte0,   /* r3 */
;;;            int                  byte1,   /* r4 */
;;;            int                  byte2,   /* r5 */
;;;            int                  byte3);  /* r6 */
;;;
;;; FUNCTION
;;;   Reorders the bytes in each 32-bit word.  This function is limited to
;;;   operating in multiples of 256 words.
;;;
;;; RETURNS
;;;    -

.if _VC_VERSION >= 3
.function vclib_bytereorder32
         stm r6-r7,lr,(--sp)
         .cfa_push{%lr,%r6,%r7}

         ld     r6, (sp+12)
         mov   r7, 64
         asr   r2, 8

         and   r3, 3
         and   r4, 3
         and   r5, 3
         and   r6, 3
         shl   r3, 4
         shl   r4, 4
         shl   r5, 4
         shl   r6, 4

.loop:   vld   H32(0++,0), (r1+=r7)          REP 16
         add   r1, 1024
         vmov  H8(48++,0),  H8(0++,0)+r3     REP 16
         vmov  H8(48++,16), H8(0++,0)+r4     REP 16
         vmov  H8(48++,32), H8(0++,0)+r5     REP 16
         vmov  H8(48++,48), H8(0++,0)+r6     REP 16
         vst   H32(48++,0), (r0+=r7)         REP 16
         add   r0, 1024
         addcmpbgt r2, -1, 0, .loop

         ldm r6-r7,pc,(sp++)
         .cfa_pop{%r7,%r6,%pc}
.endfn
.endif

