/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "vcfw/rtos/common/rtos_common_mem.h"
#include "vcfw/rtos/common/rtos_common_mem_priv.h"
#include "vcfw/rtos/rtos.h"   /* for malloc functions */
#include <string.h>


#ifdef MEM_USE_SMALL_ALLOC_POOL
#error "Can't compile with MEM_USE_SMALL_ALLOC_POOL"
#endif

#ifdef DMALIB_AVAILABLE
#include "helpers/dmalib/dmalib.h"
#endif

#define MALLOC_ALIGN(size, align, desc) rtos_prioritymalloc((size), (align), RTOS_PRIORITY_DIRECT, (desc))
#define REALLOC(ptr, size) rtos_realloc_256bit((ptr), (size))     
#define FREE(ptr) rtos_priorityfree((ptr))

#define DMALIB_MEMSET_THRESHOLD (1<<13) /* Allocations > this size use vclib_memset. Must be power of 2. */
#define USE_DMALIB_MEMSET_MASK (~(DMALIB_MEMSET_THRESHOLD - 1))

static RCM_INLINE void memcpy_fast(void *dst, const void *src, uint32_t size);
static RCM_INLINE void initialise_memory(void *ptr, MEM_FLAG_T flags, uint32_t size);
static RCM_INLINE int is_power_of_2(uint32_t x);
static RCM_INLINE uint32_t round_up(uint32_t x, uint32_t y);

#ifndef MEM_LEGACY_BLOCK_SIZE
#define MEM_LEGACY_BLOCK_SIZE    0x00210000
#endif

#ifndef MEM_LEGACY_BLOCK_ALIGN
#define MEM_LEGACY_BLOCK_ALIGN   4096
#endif


typedef struct {
   MEM_HANDLE_T next;

   MEM_HEADER_X_T x;

   uint32_t size;
   const char *desc;
   MEM_TERM_T term;

   void *ptr;

   uint32_t ref_count;
   //uint32_t retain_count;

} MEM_ON_MALLOC_HEADER_T;

typedef struct {
   uint32_t max_handles;
   MEM_ON_MALLOC_HEADER_T *handles;
   VCOS_MUTEX_T mutex;
   MEM_HANDLE_T free_handle;
   uint32_t legacy_size;
   uint32_t legacy_align;
} MEM_ON_MALLOC_MANAGER_T;

static MEM_ON_MALLOC_MANAGER_T manager;

/******************************************************************************
Global initialisation helpers
******************************************************************************/

int32_t rtos_common_mem_init( void )
{
   int32_t success = 0;
   uint32_t handles_size;
   void *handles_base;

   mem_get_default_partition(NULL, NULL, &handles_base, &handles_size);

   // make a wild guess about the sizes needed
   if (!handles_size)
      handles_size = 4096*4;

   if (!handles_base)
      handles_base = MALLOC_ALIGN(handles_size, 16, "rtos_common_mem_init");
   vcos_assert(handles_base);
   if (!handles_base)
      success = -1;

   if ((success >= 0) &&
       mem_init(NULL, 0, handles_base, handles_size))
      success = -1;

   if (success < 0)
   {
      if (handles_base) FREE(handles_base);
   }

   return success;
}

/******************************************************************************
Pool management API
******************************************************************************/


void mem_get_default_partition(void **xxx_mempool_base, uint32_t *xxx_mempool_size, void **mempool_handles_base, uint32_t *mempool_handles_size)
{
#ifndef __VIDEOCORE__
   /*
    * Dirty ugly hack.  TODO - discuss whether running cubes.exe on windows ought to refer to vcfw at all
    * This hack is what was in egl/server.c - copied here just to get driver to build when using simpenrose
    */
   *mempool_handles_base = 0;
   *mempool_handles_size = 32 * 1024;
#else
   // the following come from the platform makefile, through the linker command file
   extern _Far unsigned char __MEMPOOL_HANDLES_START[], __MEMPOOL_HANDLES_END[];
   *mempool_handles_base = __MEMPOOL_HANDLES_START;
   *mempool_handles_size = __MEMPOOL_HANDLES_END-__MEMPOOL_HANDLES_START;
#endif
}

