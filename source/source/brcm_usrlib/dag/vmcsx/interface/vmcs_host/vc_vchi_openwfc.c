
#include "WF/wfc.h"
#include "interface/peer/vc_vchi_dispmanx_common.h"
#include "vchost.h"

#define DISPMANX_NO_REPLY_MASK (1<<31)
extern int32_t vc_dispmanx_send_command (uint32_t, void *, uint32_t);
extern int32_t vc_dispmanx_send_command_reply (uint32_t, void *, uint32_t,
                                               void *, uint32_t);


#define GENERATE_FUNC_HEADER_B(FUNC, ...)       \
  vc_ ## FUNC (__VA_ARGS__) WFC_APIEXIT
#define GENERATE_FUNC_HEADER_A(RET, ...)                                \
  WFC_API_CALL RET WFC_APIENTRY GENERATE_FUNC_HEADER_B(__VA_ARGS__) 

#define GENERATE_PARAM1(P1)                     \
  uint32_t param[] = {                          \
    VC_HTOV32((uint32_t)P1)                     \
  };
#define GENERATE_PARAM2(P1, P2)                 \
  uint32_t param[] = {                          \
    VC_HTOV32((uint32_t)P1),                    \
    VC_HTOV32((uint32_t)P2)                     \
  };
#define GENERATE_PARAM3(P1, P2, P3)             \
  uint32_t param[] = {                          \
    VC_HTOV32((uint32_t)P1),                    \
    VC_HTOV32((uint32_t)P2),                    \
    VC_HTOV32((uint32_t)P3)                     \
  };
#define GENERATE_PARAM4(P1, P2, P3, P4)         \
  uint32_t param[] = {                          \
    VC_HTOV32((uint32_t)P1),                    \
    VC_HTOV32((uint32_t)P2),                    \
    VC_HTOV32((uint32_t)P3),                    \
    VC_HTOV32((uint32_t)P4)                     \
  }; 
#define GENERATE_PARAM5(P1, P2, P3, P4, P5)     \
  uint32_t param[] = {                          \
    VC_HTOV32((uint32_t)P1),                    \
    VC_HTOV32((uint32_t)P2),                    \
    VC_HTOV32((uint32_t)P3),                    \
    VC_HTOV32((uint32_t)P4),                    \
    VC_HTOV32((uint32_t)P5)                     \
  };
#define GENERATE_PARAM7(P1,P2,P3,P4,P5,P6,P7)           \
  uint32_t param[] = {                                  \
    VC_HTOV32((uint32_t)P1),                            \
    VC_HTOV32((uint32_t)P2),                            \
    VC_HTOV32((uint32_t)P3),                            \
    VC_HTOV32((uint32_t)P4),                            \
    VC_HTOV32((uint32_t)P5),                            \
    VC_HTOV32((uint32_t)P6),                            \
    VC_HTOV32((uint32_t)P7)                             \
  };

