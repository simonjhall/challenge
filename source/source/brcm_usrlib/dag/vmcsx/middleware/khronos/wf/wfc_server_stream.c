/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#define VCOS_VERIFY_BKPTS 1 // TODO remove

#include "middleware/khronos/common/khrn_map.h"
#include "vcfw/rtos/common/rtos_common_mem.h"
#include "interface/vcos/vcos.h"
#include "interface/khronos/common/khrn_int_ids.h"
#include "vcfw/logging/logging.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h" // khdispatch_send_async()

#include "wfc_server_stream.h"

//==============================================================================

#define LOGGING_WFC  LOGGING_GENERAL // TODO remove this

// Shift for stream key when signalling async semaphore
#define WFC_STREAM_KEY_SHIFT           16

#define WFC_STREAM_NUM_OF_ELEMENTS     8

#define WFC_STREAM_USAGE_COUNT_RESET   -1

// Handy mutex macros
#define LOCK()    (vcos_mutex_lock(&wfc_server_stream_state.mutex))
#define UNLOCK()  (vcos_mutex_unlock(&wfc_server_stream_state.mutex))

#define wfc_stream_get_front_buffer(queue_ptr)    ((queue_ptr)->first)

//==============================================================================

/*
 * Note that "buffer" really describes a queue item, rather than an image
 * buffer. The image buffer is created by the renderer, and inserted into a
 * queue buffer.
 *
 * For a given stream, queue buffers are created whenever the renderer tries to
 * put an image in a queue which is full. Thereafter, unused ones are put
 * in the free_pool.
 *
 * Queue buffers are only destroyed when the stream is.
 * */

typedef struct WFC_SERVER_BUFFER_T_tag
{
   struct WFC_SERVER_BUFFER_T_tag *next;

   uint32_t usage_count;
   VC_IMAGE_T *image;
   WFC_STREAM_SERVER_RENDERER_CALLBACK_T cb_func;
   uint32_t cb_arg;
} WFC_SERVER_BUFFER_T;

typedef struct
{
   // Pool containing free buffers.
   WFC_SERVER_BUFFER_T *free_pool;
   // Queue containing full buffers for display.
   WFC_SERVER_BUFFER_T *first;
   // Pointer to last item in the queue.
   WFC_SERVER_BUFFER_T *last;
} WFC_SERVER_QUEUE_T;

// TODO: tidy this up
typedef struct
{
   // Image parameters
   VC_IMAGE_TYPE_T pixel_format;
   uint32_t width;
   uint32_t height;
   // Number of elements using this stream
   uint32_t num_of_elements;

   // Set to true if a stream's destruction is requested.
   bool destroy_pending;

   // Queue (and related storage) for buffers.
   WFC_SERVER_QUEUE_T queue;

   // Client process ID; used for asynchronous (KHAN) semaphore
   uint64_t pid;
   // Total number of buffers using this stream
   uint32_t num_of_buffers;

   // Elements which take data from this stream; recorded so that we know how
   // many times a front buffer will be used before it can be released.
   WFCElement element_list[WFC_STREAM_NUM_OF_ELEMENTS];
} WFC_SERVER_STREAM_T;

typedef struct
{
   bool is_initialised;
   KHRN_MAP_T stream_map;
   VCOS_MUTEX_T mutex;
   WFC_STREAM_SERVER_UPDATE_CALLBACK_T update_callback;
} WFC_SERVER_STREAM_STATE_T;

//==============================================================================

static WFC_SERVER_STREAM_STATE_T wfc_server_stream_state;

//------------------------------------------------------------------------------

static void wfc_stream_destroy_actual(WFCNativeStreamType stream, MEM_HANDLE_T stream_handle);
static WFC_SERVER_BUFFER_T *wfc_stream_get_free_buffer(WFC_SERVER_QUEUE_T *queue_ptr);
static void wfc_stream_queue_buffer
   (WFC_SERVER_QUEUE_T *queue_ptr, WFC_SERVER_BUFFER_T *buffer);
