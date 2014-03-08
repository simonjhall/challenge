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
.stabs "asm_file",100,0,0,vc_rgb565_fill_16_rows

.macro function, name
.define funcname, name
.pushsect .text
.section .text$\&funcname,"text"
.globl funcname
funcname:
.cfa_bf funcname
.endm

.macro endfunc
.cfa_ef
.size funcname,.-funcname
.popsect
.undef funcname
.endm

;;; *****************************************************************************

;;; NAME
;;;    vc_rgb565_fill_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_fill_16_rows(unsigned short *dest, int pitch_bytes, int start_offset,
;;;                                int n_blocks, int n_extra_cols, int value)
;;;
;;; FUNCTION
;;;    Fill a 16-deep stripe of the dest array with the constant value.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_fill_16_rows,"text"
.globl vc_rgb565_fill_16_rows
vc_rgb565_fill_16_rows:
      stm r6-r7, lr, (--sp)
      add r6, pc, pcrel(vc_image_const_0_15)
      vld H(63,0), (r6)
.if _VC_VERSION > 2
      lsr r0, 5
      shl r0, 5
.endif
      addcmpbge r3, 0, 0, rgb656_fill_16_rows_beyond_block
      ;; The region to fill lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r2 SETF
      vmov H(62,0), 16 IFN
      sub r4, r2
      vsub -, H(62,0), r4 SETF
      vld HX(0++,0), (r0+=r1) REP 16
      vmov HX(0++,0), r5 IFN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      ldm r6-r7, pc, (sp++)

rgb656_fill_16_rows_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r2, 0, 0, rgb656_fill_16_rows_start_aligned
      vsub -, H(63,0), r2 SETF
      vld HX(0++,0), (r0+=r1) REP 16
      vmov HX(0++,0), r5 IFNN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      add r0, 32

rgb656_fill_16_rows_start_aligned:
      addcmpbeq r3, 0, 0, rgb656_fill_16_rows_no_blocks
      vmov HX(0,0), r5
      mov r7, 32
      shl r3, 5            ; compute terminating address for r0
      add r3, r0

      ;; This loop simple blasts whole 16x16 pixel blocks with the constant value.
rgb656_fill_16_rows_loop:
.pragma offwarn
      vst HX(0,0), (r0+=r1) REP 16
.pragma popwarn
      addcmpbne r0, r7, r3, rgb656_fill_16_rows_loop

rgb656_fill_16_rows_no_blocks:
      addcmpble r4, 0, 0, vc_rgb565_fill_16_skip_trailing
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0++,0), (r0+=r1) REP 16
      vsub -, H(63,0), r4 SETF
      vmov HX(0++,0), r5 IFN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
vc_rgb565_fill_16_skip_trailing:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_fill_16_rows:F1",36,0,0,vc_rgb565_fill_16_rows

;;; NAME
;;;    vc_rgb565_fill_row
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_fill_row(unsigned short *dest, int start_offset,
;;;                            int n_blocks, int n_extra_cols, int value)
;;;
;;; FUNCTION
;;;    Fill a single stripe of the dest array with the constant value.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_fill_row,"text"
.globl vc_rgb565_fill_row
vc_rgb565_fill_row:
      ;; This function is identical to vc_rgb565_fill_16_rows except that it operates
      ;; only on a single row of pixels rather than 16.
      stm r6-r6, lr, (--sp)
      add r5, pc, pcrel(vc_image_const_0_15)
.if _VC_VERSION > 2
      lsr r0, 5
      shl r0, 5
.endif
      vld H(63,0), (r5)

      addcmpbge r2, 0, 0, rgb656_fill_row_beyond_block
      ;; The region to fill lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r1 SETF
      vmov H(62,0), 16 IFN
      sub r3, r1
      vsub -, H(62,0), r3 SETF
      vld HX(0,0), (r0)
      vmov HX(0,0), r4 IFN
      vst HX(0,0), (r0)
      ldm r6-r6, pc, (sp++)

rgb656_fill_row_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r1, 0, 0, vc_rgb565_fill_row_start_aligned
      vsub -, H(63,0), r1 SETF
      vld HX(0,0), (r0)
      vmov HX(0,0), r4 IFNN
      vst HX(0,0), (r0)
      add r0, 32

vc_rgb565_fill_row_start_aligned:
      addcmpbeq r2, 0, 0, vc_rgb565_fill_row_no_blocks
      vmov HX(0,0), r4
      mov r6, 32
      shl r2, 5            ; compute terminating address for r0
      add r2, r0

      ;; This loop simple blasts 16 pixels with the constant value.
vc_rgb565_fill_row_loop:
      vst HX(0,0), (r0)
      addcmpbne r0, r6, r2, vc_rgb565_fill_row_loop

vc_rgb565_fill_row_no_blocks:
      addcmpble r3, 0, 0, vc_rgb565_fill_row_skip_trailing
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0,0), (r0)
      vsub -, H(63,0), r3 SETF
      vmov HX(0,0), r4 IFN
      vst HX(0,0), (r0)
vc_rgb565_fill_row_skip_trailing:
      ldm r6-r6, pc, (sp++)
.stabs "vc_rgb565_fill_row:F1",36,0,0,vc_rgb565_fill_row

;;; NAME
;;;    vc_rgb565_blt_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width)
;;;
;;; FUNCTION
;;;    Copy a 16-deep stripe of the src array to the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_blt_16_rows,"text"
.globl vc_rgb565_blt_16_rows
vc_rgb565_blt_16_rows:
      stm r6-r7, lr, (--sp)

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)
.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      addcmpbeq r6, 0, 0, vc_rgb565_blt_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_blt_16_rows_loop:
      ;; Only load next dest block if it's the last.
      addcmpbne r6, 0, 1, vc_rgb565_blt_16_rows_not_last
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_blt_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      vmov VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif
      vst HX(0++,0)*, (r0+=r2) REP 16
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif

      add r0, 32
      add r1, 32
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_blt_16_rows_loop

vc_rgb565_blt_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_blt_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_blt_16_rows_trailing_loop:
      vmov VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_blt_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_blt_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif
      vst HX(0++,0)*, (r0+=r2) REP 16
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif
      add r0, 32
      cbadd2

vc_rgb565_blt_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_blt_16_rows_end
      ;; Must write out the last block.
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif
      vst HX(0++,0)*, (r0+=r2) REP 16
.ifdef __BCM2707B0__
      vadd VX(0,0)*, VX(0,0)*, 0
.endif

vc_rgb565_blt_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_blt_16_rows:F1",36,0,0,vc_rgb565_blt_16_rows

;;; NAME
;;;    vc_rgb565_blt_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows)
;;;
;;; FUNCTION
;;;    Copy an n-row deep stripe of the src array to the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_blt_n_rows,"text"
.globl vc_rgb565_blt_n_rows
vc_rgb565_blt_n_rows:
      stm r6-r12, lr, (--sp)

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_blt_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_blt_n_rows_loop:
      ;; Only load next dest block if it's the last.
      addcmpbne r8, 0, 1, vc_rgb565_blt_n_rows_not_last
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_blt_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      vmov VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_blt_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_blt_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_blt_n_rows_loop

vc_rgb565_blt_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_blt_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_blt_n_rows_trailing_loop:
      vmov VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_blt_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_blt_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_blt_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_blt_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_blt_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_blt_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_blt_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_blt_n_rows_save3

vc_rgb565_blt_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_blt_n_rows:F1",36,0,0,vc_rgb565_blt_n_rows

;;; NAME
;;;    vc_rgb565_masked_fill_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_masked_fill_16_rows(unsigned short *dest, unsigned char *src,
;;;                                       int dest_pitch_bytes, int src_pitch_bytes, int width, int value, int mask_x_offset, int invert)
;;;
;;; FUNCTION
;;;    Copy a 16-deep stripe of the src array to the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_masked_fill_16_rows,"text"
.globl vc_rgb565_masked_fill_16_rows
vc_rgb565_masked_fill_16_rows:
      stm r6-r7, lr, (--sp)

      vmov HX(62,0), r5    ; save value

      ld r5, (sp+16)       ; invert
      vmov HX(63,0), r5

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      ;; src is the address in *bytes* (should be even)
      ld r5, (sp+12)       ; mask offset

.if _VC_VERSION > 2
      bic r0, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vldleft HX(16++,0), (r1+=r3) REP 16 ; load up first mask block and unpack
      veor VX(16,0), VX(16,0), HX(63,0)   ; take care of invert
      vbitplanes HX(16++,0), HX(16++,0) REP 16

      addcmpbeq r6, 0, 0, vc_rgb565_masked_fill_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_masked_fill_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_masked_fill_16_rows_not_last:
      ;; Load next mask block and unpack.
      vldleft HX(16++,32)*, (r1+2+=r3) REP 16
      veor VX(16,32)*, VX(16,32)*, HX(63,0)   ; take care of invert
      vbitplanes HX(16++,32)*, HX(16++,32)* REP 16
      ;; Copy 16-columns over to the correct alignment

      ;; Writing the code out with SETF and IFNZ, two instructions per 16 pixels does
      ;; work because of VX addressing mode problems. This method is more painful but
      ;; does it without the annoyance of an explicit loop.
      vsub HX(32++,0), HX(62,0), VX(0,0++)+r4* REP 16
      vand HX(32++,32), VX(16,0++)+r5*, 1 REP 16
      vmul HX(32++,0), HX(32++,0), HX(32++,32) REP 16
      vadd VX(0,0++)+r4*, VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 2
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_masked_fill_16_rows_loop

vc_rgb565_masked_fill_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_masked_fill_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vldleft HX(16++,32)*, (r1+2+=r3) REP 16
      veor VX(16,32)*, VX(16,32)*, HX(63,0)   ; take care of invert
      vbitplanes HX(16++,32)*, HX(16++,32)* REP 16

      ;; Copy r7 columns from one to t'other.

vc_rgb565_masked_fill_16_rows_trailing_loop:
      vand -, VX(16,0)+r5*, 1 SETF
      vmov VX(0,0)+r4*, HX(62,0) IFNZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_masked_fill_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_masked_fill_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_masked_fill_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_masked_fill_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_masked_fill_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_masked_fill_16_rows:F1",36,0,0,vc_rgb565_masked_fill_16_rows

;;; NAME
;;;    vc_rgb565_masked_fill_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_masked_fill_n_rows(unsigned short *dest, unsigned char *src,
;;;                                       int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows, int value, int mask_x_offset)
;;;
;;; FUNCTION
;;;    Copy a 16-deep stripe of the src array to the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_masked_fill_n_rows,"text"
.globl vc_rgb565_masked_fill_n_rows
vc_rgb565_masked_fill_n_rows:
      stm r6-r12, lr, (--sp)

      ld r6, (sp+32)
      vmov HX(62,0), r6    ; save value

      ld r6, (sp+40)       ; invert
      vmov HX(63,0), r6

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      ;; src is the address in *bytes* (should be even)
      ld r5, (sp+36)       ; mask offset

.if _VC_VERSION > 2
      bic r0, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vldleft HX(16++,0), (r1+=r3) REP 16 ; load up first mask block and unpack
      veor VX(16,0), VX(16,0), HX(63,0)   ; take care of invert
      vbitplanes HX(16++,0), HX(16++,0) REP 16

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_masked_fill_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_masked_fill_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_masked_fill_n_rows_not_last:
      ;; Load next mask block and unpack.
      vldleft HX(16++,32)*, (r1+2+=r3) REP 16
      veor VX(16,32)*, VX(16,32)*, HX(63,0)   ; take care of invert
      vbitplanes HX(16++,32)*, HX(16++,32)* REP 16
      ;; Copy 16-columns over to the correct alignment

      ;; Writing the code out with SETF and IFNZ, two instructions per 16 pixels does
      ;; work because of VX addressing mode problems. This method is more painful but
      ;; does it without the annoyance of an explicit loop.
      vsub HX(32++,0), HX(62,0), VX(0,0++)+r4* REP 16
      vand HX(32++,32), VX(16,0++)+r5*, 1 REP 16
      vmul HX(32++,0), HX(32++,0), HX(32++,32) REP 16
      vadd VX(0,0++)+r4*, VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_masked_fill_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_fill_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 2
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_masked_fill_n_rows_loop

vc_rgb565_masked_fill_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_masked_fill_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vldleft HX(16++,32)*, (r1+2+=r3) REP 16
      veor VX(16,32)*, VX(16,32)*, HX(63,0)   ; take care of invert
      vbitplanes HX(16++,32)*, HX(16++,32)* REP 16

      ;; Copy r7 columns from one to t'other.

vc_rgb565_masked_fill_n_rows_trailing_loop:
      vand -, VX(16,0)+r5*, 1 SETF
      vmov VX(0,0)+r4*, HX(62,0) IFNZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_masked_fill_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_masked_fill_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_masked_fill_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_fill_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_masked_fill_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_masked_fill_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_masked_fill_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_fill_n_rows_save3

vc_rgb565_masked_fill_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_masked_fill_n_rows:F1",36,0,0,vc_rgb565_masked_fill_n_rows

;;; NAME
;;;    vc_rgb565_not_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_not_16_rows(unsigned short *dest, int pitch_bytes, int start_offset,
;;;                               int n_blocks, int n_extra_cols)
;;;
;;; FUNCTION
;;;    Not a 16-deep stripe of the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_not_16_rows,"text"
.globl vc_rgb565_not_16_rows
vc_rgb565_not_16_rows:
      stm r6-r7, lr, (--sp)

      add r6, pc, pcrel(vc_image_const_0_15)
      vld H(63,0), (r6)

      addcmpbge r3, 0, 0, rgb656_not_16_rows_beyond_block
      ;; The region to not lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r2 SETF
      vmov H(62,0), 16 IFN
      sub r4, r2
      vsub -, H(62,0), r4 SETF
      vld HX(0++,0), (r0+=r1) REP 16
      veor HX(0++,0), HX(0++,0), 0xffff IFN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      ldm r6-r7, pc, (sp++)

rgb656_not_16_rows_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r2, 0, 0, rgb656_not_16_rows_start_aligned
      vsub -, H(63,0), r2 SETF
      vld HX(0++,0), (r0+=r1) REP 16
      veor HX(0++,0), HX(0++,0), 0xffff IFNN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      add r0, 32

rgb656_not_16_rows_start_aligned:
      addcmpbeq r3, 0, 0, rgb656_not_16_rows_no_blocks
      mov r7, 32
      shl r3, 5            ; compute terminating address for r0
      add r3, r0

      ;; This loop simple nots whole 16x16 pixel blocks.
rgb656_not_16_rows_loop:
      vld HX(0++,0), (r0+=r1) REP 16
      veor HX(0++,0), HX(0++,0), 0xffff REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      addcmpbne r0, r7, r3, rgb656_not_16_rows_loop

rgb656_not_16_rows_no_blocks:
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0++,0), (r0+=r1) REP 16
      vsub -, H(63,0), r4 SETF
      veor HX(0++,0), HX(0++,0), 0xffff IFN REP 16
      vst HX(0++,0), (r0+=r1) REP 16
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_not_16_rows:F1",36,0,0,vc_rgb565_not_16_rows

;;; NAME
;;;    vc_rgb565_not_row
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_not_row(unsigned short *dest, int start_offset,
;;;                            int n_blocks, int n_extra_cols)
;;;
;;; FUNCTION
;;;    Not a single stripe of the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_not_row,"text"
.globl vc_rgb565_not_row
vc_rgb565_not_row:
      ;; This function is identical to vc_rgb565_not_16_rows except that it operates
      ;; only on a single row of pixels rather than 16.
      stm r6-r6, lr, (--sp)
      add r5, pc, pcrel(vc_image_const_0_15)
      vld H(63,0), (r5)

      addcmpbge r2, 0, 0, rgb656_not_row_beyond_block
      ;; The region to not lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r1 SETF
      vmov H(62,0), 16 IFN
      sub r3, r1
      vsub -, H(62,0), r3 SETF
      vld HX(0,0), (r0)
      veor HX(0,0), HX(0,0), 0xffff IFN
      vst HX(0,0), (r0)
      ldm r6-r6, pc, (sp++)

rgb656_not_row_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r1, 0, 0, vc_rgb565_not_row_start_aligned
      vsub -, H(63,0), r1 SETF
      vld HX(0,0), (r0)
      veor HX(0,0), HX(0,0), 0xffff IFNN
      vst HX(0,0), (r0)
      add r0, 32

vc_rgb565_not_row_start_aligned:
      addcmpbeq r2, 0, 0, vc_rgb565_not_row_no_blocks
      mov r6, 32
      shl r2, 5            ; compute terminating address for r0
      add r2, r0

      ;; This loop simple blasts 16 pixels with the constant value.
vc_rgb565_not_row_loop:
      vld HX(0,0), (r0)
      veor HX(0,0), HX(0,0), 0xffff
      vst HX(0,0), (r0)
      addcmpbne r0, r6, r2, vc_rgb565_not_row_loop

