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
#include "middleware/khronos/common/khrn_interlock.h"
#include "middleware/khronos/ext/egl_khr_image.h"
#include "middleware/khronos/egl/egl_server.h"

#include "interface/vmcs_host/khronos/IL/OMX_Core.h"
#include "interface/vmcs_host/khronos/IL/OMX_Broadcom.h"
#include <assert.h>

int eglIntOpenMAXILDoneMarker_impl (void* component_handle, EGLImageKHR egl_image)
{
   OMX_ERRORTYPE eErr = OMX_ErrorNone;
   OMX_CONFIG_BRCMEGLIMAGEMEMHANDLETYPE cfg;

   MEM_HANDLE_T h_eglimage;
   MEM_HANDLE_T h_image;

   EGL_SERVER_STATE_T *eglstate = EGL_GET_SERVER_STATE();

   h_eglimage = khrn_map_lookup(&eglstate->eglimages, (uint32_t)egl_image);
   h_image = ((EGL_IMAGE_T *)mem_lock(h_eglimage))->mh_image;
   mem_unlock(h_eglimage);

   khrn_interlock_write_immediate(&((KHRN_IMAGE_T *)mem_lock(h_image))->interlock);
   mem_unlock(h_image);

   memset(&cfg, 0, sizeof(OMX_CONFIG_BRCMEGLIMAGEMEMHANDLETYPE));
   cfg.nSize = sizeof(OMX_CONFIG_BRCMEGLIMAGEMEMHANDLETYPE);
   cfg.nVersion.nVersion = OMX_VERSION;
   cfg.nPortIndex = 221;
   cfg.eglImage = (OMX_PTR)egl_image;
   cfg.memHandle = (OMX_PTR)h_image;

   eErr = OMX_SetConfig((OMX_HANDLETYPE)component_handle, OMX_IndexConfigBrcmEGLImageMemHandle, &cfg);

   assert(eErr == OMX_ErrorNone);

   return 0;
}
