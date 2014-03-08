/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
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

/* Project includes */
#include "vcinclude/common.h"
#include "vcfw/rtos/rtos.h"
#include "interface/vcos/vcos.h"
#include "vcfw/rtos/common/rtos_common_mem.h"
//#include "vcfw/logging/logging.h"

#ifdef __CC_ARM
#include "vcinclude/vc_asm_ops.h"
#endif

#include "vc_pool.h"
#include "vc_pool_private.h"


/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#define RELEASE_EVENT 1
#define POOL_OBJECT_LOGGING 0


/******************************************************************************
Private functions in this file.
Define as static.
******************************************************************************/

static void lock_pool( VC_POOL_T *pool );
static void unlock_pool( VC_POOL_T *pool );
static VC_POOL_OBJECT_T *alloc_object( VC_POOL_T *pool, size_t size );
static void free_object( VC_POOL_OBJECT_T *object );
static void create_event( VC_POOL_T *pool );
static void destroy_event( VC_POOL_T *pool );
static int  wait_event( VC_POOL_T *pool, uint32_t usecs );
static void signal_event( VC_POOL_T *pool );

#if POOL_OBJECT_LOGGING
static void pool_object_logging( const char *fname, VC_POOL_OBJECT_T *object );
#else
#   define pool_object_logging(a,b) do {} while(0)
#endif


/******************************************************************************
Static data defined in this file. Static data may also be defined within
functions or at the head of a group of functions that share data. Also
for multi-file modules, public data which is shared within the module.
******************************************************************************/

/** Maintain a linked list of all the vc_pools in the system, for debugging
  */
static RTOS_LATCH_T pool_list_latch = rtos_latch_unlocked();
static VC_POOL_T *vc_pool_list;


/******************************************************************************
 Global Functions
 *****************************************************************************/

/**
 * create a fixed pool of relocatable objects.
 *
 * return (opaque) pointer to the newly created pool, or NULL if
 * there was insufficient memory.
 *
 * @param size     Size of each sub-object
 * @param num      Number of sub-objects
 * @param align    Alignment of sub-objects
 * @param flags    Flags
 * @param name     A name for this pool
 * @param overhead Allocate additional space in the non-moveable heap
 *
 * If flags include VC_POOL_FLAGS_SUBDIVISIBLE we get a single relocatable
 * memory block large enough for all 'n' objects; it can either be used
 * as a single block, or divided up into 'n' of them.
 * -------------------------------------------------------------------- */
VC_POOL_T *
vc_pool_create( size_t size, uint32_t num, uint32_t align, VC_POOL_FLAGS_T flags, const char *name, uint32_t overhead_size )
{
   int i;
   int mem_flags = MEM_FLAG_NO_INIT;

   vcos_assert(size != 0);
   vcos_assert(num != 0);
   vcos_assert(name);

   overhead_size = (overhead_size+OVERHEAD_ALIGN-1) & ~(OVERHEAD_ALIGN-1);

   // allocate and zero main struct
   int alloc_len = sizeof(VC_POOL_T) + num * sizeof(VC_POOL_OBJECT_T) + num * overhead_size;
   VC_POOL_T *pool = (VC_POOL_T*)rtos_prioritymalloc( alloc_len,
                                          RTOS_ALIGN_DEFAULT,
                                          RTOS_PRIORITY_UNIMPORTANT,
                                          "vc_pool" );
   if ( !pool ) return NULL; // failed to allocate pool
   memset( pool, 0, alloc_len );

   // array of pool objects
   pool->object = (VC_POOL_OBJECT_T *)((unsigned char *)pool + sizeof(VC_POOL_T));

   // initialise
   pool->magic = POOL_MAGIC;
   pool->latch = rtos_latch_unlocked();

   if ( flags & VC_POOL_FLAGS_DIRECT )
      mem_flags |= MEM_FLAG_DIRECT;

   if ( flags & VC_POOL_FLAGS_COHERENT )
      mem_flags |= MEM_FLAG_COHERENT;

   if ( flags & VC_POOL_FLAGS_HINT_PERMALOCK )
      mem_flags |= MEM_FLAG_HINT_PERMALOCK;

   if ( align == 0 ) align = 32; // minimum 256-bit aligned
   vcos_assert( _count(align) == 1 ); // must be power of 2
   pool->alignment = align;
   pool->overhead = (uint8_t*)(pool+1) + num*sizeof(VC_POOL_OBJECT_T);
   pool->overhead_size = overhead_size;
   pool->name = name;

   pool->max_objects = num;
   pool->pool_flags = flags;

   if ( flags & VC_POOL_FLAGS_SUBDIVISIBLE ) {

      // a single mem_handle, shared between objects
      uint32_t rounded_size = (size + align - 1) & ~(align - 1);
      pool->mem = mem_alloc( rounded_size, align, (MEM_FLAG_T)mem_flags, name );
      if ( pool->mem == MEM_INVALID_HANDLE ) {
         // out of memory... clean up nicely and return error
         rtos_priorityfree( pool );
         return NULL;
      }

      pool->nobjects = 0;
      pool->object_size = 0;
      pool->max_object_size = rounded_size;
   } else {

      // bunch of individual objects

      for (i=0; i<num; i++) {
         MEM_HANDLE_T mem = mem_alloc( size, align, (MEM_FLAG_T)mem_flags, name );
         pool->object[i].mem = mem;
         // all ->offset fields are 0 from the previous memset
         if ( mem == MEM_INVALID_HANDLE ) {
            // out of memory... clean up nicely and return error
            while (i > 0)
               mem_release( pool->object[--i].mem );
            rtos_priorityfree( pool );
            return NULL; // failed to allocate pool
         }
         // pointer to 'overhead' memory for this entry
         pool->object[i].overhead = pool->overhead + i*pool->overhead_size;
      }

      pool->mem = MEM_INVALID_HANDLE;
      pool->nobjects = num;
      pool->object_size = size;
      pool->max_object_size = size;
   }

   create_event( pool );

   // link into global list
   rtos_latch_get(&pool_list_latch);
   pool->next = vc_pool_list;
   vc_pool_list = pool;
   rtos_latch_put(&pool_list_latch);

   // done
   return pool;
}

