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

#include "ipc_vchi.h"

static IPC_shared_mem_t *ipc = NULL;
static void *workspace; /* for scatter/gather bulks */

static int32_t set_vchi_handle(/*VCHI_SERVICE_HANDLE_T*/int32_t handle)
{
   //return (handle==khhn_handle)?0:1;
   return handle;

}

static int32_t ipc_aquire(IPC_SEMAPHORE_T sem)
{
	sem_wait(&ipc->ipc_sem[sem]);
	return 0;
}

static int32_t ipc_release(IPC_SEMAPHORE_T sem)
{
	sem_post(&ipc->ipc_sem[sem]);
	return 0;
}

static int32_t ipc_wait(int32_t *events)
{
	sem_wait(&ipc->ipc_sem[IPC_SEM_S2C]);
	if (events)
		*events = ipc->s2c_events;
	return 0;
}

static int32_t ipc_signal(int32_t event_mask)
{
	ipc->c2s_events = (1 << event_mask);
	sem_post(&ipc->ipc_sem[IPC_SEM_C2S]);
	return 0;
}

int32_t ipc_vchi_msg_queue(/*VCHI_SERVICE_HANDLE_T*/int32_t handle,
	const void * data,
	uint32_t data_size,
	VCHI_FLAGS_T flags,
	void * msg_handle )
{
	// vchi_msg_queue(handle,data,data_size,flags,msg_handle);
	uint32_t events;

	ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = set_vchi_handle(handle);
	ipc->buffer[1] = data_size;
	ipc->buffer[2] = flags;
	ipc->buffer[3] = 0xbabeface;
	memcpy(&ipc->buffer[4],data,data_size);
	ipc_signal(IPC_VCHI_MSG_QUEUE_EVENT);
	ipc_wait(&events);
	ipc_release(IPC_SEM_ACCESS);

	return 0;
}

int32_t ipc_vchi_bulk_queue_transmit( /*VCHI_SERVICE_HANDLE_T*/int32_t handle,
                                  const void * data_src,
                                  uint32_t data_size,
                                  VCHI_FLAGS_T flags,
                                  void * bulk_handle )
{
	// vchi_bulk_queue_transmit(handle,data_src,data_size,flags,bulk_handle);
	uint32_t events;
	int32_t success=0;

	os_assert(data_size<=IPC_SHARED_MEM_SIZE);

	ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = set_vchi_handle(handle);
	ipc->buffer[1] = data_size;
	ipc->buffer[2] = flags;
	ipc->buffer[3] = 0xbabeface;
	memcpy(&ipc->buffer[4],data_src,data_size);
	ipc_signal(IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT);
	ipc_wait(&events);
	ipc_release(IPC_SEM_ACCESS);

	return 0;
}

int32_t ipc_vchi_msg_dequeue( /*VCHI_SERVICE_HANDLE_T*/int32_t handle,
                          void *data,
                          uint32_t max_data_size_to_read,
                          uint32_t *actual_msg_size,
                          VCHI_FLAGS_T flags )
{
	// vchi_msg_dequeue(handle,data,max_data_size_to_read,actual_msg_size,flags);
	uint32_t events;
	uint32_t read_size;
	int32_t success=0;

	os_assert(max_data_size_to_read<IPC_SHARED_MEM_SIZE);

	ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = set_vchi_handle(handle);
	ipc->buffer[1] = max_data_size_to_read;
	ipc->buffer[2] = flags;
	ipc->buffer[3] = 0xbabeface;
	ipc_signal(IPC_VCHI_MSG_DEQUEUE_EVENT);
	ipc_wait(&events);
	os_assert(events == (1<<IPC_VCHI_MSG_DEQUEUE_EVENT));

	read_size = ipc->buffer[1];
	os_assert(read_size <= max_data_size_to_read);

	memcpy(data,&ipc->buffer[4],read_size);
	*actual_msg_size = read_size;
	ipc_release(IPC_SEM_ACCESS);

	return success;
}

int32_t ipc_vchi_bulk_queue_receive(/*VCHI_SERVICE_HANDLE_T*/int32_t handle,
                                 void * data_dst,
                                 uint32_t data_size,
                                 VCHI_FLAGS_T flags,
                                 void * bulk_handle )
{
	// vchi_bulk_queue_receive(handle,data_dst,data_size,flags,bulk_handle);
	int32_t events;
	int32_t success=0;

	os_assert(data_size<IPC_SHARED_MEM_SIZE);

	ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = set_vchi_handle(handle);
	ipc->buffer[1] = data_size;
	ipc->buffer[2] = flags;
	ipc->buffer[3] = 0xbabeface;
	ipc_signal(IPC_VCHI_BULK_RECEIVE_EVENT);
	ipc_wait(&events);
	os_assert(events == (1<<IPC_VCHI_BULK_RECEIVE_EVENT));

	memcpy(data_dst,&ipc->buffer[4],data_size);
	ipc_release(IPC_SEM_ACCESS);

	return success;
}


