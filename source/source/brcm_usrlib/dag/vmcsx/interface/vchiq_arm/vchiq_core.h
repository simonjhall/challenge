/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Header file for core VCHIQ logic.

==============================================================================*/

#ifndef VCHIQ_CORE_H
#define VCHIQ_CORE_H

#include "vchiq.h"
#include "interface/vcos/vcos_types.h"

#define VCHIQ_CHANNEL_SIZE_LOG2   14
#define VCHIQ_CHANNEL_MASK        (VCHIQ_CHANNEL_SIZE - 1)

vcos_static_assert(VCHIQ_CHANNEL_SIZE == (1 << VCHIQ_CHANNEL_SIZE_LOG2));

#define VCHIQ_NUM_CURRENT_BULKS        32
#define VCHIQ_NUM_SERVICE_BULKS        8

#define VCHIQ_MAX_SERVICES             64

#define VCHIQ_MSG_INVALID              0  // -
#define VCHIQ_MSG_OPEN                 1  // + (srcport, -), fourcc
#define VCHIQ_MSG_OPENACK              2  // + (srcport, dstport)
#define VCHIQ_MSG_CLOSE                3  // + (srcport, dstport)
#define VCHIQ_MSG_DATA                 4  // + (srcport, dstport)
#define VCHIQ_MSG_CONNECT              5  // -

#define VCHIQ_PORT_MAX                 (VCHIQ_MAX_SERVICES - 1)
#define VCHIQ_PORT_FREE                0x10000
#define VCHIQ_PORT_IS_VALID(port)      (port < VCHIQ_PORT_FREE)
#define VCHIQ_MAKE_MSG(type,srcport,dstport)      ((type<<24) | (srcport<<12) | (dstport<<0))
#define VCHIQ_MSG_TYPE(fourcc)         ((unsigned int)fourcc >> 24)
#define VCHIQ_MSG_SRCPORT(fourcc)      (unsigned short)(((unsigned int)fourcc >> 12) & 0xfff)
#define VCHIQ_MSG_DSTPORT(fourcc)      ((unsigned short)fourcc & 0xfff)

/* Ensure the fields are wide enough */
vcos_static_assert(VCHIQ_MSG_SRCPORT(VCHIQ_MAKE_MSG(0,0,VCHIQ_PORT_MAX)) == 0);
vcos_static_assert(VCHIQ_MSG_TYPE(VCHIQ_MAKE_MSG(0,VCHIQ_PORT_MAX,0)) == 0);

#define VCHIQ_FOURCC_INVALID           0x00000000
#define VCHIQ_FOURCC_IS_LEGAL(fourcc)  (fourcc != VCHIQ_FOURCC_INVALID)

enum
{
   VCHIQ_SRVSTATE_FREE,
   VCHIQ_SRVSTATE_HIDDEN,
   VCHIQ_SRVSTATE_LISTENING,
   VCHIQ_SRVSTATE_OPENING,
   VCHIQ_SRVSTATE_OPEN,
   VCHIQ_SRVSTATE_CLOSESENT,
   VCHIQ_SRVSTATE_CLOSING,
   VCHIQ_SRVSTATE_CLOSEWAIT
};

typedef volatile struct {
   unsigned int dstport;
   VCHI_MEM_HANDLE_T handle;
   void *data;
   int size;
   void *userdata;
} VCHIQ_RX_BULK_T;

typedef volatile struct {
   unsigned int dstport;
   VCHI_MEM_HANDLE_T handle;
   const void *data;
   int size;
   void *userdata;
} VCHIQ_TX_BULK_T;

/* Padded synchronisation objects to keep layouts constant across platforms */


#define MAX_MUTEX_SIZE 12
#define MAX_EVENT_SIZE 12

#if defined(__KERNEL__) && defined(__linux__)
vcos_static_assert(sizeof(VCOS_MUTEX_T) <= MAX_MUTEX_SIZE);
vcos_static_assert(sizeof(VCOS_EVENT_T) <= MAX_EVENT_SIZE);
#endif

