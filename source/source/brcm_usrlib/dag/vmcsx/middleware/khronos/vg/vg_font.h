/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VG_FONT_H
#define VG_FONT_H

#include "middleware/khronos/common/khrn_mem.h"
#include "interface/khronos/include/VG/openvg.h"

typedef struct {
   int32_t glyphs_capacity;
} VG_FONT_BPRINT_T;

extern void vg_font_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t glyphs_capacity);

extern void vg_font_bprint_term(void *, uint32_t);

static INLINE bool vg_is_font_bprint(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_font_bprint_term;
}

typedef struct {
   uint32_t id; /* also used for slots free list */

   MEM_HANDLE_T object; /* VG_PATH_T, VG_IMAGE_T, or VG_CHILD_IMAGE_T */
   bool allow_autohinting;
   float origin[2];
   float escapement[2];
} VG_GLYPH_T;

#define SLOT_NONE ((uint32_t)-1)

typedef struct {
   uint32_t capacity;
   uint32_t count;
   MEM_HANDLE_T entries; /* mem_get_size() == (capacity * 2) * sizeof(uint32_t) */
   uint32_t slots_free_head;
   MEM_HANDLE_T slots; /* mem_get_size() == capacity * sizeof(VG_GLYPH_T) */
} VG_FONT_T;

extern bool vg_font_from_bprint(MEM_HANDLE_T handle);

extern void vg_font_term(void *, uint32_t);

static INLINE bool vg_is_font(MEM_HANDLE_T handle)
{
   return mem_get_term(handle) == vg_font_term;
}

extern bool vg_font_insert(
   VG_FONT_T *font,
   uint32_t glyph_id,
   MEM_HANDLE_T object_handle,
   bool allow_autohinting,
   float origin_x, float origin_y,
   float escapement_x, float escapement_y);

extern bool vg_font_delete(
   VG_FONT_T *font,
   uint32_t glyph_id);

typedef struct {
   uint32_t capacity;
   uint32_t *entries;
   VG_GLYPH_T *slots;
} VG_FONT_LOCKED_T;

extern void vg_font_lock(
   VG_FONT_T *font,
   VG_FONT_LOCKED_T *font_locked);
extern void vg_font_unlock(
   VG_FONT_T *font);

extern VG_GLYPH_T *vg_font_locked_lookup(
   VG_FONT_LOCKED_T *font_locked,
   uint32_t glyph_id);

#endif
