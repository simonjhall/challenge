;;; ===========================================================================
;;; Copyright (c) 2007-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"

;;; *****************************************************************************
;;; NAME
;;;   vclib_memcmp
;;;
;;; SYNOPSIS
;;;   int vclib_memcmp(const void *a, const void *b, int numbytes)
;;;
;;; FUNCTION
;;;   Like memcmp.
;;;   This implementation is not particularly fast. Feel free to improve it if
;;;   you need to.

.if _VC_VERSION >= 3

.function vclib_memcmp

; Don't want to have to deal with nonaligned case
      or r3,r0,r1
      and r3,3
      addcmpbne r3,0,0,.memcmp_std
      
      mov r3,-64
      addcmpble r2,r3,0,.memcmp_finish
      
.memcmp_loop:
      vld H32(0,0), (r0)
      vld H32(1,0), (r1)
      vdists -,H32(0,0),H32(1,0) MAX r4
      addcmpbgt r4,0,0,.memcmp_diff
      add r0,64
      add r1,64
      addcmpbge r2,r3,0,.memcmp_loop
      
.memcmp_finish:
      ; We have subtracted too much from the remaining length
      ; Add it back and then call the standard memcmp
      mov r3,64
      addcmpbgt r2,r3,0,.memcmp_std
      mov r0,0
      b lr
      
.memcmp_diff:
      mov r2,64
      
.memcmp_std:
      b memcmp
.endfn

.endif
