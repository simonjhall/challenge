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
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/common/khrn_stats.h"

#ifdef __VIDEOCORE__
#include "helpers/dmalib/dmalib.h"
#endif

#if EGL_BRCM_perf_monitor

#include "middleware/khronos/perform/khrn_visual_stats.h"
#include "helpers/vc_graphlib/graphlibrary.h"

static VC_IMAGE_T vc_images[2];
static int next_image;

bool eglInitPerfMonitorBRCM_impl()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   if (state->perf_monitor_refcount == 0) {
      MEM_HANDLE_T images[2];

      int i;

      for (i = 0; i < 2; i++)
         images[i] = khrn_image_create(ARGB_4444_RSO, 800, 480, IMAGE_CREATE_FLAG_NONE);

      if (images[0] && images[1]) {
         for (i = 0; i < 2; i++) {
            khrn_image_lock_vcimage(mem_lock(images[i]), &vc_images[i]);
            mem_unlock(images[i]);

            vcos_assert(!state->perf_monitor_images[i]);

            MEM_ASSIGN(state->perf_monitor_images[i], images[i]);
            mem_release(images[i]);
         }

         next_image = 0;

         khrn_perform_init();
         khrn_start_perf_counters();
      } else {
         for (i = 0; i < 2; i++)
            if (images[i])
               mem_release(images[i]);

         return false;
      }
   }

   state->perf_monitor_refcount++;

   return true;
}

void eglTermPerfMonitorBRCM_impl()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   vcos_assert(state->perf_monitor_refcount > 0);

   state->perf_monitor_refcount--;

   if (state->perf_monitor_refcount == 0) {
      egl_server_platform_perf_monitor_hide();

      int i;

      for (i = 0; i < 2; i++) {
         khrn_image_unlock_vcimage(mem_lock(state->perf_monitor_images[i]));
         mem_unlock(state->perf_monitor_images[i]);

         MEM_ASSIGN(state->perf_monitor_images[i], MEM_INVALID_HANDLE);
      }

      khrn_stop_perf_counters();
   }
}

#define PERF_MONITOR_PERIOD 1000000

void egl_brcm_perf_monitor_update()
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();

   if (state->perf_monitor_refcount) {
      uint32_t time = vcos_getmicrosecs();
      uint32_t delta = time - state->perf_monitor_lasttime;

      if (delta > PERF_MONITOR_PERIOD) {
         KHRN_PERF_COUNTERS_T counters;

         khrn_read_perf_counters(&counters);

         khrn_perform_addPRIMStats(counters.fovcull - state->perf_monitor_counters.fovcull,
                                   counters.fovclip - state->perf_monitor_counters.fovclip,
                                   counters.revcull - state->perf_monitor_counters.revcull,
                                   counters.nofepix - state->perf_monitor_counters.nofepix);

         khrn_perform_addTXTStats(counters.tu0access - state->perf_monitor_counters.tu0access,
                                  counters.tu0miss - state->perf_monitor_counters.tu0miss,
                                  counters.tu1access - state->perf_monitor_counters.tu1access,
                                  counters.tu1miss - state->perf_monitor_counters.tu1miss);

         khrn_perform_addPBEStats(counters.dpthfail - state->perf_monitor_counters.dpthfail,
                                  counters.stclfail - state->perf_monitor_counters.stclfail,
                                  counters.dpthstclpass - state->perf_monitor_counters.dpthstclpass);

         memcpy(&state->perf_monitor_counters, &counters, sizeof(KHRN_PERF_COUNTERS_T));

#ifdef __VIDEOCORE__
         dma_memset(vc_images[next_image].image_data, 0, 800*480*2);
#else
         memset(vc_images[next_image].image_data, 0, 800*480*2);
#endif

         t_Image buf = {800, 480, vc_images[next_image].image_data};

         setFB(&buf);
         khrn_perform_drawStats(NULL, 3);

         egl_server_platform_perf_monitor_show(&vc_images[next_image]);

         next_image ^= 1;

         state->perf_monitor_lasttime = time;
      }
   }
}

#endif

#if EGL_BRCM_perf_stats

void eglPerfStatsResetBRCM_impl()
{
   khrn_stats_reset();
}

void eglPerfStatsGetBRCM_impl(EGLint buffer_len, EGLBoolean reset, char *buffer)
{
   khrn_stats_get_human_readable(buffer, buffer_len, reset);
}

#endif
