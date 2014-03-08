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
#include "middleware/khronos/common/2708/khrn_prod_4.h"
#include "middleware/khronos/common/2708/khrn_worker_4.h"
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "interface/vcos/vcos.h"

#if defined(SIMPENROSE) || !VCOS_HAVE_RTOS || defined(__CC_ARM)
   #define KHRN_WORKER_USE_LLAT
#endif

/******************************************************************************
fifo pos stuff
******************************************************************************/

uint64_t khrn_worker_enter_pos;
uint32_t khrn_worker_exit_pos_0, khrn_worker_exit_pos_1;

/******************************************************************************
message stuff
******************************************************************************/

#define MSGS_SIZE 16384 /* todo: how big should this be? */

/* if callback is NULL, the message is a nop. a message of size 4 is permitted;
 * callback is assumed to be NULL, but isn't actually present! */
typedef struct {
   uint32_t size;
   KHRN_WORKER_CALLBACK_T callback;
} MSG_HEADER_T;

vcos_static_assert(sizeof(MSG_HEADER_T) == 8);
vcos_static_assert(alignof(MSG_HEADER_T) == 4);

static ALIGNED(4) uint8_t msgs[MSGS_SIZE];
ALIGN_TO(msgs, 4);

static struct {
   uint8_t *post, *done_it, *cleanup;
} khrn_worker_msg;

static INLINE uint8_t *next_msg(uint8_t *msg, uint32_t size)
{
   msg += size;
   return (msg == (msgs + MSGS_SIZE)) ? msgs : msg;
}

static INLINE void advance_msg(uint8_t **msg, uint32_t size)
{
   khrn_barrier();
   *msg = next_msg(*msg, size);
}

static INLINE bool room_for_msg(uint8_t *msg_a, uint32_t size, uint8_t *msg_b)
{
   if (msg_b == msgs) { msg_b = msgs + MSGS_SIZE; }
   if ((msg_b <= msg_a) || (msg_b > (msg_a + size))) {
      return true;
   }
   return false;
}

static INLINE bool more_msgs(uint8_t *msg_a, uint8_t *msg_b)
{
   if (msg_a != msg_b) {
      khrn_barrier();
      return true;
   }
   return false;
}

static INLINE uint32_t get_msg_size_max(uint8_t *msg)
{
   return (msgs + MSGS_SIZE) - msg;
}

static void post_nop(uint32_t size);

static void *post_begin(KHRN_WORKER_CALLBACK_T callback, uint32_t size)
{
   uint32_t size_max;
   MSG_HEADER_T *h;

   size += sizeof(MSG_HEADER_T);
   vcos_assert(!(size & 0x3)); /* everything should by 4-byte aligned */
   vcos_assert(callback ? (size >= sizeof(MSG_HEADER_T)) : (size >= 4));

   size_max = get_msg_size_max(khrn_worker_msg.post);
   if (size > size_max) {
      post_nop(size_max);
      size_max = get_msg_size_max(khrn_worker_msg.post);
      vcos_assert(size <= size_max);
   }

   while (!room_for_msg(khrn_worker_msg.post, size, khrn_worker_msg.cleanup)) {
      khrn_sync_master_wait();
   }

   h = (MSG_HEADER_T *)khrn_worker_msg.post;
   h->size = size;
   if (size != 4) {
      h->callback = callback;
   }
   return h + 1;
}

static void post_end(void)
{
   advance_msg(&khrn_worker_msg.post, ((MSG_HEADER_T *)khrn_worker_msg.post)->size);
   /* the caller will notify worker if necessary */
}

static void post_nop(uint32_t size)
{
   post_begin(NULL, size - sizeof(MSG_HEADER_T));
   post_end();
}

/* attempt to process the next message. if false is returned, try again later */
static bool do_it(void)
{
   MSG_HEADER_T *h = (MSG_HEADER_T *)khrn_worker_msg.done_it;
   if ((h->size != 4) && h->callback) {
      switch (h->callback(
         KHRN_WORKER_CALLBACK_REASON_DO_IT,
         h + 1, h->size - sizeof(MSG_HEADER_T))) {
      case KHRN_WORKER_CALLBACK_RESULT_DO_IT_WAIT: return false;
      case KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK: break;
      case KHRN_WORKER_CALLBACK_RESULT_DO_IT_OK_ADVANCE_EXIT_POS:
      {
         khrn_worker_advance_exit_pos();
#ifndef SIMPENROSE
         khrn_hw_notify_llat(); /* might be waiting for khrn_worker_advance_exit_pos() */
#endif
         break;
      }
      default: UNREACHABLE();
      }
   }
   advance_msg(&khrn_worker_msg.done_it, h->size);
   khrn_sync_notify_master(); /* cleanup, might be waiting for khrn_worker_advance_exit_pos() */
   return true;
}

