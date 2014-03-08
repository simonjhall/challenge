/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_int_util.h"

#include "middleware/khronos/glxx/glxx_buffer.h"

#include <string.h>

#include "middleware/khronos/glxx/glxx_buffer_cr.c"

static bool write_would_block(GLXX_BUFFER_INNER_T *item)
{
   return khrn_interlock_write_would_block(&item->interlock);
}

/*
   void glxx_buffer_init(GLXX_BUFFER_T *buffer, uint32_t name)

   initialize a buffer object

   Implementation notes:

   -

   Precondition:

   buffer is a valid pointer to an uninitialised GLXX_BUFFER_T
   memory management invariants for buffer are satisfied
   name != 0

   Postcondition:
   
   The GLXX_BUFFER_T pointed to by buffer is valid
*/

static void glxx_buffer_inner_init(GLXX_BUFFER_INNER_T *item)
{
   vcos_assert(item->mh_storage == MEM_INVALID_HANDLE);
   MEM_ASSIGN(item->mh_storage, MEM_ZERO_SIZE_HANDLE);

   khrn_interlock_init(&item->interlock);

}

void glxx_buffer_init(GLXX_BUFFER_T *buffer, uint32_t name)
{
   uint32_t i;
   vcos_assert(buffer);

   buffer->name = name;

   buffer->usage = GL_STATIC_DRAW;

   for(i = 0; i< GLXX_BUFFER_POOL_SIZE; i++)
      glxx_buffer_inner_init(&buffer->pool[i]);

   buffer->current_item = 0;

#ifdef REQUIRE_MAX_INDEX_CHECK
   buffer->size_used_for_max = 0;
#endif
}

/*
   void glxx_buffer_term(void *v, uint32_t size)

   Terminator for GLXX_BUFFER_T. 

   Implementation notes:

   Precondition:

   is only called from memory manager internals during destruction of a GLXX_BUFFER_T
   v is a valid pointer to a (possibly uninitialised or partially initialised*) GLXX_BUFFER_T
   memory management invariants for v are satisfied

   * uninitialised sections of a GLXX_BUFFER_T are set to -1

   Postcondition:
   
   Frees any references to resources that are referred to by the GLXX_BUFFER_T
*/

static void glxx_buffer_inner_term(GLXX_BUFFER_INNER_T *item)
{
   MEM_ASSIGN(item->mh_storage, MEM_INVALID_HANDLE);

   khrn_interlock_term(&item->interlock);
}

void glxx_buffer_term(void *v, uint32_t size)
{
   uint32_t i;
   GLXX_BUFFER_T *buffer = (GLXX_BUFFER_T *)v;
   UNUSED(size);

   if(buffer->pool[buffer->current_item].mh_storage!=MEM_ZERO_SIZE_HANDLE)
      mem_unretain(buffer->pool[buffer->current_item].mh_storage);

   for(i = 0; i< GLXX_BUFFER_POOL_SIZE; i++)
      glxx_buffer_inner_term(&buffer->pool[i]);
}

/*
   bool glxx_buffer_data(GLXX_BUFFER_T *buffer, int32_t size, const void *data, GLenum usage)

   Set a buffer object's size, contents and usage. Returns true if successful or false if out of memory

   Implementation notes:

   As the storage associated with the buffer is always the size of the buffer, we reallocate
   even when shrinking. Thus making a buffer smaller can lead to GL_OUT_OF_MEMORY.

   Preconditions:

   buffer is a valid pointer
   size >= 0
   data is either NULL or a valid pointer to size bytes which does not overlap buffer contents in memory
   usage is GL_STATIC_DRAW or GL_DYNAMIC_DRAW
   conceptual_buffers_owned_by_master is true

   Postconditions:

   If we fail (due to out of memory), the state of buffer is unmodified.

   Invariants preserved:

   All invariants on buffer
*/

