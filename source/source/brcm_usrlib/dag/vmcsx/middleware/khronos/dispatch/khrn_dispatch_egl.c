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

#include "middleware/khronos/gl20/gl20_server.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"

void egl_dispatch(uint32_t id)
{
   switch (id) {
   case EGLINTCREATESURFACE_ID:
   {
      uint32_t win = khdispatch_read_uint();
      uint32_t buffers = khdispatch_read_uint();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      uint32_t color = khdispatch_read_uint();
      uint32_t depth = khdispatch_read_uint();
      uint32_t mask = khdispatch_read_uint();
      uint32_t multi = khdispatch_read_uint();
      uint32_t largest = khdispatch_read_uint();
      uint32_t miptex = khdispatch_read_uint();
      uint32_t sem = khdispatch_read_uint();

      uint32_t results[3];

      int count = eglIntCreateSurface_impl(win, buffers, width, height, color, depth, mask, multi, largest, miptex, sem, results);

      vcos_assert(count == 3);

      khdispatch_write_out_ctrl(results, count * sizeof(uint32_t));
      break;
   }
   case EGLINTCREATEGLES11_ID:
   {
      uint32_t share = khdispatch_read_uint();
      uint32_t type = khdispatch_read_uint();

      EGL_GL_CONTEXT_ID_T id = eglIntCreateGLES11_impl(share, type);

      khdispatch_write_glcontextid(id);
      break;
   }
   case EGLINTCREATEGLES20_ID:
   {
      uint32_t share = khdispatch_read_uint();
      uint32_t type = khdispatch_read_uint();

      EGL_GL_CONTEXT_ID_T id = eglIntCreateGLES20_impl(share, type);

      khdispatch_write_glcontextid(id);
      break;
   }
   case EGLINTCREATEVG_ID:
   {
      uint32_t share = khdispatch_read_uint();
      uint32_t type = khdispatch_read_uint();

      EGL_GL_CONTEXT_ID_T id = eglIntCreateVG_impl(share, type);

      khdispatch_write_vgcontextid(id);
      break;
   }
   case EGLINTDESTROYSURFACE_ID:
   {
      uint32_t buffer = khdispatch_read_uint();

      khdispatch_write_int(eglIntDestroySurface_impl(buffer));
      break;
   }
   case EGLINTDESTROYGL_ID:
   {
      uint32_t context = khdispatch_read_uint();

      eglIntDestroyGL_impl(context);
      break;
   }
   case EGLINTDESTROYVG_ID:
   {
      uint32_t context = khdispatch_read_uint();

      eglIntDestroyVG_impl(context);
      break;
   }
   case EGLINTMAKECURRENT_ID:
   {
      uint32_t pid_0 = khdispatch_read_uint();
      uint32_t pid_1 = khdispatch_read_uint();
      uint32_t gltype = khdispatch_read_uint();
      uint32_t servergl = khdispatch_read_uint();
      uint32_t servergldraw = khdispatch_read_uint();
      uint32_t serverglread = khdispatch_read_uint();
      uint32_t servervg = khdispatch_read_uint();
      uint32_t servervgsurf = khdispatch_read_uint();

      eglIntMakeCurrent_impl(pid_0, pid_1, gltype, servergl, servergldraw, serverglread, servervg, servervgsurf);
      break;
   }
   case EGLINTFLUSHANDWAIT_ID:
   {
      uint32_t gl = khdispatch_read_uint();
      uint32_t vg = khdispatch_read_uint();

      int res = eglIntFlushAndWait_impl(gl, vg);

      khdispatch_write_int(res);
      break;
   }
   case EGLINTSWAPBUFFERS_ID:
   {
      uint32_t buffer = khdispatch_read_uint();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      uint32_t handle = khdispatch_read_uint();
      uint32_t preserve = khdispatch_read_uint();
      uint32_t position = khdispatch_read_uint();

      eglIntSwapBuffers_impl(buffer, width, height, handle, preserve, position);
      break;
   }
   case EGLINTFLUSH_ID:
   {
      uint32_t gl = khdispatch_read_uint();
      uint32_t vg = khdispatch_read_uint();

      eglIntFlush_impl(gl, vg);
      break;
   }
   case EGLINTGETCOLORDATA_ID:
   {
      uint32_t s = khdispatch_read_uint();
      uint32_t format = khdispatch_read_uint();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      int stride = khdispatch_read_int();
      uint32_t y_offset = khdispatch_read_uint();

      int len = height * ((stride < 0) ? -stride : stride);

      khdispatch_check_workspace(khdispatch_pad_bulk(len));

      eglIntGetColorData_impl(s, format, width, height, stride, y_offset, khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, len);
      break;
   }
   case EGLINTSETCOLORDATA_ID:
   {
      uint32_t s = khdispatch_read_uint();
      uint32_t format = khdispatch_read_uint();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      int stride = khdispatch_read_int();
      uint32_t y_offset = khdispatch_read_uint();

      uint32_t inlen = khdispatch_read_uint();

      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         eglIntSetColorData_impl(s, format, width, height, stride, y_offset, indata);
      break;
   }
   case EGLINTBINDTEXIMAGE_ID:
   {
      uint32_t surface = khdispatch_read_uint();

      bool result = eglIntBindTexImage_impl(surface);

      khdispatch_write_boolean(result);
      break;
   }
   case EGLINTRELEASETEXIMAGE_ID:
   {
      uint32_t surface = khdispatch_read_uint();

      eglIntReleaseTexImage_impl(surface);
      break;
   }
   case EGLINTCREATEPBUFFERFROMVGIMAGE_ID:
   {
      VGHandle vg_handle = khdispatch_read_handle();
      uint32_t color = khdispatch_read_uint();
      uint32_t depth = khdispatch_read_uint();
      uint32_t mask = khdispatch_read_uint();
      uint32_t multi = khdispatch_read_uint();
      uint32_t miptex = khdispatch_read_uint();

      uint32_t results[5];

      int count = eglIntCreatePbufferFromVGImage_impl(vg_handle, color, depth, mask, multi, miptex, results);

      vcos_assert(count == 5);

      khdispatch_write_out_ctrl(results, count * sizeof(uint32_t));
      break;
   }
   case EGLINTCREATEWRAPPEDSURFACE_ID:
   {
      uint32_t handle_0 = khdispatch_read_uint();
      uint32_t handle_1 = khdispatch_read_uint();
      uint32_t depth = khdispatch_read_uint();
      uint32_t mask = khdispatch_read_uint();
      uint32_t multi = khdispatch_read_uint();

      EGL_SURFACE_ID_T id = eglIntCreateWrappedSurface_impl(handle_0, handle_1, depth, mask, multi);

      khdispatch_write_surfaceid(id);
      break;
   }
   case EGLCREATEIMAGEKHR_ID: /* RPC_CALL6_OUT_CTRL (EGL_KHR_image) */
   {
      uint32_t glversion = khdispatch_read_uint();
      EGL_CONTEXT_ID_T ctx = khdispatch_read_uint();
      EGLenum target = khdispatch_read_enum();
	  uint32_t bufferPhys = khdispatch_read_uint();
      uint32_t buffer = khdispatch_read_uint();
      KHRN_IMAGE_FORMAT_T buffer_format = khdispatch_read_uint();
      uint32_t buffer_width = khdispatch_read_uint();
      uint32_t buffer_height = khdispatch_read_uint();
      uint32_t buffer_stride = khdispatch_read_uint();
      EGLint texture_level = khdispatch_read_int();
      EGLint results[2];

      int count = eglCreateImageKHR_impl(
         glversion,
         ctx,
         target,
         bufferPhys,
         buffer,
         buffer_format,
         buffer_width,
         buffer_height,
         buffer_stride,
         texture_level,
         results);

      vcos_assert(count == 2);

      khdispatch_write_out_ctrl(results, count * sizeof(uint32_t));
      break;
   }
   case EGLDESTROYIMAGEKHR_ID: /* RPC_CALL1_RES (EGL_KHR_image) */
   {
      EGLImageKHR image = khdispatch_read_eglhandle();

      EGLBoolean result = eglDestroyImageKHR_impl(image);

      khdispatch_write_boolean(result);
      break;
   }
   case EGLINTOPENMAXILDONEMARKER_ID:
   {
      void* component_handle = khdispatch_read_eglhandle();
      EGLImageKHR egl_image = khdispatch_read_eglhandle();

      int res = eglIntOpenMAXILDoneMarker_impl(component_handle, egl_image);

      khdispatch_write_int(res);
      break;
   }
   case EGLINTSWAPINTERVAL_ID:
   {
      uint32_t surface = khdispatch_read_uint();
      uint32_t swap_interval = khdispatch_read_uint();

      eglIntSwapInterval_impl(surface, swap_interval);
      break;
   }
   case EGLINTGETPROCESSMEMUSAGE_ID:
   {
      uint32_t size = khdispatch_read_uint();

      khdispatch_check_workspace(khdispatch_pad_bulk(size * sizeof(uint32_t)));

      eglIntGetProcessMemUsage_impl(size, (uint32_t *)khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size * sizeof(uint32_t));
      break;
   }
   case EGLINTGETGLOBALMEMUSAGE_ID:
   {
      uint32_t result[2];

      eglIntGetGlobalMemUsage_impl(result);

      khdispatch_write_out_ctrl(result, sizeof(result));
      break;
   }
   case EGLINTDESTROYBYPID_ID:
   {
      uint32_t pid_0 = khdispatch_read_uint();
      uint32_t pid_1 = khdispatch_read_uint();

      eglIntDestroyByPid_impl(pid_0, pid_1);
      break;
   }
#if EGL_BRCM_global_image
   case EGLCREATEGLOBALIMAGEBRCM_ID: /* RPC_CALL4_OUT_CTRL (EGL_BRCM_global_image) */
   {
      EGLint width = khdispatch_read_int();
      EGLint height = khdispatch_read_int();
      EGLint pixel_format = khdispatch_read_int();
      EGLint id[2];
      eglCreateGlobalImageBRCM_impl(width, height, pixel_format, id);
      khdispatch_write_out_ctrl(id, sizeof(id));
      break;
   }
   case EGLFILLGLOBALIMAGEBRCM_ID: /* custom (EGL_BRCM_global_image) */
   {
      EGLint id_0 = khdispatch_read_int();
      EGLint id_1 = khdispatch_read_int();
      EGLint y = khdispatch_read_int();
      EGLint height = khdispatch_read_int();
      EGLint line_size = khdispatch_read_int();
      EGLint data_pixel_format = khdispatch_read_int();
      khdispatch_check_workspace(khdispatch_pad_bulk(height * line_size));
      void *data = khdispatch_workspace;
      if(khdispatch_recv_bulk(data, height * line_size))
         eglFillGlobalImageBRCM_impl(id_0, id_1, y, height, data, line_size, data_pixel_format);
      break;
   }
   case EGLCREATECOPYGLOBALIMAGEBRCM_ID: /* RPC_CALL3_OUT_CTRL (EGL_BRCM_global_image) */
   {
      EGLint src_id_0 = khdispatch_read_int();
      EGLint src_id_1 = khdispatch_read_int();
      EGLint id[2];
      eglCreateCopyGlobalImageBRCM_impl(src_id_0, src_id_1, id);
      khdispatch_write_out_ctrl(id, sizeof(id));
      break;
   }
   case EGLDESTROYGLOBALIMAGEBRCM_ID: /* RPC_CALL2_RES (EGL_BRCM_global_image) */
   {
      EGLint id_0 = khdispatch_read_int();
      EGLint id_1 = khdispatch_read_int();
      khdispatch_write_boolean(eglDestroyGlobalImageBRCM_impl(id_0, id_1));
      break;
   }
   case EGLQUERYGLOBALIMAGEBRCM_ID: /* RPC_CALL3_OUT_CTRL_RES (EGL_BRCM_global_image) */
   {
      EGLint id_0 = khdispatch_read_int();
      EGLint id_1 = khdispatch_read_int();
      EGLint width_height_pixel_format[3];
      bool success = eglQueryGlobalImageBRCM_impl(id_0, id_1, width_height_pixel_format);
      khdispatch_write_out_ctrl_res(width_height_pixel_format, success ? sizeof(width_height_pixel_format) : 0, success);
      break;
   }
#endif
#if EGL_KHR_sync
   case EGLINTCREATESYNC_ID:
   {
      uint32_t condition = khdispatch_read_uint();
      int32_t threshold = khdispatch_read_int();
      uint32_t sem = khdispatch_read_uint();

      EGL_SYNC_ID_T id = eglIntCreateSync_impl(condition, threshold, sem);

      khdispatch_write_syncid(id);
      break;
   }
   case EGLINTDESTROYSYNC_ID:
   {
      uint32_t sync = khdispatch_read_uint();

      eglIntDestroySync_impl(sync);
      break;
   }
#endif
#if EGL_BRCM_perf_monitor
   case EGLINITPERFMONITORBRCM_ID:
   {
      bool result = eglInitPerfMonitorBRCM_impl();

      khdispatch_write_boolean(result);
      break;
   }
   case EGLTERMPERFMONITORBRCM_ID:
   {
      eglTermPerfMonitorBRCM_impl();

      break;
   }
#endif
   case EGLIMAGESETCOLORDATA_ID:
   {
      EGLImageKHR image = khdispatch_read_eglhandle();
      uint32_t format = khdispatch_read_uint();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      int stride = khdispatch_read_int();
      uint32_t y_offset = khdispatch_read_uint();

      uint32_t inlen = khdispatch_read_uint();

      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         eglImageSetColorData_impl(image, format, width, height, stride, y_offset, indata);
      break;
   }
#if EGL_BRCM_perf_stats
   case EGLPERFSTATSRESETBRCM_ID:
   {
      eglPerfStatsResetBRCM_impl();

      break;
   }
   case EGLPERFSTATSGETBRCM_ID:
   {
      int size = khdispatch_read_int();
      EGLBoolean reset = khdispatch_read_boolean();

      khdispatch_check_workspace(khdispatch_pad_bulk(size));
      eglPerfStatsGetBRCM_impl(size, reset, khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
#endif
#if EGL_BRCM_driver_monitor
   case EGLINITDRIVERMONITORBRCM_ID:
   {
      int hw_bank = khdispatch_read_int();
      int l3_bank = khdispatch_read_int();

      bool result = eglInitDriverMonitorBRCM_impl(hw_bank, l3_bank);

      khdispatch_write_boolean(result);
      break;
   }
   case EGLTERMDRIVERMONITORBRCM_ID:
   {
      eglTermDriverMonitorBRCM_impl();
      break;
   }
   case EGLGETDRIVERMONITORXMLBRCM_ID:
   {
      int size = khdispatch_read_int();

      khdispatch_check_workspace(khdispatch_pad_bulk(size));
      eglGetDriverMonitorXMLBRCM_impl(size, khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
#endif
#ifdef DIRECT_RENDERING
   case EGLDIRECTRENDERINGPOINTER_ID:
   {
      uint32_t surface = khdispatch_read_uint();
#ifdef BRCM_V3D_OPT
      uint32_t aux = khdispatch_read_uint();
#endif
      uint32_t buffer = khdispatch_read_uint();
      KHRN_IMAGE_FORMAT_T buffer_format = khdispatch_read_uint();
      uint32_t buffer_width = khdispatch_read_uint();
      uint32_t buffer_height = khdispatch_read_uint();
      uint32_t buffer_stride = khdispatch_read_uint();

#ifdef BRCM_V3D_OPT
      eglDirectRenderingPointer_impl(surface, aux, buffer, buffer_format, buffer_width, buffer_height, buffer_stride);
#else
      eglDirectRenderingPointer_impl(surface, buffer, buffer_format, buffer_width, buffer_height, buffer_stride);
#endif
      break;
   }
#endif
   default:
      UNREACHABLE();
      break;
   }
}