/* ----------------------------------------------------------------------
 * destroy a pool of objects.  if none of the pool objects are in use,
 * the destroy will happen immediately.  otherwise, we will sleep for
 * up to 'timeout' microseconds waiting for the objects to be released.
 *
 * timeout of 0xffffffff means 'wait forever';  0 means 'do not wait'.
 *
 * returns 0 on success, -1 on error (ie. even after 'timeout' there
 * were still objects in use)
 * -------------------------------------------------------------------- */
int32_t
vc_pool_destroy( VC_POOL_T *pool, uint32_t timeout )
{
   int i;
   vcos_assert( pool->magic == POOL_MAGIC );

   // wait for all objects to become free
   for (;;) {
      lock_pool( pool );
      if ( pool->allocated == 0 )
         break;
      unlock_pool( pool );
      if ( wait_event(pool,timeout) )
         return -1; // timed out
   }

   if ( pool->mem != MEM_INVALID_HANDLE ) {

      // just a single memory object to free
      mem_release( pool->mem );

   } else {

      // release individual pool entries back to mempool
      for (i=0; i<pool->nobjects; i++)
         mem_release( pool->object[i].mem );

   }

   // remove from the global list
   rtos_latch_get(&pool_list_latch);
   VC_POOL_T **pp = &vc_pool_list;
   while (*pp != pool)
   {
      pp = &((*pp)->next);
   }
   vcos_assert(*pp);
   *pp = pool->next;
   rtos_latch_put(&pool_list_latch);


   // kill the pool struct
   pool->magic = 0;
   destroy_event( pool );
   rtos_priorityfree( pool );
   return 0;
}

/* ----------------------------------------------------------------------
 * allocate an object from the pool.  if there are free objects in
 * the pool, the allocation will happen immediately.  otherwise, we
 * will sleep for up to 'timeout' microseconds waiting for an object
 * to be released.
 *
 * timeout of 0xffffffff means 'wait forever';  0 means 'do not wait'.
 *
 * returns (opaque) pointer to the newly allocated object, or NULL on
 * failure (ie. even after 'timeout' there were still no free objects)
 *
 * a newly allocated object will have its reference count initialised
 * to 1.
 * -------------------------------------------------------------------- */
VC_POOL_OBJECT_T *
vc_pool_alloc( VC_POOL_T *pool, size_t size, uint32_t timeout )
{
   VC_POOL_OBJECT_T *object;
   vcos_assert( pool->magic == POOL_MAGIC );

   // wait for an object to become free
   for (;;) {
      lock_pool( pool );
      object = alloc_object( pool, size );
      unlock_pool( pool );

      if ( object )
         break;

      if ( wait_event(pool,timeout) )
      {
         pool->alloc_fails++;
         return NULL; // timed out
      }
   }

   pool_object_logging( __FUNCTION__, object );
   return object;
}

/* ----------------------------------------------------------------------
 * Return the maximum size of object which can ever be allocated from the pool.
 * -------------------------------------------------------------------- */
size_t vc_pool_max_object_size( VC_POOL_T *pool)
{
   vcos_assert( pool->magic == POOL_MAGIC );
   return pool->object_size;
}

