/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  Khronos
Module   :  OpenWF-C

FILE DESCRIPTION
OpenWF-C native stream definitions
=============================================================================*/

#ifndef WF_INT_STREAM_H_
#define WF_INT_STREAM_H_

#include "helpers/vc_image/vc_image.h"

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/include/WF/wfc.h"

//==============================================================================

//==============================================================================

// Create a stream able to queue the specified number of buffers, using either
// the given image properties, or an off-screen context, as a template.

#define WFC_STREAM_FLAGS_NONE          0
// Create asynchronous (KHAN) semaphore, for use with wfc_stream_await_buffer().
#define WFC_STREAM_FLAGS_ASYNC_SEM     (1 << 0)

// Create a stream, using the given stream handle (typically assigned by the
// window manager), containing the specified number of buffers, and using
// a bufman image as a template. Return zero if OK.
uint32_t wfc_stream_create_from_image
   (WFCNativeStreamType stream, VC_IMAGE_TYPE_T pixel_format,
      uint32_t width, uint32_t height, uint32_t flags);

WFCNativeStreamType wfc_stream_create_from_context
   (WFCContext context, uint32_t num_of_buffers, uint32_t flags);

// Suspend until buffer is available on the server (requires
// WFC_STREAM_FLAGS_ASYNC_SEM to have been specified on creation).
void wfc_stream_await_buffer(WFCNativeStreamType stream);

// Destroy a stream.
void wfc_stream_destroy(WFCNativeStreamType stream);

//------------------------------------------------------------------------------
// Off-screen composition functions

// Get off-screen image from stream. Fills buffer passed to it.
void wfc_stream_get_off_screen_image
   (WFCNativeStreamType stream, VC_IMAGE_T *image);

// Returns the number of buffers containing data.
uint32_t wfc_stream_num_of_ready_buffers(WFCNativeStreamType stream);

// Returns true if stream is not a valid stream.
bool wfc_stream_is_invalid(WFCNativeStreamType stream);

// TODO register notify callback. To be triggered when a new buffer is available
// for reading. Could be important for autonomous rendering.

// TODO may need some synchronisation.

//------------------------------------------------------------------------------
//==============================================================================

#endif /* WF_INT_STREAM_H_ */
