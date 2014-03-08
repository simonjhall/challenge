/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/*
   Khronos spec bugs/ambiguity:

   The following sentence makes no sense:
   "If texture is not zero,
   then texture must either name an existing texture object with an target of textarget,
   or texture must name an existing cube map texture and textarget must
   be one of: TEXTURE CUBE MAP POSITIVE X, TEXTURE CUBE MAP POSITIVE Y,
   TEXTURE CUBE MAP POSITIVE Z, TEXTURE CUBE MAP NEGATIVE X,
   TEXTURE CUBE MAP NEGATIVE Y, or TEXTURE CUBE MAP NEGATIVE Z. Otherwise,
   INVALID OPERATION is generated."

   I assume it means:
   "If texture is not zero,
   then texture must either name an existing 2d texture and textarget must be TEXTURE_2D,
   or [as before]"
*/

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/glxx/glxx_shared.h"

#include "middleware/khronos/gl20/gl20_server.h"
#include "middleware/khronos/glxx/glxx_server_internal.h"

#include "middleware/khronos/gl20/gl20_program.h"
#include "middleware/khronos/gl20/gl20_shader.h"
#include "middleware/khronos/glxx/glxx_renderbuffer.h"
#include "middleware/khronos/glxx/glxx_framebuffer.h"
#include "interface/khronos/include/GLES2/gl2ext.h"

#include "middleware/khronos/glxx/glxx_texture.h"
#include "middleware/khronos/glxx/glxx_buffer.h"
#include "interface/khronos/common/khrn_int_util.h"
#include "middleware/khronos/common/khrn_interlock.h"
#include "middleware/khronos/common/khrn_hw.h"

#include <string.h>
#include <math.h>
#include <limits.h>

static int get_uniform_internal(GLuint p, GLint location, const void *v, GLboolean is_float);
static const char *lock_shader_info_log(GL20_SHADER_T *shader);
static void unlock_shader_info_log(GL20_SHADER_T *shader);



bool gl20_server_state_init(GLXX_SERVER_STATE_T *state, uint32_t name, uint64_t pid, MEM_HANDLE_T shared)
{
   state->type = OPENGL_ES_20;

   //initialise common portions of state
   if(!glxx_server_state_init(state, name, pid, shared))
      return false;

   //gl 2.0 specific parts


   vcos_assert(state->mh_program == MEM_INVALID_HANDLE);

   state->point_size = 1.0f;

   return true;
}

void glPointSize_impl_20 (GLfloat size) // S
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   size = clean_float(size);

   if (size > 0.0f)
      state->point_size = size;
   else
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

   GL20_UNLOCK_SERVER_STATE();
}


////




/*
   Get a program pointer from a program name. Optionally retrieve
   the handle to the memory block storing the program structure.
   Gives GL_INVALID_VALUE if no object exists with that name, or
   GL_INVALID_OPERATION if the object is actually a shader and
   returns NULL in either case.
*/

static GL20_PROGRAM_T *get_program(GLXX_SERVER_STATE_T *state, GLuint p, MEM_HANDLE_T *handle)
{
   GL20_PROGRAM_T *program;
   MEM_HANDLE_T phandle = glxx_shared_get_pobject((GLXX_SHARED_T *)mem_lock(state->mh_shared), p);
   mem_unlock(state->mh_shared);

   if (phandle == MEM_INVALID_HANDLE) {
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      return NULL;
   }

   program = (GL20_PROGRAM_T *)mem_lock(phandle);

   vcos_assert(program);

   if (!gl20_is_program(program)) {
      vcos_assert(gl20_is_shader((GL20_SHADER_T *)program));

      glxx_server_state_set_error(state, GL_INVALID_OPERATION);

      mem_unlock(phandle);

      return NULL;
   }

   if (handle)
      *handle = phandle;

   return program;
}

/*
   Get a shader pointer from a shader name. Optionally retrieve
   the handle to the memory block storing the shader structure.
   Gives GL_INVALID_VALUE if no object exists with that name, or
   GL_INVALID_OPERATION if the object is actually a program and
   returns NULL in either case.
*/

static GL20_SHADER_T *get_shader(GLXX_SERVER_STATE_T *state, GLuint s, MEM_HANDLE_T *handle)
{
   GL20_SHADER_T *shader;
   MEM_HANDLE_T shandle = glxx_shared_get_pobject((GLXX_SHARED_T *)mem_lock(state->mh_shared), s);
   mem_unlock(state->mh_shared);

   if (shandle == MEM_INVALID_HANDLE) {
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      return NULL;
   }

   shader = (GL20_SHADER_T *)mem_lock(shandle);

   vcos_assert(shader);

   if (!gl20_is_shader(shader)) {
      vcos_assert(gl20_is_program((GL20_PROGRAM_T *)shader));

      glxx_server_state_set_error(state, GL_INVALID_OPERATION);

      mem_unlock(shandle);

      return NULL;
   }

   if (handle)
      *handle = shandle;

   return shader;
}


/*
   glAttachShader()

   Attach a fragment or vertex shader to a program object. We make use of the
   ES restriction that a program may only have a single shader of each type
   attached. Gives GL_INVALID_VALUE error if either program or shader is not
   a valid 'program object', or GL_INVALID_OPERATION if both are valid but
   either is of the wrong type.

   Implementation: Done
   Error Checks: Done
*/

void glAttachShader_impl_20 (GLuint p, GLuint s)
{
   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   program = get_program(state, p, &phandle);

   if (program) {
      MEM_HANDLE_T shandle;
      GL20_SHADER_T *shader = get_shader(state, s, &shandle);

      if (shader) {
         MEM_HANDLE_T *pmh_shader = NULL;

         switch (shader->type) {
         case GL_VERTEX_SHADER:
            pmh_shader = &program->mh_vertex;
            break;
         case GL_FRAGMENT_SHADER:
            pmh_shader = &program->mh_fragment;
            break;
         default:
            UNREACHABLE();
            break;
         }

         if (*pmh_shader != MEM_INVALID_HANDLE)
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         else {
            gl20_shader_acquire(shader);

            MEM_ASSIGN(*pmh_shader, shandle);
         }

         mem_unlock(shandle);
      }

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

void glBindAttribLocation_impl_20 (GLuint p, GLuint index, const char *name)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   if (name)
   {
      if (index < GLXX_CONFIG_MAX_VERTEX_ATTRIBS)
         if (strncmp(name, "gl_", 3)) {
            MEM_HANDLE_T phandle;
            GL20_PROGRAM_T *program = get_program(state, p, &phandle);

            if (program) {
               if (!gl20_program_bind_attrib(program, index, name))
                  glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

               mem_unlock(phandle);
            }
         } else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
   }

   GL20_UNLOCK_SERVER_STATE();
}





/*
   glBlendColor()

   Sets the constant color for use in blending. All inputs are clamped to the
   range [0.0, 1.0] before being stored. No errors are generated.

   Implementation: Done
   Error Checks: Done
*/

void glBlendColor_impl_20 (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) // S
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   state->blend_color[0] = clampf(red, 0.0f, 1.0f);
   state->blend_color[1] = clampf(green, 0.0f, 1.0f);
   state->blend_color[2] = clampf(blue, 0.0f, 1.0f);
   state->blend_color[3] = clampf(alpha, 0.0f, 1.0f);

   GL20_UNLOCK_SERVER_STATE();
}
/*
void glBlendEquation_impl_20 ( GLenum mode ) // S
{
   UNREACHABLE();
}
*/

