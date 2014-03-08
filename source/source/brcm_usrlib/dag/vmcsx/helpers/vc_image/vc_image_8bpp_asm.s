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

.define CONDITIONAL_STORE_WORKS, 1
.define BILINEAR
.define Rdst, r0
.define Rsrc, r1
.define Rdstpitch, r2
.define Rsrcpitch, r3
.define Rwidth, r4
.define Rheight, r5
.define Rtemp, r6         ; general use by vector instructions
.define Rorigheight, r7
.define Rdstinc, r8
.define Rtemp2, r9
.define Rwidthinc, r9 ; shared with temp2

.define STACK_ARG_0, 16
.define STACK_ARG_1, 20
.define STACK_ARG_2, 24
.define STACK_ARG_3, 28
.define STACK_ARG_4, 32
.define STACK_ARG_5, 36
.define STACK_ARG_6, 40
.define STACK_ARG_7, 44
.define STACK_ARG_8, 48

.define N_src, 0
.define N_src1, 1
.define N_src2, 2
.define N_src3, 3
.define N_trans, 4
.define N_dst, 5
.define N_working, 6
.define N_frac0, 8
.define N_frac1, 9
.define N_frac2, 10
.define N_frac3, 11
.define N_cx_i, 12
.define N_cy_i, 13

.define N_incxx, 14
.define N_incxy, 15
.define N_incyx, 16
.define N_incyy, 17
.define N_incxx_16, 18
.define N_incxy_16, 19
.define N_incxx_0to15, 20
.define N_incxy_0to15, 21

.define N_src_width, 22
.define N_src_height, 23
.define N_0to15, 24

.define N_cx, 25
.define N_cy, 26

.define N_frac, 27
.define N_ifrac, 28

;void vc_roz_blt_8bpp_to_8bpp(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
;   int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color)

