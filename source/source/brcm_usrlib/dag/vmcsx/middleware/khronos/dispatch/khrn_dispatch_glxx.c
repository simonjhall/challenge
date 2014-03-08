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
#include "middleware/khronos/common/khrn_hw.h"

#include "interface/khronos/include/GLES2/gl2.h"
#include "interface/khronos/include/GLES2/gl2ext.h"
#include "middleware/khronos/glxx/glxx_server.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

extern bool gl_dispatch_11(uint32_t id);
extern bool gl_dispatch_20(uint32_t id);

static uint32_t get_texture_size(uint32_t w, uint32_t h, GLenum format, GLenum type, uint32_t a)
{
   uint32_t n = 0;
   uint32_t s = 0;
   uint32_t k;

   switch (format) {
   case GL_RGBA:
      switch (type) {
      case GL_UNSIGNED_BYTE:
         n = 4;
         s = 1;
         break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
         n = 1;
         s = 2;
         break;
      }
      break;
   case GL_RGB:
      switch (type) {
      case GL_UNSIGNED_BYTE:
         n = 3;
         s = 1;
         break;
      case GL_UNSIGNED_SHORT_5_6_5:
         n = 1;
         s = 2;
         break;
      }
      break;
   case GL_LUMINANCE_ALPHA:
      n = 2;
      s = 1;
      break;
   case GL_LUMINANCE:
   case GL_ALPHA:
      n = 1;
      s = 1;
      break;
   }

   if (s < a)
      k = (a / s) * ((s * n * w + a - 1) / a);
   else
      k = n * w;

   switch (format) {
   case GL_RGBA:
      switch (type) {
      case GL_UNSIGNED_BYTE:
         return h * k;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
         return h * k * 2;
      }
      break;
   case GL_RGB:
      switch (type) {
      case GL_UNSIGNED_BYTE:
         return h * k;
      case GL_UNSIGNED_SHORT_5_6_5:
         return h * k * 2;
      }
      break;
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE:
   case GL_ALPHA:
      return h * k;
   }

   return 0;      // transfer no data, format will be rejected by server
}

static void arrays_or_elements_common(GLXX_ATTRIB_T *attrib, int *keys, int *keys_count)
{
   int i, j;

   /*
      read enabled values
   */

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
      attrib[i].enabled = khdispatch_read_boolean();

   /*
      unpack attribute structures
   */

   for (i = 0; i < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; i++)
   {
      if (attrib[i].enabled) {
         attrib[i].size = khdispatch_read_int();
         attrib[i].type = khdispatch_read_enum();
         attrib[i].normalized = khdispatch_read_boolean();
         attrib[i].stride = khdispatch_read_sizei();
         attrib[i].pointer = (GLvoid *)khdispatch_read_uint();
         attrib[i].buffer = khdispatch_read_uint();
      } else
         for (j = 0; j < 4; j++)
            attrib[i].value[j] = khdispatch_read_float();      
    }        
   
   *keys_count = khdispatch_read_int();
   vcos_assert((*keys_count >= 0) && (*keys_count <= GLXX_CONFIG_MAX_VERTEX_ATTRIBS + 1));

   for (i = 0; i < *keys_count ; i++)
   {
      keys[i] = khdispatch_read_int();
   }
}

