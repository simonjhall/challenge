/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef _GHW_COMPOSER_IMPL_H_
#define _GHW_COMPOSER_IMPL_H_

#include "list.h"

#include <stdint.h>

#ifdef PC_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

//#include "proto.h"
#include "ghw_composer.h"
#include "ghw_allocator.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <dlfcn.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include "v3d_linux.h"
#include "cacheops.h"
#include "ghw_composer.h"
#include "ghw_allocator.h"
#define LOGT(x...) do {} while (0)

#endif


#define V3D_DEVICE "/dev/v3d"

#ifndef MAX
#define MAX(A,B)      ((A) < (B) ? (B) : (A))
#endif

namespace ghw {

#define RB_SWAP_SHADER_NUM_VARYINGS (0)
static u32 rb_swap_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
};

static u32 rb_swap_shader_uniforms[] = {
    0x00FF00FF,
    0xFF00FF00,
    16,
    0,
};

static u32 rb_swap_shader[] =
{
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x400009e7, /* sbwait */
		0x009e7000, 0x800009e7, /* tlb load */
		0x14827d00, 0x10020867, /* and	r1, r4, aunif	 */
		0x14827d00, 0x10020827, /* and	r0, r4, aunif	 */
		0x10827380, 0x10020867, /* ror	r1, r1, aunif	 */
		0x159e7200, 0x10020ba7, /* or	tlbc, r1, r0   */
		0x009e7000, 0x500009e7, /* nop				  ; sbdone */
		0x009e7000, 0x300009e7, /* nop				  ; thrend */
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x100009e7, /* nop */
		0x009e7000, 0x100009e7, /* nop */
};


#define FB_COMP_SHADER_NUM_VARYINGS (2)
static u32 fb_comp_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
};

static u32 fb_comp_shader_uniforms[] = {
    0,
    0,
    0x00FF00FF,
    0xFF00FF00,
    0,
};

static u32 argb_shader[] =
{
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */
    0x14827d00, 0x10020867, /* and  r1, r4, aunif    */
    0x14827d00, 0x10020827, /* and  r0, r4, aunif    */
    0x10827380, 0x10020867, /* ror  r1, r1, aunif    */
    0x159e7200, 0x10020027, /* or   ar0, r1, r0       */
    0x159e7200, 0x10020067, /* or   ar1, r1, r0       */
    0x009e7000, 0x400009e7, /* nop */
    0x17067d80, 0x86020067, /* not ar1 unpack(ar1) ; tlb colo load */
    0x009e7000, 0x100009e7, /* nop */
    0x60067034, 0x160049c1, /* vmuld8 br1 ar1 r4 */
    0x009e7000, 0x100009e7, /* nop */
    0x1e001f80, 0x10020ba7, /* v8add  tlbc, br1,ar0*/
    0x009e7000, 0x500009e7, /* nop                ; sbdone */
    0x009e7000, 0x300009e7, /* nop                ; thrend */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
};

/* RGB shader
 * Uniform 0 = TMU Config 0
 * Uniform 1 = TMU Config 1
 * Uniform 2 = RB Mask
 * Uniform 3 = AG Mask
 * Uniform 4 = rb_swap rotate = 16 or 0 */

static u32 rgbx_shader[] =
{
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */
    0x14827d00, 0x10020867, /* and  r1, r4, aunif    */
    0x14827d00, 0x10020827, /* and  r0, r4, aunif    */
    0x10827380, 0x10020867, /* ror  r1, r1, aunif    */
    0x159e7200, 0x40020ba7, /* or   tlbc, r1, r0   ; sbwait     */
    0x009e7000, 0x500009e7, /* nop                ; sbdone */
    0x009e7000, 0x300009e7, /* nop                ; thrend */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
};


#define YUV444_SHADER_NUM_VARYINGS (4)

static u32 yuv444_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,

};

#define YUV444_TEX_PA_OFFSET0 0
#define YUV444_TEX_STRIDE_OFFSET0 1
#define YUV444_TEX_PA_OFFSET1 2
#define YUV444_TEX_STRIDE_OFFSET1 3
#define YUV444_TEX_UV_SWAP_OFFSET 6
#define YUV444_TEX_ROTATE_OFFSET 11

static u32 yuv444_shader_uniforms[] = {
    0,
    0,
	0,
	0,
	1,
	1,
	0,
	1,
	1,
	1,
	1,
	0,
	8,
};

/* Y shader
 * Uniform 0 = TMU Config 0
 * Uniform 1 = TMU Config 1
 * Uniform 2 = 1/width in float
 * Uniform 3 = 8
 * Uniform 4 = rb_swap rotate = 16 or 0 */
static u32 yuv444_shader[] =
{
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x149e7900, 0x10020027, /* and   ar0, r4, r4 */

    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x149e7900, 0x10020067, /* and   ar1, r4, r4 */

	0x15027d80, 0x1a020827, /* or r0, ar0(G) */
	0x15027d80, 0x1c020867, /* or r1, ar0(B) */

	0x15067d80, 0x18620027, /* or ar0 (B), ar1(R), ar1(R) */

	0x0c067180, 0x1a020827, /* add r0 , ar1(G), r0 */
	0x0c067380, 0x1c020867, /* add r1 , ar1(A), r1 */
	0x0c827180, 0x10020827, /* add r0 , r0, aunif */
	0x0c827380, 0x10020867, /* add r1 , r1, aunif */

    0x14827d80, 0x100229e7, /* and    aunif, aunif ;set flags*/

	0x0f827180, 0x10540027, /* asr ar0 (G), r0, 1 */
	0x0f827380, 0x10740027, /* asr ar0 (A), r1, 1 */
	0x0f827180, 0x10760027, /* asr ar0 (G), r0, 1 */
	0x0f827380, 0x10560027, /* asr ar0 (A), r1, 1 */

	
	0x009e7000, 0x100009e7, /* nop */
	0x10020dc0, 0x10020ba7, /* ror tlbc, ar0, bunif */

    0x009e7000, 0x500009e7, /* nop                ; sbdone */
    0x009e7000, 0x300009e7, /* nop                ; thrend */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
};



#define YSCALE_SHADER_NUM_VARYINGS (8)