.function vc_roz_blt_8bpp_to_8bpp ; {{{
   stm r6-r9, (--sp)
   .cfa_push {r6,r7,r8,r9}
   ld Rtemp, (sp+STACK_ARG_0)
.ifdef BILINEAR
   vmov HX(N_src_width, 0), Rtemp-2
.else
   vmov HX(N_src_width, 0), Rtemp-1
.endif
   ld Rtemp, (sp+STACK_ARG_1)
.ifdef BILINEAR
   vmov HX(N_src_height, 0), Rtemp-2
.else
   vmov HX(N_src_height, 0), Rtemp-1
.endif

   ld Rtemp, (sp+STACK_ARG_2)
   VMOV_H32 N_incxx, Rtemp

   ld Rtemp, (sp+STACK_ARG_3)
   VMOV_H32 N_incxy, Rtemp

   ld Rtemp, (sp+STACK_ARG_4)
   VMOV_H32 N_incyx, Rtemp

   ld Rtemp, (sp+STACK_ARG_5)
   VMOV_H32 N_incyy, Rtemp

   ld Rtemp, (sp+STACK_ARG_6)
   VMOV_H32 N_cx, Rtemp

   ld Rtemp, (sp+STACK_ARG_7)
   VMOV_H32 N_cy, Rtemp

   ld Rtemp, (sp+STACK_ARG_8)
   vmov HX(N_trans, 0), Rtemp

   ld Rtemp,  (sp+STACK_ARG_2) ; incxx
   ld Rtemp2, (sp+STACK_ARG_4) ; incyx
   mul Rtemp2, Rheight
   shl Rtemp, 4
   sub Rtemp, Rtemp2           ; 16*incxx-height*incyx
   VMOV_H32 N_incxx_16, Rtemp

   ld Rtemp,  (sp+STACK_ARG_3) ; incxy
   ld Rtemp2, (sp+STACK_ARG_5) ; incyy
   mul Rtemp2, Rheight
   shl Rtemp, 4
   sub Rtemp, Rtemp2           ; 16*incxy-height*incyy
   VMOV_H32 N_incxy_16, Rtemp

   add Rtemp, pc, pcrel(vc_image_const_0_15)
   vld H(N_0to15, 0), (Rtemp)

   mov Rorigheight, Rheight
   mul Rdstinc, Rheight, Rdstpitch
   rsub Rdstinc, 16
   mov Rwidthinc, -16

   vmul      HX(N_incxx_0to15, 0), HX(N_incxx, 0), H(N_0to15, 0) CLRA UACC
   vmulhd.uu -,                       HX(N_incxx, 0), H(N_0to15, 0) ACC
   vmul      HX(N_incxx_0to15, 32), HX(N_incxx, 32), H(N_0to15, 0) ACC

   vmul      HX(N_incxy_0to15, 0), HX(N_incxy, 0), H(N_0to15, 0) CLRA UACC
   vmulhd.uu -,                       HX(N_incxy, 0), H(N_0to15, 0) ACC
   vmul      HX(N_incxy_0to15, 32), HX(N_incxy, 32), H(N_0to15, 0) ACC
   
   VADD_H32 N_cx, N_cx, N_incxx_0to15
   VADD_H32 N_cy, N_cy, N_incxy_0to15

;   int dst_inc = dst_pitch-dst_width;
;   int i,j;
;   incyx -= incxx*dst_width;
;   incyy -= incxy*dst_width;
;	for (i=0; i<dst_height; i++)
;	{
;	   for (j=0; j<dst_width; j++)
;	   {
;		   if ((cx>>16) < src_width && (cy>>16) < src_height)
;		   {
;			   int c = src_bitmap[(cy>>16)*src_pitch+(cx>>16)];
;			   if (c != transparent_color)
;				   *dst_bitmap = c;
;		   }
;		   cx += incxx;
;		   cy += incxy;
;		   dst_bitmap++;
;	   }
;     cx += incyx;
;     cy += incyy;
;     dst_bitmap += dst_inc;
;   }

1:
; check for clipping with source limits
   vrsub -, HX(N_cx, 32), HX(N_src_width, 0)  SETF
   vsub  -, HX(N_cx, 32), 0                   SETF IFNN
   vrsub -, HX(N_cy, 32), HX(N_src_height, 0) SETF IFNN
   vsub  -, HX(N_cy, 32), 0                   SETF IFNN

; accumulate x & y index
   vmul      -, HX(N_cy, 32), Rsrcpitch CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), Rsrcpitch ACC
   vmov      -, HX(N_cx, 32)            UACC

; lookup pixel
   vlookupm  H(N_src,0), (Rsrc+0) UACC IFNN

.ifdef BILINEAR
; lookup pixels
   add Rtemp, Rsrc, Rsrcpitch
   vlookupm  H(N_src1,0), (Rsrc+1)  UACC IFNN
   vlookupm  H(N_src2,0), (Rtemp+0) UACC IFNN
   vlookupm  H(N_src3,0), (Rtemp+1) UACC IFNN
   vshl      HX(N_src++, 0), H(N_src++, 0), 6 REP 4
; fractional bits
   veor HX(N_cx_i, 0), HX(N_cx, 0), 0xffff
   veor HX(N_cy_i, 0), HX(N_cy, 0), 0xffff

   vmulhd.uu HX(N_frac0, 0), HX(N_cx_i, 0), HX(N_cy_i, 0)
   vmulhd.uu HX(N_frac1, 0), HX(N_cx, 0), HX(N_cy_i, 0)
   vmulhd.uu HX(N_frac2, 0), HX(N_cx_i, 0), HX(N_cy, 0)
   vmulhd.uu HX(N_frac3, 0), HX(N_cx, 0), HX(N_cy, 0)

   vmulhd.uu -, HX(N_src, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_src1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_src2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_src, 0), HX(N_src3, 0), HX(N_frac3, 0) UACC
   vmulhn.uu H(N_src, 0), HX(N_src, 0), 1<<(16-6)
.endif

; check for transparency
   vdist     HX(N_working,0), H(N_src,0), HX(N_trans,0)
   vsub      -, HX(N_working,0), 1 SETF IFNN

; conditionally store
.if CONDITIONAL_STORE_WORKS
   vst  H(N_src,0), (Rdst) IFNN
.else
   vld  H(N_working,0), (Rdst)
   vmov H(N_working,0), H(N_src,0) IFNN
   vst  H(N_working,0), (Rdst)
.endif
   add Rdst, Rdstpitch

; inc source x and y
   VADD_H32 N_cx, N_cx, N_incyx
   VADD_H32 N_cy, N_cy, N_incyy
   
   addcmpbgt Rheight, -1, 0, 1b

; inc source x and y
   VADD_H32 N_cx, N_cx, N_incxx_16
   VADD_H32 N_cy, N_cy, N_incxy_16

   add Rdst, Rdstinc
   mov Rheight, Rorigheight
   addcmpbgt Rwidth, Rwidthinc, 0, 1b

   ldm r6-r9, (sp++)
   b lr
   .cfa_pop {r9,r8,r7,r6,pc}
.endfn ; }}}