/* ----------------------------------------------------------------------
 * return the memory handle associated with the object
 * -------------------------------------------------------------------- */
MEM_HANDLE_T
vc_pool_mem_handle( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   return object->mem;
}

/* ----------------------------------------------------------------------
 * if the object was allocated from a subdividable pool, then
 * the base address of the object may be offset from the base address
 * of the underlying MEM_HANDLE_T
 * -------------------------------------------------------------------- */
int
vc_pool_offsetof( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   return object->offset;
}

/* ----------------------------------------------------------------------
 * increase the refcount of a pool object
 *
 * return the new refcount
 * -------------------------------------------------------------------- */
int
vc_pool_acquire( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   lock_pool( object->pool );
   int refcount = ++object->refcount;
   pool_object_logging( __FUNCTION__, object );
   unlock_pool( object->pool );
   return refcount;
}

/* ----------------------------------------------------------------------
 * decrease the refcount of a pool object.  if the refcount hits 0,
 * the object is treated as being returned to the pool as free; update
 * pool struct accordingly (and potentially wakeup any sleepers)
 *
 * return the new refcount
 * -------------------------------------------------------------------- */
int
vc_pool_release( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   lock_pool( object->pool );
   int refcount = --object->refcount;
   pool_object_logging( __FUNCTION__, object );
   if ( refcount == 0 ) {
      free_object( object );
      signal_event( object->pool );
   }
   unlock_pool( object->pool );
   return refcount;
}

/* ----------------------------------------------------------------------
 * the object was allocated from the relocatable mempool; provide a
 * lightweight api call to lock the memory in place before use.
 *
 * returns a pointer to the locked-in-memory data.
 * -------------------------------------------------------------------- */
void *
vc_pool_lock( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   return mem_lock( object->mem );
}

/* ----------------------------------------------------------------------
 * the object was allocated from the relocatable mempool; provide a
 * lightweight api call to unlock the memory.  after calling this, any
 * pointers obtained via vc_lock_pool, above, will be invalid.
 * -------------------------------------------------------------------- */
void
vc_pool_unlock( VC_POOL_OBJECT_T *object )
{
   vcos_assert( object->magic == OBJECT_MAGIC );
   mem_unlock( object->mem );
}

int
vc_pool_valid( const VC_POOL_T *pool )
{
   return pool && pool->magic == POOL_MAGIC;
}

/** Return the overhead allocated in each object header.
  */
void *
vc_pool_overhead( VC_POOL_OBJECT_T *object )
{
   if ( !object->pool->overhead_size ) {
      vcos_assert(0); // no overhead allocated at pool creation time!
      return NULL;
   }
   return object->overhead;
}

/* ----------------------------------------------------------------------
 * Return the maximum number of objects which can be put in the pool
 * -------------------------------------------------------------------- */

int
vc_pool_max_objects( const VC_POOL_T *pool )
{
   return pool->max_objects;
}

/* ----------------------------------------------------------------------
 * Return the size of the largest single object that could ever be allocated from the pool.
 * Equates to the pool size for subdivisible pools, or the individual object size for 
 * non-subdivisible pools.
 * -------------------------------------------------------------------- */
size_t vc_pool_pool_size( const VC_POOL_T *pool )
{
   return pool->max_object_size;
}

/******************************************************************************
 Static  Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * coarse-grained locking of the entire pool
 * -------------------------------------------------------------------- */
static void
lock_pool( VC_POOL_T *pool )
{
   rtos_latch_get( &pool->latch );
}

static void
unlock_pool( VC_POOL_T *pool )
{
   rtos_latch_put( &pool->latch );
}

/* ----------------------------------------------------------------------
 * find a free object in the given pool.  returns pointer to the
 * object, or NULL if the pool is full.
 *
 * must be called with pool locked
 * -------------------------------------------------------------------- */
