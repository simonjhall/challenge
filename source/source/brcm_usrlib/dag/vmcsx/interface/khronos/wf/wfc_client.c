/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Khronos
Module   :  OpenWF-C

FILE DESCRIPTION
OpenWF Composition client.
   The OpenWF-C specification, and other documentation, can be found at:
   http://www.khronos.org/registry/wf/
=============================================================================*/

#include <stdlib.h>

#include "interface/khronos/include/WF/wfc.h"

#include "interface/khronos/common/khrn_client_rpc.h"
#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/wf/wfc_client_utils.h"

#ifdef RPC_DIRECT
   #include "interface/khronos/wf/wfc_int_impl.h" /* for _impl function calls */
#endif

//==============================================================================

#define OUR_DEVICE_ID 1

#define WFC_LOCK()      (platform_mutex_acquire(&wfc_client_state.mutex))
#define WFC_UNLOCK()    (platform_mutex_release(&wfc_client_state.mutex))

//==============================================================================
// TODO streams
extern void platform_get_stream_dimensions(WFCNativeStreamType stream, uint32_t *width, uint32_t *height);
extern uint32_t platform_get_stream_handle(WFCNativeStreamType stream);

//==============================================================================

typedef struct
{
   bool is_initialised;
   PLATFORM_MUTEX_T mutex;
} WFC_CLIENT_STATE_T;

//==============================================================================

static WFC_CLIENT_STATE_T wfc_client_state;

//==============================================================================
// Device functions

WFC_API_CALL WFCint WFC_APIENTRY
    wfcEnumerateDevices(WFCint *deviceIds, WFCint deviceIdsCount,
        const WFCint *filterList) WFC_APIEXIT
{
   // TODO tidy

   if (wfc_device_check_filters(filterList)) {
      if (deviceIds)
         if (deviceIdsCount > 0) {
            *deviceIds = OUR_DEVICE_ID;
            return 1;
         } else
            return 0;
      else
         return 1;
   }

   return 0;
}

//------------------------------------------------------------------------------

WFC_API_CALL WFCDevice WFC_APIENTRY
    wfcCreateDevice(WFCint deviceId, const WFCint *attribList) WFC_APIEXIT
{
   WFCDevice result = WFC_INVALID_HANDLE;

   // This function will be called before anything else can be created, so is
   // a good place to initialise the state.
   if(!wfc_client_state.is_initialised)
   {
      wfc_client_state.is_initialised = true;
      platform_mutex_create(&wfc_client_state.mutex);
      // Ensure that RPC is initialised
      CLIENT_GET_THREAD_STATE();
   } // if

   WFC_LOCK();

   if ((deviceId == WFC_DEFAULT_DEVICE_ID || deviceId == OUR_DEVICE_ID)
      && wfc_check_no_attribs(attribList))
   {
      WFC_DEVICE_T *device = wfc_device_create();

      if(device != NULL)
         {result = (WFCDevice) device;}
   }

   WFC_UNLOCK();

   return result;
} // wfcCreateDevice()

//------------------------------------------------------------------------------

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcGetError(WFCDevice dev) WFC_APIEXIT
{
   WFCErrorCode result;

   WFC_LOCK();

   if(dev != WFC_INVALID_HANDLE)
   {
      WFC_DEVICE_T *device = (WFC_DEVICE_T *) dev;

      result = device->error;
      device->error = WFC_ERROR_NONE;
   } else
      {result = WFC_ERROR_BAD_DEVICE;}

   WFC_UNLOCK();

   return result;
} // wfcGetError()

//------------------------------------------------------------------------------

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetDeviceAttribi(WFCDevice dev, WFCDeviceAttrib attrib) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;

   WFCint result = 0;

   WFC_LOCK();

   switch (attrib) {
   case WFC_DEVICE_CLASS:
      result = WFC_DEVICE_CLASS_FULLY_CAPABLE;
      break;
   case WFC_DEVICE_ID:
      result = OUR_DEVICE_ID;
      break;
   default:
      wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
      break;
   }

   WFC_UNLOCK();

   return result;
}