#define GENERATE_FUNC_BODY1(FUNC,RET,P1T,P1)                            \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1)                               \
  { GENERATE_PARAM1(P1)                                                 \

#define GENERATE_FUNC_BODY2(FUNC,RET,P1T,P1,P2T,P2)                     \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1,P2T P2)                        \
  { GENERATE_PARAM2(P1,P2)                                              \

#define GENERATE_FUNC_BODY3(FUNC,RET,P1T,P1,P2T,P2,P3T,P3)              \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1,P2T P2,P3T P3)                 \
  { GENERATE_PARAM3(P1,P2,P3)                                           \

#define GENERATE_FUNC_BODY4(FUNC,RET,P1T,P1,P2T,P2,P3T,P3,P4T,P4)       \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1,P2T P2,P3T P3,P4T P4)          \
  { GENERATE_PARAM4(P1,P2,P3,P4)                                        \

#define GENERATE_FUNC_BODY5(FUNC,RET,P1T,P1,P2T,P2,P3T,P3,P4T,P4,P5T,P5) \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1,P2T P2,P3T P3,P4T P4,P5T P5)   \
  { GENERATE_PARAM5(P1,P2,P3,P4,P5)                                     \

#define GENERATE_FUNC_BODY7(FUNC,RET,P1T,P1,P2T,P2,P3T,P3,P4T,P4,P5T,P5,P6T,P6,P7T,P7) \
  GENERATE_FUNC_HEADER_A(RET,FUNC,P1T P1,P2T P2,P3T P3,P4T P4,P5T P5,P6T P6,P7T P7) \
  { GENERATE_PARAM7(P1,P2,P3,P4,P5,P6,P7)                               \


#define GENERATE_OPENWFC_FUNC(NUM,FUNC,RET,ARGS...)                     \
  GENERATE_FUNC_BODY ## NUM(FUNC,RET,ARGS)                              \
  int32_t ret =                                                         \
    vc_dispmanx_send_command (E ## FUNC, param, sizeof (param));        \
  return (RET)ret;                                                      \
  }
 
#define GENERATE_OPENWFC_PROC(NUM,FUNC,RET,ARGS...)                     \
  GENERATE_FUNC_BODY ## NUM(FUNC,RET,ARGS)                              \
  vc_dispmanx_send_command (E ## FUNC | DISPMANX_NO_REPLY_MASK, param,  \
                            sizeof (param));                            \
  }


/* Device */
GENERATE_OPENWFC_FUNC (3, wfcEnumerateDevices, WFCint,
                       WFCint*, deviceIds,
                       WFCint, deviceIdsCount,
                       const WFCint *, filterList)

GENERATE_OPENWFC_FUNC (2, wfcCreateDevice, WFCDevice,
                       WFCint, deviceId,
                       const WFCint *, attribList)

GENERATE_OPENWFC_FUNC (1, wfcGetError, WFCErrorCode,
                       WFCDevice, dev)

GENERATE_OPENWFC_FUNC (2, wfcGetDeviceAttribi, WFCint,
                       WFCDevice, dev,
                       WFCDeviceAttrib, attrib)

GENERATE_OPENWFC_FUNC (1, wfcDestroyDevice, WFCErrorCode,
                       WFCDevice, dev)

/* Context */
GENERATE_OPENWFC_FUNC (3, wfcCreateOnScreenContext, WFCContext,
                       WFCDevice, dev,
                       WFCint, screenNumber,
                       const WFCint *, attribList)
GENERATE_OPENWFC_FUNC (3, wfcCreateOffScreenContext, WFCContext,
                       WFCDevice, dev,
                       WFCNativeStreamType, stream,
                       const WFCint *, attribList)
GENERATE_OPENWFC_PROC (3, wfcCommit, void,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCboolean, wait)
GENERATE_OPENWFC_FUNC (3, wfcGetContextAttribi, WFCint,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCContextAttrib, attrib)

WFC_API_CALL void WFC_APIENTRY
vc_wfcGetContextAttribfv (WFCDevice dev,
                          WFCContext ctx,
                          WFCContextAttrib attrib,
                          WFCint count,
                          WFCfloat *values) WFC_APIEXIT
{
  WFCint param[4];
  param[0] = VC_HTOV32((WFCint)dev);
  param[1] = VC_HTOV32((WFCint)ctx);
  param[2] = VC_HTOV32((WFCint)attrib);
  param[3] = VC_HTOV32((WFCint)count);
  vc_dispmanx_send_command_reply (EwfcGetContextAttribfv,
                                  param, sizeof (WFCint) * 4,
                                  (void *)values, sizeof (WFCfloat) * count);
}

GENERATE_OPENWFC_PROC (4, wfcSetContextAttribi, void,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCContextAttrib, attrib,
                       WFCint, value)

WFC_API_CALL void WFC_APIENTRY
vc_wfcSetContextAttribfv (WFCDevice dev,
                          WFCContext ctx,
                          WFCContextAttrib attrib,
                          WFCint count,
                          const WFCfloat *values) WFC_APIEXIT
{
  void* param[32];
  WFCint i;
  param[0] = VC_HTOV32((void*)dev);
  param[1] = VC_HTOV32((void*)ctx);
  param[2] = VC_HTOV32((void*)attrib);
  param[3] = VC_HTOV32((void*)count);
  i = 4;
  memcpy (&param[i], values, sizeof (WFCint) * count);
  vc_dispmanx_send_command (EwfcSetContextAttribfv | DISPMANX_NO_REPLY_MASK,
                            param, sizeof (WFCint) * (i + count));
}

GENERATE_OPENWFC_PROC (2, wfcDestroyContext, void,
                       WFCDevice, dev,
                       WFCContext, ctx)

/* Source */
GENERATE_OPENWFC_FUNC (4, wfcCreateSourceFromStream, WFCSource,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCNativeStreamType, stream,
                       const WFCint *, attribList)
GENERATE_OPENWFC_PROC (2, wfcDestroySource, void,
                       WFCDevice, dev,
                       WFCSource, src)

/* Mask */
GENERATE_OPENWFC_FUNC (4, wfcCreateMaskFromStream, WFCMask,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCNativeStreamType, stream,
                       const WFCint *, attribList)
GENERATE_OPENWFC_PROC (2, wfcDestroyMask, void,
                       WFCDevice, dev,
                       WFCMask, mask)

/* Element */
GENERATE_OPENWFC_FUNC (3, wfcCreateElement, WFCElement,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       const WFCint *, attribList)
GENERATE_OPENWFC_FUNC (3, wfcGetElementAttribi, WFCint,
                       WFCDevice, dev,
                       WFCElement, element,
                       WFCElementAttrib, attrib)
GENERATE_OPENWFC_FUNC (3, wfcGetElementAttribf, WFCfloat,
                       WFCDevice, dev,
                       WFCElement, element,
                       WFCElementAttrib, attrib)

WFC_API_CALL void WFC_APIENTRY
vc_wfcGetElementAttribiv (WFCDevice device,
                          WFCElement element,
                          WFCElementAttrib attrib,
                          WFCint count,
                          WFCint* values) WFC_APIEXIT
{
  WFCint param[4];
  param[0] = VC_HTOV32((uint32_t)device);
  param[1] = VC_HTOV32((uint32_t)element);
  param[2] = VC_HTOV32((uint32_t)attrib);
  param[3] = VC_HTOV32((uint32_t)count);
  vc_dispmanx_send_command_reply (EwfcGetElementAttribiv,
                                  param, sizeof (WFCint) * 4,
                                  (void *)values, sizeof (WFCint) * count);
}

WFC_API_CALL void WFC_APIENTRY
vc_wfcGetElementAttribfv (WFCDevice device,
                          WFCElement element,
                          WFCElementAttrib attrib,
                          WFCint count,
                          WFCfloat* values) WFC_APIEXIT
{
  WFCint param[4];
  param[0] = VC_HTOV32((uint32_t)device);
  param[1] = VC_HTOV32((uint32_t)element);
  param[2] = VC_HTOV32((uint32_t)attrib);
  param[3] = VC_HTOV32((uint32_t)count);
  vc_dispmanx_send_command_reply (EwfcGetElementAttribfv,
                                  param, sizeof (WFCint) * 4,
                                  (void *)values, sizeof (WFCfloat) * count);
}

GENERATE_OPENWFC_PROC (4, wfcSetElementAttribi, void,
                       WFCDevice, dev,
                       WFCElement, element,
                       WFCElementAttrib, attrib,
                       WFCint, value)
GENERATE_OPENWFC_PROC (4, wfcSetElementAttribf, void,
                       WFCDevice, dev,
                       WFCElement, element,
                       WFCElementAttrib, attrib,
                       WFCfloat, value)

WFC_API_CALL void WFC_APIENTRY
vc_wfcSetElementAttribiv (WFCDevice device,
                          WFCElement element,
                          WFCElementAttrib attrib,
                          WFCint count,
                          const WFCint* values) WFC_APIEXIT
{
  WFCint param[32];
  WFCint i, c;
  param[0] = VC_HTOV32((uint32_t)device);
  param[1] = VC_HTOV32((uint32_t)element);
  param[2] = VC_HTOV32((uint32_t)attrib);
  param[3] = VC_HTOV32((uint32_t)count);
  i = 4;
  for (c = 0; c < count; c++)
    param[i + c] = VC_HTOV32((uint32_t)values[c]);
  vc_dispmanx_send_command (EwfcSetElementAttribiv | DISPMANX_NO_REPLY_MASK,
                            param, sizeof (WFCint) * (i + count));
}

WFC_API_CALL void WFC_APIENTRY
vc_wfcSetElementAttribfv (WFCDevice device,
                          WFCElement element,
                          WFCElementAttrib attrib,
                          WFCint count,
                          const WFCfloat* values) WFC_APIEXIT
{
  void *param[32];
  WFCint i;
  param[0] = VC_HTOV32((void *)device);
  param[1] = VC_HTOV32((void *)element);
  param[2] = VC_HTOV32((void *)attrib);
  param[3] = VC_HTOV32((void *)count);
  i = 4;
  memcpy (&param[i], values, sizeof (WFCint) * count);
  vc_dispmanx_send_command (EwfcSetElementAttribfv | DISPMANX_NO_REPLY_MASK,
                            param, sizeof (WFCint) * (i + count));
}

GENERATE_OPENWFC_PROC (3, wfcInsertElement, void,
                       WFCDevice, dev,
                       WFCElement, element,
                       WFCElement, subordinate)
GENERATE_OPENWFC_PROC (2, wfcRemoveElement, void,
                       WFCDevice, dev,
                       WFCElement, element)
GENERATE_OPENWFC_FUNC (2, wfcGetElementAbove, WFCElement,
                       WFCDevice, dev,
                       WFCElement, element)
GENERATE_OPENWFC_FUNC (2, wfcGetElementBelow, WFCElement,
                       WFCDevice, dev,
                       WFCElement, element)
GENERATE_OPENWFC_PROC (2, wfcDestroyElement, void,
                       WFCDevice, dev,
                       WFCElement, element)

/* Rendering */
GENERATE_OPENWFC_PROC (2, wfcActivate, void,
                       WFCDevice, dev,
                       WFCContext, ctx)
GENERATE_OPENWFC_PROC (2, wfcDeactivate, void,
                       WFCDevice, dev,
                       WFCContext, ctx)
GENERATE_OPENWFC_PROC (3, wfcCompose, void,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCboolean, wait)
GENERATE_OPENWFC_PROC (4, wfcFence, void,
                       WFCDevice, dev,
                       WFCContext, ctx,
                       WFCEGLDisplay, dpy,
                       WFCEGLSync, sync)

/* Renderer and extension information */
GENERATE_OPENWFC_FUNC (4, wfcGetStrings, WFCint,
                       WFCDevice, dev,
                       WFCStringID, name,
                       const char **, strings,
                       WFCint, stringsCount)
GENERATE_OPENWFC_FUNC (2, wfcIsExtensionSupported, WFCboolean,
                       WFCDevice, dev,
                       const char *, string)


/* Native Stream */
/* Fix Me: All handles should be type OWFHandle, not WFCHandle as native streams
           works for OpenWFD, OpenMax etc.  */
// #include "owftypes.h"
#define OWFNativeStreamType WFCHandle
#define OWFNativeStreamBuffer WFCHandle
#define EGLSyncKHR WFCHandle
#define OWF_IMAGE_FORMAT WFCint
#define OWF_PIXEL_FORMAT WFCint
#define OWFboolean WFCint
#define OWFint WFCint

GENERATE_OPENWFC_FUNC (7, owfNativeStreamCreateImageStream, OWFNativeStreamType,
                       WFCint, width,
                       WFCint, height,
                       OWF_PIXEL_FORMAT, pixelFormat,
                       OWFboolean, linear,
                       OWFboolean, premultiplied,
                       OWFint, rowPadding,
                       WFCint, numBuffers)
GENERATE_OPENWFC_FUNC (1, owfNativeStreamAcquireWriteBuffer, OWFNativeStreamBuffer,
                       OWFNativeStreamType, stream)
GENERATE_OPENWFC_FUNC (2, owfNativeStreamGetResource, WFCHandle,
                       OWFNativeStreamType, stream,
                       OWFNativeStreamBuffer, buffer)
GENERATE_OPENWFC_PROC (4, owfNativeStreamReleaseWriteBuffer, void,
                       OWFNativeStreamType, stream,
                       OWFNativeStreamBuffer, buf,
                       EGLDisplay, dpy,
                       EGLSyncKHR, sync)
GENERATE_OPENWFC_PROC (1, owfNativeStreamDestroy, void,
                       OWFNativeStreamType, stream)
