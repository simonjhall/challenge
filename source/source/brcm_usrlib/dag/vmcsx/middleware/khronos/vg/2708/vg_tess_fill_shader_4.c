/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
// generated 2010-10-11 19:39:57

#include "middleware/khronos/vg/2708/vg_tess_fill_shader_4.h"
#include <assert.h>
#ifdef SIMPENROSE
#include <stddef.h>
#include "v3d/verification/tools/2760sim/simpenrose.h"

static unsigned int const annotations_0[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_OPEN, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_27[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0xffffffff,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_REGION_SET, 0x00000016,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000005,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_216[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_218[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_251[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_253[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_286[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_288[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_321[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_323[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_356[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_358[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_391[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_393[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_426[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_428[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_461[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_463[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c0c0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_512[] = {
   SIMPENROSE_SHADER_ANNOTATION_PRESERVE_COND, 0x00000001,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000009,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_TRIS_ADD, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_655[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000ff00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_656[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000f000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_657[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_658[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_663[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000ff00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_665[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000f000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_667[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000c000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_669[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x00008000,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_679[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000fff0,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_680[] = {
   SIMPENROSE_SHADER_ANNOTATION_MUL_USED, 0x0000ff00,
   SIMPENROSE_SHADER_ANNOTATION_END};

static unsigned int const annotations_748[] = {
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000003,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_TRIS_SET_CENTER, 0x0000000b,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_I, 0x00000005,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_TRIS_CLEAR, 0x00000000,
   SIMPENROSE_SHADER_ANNOTATION_GEOMD_VERTS, 0x00000006,
   SIMPENROSE_SHADER_ANNOTATION_END};

unsigned int const *const vg_tess_fill_shader_annotations_array[] = {
annotations_0,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_27,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_216,
NULL,
annotations_218,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_251,
NULL,
annotations_253,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_286,
NULL,
annotations_288,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_321,
NULL,
annotations_323,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_356,
NULL,
annotations_358,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_391,
NULL,
annotations_393,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_426,
NULL,
annotations_428,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_461,
NULL,
annotations_463,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_512,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_655,
annotations_656,
annotations_657,
annotations_658,
NULL,
NULL,
NULL,
NULL,
annotations_663,
NULL,
annotations_665,
NULL,
annotations_667,
NULL,
annotations_669,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_679,
annotations_680,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
annotations_748,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
};
#endif

static unsigned int const array[] = {
// ::vg_tess_fill_shader
/* [0x00000000] */ 0xf59e6fc0, 0x1002581e, // mov  r0, qpu_num ; mov  ra_scalar, 0  @geomd_open
/* [0x00000008] */ 0x949c31ff, 0xd0024862, // and  r1, r0, 3   ; mov  r2, 3
/* [0x00000010] */ 0xce9c21d7, 0xd0024822, // shr  r0, r0, 2   ; v8adds  r2, r2, 2
/* [0x00000018] */ 0x519c43c2, 0xd0024860, // shl  r1, r1, 4   ; mul24  r0, r0, r2
/* [0x00000020] */ 0x0c9e7040, 0x10020827, // add  r0, r0, r1
/* [0x00000028] */ 0x00101a00, 0xe0020867, // mov  r1, vpm_setup(1, 1, h32(0))
/* [0x00000030] */ 0x0d988dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_vpm_setup
/* [0x00000038] */ 0x0c9e7200, 0x100407a7, // add.ifz  ra_scalar, r1, r0
/* [0x00000040] */ 0x0c9e7200, 0x10020c67, // add  vr_setup, r1, r0
/* [0x00000048] */ 0x8cc27236, 0x100248e1, // add  r3, r1, r0 ; mov  r1, vpm
/* [0x00000050] */ 0x00003000, 0xe20229e7, // mov.setf  -, elem_range(rsi_alloc_p, 2)
/* [0x00000058] */ 0x809fc009, 0xd000d9de, // nop ; mov.ifnz  ra_scalar, r1 >> rsi_alloc_p
/* [0x00000060] */ 0x80010000, 0xe0020867, // mov  r1, vdr_setup_0(1, 16, dma_h32(0, 0), 0, 8)
/* [0x00000068] */ 0x119c41c0, 0xd00208a7, // shl  r2, r0, 4
/* [0x00000070] */ 0x0d987dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_vdr_setup_0
/* [0x00000078] */ 0x0c9e7280, 0x100407a7, // add.ifz  ra_scalar, r1, r2
/* [0x00000080] */ 0x0c9e7280, 0x10020c67, // add  vr_setup, r1, r2
/* [0x00000088] */ 0x9581fdbf, 0xd0024ca8, // mov  vr_addr, unif ; mov  unif_addr_rel, -1
/* [0x00000090] */ 0x00100a02, 0xe0020867, // mov  r1, vpm_setup(1, 0, h32(2))
/* [0x00000098] */ 0x0d98adc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_out_vpm_setup
/* [0x000000a0] */ 0x0c9e7200, 0x100407a7, // add.ifz  ra_scalar, r1, r0
/* [0x000000a8] */ 0x80904100, 0xe0020867, // mov  r1, vdw_setup_0(1, 16, dma_h32(2, 0))
/* [0x000000b0] */ 0x119c71c0, 0xd00208a7, // shl  r2, r0, 7
/* [0x000000b8] */ 0x0d98bdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_out_vdw_setup_0
/* [0x000000c0] */ 0x0c9e7280, 0x100407a7, // add.ifz  ra_scalar, r1, r2
/* [0x000000c8] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait
/* [0x000000d0] */ 0x159e76c0, 0x10020c67, // mov  vr_setup, r3
/* [0x000000d8] */ 0x15c27d80, 0x10020827, // mov  r0, vpm  @geomd_i(geomd_i_clip_inner) @geomd_region_set_a(11) @geomd_i(geomd_i_fill_pre_center)
/* [0x000000e0] */ 0x957a7036, 0x100269e1, // mov.setf  -, r0 ; mov  r1, ra_scalar
/* [0x000000e8] */ 0x0d982dc0, 0xd00429e7, // sub.ifz.setf  -, elem_num, 2
/* [0x000000f0] */ 0x809f9009, 0xd00049e1, // nop ; mov  r1, r1 << rsi_chunks_vdr_setup_0
/* [0x000000f8] */ 0x8d9d03c9, 0xd0025871, // sub  r1, r1, -16 ; mov  vr_setup, r1
/* [0x00000100] */ 0x00000028, 0xf02809e7, // brr.anyz  -, r:1f
/* [0x00000108] */ 0x809ff000, 0xd00059f2, // nop ; mov  vr_addr, r0 << 1
/* [0x00000110] */ 0x0d981dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunk_lr
/* [0x00000118] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:load
/* [0x00000120] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:load_interp
/* [0x00000128] */ 0x8d988dc9, 0xd00279f1, // sub.setf  -, elem_num, rsi_chunks_vpm_setup ; mov  vr_setup, r1
/* [0x00000130] */ 0x809fe000, 0xd00059f2, // nop ; mov  vr_addr, r0 << 2
/* [0x00000138] */ 0x00100000, 0xe0020867, // mov  r1, 1 << 20
/* [0x00000140] */ 0x0c7a7c40, 0x100407a7, // add.ifz  ra_scalar, ra_scalar, r1
// :1
/* [0x00000148] */ 0x0000007c, 0xe20229e7, // mov.setf  -, elem_range(rsi_tess_i, 5)
/* [0x00000150] */ 0x809f2000, 0xd000d9de, // nop ; mov.ifnz  ra_scalar, r0 >> rsi_tess_i
/* [0x00000158] */ 0x0d985dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_chunks_m_size
/* [0x00000160] */ 0x0d780f80, 0xd00407a7, // sub.ifz  ra_scalar, 0, ra_scalar
/* [0x00000168] */ 0x809fb000, 0xd00059c1, // nop ; mov  ra_user_to_surface_clip, r0 << 5
/* [0x00000170] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr
/* [0x00000178] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:clip_subpath_first
/* [0x00000180] */ 0x7fffffff, 0xe0020467, // mov  ra_min_x, 0x7fffffff
/* [0x00000188] */ 0x80000000, 0xe00204e7, // mov  ra_max_x, 0x80000000
/* [0x00000190] */ 0x7fffffff, 0xe0020567, // mov  ra_min_y, 0x7fffffff
/* [0x00000198] */ 0x80000000, 0xe00205e7, // mov  ra_max_y, 0x80000000
/* [0x000001a0] */ 0x0f0fcccc, 0xe2020667, // mov  ra_subd_flags, 2 * [-2, -2, -1, -1, 0, 0, 1, 1]
/* [0x000001a8] */ 0x0000ffff, 0xe00213a7, // mov  rb_0xffff, 0xffff
/* [0x000001b0] */ 0x00010000, 0xe00213e7, // mov  rb_0x10000, 0x10000
/* [0x000001b8] */ 0x0000004c, 0xe0021427, // mov  rb_76, 76
/* [0x000001c0] */ 0x809f1000, 0xd00049e5, // nop ; mov  r5rep, r0 << 15
/* [0x000001c8] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x000001d0] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x000001d8] */ 0x95ce7dad, 0x100049e1, // mov  -, mutex ; mov  r1, r5
/* [0x000001e0] */ 0x159e7000, 0x10021967, // mov  r5rep, r0
/* [0x000001e8] */ 0x00210a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(2, 16, h32(15))
/* [0x000001f0] */ 0x94c01df6, 0xd0024822, // and  r0, vpm, 1           ; mov  r2, vpm
/* [0x000001f8] */ 0x8d9b0d7f, 0x100269e3, // sub.setf  -, elem_num, r5 ; mov  r3, vpm
/* [0x00000200] */ 0x169e7040, 0x100429e7, // xor.ifz.setf  -, r0, r1
/* [0x00000208] */ 0x00000028, 0xf02809e7, // brr.anyz  -, r:1f
/* [0x00000210] */ 0x8d9a6f9b, 0x100269e0, // sub.setf  -, qpu_num, elem_num ; mov  r0, r3
/* [0x00000218] */ 0x8d9a7d6d, 0x1002a9e3, // sub.setf  -, elem_num, r5      ; mov.ifz  r3, r5
/* [0x00000220] */ 0x00100a1f, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(31))
/* [0x00000228] */ 0x8d9a6f89, 0x1002a9e2, // sub.setf  -, qpu_num, elem_num ; mov.ifz  r2, r1
/* [0x00000230] */ 0xffffffff, 0xe0028823, // mov  r0, -1                    ; mov.ifz  r3, -1
/* [0x00000238] */ 0x8d9e776d, 0x1002a9e3, // sub.setf  -, r3, r5            ; mov.ifz  r3, r5
/* [0x00000240] */ 0x00210a0f, 0xe0021c67, // mov  vw_setup, vpm_setup(2, 16, h32(15))
/* [0x00000248] */ 0x959df4bf, 0xd0028c23, // mov  vpm, r2                   ; mov.ifz  r3, -1
// :1
/* [0x00000250] */ 0x159e76c0, 0x10020c27, // mov  vpm, r3
/* [0x00000258] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00000260] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00000268] */ 0x00000000, 0xe0020ce7, // mov  mutex, 0
/* [0x00000270] */ 0xed9e7140, 0x10024821, // sub  r0, r0, r5 ; mov  r1, 0
/* [0x00000278] */ 0x0d9e63c0, 0x10021967, // sub  r5rep, r1, qpu_num
/* [0x00000280] */ 0x00002000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_size)
/* [0x00000288] */ 0x809f0000, 0xd00049e5, // nop ; mov  r5rep, r0 >> r5
/* [0x00000290] */ 0x159e7b40, 0x100629e7, // mov.ifnz.setf  -, r5
/* [0x00000298] */ 0x00000000, 0xe00607a7, // mov.ifnz  ra_scalar, 0
/* [0x000002a0] */ 0x20002000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x000002a8] */ 0x00000054, 0xe0020827, // mov  r0, 84
/* [0x000002b0] */ 0x0d7a7c00, 0x100829e7, // sub.ifn.setf  -, ra_scalar, r0 ; nop
/* [0x000002b8] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:1f
/* [0x000002c0] */ 0x000016c0, 0xf06809e7, // brr.anyn  -, r:alloc_first
/* [0x000002c8] */ 0xbfff0000, 0xe0021367, // mov  rb_list_tail, 0xbfff0000
/* [0x000002d0] */ 0x159a7d80, 0x100229e7, // mov.setf  -, elem_num ; nop
/* [0x000002d8] */ 0x00000012, 0xe0061367, // mov.ifnz  rb_list_tail, 18
/* [0x000002e0] */ 0x8d788df6, 0xd0024821, // sub  r0, ra_scalar, 8   ; mov  r1, ra_scalar
/* [0x000002e8] */ 0xfff20000, 0xe00208a7, // mov  r2, -(14 << 16)
/* [0x000002f0] */ 0x8c9f6289, 0xd0024871, // add  r1, r1, r2         ; mov  vw_setup, r1 << rsi_out_vpm_setup
/* [0x000002f8] */ 0x809cd03f, 0x100049f0, // nop                     ; mov  vpm, rb_list_tail
/* [0x00000300] */ 0x809f5009, 0xd00049f1, // nop                     ; mov  vw_setup, r1 << rsi_out_vdw_setup_0
/* [0x00000308] */ 0x00003000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x00000310] */ 0x959f4000, 0xd00647b2, // mov.ifnz  ra_scalar, r0 ; mov  vw_addr, r0 << rsi_alloc_p
// :1
/* [0x00000318] */ 0xed984dc0, 0xd00279cd, // sub.setf  -, elem_num, rsi_chunks_b ; mov  ra_subpath_sum_x, f(0.0)
/* [0x00000320] */ 0xf57a7d80, 0x100479cf, // mov.ifz.setf  -, ra_scalar          ; mov  ra_subpath_sum_y, f(0.0)
/* [0x00000328] */ 0x000000a8, 0xf02809e7, // brr.anyz  -, r:load
/* [0x00000330] */ 0x159a7d80, 0x100229e7, // mov.setf  -, elem_num               ; nop
/* [0x00000338] */ 0x10010101, 0xe0061367, // mov.ifnz  rb_list_tail, 0x10010101
/* [0x00000340] */ 0x162a0101, 0xe0021327, // mov  rb_list_head, 0x162a0101
// :load_interp
/* [0x00000348] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar        ; nop
/* [0x00000350] */ 0x95c85dbf, 0xd00049e1, // mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
/* [0x00000358] */ 0x8d9b8c40, 0xd00279f1, // sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
/* [0x00000360] */ 0x00000040, 0xe0020867, // mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
/* [0x00000368] */ 0x8c9fa040, 0xd00469e5, // add.ifz.setf  -, r0, r1   ; mov  r5rep, r0 << rsi_t
/* [0x00000370] */ 0x829e0f6d, 0xd00248e2, // fsub  r3, f(1.0), r5      ; mov  r2, r5
/* [0x00000378] */ 0x35c27d9e, 0x10025963, // mov  r5rep, vpm           ; fmul  r3, r3, vpm
/* [0x00000380] */ 0x000000c8, 0xf02809e7, // brr.anyz  -, r:chunk_normal
/* [0x00000388] */ 0x35c27b56, 0x10024722, // mov  ra_chunk, r5         ; fmul  r2, r2, vpm
/* [0x00000390] */ 0xe19e74c0, 0x1002989e, // fadd  r2, r2, r3          ; mov.ifz  ra_scalar, 0
/* [0x00000398] */ 0x95739d80, 0xd80269e3, // mov.setf  -, ra_chunk.8a  ; mov  r3, r0 << rsi_chunks_vdr_setup_0
/* [0x000003a0] */ 0x00000028, 0xe0040867, // mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
/* [0x000003a8] */ 0x8c9e705b, 0x10025831, // add  r0, r0, r1           ; mov  vr_setup, r3
/* [0x000003b0] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:chunk_normal
/* [0x000003b8] */ 0x809fd000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_a
/* [0x000003c0] */ 0xdeadbeef, 0xe00402a7, // mov.ifz  ra_temp, a:chunk_cubic
/* [0x000003c8] */ 0x0d9d07c0, 0xd0020c67, // sub  vr_setup, r3, -16    ; nop
/* [0x000003d0] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x000003d8] */ 0x809fc000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_b
/* [0x000003e0] */ 0x00000038, 0xe20229e7, // mov.setf  -, elems(rsi_chunks_a, rsi_chunks_b, rsi_chunks_m_size)
/* [0x000003e8] */ 0x159e7000, 0x100607a7, // mov.ifnz  ra_scalar, r0   ; nop
// :load
/* [0x000003f0] */ 0x157a7d80, 0x10020827, // mov  r0, ra_scalar        ; nop
/* [0x000003f8] */ 0x95c85dbf, 0xd00049e1, // mov  -, vr_wait           ; mov  r1, rsi_chunks_m_size
/* [0x00000400] */ 0x8d9b8c40, 0xd00279f1, // sub.setf  -, elem_num, r1 ; mov  vr_setup, r0 << rsi_chunks_vpm_setup
/* [0x00000408] */ 0x00000040, 0xe0020867, // mov  r1, VG_TESS_CHUNK_NORMAL_SIZE
/* [0x00000410] */ 0x0c9e7040, 0x100429e7, // add.ifz.setf  -, r0, r1   ; nop
/* [0x00000418] */ 0x00000030, 0xf02809e7, // brr.anyz  -, r:chunk_normal
/* [0x00000420] */ 0xd69cc03f, 0xd00447a3, // mov.ifz  ra_scalar, 0     ; v8adds  r3, 12, 12
/* [0x00000428] */ 0x95c27db6, 0x10025962, // mov  r5rep, vpm           ; mov  r2, vpm
/* [0x00000430] */ 0x919e7aed, 0x100279dc, // shl.setf  -, r5, r3       ; mov  ra_chunk, r5
/* [0x00000438] */ 0x00000028, 0xe0040867, // mov.ifz  r1, VG_TESS_CHUNK_CUBIC_SIZE
/* [0x00000440] */ 0x8c9f9040, 0xd0025831, // add  r0, r0, r1           ; mov  vr_setup, r0 << rsi_chunks_vdr_setup_0
/* [0x00000448] */ 0x000000e0, 0xf00809e7, // brr.allz  -, r:chunk_cubic
/* [0x00000450] */ 0x809fd000, 0xd00059f2, // nop                       ; mov  vr_addr, r0 << rsi_chunks_a
/* [0x00000458] */ 0x00000028, 0xe20229e7, // mov.setf  -, elems(rsi_chunks_a, rsi_chunks_m_size)
/* [0x00000460] */ 0x159e7000, 0x100607a7, // mov.ifnz  ra_scalar, r0   ; nop
// :chunk_normal
/* [0x00000468] */ 0x15067d80, 0x10020827, // mov  r0, ra_user_to_surface_clip ; nop
/* [0x00000470] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:end_subpath
/* [0x00000478] */ 0x9573dd80, 0xda0269e5, // mov.setf  -, ra_chunk.8b    ; mov  r5rep, r0 << 3
/* [0x00000480] */ 0x357bfd95, 0xd00442a1, // mov.ifz  ra_temp, ra_scalar ; fmul  r1 >> 1, r2, r5
/* [0x00000488] */ 0x809fc000, 0xd00049e5, // nop                         ; mov  r5rep, r0 << 4
/* [0x00000490] */ 0x209f8015, 0xd00049e3, // nop                         ; fmul  r3 >> 8, r2, r5
/* [0x00000498] */ 0x819fb2c0, 0xd0024865, // fadd  r1, r1, r3            ; mov  r5rep, r0 << 5
/* [0x000004a0] */ 0x819ff340, 0xd0024865, // fadd  r1, r1, r5            ; mov  r5rep, r0 << 1
/* [0x000004a8] */ 0x359f8015, 0xd0025963, // mov  r5rep, r0 << 0         ; fmul  r3 >> 8, r2, r5
/* [0x000004b0] */ 0x359bfd95, 0xd00269e2, // mov.setf  -, elem_num       ; fmul  r2 >> 1, r2, r5
/* [0x000004b8] */ 0x819fe4c0, 0xd0024825, // fadd  r0, r2, r3            ; mov  r5rep, r0 << 2
/* [0x000004c0] */ 0x812bf176, 0xd0024825, // fadd  r0, r0, r5            ; mov  r5rep, ra_temp << rsi_chunk_lr
/* [0x000004c8] */ 0x957a7b76, 0x100242a5, // mov  ra_temp, r5            ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x000004d0] */ 0xcc708bbf, 0xd80248a3, // add  r2, r5, ra_chunk.8a    ; v8adds  r3, 8, 8
/* [0x000004d8] */ 0x8d9e74d3, 0x1002b89e, // sub.setf  r2, r2, r3        ; v8min.ifz  ra_scalar, r2, r3
/* [0x000004e0] */ 0x00000000, 0xf04549e7, // bra.alln  -, ra_temp
/* [0x000004e8] */ 0x8d98fd7f, 0xd00269e3, // sub.setf  -, elem_num, r5   ; mov  r3, 15
/* [0x000004f0] */ 0x809f0000, 0xd00159c3, // nop                         ; mov.ifnn  ra_clip_q_x, r0 >> r5
/* [0x000004f8] */ 0x8d9b0cc9, 0xd00379c5, // sub.setf  -, elem_num, r3   ; mov.ifnn  ra_clip_q_y, r1 >> r5
/* [0x00000500] */ 0x00000000, 0xf0f7c6e7, // bra  ra_temp2, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000508] */ 0x959f0480, 0xd004d01c, // mov.ifz  rb_chunk, r2       ; mov.ifnz  ra_chunk, r0 >> r5
/* [0x00000510] */ 0x952b0d89, 0xd004c700, // mov.ifz  ra_chunk, ra_temp  ; mov.ifnz  rb_chunk, r1 >> r5
/* [0x00000518] */ 0x156e7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp2 ; nop
/* [0x00000520] */ 0x95700ff6, 0x10025803, // mov  r0, rb_chunk           ; mov  ra_clip_q_x, ra_chunk
/* [0x00000528] */ 0x00000000, 0xf0f789e7, // bra  -, ra_chunk
/* [0x00000530] */ 0x95980dbf, 0x100279c5, // mov.setf  -, elem_num       ; mov  ra_clip_q_y, rb_chunk
/* [0x00000538] */ 0x809f1000, 0xd00099de, // nop                         ; mov.ifz  ra_scalar, r0 << 15
/* [0x00000540] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :chunk_cubic
/* [0x00000548] */ 0x15067d80, 0x10020827, // mov  r0, ra_user_to_surface_clip ; nop
/* [0x00000550] */ 0x14981dc0, 0xd00229e7, // and.setf  -, elem_num, 1  ; nop
/* [0x00000558] */ 0x809fd000, 0xd00049e5, // nop                       ; mov  r5rep, r0 << 3
/* [0x00000560] */ 0x209e7015, 0x100049e1, // nop                       ; fmul  r1 >> 0, r2, r5
/* [0x00000568] */ 0x809fc000, 0xd00049e5, // nop                       ; mov  r5rep, r0 << 4
/* [0x00000570] */ 0x209ff015, 0xd00049e3, // nop                       ; fmul  r3 >> 1, r2, r5
/* [0x00000578] */ 0x809ff000, 0xd00049e5, // nop                       ; mov  r5rep, r0 << 1
/* [0x00000580] */ 0x359fe015, 0xd0029961, // mov  r5rep, r0 << 0       ; fmul.ifz  r1 >> 2, r2, r5
/* [0x00000588] */ 0x209ff015, 0xd00089e3, // nop                       ; fmul.ifz  r3 >> 1, r2, r5
/* [0x00000590] */ 0x819fe2c0, 0xd0024865, // fadd  r1, r1, r3          ; mov  r5rep, r0 << 2
/* [0x00000598] */ 0x819fb340, 0xd0044865, // fadd.ifz  r1, r1, r5      ; mov  r5rep, r0 << 5
/* [0x000005a0] */ 0x817a7376, 0x10064865, // fadd.ifnz  r1, r1, r5     ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x000005a8] */ 0x95718dbf, 0xda0269e0, // mov.setf  -, ra_chunk.8b  ; mov  r0, -8
/* [0x000005b0] */ 0x000000b8, 0xf00809e7, // brr.allz  -, r:3f
/* [0x000005b8] */ 0x8c9bac09, 0xd00269c0, // add.setf  -, elem_num, r0 ; mov  rb_chunk, r1 << 6
/* [0x000005c0] */ 0x959b8d89, 0xd00369e1, // mov.setf  -, elem_num     ; mov.ifnn  r1, r1 >> 8
/* [0x000005c8] */ 0xdeadbeef, 0xe00206a7, // mov  ra_cubic_lrs[0], a:1f
/* [0x000005d0] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num ; v8adds.ifz  ra_scalar, r5, 1
/* [0x000005d8] */ 0x00000090, 0xf06809e7, // brr.anyn  -, r:3f
/* [0x000005e0] */ 0x809ff009, 0xd00049e0, // nop                       ; mov  r0, r1 << 1
/* [0x000005e8] */ 0x809f0009, 0xd00099c3, // nop                       ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x000005f0] */ 0x809f0000, 0xd00099c5, // nop                       ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x000005f8] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000600] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000608] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:2f
/* [0x00000610] */ 0x009e7000, 0x100009e7, // nop                       ; nop
// :1
/* [0x00000618] */ 0x807bf036, 0xd00049e5, // nop                       ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x00000620] */ 0x957a7b76, 0x100242a5, // mov  ra_temp, r5          ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000628] */ 0x95980dbf, 0x100269e0, // mov.setf  -, elem_num     ; mov  r0, rb_chunk
/* [0x00000630] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000638] */ 0x00000000, 0xf06549e7, // bra.anyn  -, ra_temp
/* [0x00000640] */ 0x809ff000, 0xd00049e1, // nop                       ; mov  r1, r0 << 1
/* [0x00000648] */ 0x809f0000, 0xd00099c3, // nop                       ; mov.ifz  ra_clip_q_x, r0 >> r5
/* [0x00000650] */ 0x809f0009, 0xd00099c5, // nop                       ; mov.ifz  ra_clip_q_y, r1 >> r5
/* [0x00000658] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000660] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr ; nop
/* [0x00000668] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000670] */ 0x009e7000, 0x100009e7, // nop                       ; nop
// :2
/* [0x00000678] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk         ; nop
/* [0x00000680] */ 0x009e7000, 0x100009e7, // nop                       ; nop
// :3
/* [0x00000688] */ 0x959f2249, 0xd0025060, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x00000690] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x00000698] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x000006a0] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x000006a8] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x000006b0] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x000006b8] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x000006c0] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x000006c8] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x000006d0] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x000006d8] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x000006e0] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x000006e8] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x000006f0] */ 0x00000000, 0xf04749e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x000006f8] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000700] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000708] */ 0x95201dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000710] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000718] */ 0x00000068, 0xf0f80627, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000720] */ 0x219fc018, 0xd0030801, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000728] */ 0x809fa009, 0xd00189c1, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000730] */ 0x209fe010, 0xd00089c1, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000738] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000740] */ 0x95981dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000748] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000750] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000758] */ 0x956bfd89, 0xd0024620, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000760] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000768] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000770] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000778] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000780] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000788] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000790] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x00000798] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x000007a0] */ 0x959f2249, 0xd00250a0, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x000007a8] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x000007b0] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x000007b8] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x000007c0] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x000007c8] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x000007d0] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x000007d8] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x000007e0] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x000007e8] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x000007f0] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x000007f8] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000800] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000808] */ 0x00000000, 0xf04709e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000810] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000818] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000820] */ 0x95202dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000828] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000830] */ 0x00000068, 0xf0f805a7, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000838] */ 0x219fc018, 0xd0030802, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000840] */ 0x809fa009, 0xd00189c2, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000848] */ 0x209fe010, 0xd00089c2, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000850] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000858] */ 0x95982dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000860] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000868] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000870] */ 0x9563fd89, 0xd00245a0, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000878] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000880] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000888] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000890] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000898] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x000008a0] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x000008a8] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x000008b0] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x000008b8] */ 0x959f2249, 0xd00250e0, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x000008c0] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x000008c8] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x000008d0] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x000008d8] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x000008e0] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x000008e8] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x000008f0] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x000008f8] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000900] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000908] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000910] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000918] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000920] */ 0x00000000, 0xf046c9e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000928] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000930] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000938] */ 0x95203dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000940] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000948] */ 0x00000068, 0xf0f80527, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000950] */ 0x219fc018, 0xd0030803, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000958] */ 0x809fa009, 0xd00189c3, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000960] */ 0x209fe010, 0xd00089c3, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000968] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000970] */ 0x95983dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000978] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000980] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000988] */ 0x955bfd89, 0xd0024520, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000990] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000998] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x000009a0] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x000009a8] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x000009b0] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x000009b8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x000009c0] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x000009c8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x000009d0] */ 0x959f2249, 0xd0025120, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x000009d8] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x000009e0] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x000009e8] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x000009f0] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x000009f8] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x00000a00] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x00000a08] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x00000a10] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000a18] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000a20] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000a28] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000a30] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000a38] */ 0x00000000, 0xf04689e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000a40] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000a48] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000a50] */ 0x95204dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000a58] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000a60] */ 0x00000068, 0xf0f804a7, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000a68] */ 0x219fc018, 0xd0030804, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000a70] */ 0x809fa009, 0xd00189c4, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000a78] */ 0x209fe010, 0xd00089c4, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000a80] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000a88] */ 0x95984dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000a90] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000a98] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000aa0] */ 0x9553fd89, 0xd00244a0, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000aa8] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000ab0] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000ab8] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000ac0] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000ac8] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000ad0] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000ad8] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x00000ae0] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x00000ae8] */ 0x959f2249, 0xd0025160, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x00000af0] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x00000af8] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x00000b00] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x00000b08] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x00000b10] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x00000b18] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x00000b20] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x00000b28] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000b30] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000b38] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000b40] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000b48] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000b50] */ 0x00000000, 0xf04649e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000b58] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000b60] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000b68] */ 0x95205dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000b70] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000b78] */ 0x00000068, 0xf0f80427, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000b80] */ 0x219fc018, 0xd0030805, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000b88] */ 0x809fa009, 0xd00189c5, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000b90] */ 0x209fe010, 0xd00089c5, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000b98] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000ba0] */ 0x95985dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000ba8] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000bb0] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000bb8] */ 0x954bfd89, 0xd0024420, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000bc0] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000bc8] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000bd0] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000bd8] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000be0] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000be8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000bf0] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x00000bf8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x00000c00] */ 0x959f2249, 0xd00251a0, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x00000c08] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x00000c10] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x00000c18] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x00000c20] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x00000c28] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x00000c30] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x00000c38] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x00000c40] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000c48] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000c50] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000c58] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000c60] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000c68] */ 0x00000000, 0xf04609e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000c70] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000c78] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000c80] */ 0x95206dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000c88] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000c90] */ 0x00000068, 0xf0f803a7, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000c98] */ 0x219fc018, 0xd0030806, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000ca0] */ 0x809fa009, 0xd00189c6, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000ca8] */ 0x209fe010, 0xd00089c6, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000cb0] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000cb8] */ 0x95986dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000cc0] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000cc8] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000cd0] */ 0x9543fd89, 0xd00243a0, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000cd8] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000ce0] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000ce8] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000cf0] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000cf8] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000d00] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000d08] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x00000d10] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x00000d18] */ 0x959f2249, 0xd00251e0, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x00000d20] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x00000d28] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x00000d30] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x00000d38] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x00000d40] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x00000d48] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x00000d50] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x00000d58] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000d60] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000d68] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000d70] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000d78] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000d80] */ 0x00000000, 0xf045c9e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000d88] */ 0x1365ff80, 0xd00229e7, // max.setf  -, -1, ra_subd_flags ; nop
/* [0x00000d90] */ 0x356efdb7, 0xd00308a1, // mov  r2, ra_temp2           ; fmul.ifn  r1, ra_temp2, f(0.5)
/* [0x00000d98] */ 0x95207dbf, 0x100388e1, // mov  r3, ra_temp3           ; mov.ifc  r1, rb_cubics[i]
/* [0x00000da0] */ 0x3522eff7, 0xd0028821, // mov  r0, f(0.25)            ; fmul.ifz  r1, ra_temp3, f(0.25)
/* [0x00000da8] */ 0x00000068, 0xf0f80327, // brr  ra_cubic_lrs[i + 1], r:1f
/* [0x00000db0] */ 0x219fc018, 0xd0030807, // fadd  r0, r0, r0            ; fmul.ifn  rb_cubics[i], r3 << 4, r0 << 4
/* [0x00000db8] */ 0x809fa009, 0xd00189c7, // nop                         ; mov.ifc  rb_cubics[i], r1 << 6
/* [0x00000dc0] */ 0x209fe010, 0xd00089c7, // nop                         ; fmul.ifz  rb_cubics[i], r2 << 2, r0 << 2
/* [0x00000dc8] */ 0x807a7036, 0x100049e5, // nop                         ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x00000dd0] */ 0x95987dbf, 0x100269e1, // mov.setf  -, elem_num       ; mov  r1, rb_cubics[i]
/* [0x00000dd8] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000de0] */ 0x00000030, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x00000de8] */ 0x953bfd89, 0xd0024320, // mov  ra_cubic_lrs[i + 1], ra_cubic_lrs[i] ; mov  r0, r1 << 1
/* [0x00000df0] */ 0x809f0009, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x00000df8] */ 0x809f0000, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r0 >> r5
/* [0x00000e00] */ 0x00000000, 0xf0f7c2a7, // bra  ra_temp, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000e08] */ 0x8d98fdc9, 0xd00279dc, // sub.setf  -, elem_num, rsi_clip_lr ; mov  ra_chunk, r1
/* [0x00000e10] */ 0x152a7d80, 0x100407a7, // mov.ifz  ra_scalar, ra_temp ; nop
/* [0x00000e18] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000e20] */ 0x15727d80, 0x10020867, // mov  r1, ra_chunk           ; nop
/* [0x00000e28] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :1
/* [0x00000e30] */ 0x959f2249, 0xd0025220, // mov  rb_cubics[i], r1       ; mov  r0, r1 >> 2
/* [0x00000e38] */ 0x829e207f, 0xd00248a5, // fsub  r2, r0, r1            ; mov  r5rep, f(VG_TESS_OO_FLATNESS)
/* [0x00000e40] */ 0x219e7052, 0x1002580a, // fadd  r0, r0, r1            ; fmul  ra_temp, r2, r2
/* [0x00000e48] */ 0x359ff015, 0xd00246e1, // mov  ra_temp2, r0           ; fmul  r1, r2 << 1, r5 << 1
/* [0x00000e50] */ 0x209e700a, 0x100049e1, // nop                         ; fmul  r1, r1, r2
/* [0x00000e58] */ 0x809f2000, 0xd00049e3, // nop                         ; mov  r3, r0 >> 2
/* [0x00000e60] */ 0x219fd0d5, 0xd0024220, // fadd  ra_temp3, r0, r3      ; fmul  r0, r2 << 3, r5 << 3
/* [0x00000e68] */ 0x952bfdb6, 0xd0025963, // mov  r5rep, ra_temp         ; mov  r3, ra_temp << 1   @mul_used(0)
/* [0x00000e70] */ 0x219f1742, 0xd0025960, // fadd  r5rep, r3, r5         ; fmul  r0 << 1, r0, r2
/* [0x00000e78] */ 0x82232236, 0xd0024821, // fsub  r0, r1, r0            ; mov  r1, ra_temp3 >> 2  @mul_used(6, 7, 14, 15)
/* [0x00000e80] */ 0x21227c40, 0x10024860, // fadd  r1, ra_temp3, r1      ; fmul  r0, r0, r0
/* [0x00000e88] */ 0x229ed14f, 0xd00269e1, // fsub.setf  -, r0, r5        ; fmul  r1, r1, f(0.125)
/* [0x00000e90] */ 0xff7dff7d, 0xe20a29e7, // mov.ifnn.setf  -, [-1, 0, -1, -1, -1, -1, -1, 0] + (8 * [-1])
/* [0x00000e98] */ 0x00000000, 0xf04589e7, // bra.alln  -, ra_cubic_lrs[i]
/* [0x00000ea0] */ 0x957bad89, 0xd0025960, // mov  r5rep, ra_scalar << rsi_clip_q_n ; mov  r0, r1 << 6
/* [0x00000ea8] */ 0x959b9d89, 0xd00269e1, // mov.setf  -, elem_num       ; mov  r1, r1 << 7
/* [0x00000eb0] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000eb8] */ 0xcd981baf, 0xd002b9de, // sub.setf  -, r5, elem_num   ; v8adds.ifz  ra_scalar, r5, 1
/* [0x00000ec0] */ 0x00000000, 0xf06589e7, // bra.anyn  -, ra_cubic_lrs[i]
/* [0x00000ec8] */ 0x809f0000, 0xd00099c3, // nop                         ; mov.ifz  ra_clip_q_x, r0 >> r5
/* [0x00000ed0] */ 0x809f0009, 0xd00099c5, // nop                         ; mov.ifz  ra_clip_q_y, r1 >> r5
/* [0x00000ed8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
/* [0x00000ee0] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000ee8] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr ; nop
/* [0x00000ef0] */ 0x15327d80, 0x100407a7, // mov.ifz  ra_scalar, ra_cubic_lrs[i] ; nop
/* [0x00000ef8] */ 0x009e7000, 0x100009e7, // nop                         ; nop
// :clip_subpath_first
/* [0x00000f00] */ 0x15067d80, 0x10020827, // mov  r0, ra_user_to_surface_clip ; nop
/* [0x00000f08] */ 0x009e7000, 0x100009e7, // nop                          ; nop
/* [0x00000f10] */ 0x809fa000, 0xd00049e1, // nop                          ; mov  r1, r0 << 6
/* [0x00000f18] */ 0x840f8c40, 0xd00248a1, // fmax  r2, ra_clip_q_x, r1    ; mov  r1, r0 << 8
/* [0x00000f20] */ 0x839f9440, 0xd0025261, // fmin  rb_clip_prev_x, r2, r1 ; mov  r1, r0 << 7
/* [0x00000f28] */ 0x84177c40, 0xd00248a1, // fmax  r2, ra_clip_q_y, r1    ; mov  r1, r0 << 9
/* [0x00000f30] */ 0x039e7440, 0x100212a7, // fmin  rb_clip_prev_y, r2, r1 ; nop
// :clip_inside
/* [0x00000f38] */ 0x15067d80, 0x10020827, // mov  r0, ra_user_to_surface_clip   ; nop
/* [0x00000f40] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:clip_inside
/* [0x00000f48] */ 0x957bad80, 0xd0024865, // mov  r1, ra_scalar                 ; mov  r5rep, r0 << 6
/* [0x00000f50] */ 0x820f9d40, 0xd00269e5, // fsub.setf  -, ra_clip_q_x, r5      ; mov  r5rep, r0 << 7
/* [0x00000f58] */ 0x82178d40, 0xd00a69e5, // fsub.ifnn.setf  -, ra_clip_q_y, r5 ; mov  r5rep, r0 << 8
/* [0x00000f60] */ 0x820f7b80, 0xd00a69e5, // fsub.ifnn.setf  -, r5, ra_clip_q_x ; mov  r5rep, r0 << 9
/* [0x00000f68] */ 0x82167b89, 0x100a69e5, // fsub.ifnn.setf  -, r5, ra_clip_q_y ; mov  r5rep, r1 << rsi_clip_q_n
/* [0x00000f70] */ 0x0d9a7d40, 0x100829e7, // sub.ifn.setf  -, elem_num, r5      ; nop
/* [0x00000f78] */ 0x00000100, 0xf06809e7, // brr.anyn  -, r:clip_inside_to_outside
/* [0x00000f80] */ 0x800e7036, 0x100049e1, // nop                                ; mov  r1, ra_clip_q_x
/* [0x00000f88] */ 0x8d151f76, 0xd0025962, // sub  r5rep, -15, r5                ; mov  r2, ra_clip_q_y
/* [0x00000f90] */ 0x9578fff6, 0xd0025808, // mov  r0, rsi_clip_lr               ; mov  ra_temp3, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00000f98] */ 0x8d9b0c09, 0xd00269c9, // sub.setf  -, elem_num, r0          ; mov  rb_clip_prev_x, r1 >> r5
/* [0x00000fa0] */ 0x952b0d92, 0xd004478a, // mov.ifz  ra_scalar, ra_temp        ; mov  rb_clip_prev_y, r2 >> r5
/* [0x00000fa8] */ 0x0d9d1f40, 0xd0021967, // sub  r5rep, -15, r5                ; nop
// :clip_out
/* [0x00000fb0] */ 0x350e4ff7, 0xd0024860, // mov  r1, f(16.0)                ; fmul  r0, ra_clip_q_x, f(16.0)
/* [0x00000fb8] */ 0x817af1f6, 0xd00268a3, // fadd.setf  r2, r0, f(0.5)       ; mov  r3, ra_scalar
/* [0x00000fc0] */ 0x221605f1, 0xd00848a1, // fsub.ifn  r2, r2, f(1.0)        ; fmul  r1, ra_clip_q_y, r1
/* [0x00000fc8] */ 0x879f649b, 0xd00248b1, // ftoi  r2, r2                    ; mov  vw_setup, r3 << rsi_out_vpm_setup
/* [0x00000fd0] */ 0x019ef3c0, 0xd00228e7, // fadd.setf  r3, r1, f(0.5)       ; nop
/* [0x00000fd8] */ 0x029e07c0, 0xd00808e7, // fsub.ifn  r3, r3, f(1.0)        ; nop
/* [0x00000fe0] */ 0x079e76c0, 0x100208e7, // ftoi  r3, r3                    ; nop
/* [0x00000fe8] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5       ; nop
/* [0x00000ff0] */ 0x8134ec17, 0x10084360, // fadd.ifn  ra_subpath_sum_x, ra_subpath_sum_x, r0 ; v8min  r0, r2, rb_0xffff
/* [0x00000ff8] */ 0x413cfc5f, 0x100843e1, // fadd.ifn  ra_subpath_sum_y, ra_subpath_sum_y, r1 ; mul24  r1, r3, rb_0x10000
/* [0x00001000] */ 0xb2467c81, 0x10090467, // min.ifn  ra_min_x, ra_min_x, r2 ; v8max.ifn  -, r0, r1  @preserve_cond @geomd_verts_m(4) @geomd_tris_add
/* [0x00001008] */ 0xb34f3c81, 0xd002580a, // max  r0, ra_max_x, r2           ; v8max  ra_temp, r0 >> 3, r1 >> 3
/* [0x00001010] */ 0x92572cff, 0x10080567, // min.ifn  ra_min_y, ra_min_y, r3 ; mov  -, vw_wait
/* [0x00001018] */ 0x952be036, 0xd00844e0, // mov.ifn  ra_max_x, r0           ; mov  r0, ra_temp << 2
/* [0x00001020] */ 0x14984dc0, 0xd00229e7, // and.setf  -, elem_num, 4        ; nop
/* [0x00001028] */ 0x809f8000, 0xd000c9e0, // nop                             ; mov.ifnz  r0, r0 << 8
/* [0x00001030] */ 0x20002000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00001038] */ 0x0d790dc0, 0x100829e7, // sub.ifn.setf  -, ra_scalar, rb_76 ; nop
/* [0x00001040] */ 0x000008f8, 0xf06802a7, // brr.anyn  ra_temp, r:alloc
/* [0x00001048] */ 0x94981ded, 0xd00279dd, // and.setf  -, elem_num, 1        ; mov  ra_temp4, r5
/* [0x00001050] */ 0x8d9bed40, 0xd002e9e0, // sub.setf  -, elem_num, r5       ; mov.ifnz  r0, r0 << 2
/* [0x00001058] */ 0x935e7cc0, 0x100845f0, // max.ifn  ra_max_y, ra_max_y, r3 ; mov  vpm, r0
/* [0x00001060] */ 0x55988dbf, 0xd00269e1, // mov.setf  -, elem_num           ; mul24  r1, 8, 8
/* [0x00001068] */ 0x8d7a7c76, 0x10024860, // sub  r1, ra_scalar, r1          ; mov  r0, ra_scalar
/* [0x00001070] */ 0x4d7441b7, 0xd00447a2, // sub.ifz  ra_scalar, r0, ra_temp4 ; mul24  r2, ra_temp4, 4
/* [0x00001078] */ 0x00000000, 0xf0f509e7, // bra  -, ra_temp3
/* [0x00001080] */ 0x00003200, 0xe20229e7, // mov.setf  -, elems(rsi_subpath_n, rsi_alloc_p, rsi_alloc_size)
/* [0x00001088] */ 0x8d9f5080, 0xd00647b1, // sub.ifnz  ra_scalar, r0, r2     ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00001090] */ 0x809f4009, 0xd00049f2, // nop                             ; mov  vw_addr, r1 << rsi_alloc_p
// :clip_inside_to_outside
/* [0x00001098] */ 0xec990ffe, 0xd00b0820, // add.ifnn  r0, -16, -16             ; v8subs.ifn  r0, -16, elem_num
/* [0x000010a0] */ 0x95982dbf, 0xd00279ca, // mov.setf  -, elem_num              ; mov  ra_temp, 2
/* [0x000010a8] */ 0x809f1009, 0xd000c9c9, // nop                                ; mov.ifnz  rb_clip_prev_x, r1 >> 1
/* [0x000010b0] */ 0x95071d92, 0xd002c8ca, // mov  r3, ra_user_to_surface_clip   ; mov.ifnz  rb_clip_prev_y, r2 >> 1
/* [0x000010b8] */ 0x809f8000, 0xd00049e1, // nop                                ; mov  r1, r0 << 8
/* [0x000010c0] */ 0x939fa05b, 0xd0024825, // max  r0, r0, r1                    ; mov  r5rep, r3 << 6
/* [0x000010c8] */ 0x820e7d6d, 0x100269e2, // fsub.setf  -, ra_clip_q_x, r5      ; mov  r2, r5
/* [0x000010d0] */ 0x969f801b, 0xd00842a5, // mov.ifn  ra_temp, 0                ; mov  r5rep, r3 << 8
/* [0x000010d8] */ 0x820e7bad, 0x100b69e2, // fsub.ifnn.setf  -, r5, ra_clip_q_x ; mov.ifnn  r2, r5
/* [0x000010e0] */ 0x820c9f92, 0x10091d1b, // fsub.ifn  recip, rb_clip_prev_x, ra_clip_q_x ; mov.ifn  ra_temp2, r2
/* [0x000010e8] */ 0x809f901b, 0xd00059c8, // nop                                ; mov  ra_temp3, r3 << 7
/* [0x000010f0] */ 0x820fc580, 0xd0084862, // fsub.ifn  r1, r2, ra_clip_q_x      ; mov  r2, r0 << 4
/* [0x000010f8] */ 0x2214af8c, 0x100908e1, // fsub.ifn  r3, rb_clip_prev_y, ra_clip_q_y ; fmul.ifn  r1, r1, r4
/* [0x00001100] */ 0x830e03f6, 0xd009585b, // fmin.ifn  r1, r1, f(1.0)           ; mov.ifnn  ra_temp2, ra_clip_q_x
/* [0x00001108] */ 0x339e708b, 0x10030821, // max  r0, r0, r2                    ; fmul.ifn  r1, r1, r3
/* [0x00001110] */ 0x81167c76, 0x100948e3, // fadd.ifn  r3, ra_clip_q_y, r1      ; mov.ifnn  r3, ra_clip_q_y
/* [0x00001118] */ 0x95201ff6, 0xd00248a5, // mov  r2, 1                         ; mov  r5rep, ra_temp3
/* [0x00001120] */ 0x829e776d, 0x100269e1, // fsub.setf  -, r3, r5               ; mov  r1, r5
/* [0x00001128] */ 0x9823e4b6, 0xd00a48a5, // clz.ifnn  r2, r2                   ; mov  r5rep, ra_temp3 << 2
/* [0x00001130] */ 0x829e7aed, 0x100b69e1, // fsub.ifnn.setf  -, r5, r3          ; mov.ifnn  r1, r5
/* [0x00001138] */ 0x829caec9, 0x10091d08, // fsub.ifn  recip, rb_clip_prev_y, r3 ; mov.ifn  ra_temp3, r1
/* [0x00001140] */ 0x929835f6, 0xd00842a2, // min.ifn  ra_temp, r2, 3            ; mov  r2, elem_num
/* [0x00001148] */ 0x829fe2c0, 0xd008585d, // fsub.ifn  r1, r1, r3               ; mov  ra_temp4, r0 << 2
/* [0x00001150] */ 0x226c9f8c, 0x100908e1, // fsub.ifn  r3, rb_clip_prev_x, ra_temp2 ; fmul.ifn  r1, r1, r4
/* [0x00001158] */ 0xa37603c6, 0xd0084860, // fmin.ifn  r1, r1, f(1.0)           ; v8max  r0, r0, ra_temp4
/* [0x00001160] */ 0x35227d8b, 0x100908e1, // mov.ifn  r3, ra_temp3              ; fmul.ifn  r1, r1, r3
/* [0x00001168] */ 0x816ffc40, 0xd00846e1, // fadd.ifn  ra_temp2, ra_temp2, r1   ; mov  r1, r0 << 1
/* [0x00001170] */ 0xd3281077, 0xd0025960, // max  r5rep, r0, r1                 ; v8adds  r0, ra_temp, 1
/* [0x00001178] */ 0x8d0d0eb6, 0xd00248a1, // sub  r2, -16, r2                   ; mov  r1, ra_clip_q_x
/* [0x00001180] */ 0x8d167576, 0x100269e2, // sub.setf  -, r2, r5                ; mov  r2, ra_clip_q_y
/* [0x00001188] */ 0x809f0000, 0xd00059c7, // nop                                ; mov  ra_clip_exit_edge, r0 >> r5
/* [0x00001190] */ 0xfffffe00, 0xf0f80227, // brr  ra_temp3, r:clip_out
/* [0x00001198] */ 0x956f0d89, 0xd00440c9, // mov.ifz  ra_clip_q_x, ra_temp2     ; mov  rb_clip_prev_x, r1 >> r5
/* [0x000011a0] */ 0x959f06d2, 0xd004414a, // mov.ifz  ra_clip_q_y, r3           ; mov  rb_clip_prev_y, r2 >> r5
/* [0x000011a8] */ 0xed9d1f40, 0xd0025949, // sub  r5rep, -15, r5                ; mov  ra_clip_zone_delta, 0
/* [0x000011b0] */ 0x95049ff6, 0x10024822, // mov  r0, rb_clip_prev_x            ; mov  r2, ra_user_to_surface_clip
/* [0x000011b8] */ 0xf59cafc0, 0x10024863, // mov  r1, rb_clip_prev_y            ; mov  r3, 0
/* [0x000011c0] */ 0x979f86d2, 0xd00242a5, // not  ra_temp, r3                   ; mov  r5rep, r2 << 8
/* [0x000011c8] */ 0x829faa12, 0xd00269e5, // fsub.setf  -, r5, r0               ; mov  r5rep, r2 << 6
/* [0x000011d0] */ 0x829c417f, 0xd00329e3, // fsub.setf  -, r0, r5               ; mov.ifn  r3, CF_RIGHT
/* [0x000011d8] */ 0x8d2b9792, 0xd00848e5, // sub.ifn  r3, r3, ra_temp           ; mov  r5rep, r2 << 7
/* [0x000011e0] */ 0x829f7352, 0xd00269e5, // fsub.setf  -, r1, r5               ; mov  r5rep, r2 << 9
/* [0x000011e8] */ 0xc29c2a5f, 0xd00329e3, // fsub.setf  -, r5, r1               ; v8adds.ifn  r3, r3, CF_BOTTOM
/* [0x000011f0] */ 0x957887f6, 0xd00848e5, // or.ifn  r3, r3, CF_TOP             ; mov  r5rep, ra_scalar << rsi_clip_q_n
/* [0x000011f8] */ 0x959ffb40, 0xd00279c3, // mov.setf  -, r5                    ; mov  ra_clip_q_x, r0 << 1
/* [0x00001200] */ 0x00000030, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x00001208] */ 0xcd9c97db, 0xd00279ca, // sub.setf  -, r3, CF_LEFT | CF_TOP  ; v8adds  ra_temp, r3, r3
/* [0x00001210] */ 0x0000003e, 0xe0020827, // mov  r0, 62
/* [0x00001218] */ 0xf89c76c7, 0xd00688a2, // clz.ifnz  r2, r3                   ; v8subs.ifz  r2, r0, 7
/* [0x00001220] */ 0xd12a7c92, 0x1006e9e2, // shl.ifnz.setf  -, ra_temp, r2      ; v8adds.ifnz  r2, r2, r2
/* [0x00001228] */ 0x8d9c11db, 0xd006580b, // sub.ifnz  r0, r0, 1                ; mov  ra_clip_prev_cf, r3
/* [0x00001230] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00001238] */ 0xed98fdc2, 0xd00269cb, // sub.setf  -, elem_num, rsi_clip_lr ; v8subs  rb_clip_prev_zone, r0, r2
/* [0x00001240] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:clip_outside
/* [0x00001248] */ 0x009e7000, 0x100009e7, // nop                                ; nop
// :1
/* [0x00001250] */ 0xd12a7c92, 0x1006e9e2, // shl.ifnz.setf  -, ra_temp, r2      ; v8adds.ifnz  r2, r2, r2
/* [0x00001258] */ 0x8d9c11db, 0xd006580b, // sub.ifnz  r0, r0, 1                ; mov  ra_clip_prev_cf, r3
/* [0x00001260] */ 0x8d9ff089, 0xd00252c5, // sub  rb_clip_prev_zone, r0, r2     ; mov  ra_clip_q_y, r1 << 1
// :clip_outside
/* [0x00001268] */ 0x95044dbf, 0xd002580a, // mov  r0, ra_user_to_surface_clip  ; mov  ra_temp, CF_RIGHT
/* [0x00001270] */ 0x00000000, 0xe0020867, // mov  r1, 0                        ; nop
/* [0x00001278] */ 0x809fa000, 0xd00049e5, // nop                               ; mov  r5rep, r0 << 6
/* [0x00001280] */ 0x820f8d40, 0xd00269e5, // fsub.setf  -, ra_clip_q_x, r5     ; mov  r5rep, r0 << 8
/* [0x00001288] */ 0x820c1bbf, 0xd00329e1, // fsub.setf  -, r5, ra_clip_q_x     ; mov.ifn  r1, CF_LEFT
/* [0x00001290] */ 0x952b9d80, 0xd0084865, // mov.ifn  r1, ra_temp              ; mov  r5rep, r0 << 7
/* [0x00001298] */ 0x82177d40, 0xd00269e5, // fsub.setf  -, ra_clip_q_y, r5     ; mov  r5rep, r0 << 9
/* [0x000012a0] */ 0xc2142b8f, 0xd00329e1, // fsub.setf  -, r5, ra_clip_q_y     ; v8adds.ifn  r1, r1, CF_BOTTOM
/* [0x000012a8] */ 0x159c83c0, 0xd0080867, // or.ifn  r1, r1, CF_TOP            ; nop
/* [0x000012b0] */ 0xcd9c93c9, 0xd00269e0, // sub.setf  -, r1, CF_LEFT | CF_TOP ; v8adds  r0, r1, r1
/* [0x000012b8] */ 0x0000003e, 0xe00208a7, // mov  r2, 62
/* [0x000012c0] */ 0xf89c7257, 0xd00688e3, // clz.ifnz  r3, r1                  ; v8subs.ifz  r3, r2, 7
/* [0x000012c8] */ 0xd19e70db, 0x1006e9e3, // shl.ifnz.setf  -, r0, r3          ; v8adds.ifnz  r3, r3, r3
/* [0x000012d0] */ 0x8d0c15f6, 0xd00648a0, // sub.ifnz  r2, r2, 1               ; mov  r0, ra_clip_q_x
/* [0x000012d8] */ 0xf59a7d93, 0x100269e2, // mov.setf  -, elem_num             ; v8subs  r2, r2, r3
/* [0x000012e0] */ 0x95171d80, 0xd002c8c9, // mov  r3, ra_clip_q_y              ; mov.ifnz  rb_clip_prev_x, r0 >> 1
/* [0x000012e8] */ 0x95071d89, 0xd002d80b, // mov  r0, ra_user_to_surface_clip  ; mov.ifnz  ra_clip_prev_cf, r1 >> 1
/* [0x000012f0] */ 0x979f1252, 0xd002c84b, // not  r1, r1                       ; mov.ifnz  rb_clip_prev_zone, r2 >> 1
/* [0x000012f8] */ 0x952f1d9b, 0xd002c8ca, // mov  r3, ra_clip_prev_cf          ; mov.ifnz  rb_clip_prev_y, r3 >> 1
/* [0x00001300] */ 0x942ba780, 0xd00269e5, // and.setf  -, r3, ra_temp          ; mov  r5rep, r0 << 6
/* [0x00001308] */ 0x959e74ad, 0x100242a2, // mov  ra_temp, r2                  ; mov  r2, r5
/* [0x00001310] */ 0x942f8380, 0xd00248e5, // and  r3, r1, ra_clip_prev_cf                  ; mov  r5rep, r0 << 8
/* [0x00001318] */ 0x949c57ed, 0xd002e6e2, // and.setf  ra_temp2, r3, CF_LEFT | CF_RIGHT    ; mov.ifnz  r2, r5
/* [0x00001320] */ 0xe20c9dc0, 0x1006dd0b, // fsub.ifnz  recip, ra_clip_q_x, rb_clip_prev_x ; mov.ifnz  ra_clip_prev_cf, 0
/* [0x00001328] */ 0x809f9000, 0xd00049e5, // nop                                           ; mov  r5rep, r0 << 7
/* [0x00001330] */ 0x829c95d2, 0x1006c889, // fsub.ifnz  r2, r2, rb_clip_prev_x             ; mov.ifnz  rb_clip_prev_x, r2
/* [0x00001338] */ 0x2214add4, 0x1006c8e2, // fsub.ifnz  r3, ra_clip_q_y, rb_clip_prev_y    ; fmul.ifnz  r2, r2, r4
/* [0x00001340] */ 0x039e05c0, 0xd00608a7, // fmin.ifnz  r2, r2, f(1.0)                     ; nop
/* [0x00001348] */ 0x2e6c1dd3, 0xd006c6e2, // shr.ifnz  ra_temp2, ra_temp2, 1               ; fmul.ifnz  r2, r2, r3
/* [0x00001350] */ 0x819caebf, 0x100688a2, // fadd.ifnz  r2, rb_clip_prev_y, r2             ; mov.ifz  r2, rb_clip_prev_y
/* [0x00001358] */ 0x829f7540, 0xd00269e5, // fsub.setf  -, r2, r5                          ; mov  r5rep, r0 << 9
/* [0x00001360] */ 0x952c2dd2, 0xd00842ca, // or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_BOTTOM ; mov  rb_clip_prev_y, r2
/* [0x00001368] */ 0x829caabf, 0xd00269e3, // fsub.setf  -, r5, r2                          ; mov  r3, CF_BOTTOM | CF_TOP
/* [0x00001370] */ 0x952c8ded, 0xd00842e2, // or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_TOP ; mov  r2, r5
/* [0x00001378] */ 0x949f92c0, 0xd00248e5, // and  r3, r1, r3                               ; mov  r5rep, r0 << 7
/* [0x00001380] */ 0x942e77ad, 0x10036222, // and.setf  ra_temp3, r3, ra_clip_prev_cf       ; mov.ifnn  r2, r5
/* [0x00001388] */ 0xe214adc0, 0x1006dd0b, // fsub.ifnz  recip, ra_clip_q_y, rb_clip_prev_y ; mov.ifnz  ra_clip_prev_cf, 0
/* [0x00001390] */ 0x809fa000, 0xd00049e5, // nop                                           ; mov  r5rep, r0 << 6
/* [0x00001398] */ 0x829ca5d2, 0x1006c88a, // fsub.ifnz  r2, r2, rb_clip_prev_y             ; mov.ifnz  rb_clip_prev_y, r2
/* [0x000013a0] */ 0x220c9dd4, 0x1006c8e2, // fsub.ifnz  r3, ra_clip_q_x, rb_clip_prev_x    ; fmul.ifnz  r2, r2, r4
/* [0x000013a8] */ 0x039e05c0, 0xd00608a7, // fmin.ifnz  r2, r2, f(1.0)                     ; nop
/* [0x000013b0] */ 0x2e201dd3, 0xd006c6e2, // shr.ifnz  ra_temp2, ra_temp3, 1               ; fmul.ifnz  r2, r2, r3
/* [0x000013b8] */ 0x819c9ebf, 0x100688a2, // fadd.ifnz  r2, rb_clip_prev_x, r2             ; mov.ifz  r2, rb_clip_prev_x
/* [0x000013c0] */ 0x829f8540, 0xd00269e5, // fsub.setf  -, r2, r5                          ; mov  r5rep, r0 << 8
/* [0x000013c8] */ 0x952c1dd2, 0xd00842c9, // or.ifn  ra_clip_prev_cf, ra_clip_prev_cf, CF_LEFT ; mov  rb_clip_prev_x, r2
/* [0x000013d0] */ 0xe2781ab7, 0xd00269e5, // fsub.setf  -, r5, r2                          ; v8subs  r5rep, ra_scalar << rsi_clip_q_n, 1
/* [0x000013d8] */ 0x952c4df6, 0xd0094820, // or.ifn  r0, ra_clip_prev_cf, CF_RIGHT         ; mov.ifnn  r0, ra_clip_prev_cf
/* [0x000013e0] */ 0xff99fd79, 0xd00269e1, // v8subs.setf  -, elem_num, r5                  ; v8subs  r1, -1, r1
/* [0x000013e8] */ 0x946c3077, 0xd00466e2, // and.ifz.setf  ra_temp2, r0, r1                      ; v8min  r2, ra_temp2, 3
/* [0x000013f0] */ 0xcd28bdd2, 0x10024822, // sub  r0, ra_temp, rb_clip_prev_zone                 ; v8adds  r2, r2, r2
/* [0x000013f8] */ 0x949c71d2, 0xd0025808, // and  r0, r0, 7                                      ; mov  ra_temp3, r2
/* [0x00001400] */ 0x00000048, 0xf02809e7, // brr.anyz  -, r:clip_outside_to_inside
/* [0x00001408] */ 0x8d1441f6, 0xd00269e3, // sub.setf  -, r0, 4                                  ; mov  r3, ra_clip_q_y
/* [0x00001410] */ 0xcd298c87, 0xd00348a0, // sub  r2, ra_temp, r2                                ; v8adds.ifnn  r0, r0, -8
/* [0x00001418] */ 0x940c45f6, 0xd00469e2, // and.ifz.setf  -, r2, 4                              ; mov  r2, ra_clip_q_x
/* [0x00001420] */ 0x8d984d7f, 0xd002b960, // sub.setf  r5rep, elem_num, r5                       ; mov.ifz  r0, 4
/* [0x00001428] */ 0x0c267c00, 0x10080267, // add.ifn  ra_clip_zone_delta, ra_clip_zone_delta, r0 ; nop
/* [0x00001430] */ 0x952b0d92, 0xd0024889, // mov  r2, ra_temp                                    ; mov  rb_clip_prev_x, r2 >> r5
/* [0x00001438] */ 0x8c270c1b, 0xd004424a, // add.ifz  ra_clip_zone_delta, ra_clip_zone_delta, r0 ; mov  rb_clip_prev_y, r3 >> r5
/* [0x00001440] */ 0x959b0d89, 0xd00279cb, // mov.setf  -, elem_num                               ; mov  ra_clip_prev_cf, r1 >> r5
/* [0x00001448] */ 0x00000000, 0xf0f7c9e7, // bra  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00001450] */ 0xed98fdc0, 0xd002b9de, // sub.setf  -, elem_num, rsi_clip_lr                  ; mov.ifz  ra_scalar, 0
/* [0x00001458] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:clip_outside
/* [0x00001460] */ 0x809f0012, 0xd00049cb, // nop                                                 ; mov  rb_clip_prev_zone, r2 >> r5
// :clip_outside_to_inside
/* [0x00001468] */ 0x956c4dbf, 0xd002a9e0, // mov.setf  -, ra_temp2 ; mov.ifz  r0, 4
/* [0x00001470] */ 0x9598fdbf, 0xd004c861, // mov.ifz  r1, elem_num ; mov.ifnz  r1, 15
/* [0x00001478] */ 0x959b8d89, 0xd00269e2, // mov.setf  -, elem_num ; mov  r2 << 8, r1        @mul_used(8, 9, 10, 11, 12, 13, 14, 15)
/* [0x00001480] */ 0x929f428a, 0xd0024862, // min  r1, r1, r2       ; v8min  r2 << 4, r1, r2  @mul_used(12, 13, 14, 15)
/* [0x00001488] */ 0x929f228a, 0xd0024862, // min  r1, r1, r2       ; v8min  r2 << 2, r1, r2  @mul_used(14, 15)
/* [0x00001490] */ 0x929f128a, 0xd0024862, // min  r1, r1, r2       ; v8min  r2 << 1, r1, r2  @mul_used(15)
/* [0x00001498] */ 0x0d790dc0, 0xd00407a7, // sub.ifz  ra_scalar, ra_scalar, -16 ; nop
/* [0x000014a0] */ 0x809f100a, 0xd00049e5, // nop                   ; v8min  r5rep >> 15, r1, r2
/* [0x000014a8] */ 0xed990d7d, 0xd00269e5, // sub.setf  -, elem_num, r5           ; v8subs  r5rep, -16, r5
/* [0x000014b0] */ 0x8c267c36, 0x10094820, // add.ifn  r0, ra_clip_zone_delta, r0 ; mov.ifnn  r0, ra_clip_zone_delta
/* [0x000014b8] */ 0x950f8d80, 0xd00248a1, // mov  r2, ra_clip_q_x                ; mov  r1 << 8, r0            @mul_used(8, 9, 10, 11, 12, 13, 14, 15)
/* [0x000014c0] */ 0x8c9f005b, 0xd002580b, // add  r0, r0, r1                     ; mov  ra_clip_prev_cf, r3 >> r5
/* [0x000014c8] */ 0x95234d80, 0xd00248e1, // mov  r3, ra_temp3                   ; mov  r1 << 4, r0            @mul_used(12, 13, 14, 15)
/* [0x000014d0] */ 0x8c9f0052, 0xd002480b, // add  r0, r0, r1                     ; mov  rb_clip_prev_zone, r2 >> r5
/* [0x000014d8] */ 0x8c1f2d80, 0xd00248a1, // add  r2, ra_clip_exit_edge, ra_clip_exit_edge ; mov  r1 << 2, r0  @mul_used(14, 15)
/* [0x000014e0] */ 0x8c9f005b, 0xd0024823, // add  r0, r0, r1                     ; mov  r3, r3 >> r5
/* [0x000014e8] */ 0x8d9f1680, 0xd00248a1, // sub  r2, r3, r2                     ; mov  r1 << 1, r0            @mul_used(15)
/* [0x000014f0] */ 0x8c9c907f, 0x10024821, // add  r0, r0, r1                     ; mov  r1, rb_clip_prev_x
/* [0x000014f8] */ 0x95998dbf, 0xd00279ca, // mov.setf  -, elem_num               ; mov  ra_temp, ~7
/* [0x00001500] */ 0x8c7b1d40, 0xd00447a0, // add.ifz  ra_scalar, ra_scalar, r5   ; mov  r0, r0 << 15
/* [0x00001508] */ 0x8d9ca0bf, 0x10024822, // sub  r0, r0, r2                     ; mov  r2, rb_clip_prev_y
/* [0x00001510] */ 0x942b0189, 0xd0024809, // and  r0, r0, ra_temp                ; mov  rb_clip_prev_x, r1 >> r5
/* [0x00001518] */ 0x8c9f0612, 0xd002480a, // add  r0, r3, r0                     ; mov  rb_clip_prev_y, r2 >> r5
/* [0x00001520] */ 0x8f0411f6, 0xd0024821, // asr  r0, r0, 1                      ; mov  r1, ra_user_to_surface_clip
/* [0x00001528] */ 0xcd984dff, 0xd00269e2, // sub.setf  -, elem_num, 4            ; v8adds  r2, 4, 4
/* [0x00001530] */ 0x969fa009, 0xd00248e1, // mov  r3, 0                          ; mov  r1, r1 << 6
/* [0x00001538] */ 0x8d9b4c89, 0xd00369e1, // sub.setf  -, elem_num, r2           ; mov.ifnn  r1, r1 >> 4  @mul_unused(0, 1, 2, 3)
/* [0x00001540] */ 0x979f86c9, 0xd00348e1, // not  r3, r3                         ; mov.ifnn  r1, r1 >> 8  @mul_unused(0, 1, 2, 3, 4, 5, 6, 7)
/* [0x00001548] */ 0x94981dc9, 0xd00269e2, // and.setf  -, elem_num, 1            ; mov  r2, r1
/* [0x00001550] */ 0x8c1ffcc9, 0xd002d962, // add  r5rep, ra_clip_exit_edge, r3   ; mov.ifnz  r2, r1 << 1
/* [0x00001558] */ 0x8d9ff149, 0xd002a1e1, // sub.setf  ra_clip_exit_edge, r0, r5 ; mov.ifz  r1, r1 << 1
/* [0x00001560] */ 0xed9c0f40, 0xd00a5960, // sub.ifnn  r5rep, 0, r5              ; mov  r0, 0
/* [0x00001568] */ 0x8d1c21bf, 0xd00841e0, // sub.ifn  ra_clip_exit_edge, r0, ra_clip_exit_edge ; mov  r0, 2
/* [0x00001570] */ 0x8c9f0a12, 0xd0085943, // add.ifn  r5rep, r5, r0              ; mov  ra_clip_q_x, r2 >> r5
/* [0x00001578] */ 0x009e7000, 0x100009e7, // nop                                 ; nop
/* [0x00001580] */ 0x809f0009, 0xd00059c5, // nop                                 ; mov  ra_clip_q_y, r1 >> r5
// :1
/* [0x00001588] */ 0x8d981dff, 0xd00269e0, // sub.setf  -, elem_num, 1           ; mov  r0, 1
/* [0x00001590] */ 0x0c1d0dc0, 0xd00821e7, // add.ifn.setf  ra_clip_exit_edge, ra_clip_exit_edge, -16 ; nop
/* [0x00001598] */ 0x00000020, 0xf06809e7, // brr.anyn  -, r:1f
/* [0x000015a0] */ 0x141cfdc0, 0xd0021967, // and  r5rep, ra_clip_exit_edge, 15  ; nop
/* [0x000015a8] */ 0x809c903f, 0x100049e1, // nop                                ; mov  r1, rb_clip_prev_x
/* [0x000015b0] */ 0x8d98ad7f, 0x100269e2, // sub.setf  -, elem_num, r5          ; mov  r2, rb_clip_prev_y
/* [0x000015b8] */ 0xfffff9d8, 0xf0f809e7, // brr  -, r:clip_out
/* [0x000015c0] */ 0xd5988dbf, 0xd00269e5, // mov.setf  -, elem_num              ; v8adds  r5rep, 8, 8
/* [0x000015c8] */ 0x0d790dc0, 0xd00407a7, // sub.ifz  ra_scalar, ra_scalar, -16 ; nop
/* [0x000015d0] */ 0xdeadbeef, 0xe0020227, // mov  ra_temp3, a:1b
// :1
/* [0x000015d8] */ 0xfffff9b8, 0xf0f80227, // brr  ra_temp3, r:clip_out
/* [0x000015e0] */ 0x8c9f0a09, 0xd0029803, // add  r0, r5, r0                    ; mov.ifz  ra_clip_q_x, r1 >> r5
/* [0x000015e8] */ 0x959b0d92, 0xd002b9c5, // mov.setf  -, elem_num              ; mov.ifz  ra_clip_q_y, r2 >> r5
/* [0x000015f0] */ 0x8c7a7c00, 0x100447a5, // add.ifz  ra_scalar, ra_scalar, r0  ; mov  r5rep, r0
/* [0x000015f8] */ 0xfffff920, 0xf0f809e7, // brr  -, r:clip_inside
/* [0x00001600] */ 0x159cbfc0, 0x100200e7, // mov  ra_clip_q_x, rb_clip_prev_zone ; nop
/* [0x00001608] */ 0x152e7d80, 0x10020167, // mov  ra_clip_q_y, ra_clip_prev_cf   ; nop
/* [0x00001610] */ 0x009e7000, 0x100009e7, // nop                                 ; nop
// :end_subpath
/* [0x00001618] */ 0x0d980dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_q_n
/* [0x00001620] */ 0x957a7db6, 0x100469e0, // mov.ifz.setf  -, ra_scalar ; mov  r0, ra_scalar
/* [0x00001628] */ 0x00000000, 0xf017c9e7, // bra.allnz  -, (ra_scalar << rsi_clip_lr) >> 15
/* [0x00001630] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr
/* [0x00001638] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:1f
/* [0x00001640] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001648] */ 0x159e7000, 0x100407a7, // mov.ifz  ra_scalar, r0
// :1
/* [0x00001650] */ 0xdeadbeef, 0xe0020827, // mov  r0, a:clip_outside
/* [0x00001658] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr ; nop
/* [0x00001660] */ 0x8d781c3f, 0xd00469e3, // sub.ifz.setf  -, ra_scalar, r0     ; mov  r3, 1
/* [0x00001668] */ 0x00000030, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x00001670] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:1f
/* [0x00001678] */ 0x95049dbf, 0x10024821, // mov  r0, ra_user_to_surface_clip   ; mov  r1, rb_clip_prev_x
/* [0x00001680] */ 0x9598adbf, 0x100269e2, // mov.setf  -, elem_num              ; mov  r2, rb_clip_prev_y
/* [0x00001688] */ 0x959fa6c0, 0xd00447a3, // mov.ifz  ra_scalar, r3             ; mov  r3, r0 << 6
/* [0x00001690] */ 0x849f82c0, 0xd0024863, // fmax  r1, r1, r3                   ; mov  r3, r0 << 8
/* [0x00001698] */ 0xfffffbb0, 0xf0f809e7, // brr  -, r:clip_outside
/* [0x000016a0] */ 0x839f92c0, 0xd00240e3, // fmin  ra_clip_q_x, r1, r3          ; mov  r3, r0 << 7
/* [0x000016a8] */ 0x849f74c0, 0xd00248a3, // fmax  r2, r2, r3                   ; mov  r3, r0 << 9
/* [0x000016b0] */ 0x039e74c0, 0x10020167, // fmin  ra_clip_q_y, r2, r3          ; nop
// :1
/* [0x000016b8] */ 0x0d98fdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_clip_lr
/* [0x000016c0] */ 0xdeadbeef, 0xe00407a7, // mov.ifz  ra_scalar, a:clip_subpath_first
/* [0x000016c8] */ 0x15367d80, 0x10020827, // mov  r0, ra_subpath_sum_x       ; nop
/* [0x000016d0] */ 0x153e7d80, 0x10020867, // mov  r1, ra_subpath_sum_y       ; nop
/* [0x000016d8] */ 0x887b8d80, 0xd00248a0, // itof  r2, ra_scalar             ; mov  r0, r0 << 8
/* [0x000016e0] */ 0x81378189, 0xd0024821, // fadd  r0, r0, ra_subpath_sum_x  ; mov  r1, r1 << 8
/* [0x000016e8] */ 0x813f7392, 0xd0024874, // fadd  r1, r1, ra_subpath_sum_y  ; mov  recip, r2 << rsi_subpath_n
/* [0x000016f0] */ 0x969fc000, 0xd00248e2, // mov  r3, f(0.0)                 ; mov  r2, r0 << 4
/* [0x000016f8] */ 0x819fc089, 0xd0024822, // fadd  r0, r0, r2                ; mov  r2, r1 << 4
/* [0x00001700] */ 0x219e22a7, 0xd0024865, // fadd  r1, r1, r2                ; fmul  r5rep, r4, f(4.0)
/* [0x00001708] */ 0x829fe740, 0xd00248e2, // fsub  r3, r3, r5                ; mov  r2, r0 << 2
/* [0x00001710] */ 0x819fe089, 0xd0024822, // fadd  r0, r0, r2                ; mov  r2, r1 << 2
/* [0x00001718] */ 0xe19e7280, 0x1002584d, // fadd  r1, r1, r2                ; mov  ra_subpath_sum_x, f(0.0)
/* [0x00001720] */ 0x959bfd80, 0xd00269e2, // mov.setf  -, elem_num           ; mov  r2, r0 << 1
/* [0x00001728] */ 0x819ff089, 0xd0024822, // fadd  r0, r0, r2                ; mov  r2, r1 << 1
/* [0x00001730] */ 0x219e7283, 0x10024860, // fadd  r1, r1, r2                ; fmul  r0, r0, r3
/* [0x00001738] */ 0x279e700b, 0x10024821, // ftoi  r0, r0                    ; fmul  r1, r1, r3
/* [0x00001740] */ 0x9244ec07, 0x10044462, // min.ifz  ra_min_x, ra_min_x, r0 ; v8min  r2, r0, rb_0xffff
/* [0x00001748] */ 0x134e7c00, 0x100404e7, // max.ifz  ra_max_x, ra_max_x, r0 ; nop
/* [0x00001750] */ 0xe79e7240, 0x1002584f, // ftoi  r1, r1                    ; mov  ra_subpath_sum_y, f(0.0)
/* [0x00001758] */ 0x5254fc4f, 0x10044563, // min.ifz  ra_min_y, ra_min_y, r1 ; mul24  r3, r1, rb_0x10000
/* [0x00001760] */ 0xb35e7c53, 0x100445e5, // max.ifz  ra_max_y, ra_max_y, r1 ; v8max  r5rep, r2, r3  @geomd_i(geomd_i_fill_post_center) @geomd_tris_set_center_m(geomd_i_fill_pre_center) @geomd_i(geomd_i_fill_pre_center) @geomd_tris_clear @geomd_verts_a(3)
/* [0x00001768] */ 0x20002000, 0xe20229e7, // mov.setf  -, neg_elems(elems(rsi_alloc_size))
/* [0x00001770] */ 0x00000058, 0xe0020827, // mov  r0, 88
/* [0x00001778] */ 0x0d7a7c00, 0x100829e7, // sub.ifn.setf  -, ra_scalar, r0 ; nop
/* [0x00001780] */ 0xdeadbeef, 0xe00202a7, // mov  ra_temp, a:1f
/* [0x00001788] */ 0x000001b0, 0xf06809e7, // brr.anyn  -, r:alloc
/* [0x00001790] */ 0x062a0101, 0xe0021327, // mov  rb_list_head, 0x062a0101
/* [0x00001798] */ 0x95981dbf, 0xd00269e1, // mov.setf  -, elem_num ; mov  r1, 1
/* [0x000017a0] */ 0x8d98fded, 0xd002e9cc, // sub.setf  -, elem_num, 15 ; mov.ifnz  rb_list_head, r5
/* [0x000017a8] */ 0xbfff0000, 0xe0041327, // mov.ifz  rb_list_head, 0xbfff0000
/* [0x000017b0] */ 0xfff30080, 0xe0020827, // mov  r0, (1 << 7) - (13 << 16)
/* [0x000017b8] */ 0x8c78cc7f, 0x10024862, // add  r1, ra_scalar, r1  ; mov  r2, rb_list_head
/* [0x000017c0] */ 0x0d78cdc0, 0xd00208e7, // sub  r3, ra_scalar, 12  ; nop
/* [0x000017c8] */ 0x8c7b6c09, 0xd0024831, // add  r0, ra_scalar, r0  ; mov  vw_setup, r1 << rsi_out_vpm_setup
/* [0x000017d0] */ 0x00003000, 0xe20229e7, // mov.setf  -, elems(rsi_alloc_p, rsi_alloc_size)
/* [0x000017d8] */ 0x959f26ff, 0x100607a7, // mov.ifnz  ra_scalar, r3 ; mov  -, vw_wait
/* [0x000017e0] */ 0x809f1012, 0xd00049f0, // nop                     ; mov  vpm, r2 >> 1
/* [0x000017e8] */ 0x809f5000, 0xd00049f1, // nop                     ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x000017f0] */ 0x809f401b, 0xd00049f2, // nop                     ; mov  vw_addr, r3 << rsi_alloc_p
// :1
/* [0x000017f8] */ 0x957bfdb6, 0xd00269e5, // mov.setf  -, ra_scalar ; mov  r5rep, ra_scalar << rsi_chunk_lr
/* [0x00001800] */ 0x8d985ded, 0xd00479ca, // sub.ifz.setf  -, elem_num, rsi_chunks_m_size ; mov  ra_temp, r5
/* [0x00001808] */ 0x009e7000, 0x100009e7, // nop                    ; nop
/* [0x00001810] */ 0x00000000, 0xf01549e7, // bra.allnz  -, ra_temp
/* [0x00001818] */ 0x0d989dc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_subpath_n ; nop
/* [0x00001820] */ 0xf4982dc0, 0xd002b9de, // and.setf  -, elem_num, 2 ; mov.ifz  ra_scalar, 0
/* [0x00001828] */ 0x162a0101, 0xe0021327, // mov  rb_list_head, 0x162a0101
/* [0x00001830] */ 0x15467d80, 0x10020827, // mov  r0, ra_min_x         ; nop
/* [0x00001838] */ 0x15567d80, 0x10020867, // mov  r1, ra_min_y         ; nop
/* [0x00001840] */ 0x954fed80, 0xd00248a0, // mov  r2, ra_max_x         ; mov  r0, r0 << 2
/* [0x00001848] */ 0x955fed89, 0xd00248e1, // mov  r3, ra_max_y         ; mov  r1, r1 << 2
/* [0x00001850] */ 0x9247e192, 0xd0024822, // min  r0, r0, ra_min_x     ; mov  r2, r2 << 2
/* [0x00001858] */ 0x9257e39b, 0xd0044823, // min.ifz  r0, r1, ra_min_y ; mov  r3, r3 << 2
/* [0x00001860] */ 0x134e7580, 0x10020867, // max  r1, r2, ra_max_x     ; nop
/* [0x00001868] */ 0x135e7780, 0x10040867, // max.ifz  r1, r3, ra_max_y ; nop
/* [0x00001870] */ 0x809f8000, 0xd00049e2, // nop                       ; mov  r2, r0 << 8
/* [0x00001878] */ 0x929f8089, 0xd0024822, // min  r0, r0, r2           ; mov  r2, r1 << 8
/* [0x00001880] */ 0x139e7280, 0x10020867, // max  r1, r1, r2           ; nop
/* [0x00001888] */ 0x809fc000, 0xd00049e2, // nop                       ; mov  r2, r0 << 4
/* [0x00001890] */ 0x929fc089, 0xd0024822, // min  r0, r0, r2           ; mov  r2, r1 << 4
/* [0x00001898] */ 0x939c12bf, 0xd0024862, // max  r1, r1, r2           ; mov  r2, 1
/* [0x000018a0] */ 0x949bfc80, 0xd00269e2, // and.setf  -, elem_num, r2 ; mov  r2, r0 << 1
/* [0x000018a8] */ 0x929f1089, 0xd0024822, // min  r0, r0, r2           ; mov  r2, r1 >> 1
/* [0x000018b0] */ 0x139e7280, 0x10060827, // max.ifnz  r0, r1, r2      ; nop
/* [0x000018b8] */ 0x9578fdbf, 0xd00248a3, // mov  r2, ra_scalar        ; mov  r3, 15
/* [0x000018c0] */ 0x00000038, 0xe0020867, // mov  r1, 56
/* [0x000018c8] */ 0x8d9b4cd2, 0xd00269e5, // sub.setf  -, elem_num, r3 ; mov  r5rep, r2 << rsi_alloc_p
/* [0x000018d0] */ 0x8c99fdff, 0xd002a9e0, // add.setf  -, elem_num, -1 ; mov.ifz  r0, -1
/* [0x000018d8] */ 0xcd9c2a57, 0xd0044822, // sub.ifz  r0, r5, r1       ; v8adds  r2, r2, 2
/* [0x000018e0] */ 0x0c9c4bc0, 0xd0080827, // add.ifn  r0, r5, 4        ; nop
/* [0x000018e8] */ 0x00000100, 0xe00208e7, // mov  r3, 2 << 7
/* [0x000018f0] */ 0x8c7b6cd2, 0xd00248b1, // add  r2, ra_scalar, r3    ; mov  vw_setup, r2 << rsi_out_vpm_setup
/* [0x000018f8] */ 0x809e7000, 0x100049f0, // nop                       ; mov  vpm, r0
/* [0x00001900] */ 0x957b5d92, 0xd0024831, // mov  r0, ra_scalar        ; mov  vw_setup, r2 << rsi_out_vdw_setup_0
/* [0x00001908] */ 0x8d7a0c7f, 0x10024872, // sub  r1, ra_scalar, r1    ; mov  vw_addr, unif
/* [0x00001910] */ 0x809f8000, 0xd00049f1, // nop ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
/* [0x00001918] */ 0x809f4009, 0xd00049f0, // nop ; mov  vpm, r1 << rsi_alloc_p
/* [0x00001920] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00001928] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00001930] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00001938] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x00001940] */ 0x009e7000, 0x300009e7, // thrend
/* [0x00001948] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001950] */ 0x009e7000, 0x100009e7, // nop
// :alloc
/* [0x00001958] */ 0xcc781dff, 0xd0024822, // add  r0, ra_scalar, 1     ; v8adds  r2, 1, 1
/* [0x00001960] */ 0xfff30080, 0xe0020867, // mov  r1, (1 << 7) - (13 << 16)
/* [0x00001968] */ 0x8d98ccbf, 0xd00269e2, // sub.setf  -, elem_num, r2 ; mov  r2, 12
/* [0x00001970] */ 0x8c7b6c40, 0xd0024831, // add  r0, ra_scalar, r1    ; mov  vw_setup, r0 << rsi_out_vpm_setup
/* [0x00001978] */ 0x8d7b2cbf, 0x10020867, // sub  r1, ra_scalar, r2    ; mov  -, vw_wait
/* [0x00001980] */ 0x809cc03f, 0x100049f0, // nop                       ; mov  vpm, rb_list_head
/* [0x00001988] */ 0x809f5000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00001990] */ 0x809f4009, 0xd00049f2, // nop                       ; mov  vw_addr, r1 << rsi_alloc_p
/* [0x00001998] */ 0x809f6009, 0xd00089cd, // nop                       ; mov.ifz  rb_list_tail, (r1 << rsi_alloc_p) >> 2
// :alloc_first
/* [0x000019a0] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x000019a8] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x000019b0] */ 0x15ce7d80, 0x100009e7, // mov  -, mutex             ; nop
/* [0x000019b8] */ 0x00100a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(15))
/* [0x000019c0] */ 0x00000ffe, 0xe00208a7, // mov  r2, 0xffe
/* [0x000019c8] */ 0x807be036, 0xd00049e5, // nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
/* [0x000019d0] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x000019d8] */ 0x8dc02df6, 0xd0024821, // sub  r0, vpm, 2           ; mov  r1, vpm
/* [0x000019e0] */ 0x949e7289, 0x1004e860, // and.ifz.setf  r1, r1, r2  ; mov.ifnz  r0, r1
/* [0x000019e8] */ 0x00000120, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x000019f0] */ 0x00100a0f, 0xe0021c67, // mov  vw_setup, vpm_setup(1, 0, h32(15))
/* [0x000019f8] */ 0x000003fe, 0xe00208e7, // mov  r3, VG_TESS_SUBBLOCK_SIZE >> 1
/* [0x00001a00] */ 0x559e700b, 0x10024c21, // mov  vpm, r0              ; mul24  r1, r1, r3
/* [0x00001a08] */ 0x159e7b40, 0x10020c27, // mov  vpm, r5              ; nop
/* [0x00001a10] */ 0x80814780, 0xe0021c67, // mov  vw_setup, vdw_setup_0(1, 1, dma_h32(15, 0))
/* [0x00001a18] */ 0xdeadbeef, 0xe0021ca7, // mov  vw_addr, a:vg_tess_shader_if
/* [0x00001a20] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait           ; nop
/* [0x00001a28] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1         ; nop
/* [0x00001a30] */ 0x009e7000, 0x000009e7, // nop                       ; nop ; bkpt
/* [0x00001a38] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x00001a40] */ 0x801100f0, 0xe0020c67, // mov  vr_setup, vdr_setup_0(1, 1, dma_h32(15, 0), 0, 8)
/* [0x00001a48] */ 0xdeadbeef, 0xe0020ca7, // mov  vr_addr, a:vg_tess_shader_if + 4
/* [0x00001a50] */ 0x15ca7d80, 0x100009e7, // mov  -, vr_wait           ; nop
/* [0x00001a58] */ 0x00100a0f, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(15))
/* [0x00001a60] */ 0x0dc02dc0, 0xd0021967, // sub  r5rep, vpm, 2        ; nop
/* [0x00001a68] */ 0x8c9c2bed, 0xd0028860, // add  r1, r5, 2            ; mov.ifz  r0, r5
/* [0x00001a70] */ 0x949e7280, 0x10026870, // and.setf  r1, r1, r2      ; mov  vpm, r0
/* [0x00001a78] */ 0x00000090, 0xf01809e7, // brr.allnz  -, r:1f
/* [0x00001a80] */ 0x409e700b, 0x100049e1, // nop                       ; mul24  r1, r1, r3
/* [0x00001a88] */ 0x807be036, 0xd00049e5, // nop                       ; mov  r5rep, ra_scalar << rsi_tess_i
/* [0x00001a90] */ 0x0d9a7d40, 0x100229e7, // sub.setf  -, elem_num, r5 ; nop
/* [0x00001a98] */ 0x149c11c0, 0xd0040827, // and.ifz  r0, r0, 1        ; nop
/* [0x00001aa0] */ 0x957a7036, 0x10024c20, // mov  vpm, r0              ; mov  r0, ra_scalar
/* [0x00001aa8] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00001ab0] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00001ab8] */ 0x00000000, 0xe0020ce7, // mov  mutex, 0             ; nop
/* [0x00001ac0] */ 0x809f6000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_out_vpm_setup
/* [0x00001ac8] */ 0xffffffff, 0xe00049f0, // nop                       ; mov  vpm, -1
/* [0x00001ad0] */ 0x809f5000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00001ad8] */ 0x80827036, 0x100049f2, // nop                       ; mov  vw_addr, unif
/* [0x00001ae0] */ 0x809f8000, 0xd00049f1, // nop                       ; mov  vw_setup, r0 << rsi_chunks_vpm_setup
/* [0x00001ae8] */ 0x00000000, 0xe00049f0, // nop                       ; mov  vpm, 0
/* [0x00001af0] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00001af8] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00001b00] */ 0x159f2fc0, 0x100009e7, // mov  -, vw_wait
/* [0x00001b08] */ 0x00000001, 0xe00209a7, // mov  interrupt, 1
/* [0x00001b10] */ 0x009e7000, 0x300009e7, // thrend
/* [0x00001b18] */ 0x009e7000, 0x100009e7, // nop
/* [0x00001b20] */ 0x009e7000, 0x100009e7, // nop
// :1
/* [0x00001b28] */ 0x00100a00, 0xe0020c67, // mov  vr_setup, vpm_setup(1, 0, h32(0))
/* [0x00001b30] */ 0x15c27d80, 0x100009e7, // mov  -, vpm
/* [0x00001b38] */ 0x969cc03f, 0xd0025cdb, // mov  mutex, 0             ; mov  ra_temp2, 12
/* [0x00001b40] */ 0x0d9dee80, 0xd00208a7, // sub  r2, -2, r2           ; nop
/* [0x00001b48] */ 0xd47810b7, 0xd0024822, // and  r0, r0, r2           ; v8adds  r2, ra_scalar, 1
/* [0x00001b50] */ 0xec9d007d, 0xd0024825, // add  r0, r0, r1           ; v8subs  r5rep, -16, r5
/* [0x00001b58] */ 0x8c9f66d2, 0xd00248f1, // add  r3, r3, r3           ; mov  vw_setup, r2 << rsi_out_vpm_setup
/* [0x00001b60] */ 0x809f0000, 0xd00049e5, // nop                       ; mov  r5rep, r0 >> r5
/* [0x00001b68] */ 0xfff30080, 0xe0020827, // mov  r0, (1 << 7) - (13 << 16)
/* [0x00001b70] */ 0x0d98cdc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_alloc_p ; nop
/* [0x00001b78] */ 0x8c7b2c3f, 0x10020827, // add  r0, ra_scalar, r0    ; mov  -, vw_wait
/* [0x00001b80] */ 0x8d6cdbbf, 0x100447b0, // sub.ifz  ra_scalar, r5, ra_temp2 ; mov  vpm, rb_list_tail
/* [0x00001b88] */ 0x00000000, 0xf0f549e7, // bra  -, ra_temp
/* [0x00001b90] */ 0x0d98ddc0, 0xd00229e7, // sub.setf  -, elem_num, rsi_alloc_size ; nop
/* [0x00001b98] */ 0x8d6f5780, 0xd00447b1, // sub.ifz  ra_scalar, r3, ra_temp2 ; mov  vw_setup, r0 << rsi_out_vdw_setup_0
/* [0x00001ba0] */ 0x0d9ccbc0, 0xd0021ca7, // sub  vw_addr, r5, 12      ; nop
};

