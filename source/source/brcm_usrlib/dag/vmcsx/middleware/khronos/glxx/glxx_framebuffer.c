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

#include "middleware/khronos/glxx/glxx_framebuffer.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"

#include "middleware/khronos/glxx/glxx_texture.h"

static void attachment_info_init(GLXX_ATTACHMENT_INFO_T *attachment)
{
   vcos_assert(attachment);

   /*
      we never re-init a program structure, so this
      should be shiny and new
   */

   vcos_assert(attachment->mh_object == MEM_INVALID_HANDLE);

   attachment->type = GL_NONE;
   attachment->target = 0;
   attachment->level = 0;
}

void glxx_framebuffer_init(GLXX_FRAMEBUFFER_T *framebuffer, int32_t name)
{
   vcos_assert(framebuffer);

   framebuffer->name = name;

   attachment_info_init(&framebuffer->attachments.color);
   attachment_info_init(&framebuffer->attachments.depth);
   attachment_info_init(&framebuffer->attachments.stencil);
}

static void attachment_info_term(GLXX_ATTACHMENT_INFO_T *attachment)
{
   vcos_assert(attachment);

   MEM_ASSIGN(attachment->mh_object, MEM_INVALID_HANDLE);
}

void glxx_framebuffer_term(void *v, uint32_t size)
{
   GLXX_FRAMEBUFFER_T *framebuffer = (GLXX_FRAMEBUFFER_T *)v;
   UNUSED(size);

   attachment_info_term(&framebuffer->attachments.color);
   attachment_info_term(&framebuffer->attachments.depth);
   attachment_info_term(&framebuffer->attachments.stencil);
}

vcos_static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_X == GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1);
vcos_static_assert(GL_TEXTURE_CUBE_MAP_POSITIVE_Y == GL_TEXTURE_CUBE_MAP_NEGATIVE_X + 1);
vcos_static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y == GL_TEXTURE_CUBE_MAP_POSITIVE_Y + 1);
vcos_static_assert(GL_TEXTURE_CUBE_MAP_POSITIVE_Z == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y + 1);
vcos_static_assert(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z == GL_TEXTURE_CUBE_MAP_POSITIVE_Z + 1);
MEM_HANDLE_T glxx_attachment_info_get_image(GLXX_ATTACHMENT_INFO_T *attachment)
{
   MEM_HANDLE_T result = MEM_INVALID_HANDLE;
   
   switch (attachment->type) {
   case GL_NONE:
      result = MEM_INVALID_HANDLE;
      break;
   case GL_TEXTURE:
   {
      GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(attachment->mh_object);

      switch (attachment->target) {
      case GL_TEXTURE_2D:
	  case GL_TEXTURE_EXTERNAL_OES:
         result = glxx_texture_share_mipmap(texture, TEXTURE_BUFFER_TWOD, attachment->level);
         break;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         vcos_assert(attachment->level == 0);

         result = glxx_texture_share_mipmap(texture, TEXTURE_BUFFER_POSITIVE_X + attachment->target - GL_TEXTURE_CUBE_MAP_POSITIVE_X, attachment->level);
         break;
      default:
         UNREACHABLE();
      }
      mem_unlock(attachment->mh_object);
      break;
   }
   case GL_RENDERBUFFER:
   {
      GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(attachment->mh_object);

      result = renderbuffer->mh_storage;
      mem_unlock(attachment->mh_object);
      break;
   }
   default:
      UNREACHABLE();
   }

   return result;
}

bool glxx_framebuffer_hw_support(KHRN_IMAGE_FORMAT_T format)
{
#ifdef __VIDEOCORE4__
#ifdef __BCM2708A0__
   if (format != ABGR_8888_TF && format != XBGR_8888_TF && format != DEPTH_32_TLBD && format != DEPTH_COL_64_TLBD)
      return false;
#else
   if (format != ABGR_8888_TF && format != XBGR_8888_TF && format != RGB_565_TF && format != DEPTH_32_TLBD && format != DEPTH_COL_64_TLBD)
      return false;
#endif
#endif
   return true;
}

