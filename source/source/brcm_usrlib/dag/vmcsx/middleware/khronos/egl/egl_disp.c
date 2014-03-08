//#define EGL_DISP_FPS

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
#include "interface/khronos/common/khrn_int_ids.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_image.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"
#include "middleware/khronos/egl/egl_disp.h"
#include "middleware/khronos/egl/egl_platform.h"

/* if it takes longer than this to call egl_server_platform_display after we
 * get a callback, we're probably going to miss a v-sync and not get a solid 60
 * fps. so we kick the clocks in an attempt to get that solid 60. todo: this
 * assumes a 60 hz screen refresh rate and content that is trying to render
 * that fast (it also assumes kicking the clock will help and not just waste
 * power) */
#define PERMITTED_WANT_RESPONSE_US 15000

/******************************************************************************
handle management stuff
******************************************************************************/

#define DISPS_N_MAX 8
vcos_static_assert(DISPS_N_MAX <= 32);
#define IMAGES_N_MAX 4 /* this should be a power of 2 */
#define SLOTS_N (2 * IMAGES_N_MAX) /* this should be a power of 2 multiple of IMAGES_N_MAX */

typedef union {
   struct {
      /* for async back-pressure stuff */
      uint64_t pid;
      uint32_t sem;

      uint32_t n;
      MEM_HANDLE_T images[IMAGES_N_MAX];

      /* last window handle given to platform. updated by llat task */
      uint32_t last_win;

      /* post counter. updated by master task. read by llat task */
      uint32_t post;

      /* currently-on-the-display pointer. updated by display callback. read by
       * many tasks */
      uint32_t on;

      /* outstanding asyncs. incremented by display callback. decremented by
       * llat task */
      int32_t asyncs;

      /* send something to the display? set by display callback. cleared by
       * llat task */
      bool want;

      /* set by display callback, to vcos_getmicrosecs(), just before want is
       * set to true */
      uint32_t want_us;

      /* set by master task. cleared by llat task */
      bool finish;

      struct {
         KHRN_IMAGE_T image;
         uint32_t win; /* window handle */
         uint32_t swap_interval; /* updated by display callback */
         bool ready; /* ok to send to display? */
         bool skip; /* don't actually send to display (only with a swap_interval of 0) */
         bool wait_posted;
      } slots[SLOTS_N];
   } in_use;
   EGL_DISP_HANDLE_T free_next;
} DISP_T;

static uint32_t disps_n; /* kept track of for debugging purposes only */
static DISP_T disps[DISPS_N_MAX];
static EGL_DISP_HANDLE_T disps_free_head;

static INLINE EGL_DISP_HANDLE_T disp_to_handle(DISP_T *disp)
{
   vcos_assert(disp >= disps);
   vcos_assert(disp < (disps + DISPS_N_MAX));
   return disp - disps;
}

static INLINE DISP_T *disp_from_handle(EGL_DISP_HANDLE_T disp_handle)
{
   vcos_assert(disp_handle < DISPS_N_MAX);
   return disps + disp_handle;
}

static EGL_DISP_HANDLE_T alloc_disp_handle(void)
{
   EGL_DISP_HANDLE_T disp_handle = disps_free_head;
   if (disp_handle != EGL_DISP_HANDLE_INVALID) {
      ++disps_n;
      disps_free_head = disp_from_handle(disp_handle)->free_next;
   }
   return disp_handle;
}

static void free_disp_handle(EGL_DISP_HANDLE_T disp_handle)
{
   --disps_n;
   disp_from_handle(disp_handle)->free_next = disps_free_head;
   disps_free_head = disp_handle;
}

static INLINE EGL_DISP_SLOT_HANDLE_T egl_disp_slot_handle(
   EGL_DISP_HANDLE_T disp_handle, uint32_t slot)
{
   vcos_assert(!(disp_handle & ~0xff));
   vcos_assert(!(slot & ~0xff));
   return (EGL_DISP_SLOT_HANDLE_T)(disp_handle | (slot << 8));
}

