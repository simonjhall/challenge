/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef WFC_SERVER_STREAM_H_
#define WFC_SERVER_STREAM_H_

#include "interface/khronos/include/WF/wfc.h"
#include "helpers/vc_image/vc_image.h"
#include "interface/khronos/common/khrn_int_common.h"

//==============================================================================

typedef uint32_t BUFFER_HANDLE_T;

typedef void (*WFC_STREAM_SERVER_RENDERER_CALLBACK_T)(uint32_t cb_arg);

typedef void (*WFC_STREAM_SERVER_UPDATE_CALLBACK_T)(const WFCElement element_list[]);

//==============================================================================

// Values for "suspend" parameter for notification functions
#define WFC_SUSPEND  true
#define WFC_POLL     false

//==============================================================================
// Functions called remotely by stream client.

// Create a stream and its buffers for the specified image type and size, for
// supplying pixel data to one or more elements, and/or for storing the output
// of off-screen composition; return a stream key (or zero if there's a problem).
void wfc_stream_server_create
   (WFCNativeStreamType stream, VC_IMAGE_TYPE_T pixel_format,
      uint32_t width, uint32_t height, uint32_t flags, uint32_t pid_lo, uint32_t pid_hi);

// Check to see if the specified buffer has been filled by the renderer. If "suspend"
// is true, suspend until then. If "buffer_handle" is zero, use the next buffer to
// be filled.
bool wfc_stream_server_is_rendering_complete
   (WFCNativeStreamType stream, BUFFER_HANDLE_T buffer_handle, bool suspend);

// Check to see if all the data for the specified context has been rendered,
// and is ready to be sent to the compositor.
bool wfc_stream_server_is_update_ready
   (WFCNativeStreamType stream, WFCContext context, bool suspend);

// Destroy stream
void wfc_stream_server_destroy(WFCNativeStreamType stream);

// Lock front buffer for reading, and get its handle (for off-screen).
// (Data is pulled across using bufman).
BUFFER_HANDLE_T wfc_stream_server_acquire_front_buffer_handle(WFCNativeStreamType stream);

// Release previously acquired front buffer (for off-screen)
void wfc_stream_server_release_front_buffer_handle
   (WFCNativeStreamType stream, BUFFER_HANDLE_T buffer_handle);

//------------------------------------------------------------------------------
// Functions called by WF-C server.

// Function to be called when stream is updated (for use with activated contexts)
void wfc_stream_server_set_update_callback(WFC_STREAM_SERVER_UPDATE_CALLBACK_T cb_func);

// Lock front buffer for reading, and get its data (for elements).
VC_IMAGE_T *wfc_stream_server_acquire_front_buffer_data(WFCNativeStreamType stream);

// Record that an element has released this buffer. When all elements which are
// using this buffer have released it, the client can signal to the renderer
// that the buffer is free).
void wfc_stream_server_release_front_buffer_data(WFCNativeStreamType stream);

// Associate an element with a stream when attributes are committed, and disassociate
// the old one. Returns 0 if this is OK, 1 if not.
uint32_t wfc_stream_server_associate_element
   (WFCNativeStreamType old_stream, WFCNativeStreamType new_stream, WFCElement element);

// Return next available back buffer (or zero if none available) (for off-screen).
// (Note that acquisition of back buffers for elements is handled by the stream
// client).
VC_IMAGE_T *wfc_stream_server_acquire_back_buffer(WFCNativeStreamType stream);

//------------------------------------------------------------------------------
// Functions called by renderer server (for elements) and WF-C server (for off-screen).

// Signal to client that a buffer is available for writing to. For use when
// renderer is allocating its buffers.
void wfc_stream_server_signal_buffer_avail(WFCNativeStreamType stream);

void wfc_stream_server_signal_buffer_removed(WFCNativeStreamType stream);
// Signal to client that a buffer has been removed. For use when
// renderer is deleting its buffers.

// Queue image in stream's buffer. Returns an error code.
#define WFC_QUEUE_OK                   0
#define WFC_QUEUE_NO_IMAGE             (1 << 0)
#define WFC_QUEUE_WRONG_DIMENSIONS     (1 << 1)
#define WFC_QUEUE_WRONG_PIXEL_FORMAT   (1 << 2)

uint32_t wfc_stream_server_queue_image_data
   (WFCNativeStreamType stream, VC_IMAGE_T *image,
      WFC_STREAM_SERVER_RENDERER_CALLBACK_T cb_func, uint32_t cb_arg);

//==============================================================================

#endif /* WFC_SERVER_STREAM_H_ */