//------------------------------------------------------------------------------

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcDestroyDevice(WFCDevice dev) WFC_APIEXIT
{
   WFCErrorCode result;

   WFC_LOCK();

   if (dev != WFC_INVALID_HANDLE)
   {
      WFC_DEVICE_T *device = (WFC_DEVICE_T *) dev;

      wfc_device_destroy(device);

      result = WFC_ERROR_NONE;
   } // if
   else
      {result = WFC_ERROR_BAD_DEVICE;}

   WFC_UNLOCK();

   return result;
} // wfcDestroyDevice()

//==============================================================================
// Context functions

WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOnScreenContext(WFCDevice dev,
        WFCint screenNumber,
        const WFCint *attribList) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;

   WFCContext context = WFC_INVALID_HANDLE;

   WFC_LOCK();

   if (screenNumber < 0 || screenNumber >= MAX_DISPLAYS)
      {wfc_set_error(device_ptr, WFC_ERROR_UNSUPPORTED);}
   else if (!wfc_check_no_attribs(attribList))
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_ATTRIBUTE);}
   else
   {
      WFC_CONTEXT_T *context_ptr;

      WFCErrorCode error; 

      // Create on-screen context_ptr
      context_ptr = wfc_context_create
         (device_ptr, WFC_CONTEXT_TYPE_ON_SCREEN, screenNumber, &error);

      // Insert new context_ptr into list of contexts
      if(context_ptr)
      {
         wfc_link_attach(&context_ptr->link, &device_ptr->contexts);

         context = (WFCContext) context_ptr;
      } // if
      else
         {wfc_set_error(device_ptr, error);}
   } // else

   WFC_UNLOCK();

   return context;
} // wfcCreateOnScreenContext()

//------------------------------------------------------------------------------

WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOffScreenContext(WFCDevice dev,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WFCContext result = WFC_INVALID_HANDLE;
#if 0 // TODO wfcCreateOffScreenContext()
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;

   uint32_t handle = 0;

   handle = platform_get_stream_handle(stream);

   WFC_LOCK();

   if (!handle) {
      wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
   } else if (!wfc_check_no_attribs(attribList)) {
      wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
   } else {
      WFC_CONTEXT_T *context;

      WFCErrorCode error; 

      uint32_t width = 0;
      uint32_t height = 0;

      platform_get_stream_dimensions(stream, &width, &height);
      // TODO may need different function here for off-screen.
      context = wfc_context_create(device, WFC_CONTEXT_TYPE_OFF_SCREEN, handle, width, height, &error);

      if (context) {
         wfc_link_attach(&context->link, &device->contexts);

         result = (WFCContext)context;
      } else
         wfc_set_error(device, error);
   }
   WFC_UNLOCK();
#endif

   vcos_assert(0);

   return result;
} // wfcCreateOffScreenContext()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcCommit(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;
   bool ready = false;

   WFC_LOCK();

   // Send data for all elements
   if (context_ptr->device == device_ptr)
   {
      // Send commit header and context attributes.
      // If we're not waiting, we need to know if the server is ready for a new
      // commit. If it isn't, we will not proceed.
      ready = RPC_CALL7_RES(
         wfcIntCommitBegin_impl,
         WFCINTCOMMITBEGIN_ID,
         RPC_UINT((WFCContext) context_ptr),
         RPC_BOOLEAN(wait),
         RPC_ENUM(context_ptr->attributes.rotation),
         RPC_FLOAT(context_ptr->attributes.background_clr[0]),
         RPC_FLOAT(context_ptr->attributes.background_clr[1]),
         RPC_FLOAT(context_ptr->attributes.background_clr[2]),
         RPC_FLOAT(context_ptr->attributes.background_clr[3])
         );

      if(wait || ready)
      {
         // Send all elements to server.
         wfc_link_iterate(&context_ptr->elements_in_scene, (WFC_LINK_CALLBACK_T) wfc_commit_callback);

         // Send commit footer.
         RPC_CALL1(wfcIntCommitEnd_impl,
            WFCINTCOMMITEND_ID,
            RPC_UINT((WFCContext) context_ptr));
         RPC_FLUSH();
      } // if
      else
         {wfc_set_error(device_ptr, WFC_ERROR_BUSY);}

   } // if
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcCommit()