static void wfc_stream_unqueue_front_buffer(WFC_SERVER_QUEUE_T *queue_ptr);

//==============================================================================
// Functions called remotely by stream client.

// Create a stream and its buffers for the specified image type and size, for
// supplying pixel data to one or more elements, and/or for storing the output
// of off-screen composition; return a stream key (or zero if there's a problem).
void wfc_stream_server_create
   (WFCNativeStreamType stream, VC_IMAGE_TYPE_T pixel_format,
      uint32_t width, uint32_t height, uint32_t flags, uint32_t pid_lo, uint32_t pid_hi)
{
   MEM_HANDLE_T stream_handle = 0;
   WFC_SERVER_STREAM_T *stream_ptr = NULL;
   bool success;

   // Initialise master stream_handle map, if not already done
   if(!wfc_server_stream_state.is_initialised)
   {
      vcos_mutex_create(&wfc_server_stream_state.mutex, "wfc_stream_server_mutex");
      LOCK();
      wfc_server_stream_state.is_initialised = true;
      vcos_verify(khrn_map_init(&wfc_server_stream_state.stream_map, 8));
   } // if
   else
   {
      LOCK();
   } // else

   logging_message(LOGGING_WFC, "wfc_stream_server_create");

   // Create stream_handle, and add to map
   stream_handle = MEM_ALLOC_STRUCT_EX(WFC_SERVER_STREAM_T, MEM_COMPACT_NORMAL);
   vcos_assert(stream_handle != MEM_HANDLE_INVALID);

   success = khrn_map_insert(&wfc_server_stream_state.stream_map, stream, stream_handle);
   vcos_assert(success);

   // Initialise stream
   stream_ptr = (WFC_SERVER_STREAM_T *) mem_lock(stream_handle);
   memset(stream_ptr, 0, sizeof(stream_ptr));
   stream_ptr->pixel_format = pixel_format;
   stream_ptr->width = width;
   stream_ptr->height = height;
   stream_ptr->pid = (((uint64_t) pid_hi) << 32) | pid_lo;

   mem_unlock(stream_handle);

   UNLOCK();

} // wfc_stream_server_create()

//------------------------------------------------------------------------------

// Check to see if the specified buffer has been filled by the renderer. If "suspend"
// is true, suspend until then. If "buffer_handle" is zero, use the next buffer to
// be filled.
bool wfc_stream_server_is_rendering_complete
   (WFCNativeStreamType stream, BUFFER_HANDLE_T buffer_handle, bool suspend)
{
   LOCK();

   vcos_assert(0); // TODO

   UNLOCK();

   return 0;
} //

//------------------------------------------------------------------------------

// Check to see if all the data for the specified context has been rendered,
// and is ready to be sent to the compositor.
bool wfc_stream_server_is_update_ready
   (WFCNativeStreamType stream, WFCContext context, bool suspend)
{
   LOCK();

   vcos_assert(0); // TODO

   UNLOCK();

   return 0;
} //

//------------------------------------------------------------------------------

// Destroy stream
void wfc_stream_server_destroy(WFCNativeStreamType stream)
// Destroy a stream. Client ensures that the stream is not in use.
{
   MEM_HANDLE_T stream_handle;
   WFC_SERVER_STREAM_T *stream_ptr;

   logging_message(LOGGING_WFC, "wfc_stream_server_destroy: pre-lock; stream = %d", stream);
   LOCK();
   logging_message(LOGGING_WFC, "wfc_stream_server_destroy: post-lock");

   stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   stream_ptr = mem_lock(stream_handle);

   logging_message(LOGGING_WFC,
      "wfc_stream_server_destroy: stream_ptr->queue.first = %08X",
      stream_ptr->queue.first);

   vcos_assert(stream_ptr->queue.first == NULL);

   stream_ptr->destroy_pending = true;

   uint32_t num_of_elements = stream_ptr->num_of_elements;
   mem_unlock(stream_handle);

   wfc_stream_destroy_actual(stream, stream_handle);

   UNLOCK();
} //

//------------------------------------------------------------------------------

