/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Khronos
Module   :  OpenWF-C

FILE DESCRIPTION
OpenWF-C native stream definitions
=============================================================================*/

#define VCOS_VERIFY_BKPTS 1 // TODO remove

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client_rpc.h"
#include "interface/khronos/common/khrn_client_pointermap.h"

#include "vcfw/logging/logging.h"

#include "wfc_client_stream.h"

#include <stdio.h>

//==============================================================================

// Numbers of various things (all fixed size for now)
#define WFC_STREAM_NUM_OF_BUFFERS            8
#define WFC_STREAM_NUM_OF_SOURCES_OR_MASKS   8
#define WFC_STREAM_NUM_OF_CONTEXT_INPUTS     8

#define STREAM_LOCK(stream_ptr)      (platform_mutex_acquire(&stream_ptr->mutex))
#define STREAM_UNLOCK(stream_ptr)    (platform_mutex_release(&stream_ptr->mutex))

#define LOGGING_WFC  LOGGING_GENERAL

//==============================================================================

// Top-level stream type
typedef struct
{
   // Handle, assigned by window manager.
   WFCNativeStreamType handle;

   // Flag, indicating that the stream has been created on the server
   bool has_been_created;

   // Mutex, for thread safety
   PLATFORM_MUTEX_T mutex;

   // Configuration flags
   uint32_t flags;

   // Off-screen contexts which supply data to this stream.
   WFCContext context_input[WFC_STREAM_NUM_OF_CONTEXT_INPUTS];

   // Image providers to which this stream sends data; recorded so we do
   // not destroy this stream if it is still associated with a source or mask.
   uint32_t num_of_sources_or_masks;
   //WFCContext source_or_mask[WFC_STREAM_NUM_OF_SOURCES_OR_MASKS];

   // Asynchronous (KHAN) semaphore
   PLATFORM_SEMAPHORE_T async_sem;

   // Flag, indicating that the stream's destruction has been requested.
   //bool destroy_pending;
} WFC_STREAM_T;

//==============================================================================

static KHRN_POINTER_MAP_T stream_map;

//==============================================================================

static WFC_STREAM_T *wfc_stream_get_stream_ptr(WFCNativeStreamType stream);
static void wfc_stream_destroy_actual(WFCNativeStreamType stream, WFC_STREAM_T *stream_ptr);

//==============================================================================

uint32_t wfc_stream_create_from_image
   (WFCNativeStreamType stream, VC_IMAGE_TYPE_T pixel_format,
      uint32_t width, uint32_t height, uint32_t flags)
// Create a stream, using the given stream handle (typically assigned by the
// window manager), containing the specified number of buffers, and using
// a bufman image as a template. Return zero if OK.
{
   WFC_STREAM_T *stream_ptr;
   uint32_t pid_lo = 0;
   uint32_t pid_hi = 0;

   // Create stream
   stream_ptr = wfc_stream_get_stream_ptr(stream);

   STREAM_LOCK(stream_ptr);

   // Increment reference count, whether the stream is new or not.
   stream_ptr->num_of_sources_or_masks++;

   if(!stream_ptr->has_been_created)
   {
      // Stream did not previously exist.
      stream_ptr->has_been_created = true;
      stream_ptr->flags = flags;

      // Create KHAN semaphore, if required
      if(flags & WFC_STREAM_FLAGS_ASYNC_SEM)
      {
         uint64_t pid = khronos_platform_get_process_id();
         pid_lo = (uint32_t) pid;
         pid_hi = (uint32_t) (pid >> 32);
         int sem_name[3] = {pid_lo, pid_hi, stream};
         khronos_platform_semaphore_create(&stream_ptr->async_sem, sem_name, 0);
      } // if

      STREAM_UNLOCK(stream_ptr);

      logging_message(LOGGING_WFC, "wfc_stream_create_from_image");

      // Create server-side stream.
      RPC_CALL7
         (wfc_stream_server_create,
            WFC_STREAM_CREATE_ID,
            stream_ptr->handle, pixel_format, width, height, flags, pid_lo, pid_hi);
      RPC_FLUSH();
   } // if
   else
   {
      // Stream already exists, so nothing else to do.
      logging_message(LOGGING_WFC, "wfc_stream_create_from_image: already existed");

      vcos_assert(stream_ptr->flags == flags);

      STREAM_UNLOCK(stream_ptr);
   } // else

   return 0;
} // wfc_stream_create_from_image()

//------------------------------------------------------------------------------

WFCNativeStreamType wfc_stream_create_from_context
   (WFCContext context, uint32_t num_of_buffers, uint32_t flags)
// Create a stream containing the specified number of buffers, and using an
// off-screen context as a template. Also allocates the corresponding buffers
// on the server.
{
   vcos_assert(0); // TODO
   return 0;
} // wfc_stream_create_from_context()


//------------------------------------------------------------------------------

void wfc_stream_await_buffer(WFCNativeStreamType stream)
// Suspend until buffer is available on the server.
{
   WFC_STREAM_T *stream_ptr = wfc_stream_get_stream_ptr(stream);

   if(vcos_verify(stream_ptr->flags & WFC_STREAM_FLAGS_ASYNC_SEM))
   {
      logging_message(LOGGING_WFC, "wfc_stream_await_buffer: pre async sem acquire[%08X]", stream);
      khronos_platform_semaphore_acquire(&stream_ptr->async_sem);
      logging_message(LOGGING_WFC, "wfc_stream_await_buffer: post async sem acquire[%08X]", stream);
   } // if

} // wfc_stream_await_buffer()

