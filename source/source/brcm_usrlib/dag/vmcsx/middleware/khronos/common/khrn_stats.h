/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_STATS_H
#define KHRN_STATS_H

#include "interface/vcos/vcos.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext_brcm.h"
#include "interface/khronos/common/khrn_int_common.h"

#if EGL_BRCM_perf_stats
#define KHRN_STATS_ENABLE
#endif

#define KHRN_STATS_GL 1
#define KHRN_STATS_VG 2
#define KHRN_STATS_IMAGE 3
#define KHRN_STATS_WAIT 4
#define KHRN_STATS_HW_RENDER 5
#define KHRN_STATS_HW_BIN 6
#define KHRN_STATS_COMPILE 7
#define KHRN_STATS_THING_MAX 8

#define KHRN_STATS_SWAP_BUFFERS 1
#define KHRN_STATS_ROUND_TRIP 2
#define KHRN_STATS_INTERNAL_FLUSH 3
#define KHRN_STATS_READ_DEPTH 4
#define KHRN_STATS_READ_DEPTH_MS 5
#define KHRN_STATS_READ_COLOR 6
#define KHRN_STATS_READ_COLOR_MS 7
#define KHRN_STATS_WRITE_DEPTH 8
#define KHRN_STATS_WRITE_DEPTH_MS 9
#define KHRN_STATS_WRITE_COLOR_MS 10
#define KHRN_STATS_UNACCELERATED_IMAGE 11
#define KHRN_STATS_READ_IMMEDIATE 12
#define KHRN_STATS_WRITE_IMMEDIATE 13
#define KHRN_STATS_EVENT_MAX 14

#ifdef KHRN_STATS_ENABLE

#ifdef __VIDEOCORE__
#include "vcinclude/hardware.h"
#else
#include <time.h>
#endif

typedef struct
{
   uint32_t reset_time;
   bool in_thing[KHRN_STATS_THING_MAX];
   uint32_t thing_time[KHRN_STATS_THING_MAX];
   uint32_t event_count[KHRN_STATS_EVENT_MAX];
} KHRN_STATS_RECORD_T;

extern KHRN_STATS_RECORD_T khrn_stats_global_record;

/* Main interface */
extern void khrn_stats_reset(void);
extern void khrn_stats_get_human_readable(char *buffer, uint32_t len, bool reset);

/* Hooks */

static INLINE void khrn_stats_record_start(uint32_t thing)
{
   vcos_assert(thing < KHRN_STATS_THING_MAX);
   vcos_assert(!khrn_stats_global_record.in_thing[thing]);
   khrn_stats_global_record.in_thing[thing] = true;
   khrn_stats_global_record.thing_time[thing] -= vcos_getmicrosecs();
}

static INLINE void khrn_stats_record_end(uint32_t thing)
{
   vcos_assert(thing < KHRN_STATS_THING_MAX);
   vcos_assert(khrn_stats_global_record.in_thing[thing]);
   khrn_stats_global_record.in_thing[thing] = false;
   khrn_stats_global_record.thing_time[thing] += vcos_getmicrosecs();
}

static INLINE void khrn_stats_record_add(uint32_t thing, uint32_t microsecs)
{
   vcos_assert(!khrn_stats_global_record.in_thing[thing]);
   khrn_stats_global_record.thing_time[thing] += microsecs;
}

static INLINE void khrn_stats_record_event(uint32_t event)
{
   vcos_assert(event <= KHRN_STATS_EVENT_MAX);
   khrn_stats_global_record.event_count[event]++;
}

#else
/* Stats recording is disabled */
static INLINE void khrn_stats_record_start(uint32_t thing) {}
static INLINE void khrn_stats_record_end(uint32_t thing) {}
static INLINE void khrn_stats_record_add(uint32_t thing, uint32_t microsecs) {}
static INLINE void khrn_stats_record_event(uint32_t event) {}
#endif

#endif