vc_rgb565_not_row_no_blocks:
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0,0), (r0)
      vsub -, H(63,0), r3 SETF
      veor HX(0,0), HX(0,0), 0xffff IFN
      vst HX(0,0), (r0)
      ldm r6-r6, pc, (sp++)
.stabs "vc_rgb565_not_row:F1",36,0,0,vc_rgb565_not_row

;;; NAME
;;;    vc_rgb565_fade_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_not_16_rows(unsigned short *dest, int pitch_bytes, int start_offset,
;;;                               int n_blocks, int n_extra_cols, int fade)
;;;
;;; FUNCTION
;;;    Fade a 16-deep stripe of the dest array. fade is an 8.8 format value.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_fade_16_rows,"text"
.globl vc_rgb565_fade_16_rows
vc_rgb565_fade_16_rows:
      stm r6-r7, lr, (--sp)

      vmov HX(61,0), r5    ; store fade factor for safe keeping

      add r6, pc, pcrel(vc_image_const_0_15)
      vld H(63,0), (r6)

      addcmpbge r3, 0, 0, rgb656_fade_16_rows_beyond_block
      ;; The region to not lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r2 SETF
      vmov H(62,0), 16 IFN
      sub r4, r2
      vsub -, H(62,0), r4 SETF
      vld HX(0++,0), (r0+=r1) REP 16

;     vlsr HX(0++,32), HX(0++,0), 11 IFN REP 16 ; R
;     vand HX(16++,32), HX(0++,0), 0x07e0 IFN REP 16 ; G
;     vand HX(32++,32), HX(0++,0), 0x001f IFN REP 16 ; B
;     vmulms HX(0++,32), HX(0++,32), HX(61,0) IFN REP 16
;     vmulms HX(16++,32), HX(16++,32), HX(61,0) IFN REP 16
;     vmulms HX(32++,32), HX(32++,32), HX(61,0) IFN REP 16
;     vshl HX(0++,32), HX(0++,32), 11 IFN REP 16
;     vand HX(16++,32), HX(16++,32), 0x07e0 IFN REP 16
;     vand HX(32++,32), HX(32++,32), 0x001f IFN REP 16
;     vor HX(0++,32), HX(0++,32), HX(16++,32) IFN REP 16
;     vor HX(0++,0), HX(0++,32), HX(32++,32) IFN REP 16

;     vst HX(0++,0), (r0+=r1) REP 16
;     ldm r6-r7, pc, (sp++)

      ;; the code above is identical to that at the end of the function, so its
      ;; probably worth saving the space.
      b rgb565_fade_16_rows_end_of_fn

rgb656_fade_16_rows_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r2, 0, 0, rgb656_fade_16_rows_start_aligned
      vsub -, H(63,0), r2 SETF
      vld HX(0++,0), (r0+=r1) REP 16

      ;; This is the basic fade/brighten code. Handling saturation properly (when brightening)
      ;; makes it slightly more awkward. This block is essentially repeated a number of times
      ;; in this function and the next.
      vand HX(0++,32), HX(0++,0), 0xf800 IFNN REP 16      ; R
      vmulms H(0++,48), H(0++,48), HX(61,0) IFNN REP 16
      vand HX(0++,32), HX(0++,32), 0xf800 IFNN REP 16
      vshl HX(16++,32), HX(0++,0), 5 IFNN REP 16          ; G
      vand HX(16++,32), HX(16++,32), 0xfc00 IFNN REP 16
      vmulms H(16++,48), H(16++,48), HX(61,0) IFNN REP 16
      vand HX(16++,32), HX(16++,32), 0xfc00 IFNN REP 16
      vlsr HX(16++,32), HX(16++,32), 5 IFNN REP 16
      vshl HX(32++,32), HX(0++,0), 11 IFNN REP 16         ; B
      vmulms H(32++,48), H(32++,48), HX(61,0) IFNN REP 16
      vlsr HX(32++,32), HX(32++,32), 11 IFNN REP 16
      vor HX(0++,32), HX(0++,32), HX(16++,32) IFNN REP 16 ; recombine
      vor HX(0++,0), HX(0++,32), HX(32++,32) IFNN REP 16

      vst HX(0++,0), (r0+=r1) REP 16
      add r0, 32

rgb656_fade_16_rows_start_aligned:
      addcmpbeq r3, 0, 0, rgb656_fade_16_rows_no_blocks
      mov r7, 32
      shl r3, 5            ; compute terminating address for r0
      add r3, r0

      ;; This loop simple nots whole 16x16 pixel blocks.
rgb656_fade_16_rows_loop:
      vld HX(0++,0), (r0+=r1) REP 16

      vand HX(0++,32), HX(0++,0), 0xf800 REP 16      ; R
      vmulms H(0++,48), H(0++,48), HX(61,0) REP 16
      vand HX(0++,32), HX(0++,32), 0xf800 REP 16
      vshl HX(16++,32), HX(0++,0), 5 REP 16          ; G
      vand HX(16++,32), HX(16++,32), 0xfc00 REP 16
      vmulms H(16++,48), H(16++,48), HX(61,0) REP 16
      vand HX(16++,32), HX(16++,32), 0xfc00 REP 16
      vlsr HX(16++,32), HX(16++,32), 5 REP 16
      vshl HX(32++,32), HX(0++,0), 11 REP 16         ; B
      vmulms H(32++,48), H(32++,48), HX(61,0) REP 16
      vlsr HX(32++,32), HX(32++,32), 11 REP 16
      vor HX(0++,32), HX(0++,32), HX(16++,32) REP 16 ; recombine
      vor HX(0++,0), HX(0++,32), HX(32++,32) REP 16

      vst HX(0++,0), (r0+=r1) REP 16
      addcmpbne r0, r7, r3, rgb656_fade_16_rows_loop

rgb656_fade_16_rows_no_blocks:
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0++,0), (r0+=r1) REP 16
      vsub -, H(63,0), r4 SETF
rgb565_fade_16_rows_end_of_fn:
      vand HX(0++,32), HX(0++,0), 0xf800 IFN REP 16      ; R
      vmulms H(0++,48), H(0++,48), HX(61,0) IFN REP 16
      vand HX(0++,32), HX(0++,32), 0xf800 IFN REP 16
      vshl HX(16++,32), HX(0++,0), 5 IFN REP 16          ; G
      vand HX(16++,32), HX(16++,32), 0xfc00 IFN REP 16
      vmulms H(16++,48), H(16++,48), HX(61,0) IFN REP 16
      vand HX(16++,32), HX(16++,32), 0xfc00 IFN REP 16
      vlsr HX(16++,32), HX(16++,32), 5 IFN REP 16
      vshl HX(32++,32), HX(0++,0), 11 IFN REP 16         ; B
      vmulms H(32++,48), H(32++,48), HX(61,0) IFN REP 16
      vlsr HX(32++,32), HX(32++,32), 11 IFN REP 16
      vor HX(0++,32), HX(0++,32), HX(16++,32) IFN REP 16 ; recombine
      vor HX(0++,0), HX(0++,32), HX(32++,32) IFN REP 16

      vst HX(0++,0), (r0+=r1) REP 16
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_fade_16_rows:F1",36,0,0,vc_rgb565_fade_16_rows

;;; NAME
;;;    vc_rgb565_fade_row
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_fade_row(unsigned short *dest, int start_offset,
;;;                            int n_blocks, int n_extra_cols, int fade)
;;;
;;; FUNCTION
;;;    Not a single stripe of the dest array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_fade_row,"text"
.globl vc_rgb565_fade_row
vc_rgb565_fade_row:
      ;; This function is identical to vc_rgb565_fade_16_rows except that it operates
      ;; only on a single row of pixels rather than 16.
      stm r6-r6, lr, (--sp)

      vmov HX(61,0), r4    ; store fade factor for safe keeping

      add r5, pc, pcrel(vc_image_const_0_15)
      vld H(63,0), (r5)

      addcmpbge r2, 0, 0, rgb656_fade_row_beyond_block
      ;; The region to not lies wholly within a 16-wide block.
      vsub H(62,0), H(63,0), r1 SETF
      vmov H(62,0), 16 IFN
      sub r3, r1
      vsub -, H(62,0), r3 SETF
      vld HX(0,0), (r0)

;     vlsr HX(0,32), HX(0,0), 11 IFN ; R
;     vand HX(16,32), HX(0,0), 0x07e0 IFN ; G
;     vand HX(32,32), HX(0,0), 0x001f IFN ; B
;     vmulms HX(0,32), HX(0,32), HX(61,0) IFN
;     vmulms HX(16,32), HX(16,32), HX(61,0) IFN
;     vmulms HX(32,32), HX(32,32), HX(61,0) IFN
;     vshl HX(0,32), HX(0,32), 11 IFN
;     vand HX(16,32), HX(16,32), 0x07e0 IFN
;     vand HX(32,32), HX(32,32), 0x001f IFN
;     vor HX(0,32), HX(0,32), HX(16,32) IFN
;     vor HX(0,0), HX(0,32), HX(32,32) IFN

;     vst HX(0,0), (r0)
;     ldm r6-r6, pc, (sp++)

      ;; the code above is the same as that at the end of the fn
      b rgb565_fade_row_end_of_fn

rgb656_fade_row_beyond_block:
      ;; Test whether the start_offset was unaligned. If so, we need to load that
      ;; block and change the entries only from start_offset onwards.
      addcmpbeq r1, 0, 0, vc_rgb565_fade_row_start_aligned
      vsub -, H(63,0), r1 SETF
      vld HX(0,0), (r0)

      vand HX(0,32), HX(0,0), 0xf800 IFNN      ; R
      vmulms H(0,48), H(0,48), HX(61,0) IFNN
      vand HX(0,32), HX(0,32), 0xf800 IFNN
      vshl HX(16,32), HX(0,0), 5 IFNN          ; G
      vand HX(16,32), HX(16,32), 0xfc00 IFNN
      vmulms H(16,48), H(16,48), HX(61,0) IFNN
      vand HX(16,32), HX(16,32), 0xfc00 IFNN
      vlsr HX(16,32), HX(16,32), 5 IFNN
      vshl HX(32,32), HX(0,0), 11 IFNN         ; B
      vmulms H(32,48), H(32,48), HX(61,0) IFNN
      vlsr HX(32,32), HX(32,32), 11 IFNN
      vor HX(0,32), HX(0,32), HX(16,32) IFNN ; recombine
      vor HX(0,0), HX(0,32), HX(32,32) IFNN

      vst HX(0,0), (r0)
      add r0, 32

vc_rgb565_fade_row_start_aligned:
      addcmpbeq r2, 0, 0, vc_rgb565_fade_row_no_blocks
      mov r6, 32
      shl r2, 5            ; compute terminating address for r0
      add r2, r0

      ;; This loop simple blasts 16 pixels with the constant value.
vc_rgb565_fade_row_loop:
      vld HX(0,0), (r0)

      vand HX(0,32), HX(0,0), 0xf800       ; R
      vmulms H(0,48), H(0,48), HX(61,0)
      vand HX(0,32), HX(0,32), 0xf800
      vshl HX(16,32), HX(0,0), 5           ; G
      vand HX(16,32), HX(16,32), 0xfc00
      vmulms H(16,48), H(16,48), HX(61,0)
      vand HX(16,32), HX(16,32), 0xfc00
      vlsr HX(16,32), HX(16,32), 5
      vshl HX(32,32), HX(0,0), 11          ; B
      vmulms H(32,48), H(32,48), HX(61,0)
      vlsr HX(32,32), HX(32,32), 11
      vor HX(0,32), HX(0,32), HX(16,32)  ; recombine
      vor HX(0,0), HX(0,32), HX(32,32)

      vst HX(0,0), (r0)
      addcmpbne r0, r6, r2, vc_rgb565_fade_row_loop

vc_rgb565_fade_row_no_blocks:
      ;; There may be some trailing columns too. Load up the block that contains
      ;; them and modify the necessary entries.
      vld HX(0,0), (r0)
      vsub -, H(63,0), r3 SETF
rgb565_fade_row_end_of_fn:
      vand HX(0,32), HX(0,0), 0xf800 IFN      ; R
      vmulms H(0,48), H(0,48), HX(61,0) IFN
      vand HX(0,32), HX(0,32), 0xf800 IFN
      vshl HX(16,32), HX(0,0), 5 IFN          ; G
      vand HX(16,32), HX(16,32), 0xfc00 IFN
      vmulms H(16,48), H(16,48), HX(61,0) IFN
      vand HX(16,32), HX(16,32), 0xfc00 IFN
      vlsr HX(16,32), HX(16,32), 5 IFN
      vshl HX(32,32), HX(0,0), 11 IFN         ; B
      vmulms H(32,48), H(32,48), HX(61,0) IFN
      vlsr HX(32,32), HX(32,32), 11 IFN
      vor HX(0,32), HX(0,32), HX(16,32) IFN ; recombine
      vor HX(0,0), HX(0,32), HX(32,32) IFN

      vst HX(0,0), (r0)
      ldm r6-r6, pc, (sp++)
.stabs "vc_rgb565_fade_row:F1",36,0,0,vc_rgb565_fade_row

;;; NAME
;;;    vc_rgb565_or_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_or_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width)
;;;
;;; FUNCTION
;;;    OR 16-deep stripe of dest array and src array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_or_16_rows,"text"
.globl vc_rgb565_or_16_rows
vc_rgb565_or_16_rows:
      stm r6-r7, lr, (--sp)

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      addcmpbeq r6, 0, 0, vc_rgb565_or_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_or_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_or_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      vor VX(0,0++)+r4*, VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 32
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_or_16_rows_loop

vc_rgb565_or_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_or_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_or_16_rows_trailing_loop:
      vor VX(0,0)+r4*, VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_or_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_or_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_or_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_or_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_or_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_or_16_rows:F1",36,0,0,vc_rgb565_or_16_rows

;;; NAME
;;;    vc_rgb565_or_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_or_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows)
;;;
;;; FUNCTION
;;;    OR nrows row of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_or_n_rows,"text"
.globl vc_rgb565_or_n_rows
vc_rgb565_or_n_rows:
      stm r6-r12, lr, (--sp)

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_or_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_or_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_or_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      vor VX(0,0++)+r4*, VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_or_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_or_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_or_n_rows_loop

vc_rgb565_or_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_or_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_or_n_rows_trailing_loop:
      vor VX(0,0)+r4*, VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_or_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_or_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_or_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_or_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_or_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_or_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_or_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_or_n_rows_save3

vc_rgb565_or_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_or_n_rows:F1",36,0,0,vc_rgb565_or_n_rows

;;; NAME
;;;    vc_rgb565_xor_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_xor_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width)
;;;
;;; FUNCTION
;;;    XOR 16-deep stripe of dest array and src array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_xor_16_rows,"text"
.globl vc_rgb565_xor_16_rows
vc_rgb565_xor_16_rows:
      stm r6-r7, lr, (--sp)

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      addcmpbeq r6, 0, 0, vc_rgb565_xor_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_xor_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_xor_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      veor VX(0,0++)+r4*, VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 32
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_xor_16_rows_loop

vc_rgb565_xor_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_xor_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_xor_16_rows_trailing_loop:
      veor VX(0,0)+r4*, VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_xor_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_xor_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_xor_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_xor_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_xor_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_xor_16_rows:F1",36,0,0,vc_rgb565_xor_16_rows

;;; NAME
;;;    vc_rgb565_xor_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_xor_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes, int width, int nrows)
;;;
;;; FUNCTION
;;;    XOR nrows row of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_xor_n_rows,"text"
.globl vc_rgb565_xor_n_rows
vc_rgb565_xor_n_rows:
      stm r6-r12, lr, (--sp)

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_xor_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_xor_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_xor_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy 16-columns over to the correct alignment
      veor VX(0,0++)+r4*, VX(0,0++)+r4*, VX(16,0++)+r5* REP 16
      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_xor_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_xor_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_xor_n_rows_loop

vc_rgb565_xor_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_xor_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_xor_n_rows_trailing_loop:
      veor VX(0,0)+r4*, VX(0,0)+r4*, VX(16,0)+r5*
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_xor_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_xor_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_xor_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_xor_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_xor_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_xor_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_xor_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_xor_n_rows_save3

vc_rgb565_xor_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_xor_n_rows:F1",36,0,0,vc_rgb565_xor_n_rows

;;; NAME
;;;    vc_rgb565_transpose
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transpose(unsigned short *src, unsigned short *dest, int src_pitch_bytes, int dest_pitch_bytes, int nblocks)
;;;
;;; FUNCTION
;;;    Transpose a 16-deep stripe from src into dest.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transpose  ,"text"
.globl vc_rgb565_transpose, vc_rgb565_transpose_tiles
vc_rgb565_transpose:
      mov r5, r3
      shl r5, 4
