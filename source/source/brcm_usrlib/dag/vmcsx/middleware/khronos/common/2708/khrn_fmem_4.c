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
#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/2708/khrn_fmem_4.h"
#include "middleware/khronos/common/2708/khrn_interlock_priv_4.h"
#include "middleware/khronos/vg/vg_ramp.h"
#include "interface/vcos/vcos.h"
#include <stddef.h> /* for offsetof */

#define GAP 5

typedef struct
{
   uint32_t caller_data[6];
   KHRN_FMEM_T *fmem;
   KHRN_HW_CALLBACK_T callback;
} KHRN_FMEM_CALLBACK_DATA_T;

static void tweak_init(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list);
static KHRN_FMEM_TWEAK_T *tweak_new(KHRN_FMEM_T *fmem);
static KHRN_FMEM_TWEAK_T *tweak_next(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list);
static void tweak_close(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list);
static bool alloc_next(KHRN_FMEM_T *fmem);
static bool fix(KHRN_FMEM_T *fmem, uint8_t *location, MEM_HANDLE_T handle, uint32_t offset);
static void alloc_callback(KHRN_HW_CALLBACK_REASON_T reason, void *data_, const uint32_t *specials);
static void do_fix_lock(KHRN_FMEM_T *fmem);
static void do_fix_unlock(KHRN_FMEM_T *fmem);
static void do_specials(KHRN_FMEM_T *fmem, const uint32_t *specials);
static void do_interlock_transfer(KHRN_FMEM_T *fmem);
static void do_interlock_release(KHRN_FMEM_T *fmem);
static void do_ramp_lock(KHRN_FMEM_T *fmem);
static void do_ramp_unlock(KHRN_FMEM_T *fmem);
static uint32_t *alloc_junk(KHRN_FMEM_T *fmem, int size, int align);

static bool inited = false;

KHRN_FMEM_T *khrn_fmem_init(uint32_t render_state_n)
{
   uint32_t *block;
   KHRN_NMEM_GROUP_T temp_nmem_group;
   KHRN_FMEM_T *fmem;

   khrn_nmem_group_init(&temp_nmem_group, true);

   block = (uint32_t *)khrn_nmem_group_alloc_master(&temp_nmem_group);
   if (!block) return NULL;

   fmem = (KHRN_FMEM_T *)block;
   fmem->nmem_group = temp_nmem_group;
   block += sizeof(KHRN_FMEM_T)/4;
   fmem->nmem_entered = false;

   fmem->bin_begin = (uint8_t *)block;
   fmem->bin_end = NULL;
   fmem->render_begin = NULL;
   fmem->cle_pos = fmem->bin_begin;
   fmem->last_cle_pos = NULL;
   fmem->junk_pos = block + ((KHRN_NMEM_GROUP_BLOCK_SIZE-sizeof(KHRN_FMEM_T))/4);

   fmem->fix_start = (KHRN_FMEM_FIX_T *)alloc_junk(fmem, sizeof(KHRN_FMEM_FIX_T), 4);
   vcos_assert(fmem->fix_start);  /* Should be enough room in initial block that this didn't fail */
   fmem->fix_end = fmem->fix_start;
   fmem->fix_end->count = 0;
   fmem->fix_end->next = NULL;

   tweak_init(fmem, &fmem->special);
   tweak_init(fmem, &fmem->interlock);
   tweak_init(fmem, &fmem->ramp);

   fmem->render_state_n = render_state_n;

   return fmem;
}

void khrn_fmem_discard(KHRN_FMEM_T *fmem)
{
   vcos_assert(!fmem->nmem_entered);
   tweak_close(fmem, &fmem->interlock);
   do_interlock_release(fmem);
   khrn_nmem_group_term(&fmem->nmem_group);
}

static uint32_t *alloc_junk(KHRN_FMEM_T *fmem, int size, int align)
{
   size_t new_junk;

   vcos_assert(!((size_t)fmem->junk_pos & 3) && !(size & 3) && align >= 4 && align <= 4096);
   vcos_assert(fmem->cle_pos + GAP <= (uint8_t *)fmem->junk_pos);

   new_junk = ((size_t)fmem->junk_pos - size) & ~(align - 1);
   if ((size_t)fmem->cle_pos + GAP > new_junk)
   {
      if (!alloc_next(fmem)) return NULL;
      new_junk = ((size_t)fmem->junk_pos - size) & ~(align - 1);
   }

   vcos_assert((size_t)fmem->cle_pos + GAP <= new_junk);

   fmem->junk_pos = (uint32_t *)new_junk;
   return (uint32_t *)new_junk;
}

