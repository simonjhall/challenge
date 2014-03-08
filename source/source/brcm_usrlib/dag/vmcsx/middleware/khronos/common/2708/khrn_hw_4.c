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
#include "middleware/khronos/common/2708/khrn_copy_buffer_4.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/common/2708/khrn_render_state_4.h"
#include "middleware/khronos/common/2708/khrn_worker_4.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/egl/egl_disp.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h" /* for glxx_hw_init, glxx_hw_term */
#include "middleware/khronos/vg/vg_server.h" /* for vg_maybe_term */

#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
#include "middleware/powerman/powerman.h"
#include "helpers/vclib/vclib.h" /* for vclib_memcpy/vclib_memset */
#include "vcfw/vclib/vclib.h" /* for vclib_obtain_VRF/vclib_release_VRF */
#if EGL_BRCM_perf_monitor

#include "vcfw/drivers/chip/v3d.h"
#endif
//#include "utils/Log.h"

#define MAXIMUM_CLOCK 250000000
#define BUMP_CLOCK     20000000
static struct
{
   bool have_powerman;
   POWERMAN_HANDLE_T handle;
} khrn_power;
#endif

#ifndef SIMPENROSE
static VCOS_EVENT_T *master_event;
#endif

static KHRN_DRIVER_COUNTERS_T driver_counters;

static void nmem_callback(void * a)
{
   khrn_sync_notify_master();
#ifndef SIMPENROSE
   khrn_hw_notify_llat();
#endif
}

/* order of init/term calls is important */

bool khrn_hw_common_init(void)
{
   if (!khrn_hw_init()) {
	   assert(0);
      return false;
   }
#ifndef BRCM_V3D_OPT
   if (!khrn_worker_init()) {
      khrn_hw_term();
      return false;
   }
#endif
   if (!khrn_nmem_init()) {
      khrn_worker_term();
      khrn_hw_term();
      return false;
   }
   khrn_nmem_register(nmem_callback, NULL);
   khrn_render_state_init();
   glxx_2708_hw_init();

#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   khrn_power.have_powerman = vcos_verify(powerman_register_user(&khrn_power.handle) == 0);
#endif

   return true;
}

void khrn_hw_common_term(void)
{
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)

   if (khrn_power.have_powerman)
      vcos_verify(powerman_deregister_user(khrn_power.handle) == 0);
#endif

   glxx_2708_hw_term();
   khrn_render_state_term();
   khrn_nmem_unregister(nmem_callback, NULL);
   khrn_nmem_term();
#ifndef BRCM_V3D_OPT
   khrn_worker_term();
#endif
   khrn_hw_term();
}

void khrn_hw_common_flush(void)
{
   khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_NONE);
}

void khrn_hw_common_wait(void)
{
   khrn_hw_wait();
   khrn_worker_wait();
}

typedef struct {
   EGL_DISP_SLOT_HANDLE_T slot_handle;
} MSG_DISPLAY_T;

static KHRN_WORKER_CALLBACK_RESULT_T display_callback(
   KHRN_WORKER_CALLBACK_REASON_T reason,
   void *message, uint32_t size)
{
   MSG_DISPLAY_T *msg = (MSG_DISPLAY_T *)message;
   UNUSED_NDEBUG(size);
   vcos_assert(size == sizeof(MSG_DISPLAY_T));

   switch (reason) {
   case KHRN_WORKER_CALLBACK_REASON_DO_IT:
   {
      egl_disp_ready(msg->slot_handle, false);
      return KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK;
   }
   case KHRN_WORKER_CALLBACK_REASON_CLEANUP:
   {
      return KHRN_WORKER_CALLBACK_RESULT_CLEANUP;
   }
   default:
   {
      UNREACHABLE();
      return (KHRN_WORKER_CALLBACK_RESULT_T)0;
   }
   }
}