//------------------------------------------------------------------------------

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *)dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *)ctx;

   WFCint result = 0;

   WFC_LOCK();

   if(context_ptr->device == device_ptr)
   {
      switch (attrib)
      {
         case WFC_CONTEXT_TYPE:
            result = context_ptr->type;
            break;
         case WFC_CONTEXT_TARGET_WIDTH:
            result = context_ptr->width;
            break;
         case WFC_CONTEXT_TARGET_HEIGHT:
            result = context_ptr->height;
            break;
         case WFC_CONTEXT_LOWEST_ELEMENT:
         {
            WFC_LINK_T *first = &context_ptr->elements_in_scene;
            WFC_LINK_T *current = first;

            if(first->next == first)
            {
               // List is empty.
               result = WFC_INVALID_HANDLE;
            } // if
            else
            {
               // Move to last element in list.
               do
                  {current = current->next;}
               while(current->next != first);
               result = (WFCint) current;
            } // else
            break;
         }
         case WFC_CONTEXT_ROTATION:
            result = context_ptr->attributes.rotation;
            break;
         case WFC_CONTEXT_BG_COLOR:
            result = (WFCint) (context_ptr->attributes.background_clr[WFC_BG_CLR_RED]    * 255.0f) << 24 |
                     (WFCint) (context_ptr->attributes.background_clr[WFC_BG_CLR_GREEN]  * 255.0f) << 16 |
                     (WFCint) (context_ptr->attributes.background_clr[WFC_BG_CLR_BLUE]   * 255.0f) << 8  |
                     (WFCint) (context_ptr->attributes.background_clr[WFC_BG_CLR_ALPHA]  * 255.0f);
            break;
         default:
            wfc_set_error(device_ptr, WFC_ERROR_BAD_ATTRIBUTE);
            break;
      } // switch
   } // if
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();

   return result;
} // wfcGetContextAttribi()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcGetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;

   WFC_LOCK();

   if(context_ptr->device == device_ptr)
   {
      uint32_t i;

      switch (attrib)
      {
         case WFC_CONTEXT_BG_COLOR:
            if(vcos_verify((values != NULL)
               && (((uint32_t) values && 0x3) == 0) && (count == WFC_BG_CLR_SIZE)))
            {
               for (i = 0; i < WFC_BG_CLR_SIZE; i++)
                  {values[i] = context_ptr->attributes.background_clr[i];}
            } // if
            else
               {wfc_set_error(device_ptr, WFC_ERROR_ILLEGAL_ARGUMENT);}
            break;
         default:
            wfc_set_error(device_ptr, WFC_ERROR_BAD_ATTRIBUTE);
            break;
      } // switch
   }
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcGetContextAttribfv()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint value) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;

   WFC_LOCK();

   if (context_ptr->device == device_ptr)
   {
      switch(attrib)
      {
         case WFC_CONTEXT_ROTATION:
            if(wfc_is_rotation(value))
               {context_ptr->attributes.rotation = value;}
            else
               {wfc_set_error(device_ptr, WFC_ERROR_ILLEGAL_ARGUMENT);}
            break;
         case WFC_CONTEXT_BG_COLOR:
         {
            int32_t i;
            for(i = WFC_BG_CLR_SIZE - 1; i >= 0; i--)
            {
               context_ptr->attributes.background_clr[i]
                  = ((float) (value & 0xff)) / 255.0f;
               value >>= 8;
            } // for
            break;
         }
         default:
            wfc_set_error(device_ptr, WFC_ERROR_BAD_ATTRIBUTE);
            break;
      } // switch
   } // if
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcSetContextAttribi()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_CONTEXT_T *context = (WFC_CONTEXT_T *)ctx;

   WFC_LOCK();

   if (context->device == device)
   {
      int i;

      switch (attrib)
      {
         case WFC_CONTEXT_BG_COLOR:
            if(vcos_verify((values != NULL) && (((uint32_t) values & 3) == 0)
               && (count == WFC_BG_CLR_SIZE)))
            {
               for (i = 0; i < WFC_BG_CLR_SIZE; i++)
                  {context->attributes.background_clr[i] = values[i];}
            } // if
            else
               {wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);}
            break;
         default:
            wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
            break;
      } // switch
   } // if
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcSetContextAttribfv()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyContext(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;

   WFC_LOCK();

   if (context_ptr->device == device_ptr)
      {wfc_context_destroy(context_ptr);}
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
}

