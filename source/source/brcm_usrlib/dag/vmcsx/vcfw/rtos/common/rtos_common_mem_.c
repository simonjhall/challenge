#if 0
/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "rtos_common_mem.h"
#include "rtos_common_mem_priv.h"
#include "dmalloc.h"
//#include "utils/Log.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
//#include <utils/threads.h>

#define MEM_FLAG_INCOHERENT

#define GLOBAL_POOL_SIZE 6*1024*1024

// #define ASSERT_ON_ALLOC_FAIL
// #define ASSERT_ON_COMPACT_TIMEOUT
// Activate the shuffle task by adding MEM_SHUFFLE=<ms> to the make command line
// #define SHUFFLE_INTERVAL_MSECS 2500 // 2.5s
#ifdef _VIDEOCORE
#define AGGRESSIVE_COMPACTION_LED 5
#endif

#ifdef _VIDEOCORE
   #include "vcfw/rtos/rtos.h"
# ifdef DMALIB_AVAILABLE
   #include "helpers/dmalib/dmalib.h"
# endif
   #include "vcfw/vclib/vclib.h"
   #include "vcfw/drivers/chip/intctrl.h"
   #include "vcfw/drivers/device/leds.h"
#else
   #define RTOS_ALIAS_NORMAL(x)           (x)
   #define RTOS_ALIAS_DIRECT(x)           (x)
   #define RTOS_ALIAS_COHERENT(x)         (x)
   #define RTOS_ALIAS_L1_NONALLOCATING(x) (x)

   #define vclib_dcache_invalidate_range(a, b)  do {} while(0)
   #define vclib_dcache_flush_range(a, b)       do {} while(0)
   #define vclib_l1cache_invalidate_range(a, b) do {} while(0)
   #define vclib_icache_flush()                 do {} while(0)
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define HEADERS_IN_DIRECT_ALIAS 0
#define HANDLES_IN_DIRECT_ALIAS 0

#include "interface/vcos/vcos.h"

// Define these flags by adding MEM_CHECK=1 or MEM_CHECK_FREE=1 to the make command line.
// #define MEM_AUTO_VALIDATE
// #define MEM_FILL_FREE_SPACE

#if VCOS_HAVE_RTOS && VCOS_CAN_SET_STACK_ADDR
#define AGGRESSIVE_COMPACTION_TIMEOUT 100 /* ms */
#endif

#ifndef MEM_LEGACY_BLOCK_SIZE
#ifdef BRCM_V3D_OPT
#define MEM_LEGACY_BLOCK_SIZE    0x00200000
#else
#define MEM_LEGACY_BLOCK_SIZE    0x00210000
#endif
#endif

#ifndef MEM_LEGACY_BLOCK_ALIGN
#define MEM_LEGACY_BLOCK_ALIGN   4096
#endif

#ifndef MEM_LEGACY_BLOCK_MAX
#define MEM_LEGACY_BLOCK_MAX     32
#endif

// 0: no checking;  1: reads into free-space permitted (writes/execution trapped);  2: strict checking
#ifndef MEM_FREESPACE_PROTECT
#define MEM_FREESPACE_PROTECT    1
#endif

#define DMALIB_MEMSET_THRESHOLD (1<<13) /* Allocations > this size use vclib_memset. Must be power of 2. */
#define USE_DMALIB_MEMSET_MASK (~(DMALIB_MEMSET_THRESHOLD - 1))

/* If the lock count on a block exceeds this value, aggressive compaction will skip it. */
#define MEM_LOCK_PERMA_THRESHOLD  8

static void resume_compaction(void);
#if HEADERS_IN_DIRECT_ALIAS
static void resume_compaction_slow(void);
#endif

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

static RCM_INLINE int is_power_of_2(uint32_t x)
{
   return (x != 0) && ((x & (x - 1)) == 0);
}

static RCM_INLINE uint32_t round_up(uint32_t x, uint32_t y)
{
   vcos_assert(is_power_of_2(y));
   return (x + (y - 1)) & ~(y - 1);
}

#ifndef _VIDEOCORE
static RCM_INLINE int32_t _min(int32_t x, int32_t y)
{
   return x < y ? x : y;
}

static RCM_INLINE int32_t _max(int32_t x, int32_t y)
{
   return x > y ? x : y;
}

static RCM_INLINE int32_t _msb(uint32_t x) /* unsigned to get lsr */
{
   int32_t msb = -1;
   while (x != 0) {
      ++msb;
      x >>= 1;
   }
   return msb;
}
#endif

#define MEM_HANDLE_BIT_SMALL_ALLOC (void *)1
#define MEM_HANDLE_BIT_SMALL_ALLOC (void *)2

typedef struct{
	unsigned int a[8];
} MEM_HEADER_T;

/* a bunch of stuff below relies on MEM_HEADER_T being exactly 32 bytes */
vcos_static_assert(sizeof(MEM_HEADER_T) == 32);

static RCM_INLINE int is_small_alloc(MEM_HANDLE_T handle)
{
   return handle & MEM_HANDLE_BIT_SMALL_ALLOC;
}

#ifdef BRCM_V3D_OPT
#define GEMEMALLOC_WRAP_MAGIC  'B'
#define GEMEMALLOC_WRAP_ACQUIRE_BUFFER _IOWR(GEMEMALLOC_WRAP_MAGIC,  1, unsigned long)
#define GEMEMALLOC_WRAP_RELEASE_BUFFER _IOW(GEMEMALLOC_WRAP_MAGIC,  2, unsigned long)

#define GEMEMALLOC_WRAP_MAXNR 15

typedef struct {
	unsigned long busAddress;
	unsigned int size;
} GEMemallocwrapParams;

static int fd_gemem = -1;
static RCM_INLINE int is_big_alloc(MEM_HANDLE_T handle)
{
   return handle & MEM_HANDLE_BIT_BIG_ALLOC;
}
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL

#ifdef BRCM_V3D_OPT
#define MEM_BIG_ALLOC_COUNT 10000
#endif
#define MEM_SMALL_ALLOC_COUNT 1000
#define MEM_SMALL_ALLOC_ALIGN 32
#define MEM_SMALL_ALLOC_STRIDE 96
#define MEM_SMALL_ALLOC_SIZE (MEM_SMALL_ALLOC_STRIDE - sizeof(MEM_HEADER_T))

typedef struct {
   MEM_HEADER_T header;
#ifdef BRCM_V3D_OPT
	char data[];
#else
   char data[MEM_SMALL_ALLOC_SIZE];
#endif
} MEM_SMALL_ALLOC_T;

#ifdef WIN32
__declspec(align(MEM_SMALL_ALLOC_ALIGN))
#endif
#ifdef __CC_ARM
__attribute__((aligned(MEM_SMALL_ALLOC_ALIGN)))
#endif
#ifdef BRCM_V3D_OPT
static MEM_SMALL_ALLOC_T* g_smallallocs[MEM_SMALL_ALLOC_COUNT];
static MEM_SMALL_ALLOC_T* g_bigallocs[MEM_BIG_ALLOC_COUNT];
static int g_bigalloc_count = 0;
#ifdef _VIDEOCORE
//#pragma Align_to(MEM_SMALL_ALLOC_ALIGN, g_smallallocs)
#endif
#else
static MEM_SMALL_ALLOC_T g_smallallocs[MEM_SMALL_ALLOC_COUNT];
#ifdef _VIDEOCORE
#pragma Align_to(MEM_SMALL_ALLOC_ALIGN, g_smallallocs)
#endif
#endif
static MEM_HANDLE_T g_smallallocs_free_handle;
static uint32_t g_smallallocs_num_handles;

static RCM_INLINE MEM_HEADER_T *get_small_alloc_header(MEM_HANDLE_T handle)
{
   uint32_t n;

   vcos_assert(is_small_alloc(handle));

   n = (uint32_t)handle & ~MEM_HANDLE_BIT_SMALL_ALLOC;
   vcos_assert(n < g_smallallocs_num_handles);

#ifdef BRCM_V3D_OPT
if(g_smallallocs[n])
	{
	return &g_smallallocs[n]->header;
	}
else
	return NULL;
#else
   return &g_smallallocs[n].header;
#endif
}
static RCM_INLINE void *get_small_alloc_data(MEM_HANDLE_T handle)
{
   uint32_t n;

   vcos_assert(is_small_alloc(handle));

   n = (uint32_t)handle & ~MEM_HANDLE_BIT_SMALL_ALLOC;
   vcos_assert(n < g_smallallocs_num_handles);

#ifdef BRCM_V3D_OPT
	if(g_smallallocs[n])
		{
		return &g_smallallocs[n]->data;
		}
	else
		return NULL;
#else
   return &g_smallallocs[n].data;
#endif
}

#endif //MEM_USE_SMALL_ALLOC_POOL

#ifdef BRCM_V3D_OPT
static RCM_INLINE MEM_HEADER_T *get_big_alloc_header(MEM_HANDLE_T handle)
{
   uint32_t n;

   vcos_assert(is_big_alloc(handle));

   n = (uint32_t)handle & ~MEM_HANDLE_BIT_BIG_ALLOC;
   vcos_assert(n < g_smallallocs_num_handles);

	if(g_bigallocs[n])
		{
		return &g_bigallocs[n]->header;
		}
	else
		return NULL;
}

static RCM_INLINE void *get_big_alloc_data(MEM_HANDLE_T handle)
{
   uint32_t n;

   vcos_assert(is_big_alloc(handle));

   n = (uint32_t)handle & ~MEM_HANDLE_BIT_BIG_ALLOC;
   vcos_assert(n < g_smallallocs_num_handles);

   if(g_bigallocs[n])
	   {
	   return &g_bigallocs[n]->data;
	   }
   else
	   return NULL;
}
#endif

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

/******************************************************************************
manager
******************************************************************************/

static MEM_MANAGER_T g_mgr = { 0 };

#ifdef BRCM_V3D_OPT
extern void *memory_pool_base ; // virtual address
extern unsigned long memory_pool_addr ; 	// physical address
extern unsigned int memory_pool_size ;

typedef struct _WrapList{
	void* virtual_address;
	uint32_t physical_address;
	uint32_t size;
	struct _WrapList* next;
}WrapList;

WrapList* g_WrapList = NULL;
static uint32_t calc_space(uint32_t size, uint32_t align);
#define ENABLE_PHYSMEM
#ifdef ENABLE_PHYSMEM
unsigned int  khrn_hw_addr(const void *addr)
{
	if((addr >= memory_pool_base) &&(addr < (memory_pool_base +memory_pool_size)))
		{
		return (unsigned int)((unsigned int)(addr - memory_pool_base) + memory_pool_addr);
		}
	else
		{
		int i=0;
		for(i=0;i<MEM_BIG_ALLOC_COUNT;i++)
			{
			if(g_bigallocs[i])
				{
				void* pool_base = (void*)g_bigallocs[i];
				MEM_HEADER_T* header = (MEM_HEADER_T*)g_bigallocs[i];
				unsigned int pool_size = calc_space(header->size,1<<(header->x.log2_align));
				unsigned int pool_addr = (void*)g_bigallocs[i]->header.space;
				int pgsize = 4096;
				pool_size = (pool_size + pgsize) & (~(pgsize - 1));
				if((addr >= pool_base) &&(addr < (pool_base + pool_size)))
					{
					return (unsigned int)((unsigned int)(addr - pool_base) + pool_addr);
					}
				}
			}
		}

	vcos_quickslow_mutex_lock(&g_mgr.mutex);
	if(g_WrapList != NULL)
		{
		WrapList* head = g_WrapList;
		do{
			if(head->virtual_address == addr)
				{
				vcos_quickslow_mutex_unlock(&g_mgr.mutex);
				return head->physical_address;
				}
			head = head->next;
			}while(head != NULL);
		}
	vcos_quickslow_mutex_unlock(&g_mgr.mutex);
	LOGE("Unable to Find Phys Addr for %x",addr);
//	char* temp = NULL;
//	*temp = 0;
//	return (unsigned int)((unsigned int)(addr - memory_pool_base) + memory_pool_addr);
	return 0;
}

void *khrn_hw_unaddr(uint32_t addr)
{
	if((addr >= memory_pool_addr) &&(addr < (memory_pool_addr +memory_pool_size)))
		{
		return (void *)((unsigned int)(addr - memory_pool_addr) + memory_pool_base);
		}
	else
		{
		int i=0;
		for(i=0;i<MEM_BIG_ALLOC_COUNT;i++)
			{
			if(g_bigallocs[i])
				{
				void* pool_base = (void*)g_bigallocs[i];
				MEM_HEADER_T* header = (MEM_HEADER_T*)g_bigallocs[i];
				unsigned int pool_size = calc_space(header->size,1<<(header->x.log2_align));
				unsigned int pool_addr = (void*)g_bigallocs[i]->header.space;
				int pgsize = 4096;
				pool_size = (pool_size + pgsize) & (~(pgsize - 1));
				if((addr >= pool_addr) &&(addr < (pool_addr + pool_size)))
					{
					return (void *)((unsigned int)(addr - pool_addr) + pool_base);
					}
				}
			}
		}
	vcos_quickslow_mutex_lock(&g_mgr.mutex);
	if(g_WrapList != NULL)
		{
		WrapList* head = g_WrapList;
		do{
			if(head->physical_address == addr)
				{
				vcos_quickslow_mutex_unlock(&g_mgr.mutex);
				return head->virtual_address;
				}
			head = head->next;
			}while(head != NULL);
		}
	vcos_quickslow_mutex_unlock(&g_mgr.mutex);
	LOGE("Unable to Find Virt Addr for %x",addr);
//	return (void *)((unsigned int)(addr - memory_pool_addr) + memory_pool_base);
	return NULL;
}

#else
static uint32_t khrn_hw_addr(const void *addr)
{
   return (uint32_t)((uintptr_t)(addr - memory_pool_base) + memory_pool_addr);
}

static void *khrn_hw_unaddr(uint32_t addr)
{
   return (void *)((uintptr_t)(addr - memory_pool_addr) + memory_pool_base);
}

#endif

#endif

#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
/* These have been lifted out of g_mgr because they are conditional */
static VCOS_THREAD_T g_aggressive_timeout_task;
static int     g_aggressive_timeout_stack[256];
VCOS_TIMER_T   g_aggressive_timeout_timer;
VCOS_EVENT_T   g_aggressive_timeout_latch;
MEM_HEADER_T * g_aggressive_locked_latched;
#endif

#ifdef SHUFFLE_INTERVAL_MSECS
static VCOS_THREAD_T g_shuffle_task;
static int     g_shuffle_stack[256];
#endif

#ifdef AGGRESSIVE_COMPACTION_LED
static const LED_DRIVER_T *g_led_driver;
static DRIVER_HANDLE_T g_led_handle;
#endif

/* This is a bitmask indicating which cores need flushing on a mem_lock_exec call */
static uint32_t mem_need_icache_flush = 0;

/******************************************************************************
helpers
******************************************************************************/

static RCM_INLINE int is_aligned_32(const void *p)
{
   return ((uintptr_t)p & 0x1f) == 0;
}

static RCM_INLINE void *align_to(const void *p, uint32_t align)
{
   return (void *)(uintptr_t)round_up((uint32_t)(uintptr_t)p, align);
}

/*
   Find the space required to store a block, consisting of

   - header
   - content
   - padding

   rounding up to the next multiple of 32 bytes
*/

static uint32_t calc_space(uint32_t size, uint32_t align)
{
   vcos_assert(is_power_of_2(align));

   return sizeof(MEM_HEADER_T) + round_up(size + (_max(align, 32) - 32), 32);
}

/*
   Get the data pointer for a block, given

   - the start address of the block
   - the alignment of the data
*/

static void *get_data_pointer(const MEM_HEADER_T *header, uint32_t align, MEM_FLAG_T flags)
{
   vcos_assert(is_aligned_32(header));
   vcos_assert(is_power_of_2(align));

   {
      void *v = align_to(header + 1, align);

      switch (flags & MEM_FLAG_ALIAS_MASK)
      {
      case MEM_FLAG_COHERENT:
         return RTOS_ALIAS_COHERENT(v);
      case MEM_FLAG_DIRECT:
         return RTOS_ALIAS_DIRECT(v);
      case MEM_FLAG_L1_NONALLOCATING:
         return RTOS_ALIAS_L1_NONALLOCATING(v);
      default:
#if HEADERS_IN_DIRECT_ALIAS
         return RTOS_ALIAS_NORMAL(v);
#else
         return v;
#endif
      }
   }
}

static MEM_HANDLE_T alloc_handle(void)
{
   MEM_HANDLE_T handle = g_mgr.free_handle;
   if (handle != MEM_HANDLE_INVALID) {
      g_mgr.free_handle = g_mgr.handles[handle].next;
   }
   else if (g_mgr.num_handles < g_mgr.max_handles) {
      handle = (MEM_HANDLE_T)g_mgr.num_handles++;
   }
   else
	   assert(0);

   return handle;
}

static void free_handle(MEM_HANDLE_T handle)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);
   vcos_assert(!is_small_alloc(handle));
   vcos_assert((uint32_t)handle < g_mgr.num_handles);

   g_mgr.handles[handle].next = g_mgr.free_handle;
   g_mgr.free_handle = handle;
}

static RCM_INLINE MEM_HEADER_T *get_header(MEM_HANDLE_T handle)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle))
   {
      return get_small_alloc_header(handle);
   }
#ifdef BRCM_V3D_OPT
	if (is_big_alloc(handle))
	{
	   return get_big_alloc_header(handle);
	}
#endif
#endif

   vcos_assert((uint32_t)handle < g_mgr.num_handles);
   if ((uint32_t)handle < g_mgr.num_handles)
   {
      vcos_assert(is_aligned_32(g_mgr.handles[handle].header));
      return g_mgr.handles[handle].header;
   }
   else
   {
      return NULL;
   }
}

static RCM_INLINE void set_header(MEM_HANDLE_T handle, MEM_HEADER_T *header)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);
   vcos_assert(!is_small_alloc(handle));
   vcos_assert((uint32_t)handle < g_mgr.num_handles);
   vcos_assert(is_aligned_32(header));

  g_mgr.handles[handle].header = header;
}

static RCM_INLINE void need_icache_flush()
{
   mem_need_icache_flush = 3;   /* both cores may need their icache flushing */
}

