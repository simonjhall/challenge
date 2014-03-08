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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>

#include "vcinclude/hardware.h"
#include "vcinclude/vcore.h"

#include "vcfw/rtos/rtos.h"
#include "vcfw/rtos/common/rtos_common_malloc.h"
#include "vcfw/rtos/common/dmalloc.h"
//#include "vcfw/logging/logging.h"

#ifndef DMALLOC_LOCK
#error DMALLOC_LOCK not defined - dmalloc.c should be #included by a platform-specific file
#endif

/******************************************************************************
Extern functions
******************************************************************************/

/******************************************************************************
Private typedefs
******************************************************************************/

static enum {ENOMEM, EINVAL} errno;

#ifndef _MAL_ALIGN
#define _MAL_ALIGN 4
#endif

#define ALIGN_MASK (_MAL_ALIGN-1)

/*
 * items are basic units of memory
 * "size" is the number of bytes of this item including the header.
 * It will always be aligned to MAL_ALIGN. If free, the size will have
 * the least-significant bit set.
 * "prev_size" is only used if debug level is > 1. It is the size of the
 * previous item. This gives the integrity checker more to work with.
 * "link" is used to chain free items together.
 */
typedef struct _item_header {
   unsigned size;    /*Size of this item*/
   unsigned prev_size;     /*Size of previous item (if debug level > 1)*/
   struct _item_header *fwd_link;   /*Free chain link (only if free)*/
   struct _item_header *prev_link;
} _ITEM_HEADER;

/*
 * Segments contain a chain of allocated or unallocated items.
 */
typedef struct _segment_header {
   struct _segment_header *prev;
   struct _segment_header *next;
   unsigned size;
   _ITEM_HEADER *last_item_ptr; /* Now does double duty as buckets pointer */
   _ITEM_HEADER first_item;
} _SEGMENT_HEADER;

#define _BUCKET_COUNT 13
#define _BLEN0   8      /*Largest item in bucket 0*/
#define _BLEN1   16     /*Largest item in bucket 1*/
#define _BLEN2   32     /*Largest item in bucket 2*/
#define _BLEN3   64     /*Largest item in bucket 3*/
#define _BLEN4   128    /*Largest item in bucket 4*/
#define _BLEN5   256    /*Largest item in bucket 5*/
#define _BLEN6   512    /*Largest item in bucket 6*/
#define _BLEN7   1024      /*Largest item in bucket 7*/
#define _BLEN8   2048      /*Largest item in bucket 8*/
#define _BLEN9   4096      /*Largest item in bucket 9*/
#define _BLEN10  8192      /*Largest item in bucket 10*/
#define _BLEN11  32000     /*Largest item in bucket 11*/
#define _BLEN12  0xFFFFFFFFUL /*Largest item in bucket 12*/

#define _ITEM_SIZE(ip) ((ip)->size&_SIZE_MASK)
#define _NEXT_ITEM(ip) ((_ITEM_HEADER*)((char *)(ip)+_ITEM_SIZE(ip)))
#define _PREV_ITEM(ip) ((_ITEM_HEADER*)((char *)(ip)-ip->prev_size))
#define _SET_FREE_FLAG(ip)  ((ip)->size |= 1)
#define _CLEAR_FREE_FLAG(ip)  ((ip)->size &= ~1)
#define _IS_FREE(ip)   (((ip)->size & 1)!=0)
#define _NEXT_FREE(ip) ((ip)->fwd_link)
#define _PREV_FREE(ip) ((ip)->prev_link)
#define _SIZE_MASK (~1)
#define _USING_DOUBLE_LINKS (_malloc_level > 0)

#define PUBLIC
#define FALSE 0
#define TRUE  1

#define ABORT assert(0)
#define corrupt(a,b,c) assert(0)
#define bad_free(msg, p) assert(0)
#define _rterr_printf(a, b) assert(0)

#define _USING_DOUBLE_LINKS (_malloc_level > 0)

#define PAGE_SIZE 4096   /*Unit of allocation from system*/

#define ALIGN_FILL 0xFEFEFEFE   /*Alignment filler*/
#define ALIGN(len) (((len)+(_MAL_ALIGN-1)) & ~(_MAL_ALIGN-1))
#define PAGE_ALIGN(len) (((len)+(PAGE_SIZE-1)) / PAGE_SIZE * PAGE_SIZE)

/*
 * Any value P returned by malloc is guaranteed to be  aligned such that
 *  (P+_alignment_offset) % _MAL_ALIGN is zero.
 * _alignment_offset is ordinarily zero.
 * But if, say, an memory allocator is being implemented on top of this
 * one (e.g., for Pascal), it may have its own header at the start of
 * each item and must be assured that the alignment of the data is correct.
 */

#ifndef _malloc_level
#define _alignment_offset 0
#endif

//#define DUMP_HEAP  /*Include heap dump stuff always for now*/

#ifndef _malloc_level
#define _malloc_level 1
#endif

