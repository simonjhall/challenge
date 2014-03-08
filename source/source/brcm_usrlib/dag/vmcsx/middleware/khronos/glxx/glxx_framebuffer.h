/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_FRAMEBUFFER_H
#define GLXX_FRAMEBUFFER_H

#include "interface/khronos/include/GLES2/gl2.h"
#include "interface/khronos/include/GLES/gl.h"
#include "interface/khronos/include/GLES/glext.h"

#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"

typedef struct {
   GLenum type;
   GLenum target;
   GLint level;

   MEM_HANDLE_T mh_object;
} GLXX_ATTACHMENT_INFO_T;

typedef struct {
   int32_t name;

   struct {
      GLXX_ATTACHMENT_INFO_T color;
      GLXX_ATTACHMENT_INFO_T depth;
      GLXX_ATTACHMENT_INFO_T stencil;
   } attachments;
} GLXX_FRAMEBUFFER_T;

extern void glxx_framebuffer_init(GLXX_FRAMEBUFFER_T *framebuffer, int32_t name);
extern void glxx_framebuffer_term(void *v, uint32_t size);

extern MEM_HANDLE_T glxx_attachment_info_get_image(GLXX_ATTACHMENT_INFO_T *attachment);

extern GLenum glxx_framebuffer_check_status(GLXX_FRAMEBUFFER_T *framebuffer);

extern bool glxx_framebuffer_hw_support(KHRN_IMAGE_FORMAT_T format);

#endif