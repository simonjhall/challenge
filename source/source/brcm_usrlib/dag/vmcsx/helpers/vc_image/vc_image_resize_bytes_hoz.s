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

.FORGET

; UTILITY FUNCTION
;
; void vcir8_margin(unsigned char * dest, const unsigned char * src,
;                   int width, int height, int dest_pitch, int src_pitch);
;
; For each line, repeatedly copy the byte at src to a horizontal scan at dest.
; 
; This function exists because linear interpolation, when properly centered,
; might try to extrapolate past the bounds of the image. This cannot be
; allowed. So you must call vcir8_margin() to generate extrapolated pixels.
 
.section .text$vcir8_margin,"text"
.globl vcir8_margin
vcir8_margin:
   addcmpbgt r3, 0, 0, mgn8_nonzh
   b lr
mgn8_nonzh:
   stm r6-r7,lr,(--sp)
   mul r3, r4, r3
   add r3, r0  ; destination end pointer
   addcmpbeq r2, 0, 1, mgn8_width1
   sub r4, r2 

mgn8_widthn:
   ldb r6, (r1)
   add r7, r0, r2
mgn8_loop:
   stb r6, (r0)
   addcmpbne r0, 1, r7, mgn8_loop
   add r1, r5
   addcmpblo r0, r4, r3, mgn8_widthn
   ldm r6-r7,pc,(sp++)

mgn8_width1:
   ldb r6, (r1)
   add r1, r5
   stb r6, (r0)
   addcmpblo r0, r4, r3, mgn8_width1
   ldm r6-r7,pc,(sp++)




; === HORIZONTAL RESIZE FUNCTIONS ===
;
; Six variants are defined here, for each of the following cases:
;       
;                                 16 lines           1<=n<=8 lines
;       
; Linear interpolation            vcir8_lerp_16      vcir8_lerp_n
; Nearest neighbour               vcir8_nnbr_16      vcir8_nnbr_n
; Block average                   vcir8_bavg_16      vcir8_bavg_n
;
; Arguments are as follows in each case:
;
; void vcir8_xxxx_n(unsigned char * dest,      /*  r0  */
;                   const unsigned char * src, /*  r1  */
;                   int dest_width,            /*  r2  */
;                   int pos,                   /*  r3  */
;                   int step,                  /*  r4  */
;                   int dest_pitch,            /*  r5  */
;                   int src_pitch              /* (sp) */
;                   int nlines);               /* (sp+4) */
;        
; For linear interpolation, pos should be offset by (step-65536)>>1
; For nearest neighbour, pos should be offset by (step-1)>>1
; For block average, pos should not be offset.
;
; Source and destination pitches *must* be a multiple of 16.
; These functions do not deal with "extrapolated" boundary conditions.
; Linear interpolation variant assumes that step < (3<<16).
; Block average variant assumes than step > (1<<16).
; nlines is ignored in the 16-line cases.


; COMMON SETUP CODE

.section .text$vcir8_hozinit,"text"
.globl vcir8_hozinit
vcir8_hozinit:
   .cfa_bf vcir8_hozinit
   add r11, r0, r2             ; r11 points to end of destination array
   mov r6, 32
   add r2, pc, pcrel(vc_image_const_0_15)
   mov r7, 16                  ; r7 is constant 16
   sub r8, r11, r6             ; r8 points 32 before end of destination array
   mov r10, r4                 ; r10 is step
   ld r6, (sp+36)              ; r6 is source pitch
   mov r13, 0x100000           ; r13 is constant (16*65536)
   vld H(63,0), (r2)           ; magic number sequence 0..15
   
   cmp r0, r8                  ; LS if at least 32 columns to do; HI if <32
   mov r2, r0
   sethi r12
   bmask r2, 4                 ; Misaligned starting destination?
   vsub -, H(63,0), r2 SETF    ; N for pixels off the start. NN for valid ones.
   mul r4, r2
   sub r0, r2                  ; Backtrack destination to be aligned
   or  r12, r4
   sub r3, r4                  ; Backtrack Q16 source position to compensate
   cmp r12, 0                  ; HI if dest needed fixup, or <32 columns.

   mov r2, r1
   cbclr                       ; Clear circular buffer flags
   bmask r2, 4                 ; Misaligned source pointer?
   ld  r4, (sp+40)
   sub r1, r2                  ; Backtrack source to be aligned
   add r12, pc, pcrel(stfunctable)
   shl r2, 16
   ldh r4, (r12+r4<<1)
   add r3, r2                  ; Advance source position to compensate
   addscale r12, r4<<1         ; r12 points to "vst REP <n>" routine

   ; Note: there's a chance r3 is negative. We'll get undefined pixels at
   ; -ve positions. This is not a bug: such pixels should only arise due
   ; to alignment corrections and so will be N-flagged anyway. r3 may also
   ; have grown enough to take us into the second block of source, so we
   ; may have to load more than one tile initially.

   b lr                        ; We now return you to our main feature
   .cfa_ef