/* default to first-fit, its usually faster*/
#ifndef _mwhbest_fit
#define _mwhbest_fit  0
#endif

/* default to prefer-low */
#ifndef _prefer_low
#define _prefer_low   1
#endif

#define _SIZE_MASK (~1)
/*
 * Given the address of an item, _NEXT_ITEM returns the next one
 */
#define _ITEM_SIZE(ip) ((ip)->size&_SIZE_MASK)
#define _NEXT_ITEM(ip) ((_ITEM_HEADER*)((char *)(ip)+_ITEM_SIZE(ip)))
#define _PREV_ITEM(ip) ((_ITEM_HEADER*)((char *)(ip)-ip->prev_size))
#define _SET_FREE_FLAG(ip)  ((ip)->size |= 1)
#define _CLEAR_FREE_FLAG(ip)  ((ip)->size &= ~1)
#define _IS_FREE(ip)   (((ip)->size & 1)!=0)
#define _NEXT_FREE(ip) ((ip)->fwd_link)
#define _PREV_FREE(ip) ((ip)->prev_link)

/*
 * Given a pointer p that is about to be freed or reallocated, return
 * reference to its header. Beware of FILL words we may have stuck in
 * for boundary alignment.
 */
#define header_of(p) (_ITEM_HEADER *)(\
                                      (\
                                       ((int *)(p))[-1]==ALIGN_FILL?(char *)p-sizeof(int):\
                                       (char *)p\
                                      ) - _mwhitem_header_size\
                                     )

#define check_item(ip){\
      if (_NEXT_ITEM(ip)->prev_size != _ITEM_SIZE(ip))\
         bad_free("Bad forward link",ip);\
      if (ip->prev_size != 0 && \
            _ITEM_SIZE(_PREV_ITEM(ip)) != ip->prev_size)\
         bad_free("Bad backward link",ip);}

/******************************************************************************
Private functions
******************************************************************************/

static _ITEM_HEADER *bucket_for (unsigned len, _ITEM_HEADER *buckets);
static _SEGMENT_HEADER *each_segment (_SEGMENT_HEADER *sp);
static _ITEM_HEADER *each_item (_ITEM_HEADER *ip, _SEGMENT_HEADER *sp);
static void remove_from_free_chain (_ITEM_HEADER *ip);
static void add_to_free_chain (dmalloc_pool_t pool, _ITEM_HEADER *ip);
static _ITEM_HEADER *search_free_chain (dmalloc_pool_t pool, unsigned len);
static int __find_heap_item (dmalloc_pool_t pool, _ITEM_HEADER *ip);
static void _find_heap_item(dmalloc_pool_t pool, void *p);

/******************************************************************************
Data segments
******************************************************************************/

PUBLIC _SEGMENT_HEADER *_mwhsegment_chain = NULL;   /*Chain of allocated segments*/
static unsigned _mwhitem_header_size;  /*Size of item header*/
static unsigned _mwhsegment_header_size;  /*Size of segment header*/
static char _mwh_corrupt_flag = FALSE;

/******************************************************************************
Static function definitions
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
int dmalloc_init(void)
{
   //set_malloc_level();
   if (_USING_DOUBLE_LINKS)
      _mwhitem_header_size = _max(_MAL_ALIGN,sizeof(unsigned) *2);
   else
      _mwhitem_header_size = _max(_MAL_ALIGN,sizeof(unsigned));
   _mwhsegment_header_size = _mwhitem_header_size*2+sizeof(_SEGMENT_HEADER);

   _mwhsegment_chain = NULL;
   _mwh_corrupt_flag = FALSE;

   return 0;
}

/*
 * Unix like break space heap allocation.
 *
 * If you generate an output section named ".heap" this code will use
 * it as the break space.  The nice thing about defining a ".heap"
 * section is it's bounds are known (it has a limit).
 *
 * Otherwise, the linker defined symbol _end will be used to mark the
 * base of an unbounded heap.  The problem here is that we don't know
 * where the real end of the heap is.
 *
 * This source allows the user application to define his heap in
 * several different ways.
 *
 * 1) The most common is to let the linker define the symbol "_end"
 *    to mark the beginning address of the heap.  The heap will then
 *    increment toward higher addresses with no bound.   However, you
 *    can call _set_brk_high_address() to specify a high bound.
 *
 * 2) Very early in your application, before malloc/new are ever
 *    called, you can call _set_brk(char *addr, unsigned len). This
 *    defines both the beginning address and the size of your heap.
 *
 *    You can create your own user function named __user_init_hook()
 *    which the startup will call before initializing the rest of the
 *    C/C++ runtime and before calling any C++ constructors.  This is
 *    the recommended place to call _set_brk().
 *
 * 3) BOUNDED HEAP (_fheap[] .. _eheap[]).  You can define a linker
 *    output section with the linker command file named ".heap" and
 *    the linker will automatically create symbols that mark it's
 *    beginning and end (named _fheap[] .. _eheap[]).  Example:
 *
 *    SECTIONS
 *    {
 * .heap ALIGN(16) :
 * {
 *     * (.heap)
 *     . += 32K;
 * }
 *    }
 *
 * Yet another version of malloc.
 * This one divides the free chain into buckets so as to achieve a
 * "good" fit.
 * It runs at one of 4 debug levels, depending on the setting of the
 * environment variable "MALLOC_LEVEL". If 0 or if MALLOC_LEVEL is not
 * defined, no integrety checking is performed.
 * Note that prev_size field in item header is not valid when MALLOC_LEVEL
 * is set to 0.
 *
 * Debug levels
 *    0        -- No integrity checking (4-byte overhead)
 *    1        -- No integrity checking but double links are used
 *          (8-byte overhead per item). This
 *          permits better coalescing of free space.
 *          This is the default.
 *    2        -- Check items as there are allocated and freed.
 *    3        -- Like level 2 but heap integrity is checked
 *          completely at every call to malloc and free
 *
 *
 * Heap is maintained as a series of "segments". On brain-damaged
 * architectures (of which Intel 286 is a prime example), there will be
 * one segment per 64K segment allocated. Other machines will typically
 * have just one segment that grows.
 *
 * This file is internal to malloc().  It is not intended for general use.
 *
 */



