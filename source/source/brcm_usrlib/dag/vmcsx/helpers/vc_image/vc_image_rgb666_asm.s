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
;.stabs "asm_file",100,0,0,vc_image_rgb666_asm

; void vc_palette8_to_rgb666( unsigned char *dest (r0),unsigned char *src (r1),int destpitch (r2),int srcpitch (r3),int width (r4),short *cmap (r5));

.section .text$vc_palette8_to_rgb666,"text"
.globl vc_palette8_to_rgb666
vc_palette8_to_rgb666:
   stm   r6-r7,lr,(--sp)
   
   mov r7,r4   ; count down value for width
copyloop:

   ; First load in a block of 16 by 16 pixels    
   vldleft   H(0++,0),(r1+=r3) REP 16
;   vldright H(0++,0),(r1+=r3) REP 16
   
   ; Now lookup all the values
   mov r6,0
   mov r4,16
loopconvert:
   ;; Palette conversion to 16-bit RGB
   vmov HX(0,0)+r6,H(0,0)+r6 CLRA ACC
   vlookupm HX(16,0)+r6,(r5)
   add r6,64
   addcmpbgt r4,-1,0,loopconvert
   vmov HX(0++,0),HX(16++,0) REP 16
   
   ; Now convert from rgb565 to rgb666 format
   mov r4,0 ; offset for src (rgb565)
   mov r6,0 ; offset for dest (rgb666)
   
loop1:
   ;; Top two bits red preserved
   vlsr VX(32,1)+r6,VX(16,0)+r4,14

   ;; Remaining red bits
   vshl VX(32,0)+r6,VX(16,0)+r4,2
   vand -,VX(32,0)+r6,0xF000 CLRA ACC  
   
   ;; Shift up green, blue
   vshl VX(32,0)+r6,VX(16,0)+r4,1
   vand -,VX(32,0)+r6,0x0FFF ACC

   add r4,1
   add r6,2
   cmp r6,32
   blt loop1
   
   ;; Write out.
   mov r6, 32
   vst HX(32++,0), (r0+=r2) REP 16
   vst HX(32++,32), (r0+32+=r2) REP 16

; Update pointers and loop 
   sub r7,16
   add r0,64
   add r1,16
   cmp r7,0
   bgt copyloop
   
   ldm   r6-r7,pc,(sp++)

.stabs "vc_palette8_to_rgb666:F19",36,0,1,vc_palette8_to_rgb666


.FORGET


; void vc_rgb565_to_rgb666( unsigned char *dest (r0),unsigned char *src (r1),int destpitch (r2),int srcpitch (r3),int width (r4),short *cmap (r5));

.section .text$vc_rgb565_to_rgb666,"text"
.globl vc_rgb565_to_rgb666
vc_rgb565_to_rgb666:
   stm   r6-r7,lr,(--sp)
   
   mov r7,r4   ; count down value for width
copyloop:

   ; First load in a block of 16 by 16 pixels    
   vld   HX(16++,0),(r1+=r3) REP 16
      
   ; Now convert from rgb565 to rgb666 format
   mov r4,0 ; offset for src (rgb565)
   mov r6,0 ; offset for dest (rgb666)
   
loop1:
   ;; Top two bits red preserved
   vlsr VX(32,1)+r6,VX(16,0)+r4,14

   ;; Remaining red bits
   vshl VX(32,0)+r6,VX(16,0)+r4,2
   vand -,VX(32,0)+r6,0xF000 CLRA ACC  
   
   ;; Shift up green, blue
   vshl VX(32,0)+r6,VX(16,0)+r4,1
   vand -,VX(32,0)+r6,0x0FFF ACC

   add r4,1
   add r6,2
   cmp r6,32
   blt loop1
   
   ;; Finally write out.
   mov r6, 32
   vst HX(32++,0), (r0+=r2) REP 16
   vst HX(32++,32), (r0+32+=r2) REP 16

; Update pointers and loop 
   sub r7,16
   add r0,64
   add r1,32
   cmp r7,0
   bgt copyloop
   
   ldm   r6-r7,pc,(sp++)

.stabs "vc_rgb565_to_rgb666:F19",36,0,1,vc_rgb565_to_rgb666

.FORGET


   
; void vc_rgb888_to_rgb666( unsigned char *dest (r0),unsigned char *src (r1),int destpitch (r2),int srcpitch (r3),int width (r4),short *cmap (r5));

.section .text$vc_rgb888_to_rgb666,"text"
.globl vc_rgb888_to_rgb666
vc_rgb888_to_rgb666:

   stm r6-r7,lr,(--sp)
   mov r7, r4
   
copyloop:
   ;; Load up 16x16 block of contiguous RGB888 pixels.
   vld H(0++,0), (r1+=r3) REP 16
   vld H(0++,16), (r1+16+=r3) REP 16
   vld H(0++,32), (r1+32+=r3) REP 16
   ;; Now loop through the columns doing the processing.
   mov r6, 0      ; offset for dest
   mov r4, 0      ; offset for src

loop1:
.ifdef RGB888
   ;; Top two bits of red into top word
   vlsr VX(16,1)+r6,V(0,0)+r4,6

   ;; Bottom 4 bits of red
   vshl VX(16,0)+r6,V(0,0)+r4,10
   vand -,VX(16,0)+r6,0xF000 CLRA ACC

   ;; 6 bits green
   vshl VX(16,0)+r6,V(0,1)+r4,4
   vand VX(16,0)+r6,VX(16,0)+r6,0x0FC0 ACC

   ;; 6 bits blue
   vlsr VX(16,0)+r6,V(0,2)+r4,2
   vand VX(16,0)+r6,VX(16,0)+r6,0x003F ACC
.else
   ;; Top two bits of red into top word
   vlsr VX(16,1)+r6,V(0,2)+r4,6

   ;; Bottom 4 bits of red
   vshl VX(16,0)+r6,V(0,2)+r4,10
   vand -,VX(16,0)+r6,0xF000 CLRA ACC

   ;; 6 bits green
   vshl VX(16,0)+r6,V(0,1)+r4,4
   vand VX(16,0)+r6,VX(16,0)+r6,0x0FC0 ACC

   ;; 6 bits blue
   vlsr VX(16,0)+r6,V(0,0)+r4,2
   vand VX(16,0)+r6,VX(16,0)+r6,0x003F ACC
.endif

   add r4, 3
   add r6, 2
   cmp r6, 32
   blt loop1

   ;; Write out
   vst HX(16++,0), (r0+=r2) REP 16
   vst HX(16++,32), (r0+32+=r2) REP 16

   ;; Outer loop
   sub r7, 16
   add r0, 64
   add r1, 48
   cmp r7, 0
   bgt copyloop

   ldm r6-r7,pc,(sp++)
   
.stabs "vc_rgb888_to_rgb666:F19",36,0,1,vc_rgb888_to_rgb666


.FORGET
