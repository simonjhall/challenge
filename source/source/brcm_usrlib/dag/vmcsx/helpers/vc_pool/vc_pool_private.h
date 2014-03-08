/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VC_POOL_PRIVATE_H
#define VC_POOL_PRIVATE_H

#define POOL_MAGIC   'pOOl'
#define OBJECT_MAGIC 'p00J'
#define OVERHEAD_ALIGN 4
#define VC_POOL_VER   1

#include "vc_pool.h"
#include "vcfw/rtos/rtos.h"


struct opaque_vc_pool_object_s {
   uint32_t magic;
   VC_POOL_T *pool;        /**< which pool this object belongs to */
   int refcount;
   MEM_HANDLE_T mem;
   // if this object was allocated from a divisible pool, the base address
   // of the object may be offset from the base address of the underlying
   // MEM_HANDLE_T (->mem, above)
   int offset;
   uint8_t *overhead;
};

struct opaque_vc_pool_s {
   uint32_t magic;
   uint32_t version;       /**< For debugger support */
   RTOS_LATCH_T latch;

   size_t object_size;
   size_t max_object_size; // for divisible pools.
   uint32_t alignment;     // alignment of each object.  for divisible pools.
   int nobjects;
   int max_objects;        // for divisible pools.
   int allocated;
   MEM_HANDLE_T mem;       // for divisible pools.
   VC_POOL_OBJECT_T *object;
   uint32_t overhead_size; // size of each overhead block
   uint8_t *overhead;      // pointer to overhead area
   const char *name;                /**< for debugging */
   struct opaque_vc_pool_s *next;   /**< link in global pool list */
   uint32_t alloc_fails;   /**< Count of allocation failures */

   VC_POOL_CALLBACK_T callback; //callback to be signalled when an object is released
   void* callback_cookie; //cookie associated with above callback

#if defined( VCOS_HAVE_RTOS ) && VCOS_HAVE_RTOS
   VCOS_EVENT_FLAGS_T event;
#endif
   size_t final_object_size; // for unequally divisible pools.
   VC_POOL_FLAGS_T pool_flags;
};

#endif
