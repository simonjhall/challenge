/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/vcos/vcos.h"

#include "middleware/dispmanx/dispmanx.h"

#include "vcfw/logging/logging.h"

#include "middleware/khronos/wf/wfc_server_stream.h"
#include "middleware/khronos/wf/wfc_server.h"

//==============================================================================

// TODO remove
//#define ENABLE_BKPTS
#ifdef ENABLE_BKPTS
#define WFC_BKPT() vcos_assert(0)
#else
#define WFC_BKPT()
#endif

#define LOGGING_WFC  LOGGING_GENERAL // TODO remove this

// Handy mutex macros
#define LOCK()    (vcos_mutex_lock(&wfc_server_state.mutex))
#define UNLOCK()  (vcos_mutex_unlock(&wfc_server_state.mutex))

//==============================================================================

typedef struct
{
   WFCElement element;
   WFC_ELEMENT_ATTRIB_T attributes;

   WFCNativeStreamType source_stream;
   WFCNativeStreamType mask_stream;

   DISPMANX_ELEMENT_HANDLE_T dm_handle;

   uint32_t layer;

} WFC_SERVER_ELEMENT_T;

typedef struct
{
   WFC_CONTEXT_ATTRIB_T attributes;

   int num_of_elements;

   // Element storage. One map stores the previously committed elements,
   // while the other is used for the new commit. "active_element_index" indicates
   // which one is active, and alternates between the two on subsequent commits.
   KHRN_MAP_T element_map[2];
   uint32_t active_element_index;

   WFCContextType context_type;
   uint32_t screen_or_stream_num;

   // Dispmanx handle for display to which this context is assigned, if an
   // on-screen context.
   DISPMANX_DISPLAY_HANDLE_T dm_display_handle;

   // Bitfield used to record which elements have been updated for the current
   // composition cycle.
   uint32_t elements_updated;

   bool is_activated; // Indicates autonomous composition

   // Indicates that a commit has been committed, but not composed.
   bool commit_pending;

} WFC_SERVER_CONTEXT_T;

typedef struct
{
   KHRN_MAP_T context_map;

   // Mutex for exclusive access to common resources
   VCOS_MUTEX_T mutex;

   // Event to trigger autonomous composition
   VCOS_EVENT_T compose_event;

   // Prevent commits from occurring while compositions are in progress, and
   // vice versa (also prevents compositions from being queued up, which isn't
   // required, but probably doesn't hurt).
   VCOS_MUTEX_T busy_mutex;
} WFC_SERVER_STATE_T;

// Structure used to pass data to wfc_server_delete_element_iterator().
typedef struct
{
   KHRN_MAP_T *active_element_map;
   DISPMANX_UPDATE_HANDLE_T dm_update_handle;
} WFC_SERVER_DELETE_ELEMENT_T;

// Structure used to pass data to wfc_server_update_element_iterator().
typedef struct
{
   DISPMANX_UPDATE_HANDLE_T dm_update_handle;
   DISPMANX_DISPLAY_HANDLE_T dm_display_handle;
   uint32_t elements_updated;
   WFCRotation context_rotation;
   uint32_t current_element_bit;
} WFC_SERVER_UPDATE_ELEMENT_T;

//==============================================================================

static WFC_SERVER_STATE_T wfc_server_state;

//==============================================================================

static void wfc_server_init(void);
static void *wfc_server_task(void *arg);
static void wfc_server_empty_map_iterator
   (KHRN_MAP_T *map, WFCContext context, MEM_HANDLE_T context_handle, void *arg);
static void wfc_server_compose_iterator
   (KHRN_MAP_T *map, WFCContext context, MEM_HANDLE_T context_handle, void *arg);
static void wfc_server_update_element_iterator
   (KHRN_MAP_T *map, WFCElement element, MEM_HANDLE_T element_handle, void *arg);
static void wfc_server_update_element_attributes
   (DISPMANX_UPDATE_HANDLE_T dm_update_handle, WFCRotation context_rotation,
      const WFC_SERVER_ELEMENT_T *element_ptr);