/*
   The framebuffer attachment point attachment is said to be framebuffer attachment complete if the value
   of FRAMEBUFFER ATTACHMENT OBJECT TYPE for attachment is NONE (i.e., no image is attached), or if all
   of the following conditions are true:

   - <image> is a component of an existing object with the name specified by FRAMEBUFFER ATTACHMENT -
     OBJECT NAME, and of the type specified by FRAMEBUFFER ATTACHMENT OBJECT TYPE.
   - The width and height of <image> must be non-zero.
   - If attachment is COLOR ATTACHMENT0, then image must have a color-renderable internal format.
   - If attachment is DEPTH ATTACHMENT, then image must have a depth-renderable internal format.
   - If attachment is STENCIL ATTACHMENT, then image must have a stencil-renderable internal format.

   The framebuffer object target is said to be framebuffer complete if it is the window-system-provided
   framebuffer, or if all the following conditons are true:

   - All framebuffer attachment points are framebuffer attachment complete. FRAMEBUFFER INCOMPLETE ATTACHMENT
   - There is at least one image attached to the framebuffer. FRAMEBUFFER INCOMPLETE MISSING ATTACHMENT
   - All attached images have the same width and height. FRAMEBUFFER INCOMPLETE DIMENSIONS
   - The combination of internal formats of the attached images does not violate an implementationdependent
     set of restrictions. FRAMEBUFFER UNSUPPORTED

   The enums in bold after each clause of the framebuffer completeness rules specifies the return value of
   CheckFramebufferStatus that is generated when that clause is violated. If more than one clause is violated,
   it is implementation-dependent as to exactly which enum will be returned by CheckFramebufferStatus.
*/

#define ATTACHMENT_MISSING 0   //Attachment type is NONE, therefore complete
#define ATTACHMENT_BROKEN 1    //Attachment is incomplete
#define ATTACHMENT_COLOR 2     //Attachment type is COLOR and is complete
#define ATTACHMENT_DEPTH16 3   //Attachment type is DEPTH, is complete and the image is 16bpp
#define ATTACHMENT_DEPTH24 4   //Attachment type is DEPTH, is complete and the image is 24bpp
#define ATTACHMENT_STENCIL 5   //Attachment type is STENCIL and is complete
#define ATTACHMENT_UNSUPPORTED 6//Attachment type is valid but rendering is not supported

static uint32_t attachment_get_status(GLXX_ATTACHMENT_INFO_T *attachment, uint32_t *width, uint32_t *height)
{
   switch (attachment->type) {
   case GL_NONE:
      return ATTACHMENT_MISSING;
   case GL_RENDERBUFFER:
   {
      GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(attachment->mh_object);
      uint32_t result = ATTACHMENT_BROKEN;
      switch (renderbuffer->type) {
      case RB_NEW_T:
         result = ATTACHMENT_BROKEN; break;
      case RB_COLOR_T:
         result = ATTACHMENT_COLOR; break;
      case RB_DEPTH16_T:
         result = ATTACHMENT_DEPTH16; break;
      case RB_DEPTH24_T:
         result = ATTACHMENT_DEPTH24; break;
      case RB_STENCIL_T:
         result = ATTACHMENT_STENCIL; break;
      default:
         UNREACHABLE();
      }
      if (renderbuffer->mh_storage == MEM_INVALID_HANDLE) {
         *width = 0;
         *height = 0;
      } else {
         KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(renderbuffer->mh_storage);
         *width = image->width;
         *height = image->height;
#ifdef __VIDEOCORE4__
         if (!glxx_framebuffer_hw_support(image->format))
            result = ATTACHMENT_UNSUPPORTED;
#endif
         mem_unlock(renderbuffer->mh_storage);
      }
      if (*width == 0 || *height == 0) result = ATTACHMENT_BROKEN;
      mem_unlock(attachment->mh_object);
      return result;
   }
   case GL_TEXTURE:
   {
      MEM_HANDLE_T himg = glxx_attachment_info_get_image(attachment);
      uint32_t result = ATTACHMENT_BROKEN;
      if (himg == MEM_INVALID_HANDLE) {
         *width = 0;
         *height = 0;
      } else {
         KHRN_IMAGE_T *img = (KHRN_IMAGE_T *)mem_lock(himg);
#ifdef __VIDEOCORE4__
         switch (img->format)
         {
         case ABGR_8888_TF:
         case XBGR_8888_TF:
#ifndef __BCM2708A0__
         case RGB_565_TF:
         case ABGR_8888_LT:
         case XBGR_8888_LT:
         case RGB_565_LT:
#endif
            result = ATTACHMENT_COLOR;
            break;
         case RGBA_4444_TF:
         case RGBA_5551_TF:
#ifdef __BCM2708A0__
         case RGB_565_TF:
         case ABGR_8888_LT:
         case XBGR_8888_LT:
         case RGB_565_LT:
#endif
         case RGBA_4444_LT:
         case RGBA_5551_LT:
            result = ATTACHMENT_UNSUPPORTED;
            break;

         default:  //other texture formats or IMAGE_FORMAT_INVALID
            result = ATTACHMENT_BROKEN;
         }
#else
         switch (img->format)
         {
         case ABGR_8888_TF:
         case XBGR_8888_TF:
         case RGBA_4444_TF:
         case RGBA_5551_TF:
         case RGB_565_TF:
         case ABGR_8888_LT:  //HAL will deal with linear tile cases by doing image conversions after flushing
         case XBGR_8888_LT:
         case RGBA_4444_LT:
         case RGBA_5551_LT:
         case RGB_565_LT:
            result = ATTACHMENT_COLOR;
            break;
         default:  //other texture formats or IMAGE_FORMAT_INVALID
            result = ATTACHMENT_BROKEN;
         }
#endif
         *width = img->width;
         *height = img->height;
         mem_unlock(himg);
      }
      if (*width == 0 || *height == 0) result = ATTACHMENT_BROKEN;
      return result;
   }
   default:
      UNREACHABLE();
      return ATTACHMENT_BROKEN;
   }
}

