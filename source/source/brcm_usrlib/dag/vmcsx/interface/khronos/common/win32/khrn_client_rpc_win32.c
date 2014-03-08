/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  khronos
Module   :  RPC

FILE DESCRIPTION
Implementation of core API for VCHI over Win32 USB. Calls proxy functions in
interface/usbdk to drive VCHI via shared-memory RPC.
=============================================================================*/

#include "interface/khronos/common/khrn_int_common.h"

#include "interface/khronos/common/khrn_client.h"
#include "interface/khronos/common/khrn_client_rpc.h"

#include "interface/usbdk/khclient/proxy.h"

#include <string.h>

bool khclient_rpc_init(void)
{
   return proxy_init();
}

void rpc_term(void)
{
   proxy_term();
}

static void merge_flush(CLIENT_THREAD_STATE_T *thread, bool receive)
{
   vcos_assert(thread->merge_pos >= CLIENT_MAKE_CURRENT_SIZE);

   /*
      don't transmit just a make current -- in the case that there is only a
      make current in the merge buffer, we have already sent a control message
      for the rpc call (and with it a make current) and own the big lock
   */

   if (thread->merge_pos > CLIENT_MAKE_CURRENT_SIZE) {
      /*
         we only need to grab the mutex if we're just transmitting
         in any other case, we should already be holding the mutex
      */

      if (!receive) {
         proxy_get_mutex();
      }

      proxy_send_ctrl(thread->merge_buffer, thread->merge_pos, thread->high_priority, receive);

      if (!receive) {
         proxy_put_mutex();
      }

      thread->merge_pos = 0;

      client_send_make_current(thread);

      vcos_assert(thread->merge_pos == CLIENT_MAKE_CURRENT_SIZE);
   } else if (receive) {
      proxy_recv_ctrl(thread->high_priority);
   }
}

void rpc_flush(void)
{
   merge_flush(CLIENT_GET_THREAD_STATE(), false);
}

void rpc_high_priority_begin(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(!thread->high_priority);
   merge_flush(thread, false);
   thread->high_priority = true;
}

void rpc_high_priority_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(thread->high_priority);
   merge_flush(thread, false);
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
      merge_flush(thread, false);
   }

   thread->merge_end = thread->merge_pos + len;
}

void rpc_send_ctrl_write(const uint32_t msg[], uint32_t len)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   memcpy(thread->merge_buffer + thread->merge_pos, msg, len);
   thread->merge_pos += rpc_pad_ctrl(len);
   vcos_assert(thread->merge_pos <= MERGE_BUFFER_SIZE);
}

void rpc_send_ctrl_end(void)
{
   CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

   vcos_assert(thread->merge_pos == thread->merge_end);
}

void rpc_send_bulk(const void *in, uint32_t len)
{
   if (in && len) {
      CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

      merge_flush(thread, false);

      memcpy(proxy_send_bulk_begin(rpc_pad_bulk(len), thread->high_priority), in, len);
      proxy_send_bulk_end();
   }
}

void rpc_send_bulk_gather(const void *in, uint32_t len, int32_t stride, uint32_t n)
{
   if (in && len) {
      CLIENT_THREAD_STATE_T *thread = CLIENT_GET_THREAD_STATE();

      merge_flush(thread, false);

      rpc_gather(proxy_send_bulk_begin(rpc_pad_bulk(n * len), thread->high_priority), in, len, stride, n);
      proxy_send_bulk_end();
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
      merge_flush(thread, recv_ctrl);

      if (recv_ctrl) {
         uint32_t ctrl_len;
         uint32_t *ctrl_begin = (uint32_t *)proxy_recv_ctrl_begin(&ctrl_len);
         uint32_t *ctrl = ctrl_begin;
         vcos_assert(ctrl_len == rpc_pad_ctrl(ctrl_len));
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
         proxy_recv_ctrl_end();
      }

      if ((flags & RPC_RECV_FLAG_BULK) && len_io[0]) {
         if (flags & RPC_RECV_FLAG_BULK_SCATTER) {
            rpc_scatter(out, len_io[0], len_io[1], len_io[2], len_io[3], len_io[4], proxy_recv_bulk_begin(rpc_pad_bulk(len_io[2] * len_io[0]), thread->high_priority));
            proxy_recv_bulk_end();
         } else {
            memcpy(out, proxy_recv_bulk_begin(rpc_pad_bulk(len_io[0]), thread->high_priority), len_io[0]);
            proxy_recv_bulk_end();
         }
      }
   }

   return res;
}

void rpc_begin(void)
{
   proxy_get_mutex();
}

void rpc_end(void)
{
   proxy_put_mutex();
}