.FORGET

;;;    vc_8bpp_font_alpha_blt
;;;
;;; SYNOPSIS
;;;    void vc_8bpp_font_alpha_blt(unsigned short *dest (r0), unsigned short *src (r1), int dest_pitch_bytes (r2),
;;;                                int src_pitch_bytes (r3), int width (r4), int height(r5), int colour, int alpha, int antialias_flag)
;;;
;;; FUNCTION
;;;    Transparent blt from a special "font" image to 8BPP, Greyscale or Y.
;;;    (antialias_flag should be set for Greyscale or Y and clear for Palette)
;;;
;;; RETURNS
;;;    -

.section .text$vc_8bpp_font_alpha_blt,"text"
.globl vc_8bpp_font_alpha_blt
vc_8bpp_font_alpha_blt:
      stm r6-r8, lr, (--sp)

      ld r6, (sp+16)       ; colour
      vmov HX(62,0), r6
      ld r6, (sp+20)       ; alpha
      mul r6, 17
      vmov HX(63,0), r6
      
      ; We want to generate an alpha mask in HX(0,0) (allow for up to 256 rows in the image)
      ; Need to extend to the left and right with zeros in order to reach an aligned blit
      mov r6,r0
      bmask r6,4  ; r6 is the low 4 bits of the address
      mov r7,r6   ; save r6 in r7
      mov r6,0xffff
      shl r6,r7   ; put zeros into low bits of r6
      vbitplanes -,r6 SETF   ; Now the flags are zero for the bits at the beginning
      vmov HX(0++,0),HX(63,0) REP 16   ; Fill up the block with the alpha value
      vmov HX(0,0),0 IFZ               ; Fix the beginning     
      add r4,r7      ; Width is now extended
      sub r1,r7      ; Source is now matched to destination (source is only 8 bit wide)  
      sub r0,r7      ; Destination is now aligned
      add r7,r4,-1   ; width rounded down
      asr r7,4       ; r7 now contains the number of blocks across to apply (-1), 0 for a single block
      ; Now we want to compute the space at the end
      neg r4,r4  
      bmask r4,4     ; r4 is now 0 for aligned blocks, up to 15 for addresses just hitting the next block
      mov r6,0xffff
      lsr r6,r4
      vbitplanes -,r6 SETF ; Flags are Z for blocks past the end
      mov r4,r7
      shl r7,4       ; r7 is now the number of bytes added per row
      shl r4,6       ; r4 is now the VRF offset to the last row
      vmov HX(0,0)+r4,0 IFZ   ; Clear the alpha mask for the end of the row
      
      sub r2,r7   ; Correct pitches to take into account addition per row
      sub r3,r7
      sub r2,16
      sub r3,16

      ld r6, (sp+24)            ; antialias flag
      vmov -, r6 SETF
        
      ; We now have r6 spare, and can process all the rows
      ; Use r6 to index through the rows, with an upper limit of r4
      ; Use r8 as a loop increment
      mov r8,64
loopdown:
      mov r6,0
loopacross:

      vld H(16,0),(r0)        ; Destination
      vldleft H(17,0),(r1)    ; Source
;      vldright H(17,0),(r1)   ; in 4.4 (alpha/gray) format
      
      ; Compute actual alpha value
      vlsr     H(18,0),  H(17,0), 4       ; pick out top 4 bits
      vmulms   HX(18,0), H(18,0), HX(0,0)+r6
      vtestmag HX(18,0), HX(18,0), 96 IFZ
      vshl     HX(18,0), HX(18,0), 8 IFZ  ; adjust for "no antialiasing" case

      ; Apply alpha blend (ignore the "gray" component and use given colour)
      vsub     HX(18,32), H(62,0),   H(16,0)
      vmulm    HX(18,32), HX(18,32), HX(18,0)
      vadds    H(18,32),  HX(18,32), H(16,0) ; composited colour
      
      vst H(18,32),(r0)
      add r0,16
      add r1,16
            
      addcmpble r6,r8,r4,loopacross    
      add r0,r2
      add r1,r3
      addcmpbgt r5,-1,0,loopdown
      
      ldm r6-r8, pc, (sp++)
.stabs "vc_8bpp_font_alpha_blt:F1",36,0,0,vc_8bpp_font_alpha_blt