uint32_t *khrn_fmem_junk(KHRN_FMEM_T *fmem, int size, int align)
{
   uint32_t *result;
   result = alloc_junk(fmem, size, align);
   return result;
}

uint8_t *khrn_fmem_cle(KHRN_FMEM_T *fmem, int size)
{
   uint8_t *result;

   vcos_assert(fmem->cle_pos + GAP <= (uint8_t *)fmem->junk_pos);

   if (fmem->cle_pos + size + GAP > (uint8_t *)fmem->junk_pos)
   {
      if (!alloc_next(fmem)) return NULL;
   }

   result = fmem->cle_pos;
   fmem->cle_pos += size;

   fmem->last_cle_pos = fmem->cle_pos;

   return result;
}

bool khrn_fmem_fix(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset)
{
   bool result;
   result = fix(fmem, (uint8_t *)location, handle, offset);
   return result;
}

bool khrn_fmem_add_fix(KHRN_FMEM_T *fmem, uint8_t **p, MEM_HANDLE_T handle, uint32_t offset)
{
   if (fix(fmem, *p, handle, offset))
   {
      *p += 4;
      return true;
   }
   /* vcos_assert(0);      This is a valid out-of-memory condition */
   return false;
}

static bool fix(KHRN_FMEM_T *fmem, uint8_t *location, MEM_HANDLE_T handle, uint32_t offset)
{
   uint32_t count;

   count = fmem->fix_end->count;
   vcos_assert(count <= TWEAK_COUNT);
   if (count == TWEAK_COUNT)
   {
      KHRN_FMEM_FIX_T *f;

      f = (KHRN_FMEM_FIX_T *)alloc_junk(fmem, sizeof(KHRN_FMEM_FIX_T), 4);
      if (!f) return false;

      f->count = 0;
      f->next = NULL;
      count = 0;
      fmem->fix_end->next = f;
      fmem->fix_end = f;
   }

   vcos_assert(count < TWEAK_COUNT);

   fmem->fix_end->handles[count].mh_handle = handle;
   fmem->fix_end->handles[count].offset = offset;
   fmem->fix_end->locations[count] = location;
   fmem->fix_end->count = count + 1;

   mem_acquire_retain(handle);
   return true;
}

bool khrn_fmem_add_special(KHRN_FMEM_T *fmem, uint8_t **p, uint32_t special_i, uint32_t offset)
{
   KHRN_FMEM_TWEAK_T *tw;

   tw = tweak_next(fmem, &fmem->special);
   if (!tw) return false;

   tw->special.location = *p;
   tw->special.special_i = special_i;
   add_word(p, offset);

   return true;
}

bool khrn_fmem_interlock(KHRN_FMEM_T *fmem, MEM_HANDLE_T handle, uint32_t offset)
{
   KHRN_FMEM_TWEAK_T *tw;

   tw = tweak_next(fmem, &fmem->interlock);
   if (!tw) return false;

   tw->interlock.mh_handle = handle;
   tw->interlock.offset = offset;

   mem_acquire(handle);

   return true;
}

bool khrn_fmem_start_render(KHRN_FMEM_T *fmem)
{
   //uint8_t *instr;

   vcos_assert(fmem->bin_end == NULL && fmem->render_begin == NULL);

   fmem->bin_end = fmem->cle_pos;

#if 0
   instr = khrn_fmem_cle(fmem, 1);
   if (!instr)
   {
      vcos_assert(0);
      return false;
   }
   add_byte(&instr, 0); /* Not executed. Just marks gap between bin and render list. */
#endif

   fmem->render_begin = fmem->cle_pos;
   return true;
}

