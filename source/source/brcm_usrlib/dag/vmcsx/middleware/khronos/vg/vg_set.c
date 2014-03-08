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
#include "middleware/khronos/vg/vg_set.h"
#include "interface/khronos/vg/vg_int_util.h"

/*
   handles tend to be allocated in this order:
   (1 << 31) | 0, (1 << 31) | 1, ..., 0, 1, 2, ...
   ie small alloc handles first, then regular handles when we run out

   to avoid the regular handles colliding with the small alloc handles when we
   do (handle & (buckets_n - 1)), we ror the handle values such that the
   small-alloc bit is bit 0 (instead of bit 31)
*/

static INLINE uint32_t nice_handle(MEM_HANDLE_T handle)
{
   return _ror((uint32_t)handle, 31);
}

bool vg_set_init(VG_SET_T *set, uint32_t capacity)
{
   return khrn_map_init(set, capacity);
}

/*
   bool vg_set_insert(VG_SET_T *set, MEM_HANDLE_T key)

   Inserts key into set. No-op if it's already there.

   Returns false and leaves set unchanged is there is insufficient memory to complete the operation

   Implementation notes:

   -

   Preconditions:

   set is a valid pointer to a set that holds objects of type X
   key is a valid handle to an object of type X

   Postconditions:

   On success:
      true is returned
      set contains key
   On failure:
      false is returned
      set is unchanged

   Invariants preserved:

   All invariants on VG_SET_T
*/

bool vg_set_insert(VG_SET_T *set, MEM_HANDLE_T key)
{
   return khrn_map_insert(set, nice_handle(key), key);
}

/*
   void vg_set_delete(VG_SET_T *set, MEM_HANDLE_T key)

   If present, deletes key from the set. If not present, does nothing.

   Implementation notes:

   -

   Preconditions:

   set is a valid pointer to a set

   Postconditions:

   key is not in set.

   Invariants preserved:

   All invariants on VG_SET_T
*/

void vg_set_delete(VG_SET_T *set, MEM_HANDLE_T key)
{
   khrn_map_delete(set, nice_handle(key));
}

bool vg_set_contains(VG_SET_T *set, MEM_HANDLE_T key)
{
   return khrn_map_lookup(set, nice_handle(key)) != MEM_INVALID_HANDLE;
}

/*
   bool vg_set_contains_locked(VG_SET_T *set, MEM_HANDLE_T key, void *storage)

   Returns whether key is in set.

   Implementation notes:

   -

   Preconditions:

   set is a valid pointer to a set whose elements are of type X
   storage is the locked pointer to set->storage

   Postconditions:

   result is true iff key is in set

   Invariants preserved:

   -
*/

bool vg_set_contains_locked(VG_SET_T *set, MEM_HANDLE_T key, void *storage)
{
   return khrn_map_lookup_locked(set, nice_handle(key), storage) != MEM_INVALID_HANDLE;
}

typedef struct {
   SET_CALLBACK_T func;
   void *v;
} OUR_CALLBACK_ARG_T;

static void our_callback(KHRN_MAP_T *map, uint32_t key, MEM_HANDLE_T value, void *v)
{
   OUR_CALLBACK_ARG_T *args = (OUR_CALLBACK_ARG_T *)v;

   args->func(map, value, args->v);
}

void vg_set_iterate(VG_SET_T *set, SET_CALLBACK_T func, void *v)
{
   OUR_CALLBACK_ARG_T args;
   args.func = func;
   args.v = v;
   khrn_map_iterate(set, our_callback, &args);
}

void vg_set_term(VG_SET_T *set)
{
   khrn_map_term(set);
}
