#include <stdlib.h>

#include "interface/khronos/include/WF/wfc.h"

#include "interface/khronos/common/khrn_client_platform.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#define MAX_DISPLAYS 2
#define SCREEN_NUMBER_INVALID    -1
#define OUR_DEVICE_ID 1

#define WF_LOCK()
#define WF_UNLOCK()

extern void platform_get_screen_dimensions(int screenNumber, uint32_t *width, uint32_t *height);
extern void platform_get_stream_dimensions(WFCNativeStreamType stream, uint32_t *width, uint32_t *height);

extern uint32_t platform_get_stream_handle(WFCNativeStreamType stream);

typedef struct _WF_LINK {
   struct _WF_LINK *prev;
   struct _WF_LINK *next;
} WF_LINK_T;

static void link_detach(WF_LINK_T *link)
{
   if (link->next) {
      /*
         never unlink a base link
      */

      assert(link->next != link);
      assert(link->prev != link);

      link->next->prev = link->prev;
      link->prev->next = link->next;

      link->prev = NULL;
      link->next = NULL;
   }
}

static void link_attach(WF_LINK_T *link, WF_LINK_T *prev)
{
   link_detach(link);

   link->prev = prev;
   link->next = prev->next;

   link->prev->next = link;
   link->next->prev = link;
}

static void link_init_null(WF_LINK_T *link)
{
   link->prev = NULL;
   link->next = NULL;
}

static void link_init_empty(WF_LINK_T *link)
{
   link->prev = link;
   link->next = link;
}

static bool link_is_empty(WF_LINK_T *link)
{
   return link->next == link;
}

typedef void (*WF_LINK_CALLBACK_T)(WF_LINK_T *link);

static void link_iterate(WF_LINK_T *link, WF_LINK_CALLBACK_T func)
{
   WF_LINK_T *curr = link;
   WF_LINK_T *next = curr->next;

   while (next != link) {
      curr = next;
      next = curr->next;

      func(curr);
   }
}

static int link_get_size(WF_LINK_T *link)
{
   int size = 0;

   WF_LINK_T *curr = link;
   WF_LINK_T *next = curr->next;

   while (next != link) {
      curr = next;
      next = curr->next;

      size++;
   }

   return size;
}

typedef struct {
   WFCErrorCode error;

   WF_LINK_T contexts;
} WF_DEVICE_T;

typedef struct {
   WF_LINK_T link;

   WF_DEVICE_T *device;

   WFCContextType type;
   WFCRotation rotation;

   union {
      int handle;
      int screen;
   };

   int width;
   int height;

   float background[4];

   WF_LINK_T sources;
   WF_LINK_T masks;

   WF_LINK_T elements[2];

   bool active;
} WF_CONTEXT_T;

typedef struct {
   WF_LINK_T link;

   WF_CONTEXT_T *context;

   int handle;
   int refcount;
} WF_SOURCE_T;

typedef struct {
   WF_LINK_T link;

   WF_CONTEXT_T *context;

   int handle;
   int refcount;
} WF_MASK_T;

typedef struct {
   WF_LINK_T link;

   WF_CONTEXT_T *context;
   WF_SOURCE_T *source;
   WF_MASK_T *mask;

   int destination_rectangle[4];
   float source_rectangle[4];

   bool flip;

   WFCRotation rotation;
   WFCScaleFilter scale_filter;

   WFCbitfield transparency_types;

   float global_alpha;

   bool in_scene;
} WF_ELEMENT_T;

static WF_SOURCE_T *wf_source_create(WF_CONTEXT_T *context, int handle)
{
   WF_SOURCE_T *source = platform_malloc(sizeof(WF_SOURCE_T), "WF_SOURCE_T");

   if (source) {
      link_init_null(&source->link);

      source->context = context;
      source->handle = handle;
      source->refcount = 1;
   }

   return source;
}

static void wf_source_acquire(WF_SOURCE_T *source)
{
   if (source) 
      source->refcount++;
}

static void wf_source_release(WF_SOURCE_T *source)
{
   if (source)
      if (--source->refcount == 0)
         platform_free(source);
}

static void wf_source_destroy(WF_SOURCE_T *source)
{
   link_detach(&source->link);

   wf_source_release(source);
}

static WF_MASK_T *wf_mask_create(WF_CONTEXT_T *context, int handle)
{
   WF_MASK_T *mask = platform_malloc(sizeof(WF_MASK_T), "WF_MASK_T");

   if (mask) {
      link_init_null(&mask->link);

      mask->context = context;
      mask->handle = handle;
      mask->refcount = 1;
   }

   return mask;
}

static void wf_mask_acquire(WF_MASK_T *mask)
{
   if (mask) 
      mask->refcount++;
}

static void wf_mask_release(WF_MASK_T *mask)
{
   if (mask)
      if (--mask->refcount == 0)
         platform_free(mask);
}

static void wf_mask_destroy(WF_MASK_T *mask)
{
   link_detach(&mask->link);

   wf_mask_release(mask);
}

static WF_ELEMENT_T *wf_element_create(WF_CONTEXT_T *context)
{
   WF_ELEMENT_T *element = platform_malloc(sizeof(WF_ELEMENT_T), "WF_ELEMENT_T");

   if (element) {
      link_init_null(&element->link);

      element->context = context;

      element->source = NULL;
      element->mask = NULL;

      element->destination_rectangle[0] = 0;
      element->destination_rectangle[1] = 0;
      element->destination_rectangle[2] = 0;
      element->destination_rectangle[3] = 0;

      element->source_rectangle[0] = 0.0f;
      element->source_rectangle[1] = 0.0f;
      element->source_rectangle[2] = 0.0f;
      element->source_rectangle[3] = 0.0f;

      element->flip = false;

      element->rotation = WFC_ROTATION_0;
      element->scale_filter = WFC_SCALE_FILTER_NONE;

      element->transparency_types = 0;

      element->global_alpha = 1.0f;

      element->in_scene = false;
   }

   return element;
}

static void wf_element_destroy(WF_ELEMENT_T *element)
{
   link_detach(&element->link);

   platform_free(element);
}

