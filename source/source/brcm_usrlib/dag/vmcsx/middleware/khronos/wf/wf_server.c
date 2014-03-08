/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "middleware/khronos/wf/wf_server.h"

void wf_server_startup_hack()
{
   WF_SERVER_STATE_T *state = WF_GET_SERVER_STATE();

   memset(state, MEM_HANDLE_INVALID, sizeof(WF_SERVER_STATE_T));

   state->next_context = 0x80000000;

   /*
      this function is only called at startup; if we can't
      create this map there's nothing we can do
   */

   verify(map_init(&state->contexts, 64));
}

int wfcIntCreateContext_impl(int type, int info)
{
   WF_SERVER_STATE_T *state = WF_GET_SERVER_STATE();

   /*
      create and initialize state structure
   */

   MEM_HANDLE_T handle = MEM_ALLOC_STRUCT_EX(WF_SERVER_CONTEXT_T, MEM_COMPACT_DISCARD);
   if (handle == MEM_INVALID_HANDLE) 
      return WFC_ERROR_OUT_OF_MEMORY;

   mem_set_term(handle, wf_server_context_term);

   WF_SERVER_CONTEXT_T *context = (WF_SERVER_CONTEXT_T *)mem_lock(handle);

   if (!wf_server_context_init(context, type, info)) {
      mem_unlock(handle);
      return WFC_ERROR_IN_USE;
   }

   EGL_GL_CONTEXT_ID_T result;

   if (map_insert(&state->contexts, state->next_context, handle))
      result = state->next_context++;
   else
      result = WFC_ERROR_OUT_OF_MEMORY;

   mem_unlock(handle);
   mem_release(handle);

   return result;
}

void wfcIntDestroyContext_impl(int handle);
{
}

void wfcIntCommitBegin_impl(int ctx)
{
   WF_SERVER_STATE_T *state = WF_GET_SERVER_STATE();

   MEM_HANDLE_T handle = map_lookup(&state->contexts, ctx);

   WF_SERVER_CONTEXT_T *context = mem_lock(handle);

   context->pos = 0;

   mem_unlock(handle);
}

void wfcIntCommitAdd_impl(int ctx, int source, int mask, int drect_0, int drect_1, int drect_2, int drect_3, float srect_0, float srect_1, float srect_2, float srect_3, int flip, int rotation, int scale_filter, int transparency, float global_alpha)
{
   WF_SERVER_STATE_T *state = WF_GET_SERVER_STATE();

   MEM_HANDLE_T handle = map_lookup(&state->contexts, ctx);

   WF_SERVER_CONTEXT_T *context = mem_lock(handle);

   context->layers[context->pos].source = source;
   context->layers[context->pos].mask = mask;
   context->layers[context->pos].dst_rect[0] = dst_rect_0;
   context->layers[context->pos].dst_rect[1] = dst_rect_1;
   context->layers[context->pos].dst_rect[2] = dst_rect_2;
   context->layers[context->pos].dst_rect[3] = dst_rect_3;
   context->layers[context->pos].src_rect[0] = src_rect_0;
   context->layers[context->pos].src_rect[1] = src_rect_1;
   context->layers[context->pos].src_rect[2] = src_rect_2;
   context->layers[context->pos].src_rect[3] = src_rect_3;
   context->layers[context->pos].flip = flip;
   context->layers[context->pos].rotation = rotation;
   context->layers[context->pos].scale_filter = scale_filter;
   context->layers[context->pos].transparency = transparency;
   context->layers[context->pos].global_alpha = global_alpha;

   context->pos++;

   mem_unlock(handle);
}

void wfcIntCommitEnd_impl(int ctx)
{
   WF_SERVER_STATE_T *state = WF_GET_SERVER_STATE();

   MEM_HANDLE_T handle = map_lookup(&state->contexts, ctx);

   WF_SERVER_CONTEXT_T *context = mem_lock(handle);


   mem_unlock(handle);
}

void wfcActivate_impl(int handle)
{
}

void wfcDeactivate_impl(int handle)
{
}

void wfcCompose_impl(int handle)
{
}