static void wfc_server_update_element_image
   (DISPMANX_UPDATE_HANDLE_T dm_update_handle, const WFC_SERVER_ELEMENT_T *element_ptr,
      VC_IMAGE_T *image);
static void wfc_server_delete_element_iterator
   (KHRN_MAP_T *prev_element_map, WFCElement element, MEM_HANDLE_T element_handle, void *arg);
static void wfc_server_stream_update_callback(const WFCElement element_list[]);
static void wfc_server_dm_callback(DISPMANX_UPDATE_HANDLE_T update_handle, void *data);
static void wfc_server_release_buffer_iterator
   (KHRN_MAP_T *map, WFCElement element, MEM_HANDLE_T element_handle, void *arg);

//==============================================================================

#define WFC_CONTEXT_RESULT(width, height)    (((width) << 16) | (height))

uint32_t wfcIntCreateContext_impl
   (WFCContext context, uint32_t context_type, uint32_t screen_or_stream_num)
// Create a context.
{
   uint32_t result = WFC_ERROR_NONE;

   static bool is_initialised = false;

   if(!is_initialised)
   {
      is_initialised = true;
      wfc_server_init();
   } // if

   LOCK();

   logging_message(LOGGING_WFC, "wfcIntCreateContext_impl");
   WFC_BKPT();

   // Allocate memory
   MEM_HANDLE_T context_handle = MEM_ALLOC_STRUCT_EX(WFC_SERVER_CONTEXT_T, MEM_COMPACT_DISCARD);
   if (context_handle == MEM_HANDLE_INVALID)
      {result = WFC_ERROR_OUT_OF_MEMORY;}

   WFC_SERVER_CONTEXT_T *context_ptr = NULL;

   if(result == 0)
   {
      context_ptr = (WFC_SERVER_CONTEXT_T *) mem_lock(context_handle);

      // Assign parameters
      if(vcos_verify(context_ptr != NULL))
      {
         vcos_assert((context_type == WFC_CONTEXT_TYPE_ON_SCREEN)
            || (context_type == WFC_CONTEXT_TYPE_OFF_SCREEN));
         context_ptr->context_type = context_type;
         context_ptr->screen_or_stream_num = screen_or_stream_num;

         // Initialise element maps
         vcos_assert(khrn_map_init(&context_ptr->element_map[0], 8));
         vcos_assert(khrn_map_init(&context_ptr->element_map[1], 8));
      } // if
      else
         {result = WFC_ERROR_OUT_OF_MEMORY;}
   } // if

   if(result == 0)
   {
      // Add this context to global list
      if(!khrn_map_insert(&wfc_server_state.context_map, context, context_handle))
         {result = WFC_ERROR_OUT_OF_MEMORY;}
   } // if

   if(result == 0)
   {
      uint32_t width, height;

      if(context_type == WFC_CONTEXT_TYPE_ON_SCREEN)
      {
         // Get screen dimensions, for use by the WF-C client.
         context_ptr->dm_display_handle = dispmanx_display_open(screen_or_stream_num);
         if(vcos_verify(context_ptr->dm_display_handle != 0))
         {
            DISPMANX_MODEINFO_T dm_info;

            // For on-screen display, return display dimensions
            dispmanx_display_get_info(context_ptr->dm_display_handle, &dm_info);
            width = dm_info.width;
            height = dm_info.height;

            result = WFC_CONTEXT_RESULT(width, height);

         } // if
         else
            {result = WFC_ERROR_UNSUPPORTED;}
      } // if
      else
      {
         vcos_assert(0); // TODO
         width = 1; // Dummy values
         height = 1;
         result = WFC_CONTEXT_RESULT(width, height);
      }
   } // if

   if(context_handle != MEM_HANDLE_INVALID)
      {mem_unlock(context_handle);}

   UNLOCK();

   return result;
} // wfcIntCreateContext_impl()