static WF_CONTEXT_T *wf_context_create(WF_DEVICE_T *device, WFCContextType type, int info, int width, int height, WFCErrorCode *error)
{
   WF_CONTEXT_T *context = platform_malloc(sizeof(WF_CONTEXT_T), "WF_CONTEXT_T");

   if (context) {
      int response = RPC_CALL2_RES(wfcintCreateContext_impl,
                                   WFCINTCREATECONTEXT_ID,
                                   RPC_ENUM(type),
                                   RPC_INT(info));
      
      if (response & 0x80000000) {
         int i;

         link_init_null(&context->link);

         context->device = device;

         context->type = type;
         context->rotation = WFC_ROTATION_0;

         context->handle = response;

         context->width = width;
         context->height = height;

         context->background[0] = 0.0f;
         context->background[1] = 0.0f;
         context->background[2] = 0.0f;
         context->background[3] = 1.0f;

         link_init_empty(&context->sources);
         link_init_empty(&context->masks);

         for (i = 0; i < 2; i++)
            link_init_empty(&context->elements[i]);

         context->active = false;
      } else {
         platform_free(context);

         context = NULL;

         *error = (WFCErrorCode)response;
      }
   } else
      *error = WFC_ERROR_OUT_OF_MEMORY;

   return context;
}

static void wf_context_destroy(WF_CONTEXT_T *context)
{
   int i;

   link_detach(&context->link);

   link_iterate(&context->sources, (WF_LINK_CALLBACK_T)wf_source_destroy);
   link_iterate(&context->masks, (WF_LINK_CALLBACK_T)wf_mask_destroy);

   for (i = 0; i < 2; i++)
      link_iterate(&context->elements[i], (WF_LINK_CALLBACK_T)wf_element_destroy);

   RPC_CALL1(wfcIntDestroyContext_impl,
             WFCINTDESTROYCONTEXT_ID,
             RPC_INT(context->handle));

   platform_free(context);
}

static WF_DEVICE_T *wf_device_create()
{
   WF_DEVICE_T *device = platform_malloc(sizeof(WF_DEVICE_T), "WF_DEVICE_T");

   if (device) {
      device->error = WFC_ERROR_NONE;

      link_init_empty(&device->contexts);
   }

   return device;
}

static void wf_device_destroy(WF_DEVICE_T *device)
{
   /*
      Destroy all of the contexts associated with the device. This will in turn 
      destroy all of the sources, masks and elements associated with each context.
   */

   link_iterate(&device->contexts, (WF_LINK_CALLBACK_T)wf_context_destroy);

   platform_free(device);
}

static void wf_set_error(WF_DEVICE_T *device, WFCErrorCode error)
{
   if (device->error == WFC_ERROR_NONE)
      device->error = error;
}

/*
   Devices are identified using integer IDs assigned by the implementation. The
   number and IDs of the available devices on the system can be retrieved by
   calling wfcEnumerateDevices.

   The user provides a buffer to receive the list of available device IDs. deviceIds
   is the address of the buffer. deviceIdsCount is the number of device IDs that
   can fit into the buffer.

   The filterList parameter contains a list of filtering attributes which are used
   to control the device IDs returned by wfcEnumerateDevices. All attributes in
   filterList are immediately followed by the corresponding value. The list is
   terminated with WFC_NONE. filterList may be NULL or empty (first attribute
   is WFC_NONE), in which case all valid device IDs for the platform are returned.
   Providing a non-NULL filterList that is not terminated with WFC_NONE will
   result in undefined behavior. Providing an invalid filter attribute or filter
   attribute value will result in an empty device ID list being returned. See below
   for the list of available device filtering attributes.

   If deviceIds is NULL, the total number of available devices on the system is
   returned and the buffer is not modified.

   If deviceIds is not NULL, the buffer is populated with a list of available device
   IDs. No more than deviceIdsCount will be written even if more are available.

   The number of device IDs written into deviceIds is returned. If
   deviceIdsCount is negative, no device IDs will be written.

   Device IDs should not be expected to be contiguous. The list of device IDs will
   not include a device ID equal to WFC_DEFAULT_DEVICE_ID. The list of
   available devices is not expected to change.

   The data types and default values for all device filter attributes are listed in the
   table below.

   Attribute                       Type   Default

   WFC_DEVICE_FILTER_SCREEN_NUMBER WFCint N/A

   The WFC_DEVICE_FILTER_SCREEN_NUMBER filter attribute is used to limit the
   returned device ID list to only include devices that can create on-screen Contexts
   using the given screen number. If this attribute appears more than once in the
   filter list, no device IDs will be returned.
*/