/*
   Check if 'mode' is a valid blend equation enum.
*/

static GLboolean is_blend_equation(GLenum mode)
{
   return mode == GL_FUNC_ADD ||
          mode == GL_FUNC_SUBTRACT ||
          mode == GL_FUNC_REVERSE_SUBTRACT;
}

/*
   glBlendEquationSeparate()

   Sets the RGB and alpha blend equations to one of ADD, SUBTRACT or REVERSE_SUBTRACT.
   Gives GL_INVALID_ENUM error if either equation is not one of these.

   Implementation: Done
   Error Checks: Done
*/

void glBlendEquationSeparate_impl_20 (GLenum modeRGB, GLenum modeAlpha) // S
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   if (is_blend_equation(modeRGB) && is_blend_equation(modeAlpha)) {
      state->changed_backend = true;
      state->blend_equation.rgb = modeRGB;
      state->blend_equation.alpha = modeAlpha;
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL20_UNLOCK_SERVER_STATE();
}

/*
   glCreateProgram()

   Creates a new, empty program and returns its name. No errors are generated.

   Implementation: Done
   Error Checks: Done
*/

GLuint glCreateProgram_impl_20 (void)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   GLuint result = glxx_shared_create_program((GLXX_SHARED_T *)mem_lock(state->mh_shared));
   mem_unlock(state->mh_shared);

   if (result == 0)
      glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

   GL20_UNLOCK_SERVER_STATE();

   return result;
}

/*
   glCreateShader()

   Creates a new, empty shader and returns its name. Gives GL_INVALID_ENUM if
   type is not one of GL_VERTEX_SHADER or GL_FRAGMENT_SHADER.

   Implementation: Done
   Error Checks: Done
*/

static bool is_shader_type(GLenum type)
{
   return type == GL_VERTEX_SHADER ||
          type == GL_FRAGMENT_SHADER;
}

GLuint glCreateShader_impl_20 (GLenum type)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   GLuint result = 0;

   if (is_shader_type(type)) {
      result = glxx_shared_create_shader((GLXX_SHARED_T *)mem_lock(state->mh_shared), type);
      mem_unlock(state->mh_shared);

      if (result == 0)
         glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL20_UNLOCK_SERVER_STATE();

   return result;
}



static void try_delete_shader(GLXX_SHARED_T *shared, GL20_SHADER_T *shader)
{
   if (shader->refs == 0 && shader->deleted)
      glxx_shared_delete_pobject(shared, shader->name);
}

static void release_shader(GLXX_SHARED_T *shared, MEM_HANDLE_T handle)
{
   if (handle != MEM_INVALID_HANDLE) {
      GL20_SHADER_T *shader;

      mem_acquire(handle);

      shader = (GL20_SHADER_T *)mem_lock(handle);

      vcos_assert(gl20_is_shader(shader));

      gl20_shader_release(shader);

      try_delete_shader(shared, shader);

      mem_unlock(handle);
      mem_release(handle);
   }
}

static void try_delete_program(GLXX_SHARED_T *shared, GL20_PROGRAM_T *program)
{
   if (program->refs == 0 && program->deleted) {
      release_shader(shared, program->mh_vertex);
      release_shader(shared, program->mh_fragment);

      glxx_shared_delete_pobject(shared, program->name);
   }
}

static void release_program(GLXX_SHARED_T *shared, MEM_HANDLE_T handle)
{
   if (handle != MEM_INVALID_HANDLE) {
      GL20_PROGRAM_T *program;

      mem_acquire(handle);

      program = (GL20_PROGRAM_T *)mem_lock(handle);

      vcos_assert(gl20_is_program(program));

      gl20_program_release(program);

      try_delete_program(shared, program);

      mem_unlock(handle);
      mem_release(handle);
   }
}

/*
   glDeleteProgram()

   Deletes a specified program. If the program is currently active in a
   context, the program is marked as pending deletion. Gives GL_INVALID_VALUE
   if the argument is neither a program nor a shader, or GL_INVALID_OPERATION
   if the argument is a shader.

   Implementation: Done
   Error Checks: Done
*/

void glDeleteProgram_impl_20 (GLuint p)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (p) {
      MEM_HANDLE_T handle = glxx_shared_get_pobject(shared, p);

      if (handle != MEM_INVALID_HANDLE) {
         GL20_PROGRAM_T *program;

         /* wait to make sure noone is using the buffer */
         khrn_hw_common_wait();

         mem_acquire(handle);

         program = (GL20_PROGRAM_T *)mem_lock(handle);

         if (gl20_is_program(program)) {
            program->deleted = GL_TRUE;

            try_delete_program(shared, program);
         } else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);

         mem_unlock(handle);
         mem_release(handle);
      } else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
   }

   mem_unlock(state->mh_shared);

   GL20_UNLOCK_SERVER_STATE();
}

/*
   glDeleteShader()

   Deletes a specified shader. If the shader is currently attached to a program,
   the shader is marked as pending deletion. Gives GL_INVALID_VALUE if the argument
   is neither a program nor a shader, or GL_INVALID_OPERATION if the argument is a
   program.

   Implementation: Done
   Error Checks: Done
*/

void glDeleteShader_impl_20 (GLuint s)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   GLXX_SHARED_T *shared = (GLXX_SHARED_T *)mem_lock(state->mh_shared);

   if (s) {
      MEM_HANDLE_T handle = glxx_shared_get_pobject(shared, s);

      if (handle != MEM_INVALID_HANDLE) {
         GL20_SHADER_T *shader;

         mem_acquire(handle);

         shader = (GL20_SHADER_T *)mem_lock(handle);

         if (gl20_is_shader(shader)) {
            shader->deleted = GL_TRUE;

            try_delete_shader(shared, shader);
         } else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);

         mem_unlock(handle);
         mem_release(handle);
      } else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);
   }

   mem_unlock(state->mh_shared);

   GL20_UNLOCK_SERVER_STATE();
}

/*
   glDetachShader()

   Detaches a shader from a program. If the shader is marked as pending deletion, and
   is not attached to another program, it is deleted. Gives GL_INVALID_VALUE if the
   program or shader does not exist, or GL_INVALID_OPERATION if the program argument
   is not a program, the shader argument is not a shader, or the shader is not attached
   to the program.

   Implementation: Done
   Error Checks: Done
*/