static u32 yscale_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,

};

#define YSCALE_TEX_PA_OFFSET0 0
#define YSCALE_TEX_STRIDE_OFFSET0 1
#define YSCALE_TEX_PA_OFFSET1 3
#define YSCALE_TEX_STRIDE_OFFSET1 4
#define YSCALE_TEX_PA_OFFSET2 7
#define YSCALE_TEX_STRIDE_OFFSET2 8
#define YSCALE_TEX_PA_OFFSET3 11
#define YSCALE_TEX_STRIDE_OFFSET3 12
static u32 yscale_shader_uniforms[] = {
    0,
    0,
    0xFF,
	0,
	0,
	0xFF,
	8,
	0,
	0,
	0xFF,
	16,
	0,
	0,
	0xFF,
	24,
};

/* Y shader
 * Uniform 0 = TMU Config 0
 * Uniform 1 = TMU Config 1
 * Uniform 2 = 1/width in float
 * Uniform 3 = 8
 * Uniform 4 = rb_swap rotate = 16 or 0 */
static u32 yscale_shader[] =
{
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x14827d00, 0x100200a7, /* and   ar2, r4, aunif   */

    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x14827d00, 0x10020827, /* and   r0, r4, aunif   */
	0x11827180, 0x10020827, /* lsl	 r0, r0, aunif	 */
	0x150a7c00, 0x100200a7, /* or	 ar2, r0, ar2	 */
	
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x14827d00, 0x10020827, /* and   r0, r4, aunif   */
	0x11827180, 0x10020827, /* lsl	 r0, r0, aunif	 */
	0x150a7c00, 0x100200a7, /* or	 ar2, r0, ar2	 */

    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x60020e67, /* fadd  tmu0_t, r0, r5  */
    0x203e3037, 0x100049e0, /* fmul  r0, w, varying    */
    0x019e7a00, 0x10020e27, /* fadd  tmu0_s, r0, r5  */
    0x009e7000, 0xA00009e7, /* nop                ; tmu load */

    0x14827d00, 0x10020827, /* and   r0, r4, aunif   */
	0x11827180, 0x10020827, /* lsl	 r0, r0, aunif	 */
	0x150a7c00, 0x40020ba7, /* or	 tlbc, r0, ar2	 */

    0x009e7000, 0x500009e7, /* nop                ; sbdone */
    0x009e7000, 0x300009e7, /* nop                ; thrend */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
    0x009e7000, 0x100009e7, /* nop */
};


/* Dither shader for RGB666 display */
#define DITHER_SHADER_NUM_VARYINGS (0)
static u32 dither_shader_vertices[] = {
    /*  Vertex 0 */
        0x00000000, /* (x,y) */
        0x3F800000, /* (z) */
        0x3F800000, /* (w) */

    /*  Vertex 1 */
        0x00000000, /* (x,y) */
        0x3F800000, /* (z) */
        0x3F800000, /* (w) */

    /*  Vertex 2 */
        0x00000000, /* (x,y) */
        0x3F800000, /* (z) */
        0x3F800000, /* (w) */

    /*  Vertex 3 */
        0x00000000, /* (x,y) */
        0x3F800000, /* (z) */
        0x3F800000, /* (w) */
    };


#define DITHER_MATRIX_OFFSET 4
static u32 dither_uniforms[] =
{
	0x3,
	0x4,
	0x3,
	0x2,
	0
};

static u32 dither_shader[] =
{

	0x14829f80, 0x10020827, /* and r0 y, aunif */
	0x40827006, 0x100049e0, /* mul r0 r0, aunif */
	0x14a60f80, 0x10020867, /* and r1 x, unif */
	0x0c9e7200, 0x60020827, /* add r0 r0, r1*/
	0x11827180, 0x10020827, /* shl r0 r0, unif*/
	0x0c827180, 0x10020e27, /* add tmu0_s r0, unif*/

	0x009e7000, 0xa00009e7, /* tmu load */
	0x159e7900, 0x40020827, /* mov	r0, r4,r4	  */
	0x009e7000, 0x800009e7, /* tlb load */
	0x809e7004, 0x100049e0, /* v8min r0 r4, r0 */

	0x1e9e7800, 0x10020ba7, /* v8adds	tlbc, r4,r0	  ; sbwait */
	0x009e7000, 0x500009e7, /* nop				  ; sbdone */
	0x009e7000, 0x300009e7, /* nop				  ; thrend */
	0x009e7000, 0x100009e7, /* nop */
	0x009e7000, 0x100009e7, /* nop */
};

static uint32_t dither_shader_matrix[] = {
	0x00000100, 0x00040304,0x00010101, 0x00050305,
	0x00060406, 0x00020202,0x00070407, 0x00030203,
	0x00020102, 0x00060306,0x00010101, 0x00050305,
	0x00080408, 0x00040204,0x00070407, 0x00030203
};


/* RGB2YUV shader - vertex, varyings */
#define RGB2YUV422_SHADER_NUM_VARYINGS (0)
static u32 rgb2yuv422i_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
};

#define RGB2YUV422I_TEX_PA_OFFSET 0
#define RGB2YUV422I_TEX_STRIDE_OFFSET 1
#define RGB2YUV422I_X_SCALE_OFFSET 2
#define RGB2YUV422I_TEX_PA_PLUS4 3
#define RGB2YUV422I_UV 28
#define RGB2YUV422I_Y 29
#define RGB2YUV422I_ROR 30

static float rgb2yuv422i_shader_uniformsf[] = {
	0,
	0,
	0,
	0,
	16,   // Y offset
	128/2,  // U offset
	128/2,	// V offset
	255*	0.257,
	255*	-0.148/2,
	255*	0.439/2,
	255*	0.504,
	255*	-0.291/2,
	255*	-0.368/2,
	255*	0.098,
	255*	0.439/2,
	255*	-0.071/2,
	16,   // Y offset
	128/2,	// U offset
	128/2,	// V offset
	255*	0.257,
	255*	-0.148/2,
	255*	0.439/2,
	255*	0.504,
	255*	-0.291/2,
	255*	-0.368/2,
	255*	0.098,
	255*	0.439/2,
	255*	-0.071/2,
	0,
	0,
	0
	
};