vc_rgb565_transpose1:
      vld HX(0++,0), (r0+=r2) REP 16
      vst VX(0,0++), (r1+=r3) REP 16
      add r0, 32
      add r1, r5
      addcmpbgt r4, -1, 0, vc_rgb565_transpose1
      b lr
.stabs "vc_rgb565_transpose:F1",36,0,0,vc_rgb565_transpose

;;; NAME
;;;    vc_rgb565_transpose_chunky
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transpose_chunky(unsigned short *dest, int dpitch, unsigned short *src, int spitch);
;;;
;;; FUNCTION
;;;    Transpose 4kB of stuff in-place.
;;;
;;; RETURNS
;;;    -

function vc_rgb565_transpose_chunky ; {{{
               addscale r4, r2, r3 << 6
               vld      HX(0++,0), (r2+=r3)        REP 64
               vld      HX(0++,32), (r4+=r3)       REP 64

               vst      VX(0,0++), (r0+=r1)        REP 16
               vst      VX(16,0++), (r0+32+=r1)    REP 16
               vst      VX(32,0++), (r0+64+=r1)    REP 16
               vst      VX(48,0++), (r0+96+=r1)    REP 16
               vst      VX(0,32++), (r0+128+=r1)   REP 16
               vst      VX(16,32++), (r0+160+=r1)  REP 16
               vst      VX(32,32++), (r0+192+=r1)  REP 16
               vst      VX(48,32++), (r0+224+=r1)  REP 16
               b        lr
endfunc ; }}}
.FORGET

;;; NAME
;;;    vc_rgb565_transpose_chunky0
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transpose_chunky0(unsigned short *dest, int dpitch, unsigned short *src, int spitch);
;;;
;;; FUNCTION
;;;    Transpose 4kB of stuff in-place, dropping every odd pixel.
;;;
;;; RETURNS
;;;    -

function vc_rgb565_transpose_chunky0 ; {{{
               vld      HX(0++,0), (r2+=r3)        REP 64
               vld      HX(0++,32), (r2+32+=r3)    REP 64

               vfrom32l HX(0++,0), HX(0++,0), HX(0++,32) REP 64

               vst      VX(0,0++), (r0+=r1)        REP 16
               vst      VX(16,0++), (r0+32+=r1)    REP 16
               vst      VX(32,0++), (r0+64+=r1)    REP 16
               vst      VX(48,0++), (r0+96+=r1)    REP 16
               b        lr
endfunc ; }}}
.FORGET

;;; NAME
;;;    vc_rgb565_transpose_chunky1
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transpose_chunky1(unsigned short *dest, int dpitch, unsigned short *src, int spitch);
;;;
;;; FUNCTION
;;;    Transpose 4kB of stuff in-place, dropping every even pixel.
;;;
;;; RETURNS
;;;    -

function vc_rgb565_transpose_chunky1 ; {{{
               vld      HX(0++,0), (r2+=r3)        REP 64
               vld      HX(0++,32), (r2+32+=r3)    REP 64

               vfrom32h HX(0++,0), HX(0++,0), HX(0++,32) REP 64

               vst      VX(0,0++), (r0+=r1)        REP 16
               vst      VX(16,0++), (r0+32+=r1)    REP 16
               vst      VX(32,0++), (r0+64+=r1)    REP 16
               vst      VX(48,0++), (r0+96+=r1)    REP 16
               b        lr
endfunc ; }}}
.FORGET

;;; NAME
;;;    vc_rgb565_vflip
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_vflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int height, int nblocks, int dest_pitch_bytes)
;;;
;;; FUNCTION
;;;    Flip image vertically.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_vflip,"text"
.globl vc_rgb565_vflip
vc_rgb565_vflip:
      stm r6-r9, lr, (--sp)
      mov r9, r5
      mov r6, 32
      ;; It is easiest just to copy over 1 row at a time, and not desparately inefficient.
vc_rgb565_vflip_vloop:
      mov r5, r4
      shl r5, 5
      add r5, r0           ; target address for r0
      mov r7, r0
      mov r8, r1
vc_rgb565_vflip_hloop:
      vld HX(0,0), (r1)
      vst HX(0,0), (r0)
      add r1, r6
      addcmpbne r0, r6, r5, vc_rgb565_vflip_hloop
      add r0, r7, r9
      sub r1, r8, r2
      addcmpbne r3, -1, 0, vc_rgb565_vflip_vloop
      ldm r6-r9, pc, (sp++)
.stabs "vc_rgb565_vflip:F1",36,0,0,vc_rgb565_vflip

;;; NAME
;;;    vc_rgb565_hflip
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_hflip(unsigned short *dest, unsigned short *src, int src_pitch_bytes, int nblocks, int dest_pitch_bytes)
;;;
;;; FUNCTION
;;;    Flip image horizontally.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_hflip,"text"
.globl vc_rgb565_hflip
vc_rgb565_hflip:
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; offset into block of 16 of src
      cbclr

      addcmpbeq r5, 0, 0, vc_rgb565_hflip_loop
      vld HX(0++,32), (r1+=r2) REP 16
vc_rgb565_hflip_loop:
      sub r1, 32
      ;; for the last block this may load some garbage, but it will only appear in the
      ;; padding at the end of the dest image
      vld HX(0++,0)*, (r1+=r2) REP 16
      ;; VX addressing bug in core means we have to copy them out first before reversing them
      vmov VX(16,0++), VX(0,0++)+r5* REP 16
      vmov VX(32,0), VX(16,15)
      vmov VX(32,1), VX(16,14)
      vmov VX(32,2), VX(16,13)
      vmov VX(32,3), VX(16,12)
      vmov VX(32,4), VX(16,11)
      vmov VX(32,5), VX(16,10)
      vmov VX(32,6), VX(16,9)
      vmov VX(32,7), VX(16,8)
      vmov VX(32,8), VX(16,7)
      vmov VX(32,9), VX(16,6)
      vmov VX(32,10), VX(16,5)
      vmov VX(32,11), VX(16,4)
      vmov VX(32,12), VX(16,3)
      vmov VX(32,13), VX(16,2)
      vmov VX(32,14), VX(16,1)
      vmov VX(32,15), VX(16,0)
      vst HX(32++,0), (r0+=r4) REP 16
      add r0, 32
      cbadd2
      addcmpbne r3, -1, 0, vc_rgb565_hflip_loop

      b lr
.stabs "vc_rgb565_hflip:F1",36,0,0,vc_rgb565_hflip

;;; NAME
;;;    vc_rgb565_transpose_in_place
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transpose_in_place(unsigned short *dest, unsigned short *src, int pitch_bytes, int swap);
;;;
;;; FUNCTION
;;;    Transpose a 16x16 block, swapping with the destination block if necessary.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transpose_in_place,"text"
.globl vc_rgb565_transpose_in_place
vc_rgb565_transpose_in_place:
      vld HX(0++,0), (r1+=r2) REP 16
      addcmpbeq r3, 0, 0, vc_rgb565_transpose_in_place_no_swap
      ;; Need to swap the two blocks.
      vld HX(0++,32), (r0+=r2) REP 16
      vst VX(0,32++), (r1+=r2) REP 16
vc_rgb565_transpose_in_place_no_swap:
      vst VX(0,0++), (r0+=r2) REP 16
      b lr
.stabs "vc_rgb565_transpose_in_place:F1",36,0,0,vc_rgb565_transpose_in_place

;;; NAME
;;;    vc_rgb565_vflip_in_place
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_vflip_in_place(unsigned short *start, unsigned short *end, int pitch_bytes, int nblocks)
;;;
;;; FUNCTION
;;;    Flip image vertically.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_vflip_in_place,"text"
.globl vc_rgb565_vflip_in_place
vc_rgb565_vflip_in_place:
      stm r6-r7, lr, (--sp)
      mov r5, 32
      ;; It is easiest just to copy over 1 row at a time, and not desparately inefficient.
vc_rgb565_vflip_in_place_vloop:
      mov r4, r3
      shl r4, 5
      add r4, r0           ; target address for r0
      mov r6, r0
      mov r7, r1
vc_rgb565_vflip_in_place_hloop:
      vld HX(0,0), (r1)
      vld HX(1,0), (r0)
      vst HX(0,0), (r0)
      vst HX(1,0), (r1)
      add r1, r5
      addcmpbne r0, r5, r4, vc_rgb565_vflip_in_place_hloop
      add r0, r6, r2
      sub r1, r7, r2
      ;; Keep going till end <= start.
      addcmpbgt r1, 0, r0, vc_rgb565_vflip_in_place_vloop
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_vflip_in_place:F1",36,0,0,vc_rgb565_vflip_in_place

;;; NAME
;;;    vc_rgb565_hflip_in_place_aligned
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_hflip_in_place_aligned(unsigned short *start, unsigned short *end, int pitch_bytes)
;;;
;;; FUNCTION
;;;    Flip image horizontally.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_hflip_in_place_aligned,"text"
.globl vc_rgb565_hflip_in_place_aligned
vc_rgb565_hflip_in_place_aligned:
      mov r3, 32
vc_rgb565_hflip_in_place_aligned_loop:
      vld HX(0++,0), (r0+=r2) REP 16
      vmov VX(16,0), VX(0,15)
      vmov VX(16,1), VX(0,14)
      vmov VX(16,2), VX(0,13)
      vmov VX(16,3), VX(0,12)
      vmov VX(16,4), VX(0,11)
      vmov VX(16,5), VX(0,10)
      vmov VX(16,6), VX(0,9)
      vmov VX(16,7), VX(0,8)
      vmov VX(16,8), VX(0,7)
      vmov VX(16,9), VX(0,6)
      vmov VX(16,10), VX(0,5)
      vmov VX(16,11), VX(0,4)
      vmov VX(16,12), VX(0,3)
      vmov VX(16,13), VX(0,2)
      vmov VX(16,14), VX(0,1)
      vmov VX(16,15), VX(0,0)
      addcmpbeq r0, 0, r1, vc_rgb565_hflip_in_place_aligned_last_block
      vld HX(0++,32), (r1+=r2) REP 16
      vmov VX(16,32), VX(0,47)
      vmov VX(16,33), VX(0,46)
      vmov VX(16,34), VX(0,45)
      vmov VX(16,35), VX(0,44)
      vmov VX(16,36), VX(0,43)
      vmov VX(16,37), VX(0,42)
      vmov VX(16,38), VX(0,41)
      vmov VX(16,39), VX(0,40)
      vmov VX(16,40), VX(0,39)
      vmov VX(16,41), VX(0,38)
      vmov VX(16,42), VX(0,37)
      vmov VX(16,43), VX(0,36)
      vmov VX(16,44), VX(0,35)
      vmov VX(16,45), VX(0,34)
      vmov VX(16,46), VX(0,33)
      vmov VX(16,47), VX(0,32)
      vst HX(16++,32), (r0+=r2) REP 16
vc_rgb565_hflip_in_place_aligned_last_block:
      vst HX(16++,0), (r1+=r2) REP 16
      sub r1, r3
      addcmpble r0, r3, r1, vc_rgb565_hflip_in_place_aligned_loop
      b lr
.stabs "vc_rgb565_hflip_in_place_aligned:F1",36,0,0,vc_rgb565_hflip_in_place_aligned

;;; NAME
;;;    vc_rgb565_hflip_in_place_unaligned
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_hflip_in_place_unaligned(unsigned short *start, unsigned short *end, int pitch_bytes)
;;;
;;; FUNCTION
;;;    Flip image horizontally.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_hflip_in_place_unaligned,"text"
.globl vc_rgb565_hflip_in_place_unaligned
vc_rgb565_hflip_in_place_unaligned:
      mov r4, r1
      bmask r4, 5
      asr r4, 1            ; offset into block of 16 of src
      cbclr
      mov r3, 32

      ;; r4 is never 0 because that uses the "aligned" routine above
      vld HX(0++,32), (r1+32+=r2) REP 16
vc_rgb565_hflip_in_place_unaligned_loop:
      ;; for the last block this may load some garbage, but it will only appear in the
      ;; padding at the end of the dest image
      vld HX(0++,0)*, (r1+=r2) REP 16
      ;; VX addressing bug in core means we have to copy them out first before reversing them
      vmov VX(16,0++), VX(0,0++)+r4* REP 16
      vmov VX(32,0), VX(16,15)
      vmov VX(32,1), VX(16,14)
      vmov VX(32,2), VX(16,13)
      vmov VX(32,3), VX(16,12)
      vmov VX(32,4), VX(16,11)
      vmov VX(32,5), VX(16,10)
      vmov VX(32,6), VX(16,9)
      vmov VX(32,7), VX(16,8)
      vmov VX(32,8), VX(16,7)
      vmov VX(32,9), VX(16,6)
      vmov VX(32,10), VX(16,5)
      vmov VX(32,11), VX(16,4)
      vmov VX(32,12), VX(16,3)
      vmov VX(32,13), VX(16,2)
      vmov VX(32,14), VX(16,1)
      vmov VX(32,15), VX(16,0)
      ;; Load the block these will replace, and reverse them.
      vld HX(16++,32), (r0+=r2) REP 16
      ;; VX addressing bug means we have to reverse before copying back.
      vmov VX(32,32), VX(16,47)
      vmov VX(32,33), VX(16,46)
      vmov VX(32,34), VX(16,45)
      vmov VX(32,35), VX(16,44)
      vmov VX(32,36), VX(16,43)
      vmov VX(32,37), VX(16,42)
      vmov VX(32,38), VX(16,41)
      vmov VX(32,39), VX(16,40)
      vmov VX(32,40), VX(16,39)
      vmov VX(32,41), VX(16,38)
      vmov VX(32,42), VX(16,37)
      vmov VX(32,43), VX(16,36)
      vmov VX(32,44), VX(16,35)
      vmov VX(32,45), VX(16,34)
      vmov VX(32,46), VX(16,33)
      vmov VX(32,47), VX(16,32)
      ;; The second lot must be replaced to their unaligned locations before writing out.
      vmov VX(0,0++)+r4*, VX(32,32++) REP 16
      ;; Write previous block
      vst HX(0++,32)*, (r1+32+=r2) REP 16
      ;; If we're not done, write out the aligned block and continue.
      addcmpblt r0, r3, r1, vc_rgb565_hflip_in_place_unaligned_continue
      ;; We may still need to write out the aligned block if an even number of blocks was processed.
      ;; If an odd number of blocks was processed we mustn't write it out, as it would trash things.
      sub r0, 32
      addcmpblt r1, 0, r0, vc_rgb565_hflip_in_place_unaligned_done
      vst HX(32++,0), (r0+=r2) REP 16
vc_rgb565_hflip_in_place_unaligned_done:
      b lr
vc_rgb565_hflip_in_place_unaligned_continue:
      vst HX(32++,0), (r0+-32+=r2) REP 16
      cbadd2
      sub r1, r3
      addcmpb r0, 0, 0, vc_rgb565_hflip_in_place_unaligned_loop
      b lr
.stabs "vc_rgb565_hflip_in_place_unaligned:F1",36,0,0,vc_rgb565_hflip_in_place_unaligned

;;; NAME
;;;    vc_rgb565_transparent_blt_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transparent_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                           int src_pitch_bytes, int width, int transparent_colour)
;;;
;;; FUNCTION
;;;    Transparent blt of 16-deep stripe of dest array and src array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transparent_blt_16_rows,"text"
.globl vc_rgb565_transparent_blt_16_rows
vc_rgb565_transparent_blt_16_rows:
      stm r6-r7, lr, (--sp)

      ;; save the transparent colour
      vmov HX(62,0), r5

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      addcmpbeq r6, 0, 0, vc_rgb565_transparent_blt_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_transparent_blt_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_transparent_blt_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16

      ;; VX addressing bug means we have to copy the blocks out before processing...
      vmov HX(32++,0), VX(0,0++)+r4* REP 16 ; dest pixels
      vmov HX(32++,32), VX(16,0++)+r5* REP 16 ; src pixels
      vsub -, HX(32,32), HX(62,0) SETF
      vmov HX(32,0), HX(32,32) IFNZ
      vsub -, HX(33,32), HX(62,0) SETF
      vmov HX(33,0), HX(33,32) IFNZ
      vsub -, HX(34,32), HX(62,0) SETF
      vmov HX(34,0), HX(34,32) IFNZ
      vsub -, HX(35,32), HX(62,0) SETF
      vmov HX(35,0), HX(35,32) IFNZ
      vsub -, HX(36,32), HX(62,0) SETF
      vmov HX(36,0), HX(36,32) IFNZ
      vsub -, HX(37,32), HX(62,0) SETF
      vmov HX(37,0), HX(37,32) IFNZ
      vsub -, HX(38,32), HX(62,0) SETF
      vmov HX(38,0), HX(38,32) IFNZ
      vsub -, HX(39,32), HX(62,0) SETF
      vmov HX(39,0), HX(39,32) IFNZ
      vsub -, HX(40,32), HX(62,0) SETF
      vmov HX(40,0), HX(40,32) IFNZ
      vsub -, HX(41,32), HX(62,0) SETF
      vmov HX(41,0), HX(41,32) IFNZ
      vsub -, HX(42,32), HX(62,0) SETF
      vmov HX(42,0), HX(42,32) IFNZ
      vsub -, HX(43,32), HX(62,0) SETF
      vmov HX(43,0), HX(43,32) IFNZ
      vsub -, HX(44,32), HX(62,0) SETF
      vmov HX(44,0), HX(44,32) IFNZ
      vsub -, HX(45,32), HX(62,0) SETF
      vmov HX(45,0), HX(45,32) IFNZ
      vsub -, HX(46,32), HX(62,0) SETF
      vmov HX(46,0), HX(46,32) IFNZ
      vsub -, HX(47,32), HX(62,0) SETF
      vmov HX(47,0), HX(47,32) IFNZ
      ;; and now copy back
      vmov VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 32
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_transparent_blt_16_rows_loop