/*
 * Given a length, return pointer to bucket in which such an item would be
 * placed.
 */
static _ITEM_HEADER *bucket_for(unsigned len, _ITEM_HEADER *buckets)
{
   if (len <= _BLEN6)
      /*At this point it could go in buckets 0 thru 6*/
      if (len <= _BLEN3)
         /*At this point it could go in buckets 0 thru 3*/
         if (len <= _BLEN1)
            /*At this point it could go in buckets 0 or 1*/
            if (len <= _BLEN0)
               return buckets+0;
            else
               return buckets+1;
         else
            /*At this point it could go in buckets 2 or 3*/
            if (len <= _BLEN2)
               return buckets+2;
            else
               return buckets+3;
      else
         /*At this point it could go in buckets 4 thru 6*/
         if (len <= _BLEN4)
            return buckets+4;
         else if (len <= _BLEN5)
            return buckets+5;
         else
            return buckets+6;
   else
      /*At this point it could go in buckets 7 thru 12*/
      if (len <= _BLEN9)
         /*At this point it could go in buckets 7 thru 9*/
         if (len <= _BLEN7)
            return buckets+7;
         else
            /*At this point it could go in buckets 8 or 9*/
            if (len <= _BLEN8)
               return buckets+8;
            else
               return buckets+9;
      else
         /*At this point it could go in buckets 10 thru 12*/
         if (len <= _BLEN10)
            return buckets+10;
         else
            if (len <= _BLEN11)
               return buckets+11;
   return buckets+12;
}

static _SEGMENT_HEADER *each_segment(_SEGMENT_HEADER *sp)
{
   sp = sp ? sp->next : _mwhsegment_chain;
   if (!sp) return 0;

   if (sp->next && sp->next->prev != sp)
      corrupt("Segment links bad",0,sp);

   return sp;
}

static _ITEM_HEADER *each_item(_ITEM_HEADER *ip, _SEGMENT_HEADER *sp)
{
   unsigned size;

   ip = ip ? _NEXT_ITEM(ip) : &sp->first_item;
   if (!ip) return 0;

   if (ip > sp->last_item_ptr)
      corrupt("item outside segment",ip,sp);

   size = _ITEM_SIZE(ip);
   if (_NEXT_ITEM(ip) > sp->last_item_ptr)
      corrupt("size of item is garbage",ip,sp);
   if (size % _MAL_ALIGN)
      corrupt("Item size bad",ip,sp);
   if (_USING_DOUBLE_LINKS && size != 0 && _NEXT_ITEM(ip)->prev_size != size)
      corrupt("prev of next is bad",ip,sp);

   return ip;
}

static void remove_from_free_chain(_ITEM_HEADER *ip)
{
   ip->prev_link->fwd_link = ip->fwd_link;
   ip->fwd_link->prev_link = ip->prev_link;
   _CLEAR_FREE_FLAG(ip);
}