static INLINE DISP_T *slot_handle_get_disp(EGL_DISP_SLOT_HANDLE_T slot_handle)
{
   vcos_assert(slot_handle != EGL_DISP_SLOT_HANDLE_INVALID);
   return disps + (slot_handle & 0xff);
}

static INLINE uint32_t slot_handle_get_slot(EGL_DISP_SLOT_HANDLE_T slot_handle)
{
   vcos_assert(slot_handle != EGL_DISP_SLOT_HANDLE_INVALID);
   return slot_handle >> 8;
}

static INLINE EGL_DISP_IMAGE_HANDLE_T egl_disp_image_handle(
   EGL_DISP_HANDLE_T disp_handle, uint32_t i)
{
   vcos_assert(!(disp_handle & ~0xff));
   vcos_assert(!(i & ~0xff));
   return (EGL_DISP_IMAGE_HANDLE_T)(disp_handle | (i << 8));
}

static INLINE DISP_T *image_handle_get_disp(EGL_DISP_IMAGE_HANDLE_T image_handle)
{
   vcos_assert(image_handle != EGL_DISP_IMAGE_HANDLE_INVALID);
   return disps + (image_handle & 0xff);
}

static INLINE uint32_t image_handle_get_i(EGL_DISP_IMAGE_HANDLE_T image_handle)
{
   vcos_assert(image_handle != EGL_DISP_IMAGE_HANDLE_INVALID);
   return image_handle >> 8;
}

/******************************************************************************
posting
******************************************************************************/

static INLINE uint32_t next_pos(uint32_t pos, uint32_t n)
{
   ++pos;
   if (!is_power_of_2(n)) {
      uint32_t p = 1 << (_msb(n) + 1);
      if ((pos & (p - 1)) == n) {
         pos += p - n;
      }
   }
   return pos;
}

static INLINE void advance_pos(uint32_t *pos, uint32_t n)
{
   khrn_barrier();
   *pos = next_pos(*pos, n);
}

/* call from master task. caller should notify llat task if necessary */
static uint32_t post(DISP_T *disp, uint32_t win, uint32_t swap_interval)
{
   uint32_t slot;
   MEM_HANDLE_T handle;

   /* wait for a free slot */
   while ((next_pos(disp->in_use.post, disp->in_use.n) - disp->in_use.on) > SLOTS_N) {
      khrn_sync_master_wait();
   }
   khrn_barrier();

   /* fill it in */
   slot = disp->in_use.post & (SLOTS_N - 1);
   /* we take a copy of the KHRN_IMAGE_T here as in the swap interval 0 case the
    * width/height/stride could change before we get around to putting the image
    * on the display */
   handle = disp->in_use.images[slot & (next_power_of_2(disp->in_use.n) - 1)];
   disp->in_use.slots[slot].image = *(KHRN_IMAGE_T *)mem_lock(handle);
   mem_unlock(handle);
   disp->in_use.slots[slot].win = win;
   disp->in_use.slots[slot].swap_interval = swap_interval;
   disp->in_use.slots[slot].ready = false;
   disp->in_use.slots[slot].skip = false;
   disp->in_use.slots[slot].wait_posted = false;

   /* advance the post counter */
   advance_pos(&disp->in_use.post, disp->in_use.n);

   return slot;
}

static uint32_t get_last_pos(DISP_T *disp, uint32_t i)
{
   uint32_t j = disp->in_use.post & (next_power_of_2(disp->in_use.n) - 1);
   return disp->in_use.post - ((i < j) ? (j - i) : ((next_power_of_2(disp->in_use.n) - i) + j));
}

static bool on_passed(DISP_T *disp, uint32_t pos)
{
   if ((int32_t)(disp->in_use.on - pos) > 0) {
      khrn_barrier();
      return true;
   }
   return false;
}