;;; vst H(32++,0), (r0+=r5) REP <n>; r2 may be used as a temporary
stfunctable:
   .half (store0-stfunctable)>>1
   .half (store1-stfunctable)>>1
   .half (store2-stfunctable)>>1
   .half (store3-stfunctable)>>1
   .half (store4-stfunctable)>>1
   .half (store5-stfunctable)>>1
   .half (store6-stfunctable)>>1
   .half (store7-stfunctable)>>1
   .half (store8-stfunctable)>>1
store1:
   vst H(32,0), (r0)
store0:
   b lr
store2:
   vst H(32++,0), (r0+=r5) REP 2
   b lr
store3:
   addscale r2, r0, r5<<1
   vst H(32++,0), (r0+=r5) REP 2
   vst H(34,0), (r2)
   b lr
store4:
   vst H(32++,0), (r0+=r5) REP 4
   b lr
store5:
   addscale r2, r0, r5<<2
   vst H(32++,0), (r0+=r5) REP 4
   vst H(36,0), (r2)
   b lr
store7:
   addscale r2, r0, r5<<2
   addscale r2, r5<<1
   vst H(38,0), (r2)
store6:
   addscale r2, r0, r5<<2
   vst H(32++,0), (r0+=r5) REP 4
   vst H(36++,0), (r2+=r5) REP 2
   b lr
store8:
   vst H(32++,0), (r0+=r5) REP 8
   b lr



.section .text$vcir8_lerp_16,"text"
.globl vcir8_lerp_16
vcir8_lerp_16:
   .cfa_bf vcir8_lerp_16
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
   
   bl vcir8_hozinit ; Setup registers and correct alignment
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   source pointer (aligned)
   ; r2   [temporary]
   ; r3   source position in Q16 format
   ; r4   [inner loop counter]
   ; r5   dest_pitch
   ; r6   src_pitch
   ; r7   constant #16
   ; r8   32 before end of destination top line
   ; r9   columns of source loaded so far
   ; r10  step size in Q16 format
   ; r11  points to end of destination top line
   ; r12  points to nlines storing function (not used for 16 lines)
   ; r13  constant #0x10000 (16*65536)
   ; BLS branch will succeed if first tile all present, else fail.
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains the "magic" byte sequence (0..15)

   mov r9, -65408      ; Nothing loaded initially, and need extra for lerp
   mov r12, 0x2000000  ; Handy constant (8*64*65536)

i816_loop: ; OUTER LOOP
   sub r2, r3, r10                        ; We need to have loaded
   addscale r2, r10<<4                    ; (pos+15*step+0xff80)>>16 inputs
   addcmpblt r2, 0, r9, i816_doneload     ; before we can proceed
i816_loadmore:
   vld H(0++,0)*, (r1+=r6) REP 16
   cbadd1
   add r1, r7
   addcmpble r9, r13, r2, i816_loadmore
i816_doneload:
   asr r2, r3, r7                         ; Integer part of pos   
   mov r4, 0                              ; Inner loop counter
.if _VC_VERSION >= 3
   bmask r2, 6
.else        
   addcmpble r3, 0, r12, i816_loop2
   sub r3, r12      ; To ensure that V(0,0)+r2 cannot wrap to V(0,16)
   sub r9, r12      ; we reset pos whenever it gets above 8*64<<16
.endif

i816_loop2: ; INNER LOOP
   vsub VX(16,0), V(0,1)+r2, V(0,0)+r2
   vmulhn.su VX(16,0), VX(16,0), r3
   vadds V(32,0)+r4, V(0,0)+r2, VX(16,0)
   add r3, r10
   asr r2, r3, r7
.if _VC_VERSION >= 3
   bmask r2, 6
.endif
   addcmpblt r4, 1, 16, i816_loop2

.if _VC_VERSION >= 3
   sub r2, r11, r0                      ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF    ; NN for valid pixels. N off the end
.ifdef __BCM2707B0__
.pragma offwarn
   vadd -, VX(32,0), 0
.pragma popwarn
.endif
   vst H(32++,0), (r0+=r5) IFNN REP 16  ; Store only the required pixels