//==============================================================================
// Source functions

WFC_API_CALL WFCSource WFC_APIENTRY
    wfcCreateSourceFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WFCSource source;

   WFC_LOCK();

   source = (WFCSource) wfc_source_or_mask_create
      (WFC_IS_SOURCE, dev, ctx, stream, attribList);

   WFC_UNLOCK();

   return source;
} // wfcCreateSourceFromStream()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcDestroySource(WFCDevice dev, WFCSource src) WFC_APIEXIT
{
   WFC_LOCK();
   wfc_source_or_mask_destroy(dev, (WFCHandle) src);
   WFC_UNLOCK();
} // wfcDestroySource()

//==============================================================================
// Mask functions

WFC_API_CALL WFCMask WFC_APIENTRY
    wfcCreateMaskFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WFCMask mask;

   WFC_LOCK();

   mask = (WFCMask) wfc_source_or_mask_create
      (WFC_IS_MASK, dev, ctx, stream, attribList);

   WFC_UNLOCK();

   return mask;

} // wfcCreateMaskFromStream()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyMask(WFCDevice dev, WFCMask msk) WFC_APIEXIT
{
   WFC_LOCK();
   wfc_source_or_mask_destroy(dev, (WFCHandle) msk);
   WFC_UNLOCK();
} // wfcDestroyMask()

//==============================================================================
// Element functions

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcCreateElement(WFCDevice dev, WFCContext ctx,
        const WFCint *attribList) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_CONTEXT_T *context = (WFC_CONTEXT_T *)ctx;

   WFCElement element = WFC_INVALID_HANDLE;

   WFC_LOCK();

   if(context->device == device)
   {
      if(wfc_check_no_attribs(attribList))
      {
         WFC_ELEMENT_T *element_ptr =
            khrn_platform_malloc(sizeof(WFC_ELEMENT_T), "WFC_ELEMENT_T");
         const WFC_ELEMENT_ATTRIB_T element_attrib_default = WFC_ELEMENT_ATTRIB_DEFAULT;

         if(element_ptr != NULL)
         {
            memset(element_ptr, 0, sizeof(WFC_ELEMENT_T));

            wfc_link_init_null(&element_ptr->link);
            element_ptr->context = context;
            element_ptr->attributes = element_attrib_default;

            wfc_link_attach(&element_ptr->link, &context->elements_not_in_scene);

            element = (WFCElement) element_ptr;
         } // if
         else
            {wfc_set_error(device, WFC_ERROR_OUT_OF_MEMORY);}
      } // if
      else
         {wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);}
   } // if
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();

   return element;
} // wfcCreateElement()

//------------------------------------------------------------------------------

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetElementAttribi(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFCint result = 0;

   WFC_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_SOURCE:
         result = element->attributes.source_handle;
         break;
      case WFC_ELEMENT_SOURCE_FLIP:
         result = element->attributes.flip;
         break;
      case WFC_ELEMENT_SOURCE_ROTATION:
         result = element->attributes.rotation;
         break;
      case WFC_ELEMENT_SOURCE_SCALE_FILTER:
         result = element->attributes.scale_filter;
         break;
      case WFC_ELEMENT_TRANSPARENCY_TYPES:
         result = element->attributes.transparency_types;
         break;
      case WFC_ELEMENT_GLOBAL_ALPHA:
         result = wfc_round(element->attributes.global_alpha * 255.0f);
         break;
      case WFC_ELEMENT_MASK:
         result = element->attributes.mask_handle;
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();

   return result;
} // wfcGetElementAttribi()

//------------------------------------------------------------------------------

