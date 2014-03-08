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
#include "middleware/khronos/glxx/glxx_server.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/egl/egl_server.h"

#include "middleware/khronos/ext/egl_khr_image.h"
#include "interface/khronos/include/GLES2/gl2.h"
#include "interface/khronos/include/GLES2/gl2ext.h"
#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES/glext.h"

/*
   void glEGLImageTargetTexture2DOES_impl (GLenum target, GLeglImageOES image)

   Khronos documentation:
Another way of specifying two-dimensional texture images is to
    reference them from existing EGLImage objects.  Images specified
    this way will be EGLImage siblings with the original EGLImage
    source and any other EGLImage targets.

    The command

        void EGLImageTargetTexture2DOES(enum target, eglImageOES image);

    defines an entire two-dimensional texture array.  All properties
    of the texture images (including width, height, format, border, mipmap
    levels of detail, and image data) are taken from the specified
    eglImageOES <image>, rather than from the client or the framebuffer.
    Any existing image arrays associated with any mipmap levels in the texture
    object are freed (as if TexImage was called for each, with an image of
    zero size).  As a result of this referencing operation, all of the pixel
    data in the <buffer> used as the EGLImage source resource (i.e., the
    <buffer> parameter passed to the CreateImageOES command that returned
    <image>) will become undefined.

    Currently, <target> must be TEXTURE_2D.  <image> must be the
    handle of a valid EGLImage resource, cast into the type eglImageOES.
    Assuming no errors are generated in EGLImageTargetTexture2DOES, the
    newly specified texture object will be an EGLImage target of the
    specified  eglImageOES.  If an application later respecifies any image
    array in the texture object (through mechanisms such as calls to
    TexImage2D and/or GenerateMipmapOES, or setting the
    SGIS_GENERATE_MIPMAP parameter to TRUE), implementations should allocate
    additional space for all specified (and respecified) image arrays,
    and copy any existing image data to the newly (re)specified texture
    object (as if TexImage was called for every level-of-detail in the
    texture object).  The respecified texture object will not be an
    EGLImage target.

    If the GL is unable to specify a texture object using the supplied
    eglImageOES <image> (if, for example, <image> refers to a multisampled
    eglImageOES), the error INVALID_OPERATION is generated.

    If <target> is not TEXTURE_2D, the error INVALID_ENUM is generated.
*/

void glEGLImageTargetTexture2DOES_impl (GLenum target, GLeglImageOES image)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   EGL_SERVER_STATE_T *eglstate = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T heglimage = khrn_map_lookup(&eglstate->eglimages, (uint32_t)image);
   
   if (heglimage == MEM_INVALID_HANDLE)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else if (target != GL_TEXTURE_2D && target != GL_TEXTURE_EXTERNAL_OES)   /* can only create 2D textures this way */
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   else {
      MEM_HANDLE_T htexture;
      GLXX_TEXTURE_T *texture = glxx_server_state_get_texture(state, target, GL_FALSE, &htexture);
      vcos_assert(!texture->is_cube);
   
      if (texture) {
         //VC_IMAGE_T vc_image;//for debugging
         KHRN_IMAGE_T *image;
         MEM_HANDLE_T himage = ((EGL_IMAGE_T *)mem_lock(heglimage))->mh_image;
         mem_unlock(heglimage);

         image = (KHRN_IMAGE_T *)mem_lock(himage);

         //debugging
         //if(image->format!=ABGR_8888_LT)
         //{
         //   khrn_image_lock_vcimage(image, &vc_image);
         //   khrn_image_unlock_vcimage(image);
         //}
         //

         if (glxx_texture_is_valid_image(image))
            glxx_texture_bind_images(texture, 1, &himage, TEXTURE_STATE_BOUND_EGLIMAGE, MEM_INVALID_HANDLE, 0);
         else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);

         mem_unlock(himage);
         mem_unlock(htexture);
      }
   }
   
   GLXX_UNLOCK_SERVER_STATE();
}

/*
   void glEGLImageTargetRenderbufferStorageOES_impl_20 (GLenum target, GLeglImageOES image)

   Khronos documentation:
   
   The command

        void EGLImageTargetRenderbufferStorageOES(enum target,
                                                  eglImageOES image)

    establishes the data storage, format, and dimensions of a
    renderbuffer object's image, using the parameters and storage
    associated with the eglImageOES <image>.  Assuming no errors
    are generated in this process, the resulting renderbuffer
    will be an EGLImage target of the specifed eglImageOES <image>.
    The renderbuffer will remain an EGLImage target until it is
    deleted or respecified by a call to
    {Reference}RenderbufferStorageOES.  As a result of this referencing
    operation, all of the pixel data in the <buffer> used as the
    EGLImage source resource (i.e., the <buffer> parameter passed to
    the CreateImageOES command that returned <image>) will become
    undefined.

    <target> must be RENDERBUFFER_OES, and <image> must be the
    handle of a valid EGLImage resource, cast into the type
    eglImageOES.

    If the GL is unable to create a renderbuffer using the specified
    eglImageOES, the error INVALID_OPERATION is generated.  If <image>
    does not refer to a valid eglImageOES object, the error
    INVALID_VALUE is generated.
   
    Dependencies on OES_framebuffer_object

    If the OES_framebuffer_object extension is not supported, all
    discussion of renderbuffers should be ignored, and all calls to
    EGLImageTargetRenderbufferStorageOES should generate the error
    INVALID_OPERATION.
*/

void glEGLImageTargetRenderbufferStorageOES_impl_20 (GLenum target, GLeglImageOES image)
{
   GLXX_SERVER_STATE_T *state = GLXX_LOCK_SERVER_STATE();
   EGL_SERVER_STATE_T *eglstate = EGL_GET_SERVER_STATE();
   MEM_HANDLE_T heglimage = khrn_map_lookup(&eglstate->eglimages, (uint32_t)image);
   
   if (heglimage == MEM_INVALID_HANDLE)
      glxx_server_state_set_error(state, GL_INVALID_VALUE);
   else if (state->mh_bound_renderbuffer == MEM_INVALID_HANDLE) 
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);
   else if (target != GL_RENDERBUFFER)
      glxx_server_state_set_error(state, GL_INVALID_ENUM);
   else {
      GLXX_RENDERBUFFER_T *renderbuffer;
      MEM_HANDLE_T himage = ((EGL_IMAGE_T *)mem_lock(heglimage))->mh_image;
      mem_unlock(heglimage);

      renderbuffer = (GLXX_RENDERBUFFER_T *)mem_lock(state->mh_bound_renderbuffer);
   
      if (!glxx_renderbuffer_bind_image(renderbuffer, himage))
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);

      mem_unlock(state->mh_bound_renderbuffer);
   }
   
   GLXX_UNLOCK_SERVER_STATE();
}