static u32 rgb2yuv422i_shader[] =
{
	0x15827d80, 0x10020827, /* r0 = Texture PA */
	0x40829037, 0x100049e1, /* mul24  r1, aunif, y   */
	0x40a60037, 0x100049e2, /* mul24  r2, x, bunif   */
	0x0c9e7440, 0x10020867, /* add  r1, r1, r2   */
	0x0c9e7200, 0x600200e7, /* add  ar3, r1, r0   */
	0x0c9e7200, 0x10020e27, /* add  tmu0_s, r1, r0   */
	0x0c0e0f80, 0x10020e27, /* add	tmu0_s, ar3, bunif	 */

	0x15827d80, 0x10020827, /* r0 = 16.0 */
	0x15827d80, 0x10020867, /* r1 = 64.0 */
	0x15827d80, 0xA00208a7, /* r2 = 64.0 */

	0x20827026, 0x190049c0, /* fmul br0, r4 (R), aunif	 */
	0x20827026, 0x190049c1, /* fmul br1, r4 (R), aunif	 */
	0x218001e6, 0x19024802, /* fmul br2, r4 (R), aunif	;  fadd r0, r0,br0 */
	0x218013e6, 0x1b024840, /* fmul br0, r4 (G), aunif	;  fadd r1, r1,br1 */
	0x218025e6, 0x1b024881, /* fmul br1, r4 (G), aunif	;  fadd r2, r2,br2 */
	0x218001e6, 0x1b024802, /* fmul br2, r4 (G), aunif	;  fadd r0, r0,br0 */
	0x218013e6, 0x1d024840, /* fmul br0, r4 (B), aunif	;  fadd r1, r1,br1 */
	0x218025e6, 0x1d024881, /* fmul br1, r4 (B), aunif	;  fadd r2, r2,br2 */
	0x218001e6, 0x1d024002, /* fmul br2, r4 (B), aunif	;  fadd ar0, r0,br0 */
	0x019c13c0, 0x10020067, /*                                       ;  fadd ar1, r1,br1 */
	0x019c25c0, 0x100200a7, /*                                       ;  fadd ar2, r2,br2 */

	0x15827d80, 0x10020827, /* r0 = 16.0 */
	0x01060f80, 0x10020867, /* r1 = ar1 + 64 */
	0x010a0f80, 0xA00208a7, /* r2 = ar2 + 64  */

	0x20827026, 0x190049c0, /* fmul br0, r4 (R), aunif	 */
	0x20827026, 0x190049c1, /* fmul br1, r4 (R), aunif	 */
	0x218001e6, 0x19024802, /* fmul br2, r4 (R), aunif	;  fadd r0, r0,br0 */
	0x218013e6, 0x1b024840, /* fmul br0, r4 (G), aunif	;  fadd r1, r1,br1 */
	0x218025e6, 0x1b024881, /* fmul br1, r4 (G), aunif	;  fadd r2, r2,br2 */
	0x218001e6, 0x1b024802, /* fmul br2, r4 (G), aunif	;  fadd r0, r0,br0 */
	0x218013e6, 0x1d024840, /* fmul br0, r4 (B), aunif	;  fadd r1, r1,br1 */
	0x218025e6, 0x1d024881, /* fmul br1, r4 (B), aunif	;  fadd r2, r2,br2 */
	0x218001e6, 0x1d024802, /* fmul br2, r4 (B), aunif	;  fadd r0, r0,br0 */
	0x019c13c0, 0x10020867, /*                                       ;  fadd r1, r1,br1 */
	0x019c25c0, 0x100208a7, /*                                       ;  fadd r2, r2,br2 */

	0x15827d80, 0x10022067, /* or ar1 aunif aunif ; set flags*/
	0x079e7240, 0x10f600a7, /* ftoi  ar2, r1 (pack to a) , if ~0 */
	0x079e7480, 0x10d600a7, /* ftoi  ar2, r2 (pack to g) , if ~0 */
	0x079e7240, 0x10d400a7, /* ftoi  ar2, r1 (pack to g) , if 0 */
	0x079e7480, 0x10f400a7, /* ftoi  ar2, r2 (pack to a) , if 0 */

	0x15827d80, 0x10022067, /* or ar1 aunif aunif ; set flags*/
	0x07027d80, 0x10e400a7, /* ftoi  ar2, ar0 (pack to b) , if 0 */
	0x079e7000, 0x10c400a7, /* ftoi  ar2, r0 (pack to r) , if 0 */
	0x07027d80, 0x10c600a7, /* ftoi  ar2, ar0 (pack to r) , if ~0 */
	0x079e7000, 0x10e600a7, /* ftoi  ar2, r0 (pack to b) , if ~0 */

	0x009e7000, 0x100009e7, /* nop */
	0x100a0dc0, 0x10040ba7, /* ror tlbc, ar2,bunif ; if 0*/
	0x150a7d80, 0x10060ba7, /* or tlbc, ar2,ar2 ; if ~0*/

	0x009e7000, 0x500009e7, /* nop				  ; sbdone */
	0x009e7000, 0x300009e7, /* nop				  ; thrend */
	0x009e7000, 0x100009e7, /* nop */
	0x009e7000, 0x100009e7, /* nop */

};


/* YuvTile shader - vertex, varyings */
#define YUVTILE_SHADER_NUM_VARYINGS (0)
static u32 yuvtile_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
};

#define YUVTILE_TEX_STRIDE_OFFSET 2
#define YUVTILE_TEX_PA_OFFSET 3
#define YUVTILE_TEX_ROTATE_OFFSET 4
#define YUVTILE_TEX_UV_SWAP_OFFSET 6

static u32 yuvtile_shader_uniforms[] = {
	1,
	0xFFFFFFFC,
	0,
	0,
	0,
	1,
	0,
	8,
	8,
	16,
	16,

};

