;;; ===========================================================================
;;; Copyright (c) 2005-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================


;;; void vc_image_atext_conv(unsigned char * ptr, int nbytes)
;;; 
;;; Counts the bits set in each byte and converts this to a form
;;; suitable for use with vc_image_font_alpha_blit().
;;; We use 256-bit wide memory access; the pointer must be aligned.

.section .text$vc_image_atext_conv,"text"
.globl vc_image_atext_conv
        ;; r0 src/dest pointer (32-byte aligned)
        ;; r1 number of bytes (multiple of 32)
        ;; r2 offset (hack in lieu of gamma)
vc_image_atext_conv:
        add r1, r0
        mov r4, 32
convloop:
.if _VC_VERSION > 2
        vld    HX(0,0), (r0)
        vmov   -, HX(0,0) USUM r3
.else
        vld    HX(0,0), (r0) USUM r3
.endif
        vcount H(0,0), H(0,0), 0
        vcount H(0,16), H(0,16), 0
        addcmpbeq r3, 0, 0, convskip
        vshl  HX(1,0), H(0,0),  5 SETF
        vadds H(0,0),  HX(1,0), r2 IFNZ
        vshls HX(1,0), H(0,16), 5 SETF
        vadds H(0,16), HX(1,0), r2 IFNZ
        vst   HX(0,0), (r0)
convskip:
        addcmpblo r0, r4, r1, convloop
        b lr



;;; void atext_strip(int * ptr, int x0, int x1, int mask)
;;;
;;; Draw a "strip" in a bitmap image, where each byte represents 4x2
;;; "sub-pixels", thus:
;;;                     b0 b1 b2 b3
;;;                     b4 b5 b6 b7
;;; Depending on the value of mask, draws to upper or lower half or both.
;;; For efficiency we use 32-bit accesses, so ptr must be 4-byte aligned.
;;; There is no bounds checking!

.section .text$vc_image_atext_strip,"text"
.globl vc_image_atext_strip
        ;; r0 points to line (must be 4-byte aligned)
        ;; r1 x0
        ;; r2 x1
        ;; r3 mask (0x0f0f0f0f for upper half; 0xf0f0f0f0 for lower half)
vc_image_atext_strip:
        .cfa_bf vc_image_atext_strip
        stm r6-r7,(--sp)
        .cfa_push {%r6,%r7}
        add r6, pc, pcrel(strip_table)
        bmask r4, r1, 4         ; r4:frac0 (0 to 15)
        bmask r5, r2, 4         ; r5:frac1 (0 to 15)
        asr r1, 4               ; r1:x0    (in 4-byte (16-pixel) units)
        asr r2, 4               ; r2:x1    (in 4-byte (16-pixel) units)
        addcmpbeq r4, 0, 0, donefrac0
        ld  r4, (r6+r4<<2)
        addcmpblo r1, 0, r2, x0lox1
        ld  r5, (r6+r5<<2)
        and r4, r3
        ld  r7, (r0+r1<<2)
        bic r4, r5
        or  r7, r4
        st  r7, (r0+r1<<2)
        b donefrac1
x0lox1:
        ld  r7, (r0+r1<<2)
        and r4, r3
        or  r7, r4
        st  r7, (r0+r1<<2)
        add r1, 1
donefrac0:
        addcmpbhs r1, 0, r2, doneloop
loop:
        ld  r7, (r0+r1<<2)
        or  r7, r3
        st  r7, (r0+r1<<2)
        addcmpblo r1, 1, r2, loop
doneloop:
        addcmpbeq r5, 0, 0, donefrac1
        ld  r5, (r6+r5<<2)
        bic r5, r3, r5
        ld  r7, (r0+r2<<2) 
        or  r7, r5
        st  r7, (r0+r2<<2)
donefrac1:
        ldm r6-r7,(sp++)
        .cfa_pop {%r7,%r6}
        b lr

        .align 4
strip_table:
        .word 0xffffffff
        .word 0xffffffee
        .word 0xffffffcc
        .word 0xffffff88
        .word 0xffffff00
        .word 0xffffee00
        .word 0xffffcc00
        .word 0xffff8800
        .word 0xffff0000
        .word 0xffee0000
        .word 0xffcc0000
        .word 0xff880000
        .word 0xff000000
        .word 0xee000000
        .word 0xcc000000
        .word 0x88000000

        .cfa_ef



;;; void vc_image_atext_rect(void * ptr, int pitch,
;;;                          int x, int y, int w, int h)
;;; 
;;; Draw a rectangle. Coordinates are in units of "big pixels" scaled
;;; by 256; Each "big pixel" is divided into 4x2 "sub-pixels"

.section .text$vc_image_atext_rect,"text"
.globl vc_image_atext_rect
        ;; r0 ptr (must be 4-byte aligned)
        ;; r1 pitch in bytes (must be multiple of 4)
        ;; r2 x (scaled by 256)
        ;; r3 y (scaled by 256)
        ;; r4 w (scaled by 256)
        ;; r5 h (scaled by 256)
vc_image_atext_rect:
        .cfa_bf vc_image_atext_rect
        stm r6-r11,lr,(--sp)
        .cfa_push {%lr,%r6,%r7,%r8,%r9,%r10,%r11}
        add r7, r2, 32          ; r7:x0
        add r3, 64
        asr r8, r3, 8
        mul r8, r1        
        add r6, r0, r8          ; r6:ptr (offset for y)
        bmask r3, 8             ; r3:y0 (fractional part)
        add r8, r7, r4          ; r8:x1
        btest r3, 7
        add r9, r5, r3          ; r9:y1
        mov r10, r1             ; r10:pitch
        beq ynofrac
        mov r0, r6
        asr r1, r7, 6
        asr r2, r8, 6
        mov r3, 0xf0f0f0f0
        bl  vc_image_atext_strip
        add r6, r10
        add r9, -256
ynofrac:
        asr r11, r9, 8
        addcmpble r11, 0, 0, nomainloop
mainloop:
        mov r0, r6
        asr r1, r7, 6
        asr r2, r8, 6
        mov r3, -1
        bl  vc_image_atext_strip
        add r6, r10
        addcmpbgt r11, -1, 0, mainloop
nomainloop:
        btest r9, 7
        mov r0, r6
        asr r1, r7, 6
        asr r2, r8, 6
        beq done
        mov r3, 0x0f0f0f0f
        bl  vc_image_atext_strip
done:
        ldm r6-r11,pc,(sp++)
        .cfa_pop {%r11,%r10,%r9,%r8,%r7,%r6,%pc}
        .cfa_ef


