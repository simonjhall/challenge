#ifndef VCHIQ_CORE_H
#define VCHIQ_CORE_H

#include "os.h"
#include "interface/vchi/vchi.h"    /* for VCHI_MEM_HANDLE_T */

#if defined(_VIDEOCORE) && !defined(VCHI_LOCAL_HOST_PORT)
#define _VIDEOCORE_MPHI
#endif

#ifdef _VIDEOCORE
#define USE_MEMMGR
#include "vcfw/rtos/common/rtos_common_mem.h"
#endif

#ifdef _VIDEOCORE_MPHI
#include "vcfw/drivers/chip/mphiq.h"
#endif

#ifdef USE_USBHP
#include "tools/usbhp/bcm2727dkb0rev2/hp2727dk/hpapi.h"
#endif

#define VCHIQ_NUM_RX_SLOTS       64
#define VCHIQ_NUM_TX_SLOTS       32
#define VCHIQ_SLOT_SIZE          4096

#define VCHIQ_NUM_RX_BULKS             64
#define VCHIQ_NUM_CURRENT_TX_BULKS     32
#define VCHIQ_NUM_SERVICE_TX_BULKS     8

#define VCHIQ_MAX_SERVICES       32

#define VCHIQ_MAKE_FOURCC(x0, x1, x2, x3)     (((x3) << 24) | ((x2) << 16) | ((x1) << 8) | (x0))

#define VCHIQ_FOURCC_INVALID              0x00000000
#define VCHIQ_FOURCC_HEARTBEAT            VCHIQ_MAKE_FOURCC('B', 'E', 'A', 'T')
#define VCHIQ_FOURCC_TERMINATE            VCHIQ_MAKE_FOURCC('T', 'E', 'R' ,'M')
#define VCHIQ_FOURCC_CONNECT              VCHIQ_MAKE_FOURCC('C', 'O', 'N', 'N')
#define VCHIQ_FOURCC_BULK                 VCHIQ_MAKE_FOURCC('B', 'U', 'L', 'K')

/* TODO:  This suspend/req/ack/fin stuff is a short-term fix for SW-4690 and is to be replaced */
#define VCHIQ_FOURCC_SUSPENDREQ           VCHIQ_MAKE_FOURCC('S', 'U', 'R', 'Q')
#define VCHIQ_FOURCC_SUSPENDACK           VCHIQ_MAKE_FOURCC('S', 'U', 'A', 'K')
#define VCHIQ_FOURCC_SUSPENDFIN           VCHIQ_MAKE_FOURCC('S', 'U', 'F', 'I')

typedef enum {
   VCHIQ_MESSAGE_AVAILABLE,
   VCHIQ_BULK_TRANSMIT_DONE,
   VCHIQ_BULK_RECEIVE_DONE
} VCHIQ_REASON_T;

typedef union {
   struct {
      volatile short slot;
      volatile short mark;
   } s;
   int atomic;
} VCHIQ_ATOMIC_SHORTS_T;

typedef struct {
   char *data;
   int size;            // size of the data in this slot

   int received;        // total messages received into slot
   int released;        // conservative estimate of messages released from slot
} VCHIQ_SLOT_T;

typedef struct {
   const void *data;
   int size;
} VCHIQ_ELEMENT_T;

// VCHIQ_HEADER_T is required to be 12 bytes for compatibility. 
// On 64 bit systems, this requires that packing be set to 32bits.
#pragma pack(push)
#pragma pack(4)

#pragma warning (push)
#pragma warning (disable : 4200)

typedef struct {
   union {
      struct {
    	 int16_t peer;       // peer install position (valid until callback)
    	 int16_t filler;     // Filler bytes to ensure that the size is exactly 12 bytes
    	 int32_t fourcc;     // fourcc of target service
      } s;
      VCHIQ_SLOT_T *slot;  // pointer back to slot (valid during and after callback)
   } u;
   uint32_t size;               // size of message
#ifdef __CC_ARM
   char data[1];           // message
#else
   char data[0];           // message
#endif
} VCHIQ_HEADER_T;

#pragma warning (pop)
#pragma pack(pop)

#ifdef __CC_ARM
extern int vchiq_header_size_check[(sizeof(VCHIQ_HEADER_T) == (12 + 4)) ? 1 : -1]; // we require this for consistency between endpoints
#else
extern int vchiq_header_size_check[(sizeof(VCHIQ_HEADER_T) == 12) ? 1 : -1]; // we require this for consistency between endpoints
#endif

typedef struct {
   VCHIQ_SLOT_T slots[VCHIQ_NUM_RX_SLOTS];

   volatile short peer;             // last value of install sent to peer

   volatile short install;          // next slot to install (first free slot)

   VCHIQ_ATOMIC_SHORTS_T remove;    // next slot to remove and write marker
   VCHIQ_ATOMIC_SHORTS_T process;   // next slot to process and read marker
} VCHIQ_CTRL_RX_STATE_T;

typedef struct {
   VCHIQ_HEADER_T *data;
   unsigned size  : 15;
   signed slot    : 16;
   unsigned term  : 1;
} VCHIQ_TASK_T;

#define VCHIQ_MIN_MSG_SIZE 16
#define VCHIQ_NUM_TX_TASKS (VCHIQ_NUM_TX_SLOTS * VCHIQ_SLOT_SIZE / VCHIQ_MIN_MSG_SIZE)