vc_rgb565_transparent_blt_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_transparent_blt_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_transparent_blt_16_rows_trailing_loop:
      vsub -, VX(16,0)+r5*, HX(62,0) SETF
      vmov VX(0,0)+r4*, VX(16,0)+r5* IFNZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_transparent_blt_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_transparent_blt_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_transparent_blt_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_transparent_blt_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_transparent_blt_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_transparent_blt_16_rows:F1",36,0,0,vc_rgb565_transparent_blt_16_rows

;;; NAME
;;;    vc_rgb565_transparent_blt_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transparent_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                          int src_pitch_bytes, int width, int nrows, int transparent_colour)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transparent_blt_n_rows,"text"
.globl vc_rgb565_transparent_blt_n_rows
vc_rgb565_transparent_blt_n_rows:
      stm r6-r12, lr, (--sp)

      ld r6, (sp+32)       ; transparent colour
      vmov HX(62,0), r6

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_transparent_blt_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_transparent_blt_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_transparent_blt_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16

      ;; VX addressing bug means we have to copy the blocks out before processing...
      vmov HX(32++,0), VX(0,0++)+r4* REP 16 ; dest pixels
      vmov HX(32++,32), VX(16,0++)+r5* REP 16 ; src pixels
      vsub -, HX(32,32), HX(62,0) SETF
      vmov HX(32,0), HX(32,32) IFNZ
      vsub -, HX(33,32), HX(62,0) SETF
      vmov HX(33,0), HX(33,32) IFNZ
      vsub -, HX(34,32), HX(62,0) SETF
      vmov HX(34,0), HX(34,32) IFNZ
      vsub -, HX(35,32), HX(62,0) SETF
      vmov HX(35,0), HX(35,32) IFNZ
      vsub -, HX(36,32), HX(62,0) SETF
      vmov HX(36,0), HX(36,32) IFNZ
      vsub -, HX(37,32), HX(62,0) SETF
      vmov HX(37,0), HX(37,32) IFNZ
      vsub -, HX(38,32), HX(62,0) SETF
      vmov HX(38,0), HX(38,32) IFNZ
      vsub -, HX(39,32), HX(62,0) SETF
      vmov HX(39,0), HX(39,32) IFNZ
      vsub -, HX(40,32), HX(62,0) SETF
      vmov HX(40,0), HX(40,32) IFNZ
      vsub -, HX(41,32), HX(62,0) SETF
      vmov HX(41,0), HX(41,32) IFNZ
      vsub -, HX(42,32), HX(62,0) SETF
      vmov HX(42,0), HX(42,32) IFNZ
      vsub -, HX(43,32), HX(62,0) SETF
      vmov HX(43,0), HX(43,32) IFNZ
      vsub -, HX(44,32), HX(62,0) SETF
      vmov HX(44,0), HX(44,32) IFNZ
      vsub -, HX(45,32), HX(62,0) SETF
      vmov HX(45,0), HX(45,32) IFNZ
      vsub -, HX(46,32), HX(62,0) SETF
      vmov HX(46,0), HX(46,32) IFNZ
      vsub -, HX(47,32), HX(62,0) SETF
      vmov HX(47,0), HX(47,32) IFNZ
      ;; and now copy back
      vmov VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_transparent_blt_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_transparent_blt_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_transparent_blt_n_rows_loop

vc_rgb565_transparent_blt_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_transparent_blt_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_transparent_blt_n_rows_trailing_loop:
      vsub -, VX(16,0)+r5*, HX(62,0) SETF
      vmov VX(0,0)+r4*, VX(16,0)+r5* IFNZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_transparent_blt_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_transparent_blt_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_transparent_blt_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_transparent_blt_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_transparent_blt_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_transparent_blt_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_transparent_blt_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_transparent_blt_n_rows_save3

vc_rgb565_transparent_blt_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_transparent_blt_n_rows:F1",36,0,0,vc_rgb565_transparent_blt_n_rows

;;; NAME
;;;    vc_rgb565_overwrite_blt_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_overwrite_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                           int src_pitch_bytes, int width, int overwrite_colour)
;;;
;;; FUNCTION
;;;    Transparent blt of 16-deep stripe of dest array and src array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_overwrite_blt_16_rows,"text"
.globl vc_rgb565_overwrite_blt_16_rows
vc_rgb565_overwrite_blt_16_rows:
      stm r6-r7, lr, (--sp)

      ;; save the overwrite colour
      vmov HX(62,0), r5

      mov r6, r4
      mov r7, r4
      asr r6, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      addcmpbeq r6, 0, 0, vc_rgb565_overwrite_blt_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_overwrite_blt_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_overwrite_blt_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16

      ;; VX addressing bug means we have to copy the blocks out before processing...
      vmov HX(32++,0), VX(0,0++)+r4* REP 16 ; dest pixels
      vmov HX(32++,32), VX(16,0++)+r5* REP 16 ; src pixels
      vsub -, HX(32,0), HX(62,0) SETF
      vmov HX(32,0), HX(32,32) IFZ
      vsub -, HX(33,0), HX(62,0) SETF
      vmov HX(33,0), HX(33,32) IFZ
      vsub -, HX(34,0), HX(62,0) SETF
      vmov HX(34,0), HX(34,32) IFZ
      vsub -, HX(35,0), HX(62,0) SETF
      vmov HX(35,0), HX(35,32) IFZ
      vsub -, HX(36,0), HX(62,0) SETF
      vmov HX(36,0), HX(36,32) IFZ
      vsub -, HX(37,0), HX(62,0) SETF
      vmov HX(37,0), HX(37,32) IFZ
      vsub -, HX(38,0), HX(62,0) SETF
      vmov HX(38,0), HX(38,32) IFZ
      vsub -, HX(39,0), HX(62,0) SETF
      vmov HX(39,0), HX(39,32) IFZ
      vsub -, HX(40,0), HX(62,0) SETF
      vmov HX(40,0), HX(40,32) IFZ
      vsub -, HX(41,0), HX(62,0) SETF
      vmov HX(41,0), HX(41,32) IFZ
      vsub -, HX(42,0), HX(62,0) SETF
      vmov HX(42,0), HX(42,32) IFZ
      vsub -, HX(43,0), HX(62,0) SETF
      vmov HX(43,0), HX(43,32) IFZ
      vsub -, HX(44,0), HX(62,0) SETF
      vmov HX(44,0), HX(44,32) IFZ
      vsub -, HX(45,0), HX(62,0) SETF
      vmov HX(45,0), HX(45,32) IFZ
      vsub -, HX(46,0), HX(62,0) SETF
      vmov HX(46,0), HX(46,32) IFZ
      vsub -, HX(47,0), HX(62,0) SETF
      vmov HX(47,0), HX(47,32) IFZ
      ;; and now copy back
      vmov VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 32
      cbadd2
      addcmpbne r6, -1, 0, vc_rgb565_overwrite_blt_16_rows_loop

vc_rgb565_overwrite_blt_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_overwrite_blt_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_overwrite_blt_16_rows_trailing_loop:
      vsub -, VX(0,0)+r4*, HX(62,0) SETF
      vmov VX(0,0)+r4*, VX(16,0)+r5* IFZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_overwrite_blt_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_overwrite_blt_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_overwrite_blt_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_overwrite_blt_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_overwrite_blt_16_rows_end:
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_overwrite_blt_16_rows:F1",36,0,0,vc_rgb565_overwrite_blt_16_rows

;;; NAME
;;;    vc_rgb565_overwrite_blt_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_overwrite_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                          int src_pitch_bytes, int width, int nrows, int overwrite_colour)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_overwrite_blt_n_rows,"text"
.globl vc_rgb565_overwrite_blt_n_rows
vc_rgb565_overwrite_blt_n_rows:
      stm r6-r12, lr, (--sp)

      ld r6, (sp+32)       ; transparent colour
      vmov HX(62,0), r6

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_overwrite_blt_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_overwrite_blt_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_overwrite_blt_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16

      ;; VX addressing bug means we have to copy the blocks out before processing...
      vmov HX(32++,0), VX(0,0++)+r4* REP 16 ; dest pixels
      vmov HX(32++,32), VX(16,0++)+r5* REP 16 ; src pixels
      vsub -, HX(32,0), HX(62,0) SETF
      vmov HX(32,0), HX(32,32) IFZ
      vsub -, HX(33,0), HX(62,0) SETF
      vmov HX(33,0), HX(33,32) IFZ
      vsub -, HX(34,0), HX(62,0) SETF
      vmov HX(34,0), HX(34,32) IFZ
      vsub -, HX(35,0), HX(62,0) SETF
      vmov HX(35,0), HX(35,32) IFZ
      vsub -, HX(36,0), HX(62,0) SETF
      vmov HX(36,0), HX(36,32) IFZ
      vsub -, HX(37,0), HX(62,0) SETF
      vmov HX(37,0), HX(37,32) IFZ
      vsub -, HX(38,0), HX(62,0) SETF
      vmov HX(38,0), HX(38,32) IFZ
      vsub -, HX(39,0), HX(62,0) SETF
      vmov HX(39,0), HX(39,32) IFZ
      vsub -, HX(40,0), HX(62,0) SETF
      vmov HX(40,0), HX(40,32) IFZ
      vsub -, HX(41,0), HX(62,0) SETF
      vmov HX(41,0), HX(41,32) IFZ
      vsub -, HX(42,0), HX(62,0) SETF
      vmov HX(42,0), HX(42,32) IFZ
      vsub -, HX(43,0), HX(62,0) SETF
      vmov HX(43,0), HX(43,32) IFZ
      vsub -, HX(44,0), HX(62,0) SETF
      vmov HX(44,0), HX(44,32) IFZ
      vsub -, HX(45,0), HX(62,0) SETF
      vmov HX(45,0), HX(45,32) IFZ
      vsub -, HX(46,0), HX(62,0) SETF
      vmov HX(46,0), HX(46,32) IFZ
      vsub -, HX(47,0), HX(62,0) SETF
      vmov HX(47,0), HX(47,32) IFZ
      ;; and now copy back
      vmov VX(0,0++)+r4*, HX(32++,0) REP 16

      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_overwrite_blt_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_overwrite_blt_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_overwrite_blt_n_rows_loop

vc_rgb565_overwrite_blt_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_overwrite_blt_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Copy r7 columns from one to t'other.

vc_rgb565_overwrite_blt_n_rows_trailing_loop:
      vsub -, VX(0,0)+r4*, HX(62,0) SETF
      vmov VX(0,0)+r4*, VX(16,0)+r5* IFZ
      add r4, 1
      add r5, 1
      addcmpbne r7, -1, 0, vc_rgb565_overwrite_blt_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_overwrite_blt_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_overwrite_blt_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_overwrite_blt_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_overwrite_blt_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_overwrite_blt_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_overwrite_blt_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_overwrite_blt_n_rows_save3

vc_rgb565_overwrite_blt_n_rows_end:
      ldm r6-r12, pc, (sp++)
.stabs "vc_rgb565_overwrite_blt_n_rows:F1",36,0,0,vc_rgb565_overwrite_blt_n_rows

;;; NAME
;;;    vc_rgb565_masked_blt_16_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_masked_blt_16_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                      int src_pitch_bytes, int width,
;;;                                      char *mask, int mask_pitch_bytes, int mask_offset, int invert)
;;;
;;; FUNCTION
;;;    Transparent blt of 16-deep stripe of dest array and src array.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_masked_blt_16_rows,"text"
.globl vc_rgb565_masked_blt_16_rows
vc_rgb565_masked_blt_16_rows:
      stm r6-r15, lr, (--sp)

      mov r6, r5           ; mask
      ld r7, (sp+44)       ; mask_pitch_bytes
      ld r8, (sp+48)       ; mask_offset
      ld r5, (sp+52)       ; invert
      vmov HX(63,32), r5

      mov r14, r4
      mov r15, r4
      asr r14, 4           ; number of 16-wide blocks
      bmask r15, 4         ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block
      mov r9, r5                    ; load up first mask block
      mov r5, r7
      vldleft HX(32++,0), (r6+=r5) REP 16
      veor VX(32,0), VX(32,0), HX(63,32)   ; takes care of invert
      mov r5, r9
      vbitplanes HX(32++,0), HX(32++,0) REP 16
      vand HX(32++,0), HX(32++,0), 1 REP 16

      addcmpbeq r14, 0, 0, vc_rgb565_masked_blt_16_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_masked_blt_16_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_masked_blt_16_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Load next mask block.
      mov r9, r5
      mov r5, r7
      vldleft HX(32++,32)*, (r6+2+=r5) REP 16
      veor VX(32,32)*, VX(32,32)*, HX(63,32)   ; takes care of invert
      mov r5, r9
      vbitplanes HX(32++,32)*, HX(32++,32)* REP 16
      vand HX(32++,32)*, HX(32++,32)*, 1 REP 16

      ;; VX addressing bug means we don't do things the obvious way...
      ;; Put src-dest into HX(48++,0) and multiply by the mask. We then add these to the dest pixels.
      vsub VX(48,0++), VX(16,0++)+r5*, VX(0,0++)+r4* REP 16 ; src-dest
      mov r9, r5
      mov r5, r8
      vmul VX(48,0++), VX(48,0++), VX(32,0++)+r5* REP 16 ; (src-dest)*mask
      mov r5, r9
      vadd VX(0,0++)+r4*, VX(0,0++)+r4*, VX(48,0++) REP 16 ; dest = dest + (src-dest)*mask

      ;; Write out the previous block to dest
      vst HX(0++,0)*, (r0+=r2) REP 16

      add r0, 32
      add r1, 32
      add r6, 2
      cbadd2
      addcmpbne r14, -1, 0, vc_rgb565_masked_blt_16_rows_loop

vc_rgb565_masked_blt_16_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r15, 0, 0, vc_rgb565_masked_blt_16_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; And of mask. Note that we are done with r1 now.
      mov r1, r7
      vldleft HX(32++,32)*, (r6+2+=r1) REP 16
      veor VX(32,32)*, VX(32,32)*, HX(63,32)   ; takes care of invert
      vbitplanes HX(32++,32)*, HX(32++,32)* REP 16
      vand HX(32++,32)*, HX(32++,32)*, 1 REP 16
      ;; Copy r15 columns from one to t'other.

      mov r1, r8           ; now mask_offset
vc_rgb565_masked_blt_16_rows_trailing_loop:
      vsub HX(48,0), VX(16,0)+r5*, VX(0,0)+r4* ; src-dest
      vmul HX(48,0), HX(48,0), VX(32,0)+r1* ; (src-dest)*mask
      vadd VX(0,0)+r4*, VX(0,0)+r4*, HX(48,0) ; dest = (src-dest)*mask
      add r4, 1
      add r5, 1
      add r1, 1
      addcmpbne r15, -1, 0, vc_rgb565_masked_blt_16_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_masked_blt_16_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      vst HX(0++,0)*, (r0+=r2) REP 16
      add r0, 32
      cbadd2

vc_rgb565_masked_blt_16_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_masked_blt_16_rows_end
      ;; Must write out the last block.
      vst HX(0++,0)*, (r0+=r2) REP 16

vc_rgb565_masked_blt_16_rows_end:
      ldm r6-r15, pc, (sp++)
.stabs "vc_rgb565_masked_blt_16_rows:F1",36,0,0,vc_rgb565_masked_blt_16_rows