static bool glxx_buffer_inner_data(GLXX_BUFFER_INNER_T *item, int32_t size, const void *data, bool is_new_item)
{
   uint32_t current_size;
   vcos_assert(size >= 0);

   khrn_interlock_write_immediate(&item->interlock);

   current_size = mem_get_size(item->mh_storage);
   if (current_size != (uint32_t)size) {
#ifdef __VIDEOCORE4__
      MEM_HANDLE_T handle;
      MEM_FLAG_T flags = MEM_FLAG_DIRECT | MEM_FLAG_DISCARDABLE;
      if(!is_new_item)
      {
         /* unretain existing, retain new */
         if(item->mh_storage!=MEM_ZERO_SIZE_HANDLE)
            mem_unretain(item->mh_storage);

         flags |= MEM_FLAG_RETAINED;
      }
      /* discardable so can be reclaimed if short of memory and no longer retained */
      handle = mem_alloc_ex((uint32_t)size, 4, flags,
         "GLXX_BUFFER_INNER_T.storage", MEM_COMPACT_DISCARD);
#else
      MEM_HANDLE_T handle = mem_alloc_ex((uint32_t)size, 4, MEM_FLAG_NONE, "GLXX_BUFFER_INNER_T.storage", MEM_COMPACT_DISCARD);        // check, no term
#endif

      if (handle == MEM_INVALID_HANDLE)
      {
         MEM_ASSIGN(item->mh_storage, MEM_ZERO_SIZE_HANDLE);
         return false;
      }

      MEM_ASSIGN(item->mh_storage, handle);
      mem_release(handle);
   }

   /*
      at this point buffer->mh_storage is guaranteed to have size size
   */

   if (data) {
      memcpy(mem_lock(item->mh_storage), data, size);
      mem_unlock(item->mh_storage);
   }

   return true;
}

bool glxx_buffer_data(GLXX_BUFFER_T *buffer, int32_t size, const void *data, GLenum usage)
{
   vcos_assert(size >= 0);

   if(write_would_block(&buffer->pool[buffer->current_item]))
   {
      //pick a non busy entry from the pool;
      uint32_t i;
      for(i = 0; i< GLXX_BUFFER_POOL_SIZE; i++)
      {
         if(i!=(uint32_t)buffer->current_item && !write_would_block(&buffer->pool[i]))
            break;
      }
      if(i<GLXX_BUFFER_POOL_SIZE)
      {  //found one
#ifdef __VIDEOCORE4__
         //unretain current one, so when fixer also unretains, retain count will fall to 0
         mem_unretain(buffer->pool[buffer->current_item].mh_storage);
         //retain new one
         if(buffer->pool[i].mh_storage!=MEM_ZERO_SIZE_HANDLE)
            mem_retain(buffer->pool[i].mh_storage);
#endif
         buffer->current_item = i;

      } //else stick with the existing and wait
   }

   if(!glxx_buffer_inner_data(&buffer->pool[buffer->current_item],size,data,false))
      return false;

   //successfuly allocated memory and copied data
   buffer->usage = usage;

#ifdef REQUIRE_MAX_INDEX_CHECK
   buffer->size_used_for_max = 0;
#endif

   return true;
}

/*
   void glxx_buffer_subdata(GLXX_BUFFER_T *buffer, int32_t offset, int32_t size, const void *data)

   Overwrites a portion of a buffer object's contents with new data. Does not change its size so
   cannot fail due to out-of-memory.

   Implementation notes:

   We rely on memcpy(x,y,0) doing nothing (i.e. not crashing) when x and y are invalid pointers

   Preconditions:

   buffer is a valid pointer
   offset >= 0
   size >= 0
   offset + size <= buffer.size
   data is a valid pointer to size bytes which does not overlap buffer contents in memory
   conceptual_buffers_owned_by_master is true

   Postconditions:

   -

   Invariants preserved:

   Invariants on size_used_for_max and max
*/

static void glxx_buffer_inner_subdata(GLXX_BUFFER_INNER_T *item, int32_t offset, int32_t size, const void *data)
{
   vcos_assert(offset >= 0 && size >= 0 && (uint32_t)offset + (uint32_t)size <= mem_get_size(item->mh_storage));
   vcos_assert(data);

   khrn_interlock_write_immediate(&item->interlock);

   if(size>0) {
      memcpy((uint8_t *)mem_lock(item->mh_storage) + offset, data, size);
      mem_unlock(item->mh_storage);
   }
}

