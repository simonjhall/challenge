/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vcinclude/hardware.h"

#include "interface/vcos/vcos.h"
#include "vcfw/rtos/rtos.h"
#include "vcfw/vclib/vclib.h"
#include "rtos_common_malloc.h"
#include "dmalloc.h"

#undef malloc
#undef calloc
#undef realloc
#undef free

/******************************************************************************
Extern functions
******************************************************************************/

/******************************************************************************
Private typedefs
******************************************************************************/

/*
The heap is designed to use 32-byte headers and always return 32-byte aligned malloced blocks.
This is efficient for the common (on VideoCore) case of requiring 32-byte alignment.
It also separates the header from the data in cache lines, which helps with handling of direct alias mallocs.
It has a (marginally) better fragmentation behaviour. It does have more wastage in the case of many small unaligned mallocs.

The heap structure will look like:

[..........][malloc0][malloc1][size/align][description][guardword]
[........alignment padding (may be none)..[ptrheader  ][paddgword]
[malloced block .................................................]
[guardwords]

*/

//#define FILL_MALLOC
#define BREAKPOINT 0x5230

#define fatal_assert(x) vcos_demand( x ) //vclib_fatal_assert(x, FATAL_WHITE|FATAL_SUB_BLACK)

/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/

static void const *rtos_check_block(const void *base, int length, void *handle);

/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

dmalloc_pool_t system_internal_memory = DMALLOC_POOL_INVALID;

static int rtos_current_priority = 0;

// SW-5505 : rtos_find_heap_corruption() should be thread-safe.
#if VCOS_HAVE_RTOS
static RTOS_LATCH_T    rmp_latch;

#define RMP_GET_LATCH  rtos_latch_get( &rmp_latch )
#define RMP_PUT_LATCH  rtos_latch_put( &rmp_latch )
#else
#define RMP_GET_LATCH  (void)0
#define RMP_PUT_LATCH  (void)0
#endif

/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   malloc_priority

SYNOPSIS
   void *malloc_priority(int size, int align, int priority, const char *description)

FUNCTION
   Mallocs a buffer of the specified size and alignment (in bytes).
   Buffer is in internal memory if space and priority is above the current level.
   The priority is the number of times the entire buffer will be accessed per second.
   The current priority level should be set for the entire application
   to maximise the use of internal memory subject to never putting an important buffer
   in external memory.

RETURNS
   Pointer to malloced memory
******************************************************************************/

void *rtos_malloc_priority(const uint32_t size, const uint32_t align, const uint32_t priority, const char *description)
{
   void *ret = NULL;
   unsigned long cache_alias = priority >> RTOS_PRIORITY_SHIFT;
   const uint32_t local_priority = priority & 0xfffff;
   dmalloc_pool_t pool = system_internal_memory;

   description = description ? description : rtos_default_malloc_name();

   ret = rtos_pool_aligned_malloc(pool, size, align, description);

   if (ret)
   {
      ret = (void *)((unsigned long)ret | (cache_alias << RTOS_PRIORITY_SHIFT));
   }

   return ret;
}


/******************************************************************************
NAME
   calloc_priority

SYNOPSIS
   void *calloc_priority(int size, int align, int priority, const char *description)

FUNCTION
   As above, but followed by a memset(<result>, 0, size) if appropriate.

RETURNS
   Pointer to malloced memory
******************************************************************************/

void *rtos_calloc_priority(const uint32_t size, const uint32_t align, const uint32_t priority, const char *description)
{
   void *ret;

   description = description ? description : rtos_default_malloc_name();

   ret = rtos_malloc_priority(size, align, priority, description);

   if (ret != NULL)
   {
      memset(ret, 0, size);
   }

   return ret;
}


void free(void *ret)
{
   rtos_pool_aligned_free(system_internal_memory, ret);
}

