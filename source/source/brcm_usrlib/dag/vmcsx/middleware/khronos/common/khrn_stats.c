/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_stats.h"
#include <stdio.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

#ifndef KHRN_STATS_ENABLE
void khrn_stats_reset(void) {}
void khrn_stats_get_human_readable(char *buffer, uint32_t len, bool reset)
{
   snprintf(buffer, len, "Stats recording is disabled\n");
}
#else

KHRN_STATS_RECORD_T khrn_stats_global_record;

void khrn_stats_reset(void)
{
   int i;
   khrn_stats_global_record.reset_time = vcos_getmicrosecs();
   for (i = 0; i < KHRN_STATS_THING_MAX; i++)
   {
      khrn_stats_global_record.in_thing[i] = false;
      khrn_stats_global_record.thing_time[i] = 0;
   }
   for (i = 0; i < KHRN_STATS_EVENT_MAX; i++)
   {
      khrn_stats_global_record.event_count[i] = 0;
   }
}

void khrn_stats_get_human_readable(char *buffer, uint32_t len, bool reset)
{
   uint32_t t = vcos_getmicrosecs();
   int i;
   float elapsed = (float)(t - khrn_stats_global_record.reset_time) / 1000000.0f;
   float scale = 100.0f / elapsed;
   int displayed = khrn_stats_global_record.event_count[KHRN_STATS_SWAP_BUFFERS];
   float gl = (float)khrn_stats_global_record.thing_time[KHRN_STATS_GL] / 1000000.0f;
   float vg = (float)khrn_stats_global_record.thing_time[KHRN_STATS_VG] / 1000000.0f;
   float rdr = (float)khrn_stats_global_record.thing_time[KHRN_STATS_HW_RENDER] / 1000000.0f;
   float bin = (float)khrn_stats_global_record.thing_time[KHRN_STATS_HW_BIN] / 1000000.0f;
   float wait = (float)khrn_stats_global_record.thing_time[KHRN_STATS_WAIT] / 1000000.0f;
   float img = (float)khrn_stats_global_record.thing_time[KHRN_STATS_IMAGE] / 1000000.0f;
   float comp = (float)khrn_stats_global_record.thing_time[KHRN_STATS_COMPILE] / 1000000.0f;

   for (i = 0; i < KHRN_STATS_THING_MAX; i++)
      vcos_assert(!khrn_stats_global_record.in_thing[i]);

   snprintf(buffer, len,
      "Time:%5.2fs Displayed:%d FPS:%3.2f\n"
      "-----\n"
      "Time in driver:%5.2fs (%2.2f%%)\n"
      "  GL: %5.2fs (%2.2f%%)\n"
      "  VG: %5.2fs (%2.2f%%)\n"
      "\n"
      "Time waiting:%5.2fs (%2.2f%%)\n"
      "\n"
      "Time in image manipulation:%5.2fs (%2.2f%%)\n"
      "Unaccelerated image manipulation: %d\n"
      "Time compiling shaders:%5.2fs\n"
      "\n"
      "HW render time:%5.2fs (%2.2f%%)\n"
      "HW bin time:   %5.2fs (%2.2f%%)\n"
      "-----\n"
      "Flushes: %d\n"
      "Round trips: %d\n"
      "Read immediate: %d. Write immediate: %d\n"
      "Framebuffer load/stores: %d\n"
      "                  MS   Non-MS\n"
      "    Color read:  %4d  %4d\n"
      "    Color write: %4d\n"
      "    Depth read:  %4d  %4d\n"
      "    Depth write: %4d  %4d\n"
      "-----%s\n",
      elapsed, displayed, (float)displayed / elapsed,
      gl + vg, scale * (gl + vg),
      gl, scale * gl,
      vg, scale * vg,
      wait, scale * wait,
      img, scale * img,
      khrn_stats_global_record.event_count[KHRN_STATS_UNACCELERATED_IMAGE],
      comp,
      rdr, scale * rdr,
      bin, scale * bin,
      khrn_stats_global_record.event_count[KHRN_STATS_INTERNAL_FLUSH],
      khrn_stats_global_record.event_count[KHRN_STATS_ROUND_TRIP],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_IMMEDIATE],
      khrn_stats_global_record.event_count[KHRN_STATS_WRITE_IMMEDIATE],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_COLOR_MS] +
         khrn_stats_global_record.event_count[KHRN_STATS_READ_COLOR] +
         khrn_stats_global_record.event_count[KHRN_STATS_WRITE_COLOR_MS] +
         khrn_stats_global_record.event_count[KHRN_STATS_READ_DEPTH_MS] +
         khrn_stats_global_record.event_count[KHRN_STATS_READ_DEPTH] +
         khrn_stats_global_record.event_count[KHRN_STATS_WRITE_DEPTH_MS] +
         khrn_stats_global_record.event_count[KHRN_STATS_WRITE_DEPTH],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_COLOR_MS],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_COLOR],
      khrn_stats_global_record.event_count[KHRN_STATS_WRITE_COLOR_MS],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_DEPTH_MS],
      khrn_stats_global_record.event_count[KHRN_STATS_READ_DEPTH],
      khrn_stats_global_record.event_count[KHRN_STATS_WRITE_DEPTH_MS],
      khrn_stats_global_record.event_count[KHRN_STATS_WRITE_DEPTH],
      reset ? "[reset]" : "");


   if (reset)
   {
      khrn_stats_reset();
      khrn_stats_global_record.reset_time = t;
   }
}

#endif