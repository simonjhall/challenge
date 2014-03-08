/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

 \


#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/vg/2708/vg_shader_4.h"

 \


const uint32_t VG_SHADER_MD_CLEAR_EXTRA_THRSWS = 5;

const uint32_t VG_SHADER_MD_CLEAR[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop              ; nop */ \
, 0x15827d80, 0x50020be7 /* mov  tlbam, unif ; nop ; sbdone */ \
, 0x009e7000, 0x300009e7 /* nop              ; nop ; thrend */ \
, 0x009e7000, 0x100009e7 /* nop              ; nop */ \
, 0x009e7000, 0x100009e7 /* nop              ; nop */ \
 };

const uint32_t VG_SHADER_MD_CLEAR_SIZE = sizeof(VG_SHADER_MD_CLEAR);

const uint32_t VG_SHADER_MD_IMAGE_EXTRA_THRSWS = 4;

 \


const uint32_t VG_SHADER_MD_IMAGE_SET[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0x400009e7 /* nop                ; nop ; sbwait */ \
, 0x009e7000, 0x300009e7 /* nop                ; nop ; thrend */ \
, 0x009e7000, 0xa00009e7 /* nop                ; nop ; ldtmu0 */ \
, 0x159e7900, 0x57020be7 /* mov  tlbam, r4.8dr ; nop ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_SET_SIZE = sizeof(VG_SHADER_MD_IMAGE_SET);

const uint32_t VG_SHADER_MD_IMAGE_UNION[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xc00009e7 /* nop                   ; nop                    ; loadam */ \
, 0x979e7924, 0xa0024821 /* not  r0, r4           ; mov  r1, r4            ; ldtmu0 */ \
, 0x609e7004, 0x370049e0 /* nop                   ; v8muld  r0, r0, r4.8dr ; thrend */ \
, 0x1e9e7040, 0x50020be7 /* v8adds  tlbam, r0, r1 ; nop                    ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop                   ; nop */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_UNION_SIZE = sizeof(VG_SHADER_MD_IMAGE_UNION);

const uint32_t VG_SHADER_MD_IMAGE_INTERSECT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xc00009e7 /* nop ; nop                       ; loadam */ \
, 0x809e7024, 0x300049e0 /* nop ; mov  r0, r4               ; thrend */ \
, 0x009e7000, 0xa00009e7 /* nop ; nop                       ; ldtmu0 */ \
, 0x609e7004, 0x570049ef /* nop ; v8muld  tlbam, r0, r4.8dr ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_INTERSECT_SIZE = sizeof(VG_SHADER_MD_IMAGE_INTERSECT);

const uint32_t VG_SHADER_MD_IMAGE_SUBTRACT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xa00009e7 /* nop             ; nop                   ; ldtmu0 */ \
, 0x179e7900, 0xc7020827 /* not  r0, r4.8dr ; nop                   ; loadam */ \
, 0x609e7004, 0x300049ef /* nop             ; v8muld  tlbam, r0, r4 ; thrend */ \
, 0x009e7000, 0x500009e7 /* nop             ; nop                   ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop             ; nop */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_SUBTRACT_SIZE = sizeof(VG_SHADER_MD_IMAGE_SUBTRACT);

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_SET[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0x400009e7 /* nop            ; nop ; sbwait */ \
, 0x009e7000, 0x300009e7 /* nop            ; nop ; thrend */ \
, 0x009e7000, 0xa00009e7 /* nop            ; nop ; ldtmu0 */ \
, 0x159e7900, 0x50020be7 /* mov  tlbam, r4 ; nop ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_SET_SIZE = sizeof(VG_SHADER_MD_IMAGE_NO_ALPHA_SET);

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_UNION[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xc00009e7 /* nop                   ; nop                ; loadam */ \
, 0x979e7924, 0xa0024821 /* not  r0, r4           ; mov  r1, r4        ; ldtmu0 */ \
, 0x609e7004, 0x300049e0 /* nop                   ; v8muld  r0, r0, r4 ; thrend */ \
, 0x1e9e7040, 0x50020be7 /* v8adds  tlbam, r0, r1 ; nop                ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop                   ; nop */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_UNION_SIZE = sizeof(VG_SHADER_MD_IMAGE_NO_ALPHA_UNION);

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_INTERSECT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xc00009e7 /* nop ; nop                   ; loadam */ \
, 0x809e7024, 0x300049e0 /* nop ; mov  r0, r4           ; thrend */ \
, 0x009e7000, 0xa00009e7 /* nop ; nop                   ; ldtmu0 */ \
, 0x609e7004, 0x500049ef /* nop ; v8muld  tlbam, r0, r4 ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_INTERSECT_SIZE = sizeof(VG_SHADER_MD_IMAGE_NO_ALPHA_INTERSECT);

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_SUBTRACT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x089e9fc0, 0x10020827 /* itof  r0, y_coord  ; nop */ \
, 0x01827180, 0x10020827 /* fadd  r0, r0, unif ; nop */ \
, 0x28a60d87, 0x10024860 /* itof  r1, x_coord  ; fmul  r0, r0, unif */ \
, 0x01827380, 0x10020867 /* fadd  r1, r1, unif ; nop */ \
, 0x2082700e, 0x200049e1 /* nop                ; fmul  r1, r1, unif ; thrsw */ \
, 0x159e7000, 0x10020e67 /* mov  t0t, r0       ; nop */ \
, 0x159e7240, 0x10020e27 /* mov  t0s, r1       ; nop */ \
, 0x009e7000, 0xa00009e7 /* nop         ; nop                   ; ldtmu0 */ \
, 0x179e7900, 0xc0020827 /* not  r0, r4 ; nop                   ; loadam */ \
, 0x609e7004, 0x300049ef /* nop         ; v8muld  tlbam, r0, r4 ; thrend */ \
, 0x009e7000, 0x500009e7 /* nop         ; nop                   ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop         ; nop */ \
 };

const uint32_t VG_SHADER_MD_IMAGE_NO_ALPHA_SUBTRACT_SIZE = sizeof(VG_SHADER_MD_IMAGE_NO_ALPHA_SUBTRACT);

const uint32_t VG_SHADER_MD_CVG_EXTRA_THRSWS = 5;

const uint32_t VG_SHADER_MD_CVG_SET[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop            ; nop */ \
, 0x009e7000, 0x700009e7 /* nop            ; nop ; loadcv */ \
, 0x159e7900, 0x30020be7 /* mov  tlbam, r4 ; nop ; thrend */ \
, 0x009e7000, 0x500009e7 /* nop            ; nop ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop            ; nop */ \
 };

const uint32_t VG_SHADER_MD_CVG_SET_SIZE = sizeof(VG_SHADER_MD_CVG_SET);

const uint32_t VG_SHADER_MD_CVG_UNION[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop                   ; nop */ \
, 0x009e7000, 0x700009e7 /* nop                   ; nop                ; loadcv */ \
, 0x979e7924, 0xc0024821 /* not  r0, r4           ; mov  r1, r4        ; loadam */ \
, 0x609e7004, 0x300049e0 /* nop                   ; v8muld  r0, r0, r4 ; thrend */ \
, 0x1e9e7040, 0x50020be7 /* v8adds  tlbam, r0, r1 ; nop                ; sbdone */ \
, 0x009e7000, 0x100009e7 /* nop                   ; nop */ \
 };

const uint32_t VG_SHADER_MD_CVG_UNION_SIZE = sizeof(VG_SHADER_MD_CVG_UNION);

const uint32_t VG_SHADER_MD_CVG_INTERSECT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop ; nop */ \
, 0x009e7000, 0x700009e7 /* nop ; nop                   ; loadcv */ \
, 0x809e7024, 0x300049e0 /* nop ; mov  r0, r4           ; thrend */ \
, 0x009e7000, 0xc00009e7 /* nop ; nop                   ; loadam */ \
, 0x609e7004, 0x500049ef /* nop ; v8muld  tlbam, r0, r4 ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_INTERSECT_SIZE = sizeof(VG_SHADER_MD_CVG_INTERSECT);

const uint32_t VG_SHADER_MD_CVG_SUBTRACT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop         ; nop */ \
, 0x009e7000, 0x700009e7 /* nop         ; nop                   ; loadcv */ \
, 0x179e7900, 0x30020827 /* not  r0, r4 ; nop                   ; thrend */ \
, 0x009e7000, 0xc00009e7 /* nop         ; nop                   ; loadam */ \
, 0x609e7004, 0x500049ef /* nop         ; v8muld  tlbam, r0, r4 ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_SUBTRACT_SIZE = sizeof(VG_SHADER_MD_CVG_SUBTRACT);

const uint32_t VG_SHADER_MD_CVG_SCISSOR_EXTRA_THRSWS = 4;

 \


const uint32_t VG_SHADER_MD_CVG_SCISSOR_SET[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x88a6fdbf, 0xd0025800 /* itof  r0, x_coord ; mov  ra0, f(0.5) */ \
, 0x88829ff6, 0x10024862 /* itof  r1, y_coord ; mov  r2, unif */ \
, 0x810203bf, 0x20024863 /* fadd  r1, r1, ra0 ; mov  r3, unif     ; thrsw */ \
, 0x2102718b, 0x10024839 /* fadd  r0, r0, ra0 ; fmul  t0t, r1, r3 */ \
, 0x209e7002, 0x100049f8 /* nop               ; fmul  t0s, r0, r2 */ \
, 0x009e7000, 0x700009e7 /* nop                ; nop ; loadcv */ \
, 0x159e7900, 0x30020827 /* mov  r0, r4        ; nop ; thrend */ \
, 0x009e7000, 0xa00009e7 /* nop                ; nop ; ldtmu0 */ \
, 0x149e7100, 0x50020be7 /* and  tlbam, r0, r4 ; nop ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_SCISSOR_SET_SIZE = sizeof(VG_SHADER_MD_CVG_SCISSOR_SET);

const uint32_t VG_SHADER_MD_CVG_SCISSOR_UNION[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x88a6fdbf, 0xd0025800 /* itof  r0, x_coord ; mov  ra0, f(0.5) */ \
, 0x88829ff6, 0x10024862 /* itof  r1, y_coord ; mov  r2, unif */ \
, 0x810203bf, 0x20024863 /* fadd  r1, r1, ra0 ; mov  r3, unif     ; thrsw */ \
, 0x2102718b, 0x10024839 /* fadd  r0, r0, ra0 ; fmul  t0t, r1, r3 */ \
, 0x209e7002, 0x100049f8 /* nop               ; fmul  t0s, r0, r2 */ \
, 0x009e7000, 0x700009e7 /* nop                   ; nop                ; loadcv */ \
, 0x159e7900, 0xa0020827 /* mov  r0, r4           ; nop                ; ldtmu0 */ \
, 0x149e7100, 0xc0020827 /* and  r0, r0, r4       ; nop                ; loadam */ \
, 0x179e7000, 0x30020867 /* not  r1, r0           ; nop                ; thrend */ \
, 0x609e700c, 0x100049e1 /* nop                   ; v8muld  r1, r1, r4 */ \
, 0x1e9e7040, 0x50020be7 /* v8adds  tlbam, r0, r1 ; nop                ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_SCISSOR_UNION_SIZE = sizeof(VG_SHADER_MD_CVG_SCISSOR_UNION);

const uint32_t VG_SHADER_MD_CVG_SCISSOR_INTERSECT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x88a6fdbf, 0xd0025800 /* itof  r0, x_coord ; mov  ra0, f(0.5) */ \
, 0x88829ff6, 0x10024862 /* itof  r1, y_coord ; mov  r2, unif */ \
, 0x810203bf, 0x20024863 /* fadd  r1, r1, ra0 ; mov  r3, unif     ; thrsw */ \
, 0x2102718b, 0x10024839 /* fadd  r0, r0, ra0 ; fmul  t0t, r1, r3 */ \
, 0x209e7002, 0x100049f8 /* nop               ; fmul  t0s, r0, r2 */ \
, 0x009e7000, 0x700009e7 /* nop             ; nop                   ; loadcv */ \
, 0x159e7900, 0xa0020827 /* mov  r0, r4     ; nop                   ; ldtmu0 */ \
, 0x149e7100, 0x30020827 /* and  r0, r0, r4 ; nop                   ; thrend */ \
, 0x009e7000, 0xc00009e7 /* nop             ; nop                   ; loadam */ \
, 0x609e7004, 0x500049ef /* nop             ; v8muld  tlbam, r0, r4 ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_SCISSOR_INTERSECT_SIZE = sizeof(VG_SHADER_MD_CVG_SCISSOR_INTERSECT);

const uint32_t VG_SHADER_MD_CVG_SCISSOR_SUBTRACT[] = { \
0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x200009e7 /* thrsw */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x009e7000, 0x100009e7 /* nop */ \
, 0x88a6fdbf, 0xd0025800 /* itof  r0, x_coord ; mov  ra0, f(0.5) */ \
, 0x88829ff6, 0x10024862 /* itof  r1, y_coord ; mov  r2, unif */ \
, 0x810203bf, 0x20024863 /* fadd  r1, r1, ra0 ; mov  r3, unif     ; thrsw */ \
, 0x2102718b, 0x10024839 /* fadd  r0, r0, ra0 ; fmul  t0t, r1, r3 */ \
, 0x209e7002, 0x100049f8 /* nop               ; fmul  t0s, r0, r2 */ \
, 0x009e7000, 0x700009e7 /* nop             ; nop                   ; loadcv */ \
, 0x159e7900, 0xa0020827 /* mov  r0, r4     ; nop                   ; ldtmu0 */ \
, 0x149e7100, 0x30020827 /* and  r0, r0, r4 ; nop                   ; thrend */ \
, 0x179e7000, 0xc0020827 /* not  r0, r0     ; nop                   ; loadam */ \
, 0x609e7004, 0x500049ef /* nop             ; v8muld  tlbam, r0, r4 ; sbdone */ \
 };

const uint32_t VG_SHADER_MD_CVG_SCISSOR_SUBTRACT_SIZE = sizeof(VG_SHADER_MD_CVG_SCISSOR_SUBTRACT);
