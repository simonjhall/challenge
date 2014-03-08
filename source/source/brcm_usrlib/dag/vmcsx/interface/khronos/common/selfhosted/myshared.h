/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef __MYSHARED_H__
#define __MYSHARED_H__

#include "interface/vchi/os/os.h"

#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/common/khrn_client.h"

//#define __linux__

#define IPC_SHARED_CTRL_SIZE (MERGE_BUFFER_SIZE)
#define IPC_SHARED_BULK_SIZE (KHDISPATCH_WORKSPACE_SIZE)

#define IPC_CLIENT_SIG 0xcccccccc
#define IPC_SERVER_SIG 0x44444444


#define IPC_DATA_STATUS_CTRL_READY     1
#define IPC_DATA_STATUS_BULK_READY     2

typedef struct
{
	//used by ipc_server and khronos_server threads to access this structure
	VCOS_MUTEX_T lock; 

	//used by application thread and khronos_server thread to be sync to access buffer
	//application thread needs to obtain this lock to write to buffer
	//khronos_server would unlock it to signal buffer can be written again
	VCOS_SEMAPHORE_T client_write_lock;
	
	//used by application thread and khronos_server thread to be sync to access buffer
	//khronos_server needs to obtain this lock to write to buffer
	//IPC server/application thread would unlock it to signal buffer can be written again
	VCOS_SEMAPHORE_T server_write_lock;
	
	OS_EVENTGROUP_T c2s_event;
	OS_EVENTGROUP_T s2c_event;

    int client_data_ctrl_available_flag;
    int client_data_bulk_available_flag;
	VCOS_EVENT_FLAGS_T client_data_status;
	
    int server_data_ctrl_available_flag;
    int server_data_bulk_available_flag;
	VCOS_EVENT_FLAGS_T server_data_status;

	void (*khrn_callback)(void * callback_param,VCHI_CALLBACK_REASON_T reason,void * msg_handle);

	unsigned int buffer[5+IPC_SHARED_CTRL_SIZE/4];

	//in buffer
	//buffer[0]: data size, server -> client
	//buffer[1]: data size, client -> server
	//buffer[2]: flag
	//buffer[3]: handle //0xcccccccc, client->server; 0x44444444, server->client
	//buffer[4]: data pointer
	//buffer[5:]: data content, optional 

	//in some instance client can start write bulk data before server release ctrl message
	//so to be safe, seperate bulk and ctrl buffer
	VCOS_SEMAPHORE_T bulk_write_lock;
	unsigned int bulk_buffer[5+IPC_SHARED_BULK_SIZE/4];
	//bulk_buffer[0]: data size, server -> client
	//bulk_buffer[1]: data size, client -> server
	//bulk_buffer[2]: flag
	//bulk_buffer[3]: handle //0xcccccccc, client->server; 0x44444444, server->client
	//bulk_buffer[4]: data pointer
	//bulk_buffer[5:]: data content, optional 
}  IPC_shared_mem;


extern IPC_shared_mem *myIPC;


typedef enum
{
	IPC_VCHI_MSG_QUEUE_EVENT = 0,
	IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT,
    IPC_VCHI_MSG_DEQUEUE_EVENT,
    IPC_VCHI_BULK_RECEIVE_EVENT,
	IPC_VCHI_MSG_HOLD_EVENT,
	IPC_VCHI_MSG_RELEASE_EVENT,
	IPC_VCHI_BULK_WRITE_LOCK_EVENT,
	IPC_VCHI_BULK_WRITE_RELEASE_EVENT,
	IPC_VCHI_CLIENT_CTRL_WRITE_LOCK_EVENT,

} IPC_EVENT_BIT;

//#define RPC_IPC_DEBUG_LOG

#ifdef RPC_IPC_DEBUG_LOG
#define  RPC_IPC_LOG(...) Dbgprintf(34,__VA_ARGS__)
#else
#define  RPC_IPC_LOG(...) 
#endif
#endif