static u32 yuvtile_shader[] =
{
	0x11a60dc0, 0x10020827, /* r0 = x << 1*/
	0x14827c00, 0x10020827, /* r0 = r0 & 0xFFFFFFFC*/
	0x4082903e, 0x600049e2, /* r2 = y*s */
	0x0c9e7400, 0x10020827, /* r0 = r2 +r0*/
	0x0c827c00, 0x10020e27, /* tmu0_s = base +r0*/

	0x009e7000, 0xA00009e7, /* tmu load */
	0x10827980, 0x10020027, /* ror ar0, r4,aunif*/
	0x14a60dc0, 0x100229e7, /*  x & 1; set flags*/
	0x15027d80, 0x18040827, /* or r0, ar0(R) */
	0x15027d80, 0x1c060827, /* or r0, ar0(B) */

	0x14827d80, 0x100229e7, /*  aunif; set flags*/

	0x11020dc0, 0x1a040867, /* r1 = ar0(G) << 8*/
	0x11020dc0, 0x1e060867, /* r1 = ar0(G) << 8*/
	0x159e7200, 0x10020827, /* or r0 , r1 ,r0*/

	0x11020dc0, 0x1e040867, /* r1 = ar0(A) << 16*/
	0x11020dc0, 0x1a060867, /* r1 = ar0(A) << 16*/
	0x159e7200, 0x10020ba7, /* or tlbc, r1,r0 */

	0x009e7000, 0x500009e7, /* nop				  ; sbdone */
	0x009e7000, 0x300009e7, /* nop				  ; thrend */
	0x009e7000, 0x100009e7, /* nop */
	0x009e7000, 0x100009e7, /* nop */

};

/* Ytile shader - vertex, varyings */
#define YTILE_SHADER_NUM_VARYINGS (0)
static u32 ytile_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
};

#define YTILE_TEX_STRIDE_OFFSET 5
#define YTILE_TEX_PA_OFFSET 7

static u32 ytile_shader_uniforms[] = {
	1,
	0xFFFFFFFE,
	1,
	2,
	1,
	0,
	2,
	0,

};

static u32 ytile_shader[] =
{
	0x0fa60dc0, 0x10020827, /* r0 = x >> 1*/
	0x14827c00, 0x10020827, /* r0 = r0 & 0xFFFFFFFE*/
	0x14a60dc0, 0x10020867, /* r1 = x & 1*/
	0x0c9e9fc0, 0x100208a7, /* r2 = y+y*/
	0x14a60dc0, 0x100228e7, /* r3 = 	x & 2 ; set flags*/
	0x0c827c80, 0x100608a7, /* r2 = r2 +1*/
	0x4c827072, 0x10024822, /* r0 = r0 +r1 ;  r2 = r2*s*/
	0x11827180, 0x60020827, /* r0 = r0 << 2*/
	0x0c9e7400, 0x10020827, /* r0 = r2 +r0*/
	0x0c827c00, 0x10020e27, /* tmu0_s = base +r0*/
	
	0x009e7000, 0xA00009e7, /* tmu load */
	0x159e7900, 0x10020ba7, /* or tlbc, r4,r4 */

	0x009e7000, 0x500009e7, /* nop				  ; sbdone */
	0x009e7000, 0x300009e7, /* nop				  ; thrend */
	0x009e7000, 0x100009e7, /* nop */
	0x009e7000, 0x100009e7, /* nop */

};



/* YUV420 shader - vertex, varyings */
#define YUV420_SHADER_NUM_VARYINGS (0)
static u32 yuv420_shader_vertices[] = {
/*  Vertex 0 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 1 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 2 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */

/*  Vertex 3 */
    0x00000000, /* (x,y) */
    0x3F800000, /* (z) */
    0x3F800000, /* (w) */
};

/* YUV420 SP shader - uniform, shader */
#define YUV420SP_SHADER_WIDTH_OFFSET       (1)
#define YUV420SP_SHADER_YBASE_OFFSET       (2)
#define YUV420SP_SHADER_UVBASE_OFFSET      (3)
#define YUV420SP_SHADER_USHIFT_OFFSET      (4)
#define YUV420SP_SHADER_VSHIFT_OFFSET      (5)

static u32 yuv420sp_shader_uniforms[] = {
/*  0xAABBGGRR */
    1,          /* Dummy - to read y-pos to a variable */
    0,          /* width  - to be updated */
    0,          /* y-base - to be updated */
    0,          /* uv-base - to be updated */
    8,          /* u shift (0 for NV12, 8 for NV21) */
    0,          /* v shift (8 for NV12, 0 for NV12) */
    0x3f008081, /* (float)128/255 - U/V clamp */
    0x3d808081, /* (float)16/255  - Y clamp */
    0x3faf8152, /* (float)1.596/1.164 - V coeff for R */
    0x3fdde921, /* (float)2.018/1.164 - U coeff for B */
    0x3f32cddb, /* (float)0.813/1.164 - V coeff for G */
    0x3eabfc7b, /* (float)0.391/1.164 - U coeff for G */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0xFF000000, /* Alpha = 1 */
};