static void add_to_free_chain(dmalloc_pool_t pool, _ITEM_HEADER *ip)
{
   _ITEM_HEADER *next;
   _ITEM_HEADER *insert_after;
   next = _NEXT_ITEM(ip);
   /*
   * Coalesce with following item?
   */
   if (_IS_FREE(next)) {
      remove_from_free_chain(next);
      ip->size += next->size;
      if (_USING_DOUBLE_LINKS)
         _NEXT_ITEM(ip)->prev_size = ip->size;
   }
   /*
   * Coalesce with previous item?
   * We only do this if we're using double links
   */
   if (_USING_DOUBLE_LINKS && _IS_FREE(_PREV_ITEM(ip))) {
      _ITEM_HEADER *prev = _PREV_ITEM(ip);
      remove_from_free_chain(prev);
      prev->size += ip->size;
      _NEXT_ITEM(prev)->prev_size = prev->size;
      ip = prev;
   }
   _SET_FREE_FLAG(ip);
   insert_after = bucket_for(ip->size, pool->last_item_ptr);
   if (_mwhbest_fit) {
      /*
       * Keep free chains sorted by size; smallest first.
       * This guarantees best fit.
       */
      while (_NEXT_FREE(insert_after)->size != 0 &&
             _NEXT_FREE(insert_after)->size < ip->size) {
         insert_after = _NEXT_FREE(insert_after);
      }
   }
   else if (_prefer_low) {
      /*
       * Keep free chains sorted by address.
       */
      while (_NEXT_FREE(insert_after)->size != 0 &&
             _NEXT_FREE(insert_after) < ip) {
         insert_after = _NEXT_FREE(insert_after);
      }
   }
   ip->fwd_link = insert_after->fwd_link;
   ip->prev_link = insert_after;
   ip->fwd_link->prev_link = ip;
   insert_after->fwd_link = ip;
}

static _ITEM_HEADER *search_free_chain(dmalloc_pool_t pool, unsigned len)
/*
 *Return first item, if any on free chain that is at least "len" bytes
 */
{
   _ITEM_HEADER *ip;
   _ITEM_HEADER *buckets = pool->last_item_ptr;
   _ITEM_HEADER *bucketp = bucket_for(len, buckets);
again:
   for (ip=_NEXT_FREE(bucketp);ip->size != 0;ip = _NEXT_FREE(ip)) {
      if (_malloc_level > 1) {
         if (ip==0)
            corrupt("Free chain broken",bucketp,0);
         if (!_IS_FREE(ip))
            corrupt("Item on free chain not free",ip,0);
         if (_ITEM_SIZE(ip) != _NEXT_ITEM(ip)->prev_size)
            corrupt("Prev of next not consistent",ip,0);
      }
      if (_ITEM_SIZE(ip)>=len)
         return ip;
   }
   /*
   * Bucket empty, try next one.
   */
   if (++bucketp<buckets+_BUCKET_COUNT)
      goto again;
   return 0;
}

/***********************************************************
 * Name: dmalloc
 *
 * Arguments:
 *       dmalloc_pool_t pool
 *       size_t size
 *
 * Description: Routine to malloc memory
 *
 * Returns: void *- mem ptr
 *
 ***********************************************************/


void *dmalloc(dmalloc_pool_t pool, size_t size)
{
   unsigned adjust;
   int modulo;
   void *retval = NULL;
   _ITEM_HEADER *ip;
   unsigned item_size;
   size_t mlen;

   mlen = _max(sizeof(_ITEM_HEADER), ALIGN(size)+_mwhitem_header_size+_alignment_offset);

   /* Check for overflow due to large malloc's.  Addition of header info
   * can cause length to overflow zero to a small value.
   */

   if ((mlen >= size) && (pool != DMALLOC_POOL_INVALID))
   {
      DMALLOC_LOCK(pool->last_item_ptr + _BUCKET_COUNT)

#ifdef TEST_ALLOC_FAIL
      if (should_check_fail())
      {
         ip = 0;
      }
      else
#endif
      {
         /*
         * Scan free chain looking for item big enough.
         * Use first fit for now.
         */
         ip = search_free_chain(pool, mlen);
      }

      if (ip==0) {
         errno = ENOMEM;
         DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
      }
      else
      {
         item_size = _ITEM_SIZE(ip);
         remove_from_free_chain(ip);
         modulo = ((int)ip + _mwhitem_header_size + _alignment_offset) & ALIGN_MASK;
         adjust = modulo ? _MAL_ALIGN-modulo : 0;
         /*
         * If pointer is not properly aligned, then adjust things.
         */
         if (modulo) {
            item_size -= adjust;
            mlen -= adjust;
            /*
             * If using double links then extend size of previous
             * item so that this one falls on appropriate boundary
             */
            if (_USING_DOUBLE_LINKS) {
               _ITEM_HEADER *prev = _PREV_ITEM(ip);
               prev->size += adjust;
               ip = _NEXT_ITEM(prev);
               ip->size = item_size;
               _NEXT_ITEM(ip)->prev_size = item_size;
               adjust = 0;
            }
            else{
               int fillwords = adjust / sizeof (int);
               int i;
               int *p = (int*)((char *)ip+_mwhitem_header_size);
               for (i=0;i<fillwords;i++)
                  p[i] = ALIGN_FILL;
            }
         }
         if (mlen+sizeof(_ITEM_HEADER) < item_size) {
            _ITEM_HEADER *next;
            ip->size = mlen;
            next = _NEXT_ITEM(ip);
            next->size = item_size-mlen;
            if (_USING_DOUBLE_LINKS) {
               next->prev_size = mlen;
               _NEXT_ITEM(next)->prev_size = next->size;
            }
            add_to_free_chain(pool, next);
         }
         retval = (char *)ip+_mwhitem_header_size+adjust;
         if (_malloc_level > 2) {
            _find_heap_item(pool, retval);
         }
         DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
         if (_malloc_level > 1)
            memset((char *)ip+_mwhitem_header_size,0xAA,size);
      }
   }
   logging_message(LOGGING_MEMORY_VERBOSE, "dmalloc: size %d pool 0x%x ptr 0x%x", size,
                   (unsigned long)pool, (unsigned long)retval);
   return retval;
}

