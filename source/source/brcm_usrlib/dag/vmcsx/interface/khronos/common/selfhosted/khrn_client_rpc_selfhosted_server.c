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

#include "myshared.h"

static VCHI_SERVICE_HANDLE_T khrn_handle;
static VCHI_SERVICE_HANDLE_T khhn_handle;
static VCHI_SERVICE_HANDLE_T khan_handle;

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

static  VCHI_SERVICE_HANDLE_T  parse_vchi_handle(int32_t priority)
{
	return priority? khhn_handle:khrn_handle;

}


IPC_shared_mem *myIPC;
static void  ipc_vchi_server(unsigned a, void *b)
{
	int32_t success;

	myIPC = (IPC_shared_mem*)vcos_malloc(sizeof(IPC_shared_mem), NULL);
	os_assert(myIPC);

	success = os_semaphore_create( &myIPC->lock, OS_SEMAPHORE_TYPE_SUSPEND );
	os_assert(success==0);
	
	success = os_eventgroup_create( &myIPC->c2s_event, NULL );
	os_assert( success == 0 );

	success = os_eventgroup_create( &myIPC->s2c_event, NULL );
	os_assert( success == 0 );


   while(1)
   {
      // wait for the event to say that there is a message
      uint32_t events;
      int32_t status;
	  VCHI_SERVICE_HANDLE_T handle;
	  int32_t data_size,flags;
	  void *data;
	  int32_t actual_msg_size;
	  VCHI_HELD_MSG_T held_msg,*message;
	  

      status = os_eventgroup_retrieve(&myIPC->c2s_event, &events );
      os_assert(status == 0 );

	  handle    = parse_vchi_handle(myIPC->buffer[0]);
	  data_size = myIPC->buffer[1];
	  flags     = myIPC->buffer[2];
#ifndef __linux__
	  data      = (void*) myIPC->buffer[4];
#else
	  data      = &myIPC->buffer[4];//copy from or to user provided address directly
#endif
	  
	  switch (events)
	  {
	  	case 1<<IPC_VCHI_MSG_QUEUE_EVENT:
	  		vchi_msg_queue(handle,data,data_size,flags,NULL);
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_QUEUE_EVENT);
			break;
		case 1<<IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT:
			vchi_bulk_queue_transmit(handle,data,data_size,flags,NULL);
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_DEQUEUE_EVENT:
			vchi_msg_dequeue(handle,data,data_size,&actual_msg_size,flags);
			myIPC->buffer[1] = actual_msg_size;
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_DEQUEUE_EVENT);
			break;
		case 1<<IPC_VCHI_BULK_RECEIVE_EVENT:
			vchi_bulk_queue_receive(handle,data,data_size,flags,NULL);
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_BULK_RECEIVE_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_HOLD_EVENT:
			vchi_msg_hold(handle,&data,&data_size,flags,&held_msg);
			memcpy(&myIPC->buffer[4],data,data_size);
			myIPC->buffer[1] = data_size;
			//myIPC->buffer[2] = (uint32_t)&held_msg;
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_HOLD_EVENT);
			break;
		case 1<<IPC_VCHI_MSG_RELEASE_EVENT:
			message = (VCHI_HELD_MSG_T*)myIPC->buffer[2];
			//os_assert(message == &held_msg);
			vchi_held_msg_release(&held_msg);
			os_eventgroup_signal(&myIPC->s2c_event,IPC_VCHI_MSG_RELEASE_EVENT);
			break;
		default:
			os_assert(0);
	  }
	  	
   	}


   
}



void vc_vchi_khronos_init(VCHI_INSTANCE_T initialise_instance,
                          VCHI_CONNECTION_T **connections,
                          uint32_t num_connections)
{
   vcos_assert(num_connections == 1);
   VCOS_THREAD_T   mythread;
   const int	   STACK	   = 16*1024;

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


	success = os_thread_start(&mythread, ipc_vchi_server, NULL, STACK, "ipc_vchi_server");
	assert(success==0);
}


static VCHI_SERVICE_HANDLE_T get_vchi_handle(CLIENT_THREAD_STATE_T *thread)
{
   return thread->high_priority ? khhn_handle : khrn_handle;
}



////