void glDetachShader_impl_20 (GLuint p, GLuint s)
{
   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   program = get_program(state, p, &phandle);

   if (program) {
      MEM_HANDLE_T shandle;
      GL20_SHADER_T *shader = get_shader(state, s, &shandle);

      if (shader) {
         MEM_HANDLE_T *pmh_shader = NULL;

         switch (shader->type) {
         case GL_VERTEX_SHADER:
            pmh_shader = &program->mh_vertex;
            break;
         case GL_FRAGMENT_SHADER:
            pmh_shader = &program->mh_fragment;
            break;
         default:
            UNREACHABLE();
            break;
         }

         if (*pmh_shader != shandle)
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         else {
            gl20_shader_release(shader);

            try_delete_shader((GLXX_SHARED_T *)mem_lock(state->mh_shared), shader);
            mem_unlock(state->mh_shared);

            MEM_ASSIGN(*pmh_shader, MEM_INVALID_HANDLE);
         }

         mem_unlock(shandle);
      }

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}



/*
void glDisableVertexAttribArray_impl_20 (GLuint index)
{
   UNREACHABLE();
}
*/





/*
void glEnableVertexAttribArray_impl_20 (GLuint index)
{
   UNREACHABLE();
}
*/


/*
   A null-terminating version of strncpy. Copies a string from src
   to dst with a maximum length of len, and forcibly null-terminates
   the result. Returns the number of characters written, not
   including the null terminator, or -1 either dst is NULL or length
   is less than 1 (giving us no space to even write the terminator).
*/

static size_t strzncpy(char *dst, const char *src, size_t len)
{
   if (dst && len > 0) {
      strncpy(dst, src, len);

      dst[len - 1] = '\0';

      return strlen(dst);
   } else
      return -1;
}

/*
   glGetActiveAttrib()

   Gets the name, size and type of a specified attribute of a program. Gives
   GL_INVALID_VALUE if the program does not exist, or GL_INVALID_OPERATION
   if the program argument is actually a shader. Also gives GL_INVALID_VALUE
   if the specified index is greater than the number of attributes of the
   linked program.

   Implementation: Done
   Error Checks: Done
*/

void glGetActiveAttrib_impl_20 (GLuint p, GLuint index, GLsizei buf_len, GLsizei *length, GLint *size, GLenum *type, char *buf_ptr)
{
   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   program = get_program(state, p, &phandle);

   if (program) {
      uint32_t count = mem_get_size(program->mh_attrib_info) / sizeof(GL20_ATTRIB_INFO_T);

      vcos_assert(mem_get_size(program->mh_attrib_info) % sizeof(GL20_ATTRIB_INFO_T) == 0);

      if (index < count) {
         const char *name_ptr;
         int32_t name_len;
         size_t chars;

         GL20_ATTRIB_INFO_T *base = (GL20_ATTRIB_INFO_T *)mem_lock(program->mh_attrib_info);
         vcos_assert(base);

         name_ptr = (const char *)mem_lock(base[index].mh_name);
         name_len = mem_get_size(base[index].mh_name);

         vcos_assert(name_ptr);
         vcos_assert(name_len > 0);
         vcos_assert(strlen(name_ptr) == (size_t)(name_len - 1));

         chars = strzncpy(buf_ptr, name_ptr, buf_len);

         if (length)
            *length = (GLsizei)chars;
         if (size)
            *size = 1;        // no array or structure attributes
         if (type)
            *type = base[index].type;

         mem_unlock(base[index].mh_name);
         mem_unlock(program->mh_attrib_info);
      } else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

/*
   glGetActiveUniform()

   Gets the name, size and type of a specified uniform of a program. Gives
   GL_INVALID_VALUE if the program does not exist, or GL_INVALID_OPERATION
   if the program argument is actually a shader. Also gives GL_INVALID_VALUE
   if the specified index is greater than the number of uniforms of the
   linked program.

   Implementation: Done
   Error Checks: Done
*/

void glGetActiveUniform_impl_20 (GLuint p, GLuint index, GLsizei buf_len, GLsizei *length, GLint *size, GLenum *type, char *buf_ptr)
{
   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   program = get_program(state, p, &phandle);

   if (program) {
      uint32_t count = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);

      vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

      if (index < count) {
         const char *name_ptr;
         int32_t name_len;
         size_t chars;

         GL20_UNIFORM_INFO_T *base = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
         vcos_assert(base);

         name_ptr = (const char *)mem_lock(base[index].mh_name);
         name_len = mem_get_size(base[index].mh_name);

         vcos_assert(name_ptr);
         vcos_assert(name_len > 0);
         vcos_assert(strlen(name_ptr) == (size_t)(name_len - 1));

         chars = strzncpy(buf_ptr, name_ptr, buf_len);

         if (length)
            *length = (GLsizei)chars;
         if (size)
            *size = base[index].size;
         if (type)
            *type = base[index].type;

         mem_unlock(base[index].mh_name);
         mem_unlock(program->mh_uniform_info);
      } else
         glxx_server_state_set_error(state, GL_INVALID_VALUE);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

/*
   glGetAttachedShaders()

   Gets the names of the shaders attached to a specified program. Gives
   GL_INVALID_VALUE if the program does not exist, or GL_INVALID_OPERATION
   if the program argument is actually a shader.

   Implementation: Done
   Error Checks: Done
*/

void glGetAttachedShaders_impl_20 (GLuint p, GLsizei maxcount, GLsizei *pcount, GLuint *shaders)
{
   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   program = get_program(state, p, &phandle);

   if (program) {
      int32_t count = 0;

      if (shaders) {
         if (maxcount > 0) {
            if (program->mh_vertex != MEM_INVALID_HANDLE) {
               GL20_SHADER_T *vertex = (GL20_SHADER_T *)mem_lock(program->mh_vertex);

               shaders[count++] = vertex->name;
               maxcount--;

               mem_unlock(program->mh_vertex);
            }
         }

         if (maxcount > 0) {
            if (program->mh_fragment != MEM_INVALID_HANDLE) {
               GL20_SHADER_T *fragment = (GL20_SHADER_T *)mem_lock(program->mh_fragment);

               shaders[count++] = fragment->name;
               maxcount--;

               mem_unlock(program->mh_fragment);
            }
         }
      }

      if (pcount)
         *pcount = count;

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

int glGetAttribLocation_impl_20 (GLuint p, const char *name)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;
   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   int result = -1;

   if (program) {
      if (name)
      {
         if (program->linked) {
            uint32_t i;
            GL20_ATTRIB_INFO_T *base = (GL20_ATTRIB_INFO_T *)mem_lock(program->mh_attrib_info);
            uint32_t count = mem_get_size(program->mh_attrib_info) / sizeof(GL20_ATTRIB_INFO_T);

            vcos_assert(base);
            vcos_assert(mem_get_size(program->mh_attrib_info) % sizeof(GL20_ATTRIB_INFO_T) == 0);

            for (i = 0; i < count; i++) {
               int b = strcmp((char *)mem_lock(base[i].mh_name), name);
               mem_unlock(base[i].mh_name);

               vcos_assert((base[i].offset & 3) == 0);

               if (!b) {
                  result = base[i].offset >> 2;
                  break;
               }
            }

            mem_unlock(program->mh_attrib_info);
         } else
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      }

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();

   return result;
}

/*
   GetProgramiv

   DELETE STATUS False GetProgramiv
   LINK STATUS False GetProgamiv
   VALIDATE STATUS False GetProgramiv
   ATTACHED SHADERS 0 GetProgramiv
   INFO LOG LENGTH 0 GetProgramiv
   ACTIVE UNIFORMS 0 GetProgamiv
   ACTIVE UNIFORM MAX LENGTH 0 GetProgramiv
   ACTIVE ATTRIBUTES 0 GetProgramiv
   ACTIVE ATTRIBUTE MAX LENGTH 0 GetProgramiv

   If pname is ACTIVE ATTRIBUTE MAX LENGTH, the length of the longest
   active attribute name, including a null terminator, is returned.
*/

int glGetProgramiv_impl_20 (GLuint p, GLenum pname, GLint *params)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   int result;

   if (program) {
      switch (pname) {
      case GL_DELETE_STATUS:
         params[0] = program->deleted ? 1 : 0;
         result = 1;
         break;
      case GL_LINK_STATUS:
         params[0] = program->linked ? 1 : 0;
         result = 1;
         break;
      case GL_VALIDATE_STATUS:
         params[0] = program->validated ? 1 : 0;
         result = 1;
         break;
      case GL_ATTACHED_SHADERS:
         params[0] = (program->mh_vertex != MEM_INVALID_HANDLE) + (program->mh_fragment != MEM_INVALID_HANDLE);
         result = 1;
         break;
      case GL_INFO_LOG_LENGTH:
      {
         params[0] = mem_get_size(program->mh_info) - 1;
         result = 1;
         break;
      }
      case GL_ACTIVE_UNIFORMS:
         vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

         params[0] = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);
         result = 1;
         break;
      case GL_ACTIVE_UNIFORM_MAX_LENGTH:
      {
         uint32_t max = 0;
         uint32_t i;

         GL20_UNIFORM_INFO_T *base = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
         uint32_t count = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);

         vcos_assert(base != 0 || count == 0);
         vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

         for (i = 0; i < count; i++) {
            uint32_t size = mem_get_size(base[i].mh_name);

            if (size > max)
               max = size;
         }

         mem_unlock(program->mh_uniform_info);

         params[0] = max;
         result = 1;
         break;
      }
      case GL_ACTIVE_ATTRIBUTES:
         vcos_assert(mem_get_size(program->mh_attrib_info) % sizeof(GL20_ATTRIB_INFO_T) == 0);

         params[0] = mem_get_size(program->mh_attrib_info) / sizeof(GL20_ATTRIB_INFO_T);
         result = 1;
         break;
      case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      {
         uint32_t max = 0;
         uint32_t i;

         GL20_ATTRIB_INFO_T *base = (GL20_ATTRIB_INFO_T *)mem_lock(program->mh_attrib_info);
         uint32_t count = mem_get_size(program->mh_attrib_info) / sizeof(GL20_ATTRIB_INFO_T);

         vcos_assert(base != 0 || count == 0);
         vcos_assert(mem_get_size(program->mh_attrib_info) % sizeof(GL20_ATTRIB_INFO_T) == 0);

         for (i = 0; i < count; i++) {
            uint32_t size = mem_get_size(base[i].mh_name);

            if (size > max)
               max = size;
         }

         mem_unlock(program->mh_attrib_info);

         params[0] = max;
         result = 1;
         break;
      }
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = 0;
         break;
      }

      mem_unlock(phandle);
   } else
      result = 0;

   GL20_UNLOCK_SERVER_STATE();

   return result;
}

void glGetProgramInfoLog_impl_20 (GLuint p, GLsizei bufsize, GLsizei *length, char *infolog)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   if (program) {
      size_t chars = strzncpy(infolog, (const char *)mem_lock(program->mh_info), bufsize);
      mem_unlock(program->mh_info);

      if (length)
         *length = _max(0, (GLsizei)chars);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}



#ifdef UNIFORM_LOCATION_IS_16_BIT
#define LOCATION_ENCODE(index, offset)    ((offset << program->uniform_index_mask_bits) | (index))
#define LOCATION_DECODE_INDEX(location)   ((location) & ((1 << program->uniform_index_mask_bits) - 1))
#define LOCATION_DECODE_OFFSET(location)  ((location) >> program->uniform_index_mask_bits)
#else
#define LOCATION_ENCODE(index, offset)    ((index) << 16 | (offset))
#define LOCATION_DECODE_INDEX(location)   ((location) >> 16 & 0xffff)
#define LOCATION_DECODE_OFFSET(location)  ((location) & 0xffff)
#endif

int glGetUniformfv_impl_20 (GLuint p, GLint location, GLfloat *params)
{
   return get_uniform_internal(p, location, params, 1);
}

int glGetUniformiv_impl_20 (GLuint p, GLint location, GLint *params)
{
   return get_uniform_internal(p, location, params, 0);
}

static int is_bracket_or_digit(char c)
{
   return c == '[' || isdigit(c);
}

static int32_t get_uniform_length(const char *name)
{
   int32_t len;

   vcos_assert(name);

   len = (int32_t)strlen(name);

   if (len > 0 && name[len - 1] == ']') {
      len--;

      while (len > 0 && is_bracket_or_digit(name[len - 1]))
         len--;
   }

   return len;
}

static int32_t get_uniform_offset(const char *name, int32_t pos)
{
   int32_t len;
   int32_t off = 0;

   vcos_assert(name);
   vcos_assert(pos >= 0);

   len = (int32_t)strlen(name);

   vcos_assert(pos <= len);

   if (pos < len) {
      if (name[pos++] != '[')
         return -1;

      while (isdigit(name[pos]))
         off = off * 10 + name[pos++] - '0';

      if (name[pos++] != ']')
         return -1;

      if (pos < len)
         return -1;
   }

   return off;
}

int glGetUniformLocation_impl_20 (GLuint p, const char *name)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   int result = -1;

   if (program) {
      if (program->linked) {
         int32_t len = get_uniform_length(name);
         int32_t off = get_uniform_offset(name, len);

         if (off >= 0) {
            uint32_t i;
            GL20_UNIFORM_INFO_T *base = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
            uint32_t count = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);

            //if there are no uniforms, program->mh_uniform_info will point to a zero sized handle
            //so base == 0
            vcos_assert(base || count == 0);
            vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

            for (i = 0; i < count; i++) {
               const char *curr = (const char *)mem_lock(base[i].mh_name);
               int b = (strlen(curr) == (size_t)len) && !strncmp(curr, name, len) && (off < base[i].size);
               mem_unlock(base[i].mh_name);

               if (b) {
                  result = LOCATION_ENCODE(i, off);
                  break;
               }
            }

            mem_unlock(program->mh_uniform_info);
         }
      } else
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();

   return result;
}
/*
void glGetVertexAttribfv_impl_20 (GLuint index, GLenum pname, GLfloat *params)
{
   UNREACHABLE();
}

void glGetVertexAttribiv_impl_20 (GLuint index, GLenum pname, GLint *params)
{
   UNREACHABLE();
}

void glGetVertexAttribPointerv_impl_20 (GLuint index, GLenum pname, void **pointer)
{
   UNREACHABLE();
}
*/


/*
   glIsProgram()

   Returns TRUE if program is the name of a program object. If program is zero,
   or a non-zero value that is not the name of a program object, IsProgram returns
   FALSE. No error is generated if program is not a valid program object name.

   Implementation: Done
   Error Checks: Done
*/

GLboolean glIsProgram_impl_20 (GLuint p)
{
   GLboolean result;
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = glxx_shared_get_pobject((GLXX_SHARED_T *)mem_lock(state->mh_shared), p);
   mem_unlock(state->mh_shared);

   if (handle == MEM_INVALID_HANDLE)
      result = GL_FALSE;
   else {
      result = gl20_is_program((GL20_PROGRAM_T *)mem_lock(handle));
      mem_unlock(handle);
   }

   GL20_UNLOCK_SERVER_STATE();

   return result;
}

/*
   glIsShader()

   Returns TRUE if shader is the name of a shader object. If shader is zero,
   or a non-zero value that is not the name of a shader object, IsShader returns
   FALSE. No error is generated if shader is not a valid shader object name.

   Implementation: Done
   Error Checks: Done
*/

GLboolean glIsShader_impl_20 (GLuint s)
{
   GLboolean result;
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T handle = glxx_shared_get_pobject((GLXX_SHARED_T *)mem_lock(state->mh_shared), s);
   mem_unlock(state->mh_shared);

   if (handle == MEM_INVALID_HANDLE)
      result = GL_FALSE;
   else {
      result = gl20_is_shader((GL20_SHADER_T *)mem_lock(handle));
      mem_unlock(handle);
   }

   GL20_UNLOCK_SERVER_STATE();

   return result;
}




void glLinkProgram_impl_20 (GLuint p)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   if (program) {
      /* wait to make sure noone is using the buffer */
      khrn_hw_common_wait();

      gl20_program_link(program);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

#define COERCE_ID 0
#define COERCE_INT_TO_FLOAT 1
#define COERCE_INT_TO_BOOL 2
#define COERCE_FLOAT_TO_BOOL 3
#define COERCE_FLOAT_TO_INT 4
static GLboolean is_compatible_uniform(GL20_UNIFORM_INFO_T *info, GLint stride, GLboolean is_float, GLboolean is_matrix, uint32_t *coercion)
{
   switch (info->type) {
   case GL_FLOAT:
      *coercion = COERCE_ID;
      return stride == 1 && is_float && !is_matrix;
   case GL_FLOAT_VEC2:
      *coercion = COERCE_ID;
      return stride == 2 && is_float && !is_matrix;
   case GL_FLOAT_VEC3:
      *coercion = COERCE_ID;
      return stride == 3 && is_float && !is_matrix;
   case GL_FLOAT_VEC4:
      *coercion = COERCE_ID;
      return stride == 4 && is_float && !is_matrix;
   case GL_INT:
      *coercion = COERCE_INT_TO_FLOAT;
      return stride == 1 && !is_float && !is_matrix;
   case GL_INT_VEC2:
      *coercion = COERCE_INT_TO_FLOAT;
      return stride == 2 && !is_float && !is_matrix;
   case GL_INT_VEC3:
      *coercion = COERCE_INT_TO_FLOAT;
      return stride == 3 && !is_float && !is_matrix;
   case GL_INT_VEC4:
      *coercion = COERCE_INT_TO_FLOAT;
      return stride == 4 && !is_float && !is_matrix;
   case GL_BOOL:
      *coercion = is_float ? COERCE_FLOAT_TO_BOOL : COERCE_INT_TO_BOOL;
      return stride == 1 && !is_matrix;
   case GL_BOOL_VEC2:
      *coercion = is_float ? COERCE_FLOAT_TO_BOOL : COERCE_INT_TO_BOOL;
      return stride == 2 && !is_matrix;
   case GL_BOOL_VEC3:
      *coercion = is_float ? COERCE_FLOAT_TO_BOOL : COERCE_INT_TO_BOOL;
      return stride == 3 && !is_matrix;
   case GL_BOOL_VEC4:
      *coercion = is_float ? COERCE_FLOAT_TO_BOOL : COERCE_INT_TO_BOOL;
      return stride == 4 && !is_matrix;
   case GL_FLOAT_MAT2:
      *coercion = COERCE_ID;
      return stride == 4 && is_float && is_matrix;
   case GL_FLOAT_MAT3:
      *coercion = COERCE_ID;
      return stride == 9 && is_float && is_matrix;
   case GL_FLOAT_MAT4:
      *coercion = COERCE_ID;
      return stride == 16 && is_float && is_matrix;
   case GL_SAMPLER_2D:
   case GL_SAMPLER_CUBE:
      *coercion = COERCE_ID;
      return stride == 1 && !is_float && !is_matrix;
   default:
      UNREACHABLE();
      break;
   }

   return GL_FALSE;
}

static void uniformv_internal(GLint location, GLsizei num, const void *v, GLint stride, GLboolean is_float, GLboolean is_matrix)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   if (location == -1) {
      // silently do nothing
   } else if (state->mh_program != MEM_INVALID_HANDLE) {
      GL20_PROGRAM_T *program = (GL20_PROGRAM_T *)mem_lock(state->mh_program);

      if (program->linked) {
         int32_t index, offset;
         uint32_t coercion = 0;
         GL20_UNIFORM_INFO_T *info = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
         int32_t count = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);

         vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

         index = LOCATION_DECODE_INDEX(location);
         offset = LOCATION_DECODE_OFFSET(location);
		 program->uniforms_updated = 1;

         if (index >= 0 && index < count && offset >= 0 && offset < info[index].size && is_compatible_uniform(&info[index], stride, is_float, is_matrix, &coercion)) {
            int i;
            GLint *datain;
            GLint *data = (GLint *)mem_lock(program->mh_uniform_data);

            vcos_assert(data);

            if (offset + num > info[index].size)
               num = info[index].size - offset;

            data += info[index].offset + offset * stride;
            datain = (GLint*)v;
            for (i = 0; i < num * stride; i++)
            {
               switch (coercion)
               {
               case COERCE_ID:
                  *(data++) = *(datain++);
                  break;
               case COERCE_INT_TO_FLOAT:
                  //TODO: is it acceptable to lose precision by converting ints to floats?
                  *(float*)(data++) = (float)*(datain++);
                  break;
               case COERCE_INT_TO_BOOL:
                  *(data++) = *(datain++) ? 1 : 0;
                  break;
               case COERCE_FLOAT_TO_BOOL:
                  *(data++) = (*(float*)(datain++) != 0.0f) ? 1 : 0;
                  break;
               default:
                  //Unreachable
                  UNREACHABLE();
               }
            }

            mem_unlock(program->mh_uniform_data);
         } else {
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         }

         mem_unlock(program->mh_uniform_info);
      } else {
         UNREACHABLE();
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      }

      mem_unlock(state->mh_program);
   } else
      glxx_server_state_set_error(state, GL_INVALID_OPERATION);

   GL20_UNLOCK_SERVER_STATE();
}

static void uniform_coercion_for_get(GL20_UNIFORM_INFO_T *info, GLboolean is_float, uint32_t *coercion, int *elementcount)
{
   switch (info->type) {
   case GL_FLOAT:
   case GL_INT:
      *elementcount = 1;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_FLOAT_VEC2:
   case GL_INT_VEC2:
      *elementcount = 2;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_FLOAT_VEC3:
   case GL_INT_VEC3:
      *elementcount = 3;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_FLOAT_VEC4:
   case GL_INT_VEC4:
      *elementcount = 4;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_BOOL:
   case GL_SAMPLER_2D:
   case GL_SAMPLER_CUBE:
      *elementcount = 1;
      *coercion = is_float ? COERCE_INT_TO_FLOAT : COERCE_ID;
      break;
   case GL_BOOL_VEC2:
      *elementcount = 2;
      *coercion = is_float ? COERCE_INT_TO_FLOAT : COERCE_ID;
      break;
   case GL_BOOL_VEC3:
      *elementcount = 3;
      *coercion = is_float ? COERCE_INT_TO_FLOAT : COERCE_ID;
      break;
   case GL_BOOL_VEC4:
      *elementcount = 4;
      *coercion = is_float ? COERCE_INT_TO_FLOAT : COERCE_ID;
      break;
   case GL_FLOAT_MAT2:
      *elementcount = 4;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_FLOAT_MAT3:
      *elementcount = 9;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   case GL_FLOAT_MAT4:
      *elementcount = 16;
      *coercion = is_float ? COERCE_ID : COERCE_FLOAT_TO_INT;
      break;
   default:
      //Unreachable
      UNREACHABLE();
      break;
   }
}

static int get_uniform_internal(GLuint p, GLint location, const void *v, GLboolean is_float)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   int elementcount = 0;
   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   if (program) {

      if (program->linked) {
         int32_t index, offset;
         uint32_t coercion = 0;
         GL20_UNIFORM_INFO_T *info = (GL20_UNIFORM_INFO_T *)mem_lock(program->mh_uniform_info);
         int32_t count = mem_get_size(program->mh_uniform_info) / sizeof(GL20_UNIFORM_INFO_T);

         vcos_assert(info!=0 || count == 0);
         vcos_assert(mem_get_size(program->mh_uniform_info) % sizeof(GL20_UNIFORM_INFO_T) == 0);

         index = LOCATION_DECODE_INDEX(location);
         offset = LOCATION_DECODE_OFFSET(location);

         if (index >= 0 && index < count && offset >= 0 && offset < info[index].size) {
            int i;
            GLint *data, *dataout;
            uniform_coercion_for_get(&info[index], is_float, &coercion, &elementcount);

            data = (GLint *)mem_lock(program->mh_uniform_data);

            vcos_assert(data);

            data += info[index].offset + offset * elementcount;
            dataout = (GLint*)v;
            for (i = 0; i < elementcount; i++)
            {
               switch (coercion)
               {
               case COERCE_ID:
                  *(dataout++) = *(data++);
                  break;
               case COERCE_INT_TO_FLOAT:
                  *(float*)(dataout++) = (float)*(data++);
                  break;
               case COERCE_FLOAT_TO_INT:
                  *(dataout++) = (GLint)*(float*)(data++);
                  break;
               default:
                  //Unreachable
                  UNREACHABLE();
               }
            }

            mem_unlock(program->mh_uniform_data);
         } else {
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         }

         mem_unlock(program->mh_uniform_info);
      } else {
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      }

      mem_unlock(phandle);
   } else {
      elementcount = 0;
   }

   GL20_UNLOCK_SERVER_STATE();
   return elementcount;
}

