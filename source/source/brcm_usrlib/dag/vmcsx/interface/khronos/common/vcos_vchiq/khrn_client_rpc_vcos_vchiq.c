/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  khronos
Module   :  RPC

FILE DESCRIPTION
Implementation of core API for local VCHIQ on VC.
=============================================================================*/

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_int_ids.h"

#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#include "middleware/khronos/dispatch/khrn_dispatch_rpc.h" /* for KHDISPATCH_WORKSPACE_SIZE */

#include "interface/vchiq_arm/vchiq.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

static void *workspace; /* for scatter/gather bulks */
static PLATFORM_MUTEX_T mutex;

#define FOURCC_KHAN VCHIQ_MAKE_FOURCC('K', 'H', 'A', 'N')
#define FOURCC_KHRN VCHIQ_MAKE_FOURCC('K', 'H', 'R', 'N')
#define FOURCC_KHHN VCHIQ_MAKE_FOURCC('K', 'H', 'H', 'N')

static VCHIQ_INSTANCE_T khrn_vchiq_instance;

static VCHIQ_SERVICE_HANDLE_T vchiq_khan_service;
static VCHIQ_SERVICE_HANDLE_T vchiq_khrn_service;
static VCHIQ_SERVICE_HANDLE_T vchiq_khhn_service;

static VCHIU_QUEUE_T khrn_queue;
static VCHIU_QUEUE_T khhn_queue;

static VCOS_EVENT_T bulk_event;

VCHIQ_STATUS_T khrn_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                  VCHIQ_SERVICE_HANDLE_T handle, void *bulk_userdata)
{
   switch (reason) {
   case VCHIQ_MESSAGE_AVAILABLE:
      vchiu_queue_push(&khrn_queue, header);
      break;
   case VCHIQ_BULK_TRANSMIT_DONE:
   case VCHIQ_BULK_RECEIVE_DONE:
      vcos_event_signal(&bulk_event);
      break;
   }

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T khhn_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                  VCHIQ_SERVICE_HANDLE_T handle, void *bulk_userdata)
{
   switch (reason) {
   case VCHIQ_MESSAGE_AVAILABLE:
      vchiu_queue_push(&khhn_queue, header);
      break;
   case VCHIQ_BULK_TRANSMIT_DONE:
   case VCHIQ_BULK_RECEIVE_DONE:
      vcos_event_signal(&bulk_event);
      break;
   }

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T khan_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                  VCHIQ_SERVICE_HANDLE_T handle, void *bulk_userdata)
{
   switch (reason) {
   case VCHIQ_MESSAGE_AVAILABLE:
      vchiq_release_message(handle, header);
      break;
   }

   return VCHIQ_SUCCESS;
}

void vc_vchi_khronos_init()
{
   VCOS_STATUS_T status = vcos_event_create(&bulk_event, NULL);
   assert(status == VCOS_SUCCESS);

   if (vchiq_initialise(&khrn_vchiq_instance) != VCHIQ_SUCCESS)
   {
      printf("* failed to open vchiq device\n");
      exit(1);
   }

   fprintf(stderr, "gldemo: connecting\n");

   if (vchiq_connect(khrn_vchiq_instance) != VCHIQ_SUCCESS)
   {
      printf("* failed to connect\n");
      exit(1);
   }

   if (vchiq_open_service(khrn_vchiq_instance, FOURCC_KHAN, khan_callback, NULL, &vchiq_khan_service) != VCHIQ_SUCCESS ||
       vchiq_open_service(khrn_vchiq_instance, FOURCC_KHRN, khrn_callback, NULL, &vchiq_khrn_service) != VCHIQ_SUCCESS ||
       vchiq_open_service(khrn_vchiq_instance, FOURCC_KHHN, khhn_callback, NULL, &vchiq_khhn_service) != VCHIQ_SUCCESS)
   {
      printf("* failed to add service - already in use?\n");
      exit(1);
   }

   vchiu_queue_init(&khrn_queue, 64);
   vchiu_queue_init(&khhn_queue, 64);

   fprintf(stderr, "gldemo: connected\n");

   /*
      attach to process (there's just one)
   */

//   bool success = client_process_attach();
//   assert(success);
}

bool khclient_rpc_init(void)
{
   workspace = NULL;
   return platform_mutex_create(&mutex) == KHR_SUCCESS;
}

void rpc_term(void)
{
   if (workspace) { khrn_platform_free(workspace); }
   platform_mutex_destroy(&mutex);
}

static VCHIQ_SERVICE_HANDLE_T get_handle(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority ? vchiq_khhn_service : vchiq_khrn_service;
}

static VCHIU_QUEUE_T *get_queue(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority ? &khhn_queue : &khrn_queue;
}

static void check_workspace(uint32_t size)
{
   /* todo: find a better way to handle scatter/gather bulks */
   assert(size <= KHDISPATCH_WORKSPACE_SIZE);
   if (!workspace) {
      workspace = khrn_platform_malloc(KHDISPATCH_WORKSPACE_SIZE, "rpc_workspace");
      assert(workspace);
   }
}