WFC_API_CALL WFCfloat WFC_APIENTRY
    wfcGetElementAttribf(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFCfloat result = 0;

   WFC_LOCK();

   if (element->context->device == device)
   {
      switch (attrib) {
      case WFC_ELEMENT_GLOBAL_ALPHA:
         result = element->attributes.global_alpha;
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();

   return result;
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribiv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint count, WFCint *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device)
   {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               values[i] = element->attributes.dest_rect[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               values[i] = (WFCint) element->attributes.src_rect[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribfv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device)
   {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               values[i] = (WFCfloat) element->attributes.dest_rect[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               values[i] = element->attributes.src_rect[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribi(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint value) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_SOURCE:
      {
         wfc_source_or_mask_release(element->source);
         element->source = (WFC_SOURCE_OR_MASK_T *) value;
         wfc_source_or_mask_acquire(element->source);

         element->attributes.source_handle =
            element->source ? (WFCSource) element->source : WFC_INVALID_HANDLE;

         break;
      }
      case WFC_ELEMENT_SOURCE_FLIP:
      {
         if (value == WFC_FALSE || value == WFC_TRUE)
            element->attributes.flip = value;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_SOURCE_ROTATION:
      {
         if (wfc_is_rotation(value))
            element->attributes.rotation = value;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_SOURCE_SCALE_FILTER:
      {
         if (wfc_is_scale_filter(value))
            element->attributes.scale_filter = value;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_TRANSPARENCY_TYPES:
      {
         if (wfc_are_transparency_types(value))
            element->attributes.transparency_types = value;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_GLOBAL_ALPHA:
      {
         if (value >= 0 && value <= 255)
            element->attributes.global_alpha = (float)value / 255.0f;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
      }
      case WFC_ELEMENT_MASK:
      {
         wfc_source_or_mask_release(element->mask);
         element->mask = (WFC_SOURCE_OR_MASK_T *) value;
         wfc_source_or_mask_acquire(element->mask);

         element->attributes.mask_handle =
            element->mask ? (WFCMask) element->mask : WFC_INVALID_HANDLE;

         break;
      }
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
} // wfcSetElementAttribi()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribf(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCfloat value) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_GLOBAL_ALPHA:
         if (value >= 0.0f && value <= 1.0f)
            element->attributes.global_alpha = value;
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribiv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib,
        WFCint count, const WFCint *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               element->attributes.dest_rect[i] = values[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               element->attributes.src_rect[i] = (float)values[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribfv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFC_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               element->attributes.dest_rect[i] = (int)values[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == WFC_RECT_SIZE)
            for (i = 0; i < WFC_RECT_SIZE; i++)
               element->attributes.src_rect[i] = values[i];
         else
            wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wfc_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcInsertElement(WFCDevice dev, WFCElement elm, WFCElement sub) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *) dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *) elm;
   WFC_ELEMENT_T *subordn = (WFC_ELEMENT_T *) sub;

   WFC_LOCK();

   if ((element->context->device == device)
      && ((subordn == WFC_INVALID_HANDLE) || (subordn->context->device == device)))
   {
      if(subordn != WFC_INVALID_HANDLE)
      {
         // Insert above subordn.
         if ((element->context == subordn->context) && subordn->is_in_scene)
         {
            wfc_link_attach(&element->link, &subordn->link);
            element->is_in_scene = true;
         }
         else
            {wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);}
      } // if
      else
      {
         // Insert at the "bottom of the scene" - which is the top of the list.
         wfc_link_attach(&element->link, &element->context->elements_in_scene);
         element->is_in_scene = true;
      } // else
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcInsertElement()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcRemoveElement(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *) dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *) elm;

   WFC_LOCK();

   if(element->context->device == device)
   {
      wfc_link_attach(&element->link, &element->context->elements_not_in_scene);
      element->is_in_scene = false;
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
} // wfcRemoveElement()

//------------------------------------------------------------------------------

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementAbove(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *)elm;

   WFCElement result = WFC_INVALID_HANDLE;

   WFC_LOCK();

   if ((element != NULL) && (element->context->device == device))
   {
      if (element->is_in_scene)
      {
         if (element->link.next != &element->context->elements_in_scene)
            {result = (WFCElement)element->link.next;}
      }
      else
         {wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);}
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();

   return result;
}

//------------------------------------------------------------------------------

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementBelow(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *) dev;
   WFC_ELEMENT_T *element = (WFC_ELEMENT_T *) elm;

   WFCElement result = WFC_INVALID_HANDLE;

   WFC_LOCK();

   if (element->context->device == device)
   {
      if (element->is_in_scene)
      {
         if (element->link.prev != &element->context->elements_in_scene)
            {result = (WFCElement) element->link.prev;}
      }
      else
         {wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);}
   }
   else
      {wfc_set_error(device, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();

   return result;
} // wfcGetElementBelow()

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyElement(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_ELEMENT_T *element_ptr = (WFC_ELEMENT_T *) elm;

   WFC_LOCK();

   if(element_ptr->context->device == device_ptr)
      {wfc_element_destroy(element_ptr);}
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
}

//==============================================================================
// Rendering

WFC_API_CALL void WFC_APIENTRY
    wfcActivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_CONTEXT_T *context = (WFC_CONTEXT_T *)ctx;

   WFC_LOCK();

   if (context->device == device) {
      RPC_CALL1(wfcActivate_impl,
                WFCACTIVATE_ID,
                RPC_UINT((WFCContext) context));
      RPC_FLUSH();

      context->active = true;
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcDeactivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;
   WFC_CONTEXT_T *context = (WFC_CONTEXT_T *)ctx;

   WFC_LOCK();

   if (context->device == device) {
      RPC_CALL1(wfcDeactivate_impl,
                WFCDEACTIVATE_ID,
                RPC_UINT((WFCContext) context));
      RPC_FLUSH();

      context->active = false;
   } else
      wfc_set_error(device, WFC_ERROR_BAD_HANDLE);

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcCompose(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
   WFC_DEVICE_T *device_ptr = (WFC_DEVICE_T *) dev;
   WFC_CONTEXT_T *context_ptr = (WFC_CONTEXT_T *) ctx;

   WFC_LOCK();

   // TODO use "wait" parameter

   if (context_ptr->device == device_ptr)
   {
      if (!context_ptr->active)
      {
         RPC_CALL1(wfcCompose_impl, WFCCOMPOSE_ID,
            RPC_UINT((WFCContext) context_ptr));
         RPC_FLUSH();
      }
      else
         {wfc_set_error(device_ptr, WFC_ERROR_UNSUPPORTED);}
   }
   else
      {wfc_set_error(device_ptr, WFC_ERROR_BAD_HANDLE);}

   WFC_UNLOCK();
}

//------------------------------------------------------------------------------

WFC_API_CALL void WFC_APIENTRY
    wfcFence(WFCDevice dev, WFCContext ctx, WFCEGLDisplay dpy,
        WFCEGLSync sync) WFC_APIEXIT
{
   WFC_LOCK();

   // TODO wfcFence()
   vcos_assert(0);

   WFC_UNLOCK();
}

//==============================================================================
// Renderer and extension information

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetStrings(WFCDevice dev,
        WFCStringID name,
        const char **strings,
        WFCint stringsCount) WFC_APIEXIT
{
   WFC_DEVICE_T *device = (WFC_DEVICE_T *)dev;

   WFCint result = 0;

   const char *string = NULL;

   WFC_LOCK();

   switch (name) {
   case WFC_VENDOR:
      string = "Broadcom";
      break;
   case WFC_RENDERER:
      string = "VideoCore IV HW";
      break;
   case WFC_VERSION:
      string = "1.0";
      break;
   case WFC_EXTENSIONS:
      string = "";
      break;
   default:
      vcos_assert(0);
   }

   if (stringsCount >= 0 && string) {
      if (strings) {
         if (stringsCount > 0) {
            *strings = string;
            result = 1;
         }
      } else 
         result = 1;
   } else
      wfc_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

   WFC_UNLOCK();

   return result;
} // wfcGetStrings()

//------------------------------------------------------------------------------

WFC_API_CALL WFCboolean WFC_APIENTRY
    wfcIsExtensionSupported(WFCDevice dev, const char *string) WFC_APIEXIT
{
   // TODO wfcIsExtensionSupported()
   vcos_assert(0);

   return WFC_FALSE;
}

//==============================================================================
//------------------------------------------------------------------------------