int32_t ipc_vchi_msg_hold( /*VCHI_SERVICE_HANDLE_T*/int32_t handle,
                       void **data,
                       uint32_t *msg_size,
                       VCHI_FLAGS_T flags,
                       VCHI_HELD_MSG_T *message_handle )
{
	//vchi_msg_hold(handle,data,msg_size,flags,message_handle);
	int32_t events;
	int32_t success=0;

	ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = set_vchi_handle(handle);
	ipc->buffer[1] = 0;
	ipc->buffer[2] = flags;
	ipc->buffer[3] = 0xbabeface;
	ipc_signal(IPC_VCHI_MSG_HOLD_EVENT);
	ipc_wait(&events);
	os_assert(events == (1<<IPC_VCHI_MSG_HOLD_EVENT));

	*msg_size = ipc->buffer[1];
	// message_handle = (VCHI_HELD_MSG_T*)ipc->buffer[2];
	*data = &ipc->buffer[4];
	// ipc_release(IPC_SEM_ACCESS);

	return success;
}


int32_t ipc_vchi_held_msg_release( VCHI_HELD_MSG_T *message )
{
	// vchi_held_msg_release(message);
	int32_t events;
	int32_t success=0;

	// ipc_aquire(IPC_SEM_ACCESS);
	ipc->buffer[0] = 0;
	ipc->buffer[1] = 0;
	ipc->buffer[2] = (uint32_t)message;
	ipc->buffer[3] = 0xbabeface;
	ipc_signal(IPC_VCHI_MSG_RELEASE_EVENT);
	ipc_wait(&events);
	os_assert(events == (1<<IPC_VCHI_MSG_RELEASE_EVENT));
	ipc_release(IPC_SEM_ACCESS);

	return success;
}

bool khclient_rpc_init(void)
{
   workspace = NULL;
   ipc = (IPC_shared_mem_t*)getBufferPointer();
   return true;
}

void rpc_term(void)
{
   if (workspace) { khrn_platform_free(workspace); }
}

static /*VCHI_SERVICE_HANDLE_T*/int32_t get_vchi_handle(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority;
}


static void check_workspace(uint32_t size)
{
   /* todo: find a better way to handle scatter/gather bulks */
   vcos_assert(size <= KHDISPATCH_WORKSPACE_SIZE);
   if (!workspace) {
      workspace = khrn_platform_malloc(KHDISPATCH_WORKSPACE_SIZE, "rpc_workspace");
      assert(workspace);
   }
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
      int32_t success = ipc_vchi_msg_queue(get_vchi_handle(thread), thread->merge_buffer, thread->merge_pos, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
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

// #define CTRL_THRESHOLD (VCHI_MAX_MSG_SIZE >> 1)
#define CTRL_THRESHOLD (4096-32) //vchiq msgsize header size

static void send_bulk(CLIENT_THREAD_STATE_T *thread, const void *in, uint32_t len)
{
   vcos_assert(len < 0x10000000);

   int32_t success = (len <= CTRL_THRESHOLD) ?
      ipc_vchi_msg_queue(get_vchi_handle(thread), in, len, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) :
      ipc_vchi_bulk_queue_transmit(get_vchi_handle(thread), in, len, VCHI_FLAGS_BLOCK_UNTIL_DATA_READ, NULL);
   assert(success == 0);
}

static void recv_bulk(CLIENT_THREAD_STATE_T *thread, void *out, uint32_t len)
{
   vcos_assert(len < 0x10000000);

   uint32_t msg_len;
   int32_t success = (len <= CTRL_THRESHOLD) ?
      ipc_vchi_msg_dequeue(get_vchi_handle(thread), out, len, &msg_len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) :
      ipc_vchi_bulk_queue_receive(get_vchi_handle(thread), out, len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL);
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
         int32_t success = ipc_vchi_msg_hold(get_vchi_handle(thread), &ctrl_begin, &ctrl_len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, &held_msg);
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
         success = ipc_vchi_held_msg_release(&held_msg);
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

static void ipc_reentrant_mutex_lock(IPC_REENTRANT_MUTEX_T *m)
{
   pid_t p = getpid();

   if (m->owner != p)
   {
      ipc_aquire(m->name);
      m->owner = p;
      m->count = 0;
   }
   m->count++;
}

static void ipc_reentrant_mutex_unlock(IPC_REENTRANT_MUTEX_T *m)
{
   m->count--;
   if (m->count == 0)
   {
      m->owner = 0;
      ipc_release(m->name);
   }
}

void rpc_begin(void)
{
   ipc_reentrant_mutex_lock(&ipc->ipc_mutex);
}

void rpc_end(void)
{
   ipc_reentrant_mutex_unlock(&ipc->ipc_mutex);
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
