/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VMCS_H
#define VMCS_H

#include "vcinclude/common.h"
#include "interface/vcos/vcos.h"

// Default stack size.

#define VMCS_STACK_SIZE 3000

#ifndef VMCS_HOST_APP_STACK_SIZE
#define VMCS_HOST_APP_STACK_SIZE         VMCS_STACK_SIZE
#endif //ifndef VMCS_HOST_APP_STACK_SIZE

#if !defined(_MSC_VER)                     // Visual C doesn't like ERROR
typedef enum err_e
{
   OK, ERROR
} err_t;
#endif //if !defined(_MSC_VER)

typedef uint8_t *address_t;

// All the messages should be listed here.

typedef enum {
   // System messages.
   M_INIT = 1,            // param1, param2: given by call to vmcs_create_task
   M_INIT_DONE,           // param1: non-zero if initialisation unsuccessful
   M_QUIT,                // no parameters
   M_QUIT_DONE,           // no parameters

   // Host interface messages.
   M_CMD_FROM_EMPTY_FIFO, // param1: the interface number that posted the message

   // Data service messages.
   M_SOCK_READY,          // no parameters

   // Touchscreen service messages.
   M_TOUCH,               // param1: ignored, param2: bit field of touch|up|down|left|right

   // Timers.
   M_TIMER,               // param1, param2: as given in vmcs_schedule_timer

   // Gencmd messages.
   M_CMD_EXECUTE,         // param1 points to argument data structure
   M_CMD_DONE,            // param1 gives return code of command

   // Media Player internal messages.
   M_MP_PRELOAD,          // no parameters
   M_MP_SELECTPLAY,       // param1: time (ms) from which to start playing
   M_MP_RECORD,           // param1: filename; param2: zero or pointer to MP_HANDLER_FILE_OPTS_T.
   M_MP_VIEWFINDER,       // no parameters
   M_MP_RECORDSTILL,      // no parameters
   M_MP_PAUSE,            // no parameters
   M_MP_PLAY,             // param1: filename, param2: time in ms to seek to
   M_MP_SEEK,             // param1: time in ms to seek to
   M_MP_STOP,             // no parameters
   M_MP_CLEAR,            // no parameters
   M_MP_RESTORE,          // no parameters
   M_MP_STAB_ON,          // no parameters
   M_MP_STAB_OFF,          // no parameters
   M_MP_STREAMS,          // param1: pointer to source info, param2: array of pointers to stream infos
   M_MP_GET_FRAME,        // param1: play mode (normal/ff/rew/step), param2: desired stream id
   M_MP_FRAME_AVAILABLE,  // param1: frame info, param2 frame data
   M_MP_END,              // param1: zero
   M_MP_ERROR,            // param1: non-zero error code
   M_MP_PAN,              // param1, param2: pan direction
   M_MP_ZOOM,             // param1: zoom factor
   M_MP_INSTALLDRM,       // param1: pointer to structure of replacement file functions
   M_MP_CONTROL,          // param1: control parameter, param2: dependent on param1
   // Media player internal messages related to DRM 
   M_MP_DRM_VIEW,         // param1: current view count, param2: max view count
   M_MP_DRM_PLAY,         // no parameters
   M_MP_DRM_ERROR,        // param1: reason for DRM error (DivX DRM error code)
   M_MP_DIVX_ERROR,       // divx specific error (e.g. video resolution not supported)
  
   M_MP_RESEND,           // param1: seq num for frame to resend

   // CamPlus messages.
   M_FRAME_READY,          // no parameters

   // Ringtone generator
   M_RG_PLAY,
   M_RG_STOP,

   // Message to force applications to redraw their screen areas.
   M_PAINT,

   // Key messages for host request service
   M_KEY,

   // Initiate gencmd from local task
   M_CMD_LOCAL_GENCMD,     // param1: request structure

   // File service messages
   M_SEND_DISK_STATUS,     // param1: disk status

   // General acknowledgement of another message. Parameters are up to the applications
   // in question, but usually param1 may be the message that is being acknowledged.
   M_ACK,
   // these will generally be parameters to M_ACK indicating completion of host read/write
   M_HOSTREQ_READMEM,
   M_HOSTREQ_WRITEMEM,

   // PCM player
   M_PCM_LOAD,             // param1: channel
   M_PCM_PLAY,             // param1: channel
   M_PCM_STOP,             // param1: channel
   M_PCM_SEEK,             // param1: channel
   M_PCM_DONE,             // param1: channel

   M_MP_BUFFERING_TIME,        // param1: fifo_buffering_time, fifo_buffering_time_left, param2: fifo_buffering_space_left
   M_MP_SEEK_ACK,          // param1: real time seeked to (ms)

   // DMB messages
   M_DMB_POWERUP,   // Power up the DMB hardware, set gpios, etc.
   M_DMB_POWERDOWN, // Power down the DMB hardware, clearing state, etc.

   // Editor messages
   M_ED_START,
   M_ED_STOP,

   // NoTA messages
   M_NOTA_EVENTHANDLER,

   // VSIF message
   M_VSIF_REQUEST,

   // VCHI message
   M_VCHI_MESSAGE,

   // Buffer Manager event
   M_BUFMAN_EVENT,

   // Buffer Manager internal messages
   M_BUFMAN_CONVERT_START,
   M_BUFMAN_CONVERT_DONE,

   // Add more messages here.
   // End of list.
   M_END_OF_LIST
} VMCS_MSG_T;

typedef struct {
   int16_t src_task_id;
   uint16_t msg;
   uint32_t param1;
   uint32_t param2;
} VMCS_MSG_DATA_T;

