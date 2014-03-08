/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#define GL_GLEXT_PROTOTYPES /* we want the prototypes so the compiler will check that the signatures match */

#include "interface/khronos/common/khrn_int_common.h"

#ifdef DIRECT_RENDERING
#include "interface/khronos/glxx/gl_brcm_mangle.h"
#endif

#include "interface/khronos/glxx/glxx_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#ifdef RPC_DIRECT
#include "interface/khronos/glxx/glxx_int_impl.h"
#include "interface/khronos/glxx/gl20_int_impl.h"
#endif

#include "interface/khronos/include/GLES2/gl2.h"
#include "interface/khronos/include/GLES2/gl2ext.h"

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
#if EGL_BRCM_global_image
   /* todo: if ((uintptr_t)image & (1 << 31)) */
#endif

   if (IS_OPENGLES_11() || IS_OPENGLES_20()) {
      RPC_CALL2(glEGLImageTargetTexture2DOES_impl,
                GLEGLIMAGETARGETTEXTURE2DOES_ID,
                RPC_ENUM(target),
                RPC_EGLID(image));
   }
}

GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES (GLenum target, GLeglImageOES image)
{
#if EGL_BRCM_global_image
   /* todo: if ((uintptr_t)image & (1 << 31)) */
#endif

   if (IS_OPENGLES_11()) {
      /* OES_framebuffer_object not supported for GLES1.1 */
      GLXX_CLIENT_STATE_T *state = GLXX_GET_CLIENT_STATE();
      if (state->error == GL_NO_ERROR)
         state->error = GL_INVALID_OPERATION;
   }
   if (IS_OPENGLES_20()) {
      RPC_CALL2(glEGLImageTargetRenderbufferStorageOES_impl_20,
                GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES_ID_20,
                RPC_ENUM(target),
                RPC_EGLID(image));
   }
}
