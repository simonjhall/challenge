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

; vc_printchar(unsigned short *buffer,short *fontdata,int ysize);
; Expands data from fontdata and copies to buffer
.globl vc_printchar
vc_printchar:
   vld   VX(0,0),(r1)   ; compressed data
   vasr VX(0,1++),VX(0,0++),1 REP 16 ; LSB belongs on the right
   vand  HX(0++,0),HX(0++,0),1 REP 16 ; select bottom bit
   vmul  HX(0++,0),HX(0++,0),0xffff REP 16 ; convert to black or white
   vst   VX(0,0++),(r0+=r2) REP 16
   b  lr
   
; vc_printcharhigh(unsigned short *buffer,short *fontdata,int ysize);
; Expands data from fontdata and copies to buffer
.globl vc_printcharhigh
vc_printcharhigh:
   vld   VX(0,0),(r1)   ; compressed data
   vasr VX(0,1++),VX(0,0++),1 REP 16 ; LSB belongs on the right
   vand  HX(0++,0),HX(0++,0),1 REP 16 ; select bottom bit
   vmul  HX(0++,0),HX(0++,0),0xffe1 REP 16 ; background to 0xffe1
   vadd  HX(0++,0),HX(0++,0),0x1f REP 16 ; Convert text to black, and background to blue
   vst   VX(0,0++),(r0+=r2) REP 16
   b  lr