//------------------------------------------------------------------------------

void wfc_stream_destroy(WFCNativeStreamType stream)
// Destroy a stream - unless it is still in use, in which case, mark it for
// destruction once all users have finished with it.
{
   WFC_STREAM_T *stream_ptr = wfc_stream_get_stream_ptr(stream);

   /* If stream is still in use (i.e. it's attached to at least one source/mask
    * which is associated with at least one element) then destruction is delayed
    * until it's no longer in use.
    * Element-source/mask associations must be dealt with in wfc_client.c. */
   STREAM_LOCK(stream_ptr);

   //stream_ptr->destroy_pending = true;
   stream_ptr->num_of_sources_or_masks--;
   if(stream_ptr->num_of_sources_or_masks == 0)
   {
      wfc_stream_destroy_actual(stream, stream_ptr);
   } // if
   else
   {
      STREAM_UNLOCK(stream_ptr);
   } // else

} // wfc_stream_destroy()

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Off-screen composition functions
//------------------------------------------------------------------------------

void wfc_stream_get_off_screen_image
   (WFCNativeStreamType stream, VC_IMAGE_T *image)
// Get off-screen image from stream. Fills buffer passed to it.
{
#if 1
   WFC_STREAM_T *stream_ptr = wfc_stream_get_stream_ptr(stream);

   STREAM_LOCK(stream_ptr);

   vcos_assert(0); // TODO

   STREAM_UNLOCK(stream_ptr);
#else
   WFC_STREAM_T *stream_ptr = wfc_stream_get_stream_ptr(stream);
   uint32_t i;

   vcos_assert(stream_ptr != NULL);

   // TODO get next buffer
   i = 0;

   stream_ptr->buffer[i].status = WFC_STREAM_BUFFER_LOCKED_FOR_READING;
   *dm_handle = stream_ptr->buffer[i].dispmanx_handle;
   return stream_ptr->buffer[i].bm_image;
#endif
} // wfc_stream_get_off_screen_image()

//------------------------------------------------------------------------------

uint32_t wfc_stream_num_of_ready_buffers(WFCNativeStreamType stream)
// Returns the number of buffers containing data.
{
   vcos_assert(0); // TODO
   return 0;
} // wr_stream_num_of_ready_buffers()

//------------------------------------------------------------------------------

bool wfc_stream_is_invalid(WFCNativeStreamType stream)
// Returns true if stream is not a valid stream.
{
   WFC_STREAM_T *stream_ptr = wfc_stream_get_stream_ptr(stream);

   return(stream_ptr == NULL);
} // wfc_stream_is_invalid()

//==============================================================================

static WFC_STREAM_T *wfc_stream_get_stream_ptr(WFCNativeStreamType stream)
// Return the pointer to the stream structure corresponding to the specified
// stream handle. If it doesn't exist, create it.
{
   WFC_STREAM_T *stream_ptr = NULL;

   // Initialise stream_map the first time around
   if(stream_map.storage == NULL)
   {
      if(!vcos_verify(khrn_pointer_map_init(&stream_map, 8)))
         {return NULL;}
   } // if

   // Look up stream
   stream_ptr = (WFC_STREAM_T *) khrn_pointer_map_lookup(&stream_map, stream);

   // If it doesn't exist, then create it, and insert into the map
   if(stream_ptr == NULL)
   {
      // Allocate memory for stream_ptr
      stream_ptr = (WFC_STREAM_T *) khrn_platform_malloc(sizeof(WFC_STREAM_T), "WFC_STREAM_T");

      if(vcos_verify(stream_ptr != NULL))
      {
         // Initialise new stream
         memset(stream_ptr, 0, sizeof(WFC_STREAM_T));
         platform_mutex_create(&stream_ptr->mutex);
         stream_ptr->handle = stream;
         // Insert stream into map
         khrn_pointer_map_insert(&stream_map, stream, stream_ptr);
      } // if
   } // if

   return stream_ptr;
} // wfc_stream_get_stream_ptr()

//------------------------------------------------------------------------------

static void wfc_stream_destroy_actual(WFCNativeStreamType stream, WFC_STREAM_T *stream_ptr)
// Actually destroy the stream, once it is no longer in use.
{
   vcos_assert(stream_ptr != NULL);
   vcos_assert(stream_ptr->num_of_sources_or_masks == 0);
   //vcos_assert(stream_ptr->destroy_pending);

   logging_message(LOGGING_WFC, "wfc_stream_destroy_actual: stream = %d", stream);

   // Delete server-side stream
   RPC_CALL1(wfc_stream_server_destroy, WFC_STREAM_DESTROY_ID, stream_ptr->handle);
   RPC_FLUSH();

   // Remove from map
   khrn_pointer_map_delete(&stream_map, stream);

   // Destroy async semaphore.
   khronos_platform_semaphore_destroy(&stream_ptr->async_sem);

   // Destroy mutex
   STREAM_UNLOCK(stream_ptr);
   platform_mutex_destroy(&stream_ptr->mutex);

   // Delete
   khrn_platform_free(stream_ptr);

} // wfc_stream_destroy_actual()

//==============================================================================