//------------------------------------------------------------------------------

void wfcIntDestroyContext_impl(WFCContext ctx)
{
   // Ensure that any display activity is complete before continuing.
   logging_message(LOGGING_WFC, "wfcIntDestroyContext_impl: pre-lock");
   vcos_mutex_lock(&wfc_server_state.busy_mutex);

   LOCK();

   logging_message(LOGGING_WFC, "wfcIntDestroyContext_impl: post-lock");
   WFC_BKPT();

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, ctx);
   WFC_SERVER_CONTEXT_T *context_ptr = mem_lock(context_handle);

   // Remove from map
   khrn_map_delete(&wfc_server_state.context_map, ctx);

   KHRN_MAP_T *active_element_map;

   // If a commit is pending, cancel its effects
   if(context_ptr->commit_pending)
   {
      // Empty active map.
      active_element_map = &context_ptr->element_map[context_ptr->active_element_index];
      khrn_map_iterate(active_element_map, wfc_server_empty_map_iterator, NULL);
   } // if
   else
   {
      context_ptr->active_element_index = 1 - context_ptr->active_element_index;
   } // else

   // Delete all on-screen elements (stored in previous map).
   active_element_map = &context_ptr->element_map[context_ptr->active_element_index];
   KHRN_MAP_T *prev_element_map = &context_ptr->element_map[1 - context_ptr->active_element_index];

   DISPMANX_UPDATE_HANDLE_T dm_update_handle = dispmanx_update_start(0);
   WFC_SERVER_DELETE_ELEMENT_T delete_data =
   {
      active_element_map,
      dm_update_handle
   };
   khrn_map_iterate(prev_element_map, wfc_server_delete_element_iterator, (void *) &delete_data);
   dispmanx_update_submit_sync(dm_update_handle);

   // Delete
   vcos_assert(khrn_map_get_count(&context_ptr->element_map[0]) == 0);
   vcos_assert(khrn_map_get_count(&context_ptr->element_map[1]) == 0);
   khrn_map_term(&context_ptr->element_map[0]);
   khrn_map_term(&context_ptr->element_map[1]);

   mem_unlock(context_handle);
   mem_release(context_handle);

   // Close display
   dispmanx_display_close(context_ptr->dm_display_handle);

   UNLOCK();

   vcos_mutex_unlock(&wfc_server_state.busy_mutex);

} // wfcIntDestroyContext_impl()

//------------------------------------------------------------------------------

bool wfcIntCommitBegin_impl(WFCContext context, bool wait, WFCRotation rotation,
   WFCfloat background_red, WFCfloat background_green, WFCfloat background_blue,
   WFCfloat background_alpha)
// Begin composition. If neither waiting nor ready, return false; otherwise,
// return true.
{
   // Wait for composition to complete before continuing
   if(wait)
   {
      vcos_mutex_lock(&wfc_server_state.busy_mutex);
   }
   else
   {
      if(vcos_mutex_trylock(&wfc_server_state.busy_mutex) != VCOS_SUCCESS)
         {return false;}
   }

   LOCK();

   WFC_BKPT();

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, context);

   WFC_SERVER_CONTEXT_T *context_ptr = mem_lock(context_handle);

   // Update context attributes
   context_ptr->attributes.rotation = rotation;
   context_ptr->attributes.background_clr[WFC_BG_CLR_RED] = background_red;
   context_ptr->attributes.background_clr[WFC_BG_CLR_GREEN] = background_green;
   context_ptr->attributes.background_clr[WFC_BG_CLR_BLUE] = background_blue;
   context_ptr->attributes.background_clr[WFC_BG_CLR_ALPHA] = background_alpha;

   logging_message(LOGGING_WFC, "wfcIntCommitBegin_impl; active_element_index = %d",
      context_ptr->active_element_index);

   // Switch active element map (unless a previous commit is still pending, in
   // which case, we're going to empty the old map and start again).
   if(!context_ptr->commit_pending)
      {context_ptr->active_element_index = 1 - context_ptr->active_element_index;}

   KHRN_MAP_T *active_element_map = &context_ptr->element_map[context_ptr->active_element_index];
   bool active_element_map_is_empty = khrn_map_get_count(active_element_map) == 0;
   vcos_assert(context_ptr->commit_pending || active_element_map_is_empty);

   if(!active_element_map_is_empty)
   {
      // Empty active element map if there are two (or more) consecutive commits
      // without an intervening compose.
      khrn_map_iterate(active_element_map, wfc_server_empty_map_iterator, NULL);
   } // if
   vcos_assert(khrn_map_get_count(active_element_map) == 0);

   context_ptr->num_of_elements = 0;
   context_ptr->commit_pending = true;

   mem_unlock(context_handle);

   UNLOCK();

   return true;
} // wfcIntCommitBegin_impl()