void rtos_pool_aligned_free(dmalloc_pool_t pool, void *ret)
{
   MALLOC_HEADER_T *q;
   void *pointer;
   int size, align;

   if (!ret) return;

   RMP_GET_LATCH;

   ret = DMALLOC_ALIAS(ret);

   q = ((MALLOC_HEADER_T *)ret)-1;

   // if there is padding our malloc header is earlier
   if (q->guardword == PADDINGWORD) {
      fatal_assert(q->requested_size_align == GUARDWORDHEAP);
      q = (MALLOC_HEADER_T *)q->description;
   }

   // guard at start
   fatal_assert(q->guardword == GUARDWORDHEAP);
   // mark it so double free not possible, and prevent heap checker from
   // referencing this block while in an inconsistent state
   q->guardword = GUARDWORDALLOC;

   GETREQSIZEALIGN(q, size, align);

   // guard at end
   {
      int i; unsigned *p = ((unsigned *)ret)+((size+3)>>2);
      for (i=0; i<(int)GUARDWORDS; i++)
         fatal_assert(*p++==GUARDWORDHEAP);
   }
   // mark it so double free not possible
   ((unsigned *)ret)[(size+3)>>2] = ~GUARDWORDHEAP;
   pointer = (char *)q;
#ifdef FILL_MALLOC
   short *instruction;
   for (instruction = (short *)ret; instruction < (short *)ret + (size>>1); instruction++)
      *instruction = BREAKPOINT;
#endif

#ifdef DMALLOC_USE_ALIAS_DIRECT
   // we always invalidate cached data here whatever the alias. Data after the free will not be used again (and dirty data may be harmful)
   vclib_dcache_invalidate_range(pointer, size);
#endif

   dfree(pool, pointer);

   RMP_PUT_LATCH;
}

// wrapper over realloc
void *realloc(void *inp, size_t size)
{
   int align_padding = 0;
   int requested_size = size;
   const dmalloc_pool_t pool = system_internal_memory;
   const char *description;
   void *pointer, *new_pointer;
   int orig_size, orig_align, orig_alias;
   MALLOC_HEADER_T *q;
   void *ret;

   // it's just a malloc if pointer is null
   if (!inp) {
      return rtos_malloc_priority(size, RTOS_ALIGN_DEFAULT, 1, rtos_default_malloc_name());
   }

   // it's just a free if size is zero
   if (!size) {
      free(inp);
      return 0;
   }

   orig_alias = ((uint32_t)inp) >> RTOS_PRIORITY_SHIFT;

   inp = DMALLOC_ALIAS(inp);

   q = ((MALLOC_HEADER_T *)inp)-1;

   // if there is padding our malloc header is earlier
   if (q->guardword == PADDINGWORD) {
      fatal_assert(q->requested_size_align == GUARDWORDHEAP);
      q = (MALLOC_HEADER_T *)q->description;
   }

   // guard at start
   fatal_assert(q->guardword == GUARDWORDHEAP);
   // mark it so double free not possible
   q->guardword = GUARDWORDALLOC;

   GETREQSIZEALIGN(q, orig_size, orig_align);

#ifdef TEST_ALLOC_FAIL
   if (requested_size >= orig_size)
   {
      if (should_check_fail())
      {
         RMP_PUT_LATCH;
         return 0;
      }
   }
#endif

   // guard at end
   {
      int i; unsigned *p = ((unsigned *)inp)+((orig_size+3)>>2);
      for (i=0; i<(int)GUARDWORDS; i++)
         fatal_assert(*p++==GUARDWORDHEAP);
   }
   description = (void *)q->description;

   pointer = (char *)q;

#ifdef DMALLOC_USE_ALIAS_DIRECT
   // dirty data may be harmful to dmalloc...
   {
      int range = ALIGN_UP(orig_size+orig_align-RTOS_ALIGN_DEFAULT, RTOS_ALIGN_DEFAULT) + sizeof(MALLOC_HEADER_T) + GUARDWORDS*sizeof(unsigned);
      vclib_dcache_flush_range(pointer, range);
   }
#endif

   // extra words for header and guard words
   size = ALIGN_UP(size+orig_align-RTOS_ALIGN_DEFAULT, RTOS_ALIGN_DEFAULT) + sizeof(MALLOC_HEADER_T) + GUARDWORDS*sizeof(unsigned);

   new_pointer = drealloc(pool, pointer, size);
   if (new_pointer) {
      if (new_pointer != pointer)
      {
         // the block has moved - patch up inp accordingly
         inp = (void *)((uint32_t)inp + ((uint32_t)new_pointer - (uint32_t)pointer));
      }
   }
   else
   {
      // the old block is still valid, even though the realloc failed
      q->guardword = GUARDWORDHEAP;
      RMP_PUT_LATCH;
      return NULL;
   }

   // return value of malloc + MALLOC_HEADER_T should be aligned to RTOS_ALIGN_DEFAULT
   vcos_assert((((unsigned)new_pointer + sizeof(MALLOC_HEADER_T)) & (RTOS_ALIGN_DEFAULT-1))==0);

   // the correctly aligned data we will return
   ret = (void *)ALIGN_UP((char *)new_pointer + sizeof(MALLOC_HEADER_T), orig_align);

   align_padding = (char *)ret - ((char *)new_pointer + sizeof(MALLOC_HEADER_T));
   vcos_assert((align_padding & (RTOS_ALIGN_DEFAULT-1))==0);

   if (ret != inp)
      memmove(ret, inp, orig_size);
   // our malloc header
   q = (MALLOC_HEADER_T *)new_pointer;
   // store description
   q->description = (unsigned)description;
   // store requested malloc size and alignment
   SETREQSIZEALIGN(q, requested_size, orig_align);

   // guard at end
   {
      int i; unsigned *p = ((unsigned *)ret)+((requested_size+3)>>2);
      for (i=0; i<(int)GUARDWORDS; i++)
         *p++ = GUARDWORDHEAP;
      vcos_assert((char *)p <= (char*)new_pointer+size);
   }

   if (align_padding > 0) {
      int i; unsigned *p = (unsigned *)new_pointer+sizeof(MALLOC_HEADER_T)/sizeof(unsigned);
      for (i=0; i<(align_padding>>2)-2; i++)
         *p++ = GUARDWORDHEAP;
      // we need to remember pointer returned by underlying malloc and flag there is padding
      *p++ = (unsigned)new_pointer;
      *p++ = PADDINGWORD;
      vcos_assert((char *)p <= (char*)new_pointer+size);
   }

#ifdef FILL_MALLOC
   short *instruction;
   for (instruction = (short *)ret; instruction < (short *)ret + (size>>1); instruction++)
      *instruction = BREAKPOINT;
#endif

   // guard at start - this must be set last so as not to allow the heap
   // checker to look at this block until it's in a consistent state
   q->guardword = GUARDWORDHEAP;

   RMP_PUT_LATCH;

   // returned pointer
   return (void *)((uint32_t)ALIAS_NORMAL(ret) + (orig_alias << RTOS_PRIORITY_SHIFT));
}