void khrn_delayed_display(KHRN_IMAGE_T *image, EGL_DISP_SLOT_HANDLE_T slot_handle)
{
   switch (image ? khrn_interlock_get_write_fifo(&image->interlock) : KHRN_INTERLOCK_FIFO_NONE) {
   case KHRN_INTERLOCK_FIFO_NONE:
   {
      /* no outstanding writes. display now! */
      egl_disp_ready(slot_handle, false);
      break;
   }
   case KHRN_INTERLOCK_FIFO_HW_RENDER:
   {
      /* latest outstanding write is in the hw fifo. post the display message there too */
      khrn_hw_queue_display(slot_handle);
      break;
   }
   case KHRN_INTERLOCK_FIFO_WORKER:
   {
      /* latest outstanding write is in the worker fifo. post the display message there too */
      MSG_DISPLAY_T *msg = (MSG_DISPLAY_T *)khrn_worker_post_begin(display_callback, sizeof(MSG_DISPLAY_T));
      msg->slot_handle = slot_handle;
      khrn_worker_post_end();
      break;
   }
   default: UNREACHABLE();
   }
}

typedef struct {
   EGL_DISP_HANDLE_T disp_handle;
   uint32_t pos;
} MSG_WAIT_FOR_DISPLAY_T;

static KHRN_WORKER_CALLBACK_RESULT_T wait_for_display_callback(
   KHRN_WORKER_CALLBACK_REASON_T reason,
   void *message, uint32_t size)
{
   MSG_WAIT_FOR_DISPLAY_T *msg = (MSG_WAIT_FOR_DISPLAY_T *)message;
   UNUSED_NDEBUG(size);
   vcos_assert(size == sizeof(MSG_WAIT_FOR_DISPLAY_T));

   switch (reason) {
   case KHRN_WORKER_CALLBACK_REASON_DO_IT:
   {
      return egl_disp_still_on(msg->disp_handle, msg->pos) ?
         KHRN_WORKER_CALLBACK_RESULT_DO_IT_WAIT :
         KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK;
   }
   case KHRN_WORKER_CALLBACK_REASON_CLEANUP:
   {
      return KHRN_WORKER_CALLBACK_RESULT_CLEANUP;
   }
   default:
   {
      UNREACHABLE();
      return (KHRN_WORKER_CALLBACK_RESULT_T)0;
   }
   }
}

void khrn_delayed_wait_for_display(uint32_t fifo, EGL_DISP_HANDLE_T disp_handle, uint32_t pos)
{
   switch (fifo) {
   case KHRN_INTERLOCK_FIFO_HW_RENDER:
   {
      khrn_hw_queue_wait_for_display(disp_handle, pos);
      break;
   }
   case KHRN_INTERLOCK_FIFO_WORKER:
   {
      MSG_WAIT_FOR_DISPLAY_T *msg = (MSG_WAIT_FOR_DISPLAY_T *)khrn_worker_post_begin(wait_for_display_callback, sizeof(MSG_WAIT_FOR_DISPLAY_T));
      msg->disp_handle = disp_handle;
      msg->pos = pos;
      khrn_worker_post_end();
      break;
   }
   default: UNREACHABLE();
   }
}

void khrn_delayed_copy_buffer(MEM_HANDLE_T dst_handle, MEM_HANDLE_T src_handle)
{
   khrn_copy_buffer(dst_handle, src_handle);
}

uint32_t khrn_hw_required_texture_blob(bool is_cube, bool depaletted)
{
   UNUSED(is_cube);
   UNUSED(depaletted);

   return KHRN_TEXTURE_REVERSE_MIPMAP_BLOB;
}

bool khrn_hw_supports_early_z(void)
{
   return false;
}

void khrn_sync_notify_master(void)
{
#ifndef SIMPENROSE
   vcos_event_signal(master_event);
#endif
}

void khrn_sync_master_wait(void)
{
   khrn_stats_record_start(KHRN_STATS_WAIT);
#ifndef SIMPENROSE
#ifdef BRCM_V3D_OPT
   khrn_hw_wait();
#else
   vcos_event_wait(master_event);
#endif
#endif
   khrn_stats_record_end(KHRN_STATS_WAIT);
#ifdef KHRN_LLAT_NO_THREAD
   khrn_llat_process();
#endif
#ifndef SIMPENROSE
   khrn_hw_cleanup();
#endif
   khrn_worker_cleanup();
   vg_maybe_term();
}

void khrn_sync_display_returned_notify(void)
{
   khrn_sync_notify_master();
#ifndef SIMPENROSE
   khrn_hw_notify_llat();
#endif
   khrn_worker_notify();
}

