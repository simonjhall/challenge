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

;;;
;;; NAME
;;; vclib_medianfilter
;;;
;;; SYNOPSIS
;;;  void vclib_medianfilter(unsigned char *src, unsigned char *dest, int width, int height)
;;;
;;; FUNCTION
;;;  Median filters an entire image of size width*height, identified by the src
;;;  argument, and writes the resulting image to dest. Does not quite work in place,
;;;  but so long as the memory has been allocated accordingly, it should work if
;;;  dest is set to src - width (so the extra large image buffer can be saved).
;;;
;;;   Both width and height should be divisible by 16.
;;;
;;; RETURNS
;;;    Nothing.
;;;
      
.section .text$vclib_medianfilter,"text"
.globl vclib_medianfilter
vclib_medianfilter:
      ;; Arguments:  
      ;; r0 - source address
      ;; r1 - destination address
      ;; r2 - image width
      ;; r3 - image height
      stm r6-r9, lr, (--sp)
      
      mov r4, 15
      mov r8, r2
      mul r8, r4           ; r8 - 15*image width
      mov r9, r8
      add r9, r2           ; r9 - 16*image width
      mov r4, r3
      asr r4, 4            ; r4 - image height in MBs
      mov r5, r4           ; r5 - counter for vertical loop
      mov r6, r2
      asr r6, 4            ; r6 - image width in MBs
                        ; r7 - horizontal loop counter

      ;; In what follows, the 16 rows of 16 pixels for which we compute
      ;; the medians are in H1A* to H16A*. We also keep the row of pixels
      ;; above in H0A*, and the row below in H17A*. Additionally, we
      ;; maintain in H0C* to H17C* the 16-pixel wide block of pixels
      ;; to the left of the pixels we are interested in, and in H0B* to
      ;; H17B* the ones to the right, using the circular buffer mechanism
      ;; to cylce through them.

loop_rows:
      cbclr

      ;; Fill H0C to H17C with zeroes. These are the pixels to the left
      ;; of those we are interested in.
      vmov H(0++,48), 0  REP 16
      vmov H(16++,48), 0  REP 2

      ;; Load up H0A to H17A. This is the first lot of pixels for which
      ;; we will compute medians. For the first (last) row of the image,
      ;; we fill H0A (H17A) with the same as H1A (H16A). This is a cheat,
      ;; but gives us a slightly less dubious median than filling with,
      ;; say, zeroes would have done.

      vld H(1++,0), (r0+=r2) REP 16
      vmov H(0,0), H(1,0)
      vmov H(17,0), H(16,0)
      cmp r5, r4
      beq first_row
      sub r0, r2
      vld H(0,0), (r0)
      add r0, r2
first_row:
      addcmpbeq r5, 0, 1, last_row
      add r0, r9
      vld H(17,0), (r0)
      sub r0, r9
last_row:

      ;; Loop over all the columns, again 16 at a time.

      mov r7, r6           ; horizontal loop counter
loop_cols:
      ;; First thing is to fetch all the pixels to the right of the
      ;; "A" pixels. For the last iteration round this loop we fill
      ;; them with zeroes, otherwise from the image memory. For the
      ;; first/last rows of the image, we perform the same cheat as above.
      addcmpbne r7, 0, 1, not_last_col
      vmov H(0++,16)*, 0 REP 16
      vmov H(16++,16)*, 0 REP 2
      b main_body_of_loop
not_last_col:  
      ;; Load up H0B* to H17B* from the image.
      vld H(1++,16)*, (r0+16+=r2) REP 16
      vmov H(0,16)*, H(1,16)*
      vmov H(17,16)*, H(16,16)*
      cmp r5, r4
      beq first_row2
      sub r0, r2
      vld H(0,16)*, (r0+16)
      add r0, r2
first_row2:
      addcmpbeq r5, 0, 1, main_body_of_loop
      add r0, r9
      vld H(17,16)*, (r0+16)
      sub r0, r9

      ;; Here we compute 256 median values for the block of 16x16 pixels
      ;; in H1A* to H16A*.

main_body_of_loop:
      ;; Here we want the max, median and min of H(0++,)A[-1,0,1]*
      ;; in, respectively, H(18++,0), H(18++,16), H(18++,32)
      vmax H(18++,16), H(0++,1)*, H(0++,0)*  REP 16
      vmax H(34++,16), H(16++,1)*, H(16++,0)*  REP 2
      vmin H(36++,0), H(0++,1)*, H(0++,0)*  REP 16
      vmin H(52++,0), H(16++,1)*, H(16++,0)*  REP 2
      vmax H(18++,0), H(0++,63)*, H(18++,16)  REP 16
      vmax H(34++,0), H(16++,63)*, H(34++,16)  REP 2
      vmin H(18++,32), H(0++,63)*, H(18++,16)  REP 16
      vmin H(34++,32), H(16++,63)*, H(34++,16)  REP 2
      vmax H(18++,16), H(18++,32), H(36++,0)  REP 16
      vmax H(34++,16), H(34++,32), H(52++,0)  REP 2
      vmin H(18++,32), H(18++,32), H(36++,0)  REP 16
      vmin H(34++,32), H(34++,32), H(52++,0)  REP 2

      ;; Here we want the median only of H(18++,16), H(19++,16), H(20++,16)
      vmax H(36++,0), H(19++,16), H(20++,16)  REP 16        ;; 5,8
      vmin H(19++,16), H(19++,16), H(20++,16)  REP 16
      vmax H(18++,16), H(18++,16), H(36++,0)  REP 16        ;; 2,5
      vmin H(18++,16), H(18++,16), H(19++,16)  REP 16       ;; 5,8

      ;; Here we want the min only of H(18++,0), H(19++,0), H(20++,0)
      vmin H(18++,0), H(18++,0), H(19++,0)  REP 16       ;; 1,4
      vmin H(18++,0), H(20++,0), H(18++,0)  REP 16       ;; 4,7

      ;; Here we want the max only of H(18++,32), H(19++,32), H(20++,32)
      vmax H(19++,32), H(19++,32), H(20++,32)  REP 16       ;; 6,9
      vmax H(18++,32), H(18++,32), H(19++,32)  REP 16       ;; 3,6

      ;; Here we want the median only of H(18++,0), H(18++,16), H(18++,32)
      vmax H(36++,0), H(18++,32), H(18++,16)  REP 16        ;; 3,5
      vmin H(18++,16), H(18++,32), H(18++,16)  REP 16
      vmin H(18++,0), H(18++,0), H(36++,0)  REP 16       ;; 3,7
      vmax H(18++,16), H(18++,16), H(18++,0)  REP 16        ;; 5,7

      vst H(18++,16), (r1+=r2) REP 16

      cbadd1            ;; advance circular buffer

      add r0, 16
      add r1, 16
      addcmpbgt r7, -1, 0, loop_cols

      add r0, r8
      add r1, r8
      addcmpbgt r5, -1, 0, loop_rows

      ldm r6-r9, pc, (sp++)
   .stabs "vclib_medianfilter:F1",36,0,0,vclib_medianfilter