static bool wfc_device_check_filters(const WFCint *filterList)
{
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

WFC_API_CALL WFCint WFC_APIENTRY
    wfcEnumerateDevices(WFCint *deviceIds, WFCint deviceIdsCount,
        const WFCint *filterList) WFC_APIEXIT
{
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

/*
   A device can be created and a handle to it returned by calling wfcCreateDevice.

   The value of deviceId should be either a device ID retrieved using
   wfcEnumerateDevices or WFC_DEFAULT_DEVICE_ID. If deviceId is
   WFC_DEFAULT_DEVICE_ID, a default device is returned. The system integrator
   will determine the default device.

   No valid attributes are defined for attribList in this specification.

   If no device matching deviceId exists or if there not enough memory to create
   the device, WFC_INVALID_HANDLE is returned.

   Creation of multiple devices using the same device ID is supported. Each of
   these devices will have a unique handle. State is not shared between these
   devices.
*/

static bool check_no_attribs(const WFCint *attribList)
{
   return !attribList || *attribList == WFC_NONE;
}

WFC_API_CALL WFCDevice WFC_APIENTRY
    wfcCreateDevice(WFCint deviceId, const WFCint *attribList) WFC_APIEXIT
{
   WFCDevice result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if ((deviceId == WFC_DEFAULT_DEVICE_ID || deviceId == OUR_DEVICE_ID) && check_no_attribs(attribList)) {
      WF_DEVICE_T *device = wf_device_create();

      if (device)
         result = (WFCDevice)device;
   }

   WF_UNLOCK();

   return 1;
}

/*
   Error codes are retrieved by calling wfcGetError.

   wfcGetError returns the oldest error code provided by an API call on dev since
   the previous call to wfcGetError on that device (or since the creation of the
   device). No error is indicated by a return value of 0 (WFC_ERROR_NONE). After
   the call, the error code is cleared to 0. If dev is not a valid Device,
   WFC_ERROR_BAD_DEVICE is returned. 
*/

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcGetError(WFCDevice dev) WFC_APIEXIT
{
   WFCErrorCode result;

   WF_LOCK();

   if (dev != WFC_INVALID_HANDLE) {
      WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

      result = device->error;
      device->error = WFC_ERROR_NONE;
   } else
      result = WFC_ERROR_BAD_DEVICE;

   WF_UNLOCK();

   return result;
}

/*
   To query an attribute associated with a device call wfcGetDeviceAttribi.

   On success, the value of attrib for dev is returned. Refer to the following
   list of valid attributes.

   Attribute         Type           R/W  Default
   WFC_DEVICE_CLASS  WFCDeviceClass R    N/A
   WFC_DEVICE_ID     WFCint         R    N/A

   ERRORS

   WFC_ERROR_BAD_ATTRIBUTE
   - if attrib is not a valid device attribute
*/

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetDeviceAttribi(WFCDevice dev, WFCDeviceAttrib attrib) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

   WFCint result = 0;

   WF_LOCK();

   switch (attrib) {
   case WFC_DEVICE_CLASS:
      result = WFC_DEVICE_CLASS_FULLY_CAPABLE;
      break;
   case WFC_DEVICE_ID:
      result = OUR_DEVICE_ID;
      break;
   default:
      wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
      break;
   }

   WF_UNLOCK();

   return result;
}

/*
   A device is destroyed by calling wfcDestroyDevice.

   All resources owned by dev are deleted before this call completes. dev is no
   longer a valid device handle. All resource handles associated with dev become
   invalid. Any references held by dev on external resources are removed.
   All pending composition operations associated with dev are completed before
   this call completes.

   If dev is not a valid Device, WFC_ERROR_BAD_DEVICE is returned. Otherwise,
   WFC_ERROR_NONE is returned.
*/

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcDestroyDevice(WFCDevice dev) WFC_APIEXIT
{
   WFCErrorCode result;

   WF_LOCK();

   if (dev != WFC_INVALID_HANDLE) {
      WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

      wf_device_destroy(device);

      result = WFC_ERROR_NONE;
   } else
      result = WFC_ERROR_BAD_DEVICE;

   WF_UNLOCK();

   return result;
}

/*
   To create a context to control a screen, call wfcCreateOnScreenContext.

   If wfcCreateOnScreenContext succeeds, a new Context for screenNumber is
   initialized and a handle to it is returned.

   screenNumber is an implementation-specific identifier for the particular screen
   this Context will be used to control. At most one Context per-screen may be
   created. If screenNumber is WFC_DEFAULT_SCREEN_NUMBER, the system’s
   default screen will be selected. The system integrator will determine the
   meaning of the default screen.

   No valid attributes are defined in this specification.

   Creation of an on-screen Context does not cause any change to the state of the
   physical screen. The first change to the physical screen is caused by the first
   successful call to wfcActivate or wfcCompose on the associated Context.

   If an error occurs, WFC_INVALID_HANDLE is returned.

   ERRORS

   WFC_ERROR_UNSUPPORTED
   - if dev does not support the creation of an on-screen Context for the specified
   screen
   WFC_ERROR_OUT_OF_MEMORY
   - if the implementation can not allocate resources for the Context
   WFC_ERROR_IN_USE
   - if a Context has already been created for screenNumber
   WFC_ERROR_BAD_ATTRIBUTE
   - if attribList contains an invalid attribute
*/

WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOnScreenContext(WFCDevice dev,
        WFCint screenNumber,
        const WFCint *attribList) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

   WFCContext result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (screenNumber < 0 || screenNumber >= MAX_DISPLAYS) {
      wf_set_error(device, WFC_ERROR_UNSUPPORTED);
   } else if (!check_no_attribs(attribList)) {
      wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
   } else {
      WF_CONTEXT_T *context;

      WFCErrorCode error; 

      uint32_t width;
      uint32_t height;

      platform_get_screen_dimensions(screenNumber, &width, &height);

      context = wf_context_create(device, WFC_CONTEXT_TYPE_ON_SCREEN, screenNumber, width, height, &error);

      if (context) {
         link_attach(&context->link, &device->contexts);

         result = (WFCContext)context;
      } else
         wf_set_error(device, error);
   }

   WF_UNLOCK();

   return result;
}

/*
   To create a Context for compositing into a user-specified image stream, call
   wfcCreateOffScreenContext.

   If wfcCreateOffScreenContext succeeds, a new Context is initialized and a
   handle to it is returned.

   stream must be a valid WFCNativeStreamType that identifies the composition
   target that the returned Context will compose to. The returned Context places a
   reference on stream. stream must be compatible as a target with dev.

   Multiple Contexts may be bound to the same stream, but only one Context is
   allowed to render to a stream at any point in time.

   No valid attributes are defined in this specification.

   If an error occurs, WFC_INVALID_HANDLE is returned.

   ERRORS

   WFC_ERROR_UNSUPPORTED
   - if stream is not compatible with dev
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if stream is not a valid WFCNativeStreamType
   WFC_ERROR_OUT_OF_MEMORY
   - if the implementation can not allocate resources for the Context
   WFC_ERROR_BAD_ATTRIBUTE
   - if attribList contains an invalid attribute
*/

WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOffScreenContext(WFCDevice dev,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

   WFCContext result = WFC_INVALID_HANDLE;

   uint32_t handle = platform_get_stream_handle(stream);

   WF_LOCK();

   if (!handle) {
      wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
   } else if (!check_no_attribs(attribList)) {
      wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
   } else {
      WF_CONTEXT_T *context;

      WFCErrorCode error; 

      uint32_t width;
      uint32_t height;

      platform_get_stream_dimensions(stream, &width, &height);

      context = wf_context_create(device, WFC_CONTEXT_TYPE_OFF_SCREEN, handle, width, height, &error);

      if (context) {
         link_attach(&context->link, &device->contexts);

         result = (WFCContext)context;
      } else
         wf_set_error(device, error);
   }

   WF_UNLOCK();

   return result;
}

