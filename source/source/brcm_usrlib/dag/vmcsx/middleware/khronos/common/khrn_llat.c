#ifndef __HERA_V3D__
#define KHRN_LLAT_OTHER_CORE
#endif

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
#include "middleware/khronos/common/khrn_hw.h"
#include "middleware/khronos/common/khrn_llat.h"
#include "interface/khronos/common/khrn_int_util.h"

#define CALLBACKS_N 5

#ifndef KHRN_LLAT_NO_THREAD
   #define THREAD_PRIORITY VCOS_THREAD_PRI_ABOVE_NORMAL
   #define THREAD_STACK_SIZE 8192 /* todo: how big does the stack need to be? */
#endif

static bool inited = false;
static KHRN_LLAT_CALLBACK_T callbacks[CALLBACKS_N];
vcos_static_assert(CALLBACKS_N < 32); /* 32 flags. (1 << 31) used for exit condition when KHRN_LLAT_NO_THREAD not defined */
static VCOS_ATOMIC_FLAGS_T event_flags;
#ifndef KHRN_LLAT_NO_THREAD
   static VCOS_EVENT_T event;
   static VCOS_THREAD_T thread;
#endif

static void process(uint32_t flags)
{
   vcos_assert(inited);

   while (flags) {
      uint32_t i;
      KHRN_LLAT_CALLBACK_T callback;

      i = _msb(flags);
      flags &= ~(1 << i);
      vcos_assert(i < CALLBACKS_N);
      callback = callbacks[i];
      vcos_assert(callback);
#ifdef ANDROID
   if (callback)
#endif
      callback();
   }
}

#ifdef KHRN_LLAT_NO_THREAD
void khrn_llat_process(void)
{
   if (inited) {
      uint32_t flags = vcos_atomic_flags_get_and_clear(&event_flags);
      khrn_barrier();
      process(flags);
   }
}
#else
static void *khrn_llat_main(void *p)
{
   UNUSED(p);

   for (;;) {
      uint32_t flags;

      vcos_event_wait(&event);
      flags = vcos_atomic_flags_get_and_clear(&event_flags);
      khrn_barrier();

      if (flags & (1 << 31)) {
         vcos_assert(flags == (uint32_t)(1 << 31)); /* all callbacks should have been unregistered by now */
         return NULL;
      }

      process(flags);
   }
}
#endif

bool khrn_llat_init(void)
{
   uint32_t i;
#ifndef KHRN_LLAT_NO_THREAD
   VCOS_THREAD_ATTR_T attr;
#endif

   vcos_assert(!inited);

   for (i = 0; i != CALLBACKS_N; ++i) {
      callbacks[i] = NULL;
   }

   if (vcos_atomic_flags_create(&event_flags) != VCOS_SUCCESS) {
      return false;
   }

#ifndef KHRN_LLAT_NO_THREAD
   if (vcos_event_create(&event, "khrn_llat_event") != VCOS_SUCCESS) {
      vcos_atomic_flags_delete(&event_flags);
      return false;
   }

   vcos_thread_attr_init(&attr);
   vcos_thread_attr_setpriority(&attr, THREAD_PRIORITY);
#ifdef KHRN_LLAT_OTHER_CORE
   switch (vcos_thread_get_affinity(vcos_thread_current())) {
   case VCOS_AFFINITY_CPU0: vcos_thread_attr_setaffinity(&attr, VCOS_AFFINITY_CPU1); break;
   case VCOS_AFFINITY_CPU1: vcos_thread_attr_setaffinity(&attr, VCOS_AFFINITY_CPU0); break;
   }
#endif
   vcos_thread_attr_setstacksize(&attr, THREAD_STACK_SIZE);
   if (vcos_thread_create(&thread, "khrn_llat_thread", &attr, khrn_llat_main, NULL) != VCOS_SUCCESS) {
      vcos_event_delete(&event);
      vcos_atomic_flags_delete(&event_flags);
      return false;
   }
#endif

   inited = true;

   return true;
}

void khrn_llat_term(void)
{
#ifndef NDEBUG
   uint32_t i;
#endif

#ifdef BRCM_V3D_OPT
   if(!inited)
      return ;
#endif
   vcos_assert(inited);

#ifndef KHRN_LLAT_NO_THREAD
   khrn_llat_notify(31); /* ask the thread to exit */
   vcos_thread_join(&thread, NULL);

   vcos_event_delete(&event);
#endif

   vcos_assert(!vcos_atomic_flags_get_and_clear(&event_flags));
   vcos_atomic_flags_delete(&event_flags);

#ifndef NDEBUG
   for (i = 0; i != CALLBACKS_N; ++i) {
      vcos_assert(!callbacks[i]);
   }
#endif

   inited = false;
}

uint32_t khrn_llat_register(KHRN_LLAT_CALLBACK_T callback)
{
   uint32_t i;

#ifdef BRCM_V3D_OPT
   if(!inited)
      return -1;
#endif
   vcos_assert(inited);

   for (i = 0; i != CALLBACKS_N; ++i) {
      if (!callbacks[i]) {
         callbacks[i] = callback;
         return i;
      }
   }

   UNREACHABLE(); /* increase CALLBACKS_N? */
   return -1;
}

void khrn_llat_unregister(uint32_t i)
{
#ifdef BRCM_V3D_OPT
   if(!inited)
       return;
#endif
   vcos_assert(inited);

   vcos_assert(callbacks[i]);
   callbacks[i] = NULL;
}

void khrn_llat_notify(uint32_t i)
{
#ifdef BRCM_V3D_OPT
   if(!inited)
      return;
#endif
   vcos_assert(inited);

   khrn_barrier();
   vcos_atomic_flags_or(&event_flags, 1 << i);
#ifdef KHRN_LLAT_NO_THREAD
   khrn_sync_notify_master(); /* the master thread is responsible for calling khrn_llat_process() */
#else
   vcos_event_signal(&event);
#endif
}

static bool waited;

static void wait_callback(void)
{
#ifdef BRCM_V3D_OPT
   if(!inited)
	  return;
#endif
   khrn_barrier();
   waited = true;
   khrn_sync_notify_master();
}

void khrn_llat_wait(void)
{
#ifdef BRCM_V3D_OPT
   if(!inited)
	  return;
#endif
   uint32_t i = khrn_llat_register(wait_callback), j;
   for (j = 0; j != 2; ++j) {
      waited = false;
      khrn_llat_notify(i);
      while (!waited) {
         khrn_sync_master_wait();
      }
      khrn_barrier();
   }
   khrn_llat_unregister(i);
}
