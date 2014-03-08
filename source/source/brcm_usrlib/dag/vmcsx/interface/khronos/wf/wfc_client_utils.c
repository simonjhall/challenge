/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Khronos
Module   :  OpenWF-C

FILE DESCRIPTION
Additional functions used by wfc_client.c
=============================================================================*/

#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#include "interface/khronos/wf/wfc_client_utils.h"

#include "vcfw/logging/logging.h" // TODO logging:remove

#ifdef RPC_DIRECT
   #include "interface/khronos/wf/wfc_int_impl.h" /* for _impl function calls */
#endif

//==============================================================================

#define LOGGING_WFC  LOGGING_GENERAL

//==============================================================================

static void wfc_source_or_mask_destroy_actual(WFC_SOURCE_OR_MASK_T *source_or_mask_ptr);

//==============================================================================

WFC_DEVICE_T *wfc_device_create()
{
   WFC_DEVICE_T *device = khrn_platform_malloc(sizeof(WFC_DEVICE_T), "WFC_DEVICE_T");

   if(device != NULL)
   {
      device->error = WFC_ERROR_NONE;
      wfc_link_init_empty(&device->contexts);
   }

   return device;
} // wfc_device_create()

//------------------------------------------------------------------------------

void wfc_device_destroy(WFC_DEVICE_T *device)
{
   /*
      Destroy all of the contexts associated with the device. This will in turn
      destroy all of the sources, masks and elements associated with each context.
   */

   wfc_link_iterate(&device->contexts, (WFC_LINK_CALLBACK_T) wfc_context_destroy);

   khrn_platform_free(device);
} // wfc_device_destroy()

//==============================================================================

#define WFC_CONTEXT_WIDTH(data)  ((data) >> 16)
#define WFC_CONTEXT_HEIGHT_OR_ERR(data) ((data) & 0xFFFF)

WFC_CONTEXT_T *wfc_context_create
   (WFC_DEVICE_T *device_ptr, WFCContextType context_type,
      uint32_t screen_or_stream_num, WFCErrorCode *error)
{
   WFC_CONTEXT_T *context_ptr =
      khrn_platform_malloc(sizeof(WFC_CONTEXT_T), "WFC_CONTEXT_T");
   const WFC_CONTEXT_ATTRIB_T context_attrib_default = WFC_CONTEXT_ATTRIB_DEFAULT;

   if(context_ptr != NULL)
   {
      uint32_t response = RPC_CALL3_RES(wfcIntCreateContext_impl,
                                   WFCINTCREATECONTEXT_ID,
                                   RPC_UINT((uint32_t) context_ptr),
                                   RPC_ENUM(context_type),
                                   RPC_UINT(screen_or_stream_num));

      uint32_t height_or_err = WFC_CONTEXT_HEIGHT_OR_ERR(response);
      uint32_t width = WFC_CONTEXT_WIDTH(response);

      if(width != 0)
      {
         wfc_link_init_null(&context_ptr->link);

         context_ptr->device = device_ptr;
         wfc_link_init_empty(&context_ptr->sources);
         wfc_link_init_empty(&context_ptr->masks);

         wfc_link_init_empty(&context_ptr->elements_not_in_scene);
         wfc_link_init_empty(&context_ptr->elements_in_scene);
         context_ptr->active = false;

         context_ptr->type = context_type;
         context_ptr->height = height_or_err;
         context_ptr->width = width;
         context_ptr->attributes = context_attrib_default;
      } // if
      else
      {
         khrn_platform_free(context_ptr);

         context_ptr = NULL;

         *error = (WFCErrorCode) response;
      } // else
   } // if
   else
      {*error = WFC_ERROR_OUT_OF_MEMORY;}

   return context_ptr;
} // wfc_context_create()

//------------------------------------------------------------------------------

