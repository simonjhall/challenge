#include "interface/vmcs_host/wfc.h"
#include "interface/vmcs_host/owfstream.h"
#include "vc_vchi_bufman.h"

WFCNativeStreamType
createNativeStream (WFCint width,
                    WFCint height,
                    WFCint numBuffers)
{
  WFCNativeStreamType stream =
    owfNativeStreamCreateImageStream (width,
                                      height,
                                      OWF_IMAGE_ARGB8888,
                                      OWF_FALSE,
                                      OWF_TRUE,
                                      4,
                                      numBuffers);
  return stream;
}

void
writeImageToNativeStream (WFCNativeStreamType stream, WFCint width,
                          WFCint height, WFCint stride, void* data)
{
  OWFNativeStreamBuffer buffer;
  BUFMANX_HANDLE_T dummy_handle;
  BUFMANX_IMAGE_T image;
  VC_RECT_T src_rect, dst_rect;
  DISPMANX_RESOURCE_HANDLE_T resource;
  int status;

  buffer = owfNativeStreamAcquireWriteBuffer (stream);
  resource = owfNativeStreamGetResource(stream, buffer);
  image.type = FRAME_HOST_IMAGE_EFormatRgbA32;
  //    image.type = FRAME_HOST_IMAGE_EFormatRgb888;
  image.width = width;
  image.height = height;
  image.pitch = stride;
  image.bpp = 32;
  image.size = stride * height;
  image.pixels = data;
  src_rect.x = 0;
  src_rect.y = 0;
  src_rect.width = width << 16;
  src_rect.height = height << 16;
  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.width = width;
  dst_rect.height = height;
  status = vc_bufmanx_push_blocking (&dummy_handle, &image, resource,
                                     &src_rect, &dst_rect, 0);
  /* USBDK: the handle is created on the bridge */
  vcos_assert (status == 0);
  owfNativeStreamReleaseWriteBuffer (stream, buffer,
                                     EGL_DEFAULT_DISPLAY,
                                     EGL_NO_SYNC_KHR);
}

void
destroyNativeStream (WFCNativeStreamType stream)
{
  owfNativeStreamDestroy(stream);
}
