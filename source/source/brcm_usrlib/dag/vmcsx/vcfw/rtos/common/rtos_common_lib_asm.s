;;; ===========================================================================
;;; Copyright (c) 2008-2014, Broadcom Corporation
;;; All rights reserved.
;;; Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
;;; 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
;;; 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
;;; 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
;;; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;; ===========================================================================

.include "vcinclude/common.inc"
.include "vcinclude/hardware.inc"

; /******************************************************************************
; NAME
;   memset_asm
;
; SYNOPSIS
;    void *memset_asm(void *s, int _c, size_t n)
;
; FUNCTION
;
;    Sets n bytes of s to _c. Uses this method:
;
; void *memset(QUALIFIER void *s, int _c, size_t n) {
;    char *p = (char*)s;
;    unsigned c = (_c & 0xff) * 0x01010101;
;
;    if (sizeof(int) == 4 && n >= 8) {

;    unsigned t = (unsigned)p & 3);
;    if( t )
;    {
;       t -= 4;
;       n += t;
;       do {
;          *p++ = c;
;          ++t;
;       } while( t != 0 );
;    }

;    do {
;       *((int*)p)++ = c;
;       n -= 4;
;       } while( n >= 4u );
;    }
;
;    if (n > 0) do { *p++ = c; } while (--n);
;    return s;
;    }
;
; RETURNS
;    s
; ******************************************************************************/
.function memset_asm; {{{
   mov   r3,r0
   addcmpblo   r2,0,8,.do_trailing ; skip optimised for small memsets
   bmask r1,8
   bmask r4,r0,2
   mul r1,0x01010101    ; 2 cycle latency

   addcmpbeq r4,0,0,.do_aligned
; do unaligned leading bytes
   sub r4,4     ; -ve count of leading bytes -> r4
   add r2,r4    ; subtract count of unaligned bytes from r2
1:
   stb   r1,(r3++)
   addcmpbne   r4,1,0,1b

.do_aligned:
   ; (remaining bytes) >= 5

1: ; do aligned words
   st r1,(r3++)
   addcmpbhs   r2,-4,4,1b       ; while( at least 4 bytes to go )

.do_trailing:
   addcmpbeq   r2,0,0,.finished
; do trailing bytes
1:
   stb   r1,(r3++)
   addcmpbne   r2,-1,0,1b
.finished:
   b  lr
.endfn ; }}}


;/**************************************** End of file ************************************************************/
