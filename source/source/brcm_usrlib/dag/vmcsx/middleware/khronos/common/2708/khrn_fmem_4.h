/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_FMEM_4_H
#define KHRN_FMEM_4_H

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_nmem_4.h"

typedef union _KHRN_FMEM_TWEAK_T
{
   struct
   {
      union _KHRN_FMEM_TWEAK_T *next;
      uint32_t count;
   } header;

   struct
   {
      uint8_t *location;
      uint32_t special_i;
   } special;

   MEM_HANDLE_OFFSET_T interlock;

   struct
   {
      uint32_t *location;
      MEM_HANDLE_T handle;
      uint32_t i; /* todo: increases size of KHRN_FMEM_TWEAK_T... */
   } ramp;
} KHRN_FMEM_TWEAK_T;

typedef struct
{
   KHRN_FMEM_TWEAK_T *start;
   KHRN_FMEM_TWEAK_T *header;
   uint32_t i;
} KHRN_FMEM_TWEAK_LIST_T;

#define TWEAK_COUNT 32

typedef struct _KHRN_FMEM_FIX_T
{
   struct _KHRN_FMEM_FIX_T *next;
   uint32_t count;
   MEM_HANDLE_OFFSET_T handles[TWEAK_COUNT];
   uint8_t *locations[TWEAK_COUNT];
} KHRN_FMEM_FIX_T;

typedef struct
{
   KHRN_NMEM_GROUP_T nmem_group;
   bool nmem_entered;
   uint32_t nmem_pos;

   uint8_t *bin_begin;
   uint8_t *bin_end;
   uint8_t *render_begin;
   uint8_t *cle_pos;
   /* cle_pos after the last cle alloc. if we've allocated another block since
    * then, this won't match cle_pos. this is to support khrn_fmem_is_here() */
   uint8_t *last_cle_pos;
   uint32_t *junk_pos;

   KHRN_FMEM_FIX_T *fix_start;
   KHRN_FMEM_FIX_T *fix_end;
   KHRN_FMEM_TWEAK_LIST_T special;
   KHRN_FMEM_TWEAK_LIST_T interlock;
   KHRN_FMEM_TWEAK_LIST_T ramp;

   uint32_t render_state_n;
} KHRN_FMEM_T;

#define KHRN_FMEM_SPECIAL_0            0
#define KHRN_FMEM_SPECIAL_BIN_MEM      1
#define KHRN_FMEM_SPECIAL_BIN_MEM_END  2
#define KHRN_FMEM_SPECIAL_BIN_MEM_SIZE 3

extern KHRN_FMEM_T *khrn_fmem_init(uint32_t render_state_n);
extern void khrn_fmem_discard(KHRN_FMEM_T *fmem);
extern uint32_t *khrn_fmem_junk(KHRN_FMEM_T *fmem, int size, int align);
extern uint8_t *khrn_fmem_cle(KHRN_FMEM_T *fmem, int size);
extern bool khrn_fmem_fix(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset);
extern bool khrn_fmem_add_fix(KHRN_FMEM_T *fmem, uint8_t **p, MEM_HANDLE_T handle, uint32_t offset);
extern bool khrn_fmem_add_special(KHRN_FMEM_T *fmem, uint8_t **p, uint32_t special_i, uint32_t offset);
extern bool khrn_fmem_interlock(KHRN_FMEM_T *fmem, MEM_HANDLE_T handle, uint32_t offset);
extern bool khrn_fmem_start_render(KHRN_FMEM_T *fmem);
extern void *khrn_fmem_queue(
   KHRN_FMEM_T *fmem,
   KHRN_HW_CC_FLAG_T bin_cc, KHRN_HW_CC_FLAG_T render_cc,
   uint32_t frame_count,
   uint32_t special_0,
   uint32_t bin_mem_size,
   uint32_t actual_user_vpm,
   uint32_t max_user_vpm,
   KHRN_HW_TYPE_T type,
   KHRN_HW_CALLBACK_T callback,
   uint32_t callback_data_size);

extern bool khrn_fmem_special(KHRN_FMEM_T *fmem, uint32_t *location, uint32_t special_i, uint32_t offset);
extern bool khrn_fmem_is_here(KHRN_FMEM_T *fmem, uint8_t *p);
extern bool khrn_fmem_ramp(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset, uint32_t ramp_i);
extern bool khrn_fmem_fix_special_0(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset);
extern bool khrn_fmem_add_fix_special_0(KHRN_FMEM_T *fmem, uint8_t **p, MEM_HANDLE_T handle, uint32_t offset);
extern bool khrn_fmem_add_fix_image(KHRN_FMEM_T *fmem, uint8_t **p, uint32_t render_i, MEM_HANDLE_T image_handle, uint32_t offset);

static INLINE uint32_t khrn_fmem_get_nmem_n(KHRN_FMEM_T *fmem)
{
   return fmem->nmem_group.n;
}

#endif