void glUniform1i_impl_20 (GLint location, GLint x)
{
   uniformv_internal(location, 1, &x, 1, GL_FALSE, GL_FALSE);
}

void glUniform2i_impl_20 (GLint location, GLint x, GLint y)
{
   GLint v[2];

   v[0] = x; v[1] = y;

   uniformv_internal(location, 1, v, 2, GL_FALSE, GL_FALSE);
}

void glUniform3i_impl_20 (GLint location, GLint x, GLint y, GLint z)
{
   GLint v[3];

   v[0] = x; v[1] = y; v[2] = z;

   uniformv_internal(location, 1, v, 3, GL_FALSE, GL_FALSE);
}

void glUniform4i_impl_20 (GLint location, GLint x, GLint y, GLint z, GLint w)
{
   GLint v[4];

   v[0] = x; v[1] = y; v[2] = z; v[3] = w;

   uniformv_internal(location, 1, v, 4, GL_FALSE, GL_FALSE);
}

void glUniform1f_impl_20 (GLint location, GLfloat x)
{
   uniformv_internal(location, 1, &x, 1, GL_TRUE, GL_FALSE);
}

void glUniform2f_impl_20 (GLint location, GLfloat x, GLfloat y)
{
   GLfloat v[2];

   v[0] = x; v[1] = y;

   uniformv_internal(location, 1, v, 2, GL_TRUE, GL_FALSE);
}