void wfc_context_destroy(WFC_CONTEXT_T *context)
{
   // Remove from parent device's list of contexts.
   wfc_link_detach(&context->link);

   // Destroy all components
   wfc_link_iterate(&context->elements_in_scene, (WFC_LINK_CALLBACK_T) wfc_element_destroy);
   wfc_link_iterate(&context->elements_not_in_scene, (WFC_LINK_CALLBACK_T) wfc_element_destroy);
   wfc_link_iterate(&context->sources, (WFC_LINK_CALLBACK_T) wfc_source_or_mask_destroy_actual);
   wfc_link_iterate(&context->masks, (WFC_LINK_CALLBACK_T) wfc_source_or_mask_destroy_actual);

   RPC_CALL1(wfcIntDestroyContext_impl,
             WFCINTDESTROYCONTEXT_ID,
             RPC_UINT((WFCContext) context));
   RPC_FLUSH();

   khrn_platform_free(context);
} // wfc_context_destroy()

//==============================================================================

WFCHandle wfc_source_or_mask_create
   (bool is_source, WFCDevice dev, WFCContext ctx,
      WFCNativeStreamType stream, const WFCint *attribList)
// Create a new image provider and associate it with a stream.
// wfcCreateSourceFromStream() and wfcCreateMaskFromStream() are essentially
// wrappers for this function.
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;

   WFCHandle handle = WFC_INVALID_HANDLE;

   if(context_ptr->device == device_ptr)
   {
      if(!wfc_check_no_attribs(attribList))
      {
         wfc_set_error(device_ptr, WFC_ERROR_BAD_ATTRIBUTE);
      }
      else if(context_ptr->output_stream == stream)
      {
         // Verify that context isn't an input for this stream
         wfc_set_error(device_ptr, WFC_ERROR_IN_USE);
      } // else if
      else
      {
         WFC_SOURCE_OR_MASK_T *source_or_mask_ptr = khrn_platform_malloc
            (sizeof(WFC_SOURCE_OR_MASK_T), "WFC_SOURCE_OR_MASK_T");

         if(source_or_mask_ptr != NULL)
         {
            memset(source_or_mask_ptr, 0, sizeof(WFC_SOURCE_OR_MASK_T));
            // Note that refcount is initialised to zero here, as a source or mask is
            // only in use when it is linked to an element.

            wfc_link_init_null(&source_or_mask_ptr->link);

            source_or_mask_ptr->is_source = is_source;
            source_or_mask_ptr->context = context_ptr;
            source_or_mask_ptr->stream = stream;

            if(is_source)
               {wfc_link_attach(&source_or_mask_ptr->link, &context_ptr->sources);}
            else
               {wfc_link_attach(&source_or_mask_ptr->link, &context_ptr->masks);}
            handle = (WFCHandle) source_or_mask_ptr;
         } // if
         else
            {wfc_set_error(device_ptr, WFC_ERROR_OUT_OF_MEMORY);}
      } // else
   } // if
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   return handle;
} // wfc_source_or_mask_create()

//------------------------------------------------------------------------------

void wfc_source_or_mask_destroy(WFCDevice dev, WFCHandle source_or_mask)
// Destroy an image provider and dissociate its stream. wfcDestroySource() and
// wfcDestroyMask() are essentially wrappers for this function.
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_SOURCE_OR_MASK_T *source_or_mask_ptr = (WFC_SOURCE_OR_MASK_T *) source_or_mask;

   if(source_or_mask_ptr->context->device == device_ptr)
      {wfc_source_or_mask_destroy_actual(source_or_mask_ptr);}
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}
} // wfc_source_or_mask_destroy()

//------------------------------------------------------------------------------

void wfc_source_or_mask_acquire(WFC_SOURCE_OR_MASK_T *source_or_mask_ptr)
// Indicate that the image provider is now linked to an element.
{
   if(vcos_verify(source_or_mask_ptr != NULL))
      {source_or_mask_ptr->refcount++;}
} // wfc_source_or_mask_acquire()

//------------------------------------------------------------------------------