static void merge_flush(CLIENT_THREAD_STATE_T *thread)
{
   assert(thread->merge_pos >= CLIENT_MAKE_CURRENT_SIZE);

   /*
      don't transmit just a make current -- in the case that there is only a
      make current in the merge buffer, we have already sent a control message
      for the rpc call (and with it a make current) and own the big lock
   */

   if (thread->merge_pos > CLIENT_MAKE_CURRENT_SIZE) {
      VCHIQ_ELEMENT_T element;

      element.data = thread->merge_buffer;
      element.size = thread->merge_pos;

      VCHIQ_STATUS_T success = vchiq_queue_message(get_handle(thread), &element, 1);      
      assert(success == VCHIQ_SUCCESS);

      thread->merge_pos = 0;

      client_send_make_current(thread);

      assert(thread->merge_pos == CLIENT_MAKE_CURRENT_SIZE);
   }
}

void rpc_flush(void)
{
   merge_flush(CLIENT_GET_THREAD_STATE());
}

void rpc_high_priority_begin(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   assert(!thread->high_priority);
   merge_flush(thread);
   thread->high_priority = true;
}

void rpc_high_priority_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   assert(thread->high_priority);
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

   assert(len == rpc_pad_ctrl(len));
   if ((thread->merge_pos + len) > MERGE_BUFFER_SIZE) {
      merge_flush(thread);
   }

   thread->merge_end = thread->merge_pos + len;
}

void rpc_send_ctrl_write(const uint32_t in[], uint32_t len) /* len bytes read, rpc_pad_ctrl(len) bytes written */
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   memcpy(thread->merge_buffer + thread->merge_pos, in, len);
   thread->merge_pos += rpc_pad_ctrl(len);
   assert(thread->merge_pos <= MERGE_BUFFER_SIZE);
}

void rpc_send_ctrl_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   assert(thread->merge_pos == thread->merge_end);
}

#define CTRL_THRESHOLD (VCHIQ_CHANNEL_SIZE >> 1)

static void send_bulk(CLIENT_THREAD_STATE_T *thread, const void *in, uint32_t len)
{
   if (len <= CTRL_THRESHOLD) {
      VCHIQ_ELEMENT_T element;

      element.data = in;
      element.size = len;

      VCHIQ_STATUS_T vchiq_status = vchiq_queue_message(get_handle(thread), &element, 1);
      assert(vchiq_status == VCHIQ_SUCCESS);
   } else {
      VCHIQ_STATUS_T vchiq_status = vchiq_queue_bulk_transmit(get_handle(thread), in, rpc_pad_bulk(len), NULL);
      assert(vchiq_status == VCHIQ_SUCCESS);
      VCOS_STATUS_T vcos_status = vcos_event_wait(&bulk_event);
      assert(vcos_status == VCOS_SUCCESS);
   }
}

static void recv_bulk(CLIENT_THREAD_STATE_T *thread, void *out, uint32_t len)
{
   if (len <= CTRL_THRESHOLD) {
      VCHIQ_HEADER_T *header = vchiu_queue_pop(get_queue(thread));
      assert(header->size == len);
      memcpy(out, header->data, len);
      vchiq_release_message(get_handle(thread), header);
   } else {
      VCHIQ_STATUS_T vchiq_status = vchiq_queue_bulk_receive(get_handle(thread), out, rpc_pad_bulk(len), NULL);
      assert(vchiq_status == VCHIQ_SUCCESS);
      VCOS_STATUS_T vcos_status = vcos_event_wait(&bulk_event);
      assert(vcos_status == VCOS_SUCCESS);
   }
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
#if 0
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
#else
   assert(0);
#endif
}

uint32_t rpc_recv(void *out, uint32_t *len_io, RPC_RECV_FLAG_T flags)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();
   uint32_t res = 0;
   uint32_t len;
   bool recv_ctrl;

   if (!len_io) { len_io = &len; }

   recv_ctrl = flags & (RPC_RECV_FLAG_RES | RPC_RECV_FLAG_CTRL | RPC_RECV_FLAG_LEN); /* do we want to receive anything in the control channel at all? */
   assert(recv_ctrl || (flags & RPC_RECV_FLAG_BULK)); /* must receive something... */
   assert(!(flags & RPC_RECV_FLAG_CTRL) || !(flags & RPC_RECV_FLAG_BULK)); /* can't receive user data over both bulk and control... */

   if (recv_ctrl || len_io[0]) { /* do nothing if we're just receiving bulk of length 0 */
      merge_flush(thread);

      if (recv_ctrl) {
         VCHIQ_HEADER_T *header = vchiu_queue_pop(get_queue(thread));
         uint32_t *ctrl = (uint32_t *)header->data;
         assert(header->size == rpc_pad_ctrl(header->size));
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
         assert((uint8_t *)ctrl == ((uint8_t *)header->data + header->size));
         vchiq_release_message(get_handle(thread), header);
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