/*---------------------------------------------------------------------------
*/
static int __find_heap_item(dmalloc_pool_t pool, _ITEM_HEADER *ip) {
   char found = FALSE;
   _SEGMENT_HEADER *sp = 0;
   _ITEM_HEADER *ip_ = 0;
   while (sp = each_segment(sp), sp)
      while (ip_ = each_item(ip_, sp), ip_)
         if (ip == ip_)
         {
            found = TRUE; break; }
   return found;
}

/*---------------------------------------------------------------------------
*/
// This can be called from user code.
static void _find_heap_item(dmalloc_pool_t pool, void *p) {
   _ITEM_HEADER *ip = header_of(p);
   if (__find_heap_item(pool, ip) == FALSE) {
      _rterr_printf("find_heap_item: couldn't find this element! %x\n",ip);
   }
}


/***********************************************************
 * Name: dfree
 *
 * Arguments:
 *       dmalloc_pool_t pool
 *       void* ptr
 *
 * Description: Routine to free memory
 *
 * Returns: void
 *
 ***********************************************************/
void dfree(dmalloc_pool_t pool, void *ptr)
{
   char found;
   logging_message(LOGGING_MEMORY_VERBOSE, "dfree: pool 0x%x, ptr 0x%x", (unsigned long)pool, (unsigned long)ptr);
   if (ptr && (pool != DMALLOC_POOL_INVALID)) {
      _ITEM_HEADER *ip = header_of(ptr);
      DMALLOC_LOCK(pool->last_item_ptr + _BUCKET_COUNT)
      if (_malloc_level > 1) {
         check_item(ip);
         if (_malloc_level > 2) {
            //_check_heap();
            if (ip->size > _mwhitem_header_size)
               memset((char *)ptr,0xBB,ip->size-_mwhitem_header_size);
            found = FALSE;
            {
               _SEGMENT_HEADER *sp = 0;
               _ITEM_HEADER *ip_ = 0;
               while (sp = each_segment(sp), sp)
                  while (ip_ = each_item(ip_, sp), ip_)
                     if (ip == ip_) found = TRUE;
            }
            if (!found) {
               DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
               _rterr_printf("free(%x): couldn't find item in heap.\n",p);
               // Don't free something you couldn't find.
               // It just makes things worse.  Who knows what you
               // are pointing to!
               return;
            }
         }
      }
      if (_IS_FREE(ip))
         bad_free("Pointer already free",ptr);
      add_to_free_chain(pool, ip);
      DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
      if (_malloc_level > 2)
      {
         if (ip->size > _mwhitem_header_size)
            memset((char *)ptr,0xBB,ip->size-_mwhitem_header_size);
      }
   }
}




/***********************************************************
 * Name: drealloc
 *
 * Arguments:
 *       dmalloc_pool_t pool
 *       void *ptr,
 *       size_t size,
 *
 * Description: Routine to realloc memory
 *
 * Returns: void *- mem ptr
 *
 ***********************************************************/