void wfc_source_or_mask_release(WFC_SOURCE_OR_MASK_T *source_or_mask_ptr)
// Indicate the the image provider is no longer linked to an element;
// destroy if previously requested.
{
   if(source_or_mask_ptr != NULL)
   {
      if(source_or_mask_ptr->refcount > 0)
         {source_or_mask_ptr->refcount--;}

      // If no-one is using this source or mask, and a request has previously
      // been made to destroy it, do so now.
      if((source_or_mask_ptr->refcount == 0) && source_or_mask_ptr->destroy_pending)
         {wfc_source_or_mask_destroy_actual(source_or_mask_ptr);}
   } // if
}

//==============================================================================

void wfc_element_destroy(WFC_ELEMENT_T *element_ptr)
{
   logging_message(LOGGING_WFC, "wfc_element_destroy"); // TODO logging: remove

   // Release source and mask (if present); destroy if previously requested.
   wfc_source_or_mask_release(element_ptr->source);
   wfc_source_or_mask_release(element_ptr->mask);

   element_ptr->source = NULL;
   element_ptr->mask = NULL;

   wfc_link_detach(&element_ptr->link);

   khrn_platform_free(element_ptr);
} // wfc_element_destroy()

//==============================================================================

void wfc_commit_callback(WFC_ELEMENT_T *element)
{
   WFCNativeStreamType source_stream =
      element->source ? element->source->stream : WFC_INVALID_HANDLE;
   WFCNativeStreamType mask_stream =
      element->mask ? element->mask->stream : WFC_INVALID_HANDLE;

#ifdef RPC_DIRECT
   wfcIntCommitAdd_impl((WFCContext) element->context,
      (WFCElement) element, &element->attributes, source_stream, mask_stream);
#else
   RPC_CALL18(wfcIntCommitAdd_impl,
      WFCINTCOMMITADD_ID,
      RPC_UINT((WFCContext) element->context),
      RPC_UINT((WFCElement) element),
      RPC_INT(element->attributes.dest_rect[0]),
      RPC_INT(element->attributes.dest_rect[1]),
      RPC_INT(element->attributes.dest_rect[2]),
      RPC_INT(element->attributes.dest_rect[3]),
      RPC_FLOAT(element->attributes.src_rect[0]),
      RPC_FLOAT(element->attributes.src_rect[1]),
      RPC_FLOAT(element->attributes.src_rect[2]),
      RPC_FLOAT(element->attributes.src_rect[3]),
      RPC_BOOLEAN(element->attributes.flip),
      RPC_ENUM(element->attributes.rotation),
      RPC_ENUM(element->attributes.scale_filter),
      RPC_BITFIELD(element->attributes.transparency_types),
      RPC_FLOAT(element->attributes.global_alpha),
      RPC_UINT(source_stream),
      RPC_UINT(mask_stream),
      RPC_UINT(0)); // dummy parameter; currently unused.
#endif

} // wfc_commit_callback()

//==============================================================================

void wfc_set_error(WFC_DEVICE_T *device, WFCErrorCode error)
{
   if (device->error == WFC_ERROR_NONE)
      device->error = error;
}

//==============================================================================

bool wfc_check_no_attribs(const WFCint *attribList)
{
   return !attribList || *attribList == WFC_NONE;
}

//------------------------------------------------------------------------------

bool wfc_device_check_filters(const WFCint *filterList)
{
   // TODO tidy

   bool found_screen = WFC_FALSE;

   if (!filterList)
      return WFC_TRUE;

   while (1) {
      int name = *filterList++;
      if (name == WFC_NONE)
         return WFC_TRUE;
      else {
         int value = *filterList++;
         switch (name) {
         case WFC_DEVICE_FILTER_SCREEN_NUMBER:
            if (value <= 0 || value >= MAX_DISPLAYS || found_screen)
               return WFC_FALSE;
            found_screen = WFC_TRUE;
            break;
         }
      }
   }
}

//------------------------------------------------------------------------------