static VC_POOL_OBJECT_T *
alloc_object( VC_POOL_T *pool, size_t size )
{
   VC_POOL_OBJECT_T *object = NULL;
   int i;

   // is this the first subobject?
   if ((pool->allocated == 0) && (pool->mem != MEM_INVALID_HANDLE))
   {
      uint32_t rounded_size = (size + pool->alignment - 1) & ~(pool->alignment - 1);
      int num = pool->max_object_size/rounded_size;
      if (num == 0) // pool too small for object
         return NULL;
      if (num > pool->max_objects)
         num = pool->max_objects;
      if(pool->pool_flags & VC_POOL_FLAGS_UNEQUAL_SUBDIVISION)
      {
         pool->object_size = rounded_size;
         if(num != pool->max_objects)
            num++;
         pool->final_object_size = pool->max_object_size - (rounded_size * (num-1));
      }
      else
      {
         pool->object_size = (pool->max_object_size / num) & ~(pool->alignment - 1);
         vcos_assert(pool->object_size >= size);
      }
      
      // divide up the pool
      for (i=0; i<num; i++) {
         VC_POOL_OBJECT_T *obj = &pool->object[i];
         obj->mem = pool->mem;
         obj->offset = i * pool->object_size;
         obj->overhead = pool->overhead + i*pool->overhead_size;
      }
      pool->nobjects = num;
   }

   if ( size > pool->object_size )
      return NULL;

   // are there any free objects?
   if ( pool->allocated >= pool->nobjects )
      return NULL;

   // at this point there is at least one free object in the pool. find it.
   // could be done more efficiently, although expected use case is for
   // small ->nobjects.
   for (i=0; i<pool->nobjects; i++) {
      object = &pool->object[i];
      if ( object->refcount == 0 )
      {
         if((pool->pool_flags & VC_POOL_FLAGS_UNEQUAL_SUBDIVISION) &&
            i == (pool->nobjects-1) && 
            size > pool->final_object_size)
            //Only the last object left, and that is smaller than we require.
            return NULL;
            
         break;
      }
   }
   vcos_assert( object->refcount == 0 );

   object->magic = OBJECT_MAGIC;
   object->pool = pool;
   object->refcount = 1;
   pool->allocated++;
   vcos_assert( pool->allocated <= pool->nobjects );

   return object;
}

static void
free_object( VC_POOL_OBJECT_T *object )
{
   object->magic = 0;
   object->pool->allocated--;
   if ((object->pool->allocated == 0) ||
       (object->pool->mem == MEM_HANDLE_INVALID))
      mem_abandon(object->mem);
   vcos_assert( object->pool->allocated >= 0 );
}

/* ----------------------------------------------------------------------
 * A thin veneer over the VCOS event_flags synchronisation object.
 * -------------------------------------------------------------------- */
#if VCOS_HAVE_RTOS

static void
create_event( VC_POOL_T *pool )
{
   vcos_event_flags_create( &pool->event, "vc_pool event" );
}

static void
destroy_event( VC_POOL_T *pool )
{
   vcos_event_flags_delete( &pool->event );
}

static int
wait_event( VC_POOL_T *pool, uint32_t usecs )
{
   VCOS_STATUS_T status;
   VCOS_UNSIGNED timeout;
   VCOS_UNSIGNED event;

   if (pool->callback)
   {
      vcos_assert(!usecs); //definitely shouldn't be using timeouts with callbacks
      return -1; //don't try to wait with a callback set
   }

   if ( usecs == 0 )
      timeout = VCOS_NO_SUSPEND;
   else if ( usecs == 0xffffffff )
      timeout = VCOS_SUSPEND;
   else
      timeout = usecs / 1000;

   status = vcos_event_flags_get( &pool->event, RELEASE_EVENT, VCOS_OR_CONSUME, timeout, &event );
   if ( status == VCOS_SUCCESS )
      return 0; // success
   else
      return -1; // timed out
}

static void
signal_event( VC_POOL_T *pool )
{
   //either call the callback or signal the event
   //the use of both together is prohibited
   if (pool->callback)
      pool->callback(pool->callback_cookie);
   else
      vcos_event_flags_set( &pool->event, RELEASE_EVENT, VCOS_OR );
}

#if POOL_OBJECT_LOGGING
static void
pool_object_logging( const char *fname, VC_POOL_OBJECT_T *object )
{
   logging_message( LOGGING_VMCS,"%s: %p: task %s: refcount=%d", fname, object,
                    vcos_thread_get_name( vcos_thread_current() ), object->refcount );
}
#endif

#else

// use same constructs as old vc_image_pool code
static void create_event( VC_POOL_T *pool ) { }
static void destroy_event( VC_POOL_T *pool ) { }
static int  wait_event( VC_POOL_T *pool, uint32_t usecs ) { return 0; /* always succeed */ }
static void signal_event( VC_POOL_T *pool ) { }

#if POOL_OBJECT_LOGGING
static void
pool_object_logging( const char *fname, VC_POOL_OBJECT_T *object )
{
   logging_message( LOGGING_VMCS,"%s: %p: refcount=%d", fname, object, object->refcount );
}
#endif

#endif

/** Set a callback to be called when an object is finally freed.
  * Using a callback and a timeout together is dangerous.
  * It is the resposibility of the client to ensure that a callback
  * isn't set whilst alloc is called with a timeout.
  */
void vc_pool_set_callback(VC_POOL_T *pool, VC_POOL_CALLBACK_T callback, void* cookie)
{
   pool->callback = callback;
   pool->callback_cookie = cookie;
}