//------------------------------------------------------------------------------

void wfcIntCommitAdd_impl(WFCContext context, WFCElement element, WFC_ELEMENT_ATTRIB_T *element_attr,
   WFCNativeStreamType source_stream, WFCNativeStreamType mask_stream)
// Add details for each element, with z-order from lowest to highest.
{
   LOCK();

   logging_message(LOGGING_WFC, "wfcIntCommitAdd_impl");
   WFC_BKPT();

   // Get context from list
   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, context);
   WFC_SERVER_CONTEXT_T *context_ptr = mem_lock(context_handle);

   // Get current and previous element maps
   vcos_assert((context_ptr->active_element_index == 0) || (context_ptr->active_element_index == 1));
   KHRN_MAP_T *prev_element_map = &context_ptr->element_map[1 - context_ptr->active_element_index];
   KHRN_MAP_T *active_element_map = &context_ptr->element_map[context_ptr->active_element_index];

   MEM_HANDLE_T element_handle = khrn_map_lookup(prev_element_map, element);

   // If element is not already on screen, create it.
   if(element_handle == MEM_HANDLE_INVALID)
   {
      element_handle = MEM_ALLOC_STRUCT_EX(WFC_SERVER_ELEMENT_T, MEM_COMPACT_DISCARD);
      vcos_assert(element_handle != MEM_HANDLE_INVALID);
      logging_message(LOGGING_WFC, "wfcIntCommitAdd_impl: create element [%d]", element_handle);
   } // if

   // Add element to active map.
   khrn_map_insert(active_element_map, element, element_handle);

   WFC_SERVER_ELEMENT_T *element_ptr = mem_lock(element_handle);

   element_ptr->element = element;
   element_ptr->attributes = *element_attr;
   element_ptr->layer = context_ptr->num_of_elements;

   // Diassociate element with old streams, and associate with the new ones
   wfc_stream_server_associate_element
      (element_ptr->source_stream, source_stream, element_ptr->element);
   element_ptr->source_stream = source_stream;
   logging_message(LOGGING_WFC, "wfcIntCommitAdd_impl: source_stream = 0x%X", source_stream);

   wfc_stream_server_associate_element
      (element_ptr->mask_stream, mask_stream, element_ptr->element);
   element_ptr->mask_stream = mask_stream;

   context_ptr->num_of_elements++;

   mem_unlock(element_handle);
   mem_unlock(context_handle);

   UNLOCK();
} // wfcIntCommitAdd_impl()

//------------------------------------------------------------------------------

void wfcIntCommitEnd_impl(WFCContext ctx)
{
   LOCK();

   WFC_BKPT();

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, ctx);

   WFC_SERVER_CONTEXT_T *context = mem_lock(context_handle);

   // Trigger autonomous composition (if enabled)
   if(context->is_activated)
   {
      vcos_event_signal(&wfc_server_state.compose_event);
   } // if

   mem_unlock(context_handle);

   UNLOCK();

   vcos_mutex_unlock(&wfc_server_state.busy_mutex);

} // wfcIntCommitEnd_impl()

//------------------------------------------------------------------------------