// Lock front buffer for reading, and get its handle (for off-screen).
// (Data is pulled across using bufman).
BUFFER_HANDLE_T wfc_stream_server_acquire_front_buffer_handle(WFCNativeStreamType stream)
{
   LOCK();

   vcos_assert(0); // TODO

   UNLOCK();

   return 0;
} //

//------------------------------------------------------------------------------

// Release previously acquired front buffer (for off-screen)
void wfc_stream_server_release_front_buffer_handle
   (WFCNativeStreamType stream, BUFFER_HANDLE_T buffer_handle)
{
   LOCK();

   vcos_assert(0); // TODO

   UNLOCK();
} //

//==============================================================================
// Functions called by WF-C server.

// Function to be called when stream is updated (for use with activated contexts)
void wfc_stream_server_set_update_callback(WFC_STREAM_SERVER_UPDATE_CALLBACK_T cb_func)
{
   wfc_server_stream_state.update_callback = cb_func;
} // wfc_stream_server_set_update_callback()

//------------------------------------------------------------------------------

// Get data in front buffer (for elements).
VC_IMAGE_T *wfc_stream_server_acquire_front_buffer_data(WFCNativeStreamType stream)
{
   MEM_HANDLE_T stream_handle = 0;
   WFC_SERVER_STREAM_T *stream_ptr = NULL;
   WFC_SERVER_BUFFER_T *buffer;
   VC_IMAGE_T *image = NULL;

   LOCK();

   stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   stream_ptr = mem_lock(stream_handle);

   logging_message(LOGGING_WFC, "wfc_stream_server_acquire_front_buffer_data: %d", stream);

   // Get most recent front buffer
   buffer = wfc_stream_get_front_buffer(&stream_ptr->queue);

   // Get image pointer
   if(buffer != NULL)
      {image = buffer->image;}

   mem_unlock(stream_handle);

   UNLOCK();

   return image;
} //

//------------------------------------------------------------------------------

// Record that an element has released this buffer. When all elements which are
// using this buffer have released it, the client can signal to the renderer
// that the buffer is free).
void wfc_stream_server_release_front_buffer_data(WFCNativeStreamType stream)
{
   MEM_HANDLE_T stream_handle = 0;
   WFC_SERVER_STREAM_T *stream_ptr = NULL;
   WFC_SERVER_BUFFER_T *buffer;

   LOCK();

   stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   stream_ptr = mem_lock(stream_handle);

   // Get most recent front buffer
   buffer = wfc_stream_get_front_buffer(&stream_ptr->queue);
   vcos_assert(buffer != NULL);

   // Set usage count to number of elements if this is the first buffer release for
   // this composition cycle.
   if(buffer->usage_count == WFC_STREAM_USAGE_COUNT_RESET)
      {buffer->usage_count = stream_ptr->num_of_elements;}

   logging_message(LOGGING_WFC, "wfc_stream_server_release_front_buffer_data[%08X],"
      "usage_count = %d", stream_handle, buffer->usage_count);

   // Decrease usage count
   vcos_assert(buffer->usage_count > 0);
   buffer->usage_count--;
   // If buffer has been released by all elements which use it, then we're done.
   if(buffer->usage_count == 0)
   {
      // Remove the current front buffer from the queue.
      buffer->image = NULL;
      wfc_stream_unqueue_front_buffer(&stream_ptr->queue);

      // Signal to the client that a buffer is free.
      logging_message(LOGGING_WFC, "wfc_stream_server_release_front_buffer_data: ASYNC_COMMAND_POST; "
         "stream = 0x%X, pid = 0x%X", (uint32_t) stream, (uint32_t) (stream_ptr->pid));
      khdispatch_send_async(ASYNC_COMMAND_POST, stream_ptr->pid, (uint32_t) stream);

      // Unlock here, in case renderer callback calls stream function.
      mem_unlock(stream_handle);

      // Signal to renderer that a buffer is free.
      if(buffer->cb_func)
      {
         buffer->cb_func(buffer->cb_arg);
      } // if

   } // if
   else
      {mem_unlock(stream_handle);}

   UNLOCK();

} // wfc_stream_server_release_front_buffer_data()