void *drealloc(dmalloc_pool_t pool, void *ptr, size_t size) {
   unsigned new_size, old_size, total_inplace_size, next_old_size,
   next_new_size;
   _ITEM_HEADER *ip, *next_ip, *prev_ip;
   unsigned prev_old_size;
   void *p;

   if (ptr == NULL)
      return dmalloc(pool, size);
   if (size == 0) {
      dfree(pool, ptr);
      return 0;
   }
   // check for overflow
   if ((new_size = size+_mwhitem_header_size) < size)
      return NULL;
   new_size = _max(ALIGN(new_size),sizeof(_ITEM_HEADER));
   ip = header_of(ptr);
   DMALLOC_LOCK(pool->last_item_ptr + _BUCKET_COUNT)
   if (_malloc_level > 1)
      check_item(ip);
   if (_IS_FREE(ip))
      bad_free("Pointer already free",ptr);
   old_size = _ITEM_SIZE(ip);

   /* reference previous item only when _USING_DOUBLE_LINKS */
   if (_USING_DOUBLE_LINKS) {
      prev_ip = _PREV_ITEM(ip);
      prev_old_size = (_IS_FREE(prev_ip) ? _ITEM_SIZE(prev_ip) : 0);
   }
   next_ip = _NEXT_ITEM(ip);
   next_old_size = (_IS_FREE(next_ip) ? _ITEM_SIZE(next_ip) : 0);
   total_inplace_size = old_size + next_old_size;

   if (new_size <= old_size) {
      /* shrink item in-place */
      next_new_size = old_size - new_size;
      if (next_new_size >= sizeof(_ITEM_HEADER)) {
         next_ip = _NEXT_ITEM(ip);
         if (_USING_DOUBLE_LINKS || _IS_FREE(next_ip))
            next_ip->prev_size = next_new_size;
         ip->size = new_size;
         next_ip = _NEXT_ITEM(ip);
         /* set prev_size only when _USING_DOUBLE_LINKS */
         if (_USING_DOUBLE_LINKS)
            next_ip->prev_size = new_size;
         next_ip->size = next_new_size;
         add_to_free_chain(pool, next_ip);
      }
      DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
      return ptr;
   }
   else  if (new_size <= total_inplace_size /*implied: _IS_FREE(next_ip) */) {
      /* attempt to grow item in-place */

      /* grow this item into next item */
      next_new_size = total_inplace_size - new_size;
      if (next_new_size >= sizeof(_ITEM_HEADER)) {

         /* update size fields */
         ip->size = new_size;
         if (_USING_DOUBLE_LINKS || _IS_FREE(_NEXT_ITEM(next_ip)))
            _NEXT_ITEM(next_ip)->prev_size = next_new_size;

         remove_from_free_chain(next_ip);

         /* resize next item */
         next_ip = _NEXT_ITEM(ip);
         /* set prev_size only when _USING_DOUBLE_LINKS */
         if (_USING_DOUBLE_LINKS)
            next_ip->prev_size = new_size;
         next_ip->size = next_new_size;

         add_to_free_chain(pool, next_ip);
      }
      else {

         /* update size fields */
         ip->size = total_inplace_size;
         if (_USING_DOUBLE_LINKS || _IS_FREE(_NEXT_ITEM(next_ip)))
            _NEXT_ITEM(next_ip)->prev_size = total_inplace_size;

         remove_from_free_chain(next_ip);
      }

      DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)

      /* fill new space */
      if (_malloc_level > 1)
         memset((char *)ip + old_size,
                0xAA,
                new_size - old_size);
      return ptr;
   }
   else if (_USING_DOUBLE_LINKS && _alignment_offset == 0 &&
            new_size <= total_inplace_size + prev_old_size) {
      /* copy the block over the previous block. After this operation,
         we call realloc again, and this time we should fall into the
         "merge in place into next item" case handled above.  We don't
         handle the case where a user has set _alignment_offset, because
         I don't understand the handling of it well enough elsewhere in
         this module.  I think there may be bugs with it, actually. */
      unsigned tmp;
      remove_from_free_chain(prev_ip);
      tmp = prev_ip->prev_size;
      memcpy(prev_ip, ip, old_size);
      prev_ip->size = old_size;
      prev_ip->prev_size = tmp;
      ip = _NEXT_ITEM(prev_ip);
      ip->size = prev_old_size;
      ip->prev_size = old_size;
      next_ip->prev_size = ip->size;
      add_to_free_chain(pool, ip); /* this will merge with possible next_ip, also */
      DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)
      return(drealloc(pool, (char *)prev_ip+_mwhitem_header_size, size));
   }

   DMALLOC_UNLOCK(pool->last_item_ptr + _BUCKET_COUNT)

   /* when all else fails, malloc and copy */
   p = dmalloc(pool, size);
   if (p) {
      memcpy(p, ptr, old_size-_mwhitem_header_size);
      dfree(pool, ptr);
   }
   return p;
}

#define FORCE_ALIGNMENT(x) (x) = ((x) ? ALIGN_UP((x)-(4+GUARDWORDS)*sizeof(unsigned), RTOS_ALIGN_DEFAULT) + (4+GUARDWORDS)*sizeof(unsigned):(x))