GLenum glxx_framebuffer_check_status(GLXX_FRAMEBUFFER_T *framebuffer)
{
   uint32_t ca, cw, ch;
   uint32_t da, dw, dh;
   uint32_t sa, sw, sh;
   GLenum result = GL_FRAMEBUFFER_COMPLETE;

   vcos_assert(framebuffer);

   //color/depth/stencil attachment/width/height
   //If attachment is NONE or BROKEN, width and height should be ignored

   ca = attachment_get_status(&framebuffer->attachments.color, &cw, &ch);
   da = attachment_get_status(&framebuffer->attachments.depth, &dw, &dh);
   sa = attachment_get_status(&framebuffer->attachments.stencil, &sw, &sh);

   if (ca == ATTACHMENT_MISSING && da == ATTACHMENT_MISSING && sa == ATTACHMENT_MISSING)
      result = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
   else if (ca == ATTACHMENT_UNSUPPORTED || da == ATTACHMENT_UNSUPPORTED || sa == ATTACHMENT_UNSUPPORTED)
      result = GL_FRAMEBUFFER_UNSUPPORTED;
   else {
      if (ca == ATTACHMENT_MISSING)
         result = GL_FRAMEBUFFER_UNSUPPORTED;
      else if (da == ATTACHMENT_DEPTH16 && sa != ATTACHMENT_MISSING)
         result = GL_FRAMEBUFFER_UNSUPPORTED;
      else {
         GLboolean color_complete = (ca == ATTACHMENT_MISSING || ca == ATTACHMENT_COLOR);
         GLboolean depth_complete = (da == ATTACHMENT_MISSING || da == ATTACHMENT_DEPTH16 || da == ATTACHMENT_DEPTH24);
         GLboolean stencil_complete = (sa == ATTACHMENT_MISSING || sa == ATTACHMENT_STENCIL);

         if (!color_complete || !depth_complete || !stencil_complete)
            result = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
         else {
            /*
               at this point we know we have 

               - a complete color buffer
               - a complete depth buffer (if present)
               - a complete stencil buffer (if present)

               we need to check whether we have a depth buffer, and if so whether it has the same size as the color buffer
               similarly for stencil
            */

            if (da != ATTACHMENT_MISSING && (cw != dw || ch != dh))
               result = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
            else if (sa != ATTACHMENT_MISSING && (cw != sw || ch != sh))
               result = GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
         }
      }
   }

   return result;
}