void glUniform3f_impl_20 (GLint location, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat v[3];

   v[0] = x; v[1] = y; v[2] = z;

   uniformv_internal(location, 1, v, 3, GL_TRUE, GL_FALSE);
}

void glUniform4f_impl_20 (GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GLfloat v[4];

   v[0] = x; v[1] = y; v[2] = z; v[3] = w;

   uniformv_internal(location, 1, v, 4, GL_TRUE, GL_FALSE);
}

void glUniform1iv_impl_20 (GLint location, GLsizei num, int size, const GLint *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 1, GL_FALSE, GL_FALSE);
}

void glUniform2iv_impl_20 (GLint location, GLsizei num, int size, const GLint *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 2, GL_FALSE, GL_FALSE);
}

void glUniform3iv_impl_20 (GLint location, GLsizei num, int size, const GLint *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 3, GL_FALSE, GL_FALSE);
}

void glUniform4iv_impl_20 (GLint location, GLsizei num, int size, const GLint *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 4, GL_FALSE, GL_FALSE);
}

void glUniform1fv_impl_20 (GLint location, GLsizei num, int size, const GLfloat *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 1, GL_TRUE, GL_FALSE);
}

void glUniform2fv_impl_20 (GLint location, GLsizei num, int size, const GLfloat *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 2, GL_TRUE, GL_FALSE);
}

