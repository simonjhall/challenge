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
#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES/glext.h"
#include "interface/khronos/include/GLES2/gl2.h"
#include "interface/khronos/include/GLES2/gl2ext.h"
#include "interface/khronos/glxx/glxx_int_config.h"

#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"


static KHRN_IMAGE_FORMAT_T convert_format(GLenum format);
static GLXX_RENDERBUFFER_TYPE_T convert_renderbuffer_type(GLenum format);

void glxx_renderbuffer_init(GLXX_RENDERBUFFER_T *renderbuffer, int32_t name)
{
   vcos_assert(renderbuffer);

   renderbuffer->name = name;

   renderbuffer->type = RB_NEW_T;
   vcos_assert(renderbuffer->mh_storage == MEM_INVALID_HANDLE);
}

void glxx_renderbuffer_term(void *v, uint32_t size)
{
   GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)v;

   UNUSED(size);

   MEM_ASSIGN(renderbuffer->mh_storage, MEM_INVALID_HANDLE);
}

static KHRN_IMAGE_FORMAT_T convert_format(GLenum format)
{
   switch (format) {
   /* case GL_DEPTH_COMPONENT: TODO confirm if this is a permissable value */
   case GL_DEPTH_COMPONENT24_OES:
   case GL_DEPTH_COMPONENT32_OES:
   /* case GL_STENCIL_INDEX: TODO confirm if this is a permissable value */
   case GL_STENCIL_INDEX8:
#ifdef __VIDEOCORE4__
      return DEPTH_32_TLBD;
#else
      return DEPTH_32_TF;
#endif
   /* case GL_RGB: TODO confirm if this is a permissable value */
   case GL_RGB8_OES:
      #ifdef BIG_ENDIAN
         return RGBX_8888_TF;
      #else
         return XBGR_8888_TF;
      #endif
   /* case GL_RGBA: TODO confirm if this is a permissable value */
   case GL_RGBA8_OES:
      #ifdef BIG_ENDIAN
         return RGBA_8888_TF;
      #else
         return ABGR_8888_TF;
      #endif
   case GL_RGBA4:
      return RGBA_4444_TF;
   case GL_RGB5_A1:
      return RGBA_5551_TF;
   case GL_RGB565:
      return RGB_565_TF;
   case GL_DEPTH_COMPONENT16:
#ifdef __VIDEOCORE4__
      return DEPTH_32_TLBD;
#else
      return DEPTH_16_TF;
#endif
   default:
      UNREACHABLE();
      break;
   }

   return IMAGE_FORMAT_INVALID;
}

bool glxx_renderbuffer_storage(GLXX_RENDERBUFFER_T *renderbuffer, GLenum internalformat, GLuint width, GLuint height)
{
   KHRN_IMAGE_FORMAT_T format = convert_format(internalformat);
   GLXX_RENDERBUFFER_TYPE_T type = convert_renderbuffer_type(internalformat);
   KHRN_IMAGE_T *oldimage = (KHRN_IMAGE_T *)mem_maybe_lock(renderbuffer->mh_storage);

   /* TODO: what about if we're sharing the image? */
   bool imagechanged = !oldimage || oldimage->format != format || oldimage->width != width || oldimage->height != height || renderbuffer->type != type;
   mem_maybe_unlock(renderbuffer->mh_storage);

   if (imagechanged)
   {
      uint32_t flags;
      MEM_HANDLE_T hnew;
      
      flags = IMAGE_CREATE_FLAG_ONE;
      if (type == RB_COLOR_T) flags |= IMAGE_CREATE_FLAG_TEXTURE;
      if (glxx_framebuffer_hw_support(format)) flags |= IMAGE_CREATE_FLAG_RENDER_TARGET;
      hnew = khrn_image_create(format, width, height, flags); /* todo: check usage flags */
      if (hnew == MEM_INVALID_HANDLE)
         return false;

      MEM_ASSIGN(renderbuffer->mh_storage, hnew);
      mem_release(hnew);
      renderbuffer->type = type;
   }

   return true;
}

static bool valid_image(KHRN_IMAGE_T *image)
{
   KHRN_IMAGE_FORMAT_T format;
   if ((image->width > GLXX_CONFIG_MAX_RENDERBUFFER_SIZE) || (image->height > GLXX_CONFIG_MAX_RENDERBUFFER_SIZE))
      return false;

   if (!(khrn_image_is_tformat(image->format) || (khrn_image_is_color(image->format) && khrn_image_is_lineartile(image->format))))
      return false;

   format = khrn_image_to_tf_format(image->format);

   return
      format == ABGR_8888_TF ||
      format == XBGR_8888_TF ||
      format == RGBA_4444_TF ||
      format == RGBA_5551_TF ||
      format == RGB_565_TF;
      /* currently don't allow depth/stencil formats
      format == DEPTH_16_TF ||
      format == DEPTH_32_TF;*/
}