//------------------------------------------------------------------------------

uint32_t wfc_stream_server_associate_element
   (WFCNativeStreamType old_stream, WFCNativeStreamType new_stream, WFCElement element)
// Associate an element with a stream when attributes are committed, and disassociate
// the old one. Returns 0 if this is OK, > 0 if not.
{
   MEM_HANDLE_T stream_handle = 0;
   WFC_SERVER_STREAM_T *stream_ptr = NULL;
   uint32_t failure = 0;
   uint32_t i;

   LOCK();

   if(old_stream != new_stream)
   {
      // Disassociate.
      // Invalid handle implies that this element wasn't previously
      // associated with any stream.
      if(old_stream != WFC_INVALID_HANDLE)
      {
         stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, old_stream);
         stream_ptr = mem_lock(stream_handle);

         // Find the source or mask in the list, and remove it
         i = 0;
         while((i < WFC_STREAM_NUM_OF_ELEMENTS)
            && (stream_ptr->element_list[i] != element))
            {i++;}

         if(vcos_verify(i < WFC_STREAM_NUM_OF_ELEMENTS))
         {
            stream_ptr->element_list[i] = WFC_INVALID_HANDLE;
            vcos_assert(stream_ptr->num_of_elements > 0);
            stream_ptr->num_of_elements--;

            uint32_t num_of_elements = stream_ptr->num_of_elements;
            bool destroy_pending = stream_ptr->destroy_pending;

            mem_unlock(stream_handle);

            // If the stream is no longer in use, and its destruction was previously requested,
            // then destroy it.
            wfc_stream_destroy_actual(old_stream, stream_handle);
         } // if
         else
            {mem_unlock(stream_handle);}
      } // if

      // Associate: record that this element is an output for this stream.
      // Handle can be invalid if stream is being disassociated rather than replaced
      // with another.
      if(new_stream != WFC_INVALID_HANDLE)
      {
         stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, new_stream);
         stream_ptr = mem_lock(stream_handle);

         // Find free space
         i = 0;
         while((stream_ptr->element_list[i] != WFC_INVALID_HANDLE)
            && (i < WFC_STREAM_NUM_OF_ELEMENTS))
            {i++;}

         // Insert, if not full
         if(vcos_verify(i < WFC_STREAM_NUM_OF_ELEMENTS))
         {
            stream_ptr->element_list[i] = element;
            stream_ptr->num_of_elements++;
         } // if
         else
            {failure = 1;}

         mem_unlock(stream_handle);
      } // if
   } // if

   UNLOCK();

   return failure;

} // wfc_stream_server_associate_element()

//------------------------------------------------------------------------------

// Return next available back buffer (or zero if none available) (for off-screen).
// (Note that acquisition of back buffers for elements is handled by the stream
// client).
VC_IMAGE_T *wfc_stream_server_acquire_back_buffer(WFCNativeStreamType stream)
{
   LOCK();

   vcos_assert(0); // TODO

   UNLOCK();

   return 0;
} //

//==============================================================================
// Functions called by renderer server (for elements) and WF-C server (for off-screen).

void wfc_stream_server_signal_buffer_avail(WFCNativeStreamType stream)
// Signal to client that a buffer is available for writing to. For use when
// renderer is allocating its buffers.
{
   MEM_HANDLE_T stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   WFC_SERVER_STREAM_T *stream_ptr = mem_lock(stream_handle);

   logging_message(LOGGING_WFC, "wfc_stream_server_signal_buffer_avail: ASYNC_COMMAND_POST; "
      "stream = 0x%X, pid = 0x%X", (uint32_t) stream, (uint32_t) (stream_ptr->pid));
   khdispatch_send_async(ASYNC_COMMAND_POST, stream_ptr->pid, (uint32_t) stream);

   stream_ptr->num_of_buffers++;

   mem_unlock(stream_handle);
} // wfc_stream_server_signal_buffer_avail()

