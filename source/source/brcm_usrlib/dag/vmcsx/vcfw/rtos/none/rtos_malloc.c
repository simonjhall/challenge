/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include "interface/vcos/vcos.h"

#include "vcinclude/hardware.h"

#include "vcfw/rtos/rtos.h"
#include "vcfw/rtos/common/rtos_common_malloc.h"
#include "vcfw/rtos/common/dmalloc.h"

/******************************************************************************
Extern functions
******************************************************************************/

/******************************************************************************
Private typedefs, macros and constants.
******************************************************************************/

#define DMALLOC_LOCK(ptr)
#define DMALLOC_UNLOCK(ptr)
#define DMALLOC_LOCKCHAIN
#define DMALLOC_UNLOCKCHAIN
#define DMALLOC_INITLOCK(ptr)
#define DMALLOC_UNINITLOCK(ptr)
#define DMALLOC_LOCKBYTES     (0)

/******************************************************************************
Static data
******************************************************************************/

#ifdef __HIGHC__

extern char _fheap[];
extern char _eheap[];
#pragma weak(_fheap);
#pragma weak(_eheap);

#pragma weak(__RTOS_HEAP_START)
#pragma weak(__RTOS_HEAP_END)

#endif


/******************************************************************************
Static function definitions.
******************************************************************************/

/******************************************************************************
Global function definitions.
******************************************************************************/

/***********************************************************
 * Name: rtos_malloc_init
 *
 * Arguments:
 *       void
 *
 * Description: Routine to init the malloc pool
 *
 * Returns: int32_t - (0 == success)
 *
 ***********************************************************/
int32_t rtos_malloc_init(void)
{
   void *start = 0;
   void *end = 0;

   dmalloc_init();

#ifdef __HIGHC__
   if (&__RTOS_HEAP_START && &__RTOS_HEAP_END && __RTOS_HEAP_START && __RTOS_HEAP_END)
   {
      /* nucleus "style" heap */
      start = __RTOS_HEAP_START;
      end = __RTOS_HEAP_END;
   }
   else if (&_fheap && &_eheap && _fheap && _eheap)
   {
      /* Bounded heap */
      start = _fheap;
      end = (void *)_eheap;
   }
   else
   {
      /* This is no longer supported */
      vcos_assert(0);
      return 0;
   }
#else
   int stack=1; /* Use a dummy variable to access stack location */
   start = _end;
   end = (char *)&stack-4000;
#endif //__HIGHC__

   system_internal_memory = dmalloc_create_pool(start, end);

   return (system_internal_memory != DMALLOC_POOL_INVALID);
}


/***********************************************************
 * Name: rtos_get_free_mem
 *
 * Arguments:
 *       const uint32_t pool - which pool
 *
 * Description: Routine to get free memory in the system
 *
 * Returns: int32_t - (0 == success)
 *
 ***********************************************************/
uint32_t rtos_get_free_mem( rtos_mempool_t pool )
{
   return dmalloc_get_free_mem(pool ? (dmalloc_pool_t)pool : system_internal_memory);
}

/* Pull in the common dmalloc implementation, to be macro-expanded */

#include "vcfw/rtos/common/dmalloc.c"