static u32 yuv420sp_shader[] =
{
/*    o r  m           w  */
/* r0=Y,r1=U,r2=V; unif(1,w,Ybase,UVbase,Ushift,Vshift); used(r0-r4,t0-t3) */
    0x4e829fbe, 0x10024860, /* r1=shr(b,a),   r0=mul24(b,a)  || a=u(1),b=(y)        ; r0 = y, r1=y/2 */
    0x559e0fc7, 0x100248a0, /* r2=or(b,b),    r0=mul24(r0,b) || b=u(w)              ; r0 = yw, r2=w */
    0x4ca6718a, 0x10024821, /* r0=add(r0,a),  r1=mul24(r1,r2)|| a=(x)               ; r0 = yw+x, r1=y/2*w */
    0x0c9e01c0, 0x10020e27, /* s =add(r0,b),  nop            || b=u(Ybase)          ; s = Ybase + y*w + x */
    0xaca48c7f, 0xd0024863, /* r1=add(a,r1),  r3=v8max(b,b)  || a=(x),b=(8)         ; r1 = y/2*w + x, r3 = 8 */
    0xac803c7f, 0xd0024e22, /* s =add(a,r1),  r2=v8max(b,b)  || a=u(UVb),b=3        ; s  = UVbase+y/2*w+x, r2=3 */

    0x14a42dc0, 0xd0020827, /* r0=and(a,b),   nop            || a=(x),b=(2)         ; r0 = x&2 */
    0x119e7080, 0x100200e7, /* t3=shl(r0,r2), nop            || nop                 ; t3 = (x&2)*8 */
    0x14a67c80, 0x60020827, /* r0=and(a,r2),  nop            || a=(x)               ; r0 = x&3 ; Thread Switch */

    0x119c31c0, 0xd0020027, /* t0=shl(r0,b),  nop            || b=(3)               ; t0 = x&3 * 8 */
    0x0c0e0dc0, 0x10020067, /* t1=add(a,b),   nop            || a=(t3),b=u(Us)      ; t1=Ushift+(x&2)*8 */

    0x0c0e0dc0, 0xa00200a7, /* t2=add(a,b),   nop            || a=t3, b=u(Vs)       ; t2=Vshift+(x&2)*8 ; tmuload */
    0x0e027980, 0xa0020027, /* t0=shr(r4,a),  nop            || a=(t0)              ; t0 = Y in lsb ; tmuload */
    0xae0a09bf, 0x100240a2, /* t2=shr(r4,a),  r2=v8max(b,b)  || a=(t2), b=u(128/255); t2 = V in lsb, r2 = 128/255 */
    0xae0609bf, 0x10024061, /* t1=shr(r4,a),  r1=v8max(b,b)  || a=(t1), b=u(16/255) ; t1 = U in lsb, r1 = 16/255 */

    0x020a7c80, 0x180208e7, /* r3=fsub(a,r2),  nop           || a=(t2)              ; r3 = flt(V-128) */
    0x22060c9f, 0x180248a0, /* r2=fsub(a,r2),  r0=fmul(r3,b) || a=(t1), b=u(VrR)    ; r2 = flt(U-128),r0 = 1.596/1.164 V */
    0x22020c57, 0x18025840, /* r1=fsub(a,r1),  t0=fmul(r2,b) || a=(t0), b=u(UrB)    ; r1 = flt(Y-16), t0 = 2.018/1.164 U */
    0x219e021f, 0x10025801, /* r0=fadd(r1,r0), t1=fmul(r3,b) || b=u(VrG)            ; r0 = Y + VrR V, t1 = 0.813/1.164 V */
    0x21020397, 0x10024022, /* t0=fadd(r1,a),  r2=fmul(r2,b) || a=(t0), b=u(UrG)    ; t0 = Y + UrB U, r2 = 0.391/1.164 U */
    0x22060387, 0x11424860, /* r1=fsub(r1,a),  r0=fmul(r0,b) || a=(t1), b=u(1.164)  ; r1 = Y - VrG V, r0(0) = 1.164 * r0 */
    0x220202b7, 0x11624860, /* r1=fsub(r1,r2), r0=fmul(a,b)  || a=(t0), b=u(1.164)  ; r1 = Y - VrG V - UrG U, r0(2) = 1.164 * t0 */
    0x209e000f, 0x115049e0, /* nop,            r0=fmul(r1,b) || b=u(1.164)          ; r0(1) = 1.164 * r1 */
    0x159e01c0, 0x40020827, /* r0=or(r0,b),   nop            || b=u(0xFF000000)     ; r0 = r0 | alpha ; sbwait*/
    0x0e9c01c0, 0xd0020ba7, /* C=shr(r0,b),   nop            || b=(0)               ; tlb store = r0 */
    0x009e7000, 0x500009e7, /* nop            nop            || nop       || sbdone ; sbdone */
    0x009e7000, 0x300009e7, /* nop            nop            || nop       || thrend ; thrend*/
    0x009e7000, 0x100009e7, /* nop            nop            || nop       || nop    ; nop */
    0x009e7000, 0x100009e7, /* nop            nop            || nop       || nop    ; nop */
// (18)Thread two ends (2 TMU loads; 0 S writes)
};

/* YUV 420 Planar shader - uniform, shader */
#define YUV420P_SHADER_WIDTH_OFFSET       (1)
#define YUV420P_SHADER_YBASE_OFFSET       (3)
#define YUV420P_SHADER_UBASE_OFFSET       (6)
#define YUV420P_SHADER_VBASE_OFFSET       (2)

static u32 yuv420p_shader_uniforms[] = {
/*  0xAABBGGRR */
    1,          /* Dummy - to read y-pos to a variable */
    0,          /* width  - to be updated */
    0,          /* v-base - to be updated */
    0,          /* y-base - to be updated */
    3,          /* Dummy - to read const to a variable */
    8,          /* Dummy - to read const to a variable */
    0,          /* u-base - to be updated */
    0x3d808081, /* (float)16/255  - Y clamp */
    0x3f008081, /* (float)128/255 - U/V clamp */
    0x3f32cddb, /* (float)0.813/1.164 - V coeff for G */
    0x3faf8152, /* (float)1.596/1.164 - V coeff for R */
    0xFF000000, /* Alpha = 1 */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0x3f008081, /* (float)128/255 - U/V clamp */
    0x3fdde921, /* (float)2.018/1.164 - U coeff for B */
    0x3eabfc7b, /* (float)0.391/1.164 - U coeff for G */
};