static RCM_INLINE int is_power_of_2(uint32_t x)
{
   return (x != 0) && ((x & (x - 1)) == 0);
}

static RCM_INLINE uint32_t round_up(uint32_t x, uint32_t y)
{
   vcos_assert(is_power_of_2(y));
   return (x + (y - 1)) & ~(y - 1);
}

int mem_init(void *xxx_mempool_base, uint32_t xxx_mempool_size, void *mempool_handles_base, uint32_t mempool_handles_size)
{
   int success;
   int i;

   manager.handles = (MEM_ON_MALLOC_HEADER_T *)mempool_handles_base;
   manager.max_handles = mempool_handles_size / sizeof(MEM_ON_MALLOC_HEADER_T);

   manager.legacy_align = _max(MEM_LEGACY_BLOCK_ALIGN, sizeof(MEM_HEADER_T));
   manager.legacy_size = round_up(MEM_LEGACY_BLOCK_SIZE, manager.legacy_align);

   for (i = 1; i < manager.max_handles; i++)
   {
      manager.handles[i].ptr = NULL;
      if (i == manager.max_handles - 1)
         manager.handles[i].next = MEM_INVALID_HANDLE;
      else
         manager.handles[i].next = (MEM_HANDLE_T)(i + 1);
   }
   manager.free_handle = (MEM_HANDLE_T)(1);

   success = vcos_mutex_create(&manager.mutex, "mutex for rtos_common_mem on malloc");
   if (success)
      return success;

   if (mem_alloc_ex(0, 4, MEM_FLAG_NONE, "zero size", MEM_COMPACT_NONE) != MEM_ZERO_SIZE_HANDLE)
      vcos_assert(0);

   if (mem_alloc_ex(1, 1, MEM_FLAG_NONE, "empty string", MEM_COMPACT_NONE) != MEM_EMPTY_STRING_HANDLE)
      vcos_assert(0);

   return 0;
}

void mem_term(void)
{
   vcos_mutex_delete(&manager.mutex);
   manager.handles = 0;
   manager.max_handles = 0;
}

void mem_compact(mem_compact_mode_t mode)
{
   /* Nothing to do */
}

/******************************************************************************
Movable memory core API
******************************************************************************/

MEM_HANDLE_T mem_alloc_ex(
   uint32_t size,
   uint32_t align,
   MEM_FLAG_T flags,
   const char *desc,
   mem_compact_mode_t mode)
{
   void *ptr =NULL;
   MEM_HANDLE_T result = MEM_HANDLE_INVALID;
   uint32_t i;

   vcos_mutex_lock(&manager.mutex);

   if (manager.free_handle != MEM_INVALID_HANDLE)
   {
      i = (uint32_t)manager.free_handle;
      manager.free_handle = manager.handles[i].next;

      if (size != 0)
      {
         ptr = MALLOC_ALIGN(size, align, desc);
      }

      if (ptr != NULL || size == 0)
      {
         manager.handles[i].desc = desc;
         manager.handles[i].ptr = ptr;
         manager.handles[i].next = MEM_INVALID_HANDLE;
         manager.handles[i].ref_count = 1;
         //manager.handles[i].retain_count = 0;  /* XXX? */
         manager.handles[i].size = size;
         manager.handles[i].term = NULL;
         manager.handles[i].x.exec_locked = 0;
         manager.handles[i].x.flags = flags;
         manager.handles[i].x.lock_count = 0;
         manager.handles[i].x.log2_align = _msb(align);
         manager.handles[i].x.size_is_zero = size == 0;

         if (size != 0)
            initialise_memory(ptr, flags, size);

         result = (MEM_HANDLE_T)i;
      }
   }
   vcos_mutex_unlock(&manager.mutex);

   return result;
}

void mem_acquire(MEM_HANDLE_T handle)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_assert(manager.handles[i].ref_count > 0);

   vcos_mutex_lock(&manager.mutex);
   manager.handles[i].ref_count++;
   vcos_assert(manager.handles[i].ref_count);
   vcos_mutex_unlock(&manager.mutex);
}