void vg_tess_fill_shader_link(void *p_in, unsigned int base
   , unsigned int vg_tess_shader_if
   )
{
   unsigned int *p = (unsigned int *)p_in;
   unsigned int i;
   for (i = 0; i != (VG_TESS_FILL_SHADER_SIZE / 4); ++i) {
      p[i] = array[i];
   }
   assert(p[70] == 0xdeadbeef);
   p[70] = base + 1008;
   assert(p[72] == 0xdeadbeef);
   p[72] = base + 840;
   assert(p[94] == 0xdeadbeef);
   p[94] = base + 3840;
   assert(p[174] == 0xdeadbeef);
   p[174] = base + 792;
   assert(p[236] == 0xdeadbeef);
   p[236] = base + 1128;
   assert(p[240] == 0xdeadbeef);
   p[240] = base + 1352;
   assert(p[284] == 0xdeadbeef);
   p[284] = base + 5656;
   assert(p[370] == 0xdeadbeef);
   p[370] = base + 1560;
   assert(p[386] == 0xdeadbeef);
   p[386] = base + 1656;
   assert(p[976] == 0xdeadbeef);
   p[976] = base + 3896;
   assert(p[1168] == 0xdeadbeef);
   p[1168] = base + 4712;
   assert(p[1302] == 0xdeadbeef);
   p[1302] = base + 4712;
   assert(p[1396] == 0xdeadbeef);
   p[1396] = base + 5512;
   assert(p[1422] == 0xdeadbeef);
   p[1422] = base + 5712;
   assert(p[1428] == 0xdeadbeef);
   p[1428] = base + 4712;
   assert(p[1436] == 0xdeadbeef);
   p[1436] = base + 5816;
   assert(p[1456] == 0xdeadbeef);
   p[1456] = base + 3840;
   assert(p[1504] == 0xdeadbeef);
   p[1504] = base + 6136;
   assert(p[1670] == 0xdeadbeef);
   p[1670] = vg_tess_shader_if + 0;
   assert(p[1682] == 0xdeadbeef);
   p[1682] = vg_tess_shader_if + 4;
}