// wrapper for malloc routine
void *malloc(size_t size)
{
   return rtos_malloc_priority(size, RTOS_ALIGN_DEFAULT, 1, rtos_default_malloc_name());
}

// wrapper for calloc routine
void *calloc(size_t nelem, size_t el_size)
{
   return rtos_calloc_priority(nelem * el_size, RTOS_ALIGN_DEFAULT, 1, rtos_default_malloc_name());
}

/******************************************************************************
NAME
   rtos_setpriority

SYNOPSIS
   int rtos_setpriority(int priority)

FUNCTION
   Sets the priority level for mallocs.
   The current priority level should be set for the entire application
   to maximise the use of internal memory subject to never putting an important buffer
   in external memory.

RETURNS
   Previous priority level
******************************************************************************/

int rtos_setpriority(const uint32_t priority)
{
   int old = rtos_current_priority;
   rtos_current_priority = priority;
   return old;
}


/******************************************************************************
NAME
   rtos_getpriority

SYNOPSIS
   int rtos_getpriority(void)

RETURNS
   Priority level for mallocs
******************************************************************************/

int rtos_getpriority(void)
{
   return rtos_current_priority;
}

/******************************************************************************
NAME
   rtos_memory_is_valid

SYNOPSIS
   int rtos_memory_is_valid(void const *base, int length)

FUNCTION
   Makes all feasible sanity checks against a memory range to minimise the risk
   that operations within that range could cause memory corruption.

   It's expected that this will only be called within assert()s.

RETURNS
   Returns 0 if the memory range is unsafe, or non-zero if we don't know.
******************************************************************************/

