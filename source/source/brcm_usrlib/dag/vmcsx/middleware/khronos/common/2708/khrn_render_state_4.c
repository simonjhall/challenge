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
#include "middleware/khronos/common/2708/khrn_copy_buffer_4.h"
#include "middleware/khronos/common/2708/khrn_render_state_4.h"
#include "middleware/khronos/glxx/glxx_hw.h"
#include "middleware/khronos/glxx/2708/glxx_inner_4.h"
#include "middleware/khronos/vg/2708/vg_hw_priv_4.h"
#include <stddef.h>

#define N 16 /* keep this <= 29 (the interlock stuff relies on this) and <= TESSS_N_MAX in vg/2708/tess.c */

typedef struct {
   KHRN_RENDER_STATE_TYPE_T type;
   union {
      KHRN_COPY_BUFFER_RENDER_STATE_T copy_buffer;
      GLXX_HW_RENDER_STATE_T glxx;
      VG_BE_RENDER_STATE_T vg;
   } data;
} KHRN_RENDER_STATE_T;

static KHRN_RENDER_STATE_T render_states[N];
static uint32_t prefer = 0;

void khrn_render_state_init(void)
{
   uint32_t i;
   for (i = 0; i != N; ++i) {
      render_states[i].type = KHRN_RENDER_STATE_TYPE_NONE;
   }
}

void khrn_render_state_term(void)
{
#ifndef NDEBUG
   uint32_t i;
   for (i = 0; i != N; ++i) {
      vcos_assert(render_states[i].type == KHRN_RENDER_STATE_TYPE_NONE);
   }
#endif
}

uint32_t khrn_render_state_start(KHRN_RENDER_STATE_TYPE_T type)
{
   uint32_t i, j;

   vcos_assert(type != KHRN_RENDER_STATE_TYPE_NONE);

   prefer = (prefer == (N - 1)) ? 0 : (prefer + 1);

   for (i = 0, j = prefer; i != N; ++i, j = ((j == 0) ? (N - 1) : (j - 1))) {
      if (render_states[j].type == KHRN_RENDER_STATE_TYPE_NONE) {
         render_states[j].type = type;
         return j;
      }
   }
   khrn_render_state_flush(prefer);
   vcos_assert(render_states[prefer].type == KHRN_RENDER_STATE_TYPE_NONE);
   render_states[prefer].type = type;
   return prefer;
}

KHRN_RENDER_STATE_TYPE_T khrn_render_state_get_type(uint32_t i)
{
   vcos_assert(render_states[i].type != KHRN_RENDER_STATE_TYPE_NONE);
   return render_states[i].type;
}

void *khrn_render_state_get_data(uint32_t i)
{
   vcos_assert(render_states[i].type != KHRN_RENDER_STATE_TYPE_NONE);
   return &render_states[i].data;
}

uint32_t khrn_render_state_get_index_from_data(void *data)
{
   uint32_t i = (KHRN_RENDER_STATE_T *)((uint8_t *)data - offsetof(KHRN_RENDER_STATE_T, data)) - render_states;
   vcos_assert(i < N);
   return i;
}

void khrn_render_state_usurp(uint32_t i, KHRN_RENDER_STATE_TYPE_T type)
{
   vcos_assert(render_states[i].type != KHRN_RENDER_STATE_TYPE_NONE);
   render_states[i].type = type;
}

void khrn_render_state_flush(uint32_t i)
{
   switch (render_states[i].type) {
   case KHRN_RENDER_STATE_TYPE_COPY_BUFFER: khrn_copy_buffer_render_state_flush(&render_states[i].data.copy_buffer); break;
      /* TODO: error checking for GLXX! */
   case KHRN_RENDER_STATE_TYPE_GLXX: glxx_hw_render_state_flush(&render_states[i].data.glxx); break;
#ifndef NO_OPENVG
   case KHRN_RENDER_STATE_TYPE_VG:          vg_be_render_state_flush(&render_states[i].data.vg); break;
#endif /* NO_OPENVG */
   default:                                 UNREACHABLE();
   }
   vcos_assert(render_states[i].type == KHRN_RENDER_STATE_TYPE_NONE);
}

void khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_T type)
{
   uint32_t i;
   for (i = 0; i != N; ++i) {
      if ((type == KHRN_RENDER_STATE_TYPE_NONE) ^ (render_states[i].type == type)) {
         khrn_render_state_flush(i);
      }
   }
}

void khrn_render_state_flush_except(uint32_t i)
{
   uint32_t j;
   for (j = 0; j != N; ++j) {
      if ((j != i) && (render_states[j].type != KHRN_RENDER_STATE_TYPE_NONE)) {
         khrn_render_state_flush(j);
      }
   }
}

void khrn_render_state_finish(uint32_t i)
{
   vcos_assert(render_states[i].type != KHRN_RENDER_STATE_TYPE_NONE);
   render_states[i].type = KHRN_RENDER_STATE_TYPE_NONE;
}