.ifdef __BCM2707B0__
   vadd HX(47,0), HX(47,0), 0
.endif
   vmov -, 0 SETF                      ; Clear vector N flags for next time
.else
   bls i816_shortcut                    ; DANGER!!!! Long-distance flags!
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld H(32++,0), (r0+=r5) IFN REP 16  ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

i816_shortcut:
   vst H(32++,0), (r0+=r5) REP 16
   cmp r0, r8                          ; If at least 16 remain, take shortcut
.endif
 
   addcmpblo r0, r7, r11, i816_loop
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_lerp_16:F1",36,0,0,vcir8_lerp_16


.section .text$vcir8_lerp_n,"text"
.globl vcir8_lerp_n
vcir8_lerp_n:
   .cfa_bf vcir8_lerp_n
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
   
   bl vcir8_hozinit ; Setup registers and correct alignment
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   source pointer (aligned)
   ; r2   [temporary]
   ; r3   source position in Q16 format
   ; r4   [inner loop counter]
   ; r5   dest_pitch
   ; r6   src_pitch
   ; r7   constant #16
   ; r8   32 before end of destination top line
   ; r9   columns of source loaded so far
   ; r10  step size in Q16 format
   ; r11  points to end of destination top line
   ; r12  points to nlines storing function
   ; r13  constant #0x10000 (16*65536)
   ; BLS branch will succeed if first tile all present, else fail.
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains the "magic" byte sequence (0..15)

   mov r9, -65408     ; Nothing loaded initially, and need extra for lerp

i8n_loop: ; OUTER LOOP
   mov lr, r12
   sub r2, r3, r10                        ; We need to have loaded
   addscale r2, r10<<4                    ; (pos+15*step+0xff80)>>16 inputs
   addcmpblt r2, 0, r9, i8n_doneload      ; before we can proceed
i8n_loadmore:
   vld H(0++,0)*, (r1+=r6) REP 8
   cbadd1
   add r1, r7
   addcmpble r9, r13, r2, i8n_loadmore
i8n_doneload:
   asr r2, r3, r7                         ; Integer part of pos
   bmask r2, 6                            ; Mask for circular buffer
   mov r4, 0                              ; Inner loop counter
i8n_loop2: ; INNER LOOP
   vsub VX(16,0), V(0,1)+r2, V(0,0)+r2
   vmulhn.su VX(16,0), VX(16,0), r3
   vadds V(32,0)+r4, V(0,0)+r2, VX(16,0)
   add r3, r10
   asr r2, r3, r7
   bmask r2, 6
   addcmpblt r4, 1, 16, i8n_loop2

   bls i8n_shortcut            ; DANGER!!!! Long-distance flags!
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld H(32++,0), (r0+=r5) IFN REP 8   ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

i8n_shortcut:
   bl lr                               ; Subroutine for storing <n> rows
   cmp r0, r8                          ; If at least 16 remain, take shortcut
   addcmpblo r0, r7, r11, i8n_loop
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_lerp_n:F1",36,0,0,vcir8_lerp_n



.section .text$vcir8_nnbr_16,"text"
.globl vcir8_nnbr_16
vcir8_nnbr_16:
   .cfa_bf vcir8_nnbr_16
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
   
   bl vcir8_hozinit ; Setup registers and correct alignment
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   source pointer (aligned)
   ; r2   [temporary]
   ; r3   source position in Q16 format
   ; r4   [inner loop counter]
   ; r5   dest_pitch
   ; r6   src_pitch
   ; r7   constant #16
   ; r8   32 before end of destination top line
   ; r9   columns of source loaded so far
   ; r10  step size in Q16 format
   ; r11  points to end of destination top line
   ; r12  points to nlines storing function (not used for 16 lines)
   ; r13  constant #0x10000 (16*65536)
   ; BLS branch will succeed if first tile all present, else fail.
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains the "magic" byte sequence (0..15)

   mov r9, r10         ; Nothing loaded initially

n816_loop: ; OUTER LOOP
   mov r4, 0                              ; Inner loop counter
n816_loop2: ; INNER LOOP
   asr r2, r3, r7                         ; Integer part of pos
   bmask r2, 4
   addcmpblt r3, r10, r9, n816_doneload
n816_loadmore:
   add r1, 16
   addcmpble r9, r13, r3, n816_loadmore
   vld H(0++,0), (r1+-16+=r6) REP 16
