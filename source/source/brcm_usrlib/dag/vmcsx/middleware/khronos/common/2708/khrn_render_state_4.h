/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_RENDER_STATE_4_H
#define KHRN_RENDER_STATE_4_H

typedef enum {
   KHRN_RENDER_STATE_TYPE_NONE,
   KHRN_RENDER_STATE_TYPE_COPY_BUFFER,
   KHRN_RENDER_STATE_TYPE_GLXX,
   KHRN_RENDER_STATE_TYPE_VG
} KHRN_RENDER_STATE_TYPE_T;

/* global init/term */
extern void khrn_render_state_init(void);
extern void khrn_render_state_term(void); /* will flush any unflushed render states */

/*
   allocates a render state of the specified type and returns the slot index
   this may have to flush a slot to make room
*/

extern uint32_t khrn_render_state_start(KHRN_RENDER_STATE_TYPE_T type);

/*
   queries
*/

extern KHRN_RENDER_STATE_TYPE_T khrn_render_state_get_type(uint32_t i);
extern void *khrn_render_state_get_data(uint32_t i);
extern uint32_t khrn_render_state_get_index_from_data(void *data);

/*
   take over a render state. the caller is responsible for ensuring the old data
   structure is cleaned up and the new data structure is setup correctly
*/

extern void khrn_render_state_usurp(uint32_t i, KHRN_RENDER_STATE_TYPE_T type);

/*
   flush will call a type-specific flush function which will flush the render
   state and then call khrn_render_state_finish to release the slot

   flush_all will call flush on all slots with the specified type (or just all
   slots if type is KHRN_RENDER_STATE_TYPE_NONE)

   flush_except will call flush on all slots except the specified one
*/

extern void khrn_render_state_flush(uint32_t i);
extern void khrn_render_state_flush_all(KHRN_RENDER_STATE_TYPE_T type);
extern void khrn_render_state_flush_except(uint32_t i);

/*
   finish is the complement to start -- it will release the specified slot. this
   should only be called by the code that conceptually owns the slot (which
   should have released or transferred all resources owned by the render state)
*/

extern void khrn_render_state_finish(uint32_t i);

#endif
