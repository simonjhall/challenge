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
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/ext/egl_khr_image.h"
#include "middleware/khronos/egl/egl_platform.h"
#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/gl11/gl11_server.h"
#include "middleware/khronos/gl20/gl20_server.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/vg/vg_server.h"
#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"
#include "interface/khronos/include/EGL/eglext_brcm.h"
#include "opensrc/interface/khronos/include/EGL/eglext_android.h"

/* */


void egl_image_term(void *v, uint32_t size)
{
   EGL_IMAGE_T *image = (EGL_IMAGE_T *)v;
   UNUSED(size);

   MEM_ASSIGN(image->mh_image, MEM_INVALID_HANDLE);
}

/*
    int eglCreateImageKHR_impl (uint32_t glversion, EGL_CONTEXT_ID_T ctx, EGLenum target, uint32_t buffer, EGLint texture_level, EGLint *results)

    EGL_BAD_CONTEXT     ctx is not a valid EGLContext handle
    EGL_BAD_PARAMETER   target is not one of the values in Table aaa
    EGL_BAD_PARAMETER   attr_list includes an attribute not listed in Table bbb
    EGL_BAD_ACCESS      Resource bound with eglBindTexImage, eglCreatePbufferFromClientBuffer or is already an EGLImage sibling
    EGL_BAD_ALLOC       Out of memory
    EGL_BAD_MATCH       Type of object does not match type of context
    EGL_BAD_PARAMETER   Buffer is not the name of a valid object of the specified target type in the specified context
    EGL_BAD_PARAMETER   Buffer refers to default object (0)
    EGL_BAD_PARAMETER   Buffer refers to an incomplete texture or a renderbuffer without storage (TODO: should we forbid depth/stencil renderbuffers at this stage?)
    EGL_BAD_MATCH       VGImage is a child image
    EGL_BAD_MATCH       Invalid mipmap level specified

   results
   0  EGLImageKHR       Handle to new EGLImage (0 on failure)
   1  EGLInt            Error code (EGL_SUCCESS on success)
   We always return 2.

   Khronos documentation:
   [mostly EGL_KHR_image but other extensions amend it]

   The command

        EGLImageKHR eglCreateImageKHR(
                                EGLDisplay dpy,
                                EGLContext ctx,
                                EGLenum target,
                                EGLClientBuffer buffer,
                                EGLint* attr_list)

    is used to create an EGLImage from an existing image resource <buffer>.
    <dpy> specifies the EGL display used for this operation, or EGL_NO_DISPLAY
    if a display is not required.  <ctx> specifies the EGL client API context
    used for this operation, or EGL_NO_CONTEXT if a client API context is not
    required.  <target> specifies the type of resource being used as the
    EGLImage source (examples include two-dimensional textures in OpenGL ES
    contexts and VGImage objects in OpenVG contexts).  <buffer> is the name
    (or handle) of a resource to be used as the EGLImage source, cast into the
    type EGLClientBuffer.  <attr_list> is an list of attribute-value pairs
    which is used to select sub-sections of <buffer> for use as the EGLImage
    source, such as mipmap levels for OpenGL ES texture map resources.  If
    <attr_list> is non-NULL, the last attribute specified in the list should
    be EGL_NONE.

    The resource specified by <dpy>, <ctx>, <target>, <buffer>, and
    <attr_list> must not itself be an EGLImage sibling, or bound to an EGL
    PBuffer resource (eglBindTexImage, eglCreatePbufferFromClientBuffer).

    Values accepted for <target> are listed in Table aaa, below.

      +-----------------------------------------+------------------------------------+
      |  <target>                               |  Notes                             |
      +-----------------------------------------+------------------------------------+
      |  EGL_NATIVE_PIXMAP_KHR                  |Used for EGLNativePixmapType objects|    [EGL_KHR_image_pixmap]
      +-----------------------------------------+------------------------------------+
      |  EGL_VG_PARENT_IMAGE_KHR                |  Used for OpenVG VGImage objects   |    [EGL_KHR_vg_parent_image]
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_2D_KHR                  |      Used for OpenGL ES            |    [EGL_KHR_gl_texture_2D_image]
      |                                         |      2D texture images.            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_POSITIVE_X_KHR  |  Used for the +X face of           |    [EGL_KHR_gl_texture_cubemap_image]
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_X_KHR  |  Used for the -X face of           |
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_POSITIVE_Y_KHR  |  Used for the +Y face of           |
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_Y_KHR  |  Used for the -Y face of           |
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_POSITIVE_Z_KHR  |  Used for the +Z face of           |
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_Z_KHR  |  Used for the -Z face of           |
      |                                         |  OpenGL-ES cubemap texture         |
      |                                         |  images                            |
      +-----------------------------------------+------------------------------------+
      |  EGL_GL_RENDERBUFFER_KHR                |  Used for OpenGL ES                |    [EGL_KHR_gl_texture_renderbuffer_image]
      |                                         |  renderbuffer images               |
      +-----------------------------------------+------------------------------------+
       Table aaa.  Legal values for eglCreateImageKHR <target> parameter

    Attributes accepted in the attribute-value list <attr_list> are listed
    in Table bbb, below.

      +------------------------+-----------------------------------------+---------------+
      | Attribute              | Description                             | Default Value |
      +------------------------+-----------------------------------------+---------------+
      |  EGL_NONE              |Marks the end of the attribute-value list| N/A           |
      +------------------------+-----------------------------------------+---------------+
      |EGL_GL_TEXTURE_LEVEL_KHR|  Specifies the mipmap level             |   0           |    [EGL_KHR_gl_*]
      |                        |  used as the EGLImage source.           |               |
      |                        |  Must be part of the complete           |               |
      |                        |  texture object <buffer>                |               |
      +-----------+------------------------------------------------------+---------------+
       Table bbb.  Legal attributes for eglCreateImageKHR <attr_list> parameter

    If <target> is EGL_NATIVE_PIXMAP_KHR, <dpy> must be a valid display, <ctx>
    must be EGL_NO_CONTEXT; <buffer> must be a handle to a valid
    NativePixmapType object, cast into the type EGLClientBuffer, and
    <attr_list> is ignored.

    [EGL_KHR_vg_parent_image]
    If <target> is EGL_VG_PARENT_IMAGE_KHR, <dpy> must be a valid EGLDisplay,
    <ctx> must be a valid OpenVG API context on that display, and <buffer>
    must be a handle of a VGImage object valid in the specified context, cast
    into the type EGLClientBuffer.  Furthermore, the specified VGImage
    <buffer> must not be a child image (i.e. the value returned by
    vgGetParent(<buffer>) must be <buffer>).  If the specified VGImage
    <buffer> has any child images (i.e., vgChildImage has been previously
    called with the parent parameter set to <buffer>), all child images will
    be treated as EGLImage siblings after CreateImageKHR returns.  Any values
    specified in <attr_list> are ignored.

    [EGL_KHR_gl_*]
    If <target> is EGL_GL_TEXTURE_2D_KHR,
    EGL_GL_RENDERBUFFER_KHR,
    EGL_GL_TEXTURE_CUBEMAP_POSITIVE_X_KHR,
    EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_X_KHR,
    EGL_GL_TEXTURE_CUBEMAP_POSITIVE_Y_KHR,
    EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_Y_KHR,
    EGL_GL_TEXTURE_CUBEMAP_POSITIVE_Z_KHR, or
    EGL_GL_TEXTURE_CUBEMAP_NEGATIVE_Z_KHR,
    <dpy> must be a valid EGLDisplay,
    and <ctx> must be a valid OpenGL ES API context on that display.

    [EGL_KHR_gl_texture_2D_image]
    If <target> is EGL_GL_TEXTURE_2D_KHR, <buffer> must be the name of a
    complete, non-default EGL_GL_TEXTURE_2D target texture object, cast into
    the type EGLClientBuffer.  <attr_list> should specify the mipmap level
    which will be used as the EGLImage source (EGL_GL_TEXTURE_LEVEL_KHR); the
    specified mipmap level must be part of <buffer>.  If not specified, the
    default value listed in Table bbb will be used, instead.  Additional
    values specified in <attr_list> are ignored.

    [EGL_KHR_gl_texture_cubemap_image]
    If <target> is one of the EGL_GL_TEXTURE_CUBEMAP_* enumerants,
    <buffer> must be the name of a cube-complete, non-default
    EGL_GL_TEXTURE_CUBEMAP_OES target texture object, cast into the type
    EGLClientBuffer.  <attr_list> should specify the mipmap level which will
    be used as the EGLImage source (EGL_GL_TEXTURE_LEVEL_KHR); the specified
    mipmap level must be part of <buffer>.  If not specified, the default
    value listed in Table bbb will be used, instead.  Additional values
    specified in <attr_list> are ignored.

    [EGL_KHR_gl_renderbuffer_image]
    If <target> is EGL_GL_RENDERBUFFER_KHR, <buffer> must be the name of a
    complete, non-default EGL_GL_RENDERBUFFER_OES object, cast into the type
    EGLClientBuffer.  Values specified in <attr_list> are ignored.

    This command returns an EGLImageKHR object corresponding to the image
    data specified by <dpy>, <ctx>, <target>, <buffer> and <attr_list> which
    may be referenced by client API operations, or EGL_NO_IMAGE_KHR in the
    event of an error.  After this function returns, the pixel data in
    <buffer> will be undefined (this applies to all sub-objects which may be
    included in <buffer>, not just those which are specified by <attr_list>).
    If the <attr_list> contains attributes from table bbb which are not
    applicable to the buffer specified by <target> then those extra attributes
    are ignored.

       * If <dpy> is not the handle of a valid EGLDisplay object or
         EGL_NO_DISPLAY, the error EGL_BAD_DISPLAY is generated.

       * If <ctx> is not the handle of a valid EGLContext object on <dpy> or
         EGL_NO_CONTEXT, the error EGL_BAD_CONTEXT
         is generated.

       * If <target> is not one of the values in Table aaa, the error
         EGL_BAD_PARAMETER is generated.

       * If <target> is EGL_NATIVE_PIXMAP_KHR and <buffer> is not a
         valid native pixmap handle, or if there is no EGLConfig
         supporting native pixmaps whose color buffer format matches the
         format of <buffer>, the error EGL_BAD_PARAMETER is generated.

       * If an attribute specified in <attr_list> is not one of the
         attributes listed in Table bbb, the error EGL_BAD_PARAMETER is
         generated.

       * If the resource specified by <dpy>, <ctx>, <target>, <buffer> and
         <attr_list> has an off-screen buffer bound to it (e.g., by a
         previous call to eglBindTexImage), the error EGL_BAD_ACCESS is
         generated.

       * If the resource specified by <dpy>, <ctx>, <target>, <buffer> and
         <attr_list> is bound to an off-screen buffer (e.g., by a previous
         call to eglCreatePbufferFromClientBuffer), the error
         EGL_BAD_ACCESS is generated.

       * If the resource specified by <dpy>, <ctx>, <target>, <buffer> and
         <attr_list> is itself an EGLImage sibling, the error
         EGL_BAD_ACCESS is generated.

       * If <target> is EGL_NATIVE_PIXMAP_KHR, and <dpy> is not a valid
         EGLDisplay object the error EGL_BAD_DISPLAY is generated.

       * If <target> is EGL_NATIVE_PIXMAP_KHR, and <ctx> is not EGL_NO_CONTEXT,
         the error EGL_BAD_PARAMETER is generated.

       * If <target> is NATIVE_PIXMAP_KHR, and <buffer> is not a handle
         to a valid NativePixmapType object, the error EGL_BAD_PARAMETER
         is generated.

       * If insufficient memory is available to complete the specified
         operation, the error EGL_BAD_ALLOC is generated.

       * If the call to eglCreateImageKHR fails for multiple reasons, the
         generated error must be appropriate for one of the reasons,
         although the specific error returned is undefined.

    [EGL_KHR_vg_parent_image]
        * If <target> is EGL_VG_PARENT_IMAGE_KHR, and <dpy> is not a
          valid EGLDisplay, the error EGL_BAD_DISPLAY is generated.

        * If <target> is EGL_VG_PARENT_IMAGE_KHR and <ctx> is not a
          valid EGLContext, the error EGL_BAD_CONTEXT is generated.

        * If <target> is EGL_VG_PARENT_IMAGE_KHR and <ctx> is not a valid
          OpenVG context, the error EGL_BAD_MATCH is returned.

        * If <target> is EGL_VG_PARENT_IMAGE_KHR and <buffer> is not a handle
          to a VGImage object in the specified API context <ctx>, the error
          EGL_BAD_PARAMETER is generated.

        * If <target> is EGL_VG_PARENT_IMAGE_KHR, and the VGImage specified by
          <buffer> is a child image (i.e., vgGetParent(<buffer>) returns
          a different handle), the error EGL_BAD_ACCESS is generated.

    [EGL_KHR_gl_*]
        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR,
          EGL_GL_RENDERBUFFER_KHR or EGL_GL_TEXTURE_3D_KHR, and <dpy> is not a
          valid EGLDisplay, the error EGL_BAD_DISPLAY is generated.

        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR,
          EGL_GL_RENDERBUFFER_KHR or EGL_GL_TEXTURE_3D_KHR, and <ctx> is not a
          valid EGLContext, the error EGL_BAD_CONTEXT is generated.

        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR,
          EGL_GL_RENDERBUFFER_KHR or EGL_GL_TEXTURE_3D_KHR, and <ctx> is not a
          valid OpenGL ES context, or does not match the <dpy>, the error
          EGL_BAD_MATCH is generated.

        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR
          or EGL_GL_TEXTURE_3D_KHR and <buffer> is not the name of a complete
          texture object of type <target>, the error EGL_BAD_PARAMETER
          is generated.

        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR,
          EGL_GL_RENDERBUFFER_KHR or EGL_GL_TEXTURE_3D_KHR and <buffer> refers
          to the default object (0), the error EGL_BAD_PARAMETER is
          generated.

        * If <target> is EGL_GL_TEXTURE_2D_KHR, EGL_GL_TEXTURE_CUBEMAP_*_KHR,
          or EGL_GL_TEXTURE_3D_KHR, and the value specified in <attr_list>
          for EGL_GL_TEXTURE_LEVEL_KHR is not a valid mipmap level for
          the specified texture object <buffer>, the error EGL_BAD_MATCH
          is generated.


    Note that the success or failure of eglCreateImageKHR should not affect the
    ability to use <buffer> in its original API context (or context share
    group) (although the pixel data will be undefined).

*/