struct VMCS_TASK_DATA_T;
typedef struct VMCS_TASK_DATA_T VMCS_TASK_DATA_T;

// The type for all message handler functions. Returns non-zero if the task is to exit.

typedef int (*VMCS_MESSAGE_HANDLER_T)(int16_t task_id, int16_t src_task_id, uint16_t msg, uint32_t param1, uint32_t param2);

// Initialise the VMCS. Must be called before any of the below functions. Return non-zero
// for failure.

int vmcs_init(void);

void vmcs_done_init(void);

// VMCS startup function. This creates all the fifos and tasks necessary.

void vmcs_main();

// Create a task. Returns task_id (which is >= 0), or one of the return codes below if it fails.
// Posts an M_INIT message to the new task which, if calling_task_id is >= 0, responds with an
// M_INIT_DONE message at the end of its initialisation. If M_INIT_DONE has a non-zero param1, it
// means the initialisation failed, and the new task has terminated itself. param1 and param2
// are the arguments to the M_INIT message.

#define VMCS_NUM_TASKS 12
#define VMCS_TOO_MANY_TASKS -1
#define VMCS_CREATE_TASK_FAILED -2
// This one should be called by applications.
int vmcs_create_task (int16_t calling_task_id,
                      char const *name, int stack_size, int stack_priority,
                      VMCS_MESSAGE_HANDLER_T msg_handler, uint32_t param1, uint32_t param2);
int vmcs_create_task_ex (int16_t calling_task_id,
                         char const *name_in, int stack_size, int stack_priority, int task_priority,
                         VMCS_MESSAGE_HANDLER_T msg_handler, uint32_t param1, uint32_t param2);

// This version is only for the startup code to create an initial VMCS task.
int vmcs_create_task_init (int16_t calling_task_id,
                           char const *name_in, int stack_size, int stack_priority, int task_priorty,
                           VMCS_MESSAGE_HANDLER_T msg_handler, uint32_t param1, uint32_t param2);

int _vmcs_create_task_init (int16_t calling_task_id,
                           char const *name_in, int stack_size, int stack_priority, int task_priorty,
                           VMCS_MESSAGE_HANDLER_T msg_handler, uint32_t param1, uint32_t param2);

// Create a task. Passed in function needs to explicitly retrieve messages.
int vmcs_task_alloc(int16_t calling_task_id, char const *name_in, uint32_t stack_size, uint32_t stack_priority, uint32_t task_priority, void *(*pfn)(VMCS_TASK_DATA_T*), void *cxt, uint32_t param1, uint32_t param2);

// Retrieve user context from the argument to the thread function.
void *vmcs_get_user_context(VMCS_TASK_DATA_T *vmcs_cxt);

// Read a message from the queue
void vmcs_queue_receive(VMCS_TASK_DATA_T *vmcs_cxt, VMCS_MSG_DATA_T *msg);

int16_t vmcs_get_task_id(VMCS_TASK_DATA_T *vmcs_cxt);

// Terminate a task. Return non-zero if it fails. DAP says that it
// will wait for the task to die. If calling_task_id is >= 0, the task
// being deleted responds to the caller with an M_QUIT_DONE message as
// its very last act before terminating.

int vmcs_end_task(int16_t task_id, int16_t calling_task_id);

// From the task_id, return an integer unique to the task which can be used for indexing. For
// example, if 3 tasks are running, the integers returned for the 3 tasks will be 0, 1 and 2.
// This integer does not change for the lifetime of the task.

int vmcs_task_num_from_id(int16_t task_id);

// sets a cookie based on task id, for thread local storage

void vmcs_set_cookie(int16_t task_id, void *cookie);

// gets a cookie based on task id, for thread local storage

void *vmcs_get_cookie(int16_t task_id);

// Queue a message on an application's message queue. Return non-zero if we fail to queue the message.
// Use only from a task.

int vmcs_queue_message(int16_t dest_task_id, int16_t src_task_id, uint16_t msg, uint32_t param1, uint32_t param2);

// Queue a message on an application's message queue. Return non-zero if we fail to queue the message.
// Use only from a HISR.

int vmcs_queue_message_hisr(int16_t dest_task_id, int16_t src_task_id, uint16_t msg, uint32_t param1, uint32_t param2);

// Timers.

typedef struct {
   VCOS_TIMER_T nu_timer;
   uint16_t dest_task_id;
   uint32_t param1;
   uint32_t param2;
   uint32_t nu_created;
} VMCS_TIMER_T;

// Create a VMCS timer. A pointer to an already-allocated VMCS_TIMER_T structure must be provided.

int vmcs_create_timer(VMCS_TIMER_T *timer);

// Ends a VMCS timer. A pointer to an already-allocated VMCS_TIMER_T structure must be provided.

int vmcs_end_timer(VMCS_TIMER_T *timer);

// Schedule the timer to go off after the given time, sending an M_TIMER message to the task
// with id dest_task_id, and the given 2 message parameters. Timers are always one-shot, and
// must be reenabled if necessary after they have gone off. Rescheduling a running timer
// clobbers and restarts it with the new time.

int vmcs_schedule_timer(VMCS_TIMER_T *timer, uint32_t time, uint16_t dest_task_id,
                        uint32_t param1, uint32_t param2);

// Cancel a (running) VMCS timer.

int vmcs_cancel_timer(VMCS_TIMER_T *timer);

// Return non-zero if the task is still running.
int vmcs_task_running (int16_t task_id);

// Queue the last message we will ever send, which should be telling whoever told us to exit that 
// we are about to do so.
int vmcs_queue_last_message(int16_t dest_task_id, int16_t src_task_id, uint16_t msg, uint32_t param1, uint32_t param2);


#endif
