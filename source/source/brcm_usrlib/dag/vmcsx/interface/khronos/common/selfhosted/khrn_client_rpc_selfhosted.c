/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_int_ids.h"

#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#include "interface/vchi/vchi.h"

#include <assert.h>
#include <string.h>

static VCHI_SERVICE_HANDLE_T khrn_handle;
static VCHI_SERVICE_HANDLE_T khhn_handle;
static VCHI_SERVICE_HANDLE_T khan_handle;

static void *workspace; /* for scatter/gather bulks */
static PLATFORM_MUTEX_T mutex;

static void khrn_callback(void *callback_param, VCHI_CALLBACK_REASON_T reason, void *msg_handle)
{
}

static void khan_callback(void *callback_param, VCHI_CALLBACK_REASON_T reason, void *msg_handle)
{
   switch (reason) {
   case VCHI_CALLBACK_MSG_AVAILABLE:
   {
      int msg[4];
      uint32_t length;
      int32_t success = vchi_msg_dequeue(khan_handle, msg, sizeof(msg), &length, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
      assert(success == 0);
      vcos_assert(length == sizeof(msg));

      if (msg[0] == ASYNC_COMMAND_DESTROY) {
         /* todo: destroy */
      } else {
         PLATFORM_SEMAPHORE_T sem;
         if (khronos_platform_semaphore_create(&sem, msg + 1, 1) == KHR_SUCCESS) {
            switch (msg[0]) {
            case ASYNC_COMMAND_WAIT:
               khronos_platform_semaphore_acquire(&sem);
               break;
            case ASYNC_COMMAND_POST:
               khronos_platform_semaphore_release(&sem);
               break;
            default:
               UNREACHABLE();
               break;
            }
            khronos_platform_semaphore_destroy(&sem);
         }
      }

      break;
   }
   }
}

void vc_vchi_khronos_init(VCHI_INSTANCE_T initialise_instance,
                          VCHI_CONNECTION_T **connections,
                          uint32_t num_connections)
{
   vcos_assert(num_connections == 1);

   /*
      create services
   */

   struct {
      VCHI_SERVICE_HANDLE_T *vchi_handle;
      fourcc_t fourcc;
      VCHI_CALLBACK_T callback;
   } services[] = {
      { &khrn_handle, MAKE_FOURCC("KHRN"), khrn_callback },
      { &khhn_handle, MAKE_FOURCC("KHHN"), khrn_callback },
      { &khan_handle, MAKE_FOURCC("KHAN"), khan_callback } };

   uint32_t i;
   for (i = 0; i != (sizeof(services) / sizeof(*services)); ++i) {
      SERVICE_CREATION_T parameters = { services[i].fourcc,   // 4cc service code
                                        connections[0],       // passed in fn ptrs
                                        0,                    // rx fifo size (unused)
                                        0,                    // tx fifo size (unused)
                                        services[i].callback, // service callback
                                        NULL,                 // callback parameter
                                        1,                    // want unaligned bulk rx
                                        1,                    // want unaligned bulk tx
                                        1 };                  // want crc
      int32_t success = vchi_service_open(initialise_instance, &parameters, services[i].vchi_handle);
      assert(success == 0);
   }

   /*
      attach to process (there's just one)
   */

   bool success = client_process_attach();
   assert(success);

}

bool khclient_rpc_init(void)
{
   workspace = khrn_platform_malloc(KHDISPATCH_WORKSPACE_SIZE, "rpc_workspace");
   if(workspace == NULL)
   {
      return false;
   }
   if(platform_mutex_create(&mutex) != KHR_SUCCESS)
   {
      khrn_platform_free(workspace);
      return false;
   }
   return true;
}

void rpc_term(void)
{
   khrn_platform_free(workspace);
   platform_mutex_destroy(&mutex);
}

static VCHI_SERVICE_HANDLE_T get_vchi_handle(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority ? khhn_handle : khrn_handle;
}

static void check_workspace(uint32_t size)
{
   /* todo: find a better way to handle scatter/gather bulks */
   vcos_assert(size <= KHDISPATCH_WORKSPACE_SIZE);
}

static void merge_flush(CLIENT_THREAD_STATE_T *thread)
{
   vcos_assert(thread->merge_pos >= CLIENT_MAKE_CURRENT_SIZE);

   /*
      don't transmit just a make current -- in the case that there is only a
      make current in the merge buffer, we have already sent a control message
      for the rpc call (and with it a make current) and own the big lock
   */

   if (thread->merge_pos > CLIENT_MAKE_CURRENT_SIZE) {
      int32_t success = vchi_msg_queue(get_vchi_handle(thread), thread->merge_buffer, thread->merge_pos, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
      assert(success == 0);

      thread->merge_pos = 0;

      client_send_make_current(thread);

      vcos_assert(thread->merge_pos == CLIENT_MAKE_CURRENT_SIZE);
   }
}

void rpc_flush(void)
{
   merge_flush(CLIENT_GET_THREAD_STATE());
}

void rpc_high_priority_begin(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(!thread->high_priority);
   merge_flush(thread);
   thread->high_priority = true;
}

void rpc_high_priority_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(thread->high_priority);
   merge_flush(thread);
   thread->high_priority = false;
}

uint32_t rpc_send_ctrl_longest(uint32_t len_min)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   uint32_t len = MERGE_BUFFER_SIZE - thread->merge_pos;
   if (len < len_min) {
      len = MERGE_BUFFER_SIZE - CLIENT_MAKE_CURRENT_SIZE;
   }
   return len;
}

void rpc_send_ctrl_begin(uint32_t len)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(len == rpc_pad_ctrl(len));
   if ((thread->merge_pos + len) > MERGE_BUFFER_SIZE) {
      merge_flush(thread);
   }

   thread->merge_end = thread->merge_pos + len;
}

