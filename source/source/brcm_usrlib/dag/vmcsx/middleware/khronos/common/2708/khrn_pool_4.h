/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef KHRN_POOL_4_H
#define KHRN_POOL_4_H

#include "middleware/khronos/common/khrn_mem.h"

typedef struct {
   uint32_t size, align, n_max;
   MEM_FLAG_T flags;
   const char *desc;

   uint32_t n; /* total (in use + in free list) */
   MEM_HANDLE_T free_head;
} KHRN_POOL_T;

extern void khrn_pool_init(KHRN_POOL_T *pool,
   uint32_t size, uint32_t align, uint32_t n_max,
   MEM_FLAG_T extra_flags, const char *desc);
extern void khrn_pool_term(KHRN_POOL_T *pool);

extern MEM_HANDLE_T khrn_pool_alloc(KHRN_POOL_T *pool, bool fail_ok);
extern void khrn_pool_free(KHRN_POOL_T *pool, MEM_HANDLE_T handle);

extern void khrn_pool_cleanup(KHRN_POOL_T *pool); /* release discarded buffers */

#endif