void mem_acquire_retain(MEM_HANDLE_T handle)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_assert(manager.handles[i].ref_count > 0);

   vcos_mutex_lock(&manager.mutex);
   manager.handles[i].ref_count++;
   vcos_assert(manager.handles[i].ref_count);

   /*if (manager.handles[i].x.flags & MEM_FLAG_DISCARDABLE)
   {
      manager.handles[i].retain_count++;
      vcos_assert(manager.handles[i].retain_count);
   }*/
   vcos_mutex_unlock(&manager.mutex);
}

static void free_thing(uint32_t i)
{
   MEM_TERM_T term;
   vcos_assert(i < manager.max_handles);
   vcos_assert(manager.handles[i].ref_count == 1);

   term = manager.handles[i].term;

   if (term)
   {
      vcos_mutex_unlock(&manager.mutex);

      vcos_assert (manager.handles[i].size == 0 || manager.handles[i].ptr != NULL); 

      term(manager.handles[i].ptr, manager.handles[i].size);

      vcos_mutex_lock(&manager.mutex);
   }

   if (manager.handles[i].size != 0)
   {
      vcos_assert(manager.handles[i].ptr != NULL); 
      FREE(manager.handles[i].ptr);
   }

   
   manager.handles[i].ref_count = 0;
   manager.handles[i].ptr = NULL;
   manager.handles[i].next = manager.free_handle;
   manager.free_handle = (MEM_HANDLE_T)i;
}

void mem_release(MEM_HANDLE_T handle)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);

   vcos_mutex_lock(&manager.mutex);
   vcos_assert(manager.handles[i].ref_count);

   if (manager.handles[i].ref_count == 1)
   {
      free_thing(i);
   }
   else
   {
      manager.handles[i].ref_count--;
   }
   vcos_mutex_unlock(&manager.mutex);
}

int mem_try_release(MEM_HANDLE_T handle)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   vcos_assert(manager.handles[i].ref_count);

   if (manager.handles[i].ref_count == 1)
   {
      vcos_mutex_unlock(&manager.mutex);
      return 0;
   }

   manager.handles[i].ref_count--;
   vcos_mutex_unlock(&manager.mutex);
   return 1;
}

uint32_t mem_get_space(MEM_HANDLE_T handle)
{
   vcos_assert(0);
   return 0;
}

