/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef RTOS_COMMON_MEMPOOL_H
#define RTOS_COMMON_MEMPOOL_H

#include <stdlib.h>

#include "vcinclude/common.h"

#include "interface/vcos/vcos.h"
#include "vcfw/rtos/common/dmalloc.h"
typedef dmalloc_pool_t MEMPOOL_T;

#define MEMPOOL_SUCCESS 0
#define MEMPOOL_FAIL -1

#ifdef _VIDEOCORE

// Create a memory pool.Returns a mempool, or MEMPOOL_INVALID_HANDLE if it fails.
// Returns MEMPOOL_SUCCESS if it succeeds.

int rtos_mempool_create(MEMPOOL_T *mempool, void *mem, uint32_t size, const char *description);

// Destroy a memory pool.

int rtos_mempool_destroy(MEMPOOL_T *mempool);

// Allocate from a memory pool. mempool may be MEMPOOL_DEFAULT_HANDLE to allocate in
// the default heap.

void *rtos_mempool_alloc(MEMPOOL_T *mempool, size_t size, int align, const char *description);

// Free from a memory pool. For legacy reasons, you have to pass the mempool back too, which
// should be the value that was passed when the memory as allocated.

void rtos_mempool_free(void *mem, MEMPOOL_T *mempool);

#else

// For Windows and other builds. Memory pools don't do anything (except waste the memory you
// allocate to them), everything is just diverted to the standard heap.

#define rtos_mempool_create(mempool, mem, size, description) (MEMPOOL_SUCCESS)
#define rtos_mempool_destroy(mempool) (MEMPOOL_SUCCESS)
#ifdef NDEBUG
   #define rtos_mempool_alloc(mempool, size, align, description) rtos_malloc_priority(size, align, VCLIB_PRIORITY_UNIMPORTANT, (char const *)0)
#else
   #define rtos_mempool_alloc(mempool, size, align, description) rtos_malloc_priority(size, align, VCLIB_PRIORITY_UNIMPORTANT, description)
#endif
#define rtos_mempool_free(mem, mempool) free(mem)

#endif

#endif