void wfcActivate_impl(WFCContext ctx)
{
   LOCK();

   logging_message(LOGGING_WFC, "wfcActivate_impl");

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, ctx);
   WFC_SERVER_CONTEXT_T *context = mem_lock(context_handle);

   context->is_activated = true;

   mem_unlock(context_handle);

   // Trigger composition immediately.
   vcos_event_signal(&wfc_server_state.compose_event);

   UNLOCK();
} // wfcActivate_impl()

//------------------------------------------------------------------------------

void wfcDeactivate_impl(WFCContext ctx)
{
   LOCK();

   logging_message(LOGGING_WFC, "wfcDeactivate_impl");

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, ctx);
   WFC_SERVER_CONTEXT_T *context = mem_lock(context_handle);

   context->is_activated = false;

   mem_unlock(context_handle);

   UNLOCK();
} // wfcDeactivate_impl()

//------------------------------------------------------------------------------

#define WFC_SRC_VAL_TO_DISPMANX(value) ((int32_t) ((value) * (1 << 16)))

void wfcCompose_impl(WFCContext context)
{
   logging_message(LOGGING_WFC, "wfcCompose_impl: start");

   // Wait for any commit to complete before continuing.
   vcos_mutex_lock(&wfc_server_state.busy_mutex);

   logging_message(LOGGING_WFC, "wfcCompose_impl: pre-lock");
   LOCK();
   logging_message(LOGGING_WFC, "wfcCompose_impl: post-lock");

   WFC_BKPT();

   MEM_HANDLE_T context_handle = khrn_map_lookup(&wfc_server_state.context_map, context);

   // For special case where context is deleted between a commit and a compose,
   // abort now.
   if(context_handle == MEM_HANDLE_INVALID)
   {
      logging_message(LOGGING_WFC, "wfcCompose_impl: context no longer present ########");
      UNLOCK();
      vcos_mutex_unlock(&wfc_server_state.busy_mutex);
      return;
   } // if

   WFC_SERVER_CONTEXT_T *context_ptr = mem_lock(context_handle);

   DISPMANX_UPDATE_HANDLE_T dm_update_handle = dispmanx_update_start(0);

   //--------------------------------------
   // Apply context attributes

   // Set background colour
   dispmanx_display_set_background(dm_update_handle, context_ptr->dm_display_handle,
      (uint8_t) (context_ptr->attributes.background_clr[WFC_BG_CLR_RED] * 255.0f),
      (uint8_t) (context_ptr->attributes.background_clr[WFC_BG_CLR_GREEN] * 255.0f),
      (uint8_t) (context_ptr->attributes.background_clr[WFC_BG_CLR_BLUE] * 255.0f));

   //--------------------------------------

   KHRN_MAP_T *active_element_map = &context_ptr->element_map[context_ptr->active_element_index];
   KHRN_MAP_T *prev_element_map = &context_ptr->element_map[1 - context_ptr->active_element_index];

   // Work out which elements are no longer being displayed, and delete them.
   WFC_SERVER_DELETE_ELEMENT_T delete_data =
   {
      active_element_map,
      dm_update_handle
   };
   khrn_map_iterate(prev_element_map, wfc_server_delete_element_iterator, (void *) &delete_data);
   vcos_assert(khrn_map_get_count(prev_element_map) == 0);

   // Update the properties of all elements that need updating.
   uint32_t current_element_bit = 0x1;
   context_ptr->elements_updated = 0x0;
   WFC_SERVER_UPDATE_ELEMENT_T update_data =
   {
      dm_update_handle,
      context_ptr->dm_display_handle,
      context_ptr->elements_updated,
      context_ptr->attributes.rotation,
      current_element_bit
   };
   khrn_map_iterate(active_element_map, wfc_server_update_element_iterator, (void *) &update_data);
   context_ptr->elements_updated = update_data.elements_updated;

   logging_message(LOGGING_WFC, "wfcCompose_impl: elements_updated = %X; active_element_index = %d",
      context_ptr->elements_updated, context_ptr->active_element_index);

   mem_unlock(context_handle);
   UNLOCK();

   // Send update (must unlock first, as callback function can (in theory) be
   // called immediately
   vcos_assert(sizeof(void *) >= sizeof(context_handle));
   dispmanx_update_submit(dm_update_handle,
      wfc_server_dm_callback, (void *) context_handle);
} // wfcCompose_impl()