dmalloc_pool_t dmalloc_create_pool(void *start, void *end)
{
   int current_base_int=0;
   int remaining_int=0;
   unsigned size;
   _SEGMENT_HEADER *sp;
   _ITEM_HEADER *buckets;
   int i;

   current_base_int = (int)start;
   FORCE_ALIGNMENT(current_base_int);
   remaining_int = (int)end - current_base_int;

   if (!current_base_int || remaining_int<=0) return NULL;

   int sz = remaining_int;
   if (sz>32*1024*1024)
   {
      // Check that the SDRAM settings allow for more than 32megabytes
      char *testdata= (char*) ALIAS_DIRECT(current_base_int);
      char *testdata2 = testdata+32*1024*1024;
      *testdata2=0xaa;
      *testdata=0x55;
      assert(*testdata2==0xaa);
   }

   current_base_int = (int) DMALLOC_ALIAS(current_base_int);
   // internal
   size = remaining_int;
   sp = (_SEGMENT_HEADER *)current_base_int;
   if (_malloc_level > 1)
      memset(sp,0xCC,size);
   sp->prev = 0;
   sp->size = size;
   sp->first_item.size =
      size-offsetof(_SEGMENT_HEADER,first_item)-_BUCKET_COUNT*sizeof(_ITEM_HEADER) - DMALLOC_LOCKBYTES;
   sp->first_item.prev_size = 0;
   sp->last_item_ptr = _NEXT_ITEM(&sp->first_item);
   // sp->last_item_ptr->size = 0; Obviated by bucket initialisation below
   /* set prev_size only when _USING_DOUBLE_LINKS */
   if (_USING_DOUBLE_LINKS)
      sp->last_item_ptr->prev_size = sp->first_item.size;

   /*
   * Initialize each bucket to be a circular list
   */
   buckets = sp->last_item_ptr;
   for (i = 0; i < _BUCKET_COUNT; i++)
   {
      buckets[i].fwd_link = &buckets[i];
      buckets[i].prev_link = &buckets[i];
      buckets[i].size = 0;
   }

   add_to_free_chain(sp, &sp->first_item);

   DMALLOC_INITLOCK(buckets + _BUCKET_COUNT)

   DMALLOC_LOCKCHAIN

   /* Finally, add to the ch-ch-chain of pools */
   sp->next = _mwhsegment_chain;
   if (_mwhsegment_chain) _mwhsegment_chain->prev = sp;
   _mwhsegment_chain = sp;

   DMALLOC_UNLOCKCHAIN

   return sp;
}

int dmalloc_delete_pool(dmalloc_pool_t pool, int force)
{
   _SEGMENT_HEADER *sp;

   DMALLOC_LOCKCHAIN

   /* This will be called rarely, so validate the argument */

   for (sp = _mwhsegment_chain; sp != pool; sp = sp->next)
   {
      if (sp == NULL)
      {
         DMALLOC_UNLOCKCHAIN
         return FALSE; /* Not a valid pool */
      }
   }

   if (!force)
   {
      _ITEM_HEADER *buckets = pool->last_item_ptr;
      int i;

      DMALLOC_LOCK(buckets + _BUCKET_COUNT)
      /* Check the pool is empty */
      for (i = 0; i < _BUCKET_COUNT; i++)
      {
         if (buckets[i].fwd_link != &buckets[i])
         {
            DMALLOC_UNLOCK(buckets + _BUCKET_COUNT)
            DMALLOC_UNLOCKCHAIN
            return FALSE;
         }
      }
      DMALLOC_UNLOCK(buckets + _BUCKET_COUNT)
      DMALLOC_UNINITLOCK(buckets + _BUCKET_COUNT)
   }

   /* Go ahead and delete it */

   if (pool->next)
      pool->next->prev = pool->prev;
   if (pool->prev)
      pool->prev->next = pool->next;
   else
      _mwhsegment_chain = pool->next;

   DMALLOC_UNLOCKCHAIN


   if (_malloc_level > 1)
      memset(sp, 0xDD, sp->size);

   return TRUE;
}

/***********************************************************
 * Name: dmalloc_find
 *
 * Arguments:
 *       const void **ptr
 *           - (input) pointer to the address we're looking for
 *             (output) if (return > 0) then a pointer to the
 *                      result of the corresponding dmalloc(),
 *                      otherwise undefined
 *
 * Description: Find the base pointer and approximate size of
 *             the allocation containing the pointer at *ptr.
 *
 * Returns: int - size of allocated block, 0 if not found,
 *                or -1 if it's an illegal address
 *
 ***********************************************************/

int dmalloc_find(dmalloc_pool_t pool, const void **ptr)
{
   uintptr_t value = (uintptr_t)*ptr;
   const _SEGMENT_HEADER *sp;

   if (pool == DMALLOC_POOL_ANY)
   {
      DMALLOC_LOCKCHAIN
      for (sp = _mwhsegment_chain; sp != NULL; sp = sp->next)
      {
         if ((uintptr_t)sp <= value && value < (uintptr_t)sp + sp->size)
            break;
      }
      DMALLOC_UNLOCKCHAIN
   }
   else
   {
      if ((uintptr_t)pool <= value && value < (uintptr_t)pool + pool->size)
         sp = pool;
      else
         sp = NULL;
   }

   if (sp != NULL)
   {
      const _ITEM_HEADER *ip = &sp->first_item;

      DMALLOC_LOCK(sp->last_item_ptr + _BUCKET_COUNT)

      do
      {
         int size = _ITEM_SIZE(ip);

         if ((uintptr_t)ip <= value && value < (uintptr_t)ip + size)
         {
            int adjust = _USING_DOUBLE_LINKS ? 0 : ((_MAL_ALIGN - ((int)ip + _mwhitem_header_size + _alignment_offset)) & ALIGN_MASK);
            uintptr_t block = (uintptr_t)ip + _mwhitem_header_size + adjust;
            assert(ip == header_of(block));
            size -= _mwhitem_header_size + adjust;
            if (_IS_FREE(ip) == 0 && value >= block)
            {
               DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)
               *ptr = (const void *)block;
               return size;
            }

            /* within an item, but not within the data part */
            return -1;
         }
         ip = _NEXT_ITEM(ip);
      } while (ip != NULL);

      DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)

      /* within a segment, but not within an allocated block */
      return -1;
   }

   return 0;
}