void glUniform3fv_impl_20 (GLint location, GLsizei num, int size, const GLfloat *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 3, GL_TRUE, GL_FALSE);
}

void glUniform4fv_impl_20 (GLint location, GLsizei num, int size, const GLfloat *v)
{
   UNUSED(size);

   uniformv_internal(location, num, v, 4, GL_TRUE, GL_FALSE);
}

/*
   The transpose parameter in the UniformMatrix API call can only be FALSE in
   OpenGL ES 2.0. The transpose field was added to UniformMatrix as OpenGL 2.0
   supports both column major and row major matrices. OpenGL ES 1.0 and 1.1 do
   not support row major matrices because there was no real demand for it. There
   is no reason to support both column major and row major matrices in OpenGL ES
   2.0, so the default matrix type used in OpenGL (i.e. column major) is the
   only one supported. An INVALID VALUE error will be generated if tranpose is
   not FALSE.
*/

void glUniformMatrix2fv_impl_20 (GLint location, GLsizei num, GLboolean transpose, int size, const GLfloat *v)
{
   UNUSED(size);

   if (!transpose)
      uniformv_internal(location, num, v, 4, GL_TRUE, GL_TRUE);
   else {
      GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      GL20_UNLOCK_SERVER_STATE();
   }
}

void glUniformMatrix3fv_impl_20 (GLint location, GLsizei num, GLboolean transpose, int size, const GLfloat *v)
{
   UNUSED(size);

   if (!transpose)
      uniformv_internal(location, num, v, 9, GL_TRUE, GL_TRUE);
   else {
      GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      GL20_UNLOCK_SERVER_STATE();
   }
}