//------------------------------------------------------------------------------

void wfc_stream_server_signal_buffer_removed(WFCNativeStreamType stream)
// Signal to client that a buffer has been removed. For use when
// renderer is deleting its buffers.
{
   logging_message(LOGGING_WFC, "wfc_stream_server_signal_buffer_removed: stream %d", stream);

   MEM_HANDLE_T stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   WFC_SERVER_STREAM_T *stream_ptr = mem_lock(stream_handle);

   khdispatch_send_async(ASYNC_COMMAND_WAIT, stream_ptr->pid, (uint32_t) stream);

   if(vcos_verify(stream_ptr->num_of_buffers > 0))
      {stream_ptr->num_of_buffers--;}

   mem_unlock(stream_handle);

   // If the stream is no longer in use, and its destruction was previously requested,
   // then destroy it.
   wfc_stream_destroy_actual(stream, stream_handle);
} // wfc_stream_server_signal_buffer_avail()

//------------------------------------------------------------------------------

uint32_t wfc_stream_server_queue_image_data
   (WFCNativeStreamType stream, VC_IMAGE_T *image,
      WFC_STREAM_SERVER_RENDERER_CALLBACK_T cb_func, uint32_t cb_arg)
// Queue image in stream's buffer. Returns an error code.
{
   uint32_t ret_val = WFC_QUEUE_OK;
   MEM_HANDLE_T stream_handle = 0;
   WFC_SERVER_STREAM_T *stream_ptr = NULL;
   WFC_SERVER_BUFFER_T *buffer;

   LOCK();

   stream_handle = khrn_map_lookup(&wfc_server_stream_state.stream_map, stream);
   stream_ptr = mem_lock(stream_handle);

   logging_message(LOGGING_WFC, "wfc_stream_server_queue_image_data: stream = %d, buf_num = %d",
      stream, cb_arg);

   if(vcos_verify(image != NULL))
   {
      if(!vcos_verify((image->height == stream_ptr->height) && (image->width == stream_ptr->width)))
         {ret_val |= WFC_QUEUE_WRONG_DIMENSIONS;}

      if(!vcos_verify((image->type == stream_ptr->pixel_format)))
         {ret_val |= WFC_QUEUE_WRONG_PIXEL_FORMAT;}

      if(ret_val == WFC_QUEUE_OK)
      {
         // Get next free buffer
         buffer = wfc_stream_get_free_buffer(&stream_ptr->queue);

         if(buffer == NULL)
         {
            vcos_assert(0);
            // TODO: discard oldest buffer if there's no room left.
         } // if

         // Store image pointer
         buffer->image = image;
         // Indicate that usage count has been reset
         buffer->usage_count = WFC_STREAM_USAGE_COUNT_RESET;

         // Store renderer's callback
         buffer->cb_func = cb_func;
         buffer->cb_arg = cb_arg;

         // Queue buffer in stream
         wfc_stream_queue_buffer(&stream_ptr->queue, buffer);

         // Signal to WFC server that a stream has been updated.
         if(wfc_server_stream_state.update_callback)
               {wfc_server_stream_state.update_callback(stream_ptr->element_list);}
      } // if
   } // if
   else
   {
      ret_val |= WFC_QUEUE_NO_IMAGE;
   } // else

   mem_unlock(stream_handle);

   UNLOCK();

   return ret_val;

} // wfc_stream_server_queue_image_data()

//==============================================================================