void *khrn_fmem_queue(
   KHRN_FMEM_T *fmem,
   KHRN_HW_CC_FLAG_T bin_cc, KHRN_HW_CC_FLAG_T render_cc,
   uint32_t frame_count,
   uint32_t special_0,
   uint32_t bin_mem_size,
   uint32_t actual_user_vpm,
   uint32_t max_user_vpm,
   KHRN_HW_TYPE_T type,
   KHRN_HW_CALLBACK_T callback,
   uint32_t callback_data_size)
{
	unsigned char *temp_bin, *temp_render;
	unsigned char *temp_bin2, *temp_render2;
	int bin_length, render_length;
	int count;

   KHRN_FMEM_CALLBACK_DATA_T *callback_data;

   vcos_assert(fmem->bin_end != NULL && fmem->render_begin != NULL);
   vcos_assert(callback_data_size <= sizeof(callback_data->caller_data));

   tweak_close(fmem, &fmem->special);
   tweak_close(fmem, &fmem->interlock);
   tweak_close(fmem, &fmem->ramp);

   do_interlock_transfer(fmem);

#ifdef SIMPENROSE_RECORD_OUTPUT
   record_map_buffer(khrn_hw_addr(fmem->bin_begin), (uint8_t *)fmem->cle_pos - (uint8_t *)fmem->bin_begin, L_INSTRUCTIONS, RECORD_BUFFER_IS_BOTH, 1);
#endif

   fmem->nmem_entered = true;
   fmem->nmem_pos = khrn_nmem_enter();

   bin_length = (char *)fmem->bin_end - (char *)fmem->bin_begin;
   render_length = (char *)fmem->cle_pos - (char *)fmem->render_begin;

#ifdef MEGA_DEBUG
   temp_bin = (unsigned char *)malloc(bin_length);
   temp_render = (unsigned char *)malloc(render_length);
   temp_bin2 = (unsigned char *)malloc(bin_length);
   temp_render2 = (unsigned char *)malloc(render_length);

   memcpy(temp_bin, fmem->bin_begin, bin_length);
   memcpy(temp_render, fmem->render_begin, render_length);

   assert((memcmp(temp_bin, temp_bin, bin_length) == 0));
   assert((memcmp(temp_render, temp_render, render_length) == 0));

   for (count = 0; count < 10; count++)
	   printf("B%d: %08x %08x\n", count, ((int *)temp_bin)[count], ((int *)temp_render)[count]);
#endif

  /* want all the vpm we can get */
  /* but we only need 120 rows to ensure we don't lock up (32 for
    * attributes, 16 for shaded coordinates, 72 for shaded vertices) */
   callback_data = (KHRN_FMEM_CALLBACK_DATA_T *)khrn_hw_queue(
      khrn_hw_alias_direct(fmem->bin_begin),    khrn_hw_alias_direct(fmem->bin_end), bin_cc,
      khrn_hw_alias_direct(fmem->render_begin), khrn_hw_alias_direct(fmem->cle_pos), render_cc,
      frame_count,
      special_0,
      bin_mem_size,
      actual_user_vpm,
      max_user_vpm,
      type,
#ifdef BRCM_V3D_OPT
      alloc_callback,callback,fmem, sizeof(KHRN_FMEM_CALLBACK_DATA_T));
#else
      alloc_callback, sizeof(KHRN_FMEM_CALLBACK_DATA_T));
#endif

   callback_data->fmem = fmem;
   callback_data->callback = callback;

#ifdef MEGA_DEBUG
   memcpy(temp_bin2, fmem->bin_begin, bin_length);
   memcpy(temp_render2, fmem->render_begin, render_length);

   for (count = 0; count < 10; count++)
	   printf("A%d: %08x %08x\n", count, ((int *)temp_bin)[count], ((int *)temp_render)[count]);

   if (memcmp(temp_bin, temp_bin2, bin_length) == 0)
	   printf("binning is equal\n");
   else
	   printf("binning is not equal\n");

   if (memcmp(temp_render, temp_render2, render_length) == 0)
	   printf("render is equal\n");
   else
	   printf("render is not equal\n");

   free(temp_bin);
   free(temp_bin2);
   free(temp_render);
   free(temp_render2);
#endif



#ifdef XXX_OFFSET
   /*khrn_hw_ready_with_user_shader(true, callback_data,
      next_user_shader.handle, next_user_shader.offset,
      next_user_unif.handle, next_user_unif.offset);*/
#endif
   //khrn_hw_ready(true, callback_data);

   return callback_data;
}