int rtos_memory_is_valid(void const *base, int length)
{
   const int memory_size = (256*1024*1024);
   void const *low_base = ALIAS_NORMAL(base);

   /* The obvious NULL check, extended to cover all the secure memory.
    */
   if ((uintptr_t)low_base < 0x2000)
      return 0;

   /* Reject ranges that hit I/O */
   if (IS_ALIAS_PERIPHERAL(base))
      return 0;

   /* Reject ranges that wrap around or don't fit in memory.
    */
   if ((uintptr_t)length >= memory_size
      || (uintptr_t)low_base >= memory_size
      || (uintptr_t)low_base + (uintptr_t)length >= memory_size)
      return 0;

   base = DMALLOC_ALIAS(base);

   {
      void const *ptr = base;
      int size, align;
      MALLOC_HEADER_T const *q;

      if ((size = dmalloc_find(DMALLOC_POOL_ANY, &ptr)) > 0)
      {
         char const *description;
         vcos_assert(base >= ptr && (uintptr_t)base < (uintptr_t)ptr + size);

         q = ptr;
         fatal_assert(q->guardword == GUARDWORDHEAP);
         description = (char const *)q->description;
         GETREQSIZEALIGN(q, size, align);
         ptr = (void const *)ALIGN_UP(q + 1, align);

         if (base < ptr || (uintptr_t)base + length > (uintptr_t)ptr + size)
         {
            /* if you put a breakpoint here then 'description' might show
             * you the name of the allocation we're pointing at.
             */
            _nop();
            return 0;
         }
      }

      if (size < 0) /* dmalloc_find() thought the pointer was crazy */
         return 0;
   }

   /* TODO: pass the range to the debugger to be checked against the symbol
    * table to ensure that it doesn't cross any symbol boundaries.
    */

   /* TODO: Ensure that we're not pointing to code (perhaps dlopen() can leave
    * us some hints).
    */

   return 1; /* It might be OK */
}


/******************************************************************************
NAME
   rtos_find_heap_corruption

SYNOPSIS
   void const *rtos_find_heap_corruption(int flags)

FUNCTION
   Scan the heap for evidence of memory corruption.

   Flags can be used to modify the types of checks that are performed, but none
   are defined at present, so just pass 0.

   It's expected that this will only be called from test applications trying
   to get the earliest possible indication that memory has been corrupted.

RETURNS
   Returns NULL if the heap appears to be OK
   Returns a pointer to the first evident corruption, otherwise

NOTES
   It may make sense to find unallocated blocks and check that they've been
   zeroed
******************************************************************************/

extern void const *rtos_find_heap_corruption(int flags)
{
   const void *corruption = NULL;
   (void)flags;

   /* perhaps we could check empty blocks too */
   if (system_internal_memory != DMALLOC_POOL_INVALID)
   {
      RMP_GET_LATCH;
      corruption = dmalloc_scan(system_internal_memory, rtos_check_block, NULL, NULL);
      RMP_PUT_LATCH;
   } // if

   return corruption;
} // rtos_find_heap_corruption


void *rtos_pool_aligned_malloc( dmalloc_pool_t pool, size_t size, size_t align, const char *description)
{
   void *ret = NULL;
   int align_padding = 0;
   void *pointer = NULL;
   MALLOC_HEADER_T *q = NULL;
   uint32_t local_align = _max(align, RTOS_ALIGN_DEFAULT);
   uint32_t local_size = size;

   if ( 0 == local_size )
   {
      return( 0 );
   } // if

   RMP_GET_LATCH;

   // extra words for header and guard words
   local_size = ALIGN_UP(local_size+local_align-RTOS_ALIGN_DEFAULT, RTOS_ALIGN_DEFAULT) + sizeof(MALLOC_HEADER_T) + GUARDWORDS*sizeof(unsigned);
   pointer = dmalloc(pool, local_size);

   if (!pointer)
   {
      RMP_PUT_LATCH;
      // there is not enough available memory
      return 0;
   }

   // return value of malloc + MALLOC_HEADER_T should be aligned to RTOS_ALIGN_DEFAULT
   vcos_assert((((unsigned)pointer + sizeof(MALLOC_HEADER_T)) & (RTOS_ALIGN_DEFAULT-1))==0);

   // the correctly aligned data we will return
   ret = (void *)ALIGN_UP((char *)pointer + sizeof(MALLOC_HEADER_T), local_align);

   // we always invalidate cached data here whatever the alias. Data from before the malloc will not be used again (and dirty data may be harmful)
   // (probably don't need this if DMALLOC_USE_ALIAS_DIRECT, but whatever)
   vclib_dcache_invalidate_range(ret, size);

   align_padding = (char *)ret - ((char *)pointer + sizeof(MALLOC_HEADER_T));
   vcos_assert((align_padding & (RTOS_ALIGN_DEFAULT-1))==0);

   // our malloc header
   q = (MALLOC_HEADER_T *)pointer;
   // store description
   q->description = (unsigned)description;
   // store requested malloc size and alignment
   SETREQSIZEALIGN(q, size, local_align);

   // guard at end
   {
      int i; unsigned *p = ((unsigned *)ret)+((size+3)>>2);
      for (i=0; i<(int)GUARDWORDS; i++)
         *p++ = GUARDWORDHEAP;
      vcos_assert((char *)p <= (char*)pointer+local_size);
   }

   if (align_padding > 0)
   {
      int i; unsigned *p = (unsigned *)pointer+sizeof(MALLOC_HEADER_T)/sizeof(unsigned);
      for (i=0; i<(align_padding>>2)-2; i++)
         *p++ = GUARDWORDHEAP;
      // we need to remember pointer returned by underlying malloc and flag there is padding
      *p++ = (unsigned)pointer;
      *p++ = PADDINGWORD;
      vcos_assert((char *)p <= (char*)pointer+local_size);
   }

#ifdef FILL_MALLOC
   short *instruction;
   for (instruction = (short *)ret; instruction < (short *)ret + (size>>1); instruction++)
      *instruction = BREAKPOINT;
#endif

   // guard at start - this must be set last so as not to allow the heap
   // checker to look at this block until it's in a consistent state
   q->guardword = GUARDWORDHEAP;

   RMP_PUT_LATCH;

   return ALIAS_NORMAL(ret);
}