/******************************************************************************
llat stuff
******************************************************************************/

static VCOS_ATOMIC_FLAGS_T llat_flags;
static uint32_t llat_i;

static INLINE void notify_llat(DISP_T *disp)
{
   khrn_barrier();
   vcos_atomic_flags_or(&llat_flags, 1 << (disp - disps));
   khrn_llat_notify(llat_i);
}

static void egl_disp_llat_callback(void)
{
   uint32_t flags = vcos_atomic_flags_get_and_clear(&llat_flags);
   khrn_barrier();

   while (flags) {
      uint32_t i;
      DISP_T *disp;

      i = _msb(flags);
      flags &= ~(1 << i);
      disp = disps + i;

      /* do asyncs before we (potentially) provoke another callback */
      if (disp->in_use.asyncs > 0) {
         vcos_assert(disp->in_use.asyncs == 1);
         disp->in_use.asyncs = 0;
         khdispatch_send_async(ASYNC_COMMAND_POST, disp->in_use.pid, disp->in_use.sem);
      }

      /* try to send something to the display */
      if (disp->in_use.want) {
         uint32_t slot;
         khrn_barrier();
         slot = disp->in_use.on & (SLOTS_N - 1);
         if (disp->in_use.slots[slot].swap_interval == 0) {
            uint32_t next_on = next_pos(disp->in_use.on, disp->in_use.n);
            if (next_on != disp->in_use.post) {
               khrn_barrier();
               slot = next_on & (SLOTS_N - 1);
               if (disp->in_use.slots[slot].ready) {
                  khrn_barrier();
               } else {
                  /* next image not ready yet */
                  slot = -1;
               }
            } else {
               /* no more images to display */
               slot = -1;
               if (disp->in_use.finish) {
                  /* master task can continue now... */
                  khrn_barrier();
                  disp->in_use.finish = false;
                  khrn_sync_notify_master();
               }
            }
         } /* else: display current image again */
         if (slot != (uint32_t)-1) {
            EGL_DISP_SLOT_HANDLE_T slot_handle = egl_disp_slot_handle(disp_to_handle(disp), slot);
            bool swap_interval_0 = disp->in_use.slots[slot].swap_interval == 0;
            disp->in_use.want = false;
            if ((vcos_getmicrosecs() - disp->in_use.want_us) > PERMITTED_WANT_RESPONSE_US) {
               khrn_hw_kick();
            }
            if (disp->in_use.slots[slot].skip) {
               vcos_assert(swap_interval_0);
            } else {
               /* todo: what should happen when the window handle changes? */
               disp->in_use.last_win = disp->in_use.slots[slot].win;
               egl_server_platform_display(disp->in_use.slots[slot].win,
                  &disp->in_use.slots[slot].image,
                  (uint32_t)(swap_interval_0 ? EGL_DISP_SLOT_HANDLE_INVALID :
                  slot_handle));
            }
            if (swap_interval_0) {
               egl_disp_callback((uint32_t)slot_handle);
            }
         }
      }
   }
}

/******************************************************************************
display callback
******************************************************************************/

void egl_disp_callback(uint32_t cb_arg)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   EGL_DISP_SLOT_HANDLE_T slot_handle = (EGL_DISP_SLOT_HANDLE_T)cb_arg;
   DISP_T *disp;
   uint32_t slot;

   if (slot_handle == EGL_DISP_SLOT_HANDLE_INVALID) {
      return;
   }
   disp = slot_handle_get_disp(slot_handle);
   slot = slot_handle_get_slot(slot_handle);

   if ((disp->in_use.on & (SLOTS_N - 1)) != slot) {
      /* the previous image has just come off the display. we want to:
       * - send an async message back to the client
       * - notify any tasks that may have been waiting for the image to come off
       *   the display */
      ++disp->in_use.asyncs; /* we notify the llat task below */
      advance_pos(&disp->in_use.on, disp->in_use.n);
      vcos_assert((disp->in_use.on & (SLOTS_N - 1)) == slot);
      khrn_sync_display_returned_notify();
   }

   if (disp->in_use.slots[slot].swap_interval != 0) {
      --disp->in_use.slots[slot].swap_interval;
   }
   disp->in_use.want_us = vcos_getmicrosecs();
   khrn_barrier();
   disp->in_use.want = true;
   notify_llat(disp);
}