bool wfc_is_rotation(WFCint value)
{
   return value == WFC_ROTATION_0   ||
          value == WFC_ROTATION_90  ||
          value == WFC_ROTATION_180 ||
          value == WFC_ROTATION_270;
}

//------------------------------------------------------------------------------

bool wfc_is_scale_filter(WFCint value)
{
   return value == WFC_SCALE_FILTER_NONE   ||
          value == WFC_SCALE_FILTER_FASTER ||
          value == WFC_SCALE_FILTER_BETTER;
}

//------------------------------------------------------------------------------

bool wfc_are_transparency_types(WFCint value)
{
   return value == WFC_TRANSPARENCY_NONE ||
          value == WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA ||
          value == WFC_TRANSPARENCY_SOURCE ||
          value == WFC_TRANSPARENCY_MASK ||
          value == (WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA | WFC_TRANSPARENCY_SOURCE) ||
          value == (WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA | WFC_TRANSPARENCY_MASK);
}

//------------------------------------------------------------------------------

int32_t wfc_round(float f)
{
   int result = (int)f;
   if (f>=0)
      if ((f-result)>=0.5) return ++result; else return result;
   else
      if ((f-result)<=-0.5) return --result; else return result;
}

//==============================================================================

void wfc_link_detach(WFC_LINK_T *link)
{
   vcos_assert(link != NULL);
   if (link->next) {
      /*
         never unlink a base link
      */

      vcos_assert(link->next != link);
      vcos_assert(link->prev != link);

      link->next->prev = link->prev;
      link->prev->next = link->next;

      link->prev = NULL;
      link->next = NULL;
   }
}

//------------------------------------------------------------------------------

void wfc_link_attach(WFC_LINK_T *link, WFC_LINK_T *prev)
{
   wfc_link_detach(link);

   link->prev = prev;
   link->next = prev->next;

   link->prev->next = link;
   link->next->prev = link;
}

//------------------------------------------------------------------------------

void wfc_link_init_null(WFC_LINK_T *link)
{
   link->prev = NULL;
   link->next = NULL;
}

//------------------------------------------------------------------------------

void wfc_link_init_empty(WFC_LINK_T *link)
{
   link->prev = link;
   link->next = link;
}

//------------------------------------------------------------------------------

void wfc_link_iterate(WFC_LINK_T *link, WFC_LINK_CALLBACK_T func)
{
   WFC_LINK_T *curr = link;
   WFC_LINK_T *next = curr->next;

   while (next != link) {
      curr = next;
      next = curr->next;

      func(curr);
   }
}

//------------------------------------------------------------------------------

#if 0 // TODO not used
bool wfc_link_is_empty(WFC_LINK_T *link)
{
   return link->next == link;
}
#endif

#if 0
int wfc_link_get_size(WFC_LINK_T *link)
{
   int size = 0;

   WFC_LINK_T *curr = link;
   WFC_LINK_T *next = curr->next;

   while (next != link) {
      curr = next;
      next = curr->next;

      size++;
   }

   return size;
}
#endif

//==============================================================================

static void wfc_source_or_mask_destroy_actual(WFC_SOURCE_OR_MASK_T *source_or_mask_ptr)
{
   source_or_mask_ptr->destroy_pending = true;

   if(source_or_mask_ptr->refcount == 0)
   {
      logging_message(LOGGING_WFC, "wfc_source_or_mask_destroy_actual %d",
         (uint32_t) source_or_mask_ptr);

      // Remove from parent context's list of sources or masks.
      wfc_link_detach(&source_or_mask_ptr->link);

      // Destroy.
      khrn_platform_free(source_or_mask_ptr);
   } // if
   else
   {
      logging_message(LOGGING_WFC, "wfc_source_or_mask_destroy_actual: pending %d",
         (uint32_t) source_or_mask_ptr);
   } // else
} // wfc_source_or_mask_destroy_actual()

//==============================================================================
//------------------------------------------------------------------------------