bool glxx_renderbuffer_bind_image(GLXX_RENDERBUFFER_T *renderbuffer, MEM_HANDLE_T himage)
{
   KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(himage);
   bool result;

   if (valid_image(image)) {
      MEM_ASSIGN(renderbuffer->mh_storage, himage);
      vcos_assert(khrn_image_is_color(image->format));
      renderbuffer->type = RB_COLOR_T;
      renderbuffer->merged = false;
      result = true;
   } else
      result = false;

   mem_unlock(himage);
   return result;
}

static bool single_ref_count(GLXX_RENDERBUFFER_T *renderbuffer)
{
   uint32_t refcount;
   vcos_assert(renderbuffer->type != RB_NEW_T);
   vcos_assert(renderbuffer->mh_storage != MEM_INVALID_HANDLE);

   refcount = mem_get_ref_count(renderbuffer->mh_storage);
   vcos_assert(refcount > 0);

   return refcount == 1;
}

// If this renderbuffer is in the merged state, force it into the shared state by
// allocating new storage.
bool glxx_renderbuffer_unmerge(GLXX_RENDERBUFFER_T *renderbuffer)
{
   if (renderbuffer->merged && !single_ref_count(renderbuffer)) {
      KHRN_IMAGE_T *storage;
      MEM_HANDLE_T hstorage;

      // Renderbuffer is in merged state. We must unmerge it.

      vcos_assert(renderbuffer->type == RB_DEPTH24_T || renderbuffer->type == RB_STENCIL_T);  // The only sorts of buffer we can merge
      storage = (KHRN_IMAGE_T *)mem_lock(renderbuffer->mh_storage);
      vcos_assert(storage->format == DEPTH_32_TF);

      hstorage = khrn_image_create_dup(storage, IMAGE_CREATE_FLAG_NONE);
      mem_unlock(renderbuffer->mh_storage);

      if (hstorage == MEM_INVALID_HANDLE)
         return false;

      MEM_ASSIGN(renderbuffer->mh_storage, hstorage);
      mem_release(hstorage);
   }
   renderbuffer->merged = false;   // This ensures we're in the correct state if someone subsequently adds a reference
   return true;
}

// Attempt to merge these buffers so that they both use the same storage buffer.
// This can only happen if there is just one reference to each buffer. If this is
// not the case then we do nothing - this is just an optimisation so it is allowed
// to fail.
// Currently if either buffer is in the merged state we will not unmerge it (though
// we could do this safely).
void glxx_renderbuffer_attempt_merge(GLXX_RENDERBUFFER_T *depth, GLXX_RENDERBUFFER_T *stencil)
{
   KHRN_IMAGE_T *dstorage = (KHRN_IMAGE_T *)mem_lock(depth->mh_storage);
   KHRN_IMAGE_T *sstorage = (KHRN_IMAGE_T *)mem_lock(stencil->mh_storage);

   vcos_assert(depth && stencil);
   vcos_assert(depth->type == RB_DEPTH24_T && stencil->type == RB_STENCIL_T);  // The only sorts of buffer we can merge
   vcos_assert(dstorage->format == DEPTH_32_TF && sstorage->format == DEPTH_32_TF);
   vcos_assert(dstorage->width == sstorage->width && dstorage->height == sstorage->height);

   if (single_ref_count(depth) && single_ref_count(stencil)) {
      khrn_image_copy_stencil_channel(dstorage, sstorage);
      mem_unlock(depth->mh_storage);     /* Need to unlock before assigning */
      mem_unlock(stencil->mh_storage);

      MEM_ASSIGN(stencil->mh_storage, depth->mh_storage);
      depth->merged = true;
      stencil->merged = true;
   } else {
      mem_unlock(depth->mh_storage);
      mem_unlock(stencil->mh_storage);
   }
}

static GLXX_RENDERBUFFER_TYPE_T convert_renderbuffer_type(GLenum format)
{
   switch (format) {
   /* case GL_DEPTH_COMPONENT: TODO confirm whether this is a permissable format */
   case GL_DEPTH_COMPONENT24_OES:
   case GL_DEPTH_COMPONENT32_OES:
      return RB_DEPTH24_T;
   /*case GL_RGB: TODO confirm whether this is a permissable format */
   case GL_RGBA:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGB565:
   case GL_RGB8_OES:
   case GL_RGBA8_OES:
      return RB_COLOR_T;
   case GL_DEPTH_COMPONENT16:
      return RB_DEPTH16_T;
   /* case GL_STENCIL_INDEX: TODO confirm whether this is a permissable format */
   case GL_STENCIL_INDEX8:
      return RB_STENCIL_T;
   default:
      UNREACHABLE();
      return RB_NEW_T;
   }
}