/******************************************************************************
interface
******************************************************************************/

bool egl_disp_init(void)
{
#ifdef BRCM_V3D_OPT
	return true;
#endif
   uint32_t i;

   disps_n = 0;
   for (i = 0; i != (DISPS_N_MAX - 1); ++i) {
      disps[i].free_next = disp_to_handle(disps + i + 1);
   }
   disps[DISPS_N_MAX - 1].free_next = EGL_DISP_HANDLE_INVALID;
   disps_free_head = disp_to_handle(disps + 0);

   if (vcos_atomic_flags_create(&llat_flags) != VCOS_SUCCESS) {
      return false;
   }

   llat_i = khrn_llat_register(egl_disp_llat_callback);
   vcos_assert(llat_i != (uint32_t)-1);

   return true;
}

void egl_disp_term(void)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   khrn_llat_unregister(llat_i);

   vcos_assert(!vcos_atomic_flags_get_and_clear(&llat_flags));
   vcos_atomic_flags_delete(&llat_flags);

   vcos_assert(disps_n == 0);
}

EGL_DISP_HANDLE_T egl_disp_alloc(
   uint64_t pid, uint32_t sem,
   uint32_t n, const MEM_HANDLE_T *images)
{
#ifdef BRCM_V3D_OPT
	return EGL_DISP_HANDLE_INVALID;
#endif
   EGL_DISP_HANDLE_T disp_handle;
   DISP_T *disp;
   uint32_t i;

   vcos_assert(n <= IMAGES_N_MAX);

   disp_handle = alloc_disp_handle();
   if (disp_handle == EGL_DISP_HANDLE_INVALID) {
      return EGL_DISP_HANDLE_INVALID;
   }

   disp = disp_from_handle(disp_handle);
   disp->in_use.pid = pid;
   disp->in_use.sem = sem;
   disp->in_use.n = n;
   for (i = 0; i != n; ++i) {
      KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(images[i]);
      vcos_assert(image->flags & IMAGE_FLAG_DISPLAY);
      if (n != 1) {
         image->interlock.disp_image_handle = egl_disp_image_handle(disp_handle, i);
      }
      mem_unlock(images[i]);
      mem_acquire(images[i]);
      disp->in_use.images[i] = images[i];
   }
   disp->in_use.last_win = EGL_PLATFORM_WIN_NONE;
   /* we need to make it look like there's something on the display (and it's
    * been there long enough -- swap interval 0), but there's nothing queued up
    * (next_pos(on) == post). we also want to make sure the next image to be
    * displayed is 0 */
   disp->in_use.post = next_power_of_2(n);
   disp->in_use.on = n - 1;
   disp->in_use.asyncs = -1; /* we don't want an async when the fake image "comes off the display" */
   disp->in_use.want = true;
   disp->in_use.want_us = 0; /* todo: will probably get spurious khrn_hw_kick() on first display. care? */
   disp->in_use.finish = false;
   disp->in_use.slots[n - 1].swap_interval = 0;

   return disp_handle;
}

/* wait until all images have been displayed for long enough */
static void finish(DISP_T *disp)
{
   disp->in_use.finish = true;
   notify_llat(disp);
   while (disp->in_use.finish) {
      khrn_sync_master_wait();
   }
   khrn_barrier();
}