;;; NAME
;;;    vc_rgb565_masked_blt_n_rows
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_masked_blt_n_rows(unsigned short *dest, unsigned short *src, int dest_pitch_bytes,
;;;                                     int src_pitch_bytes, int width, int nrows,
;;;                                       unsigned char *mask, int mask_pitch_bytes, int mask_offset, int invert)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_masked_blt_n_rows,"text"
.globl vc_rgb565_masked_blt_n_rows
vc_rgb565_masked_blt_n_rows:
      stm r6-r15, lr, (--sp)

      ld r13, (sp+44)         ; mask
      ld r14, (sp+48)         ; mask_pitch_bytes
      ld r15, (sp+52)         ; mask_offset

      mov r10, r5
      shl r10, 6
      mov r11, 64          ; r6 will count in steps of r11 up to r10

      ld r5, (sp+56)       ; invert
      vmov HX(63,32), r5

      mov r8, r4
      mov r7, r4
      asr r8, 4            ; number of 16-wide blocks
      bmask r7, 4          ; number of trailing columns

      mov r4, r0
      bmask r4, 5
      asr r4, 1            ; dest offset (this number was in bytes)
      mov r5, r1
      bmask r5, 5
      asr r5, 1            ; src offset (this number was in bytes)

      ;; The only totally free register is r9. However, we can also use r6
      ;; in most places as it is always set to 0 at the start of the loops
      ;; in which it is used.

.if _VC_VERSION > 2
      bic r0, 31
      bic r1, 31
.endif
      cbclr
      ;; It's probably not worth the hassle of carefully *loading* nrows...
      vld HX(0++,0), (r0+=r2) REP 16   ; load up first destination block
      vld HX(16++,0), (r1+=r3) REP 16  ; load up first source block
      mov r9, r5                    ; load up first mask block
      mov r5, r14
      mov r6, r13
      vldleft HX(32++,0), (r6+=r5) REP 16
      veor VX(32,0), VX(32,0), HX(63,32)  ; takes care of invert
      mov r5, r9
      vbitplanes HX(32++,0), HX(32++,0) REP 16
      vand HX(32++,0), HX(32++,0), 1 REP 16

      ;; r6 is free for row counter, r12 will save the value of r0
      mov r12, r0

      addcmpbeq r8, 0, 0, vc_rgb565_masked_blt_n_rows_no_blocks
      ;; There is at least one 16-wide block.

vc_rgb565_masked_blt_n_rows_loop:
      ;; Always load next dest block.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
vc_rgb565_masked_blt_n_rows_not_last:
      ;; Load next src block.
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; Load next mask block.
      mov r9, r5
      mov r5, r14
      mov r6, r13
      vldleft HX(32++,32)*, (r6+2+=r5) REP 16
      veor VX(32,32)*, VX(32,32)*, HX(63,32)  ; takes care of invert
      mov r5, r9
      vbitplanes HX(32++,32)*, HX(32++,32)* REP 16
      vand HX(32++,32)*, HX(32++,32)*, 1 REP 16

      ;; VX addressing bug means we don't do things the obvious way...
      ;; Put src-dest into HX(48++,0) and multiply by the mask. We then add these to the dest pixels.
      vsub VX(48,0++), VX(16,0++)+r5*, VX(0,0++)+r4* REP 16 ; src-dest
      mov r6, r15
      vmul VX(48,0++), VX(48,0++), VX(32,0++)+r6* REP 16 ; (src-dest)*mask
      vadd VX(0,0++)+r4*, VX(0,0++)+r4*, VX(48,0++) REP 16 ; dest = dest + (src-dest)*mask

      ;; Write out the previous block to dest
      mov r6, 0
vc_rgb565_masked_blt_n_rows_save1:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_blt_n_rows_save1

      add r12, 32
      mov r0, r12
      add r1, 32
      add r13, 2
      cbadd2
      addcmpbne r8, -1, 0, vc_rgb565_masked_blt_n_rows_loop

vc_rgb565_masked_blt_n_rows_no_blocks:
      ;; There may still be a few columns left to do.
      addcmpbeq r7, 0, 0, vc_rgb565_masked_blt_n_rows_no_trailing

      ;; It is possible that we could need extra blocks of src/dest.
      vld HX(0++,32)*, (r0+32+=r2) REP 16
      vld HX(16++,32)*, (r1+32+=r3) REP 16
      ;; And of mask. Note that we are done with r1 and r3 now.
      mov r1, r13
      mov r3, r14
      vldleft HX(32++,32)*, (r1+2+=r3) REP 16
      veor VX(32,32)*, VX(32,32)*, HX(63,32)  ; takes care of invert
      vbitplanes HX(32++,32)*, HX(32++,32)* REP 16
      vand HX(32++,32)*, HX(32++,32)*, 1 REP 16
      ;; Copy r7 columns from one to t'other.

      mov r1, r15          ; now mask_offset
vc_rgb565_masked_blt_n_rows_trailing_loop:
;;    vsub -, VX(0,0)+r4*, HX(62,0) SETF
;;    vmov VX(0,0)+r4*, VX(16,0)+r5* IFZ

      vsub HX(48,0), VX(16,0)+r5*, VX(0,0)+r4* ; src-dest
      vmul HX(48,0), HX(48,0), VX(32,0)+r1* ; (src-dest)*mask
      vadd VX(0,0)+r4*, VX(0,0)+r4*, HX(48,0) ; dest = (src-dest)*mask
      add r4, 1
      add r5, 1
      add r1, 1
      addcmpbne r7, -1, 0, vc_rgb565_masked_blt_n_rows_trailing_loop

      ;; == 16 is ok because it's been incremented since it was written to
      addcmpble r4, 0, 16, vc_rgb565_masked_blt_n_rows_no_trailing
      ;; if r4 spilled into the next block of registers (i.e. 16 or greater) then
      ;; there's an extra block to write out now.
      mov r6, 0
vc_rgb565_masked_blt_n_rows_save2:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_blt_n_rows_save2
      add r12, 32
      mov r0, r12
      cbadd2

vc_rgb565_masked_blt_n_rows_no_trailing:
      addcmpbeq r4, 0, 0, vc_rgb565_masked_blt_n_rows_end
      ;; Must write out the last block.
      mov r6, 0
vc_rgb565_masked_blt_n_rows_save3:
      vst HX(0,0)+r6*, (r0)
      add r0, r2
      addcmpblt r6, r11, r10, vc_rgb565_masked_blt_n_rows_save3

vc_rgb565_masked_blt_n_rows_end:
      ldm r6-r15, pc, (sp++)
.stabs "vc_rgb565_masked_blt_n_rows:F1",36,0,0,vc_rgb565_masked_blt_n_rows

;;; NAME
;;;    vc_rgb565_convert_48bpp
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_convert_48bpp(unsigned char *dest, unsigned short *src, int dest_pitch_bytes, int src_pitch_bytes)
;;;
;;; FUNCTION
;;;    Copy from src to dest, converting to 48bpp as we go. Converts one block of 16x16 pixels.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_convert_48bpp,"text"
.globl vc_rgb565_convert_48bpp
vc_rgb565_convert_48bpp:
        vld VX(0,0++), (r1+=r3) REP 16

        vand HX(16++,0), HX(0++,0), 0x07e0 REP 16 ; Mask green bits (middle 6)
        vand HX(32++,0), HX(0++,0), 31 REP 16     ; Mask blue bits (bottom 5)

        vand HX(0,32), H(0,16), 248 ; Red (mask top 5 from high byte)
        vlsr HX(1,32), HX(16,0), 3  ; Green
        vshl HX(2,32), HX(32,0), 3  ; Blue

        vand HX(3,32), H(1,16), 248
        vlsr HX(4,32), HX(17,0), 3
        vshl HX(5,32), HX(33,0), 3

        vand HX(6,32), H(2,16), 248
        vlsr HX(7,32), HX(18,0), 3
        vshl HX(8,32), HX(34,0), 3

        vand HX(9,32), H(3,16), 248
        vlsr HX(10,32), HX(19,0), 3
        vshl HX(11,32), HX(35,0), 3

        vand HX(12,32), H(4,16), 248
        vlsr HX(13,32), HX(20,0), 3
        vshl HX(14,32), HX(36,0), 3

        vand HX(15,32), H(5,16), 248
        vlsr HX(16,32), HX(21,0), 3
        vshl HX(17,32), HX(37,0), 3

        vand HX(18,32), H(6,16), 248
        vlsr HX(19,32), HX(22,0), 3
        vshl HX(20,32), HX(38,0), 3

        vand HX(21,32), H(7,16), 248
        vlsr HX(22,32), HX(23,0), 3
        vshl HX(23,32), HX(39,0), 3

        vand HX(24,32), H(8,16), 248
        vlsr HX(25,32), HX(24,0), 3
        vshl HX(26,32), HX(40,0), 3

        vand HX(27,32), H(9,16), 248
        vlsr HX(28,32), HX(25,0), 3
        vshl HX(29,32), HX(41,0), 3

        vand HX(30,32), H(10,16), 248
        vlsr HX(31,32), HX(26,0), 3
        vshl HX(32,32), HX(42,0), 3

        vand HX(33,32), H(11,16), 248
        vlsr HX(34,32), HX(27,0), 3
        vshl HX(35,32), HX(43,0), 3

        vand HX(36,32), H(12,16), 248
        vlsr HX(37,32), HX(28,0), 3
        vshl HX(38,32), HX(44,0), 3

        vand HX(39,32), H(13,16), 248
        vlsr HX(40,32), HX(29,0), 3
        vshl HX(41,32), HX(45,0), 3

        vand HX(42,32), H(14,16), 248
        vlsr HX(43,32), HX(30,0), 3
        vshl HX(44,32), HX(46,0), 3

        vand HX(45,32), H(15,16), 248
        vlsr HX(46,32), HX(31,0), 3
        vshl HX(47,32), HX(47,0), 3

        vst VX(0,32++), (r0+=r2) REP 16
        vst VX(16,32++), (r0+32+=r2) REP 16
        vst VX(32,32++), (r0+64+=r2) REP 16

        b lr
.stabs "vc_rgb565_convert_48bpp:F1",36,0,0,vc_rgb565_convert_48bpp

.FORGET

; void vc_rgb565_runlength(unsigned short *data,unsigned short *out_buffer);
.section .text$vc_rgb565_runlength,"text"
.globl vc_rgb565_runlength
vc_rgb565_runlength:
; Format is byte for run, followed by two bytes for 16bit colour (little-endian), 0 byte to end
   ldb   r2,(r0++)
   addcmpbne   r2,0,0,moredata
   b  lr
moredata:
   ldb   r4,(r0++)
   ldb   r5,(r0++)
loopwrite:
   stb   r4,(r1++)
   stb   r5,(r1++)
   addcmpbgt   r2,-1,0,loopwrite
   b  vc_rgb565_runlength
.stabs "vc_rgb565_runlength:F1",36,0,0,vc_rgb565_runlength

.FORGET

; void vc_runlength8(unsigned char *data,unsigned short *out_buffer);
.section .text$vc_runlength8,"text"
.globl vc_runlength8
vc_runlength8:
; Format is byte for run, followed by byte for 8bit colour (little-endian), 0 byte to end
   ldb   r2,(r0++)
   addcmpbne   r2,0,0,moredata
   b  lr
moredata:
   ldb   r4,(r0++)
loopwrite:
   stb   r4,(r1++)
   addcmpbgt   r2,-1,0,loopwrite
   b  vc_runlength8
.stabs "vc_runlength8:F1",36,0,0,vc_runlength8

.FORGET

;;; NAME
;;;    vc_rgb565_from_yuv_downsampled
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_from_yuv_downsampled(unsigned char *Y_ptr, unsigned char *U_ptr, unsigned char *V_ptr,
;;;                                        int src->pitch, unsigned short *rgb, int dest->pitch)
;;;
;;; FUNCTION
;;;    Convert from YUV to RGB565 downsampling by 2 in each direction.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_from_yuv_downsampled,"text"
.globl vc_rgb565_from_yuv_downsampled
vc_rgb565_from_yuv_downsampled:
      stm r6-r7, lr, (--sp)
      ;; only need r4 and r5 after the call to vc_image_load_yuv_downsampled
      mov r6, r4
      mov r7, r5
      bl vc_image_load_yuv_downsampled
      ;; Now convert to RGB
      bl vc_yuv16_common
      ;;  Combine G and RB values and write out.
      vor HX(0++,0), HX(16++,32), HX(0++,32) REP 16
      mov r0, r7
      vst HX(0++,0), (r6+=r0) REP 16
      ldm r6-r7, pc, (sp++)
.stabs "vc_rgb565_from_yuv_downsampled:F1",36,0,0,vc_rgb565_from_yuv_downsampled

.FORGET

;extern void vc_3d32_to_rgb565(short *lcd_buffer             r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap)                   r3
.section .text$vc_3d32_to_rgb565,"text"
.globl vc_3d32_to_rgb565
vc_3d32_to_rgb565:
       stm      r6-r6,lr,(--sp)
       mov      r5,0
       asr      r2,4
copyloop:
       vld      HX(10,0),(r1)
       add      r1,64
       vmov     -,H(10,0) CLRA ACC
       vlookupm HX(0,0),(r3)
; Now split into colour components and apply light level
   vlsr  HX(3,0),HX(0,0),5    ; 6 bits of green
   vand  HX(3,0),HX(3,0),63
   vlsr  HX(2,0),HX(0,0),11   ; 5 bits of red
   vand  HX(4,0),HX(0,0),31   ; blue

   vmulm HX(2,0),HX(2,0),H(10,16) ; red
   vmulm HX(3,0),HX(3,0),H(10,16) ; green
   vmulm HX(4,0),HX(4,0),H(10,16) CLRA ACC ; blue
   vshl  -,HX(3,0),5 ACC
   vshl  HX(0,0),HX(2,0),11 ACC

       vst      HX(0,0),(r0)
       add      r0,32
       addcmpblt r5,1,r2,copyloop
       ldm      r6-r6,pc,(sp++)

.stabs "Colourmap lookup:F19",36,0,1,vc_3d32_to_rgb565


.FORGET

;extern void vc_3d32b_to_rgb565(short *lcd_buffer            r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap)                   r3
; Strips out the z component

.section .text$vc_3d32b_to_rgb565,"text"
.globl vc_3d32b_to_rgb565
vc_3d32b_to_rgb565:
       stm      r6-r6,lr,(--sp)
       mov      r5,0
       asr      r2,4
copyloop:
       vld      HX(0,0),(r1)
       vst      HX(0,0),(r0)
       add      r1,64
       add      r0,32
       addcmpblt r5,1,r2,copyloop
       ldm      r6-r6,pc,(sp++)

.stabs "vc_3d32b_to_rgb565:F19",36,0,1,vc_3d32b_to_rgb565


.FORGET

;extern void vc_3d32b_to_rgb565_anti2(short *lcd_buffer            r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap)                   r3
; Strips out the z component

.section .text$vc_3d32b_to_rgb565_anti2,"text"
.globl vc_3d32b_to_rgb565_anti2
vc_3d32b_to_rgb565_anti2:
   stm      r6-r6,lr,(--sp)
   mov      r5,0
   asr      r2,4
copyloop:
   vld      HX(2, 0),(r1)
   vld      HX(2,32),(r1+64)
   vfrom32l HX(0, 0), HX(2,0), HX(2,32)
   vfrom32h HX(1, 0), HX(2,0), HX(2,32)

   vlsr  HX( 8++,0),HX(0++,0), 5 REP 2  ; 6 bits of green
   vand  HX( 8++,0),HX(8++,0),63 REP 2
   vlsr  HX(10++,0),HX(0++,0),11 REP 2  ; 5 bits of red
   vand  HX(12++,0),HX(0++,0),31 REP 2  ; 6 bits of blue

   vadd HX(16,0), HX( 8,0), HX( 9,0) ; green
   vadd HX(17,0), HX(10,0), HX(11,0) ; red
   vadd HX(18,0), HX(12,0), HX(13,0) ; blue

   vlsr HX(16++,0), HX(16++,0), 1 REP 4 ; one wasted

   vshl -, HX(16,0), 5 CLRA ACC   ; 6 bits of green
   vshl -, HX(17,0),11 ACC        ; 5 bits of red
   vshl HX(0,0), HX(18,0), 0 ACC  ; 5 bits of blue

   vst      HX(0,0),(r0)
   add      r1,128
   add      r0,32
   addcmpblt r5,1,r2,copyloop
   ldm      r6-r6,pc,(sp++)

.stabs "vc_3d32b_to_rgb565_anti2:F19",36,0,1,vc_3d32b_to_rgb565


.FORGET

;extern void vc_3d32_to_rgb565_anti2(short *lcd_buffer             r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap)                   r3
; Combines the values from two pixels when computing the result
.section .text$vc_3d32_to_rgb565_anti2,"text"
.globl vc_3d32_to_rgb565_anti2
vc_3d32_to_rgb565_anti2:
       stm      r6-r7,lr,(--sp)
       mov      r5,0
       mov     r7,r2 ; copy of r2 to be used for the last store pass
       bmask   r7,8 ; number of stores in last stage
       asr      r2,8

blockloop:
   mov   r4,16 ; Gather together 16 pairs
   mov   r5,0