static uint32_t convert_texture_target(EGLenum target)
{
   switch (target) {
   case EGL_GL_TEXTURE_2D_KHR: return TEXTURE_BUFFER_TWOD;
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR: return TEXTURE_BUFFER_POSITIVE_X;
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR: return TEXTURE_BUFFER_NEGATIVE_X;
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR: return TEXTURE_BUFFER_POSITIVE_Y;
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR: return TEXTURE_BUFFER_NEGATIVE_Y;
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR: return TEXTURE_BUFFER_POSITIVE_Z;
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR: return TEXTURE_BUFFER_NEGATIVE_Z;
   default:
      UNREACHABLE();
      return 0;
   }
}

int eglCreateImageKHR_impl (
   uint32_t glversion,
   EGL_CONTEXT_ID_T ctx,
   EGLenum target,
   uint32_t bufferPhys,
   uint32_t buffer,
   KHRN_IMAGE_FORMAT_T buffer_format,
   uint32_t buffer_width,
   uint32_t buffer_height,
   uint32_t buffer_stride,
   EGLint texture_level,
   EGLint *results)
{
   EGLint error = EGL_SUCCESS;
   EGL_SERVER_STATE_T *eglstate = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T heglimage = MEM_INVALID_HANDLE;
   MEM_HANDLE_T himage = MEM_INVALID_HANDLE;
   bool himage_acquired = false;

   /*
    * Preallocate the EGL_IMAGE_T struct and insert it into the map
    */

   heglimage = MEM_ALLOC_STRUCT_EX(EGL_IMAGE_T, MEM_COMPACT_DISCARD);

   if (!heglimage || !khrn_map_insert(&eglstate->eglimages, eglstate->next_eglimage, heglimage))
   {
      if (heglimage)
         mem_release(heglimage);

      results[0] = 0;
      results[1] = EGL_BAD_ALLOC;
      return 2;
   }

   mem_set_term(heglimage, egl_image_term);
   mem_release(heglimage);

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
   {
      /*
         if we get this far, we know we're dealing with a server-side pixmap
      */

      himage = egl_server_platform_create_pixmap_info(buffer);
      himage_acquired = true;

      if (himage == MEM_INVALID_HANDLE) {
         error = EGL_BAD_ALLOC;
      }

      break;
   }
   case EGL_NATIVE_PIXMAP_CLIENT_SIDE_BRCM:
   {
      uint32_t width, height;

      width = buffer & 0xffff;
      height = (buffer >> 16) & 0xffff;
      himage = khrn_image_create(ABGR_8888_TF, width, height, (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_RENDER_TARGET|IMAGE_CREATE_FLAG_TEXTURE));
      himage_acquired = true;

      if (himage == MEM_INVALID_HANDLE)
      {
         error = EGL_BAD_ALLOC;
      }
      else
      {
         /* Fill with dummy data */
         KHRN_IMAGE_T *image;
         KHRN_IMAGE_WRAP_T wrap;
         uint32_t x, y, stride;

         image = (KHRN_IMAGE_T *)mem_lock(himage);
         khrn_image_lock_wrap(image, &wrap);
         stride = image->stride / 4;

         for (y = 0; y < height; y++)
         {
            for (x = 0; x < width; x++)
            {
               /* data[y * stride + x] = 0xff000000 | x | y << 8; */
               khrn_image_wrap_put_pixel(&wrap, x, y, 0xff000000 | x | y << 8);
            }
         }

         khrn_image_unlock_wrap(image);
         mem_unlock(himage);
      }

      break;
   }
   case EGL_IMAGE_FROM_SURFACE_BRCM:
   {
      MEM_HANDLE_T shandle = khrn_map_lookup(&eglstate->surfaces, buffer);
      EGL_SERVER_SURFACE_T *surface = (EGL_SERVER_SURFACE_T *)mem_lock(shandle);
      KHRN_IMAGE_T *src;
      KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;

      src = (KHRN_IMAGE_T *)mem_lock(surface->mh_color[surface->back_buffer_index]);
      khrn_interlock_read_immediate(&src->interlock);
      khrn_image_lock_wrap(src, &src_wrap);

      himage = khrn_image_create(ABGR_8888_TF, src_wrap.width, src_wrap.height, (KHRN_IMAGE_CREATE_FLAG_T)(IMAGE_CREATE_FLAG_RENDER_TARGET|IMAGE_CREATE_FLAG_TEXTURE));
      himage_acquired = true;

      if (himage == MEM_INVALID_HANDLE)
      {
         error = EGL_BAD_ALLOC;
      }
      else
      {
         /* copy from surface ro image */
         KHRN_IMAGE_T *image;

         image = (KHRN_IMAGE_T *)mem_lock(himage);
         khrn_image_lock_wrap(image, &dst_wrap);

         khrn_image_wrap_copy_region(
         &dst_wrap, 0, 0,
         src_wrap.width, src_wrap.height,
         &src_wrap, 0, 0,
         IMAGE_CONV_GL);

         khrn_image_unlock_wrap(image);
         mem_unlock(himage);
      }

      khrn_image_unlock_wrap(src);
      mem_unlock(surface->mh_color[surface->back_buffer_index]);

      mem_unlock(shandle);
      break;
   }
#ifndef NO_OPENVG
   case EGL_VG_PARENT_IMAGE_KHR:
   {
      MEM_HANDLE_T hcontext;
      MEM_HANDLE_T hvgimage;
      VG_SERVER_STATE_T *vgstate;

      hcontext = khrn_map_lookup(&eglstate->vgcontexts, ctx);
      vcos_assert(hcontext != MEM_INVALID_HANDLE);
      vgstate = (VG_SERVER_STATE_T *)mem_lock(hcontext);

      hvgimage = vg_get_image(vgstate, (VGImage)buffer, &error);

      if (hvgimage != MEM_INVALID_HANDLE) {
         if (vg_is_child_image(hvgimage))
            error = EGL_BAD_MATCH;
         else {
            VG_IMAGE_T *vgimage = (VG_IMAGE_T *)mem_lock(hvgimage);

            if (vg_image_leak(vgimage)) {
               himage = vgimage->image;
            } else {
               error = EGL_BAD_ALLOC;
            }

            mem_unlock(hvgimage);
         }
      }

      mem_unlock(hcontext);
      break;
   }
#endif
   case EGL_GL_TEXTURE_2D_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
   case EGL_GL_RENDERBUFFER_KHR:
   {
      MEM_HANDLE_T hcontext;
      MEM_HANDLE_T hshared = MEM_INVALID_HANDLE;

      hcontext = khrn_map_lookup(&eglstate->glcontexts, ctx);
      switch (glversion) {
      case EGL_SERVER_GL11:
      {
         GLXX_SERVER_STATE_T *glstate = (GLXX_SERVER_STATE_T *)mem_lock(hcontext);

         hshared = glstate->mh_shared;

         mem_unlock(hcontext);
         break;
      }
      case EGL_SERVER_GL20:
      {
         GLXX_SERVER_STATE_T *glstate = (GLXX_SERVER_STATE_T *)mem_lock(hcontext);

         hshared = glstate->mh_shared;

         mem_unlock(hcontext);
         break;
      }
      default:
         error = EGL_BAD_MATCH;
      }
      if (hshared != MEM_INVALID_HANDLE) {
         GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(hshared);
         switch (target) {
         case EGL_GL_TEXTURE_2D_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
         case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
         {
            MEM_HANDLE_T htexture = glxx_shared_get_texture(shared, buffer);   /* returns invalid for default texture (0) */
            uint32_t buffer = convert_texture_target(target);
            bool is_cube = target != EGL_GL_TEXTURE_2D_KHR;

            if (htexture == MEM_INVALID_HANDLE)
               error = EGL_BAD_PARAMETER;
            else {
               GLXX_TEXTURE_T *texture = (GLXX_TEXTURE_T *)mem_lock(htexture);

               if (texture->is_cube != is_cube)
                  error = EGL_BAD_PARAMETER;
               else switch (glxx_texture_check_complete(texture, false))
               {
               case COMPLETE:
               {
                  uint32_t mipmap_count = glxx_texture_get_mipmap_count(texture);
                  if (texture_level < 0 || (uint32_t)texture_level >= mipmap_count)
                     error = EGL_BAD_PARAMETER;
                  else if (texture->binding_type == TEXTURE_STATE_BOUND_TEXIMAGE || texture->binding_type == TEXTURE_STATE_BOUND_EGLIMAGE)
                     error = EGL_BAD_ACCESS;
                  else {
                     /* TODO: should we reject certain textures? (e.g. paletted, ETC1) */

                     /* At this point we should succeed */
                     himage = glxx_texture_share_mipmap(texture, buffer, texture_level);

                     /*
                      * TODO: should we set the IMAGE_FLAG_BOUND_EGLIMAGE flag
                      * of the other mipmaps?
                      */
                     texture->binding_type = TEXTURE_STATE_BOUND_TEXIMAGE;
                  }
                  break;
               }
               case INCOMPLETE:
                  error = EGL_BAD_PARAMETER;
                  break;
               case OUT_OF_MEMORY:
                  error = EGL_BAD_ALLOC;
                  break;
               default:
                  UNREACHABLE();
               }

               mem_unlock(htexture);
            }
            break;
         }
         case EGL_GL_RENDERBUFFER_KHR:
         {
            MEM_HANDLE_T hrenderbuffer = glxx_shared_get_renderbuffer(shared, buffer, false);
            if (hrenderbuffer == MEM_INVALID_HANDLE)
               error = EGL_BAD_PARAMETER;
            else {
               GLXX_RENDERBUFFER_T *renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(hrenderbuffer);
               if (renderbuffer->mh_storage == MEM_INVALID_HANDLE)
                  error = EGL_BAD_PARAMETER;
               else if (!glxx_renderbuffer_unmerge(renderbuffer))
                  error = EGL_BAD_ALLOC;
               else
                  himage = renderbuffer->mh_storage;
               mem_unlock(hrenderbuffer);
            }
            break;
         }
         default:
            UNREACHABLE();
         }
         mem_unlock(hshared);
      }
      break;
   }
#ifdef EGL_BRCM_image_wrap
   case EGL_IMAGE_WRAP_BRCM:
   {
#ifdef BRCM_V3D_OPT
      MEM_HANDLE_T handle = mem_wrap( bufferPhys , buffer, buffer_height * buffer_stride, 1, 0, "EGL_IMAGE_WRAP_BRCM");
#else
      MEM_HANDLE_T handle = mem_wrap((void *)khrn_hw_unaddr(buffer), buffer_height * buffer_stride, 1, 0, "EGL_IMAGE_WRAP_BRCM");
#endif

      himage = khrn_image_create_from_storage(buffer_format,
         buffer_width, buffer_height, buffer_stride,
         MEM_INVALID_HANDLE, handle, 0, 0);
      break;
   }
#endif
   default:
      error = EGL_BAD_PARAMETER;
   }

   vcos_assert(((himage == MEM_INVALID_HANDLE) && (error != EGL_SUCCESS)) ||
      ((himage != MEM_INVALID_HANDLE) && (error == EGL_SUCCESS)));

   if (himage != MEM_INVALID_HANDLE) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(himage);
      if (image->flags & (IMAGE_FLAG_BOUND_CLIENTBUFFER|IMAGE_FLAG_BOUND_TEXIMAGE|IMAGE_FLAG_BOUND_EGLIMAGE)) {
         /*
          * Bound to an offscreen buffer, or has an offscreen buffer bound to
          * it, or is already an EGLImage sibling
          */
         mem_unlock(himage);
         error = EGL_BAD_ACCESS;
         himage = MEM_INVALID_HANDLE;
      } else {
         image->flags |= IMAGE_FLAG_BOUND_EGLIMAGE;
         mem_unlock(himage);
      }
   }

   if (error == EGL_SUCCESS) {
      EGL_IMAGE_T *eglimage;
      vcos_assert(himage != MEM_INVALID_HANDLE);

      eglimage = (EGL_IMAGE_T *)mem_lock(heglimage);
      eglimage->pid = eglstate->pid;
      MEM_ASSIGN(eglimage->mh_image, himage);
      if (himage_acquired) { mem_release(himage); }
      mem_unlock(heglimage);

      results[0] = eglstate->next_eglimage++;
   } else {
      /*
       * Remove the entry we added to the map
       */
      khrn_map_delete(&eglstate->eglimages, eglstate->next_eglimage);
      results[0] = 0;
   }
   results[1] = error;

   vcos_log("KHRN eglCreateImageKHR_impl = %d", results[0]);
   return 2;
}