//==============================================================================
// Static functions

static void wfc_server_init(void)
// Initialise server state, etc. Called only once.
{
   static VCOS_THREAD_T thread;
   uint32_t failure = 0;

   memset(&wfc_server_state, 0, sizeof(WFC_SERVER_STATE_T));

   vcos_mutex_create(&wfc_server_state.mutex, "wfc_server_mutex");

   LOCK();

   // Allocate map for contexts
   vcos_verify(khrn_map_init(&wfc_server_state.context_map, 8));

   // Initialise semaphores
   failure = vcos_event_create(&wfc_server_state.compose_event, "wfc_server_compose_event");
   vcos_assert(!failure);
   failure = vcos_mutex_create(&wfc_server_state.busy_mutex, "wfc_server_busy_sem");
   vcos_assert(!failure);

   // Create thread for autonomous composition.
   failure = vcos_thread_create(&thread, "wfc_server", NULL, wfc_server_task, NULL);
   vcos_assert(!failure);

   // Function to be called when stream is updated (for use with activated contexts)
   wfc_stream_server_set_update_callback(wfc_server_stream_update_callback);

   UNLOCK();

} // wfc_server_init()

//------------------------------------------------------------------------------

static void *wfc_server_task(void *arg)
// Task for dealing with autonomous composition
{
   while(1)
   {
      // Await semaphore
      vcos_event_wait(&wfc_server_state.compose_event);

      logging_message(LOGGING_WFC, "wfc_server_task");

      // Go through all contexts, and compose any that have just been updated.
      // TODO should only apply to activated contexts which actually have been updated!
      khrn_map_iterate(&wfc_server_state.context_map, wfc_server_compose_iterator, NULL);

   } // while

   return NULL;
} // wfc_server_task()

//------------------------------------------------------------------------------

static void wfc_server_empty_map_iterator
   (KHRN_MAP_T *map, WFCContext context, MEM_HANDLE_T context_handle, void *arg)
// Delete all elements in a map.
{
   khrn_map_delete(map, context);
} // wfc_server_empty_map_iterator()

//------------------------------------------------------------------------------

static void wfc_server_compose_iterator
   (KHRN_MAP_T *map, WFCContext context, MEM_HANDLE_T context_handle, void *arg)
// For each context, update any attributes and image data that may have changed.
{

   logging_message(LOGGING_WFC, "wfc_server_compose_iterator[%08X]", context);

   wfcCompose_impl(context);
} // wfc_server_compose_iterator()

//------------------------------------------------------------------------------

static void wfc_server_update_element_iterator
   (KHRN_MAP_T *map, WFCElement element, MEM_HANDLE_T element_handle, void *arg)