static u32 yuv420p_shader[] =
{
    0x4e829fbe, 0x10024823, /* r0=shr(b,a),   r3=mul24(b,a)  || a=u(1),    b=(y)    ;     r0=y/2,       r3=y           */
    0x4e801dde, 0xd0024863, /* r1=shr(a,b),   r3=mul24(r3,a) || a=u(w),    b=(1)    ; I,  r1=w/2,       r3=yw          */
    0x4ea41dc1, 0xd00248a0, /* r2=shr(a,b),   r0=mul24(r0,r1)|| a=(x),     b=(1)    ; I,  r0=y/2*w/2,   r2=x/2         */
    0x0c9e7080, 0x10020027, /* a0=add(r0,r2), nop            ||                     ;     a0=yw/4 + x/2                */
    0x149c35c0, 0xd00208a7, /* r2=and(r2,b),  nop            ||            b=(3)    ; I,  r2=(x/2)&3                   */
    0x0c020dc0, 0x20020e27, /* s =add(a,b),   nop            || a=a0,      b=u(Vb)  ; TS, s=Vbase+(y/2)*(w/2)+(x/2)    */
    0x4ca48797, 0xd00248c0, /* r3=add(r3,a),  b0=mul24(r2,b) || a=(x)      b=(8)    ; I,  r3 = yw+x,    b0=((x/2)&3)*8 */
    0x0c827cc0, 0x10020e27, /* s =add(a,r3),  nop            || a=u(Yb),            ;     s=Ybase+yw+x                 */
// (8)Thread one ends (a0 = UVoffset, b0 = UVshift; 0 TMU loads; 2 S writes)

    0x14a60dc0, 0xa0020827, /* r0=and(a,b),   nop            || a=(x)      b=u(3)   ; TL, r0=x&3                       */
    0x4e8009c6, 0xa0024060, /* a1=shr(r4,b),  r0=mul24(r0,a) || a=u(8),    b=b0     ; TL, a1=V in lsb,  r0=(x&3)*8     */
    0x0c020dc0, 0x10020e27, /* s =add(a,b),   nop            || a=a0,      b=u(Ub)  ;     s=Ubase+(y/2)*(w/2)+(x/2)    */
    0xae827836, 0x100240a3, /* a2=shr(r4,r0), r3=v8max(a,a)  || a=u(16)             ;     a2=Y in lsb   r3=16/255      */
    0x02060dc0, 0x18020827, /* r0=fsub(a,b),  nop            || a=a1       b=u(128) ;     r0=flt(V-128)                */
    0x220a0cc7, 0x68024862, /* r1=fsub(a,r3), r2=fmul(r0,b)  || a=a2       b=u(VcG) ; TLS,r1=flt(Y-16), r2=VcG*V       */
    0x229e0287, 0x10024022, /* a0=fsub(r1,r2) r2=fmul(r0,b)  ||            b=u(VcR) ;     a0=Y-VrG*V,   r2=VcR*V       */
    0xa19e7289, 0x10024041, /* a1=fadd(r1,r2),b1=v8max(r1,r1)||                     ;     a1=R'(Y+Vr)   b1=Y           */
// (8)Thread two ends (a0 = Y-Vb, a1 = Y+Vr, b0 = UVshift, b1 = Y; 2 TMU loads; 1 S writes)

    0xa0827036, 0xa00049e0, /* nop            r0=v8max(a,a)  || a=u(FF)             ; TL, r0=alpha                     */
    0x0e9c09c0, 0x100200a7, /* a2=shr(r4,b),  nop            ||            b=b0     ;     a2=U in lsb                  */
    0x35060ff7, 0x114248e0, /* r3=or(b,b)     r0=fmul(a,b)   || a=a1       b=u(Yc)  ;     r3=Yc         r0(0)=Yc*R'    */
    0x020a0dc0, 0x18020867, /* r1=fsub(a,b)   nop            || a=a2       b=u(128) ;     r1=flt(U-128)                */
    0x2082700e, 0x100049e2, /* nop            r2=fmul(r1,a)  || a=u(UcB)            ;     r2=UcB*U                     */
    0x21801e8e, 0x100248a1, /* r2=fadd(b,r2)  r1=fmul(r1,a)  || a=u(UcG)   b=b1     ;     r2=B'(Y+Ub)   r1=UcG*U       */
    0x22027c53, 0x11624860, /* r1=fsub(a,r1)  r0=fmul(r2,r3) || a=a0                ;     r1=G'(Y-Ug-Vg)r0(2)=Yc*B'    */
    0x209e700b, 0x415049e0, /* nop            r0=fmul(r1,r3) ||                     ; SW, r0(1)=Yc*G'                  */
    0x0e9c01c0, 0xd0020ba7, /* C=shr(r0,b),   nop            ||            b=(0)    ; I,  tlbC = r0                    */
    0x009e7000, 0x500009e7, /* nop            nop            ||                     ; SU, nop                          */
    0x009e7000, 0x300009e7, /* nop            nop            ||                     ; TE, nop                          */
    0x009e7000, 0x100009e7, /* nop            nop            ||                     ;     nop                          */
    0x009e7000, 0x100009e7, /* nop            nop            ||                     ;     nop                          */
// (13)Thread three ends (1 TMU loads; 0 S writes)
};

/* YUV422I shader - uniform, shader */
#define YUV422I_SHADER_WIDTH_OFFSET     (1)
#define YUV422I_SHADER_YBASE_OFFSET     (2)
#define YUV422I_SHADER_YSHIFT_OFFSET    (4)
#define YUV422I_SHADER_USHIFT_OFFSET    (3)
#define YUV422I_SHADER_VSHIFT_OFFSET    (5)

static u32 yuv422i_shader_uniforms[] = {
/*  0xAABBGGRR */
    2,          /* Dummy - to read y-pos to a variable */
    0,          /* width  - to be updated */
    0,          /* y-base - to be updated */
    0,          /* u shift (0 for uyvy, 8 for yuyv, 16 for vyuy, 24 for yvyu) */
    8,          /* y shift (8 for uyvy/vyuy, 0 for yuyv/yvyu) */
    16,         /* v shift (0 for vyuy, 8 for yvyu, 16 for uyvy, 24 for yuyv) */
    0xFF000000, /* Alpha = 1 */
    0x3f008081, /* (float)128/255 - U/V clamp */
    0x3d808081, /* (float)16/255  - Y clamp */
    0x3faf8152, /* (float)1.596/1.164 - V coeff for R */
    0x3fdde921, /* (float)2.018/1.164 - U coeff for B */
    0x3f32cddb, /* (float)0.813/1.164 - V coeff for G */
    0x3eabfc7b, /* (float)0.391/1.164 - U coeff for G */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
    0x3f94fdf4, /* (float)1.164 - Y coeff */
};