void rpc_send_ctrl_write(const uint32_t in[], uint32_t len)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   memcpy(thread->merge_buffer + thread->merge_pos, in, len);
   thread->merge_pos += rpc_pad_ctrl(len);
   vcos_assert(thread->merge_pos <= MERGE_BUFFER_SIZE);
}

void rpc_send_ctrl_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(thread->merge_pos == thread->merge_end);
}

//#define CTRL_THRESHOLD (VCHI_MAX_MSG_SIZE >> 1)
#define CTRL_THRESHOLD (4096-32) //vchiq msgsize header size

static void send_bulk(CLIENT_THREAD_STATE_T *thread, const void *in, uint32_t len)
{
   vcos_assert(len < 0x10000000);

   int32_t success = (len <= CTRL_THRESHOLD) ?
      vchi_msg_queue(get_vchi_handle(thread), in, len, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) :
      vchi_bulk_queue_transmit(get_vchi_handle(thread), in, len, VCHI_FLAGS_BLOCK_UNTIL_DATA_READ, NULL);
   assert(success == 0);
}

static void recv_bulk(CLIENT_THREAD_STATE_T *thread, void *out, uint32_t len)
{
   vcos_assert(len < 0x10000000);

   uint32_t msg_len;
   int32_t success = (len <= CTRL_THRESHOLD) ?
      vchi_msg_dequeue(get_vchi_handle(thread), out, len, &msg_len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) :
      vchi_bulk_queue_receive(get_vchi_handle(thread), out, len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL);
   assert(success == 0);
}

void rpc_send_bulk(const void *in, uint32_t len)
{
   if (in && len) {
      CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

      merge_flush(thread);

      send_bulk(thread, in, len);
   }
}

void rpc_send_bulk_gather(const void *in, uint32_t len, int32_t stride, uint32_t n)
{
   if (in && len) {
      CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

      merge_flush(thread);

      if (len == stride) {
         /* hopefully should be the common case */
         send_bulk(thread, in, n * len);
      } else {
         check_workspace(n * len);
         rpc_gather(workspace, in, len, stride, n);
         send_bulk(thread, workspace, n * len);
      }
   }
}

uint32_t rpc_recv(void *out, uint32_t *len_io, RPC_RECV_FLAG_T flags)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();
   uint32_t res = 0;
   uint32_t len;
   bool recv_ctrl;

   if (!len_io) { len_io = &len; }

   recv_ctrl = flags & (RPC_RECV_FLAG_RES | RPC_RECV_FLAG_CTRL | RPC_RECV_FLAG_LEN); /* do we want to receive anything in the control channel at all? */
   vcos_assert(recv_ctrl || (flags & RPC_RECV_FLAG_BULK)); /* must receive something... */
   vcos_assert(!(flags & RPC_RECV_FLAG_CTRL) || !(flags & RPC_RECV_FLAG_BULK)); /* can't receive user data over both bulk and control... */

   if (recv_ctrl || len_io[0]) { /* do nothing if we're just receiving bulk of length 0 */
      merge_flush(thread);

      if (recv_ctrl) {
         void *ctrl_begin;
         uint32_t ctrl_len;
         VCHI_HELD_MSG_T held_msg;
         int32_t success = vchi_msg_hold(get_vchi_handle(thread), &ctrl_begin, &ctrl_len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, &held_msg);
         uint32_t *ctrl = (uint32_t *)ctrl_begin;
         assert(success == 0);
         assert(ctrl_len == rpc_pad_ctrl(ctrl_len));
         if (flags & RPC_RECV_FLAG_LEN) {
            len_io[0] = *(ctrl++);
         }
         if (flags & RPC_RECV_FLAG_RES) {
            res = *(ctrl++);
         }
         if (flags & RPC_RECV_FLAG_CTRL) {
            memcpy(out, ctrl, len_io[0]);
            ctrl += rpc_pad_ctrl(len_io[0]) >> 2;
         }
         vcos_assert((uint8_t *)ctrl == ((uint8_t *)ctrl_begin + ctrl_len));
         success = vchi_held_msg_release(&held_msg);
         assert(success == 0);
      }

      if ((flags & RPC_RECV_FLAG_BULK) && len_io[0]) {
         if (flags & RPC_RECV_FLAG_BULK_SCATTER) {
            if ((len_io[0] == len_io[1]) && !len_io[3] && !len_io[4]) {
               /* hopefully should be the common case */
               recv_bulk(thread, out, len_io[2] * len_io[0]);
            } else {
               check_workspace(len_io[2] * len_io[0]);
               recv_bulk(thread, workspace, len_io[2] * len_io[0]);
               rpc_scatter(out, len_io[0], len_io[1], len_io[2], len_io[3], len_io[4], workspace);
            }
         } else {
            recv_bulk(thread, out, len_io[0]);
         }
      }
   }

   return res;
}

void rpc_begin(void)
{
   platform_mutex_acquire(&mutex);
}

void rpc_end(void)
{
   platform_mutex_release(&mutex);
}


EGLDisplay khrn_platform_set_display_id(EGLNativeDisplayType display_id)
{
	if (display_id == EGL_DEFAULT_DISPLAY)
      return (EGLDisplay)1;
   else
      return EGL_NO_DISPLAY;
}

//dummy functions

static int xxx_position = 0;
uint32_t khrn_platform_get_window_position(EGLNativeWindowType win)
{
   return xxx_position;
}

void khrn_platform_release_pixmap_info(EGLNativePixmapType pixmap, KHRN_IMAGE_WRAP_T *image)
{
   /* Nothing to do */
}

////
