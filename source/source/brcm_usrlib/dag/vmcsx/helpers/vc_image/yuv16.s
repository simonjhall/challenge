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

.section .text$vc_yuv16_common,"text"
.globl vc_yuv16_common
vc_yuv16_common:
   ;; Code common to the various flavours of YUV->RGB transformation.
   ;; Intended for internal use only; passes arguments and results
   ;; in the VRF.
   ;;
   ;; Inputs: 16*16 of Y in H(0++,0)
   ;;           8*8 of U in H(16++,0)
   ;;           8*8 of V in H(24++,0)
   ;;
   ;; Outputs: 16*16 of RGB565 in HX(0++,32)  [green bits set only]
   ;;          16*16 of RGB565 in HX(16++,32) [red and blue bits only]
   ;;
   ;; Uses the entire top half of the VRF as temporary storage.
   ;; Uses registers r0,r1,r2. Preserves registers r3 and above.
   
   ; Scale up the U and V components in the X direction only
   ; and subtract 128, placing them from HX(16,32) onwards.
   vsub VX(16,32++), V(16,0), 128 REP 2
   vsub VX(16,34++), V(16,1), 128 REP 2
   vsub VX(16,36++), V(16,2), 128 REP 2
   vsub VX(16,38++), V(16,3), 128 REP 2
   vsub VX(16,40++), V(16,4), 128 REP 2
   vsub VX(16,42++), V(16,5), 128 REP 2
   vsub VX(16,44++), V(16,6), 128 REP 2
   vsub VX(16,46++), V(16,7), 128 REP 2

   ; Apply scaling to Y and store 16-bit at HX(0..15,32).
   vsub HX(16++,0), H(0++,0), 16 REP 16
   vmulm HX(0++,32), HX(16++,0), 298 REP 16  ; (Y-16) * 1.164

   ; We are now ready to complete the conversion YCbCr -> RGB
   ; for CCIR601 inputs (limited excursion: includes scaling up)
   ; R = 1.164*(Y-16)                 + 1.596*(V-128) 
   ; G = 1.164*(Y-16) - 0.391*(U-128) - 0.781*(V-128)
   ; B = 1.164*(Y-16) + 2.018*(U-128)

   ; (U,V)->G contributions
   vmulm HX(0++,0), HX(16++,32), -100 REP 8
   vmulm HX(8++,0), HX(24++,32), -200 REP 8
   vadds HX(0++,0), HX(0++,0), HX(8++,0) REP 8

   ; U->B and V->R contributions
   vmulm HX(16++,0), HX(16++,32), 408 REP 8
   vmulm HX(24++,0), HX(24++,32), 516 REP 8

   mov r0, 896     ; 64 * 14
   mov r1, 448     ; 64 * 7
   mov r2, -64
yuv16_loop:
   vadds H(16++,48)+r0, HX(0++,32)+r0, HX(24,0)+r1 REP 2   ; red
   vadds H(16++,32)+r0, HX(0++,32)+r0, HX(16,0)+r1 REP 2   ; blue
   vadds H(0++,32)+r0, HX(0++,32)+r0, HX(0,0)+r1 REP 2     ; green
   addscale r0, r2 << 1
   addcmpbge r1, r2, 0, yuv16_loop

   vand H(16++,48), H(16++,48), 248 REP 16 ; Red in top 5 bits only
   vlsr H(16++,32), H(16++,32), 3 REP 16   ; Blue in bottom 5 bits only
   vlsr H(0++,32), H(0++,32), 2 REP 16
   vshl HX(0++,32), H(0++,32), 5 REP 16    ; Green in the middle
   b lr
   .stabs "vc_yuv16_common:F1",36,0,0,vc_yuv16_common

.FORGET

; vc_yuv16_transpose(unsigned char *Y, 
;       unsigned char *U, 
;       unsigned char *V, 
;       unsigned short *dest, int yspacing, int uvspacing, int destspacing)
;
; Copy a 16 by 16 block of data from Y,U,V (based on the given spacings)
; to the destination.  Spacings are given in bytes.
; This version applies a transposition.
; The yuv spacings can be given negative values in order to turn
; the transposition into a 90 degree rotation.