// For each element, update its properties ahead of sending it to the display.
{
   WFC_SERVER_UPDATE_ELEMENT_T *update_data = (WFC_SERVER_UPDATE_ELEMENT_T *) arg;
   VC_IMAGE_T *local_image = NULL;

   WFC_SERVER_ELEMENT_T *element_ptr = mem_lock(element_handle);

   // Get new element handle if this element hasn't been registered with
   // dispmanx yet.
   if(element_ptr->dm_handle == 0)
   {
      element_ptr->dm_handle =
         dispmanx_element_add(
            update_data->dm_update_handle, update_data->dm_display_handle, element_ptr->layer,
            NULL, NULL, NULL, DISPMANX_PROTECTION_NONE,
            NULL, NULL, DISPMANX_NO_ROTATE);
      vcos_assert(element_ptr->dm_handle != 0);
      logging_message(LOGGING_WFC, "wfc_server_update_element_iterator: add element[%d]",
         element_handle);
   } // if

   // Update attributes
   wfc_server_update_element_attributes(update_data->dm_update_handle,
      update_data->context_rotation, element_ptr);

   // Get latest image data
   local_image = wfc_stream_server_acquire_front_buffer_data(element_ptr->source_stream);

   // Update image if new data available, i.e. image pointer from buffer is not NULL.
   // However, if image was previously acquired, but has not yet been released
   // by this element, then it's not new.
   if((local_image != NULL) && ((update_data->elements_updated & update_data->current_element_bit) == 0))
   {
      // Mark this buffer as in use, and needing to be released later.
      wfc_server_update_element_image(update_data->dm_update_handle, element_ptr, local_image);
      update_data->elements_updated |= update_data->current_element_bit;
   }
   else
   {
      logging_message(LOGGING_WFC, "wfcCompose_impl: no image data[%d]", element);
   }

   update_data->current_element_bit <<= 1;

   mem_unlock(element_handle);

} // wfc_server_update_element_iterator()

//------------------------------------------------------------------------------

static void wfc_server_update_element_attributes
   (DISPMANX_UPDATE_HANDLE_T dm_update_handle, WFCRotation context_rotation,
      const WFC_SERVER_ELEMENT_T *element_ptr)
{
   VC_RECT_T dest_rect, src_rect;

   dest_rect.x = element_ptr->attributes.dest_rect[WFC_RECT_X];
   dest_rect.y = element_ptr->attributes.dest_rect[WFC_RECT_Y];
   dest_rect.width = element_ptr->attributes.dest_rect[WFC_RECT_WIDTH];
   dest_rect.height = element_ptr->attributes.dest_rect[WFC_RECT_HEIGHT];

   src_rect.x = WFC_SRC_VAL_TO_DISPMANX(element_ptr->attributes.src_rect[WFC_RECT_X]);
   src_rect.y = WFC_SRC_VAL_TO_DISPMANX(element_ptr->attributes.src_rect[WFC_RECT_Y]);
   src_rect.width = WFC_SRC_VAL_TO_DISPMANX(element_ptr->attributes.src_rect[WFC_RECT_WIDTH]);
   src_rect.height = WFC_SRC_VAL_TO_DISPMANX(element_ptr->attributes.src_rect[WFC_RECT_HEIGHT]);

   // TODO apply effects of context rotation
   vcos_assert(context_rotation == WFC_ROTATION_0); // TODO implement

   // TODO ideally, only update values that have actually changed
   dispmanx_element_change_dest_rect
      (dm_update_handle, element_ptr->dm_handle, &dest_rect);
   dispmanx_element_change_src_rect
      (dm_update_handle, element_ptr->dm_handle, &src_rect);

   if(element_ptr->attributes.flip)
   {
      dispmanx_element_change_transform(dm_update_handle, element_ptr->dm_handle, DISPMANX_FLIP_VERT);
   } // if

   logging_message(LOGGING_WFC, "wfcCompose_impl: attributes updated");

} // wfc_server_update_element_attributes()

//------------------------------------------------------------------------------

static void wfc_server_update_element_image
   (DISPMANX_UPDATE_HANDLE_T dm_update_handle, const WFC_SERVER_ELEMENT_T *element_ptr,
      VC_IMAGE_T *image)
{
   logging_message(LOGGING_WFC, "wfcCompose_impl: new image data[%d]", element_ptr->layer);

   vcos_assert(element_ptr->dm_handle != 0);
   vc_image_lock(image, image);
   dispmanx_element_change_source(dm_update_handle, element_ptr->dm_handle, image);
   vc_image_unlock(image);

   logging_message(LOGGING_WFC, "wfcCompose_impl: image data sent to display");

} // wfc_server_update_element_image()


//------------------------------------------------------------------------------

static void wfc_server_delete_element_iterator
   (KHRN_MAP_T *prev_element_map, WFCElement element, MEM_HANDLE_T element_handle, void *arg)