n816_doneload:
   vadd V(32,0)+r4, V(0,0)+r2, 0
   addcmpblt r4, 1, 16, n816_loop2

   bls n816_shortcut           ; DANGER!!!! Long-distance flags!
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld H(32++,0), (r0+=r5) IFN REP 16  ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

n816_shortcut:
   vst H(32++,0), (r0+=r5) REP 16
   cmp r0, r8                          ; If at least 16 remain, take shortcut
   addcmpblo r0, r7, r11, n816_loop
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_nnbr_16:F1",36,0,0,vcir8_nnbr_16



.section .text$vcir8_nnbr_n,"text"
.globl vcir8_nnbr_n
vcir8_nnbr_n:
   .cfa_bf vcir8_nnbr_n
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
   
   bl vcir8_hozinit ; Setup registers and correct alignment
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   source pointer (aligned)
   ; r2   [temporary]
   ; r3   source position in Q16 format
   ; r4   [inner loop counter]
   ; r5   dest_pitch
   ; r6   src_pitch
   ; r7   constant #16
   ; r8   32 before end of destination top line
   ; r9   columns of source loaded so far
   ; r10  step size in Q16 format
   ; r11  points to end of destination top line
   ; r12  points to nlines storing function
   ; r13  constant #0x10000 (16*65536)
   ; BLS branch will succeed if first tile all present, else fail.
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains the "magic" byte sequence (0..15)

   mov r9, r10         ; Nothing loaded initially

n8n_loop: ; OUTER LOOP
   mov r4, 0                              ; Inner loop counter
   mov lr, r12
n8n_loop2: ; INNER LOOP
   asr r2, r3, r7                         ; Integer part of pos
   bmask r2, 4
   addcmpblt r3, r10, r9, n8n_doneload
n8n_loadmore:
   add r1, 16
   addcmpble r9, r13, r3, n8n_loadmore
   vld H(0++,0), (r1+-16+=r6) REP 8
n8n_doneload:
   vadd V(32,0)+r4, V(0,0)+r2, 0
   addcmpblt r4, 1, 16, n8n_loop2

   bls n8n_shortcut            ; DANGER!!!! Long-distance flags!
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld H(32++,0), (r0+=r5) IFN REP 8   ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

n8n_shortcut:
   bl lr                               ; Subroutine for storing <n> rows
   cmp r0, r8                          ; If at least 16 remain, take shortcut
   addcmpblo r0, r7, r11, n8n_loop
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_nnbr_n:F1",36,0,0,vcir8_nnbr_n


.section .text$vcir8_bavg_16,"text"
.globl vcir8_bavg_16
vcir8_bavg_16:
   .cfa_bf vcir8_bavg_16
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}
   
   bl vcir8_hozinit ; Setup registers and correct alignment
   
   ; REGISTER BINDINGS:
   ; r0   dest pointer (aligned)
   ; r1   source pointer (aligned)
   ; r2   [temporary]
   ; r3   source position in Q16 format
   ; r4   [inner loop counter]
   ; r5   dest_pitch
   ; r6   src_pitch
   ; r7   constant #16
   ; r8   32 before end of destination top line
   ; r9   columns of source loaded so far
   ; r10  step size in Q16 format
   ; r11  points to end of destination top line
   ; r12  points to nlines storing function (not used for 16 lines)
   ; r13  constant #0x10000 (16*65536)
   ; BLS branch will succeed if first tile all present, else fail.
   ; Vector N flags initially asserted for "before-the-start" columns.
   ; H(63,0) contains the "magic" byte sequence (0..15)

   add sp, -8
   st  r14, (sp)
   st  r15, (sp+4)
   .cfa_push 8

   add r14, r10, 65535
   add r2, pc, pcrel(vc_image_recip_0_16)
   lsr r14, 16
   min r4, r14, r7
   min r14, r7          ; r14 is number of columns to sum (max. 16)
   cbclr
   ld r2, (r2+r4<<2)
   shl r9, r14, r7
   add r2, -1
   rsub r9, r10         ; Nothing loaded initially, and need extra for bavg
   vmov HX(62,0), r2    ; HX(62,0) is reciprocal of number of columns

b816_loop: ; OUTER LOOP
   mov r4, 0                              ; Inner loop counter
b816_loop2: ; INNER LOOP
   asr r2, r3, r7                         ; Integer part of pos
   bmask r2, 6
   addcmpblt r3, r10, r9, b816_doneload
b816_loadmore:
   vld H(0++,0)*, (r1+=r6) REP 16
   cbadd1
   add r1, r7
   addcmpble r9, r13, r3, b816_loadmore