.section .text$vc_yuv16_transpose,"text"
.globl vc_yuv16_transpose       ; formerly vc_yuv16
vc_yuv16_transpose:
   stm lr,(--sp)
   vld     H(0++,0),(r0+=r4) REP 16   ; Y component
   vldleft H(16++,0),(r1+=r5) REP 8   ; U component
   vldleft H(24++,0),(r2+=r5) REP 8   ; V component

   bl vc_yuv16_common
   vor  HX(0++,0), HX(16++,32), HX(0++,32) REP 16

   ; Now store everything (transposed).
   ld      r4,(sp+4)
   vst     VX(0,0++),(r3+=r4) REP 16
   ldm pc,(sp++)
   .stabs "vc_yuv16_transpose:F1",36,0,0,vc_yuv16_transpose

.FORGET

; vc_yuv16_hflip(unsigned char *Y, 
;       unsigned char *U, 
;       unsigned char *V, 
;       unsigned short *dest, int yspacing, int uvspacing, int destspacing)
;
; Copy a 16 by 16 block of data from Y,U,V (based on the given spacings)
; to the destination.  Spacings are given in bytes.
; Does a horizontal flip.

.section .text$vc_yuv16_hflip,"text"
.globl vc_yuv16_hflip      ; formerly vc_yuv16_norot
vc_yuv16_hflip:
   stm lr,(--sp)
   vld     H(0++,0),(r0+=r4) REP 16      ; Y component
   vldleft H(16++,0),(r1+=r5) REP 8      ; U component
   vldleft H(24++,0), (r2+=r5) REP 8     ; V component
   bl vc_yuv16_common

   ; Combine colours back to front to achieve hflip
   vor   VX(0,0),  VX(16,47), VX(0,47)
   vor   VX(0,1),  VX(16,46), VX(0,46)
   vor   VX(0,2),  VX(16,45), VX(0,45)
   vor   VX(0,3),  VX(16,44), VX(0,44)
   vor   VX(0,4),  VX(16,43), VX(0,43)
   vor   VX(0,5),  VX(16,42), VX(0,42)
   vor   VX(0,6),  VX(16,41), VX(0,41)
   vor   VX(0,7),  VX(16,40), VX(0,40)
   vor   VX(0,8),  VX(16,39), VX(0,39)
   vor   VX(0,9),  VX(16,38), VX(0,38)
   vor   VX(0,10), VX(16,37), VX(0,37)
   vor   VX(0,11), VX(16,36), VX(0,36)
   vor   VX(0,12), VX(16,35), VX(0,35)
   vor   VX(0,13), VX(16,34), VX(0,34)
   vor   VX(0,14), VX(16,33), VX(0,33)
   vor   VX(0,15), VX(16,32), VX(0,32)

   ; Store everything
   ld      r4,(sp+4)
   vst     HX(0++,0),(r3+=r4) REP 16
   ldm pc,(sp++)
   .stabs "vc_yuv16_hflip:F1",36,0,0,vc_yuv16_hflip

.FORGET

; vc_yuv16_strip(unsigned char *Y, 
;       unsigned char *U, 
;       unsigned char *V, 
;       unsigned short *dest, int yspacing, int uvspacing, int destspacing,
;       int width)
;
; Copy a 16 high by a multiple of 16 wide strip of data from Y,U,V (based
; on the given spacings) to the destination. Spacings are given in bytes.
; No spatial transformation is applied.

.section .text$vc_yuv16_strip,"text"
.globl vc_yuv16_strip      ; no spatial transformation! n*16 strip
vc_yuv16_strip:
   stm r6-r11,lr,(--sp)
   mov r6, r0
   mov r7, r1
   mov r8, r2
   ld  r9, (sp+28)    ; destspacing
   ld  r10, (sp+32)   ; width
   mov r11, 16
   addcmpbne r10,r6,r6,vc_yuv16_strip_next ; r10 is now Y end stop
   ldm r6-r11,pc,(sp++)
vc_yuv16_strip_loop:
   addscale r3, r11 << 1
   add r7, 8
   add r8, 8