/*
   Khronos documentation:

   The command

           EGLBoolean eglDestroyImageKHR(
                               EGLDisplay dpy,
                               EGLImageKHR image)

    is used to destroy the specified EGLImageKHR object <image>.  Once
    destroyed, <image> may not be used to create any additional EGLImage
    target resources within any client API contexts, although existing
    EGLImage siblings may continue to be used.  EGL_TRUE is returned
    if DestroyImageKHR succeeds, EGL_FALSE indicates failure.

       * If <dpy> is not the handle of a valid EGLDisplay object or
         EGL_NO_DISPLAY, the error EGL_BAD_DISPLAY is generated.

       * If <dpy> is not the same as the <dpy> used to create the <image>,
         then EGL_BAD_MATCH is generated.

      * If <image> is not a valid EGLImageKHR object, the error
        EGL_BAD_PARAMETER is generated."
*/

EGLBoolean eglDestroyImageKHR_impl (EGLImageKHR image)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   uint32_t id = (uint32_t)image;
   vcos_log("KHRN eglDestroyImageKHR_impl %d", image);
   return khrn_map_delete(&state->eglimages, id) ? EGL_TRUE : EGL_FALSE;
}

void eglImageSetColorData_impl(EGLImageKHR image, KHRN_IMAGE_FORMAT_T format, uint32_t width, uint32_t height, int32_t stride, uint32_t y_offset, const void *data)
{
   EGL_SERVER_STATE_T *state = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T ihandle = khrn_map_lookup(&state->eglimages, (uint32_t)image);
   EGL_IMAGE_T *egl_image = (EGL_IMAGE_T *)mem_lock(ihandle);
   KHRN_IMAGE_T *dst;
   KHRN_IMAGE_WRAP_T dst_wrap, src_wrap;

   vcos_log("KHRN eglImageSetColorData_impl %d", image);

   if (stride < 0) {
      data = (const uint8_t *)data - (stride * (height - 1));
   }
   khrn_image_wrap(&src_wrap, format, width, height, stride, (void *)data); /* casting away constness here, but we won't actually modify */

   /* TODO will this handle all necessary conversions correctly?
      Will it handle images of different sizes? */
   dst = (KHRN_IMAGE_T *)mem_lock(egl_image->mh_image);
   khrn_interlock_write_immediate(&dst->interlock);
   khrn_image_lock_wrap(dst, &dst_wrap);
   khrn_image_wrap_copy_region(
      &dst_wrap, 0, y_offset,
      width, height,
      &src_wrap, 0, 0,
      IMAGE_CONV_GL);
   khrn_image_unlock_wrap(dst);
   mem_unlock(egl_image->mh_image);

   mem_unlock(ihandle);
}