b816_doneload:
   vadd VX(16,0), V(0,0)+r2, V(0,1)+r2
   addcmpble r14, 0, 2, b816_donesum
   mov r15, 2
b816_loop3: ; INNER INNER LOOP
   vadd VX(16,0), VX(16,0), V(0,2)+r2
   add r2, 1
.if _VC_VERSION >= 3
   bmask r2, 6
.endif
   addcmpblt r15, 1, r14, b816_loop3
b816_donesum:
   vmulhn.su V(32,0)+r4, VX(16,0), HX(62,0)
   asr r2, r3, r7
   addcmpblt r4, 1, 16, b816_loop2

.if _VC_VERSION >= 3
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
.ifdef __BCM2707B0__
.pragma offwarn
   vadd -, V(32,0), 0
.pragma popwarn
.endif
   vst H(32++,0), (r0+=r5) IFNN REP 16      ; Save the desired pixels
   vmov -, 0 SETF
.ifdef __BCM2707B0__
   vadd H(47,0), H(47,0), 0
.endif
.else
   bls b816_shortcut           ; DANGER!!!! Long-distance flags!
   sub r2, r11, r0                     ; Number of pixels remaining
   vrsub -, H(63,0), r2+-1 IFNN SETF   ; NN for valid pixels. N off the end
   vld H(32++,0), (r0+=r5) IFN REP 16  ; Load in the pixels not to be modified
   vmov -, 0 SETF                      ; Clear vector N flags for next time

b816_shortcut:
   vst H(32++,0), (r0+=r5) REP 16
   cmp r0, r8                          ; If at least 16 remain, take shortcut
.endif
   addcmpblo r0, r7, r11, b816_loop

   ld  r14, (sp)
   ld  r15, (sp+4)
   add sp, 8
   .cfa_pop 8
        
   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_bavg_16:F1",36,0,0,vcir8_bavg_16


; Mostly-scalar implementation of the following:
;
; void vcir8_bavg_n(unsigned char * dest,      /*  r0  */
;                   const unsigned char * src, /*  r1  */
;                   int dest_width,            /*  r2  */
;                   int pos,                   /*  r3  */
;                   int step,                  /*  r4  */
;                   int dest_pitch,            /*  r5  */
;                   int src_pitch              /* (sp) */
;                   int nlines);               /* (sp+4) */

.section .text$vcir8_bavg_n,"text"
.globl vcir8_bavg_n
vcir8_bavg_n:
   .cfa_bf vcir8_bavg_n
   stm r6-r13,lr,(--sp)
   .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11,%r12,%r13}

   mov r6, r0                   ; r6=dest
   mov r7, r1                   ; r7=src
   add r8, r0, r2               ; r8=dest end pointer
   mov r9, r3                   ; r9=pos
   mov r10, r4                  ; r10=step
   mov r11, r5                  ; r11=dest_pitch
   ld  r12, (sp+36)             ; r12=src_pitch
   mov r5, 16                   ; r5=#16
   ld  r13, (sp+40)             ; r13=nlines

   add r4, 65535
   mov r1, 1
   lsr r4, r5
   min r4, r5                   ; #columns to average (clipped to max 16)
   shl r1, r4
   add r3, pc, pcrel(vc_image_recip_0_16)
   add r1, -1
   ld  r3, (r3+r4<<2)
   vbitplanes -, r1 SETF        ; Mask for averaging
   lsr r3, 2                    ; 16384 / #cols

b8n_loop1:
   asr       r1, r9, r5
   add       r1, r7             ; r1=address to load next block
   mov       r0, r6             ; r0=dest
   add       r2, r9, r10
   vldleft   H(0,0), (r1)
;   vldright  H(0,0), (r1)
   add       r6, r11
b8n_loop2:
   vmulm     -, H(0,0), r3 IFNZ USUM r4
   asr       r1, r2, r5
   add       r1, r7
   add       r4, 32
   add       r2, r10
   lsr       r4, 6
   vldleft   H(0,0), (r1)
;   vldright  H(0,0), (r1)
   stb       r4, (r0)
   addcmpblo r0, 1, r8, b8n_loop2
   add       r7, r12
   add       r8, r11
   addcmpbgt r13, -1, 0, b8n_loop1

   ldm r6-r13,pc,(sp++)
   .cfa_pop {%r13,%r12,%r11,%r10,%r9,%r8,%r7,%r6,%pc}
   .cfa_ef

.stabs "vcir8_bavg_n:F1",36,0,0,vcir8_bavg_n
