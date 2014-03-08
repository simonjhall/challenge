;;; ===========================================================================
;;; Copyright (c) 2006-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

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

;;; NAME
;;;    vclib_statxyn
;;;
;;; SYNOPSIS
;;;    vclib_statxyn(unsigned char *addr, int pitch, unsigned *max, unsigned *min, unsigned *sum, int ncols, int nrows)
;;;
;;; FUNCTION
;;;    Find the max/min/sum value of a nrows x ncols block (nrows rows, n columns)
;;;
;;; RETURNS
;;;    -   

function vclib_statxyn ; {{{
   stm  r6-r9, lr, (--sp)
   ld r8, (sp+20)                         ;no. or rows
   mov  r6, 1
   vmov HX(61,0), 0                       ;clear some scratch space first 
   vmov H(62,0), 0                        ;clear some space for calculating maximum
   vmov H(63,0), 255                      ;clear some space for calculating minimum
   shl  r6, r5
   add  r6, -1                            ;this is now the mask for disabling the unwanted PPU
   vbitplanes -, r6 SETF                  ;enable/disable appropriate PPUs
   ld  r7, (r4)
   mov r6, 0
   mov r9, 64
   shl r8, 6
   vld H(0++,0), (r0 += r1) REP 16        ;load all the pixels (including rubbish)

statxyn_loop:
   vadd HX(61,0), HX(61,0), H(0,0)+r6  IFNZ 
   vmax H(62,0), H(62,0), H(0,0)+r6 IFNZ
   vmin H(63,0), H(63,0), H(0,0)+r6 IFNZ
   addcmpblt r6, r9, r8, statxyn_loop     ;continue if we haven't done nrows row yet
   
   vmov -, HX(61,0) IFNZ USUM r6          ;total sum
   vmov -,  H(62,0) IFNZ IMAX r5          ;the index of PPU which has the max value goes to r5
   add  r7, r6                            ;accumulate *sum
   st   r7, (r4)                          ;store *sum
   vmov -, H(63,0) IFNZ IMIN r6           ;the index of PPU which has the min value goes to r6
   vbitplanes -, 0x4000 SETF              ;select row 62 of VRF
   vmov -, V(48,0)+r5 IFNZ SUM r5         ;get the value of the PPU which has the maximum pixel value
   vbitplanes -, 0x8000 SETF              ;select row 63 of VRF
   vmov -, V(48,0)+r6 IFNZ SUM r6         ;get the value of the PPU which has the minimum pixel value
   ld  r7, (r2)
   ld  r8, (r3)
   max r7, r5
   st  r7, (r2)                           ;store the max to *max
   min r8, r6
   st  r8, (r3)                           ;store the min to *min
   ldm r6-r9, pc, (sp++)                  ;return
endfunc ; }}}
   
;;; NAME
;;;    vclib_checksum
;;;
;;; SYNOPSIS
;;;    vclib_checksum(void *buffer, unsigned int length, unsigned int *checksum)
;;;
;;; FUNCTION
;;;    Produce a 32-bit checksum for a block of data
;;;
;;; RETURNS
;;;    checksum   

.section .sdata$checksumtemp,"data"
    .align 32
    checksumtemp:
    .block 32

function vclib_checksum ; {{{
   bmask r5, r0, 2
   addcmpbne r5, 0, 0, checksum_unaligned
   bmask r5, r1, 2
   addcmpbne r5, 0, 0, checksum_unaligned
   bmask r5, r0, 5
   cmp r5, 0      ; warning! Long range flags, avoid changing during function
   vmov HX(16,0), 0 ; checksum
   
   mov r3, 32
   shl r4, r3, 4   ; sixteen lots of 32 bytes

   addcmpblt r1, 0, r4, 2f ; skip block loading if not whole block
   lsr r5, r1, 9
   shl r5, 9
   add r5, r0
1:
   vldleft HX(0++, 0), (r0+=r3) REP 16
   beq .aligned1
;   vldright HX(0++, 0), (r0+=r3) REP 16
.aligned1:
   veor HX(16, 0), HX(16, 0), HX(0++, 0) REP 16
   vror HX(16, 0), HX(16, 0), 1
   vmulm -, H(16, 0), 239 CLRA ACC
   vmov H(16, 16), H(16, 16) ACC
   addcmpblt r0, r4, r5, 1b
2:
   bmask r1, 9  ; number of bytes remaining (<512)

; finished blocks of 16, now do remaining blocks
   vmov HX(0++, 0), 0 REP 16
   mov r4, 0

   addcmpblt r1, 0, r3, 2f ; skip block loading if not whole block
   lsr r5, r1, 5
   shl r5, 5
   add r5, r0
1:
   vldleft HX(0, 0)+r4, (r0)
   beq .aligned2
;   vldright HX(0, 0)+r4, (r0)
.aligned2:
   add r4, 64 ; next row
   addcmpblt r0, r3, r5, 1b
2:
   bmask r1, 5  ; number of bytes remaining (<32)
   lsr r1, 1    ; number of shorts          (<16)
   mov r3, -1
   shl r3, r1
   vbitplanes -, r3 SETF
   vldleft HX(0, 0)+r4, (r0) IFZ
;   vldright HX(0, 0)+r4, (r0) IFZ
   veor HX(16, 0), HX(16, 0), HX(0++, 0) REP 16
   vror HX(16, 0), HX(16, 0), 1
   vmulm -, H(16, 0), 239 CLRA ACC
   vmov H(16, 16), H(16, 16) ACC

   add r0, gp, gprel(checksumtemp)
   vst HX(16, 0), (r0)
   mov r4, 8
   ld r5, (r2)
1:
   ld r3, (r0++)
   eor r5, r3
   ror r5, 1
   addcmpbgt r4, -1, 0, 1b
   b checksum_return
checksum_unaligned:
   bkpt
   mov r5, -1
checksum_return:
   st r5, (r2)
   b lr

endfunc ; }}}
 