static u32 yuv422i_shader[] =
{
    0x55829dbe, 0x10024823, /* r0=or(a,a),    r3=mul24(b,a)  || a=u(2),    b=(y)    ;     r3=2y,        r0 = 2         */
    0x54a41dc6, 0xd00248a0, /* r2=and(a,b),   r0=mul24(r0,a) || a=(x),     b=1      ; I,  r2=x&1,       r0 = 2x        */
    0x518045de, 0xd00248a3, /* r2=shl(r2,b),  r3=mul24(r3,a) || a=u(w),    b=4      ; I,  r2=(x&1)*16,  r3 = 2yw       */
    0x0c9e07c0, 0x100208e7, /* r3=add(r3,b),  nop            ||            b=u(Yb)  ;     r3=Ybase+2yw                 */
    0xac9e063f, 0x60025e01, /* s =add(r3,r0), a1=v8max(b,b)  ||            b=u(Us)  ; LTS,s =Ybase+2yw+2x, a1=Ushift   */
    0x0c827c80, 0x10020027, /* a0=add(a,r2),  nop            || a=(Ys)              ;     a0=Yshift+(x&1)*16           */
    0x009e7000, 0x100009e7, /* nop            nop            ||                     ;     nop                          */
/* TODO: Removing the above NOP and moving the LTS instr up causes 2ms extra on Athena for VGA */
// (7)Thread one ends (a0 = Yshift, a1 = Ushift; 0 TMU loads; 1 S writes)

    0x159e0fc0, 0xa00208a7, /* r2=or(b,b),    nop            ||            b=u(Vs)  ; TL, r2 = Vshift                  */
    0xae0209bf, 0x10024020, /* a0=shr(r4,a),  r0=v8max(b,b)  || a=(a0),    b=u(FF)  ;     a0=Y in lsb,  r0 = alpha     */
    0xae0a08bf, 0x100240a2, /* a2=shr(r4,r2), r2=v8max(b,b)  ||            b=u(128) ;     a2=V in lsb,  r2 = 128/255   */
    0xae0609bf, 0x10024061, /* a1=shr(r4,a),  r1=v8max(b,b)  || a=(a1),    b=u(16)  ;     a1=U in lsb,  r1 = 16/255    */
    0x020a7c80, 0x180208e7, /* r3=fsub(a,r2), nop            || a=(a2)              ;     r3=flt(V-128)                */
    0x22060c9f, 0x18024880, /* r2=fsub(a,r2), b0=fmul(r3,b)  || a=(a1),    b=u(VcR) ;     r2=flt(U-128),b0 = VcR*V     */
    0x22020c57, 0x18025840, /* r1=fsub(a,r1), a0=fmul(r2,b)  || a=(a0),    b=u(UcB) ;     r1=flt(Y-16), a0 = UcB*U     */
    0x218003de, 0x100258c1, /* r3=fadd(r1,b), a1=fmul(r3,a)  || a=u(VcG)   b=(b0)   ;     r3=R'(Y+Vr),  a1 = VcG*V     */
    0x21020397, 0x10024022, /* a0=fadd(r1,a), r2=fmul(r2,b)  || a=(a0),    b=u(UcG) ;     a0=B'(Y+Ub),  r2 = UcG*U     */
    0x2206039f, 0x11424860, /* r1=fsub(r1,a), r0=fmul(r3,b)  || a=(a1),    b=u(Yc)  ;     r1=Y-VrG*V,   r0(0) = Yc*R'  */
    0x220202b7, 0x11624860, /* r1=fsub(r1,r2),r0=fmul(a,b)   || a=(a0),    b=u(Yc)  ;     r1=G'(Y-Ug-Vg) r0(2) = Yc*B' */
    0x209e000f, 0x415049e0, /* nop,           r0=fmul(r1,b)  ||            b=u(Yc)  ; I,  r0(1)=Yc*G'                  */
    0x0e9c01c0, 0xd0020ba7, /* C=shr(r0,b),   nop            ||            b=(0)    ; I,  tlbC=r0                      */
    0x009e7000, 0x500009e7, /* nop            nop            ||                     ; SU, nop                          */
    0x009e7000, 0x300009e7, /* nop            nop            ||                     ; TE, nop                          */
    0x009e7000, 0x100009e7, /* nop            nop            ||                     ;     nop                          */
    0x009e7000, 0x100009e7, /* nop            nop            ||                     ;     nop                          */
// (17)Thread two ends (1 TMU loads; 0 S writes)
};

typedef struct __attribute__((packed)) {
        u32 tile_alloc_addr;
        u32 tile_buffer_size;
        u32 tile_state_addr;
        unsigned char width; // in tiles;
        unsigned char height; // in tiles;
        unsigned char other;
} tile_bin_config_t;

typedef struct __attribute__((packed)) {
        u32 fb_addr;
        unsigned short width;
        unsigned short height;
        unsigned char format; // in tiles;
        unsigned char other;
} tile_render_config_t;

typedef struct __attribute__((packed)) {
        u32 config;
        u32 code;
        u32 uniforms;
        u32 vertices;
} shader_record_t;

#define NUM_SHADERS				(13)
#define ARGB_SHADER				(0)
#define RGBX_SHADER				(1)
#define DITHER_SHADER			(2)
#define DITHER_SHADER_MATRIX	(3)
#define YUV420SP_SHADER			(4)
#define YUV420P_SHADER			(5)
#define RB_SWAP_SHADER			(6)
#define YUV422I_SHADER			(7)
#define RGB2YUV422I_SHADER		(8)
#define YTILE_SHADER			(9)
#define YSCALE_SHADER			(10)
#define YUVTILE_SHADER			(11)
#define YUV444_SHADER			(12)

static u32 *ghwV3dShaders[] = {
    argb_shader,
    rgbx_shader,
    dither_shader,
	dither_shader_matrix,
    yuv420sp_shader,
	yuv420p_shader,
	rb_swap_shader,
    yuv422i_shader,
	rgb2yuv422i_shader,
	ytile_shader,
	yscale_shader,
	yuvtile_shader,
	yuv444_shader,
};

static u32 ghwV3dShaderSizes[] = {
    sizeof(argb_shader),
    sizeof(rgbx_shader),
    sizeof(dither_shader),
	sizeof(dither_shader_matrix),
    sizeof(yuv420sp_shader),
    sizeof(yuv420p_shader),
	sizeof(rb_swap_shader),
    sizeof(yuv422i_shader),
	sizeof(rgb2yuv422i_shader),
	sizeof(ytile_shader),
	sizeof(yscale_shader),
	sizeof(yuvtile_shader),
	sizeof(yuv444_shader),
};