typedef struct {
   VCHIQ_SLOT_T slots[VCHIQ_NUM_TX_SLOTS];

   volatile short peer;             // next slot to install on peer (conservative estimate)

   VCHIQ_ATOMIC_SHORTS_T fill;      // next slot to fill and write marker

   VCHIQ_TASK_T tasks[VCHIQ_NUM_TX_TASKS];

   volatile short setup;            // next task to set up (first free task)
   volatile short install;          // next task to install
   volatile short remove;           // next task to remove

   OS_EVENT_T software;             // event fired when peer changes
#ifndef _VIDEOCORE_MPHI
   OS_EVENT_T hardware;             // trigger fired when new task installed
#endif
   OS_MUTEX_T mutex;                // mutex protecting from multiple writers
} VCHIQ_CTRL_TX_STATE_T;

typedef struct {
   int fourcc;
   int size;
} VCHIQ_BULK_BODY_T;

typedef struct {
   int fourcc;
#ifdef USE_MEMMGR
   VCHI_MEM_HANDLE_T handle;
#endif
   void *data;
   int size;
   void *userdata;
} VCHIQ_RX_BULK_T;

typedef struct {
   VCHIQ_RX_BULK_T bulks[VCHIQ_NUM_RX_BULKS];

   volatile short setup;            // next bulk to set up
   volatile short install;          // next bulk to install
   volatile short message;          // next bulk to message
   volatile short remove;           // next bulk to remove
   volatile short notify;           // next bulk to notify (dispatch callbacks)

   OS_EVENT_T software;             // event fired when notify changes
   OS_MUTEX_T mutex;                // mutex protecting from multiple writers
} VCHIQ_BULK_RX_STATE_T;

typedef struct {
   int fourcc;
#ifdef USE_MEMMGR
   VCHI_MEM_HANDLE_T handle;
#endif
   const void *data;
   int size;
   void *userdata;
} VCHIQ_TX_BULK_T;

typedef struct {
   VCHIQ_TX_BULK_T bulks[VCHIQ_NUM_CURRENT_TX_BULKS];

   volatile short setup;            // next bulk to set up
   volatile short resolve;          // next bulk to resolve (get address from service queue)
   volatile short install;          // next bulk to install
   volatile short remove;           // next bulk to remove
   volatile short notify;           // next bulk to notify (dispatch callbacks)

#ifndef _VIDEOCORE_MPHI
   OS_EVENT_T hardware;             // trigger fired when new bulk installed
#endif
} VCHIQ_BULK_TX_STATE_T;

typedef int (*VCHIQ_CALLBACK_T)(VCHIQ_REASON_T, VCHIQ_HEADER_T *, void *, void *);

typedef struct {
   int fourcc;

   VCHIQ_CALLBACK_T callback;
   void *userdata;

   VCHIQ_TX_BULK_T bulks[VCHIQ_NUM_SERVICE_TX_BULKS];

   volatile short setup;
   volatile short install;

   OS_EVENT_T software;             // event fired install changes
} VCHIQ_SERVICE_T;

typedef struct {
#ifdef _VIDEOCORE_MPHI
   const MPHI_DRIVER_T *driver;     // MPHI driver function pointer table
   DRIVER_HANDLE_T handle;          // MPHI driver handle
#endif
#ifdef USE_USBHP
   HP_HANDLE_T handle;              // USBHP driver handle
#endif

   VCOS_THREAD_T slot_handler_thread; // slot handler
   VCOS_THREAD_T worker_thread;       // worker (sends heartbeat and bulk messages)
#ifdef USE_USBHP
   VCOS_THREAD_T usb_thread;
#endif
   /* suspend_thread is used for SW-4690 - TODO - consider replacing */
   VCOS_THREAD_T suspend_thread;

   struct {
      VCHIQ_CTRL_RX_STATE_T rx;
      VCHIQ_CTRL_TX_STATE_T tx;
   } ctrl;

   struct {
      VCHIQ_BULK_RX_STATE_T rx;
      VCHIQ_BULK_TX_STATE_T tx;
   } bulk;

   VCHIQ_SERVICE_T services[VCHIQ_MAX_SERVICES];

   OS_EVENT_T connect;              // event indicating connect message received
   OS_EVENT_T trigger;              // event indicating action from slot handler required
   OS_EVENT_T worker;               // event indicating a heartbeat or bulk message is required

   /* Inter-thread communication for SW-4690.  TODO: consider replacing */
   OS_EVENT_T suspend;
   int suspendfin;
   int suspendreq;
   OS_EVENT_T suspendack;

   OS_MUTEX_T mutex;                // mutex protecting services
} VCHIQ_STATE_T;

extern void vchiq_init_state(VCHIQ_STATE_T *state);
extern void vchiq_connect(VCHIQ_STATE_T *state);

extern int vchiq_add_service(VCHIQ_STATE_T *state, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata);

extern void vchiq_queue_message(VCHIQ_STATE_T *state, int fourcc, const VCHIQ_ELEMENT_T *elements, int count);

extern void vchiq_release_message(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header);

extern void vchiq_queue_bulk_transmit(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, const void *data, int size, void *userdata);
extern void vchiq_queue_bulk_receive(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, void *data, int size, void *userdata);

#endif
