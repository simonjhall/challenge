/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "rtos_common_mempool.h"

#include "rtos.h"
#include "vcfw/rtos/common/rtos_common_malloc.h"

#include "interface/vcos/vcos.h"
#include "dmalloc.h"

/*
   Turn a block of memory into a memory pool (or heap), from which allocations can happen.

   Preconditions:

   - mempool points to an uninitialised MEMPOOL_T structure
   - mem points to the block of memory from which the memory pool will allocate
   - size is the size of the block of memory
   - description is an optional brief description of the the memory pool

   Postconditions:

   If it succeeds:
   - Returns MEMPOOL_SUCCESS.
   Otherwise:
   - Returns MEMPOOL_FAIL;
*/

int rtos_mempool_create(MEMPOOL_T *mempool, void *mem, uint32_t size, const char *description)
{
   dmalloc_pool_t p = dmalloc_create_pool(mem, (uint8_t*)mem+size);
   if (p)
   {
      *mempool = p;
      return MEMPOOL_SUCCESS;
   }
   else
   {
      *mempool = 0;
      return MEMPOOL_FAIL;
   }
}

/*
   Destroy a memory pool

   Preconditions:

   - mempool points to a successfully created memory pool (via rtos_mempool_create)

   Postconditions:

   If it succeeds:
   - Returns MEMPOOL_SUCCESS
   Otherwise:
   - Returns MEMPOOL_FAIL
*/

int rtos_mempool_destroy(MEMPOOL_T *mempool)
{
   int rc;
   dmalloc_pool_t p = *mempool;
   rc = dmalloc_delete_pool(p, 1);
   vc_assert(rc);
   *mempool = 0;
   return MEMPOOL_SUCCESS;
}

/*
   Allocate from a memory pool created by rtos_mempool_create.

   Preconditions:

   - mempool points to a successfully created memory pool (via rtos_mempool_create)
   - size is the size of the block to allocate
   - align is the required alignment in bytes
   - description is an optional description of the memory block

   Postconditions:

   If it succeeds:
   - Returns the address of the memory allocated from within the pool
   Otherwise:
   - Returns NULL.
*/

void *rtos_mempool_alloc(MEMPOOL_T *mempool, size_t size, int align, const char *description)
{
   if (mempool)
   {
      return rtos_pool_aligned_malloc(*mempool, size, align, description);
   }
   else
   {
      return rtos_prioritymalloc(size, align, RTOS_PRIORITY_UNIMPORTANT, description);
   }
}

/*
   Free a block from a memory pool.

   Preconditions:

   - mempool points to a successfully created memory pool (via rtos_mempool_create)
   - ret points to memory allocated from that pool with rtos_mempool_alloc

   Postconditions:

   Memory is freed.
*/

void rtos_mempool_free(void *ret, MEMPOOL_T *mempool)
{
   if (mempool)
   {
      rtos_pool_aligned_free(*mempool, ret);
   }
   else
   {
      rtos_priorityfree(ret);
   }
}


