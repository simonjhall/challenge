#include "interface/khronos/include/WF/wfc.h"

typedef struct {

} WF_PROCESS_STATE_T;

/*
   Devices are identified using integer IDs assigned by the implementation8. The
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

WFC_API_CALL WFCint WFC_APIENTRY
    wfcEnumerateDevices(WFCint *deviceIds, WFCint deviceIdsCount,
        const WFCint *filterList) WFC_APIEXIT
{


}

WFC_API_CALL WFCDevice WFC_APIENTRY
    wfcCreateDevice(WFCint deviceId, const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcGetError(WFCDevice dev) WFC_APIEXIT
{
}

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetDeviceAttribi(WFCDevice dev, WFCDeviceAttrib attrib) WFC_APIEXIT
{
}

WFC_API_CALL WFCErrorCode WFC_APIENTRY
    wfcDestroyDevice(WFCDevice dev) WFC_APIEXIT
{
}

/* Context */
WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOnScreenContext(WFCDevice dev,
        WFCint screenNumber,
        const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL WFCContext WFC_APIENTRY
    wfcCreateOffScreenContext(WFCDevice dev,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcCommit(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
}

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribi(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib, WFCint value) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetContextAttribfv(WFCDevice dev, WFCContext ctx,
        WFCContextAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyContext(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
}

/* Source */
WFC_API_CALL WFCSource WFC_APIENTRY
    wfcCreateSourceFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcDestroySource(WFCDevice dev, WFCSource src) WFC_APIEXIT
{
}

/* Mask */
WFC_API_CALL WFCMask WFC_APIENTRY
    wfcCreateMaskFromStream(WFCDevice dev, WFCContext ctx,
        WFCNativeStreamType stream,
        const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyMask(WFCDevice dev, WFCMask mask) WFC_APIEXIT
{
}

/* Element */
WFC_API_CALL WFCElement WFC_APIENTRY
    wfcCreateElement(WFCDevice dev, WFCContext ctx,
        const WFCint *attribList) WFC_APIEXIT
{
}

WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetElementAttribi(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib) WFC_APIEXIT
{
}

WFC_API_CALL WFCfloat WFC_APIENTRY
    wfcGetElementAttribf(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribiv(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib, WFCint count, WFCint *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcGetElementAttribfv(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib, WFCint count, WFCfloat *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribi(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib, WFCint value) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribf(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib, WFCfloat value) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribiv(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib,
        WFCint count, const WFCint *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcSetElementAttribfv(WFCDevice dev, WFCElement element,
        WFCElementAttrib attrib,
        WFCint count, const WFCfloat *values) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcInsertElement(WFCDevice dev, WFCElement element,
        WFCElement subordinate) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcRemoveElement(WFCDevice dev, WFCElement element) WFC_APIEXIT
{
}

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementAbove(WFCDevice dev, WFCElement element) WFC_APIEXIT
{
}

WFC_API_CALL WFCElement WFC_APIENTRY
    wfcGetElementBelow(WFCDevice dev, WFCElement element) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcDestroyElement(WFCDevice dev, WFCElement element) WFC_APIEXIT
{
}

/* Rendering */
WFC_API_CALL void WFC_APIENTRY
    wfcActivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcDeactivate(WFCDevice dev, WFCContext ctx) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcCompose(WFCDevice dev, WFCContext ctx, WFCboolean wait) WFC_APIEXIT
{
}

WFC_API_CALL void WFC_APIENTRY
    wfcFence(WFCDevice dev, WFCContext ctx, WFCEGLDisplay dpy,
        WFCEGLSync sync) WFC_APIEXIT
{
}

/* Renderer and extension information */
WFC_API_CALL WFCint WFC_APIENTRY
    wfcGetStrings(WFCDevice dev,
        WFCStringID name,
        const char **strings,
        WFCint stringsCount) WFC_APIEXIT
{
}

WFC_API_CALL WFCboolean WFC_APIENTRY
    wfcIsExtensionSupported(WFCDevice dev, const char *string) WFC_APIEXIT
{
}

#ifdef __cplusplus
}
#endif

#endif /* _WFC_H_ */