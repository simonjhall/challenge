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
#include "middleware/khronos/vg/vg_font.h"
#include "middleware/khronos/vg/vg_image.h"
#include "middleware/khronos/vg/vg_path.h"
#include "middleware/khronos/vg/vg_stem.h"
#include "interface/khronos/common/khrn_int_util.h"

/******************************************************************************
helpers
******************************************************************************/

static INLINE uint32_t hash(uint32_t id, uint32_t capacity)
{
   return id & ((capacity * 2) - 1);
}

static uint32_t *get_entry(VG_FONT_LOCKED_T *font_locked, uint32_t id)
{
   uint32_t h;
   for (h = hash(id, font_locked->capacity);
      font_locked->entries[h] != SLOT_NONE;
      h = (h + 1) & ((font_locked->capacity * 2) - 1)) {
      if (font_locked->slots[font_locked->entries[h]].id == id) {
         return font_locked->entries + h;
      }
   }
   return NULL;
}

static INLINE VG_GLYPH_T *get_slot(VG_FONT_LOCKED_T *font_locked, uint32_t id)
{
   uint32_t *entry = get_entry(font_locked, id);
   return entry ? (font_locked->slots + *entry) : NULL;
}

static uint32_t *get_free_entry(VG_FONT_LOCKED_T *font_locked, uint32_t id)
{
   uint32_t h;
   for (h = hash(id, font_locked->capacity);
      font_locked->entries[h] != SLOT_NONE;
      h = (h + 1) & ((font_locked->capacity * 2) - 1)) ;
   return font_locked->entries + h;
}

static VG_GLYPH_T *alloc_slot(VG_FONT_T *font, VG_FONT_LOCKED_T *font_locked)
{
   VG_GLYPH_T *slot;
   vcos_assert(font->slots_free_head != SLOT_NONE);
   slot = font_locked->slots + font->slots_free_head;
   font->slots_free_head = slot->id;
   return slot;
}

static void free_slot(VG_FONT_T *font, VG_FONT_LOCKED_T *font_locked, VG_GLYPH_T *slot)
{
   slot->id = font->slots_free_head;
   font->slots_free_head = slot - font_locked->slots;
}

static bool increase_capacity(VG_FONT_T *font, uint32_t capacity)
{
   uint32_t i, old_capacity;
   MEM_HANDLE_T old_entries_handle, entries_handle;
   VG_FONT_LOCKED_T font_locked;
   uint32_t *old_entries;

   vcos_assert(capacity >= 2);
   vcos_assert(is_power_of_2(capacity));
   vcos_assert(capacity > font->capacity);

   old_capacity = font->capacity;
   old_entries_handle = font->entries;

   /*
      alloc new entries and grow slots
   */

   entries_handle = mem_alloc_ex((capacity * 2) * sizeof(uint32_t), alignof(uint32_t), MEM_FLAG_NO_INIT, "VG_FONT_T.entries", MEM_COMPACT_DISCARD);
   if (entries_handle == MEM_INVALID_HANDLE) {
      return false;
   }
   if (!mem_resize_ex(font->slots, capacity * sizeof(VG_GLYPH_T), MEM_COMPACT_DISCARD)) {
      mem_release(entries_handle);
      return false;
   }

   /*
      update struct
   */

   font->capacity = capacity;
   font->entries = entries_handle;

   /*
      clear new entries
      copy old entries across
      add new slots to free list
   */

   vg_font_lock(font, &font_locked);

   for (i = 0; i != (font_locked.capacity * 2); ++i) {
      font_locked.entries[i] = SLOT_NONE;
   }

   old_entries = (uint32_t *)mem_lock(old_entries_handle);
   for (i = 0; i != (old_capacity * 2); ++i) {
      if (old_entries[i] != SLOT_NONE) {
         VG_GLYPH_T *slot = font_locked.slots + old_entries[i];
         *get_free_entry(&font_locked, slot->id) = old_entries[i];
      }
   }
   mem_unlock(old_entries_handle);

   for (i = old_capacity; i != font_locked.capacity; ++i) {
      font_locked.slots[i].id = (i == (font_locked.capacity - 1)) ? font->slots_free_head : (i + 1);
   }
   font->slots_free_head = old_capacity;

   vg_font_unlock(font);

   /*
      free old entries
   */

   mem_release(old_entries_handle);

   return true;
}

