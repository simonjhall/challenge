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

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h"

#include "interface/khronos/common/khrn_int_ids.h"

#include "middleware/khronos/egl/egl_server.h"
#include "middleware/khronos/common/khrn_misc.h"

#ifndef KHRN_USE_VCHIQ
#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi.h"
#include "interface/vchi/common/endian.h"
#endif

#include <stdlib.h>

/******************************************************************************
workspace
******************************************************************************/

static MEM_HANDLE_T workspace_handle = MEM_INVALID_HANDLE;
void *khdispatch_workspace = NULL;

void khdispatch_check_workspace(uint32_t size)
{
   if (size == LENGTH_SIGNAL_NULL) {
      size = 0;
   }
   vcos_assert(size <= KHDISPATCH_WORKSPACE_SIZE);
   if ((size != 0) && !khdispatch_workspace) {
      khdispatch_workspace = mem_lock(workspace_handle);
   }
}

bool khdispatch_within_workspace(const void *ptr, size_t size)
{
   return khdispatch_workspace &&
          ptr >= khdispatch_workspace &&
          ptr <= (const void*)((const uint8_t*)khdispatch_workspace + KHDISPATCH_WORKSPACE_SIZE) &&
          size <= (uint8_t*)khdispatch_workspace + KHDISPATCH_WORKSPACE_SIZE - (uint8_t*)ptr; 
}

/******************************************************************************
core internal api
******************************************************************************/

uint32_t *khdispatch_message;
uint32_t khdispatch_position;
uint32_t khdispatch_length;

#ifdef KHRN_USE_VCHIQ
static VCHIQ_SERVICE_HANDLE_T khdispatch_khrn_handle;
static VCHIQ_SERVICE_HANDLE_T khdispatch_khan_handle;

static VCHIU_QUEUE_T *khdispatch_queue;

static VCOS_EVENT_T *khdispatch_event;
#else
static VCHI_SERVICE_HANDLE_T khdispatch_khrn_handle;
static VCHI_SERVICE_HANDLE_T khdispatch_khan_handle;
#endif

void khdispatch_send_ctrl(const void *out, uint32_t len)
{
#ifdef KHRN_USE_VCHIQ
   VCHIQ_ELEMENT_T element = {out, len};

   vchiq_queue_message(khdispatch_khrn_handle, &element, 1);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
#else
   verify(vchi_msg_queue(khdispatch_khrn_handle, out, khdispatch_pad_ctrl(len), VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) == 0);
#endif
}

void khdispatch_send_ctrl2(const void *out0, uint32_t len0, const void *out1, uint32_t len1)
{
#ifdef KHRN_USE_VCHIQ
   VCHIQ_ELEMENT_T elements[2] = {out0, len0, out1, len1};

   vchiq_queue_message(khdispatch_khrn_handle, elements, 2);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
#else
   VCHI_MSG_VECTOR_T vector[] = {{out0, khdispatch_pad_ctrl(len0)}, {out1, khdispatch_pad_ctrl(len1)}};
   verify(vchi_msg_queuev(khdispatch_khrn_handle, vector, 2, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) == 0);
#endif
}

#ifdef KHRN_USE_VCHIQ
#define CTRL_THRESHOLD (VCHIQ_CHANNEL_SIZE >> 1)
#else
//#define CTRL_THRESHOLD (VCHI_MAX_MSG_SIZE >> 1)
#ifdef CLIENT_IPC_NO_VCHIQ  
#define CTRL_THRESHOLD 0 //force bulk data to go bulk_queue_transmit/recevie
#else
#define CTRL_THRESHOLD (4096-32) //vchiq msgsize header size
#endif
#endif

bool khdispatch_recv_bulk(void* data, uint32_t len)
{
#ifdef KHRN_USE_VCHIQ
	uint8_t *in = (uint8_t*) data;
   if (in && len) {
      if (len <= CTRL_THRESHOLD) {
         VCHIQ_HEADER_T *header = vchiu_queue_pop(khdispatch_queue);
         if(NULL!=header) {
            vcos_assert(header->size == len);
            memcpy(data, header->data, len);
            vchiq_release_message(khdispatch_khrn_handle, header);
         } else {
            goto fail;
         }
      } else {
         if(vchiq_queue_bulk_receive(khdispatch_khrn_handle, data, len, NULL)!=VCHIQ_SUCCESS)
            goto fail;
         if(vcos_event_wait(khdispatch_event)!=VCHIQ_SUCCESS)
            goto fail;
      }
   }
#else
	uint8_t *in = (uint8_t*) data;
   if (in && len) {
      if (len <= CTRL_THRESHOLD) {
         uint32_t msg_len;
         verify(vchi_msg_dequeue(khdispatch_khrn_handle, in, len, &msg_len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) == 0);
      } else
         verify(vchi_bulk_queue_receive(khdispatch_khrn_handle, in, len, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL) == 0);
   }
#endif
   return true;
fail:
   return false;
}