copyloop:
       vld      HX(0,32),(r1)
       add      r1,64
       vmov     -,H(0,32) CLRA ACC
       vlookupm HX(1,0),(r3)
; Now split into colour components and apply light level
   vlsr  HX(32,0)+r5,HX(1,0),5    ; 6 bits of green
   vand  HX(32,0)+r5,HX(32,0)+r5,63
   vlsr  HX(16,0)+r5,HX(1,0),11   ; 5 bits of red
   vand  HX(48,0)+r5,HX(1,0),31   ; blue

   vmulm HX(16,0)+r5,HX(16,0)+r5,H(0,48) ; red
   vmulm HX(32,0)+r5,HX(32,0)+r5,H(0,48) ; green
   vmulm HX(48,0)+r5,HX(48,0)+r5,H(0,48) ; blue
;   vshl  -,HX(32,0)+r5,5 ACC
;   vshl  HX(0,0)+r5,HX(16,0)+r5,11 ACC

; Now do second pixels
   vld       HX(0,32),(r1)
       add      r1,64
       vmov     -,H(0,32) CLRA ACC
       vlookupm HX(1,0),(r3)
; Now split into colour components and apply light level
   vlsr  HX(32,32)+r5,HX(1,0),5    ; 6 bits of green
   vand  HX(32,32)+r5,HX(32,32)+r5,63
   vlsr  HX(16,32)+r5,HX(1,0),11   ; 5 bits of red
   vand  HX(48,32)+r5,HX(1,0),31   ; blue

   vmulm HX(16,32)+r5,HX(16,32)+r5,H(0,48) ; red
   vmulm HX(32,32)+r5,HX(32,32)+r5,H(0,48) ; green
   vmulm HX(48,32)+r5,HX(48,32)+r5,H(0,48) ; blue
;   vshl  -,HX(32,0)+r5,5 ACC
;   vshl  HX(0,0)+r5,HX(16,0)+r5,11 ACC

       add   r5,64
       addcmpbgt   r4,-1,0,copyloop

       ; Now combine colours (in two passes)
   mov   r5,0
   mov   r4,16
   mov   r6,0
colourloop:
   vadd  HX(0,0),VX(16,0)+r5,VX(16,1)+r5  ;red
   vadd  HX(1,0),VX(32,0)+r5,VX(32,1)+r5  ;green
   vadd  HX(2,0),VX(48,0)+r5,VX(48,1)+r5  ;blue
   vlsr  HX(0++,0),HX(0++,0),1 REP 2
   vlsr  -,HX(2,0),1 CLRA ACC
   vshl  -,HX(1,0),5 ACC
   vshl  VX(16,0)+r6,HX(0,0),11 ACC
   add   r6,1
       addcmpblt   r5,2,r4,colourloop
   mov   r5,0
   mov   r4,16
colourloop2:
   vadd  HX(0,0),VX(16,32)+r5,VX(16,33)+r5  ;red
   vadd  HX(1,0),VX(32,32)+r5,VX(32,33)+r5  ;green
   vadd  HX(2,0),VX(48,32)+r5,VX(48,33)+r5  ;blue
   vlsr  HX(0++,0),HX(0++,0),1 REP 2
   vlsr  -,HX(2,0),1 CLRA ACC
   vshl  -,HX(1,0),5 ACC
   vshl  VX(16,0)+r6,HX(0,0),11 ACC
   add   r6,1
       addcmpblt   r5,2,r4,colourloop2

       ; Now store out
       mov   r5,32
       addcmpbgt r2,-1,0,store256

       ; the first time we fall through we have r2 as 0 and we need to store 256
       addcmpbeq r2,0,0,firstfall
       ; the second time we only need to save r7...round down to multiple of 16
       asr r7,4
       mov r1,0 ; index into VRF
loopstorelast:
       vst HX(16,0)+r1,(r0)
       add r0,32
       add r1,64
       addcmpbgt r7,-1,0,loopstorelast
       b end

store256:
       vst      HX(16++,0),(r0+=r5) REP 16
       add      r0,512
       b        blockloop   ; go round one final time

firstfall:
       vst      HX(16++,0),(r0+=r5) REP 16
       add      r0,512
       ; We only need to do more if r7 is nonzero
       addcmpbgt r7,0,0,blockloop
end:
       ldm      r6-r7,pc,(sp++)

.stabs "vc_3d32_to_rgb565_anti2:F19",36,0,1,vc_3d32_to_rgb565_anti2


.FORGET

;extern void vc_palette8torgb565(unsigned char *lcd_buffer     r0
;            ,unsigned char *screen r1
;            ,int dest_pitch        r2
;            ,int width             r3
;            ,short *cmap           r4
;            ,int src_pitch)        r5
;
; Uses 16 bit lookup and converts back to 16 bit colour
; Applies conversion to a 16 deep strip

.section .text$vc_palett8_to_rgb565,"text"
.globl vc_palette8_to_rgb565
vc_palette8_to_rgb565:
   stm   r6-r8,lr,(--sp)
   mov   r8,r3  ; !!!!SPL was hard coded to 176, caused PEND to break!!!!
   add   r8, 15 ; round up to multiple of 16
   asr   r8,4   ; Numbers of 16 blocks across to process
   mov   r6,r2
rowloop:
   vldleft   H(0++,0),(r1+=r5) REP 16
;   vldright   H(0++,0),(r1+=r5) REP 16
   mov   r3,0
   mov   r7,16
downloop:
   vmov  HX(0,0)+r3,H(0,0)+r3 CLRA ACC
   vlookupm HX(16,0)+r3,(r4)
.ifndef __BCM2707A0__
.pragma offwarn
   v16add -,VX(16,0),0
.pragma popwarn
.endif
   add   r3,64
   addcmpbgt r7,-1,0,downloop

   vst  HX(16++,0),(r0+=r6) REP 16
   add   r1,16
   add   r0,32
   addcmpbgt   r8,-1,0,rowloop
   ldm   r6-r8,pc,(sp++)

.stabs "vc_palette8_to_rgb565:F19",36,0,1,vc_palette8_to_rgb565


.FORGET
; Decompresses the 16 by 16 16-bit compressed bytes in <data> into HX(0,0).  Uses blocks HX(0,0),HX(16,0),HX(0,32)
; Format is 16 background colours,
; 16 foreground colours,
; 16 16-bit numbers describing interpolators for the top 8 rows
; 16 16-bit numbers describing interpolators for the bottom 8 rows
; Interpolators are 2 bit numbers and blend from background to foreground.
;
; Takes roughly 1 cycle per decompressed 16-bit pixel
;
; Saves the decompressed 16 by 16 block into <out_buffer> with spacing <pitch>
;
; Images can be compressed via vc_image/enc4by4.m
;
; void vc_4by4_decompress(unsigned short *data,unsigned short *out_buffer,int pitch);
.section .text$vc_4by4_decompress,"text"
.globl vc_4by4_decompress
vc_4by4_decompress:

; 4CC format deals with 4 by 4 blocks with 2 bits of interpolator, and a foreground and a background colour
   vldleft HX(0,0),(r0)     ; Background
   vldleft HX(1,0),(r0+32)  ; Foreground
   vldleft HX(2,0),(r0+64)  ; Interpolators
   vldleft HX(3,0),(r0+96)  ; Interpolators
;   vldright HX(0,0),(r0)     ; Background
;   vldright HX(1,0),(r0+32)  ; Foreground
;   vldright HX(2,0),(r0+64)  ; Interpolators
;   vldright HX(3,0),(r0+96)  ; Interpolators

   vand HX(4,0),HX(0,0),31 ; blue0
   vlsr HX(5,0),HX(0,0),5
   vand HX(5,0),HX(5,0),63 ; green0
   vlsr HX(6,0),HX(0,0),11 ; red0

   vand HX(7,0),HX(1,0),31 ; blue1
   vlsr HX(8,0),HX(1,0),5
   vand HX(8,0),HX(8,0),63 ; green1
   vlsr HX(9,0),HX(1,0),11 ; red1

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   vshl HX(10,0),HX(4,0),1
   vadd HX(10,0),HX(10,0),HX(7,0)
   vmulhd.su HX(10,0),HX(10,0),21845 CLRA ACC  ; divide by 3

   vshl HX(11,0),HX(5,0),1
   vadd HX(11,0),HX(11,0),HX(8,0)
   vmulhd.su HX(11,0),HX(11,0),21845   ; divide by 3

   vshl HX(12,0),HX(6,0),1
   vadd HX(12,0),HX(12,0),HX(9,0)
   vmulhd.su HX(12,0),HX(12,0),21845   ; divide by 3

   ; We now recombine into RGB
   vshl -,HX(11,0),5 ACC
   vshl HX(14,0),HX(12,0),11 ACC       ; Colour for interpolator 1


   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   vshl HX(10,0),HX(7,0),1
   vadd HX(10,0),HX(10,0),HX(4,0)
   vmulhd.su HX(10,0),HX(10,0),21845 CLRA ACC  ; divide by 3

   vshl HX(11,0),HX(8,0),1
   vadd HX(11,0),HX(11,0),HX(5,0)
   vmulhd.su HX(11,0),HX(11,0),21845   ; divide by 3

   vshl HX(12,0),HX(9,0),1
   vadd HX(12,0),HX(12,0),HX(6,0)
   vmulhd.su HX(12,0),HX(12,0),21845   ; divide by 3

   ; We now recombine into RGB
   vshl -,HX(11,0),5 ACC
   vshl HX(15,0),HX(12,0),11 ACC       ; Colour for interpolator 2

   ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

   ; We generate data in HX(0,32) that has the first 4 columns containing data, each block has 4 rows,
   ; the different rows correspond to the different interpolations, we then mix in the correct values selectively

   ; Expand out the background pixels into the first row (interpolant of 0)
   vmov HX(0,32),HX(0,0)
   vadd H(4,32),H(0,4),0
   vadd H(4,48),H(0,20),0
   vadd H(8,32),H(0,8),0
   vadd H(8,48),H(0,24),0
   vadd H(12,32),H(0,12),0
   vadd H(12,48),H(0,28),0

   ; Expand out the background*2/3+foreground/3 pixels into the second row
   vmov HX(1,32),HX(14,0)
   vadd H(5,32),H(14,4),0
   vadd H(5,48),H(14,20),0
   vadd H(9,32),H(14,8),0
   vadd H(9,48),H(14,24),0
   vadd H(13,32),H(14,12),0
   vadd H(13,48),H(14,28),0

   ; Expand out the background/3+foreground*2/3 pixels into the third row
   vmov HX(2,32),HX(15,0)
   vadd H(6,32),H(15,4),0
   vadd H(6,48),H(15,20),0
   vadd H(10,32),H(15,8),0
   vadd H(10,48),H(15,24),0
   vadd H(14,32),H(15,12),0
   vadd H(14,48),H(15,28),0

   ; Expand out the foreground pixels into the fourth row
   vmov HX(3,32),HX(1,0)
   vadd H(7,32),H(1,4),0
   vadd H(7,48),H(1,20),0
   vadd H(11,32),H(1,8),0
   vadd H(11,48),H(1,24),0
   vadd H(15,32),H(1,12),0
   vadd H(15,48),H(1,28),0

   ; Now replicate this data across the entire block HX(16,0)
   vmov VX(16,0++),VX(0,32) REP 4
   vmov VX(16,4++),VX(0,33) REP 4
   vmov VX(16,8++),VX(0,34) REP 4
   vmov VX(16,12++),VX(0,35) REP 4

   ; Expand out the interpolators, they are 16 across, and each number contains 16*2 bits for the 16 down
   vmov HX(0,32),HX(2,0)
   vasr HX(1,32),HX(2,0),2
   vasr HX(2,32),HX(2,0),4
   vasr HX(3,32),HX(2,0),6
   vasr HX(4,32),HX(2,0),8
   vasr HX(5,32),HX(2,0),10
   vasr HX(6,32),HX(2,0),12
   vasr HX(7,32),HX(2,0),14

   vmov HX(8,32),HX(3,0)
   vasr HX(9,32),HX(3,0),2
   vasr HX(10,32),HX(3,0),4
   vasr HX(11,32),HX(3,0),6
   vasr HX(12,32),HX(3,0),8
   vasr HX(13,32),HX(3,0),10
   vasr HX(14,32),HX(3,0),12
   vasr HX(15,32),HX(3,0),14

   ; Now we need to combine the blocks HX(16,0) according to HX(0,32) bottom 2 bits
   ; Each row will be tested 4 times so a total of 8 touches per block

   ; Start by filling in with the interpolator 3 colour
   vmov HX(0++,0),HX(19,0) REP 4
   vmov HX(4++,0),HX(23,0) REP 4
   vmov HX(8++,0),HX(27,0) REP 4
   vmov HX(12++,0),HX(31,0) REP 4

   ; Now replace any 0 interpolators
   vand HX(0,32),HX(0,32),3 SETF
   vmov HX(0,0),HX(16,0) IFZ
   vand HX(1,32),HX(1,32),3 SETF
   vmov HX(1,0),HX(16,0) IFZ
   vand HX(2,32),HX(2,32),3 SETF
   vmov HX(2,0),HX(16,0) IFZ
   vand HX(3,32),HX(3,32),3 SETF
   vmov HX(3,0),HX(16,0) IFZ

   vand HX(4,32),HX(4,32),3 SETF
   vmov HX(4,0),HX(20,0) IFZ
   vand HX(5,32),HX(5,32),3 SETF
   vmov HX(5,0),HX(20,0) IFZ
   vand HX(6,32),HX(6,32),3 SETF
   vmov HX(6,0),HX(20,0) IFZ
   vand HX(7,32),HX(7,32),3 SETF
   vmov HX(7,0),HX(20,0) IFZ

   vand HX(8,32),HX(8,32),3 SETF
   vmov HX(8,0),HX(24,0) IFZ
   vand HX(9,32),HX(9,32),3 SETF
   vmov HX(9,0),HX(24,0) IFZ
   vand HX(10,32),HX(10,32),3 SETF
   vmov HX(10,0),HX(24,0) IFZ
   vand HX(11,32),HX(11,32),3 SETF
   vmov HX(11,0),HX(24,0) IFZ

   vand HX(12,32),HX(12,32),3 SETF
   vmov HX(12,0),HX(28,0) IFZ
   vand HX(13,32),HX(13,32),3 SETF
   vmov HX(13,0),HX(28,0) IFZ
   vand HX(14,32),HX(14,32),3 SETF
   vmov HX(14,0),HX(28,0) IFZ
   vand HX(15,32),HX(15,32),3 SETF
   vmov HX(15,0),HX(28,0) IFZ

   ; Now replace any 1 interpolators
   vsub -,HX(0,32),1 SETF
   vmov HX(0,0),HX(17,0) IFZ
   vsub -,HX(1,32),1 SETF
   vmov HX(1,0),HX(17,0) IFZ
   vsub -,HX(2,32),1 SETF
   vmov HX(2,0),HX(17,0) IFZ
   vsub -,HX(3,32),1 SETF
   vmov HX(3,0),HX(17,0) IFZ

   vsub -,HX(4,32),1 SETF
   vmov HX(4,0),HX(21,0) IFZ
   vsub -,HX(5,32),1 SETF
   vmov HX(5,0),HX(21,0) IFZ
   vsub -,HX(6,32),1 SETF
   vmov HX(6,0),HX(21,0) IFZ
   vsub -,HX(7,32),1 SETF
   vmov HX(7,0),HX(21,0) IFZ

   vsub -,HX(8,32),1 SETF
   vmov HX(8,0),HX(25,0) IFZ
   vsub -,HX(9,32),1 SETF
   vmov HX(9,0),HX(25,0) IFZ
   vsub -,HX(10,32),1 SETF
   vmov HX(10,0),HX(25,0) IFZ
   vsub -,HX(11,32),1 SETF
   vmov HX(11,0),HX(25,0) IFZ

   vsub -,HX(12,32),1 SETF
   vmov HX(12,0),HX(29,0) IFZ
   vsub -,HX(13,32),1 SETF
   vmov HX(13,0),HX(29,0) IFZ
   vsub -,HX(14,32),1 SETF
   vmov HX(14,0),HX(29,0) IFZ
   vsub -,HX(15,32),1 SETF
   vmov HX(15,0),HX(29,0) IFZ

   ; Now replace any 2 interpolators
   vsub -,HX(0,32),2 SETF
   vmov HX(0,0),HX(18,0) IFZ
   vsub -,HX(1,32),2 SETF
   vmov HX(1,0),HX(18,0) IFZ
   vsub -,HX(2,32),2 SETF
   vmov HX(2,0),HX(18,0) IFZ
   vsub -,HX(3,32),2 SETF
   vmov HX(3,0),HX(18,0) IFZ

   vsub -,HX(4,32),2 SETF
   vmov HX(4,0),HX(22,0) IFZ
   vsub -,HX(5,32),2 SETF
   vmov HX(5,0),HX(22,0) IFZ
   vsub -,HX(6,32),2 SETF
   vmov HX(6,0),HX(22,0) IFZ
   vsub -,HX(7,32),2 SETF
   vmov HX(7,0),HX(22,0) IFZ

   vsub -,HX(8,32),2 SETF
   vmov HX(8,0),HX(26,0) IFZ
   vsub -,HX(9,32),2 SETF
   vmov HX(9,0),HX(26,0) IFZ
   vsub -,HX(10,32),2 SETF
   vmov HX(10,0),HX(26,0) IFZ
   vsub -,HX(11,32),2 SETF
   vmov HX(11,0),HX(26,0) IFZ

   vsub -,HX(12,32),2 SETF
   vmov HX(12,0),HX(30,0) IFZ
   vsub -,HX(13,32),2 SETF
   vmov HX(13,0),HX(30,0) IFZ
   vsub -,HX(14,32),2 SETF
   vmov HX(14,0),HX(30,0) IFZ
   vsub -,HX(15,32),2 SETF
   vmov HX(15,0),HX(30,0) IFZ

   vst HX(0++,0),(r1+=r2) REP 16
   b  lr