static void wfc_stream_destroy_actual(WFCNativeStreamType stream, MEM_HANDLE_T stream_handle)
// Destroy the stream, once it is no longer in use. Stream must be unlocked
// before calling this function, and will either be unlocked, or deleted, afterwards.
{
   vcos_assert(mem_get_lock_count(stream_handle) == 0);

   WFC_SERVER_STREAM_T *stream_ptr = mem_lock(stream_handle);

   if(stream_ptr->destroy_pending
      && (stream_ptr->num_of_elements == 0)
      && (stream_ptr->num_of_buffers == 0))
   {
      // Remove from stream map.
      uint32_t success = khrn_map_delete(&wfc_server_stream_state.stream_map, stream);
      vcos_assert(success);

      logging_message(LOGGING_WFC,
            "wfc_stream_destroy_actual: stream deleted from map; stream = %d, success = %d", stream, success);

      // Delete free pool
      vcos_assert(stream_ptr->queue.first == NULL);
      WFC_SERVER_BUFFER_T *free_pool = stream_ptr->queue.free_pool;
      WFC_SERVER_BUFFER_T *next;
      while(free_pool != NULL)
      {
         next = free_pool->next;
         rtos_priorityfree(free_pool);
         free_pool = next;
      } // while

      mem_unlock(stream_handle);

      // Delete.
      mem_release(stream_handle);

      logging_message(LOGGING_WFC, "wfc_stream_destroy_actual: stream deleted");

   } // if
   else
      {mem_unlock(stream_handle);}

} // wfc_stream_destroy_actual()

//------------------------------------------------------------------------------

static WFC_SERVER_BUFFER_T *wfc_stream_get_free_buffer(WFC_SERVER_QUEUE_T *queue_ptr)
// Return a pointer to the next free buffer in the specified stream, or NULL if
// none available. Also remove said buffer from the free_pool.
{
   MEM_HANDLE_T buffer_handle = MEM_HANDLE_INVALID;
   WFC_SERVER_BUFFER_T *buffer = NULL;

   if(queue_ptr->free_pool == NULL)
   {
      // Allocate a new queue item.
      buffer = rtos_prioritycalloc(sizeof(WFC_SERVER_BUFFER_T), RTOS_ALIGN_DEFAULT,
         RTOS_PRIORITY_INTERNAL, "wfc_stream_get_free_buffer");
      logging_message(LOGGING_WFC, "wfc_stream_get_free_buffer: allocate new queue item");
   } // if
   else
   {
      // Get next free buffer.
      buffer = queue_ptr->free_pool;
      // Remove from free queue.
      queue_ptr->free_pool = queue_ptr->free_pool->next;
   } // else

   vcos_assert(buffer != NULL);
   buffer->next = NULL;

   return buffer;
} // wfc_stream_get_free_buffer()

//------------------------------------------------------------------------------

static void wfc_stream_queue_buffer
   (WFC_SERVER_QUEUE_T *queue_ptr, WFC_SERVER_BUFFER_T *buffer)
// Queue buffer in the specified stream.
{
   vcos_assert(buffer != NULL);
   vcos_assert(buffer->next == NULL);

   // Update pointer to last item in the queue.
   if(queue_ptr->last != NULL)
      {queue_ptr->last->next = buffer;}
   queue_ptr->last = buffer;

   // If the queue was previously empty, this is now at the front.
   if(queue_ptr->first == NULL)
      {queue_ptr->first = buffer;}
} // wfc_stream_queue_buffer()

//------------------------------------------------------------------------------

static void wfc_stream_unqueue_front_buffer(WFC_SERVER_QUEUE_T *queue_ptr)
// Remove the current front buffer from the queue (and put it in the free pool).
{
   WFC_SERVER_BUFFER_T *buffer = queue_ptr->first;

   vcos_assert(buffer != NULL);
   vcos_assert(buffer->image == NULL);

   // Advance queue to the next item.
   queue_ptr->first = queue_ptr->first->next;
   if(queue_ptr->first == NULL)
   {
      // Queue now empty.
      vcos_assert(queue_ptr->last == buffer);
      queue_ptr->last = NULL;
      logging_message(LOGGING_WFC, "wfc_stream_unqueue_front_buffer: queue empty; buffer %d", buffer->cb_arg);
   } // if
   else
   {
      logging_message(LOGGING_WFC, "wfc_stream_unqueue_front_buffer: queue not empty; buffer %d", buffer->cb_arg);
   } // else

   // Move old buffer to the head of the free pool.
   buffer->next = queue_ptr->free_pool;
   queue_ptr->free_pool = buffer;
} // wfc_stream_unqueue_front_buffer()

//==============================================================================

