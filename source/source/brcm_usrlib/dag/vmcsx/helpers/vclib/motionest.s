;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.if _VC_VERSION <= 2
.FORGET

; int vc_dist(pic1,pic2,pitch)
; Returns a measure of similarity between 16 by 16 blocks between pic1 and pic2
; pic2 is assumed aligned but pic1 may not be aligned

.section .text$vc_dist  ,"text"
.globl vc_dist  
vc_dist:
   vld   H(0++,0),(r0+=r2) REP 16
   vld   H(0++,16),(r0+16+=r2) REP 16
   bmask r0,4
   vld   H(0++,32),(r1+=r2) REP 16
   vdist -,H(0++,0)+r0,H(0++,32) CLRA ACC REP 16 SUM r0
   b  lr
   
; This routine is let down by a bug in vldleft and vldright    
vc_dist2:
   vldleft   H(0++,0),(r0+=r2) REP 16
   vldright   H(0++,0),(r0+=r2) REP 16
   vld   H(0++,16),(r1+=r2) REP 16
   vdist -,H(0++,0),H(0++,16) CLRA ACC REP 16 SUM r0
   b  lr

        
; Motion estimation for a *strip* whose height is 16 and whose width
; is a multiple of 16. Returns the total absolute difference in r0
; unsigned int vc_dist_strip(unaligned, aligned, width, pitch)
.section .text$vc_dist_strip,"text"
.globl vc_dist_strip
vc_dist_strip:
        stm r6-r7,lr,(--sp)
        mov r4, r0
        bmask r4, 4
        vld H(0++,0)*,(r0+=r3) REP 16
        mov r6, 0
        mov r7, 16
        add r2, r1
vc_dist_strip_loop:
        vld H(0++,16)*,(r0+16+=r3) REP 16
        vld H(32++,0),(r1+=r3) REP 16
        add r0, r7
        vdist -,H(0++,0)+r4*,H(32++,0) CLRA ACC REP 16 SUM r5
        add r6, r5
        cbadd1
        addcmpblo r1, r7, r2, vc_dist_strip_loop
        mov r0, r6
        ldm r6-r7,pc,(sp++)


; void vc_softupdate_strip(dest, src, width, pitch) 
.section .text$vc_softupdate_strip,"text"
.globl vc_softupdate_strip
vc_softupdate_strip:
        addcmpbne r2, r0, r0, vc_su_nonz
        b lr
vc_su_nonz:
        mov r4, 16
vc_su_loop:
        vldleft H(16++,0), (r1+=r3) REP 16
        vldright H(16++,0), (r1+=r3) REP 16
        vld H(0++,0), (r0+=r3) REP 16
        add r1, r4
        vsub HX(32++,0), H(16++,0), H(0++, 0) REP 16
        vasr HX(16++,0), HX(32++,0), 3 REP 16
        vadds H(0++,0), H(0++,0), HX(16++,0) REP 16
        vst H(0++,0), (r0+=r3) REP 16
        addcmpblo r0, r4, r2, vc_su_loop
        b lr


;bestscore=vc_motionest(&pic1[y*16*width+x*16],&pic2[y*16*width+x*16]
;    ,&motvectorx[y*numx+x],&motvectory[y*numx+x],width);
; Computes the best match for the given block 
.section .text$vc_motionest,"text"
.globl vc_motionest
vc_motionest:
   stm   r6-r8,lr,(--sp)
   ; we need to go back 16 rows
   mov   r5,r4
   mul   r5,16
   sub   r0,r5
   
; pic1 is the search block 48 by 48
   vld   H(0++,0),(r0+-16+=r4) REP 48
   vld   H(0++,16),(r0+=r4) REP 48
   vld   H(0++,32),(r0+16+=r4) REP 48

   vld   H(0++,48),(r1+=r4) REP 16
; now search across

; We use r0 for the best score so far, init to the 0,0 window
   vdist -, H(16++,16), H(0++,48) CLRA ACC REP 16 SUM r0

; Don't worry about early termination at the moment
   mov   r1,0  
   st r1,(r2)
   st r1,(r3)
   
; use r6 for dx
; use r7 for dy
   mov   r7,-16
yloop:
   mov   r8,r7
   shl   r8,6  // Times 64 to turn offsets into VRF locations
   mov   r6,-16
xloop:
; Calculate an offset
   add   r5,r6,r8
   vdist -, H(16++,16)+r5, H(0++,48) CLRA ACC REP 16 SUM r1
   addcmpbgt   r1,0,r0,nobetter
; Update motion vectors and best score
   mov   r0,r1
   st r6,(r2)
   st r7,(r3)
nobetter:
   addcmpblt   r6,1,16,xloop
   addcmpblt   r7,1,16,yloop
; r0 contains the best score
   ldm   r6-r8,pc,(sp++)
.endif