/***********************************************************
 * Name: dmalloc_scan
 *
 * Arguments:
 *       const void *(*)(const void *,int,void *)
 *           - allocated and free block check callbacks
 *
 * Description: Scan through the heap looking for corrupt
 *             data.  Each block is passed to an external
 *             checking function (if non-null) for further
 *             testing.
 *
 * Returns: const void * - address of first fault found
 *
 ***********************************************************/
const void *dmalloc_scan(dmalloc_pool_t pool,
   const void *(*check_allocated)(const void *base, int length, void *handle),
   const void *(*check_free)(const void *base, int length, void *handle),
   void *handle)
{
   const _SEGMENT_HEADER *sp;

   DMALLOC_LOCKCHAIN

   sp = ((pool != DMALLOC_POOL_ANY) ? pool : _mwhsegment_chain);

   while (sp != NULL)
   {
      const _ITEM_HEADER *ip, *next;
      int prev_size;

      if (sp->size < sizeof(*sp)
        || ((uintptr_t)sp->last_item_ptr !=
            ((uintptr_t)sp + (sp->size & ~1) - sizeof(_ITEM_HEADER) * _BUCKET_COUNT - DMALLOC_LOCKBYTES))
        || sp->next && sp->next->prev != sp
        || sp->prev && sp->prev->next != sp)
      {
         DMALLOC_UNLOCKCHAIN
         return sp;
      }

      DMALLOC_UNLOCKCHAIN

      DMALLOC_LOCK(sp->last_item_ptr + _BUCKET_COUNT)
      ip = &sp->first_item;
      prev_size = 0;
      while (ip != NULL)
      {
         const void *result = NULL;
         int size = _ITEM_SIZE(ip);
         int adjust = _mwhitem_header_size + (_USING_DOUBLE_LINKS ? 0 : ((_MAL_ALIGN - ((int)ip + _mwhitem_header_size + _alignment_offset)) & ALIGN_MASK));
         const char *block = (const char *)ip + adjust;
         assert(ip == header_of(block));

         if (ip == sp->last_item_ptr)
            break;

         next = _NEXT_ITEM(ip);

         if (next > sp->last_item_ptr || next < ip
            || (size % _MAL_ALIGN) != 0
            || size < adjust
            || _USING_DOUBLE_LINKS && prev_size != 0 && ip->prev_size != prev_size)
         {
            DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)
            return ip;
         }

         if (_IS_FREE(ip))
         {
            if (check_free != NULL)
               result = check_free(block, size - adjust, handle);
         }
         else
            if (check_allocated != NULL)
               result = check_allocated(block, size - adjust, handle);

         if (result)
         {
            DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)
            return result;
         }

         prev_size = size;
         ip = next;
      }

      DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)

      DMALLOC_LOCKCHAIN
      sp = sp->next;
   }

   DMALLOC_UNLOCKCHAIN

   return NULL;
}


/***********************************************************
 * Name: dmalloc_get_free_mem
 *
 * Arguments:
 *       const dmalloc_pool_t pool
 *           - allocated and free block check callbacks
 *
 * Description: Determine the amount of free memory on the
 *             heap.  Also performssome sanity checks
 *             (failures presented as asserts).
 *
 * Returns: uint32_t - amount of free memory on the heap
 *
 ***********************************************************/
uint32_t dmalloc_get_free_mem( const dmalloc_pool_t pool )
{
   const _SEGMENT_HEADER *sp = pool;
   _ITEM_HEADER *buckets = pool->last_item_ptr;
   _ITEM_HEADER *ip;
   uint32_t total_free = 0;
   int i;

   DMALLOC_LOCK(sp->last_item_ptr + _BUCKET_COUNT)

   for (i = 0; i < _BUCKET_COUNT; i++)
   {
      for (ip=_NEXT_FREE(&buckets[i]);ip->size != 0;ip = _NEXT_FREE(ip)) {
         if (_malloc_level > 1) {
            if (ip==0)
               corrupt("Free chain broken",&buckets[i],0);
            if (!_IS_FREE(ip))
               corrupt("Item on free chain not free",ip,0);
            if (_ITEM_SIZE(ip) != _NEXT_ITEM(ip)->prev_size)
               corrupt("Prev of next not consistent",ip,0);
         }
         total_free += _ITEM_SIZE(ip);
      }
   }

   DMALLOC_UNLOCK(sp->last_item_ptr + _BUCKET_COUNT)

   return total_free;
}