void glUniformMatrix4fv_impl_20 (GLint location, GLsizei num, GLboolean transpose, int size, const GLfloat *v)
{
   UNUSED(size);

   if (!transpose)
      uniformv_internal(location, num, v, 16, GL_TRUE, GL_TRUE);
   else {
      GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

      glxx_server_state_set_error(state, GL_INVALID_VALUE);

      GL20_UNLOCK_SERVER_STATE();
   }
}

void glUseProgram_impl_20 (GLuint p) // S
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   if (p) {
      MEM_HANDLE_T phandle;

      GL20_PROGRAM_T *program = get_program(state, p, &phandle);

      if (program) {
         if (program->linked) {
            gl20_program_acquire(program);

            release_program((GLXX_SHARED_T *)mem_lock(state->mh_shared), state->mh_program);
            mem_unlock(state->mh_shared);

            MEM_ASSIGN(state->mh_program, phandle);
         } else {
            //TODO have I got the reference counts right? I'm new to all this.
            glxx_server_state_set_error(state, GL_INVALID_OPERATION);
         }
         mem_unlock(phandle);
      }
   } else {
      release_program((GLXX_SHARED_T *)mem_lock(state->mh_shared), state->mh_program);
      mem_unlock(state->mh_shared);

      MEM_ASSIGN(state->mh_program, MEM_INVALID_HANDLE);
   }

   GL20_UNLOCK_SERVER_STATE();
}

void glValidateProgram_impl_20 (GLuint p)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T phandle;

   GL20_PROGRAM_T *program = get_program(state, p, &phandle);

   if (program) {
      program->validated = gl20_validate_program(state, program);

      MEM_ASSIGN(program->mh_info, MEM_EMPTY_STRING_HANDLE);

      mem_unlock(phandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}
/*
void glVertexAttrib1f_impl_20 (GLuint indx, GLfloat x)
{
  UNREACHABLE();
}

void glVertexAttrib2f_impl_20 (GLuint indx, GLfloat x, GLfloat y)
{
  UNREACHABLE();
}

void glVertexAttrib3f_impl_20 (GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
  UNREACHABLE();
}

void glVertexAttrib4f_impl_20 (GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
  UNREACHABLE();
}

void glVertexAttrib1fv_impl_20 (GLuint indx, const GLfloat *values)
{
  UNREACHABLE();
}

void glVertexAttrib2fv_impl_20 (GLuint indx, const GLfloat *values)
{
  UNREACHABLE();
}

void glVertexAttrib3fv_impl_20 (GLuint indx, const GLfloat *values)
{
  UNREACHABLE();
}

void glVertexAttrib4fv_impl_20 (GLuint indx, const GLfloat *values)
{
  UNREACHABLE();
}
*/
void glVertexAttribPointer_impl_20 (GLuint indx)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(indx < GLXX_CONFIG_MAX_VERTEX_ATTRIBS);

   MEM_ASSIGN(state->bound_buffer.mh_attrib_array[indx], state->bound_buffer.mh_array);

   GL20_UNLOCK_SERVER_STATE();
}

