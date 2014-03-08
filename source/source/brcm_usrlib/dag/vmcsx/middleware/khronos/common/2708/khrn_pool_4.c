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
#include "middleware/khronos/common/2708/khrn_pool_4.h"

static INLINE void set_next(MEM_HANDLE_T handle, MEM_HANDLE_T next_handle)
{
   mem_set_term(handle, (MEM_TERM_T)(uintptr_t)next_handle);
}

static INLINE MEM_HANDLE_T get_next(MEM_HANDLE_T handle)
{
   return (MEM_HANDLE_T)(uintptr_t)mem_get_term(handle);
}

void khrn_pool_init(KHRN_POOL_T *pool,
   uint32_t size, uint32_t align, uint32_t n_max,
   MEM_FLAG_T extra_flags, const char *desc)
{
   pool->size = size;
   pool->align = align;
   pool->n_max = n_max;
   pool->flags = (MEM_FLAG_T)(extra_flags | MEM_FLAG_DISCARDABLE | MEM_FLAG_RETAINED | MEM_FLAG_NO_INIT);
   pool->desc = desc;

   pool->n = 0;
   pool->free_head = MEM_INVALID_HANDLE;
}

void khrn_pool_term(KHRN_POOL_T *pool)
{
   MEM_HANDLE_T handle, next_handle;
   for (handle = pool->free_head; handle != MEM_INVALID_HANDLE; handle = next_handle) {
      next_handle = get_next(handle);
      mem_set_term(handle, NULL);
      mem_release(handle);
      --pool->n;
   }
   vcos_assert(pool->n == 0);
}

MEM_HANDLE_T khrn_pool_alloc(KHRN_POOL_T *pool, bool fail_ok)
{
   MEM_HANDLE_T handle, next_handle;

   for (handle = pool->free_head; handle != MEM_INVALID_HANDLE; handle = next_handle) {
      next_handle = get_next(handle);
      if (mem_retain(handle)) {
         pool->free_head = next_handle;
         return handle;
      }
      mem_unretain(handle);
      mem_set_term(handle, NULL);
      mem_release(handle);
      --pool->n;
   }
   pool->free_head = MEM_INVALID_HANDLE;

   if (fail_ok && (pool->n >= pool->n_max)) {
      /* already have alot of buffers -- caller will wait and try again later when some have been freed */
      return MEM_INVALID_HANDLE;
   }

   handle = mem_alloc_ex(pool->size, pool->align, pool->flags, pool->desc, MEM_COMPACT_DISCARD); /* todo: do we always want MEM_COMPACT_DISCARD? */
   if (handle != MEM_INVALID_HANDLE) {
      ++pool->n;
   }
   return handle;
}

void khrn_pool_free(KHRN_POOL_T *pool, MEM_HANDLE_T handle)
{
   mem_unretain(handle);
   mem_abandon(handle);
   set_next(handle, pool->free_head);
   pool->free_head = handle;
}

void khrn_pool_cleanup(KHRN_POOL_T *pool)
{
   MEM_HANDLE_T prev_handle, handle, next_handle;
   for (prev_handle = MEM_INVALID_HANDLE, handle = pool->free_head; handle != MEM_INVALID_HANDLE; prev_handle = handle, handle = next_handle) {
      next_handle = get_next(handle);
      if (mem_get_size(handle) == 0) {
         if (prev_handle == MEM_INVALID_HANDLE) {
            pool->free_head = next_handle;
         } else {
            set_next(prev_handle, next_handle);
         }
         mem_set_term(handle, NULL);
         mem_release(handle);
         --pool->n;
         handle = prev_handle;
      }
   }
}