static void const *rtos_check_block(const void *base, int length, void *handle)
{
   MALLOC_HEADER_T const *q = base;
   unsigned int const *p;
   int malloc_size, align, i;

   (void)handle;

   if (q->guardword == GUARDWORDALLOC)
      // This block is in the process of being either allocated or freed,
      // and is not in a consistent state, so we can't check further
      return NULL;

   if (q->guardword != GUARDWORDHEAP)
      return &q->guardword;

   GETREQSIZEALIGN(q, malloc_size, align);

   base = (void *)ALIGN_UP((q + 1), align);

   if ((uintptr_t)base + malloc_size > (uintptr_t)q + length)
      return &q->requested_size_align;

   if ((uintptr_t)base != (uintptr_t)(q + 1))
   {
      p = (void *)(q + 1);
      while ((uintptr_t)(p + 2) < (uintptr_t)base)
         if (*p++ != GUARDWORDHEAP)
            return p - 1;
      if (*p++ != (uintptr_t)q)
         return p - 1;
      if (*p++ != PADDINGWORD)
         return p - 1;
   }

   p = (void *)((uintptr_t)base + ALIGN_UP(malloc_size, 4u));
   for (i = 0; i < GUARDWORDS; i++)
      if (*p++ != GUARDWORDHEAP)
         return p - 1;

   return NULL;
}

/******************************************************************************
NAME
   rtos_memory_find

SYNOPSIS
   int rtos_memory_find(void const **ptr)

* Arguments:
 *       const void **ptr
 *           - (input) pointer to the address we're looking for
 *             (output) if (return > 0) then a pointer to the
 *                      result of the corresponding rtos_malloc_priority(),
 *                      otherwise undefined
 *
 * Description: Find the base pointer and approximate size of
 *             the allocation containing the pointer at *ptr.
 *
 * Returns: int - size of allocated block, 0 if not found,
 *                or -1 if it's an illegal address
 *
 ***********************************************************/

int rtos_memory_find(const void **ptr)
{
	int size = 0, align = 0;
	MALLOC_HEADER_T const *q = 0;
	
	/* The obvious NULL check, extended to cover all the secure memory.
	*/
	if ((uintptr_t)ALIAS_NORMAL(*ptr) < 0x2000)
		return -1;

	/* Reject ranges that hit I/O */
	if (IS_ALIAS_PERIPHERAL(*ptr))
		return -1;

	*ptr = DMALLOC_ALIAS(*ptr);
	
	if ((size = dmalloc_find(DMALLOC_POOL_ANY, ptr)) <= 0)
		return size;

	q = *ptr;
	fatal_assert(q->guardword == GUARDWORDHEAP);
	GETREQSIZEALIGN(q, size, align);
	*ptr = (void const *)ALIGN_UP(q + 1, align);
	
	return size;
}

/************************************ End of file ******************************************************/