static void glyph_init(VG_GLYPH_T *glyph,
   MEM_HANDLE_T object_handle,
   bool allow_autohinting,
   float origin_x, float origin_y,
   float escapement_x, float escapement_y)
{
   if (object_handle != MEM_INVALID_HANDLE) {
      mem_acquire(object_handle);
   }
   glyph->object = object_handle;
   glyph->allow_autohinting = allow_autohinting;
   glyph->origin[0] = origin_x;
   glyph->origin[1] = origin_y;
   glyph->escapement[0] = escapement_x;
   glyph->escapement[1] = escapement_y;
}

static void glyph_term(VG_GLYPH_T *glyph)
{
   if (glyph->object != MEM_INVALID_HANDLE) {
      mem_release(glyph->object);
   }
}

/******************************************************************************
font
******************************************************************************/

vcos_static_assert(sizeof(VG_FONT_BPRINT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_FONT_BPRINT_T) <= VG_STEM_ALIGN);

void vg_font_bprint_term(void *p, uint32_t size)
{
   UNUSED(p);
   UNUSED(size);
}

/*
   void vg_font_bprint_from_stem(MEM_HANDLE_T handle, int32_t glyphs_capacity)

   Converts a stem object into an object of type VG_FONT_BPRINT_T with the
   specified properties. Always succeeds

   Preconditions:

   handle is a valid handle to a stem object
   glyphs_capacity is >= 0

   Postconditions:

   handle is now a valid handle to an object of type VG_FONT_BPRINT_T
*/

void vg_font_bprint_from_stem(
   MEM_HANDLE_T handle,
   int32_t glyphs_capacity)
{
   VG_FONT_BPRINT_T *font_bprint;

   vcos_assert(vg_is_stem(handle));
   vcos_assert(glyphs_capacity >= 0); /* check performed on client-side */

   font_bprint = (VG_FONT_BPRINT_T *)mem_lock(handle);
   font_bprint->glyphs_capacity = glyphs_capacity;
   mem_unlock(handle);

   mem_set_desc(handle, "VG_FONT_BPRINT_T");
   mem_set_term(handle, vg_font_bprint_term);
}

vcos_static_assert(sizeof(VG_FONT_T) <= VG_STEM_SIZE);
vcos_static_assert(alignof(VG_FONT_T) <= VG_STEM_ALIGN);

void vg_font_term(void *p, uint32_t size)
{
   VG_FONT_T *font = (VG_FONT_T *)p;
   VG_FONT_LOCKED_T font_locked;
   uint32_t i;

   UNUSED(size);

   vg_font_lock(font, &font_locked);
   for (i = 0; i != (font_locked.capacity * 2); ++i) {
      if (font_locked.entries[i] != SLOT_NONE) {
         glyph_term(font_locked.slots + font_locked.entries[i]);
      }
   }
   vg_font_unlock(font);

   mem_release(font->entries);
   mem_release(font->slots);
}

/*
   bool vg_font_from_bprint(MEM_HANDLE_T handle)

   Converts an object of type VG_FONT_BPRINT_T to an object of type VG_FONT_T. A
   return value of false indicates failure

   Preconditions:

   handle is a valid handle to an object of type VG_FONT_BPRINT_T

   Postconditions:

   If the function succeeds:
   - true is returned
   - handle is now a valid handle to an object of type VG_FONT_T
   otherwise:
   - false is returned
   - handle is unchanged (ie it is still a valid handle to an object of type
     VG_FONT_BPRINT_T)
*/

bool vg_font_from_bprint(MEM_HANDLE_T handle)
{
   uint32_t i;
   VG_FONT_BPRINT_T *font_bprint;
   int32_t glyphs_capacity;
   MEM_HANDLE_T entries_handle, slots_handle;
   VG_FONT_T *font;
   VG_FONT_LOCKED_T font_locked;

   vcos_assert(vg_is_font_bprint(handle));

   /*
      get init params from blueprint
   */

   font_bprint = (VG_FONT_BPRINT_T *)mem_lock(handle);
   glyphs_capacity = font_bprint->glyphs_capacity;
   mem_unlock(handle);

   /*
      alloc entries and glyph slots
   */

   glyphs_capacity = clampi(glyphs_capacity, 2, 512);
   glyphs_capacity = 1 << (_msb(glyphs_capacity - 1) + 1);
   entries_handle = mem_alloc_ex((glyphs_capacity * 2) * sizeof(uint32_t), alignof(uint32_t), MEM_FLAG_NO_INIT, "VG_FONT_T.entries", MEM_COMPACT_DISCARD);
   if (entries_handle == MEM_INVALID_HANDLE) {
      return false;
   }
   slots_handle = mem_alloc_ex(glyphs_capacity * sizeof(VG_GLYPH_T), alignof(VG_GLYPH_T), (MEM_FLAG_T)(MEM_FLAG_RESIZEABLE | MEM_FLAG_NO_INIT | MEM_FLAG_HINT_GROW), "VG_FONT_T.slots", MEM_COMPACT_DISCARD);
   if (slots_handle == MEM_INVALID_HANDLE) {
      mem_release(entries_handle);
      return false;
   }

   /*
      fill in struct
   */

   font = (VG_FONT_T *)mem_lock(handle);

   font->capacity = glyphs_capacity;
   font->count = 0;
   font->entries = entries_handle;
   font->slots_free_head = 0;
   font->slots = slots_handle;

   /*
      clear entries and create slots free list
   */

   vg_font_lock(font, &font_locked);

   for (i = 0; i != (font_locked.capacity * 2); ++i) {
      font_locked.entries[i] = SLOT_NONE;
   }

   for (i = 0; i != font_locked.capacity; ++i) {
      font_locked.slots[i].id = (i == (font_locked.capacity - 1)) ? SLOT_NONE : (i + 1);
   }

   vg_font_unlock(font);

   mem_unlock(handle);

   mem_set_desc(handle, "VG_FONT_T");
   mem_set_term(handle, vg_font_term);

   return true;
}