static RCM_INLINE void copy_alloc(const MEM_HEADER_T *src, MEM_HEADER_T *dst, uint32_t dstsize)
{
   uint32_t align = 1 << src->x.log2_align;
   uint32_t srcsize = src->size;
   MEM_FLAG_T srcflags = (MEM_FLAG_T)src->x.flags;
   uint32_t invalidate_size;
   void *src_data;
   void *dst_data;

   /*
      copy header across and update handle to point to new header
   */

#if HEADERS_IN_DIRECT_ALIAS
   vclib_dcache_invalidate_range(dst, sizeof(MEM_HEADER_T));
#endif

   memcpy(dst, src, sizeof(MEM_HEADER_T));

   set_header(src->handle, dst);

   dst->size = dstsize;
   src_data = get_data_pointer(src, align, srcflags);
   dst_data = get_data_pointer(dst, align, srcflags);

   // we always invalidate cached data here whatever the alias. Data from before the malloc will not be used again (and dirty data may be harmful)
   invalidate_size = _min(dstsize, ((uint8_t *)src_data - (uint8_t *)dst_data));
   if (invalidate_size)
      vclib_dcache_invalidate_range(dst_data, invalidate_size);

   if (dstsize != srcsize)
   {
      dst->x.size_is_zero = (dstsize == 0);
      if (dstsize > srcsize)
      {
         initialise_memory((uint8_t *)dst_data + srcsize, srcflags, dstsize - srcsize);
         dstsize = srcsize;
      }
   }

   if (dstsize && !(srcflags & MEM_FLAG_ABANDONED))
   {
      /*
         copy data across
      */

      // On videocore, use DMA for large blocks
#ifdef _VIDEOCORE
      memcpy_fast(dst_data, src_data, dstsize);
#else
      memmove(dst_data, src_data, dstsize);
#endif
   }
}

#ifdef _VIDEOCORE
static RCM_INLINE uint32_t current_cpu_bit()
{
   return 1 << rtos_get_cpu_number();
}
#else
static RCM_INLINE uint32_t current_cpu_bit()
{
   return 1;
}
#endif

/******************************************************************************
memory protection using sandbox regions
******************************************************************************/

#if defined( NDEBUG ) || !defined( __BCM2708__ ) || defined(  __BCM2708A0__ ) || ( 0 == MEM_FREESPACE_PROTECT )

#define MEM_FORGET_FREE_REGIONS()           ((void)0)
#define MEM_ADD_FREE_REGION(START, SIZE)    ((void)0)
#define MEM_REMOVE_FREE_REGION(START, SIZE) ((void)0)
#define MEM_CLEAR_SANDBOX_REGIONS()         ((void)0)
#define MEM_SET_SANDBOX_REGIONS()           ((void)0)

#else // DEBUG on 2708B0 or better

#define MEM_FREE_IGNORE_SIZE 96 /* ignore any free regions with size <= this */

typedef struct {
   uint32_t start;
   uint32_t size;
} MEM_REGION_T;

/*
   we maintain a list of the free regions, in order of increasing size, in
   mem_free_regions[mem_free_regions_i:]

   this list may be incomplete, but all missing entries will be off the bottom
   (that is, if there are 3 regions in the list, they will be the 3 largest
   regions)

   mem_free_regions_all indicates whether the list is complete (ignoring regions
   with size <= MEM_FREE_IGNORE_SIZE, which never get added to the list)
*/

#define MEM_FREE_REGIONS_N 10
static MEM_REGION_T mem_free_regions[MEM_FREE_REGIONS_N];
static uint32_t mem_free_regions_i;
static int mem_free_regions_all;

/* clear the list of free regions (calling this does not imply there are no free
 * regions, so mem_free_regions_all is set to 0) */
static void mem_forget_free_regions(void)
{
   mem_free_regions_i = MEM_FREE_REGIONS_N;
   mem_free_regions_all = 0;
}

/* add a free region to the list */
static void mem_add_free_region(void *p, uint32_t size)
{
   uint32_t start = (uint32_t)(uintptr_t)p;
   uint32_t i;

   if (size <= MEM_FREE_IGNORE_SIZE) { return; }

   if (mem_free_regions_i == 0) {
      mem_free_regions_all = 0;
      if (size <= mem_free_regions[0].size) {
         return;
      }
   } else {
      if (!mem_free_regions_all && ((mem_free_regions_i == MEM_FREE_REGIONS_N) ||
         (size < mem_free_regions[mem_free_regions_i].size))) {
         /* we can't add this region -- there might be a region not in the list
          * with a greater size */
         return;
      }
      --mem_free_regions_i;
   }

   for (i = (mem_free_regions_i + 1); (i != MEM_FREE_REGIONS_N) && (size > mem_free_regions[i].size); ++i) {
      mem_free_regions[i - 1] = mem_free_regions[i];
   }

   mem_free_regions[i - 1].start = start;
   mem_free_regions[i - 1].size = size;
}

/* remove a free region from the list (no-op if region isn't in the list) */
static void mem_remove_free_region(void *p, uint32_t size)
{
   uint32_t start = (uint32_t)(uintptr_t)p;
   uint32_t i, j;

   if (size <= MEM_FREE_IGNORE_SIZE) { return; }

   for (i = mem_free_regions_i; (i != MEM_FREE_REGIONS_N) && (size >= mem_free_regions[i].size); ++i) {
      if (start == mem_free_regions[i].start) {
         vcos_assert(size == mem_free_regions[i].size);
         for (j = i; j != mem_free_regions_i; --j) {
            mem_free_regions[j] = mem_free_regions[j - 1];
         }
         ++mem_free_regions_i;
         return;
      }
   }
}

/* add all free regions to the list */
static void mem_add_all_free_regions(void)
{
   MEM_LINK_T *l;

   mem_free_regions_i = MEM_FREE_REGIONS_N;
   mem_free_regions_all = 1;

   for (l = g_mgr.free.next; l != &g_mgr.free; l = l->next) {
      mem_add_free_region(l, l->space);
   }
}

/* m = min(n, number of free regions with size > MEM_FREE_IGNORE_SIZE)
 * return m and, in regions, the m largest free regions */
static uint32_t mem_get_largest_free_regions(MEM_REGION_T *regions, uint32_t n)
{
   uint32_t i;

   if (((MEM_FREE_REGIONS_N - mem_free_regions_i) < n) && !mem_free_regions_all) {
      mem_add_all_free_regions();
   }

   for (i = MEM_FREE_REGIONS_N; (i != mem_free_regions_i) && (n != 0); --i, ++regions, --n) {
      *regions = mem_free_regions[i - 1];
   }

   return MEM_FREE_REGIONS_N - i;
}

static int mem_cmp_region_starts(const void *a, const void *b)
{
   const MEM_REGION_T *region_a = (const MEM_REGION_T *)a, *region_b = (const MEM_REGION_T *)b;
   if (region_a->start < region_b->start) {
      return -1;
   } else if (region_a->start == region_b->start) {
      return 0;
   } else {
      return 1;
   }
}

static void mem_sort_regions_by_start(MEM_REGION_T *regions, uint32_t n)
{
   qsort(regions, n, sizeof(*regions), mem_cmp_region_starts);
}


extern
void sdram_set_sandbox_region( const unsigned              start_addr_flags,   /* in     */
                               const unsigned              end_addr,           /* in     */
                               const unsigned              iregion             /* in     */ );

#define SSSR_PRIV_SECURE  0x18
#define SSSR_PRIV_SUPER   0x08
#define SSSR_PRIV_USER    0x00
#define SSSR_READ         0x04
#define SSSR_WRITE        0x02
#define SSSR_EXECUTE      0x01



/***********************************************************
 * Name: mem_clear_sandbox_regions
 *
 * Arguments:
 *       void
 *
 * Description: Disable all secure regions set by the relocatable heap
 *
 ***********************************************************/

#define MEM_EOM  ( ( 128U << 20U ) - 1 )

static
void mem_clear_sandbox_regions( void )
{
   // set region 0 to allow anyone to do anything, everywhere.
   sdram_set_sandbox_region( 0x0 | SSSR_PRIV_USER | SSSR_READ | SSSR_WRITE | SSSR_EXECUTE, MEM_EOM, 0 );

} /* mem_clear_sandbox_regions */



/***********************************************************
 * Name: mem_set_sandbox_regions
 *
 * Arguments:
 *       void
 *
 * Description: Set the H/W secure regions to cover the largest
 *              free blocks in the relocatable heap
 *
 ***********************************************************/

static
void mem_set_sandbox_regions( void )
{
   /* Call with the mutex held */

   //
   // If we are to allow reads we can only defend 4 free regions.
   //
#if ( 1 == MEM_FREESPACE_PROTECT )
#define MSSR_NFR  4
#else
#define MSSR_NFR  5
#endif

   // get the largest free regions
   MEM_REGION_T regions[MSSR_NFR];
   uint32_t n = mem_get_largest_free_regions(regions, MSSR_NFR);

   // sort them by start address
   mem_sort_regions_by_start(regions, n);

   // set the permitted sandbox regions around the free regions
   {
      unsigned start = (unsigned)RTOS_ALIAS_NORMAL(g_mgr.base);
      unsigned end = start + g_mgr.space;
      vcos_assert(!(start & 0x1f));
      vcos_assert(!(end & 0x1f));

      unsigned cur = start;
      unsigned iregion = 2; // regions 0 & 1 reserved for .text & dmalloc arena, and are set at the end
      unsigned i;

      //
      // allow reads into free space?
      //
#if ( 1 == MEM_FREESPACE_PROTECT )
      sdram_set_sandbox_region( start | SSSR_PRIV_SUPER | SSSR_READ, end - 1, iregion++ );
#endif

      for( i = 0; i < n; i++ )
      {
         unsigned r = (unsigned)RTOS_ALIAS_NORMAL(regions[i].start);
         vcos_assert(!(r & 0x1f));

         if (cur != r) {
            sdram_set_sandbox_region( cur | SSSR_PRIV_SUPER | SSSR_READ | SSSR_WRITE, r - 1, iregion++ );
         }

         cur = r + regions[i].size;
         vcos_assert(!(cur & 0x1f));
      } // for

      if (cur != end) {
         sdram_set_sandbox_region( cur | SSSR_PRIV_SUPER | SSSR_READ | SSSR_WRITE, end - 1, iregion++ );
      }

      // ensure there are no spuriously permitted accesses
      while( 8 > iregion )
      {
         sdram_set_sandbox_region( SSSR_PRIV_SECURE, 31, iregion++ ); // only SECURE code is permitted to do NOTHING with the first 32 bytes of memory
      } // while

      // finally restrict  region 1 to the dmalloc arena (executable because of .vlls)...
      sdram_set_sandbox_region( end | SSSR_PRIV_SUPER | SSSR_READ | SSSR_WRITE | SSSR_EXECUTE, MEM_EOM, 1 );
      // ...and region 0 to only allow SUPERVISOR complete access to .text
      sdram_set_sandbox_region( 0x0 | SSSR_PRIV_SUPER | SSSR_READ | SSSR_WRITE | SSSR_EXECUTE, start - 1, 0 );
   } // block

} /* mem_set_sandbox_regions */

#define MEM_FORGET_FREE_REGIONS   mem_forget_free_regions
#define MEM_ADD_FREE_REGION       mem_add_free_region
#define MEM_REMOVE_FREE_REGION    mem_remove_free_region
#define MEM_CLEAR_SANDBOX_REGIONS mem_clear_sandbox_regions
#define MEM_SET_SANDBOX_REGIONS   mem_set_sandbox_regions

#endif /* ifndef NDEBUG */

/******************************************************************************
internal
******************************************************************************/

static void add_link(MEM_LINK_T *link, MEM_LINK_T *before, uint32_t space)
{
   vcos_assert(!(space & 0x1f));

#if HEADERS_IN_DIRECT_ALIAS
   vclib_dcache_invalidate_range(link, sizeof(MEM_HEADER_T));
#endif

   link->space = space;

   link->next = before;
   link->prev = before->prev;
   before->prev = link;
   link->prev->next = link;
}

static void remove_link(MEM_LINK_T *link)
{
   vcos_assert(link->next->prev == link);
   vcos_assert(link->prev->next == link);
   link->next->prev = link->prev;
   link->prev->next = link->next;
}

static void split_link(MEM_LINK_T *link, uint32_t space)
{
   MEM_LINK_T *remainder;

   vcos_assert(!(link->space & 0x1f));
   vcos_assert(!(space & 0x1f));
   vcos_assert(space > 0);
   vcos_assert(space < link->space);

   remainder = (MEM_LINK_T *)((uint8_t *)link + space);
#if HEADERS_IN_DIRECT_ALIAS
   vclib_dcache_invalidate_range(remainder, sizeof(MEM_HEADER_T));
#endif
   remainder->space = link->space - space;
   link->space = space;

   remainder->prev = link;
   remainder->next = link->next;
   vcos_assert(link->next->prev == link);
   link->next->prev = remainder;
   link->next = remainder;
}

/* Mark the block as free space, coalescing with neighbours as necessary.
 * Returns a pointer to the next allocated block, or the end marker if none. */