void khrn_memcpy(void *dest, const void *src, uint32_t size)
{
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   if (size > 128) { /* todo: what's a good point to switch over? */
      vclib_obtain_VRF(1);
      vclib_memcpy(dest, src, size);
      vclib_release_VRF();
   } else {
#endif
      memcpy(dest, src, size);
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   }
#endif
}

void khrn_memset(void *dest, uint32_t val, uint32_t size)
{
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   if (size > 128) { /* todo: what's a good point to switch over? */
      vclib_obtain_VRF(1);
      vclib_memset(dest, val, size);
      vclib_release_VRF();
   } else {
#endif
      memset(dest, val, size);
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   }
#endif
}

#ifndef SIMPENROSE
void khrn_specify_event(VCOS_EVENT_T *ev)
{
   master_event = ev;
}
#endif

int32_t khrn_do_suspend_resume(uint32_t up)
{
   UNUSED(up);

   /* This function probably isn't needed */
   return 0;
}

void khrn_hw_kick(void)
{
#if !defined(SIMPENROSE) && !defined(V3D_LEAN)
   if (khrn_power.have_powerman)
   {
      uint32_t clk_mhz;
      powerman_get_status(&clk_mhz, NULL);
      powerman_set_user_request(khrn_power.handle, POWERMAN_REQUEST_KICK_CLOCK, _min(MAXIMUM_CLOCK, clk_mhz + BUMP_CLOCK));
   }
#endif
}

#if EGL_BRCM_perf_monitor
void khrn_start_perf_counters()
{
   v3d_write(PCTRS0, 10); /* PTB Primitives discarded by being outside the viewport */
   v3d_write(PCTRS1, 11); /* PTB Primitives that need clipping */
   v3d_write(PCTRS2, 12); /* PSE Primitives that are discarded because they are reversed */
   v3d_write(PCTRS3, 0); /* FEP Valid primitives that result in no rendered pixels, for all rendered tiles */
   v3d_write(PCTRS4, 24); /* TMU Total texture quads processed */
   v3d_write(PCTRS5, 25); /* TMU Total texture cache misses (number of fetches from memory/L2cache) */
   v3d_write(PCTRS6, 5); /* TLB Quads with no pixels passing the Z and stencil tests */
   v3d_write(PCTRS7, 4); /* TLB Quads with no pixels passing the stencil test */
   v3d_write(PCTRS8, 6); /* TLB Quads with any pixels passing the Z and stencil tests*/

   v3d_write(PCTRC, 0x1ff);
   v3d_write(PCTRE, 0x1ff);

}

void khrn_stop_perf_counters()
{
   v3d_write(PCTRE, 0);
}

void khrn_read_perf_counters(KHRN_PERF_COUNTERS_T *counters)
{
   counters->fovcull = v3d_read(PCTR0);
   counters->fovclip = v3d_read(PCTR1);
   counters->revcull = v3d_read(PCTR2);
   counters->nofepix = v3d_read(PCTR3);
   counters->tu0access = v3d_read(PCTR4);
   counters->tu0miss = v3d_read(PCTR5);
   counters->tu1access = 0;
   counters->tu1miss = 0;
   counters->dpthfail = v3d_read(PCTR6);
   counters->stclfail = v3d_read(PCTR7);
   counters->dpthstclpass = v3d_read(PCTR8);
}
#endif

void khrn_reset_driver_counters(int32_t hw_bank, int32_t l3c_bank)
{
   int32_t hw = driver_counters.hw_group_active;
   int32_t l3c = driver_counters.l3c_group_active;

   if (hw_bank >= 0 && hw_bank <= 1)
      hw = hw_bank;
   else if (hw < 0 || hw > 1)
      hw = 0;

   if (l3c_bank >= 0 && l3c_bank <= 1)
      l3c = l3c_bank;
   else if (l3c < 0 || l3c > 1)
      l3c = 0;

   memset(&driver_counters, 0, sizeof(KHRN_DRIVER_COUNTERS_T));

   driver_counters.hw_group_active = hw;
   driver_counters.l3c_group_active = l3c;

   khrn_hw_register_perf_counters(&driver_counters);
}

KHRN_DRIVER_COUNTERS_T *khrn_driver_counters(void)
{
   return &driver_counters;
}