void egl_disp_free(EGL_DISP_HANDLE_T disp_handle)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   DISP_T *disp = disp_from_handle(disp_handle);
   uint32_t i;

   finish(disp);

   /* take the current image off the display */
   if (disp->in_use.last_win != EGL_PLATFORM_WIN_NONE) {
      egl_server_platform_display_nothing_sync(disp->in_use.last_win);
   }

   for (i = 0; i != disp->in_use.n; ++i) {
      if (disp->in_use.n != 1) {
         KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(disp->in_use.images[i]);
         /* wait for all outstanding writes to the image to complete. we do
          * this to make sure there aren't any wait-for-display messages
          * hanging around in the system (we only post wait-for-display
          * messages before writes). we don't really want to flush unflushed
          * writes here, but whatever */
         khrn_interlock_read_immediate(&image->interlock);
         vcos_assert(image->interlock.disp_image_handle == egl_disp_image_handle(disp_handle, i));
         image->interlock.disp_image_handle = EGL_DISP_IMAGE_HANDLE_INVALID;
         mem_unlock(disp->in_use.images[i]);
      }
      mem_release(disp->in_use.images[i]);
   }

   free_disp_handle(disp_handle);
}

void egl_disp_finish(EGL_DISP_HANDLE_T disp_handle)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   finish(disp_from_handle(disp_handle));
}

void egl_disp_finish_all(void)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   uint32_t free;
   EGL_DISP_HANDLE_T disp_handle;
   uint32_t i;

   free = 0;
   disp_handle = disps_free_head;
   while (disp_handle != EGL_DISP_HANDLE_INVALID) {
      DISP_T *disp = disp_from_handle(disp_handle);
      free |= 1 << (disp - disps);
      disp_handle = disp->free_next;
   }

   for (i = 0; i != DISPS_N_MAX; ++i) {
      if (!(free & (1 << i))) {
         finish(disps + i);
      }
   }
}

void egl_disp_next(EGL_DISP_HANDLE_T disp_handle,
   MEM_HANDLE_T image_handle, uint32_t win, uint32_t swap_interval)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   DISP_T *disp = disp_from_handle(disp_handle);
   KHRN_IMAGE_T *image;

   {
      MEM_HANDLE_T handle = disp->in_use.images[disp->in_use.post & (next_power_of_2(disp->in_use.n) - 1)];
      if (image_handle == MEM_INVALID_HANDLE) {
         image_handle = handle;
      } else {
         vcos_assert(image_handle == handle);
      }
   }

   /* if there is an unflushed write to the image, we flush it here.
    * khrn_delayed_display is responsible for ensuring egl_disp_ready is called
    * after all flushed writes have completed (it may do this by, for example,
    * posting a message into the same fifo as the last write) */
   image = (KHRN_IMAGE_T *)mem_lock(image_handle);
   khrn_interlock_read(&image->interlock, KHRN_INTERLOCK_USER_NONE);
   khrn_delayed_display(image, egl_disp_slot_handle(disp_handle, post(disp, win, swap_interval)));
   mem_unlock(image_handle);
}

#ifdef EGL_DISP_FPS
   static void fps_put(KHRN_IMAGE_T *image, int32_t x, int32_t y);
#endif

void egl_disp_ready(EGL_DISP_SLOT_HANDLE_T slot_handle, bool skip)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   DISP_T *disp = slot_handle_get_disp(slot_handle);
   uint32_t slot = slot_handle_get_slot(slot_handle);

#ifdef EGL_DISP_FPS
   /* todo: this isn't safe when resizing with swap interval 0 */
   MEM_HANDLE_T image_handle = disp->in_use.images[slot & (next_power_of_2(disp->in_use.n) - 1)];
   KHRN_IMAGE_T *image = (KHRN_IMAGE_T *)mem_lock(image_handle);
   fps_put(image, image->width - 4, 4);
   mem_unlock(image_handle);