bool vg_font_insert(
   VG_FONT_T *font,
   uint32_t glyph_id,
   MEM_HANDLE_T object_handle,
   bool allow_autohinting,
   float origin_x, float origin_y,
   float escapement_x, float escapement_y)
{
   VG_FONT_LOCKED_T font_locked;
   VG_GLYPH_T *slot;

   vg_font_lock(font, &font_locked);

   /*
      replace existing
   */

   slot = get_slot(&font_locked, glyph_id);
   if (slot) {
      glyph_term(slot);
      glyph_init(slot, object_handle, allow_autohinting, origin_x, origin_y, escapement_x, escapement_y);
      vg_font_unlock(font);
      return true;
   }

   /*
      no existing, add new
   */

   if (font->count == font->capacity) {
      vg_font_unlock(font);
      if (!increase_capacity(font, font->capacity * 2)) {
         return false;
      }
      vg_font_lock(font, &font_locked);
   }
   vcos_assert(font->count < font->capacity);
   ++font->count;

   slot = alloc_slot(font, &font_locked);
   slot->id = glyph_id;
   glyph_init(slot, object_handle, allow_autohinting, origin_x, origin_y, escapement_x, escapement_y);
   *get_free_entry(&font_locked, glyph_id) = slot - font_locked.slots;

   vg_font_unlock(font);

   return true;
}

bool vg_font_delete(
   VG_FONT_T *font,
   uint32_t glyph_id)
{
   VG_FONT_LOCKED_T font_locked;
   uint32_t *entry;
   VG_GLYPH_T *slot;
   uint32_t i, j;

   vg_font_lock(font, &font_locked);

   entry = get_entry(&font_locked, glyph_id);
   if (!entry) {
      vg_font_unlock(font);
      return false;
   }

   vcos_assert(font->count != 0);
   --font->count;

   slot = font_locked.slots + *entry;
   glyph_term(slot);
   free_slot(font, &font_locked, slot);
   *entry = SLOT_NONE;

   i = entry - font_locked.entries;
   for (j = (i + 1) & ((font_locked.capacity * 2) - 1);
      font_locked.entries[j] != SLOT_NONE;
      j = (j + 1) & ((font_locked.capacity * 2) - 1)) {
      uint32_t h = hash(font_locked.slots[font_locked.entries[j]].id, font_locked.capacity);
      if ((j > i) ? ((h <= i) || (h > j)) : ((h > j) && (h <= i))) {
         font_locked.entries[i] = font_locked.entries[j];
         font_locked.entries[j] = SLOT_NONE;
         i = j;
      }
   }

   vg_font_unlock(font);

   return true;
}

void vg_font_lock(
   VG_FONT_T *font,
   VG_FONT_LOCKED_T *font_locked)
{
   font_locked->capacity = font->capacity;
   font_locked->entries = (uint32_t *)mem_lock(font->entries);
   font_locked->slots = (VG_GLYPH_T *)mem_lock(font->slots);
}

void vg_font_unlock(
   VG_FONT_T *font)
{
   mem_unlock(font->slots);
   mem_unlock(font->entries);
}

VG_GLYPH_T *vg_font_locked_lookup(
   VG_FONT_LOCKED_T *font_locked,
   uint32_t glyph_id)
{
   return get_slot(font_locked, glyph_id);
}