void glxx_buffer_subdata(GLXX_BUFFER_T *buffer, int32_t offset, int32_t size, const void *data)
{
   vcos_assert(offset >= 0 && size >= 0);
   vcos_assert(data);

   //if write_would_block(buffer->pool[buffer->current_item] 
   //we pick one of the other pool items, copy the entirety of the current item
   //into that item, and then the new subdata

   if(write_would_block(&buffer->pool[buffer->current_item]))
   {
      //pick a non busy entry from the pool;
      uint32_t i;
      for(i = 0; i< GLXX_BUFFER_POOL_SIZE; i++)
      {
         if(i!=(uint32_t)buffer->current_item && !write_would_block(&buffer->pool[i]))
               break;
      }
      if(i<GLXX_BUFFER_POOL_SIZE)
      {  //found one
         //copy existing data into this new item assuming alloc succeeds
         uint32_t existing_size = mem_get_size(buffer->pool[buffer->current_item].mh_storage);
         bool ok;
         void * existing_data = mem_lock(buffer->pool[buffer->current_item].mh_storage);
         ok = glxx_buffer_inner_data(&buffer->pool[i],existing_size,existing_data,true);
         mem_unlock(buffer->pool[buffer->current_item].mh_storage);
         if(ok)
         {
#ifdef __VIDEOCORE4__
            //unretain current one, so when fixer also unretains, retain count will fall to 0
            mem_unretain(buffer->pool[buffer->current_item].mh_storage);
            //retain new one
            if(buffer->pool[i].mh_storage!=MEM_ZERO_SIZE_HANDLE)
               mem_retain(buffer->pool[i].mh_storage);
#endif
            buffer->current_item = i;
         }
      } //else stick with the existing and wait

   }

   glxx_buffer_inner_subdata(&buffer->pool[buffer->current_item],offset,size,data);

#ifdef REQUIRE_MAX_INDEX_CHECK
   buffer->size_used_for_max = 0;
#endif
}

/*
   Returns whether all values in the specified sequence (defined by type_size, offset
   and count) are less than the specified value.
   
   The buffer contains a sort of cache to help with this. buffer->max tells us
   whether the entire buffer is less than the specified value. If this turns out
   not to be the case, then we check just the range we're given.
   
   TODO: is this the most sensible algorithm to use? I guess we'll find out if
   things start running slowly...
   
   Preconditions:
      type_size is 1 or 2
      offset is a multiple of type_size
      [offset, offset + size * count) fits inside the buffer
*/

#ifdef REQUIRE_MAX_INDEX_CHECK
bool glxx_buffer_values_are_less_than(GLXX_BUFFER_T *buffer, int offset, int count, int size, int value)
{
   int bsize = mem_get_size(buffer->pool[buffer->current_item].mh_storage);
   vcos_assert(size == 1 || size == 2);
   vcos_assert(offset >= 0 && size >= 0 && offset + size * count <= bsize);
   vcos_assert(!(offset & (size - 1)));
   
   if (buffer->size_used_for_max != (uint32_t)size) {
      void *data = mem_lock(buffer->pool[buffer->current_item].mh_storage);
      
      // Recalculate max
      buffer->max = find_max(bsize / size, size, data);
      buffer->size_used_for_max = size;
      
      mem_unlock(buffer->pool[buffer->current_item].mh_storage);
   }
   
   if (buffer->max < value)
      return true;
   else {
      void *data = mem_lock(buffer->pool[buffer->current_item].mh_storage);
      int max = find_max(count, size, (uint8_t *)data + offset);
      
      mem_unlock(buffer->pool[buffer->current_item].mh_storage);
      
      return max < value;
   }
}
#endif

/*
   return handle to the current storage
*/

MEM_HANDLE_T glxx_buffer_get_storage_handle(GLXX_BUFFER_T *buffer)
{
   return buffer->pool[buffer->current_item].mh_storage;
}

uint32_t glxx_buffer_get_interlock_offset(GLXX_BUFFER_T *buffer)
{
   return (uint32_t)((char *)&buffer->pool[buffer->current_item].interlock - (char *)buffer);
}