static MEM_HEADER_T *add_free(void *ptr, uint32_t space)
{
   MEM_LINK_T *new_free = (MEM_LINK_T *)ptr;
   MEM_LINK_T *end = (MEM_LINK_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   MEM_LINK_T *next_free = (MEM_LINK_T *)((uint8_t *)ptr + space);
   MEM_LINK_T *prev_free;

   if ((next_free != end) && (!(next_free->space & 1)))
   {
      /* The next block is free - what about the previous one? */
      prev_free = next_free->prev;
      if ((MEM_LINK_T *)((uint8_t *)prev_free + (prev_free->space & ~1)) == new_free)
      {
         /* Combine all three blocks into one */

         MEM_REMOVE_FREE_REGION(prev_free, prev_free->space);
         MEM_REMOVE_FREE_REGION(next_free, next_free->space);
         MEM_ADD_FREE_REGION(prev_free, prev_free->space + space + next_free->space);

         space += prev_free->space + next_free->space;
#if HEADERS_IN_DIRECT_ALIAS
         vclib_dcache_invalidate_range(prev_free, space);
#endif
         prev_free->space = space;
         prev_free->next = next_free->next;
         next_free->next->prev = prev_free;
#ifdef MEM_FILL_FREE_SPACE
         memset((MEM_HEADER_T *)prev_free + 1, 0xff, space - 32);
#endif
         return (MEM_HEADER_T *)((uint8_t *)prev_free + space);
      }
      else
      {
         /* Replace next_free with a larger block including the new space */

         MEM_REMOVE_FREE_REGION(next_free, next_free->space);
         MEM_ADD_FREE_REGION(new_free, space + next_free->space);

         space += next_free->space;
#if HEADERS_IN_DIRECT_ALIAS
         vclib_dcache_invalidate_range(new_free, space);
#endif
         new_free->space = space;
         new_free->prev = next_free->prev;
         new_free->prev->next = new_free;
         new_free->next = next_free->next;
         new_free->next->prev = new_free;
#ifdef MEM_FILL_FREE_SPACE
         memset((MEM_HEADER_T *)new_free + 1, 0xff, space - 32);
#endif
         return (MEM_HEADER_T *)((uint8_t *)new_free + space);
      }
   }

   /* The next block isn't free */
   while (1)
   {
      if (next_free == end)
      {
         next_free = &g_mgr.free;
         break;
      }

      if ((next_free->space & 1) == 0)
         break;

      next_free = (MEM_LINK_T *)((uint8_t *)next_free + (next_free->space & ~1));
   }

   /* Is the previous block free? */
   prev_free = next_free->prev;
   if ((MEM_LINK_T *)((uint8_t *)prev_free + (prev_free->space & ~1)) == new_free)
   {
      /* Extend the previous free block to include the new space */

      MEM_REMOVE_FREE_REGION(prev_free, prev_free->space);
      MEM_ADD_FREE_REGION(prev_free, prev_free->space + space);

      space += prev_free->space;
#if HEADERS_IN_DIRECT_ALIAS
      vclib_dcache_invalidate_range(prev_free, space);
#endif
      prev_free->space = space;
#ifdef MEM_FILL_FREE_SPACE
      memset((MEM_HEADER_T *)prev_free + 1, 0xff, space - 32);
#endif
      return (MEM_HEADER_T *)((uint8_t *)prev_free + space);
   }
   else
   {
      /* This free block is isolated */

      MEM_ADD_FREE_REGION(new_free, space);

#if HEADERS_IN_DIRECT_ALIAS
      vclib_dcache_invalidate_range(new_free, space);
#endif
      add_link(new_free, next_free, space);
#ifdef MEM_FILL_FREE_SPACE
      memset((MEM_HEADER_T *)new_free + 1, 0xff, space - 32);
#endif
      return (MEM_HEADER_T *)((uint8_t *)new_free + space);
   }
}

/* Remove space from the beginning of a free block. space must be <= the size of
 * the free block */
static void trim_free(MEM_LINK_T *free, uint32_t space)
{
   vcos_assert(!(free->space & 1));

   MEM_REMOVE_FREE_REGION(free, free->space);

   if (space < free->space) {
      MEM_LINK_T *new_free = (MEM_LINK_T *)((uint8_t *)free + space);
#if HEADERS_IN_DIRECT_ALIAS
      vclib_dcache_invalidate_range(new_free, sizeof(MEM_HEADER_T));
#endif
      new_free->space = free->space - space;

      MEM_ADD_FREE_REGION(new_free, new_free->space);

      new_free->prev = free->prev;
      new_free->prev->next = new_free;
      new_free->next = free->next;
      new_free->next->prev = new_free;
      return;
   }

   vcos_assert(space == free->space);
   remove_link(free);
}

static MEM_HEADER_T *alloc_internal(uint32_t space)
{
   MEM_LINK_T *curr;

   vcos_assert((space & 0x1f) == 0);

   for (curr = g_mgr.free.next; curr->space; curr = curr->next) {
      if (curr->space > space) {
         /*
            Inexact fit, so split the block into an exact fit and a
            remainder
         */

         MEM_REMOVE_FREE_REGION(curr, curr->space);
         MEM_ADD_FREE_REGION(curr, space);
         MEM_ADD_FREE_REGION((uint8_t *)curr + space, curr->space - space);

         split_link(curr, space);
      }

      if (curr->space == space) {
         /*
            Exact fit, either because we got lucky or because we made
            ourselves an exact fit block above
         */

         MEM_REMOVE_FREE_REGION(curr, curr->space);

         remove_link(curr);
         curr->space |= 1;
         return (MEM_HEADER_T *)curr;
      }
   }

   return NULL;
}

static void free_internal(MEM_LINK_T *cur)
{
   MEM_LINK_T *end, *next, *prev;
   uint32_t space = cur->space & ~1;

   end = (MEM_LINK_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   next = (MEM_LINK_T *)((uint8_t *)cur + space);

   vcos_assert(cur != (MEM_LINK_T *)g_mgr.aggressive_locked);

   if (next == end)
   {
      /* At the end of the list */
      next = &g_mgr.free;
   }
   else if ((next->space & 1) == 0)
   {
      /* The neighbour is free - coalesce */
      MEM_LINK_T* neighbour;

      MEM_REMOVE_FREE_REGION(next, next->space);

      neighbour = next;
      space += neighbour->space;
      next = next->next;
      remove_link(neighbour);
   }
   else
   {
      /* Scan for the next free block. If this scheme is working well then there
         should be fewer free blocks than allocated blocks, so scan the free
         chain for the right insertion point. */
      if (cur < (MEM_LINK_T *)((uint8_t *)g_mgr.base + (g_mgr.space>>1)))
      {
         /* Below the halfway point - scan forwards */
         for (next = g_mgr.free.next;
             next != &g_mgr.free && next < cur;
             next = next->next)
            continue;
      }
      else
      {
         /* Scan backwards */
         for (next = &g_mgr.free;
              next->prev != &g_mgr.free && next->prev > cur;
              next = next->prev)
            continue;
      }
   }

   prev = next->prev;

   /* If cur is the first block then prev == &g_mgr.free, and g_mgr.free.space == 0,
      so the following test can never be true. */
   if ((MEM_LINK_T *)((uint8_t *)prev + prev->space) == cur)
   {
      /* The free block is adjacent - coalesce */

      MEM_REMOVE_FREE_REGION(prev, prev->space);
      MEM_ADD_FREE_REGION(prev, prev->space + space);

      space += prev->space;
#if HEADERS_IN_DIRECT_ALIAS
      vclib_dcache_invalidate_range(prev, prev->space);
#endif
      prev->space = space;
#ifdef MEM_FILL_FREE_SPACE
      memset((MEM_HEADER_T *)prev + 1, 0xff, space - 32);
#endif
   }
   else
   {
      MEM_ADD_FREE_REGION(cur, space);

#if HEADERS_IN_DIRECT_ALIAS
      vclib_dcache_invalidate_range(cur, space);
#endif
      add_link(cur, next, space);
#ifdef MEM_FILL_FREE_SPACE
      memset((MEM_HEADER_T *)cur + 1, 0xff, space - 32);
#endif
   }
}

void mem_validate_internal(int full)
{
   /* Call with the mutex held */
   MEM_HEADER_T *end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   MEM_HEADER_T *src = (MEM_HEADER_T *)g_mgr.base;
   MEM_LINK_T *last_free = &g_mgr.free;

   while (src != end)
   {
      MEM_HEADER_T *next;
      uint32_t space = src->space & ~1;

      vcos_assert((space & 0x1f) == 0);

      next = (MEM_HEADER_T *)((uint8_t *)src + space);
      vcos_assert((next > src) && (next <= end));
      if (src->space & 1)
      {
         /* Allocated */
         vcos_assert((space == calc_space(src->size, (1 << src->x.log2_align))) ||
            ((space == 32) && (src->size == 0)));
         vcos_assert(src->handle != MEM_HANDLE_INVALID);
         vcos_assert(!is_small_alloc(src->handle));
         vcos_assert((uint32_t)src->handle < g_mgr.num_handles);
      }
      else
      {
         /* Free */
         MEM_LINK_T *next_free = ((MEM_LINK_T *)src)->next;
         vcos_assert(((MEM_LINK_T *)src)->prev == last_free);
         vcos_assert((next_free == &g_mgr.free) || (((MEM_HEADER_T *)next_free > src) && ((MEM_HEADER_T *)next_free < end)));
         last_free = (MEM_LINK_T *)src;
#ifdef MEM_FILL_FREE_SPACE
         if (full)
         {
            const uint32_t *p = (const uint32_t *)(src + 1);
            int count = (src->space - 32)>>2;
            while (count--)
            {
               vcos_assert(*(p++) == ~0);
            }
         }
#else
         (void)full;
#endif
      }
      src = next;
   }
}

static mem_compact_mode_t need_compact_mode(uint32_t required_space, int last, mem_compact_mode_t mode);

static void compact_internal(mem_compact_mode_t mode, uint32_t required_space, int last)
{
   MEM_HEADER_T *end;
   MEM_HEADER_T *src;
   MEM_HEADER_T *dst;
   MEM_HEADER_T *copy_base;
   int shuffle = (mode & MEM_COMPACT_SHUFFLE);
   int xxxshuffle = shuffle;
   int in_aggressive = 0;
return;
#ifdef MEM_AUTO_VALIDATE
   mem_validate_internal(1);
#endif

   mode = (mem_compact_mode_t)(mode & MEM_COMPACT_ALL);

   g_mgr.stats.compactions++;

   /* no discard + no free space = no way */
   if (((mode & MEM_COMPACT_DISCARD) == 0) && (g_mgr.free.next == &g_mgr.free))
      return;

   if (mode & MEM_COMPACT_AGGRESSIVE)
   {
      if (g_mgr.aggressive_locked)
      {
         g_mgr.busy_latch_count++;
         MEM_SET_SANDBOX_REGIONS();
         vcos_quickslow_mutex_unlock(&g_mgr.mutex);
         vcos_event_wait(&g_mgr.aggressive_busy_latch);
         vcos_quickslow_mutex_lock(&g_mgr.mutex);
         MEM_CLEAR_SANDBOX_REGIONS();
         mode = (mem_compact_mode_t)(mode & ~MEM_COMPACT_AGGRESSIVE);
      }
      else
      {
         int i;
         in_aggressive = 1;
         g_mgr.aggressive_locked = (MEM_HEADER_T *)1; /* Note that a compaction is in progress */
         g_mgr.stats.aggressive_compactions++;
#ifdef AGGRESSIVE_COMPACTION_LED
         if (g_led_driver)
            g_led_driver->led_set(g_led_handle, AGGRESSIVE_COMPACTION_LED);
#endif
         if (g_mgr.callbacks_count)
         {
            MEM_SET_SANDBOX_REGIONS();
            vcos_quickslow_mutex_unlock(&g_mgr.mutex);
            for (i = 0; i < g_mgr.callbacks_count; i++)
            {
               (*g_mgr.callbacks[i].func)(MEM_CALLBACK_REASON_UNLOCK, g_mgr.callbacks[i].context);
            }
            vcos_quickslow_mutex_lock(&g_mgr.mutex);
            MEM_CLEAR_SANDBOX_REGIONS();

            // To avoid unnecessary delays, reassess the state of the heap after the unlocking
            if (required_space != 0)
               mode = need_compact_mode(required_space, last, mode);
         }
      }
   }

restart_compaction:
   end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   copy_base = end;

   if (mode != MEM_COMPACT_NONE)
   {
      if (mode & MEM_COMPACT_DISCARD)
      {
         dst = (MEM_HEADER_T *)g_mgr.base;
         src = dst;
         g_mgr.stats.discard_compactions++;
      }
      else
      {
         dst = (MEM_HEADER_T *)g_mgr.free.next;
         src = (MEM_HEADER_T *)((uint8_t *)dst + (dst->space & ~1));
      }

      while (src < end)
      {
         uint32_t space = src->space & ~1;
         vcos_assert((dst == src) || ((dst < src) && !(dst->space & 1)));

         if (src->space & 1)
         {
            /* allocated */
            if (src->x.lock_count > 0)
            {
               /* locked */
               if ((src->x.lock_count < MEM_LOCK_PERMA_THRESHOLD) &&
                   (mode & MEM_COMPACT_AGGRESSIVE) &&
                   !(src->x.flags & MEM_FLAG_HINT_PERMALOCK) &&
                   ((src != dst) ||
                    ((mode & MEM_COMPACT_DISCARD) && (src->x.flags & MEM_FLAG_DISCARDABLE) &&
                     (src->retain_count == 0) && (space != sizeof(MEM_HEADER_T)))))
               {
                  g_mgr.aggressive_locked = src;
                  if (copy_base != end || xxxshuffle)
                  {
                     need_icache_flush();
                     vclib_dcache_flush_range(copy_base, (uint8_t *)src - (uint8_t *)copy_base);
                     copy_base = end;
                  }

#ifdef MEM_AUTO_VALIDATE
                  mem_validate_internal(0);
#endif
                  g_mgr.stats.aggressive_compaction_waits++;
#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
                  vcos_timer_set(&g_aggressive_timeout_timer, AGGRESSIVE_COMPACTION_TIMEOUT);
#endif
                  MEM_SET_SANDBOX_REGIONS();
                  vcos_quickslow_mutex_unlock(&g_mgr.mutex);
                  vcos_event_wait(&g_mgr.aggressive_owner_latch);
                  MEM_CLEAR_SANDBOX_REGIONS();
#ifdef MEM_AUTO_VALIDATE
                  mem_validate_internal(0);
#endif

                  /* At this point it is guaranteed that the mutex is already claimed,
                     because the thread that puts the latch doesn't release it.
                   */

#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
                  vcos_timer_cancel(&g_aggressive_timeout_timer);
#endif
                  vcos_assert((src->x.lock_count == 0) == (g_mgr.aggressive_locked == (MEM_HEADER_T *)1));

                  /* While the mutex was released, many things could have changed,
                     so it is necessary to reestablish a few facts. However, the block just
                     unlocked (or not, in the case of a timeout) cannot have moved, so use
                     it to find a restart point. */
                  end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);
                  copy_base = end;

                  if (src->x.lock_count != 0)
                  {
                     /* Still locked */
                     g_mgr.stats.aggressive_compaction_timeouts++;
#ifdef ASSERT_ON_COMPACT_TIMEOUT
                     assert(!"aggressive compaction timed out");
#endif
                     /* Skip this block */
                     src = (MEM_HEADER_T *)((uint8_t *)src + space);
                  }

                  /* To avoid unnecessary delays, reassess the state of the heap after each wait */
                  if (required_space != 0)
                     mode = need_compact_mode(required_space, last, mode);

                  if (mode == MEM_COMPACT_NONE)
                  {
                     /* Give up */
                     src = end;
                     break;
                  }

                  /* Skip this block */
                  dst = (MEM_HEADER_T *)g_mgr.free.next;

                  if (mode & MEM_COMPACT_DISCARD)
                  {
                     if ((dst > src) || (dst->space == 0))
                        dst = src;
                  }
                  else if (dst->space)
                  {
                     if (src < dst)
                        src = (MEM_HEADER_T *)((uint8_t *)dst + dst->space);
                  }
                  else
                  {
                     src = end;
                  }
                  continue;
               }

               /* immobile - skip this block */
               if (dst != src)
               {
                  /* Find the first free space - there must be one */
                  dst = (MEM_HEADER_T *)g_mgr.free.next;
               }
               else
               {
                  /* This must be discard mode, so resume at the next block */
                  dst = (MEM_HEADER_T *)((uint8_t *)dst + space);
               }
               src = (MEM_HEADER_T *)((uint8_t *)src + space);
               continue;
            }

            /* Not locked */

            if ((mode & MEM_COMPACT_DISCARD) && /* discarding */
                /* discardable, unlocked (above), and unretained */
                (src->x.flags & MEM_FLAG_DISCARDABLE) &&
                (src->retain_count == 0) &&
                /* not already discarded */
                (space != sizeof(MEM_HEADER_T)))
            {
               /* Replace with a zero-sized block */
               if (dst != src)
               {
                  /* Guaranteed to fit, because dst points to a space and
                     the smallest space is enough for a header */
                  trim_free((MEM_LINK_T *)dst, sizeof(MEM_HEADER_T));
                  *dst = *src;
                  set_header(dst->handle, dst);

                  /* Free the source, and find the next non-free block */
                  src = add_free(src, src->space & ~1);
               }
               else
               {
                  /* Free the remaining space, and find the next non-free block */
                  src = add_free(dst + 1, space - sizeof(MEM_HEADER_T));
               }
               dst->space = sizeof(MEM_HEADER_T) | 0x1;
               dst->size = 0;
               dst->x.size_is_zero = 1;
               dst = (MEM_HEADER_T *)g_mgr.free.next;
            }
            else if (dst != src)
            {
               /* Move the whole block */
               /* dst is guaranteed to be a space */
               while (1)
               {
                  if (src == (MEM_HEADER_T *)((uint8_t *)dst + dst->space))
                  {
                     /* The two blocks are adjacent, so the copy may be overlapping,
                      * but the move is guaranteed to succeed. */
                     uint32_t dst_space;

                     MEM_REMOVE_FREE_REGION(dst, dst->space);

					 dst_space = dst->space;
                     remove_link((MEM_LINK_T *)dst);
                     copy_alloc(src, dst, src->size);
                     if (dst < copy_base)
                        copy_base = dst;

                     /* Free the source, and find the next non-free block. */
                     src = add_free((uint8_t *)dst + space, dst_space);
                     break;
                  }
                  /* The two blocks are non-adjacent, so the move can fail if
                   * dst isn't big enough. */
                  if (space <= dst->space)
                  {
                     /* Move the block */
                     trim_free((MEM_LINK_T *)dst, space);
                     copy_alloc(src, dst, src->size);
                     if (dst < copy_base)
                        copy_base = dst;

                     /* Free the source, and find the next non-free block. */
                     src = add_free(src, space);
                     break;
                  }
                  else
                  {
                     /* Try the next free block */
                     dst = (MEM_HEADER_T *)((MEM_LINK_T *)dst)->next;
                     if ((dst > src) || (dst->space == 0))
                     {
                        /* There is no space - move on to the next block */
                        src = (MEM_HEADER_T *)((uint8_t *)src + space);
                        break;
                     }
                  }
               } /* while (1) */

               /* End of the search - reset the destination */
               dst = (MEM_HEADER_T *)g_mgr.free.next;

               if (mode & MEM_COMPACT_DISCARD)
               {
                  if ((dst > src) || (dst->space == 0))
                     dst = src;
               }
               else if (dst->space)
               {
                  if (src < dst)
                     src = (MEM_HEADER_T *)((uint8_t *)dst + dst->space);
               }
               else
               {
                  src = end;
               }
            }
            else /* if (dst != src) */
            {
               /* Move on to the next block */
               src = (MEM_HEADER_T *)((uint8_t *)src + space);
               dst = src;
            }
         }
         else /* if (src->space & 1) */
         {
            /* Move on to the next block */
            src = (MEM_HEADER_T *)((uint8_t *)src + space);
         }
      }

      vcos_assert(src == end);
   }

   if (shuffle)
   {
      /* Move the bottom-most movable block up to the top */
      src = (MEM_HEADER_T *)g_mgr.base;
      dst = (MEM_HEADER_T *)g_mgr.free.prev;

      while (src < end)
      {
         uint32_t space = src->space;

         if (space & 1)
         {
            space &= ~1;

            if ((src->x.lock_count == 0) &&
                (space <= dst->space) &&
                ((MEM_HEADER_T *)((size_t)src + space) != dst)) /* Don't shuffle the last block */
            {
               if (dst->space > space) {
                  MEM_REMOVE_FREE_REGION(dst, dst->space);
                  MEM_ADD_FREE_REGION(dst, space);
                  MEM_ADD_FREE_REGION((uint8_t *)dst + space, dst->space - space);

                  split_link((MEM_LINK_T *)dst, space);
               }
               if (dst->space == space) {
                  MEM_REMOVE_FREE_REGION(dst, dst->space);

                  remove_link((MEM_LINK_T *)dst);
               }

               /* Copy this block into the free space */
               copy_alloc(src, dst, src->size);

               free_internal((MEM_LINK_T *)src);
               if (dst < copy_base)
                  copy_base = dst;
               shuffle = 0;
               break;
            }
         }
         src = (MEM_HEADER_T *)((size_t)src + space);
      }

      /* If a block has been moved, shuffle everything down to fill the gap.
         This gives less scope for showing problems, but a compaction which
         ends with an unnecessarily fragmented heap is not a proper
         compaction. */
      if (!shuffle)
         goto restart_compaction;
   }

   if (copy_base != end || xxxshuffle)
   {
      need_icache_flush();
      vclib_dcache_flush_range(copy_base, (uint8_t *)end - (uint8_t *)copy_base);
   }

   if (in_aggressive)
   {
      int i;
      vcos_assert(g_mgr.aggressive_locked);
      g_mgr.aggressive_locked = (MEM_HEADER_T *)1;
      if (g_mgr.callbacks_count)
      {
         MEM_SET_SANDBOX_REGIONS();
         vcos_quickslow_mutex_unlock(&g_mgr.mutex);
         for (i = 0; i < g_mgr.callbacks_count; i++)
         {
            (*g_mgr.callbacks[i].func)(MEM_CALLBACK_REASON_RELOCK, g_mgr.callbacks[i].context);
         }
         vcos_quickslow_mutex_lock(&g_mgr.mutex);
         MEM_CLEAR_SANDBOX_REGIONS();
      }

      while (g_mgr.busy_latch_count > 0)
      {
         g_mgr.busy_latch_count--;
         vcos_event_signal(&g_mgr.aggressive_busy_latch);
      }

#ifdef AGGRESSIVE_COMPACTION_LED
      if (g_led_driver)
         g_led_driver->led_clear(g_led_handle, AGGRESSIVE_COMPACTION_LED);
#endif

      g_mgr.aggressive_locked = NULL;
   }

#ifdef MEM_AUTO_VALIDATE
   mem_validate_internal(0);
#endif
}

#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
static void compact_timeout(void *arg)
{
   if ((uint32_t)g_mgr.aggressive_locked & ~1)
   {
      g_aggressive_locked_latched = g_mgr.aggressive_locked;
      vcos_event_signal(&g_aggressive_timeout_latch);
   }
}

static void *compact_timeout_task(void *arg)
{
   while (1)
   {
      vcos_event_wait(&g_aggressive_timeout_latch);
      vcos_quickslow_mutex_lock(&g_mgr.mutex);
      if (g_aggressive_locked_latched == g_mgr.aggressive_locked)
         vcos_event_signal(&g_mgr.aggressive_owner_latch);
      else
         vcos_quickslow_mutex_unlock(&g_mgr.mutex);
   }
   return 0;
}
#endif

/*
   What form of compaction is required in order for there to be at least
   required_space available afterwards? Returns a bitmask of mem_compact_mode_t
   values, and MEM_COMPACT_NONE if there really isn't enough memory, the rationale
   being that this is only used after a non-compacting allocate has failed so there
   should be no free blocks of the right size.
   Note that the result is optimistic - for example, it takes no account of
   permalocked blocks - hence the "at least" in the first sentence above.
*/

static mem_compact_mode_t need_compact_mode(uint32_t required_space, int last, mem_compact_mode_t mode)
{
   MEM_HEADER_T *h = (MEM_HEADER_T *)g_mgr.base;
   MEM_HEADER_T *end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);

   uint32_t free_space = 0;         /* free space since last locked block */
   uint32_t total_free_space = 0;   /* total free space */
   uint32_t discardable = 0;        /* discardable since last locked block */
   uint32_t total_discardable = 0;  /* total discardable */
   uint32_t max_moveable = 0;       /* largest free block after discarding */

   if (mode == MEM_COMPACT_NONE)
      return MEM_COMPACT_NONE;

   while (h < end) {
      uint32_t space = h->space;

      if (space & 0x1) {
         space &= ~0x1;
         if (h->x.lock_count != 0) {
            total_free_space += free_space;
            total_discardable += discardable;
            if ((discardable + free_space) > max_moveable)
               max_moveable = discardable + free_space;
            discardable = 0;
            free_space = 0;
            if ((h->x.flags & MEM_FLAG_DISCARDABLE) && (h->retain_count == 0))
               total_discardable += space - sizeof(MEM_HEADER_T);
         }
         else if ((h->x.flags & MEM_FLAG_DISCARDABLE) && (h->retain_count == 0))
         {
            discardable += space - sizeof(MEM_HEADER_T);
         }
      } else {
         free_space += space;
         if ((free_space >= required_space) && !last)
            return MEM_COMPACT_NORMAL;
      }

      h = (MEM_HEADER_T *)((uint8_t *)h + space);
   }

   vcos_assert(h == end);

   total_free_space += free_space;
   total_discardable += discardable;
   if ((discardable + free_space) > max_moveable)
      max_moveable = discardable + free_space;

   /* Rank results in order of severity, considering aggressive compaction
      to be worse than discarding. */
   if (last)
   {
      if (free_space >= required_space)
         return MEM_COMPACT_NORMAL;
      if (((int)mode & MEM_COMPACT_DISCARD) && ((free_space + discardable) >= required_space))
         return MEM_COMPACT_DISCARD;
   }
   else
   {
      if (((int)mode & MEM_COMPACT_DISCARD) && (max_moveable >= required_space))
         return MEM_COMPACT_DISCARD;
   }
   if (((int)mode & MEM_COMPACT_AGGRESSIVE) && (total_free_space >= required_space))
      return MEM_COMPACT_AGGRESSIVE;
   if ((((int)mode & (MEM_COMPACT_DISCARD | MEM_COMPACT_AGGRESSIVE)) == (MEM_COMPACT_DISCARD | MEM_COMPACT_AGGRESSIVE)) &&
      ((total_free_space + total_discardable) >= required_space))
      return MEM_COMPACT_ALL;

   return MEM_COMPACT_NONE;
}