typedef union local_mutex_union {
   VCOS_MUTEX_T mutex;
   char padding[MAX_MUTEX_SIZE];
} LOCAL_MUTEX_T;

typedef union local_event_union {
   VCOS_EVENT_T event;
   char padding[MAX_EVENT_SIZE];
} LOCAL_EVENT_T;

#ifdef VCHIQ_LOCAL
typedef union local_event_union REMOTE_EVENT_T;
#else
typedef struct remote_event_struct {
   volatile int set_count;
   volatile int clr_count;
   LOCAL_EVENT_T local;
} REMOTE_EVENT_T;
#endif

typedef struct vchiq_state_struct VCHIQ_STATE_T;

typedef struct vchiq_service_struct {
   VCHIQ_SERVICE_BASE_T base;
   volatile int srvstate;
   unsigned int localport;
   unsigned int remoteport;
   int fourcc;
   int terminate;

   VCHIQ_STATE_T *state;
   VCHIQ_INSTANCE_T instance;

   VCHIQ_TX_BULK_T bulks[VCHIQ_NUM_SERVICE_BULKS];

   volatile int remove;
   volatile int process;
   volatile int insert;

   LOCAL_EVENT_T remove_event;
} VCHIQ_SERVICE_T;

typedef struct vchiq_channel_struct {
   struct {
      volatile char data[VCHIQ_CHANNEL_SIZE];

      volatile int remove;
      volatile int process;
      volatile int insert;

      REMOTE_EVENT_T remove_event;

      LOCAL_MUTEX_T mutex;
   } ctrl;

   struct {
      VCHIQ_RX_BULK_T bulks[VCHIQ_NUM_CURRENT_BULKS];

      volatile int remove;
      volatile int process;
      volatile int insert;

      LOCAL_EVENT_T remove_event;

      LOCAL_MUTEX_T mutex;
   } bulk;

   VCHIQ_SERVICE_T services[VCHIQ_MAX_SERVICES];

   REMOTE_EVENT_T trigger;

   volatile int initialised;
} VCHIQ_CHANNEL_T;

struct vchiq_state_struct {
   VCHIQ_CHANNEL_T *local;
   VCHIQ_CHANNEL_T *remote;

   int id;
   int initialised;
   int connected;
   VCOS_EVENT_T connect;      // event indicating connect message received
   VCOS_MUTEX_T mutex;        // mutex protecting services
   VCHIQ_CALLBACK_T connect_callback;
   VCHIQ_INSTANCE_T *instance;

   VCOS_THREAD_T slot_handler_thread;  // slot handler
};

extern void vchiq_init_channel(VCHIQ_CHANNEL_T *channel);

extern void vchiq_init_state(VCHIQ_STATE_T *state, VCHIQ_CHANNEL_T *local, VCHIQ_CHANNEL_T *remote);
extern VCHIQ_STATUS_T vchiq_connect_internal(VCHIQ_STATE_T *state, VCHIQ_INSTANCE_T instance);
extern VCHIQ_SERVICE_T *vchiq_add_service_internal(VCHIQ_STATE_T *state, int fourcc, VCHIQ_CALLBACK_T callback,
                                                   void *userdata, int srvstate, VCHIQ_INSTANCE_T instance);
extern VCHIQ_STATUS_T vchiq_open_service_internal(VCHIQ_SERVICE_T *service);
extern VCHIQ_STATUS_T vchiq_close_service_internal(VCHIQ_SERVICE_T *service);
extern void vchiq_terminate_service_internal(VCHIQ_SERVICE_T *service);

extern VCHIQ_STATUS_T vchiq_copy_from_user(void *dst, const void *src, int size);
extern void vchiq_ring_doorbell(void);
extern void vchiq_copy_bulk_from_host(void *dst, const void *src, int size);
extern void vchiq_copy_bulk_to_host(void *dst, const void *src, int size);
extern void remote_event_pollall(VCHIQ_STATE_T *state);

#endif
