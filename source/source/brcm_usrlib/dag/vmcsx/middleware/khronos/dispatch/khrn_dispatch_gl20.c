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

#include "middleware/khronos/gl20/gl20_server.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

bool gl_dispatch_20(uint32_t id)
{
   bool result = true;
   
   switch (id) {
   //gl 2.0 specific   
   case GLSHADERSOURCE_ID_20:
   {
      GLuint shader = khdispatch_read_uint();
      GLsizei count = khdispatch_read_sizei();
      GLint total = khdispatch_read_int();
      GLboolean has_length = khdispatch_read_boolean();

      khdispatch_check_workspace(total);

      uint8_t **string = (uint8_t **)khdispatch_workspace;
      GLint *length = (GLint *)((uint8_t *)khdispatch_workspace + khdispatch_pad_bulk(4 * count));
#ifdef UIDECK
      uint8_t *pos = (uint8_t *)khdispatch_workspace + khdispatch_pad_bulk(4 * count) + khdispatch_pad_bulk(4 * count);

      if (has_length) {
         void * indata = NULL;
         if(!khdispatch_read_in_bulk(length, count * sizeof(GLint),&indata))
            goto fail_shadersource;
         vcos_assert(length == indata);
      }

      for (int i = 0; i < count; i++) {
         void * indata = NULL;
         if (!has_length || length[i] < 0)
            length[i] = khdispatch_read_int();

         if(!khdispatch_read_in_bulk(pos, length[i],&indata))
            goto fail_shadersource;

         string[i] = pos;

         pos += khdispatch_pad_bulk(length[i]);
      }

      vcos_assert(pos - (uint8_t *)khdispatch_workspace == total);

      glShaderSource_impl_20(shader, count, (const char **)string, length);
#else
      GLint *len = (GLint *)((uint8_t *)khdispatch_workspace + khdispatch_pad_bulk(4 * count) + khdispatch_pad_bulk(4 * count));

      uint8_t *pos = (uint8_t *)khdispatch_workspace + khdispatch_pad_bulk(4 * count) + khdispatch_pad_bulk(4 * count) + khdispatch_pad_bulk(sizeof(GLint));

      if (has_length)
      {
         void * indata = NULL;
         if(!khdispatch_read_in_bulk(length, count * sizeof(GLint),&indata))
            goto fail_shadersource;
         vcos_assert(length == indata);
      }

      for (int i = 0; i < count; i++) {
         void * indata = NULL;
         if (!has_length || length[i] < 0) {
            if(!khdispatch_read_in_bulk(len, 4, &indata))
               goto fail_shadersource;
            vcos_assert(len == indata);
         } else
            *len = length[i];

         if(!khdispatch_read_in_bulk(pos, *len,&indata))
            goto fail_shadersource;

         string[i] = pos;

         pos += khdispatch_pad_bulk(*len);
      }

      vcos_assert(pos - (uint8_t *)khdispatch_workspace == total);

      glShaderSource_impl_20(shader, count, (const char **)string, has_length ? length : NULL);
#endif
fail_shadersource:
      break;
   }
   case GLATTACHSHADER_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLuint shader = khdispatch_read_uint();

      glAttachShader_impl_20(program, shader);
      break;
   }
   case GLBINDATTRIBLOCATION_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLuint index = khdispatch_read_uint();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
         glBindAttribLocation_impl_20(program, index, indata);
      break;
   }
   case GLBLENDCOLOR_ID_20: // check
   {
      GLfloat red = khdispatch_read_float();
      GLfloat green = khdispatch_read_float();
      GLfloat blue = khdispatch_read_float();
      GLfloat alpha = khdispatch_read_float();

      glBlendColor_impl_20(red, green, blue, alpha);
      break;
   }
   case GLBLENDEQUATIONSEPARATE_ID_20: // check
   {
      GLenum modeRGB = khdispatch_read_enum();
      GLenum modeAlpha = khdispatch_read_enum();

      glBlendEquationSeparate_impl_20(modeRGB, modeAlpha);
      break;
   }
   case GLCREATEPROGRAM_ID_20: // check
   {
      GLuint ret = glCreateProgram_impl_20();

      khdispatch_write_uint(ret);
      break;
   }
   case GLCREATESHADER_ID_20: // check
   {
      GLenum type = khdispatch_read_enum();

      GLuint ret = glCreateShader_impl_20(type);

      khdispatch_write_uint(ret);
      break;
   }
   case GLDELETEPROGRAM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      glDeleteProgram_impl_20(program);
      break;
   }
   case GLDELETESHADER_ID_20: // check
   {
      GLuint shader = khdispatch_read_uint();

      glDeleteShader_impl_20(shader);
      break;
   }
   case GLDETACHSHADER_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLuint shader = khdispatch_read_uint();

      glDetachShader_impl_20(program, shader);
      break;
   }
   case GLGETACTIVEATTRIB_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLuint index = khdispatch_read_uint();
      GLsizei bufsize = khdispatch_read_sizei();

      GLuint result[3] = {0,0,0};

      khdispatch_check_workspace(khdispatch_pad_bulk(_max(1, bufsize)));

      glGetActiveAttrib_impl_20(program, index, bufsize, (GLsizei *)&result[0], (GLint *)&result[1], (GLenum *)&result[2], khdispatch_workspace);

      khdispatch_write_out_ctrl(result, 3 * sizeof(GLuint));
      khdispatch_write_out_bulk(khdispatch_workspace, result[0] + 1);
      break;
   }
   case GLGETACTIVEUNIFORM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLuint index = khdispatch_read_uint();
      GLsizei bufsize = khdispatch_read_sizei();

      GLuint result[3] = {0,0,0};

      khdispatch_check_workspace(khdispatch_pad_bulk(_max(1, bufsize)));

      glGetActiveUniform_impl_20(program, index, bufsize, (GLsizei *)&result[0], (GLint *)&result[1], (GLenum *)&result[2], khdispatch_workspace);

      khdispatch_write_out_ctrl(result, 3 * sizeof(GLuint));
      khdispatch_write_out_bulk(khdispatch_workspace, result[0] + 1);
      break;
   }
   case GLGETATTACHEDSHADERS_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLsizei maxcount = khdispatch_read_sizei();

      GLuint result[3];

      glGetAttachedShaders_impl_20(program, maxcount, (GLsizei *)&result[0], &result[1]);

      khdispatch_write_out_ctrl(result, 3 * sizeof(GLuint));
      break;
   }
   case GLGETATTRIBLOCATION_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
      {
         int result = glGetAttribLocation_impl_20(program, indata);

         khdispatch_write_int(result);
      }
      break;
   }

   case GLGETPROGRAMIV_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetProgramiv_impl_20(program, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETPROGRAMINFOLOG_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLsizei bufsize = khdispatch_read_sizei();

      GLuint result[1] = {0};

      khdispatch_check_workspace(khdispatch_pad_bulk(_max(1, bufsize)));

      glGetProgramInfoLog_impl_20(program, bufsize, (GLsizei *)&result[0], khdispatch_workspace);

      khdispatch_write_out_ctrl(result, 1 * sizeof(GLuint));
      khdispatch_write_out_bulk(khdispatch_workspace, result[0] + 1);
      break;
   }
   case GLGETUNIFORMFV_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLint location = khdispatch_read_int();

      GLfloat params[16];

      int count = glGetUniformfv_impl_20(program, location, params);

      vcos_assert( count <= 16 );

      khdispatch_write_out_ctrl(params, count * sizeof(GLfloat));
      break;
   }
   case GLGETUNIFORMIV_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();
      GLint location = khdispatch_read_int();

      GLint params[16];

      int count = glGetUniformiv_impl_20(program, location, params);

      vcos_assert( count <= 16 );

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETUNIFORMLOCATION_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      uint32_t inlen = khdispatch_read_uint();
      void * indata = NULL;

      khdispatch_check_workspace(khdispatch_pad_bulk(inlen));

      if(khdispatch_read_in_bulk(khdispatch_workspace, inlen, &indata))
      {
         int result = glGetUniformLocation_impl_20(program, indata);

         khdispatch_write_int(result);
      }
      break;
   }
   case GLISPROGRAM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      GLboolean result = glIsProgram_impl_20(program);

      khdispatch_write_boolean(result);
      break;
   }
   case GLISSHADER_ID_20: // check
   {
      GLuint shader = khdispatch_read_uint();

      GLboolean result = glIsShader_impl_20(shader);

      khdispatch_write_boolean(result);
      break;
   }
   case GLLINKPROGRAM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      glLinkProgram_impl_20(program);
      break;
   }
   case GLPOINTSIZE_ID_20: // check
   {
      GLfloat size = khdispatch_read_float();

      glPointSize_impl_20(size);
      break;
   }
   case GLUNIFORM1I_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLint x = khdispatch_read_int();

      glUniform1i_impl_20(location, x);
      break;
   }
   case GLUNIFORM2I_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();

      glUniform2i_impl_20(location, x, y);
      break;
   }
   case GLUNIFORM3I_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLint z = khdispatch_read_int();

      glUniform3i_impl_20(location, x, y, z);
      break;
   }
   case GLUNIFORM4I_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLint x = khdispatch_read_int();
      GLint y = khdispatch_read_int();
      GLint z = khdispatch_read_int();
      GLint w = khdispatch_read_int();

      glUniform4i_impl_20(location, x, y, z, w);
      break;
   }
   case GLUNIFORM1F_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLfloat x = khdispatch_read_float();

      glUniform1f_impl_20(location, x);
      break;
   }
   case GLUNIFORM2F_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();

      glUniform2f_impl_20(location, x, y);
      break;
   }
   case GLUNIFORM3F_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();
      GLfloat z = khdispatch_read_float();

      glUniform3f_impl_20(location, x, y, z);
      break;
   }
   case GLUNIFORM4F_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLfloat x = khdispatch_read_float();
      GLfloat y = khdispatch_read_float();
      GLfloat z = khdispatch_read_float();
      GLfloat w = khdispatch_read_float();

      glUniform4f_impl_20(location, x, y, z, w);
      break;
   }
   case GLUNIFORM1IV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform1iv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM2IV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform2iv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM3IV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform3iv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM4IV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform4iv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM1FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform1fv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM2FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform2fv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM3FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform3fv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORM4FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();

      GLuint inlen = khdispatch_read_uint();

      glUniform4fv_impl_20(location, count, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORMMATRIX2FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();
      GLboolean transpose = khdispatch_read_boolean();

      GLuint inlen = khdispatch_read_uint();

      glUniformMatrix2fv_impl_20(location, count, transpose, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORMMATRIX3FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();
      GLboolean transpose = khdispatch_read_boolean();

      GLuint inlen = khdispatch_read_uint();

      glUniformMatrix3fv_impl_20(location, count, transpose, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUNIFORMMATRIX4FV_ID_20: // check
   {
      GLint location = khdispatch_read_int();
      GLsizei count = khdispatch_read_sizei();
      GLboolean transpose = khdispatch_read_boolean();

      GLuint inlen = khdispatch_read_uint();

      glUniformMatrix4fv_impl_20(location, count, transpose, 0, khdispatch_read_ctrl(inlen));
      break;
   }
   case GLUSEPROGRAM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      glUseProgram_impl_20(program);
      break;
   }
   case GLVALIDATEPROGRAM_ID_20: // check
   {
      GLuint program = khdispatch_read_uint();

      glValidateProgram_impl_20(program);
      break;
   }
   case GLCOMPILESHADER_ID_20: // check
   {
      GLuint shader = khdispatch_read_uint();

      glCompileShader_impl_20(shader);
      break;
   }
   case GLGETSHADERIV_ID_20: // check
   {
      GLenum shader = khdispatch_read_enum();
      GLenum pname = khdispatch_read_enum();

      GLint params[1];

      int count = glGetShaderiv_impl_20(shader, pname, params);

      vcos_assert(count <= 1);

      khdispatch_write_out_ctrl(params, count * sizeof(GLint));
      break;
   }
   case GLGETSHADERINFOLOG_ID_20: // check
   {
      GLuint shader = khdispatch_read_uint();
      GLsizei bufsize = khdispatch_read_sizei();

      GLuint result[1] = {0};

      khdispatch_check_workspace(khdispatch_pad_bulk(_max(1, bufsize)));

      glGetShaderInfoLog_impl_20(shader, bufsize, (GLsizei *)&result[0], khdispatch_workspace);

      khdispatch_write_out_ctrl(result, 1 * sizeof(GLuint));
      khdispatch_write_out_bulk(khdispatch_workspace, result[0] + 1);
      break;
   }
   case GLGETSHADERSOURCE_ID_20: // check
   {
      GLuint shader = khdispatch_read_uint();
      GLsizei bufsize = khdispatch_read_sizei();

      GLuint result[1] = {0};

      khdispatch_check_workspace(khdispatch_pad_bulk(_max(1, bufsize)));

      glGetShaderSource_impl_20(shader, bufsize, (GLsizei *)&result[0], khdispatch_workspace);

      khdispatch_write_out_ctrl(result, 1 * sizeof(GLuint));
      khdispatch_write_out_bulk(khdispatch_workspace, result[0] + 1);
      break;
   }
   case GLGETSHADERPRECISIONFORMAT_ID_20:
   {
      GLenum shadertype = khdispatch_read_enum();
      GLenum precisiontype = khdispatch_read_enum();

      GLint result[3];

      glGetShaderPrecisionFormat_impl_20(shadertype, precisiontype, result);

      khdispatch_write_out_ctrl(result, 3 * sizeof(GLuint));
      break;
   }
   case GLVERTEXATTRIBPOINTER_ID_20:
   {
      GLuint index = khdispatch_read_uint();

      glVertexAttribPointer_impl_20(index);
      break;
   }
   case GLEGLIMAGETARGETRENDERBUFFERSTORAGEOES_ID_20: /* RPC_CALL2 (GL_OES_EGL_image) */
   {
      GLenum target = khdispatch_read_enum();
      GLeglImageOES image = khdispatch_read_eglhandle();

      glEGLImageTargetRenderbufferStorageOES_impl_20(target, image);
      break;
   }
   default:
      result = false;
      break;
   }
   return result;
}
