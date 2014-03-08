/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_INT_COMMON_H
#define KHRN_INT_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif\

//#define KHRN_NOT_REALLY_DUALCORE   // Use dual core codebase but switch master thread to vpu1
//#define KHRN_SIMPLE_MULTISAMPLE
//#define USE_CTRL_FOR_DATA

//#define GLXX_FORCE_MULTISAMPLE

//#define KHRN_COMMAND_MODE_DISPLAY    /* Platforms where we need to submit updates even for single-buffered surfaces */

#ifdef RPC_DIRECT
#define KHDISPATCH_WORKSPACE_SIZE 0x200000
#else
#ifdef ANDROID
#define KHDISPATCH_WORKSPACE_SIZE (1024 * 32) /* should be a multiple of 16; to match shared memory size set in ashmem service */
#else // ANDROID
#ifndef __linux__
#define KHDISPATCH_WORKSPACE_SIZE (1024 * 1024) /* should be a multiple of 16, todo: how big does this need to be? (vg needs 8kB) */
#else
//to match shared memory buffer size defined in myshared.h
#define KHDISPATCH_WORKSPACE_SIZE (1024 * 32) /* should be a multiple of 16, todo: how big does this need to be? (vg needs 8kB) */
#endif
#endif // ANDROID
#endif

#ifdef __BCM2707B0__
#define WORKAROUND_HW1394 /* v8ld/v8st with an unaligned address and certain condition codes sometimes retrieves/stores bad data */
#endif

#if !defined(__VIDEOCORE4__) || defined(__BCM2708A0__)
#define WORKAROUND_HW1297 /* changing accumulators after vlookup/vindexwrite */
#endif

#ifdef __BCM2708A0__
#define WORKAROUND_HW1451 /* vfexp2 does not work between -128 and -127 */
#define WORKAROUND_HW1632 /* Potential scoreboard lockup if two shaders on the same QPU finish out of order */
#define WORKAROUND_HW1637 /* Scoreboard receives unlock signal after next 'lock' signal due to QPU pipeline latency */
#define WORKAROUND_HW2038 /* Texture cache tags can interfere even when textures don't overlap */
#define WORKAROUND_HW2136 /* Framebuffer address calculation fails for y >= 1024 */
#define WORKAROUND_HW2187 /* stuff will get out of sync if all primitives between 2 shader changes are degenerate */
#define WORKAROUND_HW2366 /* really small primitives can cause shader records to get out of sync during rendering? */
#define WORKAROUND_HW2384 /* Cant access TU instruction prior to a thrsw */
#define WORKAROUND_HW2422 /* Clearing caches while texturing causes lockups/asserts/other bad stuff */
#define WORKAROUND_HW2479 /* Write to vr_setup/vw_setup always looks at add result to decide type */
#define WORKAROUND_HW2487 /* Reading varyings after thrsw produces speckles */
#define WORKAROUND_HW2488 /* Reading and writing tile buffer in the same instruction causes speckles */
#define WORKAROUND_HW2522 /* TMU read conflicts with VPM read */
#define WORKAROUND_HW2781 /* lockup with interleaved vg and nv/gl shaders */
#define KHRN_HW_SINGLE_TEXTURE_UNIT /* Only single texture unit available */
#else
#if defined(_ATHENA_)
#define WORKAROUND_HW2422 /* Clearing caches while texturing causes lockups/asserts/other bad stuff */
#endif
#if defined(__HERA_V3D__)
#define KHRN_HW_SINGLE_TEXTURE_UNIT /* Only single texture unit available */
#endif
#endif

#ifdef __VIDEOCORE4__
#define WORKAROUND_HW2116 /* let ptb state counters wrap around safely */
#define WORKAROUND_HW2806 /* Flags get written back to wrong quad when z test result collides with new thread */
#define WORKAROUND_HW2885 /* lockup due to erroneous activity in varyings interpolation module when outputting to the coverage pipe */
#define WORKAROUND_HW2903 /* Zero-size points break PTB */
#define WORKAROUND_HW2905 /* Multisample full depth load breaks early-z */
#define WORKAROUND_HW2924 /* Fwd/rev stencil speckles */
#endif

#if defined(RPC_DIRECT) && !defined(__VIDEOCORE4__)
#define GLXX_NO_VERTEX_CACHE
#endif

#ifndef NULL
# ifdef __cplusplus
#  define NULL 0
# else
#  define NULL ((void *)0)
# endif
#endif

#include "interface/vcos/vcos_assert.h"
#include <string.h> /* size_t */

#ifdef _MSC_VER
#define INLINE __inline
typedef unsigned long long uint64_t;
#else
   #ifdef __CC_ARM
#define INLINE __inline
   #else
#define INLINE inline
   #endif
#endif

#ifndef __cplusplus
   typedef uint8_t bool;
   #define false 0
   #define true 1
#endif

#ifdef NDEBUG
   #define verify(X) X
#else
   #define verify(X) vcos_assert(X)
#endif
#define UNREACHABLE() /* coverity[dead_error_begin] */ vcos_assert(0)

#ifdef _MSC_VER
   #define UNUSED(X) X
#else
   #define UNUSED(X)
#endif

#ifdef NDEBUG
   #define UNUSED_NDEBUG(X) UNUSED(X)
#else
   #define UNUSED_NDEBUG(X)
#endif

#define KHRN_NO_SEMAPHORE 0xffffffff

#ifdef __cplusplus
 }
#endif

#endif