vc_yuv16_strip_next:
   mov r0, r6
   mov r1, r7
   mov r2, r8      
   vld     H(0++,0),(r0+=r4) REP 16   ; Y component
   vldleft H(16++,0),(r1+=r5) REP 8   ; U component
   vldleft H(24++,0),(r2+=r5) REP 8   ; V component
   bl vc_yuv16_common
   vor     HX(0++,0), HX(16++,32), HX(0++,32) REP 16

   ; Store everything
   mov r2, r9
   vst     HX(0++,0),(r3+=r2) REP 16

   ; And loop again
   addcmpblt r6,r11,r10,vc_yuv16_strip_loop   
   ldm r6-r11,pc,(sp++)
   .stabs "vc_yuv16_strip:F1",36,0,0,vc_yuv16_strip

.FORGET

;;; void vc_block_rgb2yuv(unsigned short *rgb_ptr, rgb_pitch_bytes,
;;;                       unsigned char *Y_ptr, unsigned char *U_ptr, unsigned char *V_ptr,
;;;                       int y_pitch_bytes);
;;;
;;; Convert a 16x16 block of 16-bit RGB values to YUV.
;;; Probably not the absolute tidiest way of doing it, but hopefully adequate.
;;; Most of the pain is actually to do with writing out the nastily aligned UV values.
      
.section .text$vc_block_rgb2yuv,"text"
.globl vc_block_rgb2yuv
vc_block_rgb2yuv:
    .cfa_bf vc_block_rgb2yuv
   stm r6-r7, lr, (--sp)
   .cfa_push {%lr,%r6,%r7}
    ;; First load the rgb data and convert into 8-bit sets of Rs, Gs and Bs.
   vld HX(0++,0), (r0+=r1) REP 16
   vshl H(0++,48), HX(0++,0), 3 REP 16    ; B
   vlsr H(0++,32), HX(0++,0), 3 REP 16
   vand H(0++,32), H(0++,32), 0xfc REP 16 ; G
   vand H(0++,16), H(0++,16), 0xf8 REP 16 ; R

   ;; coefficients changed to be consistent with inverse transformation of the
   ;; ITU-R.BT-601 transform VC normally uses; this may be wrong but at least is consistent

   ;; Make the Y component. Use HX(16++,32) as scratch memory.
   vmulhn.ss HX(16++,0), H(0++,16), 16843 REP 16   ; R contribution
   vmulhn.su HX(16++,32), H(0++,32), 33030 REP 16
   vadd HX(16++,0), HX(16++,0), HX(16++,32) REP 16 ; add G contribution
   vmulhn.ss HX(16++,32), H(0++,48), 6423 REP 16
   vadd HX(16++,0), HX(16++,0), HX(16++,32) REP 16 ; add B contribution
   vadds H(16++,0), HX(16++,0), 16 REP 16          ; final offset of 16
   
   ;; Make the U component.
   vmulhn.ss HX(32++,0), H(0++,16), -9699 REP 16   ; R contribution
   vmulhn.ss HX(16++,32), H(0++,32), -19070 REP 16
   vadd HX(32++,0), HX(32++,0), HX(16++,32) REP 16 ; add G contribution
   vmulhn.ss HX(16++,32), H(0++,48), 28770 REP 16
   vadd HX(32++,0), HX(32++,0), HX(16++,32) REP 16 ; add B contribution
   vadds H(32++,0), HX(32++,0), 128 REP 16         ; final offset of 128

   ;; Make the V component.
   vmulhn.ss HX(32++,32), H(0++,16), 28770 REP 16    ; R contribution
   vmulhn.ss HX(16++,32), H(0++,32), -24117 REP 16
   vadd HX(32++,32), HX(32++,32), HX(16++,32) REP 16 ; add G contribution
   vmulhn.ss HX(16++,32), H(0++,48), -4653 REP 16
   vadd HX(32++,32), HX(32++,32), HX(16++,32) REP 16 ; add B contribution
   vadds H(32++,32), HX(32++,32), 128 REP 16         ; final offset of 128
   
   ;; Now write out. Y is easy.
   vst H(16++,0), (r2+=r5) REP 16

   ;; The U and V must be averaged to make 2 8x8 blocks before output.
   ;; Note that r0, r1 and r2 are free now.
   mov r0, 0
   mov r1, 0
vloop:
   vadd VX(32,0)+r0, V(32,0)+r1, V(32,1)+r1
   vadd VX(32,32)+r0, V(32,32)+r1, V(32,33)+r1
   add r1, 2
   addcmpblt r0, 1, 8, vloop  

   mov r0, 0
   mov r1, 0
   mov r2, 512
   mov r6, 64
   mov r7, 128