// Iterator used in wfcIntCommitEnd_impl() to delete all elements no longer
// being displayed.
{
   WFC_SERVER_DELETE_ELEMENT_T *delete_data = (WFC_SERVER_DELETE_ELEMENT_T *) arg;

   logging_message(LOGGING_WFC, "wfc_server_delete_element_iterator; element = %d", element);

   // Remove from (previous) map
   khrn_map_delete(prev_element_map, element);

   // If element is not in active map, but was in previous one, then delete it.
   if(khrn_map_lookup(delete_data->active_element_map, element) == MEM_HANDLE_INVALID)
   {
      logging_message(LOGGING_WFC, "wfc_server_delete_element_iterator: delete element [%d]", element);

      WFC_SERVER_ELEMENT_T *element_ptr = mem_lock(element_handle);

      // Remove from display
      dispmanx_element_remove(delete_data->dm_update_handle, element_ptr->dm_handle);

      // Disassociate streams
      wfc_stream_server_associate_element
         (element_ptr->source_stream, WFC_INVALID_HANDLE, element_ptr->element);
      wfc_stream_server_associate_element
         (element_ptr->mask_stream, WFC_INVALID_HANDLE, element_ptr->element);

      mem_unlock(element_handle);

      // Delete
      mem_release(element_handle);
   } // if

} // wfc_server_delete_element_iterator()

//------------------------------------------------------------------------------

static void wfc_server_stream_update_callback(const WFCElement element_list[])
// Callback called by stream for activated context(s), to indicate elements
// whose data has been updated.
{
   // TODO for each element in list, need to check all contexts to see if element
   // is contained within - or is that dealt with sufficiently in wfcCompose_impl?

   // Signal to task that an update has occurred.
   vcos_event_signal(&wfc_server_state.compose_event);
} // wfc_server_stream_update_callback()

//------------------------------------------------------------------------------

static void wfc_server_dm_callback(DISPMANX_UPDATE_HANDLE_T update_handle, void *data)
// Callback following completion of dispmanx_update_submit, when image data has been
// updated (called in wfcCompose_impl()).
{
   logging_message(LOGGING_WFC, "wfc_server_dm_callback: pre lock");
   LOCK();
   logging_message(LOGGING_WFC, "wfc_server_dm_callback: post lock");

   MEM_HANDLE_T context_handle = (MEM_HANDLE_T) data;
   WFC_SERVER_CONTEXT_T *context_ptr = mem_lock(context_handle);

   // Release buffers
   if(context_ptr->elements_updated != 0x0)
   {
      logging_message(LOGGING_WFC, "wfc_server_dm_callback: image data");
      khrn_map_iterate(&context_ptr->element_map[context_ptr->active_element_index],
         wfc_server_release_buffer_iterator, (void *) &context_ptr->elements_updated);
   } // if
   else
      {logging_message(LOGGING_WFC, "wfc_server_dm_callback: no image data");}

   context_ptr->commit_pending = false;

   mem_unlock(context_handle);

   UNLOCK();
   vcos_mutex_unlock(&wfc_server_state.busy_mutex);

   // TODO: signal to host, for use with wfcFence()

} // wfc_server_dm_callback_image()

//------------------------------------------------------------------------------

static void wfc_server_release_buffer_iterator
   (KHRN_MAP_T *map, WFCElement element, MEM_HANDLE_T element_handle, void *arg)
// Iterator used in wfc_server_dm_callback_image() to release buffers updated
// by the now-complete composition.
{
   uint32_t *elements_updated = (uint32_t *) arg;

   WFC_SERVER_ELEMENT_T *element_ptr = mem_lock(element_handle);

   if(*elements_updated & 0x1)
   {
      wfc_stream_server_release_front_buffer_data(element_ptr->source_stream);
   } // if
   *elements_updated >>= 1;

   mem_unlock(element_handle);

} // wfc_server_release_buffer_iterator()

//==============================================================================