.stabs "vc_4by4_decompress:F1",36,0,0,vc_4by4_decompress
.FORGET




;;; NAME
;;;    vc_rgb565_transparent_alpha_blt
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transparent_alpha_blt(unsigned short *dest (r0), unsigned short *src (r1), int dest_pitch_bytes (r2),
;;;                                          int src_pitch_bytes (r3), int width (r4), int height(r5), int transparent_colour,int alpha)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transparent_alpha_blt,"text"
.globl vc_rgb565_transparent_alpha_blt
vc_rgb565_transparent_alpha_blt:
      stm r6-r8, lr, (--sp)

      ld r6, (sp+16)       ; transparent colour
      vmov HX(62,0), r6
      ld r6, (sp+20)       ; alpha value
      vmov HX(63,0), r6

      ; We want to generate an alpha mask in HX(0,0) (allow for up to 256 rows in the image)
      ; Need to extend to the left and right with zeros in order to reach an aligned blit
      mov r6,r0
      bmask r6,5  ; r6 is the low 5 bits of the address
      asr r6,1    ; r6 is now the distance we need to extend to the left
      mov r7,r6   ; save r6 in r7
      mov r6,0xffff
      shl r6,r7   ; put zeros into low bits of r6
      vbitplanes -,r6 SETF   ; Now the flags are zero for the bits at the beginning
      vmov HX(0++,0),HX(63,0) REP 16   ; Fill up the block with the alpha value
      vmov HX(0,0),0 IFZ               ; Fix the beginning
      add r4,r7      ; Width is now extended
      shl r7,1
      sub r0,r7      ; Destination is now aligned
      sub r1,r7      ; Source is now matched to destination
      add r7,r4,-1   ; width rounded down
      asr r7,4       ; r7 now contains the number of blocks across to apply (-1), 0 for a single block
      ; Now we want to compute the space at the end
      neg r4,r4
      bmask r4,4     ; r4 is now 0 for aligned blocks, up to 15 for addresses just hitting the next block
      mov r6,0xffff
      lsr r6,r4
      vbitplanes -,r6 SETF ; Flags are Z for blocks past the end
      mov r4,r7
      shl r7,5       ; r7 is now the number of bytes added per row
      shl r4,6       ; r4 is now the VRF offset to the last row
      vmov HX(0,0)+r4,0 IFZ   ; Clear the alpha mask for the end of the row

      ; Invert the alpha mask into HX(0,32)
      vrsub HX(0++,32),HX(0++,0),256 REP 16

      sub r2,r7   ; Correct pitches to take into account addition per row
      sub r3,r7
      sub r2,32
      sub r3,32

      ; We now have r6 spare, and can process all the rows
      ; Use r6 to index through the rows, with an upper limit of r4
      ; Use r8 as a loop increment
      mov r8,64
loopdown:
      mov r6,0
loopacross:

      vld HX(16,0),(r0)       ; Destination
      vldleft HX(17,0),(r1)   ; Source
;      vldright HX(17,0),(r1)

      ; Decompose source and destination into RGB
      vand HX(18,0),HX(17,0),0xf800 ; red    (source)
      vand HX(19,0),HX(17,0),0x07e0 ; green
      vand HX(20,0),HX(17,0),0x001f ; blue

      vand HX(18,32),HX(16,0),0xf800 ; red   (dest)
      vand HX(19,32),HX(16,0),0x07e0 ; green
      vand HX(20,32),HX(16,0),0x001f ; blue

      ; Apply alpha blend
      vmulm H(18,16),H(18,16),HX(0,0)+r6 CLRA ACC
      vmulm H(18,48),H(18,48),HX(0,32)+r6 ACC   ; Only apply to top byte to avoid problems with signed values
      vmulm HX(19,0),HX(19,0),HX(0,0)+r6 CLRA ACC
      vmulm HX(19,32),HX(19,32),HX(0,32)+r6 ACC
      vmulm HX(20,0),HX(20,0),HX(0,0)+r6 CLRA ACC
      vmulm HX(20,32),HX(20,32),HX(0,32)+r6 ACC

      ; Apply rounding correction to red and green
      vadd HX(18,32),HX(18,32),0x0400
      vadd HX(19,32),HX(19,32),0x0010

      ; Resynthesise into RGB
      vand -,HX(18,32),0xf800 CLRA ACC; red
      vand -,HX(19,32),0x07e0 ACC ; green
      vand HX(21,0),HX(20,32),0x001f ACC ; blue

      ; Test for transparency and apply destination if necessary
      vsub -,HX(17,0),HX(62,0) SETF ; Z if transparent
      vmov HX(21,0),HX(16,0) IFZ

      vst HX(21,0),(r0)
      add r0,32
      add r1,32


      addcmpble r6,r8,r4,loopacross
      add r0,r2
      add r1,r3
      addcmpbgt r5,-1,0,loopdown

      ldm r6-r8, pc, (sp++)
.stabs "vc_rgb565_transparent_alpha_blt:F1",36,0,0,vc_rgb565_transparent_alpha_blt

.FORGET

;extern void vc_3d32mat_to_rgb565(short *lcd_buffer             r0
;                      ,unsigned char *screen                r1
;                      ,int num                                 r2
;                      ,short *cmap       r3
;                      , int ambient      r4
;                      , int specular)    r5
.section .text$vc_3d32mat_to_rgb565,"text"
.globl vc_3d32mat_to_rgb565
vc_3d32mat_to_rgb565:
       stm      r6-r6,lr,(--sp)
       mov      r6,0
       asr      r2,4
copyloop:
       vld      HX(10,0),(r1)
       add      r1,64

; The format is: low 8 bits of colour, 4 bits of light, 4 bits of material
; Need to put the material bits together with the low colour bits
       vmov     -,H(10,0) CLRA ACC SETF ; (0 means transparent)
       vand HX(0,0),HX(10,0),0xf000 ; Extract material bits
       vlsr -,HX(0,0),4 ACC
       vlookupm HX(0,0),(r3)
       vand HX(10,0),HX(10,0),0x0f00  ; Extract light bits
       vmul HX(10,0),H(10,16),r5  ; Shift up light amount by specular highlight
       vadd HX(10,0),HX(10,0),r4  ; Increase by ambient light (allow for specular stuff to saturate!)
; Now split into colour components and apply light level
   vand  HX(2,0),H(0,16),0xf8   ; red
   vlsr  HX(3,0),HX(0,0),3      ; green
   vand  HX(3,0),HX(3,0),0xfc
   vshl  HX(4,0),HX(0,0),3
   vand  HX(4,0),HX(4,0),0xf8   ; blue

   vmulms H(2,0),HX(2,0),HX(10,0) ; red
   vmulms H(3,0),HX(3,0),HX(10,0) ; green
   vmulms H(4,0),HX(4,0),HX(10,0) ; blue
   ; Mask off unneeded bits
   vand HX(2,0),HX(2,0),0xf8
   vand HX(3,0),HX(3,0),0xfc
   vand HX(4,0),HX(4,0),0xf8

   vlsr HX(0,0),H(4,0),3 CLRA ACC   ; (down by 3)
   vshl HX(0,0),H(3,0),3 ACC        ; (down by 2 and up by 5)
   vshl HX(0,0),H(2,0),8 ACC        ; (down by 3 and up by 5+6)

   vmov HX(0,0),0xfffe IFZ   ; Convert transparent colour to special 1 colour)

       vst      HX(0,0),(r0)
       add      r0,32
       addcmpblt r6,1,r2,copyloop
       ldm      r6-r6,pc,(sp++)

.stabs "vc_3d32mat_to_rgb565:F19",36,0,1,vc_3d32mat_to_rgb565

.FORGET

;;;    vc_rgb565_font_alpha_blt
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_font_alpha_blt(unsigned short *dest (r0), unsigned short *src (r1), int dest_pitch_bytes (r2),
;;;                                          int src_pitch_bytes (r3), int width (r4), int height(r5), int red,int green,int blue,int alpha)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_font_alpha_blt,"text"
.globl vc_rgb565_font_alpha_blt
vc_rgb565_font_alpha_blt:
      stm r6-r8, lr, (--sp)

      ld r6, (sp+16)       ; red
      vmov HX(60,0), r6
      ld r6, (sp+20)       ; green
      vmov HX(61,0), r6
      ld r6, (sp+24)       ; blue
      vmov HX(62,0), r6
      ld r6, (sp+28)       ; alpha value
      vmov HX(63,0), r6

      ; We want to generate an alpha mask in HX(0,0) (allow for up to 256 rows in the image)
      ; Need to extend to the left and right with zeros in order to reach an aligned blit
      mov r6,r0
      bmask r6,5  ; r6 is the low 5 bits of the address
      asr r6,1    ; r6 is now the distance we need to extend to the left
      mov r7,r6   ; save r6 in r7
      mov r6,0xffff
      shl r6,r7   ; put zeros into low bits of r6
      vbitplanes -,r6 SETF   ; Now the flags are zero for the bits at the beginning
      vmov HX(0++,0),HX(63,0) REP 16   ; Fill up the block with the alpha value
      vmov HX(0,0),0 IFZ               ; Fix the beginning
      add r4,r7      ; Width is now extended
      sub r1,r7      ; Source is now matched to destination (source is only 8 bit wide)
      shl r7,1
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
      sub r2,r7   ; destination is 16 bit colour
      sub r3,r7
      sub r2,32
      sub r3,16

      ; We now have r6 spare, and can process all the rows
      ; Use r6 to index through the rows, with an upper limit of r4
      ; Use r8 as a loop increment
      mov r8,64
loopdown:
      mov r6,0
loopacross:

      vld HX(16,0),(r0)       ; Destination
      vldleft H(17,0),(r1)   ; Source
;      vldright H(17,0),(r1)   ; in 4.4 (alpha/gray) format

      ; Compute actual alpha value via a mulm
      vand HX(18,0),HX(17,0),0xf0   ; mask of grayscale value
      vmulm HX(22,0),HX(18,0),HX(0,0)+r6   ; alpha value
      vrsub HX(23,0),HX(22,0),256         ; 256-alpha value

      ; Scale source gray scale values in order to determine original colour
      ; Start with 8 bit gray, and 8 bit colours, multiply to get 8 bit of colour,
      vshl HX(17,0),H(17,0),4 ; extract gray values into full 8bit precision
      vmulm HX(18,0),HX(60,0),H(17,0)
      vmulm HX(19,0),HX(61,0),H(17,0)
      vmulm HX(20,0),HX(62,0),H(17,0)

      ; Decompose destination into RGB (scaled to 8 bit precision)
      vand HX(18,32),HX(16,0),0xf800 ; red   (dest)
      vand HX(19,32),HX(16,0),0x07e0 ; green
      vand HX(20,32),HX(16,0),0x001f ; blue
      vlsr HX(18,32),HX(18,32),8
      vlsr HX(19,32),HX(19,32),3
      vshl HX(20,32),HX(20,32),3

      ; Apply alpha blend
      vmulm -,HX(18,0),HX(22,0) CLRA ACC
      vmulm HX(18,32),H(18,32),HX(23,0) ACC
      vmulm -,HX(19,0),HX(22,0) CLRA ACC
      vmulm HX(19,32),H(19,32),HX(23,0) ACC
      vmulm -,HX(20,0),HX(22,0) CLRA ACC
      vmulm HX(20,32),H(20,32),HX(23,0) ACC

      ; Apply rounding correction
      vadd HX(18,32),HX(18,32),4
      vadd HX(19,32),HX(19,32),2
      vadd HX(20,32),HX(20,32),4

      ; Resynthesise into RGB
      vshl HX(18,32),HX(18,32),8
      vshl HX(19,32),HX(19,32),3
      vlsr HX(20,32),HX(20,32),3

      vand -,HX(18,32),0xf800 CLRA ACC; red
      vand -,HX(19,32),0x07e0 ACC ; green
      vand HX(21,0),HX(20,32),0x001f ACC ; blue

      vst HX(21,0),(r0)
      add r0,32
      add r1,16

      addcmpble r6,r8,r4,loopacross
      add r0,r2
      add r1,r3
      addcmpbgt r5,-1,0,loopdown

      ldm r6-r8, pc, (sp++)
.stabs "vc_rgb565_font_alpha_blt:F1",36,0,0,vc_rgb565_font_alpha_blt

.FORGET

;;; NAME
;;;    vc_rgb565_transparent_alpha_blt_from_pal8
;;;
;;; SYNOPSIS
;;;    void vc_rgb565_transparent_alpha_blt_from_pal8(unsigned short *dest (r0), unsigned short *src (r1), int dest_pitch_bytes (r2),
;;;                                          int src_pitch_bytes (r3), int width (r4), int height(r5), int transparent_colour,int alpha,short *palette,int shift,int mask)
;;;
;;; FUNCTION
;;;    Transparent blt of nrows rows of dest and src arrays.
;;;
;;; RETURNS
;;;    -

.section .text$vc_rgb565_transparent_alpha_blt_from_pal8,"text"
.globl vc_rgb565_transparent_alpha_blt_from_pal8
vc_rgb565_transparent_alpha_blt_from_pal8:
      stm r6-r9, lr, (--sp)

      ld r6, (sp+20)       ; transparent colour
      vmov HX(62,0), r6
      ld r6, (sp+24)       ; alpha value
      vmov HX(63,0), r6

      mov r9,r5            ; save height
      ld r5, (sp+28)       ; palette

      ld r6, (sp+32)       ; shift
      vmov HX(61,0), r6
      ld r6, (sp+36)       ; mask
      vmov HX(60,0), r6

      ; We want to generate an alpha mask in HX(0,0) (allow for up to 256 rows in the image)
      ; Need to extend to the left and right with zeros in order to reach an aligned blit
      mov r6,r0
      bmask r6,5  ; r6 is the low 5 bits of the address
      asr r6,1    ; r6 is now the distance we need to extend to the left
      mov r7,r6   ; save r6 in r7
      mov r6,0xffff
      shl r6,r7   ; put zeros into low bits of r6
      vbitplanes -,r6 SETF   ; Now the flags are zero for the bits at the beginning
      vmov HX(0++,0),HX(63,0) REP 16   ; Fill up the block with the alpha value
      vmov HX(0,0),0 IFZ               ; Fix the beginning
      add r4,r7      ; Width is now extended
      sub r1,r7      ; Source is now matched to destination
      shl r7,1
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

      ; Invert the alpha mask into HX(0,32)
      vrsub HX(0++,32),HX(0++,0),256 REP 16

      sub r2,r7   ; Correct pitches to take into account addition per row
      sub r2,r7   ; Destination is 16 bit colour
      sub r3,r7
      sub r2,32
      sub r3,16

      ; We now have r6 spare, and can process all the rows
      ; Use r6 to index through the rows, with an upper limit of r4
      ; Use r8 as a loop increment
      mov r8,64
loopdown:
      mov r6,0
loopacross:

      vld HX(16,0),(r0)       ; Destination
      vldleft H(22,0),(r1)   ; Source