/* cleanup the next message */
static void cleanup(void)
{
   MSG_HEADER_T *h = (MSG_HEADER_T *)khrn_worker_msg.cleanup;
   if ((h->size != 4) && h->callback) {
      verify(h->callback(
         KHRN_WORKER_CALLBACK_REASON_CLEANUP,
         h + 1, h->size - sizeof(MSG_HEADER_T)) ==
         KHRN_WORKER_CALLBACK_RESULT_CLEANUP);
   }
   khrn_worker_msg.cleanup = next_msg(khrn_worker_msg.cleanup, h->size);
}

/******************************************************************************
thread stuff
******************************************************************************/

#ifndef KHRN_WORKER_USE_LLAT
   #define THREAD_PRIORITY VCOS_THREAD_PRI_NORMAL
   #define THREAD_STACK_SIZE 8192 /* todo: how big does the stack need to be? */
#endif

#ifdef KHRN_WORKER_USE_LLAT
   static uint32_t llat_i;
#else
   static VCOS_EVENT_T event;
   static bool exit_thread;
   static VCOS_THREAD_T thread;
#endif

#ifdef KHRN_WORKER_USE_LLAT
static void khrn_worker_llat_callback(void)
{
   if (more_msgs(khrn_worker_msg.done_it, khrn_worker_msg.post) &&
      do_it() &&
      more_msgs(khrn_worker_msg.done_it, khrn_worker_msg.post)) {
      khrn_llat_notify(llat_i);
   }
}
#else
static void *khrn_worker_main(void * param)
{
   UNUSED(param);

   do {
      vcos_event_wait(&event);
      while (more_msgs(khrn_worker_msg.done_it, khrn_worker_msg.post) && do_it()) ;
   } while (!exit_thread);
   return NULL;
}
#endif

/******************************************************************************
interface
******************************************************************************/

static bool inited = false;

bool khrn_worker_init(void)
{
#ifndef KHRN_WORKER_USE_LLAT
   VCOS_THREAD_ATTR_T attr;
#endif
   vcos_assert(!inited);

   khrn_worker_enter_pos = 0;
   khrn_worker_exit_pos_0 = 0;
   khrn_worker_exit_pos_1 = 0;

   khrn_worker_msg.post = msgs;
   khrn_worker_msg.done_it = msgs;
   khrn_worker_msg.cleanup = msgs;

#ifdef KHRN_WORKER_USE_LLAT
   llat_i = khrn_llat_register(khrn_worker_llat_callback);
   vcos_assert(llat_i != -1);
#else
   if (vcos_event_create(&event, "khrn_worker_event") != VCOS_SUCCESS) {
      return false;
   }

   exit_thread = false;

   vcos_thread_attr_init(&attr);
   vcos_thread_attr_setpriority(&attr, THREAD_PRIORITY);
#if !defined(V3D_LEAN)
   switch (vcos_thread_get_affinity(vcos_thread_current())) {
   case VCOS_AFFINITY_CPU0: vcos_thread_attr_setaffinity(&attr, VCOS_AFFINITY_CPU1); break;
   case VCOS_AFFINITY_CPU1: vcos_thread_attr_setaffinity(&attr, VCOS_AFFINITY_CPU0); break;
   }
   vcos_thread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
#endif /* V3D_LEAN */
   if (vcos_thread_create(&thread, "khrn_worker_thread", &attr, khrn_worker_main, NULL) != VCOS_SUCCESS) {
      vcos_event_delete(&event);
      return false;
   }
#endif

   inited = true;

   return true;
}

void khrn_worker_term(void)
{
   vcos_assert(inited);
   vcos_assert(khrn_worker_msg.done_it == khrn_worker_msg.post); /* should have called khrn_worker_wait or equivalent before this */
   vcos_assert(khrn_worker_msg.cleanup == khrn_worker_msg.post);

#ifdef KHRN_WORKER_USE_LLAT
   khrn_llat_unregister(llat_i);
#else
   exit_thread = true;
   vcos_event_signal(&event);
   vcos_thread_join(&thread, NULL);

   vcos_event_delete(&event);
#endif

   inited = false;
}

void *khrn_worker_post_begin(KHRN_WORKER_CALLBACK_T callback, uint32_t size)
{
   return post_begin(callback, size);
}

void khrn_worker_post_end(void)
{
   post_end();
   khrn_worker_notify();
}

void khrn_worker_cleanup(void)
{
   while (more_msgs(khrn_worker_msg.cleanup, khrn_worker_msg.done_it)) {
      cleanup();
   }
}

void khrn_worker_wait(void)
{
   while (khrn_worker_msg.cleanup != khrn_worker_msg.post) {
      khrn_sync_master_wait();
   }
}

void khrn_worker_notify(void)
{
#ifdef KHRN_WORKER_USE_LLAT
   khrn_llat_notify(llat_i);
#else
   vcos_event_signal(&event);
#endif
}