uint32_t mem_get_size(MEM_HANDLE_T handle)
{
   uint32_t result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = manager.handles[i].size;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

uint32_t mem_get_align(MEM_HANDLE_T handle)
{
   uint32_t result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = 1 << manager.handles[i].x.log2_align;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

MEM_FLAG_T mem_get_flags(MEM_HANDLE_T handle)
{
   uint32_t result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = 1 << manager.handles[i].x.log2_align;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

uint32_t mem_get_ref_count(MEM_HANDLE_T handle)
{
   uint32_t result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = manager.handles[i].ref_count;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

uint32_t mem_get_lock_count(MEM_HANDLE_T handle)
{
   vcos_assert(0);
   return 0;
}

uint32_t mem_get_retain_count(MEM_HANDLE_T handle)
{
   /*uint32_t result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = manager.handles[i].ref_count;

   vcos_mutex_unlock(&manager.mutex);
   return result;*/
   vcos_assert(0);
   return 0;
}

const char *mem_get_desc(MEM_HANDLE_T handle)
{
   const char *result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = manager.handles[i].desc;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

void mem_set_desc(MEM_HANDLE_T handle, const char *desc)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   manager.handles[i].desc = desc;

   vcos_mutex_unlock(&manager.mutex);
}

void mem_set_term(MEM_HANDLE_T handle, MEM_TERM_T term)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   manager.handles[i].term = term;

   vcos_mutex_unlock(&manager.mutex);
}

MEM_TERM_T mem_get_term(MEM_HANDLE_T handle)
{
   MEM_TERM_T result;
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   result = manager.handles[i].term;

   vcos_mutex_unlock(&manager.mutex);
   return result;
}

void mem_set_user_flag(MEM_HANDLE_T handle, int flag)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   if (flag)
      manager.handles[i].x.flags |= MEM_FLAG_USER;
   else
      manager.handles[i].x.flags &= ~MEM_FLAG_USER;

   vcos_mutex_unlock(&manager.mutex);
}

static RCM_INLINE void initialise_memory(void *ptr, MEM_FLAG_T flags, uint32_t size)
{
   if ((flags & MEM_FLAG_NO_INIT) == 0)
   {
#ifdef DMALIB_AVAILABLE
      if(size & USE_DMALIB_MEMSET_MASK)
      {
         dma_memset(ptr, (flags & MEM_FLAG_ZERO) ? 0 : MEM_HANDLE_INVALID, size);
      }
      else
#endif
      {
         memset(ptr, (flags & MEM_FLAG_ZERO) ? 0 : MEM_HANDLE_INVALID, size);
      }
   }
   else
   {
      // in a debug build, set uninitialised memory to an illegal instruction and misaligned pointer
#if defined(NDEBUG) || defined(FORSIM) || defined(__VC_SIM__)
#else
      // don't do this for large allocations (eg frame buffers) as it hammers memory bandwidth on target
      if((size & USE_DMALIB_MEMSET_MASK) == 0)
      {
         memset(ptr, 0xdddddddd, size);
      }
#endif
   }
}

int mem_resize_ex(MEM_HANDLE_T handle, uint32_t size, mem_compact_mode_t mode)
{
   uint32_t align;
   uint32_t malloc_size;
   void *ptr = NULL;
   int old_size;
#ifndef REALLOC
   void *old_ptr=NULL;
#endif

   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_mutex_lock(&manager.mutex);

   old_size = manager.handles[i].size;
   align = 1 << manager.handles[i].x.log2_align;
   malloc_size = size + align - 1;
#ifdef REALLOC
   ptr = REALLOC(manager.handles[i].ptr, malloc_size);
#else
   old_ptr = manager.handles[i].ptr;

   ptr = MALLOC_ALIGN(malloc_size, align, "resize_ex");
#endif

   if (ptr == NULL)
   {
      /* Out of memory */
      vcos_mutex_unlock(&manager.mutex);
      return 0;
   }

   manager.handles[i].ptr = ptr;
   manager.handles[i].size = size;

#ifndef REALLOC
	//take care of old memory
	if(old_size != 0)
	{
		//memcpy( (char *)ptr, (char *)old_ptr, old_size);  
		memcpy( (char *)ptr, (char *)old_ptr, old_size < size ? old_size : size);

		assert (old_ptr != NULL); 
		FREE(old_ptr);
	}
#endif

   if (size > old_size)
   {
      initialise_memory((char *)ptr + old_size, manager.handles[i].x.flags, size - old_size);
   }

   vcos_mutex_unlock(&manager.mutex);
   return 1;
}

void *mem_lock(MEM_HANDLE_T handle)
{
   uint32_t i = (uint32_t)handle;
   vcos_assert(handle != MEM_INVALID_HANDLE);
   vcos_assert(i < manager.max_handles);
   vcos_assert(manager.handles[i].ptr != 0 || manager.handles[i].size == 0);
   return manager.handles[i].ptr;
}

void mem_lock_multiple(void **pointers, MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
   uint32_t i;

   for (i = 0; i < n; i++)
   {
      if (handles[i].mh_handle == MEM_INVALID_HANDLE)
      {
         pointers[i] = (void *)handles[i].offset;
      }
      else
      {
         uint32_t j = (uint32_t)handles[i].mh_handle;
         vcos_assert(j < manager.max_handles);
         pointers[i] = (char *)manager.handles[j].ptr + handles[i].offset;
      }
   }
}

void (*mem_lock_exec(MEM_HANDLE_T handle))()
{
   vcos_assert(0);
   return NULL;
}

void mem_unlock(MEM_HANDLE_T handle)
{
   /* Nothing to do */
}

void mem_unlock_multiple(MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
   /* Nothing to do */
}

void mem_unlock_unretain_release_multiple(MEM_HANDLE_OFFSET_T *handles, uint32_t n)
{
   uint32_t i;

   vcos_mutex_lock(&manager.mutex);
   for (i = 0; i < n; i++)
   {
      if (handles[i].mh_handle != MEM_INVALID_HANDLE)
      {
         uint32_t j = (uint32_t)handles[i].mh_handle;
         vcos_assert(j < manager.max_handles);

         /* Nothing to do for unlock or unretain */

         if (manager.handles[j].ref_count == 1)
         {
            free_thing(j);
         }
         else
         {
            manager.handles[j].ref_count--;
         }
      }
   }
   vcos_mutex_unlock(&manager.mutex);
}

void mem_abandon(MEM_HANDLE_T handle)
{
   /* Nothing to do */
}

int mem_retain(MEM_HANDLE_T handle)
{
   /* Nothing to do */
   return mem_get_size(handle) != 0;
}

void mem_unretain(MEM_HANDLE_T handle)
{
   /* Nothing to do */
}

void *mem_lock_perma(MEM_HANDLE_T handle)
{
   return mem_lock(handle);
}

void mem_unlock_perma(MEM_HANDLE_T handle)
{
   mem_unlock(handle);
}

void *mem_alloc_legacy_ex(MEM_FLAG_T flags)
{
   vcos_assert((flags & MEM_FLAG_ALIAS_MASK) == flags);

   return MALLOC_ALIGN(manager.legacy_size, manager.legacy_align, "legacy block");
}

void mem_free_legacy(void *ptr)
{
   vcos_assert (ptr != NULL);
   FREE(ptr);
}

int mem_init_legacy(uint32_t size, uint32_t align, int count_max)
{
   manager.legacy_size = size;
   manager.legacy_align = align;
   return 16;    /* TODO: return maximum number of legacy blocks */
}

uint32_t mem_get_legacy_size(void)
{
   return manager.legacy_size;
}

void mem_validate(void)
{
   vcos_assert(0);
}

/******************************************************************************
Long-term lock owners' API
******************************************************************************/

int mem_register_callback(mem_callback_func_t func, uintptr_t context)
{
   /* Nothing to do */
   return 1;
}

void mem_unregister_callback(mem_callback_func_t func, uintptr_t context)
{
   /* Nothing to do */
}

/******************************************************************************
Movable memory helpers
******************************************************************************/

void rtos_common_mem_shuffle_enable(int enable)
{
   /* Nothing to do */
}

MEM_HANDLE_T mem_strdup_ex(
   const char *str,
   mem_compact_mode_t mode)
{
   MEM_HANDLE_T handle = mem_alloc_ex((uint32_t)strlen(str) + 1, 1, MEM_FLAG_NONE, "mem_strdup", mode);
   if (handle == MEM_HANDLE_INVALID) {
      return MEM_HANDLE_INVALID;
   }
   strcpy((char *)mem_lock(handle), str);
   mem_unlock(handle);
   return handle;
}

static RCM_INLINE void memcpy_fast(void *dst, const void *src, uint32_t size)
{
#ifdef DMALIB_AVAILABLE
   // Use DMA for large blocks
   if (size > 64) {
      dma_memcpy(dst, src, size);
   } else {
#endif
      memcpy(dst, src, size);
#ifdef DMALIB_AVAILABLE
   }
#endif
}

MEM_HANDLE_T mem_dup_ex(
   MEM_HANDLE_T handle,
   const char *desc,
   mem_compact_mode_t mode)
{
   MEM_HANDLE_T newhandle;
   vcos_assert(handle != MEM_HANDLE_INVALID);
   vcos_assert(mem_get_term(handle) == 0);
   newhandle = mem_alloc_ex(
      mem_get_size(handle),
      mem_get_align(handle),
      mem_get_flags(handle),
      desc,
      mode);

   if (newhandle == MEM_HANDLE_INVALID) {
      return MEM_HANDLE_INVALID;
   }

   memcpy_fast(mem_lock(newhandle), mem_lock(handle), mem_get_size(handle));
   mem_unlock(newhandle);
   mem_unlock(handle);
   return newhandle;
}

void mem_print_state()
{
   vcos_assert(0);
}

void mem_print_small_alloc_pool_state()
{
   vcos_assert(0);
}

void mem_get_stats(uint32_t *blocks, uint32_t *bytes, uint32_t *locked)
{
   vcos_assert(0);
}

uint32_t mem_get_total_space(void)
{
   vcos_assert(0);
   return 0;
}

uint32_t mem_get_free_space(void)
{
   vcos_assert(0);
   return 0;
}

int mem_is_relocatable(const void *ptr)
{
   vcos_assert(0);
   return 0;
}