static MEM_HEADER_T *alloc_compact_internal(uint32_t space, mem_compact_mode_t mode)
{
   MEM_HEADER_T *h = alloc_internal(space);

   if (!h)
   {
      mem_compact_mode_t need_mode = need_compact_mode(space, 0, mode);
      if (need_mode != MEM_COMPACT_NONE)
      {
         compact_internal(need_mode, space, 0);

         h = alloc_internal(space);
      }
   }

   return h;
}

/*
   try to resize without moving
*/

static int inplace_resize_internal(MEM_HANDLE_T handle, uint32_t size)
{
   MEM_HEADER_T *h = get_header(handle);
   MEM_LINK_T *next_free = g_mgr.free.next;
   uint32_t space = h->space & ~0x1;
   uint32_t align = 1 << h->x.log2_align;
   uint32_t new_space = calc_space(size, align);

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      /* In place resize not supported for small allocs */
      return 0;
   }
#ifdef BRCM_V3D_OPT
	if (is_big_alloc(handle)) {
	   /* In place resize not supported for small allocs */
	   return 0;
	}
#endif
#endif

   /*
      get the right amount of space
   */

   if (new_space > space) {
      /*
         check we won't overflow the pool if we try to grow in-place
      */

      if ((((uint8_t *)h - (uint8_t *)g_mgr.base) + new_space) > g_mgr.space) {
         return 0;
      }

      /*
         try to claim the area we want to grow into
      */

      {
         MEM_LINK_T *free_head = NULL, *free_tail = NULL; /* list of free spaces we have claimed */
         MEM_LINK_T *alloc_head = NULL, *alloc_tail = NULL; /* list of new spaces we have allocated for displaced buffers */

         uint32_t space_left;
         MEM_LINK_T *curr;

         /*
            1st pass over area we want to claim:
            - fail if we find any buffers larger than our new size (it will be
              less work to do a relocate resize in this case).
            - fail if we find any locked buffers.
            - claim any free space so when we try to alloc_internal new places
              for the displaced buffers, we don't do something stupid.
         */

         space_left = new_space - space;
         curr = (MEM_LINK_T *)((uint8_t *)h + space);
         do {
            uint32_t curr_space = curr->space;

            if (curr_space & 0x1) {
               curr_space &= ~0x1;

               if ((curr_space >= new_space) || (((MEM_HEADER_T *)curr)->x.lock_count > 0)) {
                  break;
               }

               if (curr_space > space_left) {
                  curr_space = space_left;
               }
            } else {
               MEM_REMOVE_FREE_REGION(curr, curr->space);

               if (curr_space > space_left) {
                  MEM_ADD_FREE_REGION((uint8_t *)curr + space_left, curr->space - space_left);

                  split_link(curr, space_left);
                  curr_space = space_left;
               }

               /*
                  remove from the free list so it won't be used when we call
                  alloc_internal later, but remember it so we can return it to
                  the free list if we fail
               */

               next_free = curr->next;
               remove_link(curr);

               curr->next = NULL;
               curr->prev = free_tail;
               if (free_tail) {
                  vcos_assert(free_tail->next == NULL);
                  free_tail->next = curr;
               } else {
                  vcos_assert(free_head == NULL);
                  free_head = curr;
               }
               free_tail = curr;
            }

            vcos_assert(curr_space <= space_left);
            space_left -= curr_space;
            curr = (MEM_LINK_T *)((uint8_t *)curr + curr_space);
         } while (space_left != 0);

         /*
            2nd pass over area we want to claim (only if 1st pass succeeded):
            - alloc_internal new places for the buffers to live.
         */

         if (space_left == 0) {
            space_left = new_space - space;
            curr = (MEM_LINK_T *)((uint8_t *)h + space);
            do {
               uint32_t curr_space = curr->space;

               if (curr_space & 0x1) {
                  MEM_LINK_T *new_curr;

                  curr_space &= ~0x1;

                  new_curr = (MEM_LINK_T *)alloc_internal(curr_space);
                  if (!new_curr)
                     break;

                  new_curr->next = NULL;
                  if (alloc_tail) {
                     vcos_assert(alloc_tail->next == NULL);
                     alloc_tail->next = new_curr;
                  } else {
                     vcos_assert(alloc_head == NULL);
                     alloc_head = new_curr;
                  }
                  alloc_tail = new_curr;

                  if (curr_space > space_left) {
                     curr_space = space_left;
                  }
               }

               vcos_assert(curr_space <= space_left);
               space_left -= curr_space;
               curr = (MEM_LINK_T *)((uint8_t *)curr + curr_space);
            } while (space_left != 0);
         }

         /*
            handle failure of 1st/2nd passes:
            - release and free space that we claimed.
            - release the new places we allocated.
         */

         if (space_left != 0) {
            while (alloc_head) {
               MEM_LINK_T *alloc_next = alloc_head->next;
               free_internal(alloc_head);
               alloc_head = alloc_next;
            }

            if (free_head) {
               /* Find the insertion point for this subchain */
               free_head->prev = next_free->prev;
               free_head->prev->next = free_head;
               free_tail->next = next_free;
               next_free->prev = free_tail;
               if ((MEM_LINK_T *)((uint8_t *)free_tail + free_tail->space) == next_free)
               {
                  /* Undo the split_link above */

                  MEM_REMOVE_FREE_REGION(next_free, next_free->space);

                  free_tail->space += next_free->space;
                  remove_link(next_free);
#ifdef MEM_FILL_FREE_SPACE
                  memset(next_free, 0xff, sizeof(MEM_HEADER_T));
#endif
               }

               for (;;) {
                  MEM_ADD_FREE_REGION(free_head, free_head->space);
                  if (free_head == free_tail) { break; }
                  free_head = free_head->next;
               }
            }

            return 0;
         }

         /*
            3rd pass over area we want to claim:
            - move buffers to their new locations.
         */

         space_left = new_space - space;
         curr = (MEM_LINK_T *)((uint8_t *)h + space);
         do {
            uint32_t curr_space = curr->space;

            if (curr_space & 0x1) {
               MEM_HEADER_T *src = (MEM_HEADER_T *)curr;
               MEM_HEADER_T *dst = (MEM_HEADER_T *)alloc_head;

               curr_space &= ~0x1;

               alloc_head = alloc_head->next;

               vcos_assert(dst->space == src->space);

               copy_alloc(src, dst, src->size);

               if (src->x.flags & MEM_FLAG_EXECUTABLE) { need_icache_flush(); }

               /*
                  prepare to return any extra space that we don't need to the free
                  list, by emulating the case that the block was larger than needed
               */

               if (curr_space > space_left) {
                  space = new_space + curr_space - space_left;
                  curr_space = space_left;
               }
            }

            vcos_assert(curr_space <= space_left);
            space_left -= curr_space;
            curr = (MEM_LINK_T *)((uint8_t *)curr + curr_space);
         }  while (space_left != 0);
      }

      /*
         now finally grow into the claimed area
      */

      h->space = new_space | 0x1;
   }

   if (new_space < space) {
      MEM_LINK_T *end = (MEM_LINK_T *)((uint8_t *)g_mgr.base + g_mgr.space);
      MEM_LINK_T *next = (MEM_LINK_T *)((uint8_t *)h + space);
      space -= new_space;
      if ((next == end) || (next->space & 0x1))
      {
         if (next < end)
         {
            do
            {
               next = (MEM_LINK_T *)((uint8_t *)next + (next->space & ~0x1));
            }  while ((next < end) && (next->space & 0x1));
         }
         if (next == end)
            next = &g_mgr.free;

#if HEADERS_IN_DIRECT_ALIAS
         vclib_dcache_invalidate_range((MEM_HEADER_T *)((uint8_t *)h + new_space) + 1, space - 32);
#endif
#ifdef MEM_FILL_FREE_SPACE
         memset((MEM_HEADER_T *)((uint8_t *)h + new_space) + 1, 0xff, space - 32);
#endif
      }
      else
      {
#if defined(MEM_FILL_FREE_SPACE) || defined(HEADERS_IN_DIRECT_ALIAS)
         uint32_t fill_space = space;
#endif

         /* Merge with following space */

         MEM_REMOVE_FREE_REGION(next, next->space);

         space += next->space;
         next = next->next;
         remove_link(next->prev);
#if HEADERS_IN_DIRECT_ALIAS
         vclib_dcache_invalidate_range((MEM_HEADER_T *)((uint8_t *)h + new_space) + 1, fill_space);
#endif
#ifdef MEM_FILL_FREE_SPACE
         memset((MEM_HEADER_T *)((uint8_t *)h + new_space) + 1, 0xff, fill_space);
#endif
      }

      MEM_ADD_FREE_REGION((uint8_t *)h + new_space, space);

      add_link((MEM_LINK_T *)((uint8_t *)h + new_space), next, space);
      h->space = new_space | 0x1;
   }

   /*
      we now have the right amount of space
      update the header and clear any new data
   */

   {
      uint32_t old_size = h->size;

      h->size = size;
      h->x.size_is_zero = (size == 0);

      if (size > old_size) {
         MEM_FLAG_T flags = (MEM_FLAG_T)h->x.flags;
         void *p = get_data_pointer(h, align, flags);
         uint32_t rounded_old_size = round_up(old_size, 32);

         if (flags & MEM_FLAG_EXECUTABLE) { need_icache_flush(); }

         // we always invalidate cached data here whatever the alias. Data from before the malloc will not be used again (and dirty data may be harmful)
         if (rounded_old_size < size)
            vclib_dcache_invalidate_range((uint8_t *)p + rounded_old_size, size - rounded_old_size);

         initialise_memory((uint8_t *)p + old_size, flags, size - old_size);
      }
   }

   return 1;
}

static int relocate_resize_internal(MEM_HANDLE_T handle, uint32_t size, mem_compact_mode_t mode)
{
   MEM_HEADER_T *h = get_header(handle);
   uint32_t align = 1 << h->x.log2_align;

   /*
      alloc new
   */

   uint32_t space = calc_space(size, align);
   MEM_HEADER_T *new_h = alloc_compact_internal(space, mode);
   if (!new_h) {
      return 0;
   }
   h = get_header(handle); /* may have moved */

if(h==NULL)
{
return 0;
}
   vcos_assert(new_h->space == (space | 0x1));

   copy_alloc(h, new_h, size);

   new_h->space = (space | 0x1);
   /* restore the new space which will have been overwritten */

   /*
      free old
   */

   free_internal((MEM_LINK_T *)h);

   return 1;
}

/******************************************************************************
pool management api
******************************************************************************/

/*
   Get default values for memory pool defined in the platform makefile
*/

void mem_get_default_partition(void **mempool_base, uint32_t *mempool_size, void **mempool_handles_base, uint32_t *mempool_handles_size)
{
#ifndef __VIDEOCORE__
   /*
    * Dirty ugly hack.  TODO - discuss whether running cubes.exe on windows ought to refer to vcfw at all
    * This hack is what was in egl/server.c - copied here just to get driver to build when using simpenrose
    */
   *mempool_base = 0;
   *mempool_handles_base = 0;
   *mempool_size = (80 * 1024 + 64 ) * 1024;
   *mempool_handles_size = 32 * 1024;
#else
   // the following come from the platform makefile, through the linker command file
   extern _Far unsigned char __MEMPOOL_HANDLES_START[], __MEMPOOL_HANDLES_END[];
   extern _Far unsigned char __MEMPOOL_START[], __MEMPOOL_END[];
   *mempool_base = __MEMPOOL_START;
   *mempool_size = __MEMPOOL_END-__MEMPOOL_START;
   *mempool_handles_base = __MEMPOOL_HANDLES_START;
   *mempool_handles_size = __MEMPOOL_HANDLES_END-__MEMPOOL_HANDLES_START;
#endif
}

/*
   Initialize the memory subsystem, allocating a pool of a given size and
   with space for the given number of handles.
*/

static MEM_HANDLE_T mem_alloc_ex_no_mutex(
   uint32_t size,
   uint32_t align,
   MEM_FLAG_T flags,
   const char *desc,
   mem_compact_mode_t mode);

int mem_init(void *mempool_base, uint32_t mempool_size, void *mempool_handles_base, uint32_t mempool_handles_size)
{
   MEM_FORGET_FREE_REGIONS();

#ifdef BRCM_V3D_OPT
	int ret;
	fd_gemem = open("/dev/gememalloc",O_RDWR);
	if (fd_gemem < 0) {
	   LOGE("RTOS COMMON MEM mem_init failed to open /dev/gememalloc device fd[%d]", fd_gemem);
	   return 0;
	}
	int tempSize = GLOBAL_POOL_SIZE;
	GEMemallocwrapParams params;

	params.size = tempSize;
	params.busAddress =0;
	ret = ioctl(fd_gemem, GEMEMALLOC_WRAP_ACQUIRE_BUFFER, &params);
	if((params.busAddress == 0) || (ret != 0))
	{
		LOGE("RTOS COMMON MEM zero linear buffer allocated sz[%d] addr[%x] fd[%d] ret[%d]\n",
			tempSize, params.busAddress, fd_gemem, ret);
		return 0;
	}
	memory_pool_addr = params.busAddress;
	memory_pool_base = (void*)mmap(0, tempSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_gemem, memory_pool_addr);
	if (memory_pool_base == MAP_FAILED) {
		LOGE("RTOS COMMON MEM map failed sz[%d] addr[%x] virt[%x] fd[%d] ret[%d]\n",
			tempSize, params.busAddress, memory_pool_base, fd_gemem, ret);
	   return 0;
	}
	memory_pool_size = tempSize;
	mempool_base = memory_pool_base;
	mempool_size = memory_pool_size;
	/* Map the bus address to virtual address */
#endif

   g_mgr.space = mempool_size;
#if HEADERS_IN_DIRECT_ALIAS
   g_mgr.base = RTOS_ALIAS_DIRECT(mempool_base);
#else
   g_mgr.base = RTOS_ALIAS_NORMAL(mempool_base);
#endif

   vcos_assert(g_mgr.space > 0 && g_mgr.base != 0);

   if (!g_mgr.base)
      return 0;

   g_mgr.total_space = mempool_size;
   g_mgr.legacy_align = _max(MEM_LEGACY_BLOCK_ALIGN, sizeof(MEM_HEADER_T));
   g_mgr.legacy_size = round_up(MEM_LEGACY_BLOCK_SIZE, g_mgr.legacy_align);
   g_mgr.legacy_count = 0;
   if (g_mgr.legacy_size == 0)
   {
      g_mgr.legacy_count_max = 0;
   }
   else
   {
      g_mgr.legacy_count_max = _min(MEM_LEGACY_BLOCK_MAX, g_mgr.total_space / g_mgr.legacy_size);

      vcos_assert((g_mgr.legacy_size % g_mgr.legacy_align) == 0);
      vcos_assert(g_mgr.legacy_count_max <= 8*sizeof(g_mgr.legacy_free_map));
   }

   g_mgr.max_handles = mempool_handles_size / sizeof(*g_mgr.handles);
   g_mgr.num_handles = (uint32_t)MEM_HANDLE_INVALID + 1;
#if HANDLES_IN_DIRECT_ALIAS
   g_mgr.handles = (MEM_MANAGER_HANDLE_T *)RTOS_ALIAS_DIRECT(mempool_handles_base);
#else
   g_mgr.handles = (MEM_MANAGER_HANDLE_T *)RTOS_ALIAS_NORMAL(mempool_handles_base);
#endif

   vcos_assert(g_mgr.max_handles > 0 && g_mgr.handles != 0);

   if (!g_mgr.handles) {
      return 0;
   }

   g_mgr.free.prev = g_mgr.free.next = &g_mgr.free;

   add_link((MEM_LINK_T *)g_mgr.base, &g_mgr.free, g_mgr.space);
#if HEADERS_IN_DIRECT_ALIAS
   vclib_dcache_invalidate_range((MEM_HEADER_T *)g_mgr.base + 1, g_mgr.space - 32);
#endif
#ifdef MEM_FILL_FREE_SPACE
   memset((MEM_HEADER_T *)g_mgr.base + 1, 0xff, g_mgr.space - 32);
#endif

   g_mgr.free_handle = MEM_HANDLE_INVALID;
   if (MEM_HANDLE_INVALID == 0)
   {
      g_mgr.handles[0].next = (MEM_HANDLE_T)0;
      g_mgr.handles[0].header = NULL;
   }

#ifdef MEM_USE_SMALL_ALLOC_POOL
   /* initialise small alloc pool */
   g_smallallocs_free_handle = MEM_HANDLE_INVALID;
   g_smallallocs_num_handles = 0;
#ifdef BRCM_V3D_OPT
	 {
		 int i;
		 for(i=0;i<MEM_SMALL_ALLOC_COUNT;i++)
			 {
			 g_smallallocs[i] = NULL;
			 }
	 }
	{
		int i;
		for(i=0;i<MEM_BIG_ALLOC_COUNT;i++)
			{
			g_bigallocs[i] = NULL;
			}
	}
#endif
#endif

   g_mgr.magic = MEM_MGR_MAGIC;
   g_mgr.ver = MEM_MGR_VER;

   /* allocate special handles. make sure we don't acquire the mem mutex while
    * doing this as it might not work right now (interrupts are possibly
    * disabled) */

   if (mem_alloc_ex_no_mutex(0, 4, MEM_FLAG_NONE, "zero size", MEM_COMPACT_NONE) != MEM_ZERO_SIZE_HANDLE)
      vcos_assert(0);

   if (mem_alloc_ex_no_mutex(1, 1, MEM_FLAG_NONE, "empty string", MEM_COMPACT_NONE) != MEM_EMPTY_STRING_HANDLE)
      vcos_assert(0);
   ((char *)get_data_pointer(get_header(MEM_EMPTY_STRING_HANDLE), 1, MEM_FLAG_NONE))[0] = '\0';

   vcos_quickslow_mutex_create(&g_mgr.mutex, "rtos_common_mem quick/slow mutex");

#ifdef AGGRESSIVE_COMPACTION_LED
   g_led_driver = LED_DRIVER_GET_FUNC_TABLE();
   if (g_led_driver->open(NULL, &g_led_handle) != 0)
      g_led_driver = NULL;
#endif

   g_mgr.aggressive_locked = NULL;
   vcos_event_create(&g_mgr.aggressive_owner_latch, "aggressive_owner_latch");
   vcos_event_create(&g_mgr.aggressive_busy_latch, "aggressive_busy_latch");
   g_mgr.busy_latch_count = 0;
   g_mgr.callbacks_count = 0;
#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
   vcos_event_create(&g_aggressive_timeout_latch, "aggressive_timeout_latch");
   vcos_thread_create_classic(&g_aggressive_timeout_task, "mem_timo", compact_timeout_task, 0,
                              g_aggressive_timeout_stack, sizeof(g_aggressive_timeout_stack), 10, 10, VCOS_START);
   vcos_timer_create(&g_aggressive_timeout_timer, "mem_timo", compact_timeout, NULL);
#endif

   return 1;
}

