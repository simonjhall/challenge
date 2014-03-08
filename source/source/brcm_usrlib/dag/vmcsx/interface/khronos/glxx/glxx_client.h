/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GLXX_CLIENT_H
#define GLXX_CLIENT_H

#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/common/khrn_client_cache.h"

#include "interface/khronos/egl/egl_client_context.h"

#include "interface/khronos/glxx/glxx_int_attrib.h"
#include "interface/khronos/glxx/glxx_int_config.h"
//#include "utils/Log.h"

#define LOG_FUNC 
//LOGE("%s",__FUNCTION__);
/*
   Called just before a rendering command (i.e. anything which could modify
   the draw surface) is executed
 */
typedef void (*GL_RENDER_CALLBACK_T)(void);

/*
   Called just after rendering has been compeleted (i.e. flush or finish).
   wait should be true for finish-like behaviour, false for flush-like
   behaviour
*/
typedef void (*GL_FLUSH_CALLBACK_T)(bool wait);

/*
   GL 1.1 and 2.0 client state structure
*/

typedef struct {
   
   GLenum error;
   
   /*
      Open GL version

      Invariants:

      OPENGL_ES_11 or OPENGL_ES_20
   */
   
   unsigned int type;    

   /*
      alignments

      used to work out how much data to send for glTexImage2D()
   */

   struct {
      GLint pack;
      GLint unpack;
   } alignment;

   struct {
      GLuint array;
      GLuint element_array;
   } bound_buffer;

   GLXX_ATTRIB_T attrib[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];

   GL_RENDER_CALLBACK_T render_callback;
   GL_FLUSH_CALLBACK_T flush_callback;

   KHRN_CACHE_T cache;
   bool attrib_enabled[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];

   //gl 1.1 specific
   struct {
      GLenum client;
      GLenum server;
   } active_texture;

   //gl 2.0 specific
   bool default_framebuffer;   //render_callback only called if we're rendering to default framebuffer
} GLXX_CLIENT_STATE_T;

extern int gl11_client_state_init(GLXX_CLIENT_STATE_T *state);
extern int gl20_client_state_init(GLXX_CLIENT_STATE_T *state);

extern void glxx_client_state_free(GLXX_CLIENT_STATE_T *state);

#define GLXX_GET_CLIENT_STATE() glxx_get_client_state()
static INLINE GLXX_CLIENT_STATE_T *glxx_get_client_state(void)
{
   EGL_CONTEXT_T *context = CLIENT_GET_THREAD_STATE()->opengl.context;
   GLXX_CLIENT_STATE_T * state;
   vcos_assert( context != NULL );
   vcos_assert(context->type == OPENGL_ES_11 || context->type == OPENGL_ES_20);
   state = (GLXX_CLIENT_STATE_T *)context->state;
   vcos_assert(context->type == state->type);
   return state;
}

#define IS_OPENGLES_11() is_opengles_11()
#define IS_OPENGLES_20() is_opengles_20()

static INLINE bool is_opengles_11(void)
{
   EGL_CONTEXT_T *context = CLIENT_GET_THREAD_STATE()->opengl.context;
   return context && context->type == OPENGL_ES_11;
}

static INLINE bool is_opengles_20(void)
{
   EGL_CONTEXT_T *context = CLIENT_GET_THREAD_STATE()->opengl.context;
   return context && context->type == OPENGL_ES_20;
}
#endif