void glCompileShader_impl_20 (GLuint s)
{
   MEM_HANDLE_T shandle;
   GL20_SHADER_T *shader;

   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   vcos_assert(state);

   shader = get_shader(state, s, &shandle);

   if (shader) {
      gl20_shader_compile(shader);

      mem_unlock(shandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

/*
   GetShaderiv

   SHADER TYPE   GetShaderiv
   DELETE STATUS False GetShaderiv
   COMPILE STATUS   False GetShaderiv
   INFO LOG LENGTH   0 GetShaderiv
   SHADER SOURCE LENGTH   0 GetShaderiv

   If pname is SHADER SOURCE LENGTH, the length of the concatenation
   of the source strings making up the shader source, including a null
   terminator, is returned. If no source has been defined, zero is
   returned.
*/

int glGetShaderiv_impl_20 (GLuint s, GLenum pname, GLint *params)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T shandle;

   GL20_SHADER_T *shader = get_shader(state, s, &shandle);

   int result;

   if (shader) {
      switch (pname) {
      case GL_SHADER_TYPE:
         params[0] = shader->type;
         result = 1;
         break;
      case GL_DELETE_STATUS:
         params[0] = shader->deleted ? 1 : 0;
         result = 1;
         break;
      case GL_COMPILE_STATUS:
         params[0] = shader->compiled ? 1 : 0;
         result = 1;
         break;
      case GL_INFO_LOG_LENGTH:
      {
         const char *msg = lock_shader_info_log(shader);
         params[0] = strlen(msg);
         unlock_shader_info_log(shader);
         result = 1;
         break;
      }
      case GL_SHADER_SOURCE_LENGTH:
      {
         int i;
         MEM_HANDLE_T *handles = (MEM_HANDLE_T *)mem_lock(shader->mh_sources_current);

         int count = mem_get_size(shader->mh_sources_current) / sizeof(MEM_HANDLE_T);
         int total = 1;

         for (i = 0; i < count; i++)
            total += mem_get_size(handles[i]) - 1;

         mem_unlock(shader->mh_sources_current);

         params[0] = total;
         result = 1;
         break;
      }
      default:
         glxx_server_state_set_error(state, GL_INVALID_ENUM);
         result = 0;
         break;
      }

      mem_unlock(shandle);
   } else
      result = 0;

   GL20_UNLOCK_SERVER_STATE();

   return result;
}


static const char *lock_shader_info_log(GL20_SHADER_T *shader)
{
   static const char *msgEmpty = "";
   // Compilation is always successful. Or it would be if it wasn't for VND-108.
   static const char *msgCompiled = "Compiled";

   if (shader->compiled) {
      return msgCompiled;
   } else {
      return msgEmpty;
   }
}

static void unlock_shader_info_log(GL20_SHADER_T *shader)
{
   UNUSED(shader);
   /* Nothing to do - we don't have an info log handle */
}

void glGetShaderInfoLog_impl_20 (GLuint s, GLsizei bufsize, GLsizei *length, char *infolog)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T shandle;
   GL20_SHADER_T *shader = get_shader(state, s, &shandle);

   if (shader) {
      const char *c = lock_shader_info_log(shader);

      size_t chars = strzncpy(infolog, c, bufsize);
      unlock_shader_info_log(shader);

      if (length)
         *length = _max(0, (GLsizei)chars);

      mem_unlock(shandle);
   }

   GL20_UNLOCK_SERVER_STATE();
}

void glGetShaderSource_impl_20 (GLuint s, GLsizei bufsize, GLsizei *length, char *source)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   MEM_HANDLE_T shandle;
   GL20_SHADER_T *shader = get_shader(state, s, &shandle);
   uint32_t charswritten = 0;

   if (shader) {
      if (shader->mh_sources_current == MEM_INVALID_HANDLE) {
         glxx_server_state_set_error(state, GL_INVALID_OPERATION);
      } else if (bufsize > 1) {//need 1 byte for NULL terminator below
         unsigned int i;
         MEM_HANDLE_T *handles = (MEM_HANDLE_T *)mem_lock(shader->mh_sources_current);
         uint32_t count = mem_get_size(shader->mh_sources_current) / sizeof(MEM_HANDLE_T);

         for (i = 0; i < count; i++) {
            char *str = (char*)mem_lock(handles[i]);
            int32_t strlen = mem_get_size(handles[i])/sizeof(char) - 1;
            vcos_assert(strlen >= 0);
            vcos_assert(str[strlen] == 0);

            if (charswritten + strlen > (uint32_t)bufsize - 1)
            {
               vcos_assert((int)bufsize - 1 - (int)charswritten >= 0);
               memcpy(source + charswritten, str, bufsize - 1 - charswritten);
               charswritten = bufsize - 1;
               mem_unlock(handles[i]);
               break;
            }
            else
            {
               memcpy(source + charswritten, str, strlen);
               charswritten += strlen;
            }
            mem_unlock(handles[i]);
         }
         mem_unlock(shader->mh_sources_current);
      }
      mem_unlock(shandle);
   }
   if (length) {
      *length = charswritten;
   }
   if (bufsize > 0) {
      vcos_assert(charswritten < (uint32_t)bufsize);
      source[charswritten] = 0;
   }
   GL20_UNLOCK_SERVER_STATE();
}

static MEM_HANDLE_T copy_source_string(const char *string, int length)
{
   MEM_HANDLE_T handle;

   if (string) {
      if (length < 0)
         handle = mem_strdup_ex(string, MEM_COMPACT_DISCARD);                                                              // check
      else {
         handle = mem_alloc_ex(length + 1, 1, MEM_FLAG_NONE, "GL20_SHADER_T.sources_current[i]", MEM_COMPACT_DISCARD);     // check, no term

         if (handle != MEM_INVALID_HANDLE) {
            char *base = (char *)mem_lock(handle);

            memcpy(base, string, length);
            base[length] = '\0';

            mem_unlock(handle);
         }
      }
   } else {
      handle = MEM_EMPTY_STRING_HANDLE;
      mem_acquire(handle);
   }

   return handle;
}

void glShaderSource_impl_20 (GLuint s, GLsizei count, const char **string, const GLint *length)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   if (count >= 0) {
      MEM_HANDLE_T shandle;

      GL20_SHADER_T *shader = get_shader(state, s, &shandle);

      if (shader) {
         if (string) {
            MEM_HANDLE_T handle = mem_alloc_ex(count * sizeof(MEM_HANDLE_T), 4, MEM_FLAG_NONE, "GL20_SHADER_T.sources_current", MEM_COMPACT_DISCARD);                         // check, gl20_shader_sources_term

            if (handle != MEM_INVALID_HANDLE) {
               MEM_HANDLE_T *handles;
               int i;
               mem_set_term(handle, gl20_shader_sources_term);

               handles = (MEM_HANDLE_T *)mem_lock(handle);

               for (i = 0; i < count; i++) {
                  handles[i] = copy_source_string(string[i], length ? length[i] : -1);

                  if (handles[i] == MEM_INVALID_HANDLE) {
                     mem_unlock(handle);
                     mem_release(handle);

                     mem_unlock(shandle);

                     glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);

                     GL20_UNLOCK_SERVER_STATE();
                     return;
                  }
               }

               mem_unlock(handle);

               MEM_ASSIGN(shader->mh_sources_current, handle);
               mem_release(handle);
            } else
               glxx_server_state_set_error(state, GL_OUT_OF_MEMORY);
         }

         mem_unlock(shandle);
      }
   } else
      glxx_server_state_set_error(state, GL_INVALID_VALUE);

   GL20_UNLOCK_SERVER_STATE();
}

/* OES_shader_source + OES_shader_binary */

static bool is_precision_type(GLenum type)
{
   return type == GL_LOW_FLOAT ||
          type == GL_MEDIUM_FLOAT ||
          type == GL_HIGH_FLOAT ||
          type == GL_LOW_INT ||
          type == GL_MEDIUM_INT ||
          type == GL_HIGH_INT;
}

static bool is_float_type(GLenum type)
{
   return type == GL_LOW_FLOAT ||
          type == GL_MEDIUM_FLOAT ||
          type == GL_HIGH_FLOAT;
}

/*
   TODO: is this right? very poorly specified
*/

void glGetShaderPrecisionFormat_impl_20 (GLenum shadertype, GLenum precisiontype, GLint *result)
{
   GLXX_SERVER_STATE_T *state = GL20_LOCK_SERVER_STATE();

   if (is_shader_type(shadertype) && is_precision_type(precisiontype)) {
      /*
         range
      */

      result[0] = -126;
      result[1] = 127;

      /*
         precision
      */

      if (is_float_type(precisiontype))
         result[2] = -23;
      else
         result[2] = 0;
   } else
      glxx_server_state_set_error(state, GL_INVALID_ENUM);

   GL20_UNLOCK_SERVER_STATE();
}
