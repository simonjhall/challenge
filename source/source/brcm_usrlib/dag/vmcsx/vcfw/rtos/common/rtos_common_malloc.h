/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef RTOS_COMMON_MALLOC_H
#define RTOS_COMMON_MALLOC_H

typedef struct malloc_header_s {
   unsigned requested_size_align, description, guardword;
} MALLOC_HEADER_T;

#define GETREQSIZEALIGN(q, size, align) do {(size) = (q)->requested_size_align & ((1<<28)-1); (align) = (1<<((q)->requested_size_align >> 28));} while (0)
#define SETREQSIZEALIGN(q, size, align) do {(q)->requested_size_align = ((size) & ((1<<28)-1)) | (_msb(align) << 28);} while (0)

#define MALLOCWORDS 2                                                       // underlying malloc implementation words
#define EXTRAWORDS  (RTOS_ALIGN_DEFAULT/sizeof(unsigned)-MALLOCWORDS)      // we store header and guard words here
#define GUARDWORDS  (EXTRAWORDS - sizeof(MALLOC_HEADER_T)/sizeof(unsigned)) // guardwords at end of block

#define PADDINGWORD    0xa55a5aa6
#define GUARDWORDHEAP  0xa55a5aa5
#define GUARDWORDALLOC 0xa55aa55a
#define GUARDWORDSTACK 0x5a55a5aa
#define GUARDWORDHISR  0xa5a55a5a
#undef RTOS_ALIGN_DEFAULT
#define RTOS_ALIGN_DEFAULT 32

#ifdef _VC_VERSION
#include "dmalloc.h"
extern dmalloc_pool_t system_internal_memory;

/** Allocate a block of aligned memory from a pool.
  *
  * Memory must be returned using rtos_pool_free().
  *
  * @param pool     the pool to allocate from.
  * @param size     amount of memory (in bytes) to allocate
  * @param align    alignment required (in bytes; must be power of 2)
  * @param desc     description
  *
  * @return pointer to memory, or NULL if exhausted.
  */
extern void *rtos_pool_aligned_malloc(dmalloc_pool_t pool, size_t size, size_t align, const char *description);

/** Return a block of memory previously allocated with rtos_pool_malloc.
  *
  * @param pool     the pool to allocate from.
  * @param ptr      memory to return
  */
extern void rtos_pool_aligned_free(dmalloc_pool_t pool, void *ptr);

#endif

#endif