void khdispatch_send_bulk(const void *out, uint32_t len)
{
#ifdef KHRN_USE_VCHIQ
   if (len) {
      if (len <= CTRL_THRESHOLD) {
         VCHIQ_ELEMENT_T element;

         element.data = out;
         element.size = len;

         vchiq_queue_message(khdispatch_khrn_handle, &element, 1);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
      } else {
         vchiq_queue_bulk_transmit(khdispatch_khrn_handle, out, len, NULL);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
         vcos_event_wait(khdispatch_event);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
      }
   }
#else
   if (len) {
      if (len <= CTRL_THRESHOLD) {
         verify(vchi_msg_queue(khdispatch_khrn_handle, out, len, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) == 0);
	  } else {
         verify(vchi_bulk_queue_transmit(khdispatch_khrn_handle, out, len, VCHI_FLAGS_BLOCK_UNTIL_DATA_READ, NULL) == 0);
	  }
   }
#endif
}

#ifdef KHRN_USE_VCHIQ

#ifdef USE_VCHIQ_ARM
// From arm_loader.c:
extern VCHIQ_SERVICE_HANDLE_T khrn_get_khan_handle(uint32_t pid_lo, uint32_t pid_hi);
#endif

void khdispatch_send_async(uint32_t command, uint64_t pid_in, uint32_t sem)
{
   if (sem != KHRN_NO_SEMAPHORE)
   {
#ifndef USE_VCHIQ_ARM      
      uint32_t pid_lo = (uint32_t) pid_in;
      uint32_t pid_hi = (uint32_t) (pid_in >> 32);
      VCHIQ_SERVICE_HANDLE_T khan_handle = khdispatch_khan_handle;

#ifdef USE_VCHIQ_ARM
      // Linux may have multiple VCHIQ connections, so make sure we're talking
      // to the right one.
      khan_handle = khrn_get_khan_handle(pid_lo, pid_hi);
      vcos_assert(khan_handle != 0);
#endif

      uint32_t msg[] = {command, pid_lo, pid_hi, sem};
      VCHIQ_ELEMENT_T element = {msg, sizeof(msg)};

      vchiq_queue_message(khan_handle, &element, 1);/* ignoring return (VCHIQ_SUCCESS/VCHIQ_ERROR) */
   } // if
} // khdispatch_send_async()

#endif
#else

void khdispatch_send_async(uint32_t command, uint64_t pid_in, uint32_t sem)
{
   if (sem != KHRN_NO_SEMAPHORE)
   {
	#ifndef CLIENT_IPC_NO_VCHIQ
      uint32_t pid_lo = (uint32_t) pid_in;
      uint32_t pid_hi = (uint32_t) (pid_in >> 32);

      uint32_t msg[] = {command, pid_lo, pid_hi, sem};
      vcos_verify(vchi_msg_queue(khdispatch_khan_handle, msg, sizeof(msg), VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL) == 0);
	#else
   extern int32_t ipc_server_send_async(uint32_t command, uint64_t pid, uint32_t sem);		
   verify(ipc_server_send_async(command,pid_in,sem) == 0);
	#endif
   } // if
} // khdispatch_send_async()

#endif

/******************************************************************************
rpc call helpers
******************************************************************************/

/*
   in
*/

bool khdispatch_read_in_bulk(void *in, uint32_t len, void **outdata)
{
   *outdata = in;
   if (len == LENGTH_SIGNAL_NULL) {
      *outdata = NULL;
   }
   return khdispatch_recv_bulk(*outdata, len);
}

/*
   out
*/

void khdispatch_write_out_ctrl(const void *out, uint32_t len)
{
   uint32_t message[] = {len};
   khdispatch_send_ctrl2(message, sizeof(message), out, len);
}

