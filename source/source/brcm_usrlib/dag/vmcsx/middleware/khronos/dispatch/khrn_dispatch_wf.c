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

#include "middleware/khronos/wf/wf_server.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

void wf_dispatch(uint32_t id)
{
   switch (id) {
   case WFCINTCREATECONTEXT_ID:
   {
      int32_t type = khdispatch_read_int();
      int32_t info = khdispatch_read_int();

      int32_t response = wfcIntCreateContext(type, info);

      khdispatch_write_int(response);
      break;
   }
   case WFCINTDESTROYCONTEXT_ID:
   {
      int32_t handle = khdispatch_read_int();

      wfcIntDestroyContext(handle);
      break;
   }
   case WFCINTCOMMITBEGIN_ID:
   {
      WFCContext context = khdispatch_read_int();
      bool wait = khdispatch_read_boolean();
      WFCRotation rotation = khdispatch_read_enum();
      WFCfloat background_red = khdispatch_read_float();
      WFCfloat background_green = khdispatch_read_float();
      WFCfloat background_blue = khdispatch_read_float();
      WFCfloat background_alpha = khdispatch_read_float();

      bool ready = wfcIntCommitBegin_impl(context, wait, rotation,
         background_red, background_green, background_blue, background_alpha);

      khdispatch_write_boolean(ready);
      break;
   }
   case WFCINTCOMMITADD_ID:
   {
      int32_t handle = khdispatch_read_int();
      int32_t source = khdispatch_read_int();
      int32_t mask = khdispatch_read_int();
      int32_t drect_0 = khdispatch_read_int();
      int32_t drect_1 = khdispatch_read_int();
      int32_t drect_2 = khdispatch_read_int();
      int32_t drect_3 = khdispatch_read_int();
      float srect_0 = khdispatch_read_float();
      float srect_1 = khdispatch_read_float();
      float srect_2 = khdispatch_read_float();
      float srect_3 = khdispatch_read_float();
      int32_t flip = khdispatch_read_int();
      int32_t rotation = khdispatch_read_int();
      int32_t scale_filter = khdispatch_read_int();
      int32_t transparency = khdispatch_read_int();
      float global_alpha = khdispatch_read_float();

      wfcIntCommitAdd(handle,
                      source,
                      mask,
                      drect_0, drect_1, drect_2, drect_3,
                      srect_0, srect_1, srect_2, srect_3,
                      flip,
                      rotation,
                      scale_filter,
                      transparency,
                      global_alpha);
      break;
   }
   case WFCINTCOMMITEND_ID:
   {
      int32_t handle = khdispatch_read_int();

      wfcIntCommitEnd(handle);
      break;
   }
   case WFCACTIVATE_ID:
   {
      int32_t handle = khdispatch_read_int();

      wfcActivate(handle);
      break;
   }
   case WFCDEACTIVATE_ID:
   {
      int32_t handle = khdispatch_read_int();

      wfcDeactivate(handle);
      break;
   }
   case WFCCOMPOSE_ID:
   {
      int32_t handle = khdispatch_read_int();

      wfcCompose(handle);
      break;
   }
   //--------------------------------------
   case WFC_STREAM_CREATE_ID:
   {
      WFCNativeStreamType stream_key = khdispatch_read_uint();
      VC_IMAGE_TYPE_T type = khdispatch_read_enum();
      uint32_t width = khdispatch_read_uint();
      uint32_t height = khdispatch_read_uint();
      uint32_t flags = khdispatch_read_uint();
      uint32_t pid_lo = khdispatch_read_uint();
      uint32_t pid_hi = khdispatch_read_uint();

      wfc_stream_server_create
         (stream_key, type, width, height, flags, pid_lo, pid_hi);
      break;
   }
   case WFC_STREAM_DESTROY_ID:
   {
      WFCNativeStreamType stream_key = khdispatch_read_uint();
      wfc_stream_server_destroy(stream_key);
      break;
   }
   //--------------------------------------
   default:
      UNREACHABLE();
      break;
   }
}