void gl_dispatch(uint32_t id)
{
   switch (id) {
   case GLACTIVETEXTURE_ID:
   {
      GLenum texture = khdispatch_read_enum();

      glActiveTexture_impl(texture);
      break;
   }
   case GLBINDBUFFER_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLuint buffer = khdispatch_read_uint();

      glBindBuffer_impl(target, buffer);
      break;
   }
   case GLBINDTEXTURE_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLuint texture = khdispatch_read_uint();

      glBindTexture_impl(target, texture);
      break;
   }
   case GLBLENDFUNCSEPARATE_ID:
   {
      GLenum srcRGB = khdispatch_read_enum();
      GLenum dstRGB = khdispatch_read_enum();
      GLenum srcAlpha = khdispatch_read_enum();
      GLenum dstAlpha = khdispatch_read_enum();

      glBlendFuncSeparate_impl(srcRGB, dstRGB, srcAlpha, dstAlpha);
      break;
   }   
   case GLBUFFERDATA_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLsizeiptr size = khdispatch_read_sizeiptr();
      GLenum usage = khdispatch_read_enum();

      uint32_t inlen = khdispatch_read_int();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glBufferData_impl(target, size, usage, indata);
      break;
   }
   case GLBUFFERSUBDATA_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLintptr offset = khdispatch_read_intptr();
      GLsizeiptr size = khdispatch_read_sizeiptr();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glBufferSubData_impl(target, offset, size, indata);
      break;
   }   
   case GLCLEAR_ID:
   {
      GLbitfield mask = khdispatch_read_bitfield();

      glClear_impl(mask);
      break;
   }
   case GLCLEARCOLOR_ID:
   {
      GLfloat red = khdispatch_read_float();
      GLfloat green = khdispatch_read_float();
      GLfloat blue = khdispatch_read_float();
      GLfloat alpha = khdispatch_read_float();

      glClearColor_impl(red, green, blue, alpha);
      break;
   }   
   case GLCLEARDEPTHF_ID:
   {
      GLfloat depth = khdispatch_read_float();

      glClearDepthf_impl(depth);
      break;
   }   
   case GLCLEARSTENCIL_ID:
   {
      GLint stencil = khdispatch_read_int();

      glClearStencil_impl(stencil);
      break;
   }
   case GLCOLORMASK_ID:
   {
      GLboolean red = khdispatch_read_boolean();
      GLboolean green = khdispatch_read_boolean();
      GLboolean blue = khdispatch_read_boolean();
      GLboolean alpha = khdispatch_read_boolean();

      glColorMask_impl(red, green, blue, alpha);
      break;
   }
   case GLCOMPRESSEDTEXIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLenum internalformat = khdispatch_read_enum();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();
      GLint border = khdispatch_read_int();
      GLsizei imageSize = khdispatch_read_sizei();
      GLboolean result;

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
      {
         result = glCompressedTexImage2D_impl(target, level, internalformat, width, height, border, imageSize, indata);

         khdispatch_write_boolean(result);
      }
      break;
   }
   case GLCOMPRESSEDTEXSUBIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLint xoffset = khdispatch_read_int();
      GLint yoffset = khdispatch_read_int();
      GLsizei width = khdispatch_read_int();
      GLsizei height = khdispatch_read_int();
      GLenum format = khdispatch_read_enum();
      GLsizei imageSize = khdispatch_read_sizei();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glCompressedTexSubImage2D_impl(target, level, xoffset, yoffset, width, height, format, imageSize, indata);
      break;
   }   
   case GLCOPYTEXIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLenum internalformat = khdispatch_read_enum();
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();
      GLint border = khdispatch_read_int();

      glCopyTexImage2D_impl(target, level, internalformat, x, y, width, height, border);
      break;
   }
   case GLCOPYTEXSUBIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLint xoffset = khdispatch_read_int();
      GLint yoffset = khdispatch_read_int();
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();

      glCopyTexSubImage2D_impl(target, level, xoffset, yoffset, x, y, width, height);
      break;
   }   
   case GLCULLFACE_ID:
   {
      GLenum mode = khdispatch_read_enum();

      glCullFace_impl(mode);
      break;
   }
   case GLDELETEBUFFERS_ID:
   {
      GLsizei n = khdispatch_read_uint();

      GLuint inlen = khdispatch_read_uint();
      void *indata = NULL;

      vcos_assert(inlen == (n > 0 ? n * sizeof(GLuint) : 0));

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glDeleteBuffers_impl(n, indata);
      break;
   }
   case GLDELETETEXTURES_ID:
   {
      GLsizei n = khdispatch_read_uint();

      GLuint inlen = khdispatch_read_uint();
      void * indata = NULL;

      vcos_assert(inlen == (n > 0 ? n * sizeof(GLuint) : 0));

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glDeleteTextures_impl(n, indata);
      break;
   }   
   case GLDEPTHFUNC_ID:
   {
      GLenum func = khdispatch_read_enum();

      glDepthFunc_impl(func);
      break;
   }
   case GLDEPTHMASK_ID:
   {
      GLboolean flag = khdispatch_read_boolean();

      glDepthMask_impl(flag);
      break;
   }
   case GLDEPTHRANGEF_ID:
   {
      GLfloat zNear = khdispatch_read_float();
      GLfloat zFar = khdispatch_read_float();

      glDepthRangef_impl(zNear, zFar);
      break;
   }      
   case GLDISABLE_ID:
   {
      GLenum cap = khdispatch_read_enum();

      glDisable_impl(cap);
      break;
   }   
   case GLDRAWELEMENTS_ID:
   {
      GLenum mode = khdispatch_read_enum();
      GLsizei count = khdispatch_read_sizei();
      GLenum type = khdispatch_read_enum();
      GLvoid *indices = (GLvoid *)khdispatch_read_uint();
      GLuint element_array = khdispatch_read_uint();
      int keys_count;

      GLXX_ATTRIB_T attrib[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
      int keys[GLXX_CONFIG_MAX_VERTEX_ATTRIBS+1];

      arrays_or_elements_common(&attrib[0],&keys[0],&keys_count);

      /*
         call implementation
      */

      glDrawElements_impl(mode, count, type,
                             indices, element_array,
                             &attrib[0],&keys[0],keys_count);

      break;
   }   
   case GLENABLE_ID:
   {
      GLenum cap = khdispatch_read_enum();

      glEnable_impl(cap);
      break;
   }
   case GLFINISH_ID:
   {
      GLuint dummy = glFinish_impl();

      khdispatch_write_uint(dummy);
      break;
   }
   case GLFLUSH_ID:
   {
      glFlush_impl();
      break;
   }      
   case GLFRONTFACE_ID:
   {
      GLenum mode = khdispatch_read_enum();

      glFrontFace_impl(mode);
      break;
   }   
   case GLGENBUFFERS_ID:
   {
      GLsizei n = khdispatch_read_sizei();
      uint32_t size = n > 0 ? n * sizeof(GLuint) : 0;

      khdispatch_check_workspace(khdispatch_pad_bulk(size));

      glGenBuffers_impl(n, (GLuint *)khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
   case GLGENTEXTURES_ID:
   {
      GLsizei n = khdispatch_read_sizei();
      uint32_t size = n > 0 ? n * sizeof(GLuint) : 0;

      khdispatch_check_workspace(khdispatch_pad_bulk(size));

      glGenTextures_impl(n, (GLuint *)khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
   case GLGETBOOLEANV_ID:
   {
      GLenum pname = khdispatch_read_enum();

      GLboolean params[16];

      int count = glGetBooleanv_impl(pname, params);

      vcos_assert(count <= 16);

      khdispatch_write_out_ctrl(params, count * sizeof(GLboolean));
      break;
   }   
   case GLGETBUFFERPARAMETERIV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetBufferParameteriv_impl(target, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETERROR_ID:
   {
      GLenum error = glGetError_impl();

      khdispatch_write_enum(error);
      break;
   }
   case GLGETFLOATV_ID:
   {
      GLenum pname = khdispatch_read_enum();

      GLfloat params[16];

      int count = glGetFloatv_impl(pname, params);

      vcos_assert(count <= 16);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }   
   case GLGETINTEGERV_ID:
   {
      GLenum pname = khdispatch_read_enum();

      GLint params[16];

      int count = glGetIntegerv_impl(pname, params);

      vcos_assert(count <= 16);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }      
   case GLGETTEXPARAMETERFV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLfloat params[1];

      int count = glGetTexParameterfv_impl(target, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }   
   case GLGETTEXPARAMETERIV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetTexParameteriv_impl(target, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }   
   case GLHINT_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum mode = khdispatch_read_enum();

      glHint_impl(target, mode);
      break;
   }   
   case GLISBUFFER_ID:
   {
      GLuint buffer = khdispatch_read_uint();

      GLboolean isbuffer = glIsBuffer_impl(buffer);

      khdispatch_write_boolean(isbuffer);
      break;
   }   
   case GLISENABLED_ID:
   {
      GLenum cap = khdispatch_read_enum();

      GLboolean enabled = glIsEnabled_impl(cap);

      khdispatch_write_boolean(enabled);
      break;
   }   
   case GLISTEXTURE_ID:
   {
      GLuint buffer = khdispatch_read_uint();

      GLboolean istexture = glIsTexture_impl(buffer);

      khdispatch_write_boolean(istexture);
      break;
   }   
   case GLLINEWIDTH_ID:
   {
      GLfloat width = khdispatch_read_float();

      glLineWidth_impl(width);
      break;
   }   
   case GLPOLYGONOFFSET_ID:
   {
      GLfloat factor = khdispatch_read_float();
      GLfloat units = khdispatch_read_float();

      glPolygonOffset_impl(factor, units);
      break;
   }   
   case GLREADPIXELS_ID:
   {
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();
      GLenum format = khdispatch_read_enum();
      GLenum type = khdispatch_read_enum();
      GLenum align = khdispatch_read_int();

      int len = get_texture_size(width, height, format, type, align);

      khdispatch_check_workspace(khdispatch_pad_bulk(len));

      glReadPixels_impl(x, y, width, height, format, type, align, khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, len);
      break;
   }   
   case GLSAMPLECOVERAGE_ID:
   {
      GLfloat value = khdispatch_read_float();
      GLboolean invert = khdispatch_read_boolean();

      glSampleCoverage_impl(value, invert);
      break;
   }   
   case GLSCISSOR_ID:
   {
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();

      glScissor_impl(x, y, width, height);
      break;
   }   
   case GLSTENCILFUNCSEPARATE_ID:
   {
      GLenum face = khdispatch_read_enum();
      GLenum func = khdispatch_read_enum();
      GLint ref = khdispatch_read_int();
      GLuint mask = khdispatch_read_uint();

      glStencilFuncSeparate_impl(face, func, ref, mask);
      break;
   }   
   case GLSTENCILMASKSEPARATE_ID:
   {
      GLenum face = khdispatch_read_enum();
      GLuint mask = khdispatch_read_uint();

      glStencilMaskSeparate_impl(face, mask);
      break;
   }
   case GLSTENCILOPSEPARATE_ID:
   {
      GLenum face = khdispatch_read_enum();
      GLenum fail = khdispatch_read_enum();
      GLenum zfail = khdispatch_read_enum();
      GLenum zpass = khdispatch_read_enum();

      glStencilOpSeparate_impl(face, fail, zfail, zpass);
      break;
   }   
   case GLTEXIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLenum internalformat = khdispatch_read_enum();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();
      GLint border = khdispatch_read_int();
      GLenum format = khdispatch_read_enum();
      GLenum type = khdispatch_read_enum();
      GLint alignment = khdispatch_read_int();
      GLboolean result;

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
      {
         result = glTexImage2D_impl(target, level, internalformat, width, height, border, format, type, alignment, indata);

         khdispatch_write_boolean(result);
      }
      break;
   }   
   case GLTEXPARAMETERF_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfloat param = khdispatch_read_float();

      glTexParameterf_impl(target, pname, param);
      break;
   }   
   case GLTEXPARAMETERI_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLint param = khdispatch_read_int();

      glTexParameteri_impl(target, pname, param);
      break;
   }
   case GLTEXPARAMETERFV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLfloat param[4];
      param[0] = khdispatch_read_float();
      param[1] = khdispatch_read_float();
      param[2] = khdispatch_read_float();
      param[3] = khdispatch_read_float();

      glTexParameterfv_impl(target, pname, param);
      break;
   }   
   case GLTEXPARAMETERIV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();
      GLint param[4];
      param[0] = khdispatch_read_int();
      param[1] = khdispatch_read_int();
      param[2] = khdispatch_read_int();
      param[3] = khdispatch_read_int();
      
      glTexParameteriv_impl(target, pname, param);
      break;
   }
   case GLTEXSUBIMAGE2D_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLint level = khdispatch_read_int();
      GLint xoffset = khdispatch_read_int();
      GLint yoffset = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();
      GLenum format = khdispatch_read_enum();
      GLenum type = khdispatch_read_enum();
      GLint alignment = khdispatch_read_int();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glTexSubImage2D_impl(target, level, xoffset, yoffset, width, height, format, type, alignment, indata);
      break;
   }      
   case GLVIEWPORT_ID:
   {
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();

      glViewport_impl(x, y, width, height);
      break;
   }
   case GLINTFINDMAX_ID:
   {
      GLsizei count = khdispatch_read_sizei();
      GLenum type = khdispatch_read_enum();
      GLvoid *indices = (GLvoid *)khdispatch_read_uint();

      int max = glintFindMax_impl(count, type, indices);

      khdispatch_write_int(max);
      break;
   }
   case GLINTCACHECREATE_ID:
   {
      GLsizei offset = khdispatch_read_sizei();

      glintCacheCreate_impl(offset);
      break;
   }
   case GLINTCACHEDELETE_ID:
   {
      GLsizei offset = khdispatch_read_sizei();

      glintCacheDelete_impl(offset);
      break;
   }
   case GLINTCACHEDATA_ID:
   {
      GLsizei offset = khdispatch_read_sizei();
      GLsizei length = khdispatch_read_sizei();

      glintCacheData_impl(offset, length, khdispatch_read_ctrl(length));
      break;
   }
   case GLINTCACHEGROW_ID:
   {
      GLboolean success = glintCacheGrow_impl();

      khdispatch_write_boolean(success);
      break;
   }
   case GLINTCACHEUSE_ID:
   {
      GLsizei count;
      GLsizei offset[9];

      count = khdispatch_read_sizei();

      vcos_assert(count <= 9);

      for (int i = 0; i < count; i++)
         offset[i] = khdispatch_read_sizei();

      glintCacheUse_impl(count, offset);
      break;
   }
   case GLEGLIMAGETARGETTEXTURE2DOES_ID: /* RPC_CALL2 (GL_OES_EGL_image) */
   {
      GLenum target = khdispatch_read_enum();
      GLeglImageOES image = khdispatch_read_eglhandle();

      glEGLImageTargetTexture2DOES_impl(target, image);
      break;
   }
   /*OES_framebuffer_object*/
   case GLBINDRENDERBUFFER_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLuint renderbuffer = khdispatch_read_uint();

      glBindRenderbuffer_impl(target, renderbuffer);
      break;
   }
   case GLDELETERENDERBUFFERS_ID: // check
   {
      GLsizei n = khdispatch_read_uint();

      GLuint inlen = khdispatch_read_uint();
      void * indata = NULL;

      vcos_assert(inlen == (n > 0 ? n * sizeof(GLuint) : 0));

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glDeleteRenderbuffers_impl(n, indata);
      break;
   }
   case GLGENRENDERBUFFERS_ID: // check
   {
      GLsizei n = khdispatch_read_sizei();
      uint32_t size = n > 0 ? n * sizeof(GLuint) : 0;

      khdispatch_check_workspace(khdispatch_pad_bulk(size));

      glGenRenderbuffers_impl(n, (GLuint *)khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
   case GLRENDERBUFFERSTORAGE_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLenum internalformat = khdispatch_read_enum();
      GLsizei width = khdispatch_read_sizei();
      GLsizei height = khdispatch_read_sizei();

      glRenderbufferStorage_impl(target, internalformat, width, height);
      break;
   }
   case GLBINDFRAMEBUFFER_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLuint framebuffer = khdispatch_read_uint();

      glBindFramebuffer_impl(target, framebuffer);
      break;
   }
   case GLDELETEFRAMEBUFFERS_ID: // check
   {
      GLsizei n = khdispatch_read_uint();

      GLuint inlen = khdispatch_read_uint();
      void * indata = NULL;

      vcos_assert(inlen == (n > 0 ? n * sizeof(GLuint) : 0));

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glDeleteFramebuffers_impl(n, indata);
      break;
   }
   case GLGENFRAMEBUFFERS_ID: // check
   {
      GLsizei n = khdispatch_read_sizei();
      uint32_t size = n > 0 ? n * sizeof(GLuint) : 0;

      khdispatch_check_workspace(khdispatch_pad_bulk(size));

      glGenFramebuffers_impl(n, (GLuint *)khdispatch_workspace);

      khdispatch_write_out_bulk(khdispatch_workspace, size);
      break;
   }
   case GLCHECKFRAMEBUFFERSTATUS_ID: // check
   {
      GLenum target = khdispatch_read_enum();

      GLenum result = glCheckFramebufferStatus_impl(target);

      khdispatch_write_enum(result);
      break;
   }
   case GLFRAMEBUFFERTEXTURE2D_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLenum attachment = khdispatch_read_enum();
      GLenum textarget = khdispatch_read_enum();
      GLuint texture = khdispatch_read_uint();
      GLint level = khdispatch_read_int();

      glFramebufferTexture2D_impl(target, attachment, textarget, texture, level);
      break;
   }
   case GLFRAMEBUFFERRENDERBUFFER_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLenum attachment = khdispatch_read_enum();
      GLenum renderbuffertarget = khdispatch_read_enum();
      GLuint renderbuffer = khdispatch_read_uint();

      glFramebufferRenderbuffer_impl(target, attachment, renderbuffertarget, renderbuffer);
      break;
   }
   case GLGENERATEMIPMAP_ID: // check
   {
      GLenum target = khdispatch_read_enum();

      glGenerateMipmap_impl(target);
      break;
   }
   case GLGETRENDERBUFFERPARAMETERIV_ID: // check
   {
      GLenum target = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetRenderbufferParameteriv_impl(target, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETFRAMEBUFFERATTACHMENTPARAMETERIV_ID:
   {
      GLenum target = khdispatch_read_enum();
      GLenum attachment = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetFramebufferAttachmentParameteriv_impl(target, attachment, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLISFRAMEBUFFER_ID: // check
   {
      GLuint framebuffer = khdispatch_read_uint();

      GLboolean result = glIsFramebuffer_impl(framebuffer);

      khdispatch_write_boolean(result);
      break;
   }
   case GLISRENDERBUFFER_ID: // check
   {
      GLuint renderbuffer = khdispatch_read_uint();

      GLboolean result = glIsRenderbuffer_impl(renderbuffer);

      khdispatch_write_boolean(result);
      break;
   }
   default:
      UNREACHABLE();
      break;
   }
}