hloop:
   vadd HX(32,0)+r0, HX(32,0)+r1, HX(33,0)+r1
   vadd HX(32,32)+r0, HX(32,32)+r1, HX(33,32)+r1
   add r1, r7
   addcmpblt r0, r6, r2, hloop
   
   vasr H(32++,0), HX(32++,0), 2 REP 8
   vasr H(32++,32), HX(32++,32), 2 REP 8

   ;; Now save the U and V. Pitch is half the Y. Serious complications
   ;; because the pitch may only be a multiple of 16, and we only have 8 U/V
   ;; values to write out in each row. Assume that the U/V addresses have
   ;; the same alignment.
   
   ;; Only r3, r4 and r5 have meaningful values at the moment.
   asr r5, 1
.if _VC_VERSION > 2
   vbitplanes -, 0xff SETF
   vst H(32++,0), (r3+=r5) REP 8  IFNZ ; U
   vst H(32++,32), (r4+=r5) REP 8 IFNZ ; V
   b the_end
.endif   
   mov r0, r5
   bmask r0, 4
   addcmpbne r0, 0, 0, not_aligned_16

   ;; The pitch is a multiple of 16. Good. But the 8 data values in each row
   ;; may be the first or second of each group of 16.
   mov r0, r3
   bmask r0, 4
   addcmpbne r0, 0, 0, second_8

   ;; The first 8 data values. In this case we don't need to preload the 2nd 8 values
   ;; as they'll get fixed up when we do the next block.
   vst H(32++,0), (r3+=r5) REP 8  ; U
   vst H(32++,32), (r4+=r5) REP 8 ; V
   b the_end

second_8:
   ;; Must first load up the data that is already there.
   vld H(32++,16), (r3+=r5) REP 8 ; U
   vld H(32++,48), (r4+=r5) REP 8 ; V
   ;; Now move the new values in.
   vmov V(32,24++), V(32,0++) REP 8  ; U
   vmov V(32,56++), V(32,32++) REP 8 ; V
   ;; Now write out.
   vst H(32++,16), (r3+=r5) REP 8 ; U
   vst H(32++,48), (r4+=r5) REP 8 ; V
   b the_end   

not_aligned_16:
   ;; The real nightmare case.
   ;; Must first load up the data that is already there.
   vld H(32++,16), (r3+=r5) REP 8 ; U
   vld H(32++,48), (r4+=r5) REP 8 ; V
   
   ;; Now move the new data in. Where it goes depends on the address.
   vbitplanes -, 0x00ff SETF  ; get ready to do moves of the first/second 8 values only
   mov r0, 0
   mov r1, 128
   mov r2, 512
   
   mov r6, r3
   bmask r6, 4
   addcmpbne r6, 0, 0, second_first
      
loop1:
   vadd H(32,16)+r0, H(32,0)+r0, 0 IFNZ  ; U
   vadd H(33,16)+r0, H(33,56)+r0, 0 IFZ
   vadd H(32,48)+r0, H(32,32)+r0, 0 IFNZ ; V
   vadd H(33,48)+r0, H(33,24)+r0, 0 IFZ
   addcmpblt r0, r1, r2, loop1
   
   ;; Write out and we're done.
   vst H(32++,16), (r3+=r5) REP 8 ; U
   vst H(32++,48), (r4+=r5) REP 8 ; V
   b the_end

second_first:
   vadd H(32,16)+r0, H(32,56)+r0, 0 IFZ ; U
   vadd H(33,16)+r0, H(33,0)+r0, 0 IFNZ
   vadd H(32,48)+r0, H(32,24)+r0, 0 IFZ ; V
   vadd H(33,48)+r0, H(33,32)+r0, 0 IFNZ
   addcmpblt r0, r1, r2, second_first
   
   ;; Write out and we're done.
   vst H(32++,16), (r3+=r5) REP 8 ; U
   vst H(32++,48), (r4+=r5) REP 8 ; V

the_end:
   ldm r6-r7, pc, (sp++)
   .cfa_pop {%r7,%r6,%pc}
    .cfa_ef
   
.FORGET