void khdispatch_write_out_ctrl_res(const void *out, uint32_t len, uint32_t res)
{
   uint32_t message[] = {len, res};
   khdispatch_send_ctrl2(message, sizeof(message), out, len);
}

void khdispatch_write_out_bulk(const void *out, uint32_t len)
{
   uint32_t message[] = {len};
   khdispatch_send_ctrl(message, sizeof(message));
   khdispatch_send_bulk(out, len);
}

void khdispatch_write_out_bulk_res(const void *out, uint32_t len, uint32_t res)
{
   uint32_t message[] = {len, res};
   khdispatch_send_ctrl(message, sizeof(message));
   khdispatch_send_bulk(out, len);
}

/******************************************************************************
dispatch
******************************************************************************/

extern void gl_dispatch(uint32_t id);
extern void gl_dispatch_11(uint32_t id);
extern void gl_dispatch_20(uint32_t id);
extern void vg_dispatch(uint32_t id);
extern void egl_dispatch(uint32_t id);
extern void wf_dispatch(uint32_t id);

#ifdef KHRN_USE_VCHIQ
int khronos_dispatch(void *message, int length, VCHIQ_SERVICE_HANDLE_T khrn_handle, VCHIQ_SERVICE_HANDLE_T khan_handle, VCHIU_QUEUE_T *queue, VCOS_EVENT_T *event)
#else
int khronos_dispatch(void *message, int length, VCHI_SERVICE_HANDLE_T khrn_handle, VCHI_SERVICE_HANDLE_T khan_handle)
#endif
{
   static int init = 0;
   int quit = 0;
   if ( !init ) {
      init = 1;
      /* todo: handle failure */
      egl_server_startup_hack();
      workspace_handle = mem_alloc(KHDISPATCH_WORKSPACE_SIZE, 32, MEM_FLAG_NO_INIT | MEM_FLAG_L1_NONALLOCATING, "khdispatch_workspace");
      assert(workspace_handle != MEM_INVALID_HANDLE);
   }

   khdispatch_message = (uint32_t *)message;
   khdispatch_position = 0;
   khdispatch_length = length / 4;
   khdispatch_khrn_handle = khrn_handle;
   khdispatch_khan_handle = khan_handle;
#ifdef KHRN_USE_VCHIQ
   khdispatch_queue = queue;
   khdispatch_event = event;
#endif

   while (khdispatch_position < khdispatch_length) {
      uint32_t id = khdispatch_read_uint32();
      vcos_assert(!quit);   /* The try-unload message will only generate a quit if it is the last message in the block */

      switch (GET_BASE_ID(id)) {
      case GLBASE_ID:
         gl_dispatch(id);
         break;
      case GLBASE_ID_11:
         gl_dispatch_11(id);
         break;
      case GLBASE_ID_20:
         gl_dispatch_20(id);
         break;
      case VGBASE_ID:
         vg_dispatch(id);
         break;
      case EGLBASE_ID:
         egl_dispatch(id);
         break;
      case WFBASE_ID:
         //wf_dispatch(id);
         break;
      case KHRNMISC_ID:
         vcos_assert(id == KHRNMISCTRYUNLOAD_ID);  /* The only thing currently in the misc category */
         if (khdispatch_position >= khdispatch_length && egl_server_is_empty()) {
            if (khdispatch_workspace) {
               mem_unlock(workspace_handle);
               khdispatch_workspace = NULL;
            }
            mem_release(workspace_handle);
            khrn_misc_try_unload_impl();
            init = 0;
            quit = 1;

            /* Do not respond to message. The KHRN service will do that after unloading us */
         } else {
            /* Don't shutdown. Either there are objects left or there are still messages to process */
            vcos_assert(!quit);

            khdispatch_write_int(quit);
         }
         break;
      default:
         UNREACHABLE();
         break;
      }
   }

   if (init) {
      if (khdispatch_workspace) {
         mem_unlock(workspace_handle);
         mem_abandon(workspace_handle);
         khdispatch_workspace = NULL;
      }

#if EGL_KHR_sync
      egl_khr_sync_update();
#endif

#if EGL_BRCM_perf_monitor
      egl_brcm_perf_monitor_update();
#endif

      /*
         this serves two purposes:
         - avoid keeping things locked indefinitely
         - ensure that when eglMakeCurrent is called (always first command of a
           message), the states are unlocked
      */

      egl_server_unlock();
   }

   vcos_assert(khdispatch_position == khdispatch_length);
   return quit;
}
