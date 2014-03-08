/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifndef __IPC_VCHI_H__
#define __IPC_VCHI_H__

#define IPC_SHARED_MEM_SIZE	(KHDISPATCH_WORKSPACE_SIZE)
#define IPC_SHARED_BUF_SIZE	(sizeof(unsigned int)*4 + IPC_SHARED_MEM_SIZE)
// sizeof(sem_t) = 4, ((sizeof(sem_t)*3)+(sizeof(unsigned int)*2)) = 20

typedef enum
{
	IPC_VCHI_NULL_EVENT = 0,
	IPC_VCHI_MSG_QUEUE_EVENT,
	IPC_VCHI_BULK_QUEUE_TRANSMIT_EVENT,
	IPC_VCHI_MSG_DEQUEUE_EVENT,
	IPC_VCHI_BULK_RECEIVE_EVENT,
	IPC_VCHI_MSG_HOLD_EVENT,
	IPC_VCHI_MSG_RELEASE_EVENT,
} IPC_EVENT_BIT;

typedef enum
{
	IPC_SEM_ACCESS = 0,
	IPC_SEM_S2C,
	IPC_SEM_C2S,
	IPC_SEM_RPC,
	IPC_SEM_NUM
} IPC_SEMAPHORE_T;

typedef struct {
	sem_t *m;
	IPC_SEMAPHORE_T name;
	pid_t owner;
	unsigned int count;
} IPC_REENTRANT_MUTEX_T;

typedef struct IPC_shared_mem {
	sem_t ipc_sem[IPC_SEM_NUM];	// sizeof(sem_t) = 4
	IPC_REENTRANT_MUTEX_T ipc_mutex;	// sizeof(IPC_REENTRANT_MUTEX_T) = 16
	unsigned int c2s_events;	// sizeof(unsigned int) = 4
	unsigned int s2c_events;	// 4
	char padding[8];		// padding to 16-byte boundary
	unsigned int buffer[IPC_SHARED_BUF_SIZE/sizeof(unsigned int)];
} IPC_shared_mem_t;

extern void *getBufferMemPointer(void);

#endif // __IPC_VCHI_H__
