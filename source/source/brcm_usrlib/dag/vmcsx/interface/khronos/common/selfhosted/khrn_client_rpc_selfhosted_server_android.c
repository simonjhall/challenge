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
#include "interface/vchiq/vchiq.h"

#include <assert.h>
#include <string.h>
#include <cutils/log.h>
#include "ipc_vchi.h"

static VCHI_SERVICE_HANDLE_T khrn_handle;
static VCHI_SERVICE_HANDLE_T khhn_handle;
static VCHI_SERVICE_HANDLE_T khan_handle;

static IPC_shared_mem_t *ipc;

static VCHI_SERVICE_HANDLE_T  parse_vchi_handle(int32_t priority)
{
	return priority? khhn_handle:khrn_handle;
}

static int32_t ipc_wait(int32_t *events)
{
	int32_t ipc_events;
	if (events) {
		sem_wait(&ipc->ipc_sem[IPC_SEM_C2S]);
		*events = ipc->c2s_events;
	}
	return 0;
}

static int32_t ipc_signal(int32_t event_mask)
{
	ipc->s2c_events = (1 << event_mask);
	sem_post(&ipc->ipc_sem[IPC_SEM_S2C]);
	return 0;
}

static VCOS_STATUS_T ipc_reentrant_mutex_create(IPC_REENTRANT_MUTEX_T *m, IPC_SEMAPHORE_T name)
{
	m->count = 0;
	m->owner = 0;
	m->m = &ipc->ipc_sem[name];
	m->name = name;
	return VCOS_SUCCESS;
}

static void ipc_reentrant_mutex_delete(IPC_REENTRANT_MUTEX_T *m)
{
	m->count = 0;
	m->owner = 0;
	m->m = NULL;
}

static void ipc_vchi_server(unsigned a, void *b)
{
	int32_t success;

	ipc = (IPC_shared_mem_t*)getBufferPointer();
	os_assert(ipc);

	sem_init(&ipc->ipc_sem[IPC_SEM_ACCESS], 1, 0);
	sem_post(&ipc->ipc_sem[IPC_SEM_ACCESS]);
	sem_init(&ipc->ipc_sem[IPC_SEM_S2C], 1, 0);
	sem_init(&ipc->ipc_sem[IPC_SEM_C2S], 1, 0);
	sem_init(&ipc->ipc_sem[IPC_SEM_RPC], 1, 0);
	sem_post(&ipc->ipc_sem[IPC_SEM_RPC]);
	ipc_reentrant_mutex_create(&ipc->ipc_mutex, IPC_SEM_RPC);
	ipc->c2s_events = IPC_VCHI_NULL_EVENT;
	ipc->s2c_events = IPC_VCHI_NULL_EVENT;

	os_event_signal((OS_EVENT_T *)b);

	while(1) {
		// wait for the event to say that there is a message
		uint32_t events;
		int32_t status;
		VCHI_SERVICE_HANDLE_T handle;
		int32_t data_size,flags;
		void *data;
		int32_t actual_msg_size;
		VCHI_HELD_MSG_T held_msg,*message;

		ipc_wait(&events);

		handle		= parse_vchi_handle(ipc->buffer[0]);
		data_size	= ipc->buffer[1];
		flags		= ipc->buffer[2];
		data		= &ipc->buffer[4];

		switch (events) {
		case 1<<IPC_VCHI_MSG_QUEUE_EVENT:
			vchi_msg_queue(handle,data,data_size,flags,NULL);
			ipc_signal(IPC_VCHI_MSG_QUEUE_EVENT);
			break;
		case 1<<IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT:
			vchi_bulk_queue_transmit(handle,data,data_size,flags,NULL);
			ipc_signal(IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_DEQUEUE_EVENT:
			vchi_msg_dequeue(handle,data,data_size,&actual_msg_size,flags);
			ipc->buffer[1] = actual_msg_size;
			ipc_signal(IPC_VCHI_MSG_DEQUEUE_EVENT);
			break;
		case 1<<IPC_VCHI_BULK_RECEIVE_EVENT:
			vchi_bulk_queue_receive(handle,data,data_size,flags,NULL);
			ipc_signal(IPC_VCHI_BULK_RECEIVE_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_HOLD_EVENT:
			vchi_msg_hold(handle,&data,&data_size,flags,&held_msg);
			memcpy(&ipc->buffer[4],data,data_size);
			ipc->buffer[1] = data_size;
			//ipc->buffer[2] = (uint32_t)&held_msg;
			ipc_signal(IPC_VCHI_MSG_HOLD_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_RELEASE_EVENT:
			message = (VCHI_HELD_MSG_T*)ipc->buffer[2];
			//os_assert(message == &held_msg);
			vchi_held_msg_release(&held_msg);
			ipc_signal(IPC_VCHI_MSG_RELEASE_EVENT);
			break;
		default:
			os_assert(0);
		}
	}
}

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
   {
      VCOS_THREAD_T ipc_vchi_server_thread;
      const int STACK = 16*1024;
      OS_EVENT_T started;
      os_event_create(&started);
      os_thread_start(&ipc_vchi_server_thread, ipc_vchi_server, &started, STACK, "ipc_vchi_server");
      os_event_wait(&started);
      os_event_destroy(&started);
      LOGI("ipc_vchi_server started\n");
   }

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
}

static VCHI_SERVICE_HANDLE_T get_vchi_handle(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority ? khhn_handle : khrn_handle;
}