/*
   A Context’s scene can only be updated by committing modifications made to its
   attributes and Elements. Modifications are cached, left uncommitted, until the
   user explicitly applies them by calling wfcCommit. Query functions reflect the
   cached values rather than the committed values.

   If wfcCommit succeeds, the current state of ctx’s attributes and corresponding
   Elements will be synchronously captured and then used as the Context’s
   committed scene.

   The implementation will test for configuration conflicts before any modifications
   are committed. If any such conflict exists, the WFC_ERROR_INCONSISTENCY
   error is generated and the scene is left in the pre-call state.

   The previously committed scene and all image providers associated with it may
   remain in use for a short period of time after wfcCommit returns, i.e. until the
   implementation finishes any pending composition of the previously committed
   scene.

   wfcCommit completes asynchronously. Synchronization functions 
   must be used if the user needs to know if or when a particular call to
   wfcCommit has completed. Completion means that ctx is ready to accept a
   new call to wfcCommit.

   If wait is WFC_FALSE and ctx is not ready to accept a new call to wfcCommit,
   this call will fail immediately and the committed scene will not be updated. The
   previous commit will not be affected. If wait is WFC_TRUE, wfcCommit will not
   fail due to ctx not being ready to accept a new commit. This may entail a
   delay in the call returning.

   ERRORS

   WFC_ERROR_INCONSISTENCY
   - if an Element associated with ctx has a source rectangle that is not fully
   contained inside the Element’s Source
   - if an Element associated with ctx has a destination rectangle whose size does
   not match the size of its Mask
   WFC_ERROR_BUSY
   - if wait is WFC_FALSE and ctx is not ready to accept a new call to wfcCommit
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

static void commit_callback(WF_ELEMENT_T *element)
{
   RPC_CALL16(wfcIntCommitAdd_impl,
              WFCINTCOMMITADD_ID,
              RPC_INT(element->context->handle),
              RPC_INT(element->source ? element->source->handle : -1),
              RPC_INT(element->mask ? element->mask->handle : -1),
              RPC_INT(element->destination_rectangle[0]),
              RPC_INT(element->destination_rectangle[1]),
              RPC_INT(element->destination_rectangle[2]),
              RPC_INT(element->destination_rectangle[3]),
              RPC_FLOAT(element->source_rectangle[0]),
              RPC_FLOAT(element->source_rectangle[1]),
              RPC_FLOAT(element->source_rectangle[2]),
              RPC_FLOAT(element->source_rectangle[3]),
              RPC_BOOLEAN(element->flip),
              RPC_ENUM(element->rotation),
              RPC_ENUM(element->scale_filter),
              RPC_BITFIELD(element->transparency_types),
              RPC_FLOAT(element->global_alpha));
}

WFC_API_CALL void WFC_APIENTRY
    wfcCommit(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      int size = link_get_size(&context->elements[1]);

      RPC_CALL1(wfcIntCommitBegin_impl,
                WFCINTCOMMITBEGIN_ID,
                RPC_INT(context->handle));

      link_iterate(&context->elements[1], (WF_LINK_CALLBACK_T)commit_callback);

      RPC_CALL1(wfcIntCommitBegin_impl,
                WFCINTCOMMITEND_ID,
                RPC_INT(context->handle));
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   Context attributes can be queried by calling:

   WFCint wfcGetContextAttribi(WFCDevice dev,
      WFCContext ctx,
      WFCContextAttrib attrib);

   void wfcGetContextAttribfv( WFCDevice dev,
      WFCContext context,
      WFCContextAttrib attrib,
      WFCint count,
      WFCfloat *values);

   The dev and ctx parameters denote the specific Device and Context being
   queried. The attrib parameter denotes the attribute to retrieve and return via
   the function return value or the values parameter. For the vector-based
   function, count denotes the number of values to retrieve and values must be
   an array of count elements.

   On success, the value(s) of attrib for ctx is returned via the function return
   value or the values parameter. On failure, values in not modified. Refer to
   the following list of valid attributes.

   Attribute                  Type           R/W   Default
   WFC_CONTEXT_TYPE           WFCContextType R     N/A
   WFC_CONTEXT_TARGET_HEIGHT  WFCint         R     N/A
   WFC_CONTEXT_TARGET_WIDTH   WFCint         R     N/A
   WFC_CONTEXT_LOWEST_ELEMENT WFCElement     R     N/A
   WFC_CONTEXT_ROTATION       WFCRotation    R/W   WFC_ROTATION_0
   WFC_CONTEXT_BG_COLOR       WFCfloat       R/W   (0, 0, 0, 1)

   Retrieving handle-typed attributes does not affect the lifetime of the handle.

   ERRORS

   WFC_ERROR_BAD_ATTRIBUTE
   - if attrib is an invalid attribute
   - if attrib does not permit the use of this accessor
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if count does not match the size of attrib
   - if values is NULL or not aligned with its datatype
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WFCint result = 0;

   WF_LOCK();

   if (context->device == device) {
      switch (attrib) {
      case WFC_CONTEXT_TYPE:
         result = context->type;
         break;
      case WFC_CONTEXT_TARGET_WIDTH:
         result = context->width;
         break;
      case WFC_CONTEXT_TARGET_HEIGHT:
         result = context->height;
         break;
      case WFC_CONTEXT_LOWEST_ELEMENT:
         if (context->link.next == &context->link) 
            result = WFC_INVALID_HANDLE;
         else {
            WF_ELEMENT_T *element = (WF_ELEMENT_T *)context->link.next;

            result = (WFCint)element;
         }
         break;
      case WFC_CONTEXT_ROTATION:
         result = context->rotation;
         break;
      case WFC_CONTEXT_BG_COLOR:
         result = (int)(context->background[0] * 255.0f) << 24 | 
                  (int)(context->background[1] * 255.0f) << 16 | 
                  (int)(context->background[2] * 255.0f) << 8 | 
                  (int)(context->background[3] * 255.0f);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      int i;

      switch (attrib) {
      case WFC_CONTEXT_BG_COLOR:
         if (values && count == 4)
            for (i = 0; i < 4; i++) 
               values[i] = context->background[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   Context attributes can be set using:

   void wfcSetContextAttribi( WFCDevice dev,
      WFCContext ctx,
      WFCContextAttrib attrib,
      WFCint value);

   void wfcSetContextAttribfv(WFCDevice dev,
      WFCContext ctx,
      WFCContextAttrib attrib,
      WFCint count,
      const WFCfloat *values);

   The dev and ctx parameters denote the specific Device and Context,
   respectively, associated with this function call. The attrib parameter denotes
   the attribute being set. For the vector-based function, count denotes the number
   of values provided and values must be an array of count elements.

   On success, the value(s) of attrib for ctx are set to the specified value(s). On
   failure, the Context is not modified.

   Note that wfcCommit must be called on ctx before updated values will affect
   rendering.

   ERRORS

   WFC_ERROR_BAD_ATTRIBUTE
   - if attrib is an invalid attribute
   - if attrib is an immutable attribute
   - if attrib does not permit the use of this accessor
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if count does not match the size of attrib
   - if values is NULL
   - if values is non-NULL and not aligned with its datatype
   - if the user attempts to set a valid attribute to an invalid value
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

static bool is_rotation(WFCint value)
{
   return value == WFC_ROTATION_0   ||
          value == WFC_ROTATION_90  ||
          value == WFC_ROTATION_180 ||
          value == WFC_ROTATION_270;
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint value) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      int i;

      switch (attrib) {
      case WFC_CONTEXT_ROTATION:
         if (is_rotation(value))
            context->rotation = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_CONTEXT_BG_COLOR:
         for (i = 0; i < 4; i++)
            context->background[i] = (float)(value >> (24-8*i) & 0xff) / 255.0f;
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      int i;

      switch (attrib) {
      case WFC_CONTEXT_BG_COLOR:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++)
               context->background[i] = values[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   A Context is destroyed by calling wfcDestroyContext.

   Following this call, all resources owned by ctx are marked for deletion as soon
   as possible. ctx is no longer a valid Context handle. All resource handles
   associated with ctx become invalid. Any references held by ctx on external
   resources are removed when deletion occurs.

   All pending composition operations associated with ctx are completed before
   this call completes.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyContext(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device)
      wf_context_destroy(context);
   else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   To create a Source, call wfcCreateSourceFromStream.

   If wfcCreateSourceFromStream succeeds, a new WFCSource is initialized and a
   handle to it is returned. The returned WFCSource places a reference on stream.
   The Source is guaranteed to be usable for input to composition with ctx. If
   multiple Sources are created from the same stream, each Source will have a
   unique handle.

   stream must be a valid stream. stream must not be in use as the target of ctx.
   stream must be compatible as a Source with dev.

   No valid attributes are defined for attribList in this specification.

   On failure, wfcCreateSourceFromStream returns WFC_INVALID_HANDLE.

   ERRORS

   WFC_ERROR_UNSUPPORTED
   - if stream is valid but not suitable for composition with dev
   WFC_ERROR_OUT_OF_MEMORY
   - if the implementation fails to allocate resources for the Source
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if stream is not a valid WFCNativeStreamType
   WFC_ERROR_BAD_ATTRIBUTE
   - if attribList contains an invalid attribute
   WFC_ERROR_BUSY
   - if stream can not be used with ctx at this time due to other Context’s use of
   stream
   WFC_ERROR_IN_USE
   - if stream is in use as the target of ctx
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL WFCSource WFC_APIENTRY
    wfcCreateSourceFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WFCSource result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (context->device == device) {
      uint32_t handle = platform_get_stream_handle(stream);

      if (!handle) {
         wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
      } else if (context->type == WFC_CONTEXT_TYPE_OFF_SCREEN && context->handle == handle) {
         wf_set_error(device, WFC_ERROR_IN_USE);
      } else if (!check_no_attribs(attribList)) {
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
      } else {
         WF_SOURCE_T *source = wf_source_create(context, handle);

         if (source) {
            link_attach(&source->link, &context->sources);

            result = (WFCSource)source;
         } else
            wf_set_error(device, WFC_ERROR_OUT_OF_MEMORY);
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

/*
   To destroy a composition source, call wfcDestroySource.

   Following this call, all resources associated with src which were allocated by the
   implementation are marked for deletion as soon as possible. The reference
   placed on the Source’s stream at creation time is removed when deletion occurs.
   src is no longer a valid Source handle. If src is attached to an Element at the
   time wfcDestroySource is called, the Source continues to exist until it is detached
   or the Element is destroyed.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if src is not a valid Source associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcDestroySource(WFCDevice dev, WFCSource src) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_SOURCE_T *source = (WF_SOURCE_T *)src;

   WF_LOCK();

   if (source->context->device == device) 
      wf_source_destroy(source);
   else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   To create a Mask, call wfcCreateMaskFromStream.

   If wfcCreateMaskFromStream succeeds, a new WFCMask is initialized and a
   handle to it is returned. The returned WFCMask places a reference on stream.
   The source is guaranteed to be usable for input to composition with ctx. If
   multiple Masks are created from the same stream, each Mask will have a unique
   handle.

   stream must be a valid stream. stream must not be in use as the target of
   ctx. stream must be compatible as a Mask with dev.

   No valid attributes are defined in this specification.

   On failure, wfcCreateMaskFromStream returns WFC_INVALID_HANDLE.

   ERRORS

   WFC_ERROR_UNSUPPORTED
   - if stream is valid but not suitable for composition with ctx’s Device
   WFC_ERROR_OUT_OF_MEMORY
   - if the implementation fails to allocate resources for the Mask
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if stream is not a valid WFCNativeStreamType
   WFC_ERROR_BAD_ATTRIBUTE
   - if attribList contains an invalid attribute
   WFC_ERROR_BUSY
   - if stream can not be used with ctx at this time due to other Context’s use of
   stream
   WFC_ERROR_IN_USE
   - if stream is in use as the target of ctx
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL WFCMask WFC_APIENTRY
    wfcCreateMaskFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WFCMask result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (context->device == device) {
      uint32_t handle = platform_get_stream_handle(stream);

      if (!handle) {
         wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
      } else if (context->type == WFC_CONTEXT_TYPE_OFF_SCREEN && context->handle == handle) {
         wf_set_error(device, WFC_ERROR_IN_USE);
      } else if (!check_no_attribs(attribList)) {
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
      } else {
         WF_MASK_T *mask = wf_mask_create(context, handle);

         if (mask) {
            link_attach(&mask->link, &context->masks);

            result = (WFCMask)mask;
         } else
            wf_set_error(device, WFC_ERROR_OUT_OF_MEMORY);
      }
   } else 
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

/*
   To destroy a Mask, call wfcDestroyMask.

   Following this call, all resources associated with mask which were allocated by
   the implementation are marked for deletion as soon as possible. The reference
   placed on the Mask’s stream at creation time is removed when deletion occurs.
   mask is no longer a valid Mask handle. If mask is attached to an Element at the
   time wfcDestroyMask is called, the Mask continues to exist until it is detached
   or the Element is destroyed.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if mask is not a valid Mask associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyMask(WFCDevice dev, WFCMask msk) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_MASK_T *mask = (WF_MASK_T *)msk;

   WF_LOCK();

   if (mask) 
      wf_mask_destroy(mask);
   else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   An Element is created for use with a specific context by calling
   wfcCreateElement.

   If wfcCreateElement succeeds, a new WFCElement is initialized with default
   attributes and a handle to it is returned. The element is specific to ctx and can
   not be used with another Context.

   No valid attributes are defined in this specification.

   On failure, wfcCreateElement returns WFC_INVALID_HANDLE.

   ERRORS

   WFC_ERROR_OUT_OF_MEMORY
   - if the implementation fails to allocate resources for the Element
   WFC_ERROR_BAD_ATTRIBUTE
   - if attribList contains an invalid attribute
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcCreateElement(WFCDevice dev, WFCContext ctx,
        const WFCint *attribList) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WFCElement result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (context->device == device) {
      if (!check_no_attribs(attribList)) {
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
      } else {
         WF_ELEMENT_T *element = wf_element_create(context);

         if (element) {
            link_attach(&element->link, &context->elements[0]);

            result = (WFCElement)element;
         } else
            wf_set_error(device, WFC_ERROR_OUT_OF_MEMORY);
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

/*
   Element attributes can be queried by calling:

   WFCint wfcGetElementAttribi( WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib);

   WFCfloat wfcGetElementAttribf(WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib);

   void wfcGetElementAttribiv( WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib,
      WFCint count,
      WFCint *values);

   void wfcGetElementAttribfv( WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib,
      WFCint count,
      WFCfloat *values);

   The dev and element parameters denote the specific Device and Element being
   queried. The attrib parameter denotes the attribute to retrieve and return via
   the function return value or the value parameter. For the vector-based functions,
   count denotes the number of values to retrieve and values must be an array of
   count elements.

   On success, the value(s) of attrib for element is returned via the function
   return value or the values parameter. On failure, values in not modified.
   Refer to the following list of valid attributes.

   Attribute                           Type           R/W   Default
   WFC_ELEMENT_DESTINATION_RECTANGLE   WFCint[4]      R/W   (0, 0, 0, 0)
   WFC_ELEMENT_SOURCE                  WFCSource      R/W   WFC_INVALID_HANDLE
   WFC_ELEMENT_SOURCE_RECTANGLE        WFCfloat[4]    R/W   (0, 0, 0, 0)
   WFC_ELEMENT_SOURCE_FLIP             WFCboolean     R/W   WFC_FALSE
   WFC_ELEMENT_SOURCE_ROTATION         WFCRotation    R/W   WFC_ROTATION_0
   WFC_ELEMENT_SOURCE_SCALE_FILTER     WFCScaleFilter R/W   WFC_SCALE_FILTER_NONE
   WFC_ELEMENT_TRANSPARENCY_TYPES      WFCbitfield    R/W   0
   WFC_ELEMENT_GLOBAL_ALPHA            WFCfloat       R/W   1
   WFC_ELEMENT_MASK                    WFCMask        R/W   WFC_INVALID_HANDLE

   Retrieving handle-typed attributes does not affect the lifetime of the handle.

   ERRORS

   WFC_ERROR_BAD_ATTRIBUTE
   - if attrib is an invalid attribute
   - if attrib does not permit the use of this accessor
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if count does not match the size of attrib
   - if values is NULL or not aligned with its datatype
   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
*/

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetElementAttribi(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WFCint result = 0;

   WF_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_SOURCE:
         result = (WFCint)element->source;
         break;
      case WFC_ELEMENT_SOURCE_FLIP:
         result = element->flip;
         break;
      case WFC_ELEMENT_SOURCE_ROTATION:
         result = element->rotation;
         break;
      case WFC_ELEMENT_SOURCE_SCALE_FILTER:
         result = element->scale_filter;
         break;
      case WFC_ELEMENT_TRANSPARENCY_TYPES:
         result = element->transparency_types;
         break;
      case WFC_ELEMENT_GLOBAL_ALPHA:
         result = round(element->global_alpha * 255.0f);
         break;
      case WFC_ELEMENT_MASK:
         result = (WFCint)element->mask;
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

WFC_API_CALL WFCfloat WFC_APIENTRY
    wfcGetElementAttribf(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WFCfloat result = 0;

   WF_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_GLOBAL_ALPHA:
         result = element->global_alpha;
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribiv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint count, WFCint *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && count == 4)
            for (i = 0; i < 4; i++) 
               values[i] = element->destination_rectangle[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               values[i] = (int)element->source_rectangle[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribfv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               values[i] = (float)element->destination_rectangle[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               values[i] = element->source_rectangle[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   Element attributes can be set using:

   void wfcSetElementAttribi( WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib
      WFCint value);

   void wfcSetElementAttribf( WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib
      WFCfloat value);

   void wfcSetElementAttribiv(WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib,
      WFCint count,
      const WFCint *values);

   void wfcSetElementAttribfv(WFCDevice dev,
      WFCElement element,
      WFCElementAttrib attrib,
      WFCint count,
      const WFCfloat *values);

   The dev and element parameters denote the specific Device and Element,
   respectively, associated with this function call. The attrib parameter denotes
   the attribute being set. For the vector-based functions, count denotes the
   number of values provided and values must be an array of count elements.
   On success, the value(s) of attrib for element are set to value. On failure, the
   Element is not modified. 

   Note that wfcCommit must be called on the associated Context before updated
   values will affect rendering.

   ERRORS
   
   WFC_ERROR_BAD_ATTRIBUTE
   - if attrib is an invalid attribute
   - if attrib is an immutable attribute
   - if attrib does not permit the use of this accessor
   WFC_ERROR_ILLEGAL_ARGUMENT
   - if count does not match the size of attrib
   - if values is NULL
   - if values is non-NULL and not aligned with its datatype
   - if the user attempts to set a valid attribute to an invalid value
   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
*/

static bool is_scale_filter(WFCint value)
{
   return value == WFC_SCALE_FILTER_NONE   ||
          value == WFC_SCALE_FILTER_FASTER ||
          value == WFC_SCALE_FILTER_BETTER;
}

static bool are_transparency_types(WFCint value)
{
   return value == WFC_TRANSPARENCY_NONE ||
          value == WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA ||
          value == WFC_TRANSPARENCY_SOURCE ||
          value == WFC_TRANSPARENCY_MASK ||
          value == (WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA | WFC_TRANSPARENCY_SOURCE) ||
          value == (WFC_TRANSPARENCY_ELEMENT_GLOBAL_ALPHA | WFC_TRANSPARENCY_MASK);
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribi(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCint value) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_SOURCE:
      {
         wf_source_release(element->source);
         element->source = (WF_SOURCE_T *)value;
         wf_source_acquire(element->source);

         break;
      }
      case WFC_ELEMENT_SOURCE_FLIP:
      {
         if (value == WFC_FALSE || value == WFC_TRUE)
            element->flip = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_SOURCE_ROTATION:
      {
         if (is_rotation(value))
            element->rotation = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_SOURCE_SCALE_FILTER:
      {
         if (is_scale_filter(value))
            element->scale_filter = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_TRANSPARENCY_TYPES:
      {
         if (are_transparency_types(value))
            element->transparency_types = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      }
      case WFC_ELEMENT_GLOBAL_ALPHA:
      {
         if (value >= 0 && value <= 255)
            element->global_alpha = (float)value / 255.0f;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
      }
      case WFC_ELEMENT_MASK:
      {
         wf_mask_release(element->mask);
         element->mask = (WF_MASK_T *)value;
         wf_mask_acquire(element->mask);

         break;
      }
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribf(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib, WFCfloat value) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      switch (attrib) {
      case WFC_ELEMENT_GLOBAL_ALPHA:
         if (value >= 0.0f && value <= 1.0f)
            element->global_alpha = value;
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribiv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib,
        WFCint count, const WFCint *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               element->destination_rectangle[i] = values[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               element->source_rectangle[i] = (float)values[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribfv(WFCDevice dev, WFCElement elm,
        WFCElementAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      int i;

      switch (attrib) {
      case WFC_ELEMENT_DESTINATION_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               element->destination_rectangle[i] = (int)values[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      case WFC_ELEMENT_SOURCE_RECTANGLE:
         if (values && !((size_t)values & 3) && count == 4)
            for (i = 0; i < 4; i++) 
               element->source_rectangle[i] = values[i];
         else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
         break;
      default:
         wf_set_error(device, WFC_ERROR_BAD_ATTRIBUTE);
         break;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   To place an Element into its Context’s scene, call wfcInsertElement.

   element is placed inside its Context’s scene immediately above subordinate.
   Any Element that was immediately above subordinate is placed immediately
   above element. If element is already inside the scene, it is effectively
   removed and re-inserted in the new location. Inserting an Element above itself is
   permitted but has no effect.

   If subordinate is WFC_INVALID_HANDLE, the element is placed at the bottom
   of the scene. Otherwise, element and subordinate must have been created
   using the same context and subordinate must be in the scene.

   ERRORS

   WFC_ERROR_ILLEGAL_ARGUMENT
   - if subordinate and element were not created using the same context
   - if subordinate is outside the scene
   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
   - if subordinate is not WFC_INVALID_HANDLE and is not a valid Element
   associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcInsertElement(WFCDevice dev, WFCElement elm,
        WFCElement sub) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;
   WF_ELEMENT_T *subordn = (WF_ELEMENT_T *)sub;

   WF_LOCK();

   if (element->context->device == device && (!subordn || subordn->context->device == device)) {
      if (subordn) {
         if (element->context == subordn->context && subordn->in_scene) {
            link_attach(&element->link, &subordn->link);
            element->in_scene = true;
         } else
            wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
      } else {
         link_attach(&element->link, &element->context->elements[1]);
         element->in_scene = true;
      }
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   To remove an Element from its Context’s scene, call wfcRemoveElement.

   element is placed outside the scene and will not affect composition. Removing
   an Element that is already outside the scene has no effect.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcRemoveElement(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device) {
      link_attach(&element->link, &element->context->elements[0]);
      element->in_scene = false;
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   To determine the Element above a specified Element call wfcGetElementAbove.

   element must be inside its Context’s scene. If element is top-most or outside
   its Context’s scene, WFC_INVALID_HANDLE is returned. Otherwise the Element
   above element is returned. This call does not affect the lifetime or validity of
   the Element returned.

   ERRORS

   WFC_ERROR_ILLEGAL_ARGUMENT
   - if element is outside the scene
   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
*/

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementAbove(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WFCElement result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (element->context->device == device) {
      if (element->in_scene) {
         if (element->link.next != &element->context->elements[1])
            result = (WFCElement)element->link.next;
      } else
         wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

/*
   To determine the Element below a specified Element call wfcGetElementBelow.

   element must be inside its Context’s scene. If element is bottom-most or
   outside its Context’s scene, WFC_INVALID_HANDLE is returned. Otherwise the
   Element below element is returned. This call does not affect the lifetime or
   validity of the Element returned.

   ERRORS

   WFC_ERROR_ILLEGAL_ARGUMENT
   - if element is outside the scene
   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev   
*/

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementBelow(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WFCElement result = WFC_INVALID_HANDLE;

   WF_LOCK();

   if (element->context->device == device) {
      if (element->in_scene) {
         if (element->link.prev != &element->context->elements[1])
            result = (WFCElement)element->link.prev;
      } else
         wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();

   return result;
}

/*
   To destroy an Element, call wfcDestroyElement.

   All resources associated with element are marked for deletion as soon as
   possible. Any references held by element on Sources or Masks are removed
   when this Element is deleted. Following the call, element is no longer a valid
   Element handle. If element is inside its Context’s scene at the time
   wfcDestroyElement is called, it is removed from the scene. This removal does
   not affect the Context’s committed scene until wfcCommit is called.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if element is not a valid Element associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyElement(WFCDevice dev, WFCElement elm) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_ELEMENT_T *element = (WF_ELEMENT_T *)elm;

   WF_LOCK();

   if (element->context->device == device)
      wf_element_destroy(element);
   else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/* 
   A Context is activated by calling wfcActivate.

   If successful, ctx will begin performing autonomous composition using ctx's
   committed scene, as defined in the previous call to wfcCommit on ctx. The
   target of ctx and all image providers associated with the ctx‘s committed scene
   will be considered in use by ctx. If ctx already has autonomous composition
   enabled, this function has no effect.

   If the target of ctx is already in use as a render target by another renderer, the
   target’s contents become undefined and ctx must be deactivated before the user
   can resume defined rendering to the target.

   Note that completion of this function means only that ctx is permitted to begin
   rendering and does not imply that any rendering has started or finished.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcActivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      RPC_CALL1(wfcActivate_impl,
                WFCACTIVATE_ID,
                RPC_INT(context->handle));

      context->active = true;
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   A context is deactivated by calling wfcDeactivate.

   ctx will asynchronously stop performing autonomous composition. Any
   composition in progress will be allowed to complete normally. If ctx already
   has autonomous composition disabled, this function has no effect.
   wfcDeactivate may return before ctx has finished rendering. Completion of
   this request, as can be determined by synchronization functions (see Section 9),
   implies that any previous calls to wfcCommit on ctx have completed and that
   ctx is no longer accessing any target images or image providers.

   If the target of ctx is capable of being targeted by multiple renderers,
   completion also implies that the target is now available for those other renderers
   to use as a render target.

   ERRORS

   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcDeactivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      RPC_CALL1(wfcDeactivate_impl,
                WFCDEACTIVATE_ID,
                RPC_INT(context->handle));

      context->active = false;
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

/*
   An inactive Context can be requested to compose by calling wfcCompose.

   If wfcCompose succeeds, ctx's committed scene, as defined in the previous call
   to wfcCommit on ctx, will be asynchronously rendered to the Context's
   destination. The target of ctx and all image providers associated with the ctx‘s
   committed scene will be considered in use by ctx during this time. This
   rendering will not be affected by any subsequent changes to ctx's committed
   scene.

   Composition is equivalent to clearing the destination to the background color,
   erasing any previous pixel values, followed by iterating over the Elements inside
   the scene, rendering each element on top of the previous Elements.
   ctx must be inactive. If the target of ctx is already in use as a render target by
   another renderer, the target’s contents become undefined.

   If wait is WFC_FALSE and ctx is still processing a previous call to
   wfcCompose, this call will fail immediately without causing any rendering. The
   previous request will not be affected. If wait is WFC_TRUE, wfcCompose will
   not fail due to a previous request being incomplete. The two types of behaviour 
   aid user flexibility. Users may choose “immediate rejection” if they are 
   controlling multiple contexts from a single thread and do not wish to be stalled 
   for a long period. Users responsible for a single context may benefit from “stall 
   until ready” rather than being forced to poll. This may entail a delay in the 
   call returning, due to the implementation waiting for a previous request to
   complete.

   As a consequence of rendering being asynchronous, all image providers and any
   target image associated with the composition request may continue to be
   accessed by the implementation after wfcCompose returns. If ctx’s target is
   multi-buffered, previously rendered images may be accessed for the purpose of
   optimizing rendering.

   There is a finite period of time between the call to wfcCompose returning and
   the completion of the request. Synchronization functions, described in Section 9,
   must be used if the user needs to know if or when a particular call to
   wfcCompose has completed. Such functions enable the user to monitor the time
   taken to process requests.

   Completion of this request implies that ctx is ready to accept a new request and
   that all rendering associated with the request has finished; any target image and
   all image providers associated with the request are no longer in use by
   Composition. If the target of ctx is also capable of being targeted by multiple
   renderers, completion also implies that the target is now available for those other
   renderers to use as a render target.

   ERRORS

   WFC_ERROR_UNSUPPORTED
   - if ctx is active
   WFC_ERROR_BUSY
   - if wait is WFC_FALSE and ctx can not immediately accept any more
   composition requests
   WFC_ERROR_BAD_HANDLE
   - if ctx is not a valid Context associated with dev
*/

WFC_API_CALL void WFC_APIENTRY
    wfcCompose(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;
   WF_CONTEXT_T *context = (WF_CONTEXT_T *)ctx;

   WF_LOCK();

   if (context->device == device) {
      if (!context->active) {
         RPC_CALL1(wfcCompose_impl,
                   WFCCOMPOSE_ID,
                   RPC_INT(context->handle));
      } else
         wf_set_error(device, WFC_ERROR_UNSUPPORTED);
   } else
      wf_set_error(device, WFC_ERROR_BAD_HANDLE);

   WF_UNLOCK();
}

WFC_API_CALL void WFC_APIENTRY
    wfcFence(WFCDevice dev, WFCContext ctx, WFCEGLDisplay dpy,
        WFCEGLSync sync) WFC_APIEXIT
{
}

/*
   The wfcGetStrings function returns information about the OpenWF Composition 
   implementation, including extension information.

   The user provides a buffer to receive a list of pointers to strings specific to dev.
   strings is the address of the buffer. stringsCount is the number of string
   pointers that can fit into the buffer. If dev or name is not valid, zero is returned
   and the buffer is not modified.

   If strings is NULL, the total number of name-related strings is returned and the
   buffer is not modified.

   If strings is not NULL, the buffer is populated with a list of name-related string
   pointers. The strings are read-only and owned by the implementation. No more
   than stringsCount pointers will be written even if more are available. The
   number of string pointers written into strings is returned. If stringsCount
   is negative, no string pointers will be written.

   The combination of WFC_VENDOR and WFC_RENDERER may be used together as a
   platform identifier by applications that wish to recognize a particular platform
   and adjust their algorithms based on prior knowledge of platform bugs and
   performance characteristics.

   If name is WFC_VENDOR, a single string of the name of company responsible for
   this OpenWF Composition implementation is returned.

   If name is WFC_RENDERER, a single string of the name of the renderer is
   returned. This name is typically specific to a particular configuration of a
   hardware platform, and does not change from release to release.

   If name is WFC_VERSION, a single string of the version number of the
   specification implemented by the renderer is returned as a string in the form
   major_number.minor_number. For this specification, “1.0” is returned.

   If name is WFC_EXTENSIONS, a list of strings denoting the supported extensions
   to OpenWF Composition is returned.

   The WFCStringID enumeration defines values for strings that the user can
   query.

   typedef enum {
      WFC_VENDOR = 0x7200,
      WFC_RENDERER = 0x7201,
      WFC_VERSION = 0x7202,
      WFC_EXTENSIONS = 0x7203
   } WFCStringID;

   ERRORS

   WFC_ERROR_ILLEGAL_ARGUMENT
   - if stringsCount is negative
   - if name is not a valid string ID
*/

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetStrings(WFCDevice dev,
        WFCStringID name,
        const char **strings,
        WFCint stringsCount) WFC_APIEXIT
{
   WF_DEVICE_T *device = (WF_DEVICE_T *)dev;

   WFCint result = 0;

   const char *string = NULL;

   WF_LOCK();

   switch (name) {
   case WFC_VENDOR:
      string = "Broadcom";
      break;
   case WFC_RENDERER:
      string = "VideoCore III HW";
      break;
   case WFC_VERSION:
      string = "1.0";
      break;
   case WFC_EXTENSIONS:
      string = "";
      break;
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
      wf_set_error(device, WFC_ERROR_ILLEGAL_ARGUMENT);

   WF_UNLOCK();

   return result;
}

/*
   The wfcIsExtensionSupported function provides an alternate means of testing
   for support of a specific extension.

   WFC_TRUE will be returned if the extension denoted by string is supported by
   dev. Otherwise WFC_FALSE will be returned.
*/

WFC_API_CALL WFCboolean WFC_APIENTRY
    wfcIsExtensionSupported(WFCDevice dev, const char *string) WFC_APIEXIT
{
   return WFC_FALSE;
}