/*
   Terminate the memory subsystem, releasing the pool.
*/

void mem_term(void)
{
   MEM_CLEAR_SANDBOX_REGIONS();

#ifdef AGGRESSIVE_COMPACTION_TIMEOUT
   vcos_timer_cancel(&g_aggressive_timeout_timer);
   vcos_thread_join(&g_aggressive_timeout_task,NULL);
#endif
   vcos_quickslow_mutex_delete(&g_mgr.mutex);
#ifdef BRCM_V3D_OPT
	 GEMemallocwrapParams params;
	 params.size = memory_pool_size;
	 params.busAddress = memory_pool_addr;
#ifdef ENABLE_PHYSMEM
	 munmap(memory_pool_base, memory_pool_size);
#endif
		ioctl(fd_gemem, GEMEMALLOC_WRAP_RELEASE_BUFFER, &params);

	  memory_pool_base = 0;
	  memory_pool_addr = 0;
	  memory_pool_size = 0;
	  close(fd_gemem);
#endif
   g_mgr.magic = 0;
   g_mgr.base = 0;
   g_mgr.space = 0;
}

/*
   The heap is compacted to the maximum possible extent. If (mode & MEM_COMPACT_DISCARD)
   is non-zero, all discardable, unlocked, and unretained MEM_HANDLE_Ts are resized to size 0.
   If (mode & MEM_COMPACT_AGGRESSIVE) is non-zero, all long-term block owners (which are
   obliged to have registered a callback) are asked to unlock their blocks for the duration
   of the compaction.
*/

void mem_compact(mem_compact_mode_t mode)
{
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
   MEM_CLEAR_SANDBOX_REGIONS();

   compact_internal(mode, 0, 0);

   MEM_SET_SANDBOX_REGIONS();
   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
}

/******************************************************************************
core api
******************************************************************************/

static void init_header(MEM_HEADER_T *h, MEM_HANDLE_T handle, uint32_t size, uint32_t align, MEM_FLAG_T flags, const char *desc)
{
   h->size = size;
   h->x.size_is_zero = (size == 0);
   h->x.log2_align = _msb(align);
   h->term = NULL;
   h->desc = desc;
   h->ref_count = 1;
   h->x.lock_count = 0;
   h->retain_count = (flags & MEM_FLAG_RETAINED) ? 1 : 0;
   h->handle = handle;

   flags = (MEM_FLAG_T)((flags & ~MEM_FLAG_RETAINED) | ((flags & MEM_FLAG_NO_INIT) ? MEM_FLAG_ABANDONED : 0));
   h->x.flags = flags;

   if (flags & MEM_FLAG_EXECUTABLE) { need_icache_flush(); }

   // we always invalidate cached data here whatever the alias. Data from before the malloc will not be used again (and dirty data may be harmful)
   vclib_dcache_invalidate_range(get_data_pointer(h, align, flags), size);

   initialise_memory(get_data_pointer(h, align, flags), flags, size);
}