#endif

   if (skip) {
      disp->in_use.slots[slot].swap_interval = 0;
      disp->in_use.slots[slot].skip = true;
   }
   khrn_barrier();
   disp->in_use.slots[slot].ready = true;
   notify_llat(disp);
}

bool egl_disp_on(EGL_DISP_IMAGE_HANDLE_T image_handle)
{
#ifdef BRCM_V3D_OPT
	return false;
#endif
   DISP_T *disp = image_handle_get_disp(image_handle);
   uint32_t i = image_handle_get_i(image_handle);

   return !on_passed(disp, get_last_pos(disp, i));
}

void egl_disp_post_wait(uint32_t fifo, EGL_DISP_IMAGE_HANDLE_T image_handle)
{
#ifdef BRCM_V3D_OPT
	return;
#endif
   DISP_T *disp = image_handle_get_disp(image_handle);
   uint32_t i = image_handle_get_i(image_handle);

   uint32_t pos = get_last_pos(disp, i);
   uint32_t slot = pos & (SLOTS_N - 1);
   if (!on_passed(disp, pos) && !disp->in_use.slots[slot].wait_posted) {
      /* on display and haven't posted a wait for display message for this slot
       * yet
       *
       * the only reason we should post a wait for display message is if we're
       * about to post a write. so if we've posted a wait for display message,
       * that means there's a more recent write that we have to wait for
       * anyway, so there's no need to post a wait for display message too */
      disp->in_use.slots[slot].wait_posted = true;
      khrn_delayed_wait_for_display(fifo, disp_to_handle(disp), pos);
   }
}

bool egl_disp_still_on(EGL_DISP_HANDLE_T disp_handle, uint32_t pos)
{
#ifdef BRCM_V3D_OPT
	return false;
#endif
   return !on_passed(disp_from_handle(disp_handle), pos);
}

/******************************************************************************
fps stuff
******************************************************************************/

#ifdef EGL_DISP_FPS

#define FPS_DIGIT_WIDTH 8
#define FPS_DIGIT_HEIGHT 10
#define FPS_DIGIT_ADVANCE (FPS_DIGIT_WIDTH + 2)
static const uint32_t FPS_DIGITS[FPS_DIGIT_HEIGHT * 11] = {
   0x00ffff00, 0x0000f000, 0xffffff00, 0xffffff00, 0xff000000, 0xffffffff, 0x00ffffff, 0xffffffff, 0x00ffff00, 0x00ffff00, 0x00000000,
   0x0ffffff0, 0x000ff000, 0xfffffff0, 0xfffffff0, 0xff000000, 0xffffffff, 0x0fffffff, 0xffffffff, 0x0ffffff0, 0x0ffffff0, 0x00000000,
   0xff0000ff, 0x00fff000, 0x000000ff, 0x000000ff, 0xff00ff00, 0xff000000, 0xff000000, 0x000000ff, 0xff0000ff, 0xff0000ff, 0x00000000,
   0xff0000ff, 0x0ffff000, 0x000000ff, 0x000000ff, 0xff00ff00, 0xff000000, 0xff000000, 0x000000ff, 0xff0000ff, 0xff0000ff, 0x00000000,
   0xff0000ff, 0x000ff000, 0x00fffff0, 0xfffffff0, 0xffffffff, 0xffffff00, 0xffffff00, 0x0000ffff, 0x0ffffff0, 0x0fffffff, 0x00000000,
   0xff0000ff, 0x000ff000, 0x0fffff00, 0xfffffff0, 0xffffffff, 0xfffffff0, 0xfffffff0, 0x0000ffff, 0x0ffffff0, 0x00ffffff, 0x00000000,
   0xff0000ff, 0x000ff000, 0xff000000, 0x000000ff, 0x0000ff00, 0x000000ff, 0xff0000ff, 0x000000ff, 0xff0000ff, 0x000000ff, 0x00000000,
   0xff0000ff, 0x000ff000, 0xff000000, 0x000000ff, 0x0000ff00, 0x000000ff, 0xff0000ff, 0x000000ff, 0xff0000ff, 0x000000ff, 0x00000000,
   0x0ffffff0, 0xffffffff, 0xffffffff, 0xfffffff0, 0x0000ff00, 0xfffffff0, 0x0ffffff0, 0x000000ff, 0x0ffffff0, 0xfffffff0, 0x000ff000,
   0x00ffff00, 0xffffffff, 0xffffffff, 0xffffff00, 0x0000ff00, 0xffffff00, 0x00ffff00, 0x000000ff, 0x00ffff00, 0xffffff00, 0x000ff000};