;      vldright H(22,0),(r1)

      vlsr H(22,0),H(22,0),H(61,0)  ; shift down
      vand H(22,0),H(22,0),H(60,0)  ; and mask

      vmov -,H(22,0) CLRA ACC
      vlookupm HX(17,0),(r5)

      ; Decompose source and destination into RGB
      vand HX(18,0),HX(17,0),0xf800 ; red    (source)
      vand HX(19,0),HX(17,0),0x07e0 ; green
      vand HX(20,0),HX(17,0),0x001f ; blue

      vand HX(18,32),HX(16,0),0xf800 ; red   (dest)
      vand HX(19,32),HX(16,0),0x07e0 ; green
      vand HX(20,32),HX(16,0),0x001f ; blue

      ; Apply alpha blend
      vmulm H(18,16),H(18,16),HX(0,0)+r6 CLRA ACC
      vmulm H(18,48),H(18,48),HX(0,32)+r6 ACC   ; Only apply to top byte to avoid problems with signed values
      vmulm HX(19,0),HX(19,0),HX(0,0)+r6 CLRA ACC
      vmulm HX(19,32),HX(19,32),HX(0,32)+r6 ACC
      vmulm HX(20,0),HX(20,0),HX(0,0)+r6 CLRA ACC
      vmulm HX(20,32),HX(20,32),HX(0,32)+r6 ACC

      ; Apply rounding correction to red and green
      vadd HX(18,32),HX(18,32),0x0400
      vadd HX(19,32),HX(19,32),0x0010

      ; Resynthesise into RGB
      vand -,HX(18,32),0xf800 CLRA ACC; red
      vand -,HX(19,32),0x07e0 ACC ; green
      vand HX(21,0),HX(20,32),0x001f ACC ; blue

      ; Test for transparency and apply destination if necessary
      vsub -,H(22,0),HX(62,0) SETF ; Z if transparent
      vmov HX(21,0),HX(16,0) IFZ

      vst HX(21,0),(r0)
      add r0,32
      add r1,16


      addcmpble r6,r8,r4,loopacross
      add r0,r2
      add r1,r3
      addcmpbgt r9,-1,0,loopdown

      ldm r6-r9, pc, (sp++)
.stabs "vc_rgb565_transparent_alpha_blt_from_pal8:F1",36,0,0,vc_rgb565_transparent_alpha_blt_from_pal8

.FORGET


.include "vcinclude/common.inc"

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
.define STACK_ARG_9, 52

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

.define N_red0, 29
.define N_green0, 30
.define N_blue0, 31
.define N_red1, 32
.define N_green1, 33
.define N_blue1, 34
.define N_red2, 35
.define N_green2, 36
.define N_blue2, 37
.define N_red3, 38
.define N_green3, 39
.define N_blue3, 40

;void vc_roz_blt_8bpp_to_rgb565(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
;   int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color, short *pal)

.function vc_roz_blt_8bpp_to_rgb565 ; {{{
   stm r6-r9, (--sp)
   .cfa_push {%r6,%r7,%r8,%r9}
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
   add Rdstinc, 16
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

   vmov -, 1 IFNN SUM Rtemp
   addcmpbeq Rtemp, 0, 0, .skippixels

; accumulate x & y index
   vmul      -, HX(N_cy, 32), Rsrcpitch CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), Rsrcpitch ACC
   vmov      -, HX(N_cx, 32)            UACC

; get dest pixels
   vld HX(N_dst,0), (Rdst)

.ifdef BILINEAR
; lookup pixels
   add Rtemp, Rsrc, Rsrcpitch
   vmov HX(N_src++, 0), 0 REP 4
   vmov HX(N_src++, 0), HX(N_trans, 0) REP 4 IFN
   vlookupm  H(N_src ,0), (Rsrc+0)  UACC IFNN
   vlookupm  H(N_src1,0), (Rsrc+1)  UACC IFNN
   vlookupm  H(N_src2,0), (Rtemp+0) UACC IFNN
   vlookupm  H(N_src3,0), (Rtemp+1) UACC IFNN
   ld Rtemp, (sp+STACK_ARG_9) ; palette

; check for transparency
   vsub -, HX(N_src,0), HX(N_trans,0) SETF

; lookup palette
   vmov -, H(N_src,0) CLRA ACC
   vmov HX(N_src,0), HX(N_dst, 0)
   vlookupm HX(N_src,0), (Rtemp) IFNZ

; get components
   vshl HX(N_blue0, 0), HX(N_src,0), 11
   vand HX(N_green0, 0), HX(N_src , 0), 0x7e0
   vshl HX(N_green0, 0), HX(N_green0,0), 5
   vand HX(N_red0, 0), HX(N_src ,0), 0xf800

; check for transparency
   vsub -, HX(N_src1,0), HX(N_trans,0) SETF

; lookup palette
   vmov -, H(N_src1,0) CLRA ACC
   vmov HX(N_src1,0), HX(N_dst, 0)
   vlookupm HX(N_src1,0), (Rtemp) IFNZ

; get components
   vshl HX(N_blue1, 0), HX(N_src1,0), 11
   vand HX(N_green1, 0), HX(N_src1, 0), 0x7e0
   vshl HX(N_green1, 0), HX(N_green1,0), 5
   vand HX(N_red1, 0), HX(N_src1,0), 0xf800

; check for transparency
   vsub -, HX(N_src2,0), HX(N_trans,0) SETF

; lookup palette
   vmov -, H(N_src2,0) CLRA ACC
   vmov HX(N_src2,0), HX(N_dst, 0)
   vlookupm HX(N_src2,0), (Rtemp) IFNZ

; get components
   vshl HX(N_blue2, 0), HX(N_src2,0), 11
   vand HX(N_green2, 0), HX(N_src2, 0), 0x7e0
   vshl HX(N_green2, 0), HX(N_green2,0), 5
   vand HX(N_red2, 0), HX(N_src2,0), 0xf800

; check for transparency
   vsub -, HX(N_src3,0), HX(N_trans,0) SETF

; lookup palette
   vmov -, H(N_src3,0) CLRA ACC
   vmov HX(N_src3,0), HX(N_dst, 0)
   vlookupm HX(N_src3,0), (Rtemp) IFNZ

; get components
   vshl HX(N_blue3, 0), HX(N_src3,0), 11
   vand HX(N_green3, 0), HX(N_src3, 0), 0x7e0
   vshl HX(N_green3, 0), HX(N_green3,0), 5
   vand HX(N_red3, 0), HX(N_src3,0), 0xf800

; fractional bits
   veor HX(N_cx_i, 0), HX(N_cx, 0), 0xffff
   veor HX(N_cy_i, 0), HX(N_cy, 0), 0xffff

   vmulhd.uu HX(N_frac0, 0), HX(N_cx_i, 0), HX(N_cy_i, 0)
   vmulhd.uu HX(N_frac1, 0), HX(N_cx, 0), HX(N_cy_i, 0)
   vmulhd.uu HX(N_frac2, 0), HX(N_cx_i, 0), HX(N_cy, 0)
   vmulhd.uu HX(N_frac3, 0), HX(N_cx, 0), HX(N_cy, 0)

   vmulhd.uu -, HX(N_blue0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_blue1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_blue2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_blue0, 0), HX(N_blue3, 0), HX(N_frac3, 0) UACC

   vmulhd.uu -, HX(N_green0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_green1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_green2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_green0, 0), HX(N_green3, 0), HX(N_frac3, 0) UACC

   vmulhd.uu -, HX(N_red0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_red1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_red2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_red0, 0), HX(N_red3, 0), HX(N_frac3, 0) UACC

; combine again
   vmulhn.uu HX(N_red0, 0), HX(N_red0, 0), 1<<5
   vmulhn.uu HX(N_green0, 0), HX(N_green0, 0), 1<<6
   vmulhn.uu HX(N_blue0, 0), HX(N_blue0, 0), 1<<5 CLRA ACC
   vshl -, HX(N_green0, 0), 5 ACC
   vshl HX(N_dst, 0), HX(N_red0, 0), 11 ACC
.else
   ld Rtemp, (sp+STACK_ARG_9) ; palette
; lookup pixel
   vmov HX(N_src, 0), 0
   vmov HX(N_src, 0), HX(N_trans, 0) IFN
   vlookupm  H(N_src,0), (Rsrc) UACC IFNN

; check for transparency
   vsub -, HX(N_src,0), HX(N_trans,0) SETF

; lookup palette
   vmov -, H(N_src,0) CLRA ACC
   vmov HX(N_src,0), HX(N_dst, 0)
   vlookupm HX(N_src,0), (Rtemp) IFNZ
   vmov HX(N_dst,0), HX(N_src, 0)
.endif

; store pixels
   vst HX(N_dst,0), (Rdst)
.skippixels:
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
   .cfa_pop {%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}

;void vc_roz_blt_rgb565_to_rgb565(unsigned char *dst_bitmap,unsigned char *src_bitmap,int dst_pitch,int src_pitch,int dst_width,int dst_height,
;   int src_width, int src_height,int incxx,int incxy,int incyx,int incyy,unsigned int cx, unsigned int cy, int transparent_color)

.function vc_roz_blt_rgb565_to_rgb565 ; {{{
   stm r6-r9, (--sp)
   .cfa_push {%r6,%r7,%r8,%r9}
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
   add Rdstinc, 16
   mov Rwidthinc, -16

   vmul      HX(N_incxx_0to15, 0), HX(N_incxx, 0), H(N_0to15, 0) CLRA UACC
   vmulhd.uu -,                       HX(N_incxx, 0), H(N_0to15, 0) ACC
   vmul      HX(N_incxx_0to15, 32), HX(N_incxx, 32), H(N_0to15, 0) ACC

   vmul      HX(N_incxy_0to15, 0), HX(N_incxy, 0), H(N_0to15, 0) CLRA UACC
   vmulhd.uu -,                       HX(N_incxy, 0), H(N_0to15, 0) ACC
   vmul      HX(N_incxy_0to15, 32), HX(N_incxy, 32), H(N_0to15, 0) ACC

   VADD_H32 N_cx, N_cx, N_incxx_0to15
   VADD_H32 N_cy, N_cy, N_incxy_0to15

   lsr Rsrcpitch, 1 ; as lookup in shorts
   vmov HX(N_src++, 0), 0 REP 4

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

   vmov -, 1 IFNN SUM Rtemp
   addcmpbeq Rtemp, 0, 0, .skippixels

; accumulate x & y index
   vmul      -, HX(N_cy, 32), Rsrcpitch CLRA UACC
   vmulhd.uu -, HX(N_cy, 32), Rsrcpitch ACC
   vmov      -, HX(N_cx, 32)            UACC

; get dest pixels
   vld HX(N_dst,0), (Rdst)

.ifdef BILINEAR
; lookup pixels
   addscale Rtemp, Rsrc, Rsrcpitch << 1
   vmov HX(N_src++, 0), HX(N_trans, 0) REP 4 IFN
   vlookupm  HX(N_src,0), (Rsrc+0) UACC IFNN
   vlookupm  HX(N_src1,0), (Rsrc+2) UACC IFNN
   vlookupm  HX(N_src2,0), (Rtemp+0) UACC IFNN
   vlookupm  HX(N_src3,0), (Rtemp+2) UACC IFNN

; check for transparency
   vsub -, HX(N_src,0), HX(N_trans,0) SETF
   vmov HX(N_src,0), HX(N_dst, 0) IFZ

; get components
   vshl HX(N_blue0, 0), HX(N_src,0), 11
   vand HX(N_green0, 0), HX(N_src , 0), 0x7e0
   vshl HX(N_green0, 0), HX(N_green0,0), 5
   vand HX(N_red0, 0), HX(N_src ,0), 0xf800

; check for transparency
   vsub -, HX(N_src1,0), HX(N_trans,0) SETF
   vmov HX(N_src1,0), HX(N_dst, 0) IFZ

; get components
   vshl HX(N_blue1, 0), HX(N_src1,0), 11
   vand HX(N_green1, 0), HX(N_src1, 0), 0x7e0
   vshl HX(N_green1, 0), HX(N_green1,0), 5
   vand HX(N_red1, 0), HX(N_src1,0), 0xf800

; check for transparency
   vsub -, HX(N_src2,0), HX(N_trans,0) SETF
   vmov HX(N_src2,0), HX(N_dst, 0) IFZ

; get components
   vshl HX(N_blue2, 0), HX(N_src2,0), 11
   vand HX(N_green2, 0), HX(N_src2, 0), 0x7e0
   vshl HX(N_green2, 0), HX(N_green2,0), 5
   vand HX(N_red2, 0), HX(N_src2,0), 0xf800

; check for transparency
   vsub -, HX(N_src3,0), HX(N_trans,0) SETF
   vmov HX(N_src3,0), HX(N_dst, 0) IFZ

; get components
   vshl HX(N_blue3, 0), HX(N_src3,0), 11
   vand HX(N_green3, 0), HX(N_src3, 0), 0x7e0
   vshl HX(N_green3, 0), HX(N_green3,0), 5
   vand HX(N_red3, 0), HX(N_src3,0), 0xf800

; fractional bits
   veor HX(N_cx_i, 0), HX(N_cx, 0), 0xffff
   veor HX(N_cy_i, 0), HX(N_cy, 0), 0xffff

   vmulhn.uu HX(N_frac0, 0), HX(N_cx_i, 0), HX(N_cy_i, 0)
   vmulhn.uu HX(N_frac1, 0), HX(N_cx, 0), HX(N_cy_i, 0)
   vmulhn.uu HX(N_frac2, 0), HX(N_cx_i, 0), HX(N_cy, 0)
   vmulhn.uu HX(N_frac3, 0), HX(N_cx, 0), HX(N_cy, 0)

   vmulhd.uu -, HX(N_blue0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_blue1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_blue2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_blue0, 0), HX(N_blue3, 0), HX(N_frac3, 0) UACC

   vmulhd.uu -, HX(N_green0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_green1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_green2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_green0, 0), HX(N_green3, 0), HX(N_frac3, 0) UACC

   vmulhd.uu -, HX(N_red0, 0), HX(N_frac0, 0) CLRA UACC
   vmulhd.uu -, HX(N_red1, 0), HX(N_frac1, 0) UACC
   vmulhd.uu -, HX(N_red2, 0), HX(N_frac2, 0) UACC
   vmulhd.uu HX(N_red0, 0), HX(N_red3, 0), HX(N_frac3, 0) UACC

; combine again
   vmulhn.uu HX(N_red0, 0), HX(N_red0, 0), 1<<5
   vmulhn.uu HX(N_green0, 0), HX(N_green0, 0), 1<<6
   vmulhn.uu HX(N_blue0, 0), HX(N_blue0, 0), 1<<5 CLRA ACC
   vshl -, HX(N_green0, 0), 5 ACC
   vshl HX(N_dst, 0), HX(N_red0, 0), 11 ACC
.else
; lookup pixel
   vmov HX(N_src,0), HX(N_trans,0)
   vlookupm  HX(N_src,0), (Rsrc) UACC IFNN

; check for transparency
   vsub -, HX(N_src,0), HX(N_trans,0) SETF
   vmov HX(N_dst,0), HX(N_src, 0) IFNZ
.endif

; store pixels
   vst HX(N_dst,0), (Rdst)
.skippixels:
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
   .cfa_pop {%r9,%r8,%r7,%r6,%pc}
.endfn ; }}}


;;; *****************************************************************************
;;; NAME
;;;   rgb565_from_rgba32_blocky
;;;
;;; SYNOPSIS
;;;   void rgb565_from_rgba32_blocky(
;;;      unsigned short      *dest565,    /* r0 */
;;;      int                  dpitch,     /* r1 */
;;;      unsigned long const *srca32,     /* r2 */
;;;      int                  spitch,     /* r3 */
;;;      int                  width,      /* r4 * in pixels */
;;;      int                  height);    /* r5 * in pixels */
;;;
;;; FUNCTION
;;;   Convert an aligned rgba32 image to rgb565

function rgb565_from_rgba32_blocky ; {{{
         stm      r6-r9, (--sp)
         .cfa_push {%r6,%r7,%r8,%r9}

         add      r4, 15
         bic      r6, r4, 15
         lsr      r7, r5, 4
         shl      r6, 1
         mov      r8, 32

;         assertne r6
;         assertne r7

.vloop:  add      r9, r0, r6

         ; straddle lazy-load blocking boundary so we can start work on one part while the other finishes
.hloop:  vld      H32(8++,0), (r2+=r3)              REP 16

         mov      r4, 0
         mov      r5, 0

.loop:   vmulhn.uu H(0,0),  H(8,0)+r5, 7967                    ; red
         vmulhn.uu H(1,0),  H(8,16)+r5, 16191                  ; green
         vmulhn.uu -,       H(8,32)+r5, 7967       CLRA UACC   ; blue
         v16shl   -,           H(0,0), 11               UACC
         v16shl   HX(40,0)+r5, H(1,0), 5                UACC
         add      r5, 64
         addcmpbne r4, 1, 16, .loop

         vst      HX(40++,0), (r0+=r1)             REP 16

         addscale r2, r8 << 1
         addcmpbne r0, r8, r9, .hloop
         sub      r2, r6
         sub      r0, r6
         sub      r2, r6

         addscale r0, r1 << 4
         addscale r2, r3 << 4

         addcmpbgt r7, -1, 0, .vloop

         ldm      r6-r9, (sp++)
         .cfa_pop {%r9,%r8,%r7,%r6}
         b        lr
endfunc ; }}}
.FORGET