/*
   Attempts to allocate a movable block of memory of the specified size and
   alignment. size may be 0. A MEM_HANDLE_T with size 0 is special: see
   mem_lock/mem_unlock. mode specifies the types of compaction permitted,
   including MEM_COMPACT_NONE.

   Preconditions:

   - align is a power of 2.
   - flags only has set bits within the range specified by MEM_FLAG_ALL.
   - desc is NULL or a pointer to a null-terminated string.

   Postconditions:

   If the attempt succeeds:
   - A fresh MEM_HANDLE_T referring to the allocated block of memory is
     returned.
   - The MEM_HANDLE_T is unlocked, without a terminator, and has a reference
     count of 1.
   - If MEM_FLAG_RETAINED was specified, the MEM_HANDLE_T has a retain count of
     1, otherwise it is unretained.
   - If the MEM_FLAG_ZERO flag was specified, the block of memory is set to 0.
     Otherwise, each aligned word is set to MEM_HANDLE_INVALID.

   If the attempt fails:
   - MEM_HANDLE_INVALID is returned.
*/
static uint32_t mem_get_free_space_unlocked();
static MEM_HANDLE_T mem_alloc_ex_no_mutex(
   uint32_t size,
   uint32_t align,
   MEM_FLAG_T flags,
   const char *desc,
   mem_compact_mode_t mode)
{
   vcos_assert(g_mgr.magic == MEM_MGR_MAGIC);

   vcos_assert(is_power_of_2(align));
   vcos_assert(!(flags & ~MEM_FLAG_ALL));
   vcos_assert((flags & MEM_FLAG_RESIZEABLE) || !(flags & MEM_FLAG_HINT_GROW));

#ifdef TEST_ALLOC_FAIL
   if (should_check_fail())
   {
      return MEM_HANDLE_INVALID;
   }
#endif

   MEM_CLEAR_SANDBOX_REGIONS();

   /*
      Ignore MEM_FLAG_HINT_GROW for now.
   */

   flags = (MEM_FLAG_T)(flags & ~MEM_FLAG_HINT_GROW);

#ifdef MEM_USE_SMALL_ALLOC_POOL
#ifdef BRCM_V3D_OPT
   if (flags == MEM_FLAG_NONE) {
#else
   if (size <= MEM_SMALL_ALLOC_SIZE && align <= MEM_SMALL_ALLOC_ALIGN &&
       flags == MEM_FLAG_NONE){
#endif
      /*
         Allocate out of the small alloc pool.

         Currently we only do this if all flags are default. Particular flags we want to avoid are:
            MEM_FLAG_EXECUTABLE
            MEM_FLAG_DIRECT
            MEM_FLAG_RESIZEABLE
      */
      MEM_HANDLE_T handle;
      MEM_HEADER_T *h = 0;

      handle = g_smallallocs_free_handle;

#ifdef BRCM_V3D_OPT
	  	{
			int i;
		  for(i=0;i<MEM_SMALL_ALLOC_COUNT;i++)
			  {
			  if(g_smallallocs[i] == NULL)
			  	{
			  	g_smallallocs[i] = malloc(calc_space(size,align));
				handle = (MEM_HANDLE_T)(MEM_HANDLE_BIT_SMALL_ALLOC | i);
				break;
			  	}
			  }
      	}
        if (handle != MEM_HANDLE_INVALID)
        {
             h = get_header(handle);
        }

#else
      if (handle != MEM_HANDLE_INVALID)
      {
         h = get_header(handle);
         g_smallallocs_free_handle = (MEM_HANDLE_T)h->space;    /* Reused for "next" field of free list */
      }
      else if (g_smallallocs_num_handles < MEM_SMALL_ALLOC_COUNT)
      {
         handle = (MEM_HANDLE_T)(MEM_HANDLE_BIT_SMALL_ALLOC | g_smallallocs_num_handles++);
         h = get_header(handle);
      }
#endif
      if (handle != MEM_HANDLE_INVALID)
      {
         vcos_assert(is_small_alloc(handle));

         init_header(h, handle, size, align, flags, desc);
         h->space = 0;                                  /* Set this to 0 so that dump can distinguish free & used slots */

         MEM_SET_SANDBOX_REGIONS();

         return handle;
      }
   }
#endif
	uint32_t space =  calc_space(size,align);
	MEM_HANDLE_T handle = MEM_HANDLE_INVALID;
	MEM_HEADER_T *h = 0;


   if(space < 4096)
   	{
   	handle = alloc_handle();
	
	if (handle != MEM_HANDLE_INVALID) {
		h = alloc_compact_internal(space, mode);
		if (!h) {
			free_handle(handle);
			g_mgr.stats.alloc_fails++;
			handle = MEM_HANDLE_INVALID;

			MEM_SET_SANDBOX_REGIONS();
			LOGE("RTOS COMMON MEM allocation failed from global pool for %d bytes.. space left is %d bytes",size,mem_get_free_space_unlocked());
			
#ifdef ASSERT_ON_ALLOC_FAIL
			assert(0);
#endif
			//return MEM_HANDLE_INVALID;
			}
		else {
			set_header(handle, h);
			init_header(h, handle, size, align, flags, desc);
			return handle;
			}
		}
	else
		g_mgr.stats.alloc_fails++;
	MEM_SET_SANDBOX_REGIONS();

#ifdef ASSERT_ON_ALLOC_FAIL
	assert(handle != MEM_HANDLE_INVALID);
#endif
   }
#ifdef BRCM_V3D_OPT
	{
		{
			int i;
			for(i=0;i<MEM_BIG_ALLOC_COUNT;i++)
			{
				if(g_bigallocs[i] == NULL)
				{
					int tempSize = calc_space(size,align);
					int pgsize = 4096;
					unsigned char* buffer= 0;
					tempSize = (tempSize + pgsize) & (~(pgsize - 1));
					GEMemallocwrapParams params;

					params.size = tempSize;
					params.busAddress =0;
					/* get memory linear memory buffers */
					ioctl(fd_gemem, GEMEMALLOC_WRAP_ACQUIRE_BUFFER, &params);
					if(params.busAddress == 0)
					{
						LOGE("RTOS COMMON MEM zero linear buffer allocated %d %x %x\n",tempSize,params.busAddress,flags);
						break;
					}

					/* Map the bus address to virtual address */
#ifdef ENABLE_PHYSMEM
					buffer= (void*)mmap(0, tempSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_gemem, params.busAddress);;
#else
					buffer= (void *)((unsigned int)(params.busAddress - memory_pool_addr) + memory_pool_base);
	//				buffer= (void*)khrn_hw_unaddr(params.busAddress);
#endif

					*(unsigned int*)buffer = params.busAddress;

					g_bigallocs[i] = buffer;
					g_bigalloc_count ++;
	
					handle = (MEM_HANDLE_T)(MEM_HANDLE_BIT_BIG_ALLOC | i);
					break;
				}
			}
		}
		if (handle != MEM_HANDLE_INVALID)
		{
			h = get_header(handle);
			init_header(h, handle, size, align, flags, desc);
			MEM_SET_SANDBOX_REGIONS();

			return handle;
		}
		else
			{
			LOGE("BIG MEM OUT OF HANDLES");
			}
	}


#endif

#ifdef MEM_AUTO_VALIDATE
   mem_validate_internal(0);
#endif
	return handle;
}

MEM_HANDLE_T mem_alloc_ex(
   uint32_t size,
   uint32_t align,
   MEM_FLAG_T flags,
   const char *desc,
   mem_compact_mode_t mode)
{
   MEM_HANDLE_T handle;
   if((mem_get_free_space() < (GLOBAL_POOL_SIZE/4)) || (g_bigalloc_count	> (3*MEM_BIG_ALLOC_COUNT/4)))
   	{
   	khrn_hw_wait();
   	}
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
   handle = mem_alloc_ex_no_mutex(size, align, flags, desc, mode);
   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
   return handle;
}

#ifdef MEM_WRAP_HACK
typedef struct {
   void *p;
   uint32_t phys;
   uint32_t size;
   uint32_t align;
   MEM_FLAG_T flags;
} MEM_WRAP_T;

static RCM_INLINE int is_wrap_handle(MEM_HANDLE_T handle)           { return handle & MEM_HANDLE_BIT_WRAP_HACK; }
static RCM_INLINE MEM_HANDLE_T from_wrap_handle(MEM_HANDLE_T handle) { return (MEM_HANDLE_T)(handle & ~MEM_HANDLE_BIT_WRAP_HACK); }
static RCM_INLINE MEM_HANDLE_T to_wrap_handle(MEM_HANDLE_T handle)   { return (MEM_HANDLE_T)(handle | MEM_HANDLE_BIT_WRAP_HACK); }

#ifdef BRCM_V3D_OPT
MEM_HANDLE_T mem_wrap(void *p, uint32_t phys,uint32_t size, uint32_t align, MEM_FLAG_T flags, const char *desc)
#else
MEM_HANDLE_T mem_wrap(void *p, uint32_t size, uint32_t align, MEM_FLAG_T flags, const char *desc)
#endif
{
   MEM_HANDLE_T handle;
   MEM_WRAP_T *wrap;

   vcos_assert(g_mgr.magic == MEM_MGR_MAGIC);

#ifdef BRCM_V3D_OPT
	if((p == NULL) || (phys == NULL))
		{
		return MEM_HANDLE_INVALID;
		}
#endif
   handle = mem_alloc(sizeof(MEM_WRAP_T), RCM_ALIGNOF(MEM_WRAP_T), MEM_FLAG_NONE, desc);
   if (handle == MEM_HANDLE_INVALID) {
      return MEM_HANDLE_INVALID;
   }

   wrap = (MEM_WRAP_T *)mem_lock(handle);
   wrap->p = p;
   wrap->phys = phys;
   wrap->size = size;
   wrap->align = align;
   wrap->flags = flags;
   mem_unlock(handle);
#ifdef BRCM_V3D_OPT
   	{
		WrapList* temp = malloc(sizeof(WrapList));
		temp->next = NULL;
		temp->physical_address = phys;
		temp->virtual_address = p;
		temp->size = size;
		vcos_quickslow_mutex_lock(&g_mgr.mutex);
		if(g_WrapList == NULL)
			{
			g_WrapList = temp;
			}
		else
			{
			WrapList* head = g_WrapList;
			do{
				if((head->physical_address == temp->physical_address) && (head->virtual_address == temp->virtual_address))
					{
					free(temp);
					break;
					}
				if((head->physical_address == temp->physical_address) || (head->virtual_address == temp->virtual_address))
					{
					head->physical_address = temp->physical_address;
					head->virtual_address = temp->virtual_address;
					head->size = temp->size;
					free(temp);
					break;
					}

				if(head->next == NULL)
					{
					head->next = temp;
					break;
					}
				head = head->next;
				}while(head != NULL);
			}
		vcos_quickslow_mutex_unlock(&g_mgr.mutex);
   	}


#endif
   return to_wrap_handle(handle);
}
#endif

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The reference count of the MEM_HANDLE_T is incremented.
*/

void mem_acquire(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
      vcos_assert(h->ref_count != 0);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
      ++h->ref_count;
      vcos_assert(h->ref_count);        // check we've not wrapped
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

void mem_acquire_retain(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
      if(h==NULL)
      {
		  vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
		  return;
      }
      vcos_assert(h->ref_count != 0);
      ++h->ref_count;
      vcos_assert(h->ref_count);        // check we've not wrapped

      if (h->x.flags & MEM_FLAG_DISCARDABLE)
      {
         ++h->retain_count;
         vcos_assert(h->retain_count);        // check we've not wrapped
      }
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   If the reference count of the MEM_HANDLE_T is 1 and it has a terminator, the
   terminator is called with a pointer to and the size of the MEM_HANDLE_T's
   block of memory (or NULL/0 if the size of the MEM_HANDLE_T is 0). The
   MEM_HANDLE_T may not be used during the call.

   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - If its reference count is 1, it must not be locked or retained.

   Postconditions:

   If the reference count of the MEM_HANDLE_T was 1:
   - The MEM_HANDLE_T is now invalid. The associated block of memory has been
     freed.

   Otherwise:
   - The reference count of the MEM_HANDLE_T is decremented.
*/

static void free_thing(MEM_HANDLE_T handle, MEM_HEADER_T *h)
{
   MEM_CLEAR_SANDBOX_REGIONS();
#ifdef MEM_AUTO_VALIDATE
   mem_validate_internal(0);
#endif

   vcos_assert(h->ref_count == 0);
   vcos_assert((handle != MEM_ZERO_SIZE_HANDLE) && (handle != MEM_EMPTY_STRING_HANDLE));
   vcos_assert(h->x.lock_count == 0);
   vcos_assert(h->retain_count == 0);

   if (h->term) {
      h->x.lock_count = 1;

      MEM_SET_SANDBOX_REGIONS();

#if HEADERS_IN_DIRECT_ALIAS
      vcos_quickslow_mutex_unlock(&g_mgr.mutex);
#else
      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
#endif

      /*
         at this point, the handle still exists, but has a reference count of
         0, so using it in any mem_* calls will cause an assert
      */

      h->term(get_data_pointer(h, 1 << h->x.log2_align, (MEM_FLAG_T)h->x.flags), h->size);

#if HEADERS_IN_DIRECT_ALIAS
      vcos_quickslow_mutex_lock(&g_mgr.mutex);
#else
      vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
#endif

      h->x.lock_count = 0;
      if (h == g_mgr.aggressive_locked)
      {
#if HEADERS_IN_DIRECT_ALIAS
         resume_compaction_slow();
         vcos_quickslow_mutex_lock(&g_mgr.mutex);
#else
         resume_compaction();
         vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
#endif
         h = get_header(handle);
      }

      MEM_CLEAR_SANDBOX_REGIONS();
   }

#ifdef MEM_USE_SMALL_ALLOC_POOL
#ifdef BRCM_V3D_OPT
   if (is_small_alloc(handle))
   {
      free(h);
	  g_smallallocs[handle&~MEM_HANDLE_BIT_SMALL_ALLOC] = NULL;
   } else if (is_big_alloc(handle))
   {
	   MEM_HEADER_T* header = (MEM_HEADER_T*)g_bigallocs[handle&~MEM_HANDLE_BIT_BIG_ALLOC];
	   if(header == NULL)
	   	{
	   	return;
	   	}
	   int tempSize = calc_space(header->size,1<<(header->x.log2_align));
	   int pgsize = 4096;
	   unsigned char* buffer= 0;
	   tempSize = (tempSize + pgsize) & (~(pgsize - 1));
	   GEMemallocwrapParams params;

	   params.size = tempSize;
	   params.busAddress =header->space;
#ifdef ENABLE_PHYSMEM
					   munmap(header, tempSize);
#endif

	   /* get memory linear memory buffers */
	   ioctl(fd_gemem, GEMEMALLOC_WRAP_RELEASE_BUFFER, &params);

	   /* Map the bus address to virtual address */
	  g_bigalloc_count --;
	  g_bigallocs[handle&~MEM_HANDLE_BIT_BIG_ALLOC] = NULL;
   }else {
      free_handle(handle);
      free_internal((MEM_LINK_T *)h);
   }
#else
   if (is_small_alloc(handle))
   {
      h->space = (uint32_t)g_smallallocs_free_handle;
      g_smallallocs_free_handle = handle;
   } else {
      free_handle(handle);
      free_internal((MEM_LINK_T *)h);
   }
#endif
#else
   free_handle(handle);
   free_internal((MEM_LINK_T *)h);
#endif
   MEM_SET_SANDBOX_REGIONS();
}

void mem_release(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

#if HEADERS_IN_DIRECT_ALIAS
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
#else
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
#endif

   {
      MEM_HEADER_T *h = get_header(handle);
      vcos_assert(h->ref_count != 0);
      if (--h->ref_count == 0) {
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
         free_thing(handle, h);
      }
   }

#if HEADERS_IN_DIRECT_ALIAS
   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
#else
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
#endif
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   If the reference count of the MEM_HANDLE_T was 1:
   - 0 is returned.
   - The reference count of the MEM_HANDLE_T is still 1.

   Otherwise:
   - 1 is returned.
   - The reference count of the MEM_HANDLE_T is decremented.
*/

int mem_try_release(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
      vcos_assert(h->ref_count != 0);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      if (h->ref_count == 1) {
         vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
         return 0;
      }
      --h->ref_count;
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
   return 1;
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The total space used by the MEM_HANDLE_T is returned (this includes the
     header, the actual block, and padding).
   - sum_over_handles(mem_get_space(handle)) + mem_get_free_space() is constant
     over the lifetime of the pool.
*/

uint32_t mem_get_space(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      /* space used by MEM_WRAP_T + size of actual block */
      return mem_get_space(from_wrap_handle(handle)) + mem_get_size(handle);
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return sizeof(MEM_SMALL_ALLOC_T);
   }
#ifdef BRCM_V3D_OPT

if (is_big_alloc(handle)) {
	MEM_HEADER_T* header = get_header(handle);
	if(header ==NULL)
		return 0;
	else
	   return sizeof(MEM_HEADER_T) + header->size;
}

#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = h->space;

      vcos_assert(h->ref_count != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The size of the MEM_HANDLE_T's block of memory is returned.
*/

uint32_t mem_get_size(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      uint32_t size;
      handle = from_wrap_handle(handle);
      size = ((MEM_WRAP_T *)mem_lock(handle))->size;
      mem_unlock(handle);
      return size;
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = h->size;

      vcos_assert(h->ref_count != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The minimum required alignment of the MEM_HANDLE_T's block of memory (as
     passed to mem_alloc) is returned.
*/

uint32_t mem_get_align(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      uint32_t align;
      handle = from_wrap_handle(handle);
      align = ((MEM_WRAP_T *)mem_lock(handle))->align;
      mem_unlock(handle);
      return align;
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = 1 << h->x.log2_align;

      vcos_assert(h->ref_count != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's flags (as passed to mem_alloc) are returned.
*/

MEM_FLAG_T mem_get_flags(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      MEM_FLAG_T flags;
      handle = from_wrap_handle(handle);
      flags = ((MEM_WRAP_T *)mem_lock(handle))->flags;
      mem_unlock(handle);
      return flags;
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return MEM_FLAG_NONE;
}
      MEM_FLAG_T result = (MEM_FLAG_T)h->x.flags;

      vcos_assert(h->ref_count != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's reference count is returned.
*/

uint32_t mem_get_ref_count(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = h->ref_count;

      vcos_assert(result != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's lock count is returned.
*/

uint32_t mem_get_lock_count(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = h->x.lock_count;

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's retain count is returned.
*/

uint32_t mem_get_retain_count(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      uint32_t result = h->retain_count;

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's description string is returned.
*/

const char *mem_get_desc(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      const char *result = h->desc;

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - desc is NULL or a pointer to a null-terminated string.

   Postconditions:

   - The MEM_HANDLE_T's description is set to desc.
*/

#ifndef NDEBUG
void mem_set_desc(
   MEM_HANDLE_T handle,
   const char *desc)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
      vcos_assert(h->ref_count != 0);
      h->desc = desc;
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}
#endif

/*
   The MEM_HANDLE_T's terminator is called just before the MEM_HANDLE_T becomes
   invalid: see mem_release.

   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - term must be NULL or a pointer to a function compatible with the MEM_TERM_T
     type.

   Postconditions:

   - The MEM_HANDLE_T's terminator is set to term (if term was NULL, the
     MEM_HANDLE_T no longer has a terminator).
*/

void mem_set_term(
   MEM_HANDLE_T handle,
   MEM_TERM_T term)
{
#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle)); /* mem_release won't pass the right pointer */
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
      vcos_assert(h->ref_count != 0);
      h->term = term;
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's terminator is returned, or NULL if there is none.
*/

MEM_TERM_T mem_get_term(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return get_small_alloc_header(handle)->term; /* assume the read is atomic... */
   }
#ifdef BRCM_V3D_OPT
   else if (is_big_alloc(handle)) {
      return get_big_alloc_header(handle)->term; /* assume the read is atomic... */
   }
#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      MEM_TERM_T result = h->term;

      vcos_assert(h->ref_count != 0);

      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

      return result;
   }
}

/*
   void mem_set_user_flag(MEM_HANDLE_T handle, int flag)

   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   - The MEM_HANDLE_T's user flag is set to 0 if flag is 0, or to 1 otherwise.
*/

void mem_set_user_flag(
   MEM_HANDLE_T handle, int flag)
{
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      handle = from_wrap_handle(handle);
   }
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
      vcos_assert(h->ref_count != 0);
      if (flag)
         h->x.flags |= MEM_FLAG_USER;
      else
         h->x.flags &= ~MEM_FLAG_USER;
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   Attempts to resize the MEM_HANDLE_T's block of memory. The attempt is
   guaranteed to succeed if the new size is less than or equal to the old size.
   size may be 0. A MEM_HANDLE_T with size 0 is special: see
   mem_lock/mem_unlock. mode specifies the types of compaction permitted,
   including MEM_COMPACT_NONE.

   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - It must not be locked.
   - It must have been created with the MEM_FLAG_RESIZEABLE flag.

   Postconditions:

   If the attempt succeeds:
   - 1 is returned.
   - The MEM_HANDLE_T's block of memory has been resized.
   - The contents in the region [0, min(old size, new size)) are unchanged. If
     the MEM_HANDLE_T is zero'd, the region [min(old size, new size), new size)
     is set to 0. Otherwise, each aligned word in the region
     [min(old size, new size), new size) is set to MEM_HANDLE_INVALID.

   If the attempt fails:
   - 0 is returned.
   - The MEM_HANDLE_T is still valid.
   - Its block of memory is unchanged.
*/

int mem_resize_ex(
   MEM_HANDLE_T handle,
   uint32_t size,
   mem_compact_mode_t mode)
{
   int success;

#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif
#ifdef BRCM_V3D_OPT
	vcos_quickslow_mutex_lock(&g_mgr.mutex);
	if (is_big_alloc(handle))
	{
		MEM_HEADER_T* header = (MEM_HEADER_T*)g_bigallocs[handle&~MEM_HANDLE_BIT_BIG_ALLOC];
		unsigned int align = 32;
		unsigned int oldSize = 0;
		if(header != NULL)
			{
			align = 1 << (header->x.log2_align);
			oldSize = calc_space(header->size,1 << (header->x.log2_align));
			if(!(header->x.flags & MEM_FLAG_RESIZEABLE))
				{
				vcos_quickslow_mutex_unlock(&g_mgr.mutex);
				return 0;
				}
			}
			int tempSize = calc_space(size,align);
			int pgsize = 4096;
			unsigned char* buffer= 0;
			tempSize = (tempSize + pgsize) & (~(pgsize - 1));
			GEMemallocwrapParams params;

			params.size = tempSize;
			params.busAddress =0;
			/* get memory linear memory buffers */
			ioctl(fd_gemem, GEMEMALLOC_WRAP_ACQUIRE_BUFFER, &params);

			if(params.busAddress == 0)
			{
				LOGE("RTOS COMMON MEM RESIZE zero linear buffer allocated %d %x\n",tempSize,params.busAddress);
				vcos_quickslow_mutex_unlock(&g_mgr.mutex);
				return 0;
			}

			/* Map the bus address to virtual address */
#ifdef ENABLE_PHYSMEM
		   buffer= (void*)mmap(0, tempSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_gemem, params.busAddress);;
#else
		buffer= (void *)((unsigned int)(params.busAddress - memory_pool_addr) + memory_pool_base);
#endif
			if((header != NULL) && (oldSize <= tempSize))
				{
				memcpy(buffer,header,oldSize);
				}
			*(unsigned int*)buffer = params.busAddress;
			((MEM_HEADER_T*)buffer)->size = size;
			g_bigallocs[handle&~MEM_HANDLE_BIT_BIG_ALLOC] = buffer;
			if(header != NULL)
			 {
				tempSize = (oldSize + pgsize) & (~(pgsize - 1));
	
				params.size = tempSize;
				params.busAddress =header->space;
#ifdef ENABLE_PHYSMEM
				munmap(header, tempSize);
#endif
				/* get memory linear memory buffers */
				ioctl(fd_gemem, GEMEMALLOC_WRAP_RELEASE_BUFFER, &params);
	}
	vcos_quickslow_mutex_unlock(&g_mgr.mutex);
	return 1;
	}
	vcos_quickslow_mutex_unlock(&g_mgr.mutex);
#endif
   if(mem_get_free_space() < (GLOBAL_POOL_SIZE/4))
   	{
   	khrn_hw_wait();
   	}

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   MEM_CLEAR_SANDBOX_REGIONS();

   vcos_assert(get_header(handle)->ref_count != 0);
   vcos_assert(get_header(handle)->x.lock_count == 0);
   vcos_assert(get_header(handle)->x.flags & MEM_FLAG_RESIZEABLE);

#ifdef TEST_ALLOC_FAIL
   if (get_header(handle)->size < size) // If we're enlarging, we might not succeed
   {
      if (should_check_fail())
      {
         MEM_SET_SANDBOX_REGIONS();
         vcos_quickslow_mutex_unlock(&g_mgr.mutex);
         return 0;
      }
   }
#endif


   success =
      /* try inplace resize if it might succeed where relocate resize would fail */
      inplace_resize_internal(handle, size) ||
      /* try relocate resize, compacting the heap and trying again if the first attempt fails */
      relocate_resize_internal(handle, size, mode) ||
      /* try inplace resize again with the compacted heap if it might succeed where relocate resize failed */
      inplace_resize_internal(handle, size) ||
      (g_mgr.stats.alloc_fails++, 0);

#ifdef ASSERT_ON_ALLOC_FAIL
   assert(success);
#endif

   MEM_SET_SANDBOX_REGIONS();

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
   return success;
}

/*
   A MEM_HANDLE_T with a lock count greater than 0 is considered to be locked
   and may not be moved by the memory manager.

   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - If it is locked, the previous lock must have been by mem_lock and not
     mem_lock_exec.

   Postconditions:

   If the MEM_HANDLE_T's size is 0:
   - NULL is returned.
   - The MEM_HANDLE_T is completely unchanged.

   Otherwise:
   - A pointer to the MEM_HANDLE_T's block of memory is returned. The pointer is
     valid until the MEM_HANDLE_T's lock count reaches 0.
   - The MEM_HANDLE_T's lock count is incremented.
   - Clears MEM_FLAG_ABANDONED.
*/

static RCM_INLINE void *lock_big_internal(MEM_HANDLE_T handle)
{
   MEM_HEADER_T *h = get_header(handle);
   uint32_t x;

if(h==NULL)
{
return 0;
}
   vcos_assert(h->ref_count != 0);
   vcos_assert((h->x.lock_count == 0) || !h->x.exec_locked);

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return NULL;
   }

   x += 1 << MEM_HEADER_X_LOCK_COUNT_SHIFT;
   vcos_assert(x & MEM_HEADER_X_LOCK_COUNT_MASK);        // check we've not wrapped
   x &= ~(MEM_HEADER_X_EXEC_LOCKED | (MEM_FLAG_ABANDONED << MEM_HEADER_X_FLAGS_SHIFT));

   *(uint32_t *)&h->x = x;

   {
      void *result;
      if (x &
         (((-4 & MEM_HEADER_X_LOG2_ALIGN_MASK) << MEM_HEADER_X_LOG2_ALIGN_SHIFT) |
         ((MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) << MEM_HEADER_X_FLAGS_SHIFT))) {
         result = get_data_pointer(h,
            1 << ((x & MEM_HEADER_X_LOG2_ALIGN_MASK) >> MEM_HEADER_X_LOG2_ALIGN_SHIFT),
            (MEM_FLAG_T)((x & MEM_HEADER_X_FLAGS_MASK) >> MEM_HEADER_X_FLAGS_SHIFT));
      } else {
#if HEADERS_IN_DIRECT_ALIAS
         result = RTOS_ALIAS_NORMAL(h + 1);
#else
         result = h + 1;
#endif
      }


      return result;
   }
}
#ifdef BRCM_V3D_OPT
static RCM_INLINE void *lock_big_mem_internal(MEM_HANDLE_T handle)
{
   MEM_HEADER_T *h = get_big_alloc_header(handle);
   uint32_t x;

if(h==NULL)
{
return 0;
}
   vcos_assert(h->ref_count != 0);
   vcos_assert((h->x.lock_count == 0) || !h->x.exec_locked);

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return NULL;
   }

   x += 1 << MEM_HEADER_X_LOCK_COUNT_SHIFT;
   vcos_assert(x & MEM_HEADER_X_LOCK_COUNT_MASK);        // check we've not wrapped
   x &= ~(MEM_HEADER_X_EXEC_LOCKED | (MEM_FLAG_ABANDONED << MEM_HEADER_X_FLAGS_SHIFT));

   *(uint32_t *)&h->x = x;

   {
      void *result;
      if (x &
         (((-4 & MEM_HEADER_X_LOG2_ALIGN_MASK) << MEM_HEADER_X_LOG2_ALIGN_SHIFT) |
         ((MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) << MEM_HEADER_X_FLAGS_SHIFT))) {
         result = get_data_pointer(h,
            1 << ((x & MEM_HEADER_X_LOG2_ALIGN_MASK) >> MEM_HEADER_X_LOG2_ALIGN_SHIFT),
            (MEM_FLAG_T)((x & MEM_HEADER_X_FLAGS_MASK) >> MEM_HEADER_X_FLAGS_SHIFT));
      } else {
#if HEADERS_IN_DIRECT_ALIAS
         result = RTOS_ALIAS_NORMAL(h + 1);
#else
         result = h + 1;
#endif
      }

      return result;
   }
}
static RCM_INLINE void *lock_big_mem_internal_phys(MEM_HANDLE_T handle)
{
   MEM_HEADER_T *h = get_big_alloc_header(handle);
   uint32_t x;

if(h==NULL)
{
return 0;
}
   vcos_assert(h->ref_count != 0);
   vcos_assert((h->x.lock_count == 0) || !h->x.exec_locked);

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return NULL;
   }

   x += 1 << MEM_HEADER_X_LOCK_COUNT_SHIFT;
   vcos_assert(x & MEM_HEADER_X_LOCK_COUNT_MASK);        // check we've not wrapped
   x &= ~(MEM_HEADER_X_EXEC_LOCKED | (MEM_FLAG_ABANDONED << MEM_HEADER_X_FLAGS_SHIFT));

   *(uint32_t *)&h->x = x;

   {
      void *result;
      if (x &
         (((-4 & MEM_HEADER_X_LOG2_ALIGN_MASK) << MEM_HEADER_X_LOG2_ALIGN_SHIFT) |
         ((MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) << MEM_HEADER_X_FLAGS_SHIFT))) {
         result = get_data_pointer(h,
            1 << ((x & MEM_HEADER_X_LOG2_ALIGN_MASK) >> MEM_HEADER_X_LOG2_ALIGN_SHIFT),
            (MEM_FLAG_T)((x & MEM_HEADER_X_FLAGS_MASK) >> MEM_HEADER_X_FLAGS_SHIFT));
      } else {
#if HEADERS_IN_DIRECT_ALIAS
         result = RTOS_ALIAS_NORMAL(h + 1);
#else
         result = h + 1;
#endif
      }
	  result = (char*)(((char*)result)- ((char*)h)) + h->space;

      return result;
   }
}


#endif
void *mem_lock(
   MEM_HANDLE_T handle)
{
   void *result;
   vcos_assert(handle != MEM_HANDLE_INVALID);
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      void *p;
      handle = from_wrap_handle(handle);
      p = ((MEM_WRAP_T *)mem_lock(handle))->p;
      mem_unlock(handle);
      return p;
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return get_small_alloc_data(handle);
   }
#endif
#ifdef BRCM_V3D_OPT
   if (is_big_alloc(handle)) {
      return lock_big_mem_internal(handle);
   }
#endif
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   result = lock_big_internal(handle);
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
   return result;
}

void mem_lock_multiple(
   void **pointers,
   MEM_HANDLE_OFFSET_T *handles,
   uint32_t n)
{
   uint32_t i;
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   for (i = 0; i < n; i++) {
      MEM_HANDLE_T handle = handles[i].mh_handle;
      uint32_t offset = handles[i].offset;
      if (handle == MEM_HANDLE_INVALID)
         pointers[i] = (void*)offset;
#ifdef MEM_WRAP_HACK
      else if (is_wrap_handle(handle)) {
         void *p;
         vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
         handle = from_wrap_handle(handle);
         p = ((MEM_WRAP_T *)mem_lock(handle))->p;
         mem_unlock(handle);
         pointers[i] = (char *)p + offset;
         vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
      }
#endif
#ifdef MEM_USE_SMALL_ALLOC_POOL
      else if (is_small_alloc(handle))
         pointers[i] = (char*)get_small_alloc_data(handle) + offset;
#endif
#ifdef BRCM_V3D_OPT
      else if (is_big_alloc(handle))
         pointers[i] = (char*)lock_big_mem_internal(handle) + offset;
#endif
      else
         pointers[i] = (char*)lock_big_internal(handles[i].mh_handle) + offset;
   }
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

void mem_lock_multiple_phys(
   void **pointers,
   MEM_HANDLE_OFFSET_T *handles,
   uint32_t n)
{
   uint32_t i;
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   for (i = 0; i < n; i++) {
      MEM_HANDLE_T handle = handles[i].mh_handle;
      uint32_t offset = handles[i].offset;
      if (handle == MEM_HANDLE_INVALID)
         pointers[i] = (void*)offset;
#ifdef MEM_WRAP_HACK
      else if (is_wrap_handle(handle)) {
         void *p;
         vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
         handle = from_wrap_handle(handle);
         p = ((MEM_WRAP_T *)mem_lock(handle))->phys;
         mem_unlock(handle);
         pointers[i] = (char *)p + offset;
         vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
      }
#endif
#ifdef MEM_USE_SMALL_ALLOC_POOL
      else if (is_small_alloc(handle))
         pointers[i] =  offset;
#endif
#ifdef BRCM_V3D_OPT
      else if (is_big_alloc(handle))
         pointers[i] = (char*)lock_big_mem_internal_phys(handle) + offset;
#endif
      else
      	{
      	char* tempaddr = (char*)lock_big_internal(handles[i].mh_handle);
		
         pointers[i] = (char*)khrn_hw_addr(tempaddr) + offset;
      	}
   }
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - It must be executable.
   - If it is locked, the previous lock must have been by mem_lock_exec and not
     mem_lock.

   Postconditions:

   If the MEM_HANDLE_T's size is 0:
   - NULL is returned.
   - The MEM_HANDLE_T is completely unchanged.

   Otherwise:
   - A pointer to the MEM_HANDLE_T's block of memory is returned. The pointer is
     valid until the MEM_HANDLE_T's lock count reaches 0. The pointer may only
     be used for code execution on the current core.
   - The MEM_HANDLE_T's lock count is incremented.
*/

void (*mem_lock_exec(
   MEM_HANDLE_T handle))()
{
   int slow;

#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif

   slow = (mem_need_icache_flush & current_cpu_bit()) != 0;
   if (slow) {
      vcos_quickslow_mutex_lock(&g_mgr.mutex);
   } else {
      vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   }

   {
      MEM_HEADER_T *h = get_header(handle);
      uint32_t x;

      vcos_assert(h->ref_count != 0);
      vcos_assert(h->x.flags & MEM_FLAG_EXECUTABLE);
      vcos_assert((h->x.lock_count == 0) || h->x.exec_locked);

      x = *(uint32_t *)&h->x;

      if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
         if (slow)
            vcos_quickslow_mutex_unlock(&g_mgr.mutex);
         else
            vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
         return NULL;
      }

      if (!(x & MEM_HEADER_X_LOCK_COUNT_MASK) && (mem_need_icache_flush & current_cpu_bit())) {
         vclib_icache_flush();
         mem_need_icache_flush &= ~current_cpu_bit();
      }

      x += 1 << MEM_HEADER_X_LOCK_COUNT_SHIFT;
      vcos_assert(x & MEM_HEADER_X_LOCK_COUNT_MASK);        // check we've not wrapped
      x |= MEM_HEADER_X_EXEC_LOCKED;
      x &= ~(MEM_FLAG_ABANDONED << MEM_HEADER_X_FLAGS_SHIFT);

      *(uint32_t *)&h->x = x;

      {
         void (*result)() = (void (*)())(uintptr_t)get_data_pointer(h,
            1 << ((x & MEM_HEADER_X_LOG2_ALIGN_MASK) >> MEM_HEADER_X_LOG2_ALIGN_SHIFT),
            (MEM_FLAG_T)((x & MEM_HEADER_X_FLAGS_MASK) >> MEM_HEADER_X_FLAGS_SHIFT));

         if (slow)
            vcos_quickslow_mutex_unlock(&g_mgr.mutex);
         else
            vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

         return result;
      }
   }
}

static RCM_INLINE void *lock_perma_big_internal(MEM_HANDLE_T handle)
{
   MEM_HEADER_T *h = get_header(handle);
   uint32_t x;

if(h==NULL)
{
return 0;
}
   vcos_assert(h->ref_count != 0);
   vcos_assert((h->x.lock_count == 0) || !h->x.exec_locked);

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return NULL;
   }

   x += MEM_LOCK_PERMA_THRESHOLD << MEM_HEADER_X_LOCK_COUNT_SHIFT;
   vcos_assert((x & MEM_HEADER_X_LOCK_COUNT_MASK) >= (MEM_LOCK_PERMA_THRESHOLD << MEM_HEADER_X_LOCK_COUNT_SHIFT));        // check we've not wrapped
   x &= ~(MEM_HEADER_X_EXEC_LOCKED | (MEM_FLAG_ABANDONED << MEM_HEADER_X_FLAGS_SHIFT));

   *(uint32_t *)&h->x = x;

   {
      void *result;
      if (x &
         (((-4 & MEM_HEADER_X_LOG2_ALIGN_MASK) << MEM_HEADER_X_LOG2_ALIGN_SHIFT) |
         ((MEM_FLAG_DIRECT | MEM_FLAG_COHERENT) << MEM_HEADER_X_FLAGS_SHIFT))) {
         result = get_data_pointer(h,
            1 << ((x & MEM_HEADER_X_LOG2_ALIGN_MASK) >> MEM_HEADER_X_LOG2_ALIGN_SHIFT),
            (MEM_FLAG_T)((x & MEM_HEADER_X_FLAGS_MASK) >> MEM_HEADER_X_FLAGS_SHIFT));
      } else {
         result = h + 1;
      }


      return result;
   }
}

/*
   A version of mem_lock which indicates that an aggresive compaction
   should not wait for the block to be unlocked.
*/

void *mem_lock_perma(
   MEM_HANDLE_T handle)
{
   void *result;
   vcos_assert(handle != MEM_HANDLE_INVALID);
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      void *p;
      handle = from_wrap_handle(handle);
      p = ((MEM_WRAP_T *)mem_lock(handle))->p;
      mem_unlock(handle);
      return p;
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return get_small_alloc_data(handle);
   }
#ifdef BRCM_V3D_OPT
   else if (is_big_alloc(handle)) {
      return get_big_alloc_data(handle);
   }
#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   result = lock_perma_big_internal(handle);
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
   return result;
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - If its size is not 0, it must be locked.

   Postconditions:

   If the MEM_HANDLE_T's size is 0:
   - The MEM_HANDLE_T is completely unchanged.

   Otherwise:
   - The MEM_HANDLE_T's lock count is decremented.

   Returns non-zero iff the locked handle was blocking an aggressive compaction
   and is now unlocked.
*/

static RCM_INLINE int unlock_big_internal(MEM_HEADER_T *h)
{
   uint32_t x;

if(h==NULL)
{
return 0;
}
   vcos_assert(h->ref_count != 0);
   vcos_assert(!h->size || (h->x.lock_count != 0));

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return 0;
   }

   x -= 1 << MEM_HEADER_X_LOCK_COUNT_SHIFT;

   *(uint32_t *)&h->x = x;

   if ((x & (MEM_FLAG_EXECUTABLE << MEM_HEADER_X_FLAGS_SHIFT)) &&
      !(x & (MEM_HEADER_X_LOCK_COUNT_MASK | MEM_HEADER_X_EXEC_LOCKED))) {
      need_icache_flush();
   }

   return !(x & MEM_HEADER_X_LOCK_COUNT_MASK) && (h == g_mgr.aggressive_locked);
}

static void resume_compaction(void)
{
   /* Called with the quick mutex held.
      Handover to the slow mutex, and wake the compaction thread. */
   MEM_HEADER_T *h = g_mgr.aggressive_locked;

   vcos_assert(h->x.lock_count == 0);

   /* Mark to prevent a timeout */
   g_mgr.aggressive_locked = (MEM_HEADER_T *)1;

   /* Relock for safety */
   h->x.lock_count = 1;
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
   if (h->x.lock_count == 1)
   {
      /* Still only one lock == success - wake the compactor with the mutex held */
      h->x.lock_count = 0;
      vcos_event_signal(&g_mgr.aggressive_owner_latch);
   }
   else
   {
      vcos_assert(h->x.lock_count > 1);
      /* Unlucky - somebody else has locked this buffer.
         Pass the baton - they can resume compaction. */
      g_mgr.aggressive_locked = h;
      h->x.lock_count--;
      vcos_quickslow_mutex_unlock(&g_mgr.mutex);
   }
}

#if HEADERS_IN_DIRECT_ALIAS
static void resume_compaction_slow(void)
{
   /* Called with the slow mutex held. Just wake the compaction thread. */
   MEM_HEADER_T *h = g_mgr.aggressive_locked;

   vcos_assert(h->x.lock_count == 0);

   /* Mark to prevent a timeout */
   g_mgr.aggressive_locked = (MEM_HEADER_T *)1;

   vcos_event_signal(&g_mgr.aggressive_owner_latch);
}
#endif

void mem_unlock(
   MEM_HANDLE_T handle)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      return;
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return;
   }
#ifdef BRCM_V3D_OPT
   if (is_big_alloc(handle)) {
      return;
   }
#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   if (unlock_big_internal(get_header(handle)))
      resume_compaction();
   else
      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

void mem_unlock_multiple(
   MEM_HANDLE_OFFSET_T *handles,
   uint32_t n)
{
   int resume = 0;

   uint32_t i;
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   for (i = 0; i < n; i++) {
      MEM_HANDLE_T handle = handles[i].mh_handle;
      if (handle != MEM_HANDLE_INVALID
#ifdef MEM_WRAP_HACK
         && !is_wrap_handle(handle)
#endif
#ifdef MEM_USE_SMALL_ALLOC_POOL
#ifdef BRCM_V3D_OPT
         && !is_small_alloc(handle) && !is_big_alloc(handle)
#else
         && !is_small_alloc(handle)
#endif
#endif
         )
      {
         resume |= unlock_big_internal(get_header(handle));
      }
   }

   if (resume)
      resume_compaction();
   else
      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

void mem_unlock_unretain_release_multiple(
   MEM_HANDLE_OFFSET_T *handles,
   uint32_t n)
{
   uint32_t i;
   MEM_HEADER_T *h;

#if HEADERS_IN_DIRECT_ALIAS
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
#else
   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
#endif

   for (i = 0; i < n; i++) {
      MEM_HANDLE_T handle = handles[i].mh_handle;

      if (handle == MEM_HANDLE_INVALID) { continue; }

#ifdef MEM_WRAP_HACK
      if (is_wrap_handle(handle)) {
         /* unlock, unretain are nops. just do release */
         handle = from_wrap_handle(handle);
         h = get_header(handle);
         vcos_assert(h->ref_count != 0);
         if (--h->ref_count == 0) {
            free_thing(handle, h);
         }
         continue;
      }
#endif

      h = get_header(handle);

      /* unlock */
#ifdef MEM_USE_SMALL_ALLOC_POOL
#ifdef BRCM_V3D_OPT
      if (!is_small_alloc(handle) && !is_big_alloc(handle))
#else
      if (!is_small_alloc(handle))
#endif
#endif
      {
         if (unlock_big_internal(h)) {
#if HEADERS_IN_DIRECT_ALIAS
            resume_compaction_slow();
            vcos_quickslow_mutex_lock(&g_mgr.mutex);
#else
            resume_compaction();
            vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
#endif
            h = get_header(handle);
         }
      }

      /* unretain */
      if (h->x.flags & MEM_FLAG_DISCARDABLE) {
         vcos_assert(h->ref_count != 0);
         vcos_assert(h->retain_count != 0);
         --h->retain_count;
      }

      /* release */
      vcos_assert(h->ref_count != 0);
      if (--h->ref_count == 0) {
         free_thing(handle, h);
      }
   }

#if HEADERS_IN_DIRECT_ALIAS
   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
#else
   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
#endif
}

static RCM_INLINE int unlock_perma_big_internal(MEM_HANDLE_T handle)
{
   MEM_HEADER_T *h = get_header(handle);
   uint32_t x;

   vcos_assert(h->ref_count != 0);
   vcos_assert(!h->size || (h->x.lock_count >= MEM_LOCK_PERMA_THRESHOLD));

   x = *(uint32_t *)&h->x;

   if (x & MEM_HEADER_X_SIZE_IS_ZERO) {
      return 0;
   }

   x -= MEM_LOCK_PERMA_THRESHOLD << MEM_HEADER_X_LOCK_COUNT_SHIFT;

   *(uint32_t *)&h->x = x;

   if ((x & (MEM_FLAG_EXECUTABLE << MEM_HEADER_X_FLAGS_SHIFT)) &&
      !(x & (MEM_HEADER_X_LOCK_COUNT_MASK | MEM_HEADER_X_EXEC_LOCKED))) {
      need_icache_flush();
   }

   return !(x & MEM_HEADER_X_LOCK_COUNT_MASK) && (h == g_mgr.aggressive_locked);
}

void mem_unlock_perma(
   MEM_HANDLE_T handle)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);
#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      return;
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return;
   }
#ifdef BRCM_V3D_OPT
   if (is_big_alloc(handle)) {
      return;
   }
#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);
   if (unlock_perma_big_internal(handle))
      resume_compaction();
   else
      vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.

   Postconditions:

   If the MEM_HANDLE_T is not a small handle, and the lock count is zero:
   - Sets MEM_FLAG_ABANDONED, which causes the data content to become undefined
    when the lock count reaches zero.
   - Sets MEM_FLAG_NO_INIT.

   Otherwise:
   - Does nothing.
*/

void mem_abandon(
   MEM_HANDLE_T handle)
{
   vcos_assert(handle != MEM_HANDLE_INVALID);

#ifdef MEM_WRAP_HACK
   if (is_wrap_handle(handle)) {
      return;
   }
#endif

#ifdef MEM_USE_SMALL_ALLOC_POOL
   if (is_small_alloc(handle)) {
      return;
   }
#ifdef BRCM_V3D_OPT
   if (is_big_alloc(handle)) {
      return;
   }
#endif
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
      if (h->x.lock_count == 0)
         h->x.flags |= (MEM_FLAG_ABANDONED | MEM_FLAG_NO_INIT);
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}

/*
   A discardable MEM_HANDLE_T with a retain count greater than 0 is
   considered retained and may not be discarded by the memory manager.

   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - It must be discardable.

   Postconditions:

   - 0 is returned if the size of the MEM_HANDLE_T's block of memory is 0,
     otherwise 1 is returned.
   - The retain count of the MEM_HANDLE_T is incremented.
*/

int mem_retain(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
	vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return 0;
}
      vcos_assert(h->ref_count != 0);
      vcos_assert(h->x.flags & MEM_FLAG_DISCARDABLE);
      ++h->retain_count;
      vcos_assert(h->retain_count);        // check we've not wrapped
      {
         int result = !h->x.size_is_zero;

         vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);

         return result;
      }
   }
}

/*
   Preconditions:

   - handle is a valid MEM_HANDLE_T.
   - It must be retained.

   Postconditions:

   - The retain count of the MEM_HANDLE_T is decremented.
*/

void mem_unretain(
   MEM_HANDLE_T handle)
{
#ifdef MEM_WRAP_HACK
   vcos_assert(!is_wrap_handle(handle));
#endif

   vcos_quickslow_mutex_lock_quick(&g_mgr.mutex);

   {
      MEM_HEADER_T *h = get_header(handle);
if(h==NULL)
{
	vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
return ;
}
      vcos_assert(h->ref_count != 0);
      vcos_assert(h->retain_count != 0);
      --h->retain_count;
   }

   vcos_quickslow_mutex_unlock_quick(&g_mgr.mutex);
}


/******************************************************************************
Legacy memory blocks API
******************************************************************************/

static void *set_heap_end(void *new_end)
{
   MEM_LINK_T *end = (MEM_LINK_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   MEM_LINK_T *curr;

   vcos_assert(((uintptr_t)new_end & 0x1f) == 0);

   if (new_end < (void *)end)
   {
      /* The heap is shrinking */
      curr = g_mgr.free.prev;
      if ((MEM_LINK_T *)((size_t)curr + curr->space) == end)
      {
         /* This is the last block, and it is free */
         uint32_t reduction = (uint8_t *)end - (uint8_t *)new_end;

         if (curr < (MEM_LINK_T *)new_end)
         {
            /* Truncate the current block */

            MEM_REMOVE_FREE_REGION(curr, curr->space);
            MEM_ADD_FREE_REGION(curr, curr->space - reduction);

            curr->space -= reduction;
            g_mgr.space -= reduction;

            new_end = (uint8_t *)curr + curr->space;
            vcos_assert(new_end == ((uint8_t *)g_mgr.base + g_mgr.space));
         }
         else if (curr == new_end)
         {
            /* Exact fit - remove from the free list */

            MEM_REMOVE_FREE_REGION(curr, curr->space);

            remove_link(curr);
            g_mgr.space -= reduction;

            new_end = curr;
            vcos_assert(new_end == ((uint8_t *)g_mgr.base + g_mgr.space));
         }
         else
         {
            /* Not enough space */
            new_end = NULL;
         }
      }
   }
   else if (new_end > (void *)end)
   {
      /* Create a new free block to fill the gap */
      uint32_t increase = (uint8_t *)new_end - (uint8_t *)end;

      g_mgr.space += increase;

      end->space = increase;
      free_internal(end);
   }

   return new_end;
}

/*
   Allocate a fixed-size block. The size of legacy blocks is a constant for the
   life of the memory subsystem (and may be a build-time constant). If no existing
   blocks are free, the relocatable heap high-water mark is lowered to make room
   for the  block, which may necessitate a compaction.

   Preconditions:

   - flags must specify a memory alias
     o MEM_FLAG_NORMAL
     o MEM_FLAG_COHERENT
     o MEM_FLAG_DIRECT
     o MEM_FLAG_L1_NONALLOCATING (VC4 only)
     No other flags are permitted.

   Postconditions:

   If the attempt succeeds:
   - A pointer to the legacy block is returned.
   - The relocatable heap high-water mark may have been lowered to accommodate the
     new block.

   If the attempt fails:
   - NULL (0) is returned.
*/

void *mem_alloc_legacy_ex(MEM_FLAG_T flags)
{
   uint8_t *legacy_base;
   void *block = NULL;


   
   if(mem_get_free_space() < (GLOBAL_POOL_SIZE/4))
   	{
   	khrn_hw_wait();
   	}
   /* Do not allow other flags */
   vcos_assert((flags & MEM_FLAG_ALIAS_MASK) == flags);

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

#ifdef TEST_ALLOC_FAIL
   if (should_check_fail())
   {
      vcos_quickslow_mutex_unlock(&g_mgr.mutex);
      return 0;
   }
#endif

   MEM_CLEAR_SANDBOX_REGIONS();

   legacy_base = (uint8_t *)g_mgr.base + g_mgr.space;

   if (g_mgr.legacy_free_map == 0)
   {
      /* Another legacy block is required */
      if (g_mgr.legacy_count < g_mgr.legacy_count_max)
      {
         /* Is there space? */
         uint8_t *new_heap_end = (uint8_t *)((uintptr_t)(legacy_base - g_mgr.legacy_size) &
                                             ~(g_mgr.legacy_align - 1));

         block = set_heap_end(new_heap_end);
         if (!block)
         {
            /* Try compaction */
            mem_compact_mode_t mode = need_compact_mode(legacy_base - new_heap_end, 1, MEM_COMPACT_ALL);
            if (mode != MEM_COMPACT_NONE)
            {
               compact_internal((mem_compact_mode_t)mode, legacy_base - new_heap_end, 1);
               block = set_heap_end(new_heap_end);
            }
            if (!block)
               assert(!"no space for legacy block");
         }
         if (block)
            g_mgr.legacy_count++;
      }
   }
   else
   {
      /* Use a free legacy block. Choose the block at the highest memory location
         to make it more likely that the relocatable heap can be re-expanded. */
      int idx;

      for (idx = 0; ((g_mgr.legacy_free_map & (1 << idx)) == 0); idx++) continue;

      vcos_assert((idx >= 0) && (idx < g_mgr.legacy_count));

      g_mgr.legacy_free_map &= ~(1u << idx);

      block = (void *)(legacy_base + (g_mgr.legacy_count - 1 - idx) * g_mgr.legacy_size);
   }

   MEM_SET_SANDBOX_REGIONS();

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);

   if (block)
   {
      // we always invalidate cached data here whatever the alias. Data from before the malloc will not be used again (and dirty data may be harmful)
      vclib_dcache_invalidate_range(block, g_mgr.legacy_size);

      switch (flags & MEM_FLAG_ALIAS_MASK)
      {
      case MEM_FLAG_COHERENT:
         return RTOS_ALIAS_COHERENT(block);
      case MEM_FLAG_DIRECT:
         return RTOS_ALIAS_DIRECT(block);
      case MEM_FLAG_L1_NONALLOCATING:
         return RTOS_ALIAS_L1_NONALLOCATING(block);
      default:
         return RTOS_ALIAS_NORMAL(block);
      }
   }
   return block;
}


/*
   Free a previously-allocated fixed-size block.

   Preconditions:

   - ptr must point to a block previously returned by mem_alloc_legacy.

   Postconditions:

   - The relocatable heap high-water mark may have been raised.
*/

void mem_free_legacy(void *ptr)
{

   vcos_assert(ptr && g_mgr.legacy_size);

#if HEADERS_IN_DIRECT_ALIAS
   ptr = RTOS_ALIAS_DIRECT(ptr);
#else
   ptr = RTOS_ALIAS_NORMAL(ptr);
#endif

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   MEM_CLEAR_SANDBOX_REGIONS();

   if (ptr && g_mgr.legacy_size)
   {
      uint8_t *legacy_base = (uint8_t *)g_mgr.base + g_mgr.space;
      int idx = ((uint8_t *)ptr - legacy_base) / g_mgr.legacy_size;
      if ((idx >= 0) &&
          (idx < g_mgr.legacy_count) &&
          ((uint8_t *)ptr == (legacy_base + idx * g_mgr.legacy_size)))
      {
         uint32_t bitmask = (1u << (g_mgr.legacy_count - 1 - idx));
         if ((g_mgr.legacy_free_map & bitmask) == 0)
         {
            uint8_t *heap_end = legacy_base;

            g_mgr.legacy_free_map |= bitmask;

            /* Can the heap high-water mark be raised? */
            bitmask <<= idx;

            while (g_mgr.legacy_free_map & bitmask)
            {
               /* The lowest legacy block is free, so it can be reclaimed */
               g_mgr.legacy_free_map &= ~bitmask;
               bitmask >>= 1;
               g_mgr.legacy_count--;
               if (g_mgr.legacy_count)
                  heap_end += g_mgr.legacy_size;
               else
                  heap_end = (uint8_t *)g_mgr.base + g_mgr.total_space;
            }

            if (heap_end != legacy_base)
               set_heap_end(heap_end); /* Cannot fail */
         }
         else vcos_assert(!"ptr is already a free legacy block");
      }
      else vcos_assert(!"ptr is not a valid legacy block");
   }
   else vcos_assert(!"ptr or legacy_block_size is zero");

   MEM_SET_SANDBOX_REGIONS();

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
}


/*
   If count_max is positive then it sets a maximum number of legacy blocks which
   can be allocated, otherwise the maximum possible (capped at 32) are allowed.

   Preconditions:

   - align must be a power of two.
   - There must be no allocated legacy blocks.

   Postconditions:

   If the attempt succeeds:
   - Returns the maximum number of legacy blocks, assuming no other allocations.

   If the attempt fails:
   - Returns -1.
*/

int mem_init_legacy(uint32_t size, uint32_t align, int count_max)
{
   uint32_t count = -1;

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   vcos_assert(is_power_of_2(align));

   align = _max(sizeof(MEM_HEADER_T), align);

   if (is_power_of_2(align) &&
       (g_mgr.legacy_count == 0))
   {
      g_mgr.legacy_size = round_up(size, align);
      g_mgr.legacy_align = align;
      g_mgr.legacy_count_max = _min(MEM_LEGACY_BLOCK_MAX, g_mgr.total_space / g_mgr.legacy_size);
      if (count_max >= 0)
          g_mgr.legacy_count_max = _min((uint32_t)count_max, g_mgr.legacy_count_max);

      count = (int)g_mgr.legacy_count_max;
   }

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);

   return count;
}


/*
   Preconditions:

   - None.

   Postconditions:

   - Returns the size of a legacy block.
*/

extern uint32_t mem_get_legacy_size(void)
{
   return g_mgr.legacy_size;
}


/*
   Check the internal consistency of the heap. A corruption will result in
   a failed assert, which will cause a breakpoint in a debug build.

   If MEM_FILL_FREE_SPACE is defined for the build, then free space is also
   checked for corruption; this has a large performance penalty, but can help
   to track down random memory corruptions.

   Note that defining MEM_AUTO_VALIDATE will enable the automatic validation
   of the heap at key points - allocation, deallocation and compaction.

   Preconditions:

   - None.

   Postconditions:

   - None.
*/

void mem_validate(void)
{
   vcos_quickslow_mutex_lock(&g_mgr.mutex);
   MEM_CLEAR_SANDBOX_REGIONS();

   mem_validate_internal(1);

   MEM_SET_SANDBOX_REGIONS();
   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
}


/******************************************************************************
Long-term lock owners' API
******************************************************************************/

/* Returns 1 on success, 0 on failure. */
int mem_register_callback(mem_callback_func_t func, uintptr_t context)
{
   vcos_assert(func != NULL);
   if (g_mgr.callbacks_count < MEM_MAX_CALLBACKS)
   {
      MEM_CALLBACK_INFO_T *callback = &g_mgr.callbacks[g_mgr.callbacks_count++];
      callback->func = func;
      callback->context = context;
      return 1;
   }

   return 0;
}

void mem_unregister_callback(mem_callback_func_t func, uintptr_t context)
{
   int i;
   for (i = 0; i < g_mgr.callbacks_count; i++)
   {
      MEM_CALLBACK_INFO_T *callback = &g_mgr.callbacks[i];
      if ((callback->func == func) &&
          (callback->context == context))
      {
         g_mgr.callbacks_count--;
         for (; i < g_mgr.callbacks_count; i++)
         {
            g_mgr.callbacks[i] = g_mgr.callbacks[i+1];
         }
         break;
      }
   }
}

/******************************************************************************
Movable memory helpers
******************************************************************************/

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
   MEM_HEADER_T *end, *pos;

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   pos = (MEM_HEADER_T *)g_mgr.base;

   while (pos < end) {
      uint32_t space = pos->space;

      if (space & 1) {
         space &= ~1;

         printf("%p: used %d (s %d, r %d, l %d) %s\n", pos, space, pos->size, pos->ref_count, pos->x.lock_count, pos->desc);
      } else
         printf("%p: free %d\n", pos, space);

      pos = (MEM_HEADER_T *)((size_t)pos + space);
   }

   vcos_assert(pos == end);

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);
}

