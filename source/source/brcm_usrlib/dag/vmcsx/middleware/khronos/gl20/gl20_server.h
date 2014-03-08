/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef GL20_SERVER_H
#define GL20_SERVER_H

#include "middleware/khronos/gl20/gl20_config.h"
#include "middleware/khronos/glxx/glxx_server.h"
#include "interface/khronos/include/GLES2/gl2ext.h"

extern bool gl20_server_state_init(GLXX_SERVER_STATE_T *state, uint32_t name, uint64_t pid, MEM_HANDLE_T handle);

static INLINE GLXX_SERVER_STATE_T *gl20_lock_server_state(void)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   GLXX_SERVER_STATE_T *state;
   vcos_assert(egl_state->glversion == EGL_SERVER_GL20);
   if (!egl_state->locked_glcontext) {
      egl_state->locked_glcontext = mem_lock(egl_state->glcontext);
   }
   state = (GLXX_SERVER_STATE_T *)egl_state->locked_glcontext;
   state->changed = GL_TRUE;
   return state;
}

static INLINE void gl20_unlock_server_state(void)
{
	EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
	GLXX_SERVER_STATE_T *state = (GLXX_SERVER_STATE_T *)egl_state->locked_glcontext;
	if (state->changed) {
	 state->shader_record_addr = NULL;
	 state->pres_mode = 0;
	 }

#ifndef NDEBUG
   vcos_assert(egl_state->glversion == EGL_SERVER_GL20);
   vcos_assert(egl_state->locked_glcontext);
#endif /* NDEBUG */
}

static INLINE void gl20_force_unlock_server_state(void)
{
   EGL_SERVER_STATE_T *egl_state = EGL_GET_SERVER_STATE();
   if ((egl_state->glversion == EGL_SERVER_GL20) && egl_state->locked_glcontext) {
      mem_unlock(egl_state->glcontext);
      egl_state->locked_glcontext = NULL;
   }
}

#define GL20_LOCK_SERVER_STATE()         gl20_lock_server_state()
#define GL20_UNLOCK_SERVER_STATE()       gl20_unlock_server_state()
#define GL20_FORCE_UNLOCK_SERVER_STATE() gl20_force_unlock_server_state()

#include "interface/khronos/glxx/gl20_int_impl.h"

#endif
