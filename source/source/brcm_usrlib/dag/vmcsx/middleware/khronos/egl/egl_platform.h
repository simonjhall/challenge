/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef EGL_PLATFORM_H
#define EGL_PLATFORM_H

#include "interface/khronos/common/khrn_int_common.h"
#include "middleware/khronos/common/khrn_image.h"

#include "interface/khronos/include/EGL/egl.h"
#include "interface/khronos/include/EGL/eglext.h"

#define EGL_PLATFORM_WIN_NONE 0xffffffff

/*
   Schedule the given image to be displayed on the given window as soon as
   possible. The dimensions of the image should agree with the window
   dimensions most of the time, but if the window has changed size recently
   they may not. In this case it's up to the platform what it should do, but
   the most sensible thing is probably to scale the image (TODO: is this really
   true?)

   When the image comes onto the display, the return callback (as passed to
   egl_server_platform_init) should be called with argument cb_arg. At this
   point, the image previously being displayed on the given window is assumed to
   be finished with
*/

extern void egl_server_platform_display(uint32_t win, KHRN_IMAGE_T *image, uint32_t cb_arg);

/*
   Remove the image being displayed on the given window (if there is one).
   Return when the image is off the display

   When this function returns, we assume:
   - the image that was on the display is finished with
   - any callbacks for previous updates to the given window have returned
     (todo: what about the case where we switch windows?)
*/

extern void egl_server_platform_display_nothing_sync(uint32_t win);

/*
   Used on platforms with server-side pixmaps. Retrieves all of the relevant
   information about a pixmap.
*/

MEM_HANDLE_T egl_server_platform_create_pixmap_info(uint32_t pixmap);

typedef void (*EGL_SERVER_RETURN_CALLBACK_T)(uint32_t cb_arg);

void egl_server_platform_init(EGL_SERVER_RETURN_CALLBACK_T return_callback);
void egl_server_platform_shutdown(void);

extern void khdispatch_send_async(uint32_t command, uint64_t pid_in, uint32_t sem);

#if EGL_BRCM_perf_monitor

struct VC_IMAGE_T;

/*
   Show or hide the performance monitor surface.
*/

extern void egl_server_platform_perf_monitor_show(struct VC_IMAGE_T *image);
extern void egl_server_platform_perf_monitor_hide();
#endif

extern void egl_server_platform_set_position(uint32_t handle, uint32_t position, uint32_t width, uint32_t height);

#endif