void mem_print_small_alloc_pool_state()
{
#ifdef MEM_USE_SMALL_ALLOC_POOL
   uint32_t i;
   uint32_t count = 0;

   for (i = 0; i < g_smallallocs_num_handles; i++)
   {
#ifdef BRCM_V3D_OPT
      if(g_smallallocs[i] == NULL)
      	{
      	continue;
      	}
      MEM_HEADER_T *pos = &g_smallallocs[i]->header;
#else
      MEM_HEADER_T *pos = &g_smallallocs[i].header;
#endif
      uint32_t space = pos->space;

      if (!space) {
         printf("%x: smal %d (s %d, r %d, l %d) %s\n", pos, sizeof(MEM_SMALL_ALLOC_T), pos->size, pos->ref_count, pos->x.lock_count, pos->desc);
         count++;
      }
      // don't bother printing the free ones
   }

   printf("%d/%d small allocs used\n", count, MEM_SMALL_ALLOC_COUNT);

#else
   printf("No small alloc pool (MEM_USE_SMALL_ALLOC_POOL not defined).\n");
#endif
}

int mem_is_relocatable(const void *ptr)
{
   void *end = ((uint8_t *)g_mgr.base + g_mgr.space);
   void *base = g_mgr.base;

   return (ptr >= base) && (ptr < end);
}