static void alloc_callback(KHRN_HW_CALLBACK_REASON_T reason, void *data_, const uint32_t *specials)
{
   KHRN_FMEM_CALLBACK_DATA_T *data = (KHRN_FMEM_CALLBACK_DATA_T *)data_;
   switch (reason) {
   case KHRN_HW_CALLBACK_REASON_FIXUP:
   {
      vcos_assert(specials != NULL);
      do_fix_lock(data->fmem);
      do_ramp_lock(data->fmem);
      do_specials(data->fmem, specials);

      if (data->callback) data->callback(reason, data_, specials);

      khrn_hw_fixup_done(true, data_);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_UNFIXUP:
   {
      // Don't call caller?
      /* todo: should only be unlocking here. unretain + unrelease should happen
       * in out of mem / render finished. i think. would be nice if we could do
       * it all in one call to reduce overhead though... */
      do_fix_unlock(data->fmem);
      do_ramp_unlock(data->fmem);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_BIN_FINISHED:
   {
      if (data->callback) data->callback(reason, data_, specials);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_OUT_OF_MEM:
   case KHRN_HW_CALLBACK_REASON_RENDER_FINISHED:
   {
      if (data->callback) data->callback(reason, data_, specials);

      //vcos_assert((uint32_t)data->fmem != 0x494e34);
      /* todo: we want to do this on the llat thread, but we need to do all the
       * unlocking/unretaining/releasing first, which currently isn't safe to do
       * on the llat thread... */
      vcos_assert(data->fmem->nmem_entered);
      khrn_nmem_group_term_and_exit(&data->fmem->nmem_group, data->fmem->nmem_pos);
      break;
   }
   case KHRN_HW_CALLBACK_REASON_OUT_OF_MEM_LLAT:
   case KHRN_HW_CALLBACK_REASON_BIN_FINISHED_LLAT:
   {
      if (data->callback) data->callback(reason, data_, specials);
      break;
   }
   default:
   {
      UNREACHABLE();
   }
   }
}



static bool alloc_next(KHRN_FMEM_T *fmem)
{
   uint32_t *block;

   //vcos_assert(fmem->cle_pos + GAP <= (uint8_t *)fmem->junk_pos);

   block = (uint32_t *)khrn_nmem_group_alloc_master(&fmem->nmem_group);
   if (!block) return false;

   Add_byte(&fmem->cle_pos, KHRN_HW_INSTR_BRANCH);
   add_pointer(&fmem->cle_pos, block);

   fmem->cle_pos = (uint8_t *)block;
   fmem->junk_pos = block + (KHRN_NMEM_GROUP_BLOCK_SIZE/4);
   return true;
}

static void tweak_init(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list)
{
   list->start = tweak_new(fmem);
   list->header = list->start;
   list->i = 0;
   vcos_assert(list->start);   /* Should be enough room in initial block that this didn't fail */
}

static void tweak_close(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list)
{
   list->header->header.count = list->i;
}

static KHRN_FMEM_TWEAK_T *tweak_new(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_TWEAK_T *result;

   result = (KHRN_FMEM_TWEAK_T *)alloc_junk(fmem, sizeof(KHRN_FMEM_TWEAK_T) * TWEAK_COUNT, 4);
   if (!result) return NULL;

   result->header.next = NULL;
   result->header.count = TWEAK_COUNT-1;

   return result;
}

static KHRN_FMEM_TWEAK_T *tweak_next(KHRN_FMEM_T *fmem, KHRN_FMEM_TWEAK_LIST_T *list)
{
   if (list->i >= list->header->header.count)
   {
      KHRN_FMEM_TWEAK_T *tw = tweak_new(fmem);
      if (!tw) return NULL;

      vcos_assert(list->header->header.count == list->i);
      list->header->header.next = tw;
      list->header = tw;
      list->i = 0;
   }

   list->i++;
   return &list->header[list->i];
}

static void do_fix_lock(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_FIX_T *f;
   uint32_t i;
   uint32_t count;
   void *pointers[TWEAK_COUNT];

   for (f = fmem->fix_start; f != NULL; f = f->next)
   {
      count = f->count;
      mem_lock_multiple_phys(pointers, f->handles, count);
      for (i = 0; i < count; i++)
      {
#ifdef MEGA_DEBUG
    	  printf("put lock %p=%08x\n", f->locations[i], (khrn_hw_alias_direct(pointers[i])));
#endif
         put_word(f->locations[i], (khrn_hw_alias_direct(pointers[i])));    //PTR
      }
   }
}

static void do_fix_unlock(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_FIX_T *f;

   for (f = fmem->fix_start; f != NULL; f = f->next)
   {
      mem_unlock_unretain_release_multiple(f->handles, f->count);
   }
}

static void do_specials(KHRN_FMEM_T *fmem, const uint32_t *specials)
{
   KHRN_FMEM_TWEAK_T *h;
   uint32_t i;
   uint32_t w;

   for (h = fmem->special.start; h != NULL; h = h->header.next)
   {
      for (i = 1; i <= h->header.count; i++)
      {
         w = get_word(h[i].special.location);
         w += specials[h[i].special.special_i];
#ifdef MEGA_DEBUG
         printf("put special %p=%08x\n", h[i].special.location, w);
#endif
         put_word(h[i].special.location, w);
      }
   }
}

static void do_interlock_transfer(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_TWEAK_T *h;
   uint32_t i;
   KHRN_INTERLOCK_T *interlock;

   for (h = fmem->interlock.start; h != NULL; h = h->header.next)
   {
      for (i = 1; i <= h->header.count; i++)
      {
         interlock = (KHRN_INTERLOCK_T *)((uint8_t *)mem_lock(h[i].interlock.mh_handle) + h[i].interlock.offset);
         khrn_interlock_transfer(interlock, khrn_interlock_user(fmem->render_state_n), KHRN_INTERLOCK_FIFO_HW_RENDER);
         mem_unlock(h[i].interlock.mh_handle);
         mem_release(h[i].interlock.mh_handle);
      }
   }
}

static void do_interlock_release(KHRN_FMEM_T *fmem) {
   KHRN_FMEM_TWEAK_T *h;
   uint32_t i;
   KHRN_INTERLOCK_T *interlock;

   for (h = fmem->interlock.start; h != NULL; h = h->header.next)
   {
      for (i = 1; i <= h->header.count; i++)
      {
         interlock = (KHRN_INTERLOCK_T *)((uint8_t *)mem_lock(h[i].interlock.mh_handle) + h[i].interlock.offset);
         khrn_interlock_release(interlock, khrn_interlock_user(fmem->render_state_n));
         mem_unlock(h[i].interlock.mh_handle);
         mem_release(h[i].interlock.mh_handle);
      }
   }
}

static void do_ramp_lock(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_TWEAK_T *h;
   uint32_t i;
   VG_RAMP_T *ramp;

   for (h = fmem->ramp.start; h != NULL; h = h->header.next)
   {
      for (i = 1; i <= h->header.count; i++)
      {
         ramp = (VG_RAMP_T *)mem_lock(h[i].ramp.handle);
         *h[i].ramp.location += khrn_hw_addr(mem_lock(ramp->data));
         mem_unlock(h[i].ramp.handle);
      }
   }
}

static void do_ramp_unlock(KHRN_FMEM_T *fmem)
{
   KHRN_FMEM_TWEAK_T *h;
   uint32_t i;
   VG_RAMP_T *ramp;

   for (h = fmem->ramp.start; h != NULL; h = h->header.next)
   {
      for (i = 1; i <= h->header.count; i++)
      {
         ramp = (VG_RAMP_T *)mem_lock(h[i].ramp.handle);
         mem_unlock(ramp->data);
         mem_unlock(h[i].ramp.handle);
         vg_ramp_unretain(h[i].ramp.handle); /* todo: this locks internally */
         vg_ramp_release(h[i].ramp.handle, h[i].ramp.i);
      }
   }
}



bool khrn_fmem_special(KHRN_FMEM_T *fmem, uint32_t *location, uint32_t special_i, uint32_t offset)
{
   uint8_t *p = (uint8_t *)location;
   return khrn_fmem_add_special(fmem, &p, special_i, offset);
}

bool khrn_fmem_is_here(KHRN_FMEM_T *fmem, uint8_t *p)
{
   return fmem->last_cle_pos == p;
}

bool khrn_fmem_ramp(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset, uint32_t ramp_i)
{
   KHRN_FMEM_TWEAK_T *tw;

   tw = tweak_next(fmem, &fmem->ramp);
   if (!tw) return false;

   *location = offset;

   tw->ramp.location = location;
   tw->ramp.handle = handle;
   tw->ramp.i = ramp_i;

   vg_ramp_acquire(handle, ramp_i);

   return true;
}

bool khrn_fmem_fix_special_0(KHRN_FMEM_T *fmem, uint32_t *location, MEM_HANDLE_T handle, uint32_t offset)
{
   uint8_t *p = (uint8_t *)location;
   return khrn_fmem_add_fix_special_0(fmem, &p, handle, offset);
}

bool khrn_fmem_add_fix_special_0(KHRN_FMEM_T *fmem, uint8_t **p, MEM_HANDLE_T handle, uint32_t offset)
{
   uint8_t *p2 = *p;
   if (!khrn_fmem_add_special(fmem, &p2, KHRN_FMEM_SPECIAL_0, 0)) return false;

   if (handle == MEM_INVALID_HANDLE)
   {
      add_word(p, offset);
      return true;
   }
   else
   {
      return khrn_fmem_add_fix(fmem, p, handle, offset);
   }
}

bool khrn_fmem_add_fix_image(KHRN_FMEM_T *fmem, uint8_t **p, uint32_t render_i, MEM_HANDLE_T image_handle, uint32_t offset)
{
   bool result;
   KHRN_IMAGE_T *image;
   if (!khrn_fmem_interlock(fmem, image_handle, offsetof(KHRN_IMAGE_T, interlock)))
   {
      return false;
   }
   image = (KHRN_IMAGE_T *)mem_lock(image_handle);
   //khrn_interlock_read(&image->interlock, render_i);

   result = khrn_fmem_add_fix(fmem, p, image->mh_storage, image->offset + offset);
   mem_unlock(image_handle);
   return result;
}