static inline void put_short(uint8_t *p, uint16_t n)
{
   p[0] = (uint8_t)n;
   p[1] = (uint8_t)(n >> 8);
}

static inline void put_word(uint8_t *p, uint32_t n)
{
   p[0] = (uint8_t)n;
   p[1] = (uint8_t)(n >> 8);
   p[2] = (uint8_t)(n >> 16);
   p[3] = (uint8_t)(n >> 24);
}

static inline void add_byte(uint8_t **p, uint8_t n)
{
   *((*p)++) = n;
}

static inline void add_short(uint8_t **p, uint16_t n)
{
   put_short(*p, n);
   (*p) += 2;
}

static inline void add_word(uint8_t **p, uint32_t n)
{
   put_word(*p, n);
   (*p) += 4;
}

class GhwImgBufImpl : public GhwImgBuf {
public:
    GhwImgBufImpl();
    GhwImgBufImpl(GhwImgBuf* buf);
    ~GhwImgBufImpl();
    virtual ghw_error_e     setMemHandle(GhwMemHandle* mem_handle);
    virtual ghw_error_e     getMemHandle(GhwMemHandle*& mem_handle);
    virtual ghw_error_e     setGeometry(u32 width, u32 height);
    virtual ghw_error_e     getGeometry(u32& width, u32& height);
    virtual ghw_error_e     setFormat(u32 format, u32 layout, u32 blend_type);
    virtual ghw_error_e     getFormat(u32& format, u32& layout, u32& blend_type);
    virtual ghw_error_e     setCrop(u32 left, u32 top, u32 right, u32 bottom);
    virtual ghw_error_e     setCrop(u32 width, u32 height);
    virtual ghw_error_e     getCrop(u32& left, u32& top, u32& right, u32& bottom);
    virtual ghw_error_e     dump(u32 level);
private:
    GhwMemHandle* memHandle;
    u32           mWidth;
    u32           mHeight;
    u32           mFormat;
    u32           mLayout;
    u32           blendType;
    u32           mLeft;
    u32           mTop;
    u32           mRight;
    u32           mBottom;
};

class GhwImgOpImpl : public GhwImgOp {
public:
    GhwImgOpImpl();
    virtual ~GhwImgOpImpl();
    virtual ghw_error_e     setDstWindow(u32 left, u32 top, u32 right, u32 bottom);
    virtual ghw_error_e     getDstWindow(u32& left, u32& top, u32& right, u32& bottom);
    virtual ghw_error_e     setTransform(u32 transform);
    virtual ghw_error_e     getTransform(u32& transform);
    virtual ghw_error_e     dump(u32 level);
private:
    u32           mTransform;
    u32           mLeft;
    u32           mTop;
    u32           mRight;
    u32           mBottom;
};

typedef List<GhwMemHandle*> GhwMemHandleList;
typedef Node<GhwMemHandle*> GhwMemHandleNode;
class Job {
	public:
		Job(){};
		~Job() {
			GhwMemHandleNode* node = mList.getHead();
			while(node) {
				node->get()->release();
				mList.removeNode(node);
				node = mList.getHead();
				}
			};
		void dump() {
			LOGD("Job = %x",this);
			GhwMemHandleNode* node = mList.getHead();
			while(node) {
				node->get()->dump();
				node = node->getNext();
				}
			};
		void addHandle(GhwMemHandle* handle){
			if(handle) {
				handle->acquire();
				mList.addElement(handle,0);
				}
			};
	private:
		GhwMemHandleList mList;
};
typedef List<Job*> JobList;
typedef Node<Job*> JobNode;


class GhwComposerV3d : public GhwComposer {
public:

    GhwComposerV3d();
    virtual ~GhwComposerV3d();
    ghw_error_e init();
    virtual ghw_error_e     setName(const char *name);

    virtual ghw_error_e     barrier(void);

    virtual ghw_error_e     compSetFb(GhwImgBuf* fb_img, u32 dither_flag);
    virtual ghw_error_e     compDrawRect(GhwImgBuf* src_img, GhwImgOp* op);
    virtual ghw_error_e     compCommit(u32 sync_flag);

    virtual ghw_error_e     imgProcess(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, u32 sync_flag);

    virtual ghw_error_e     dump(u32 level);

private:

    virtual ghw_error_e     postJob(GhwMemHandle* bin, u32 bsize, GhwMemHandle* rend, u32 rsize, Job* job);
    virtual ghw_error_e     waitJobCompletion();
    virtual ghw_error_e     cacheFlush();

    virtual ghw_error_e     createBinList(GhwImgBuf* dst_img, GhwMemHandle*& bin, GhwMemHandle*& ta, GhwMemHandle*& ts, u32& bsize);
    virtual ghw_error_e     appendRBSwapShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendFbShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendYuvShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendYuvTileShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendYUV444ShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendYscaleShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendYtileShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendRgb2YuvShaderRec(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     appendDitherShaderRec(GhwImgBuf* dst_img, GhwMemHandle* bin_list_handle, u32& bin_size, Job* job);
    virtual ghw_error_e     closeBinList(GhwMemHandle* bin_list_handle, u32& bin_size);
    virtual ghw_error_e     createRendList(GhwImgBuf* src_img, GhwImgBuf* dst_img,GhwMemHandle*& rend_list_handle, GhwMemHandle* tile_alloc_mem, u32& rend_size);
    virtual ghw_error_e     isImgProcessValid(GhwImgBuf* src_img, GhwImgBuf* dst_img, GhwImgOp* op);

    int                 fdV3d;
    GhwMemAllocator     *shaderAlloc;

    GhwMemHandle        *v3dShaders[NUM_SHADERS];

    GhwMemHandle        *binMem;
    u32                 binMemUsed;
    GhwMemHandle        *rendMem;
    u32                 rendMemUsed;
	Job*				mJobFB;

    int                 composeReqs;
    GhwImgBuf           *fbImg;
    u32                 ditherFlag;

	JobList mList;

    char                mName[32];
    pthread_mutex_t     mLock;

};

};

#endif /* _GHW_COMPOSER_IMPL_H_ */