void mem_get_stats(uint32_t *blocks, uint32_t *bytes, uint32_t *locked)
{
   MEM_HEADER_T *end, *pos;
   uint32_t r_blocks = 0, r_bytes = 0, r_locked = 0;

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   end = (MEM_HEADER_T *)((uint8_t *)g_mgr.base + g_mgr.space);
   pos = (MEM_HEADER_T *)g_mgr.base;

   while (pos < end) {
      uint32_t space = pos->space;

      vcos_assert(space != 0);

      if (space & 1) {
         space &= ~1;
         r_blocks++;
         r_bytes += space;
         if (pos->x.lock_count != 0)
            r_locked++;
      }

      pos = (MEM_HEADER_T *)((size_t)pos + space);
   }

   vcos_assert(pos == end);

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);

   *blocks = r_blocks;
   *bytes = r_bytes;
   *locked = r_locked;
}

uint32_t mem_get_total_space(void)
{
   return g_mgr.total_space;
}

uint32_t mem_get_free_space(void)
{
   uint32_t freespace = 0;
   MEM_LINK_T *free;

   vcos_quickslow_mutex_lock(&g_mgr.mutex);

   for (free = g_mgr.free.next; free->space; free = free->next)
   {
      freespace += free->space;
   }

   vcos_quickslow_mutex_unlock(&g_mgr.mutex);

   if (freespace)
      freespace -= sizeof(MEM_HEADER_T);

   return freespace;
}

uint32_t mem_get_free_space_unlocked(void)
{
   uint32_t freespace = 0;
   MEM_LINK_T *free;

   for (free = g_mgr.free.next; free->space; free = free->next)
   {
      freespace += free->space;
   }

   if (freespace)
      freespace -= sizeof(MEM_HEADER_T);

   return freespace;
}


void mem_assign_null_multiple(
   MEM_HANDLE_OFFSET_T *handles,
   uint32_t n)
{
   // TODO implement this properly
   uint32_t i;
   for (i = 0; i < n; i++)
      MEM_ASSIGN(handles[i].mh_handle, MEM_HANDLE_INVALID);
}

/******************************************************************************
Global initialisation helpers
******************************************************************************/

/*
   Start a shuffle thread which repeatedly performs aggressive compactions at the
   specified intervals.
*/

static int rtos_shuffle_mem_disable = 0;

#ifdef SHUFFLE_INTERVAL_MSECS

static void *rtos_common_mem_shuffle_task(void *arg)
{
   uint32_t interval_msecs = (uint32_t)arg;
   while (1)
   {
      vcos_sleep(interval_msecs);

      if(!rtos_shuffle_mem_disable)
      {
         logging_message(LOGGING_GENERAL, "shuffle start");

         mem_compact((mem_compact_mode_t)(MEM_COMPACT_AGGRESSIVE | MEM_COMPACT_SHUFFLE));
         logging_message(LOGGING_GENERAL, "shuffle end");
      }
   }
   return 0;
}
#endif

void rtos_common_mem_shuffle_enable(int enable)
{
   rtos_shuffle_mem_disable = !enable;
}

/***********************************************************
 * Name: rtos_common_mem_init
 *
 * Arguments:
 *       void
 *
 * Description: Initialise the relocatable heap.
 *
 * Returns: int ( < 0 is fail )
 *
 ***********************************************************/
int32_t rtos_common_mem_init( void )
{
   int32_t success = 0;
   uint32_t pool_size, handles_size;
   void *base, *handles_base;

   mem_get_default_partition(&base, &pool_size, &handles_base, &handles_size);

   // make a wild guess about the sizes needed
   if (!handles_size)
      handles_size = 4096*4;

   if (!base)
      base = malloc(pool_size);
   assert(base);
   if (!base)
      success = -1;

   if (!handles_base)
      handles_base = malloc(handles_size);
   assert(handles_base);
   if (!handles_base)
      success = -1;

   if ((success >= 0) &&
       !mem_init(base, pool_size, handles_base, handles_size))
      success = -1;

#ifdef SHUFFLE_INTERVAL_MSECS
   if (success >= 0)
      vcos_thread_create_classic(&g_shuffle_task, "mem_shfl", rtos_common_mem_shuffle_task, (void *)SHUFFLE_INTERVAL_MSECS,
                         g_shuffle_stack, sizeof(g_shuffle_stack), 10, 10, VCOS_START);
#endif

   if (success < 0)
   {
      if (base) free(base);
      if (handles_base) free(handles_base);
   }

   return success;
}

/* Include real implementations for situations where the macro definitions haven't worked */

#undef mem_alloc
#undef mem_resize
#undef mem_alloc_legacy

MEM_HANDLE_T mem_alloc(
   uint32_t size,
   uint32_t align,
   MEM_FLAG_T flags,
   const char *desc)
{
   return mem_alloc_ex(size, align, flags, desc, MEM_COMPACT_ALL);
}


int mem_resize(
   MEM_HANDLE_T handle,
   uint32_t size,
   mem_compact_mode_t mode)
{
   return mem_resize_ex(handle, size, MEM_COMPACT_ALL);
}


void *mem_alloc_legacy()
{
   return mem_alloc_legacy_ex(MEM_FLAG_NORMAL);
}

/******************************************************************************
API of memory access control using sandbox regions
  Users take the reponsebility of assuring thread-safety
******************************************************************************/
#ifdef MEM_ACCESS_CTRL

#define SANDBOX_NUM		  (8)

#if !defined(MEM_FREESPACE_PROTECT) || (MEM_FREESPACE_PROTECT == 0)
extern
void sdram_set_sandbox_region( const unsigned              start_addr_flags,   /* in     */
                               const unsigned              end_addr,           /* in     */
                               const unsigned              iregion             /* in     */ );
#endif

void mem_clr_accessctrl()
{
	// clear sandbox regions by setting region 0 to allow anyone to do anything, everywhere.
	sdram_set_sandbox_region( 0x0 | MEM_SSSR_PRIV_USER | MEM_SSSR_READ | MEM_SSSR_WRITE | MEM_SSSR_EXECUTE, ((128U << 20U) - 1), 0 );

	// Set sandboxes up in case MEM_FREESPACE_PROTECT is defined
	MEM_SET_SANDBOX_REGIONS();
} /* mem_clr_accesstrl */

void mem_set_accessctrl(MEM_ACCESSCTRL_BLOCK *blocks, uint32_t n)
{
	uint32_t start = 0;
	uint32_t end = 0;
	uint32_t iregion = 1; // sandbox 0 reserved to allow SUPERVISOR complete access to .text
	uint32_t mask = 0xFFFFFFFF << 5;
	uint32_t alaignment = 1 << 5;
	uint32_t i;

	// clear sandbox regions by setting region 0 to allow anyone to do anything, everywhere.
	sdram_set_sandbox_region( 0x0 | MEM_SSSR_PRIV_USER | MEM_SSSR_READ | MEM_SSSR_WRITE | MEM_SSSR_EXECUTE, ((128U << 20U) - 1), 0 );

	/**/
	for(i=0; (i<n)&&(iregion<SANDBOX_NUM); i++, iregion++)
	{
		start = (blocks + i)->start & mask;
		end = (blocks + i)->start + (blocks + i)->size - 1;
		end = (end & mask) + alaignment - 1;
		sdram_set_sandbox_region( (start | (blocks + i)->flags), end, iregion);
	} // for

	// ensure there are no spuriously permitted accesses
	while(iregion < SANDBOX_NUM)
	{
		// only SECURE code is permitted to do NOTHING with the first 32 bytes of memory
		sdram_set_sandbox_region(MEM_SSSR_PRIV_SECURE, (alaignment - 1), iregion++);
	} // while

	// setup sandbox 0 to allow SUPERVISOR complete access to .text
	end = (uint32_t)RTOS_ALIAS_NORMAL(g_mgr.base) - 1;
    end = (end & mask) + alaignment - 1;
    sdram_set_sandbox_region( 0x0 | MEM_SSSR_PRIV_SUPER | MEM_SSSR_READ | MEM_SSSR_WRITE | MEM_SSSR_EXECUTE, end, 0 );
} /* mem_set_accesstrl */

#endif //#ifdef MEM_ACCESS_CTRL
#endif