static void fps_put_digit(KHRN_IMAGE_WRAP_T *wrap, int32_t x, int32_t y, uint32_t digit)
{
   int32_t i, j;
   for (j = 0; j != FPS_DIGIT_HEIGHT; ++j) {
      uint32_t row = FPS_DIGITS[(((FPS_DIGIT_HEIGHT - 1) - j) * 11) + digit];
      for (i = 0; i != FPS_DIGIT_WIDTH; ++i) {
         if (((x + i) >= 0) && ((x + i) < wrap->width) &&
            ((y + j) >= 0) && ((y + j) < wrap->height) &&
            (row & (1 << (((FPS_DIGIT_WIDTH - 1) - i) * 4)))) {
            uint32_t rgba = khrn_image_pixel_to_rgba(wrap->format, khrn_image_wrap_get_pixel(wrap, x + i, y + j), IMAGE_CONV_GL);
            rgba = ~rgba | 0xff000000;
            khrn_image_wrap_put_pixel(wrap, x + i, y + j, khrn_image_rgba_to_pixel(wrap->format, rgba, IMAGE_CONV_GL));
         }
      }
   }
}

static void fps_put_digits(KHRN_IMAGE_WRAP_T *wrap, int32_t x, int32_t y, uint32_t digits, int32_t dp)
{
   x -= FPS_DIGIT_WIDTH;
   while (digits || (dp >= 0)) {
      if (--dp == -1) {
         fps_put_digit(wrap, x, y, 10);
         x -= FPS_DIGIT_ADVANCE;
      }
      fps_put_digit(wrap, x, y, digits % 10);
      x -= FPS_DIGIT_ADVANCE;
      digits /= 10;
   }
}

static void fps_put_box(KHRN_IMAGE_WRAP_T *wrap, int32_t x, int32_t y, uint32_t rgba)
{
   uint32_t i, j;
   for (j = 0; j != FPS_DIGIT_HEIGHT; ++j) {
      for (i = 0; i != FPS_DIGIT_WIDTH; ++i) {
         khrn_image_wrap_put_pixel(wrap, x - (i + 1), y + j, rgba);
      }
   }
}

static void fps_put(KHRN_IMAGE_T *image, int32_t x, int32_t y)
{
   static uint32_t then_us = 0;
   uint32_t now_us = vcos_getmicrosecs();
   static uint32_t frame_count = 0;
   static uint32_t total_us = 0;
   ++frame_count;
   total_us += now_us - then_us;
   then_us = now_us;

   static uint32_t fps = 0;
   if (total_us > 1000000) {
      fps = (frame_count * 1000 * 100) / (total_us / 1000);
      frame_count = 0;
      total_us = 0;
   }

   KHRN_IMAGE_WRAP_T wrap;
   khrn_image_lock_wrap(image, &wrap);
   if (!(image->format & IMAGE_FORMAT_PRE)) {
      fps_put_box(&wrap, x, y, 0xff0000ff);
      x -= FPS_DIGIT_ADVANCE;
   }
   #ifndef NDEBUG
      fps_put_box(&wrap, x, y, 0xff00ffff);
      x -= FPS_DIGIT_ADVANCE;
   #endif
   fps_put_digits(&wrap, x, y, fps, 2);
   khrn_image_unlock_wrap(image);
}

#endif
