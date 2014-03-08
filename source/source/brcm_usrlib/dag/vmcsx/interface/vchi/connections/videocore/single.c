/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $File $
Revision :  $Revision: #4 $

FILE DESCRIPTION:

Implement a 'single fifo' connection driver.

This is a common component that can be attached to a number of different physical
layers (in the first instance MPHI and memory)

=============================================================================*/

#include <stdlib.h>
#include <string.h>

#include "interface/vchi/vchi.h"
#include "interface/vchi/vchi_cfg_internal.h"
#include "interface/vchi/connections/connection.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/control_service/control_service.h"
#include "interface/vchi/control_service/bulk_aux_service.h"
#include "interface/vchi/os/os.h"
#include "interface/vchi/common/multiqueue.h"
#include "interface/vchi/common/endian.h"


/******************************************************************************
Private typedefs
******************************************************************************/

//#define DEBUG_TX_BULK_INUSE
#ifndef VCHI_PUSH_FREE_DESCRIPTORS_ONTO_HEAD
#define vchi_mqueue_put_head           vchi_mqueue_put
#define vchi_mqueue_move_head          vchi_mqueue_move
#define vchi_mqueue_move_element_head  vchi_mqueue_move_element
#endif

// #define os_logging_message(a...) do { ; } while(0)


#define NUMBER_OF_EVENTS 6
// we need to set a group event for bulk rx and tx so we can ensure task safe loading of info into the DMA buffers
#define SINGLE_EVENTBIT_EVENT_PRESENT            0   // a transfer has completed
#define SINGLE_EVENTBIT_RX_BULK_SLOT_READY       1   // there is slot data ready for the rx bulk DMA FIFO
#define SINGLE_EVENTBIT_TX_SLOT_READY            2   // there is slot data ready for the tx bulk DMA FIFO
#define SINGLE_EVENTBIT_TX_BULK_EVENT            3   // a bulk tx request is pending
#define SINGLE_EVENTBIT_RX_SLOT_AVAILABLE_EVENT  4   // a rx msg slot has been freed
#define SINGLE_EVENTBIT_RX_XON_XOFF_EVENT        5

#define SINGLE_EVENTMASK_EVENT_PRESENT            (1 << SINGLE_EVENTBIT_EVENT_PRESENT)
#define SINGLE_EVENTMASK_RX_BULK_SLOT_READY       (1 << SINGLE_EVENTBIT_RX_BULK_SLOT_READY)
#define SINGLE_EVENTMASK_TX_SLOT_READY            (1 << SINGLE_EVENTBIT_TX_SLOT_READY)
#define SINGLE_EVENTMASK_TX_BULK_EVENT            (1 << SINGLE_EVENTBIT_TX_BULK_EVENT)
#define SINGLE_EVENTMASK_RX_SLOT_AVAILABLE_EVENT  (1 << SINGLE_EVENTBIT_RX_SLOT_AVAILABLE_EVENT)
#define SINGLE_EVENTMASK_RX_XON_XOFF_EVENT        (1 << SINGLE_EVENTBIT_RX_XON_XOFF_EVENT)
#define SINGLE_EVENTMASK_ANY                      ((1u << NUMBER_OF_EVENTS) - 1)

// bit mapped record of what has been done with a service
typedef enum {
   SINGLE_UNUSED = 0x0,
   SINGLE_SERVER = 0x1,
   SINGLE_CLIENT = 0x2
} SERVICE_TYPE_T;

typedef struct service_handle SERVICE_HANDLE_T;
typedef struct single_connection_state SINGLE_CONNECTION_STATE_T;

// keep track of peer bulk_receive requests
typedef struct tx_bulk_req {
   struct tx_bulk_req * volatile next;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   struct tx_bulk_req * volatile prev;
#endif
   // Information supplied by them
   fourcc_t service_id;
   uint32_t length;
   MESSAGE_TX_CHANNEL_T channel;
   uint32_t rx_data_bytes;
   uint32_t rx_data_offset;
   // Information calculated by us in single_get_bulk_tx_aligned_section
   const void *tx_data;
   uint32_t tx_data_bytes;
   uint32_t tx_head_bytes;
   uint32_t tx_tail_bytes;
   int32_t  data_shift;
   uint32_t append_dummy_data;
} TX_BULK_REQ_T;


// Message transmit slot information
typedef struct tx_msg_slotinfo {
   struct tx_msg_slotinfo * volatile next;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   struct tx_msg_slotinfo * volatile prev;
#endif

   uint8_t           *addr;               // base address of slot
   uint32_t           len;                // length of slot in bytes

   volatile uint32_t  write_ptr;          // indicates end of data ready to go - may be written by client thread
#define WPTR(w)      ((w) & 0x7FFFFFFF)   // mask within write_ptr - gives actual value of write pointer
#define IS_FILLED(w) ((w) & 0x80000000)   // indicates that slot is filled (write_ptr finalised)
#define FILLED_V(w)  ((w) | 0x80000000)   // permits atomic write to both pointer and fill flag
   uint32_t           read_ptr;           // indicates how much data has been given to hardware
   uint32_t           tx_count;           // count how many transfers from this slot have been given to hardware
   int                queue;              // which queue this slot is currently on
} TX_MSG_SLOTINFO_T;


// Information on a queued message or bulk transmission
typedef struct tx_msginfo_t {
   struct tx_msginfo_t * volatile next;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   struct tx_msginfo_t * volatile prev;
#endif

   OS_SEMAPHORE_T   *blocking;

   void             *addr;
   uint32_t          len;

   SERVICE_HANDLE_T *service;
   void             *handle;
   TX_BULK_REQ_T    *info;
   VCHI_FLAGS_T      flags;

   MESSAGE_TX_CHANNEL_T channel; // message or bulk
} TX_MSGINFO_T;


// information about a single service
struct service_handle {
#ifndef VCHI_COARSE_LOCKING
   // os semaphore use to protect this structure
   OS_SEMAPHORE_T semaphore;
#endif
   fourcc_t service_id;
   SERVICE_TYPE_T type;
   VCHI_CALLBACK_T callback;
   void *callback_param;
   int index;
   SINGLE_CONNECTION_STATE_T *state;
#ifdef VCHI_RX_NANOLOCKS
   OS_COUNT_SEMAPHORE_T rx_semaphore;
#endif
   uint32_t rx_slots_in_use;
   bool_t rx_callback_pending;
   bool_t rx_xoff_sent;
   volatile int32_t tx_xoff_in_progress;
   OS_COND_T tx_xon;
   OS_SEMAPHORE_T connect_semaphore;
   bool_t connected;
   // connection options - "want" settings are those requested by our user...
   bool_t want_crc;
   bool_t want_unaligned_bulk_tx;
   bool_t want_unaligned_bulk_rx;
   // ...whereas these are those in use - our wants ORred with peer's
   // (but note crossover of Rx and Tx when ORring)
   bool_t crc;
   bool_t unaligned_bulk_tx;
   bool_t unaligned_bulk_rx;
   bool_t always_tx_bulk_aux;
};


struct single_connection_state
{
   VCHI_CONNECTION_T *connection;

#ifdef VCHI_MULTIPLE_HANDLER_THREADS
   OS_EVENTGROUP_T slot_event_group;
   VCOS_THREAD_T slot_handling_task;
#endif

#ifndef VCHI_COARSE_LOCKING
   // os semaphore use to protect this structure
   OS_SEMAPHORE_T semaphore;

   OS_SEMAPHORE_T tx_message_queue_semaphore;
   
   OS_SEMAPHORE_T rxq_semaphore;
#elif defined VCHI_RX_NANOLOCKS
   OS_SEMAPHORE_T rxq_semaphore;
#endif

   // message driver function table
   const VCHI_MESSAGE_DRIVER_T *mdriver;
   VCHI_MDRIVER_HANDLE_T *mhandle;

   int highest_service_index;
   SERVICE_HANDLE_T services[ VCHI_MAX_SERVICES_PER_CONNECTION ];
   SERVICE_HANDLE_T *control_service;
   SERVICE_HANDLE_T *bulk_aux_service;

   // our rx slot size = the peer's tx slot size
   uint32_t rx_slot_size;
   // our tx slot size == the peer's rx slot size
   uint32_t tx_slot_size;

   // for messages, we define the queues as follows:
   //   0 = free (all elements start on here initially)
   //   1 = queue for services[0]
   //   2 = queue for services[1]
   //   ...etc
#define MESSAGE_FREE       0
#define MESSAGE_QUEUES     1
#define MESSAGE_SERVICE(n) (MESSAGE_QUEUES+(n))
   VCHI_MQUEUE_T *rx_msg_queue;

   // for receive slots, we define the queues as follows:
   //   0 = free (all elements start on here initially)
   //   1 = inuse (on dma queue, maybe containing unhandled messages)
   //   2 = pending (contain unhandled messages)
#define RX_SLOT_FREE    0
#define RX_SLOT_INUSE   1
#define RX_SLOT_PENDING 2
#define RX_SLOT_QUEUES  3
   VCHI_MQUEUE_T *rx_slots;
   uint8_t *rx_slots_buf;

   // count how many message rx DMA FIFO slots have been added
   int16_t rx_slot_delta;
   uint16_t rx_slot_count;

   // bulk transfers 'this side --> other side'
   //
   // 'this side':  services do ->bulk_queue_transmit
   // 'other side': services do ->bulk_queue_receive
   //
   // information from the other side comes in via the message
   // service.  we need to maintain list of requests from both
   // sides, so we can match them up
#define TX_BULK_REQ_FREE     0
#define TX_BULK_REQ_PENDING  1
#define TX_BULK_REQ_QUEUES   2
   VCHI_MQUEUE_T *tx_bulk_req_queue;

   // tx queues

   // for transmit slots, we define the queues as follows:
   //   0 = free (all elements start on here initially)
   //   1 = pending (filled and waiting for hardware; tail may still be being filled)
   //   2 = in use (filled and on hardware)
#define TX_SLOT_FREE    0
#define TX_SLOT_PENDING 1
#define TX_SLOT_INUSE   2
#define TX_SLOT_QUEUES  3
   VCHI_MQUEUE_T *tx_slots;
   uint8_t *tx_slots_buf;
   OS_COND_T tx_slot_free;

   // the queue of actual client message requests - these are matched against
   // completed/partially-completed slots
#define TX_MSG_FREE    0
#define TX_MSG_INUSE   1
#define TX_MSG_QUEUES  2
   VCHI_MQUEUE_T *tx_msg_queue;

#define TX_BULK_FREE          0
#define TX_BULK_PENDING       1  // queued by queue_transmit, awaiting request from peer
#define TX_BULK_INUSE         2  // data transfer in progress
#define TX_BULK_QUEUES        3
   VCHI_MQUEUE_T *tx_bulk_queue;

   // rx bulk queues
#define RX_BULK_FREE          0
#define RX_BULK_PENDING       1  // waiting to get on to hardware
#define RX_BULK_UNREQUESTED   2  // on hardware, waiting to send bulk transfer rx message
#define RX_BULK_QUEUES        3
#define RX_BULK_REQUESTED(n)  (RX_BULK_QUEUES+(n)-MESSAGE_TX_CHANNEL_BULK)
   // waiting for the data transfer to occur - 1 entry for each real channel
#define RX_BULK_DATA_ARRIVED(n) (RX_BULK_QUEUES+VCHI_MAX_BULK_RX_CHANNELS_PER_CONNECTION+(n))
   // data transfer complete (or skipped), waiting for bulk auxiliary
   // 1 entry for each real channel, and 1 for MESSAGE_TX_CHANNEL_MESSAGE,
   // meaning "bulk aux only"
   VCHI_MQUEUE_T *rx_bulk_queue;

   // peer slot information is used as a flow control mechanism.
   // in particular, we need to always have one slot available to be able
   // to transmit a message reporting how many receive slots have been added
   uint32_t peer_slot_count;
   uint32_t peer_total_msg_space;
   uint32_t min_bulk_size;

   // pump_tx_bulk cycles through channels - this holds where it's up to
   int pump_tx_bulk_next_channel;

   VCHI_CRC_CONTROL_T crc_control;

   int connection_number;
   
   bool_t suspended;

};

#define MISALIGNMENT(ptr, align) ((uintptr_t)(ptr)%(align))
#define IS_ALIGNED(ptr, align) (MISALIGNMENT(ptr,align) == 0)

/******************************************************************************
Static func forwards
******************************************************************************/

// API functions
//routine to init a connection
static VCHI_CONNECTION_STATE_T *single_init( VCHI_CONNECTION_T *connection,
                                             const VCHI_MESSAGE_DRIVER_T *message_driver );

// routine to control CRC enabling at a connection level
static int32_t single_crc_control( VCHI_CONNECTION_STATE_T *state_handle,
                                   VCHI_CRC_CONTROL_T control );

//routine to create a service
static int32_t single_service_connect( VCHI_CONNECTION_STATE_T *state_handle,
                                       fourcc_t service_id,
                                       uint32_t rx_fifo_size,
                                       uint32_t tx_fifo_size,
                                       int server,
                                       VCHI_CALLBACK_T callback,
                                       void *callback_param,
                                       bool_t want_crc,
                                       bool_t want_unaligned_bulk_rx,
                                       bool_t want_unaligned_bulk_tx,
                                       VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle );

//routine to disconnect from a service
static int32_t single_service_disconnect( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle );

//routine to queue a message
static int32_t single_service_queue_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                         const void *data,
                                         uint32_t data_size,
                                         VCHI_FLAGS_T flags,
                                         void *msg_handle );

// scatter-gather (vector) message queueing
static int32_t single_service_queue_msgv( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                          VCHI_MSG_VECTOR_T *vector,
                                          uint32_t count,
                                          VCHI_FLAGS_T flags,
                                          void *msg_handle );

//routine to dequeue a message
static int32_t single_service_dequeue_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                           void *data,
                                           uint32_t max_data_size_to_read,
                                           uint32_t *actual_msg_size,
                                           VCHI_FLAGS_T flags );

//routine to peek at a message
static int32_t single_service_peek_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                        void **data,
                                        uint32_t *msg_size,
                                        VCHI_FLAGS_T flags );

//routine to hold a message
static int32_t single_service_hold_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                        void **data,
                                        uint32_t *msg_size,
                                        VCHI_FLAGS_T flags,
                                        void **msg_handle );

// Routine to initialise a received message iterator
static int32_t single_service_look_ahead_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                              VCHI_MSG_ITER_T *iter,
                                              VCHI_FLAGS_T flags );

//routine to release a message
static int32_t single_held_msg_release( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                        void *msg_handle );

//routine to get info on a held message
static int32_t single_held_msg_info( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                     void *msg_handle,
                                     void **data,
                                     int32_t *msg_size,
                                     uint32_t *tx_timestamp,
                                     uint32_t *rx_timestamp );

// Routine to check whether the iterator has a next message
static bool_t single_msg_iter_has_next( VCHI_CONNECTION_SERVICE_HANDLE_T service,
                                        const VCHI_MSG_ITER_T *iter );

// Routine to advance the iterator
static int32_t single_msg_iter_next( VCHI_CONNECTION_SERVICE_HANDLE_T service,
                                     VCHI_MSG_ITER_T *iter,
                                     void **data,
                                     uint32_t *msg_size );

// Routine to remove the last message returned by the iterator
static int32_t single_msg_iter_remove( VCHI_CONNECTION_SERVICE_HANDLE_T service,
                                       VCHI_MSG_ITER_T *iter );

// Routine to hold the last message returned by the iterator
static int32_t single_msg_iter_hold( VCHI_CONNECTION_SERVICE_HANDLE_T service,
                                     VCHI_MSG_ITER_T *iter,
                                     void **msg_handle );

//routine to transmit bulk data
static int32_t single_bulk_queue_transmit( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                           const void *data_src,
                                           uint32_t data_size,
                                           VCHI_FLAGS_T flags,
                                           void *bulk_handle );

//routine to receive data
static int32_t single_bulk_queue_receive( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                          void *data_dst,
                                          uint32_t data_size,
                                          VCHI_FLAGS_T flags,
                                          void *bulk_handle );

// Routine to report the available servers
static int32_t single_server_present( VCHI_CONNECTION_STATE_T *state,
                                      fourcc_t service_id,
                                      int32_t peer_flags );

// Routine to report the number of RX slots available
static int     single_connection_rx_slots_available( const VCHI_CONNECTION_STATE_T *state );

// Routine to report the RX slot size
static uint32_t single_connection_rx_slot_size( const VCHI_CONNECTION_STATE_T *state );

// routine to be called when a connect is received. Used to check compatability and initialise our count of peer slots
static void    single_connection_info( VCHI_CONNECTION_STATE_T *state_handle,
                                       uint32_t protocol_version,
                                       uint32_t slot_size,
                                       uint32_t num_slots,
                                       uint32_t min_bulk_size );

// routine to be called when a disconnect is received. Used to prepare VC for power off
static void    single_disconnect( VCHI_CONNECTION_STATE_T *state_handle,
                                  uint32_t flags );

// routine to be called when a channel power control request is received
static void    single_power_control( VCHI_CONNECTION_STATE_T *state,
                                     MESSAGE_TX_CHANNEL_T channel,
                                     bool_t enable );

// Callback to indicate that the other side has added a buffer to their rx bulk DMA FIFO
static void    single_rx_bulk_buffer_added( VCHI_CONNECTION_STATE_T *state_handle,
                                            fourcc_t service,
                                            uint32_t length,
                                            MESSAGE_TX_CHANNEL_T channel,
                                            uint32_t channel_params,
                                            uint32_t data_length,
                                            uint32_t data_offset );

// Callback to inform a service that a Xon or Xoff message has been received
static void    single_flow_control( VCHI_CONNECTION_STATE_T *state_handle,
                                    fourcc_t service,
                                    int32_t xoff );

// Callback to inform a service that a server available reply message has been received
static void    single_server_available_reply( VCHI_CONNECTION_STATE_T *state_handle,
                                              fourcc_t service_id,
                                              uint32_t flags );

// Callback to inform a service that bulk auxiliary messages have arrived
static void    single_bulk_aux_received( VCHI_CONNECTION_STATE_T *state_handle );

// Callback to inform a service that a bulk auxiliary message has been transmitted
static void    single_bulk_aux_transmitted( VCHI_CONNECTION_STATE_T *state_handle, void *handle );

// callback functions passed the lower level driver
static void    single_event_completed ( void *state );

// allocate memory suitably aligned for this connection
static void * single_buffer_allocate(const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle, uint32_t *length);

// free memory allocated by buffer_allocate
static void   single_buffer_free(const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle, void *address);

// Local functions
static void    single_init_crc_table( void );
static uint32_t single_compute_crc( const void *ptr, uint32_t len );
static SERVICE_HANDLE_T *single_find_free_service( fourcc_t service_id, SERVICE_TYPE_T client_or_server, SINGLE_CONNECTION_STATE_T *state );
static void    single_event_signal( SINGLE_CONNECTION_STATE_T *state, int event );
static void    single_issue_callback( SERVICE_HANDLE_T *service, VCHI_CALLBACK_REASON_T reason, void *param );
#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
static void    single_issue_msg_available_callbacks ( SINGLE_CONNECTION_STATE_T *state );
#endif
static void    single_pump_transmit( SINGLE_CONNECTION_STATE_T *state );
static int32_t single_pump_bulk_rx_request( SINGLE_CONNECTION_STATE_T *state );
static int32_t single_pump_tx_msg( SINGLE_CONNECTION_STATE_T *state );
static int32_t single_pump_tx_bulk( SINGLE_CONNECTION_STATE_T *state );
static int32_t single_pump_tx_bulk_aux( SINGLE_CONNECTION_STATE_T *state );
static void    single_marshall_tx_bulk( SINGLE_CONNECTION_STATE_T *state, fourcc_t id );
static void    single_tx_bulk_ready( SINGLE_CONNECTION_STATE_T *state, TX_MSGINFO_T *tx_bulk_entry, TX_BULK_REQ_T *tx_bulk_req );
static void    single_pump_rx_bulk( SINGLE_CONNECTION_STATE_T *state );
static void    single_message_received_action( SINGLE_CONNECTION_STATE_T *state, fourcc_t service_id, RX_MSG_SLOTINFO_T *slot, void *addr, uint32_t len, uint32_t tx_timestamp, uint32_t rx_timestamp );
static void    single_rx_slot_completed_action( SINGLE_CONNECTION_STATE_T *state );
static void    slot_handling_func( unsigned argc, void *argv );
static void    single_dequeue_from_slot( RX_MESSAGE_INFO_T *msg, SINGLE_CONNECTION_STATE_T *state, SERVICE_HANDLE_T *service );
static void    single_add_slots_to_hardware( SINGLE_CONNECTION_STATE_T *state );
static bool_t  single_should_send_bulk_aux( const SINGLE_CONNECTION_STATE_T *state, const SERVICE_HANDLE_T *service );
static bool_t  single_expect_bulk_aux( const SINGLE_CONNECTION_STATE_T *state, const SERVICE_HANDLE_T *bulk );
static void    single_get_bulk_rx_aligned_section( const SINGLE_CONNECTION_STATE_T *state, void *addr, uint32_t len, void **addr_out, uint32_t *len_out );
static void    single_bulk_rx_complete( SINGLE_CONNECTION_STATE_T *state, RX_BULK_SLOTINFO_T *s );
static bool_t  single_want_crc( const SERVICE_HANDLE_T *service );
static void    single_process_bulk_aux( SINGLE_CONNECTION_STATE_T *state );
static bool_t  single_process_bulk_aux_for_service( SINGLE_CONNECTION_STATE_T *state, SERVICE_HANDLE_T *service );
static int32_t single_send_xon_xoff_message( SERVICE_HANDLE_T *service );
static int32_t single_send_xon_xoff_messages( SINGLE_CONNECTION_STATE_T *state );
#ifdef DEBUG_TX_BULK_INUSE
static void    single_dump_tx_bulk_inuse( const SINGLE_CONNECTION_STATE_T *state, const char *heading );
#else
#define        single_dump_tx_bulk_inuse( state, heading ) ((void) 0)
#endif
static void    single_prepare_initial_rx( SINGLE_CONNECTION_STATE_T *state );

#ifdef VCHI_COARSE_LOCKING
static void single_lock(SINGLE_CONNECTION_STATE_T *state)
{
   int32_t s = os_semaphore_obtain(&state->connection->sem);
   os_assert(s == 0);
}

static void single_unlock(SINGLE_CONNECTION_STATE_T *state)
{
   int32_t s = os_semaphore_release(&state->connection->sem);
   os_assert(s == 0);
}
#else
#define single_lock(s)          ((void)0)
#define single_unlock(s)        ((void)0)
#endif
#define single_unlock_return(s,r) (single_unlock(s),(r))


/******************************************************************************
Static data
******************************************************************************/

// flags to indicate what procesing is required
#ifndef VCHI_MULTIPLE_HANDLER_THREADS
static OS_EVENTGROUP_T slot_event_group;
static VCOS_THREAD_T slot_handling_task;
#endif

// dummy data for bulk transmission - needed in some unaligned cases
static uint8_t dummy_data[VCHI_BULK_GRANULARITY];
#ifdef __VIDEOCORE__
#pragma align_to(16,dummy_data)
#endif

// call back routine for the lower level driver to signal an event
static VCHI_MESSAGE_DRIVER_OPEN_T message_driver_open_info = { single_event_completed };

static const VCHI_CONNECTION_API_T single_functable =
{
   single_init,
   single_crc_control,
   single_service_connect,
   single_service_disconnect,
   single_service_queue_msg,
   single_service_queue_msgv,
   single_service_dequeue_msg,
   single_service_peek_msg,
   single_service_hold_msg,
   single_service_look_ahead_msg,
   single_held_msg_release,
   single_held_msg_info,
   single_msg_iter_has_next,
   single_msg_iter_next,
   single_msg_iter_remove,
   single_msg_iter_hold,
   single_bulk_queue_transmit,
   single_bulk_queue_receive,
   single_server_present,
   single_connection_rx_slots_available,
   single_connection_rx_slot_size,
   single_rx_bulk_buffer_added,
   single_flow_control,
   single_server_available_reply,
   single_bulk_aux_received,
   single_bulk_aux_transmitted,
   single_connection_info,
   single_disconnect,
   single_power_control,
   single_buffer_allocate,
   single_buffer_free
};

// we need to keep the state information from the different low level connections separate
static OS_SEMAPHORE_T state_semaphore;
static SINGLE_CONNECTION_STATE_T single_state[VCHI_MAX_NUM_CONNECTIONS];
static int32_t num_connections;

// look-up table for bulk CRC calculation
static uint32_t crc_table[256];

/******************************************************************************
Global functions
******************************************************************************/

/* ----------------------------------------------------------------------
 * return a pointer to the 'single' connection driver fops
 * -------------------------------------------------------------------- */
const VCHI_CONNECTION_API_T *
single_get_func_table( void )
{
   return &single_functable;
}


/******************************************************************************
Static functions
******************************************************************************/

/* ----------------------------------------------------------------------
 * register a given message driver with this connection
 * -------------------------------------------------------------------- */
static VCHI_CONNECTION_STATE_T *
single_init( VCHI_CONNECTION_T *connection,
             const VCHI_MESSAGE_DRIVER_T *message_driver )
{
   int32_t success = -1;
   uint32_t i;
   uint32_t length;
   RX_MSG_SLOTINFO_T *s;
   TX_MSG_SLOTINFO_T *t;
   uint8_t *addr;
   SINGLE_CONNECTION_STATE_T *state;
   static int initialised;

   // could get called from more than one thread, need to create semaphores safely
   success = os_semaphore_obtain_global();
   os_assert(success == 0);

   if ( initialised++ == 0 ) {
      int32_t status;

#ifndef VCHI_MULTIPLE_HANDLER_THREADS
      // start the slot handling task.....
      status = os_eventgroup_create( &slot_event_group, "SLOTEVT" );
      os_assert( status == 0 );
      status = os_thread_start( &slot_handling_task, slot_handling_func, NULL, 2000, "SLOT_HANDLE" );
      os_assert( status == 0 );
      status = os_thread_set_priority(&slot_handling_task, 9);
      os_assert( status == 0 );
#endif
      status  = os_semaphore_create(&state_semaphore, OS_SEMAPHORE_TYPE_SUSPEND);
      os_assert( status == 0 );

      single_init_crc_table();
   }

   success = os_semaphore_release_global();
   os_assert(success == 0);

   success = os_semaphore_obtain(&state_semaphore);
   os_assert(success == 0);

   state = &single_state[ num_connections ];

   // zero out our state information
   memset( state, 0, sizeof(SINGLE_CONNECTION_STATE_T) );

   // need to do this early on otherwise the low level stuff will be calling it before it is ready
   state->connection = connection;

#ifdef VCHI_MULTIPLE_HANDLER_THREADS
   // initialise the event group
   success = os_eventgroup_create( &state->slot_event_group, "SLOTEVT" );
   os_assert( success == 0 );
#endif

   // initialise the semaphores
#ifndef VCHI_COARSE_LOCKING
   success =  os_semaphore_create( &state->semaphore, OS_SEMAPHORE_TYPE_SUSPEND );
   success += os_semaphore_create( &state->tx_message_queue_semaphore, OS_SEMAPHORE_TYPE_SUSPEND );
   success += os_cond_create( &state->tx_slot_free, &state->tx_message_queue_semaphore );
   success += os_semaphore_create( &state->rxq_semaphore, OS_SEMAPHORE_TYPE_NANOLOCK );
#else
   success =  os_cond_create( &state->tx_slot_free, &state->connection->sem );
#ifdef VCHI_RX_NANOLOCKS
   success += os_semaphore_create( &state->rxq_semaphore, OS_SEMAPHORE_TYPE_NANOLOCK );
#endif
#endif
   os_assert( success == 0 );

   // need to record the connection number per connection so that we use the right data structures per connection
   for (i=0; i<VCHI_MAX_SERVICES_PER_CONNECTION; i++) {
      state->services[i].state = state;
      state->services[i].index = (int)i;
   }

   // highest service index in use
   state->highest_service_index = -1;

   // record the pointer to the lower level driver
   state->mdriver = message_driver;

#ifdef VCHI_COARSE_LOCKING
#define MQ_SEM &state->connection->sem
#else
#define MQ_SEM NULL
#endif
   // initialise message multiqueue (0 is free, 1 is pending processing, 2.. individual services
   state->rx_msg_queue = vchi_mqueue_create( "messages", MESSAGE_QUEUES + VCHI_MAX_SERVICES_PER_CONNECTION, (VCHI_RX_MSG_QUEUE_SIZE/4), (int) sizeof(RX_MESSAGE_INFO_T), MQ_SEM );

   // initialise slot multiqueues
#ifdef VCHI_COARSE_LOCKING
#ifdef VCHI_RX_NANOLOCKS
   state->rx_slots = vchi_mqueue_create( "rx_slots", RX_SLOT_QUEUES, VCHI_NUM_READ_SLOTS, (int) sizeof(RX_MSG_SLOTINFO_T), &state->rxq_semaphore );
#else
   state->rx_slots = vchi_mqueue_create( "rx_slots", RX_SLOT_QUEUES, VCHI_NUM_READ_SLOTS, (int) sizeof(RX_MSG_SLOTINFO_T), &state->connection->sem );
#endif
#else
   state->rx_slots = vchi_mqueue_create( "rx_slots", RX_SLOT_QUEUES, VCHI_NUM_READ_SLOTS, (int) sizeof(RX_MSG_SLOTINFO_T), NULL );
#endif
   state->tx_slots = vchi_mqueue_create( "tx_slots", TX_SLOT_QUEUES, VCHI_NUM_WRITE_SLOTS, (int) sizeof(TX_MSG_SLOTINFO_T), MQ_SEM );

   // initialise the peer rx bulk queue (ie. what the peer has in its hardware rx bulk queue)
   state->tx_bulk_req_queue = vchi_mqueue_create( "peer_rx_bulk_queue", TX_BULK_REQ_QUEUES, VCHI_MAX_PEER_BULK_REQUESTS, (int) sizeof(TX_BULK_REQ_T), MQ_SEM );

   // tx bulk queues (tracking free, pending, in DMA FIFO)
   state->tx_bulk_queue = vchi_mqueue_create( "tx_bulk_queue", TX_BULK_QUEUES, VCHI_TX_BULK_QUEUE_SIZE, (int) sizeof(TX_MSGINFO_T), MQ_SEM );

   // open the message driver
   state->mhandle = state->mdriver->open( &message_driver_open_info, state );
   os_assert( state->mhandle != NULL );

   // Find out how big a slot has to be be for VCHI_MAX_MSG_SIZE
   state->rx_slot_size = (int32_t) state->mdriver->rx_slot_size( state->mhandle, VCHI_MAX_MSG_SIZE );
   state->tx_slot_size = (int32_t) state->mdriver->tx_slot_size( state->mhandle, VCHI_MAX_MSG_SIZE );

   // now create the underlying memory that the receive slots point to
   //state->driver_info = state->mdriver->get_info();
   length = VCHI_NUM_READ_SLOTS * state->rx_slot_size;
   state->rx_slots_buf = state->mdriver->allocate_buffer( state->mhandle, &length );
   os_assert( length == VCHI_NUM_READ_SLOTS * state->rx_slot_size );

   // make the slots point to this memory
   s = vchi_mqueue_peek( state->rx_slots, RX_SLOT_FREE, 0 );
   addr = state->rx_slots_buf;
   for (i=0; i<VCHI_NUM_READ_SLOTS; i++) {
      s->addr = addr;
      s->len = state->rx_slot_size;
      // all other fields will have been memset to 0 in vchi_mqueue_create
      addr += state->rx_slot_size;
      s->state = state;
      s = s->next;
   }

   // now create the underlying memory that the transmit slots point to
   length = VCHI_NUM_WRITE_SLOTS * state->tx_slot_size;
   state->tx_slots_buf = state->mdriver->allocate_buffer( state->mhandle, &length );
   os_assert( length == VCHI_NUM_WRITE_SLOTS * state->tx_slot_size );

   // make the slots point to this memory
   t = vchi_mqueue_peek( state->tx_slots, TX_SLOT_FREE, 0 );
   addr = state->tx_slots_buf;
   for (i=0; i<VCHI_NUM_WRITE_SLOTS; i++) {
      t->addr = addr;
      t->len = state->tx_slot_size;
      t->queue = TX_SLOT_FREE;
      t->write_ptr = 0;
      t->read_ptr = 0;
      t->tx_count = 0;
      addr += state->tx_slot_size;
      t = t->next;
   }

   // tx msg queues (tracking free, pending, in DMA FIFO)
   state->tx_msg_queue = vchi_mqueue_create( "tx_msg_queue", TX_MSG_QUEUES, VCHI_TX_MSG_QUEUE_SIZE, sizeof(TX_MSGINFO_T), MQ_SEM );

   // Receive bulk queues (tracking free, pending, in DMA FIFO)
   state->rx_bulk_queue = vchi_mqueue_create( "rx_bulk_queue", RX_BULK_QUEUES + 2 * VCHI_MAX_BULK_RX_CHANNELS_PER_CONNECTION + 1, VCHI_RX_BULK_QUEUE_SIZE, sizeof(RX_BULK_SLOTINFO_T), MQ_SEM );

   state->connection_number = num_connections;

   state->crc_control = VCHI_CRC_PER_SERVICE;

   single_prepare_initial_rx(state);

   state->pump_tx_bulk_next_channel = 0;

   // another connection is valid
   num_connections++;
   os_assert( num_connections < VCHI_MAX_NUM_CONNECTIONS );
   
#ifdef VCHI_MULTIPLE_HANDLER_THREADS
   success = os_thread_start( &state->slot_handling_task, slot_handling_func, state, 2000, "SLOT_HANDLE" );
   os_assert( success == 0 );
   success = os_thread_set_priority( &state->slot_handling_task, 9);
   os_assert( success == 0 );
#endif


   success = os_semaphore_release(&state_semaphore);
   os_assert(success == 0);

#undef MQ_SEM

   // return a pointer to the state info
   return (VCHI_CONNECTION_STATE_T *) state;
}

static void single_prepare_initial_rx( SINGLE_CONNECTION_STATE_T *state )
{
   // add as many slots as we can to message driver
   state->rx_slot_count = 0;
   single_add_slots_to_hardware( state );
   // We want to report the initial slot count via the "connect" message
   // payload, rather than the normal header delta field. We achieve this
   // by resetting "rx_slot_delta" after the initial push.
   state->rx_slot_delta = 0;

   // we need to set these up so that we can send the first connect message
   // their value will be corrected once the connect message is received
   state->peer_slot_count = 3; // need to assume that there are some slots so we can send first message
   
   // now the slots are set up we can turn on the hardware
   (void) state->mdriver->enable( state->mhandle );
}


/* ----------------------------------------------------------------------
 * set connection-level CRC control
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_crc_control( VCHI_CONNECTION_STATE_T *state_handle,
                    VCHI_CRC_CONTROL_T control )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;

   switch (control)
   {
   case VCHI_CRC_EVERYTHING:
   case VCHI_CRC_NOTHING:
   case VCHI_CRC_PER_SERVICE:
      state->crc_control = control;
      return 0;
   default:
      return -1;
   }
}

/* ----------------------------------------------------------------------
 * do we want CRC checks on this service? Depends on global and per-
 * service setting.
 * -------------------------------------------------------------------- */
static bool_t
single_want_crc( const SERVICE_HANDLE_T *service )
{
   SINGLE_CONNECTION_STATE_T *state = service->state;
   return state->crc_control == VCHI_CRC_EVERYTHING ? VC_TRUE :
          state->crc_control == VCHI_CRC_NOTHING    ? VC_FALSE :
                                                      service->want_crc;
}

/* ----------------------------------------------------------------------
 * initialise the CRC lookup table
 * -------------------------------------------------------------------- */
static void
single_init_crc_table( void )
{
   uint32_t i, j;
   for (i=0; i<256; i++)
   {
      uint32_t c = i;
      for (j=0; j<8; j++)
      {
         if (c & 1)
            c = 0x82F63B78U ^ (c >> 1);
         else
            c >>= 1;
      }
      crc_table[i] = c;
   }
}

/* ----------------------------------------------------------------------
 * calculate the CRC of a block of data
 * -------------------------------------------------------------------- */
static uint32_t
single_compute_crc( const void *ptr, uint32_t len )
{
   const uint8_t *p = ptr;
   uint32_t c = 0xFFFFFFFFU;
   while (len--)
      c = crc_table[(c ^ *p++) & 0xFF] ^ (c >> 8);

   return c;
}

/* ----------------------------------------------------------------------
 * map a service ID to a service handle
 *
 * returns: the service handle, or NULL if not found
 * -------------------------------------------------------------------- */
static SERVICE_HANDLE_T *
single_fourcc_to_service( const SINGLE_CONNECTION_STATE_T *state,
                          fourcc_t id )
{
   int i;

   for (i=0; i <= state->highest_service_index; i++)
      if (state->services[i].type != SINGLE_UNUSED && state->services[i].service_id == id)
         return (SERVICE_HANDLE_T *) &state->services[i];

   return NULL;
}

/* ----------------------------------------------------------------------
 * associate a service (client or server) with the given message driver.
 * return a handle to the message_driver:service pair in 'service_info'
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_service_connect( VCHI_CONNECTION_STATE_T * state_handle,
                        fourcc_t service_id,
                        uint32_t rx_fifo_size,
                        uint32_t tx_fifo_size,
                        int server,
                        VCHI_CALLBACK_T callback,
                        void *callback_param,
                        bool_t want_crc,
                        bool_t want_unaligned_bulk_rx,
                        bool_t want_unaligned_bulk_tx,
                        VCHI_CONNECTION_SERVICE_HANDLE_T *service_info )
{
   int32_t success = 0;
   SERVICE_HANDLE_T *service;
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;

   // grab the semaphore
#ifndef VCHI_COARSE_LOCKING
   (void) os_semaphore_obtain(&state->semaphore);
#endif

   // this is where we record the service id and associated callback
   service = single_find_free_service( service_id, server ? SINGLE_SERVER : SINGLE_CLIENT, state );

   if(service)
   {
      // for some connections we will need to allocate FIFOs here
      // ........
      // record the service ID
      service->service_id = service_id;
      // record the callback
      service->callback = callback;
      // and the callback parameters
      service->callback_param = callback_param;
      // and the connection options
      service->want_crc = want_crc;
      service->want_unaligned_bulk_rx = want_unaligned_bulk_rx;
      service->want_unaligned_bulk_tx = want_unaligned_bulk_tx;

      service->tx_xoff_in_progress = VC_FALSE;
      service->rx_xoff_sent = VC_FALSE;
      service->rx_callback_pending = VC_FALSE;
#ifndef VCHI_COARSE_LOCKING
      success = os_semaphore_create(&service->semaphore, OS_SEMAPHORE_TYPE_SUSPEND);
      os_assert(success == 0);
      success = os_cond_create(&service->tx_xon, &service->semaphore);
#else
      success = os_cond_create(&service->tx_xon, &state->connection->sem);
#endif
      os_assert(success == 0);
#ifdef VCHI_RX_NANOLOCKS
      success = os_count_semaphore_create( &service->rx_semaphore, 0, OS_SEMAPHORE_TYPE_SUSPEND );
      os_assert(success == 0);
#endif
      success += os_semaphore_create( &service->connect_semaphore, OS_SEMAPHORE_TYPE_SUSPEND );
      os_assert(success == 0);

      if (server)
      {
         service->type = SINGLE_SERVER;
         service->connected = VC_TRUE;
         // Initialise these with something - will update when we get a SERVER_AVAILABLE message
         service->crc = single_want_crc(service);
         service->unaligned_bulk_rx = service->want_unaligned_bulk_rx;
         service->unaligned_bulk_tx = service->want_unaligned_bulk_tx;
         service->always_tx_bulk_aux = VC_FALSE;
         if(service_id == MAKE_FOURCC("CTRL"))
         {
            os_assert(state->control_service == NULL);
            state->control_service = service;
         }
         else if(service_id == MAKE_FOURCC("BULX"))
         {
            os_assert(state->bulk_aux_service == NULL);
            state->bulk_aux_service = service;
         }
#ifndef VCHI_COARSE_LOCKING
         (void) os_semaphore_release(&state->semaphore);
#endif

         // all fine
         success = 0;
      }
      else
      {
         // check server exists at other end
         uint32_t flags = (single_want_crc(service)        ? WANT_CRC : 0) |
                          (service->want_unaligned_bulk_rx ? WANT_UNALIGNED_BULK_RX : 0) |
                          (service->want_unaligned_bulk_tx ? WANT_UNALIGNED_BULK_TX : 0);
         os_logging_message("flags=%x, state->crc_control=%d", flags, state->crc_control);
         service->type = SINGLE_CLIENT;
         service->connected = VC_FALSE;
#ifndef VCHI_COARSE_LOCKING
         (void) os_semaphore_release(&state->semaphore);
#endif
         (void) os_semaphore_obtain(&service->connect_semaphore);
         success = vchi_control_queue_server_available( state->control_service->callback_param,
                                                        service->service_id,
                                                        flags );
         if (success == 0)
         {
            single_unlock(state);
            // wait for single_server_available_reply to get the message and signal this semaphore
            success = os_semaphore_obtain(&service->connect_semaphore);
            os_assert(success == 0);
            single_lock(state);
            if (!service->connected)
               success = -2;
         }
      }

      if (success == 0)
      {
         // return a pointer to the information about the service
         *service_info = (VCHI_CONNECTION_SERVICE_HANDLE_T)service;
      }
   }

   // kill service if something went wrong
   if (success != 0 && service)
      (void) single_service_disconnect((VCHI_CONNECTION_SERVICE_HANDLE_T)service);

   return success;
}


/* ----------------------------------------------------------------------
 * disconnect the given message_driver:service association
 *
 * I think this is for doing suspend/resume?  So, probably this
 * should be saving some state, some sort of /etc/shutdown, suspend-
 * to-ram, flush any outgoing slots?
 *
 * FIXME: am I understanding this correct?
 * -------------------------------------------------------------------- */
static int32_t
single_service_disconnect( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   int32_t success = 0;

#ifndef VCHI_COARSE_LOCKING
   // grab the semaphore
   (void) os_semaphore_obtain(&state->semaphore);
#endif

   (void) os_cond_destroy(&service->tx_xon);
   (void) os_semaphore_destroy(&service->connect_semaphore);
#if !defined VCHI_COARSE_LOCKING
   (void) os_semaphore_destroy(&service->semaphore);
#endif
#ifdef VCHI_RX_NANOLOCKS
   (void) os_count_semaphore_destroy( &service->rx_semaphore );
#endif

   // remove the service id from our list
   service->type = SINGLE_UNUSED;
   service->service_id = 0;

   // figure out if highest_service_index wants to decrease
   while (state->highest_service_index >= 0 && state->services[state->highest_service_index].type == SINGLE_UNUSED)
      state->highest_service_index--;

#ifndef VCHI_COARSE_LOCKING
   // release the semaphore
   (void) os_semaphore_release(&state->semaphore);
#endif

   return success;
}


/* ----------------------------------------------------------------------
 * issue a client callback
 *
 * Must only be called from slot handling task.
 * -------------------------------------------------------------------- */
static void
single_issue_callback( SERVICE_HANDLE_T *service,
                       VCHI_CALLBACK_REASON_T reason,
                       void *param )
{
   if (service->callback)
   {
      single_unlock( service->state );
      service->callback( service->callback_param, reason, param );
      single_lock( service->state );
   }
}

#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
/* ----------------------------------------------------------------------
 * issue message available callbacks
 *
 * Must only be called from slot handling task.
 * -------------------------------------------------------------------- */
static void
single_issue_msg_available_callbacks( SINGLE_CONNECTION_STATE_T *state )
{
   int i;
   
   for (i=0; i <= state->highest_service_index; i++)
      if (state->services[i].type != SINGLE_UNUSED && state->services[i].rx_callback_pending)
      {
         state->services[i].rx_callback_pending = VC_FALSE;
         single_issue_callback( &state->services[i], VCHI_CALLBACK_MSG_AVAILABLE, NULL );
      }
}
#endif

/* ----------------------------------------------------------------------
 * terminate the current tx slot.
 *
 * Called from client threads with tx_message_queue_semaphore obtained.
 * -------------------------------------------------------------------- */
static void
single_terminate_current_tx_slot( SINGLE_CONNECTION_STATE_T *state,
                                  uint32_t wptr,
                                  int dummy_terminator )
{
   TX_MSG_SLOTINFO_T *slot = vchi_mqueue_peek_tail( state->tx_slots, TX_SLOT_PENDING, 0 );
   os_assert( slot && !IS_FILLED(slot->write_ptr) );

   os_logging_message("terminate slot %08x(%d)", slot->addr, dummy_terminator);

   if ( dummy_terminator &&
        state->mdriver->tx_supports_terminate( state->mhandle, MESSAGE_TX_CHANNEL_MESSAGE ))
   {
      // We're terminating an in-progress slot, and we may have already transmitted all
      // existing data [should we check for this specifically?]. Stick a dummy bit of
      // extra payload on that will carry the "terminate" flag.
      int32_t new_length;
      new_length = state->mdriver->form_message( state->mhandle,
                                                 state->control_service->service_id,
                                                 0,
                                                 0,
                                                 slot->addr + wptr,
                                                 slot->len - wptr,
                                                 state->tx_slot_size,
                                                 VC_FALSE,
                                                 VC_FALSE );
      os_assert(new_length >= 0);
      slot->write_ptr = FILLED_V(wptr + new_length);
   }
   else
   {
      // mustn't just set the filled flag without moving the write pointer if the driver has
      // to send terminate messages - we have to actually transmit something to terminate, so
      // the addition of terminating message and setting fill flag must be atomic
      os_assert (wptr != WPTR(slot->write_ptr) ||
                 !state->mdriver->tx_supports_terminate( state->mhandle, MESSAGE_TX_CHANNEL_MESSAGE ));
      slot->write_ptr = FILLED_V(wptr);
   }
}

/* ----------------------------------------------------------------------
 * ensure we have a current tx slot, waiting if necessary.
 *
 * called from client threads with tx_message_queue_semaphore obtained;
 * releases tx_message_queue_semaphore while blocking.
 *
 * returns: the current tx slot, or NULL if not available
 * -------------------------------------------------------------------- */
static TX_MSG_SLOTINFO_T *
single_get_tx_slot( SINGLE_CONNECTION_STATE_T *state, int block )
{
   TX_MSG_SLOTINFO_T *slot = NULL;
   // pick up either the current slot, or if no current slot, make a new one current

   for (;;)
   {
      slot = vchi_mqueue_peek_tail( state->tx_slots, TX_SLOT_PENDING, 0 );
      if ( slot && !IS_FILLED(slot->write_ptr) )
         break;

      // we can't use the blocking form of vchi_mqueue_get, as it would block with
      // tx_message_queue_semaphore still obtained. This isn't acceptable, as that
      // would lead to other, non-blocking calls to queue_msgv blocking behind this
      // operation.
      slot = vchi_mqueue_get( state->tx_slots, TX_SLOT_FREE, 0 );
      if ( slot )
      {
         // ensure everything is correct before it goes on the pending list -
         // slot-handling thread will start monitoring it *immediately*
         slot->write_ptr = 0;
         slot->read_ptr = 0;
         slot->tx_count = 0;
         slot->queue = TX_SLOT_PENDING;
         vchi_mqueue_put( state->tx_slots, TX_SLOT_PENDING, slot );
         break;
      }

      if ( !block )
         break;

#ifdef VCHI_COARSE_LOCKING
      (void) os_cond_wait( &state->tx_slot_free, &state->connection->sem );
#else
      (void) os_cond_wait( &state->tx_slot_free, &state->tx_message_queue_semaphore );
#endif
   }

   return slot;
}

/* ----------------------------------------------------------------------
 * return a slot to the free queue, and wake any waiters
 *
 * called from slot-handling thread only
 *
 * returns: the current tx slot, or NULL if not available
 * -------------------------------------------------------------------- */
void
single_return_free_tx_slot( SINGLE_CONNECTION_STATE_T *state, TX_MSG_SLOTINFO_T *slot )
{
   slot->queue = TX_SLOT_FREE;
   vchi_mqueue_put_head( state->tx_slots, TX_SLOT_FREE, slot );

#ifdef VCHI_COARSE_LOCKING
   (void) os_cond_broadcast( &state->tx_slot_free, VC_TRUE );
#else
   (void) os_cond_broadcast( &state->tx_slot_free, VC_FALSE );
#endif
}

/* ----------------------------------------------------------------------
 * queue a message for transmission to the other side
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_service_queue_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                          const void * const data,
                          uint32_t data_size,
                          VCHI_FLAGS_T flags,
                          void * msg_handle )
{
   // fake this as a single vector queue
   VCHI_MSG_VECTOR_T vec;
   vec.vec_base = data;
   vec.vec_len = data_size;

   return single_service_queue_msgv( service_handle,
                                     &vec,
                                     1,
                                     flags,
                                     msg_handle );
}

/* ----------------------------------------------------------------------
 * scatter-gather (vector) and queue a message for
 * transmission to the other side
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_service_queue_msgv( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                           VCHI_MSG_VECTOR_T *vector,
                           uint32_t count,
                           VCHI_FLAGS_T flags,
                           void *msg_handle )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   TX_MSG_SLOTINFO_T *slot;
   OS_SEMAPHORE_T blocking_sem;
   int32_t new_length = 0;
#ifdef VCHI_MINIMISE_TX_MSG_DESCRIPTORS
   const int need_msg = flags & (VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE|VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE) ? 1 : 0;
#else
   const int need_msg = 1;
#endif
   const int block = flags & (VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? 1 : 0;
   TX_MSGINFO_T *entry = NULL;

   if (flags & (VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   if (state->suspended && service->service_id != MAKE_FOURCC("CTRL"))
   {
      os_assert(0);
      return -1;
   }
   
   os_logging_message("Tx for %c%c%c%c",FOURCC_TO_CHAR(service->service_id));
   
#ifdef VCHI_COARSE_LOCKING
      while( service->tx_xoff_in_progress && block ) {
         os_logging_message("Blocking due to XOFF");
         os_cond_wait( &service->tx_xon, &state->connection->sem );
      }
      if ( service->tx_xoff_in_progress ) {
         os_logging_message("Returning due to XOFF");
         return -1;
      }
#else
   // handle the case where there has been sent an Xoff request
   // initially, a quick sample of the variable will do, otherwise do the full
   // semaphore semantics that condvars need
   if ( service->tx_xoff_in_progress ) {
      if ( !block ) {
         os_logging_message("Returning due to XOFF");
         return -1;
      }

      (void) os_semaphore_obtain( &service->semaphore );
      while( service->tx_xoff_in_progress ) {
         os_logging_message("Blocking due to XOFF");
         (void) os_cond_wait( &service->tx_xon, &service->semaphore );
      }
      (void) os_semaphore_release( &service->semaphore );
   }
#endif

   // find a free entry in the outgoing message queue
   if ( need_msg ) {
      entry = vchi_mqueue_get( state->tx_msg_queue, TX_MSG_FREE, block );
      if ( !entry )
         return -1;
   }

#ifndef VCHI_COARSE_LOCKING
   // the formation of the message and adding to the queue must be atomic
   (void) os_semaphore_obtain( &state->tx_message_queue_semaphore );
#endif

   do
   {
      // pick up either the current slot, or if no current slot, make a new one current
      slot = single_get_tx_slot( state, block );

      if ( !slot )
      {
         // return the message entry to 'free' queue
         if (entry)
            vchi_mqueue_put_head( state->tx_msg_queue, TX_MSG_FREE, entry );
#ifndef VCHI_COARSE_LOCKING
         (void) os_semaphore_release( &state->tx_message_queue_semaphore );
#endif
         return -1;
      }

      if ( slot )
      {
         os_logging_message("qv: attempting form, slot %08x, addr %08x, rem %x", slot->addr, slot->addr + slot->write_ptr, slot->len - slot->write_ptr);
         new_length = state->mdriver->form_message( state->mhandle,
                                                    service->service_id,
                                                    vector,
                                                    count,
                                                    slot->addr + slot->write_ptr,
                                                    slot->len - slot->write_ptr,
                                                    state->tx_slot_size,
                                                    flags & VCHI_FLAGS_ALIGN_SLOT ? VC_TRUE : VC_FALSE,
                                                    flags & VCHI_FLAGS_ALLOW_PARTIAL ? VC_TRUE : VC_FALSE );
         // returns negative if absolutely too big - message won't fit in a slot.
         // returns 0 if message doesn't fit in this slot - must always work on second try.
         if (new_length == 0)
         {
            // doesn't fit in current slot - terminate it
            single_terminate_current_tx_slot( state, slot->write_ptr, 1 );
         }
         else
            os_logging_message("qv: form successful, len %08x, tail %08x", new_length, slot->addr + slot->write_ptr + new_length);
      }
   } while ( new_length == 0 );

   if ( new_length <= 0 ) {
      // couldn't form message
      os_assert( new_length == 0 ); // message larger than slot size?
      os_assert( !block );
      // return the entry to 'free' queue
      if (entry)
         vchi_mqueue_put_head( state->tx_msg_queue, TX_MSG_FREE, entry );
#ifndef VCHI_COARSE_LOCKING
      (void) os_semaphore_release( &state->tx_message_queue_semaphore );
#else
      return -1;
#endif
   }

   if (entry) {
      // fill in the rest of the structure
      entry->addr    = slot->addr + slot->write_ptr;
      entry->len     = new_length;
      entry->channel = MESSAGE_TX_CHANNEL_MESSAGE;
      entry->handle  = msg_handle;
      entry->flags   = flags;
      entry->service = service;
      entry->info    = NULL;
      // setup for blocking behaviour if required
      if ( flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE ) {
         (void) os_semaphore_create( &blocking_sem, OS_SEMAPHORE_TYPE_SUSPEND );
         (void) os_semaphore_obtain( &blocking_sem );
         entry->blocking = &blocking_sem;
      }
      // add to the in-use queue - do this before we advance the write pointer, to
      // ensure it's there to be matched before the write completes.
      vchi_mqueue_put( state->tx_msg_queue, TX_MSG_INUSE, entry );
      
      // we can no longer refer to entry; it may have been put back onto the free queue and
      // re-used by another task
      entry = NULL;
   }

   // advance the write pointer, atomically - slot_handling func could transmit immediately.
   if ( (flags & VCHI_FLAGS_ALIGN_SLOT) || slot->write_ptr + new_length == slot->len )
      single_terminate_current_tx_slot( state, slot->write_ptr + new_length, 0 );
   else
      slot->write_ptr += new_length;

#ifndef VCHI_COARSE_LOCKING
   // now safe for another thread to form a message
   (void) os_semaphore_release( &state->tx_message_queue_semaphore );
#endif

   // alert the slot handling task that there is another message queued
   single_event_signal( state, SINGLE_EVENTBIT_TX_SLOT_READY );

   // wait for operation to complete (if requested)
   if ( flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE ) {
      single_unlock( state );
      (void) os_semaphore_obtain( &blocking_sem );
#ifndef VCHI_ELIDE_BLOCK_EXIT_LOCK
      single_lock( state ); // would be nice to short-circuit this - vchi.c will unlock again immediately
#endif
      (void) os_semaphore_destroy( &blocking_sem );
   }

   return 0;
}

/*----------------------------------------------------------------------
 * get the first pending received message on the given service
 * (from the given message driver)
 *
 * -------------------------------------------------------------------- */
static RX_MESSAGE_INFO_T *
single_get_msg( SINGLE_CONNECTION_STATE_T *state,
                SERVICE_HANDLE_T *service,
                bool_t block )
{
   RX_MESSAGE_INFO_T *msg;
#ifdef VCHI_RX_NANOLOCKS
   if (block)
   {
      os_assert(0);
      return NULL;
   }
   // Yuck - should be able to do better than this. Kind of tied up with use of condvar in mqueue?
   // Could use non-blocking semaphore_obtain, but not in Symbian or OpenKODE Core?
   if (block)
   {
      do
      {
         int32_t success = os_count_semaphore_obtain(&service->rx_semaphore,0);
         if (success != 0) return NULL;
         msg = vchi_mqueue_get( state->rx_msg_queue, MESSAGE_SERVICE(service->index), 0 );
      } while (msg == NULL /* && !service closed */);
   }
   else {
      (void) os_semaphore_obtain(&state->rxq_semaphore);
      msg = vchi_mqueue_get( state->rx_msg_queue, MESSAGE_SERVICE(service->index), 0 );
      (void) os_semaphore_release(&state->rxq_semaphore);
   }
#else
   msg = vchi_mqueue_get( state->rx_msg_queue, MESSAGE_SERVICE(service->index), block );
#endif
   return msg;
}

/* ----------------------------------------------------------------------
 * fetch the first pending received message on the given service
 * (from the given message driver)
 *
 * actual payload is copied to the given buffer
 *
 * returns: 0 on success; -1 on failure (ie. no pending messages)
 * -------------------------------------------------------------------- */
static int32_t
single_service_dequeue_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                            void *data,
                            uint32_t max_data_size_to_read,
                            uint32_t *actual_msg_size,
                            VCHI_FLAGS_T flags )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   RX_MESSAGE_INFO_T *msg;
   uint32_t len;
   const bool_t block = (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? VC_TRUE : VC_FALSE;

   if (flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|
                VCHI_FLAGS_BLOCK_UNTIL_QUEUED|
                VCHI_FLAGS_ALLOW_PARTIAL|
                VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   msg = single_get_msg( state, service, block );
   if ( msg == NULL )
      return -1;

   //RX_MSG_SLOTINFO_T *slot = msg->slot;
   //os_logging_message( "service_dequeue_msg (service = %c%c%c%c), slot address = 0x%p" ,
   //FOURCC_TO_CHAR(service->service_id), slot->addr );

   // check that there is enough room for the message
   os_assert( msg->len <= max_data_size_to_read );
   len = OS_MIN( msg->len, max_data_size_to_read );

   // copy the message into the supplied buffer
   //os_logging_message( "address = 0x%p, length = %d" , msg->addr, len );
   memcpy( data, msg->addr, len );
   *actual_msg_size = len;

   // signal the low level driver that the message has been dealt with
   single_dequeue_from_slot( msg, state, service ); //single_state.mdriver->dequeue( msg );

   return 0; // success!
}

/* ----------------------------------------------------------------------
 * fetch the first pending received message on the given service
 * (from the given message driver)
 *
 * no copying performed: just return pointer to and length of payload
 *
 * returns: 0 on success; -1 on failure (ie. no pending messages)
 * -------------------------------------------------------------------- */
static int32_t
single_service_peek_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                         void **data,
                         uint32_t *msg_size,
                         VCHI_FLAGS_T flags )
{
   int32_t success = -1;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   RX_MESSAGE_INFO_T *msg;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   const int block = (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? 1 : 0;

   if (flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|
                VCHI_FLAGS_BLOCK_UNTIL_QUEUED|
                VCHI_FLAGS_ALLOW_PARTIAL|
                VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   //os_logging_message( "service_peek_msg" );
   msg = vchi_mqueue_peek( state->rx_msg_queue, MESSAGE_SERVICE(service->index), block );

   // check that there is something in the linked list
   if ( msg ) {
      //success = state->mdriver->extract_payload( msg->addr, data, actual_msg_size );
      *data = msg->addr;
      *msg_size = msg->len;
      success = 0;
   }

   return success;
}

/* ----------------------------------------------------------------------
 * fetch the first pending received message on the given service
 * (from the given message driver)
 *
 * no copying performed: just return pointer to and length of payload
 *
 * returns: 0 on success; -1 on failure (ie. no pending messages)
 * -------------------------------------------------------------------- */
static int32_t
single_service_hold_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                         void **data,
                         uint32_t *msg_size,
                         VCHI_FLAGS_T flags,
                         void **msg_handle )
{
   int32_t success = -1;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   RX_MESSAGE_INFO_T *msg;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   const int block = (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? 1 : 0;

   if (flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|
                VCHI_FLAGS_BLOCK_UNTIL_QUEUED|
                VCHI_FLAGS_ALLOW_PARTIAL|
                VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   //os_logging_message( "service_hold_msg" );
   msg = single_get_msg( state, service, block );

   // check that there is something in the linked list
   if ( msg ) {
      //success = state->mdriver->extract_payload( msg->addr, data, actual_msg_size );
      if (data) *data = msg->addr;
      if (msg_size) *msg_size = msg->len;
      success = 0;
      *msg_handle = msg;
   }

   return success;
}

/* ----------------------------------------------------------------------
 * initialise an iterator over the received message queue.
 *
 * returns: 0 on success; -1 on failure (no messages is not a failure)
 * -------------------------------------------------------------------- */
static int32_t
single_service_look_ahead_msg( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                               VCHI_MSG_ITER_T *iter,
                               VCHI_FLAGS_T flags )
{
   int32_t success = -1;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   const int block = (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? 1 : 0;
   void *head, *tail;

   if (flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|
                VCHI_FLAGS_BLOCK_UNTIL_QUEUED|
                VCHI_FLAGS_ALLOW_PARTIAL|
                VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   //os_logging_message( "service_look_ahead_msg" );
   vchi_mqueue_limits( state->rx_msg_queue, MESSAGE_SERVICE(service->index), &head, &tail, block );

   iter->next = head;
   iter->last = tail;
   iter->remove = NULL;

   success = 0;

   return success;
}

/* ----------------------------------------------------------------------
 * discard a held message
 *
 * intended to be used in conjunction with _peek_msg() for in-place
 * handling of payloads
 *
 * returns: 0 on success; -1 on failure (ie. no pending messages)
 * -------------------------------------------------------------------- */
static int32_t
single_held_msg_release( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                         void *msg_handle )
{
   int32_t success = 0;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;

   //os_logging_message( "service_release_msg" );

   RX_MESSAGE_INFO_T *msg = msg_handle;
   if ( msg == NULL )
      return -1;
   
   // signal the low level driver that the message has been dealt with
   single_dequeue_from_slot( msg, state, service );

   //if ( !msg ) success = -1;

   return success;
}

/* ----------------------------------------------------------------------
 * get information on a held message
 *
 * returns: 0 on success; -1 on failure
 * -------------------------------------------------------------------- */
static int32_t
single_held_msg_info( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                      void *msg_handle,
                      void **data,
                      int32_t *msg_size,
                      uint32_t *tx_timestamp,
                      uint32_t *rx_timestamp )
{
   int32_t success = -1;

   //os_logging_message( "held_msg_info" );

   RX_MESSAGE_INFO_T *msg = msg_handle;
   if ( msg == NULL )
      return -1;

   if (data) *data = msg->addr;
   if (msg_size) *msg_size = msg->len;
   if (tx_timestamp) *tx_timestamp = msg->tx_timestamp;
   if (rx_timestamp) *rx_timestamp = msg->rx_timestamp;

   success = 0;

   return success;
}

/* ----------------------------------------------------------------------
 * does the iterator have a next message?
 * -------------------------------------------------------------------- */
static bool_t
single_msg_iter_has_next( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                          const VCHI_MSG_ITER_T *iter )
{
   return (bool_t) ( iter->next != NULL );
}

/* ----------------------------------------------------------------------
 * get the next message from the iterator
 *
 * no copying performed: just return pointer to and length of payload
 *
 * returns: 0 on success; -1 on failure (ie. no further messages)
 * -------------------------------------------------------------------- */
static int32_t
single_msg_iter_next( VCHI_CONNECTION_SERVICE_HANDLE_T service,
                      VCHI_MSG_ITER_T *iter,
                      void **data,
                      uint32_t *msg_size )
{
   RX_MESSAGE_INFO_T *msg = iter->next;
   if ( msg == NULL )
      return -1;

   if (data) *data = msg->addr;
   if (msg_size) *msg_size = msg->len;
   iter->remove = msg;
   if (msg == iter->last)
      iter->next = NULL;
   else
      iter->next = msg->next;

   return 0;
}

/* ----------------------------------------------------------------------
 * remove the last message returned by the iterator
 *
 * returns: 0 on success; -1 on failure
 * -------------------------------------------------------------------- */
static int32_t
single_msg_iter_remove( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                        VCHI_MSG_ITER_T *iter )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   int32_t success = -1;

   RX_MESSAGE_INFO_T *msg = iter->remove;
   if ( msg == NULL )
      return -1;

   success = vchi_mqueue_element_get( state->rx_msg_queue, MESSAGE_SERVICE(service->index), msg );
   if ( success == 0 )
   {
      iter->remove = NULL;

      // signal the low level driver that the message has been dealt with
      single_dequeue_from_slot( msg, state, service );
   }

   os_assert( success == 0);

   return success;
}

/* ----------------------------------------------------------------------
 * hold the last message returned by the iterator
 *
 * returns: 0 on success; -1 on failure
 * -------------------------------------------------------------------- */
static int32_t
single_msg_iter_hold( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                      VCHI_MSG_ITER_T *iter,
                      void **msg_handle )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   int32_t success = -1;

   RX_MESSAGE_INFO_T *msg = iter->remove;
   if ( msg == NULL )
      return -1;

   success = vchi_mqueue_element_get( state->rx_msg_queue, MESSAGE_SERVICE(service->index), msg );
   if ( success == 0 )
   {
      *msg_handle = msg;
      iter->remove = NULL;
   }

   os_assert( success == 0);

   return success;
}

/* ----------------------------------------------------------------------
 * should we send a bulk auxiliary message for this service?
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static bool_t
single_should_send_bulk_aux( const SINGLE_CONNECTION_STATE_T *state,
                             const SERVICE_HANDLE_T *service )
{
   return (bool_t) ( service->always_tx_bulk_aux ||
                     service->crc ||
                     service->unaligned_bulk_tx );
}

/* ----------------------------------------------------------------------
 * compute the details of the core transfer and patchup to deal with
 * unalignment. Should this routine be in bulk_aux_service.c?
 * -------------------------------------------------------------------- */
static void
single_get_bulk_tx_aligned_section( TX_MSGINFO_T *bulk )
{
   TX_BULK_REQ_T *req = bulk->info;
   SERVICE_HANDLE_T *service = bulk->service;
   SINGLE_CONNECTION_STATE_T *state = service->state;

   // Unrequested (short) transfer - exit
   if (!req)
      return;

   req->append_dummy_data = 0;

   if (service->unaligned_bulk_tx)
   {
      //const TUint alignment = DVCMessiTxChannel::GetBufferRequirements().iAlignment;
      const int alignment = state->mdriver->tx_alignment( state->mhandle, bulk->channel );
      if (req->rx_data_bytes == 0)
      {
         // they're not expecting anything over data, so we won't transmit anything over data
         req->tx_data = NULL;
         req->tx_data_bytes = 0;
         req->tx_head_bytes = bulk->len;
         req->tx_tail_bytes = 0;
         req->data_shift = 0;
      }
      else
      {
         // We have two options:
         //  1) Transmit from as close to the start of our buffer as we can, given alignment
         //  2) Transmit from their start offset, but only if it is aligned
         //
         // Option 2 saves a memmove at the destination, but may mean we send KAlignment
         // bytes fewer over the data channel. Current strategy is to only take this option
         // if it doesn't reduce the amount sent.
         //
         // First work out the parameters for option 1
         const void *our_start = bulk->addr, *their_start;
         uint32_t our_len = bulk->len;
         if (!IS_ALIGNED(our_start, alignment))
         {
            uint32_t misalignment = alignment - MISALIGNMENT(our_start, alignment);
            our_start = (uint8_t*)our_start + misalignment;
            our_len -= misalignment;
         }
         if (our_len % VCHI_BULK_GRANULARITY)
            our_len -= our_len % VCHI_BULK_GRANULARITY;

         if (our_len > req->rx_data_bytes)
            our_len = req->rx_data_bytes;

         // Now consider option 2
         their_start = (const uint8_t *)bulk->addr + req->rx_data_offset;
         if (their_start != our_start && IS_ALIGNED(their_start, alignment) && our_len == req->rx_data_bytes)
            our_start = their_start;

         if (our_len == 0 ||
             (our_len < req->rx_data_bytes && !state->mdriver->tx_supports_terminate( state->mhandle, bulk->channel )))
         {
            // Either we don't have any data to transmit at all, or we're short and we can't
            // terminate after a short message. In both cases, add on a dummy transmission.
            os_assert(our_len == req->rx_data_bytes - (int)VCHI_BULK_GRANULARITY);
            req->append_dummy_data = VCHI_BULK_GRANULARITY;
         }

         req->tx_data = our_start;
         req->tx_data_bytes = our_len;
         if (our_len != 0)
         {
            req->tx_head_bytes = (uint32_t) ( (uint8_t*)our_start - (uint8_t*)bulk->addr );
            req->tx_tail_bytes = bulk->len - our_len - req->tx_head_bytes;
            req->data_shift = req->tx_head_bytes - req->rx_data_offset;
         }
         else
         {
            req->tx_head_bytes = bulk->len;
            req->tx_tail_bytes = 0;
            req->data_shift = 0;
         }
      }
   }
   else
   {
      req->tx_data = bulk->addr;
      req->tx_data_bytes = bulk->len;
      req->tx_head_bytes = req->tx_tail_bytes = 0;
      req->data_shift = 0;
   }
}

/* ----------------------------------------------------------------------
 * queue a bulk message for transmission to the other side
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_bulk_queue_transmit( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                            const void *data_src,
                            uint32_t data_size,
                            VCHI_FLAGS_T flags,
                            void *bulk_handle )
{
   TX_MSGINFO_T *entry;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   OS_SEMAPHORE_T blocking_sem;
   const int block = (flags & (VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)) ? 1 : 0;

   if (flags & VCHI_FLAGS_ALLOW_PARTIAL)
      return -2;

   // purify flags, to save future headaches;
   if ( flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE )
      flags &=~ VCHI_FLAGS_BLOCK_UNTIL_DATA_READ;

   if ( data_size == 0 ) return -2;

#if 0
   // Emergency debugging for when bulk/CCP goes nuts
   static int32_t bulk_tx_no;
   const uint8_t *p = data_src, *t = p + data_size - 8;
   bulk_tx_no++;
   *(uint8_t *) p = bulk_tx_no;
   os_logging_message("Bulk TX %d address: %08X", bulk_tx_no, p);
   os_logging_message("Bulk TX %d start: %02X %02X %02X %02X %02X %02X %02X %02X ... %02X %02X %02X %02X %02X %02X %02X %02X",
                      bulk_tx_no,
                      p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7],
                      t[0], t[1], t[2], t[3], t[4], t[5], t[6], t[7]);
#endif

   if ( !service->want_unaligned_bulk_tx && // check against want_ flag to make sure they're using API properly
        !state->mdriver->buffer_aligned(state->mhandle,1,0,data_src,data_size) )
   {
      return -2;
   }

   entry = vchi_mqueue_get( state->tx_bulk_queue, TX_BULK_FREE, block );
   if ( !entry )
      return -1;

   // fill in 'entry'
   entry->channel = MESSAGE_TX_CHANNEL_BULK;
   entry->handle  = bulk_handle;
   entry->service = service;
   entry->info    = NULL;
   entry->flags   = flags;
   entry->addr    = (void *)data_src;
   entry->len     = data_size;

   // setup for blocking behaviour if required
   if ( flags & (VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ) {
      (void) os_semaphore_create( &blocking_sem, OS_SEMAPHORE_TYPE_SUSPEND );
      (void) os_semaphore_obtain( &blocking_sem );
      entry->blocking = &blocking_sem;
   }

   // move this slot on to our pending queue
   vchi_mqueue_put( state->tx_bulk_queue, TX_BULK_PENDING, entry );

   //const fourcc_t sid = service->service_id;
   //os_logging_message( "tx_bulk_queue 0>1 (%p,%d,%d,%c%c%c%c)", entry->addr, entry->len, entry->flags,
   //FOURCC_TO_CHAR(sid) );
   //vchi_mqueue_debug( state->tx_bulk_queue );

   // after we put it on the pending queue, we can no longer refer to entry;
   // it may have been put back onto the free queue and re-used by another task
   entry = NULL;

   // signal that there is a request for a transfer pending
   // [would like to just call single_marshall_tx_bulk here, specifying the
   // specific service - could do if we were operating a global lock model]
   single_event_signal( state, SINGLE_EVENTBIT_TX_BULK_EVENT );

   // if requested we will now wait for the signal that the transfer has completed
   if ( flags & (VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ) {
      single_unlock( state );
      (void) os_semaphore_obtain( &blocking_sem );
#ifndef VCHI_ELIDE_BLOCK_EXIT_LOCK
      single_lock( state ); // would be nice to short-circuit this - vchi.c will unlock again immediately
#endif
      (void) os_semaphore_destroy( &blocking_sem );
   }
   return 0;
}

/* ----------------------------------------------------------------------
 * run through the TX_BULK_INUSE queue, and deal with any
 * completed transfers. Maintain client-visible per-service order for
 * completion unblocks and callbacks.
 *
 * Called from slot handling task, whenever a completion flag is set.
 * -------------------------------------------------------------------- */
static void
single_process_completed_tx_bulk( SINGLE_CONNECTION_STATE_T *state )
{
   char service_preceding_unread[VCHI_MAX_SERVICES_PER_CONNECTION] = { 0 };

   TX_MSGINFO_T *t, *next;
   
   single_dump_tx_bulk_inuse( state, "single_process_completed_tx_bulk" );

   for ( t = vchi_mqueue_peek( state->tx_bulk_queue, TX_BULK_INUSE, 0 ); t; t = next )
   {
      next = t->next;

      // Don't notify if any preceding entries for this service are unread.
      // Clients will assume this (eg doing 3 queues with only the last one having
      // BLOCK_UNTIL_DATA READ).
      if ( service_preceding_unread[t->service->index] &&
           t->flags & (VCHI_FLAGS_CALLBACK_WHEN_DATA_READ|
                       VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                       VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|
                       VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE))
         continue;

      // If BulkAux has been queued and data sent, then all data has been read:
      // deal with callback and unblocking.
      if ( (t->flags & (VCHI_FLAGS_BULK_AUX_QUEUED|VCHI_FLAGS_BULK_DATA_COMPLETE))
                    == (VCHI_FLAGS_BULK_AUX_QUEUED|VCHI_FLAGS_BULK_DATA_COMPLETE) )
      {
         if ( t->flags & VCHI_FLAGS_CALLBACK_WHEN_DATA_READ )
         {
            os_logging_message("Bulk TX callback: data read (%c%c%c%c:%d)", FOURCC_TO_CHAR(t->service->service_id), t->len);
            single_issue_callback( t->service, VCHI_CALLBACK_BULK_DATA_READ, t->handle );
            t->flags &=~ VCHI_FLAGS_CALLBACK_WHEN_DATA_READ;
         }

         if ( t->flags & VCHI_FLAGS_BLOCK_UNTIL_DATA_READ )
         {
            os_logging_message("Bulk TX unblocking: data read (%c%c%c%c:%d)", FOURCC_TO_CHAR(t->service->service_id), t->len);
            (void) os_semaphore_release( t->blocking );
            t->flags &=~ VCHI_FLAGS_BLOCK_UNTIL_DATA_READ;
         }

         // If BulkAux has also completed, then op is complete.
         if ( t->flags & VCHI_FLAGS_BULK_AUX_COMPLETE )
         {
            // No need for extra preceding entries check - we already know all preceding entries
            // have their data read, and if this bulk aux has completed, then all previous ones
            // have, and have been reported, as they're done in order.
            if ( t->flags & VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE )
            {
               os_logging_message("Bulk TX callback: op complete (%c%c%c%c:%d)", FOURCC_TO_CHAR(t->service->service_id), t->len);
               single_issue_callback( t->service, VCHI_CALLBACK_BULK_SENT, t->handle );
               t->flags &=~ VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE;
            }

            if ( t->flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE )
            {
               os_logging_message("Bulk TX unblocking: op complete (%c%c%c%c:%d)", FOURCC_TO_CHAR(t->service->service_id), t->len);
               (void) os_semaphore_release( t->blocking );
               t->flags &=~ VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE;
            }
         }

         // As long as we're not still waiting for aux completion for the
         // user, we're done.
         if ((t->flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)) == 0)
         {
            int32_t status;
            os_logging_message("Bulk TX complete (%c%c%c%c:%d)", FOURCC_TO_CHAR(t->service->service_id), t->len);
            // return the peer request structure
            if ( t->info ) {
               vchi_mqueue_put_head( state->tx_bulk_req_queue, TX_BULK_REQ_FREE, t->info );
               t->info = NULL;
            }

            // return the bulk tx queue structure
            status = vchi_mqueue_element_move( state->tx_bulk_queue, TX_BULK_INUSE, TX_BULK_FREE, t);
            os_assert( status == 0 );
            t = NULL;
         }
      }
      else
         service_preceding_unread[t->service->index] = 1;
   }
}


/* ----------------------------------------------------------------------
 * handle the removal of a message from a slot
 *
 * called from ->dequeue_msg(), ->release() and ->remove()
 * in service task context
 *
 * called from _message_received_action() in vchi task context
 * -------------------------------------------------------------------- */
static void
single_dequeue_from_slot( RX_MESSAGE_INFO_T *msg, SINGLE_CONNECTION_STATE_T *state, SERVICE_HANDLE_T *service )
{
   RX_MSG_SLOTINFO_T *slot = msg->slot;
   bool_t signal_xon_xoff = VC_FALSE, signal_slot_available = VC_FALSE;

#if !defined VCHI_COARSE_LOCKING || defined VCHI_RX_NANOLOCKS
   os_semaphore_obtain( &state->rxq_semaphore );
#endif
   
   if(service)
   {
      // now record that we have removed a message from this slot for the relevant service
      // if we are making the 1->0 transition then we have unblocked a slot
      if(--slot->ref_count[service->index] == 0) {
         service->rx_slots_in_use--;
         // wake up slot handling task to do XON/XOFF - would like to call xon_xoff directly here,
         // like in Symbian-land, but it would need more locks. Maybe if we go to global locks.
         // single_send_xon_xoff_message( service );
         signal_xon_xoff = VC_TRUE;
      }
   }

   os_assert( slot->msgs_released < slot->msgs_parsed ); // this would be bad
   slot->msgs_released++;

   if ( slot->msgs_released == slot->msgs_parsed && slot->active == 0 ) {
      //os_logging_message( "Dequeue from slot : Dequeuing slot 0x%p, slot_no %d",slot,slot->slot_no);

      // at this point, slot should be on 'pending' queue
      int32_t success = vchi_mqueue_element_move( state->rx_slots, RX_SLOT_PENDING, RX_SLOT_FREE, slot );

      os_assert( success == 0 );

      // we have freed a slot so we should try and queue as many as possible onto the hardware
      // we will set a flag so that the slot_handling_task can take care of it (avoids deadlock problems)

      //os_logging_message( "Dequeue from slot : About to set events");


      signal_slot_available = VC_TRUE;

   }

   // now we've dealt with all the information about the message we can move it to the free queue
   vchi_mqueue_put_head( state->rx_msg_queue, MESSAGE_FREE, msg );

#if !defined VCHI_COARSE_LOCKING || defined VCHI_RX_NANOLOCKS
   (void) os_semaphore_release( &state->rxq_semaphore );
#endif

   if ( signal_slot_available )
      single_event_signal( state, SINGLE_EVENTBIT_RX_SLOT_AVAILABLE_EVENT );
   if ( signal_xon_xoff )
      single_event_signal( state, SINGLE_EVENTBIT_RX_XON_XOFF_EVENT );

}

/* ----------------------------------------------------------------------
 * Returns whether any message transmits are pending
 * -------------------------------------------------------------------- */
static bool_t
single_any_tx_pending( const SINGLE_CONNECTION_STATE_T *state )
{
   TX_MSG_SLOTINFO_T *slot = vchi_mqueue_peek(state->tx_slots, TX_SLOT_PENDING, 0);
   return (bool_t) (slot && WPTR(slot->write_ptr) > slot->read_ptr);
}

/* ----------------------------------------------------------------------
 * Add as many free slots to the low level hardware as possible
 * -------------------------------------------------------------------- */
static void
single_add_slots_to_hardware( SINGLE_CONNECTION_STATE_T *state )
{
   RX_MSG_SLOTINFO_T *s;
   int32_t success;

   // if there is space in the rx msg FIFO add free slots (if there are any) to it
   for (;;) {
      s = vchi_mqueue_get( state->rx_slots, RX_SLOT_FREE, 0 );
      if ( !s ) break; // no more slots on 'free' queue

      success = state->mdriver->add_msg_rx_slot( state->mhandle, s );
      if ( success != 0 ) {
         // didn't manage to push this slot to the message driver:
         // push back onto 'free' queue
         vchi_mqueue_put_head( state->rx_slots, RX_SLOT_FREE, s );
         break;
      }

      vchi_mqueue_put( state->rx_slots, RX_SLOT_INUSE, s );
      state->rx_slot_count++;
      state->rx_slot_delta++;
   }

   // Check whether any messages are waiting to go out. If so, they'll carry the indication
   // of the newly available slot. If not, send an immediate dummy message to carry it.
   if ( state->rx_slot_delta > 0 && !single_any_tx_pending( state ) && state->control_service && !state->suspended ) {
      (void) single_service_queue_msg( (VCHI_CONNECTION_SERVICE_HANDLE_T)state->control_service,NULL,0,VCHI_FLAGS_NONE,NULL);
   }
}

extern fourcc_t vchi_control_reply_pick_service(void *handle, uint8_t *message);
extern void vchi_control_reply_sub_service(void *handle, fourcc_t service_id);

static void
single_send_server_available_reply_messages(SINGLE_CONNECTION_STATE_T *state )
{
   if ( !state->control_service )
      return;

   for (;;)
   {
      int32_t return_value;
      uint8_t message[12];
      fourcc_t service_id;

      //pick a service need to send reply
      //and return message needs to be send
      service_id = vchi_control_reply_pick_service(state->control_service->callback_param, message);
      if (service_id == 0) //nothing more to send
         return;

      return_value = single_service_queue_msg((VCHI_CONNECTION_SERVICE_HANDLE_T) state->control_service,
                                              message,
                                              12,
                                              VCHI_FLAGS_NONE,
                                              NULL);

      if (return_value==0)
         vchi_control_reply_sub_service(state->control_service->callback_param, service_id);
   }
}

/* ----------------------------------------------------------------------
 * try to shovel as many slots as possible (message and bulk, as
 * marshalled through tx_slots and tx_bulk_queue) out to the
 * hardware as possible.
 *
 * take care to ensure we don't write too much; we need to ensure the
 * receiving end has at least one free rx slot (so we can inform it
 * when our side has added more rx slots).
 * -------------------------------------------------------------------- */
static void
single_pump_transmit( SINGLE_CONNECTION_STATE_T *state )
{
  (void) single_send_xon_xoff_messages( state );
   single_send_server_available_reply_messages( state );

   for (;;) {

      // bulk advertising stuff goes here
      if ( single_pump_bulk_rx_request(state) )
         continue;

      // queued messages from services take priority over bulk
      if ( single_pump_tx_msg(state) )
         continue;

      // any BULX stuff would go here (or, possibly, after bulk?)
      if ( single_pump_tx_bulk_aux(state) )
         continue;

      // send any bulk messages if possible
      if ( single_pump_tx_bulk(state) )
         continue;

      break;
   }

   // If we still need to inform them of newly available slots, and there aren't
   // any pending transmissions to carry the indication, send a dummy one.
   if ( state->rx_slot_delta > 0 && state->control_service && !single_any_tx_pending( state ) )
      (void) single_service_queue_msg( (VCHI_CONNECTION_SERVICE_HANDLE_T)state->control_service,NULL,0,VCHI_FLAGS_NONE,NULL);
}


/* ----------------------------------------------------------------------
 * send any XON/XOFF messages that need to be sent
 *
 * returns negative if error, >=0 if nothing to be sent, or sent OK
 *
 * called from slot-handling function
 * -------------------------------------------------------------------- */
static int32_t
single_send_xon_xoff_message( SERVICE_HANDLE_T *service )
{
   SINGLE_CONNECTION_STATE_T *state = service->state;
   int32_t success = 0;

   if ( service == state->control_service )
      return 0;

   if ( service->rx_xoff_sent ) {
      if ( service->rx_slots_in_use <= VCHI_XON_THRESHOLD ) {
         success = vchi_control_queue_xon_xoff( state->control_service->callback_param, service->service_id, VC_TRUE );
         if ( success >= 0 ) {
            service->rx_xoff_sent = VC_FALSE;
            single_issue_callback( service, VCHI_CALLBACK_SENT_XON, NULL );
         }
      }
   }
   else {
      if ( service->rx_slots_in_use >= VCHI_XOFF_THRESHOLD ) {
         success = vchi_control_queue_xon_xoff( state->control_service->callback_param, service->service_id, VC_FALSE );
         if ( success >= 0 ) {
            service->rx_xoff_sent = VC_TRUE;
            single_issue_callback( service, VCHI_CALLBACK_SENT_XOFF, NULL );
         }
      }
   }

   return success;
}

/* ----------------------------------------------------------------------
 * send any XON/XOFF messages that need to be sent
 *
 * returns negative if error, >=0 if nothing to be sent, or all sent OK
 * -------------------------------------------------------------------- */
static int32_t
single_send_xon_xoff_messages(SINGLE_CONNECTION_STATE_T *state )
{
   int32_t success = 0;
   int i;
   for (i = 0; i <= state->highest_service_index; i++)
      if (state->services[i].type != SINGLE_UNUSED)
      {
         success = single_send_xon_xoff_message( &state->services[i] );
         if (success < 0)
            break;
      }
   return success;
}

/* ----------------------------------------------------------------------
 * try to push a single bulk rx request
 *
 * returns 0 if unable to send this slot; 1 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_pump_bulk_rx_request( SINGLE_CONNECTION_STATE_T *state )
{
   RX_BULK_SLOTINFO_T *entry;
   SERVICE_HANDLE_T *service;
   uint32_t data_len, data_offset;
   bool_t requested;

   entry = vchi_mqueue_peek( state->rx_bulk_queue, RX_BULK_UNREQUESTED, 0 );
   if ( !entry )
      return 0;

   service = entry->service;
   if ( entry->len >= state->min_bulk_size || !service->unaligned_bulk_rx )
   {
      int32_t success;

      if ( service->unaligned_bulk_rx )
      {
         void *data_ptr;
         single_get_bulk_rx_aligned_section( state, entry->addr, entry->len, &data_ptr, &data_len );
         data_offset = (uint32_t) ( (uint8_t*)data_ptr - (uint8_t*)entry->addr );
      }
      else
      {
         data_len = entry->len;
         data_offset = 0;
      }

#if VCHI_MAX_BULK_RX_CHANNELS_PER_CONNECTION > 1
#error "This VCHI implementation doesn't currently support multiple receive channels."
#error "See Symbian implementation for example of how to do it."
#endif

      // create and send 'I added a bulk rx slot' message
      success = vchi_control_queue_bulk_transfer_rx( state->control_service->callback_param,
                                                     service->service_id,
                                                     entry->len,
                                                     data_len == 0 ? MESSAGE_RX_CHANNEL_MESSAGE
                                                                   : MESSAGE_RX_CHANNEL_BULK,
                                                     0, // channel parameters
                                                     data_len,
                                                     data_offset );
      if ( success != 0 )
         return 0;
      os_logging_message("Sent Bulk RX request for service %c%c%c%c, %d bytes", FOURCC_TO_CHAR(service->service_id), entry->len);
      requested = VC_TRUE;
   }
   else
   {
      os_logging_message("Omitting Bulk RX request for service %c%c%c%c, %d bytes", FOURCC_TO_CHAR(service->service_id), entry->len);
      requested = VC_FALSE;
      data_len = 0;
   }

   if ( data_len == 0 )
   {
      (void) vchi_mqueue_move( state->rx_bulk_queue, RX_BULK_UNREQUESTED, RX_BULK_DATA_ARRIVED(MESSAGE_RX_CHANNEL_MESSAGE) );
      // if no request being sent, the bulk aux could have already arrived
      if (!requested)
          (void) single_process_bulk_aux_for_service( state, service );
   }
   else
     (void) vchi_mqueue_move( state->rx_bulk_queue, RX_BULK_UNREQUESTED, RX_BULK_REQUESTED(MESSAGE_RX_CHANNEL_BULK) );
   return 1;
}

/* ----------------------------------------------------------------------
 * try to push a single message slot to the underlying hardware
 *
 * returns 0 if unable to send this slot; 1 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_pump_tx_msg( SINGLE_CONNECTION_STATE_T *state )
{
   // keep track of how many slots we have potentially used on the peer
   int16_t new_rx_slot_delta = state->rx_slot_delta;
   int32_t success = -1;
   TX_MSG_SLOTINFO_T *slot;
   uint8_t *addr;

   // no room - can't send
   if ( state->peer_slot_count == 0)
      return 0;

   // First, do any filled slots from the pending queue. The first may have been
   // partially transmitted already, so make sure we only go from the read_ptr.
   slot = vchi_mqueue_peek( state->tx_slots, TX_SLOT_PENDING, 0 );
   while ( slot )
   {
      VCHI_MSG_FLAGS_T msg_flags;
      // Client threads may be writing these variables as we go. Care required.
      uint32_t write_ptr;
      write_ptr = slot->write_ptr;

      // If this slot has already been fully transmitted, move it on to the inuse or free queue
      if ( IS_FILLED(write_ptr) && WPTR(write_ptr) == slot->read_ptr )
      {
         os_assert(!state->mdriver->tx_supports_terminate( state->mhandle, MESSAGE_TX_CHANNEL_MESSAGE ));
         (void) vchi_mqueue_get( state->tx_slots, TX_SLOT_PENDING, 0 );
         if ( slot->tx_count )
            vchi_mqueue_put( state->tx_slots, slot->queue = TX_SLOT_INUSE, slot );
         else
            single_return_free_tx_slot( state, slot );
         state->peer_slot_count--;
         slot = vchi_mqueue_peek( state->tx_slots, TX_SLOT_PENDING, 0 );
         continue;
      }

      // Avoid deadlock - don't use up their last slot unless we use it to
      // advertise a new slot, or we may end up unable to signal when local slots
      // become available. (Even if all our slots are currently available, the
      // peer may have a complete fill-up of our slots in transit as we make
      // this decision; that would deadlock us if we used up our last transmit
      // permission now). This fixes it as long as both sides are using this
      // same logic.
      if ( IS_FILLED(write_ptr) && state->peer_slot_count == 1 && state->rx_slot_delta <= 0 )
         return 0;

      if ( WPTR(write_ptr) == slot->read_ptr )
         break;

      for (addr = slot->addr + slot->read_ptr; addr < slot->addr + WPTR(write_ptr); )
      {
         int32_t len = state->mdriver->update_message( state->mhandle, addr, &new_rx_slot_delta );
         addr += len;
      }

      // If they support terminate, we set the flag. Otherwise, we rely on them advancing
      // automatically to the next slot when they receive the next message, because it
      // doesn't fit in the current slot.
      if (IS_FILLED(write_ptr) &&
          state->mdriver->tx_supports_terminate( state->mhandle, MESSAGE_TX_CHANNEL_MESSAGE ))
         msg_flags = VCHI_MSG_FLAGS_TERMINATE_DMA;
      else
         msg_flags = VCHI_MSG_FLAGS_NONE;

      success = state->mdriver->send( state->mhandle,
                                      MESSAGE_TX_CHANNEL_MESSAGE,
                                      slot->addr + slot->read_ptr,
                                      WPTR(write_ptr) - slot->read_ptr,
                                      msg_flags,
                                      slot );
      if ( success == 0 )
      {
         slot->read_ptr = WPTR(write_ptr);
         slot->tx_count++;
         state->rx_slot_delta = new_rx_slot_delta;
         if (IS_FILLED(write_ptr))
         {
            TX_MSG_SLOTINFO_T *s = vchi_mqueue_get(state->tx_slots, TX_SLOT_PENDING, 0);
            os_assert(s == slot);
            slot->queue = TX_SLOT_INUSE;
            vchi_mqueue_put(state->tx_slots, TX_SLOT_INUSE, slot);
            state->peer_slot_count--;
         }
         return 1;
      }
      else
         return 0;
   }

   return 0;
}


/* ----------------------------------------------------------------------
 * match our peer's next requested bulk to our queue of outgoing bulks
 *
 * we have the following ordering requirements:
 * 1) the order of data transfers on each (real) channel must match the
 *    order of requests for that channel;
 * 2) the order of bulk auxiliary messages for each service must match
 *    the order of requests for that service
 *    (if bulk auxiliary messages are not in use, the receiver is
 *    required to make sure that all requests for a service go on the
 *    same channel, thus preserving intra-service order using rule 1;
 * 3) the client-visible completion order (callbacks and unblocking)
 *    for a service must match the order of bulk_queue_transmits for that
 *    service.
 *
 * Put together, all this means that despite the apparent awkwardness,
 * it's easiest with a single integrated "in-use" queue. This function
 * determines the final order in the in-use queue, and entries stay in
 * place on that queue until they are retired.
 *
 * In examples below, X1 indicates a request for service X on channel 1,
 * X indicates a queued transmit for X. x indicates a small transmit for
 * X that doesn't expect a request.
 * -------------------------------------------------------------------- */
static void
single_marshall_tx_bulk( SINGLE_CONNECTION_STATE_T *state, fourcc_t id )
{
   bool_t match;

   // outer loop - every match needs us to go back and look again from
   // the start
   // eg  Y1 X1 : X Y   => send Y -> 1, then go back and send X -> 1
   do
   {
      char service_block[VCHI_MAX_SERVICES_PER_CONNECTION] = { 0 };
      TX_MSGINFO_T *bulk, *next;

      match = VC_FALSE;

      // iterate first over our bulk tx queue, as some transmits may not
      // have corresponding requests
      // ( eg Y1 : x    => send x, despite no request for X )
      for ( bulk = vchi_mqueue_peek( state->tx_bulk_queue, TX_BULK_PENDING, 0 ); bulk; bulk = next )
      {
         SERVICE_HANDLE_T *service = bulk->service;
         next = bulk->next;

         if ( (id == 0 || service->service_id == id) && !service_block[service->index] )
         {
            TX_BULK_REQ_T *req = NULL;
            // For short messages, we can proceed immediately, without a
            // matching request
            if ( bulk->len < state->min_bulk_size &&
                 service->unaligned_bulk_tx )
            {
               match = VC_TRUE;
            }
            else
            {
               char channel_block[1+VCHI_MAX_BULK_TX_CHANNELS_PER_CONNECTION] = { 0 };

               // Search through the request queue, finding a channel that wants
               // a request for this service next. Need to handle cases like:
               // X0 : X        => X -> 0
               // Y1 X0 : X     => X -> 0
               // Y0 X0 : X     => can't send, channel 0 expects Y first
               // Y0 X0
               for (req = vchi_mqueue_peek( state->tx_bulk_req_queue, TX_BULK_REQ_PENDING, 0 ); req; req = req->next )
               {
                  if ( req->service_id == service->service_id )
                  {
                     // this is the match, but can we go yet?
                     if ( !channel_block[ req->channel ] )
                        match = VC_TRUE;
                     break;
                  }
                  else if ( req->channel != MESSAGE_TX_CHANNEL_MESSAGE )
                     // channel wants a different service first, so can't use it for this service
                     // eg   Y1 X1 : X    => Can't send X until found a match for Y0
                     // but  Y0 X0 : X    => x->0  is okay
                     channel_block[ req->channel ] = 1;
               }
            }

            if ( match )
            {
               single_tx_bulk_ready( state, bulk, req );
               break;
            }
            else
               // failed to match this service, so can't send subsequent references.
               // eg:  Y1       :  X x X    => Can't send x until X has gone
               //      Y1 X1 X2 :  X        => X -> 1 blocked, so mustn't try to send X -> 2
               service_block[ service->index ] = 1;
         }
      }

      // After the first iteration, need to check any service. For example,
      // having just added a queue for Y:
      // Y1 X1 : X Y  => send Y -> 1, then that frees up X
      id = 0;
   } while (match);
}

/* ----------------------------------------------------------------------
 * matched a bulk - sort out the housework
 * -------------------------------------------------------------------- */
static void
single_tx_bulk_ready( SINGLE_CONNECTION_STATE_T *state, TX_MSGINFO_T *tx_bulk_entry, TX_BULK_REQ_T *tx_bulk_req )
{
   SERVICE_HANDLE_T *service = tx_bulk_entry->service;
   int32_t status;

   if (tx_bulk_req)
   {
      os_assert( service->service_id == tx_bulk_req->service_id);
      status = vchi_mqueue_element_get( state->tx_bulk_req_queue, TX_BULK_REQ_PENDING, tx_bulk_req );
      os_assert( status == 0 );

      // now check that the length matches - if it doesn't we are somehow out of sync
      os_assert( tx_bulk_entry->len == tx_bulk_req->length );

      // check sane values in request
      if ( service->unaligned_bulk_tx )
         os_assert( tx_bulk_req->rx_data_bytes % VCHI_BULK_GRANULARITY == 0 &&
                    tx_bulk_req->rx_data_bytes <= tx_bulk_req->length ); // Odd requested data size
      else
         os_assert( tx_bulk_req->rx_data_bytes == tx_bulk_req->length &&
                    tx_bulk_req->rx_data_offset == 0 ); // Peer wants unaligned transfer - not enabled

   }

   os_assert( tx_bulk_entry->channel == MESSAGE_TX_CHANNEL_BULK );

   // successful
   tx_bulk_entry->info = tx_bulk_req;
   tx_bulk_entry->channel = tx_bulk_req ? tx_bulk_req->channel : MESSAGE_TX_CHANNEL_MESSAGE;

   // deal with unalignedness - fill in fields in peer_rx_info
   single_get_bulk_tx_aligned_section( tx_bulk_entry );

   if ( !single_should_send_bulk_aux( state, service ))
      tx_bulk_entry->flags |= VCHI_FLAGS_BULK_AUX_QUEUED | VCHI_FLAGS_BULK_AUX_COMPLETE;

   if ( single_should_send_bulk_aux( state, service ) )
       {
       if (!tx_bulk_req || (tx_bulk_req->tx_data_bytes == 0 && !tx_bulk_req->append_dummy_data))
           tx_bulk_entry->flags |= VCHI_FLAGS_BULK_DATA_QUEUED|VCHI_FLAGS_BULK_DATA_COMPLETE;
       }
   else
      tx_bulk_entry->flags |= VCHI_FLAGS_BULK_AUX_QUEUED|VCHI_FLAGS_BULK_AUX_COMPLETE;

   // move to the in-use state
   status = vchi_mqueue_element_move( state->tx_bulk_queue, TX_BULK_PENDING, TX_BULK_INUSE, tx_bulk_entry );
   os_assert( status == 0 );

   single_dump_tx_bulk_inuse( state, "single_tx_bulk_ready" );

   // signal that an entry has been reported
   single_event_signal( state, SINGLE_EVENTBIT_TX_SLOT_READY );
}

/* ----------------------------------------------------------------------
 * try to create and push a single bulk auxiliary message
 *
 * returns 0 if unable to send this slot; 1 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_pump_tx_bulk_aux( SINGLE_CONNECTION_STATE_T *state )
{
   TX_MSGINFO_T *tx_bulk_entry;
   const TX_BULK_REQ_T *req;
   TX_BULK_REQ_T dummy_req;
   SERVICE_HANDLE_T *service;
   uint32_t chunk_size;
   uint8_t message[BULX_HEADER_SIZE];
   int32_t success;
   VCHI_MSG_VECTOR_T v[5];

   single_dump_tx_bulk_inuse( state, "single_pump_tx_bulk_aux" );

   for ( tx_bulk_entry = vchi_mqueue_peek( state->tx_bulk_queue, TX_BULK_INUSE, 0 ); tx_bulk_entry; tx_bulk_entry = tx_bulk_entry->next )
   {
      if (tx_bulk_entry->flags & VCHI_FLAGS_BULK_AUX_QUEUED)
         continue;

      // create and send message
      service = tx_bulk_entry->service;
      req = tx_bulk_entry->info;
      chunk_size = state->mdriver->tx_bulk_chunk_size(state->mhandle, tx_bulk_entry->channel);

      if (!req)
      {
         // This is a unrequested, short bulk-aux only transmit - knock up
         // a local dummy representative request...
         dummy_req.channel = MESSAGE_TX_CHANNEL_MESSAGE;
         dummy_req.tx_data = NULL;
         dummy_req.tx_data_bytes = 0;
         dummy_req.data_shift = 0;
         dummy_req.tx_head_bytes = tx_bulk_entry->len;
         dummy_req.tx_tail_bytes = 0;
         req = &dummy_req;
      }
      v[0].vec_base = message;
      v[0].vec_len = vchi_bulk_aux_service_form_header( &message, sizeof message,
                                                        service->service_id,
                                                        req->channel,
                                                        tx_bulk_entry->len,
                                                        chunk_size != 0 ? chunk_size : req->tx_data_bytes,
                                                        service->crc ? single_compute_crc(tx_bulk_entry->addr, tx_bulk_entry->len) : 0,
                                                        req->tx_data_bytes,
                                                        req->data_shift,
                                                        req->tx_head_bytes,
                                                        req->tx_tail_bytes );
      // head data, if any:
      v[1].vec_base = tx_bulk_entry->addr;
      v[1].vec_len = req->tx_head_bytes;
      // tail data, if any:
      v[2].vec_base = (const uint8_t *) tx_bulk_entry->addr + tx_bulk_entry->len - req->tx_tail_bytes;
      v[2].vec_len = req->tx_tail_bytes;
      // 0-3 bytes of padding, if required;
      v[3].vec_base = "\0\0\0";
      v[3].vec_len = (4 - (v[1].vec_len + v[2].vec_len) & 3) & 3;
      // any link-specific data (eg CCP2 patch-up)
      state->mdriver->form_bulk_aux(state->mhandle,
                                    tx_bulk_entry->channel,
                                    req->tx_data,
                                    req->tx_data_bytes,
                                    chunk_size != 0 ? chunk_size : req->tx_data_bytes,
                                    &v[4].vec_base,
                                    &v[4].vec_len);

      // If user wants a callback when complete, we need bulk aux service to call us.
      success = single_service_queue_msgv( (VCHI_CONNECTION_SERVICE_HANDLE_T)state->bulk_aux_service,
                                           v, 5,
                                           tx_bulk_entry->flags & (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) ? VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE
                                                                                                                                            : VCHI_FLAGS_NONE,
                                                                                                                                            tx_bulk_entry );
      if ( success != 0 )
         break;

      tx_bulk_entry->flags |= VCHI_FLAGS_BULK_AUX_QUEUED;
      single_process_completed_tx_bulk( state );
      return 1;
   }

   return 0;
}


/* ----------------------------------------------------------------------
 * try to push a single (bulk) slot to the underlying hardware
 *
 * returns 0 if unable to send a slot; 1 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_pump_tx_bulk( SINGLE_CONNECTION_STATE_T *state )
{
   int i;

   single_dump_tx_bulk_inuse( state, "single_pump_tx_bulk" );

   for (i = 0; i < VCHI_MAX_BULK_TX_CHANNELS_PER_CONNECTION; i++)
   {
      MESSAGE_TX_CHANNEL_T channel = MESSAGE_TX_CHANNEL_BULK + (state->pump_tx_bulk_next_channel+i)%VCHI_MAX_BULK_TX_CHANNELS_PER_CONNECTION;
      TX_MSGINFO_T *tx_bulk_entry;
      TX_BULK_REQ_T *req = NULL;
      uint32_t len=0, max_chunk;

      // get the first entry in our bulk tx queue for this channel with data to go
      // note that due to need to preserve service order as well as channel order,
      // the queue may still contain entries that have been fully sent (eg via
      // bulk-aux-only)
      // (eg  X1:400 X0:0 Y1:400   X0 has finished, X1 and Y1 still have 400 bytes to go,
      //  so can't dispatch callbacks for X0 yet)
      for ( tx_bulk_entry = vchi_mqueue_peek( state->tx_bulk_queue, TX_BULK_INUSE, 0 ); tx_bulk_entry; tx_bulk_entry = tx_bulk_entry->next )
      {
         req = tx_bulk_entry->info;

         if ( tx_bulk_entry->flags & VCHI_FLAGS_BULK_DATA_QUEUED ||
              req->channel != channel )
            continue;

         len = req ? req->tx_data_bytes : 0;
         break;
      }
      
      if (tx_bulk_entry)
      {
         VCHI_MSG_FLAGS_T flags = VCHI_MSG_FLAGS_NONE;
         
         // limit chunk size
         max_chunk = state->mdriver->tx_bulk_chunk_size( state->mhandle, tx_bulk_entry->channel);
         if (max_chunk != 0 && len > max_chunk) len = max_chunk;

         // attempt to send this to the hardware. Note that we use a NULL handle for all parts except the last.
         if ( len > 0 )
         {
            bool_t last = (bool_t) ( len == req->tx_data_bytes && !req->append_dummy_data );
            if ( state->mdriver->tx_supports_terminate( state->mhandle, tx_bulk_entry->channel ) && last )
               flags |= VCHI_MSG_FLAGS_TERMINATE_DMA;

            if ( 0 != state->mdriver->send(state->mhandle,tx_bulk_entry->channel,req->tx_data,len,flags,last ? tx_bulk_entry : NULL) )
               continue; // didn't send (for instance, if there were no free slots), try next channel

            req->tx_data = (const uint8_t *) req->tx_data + len;
            req->tx_data_bytes -= len;
         }
         else
         {
            os_assert( req->append_dummy_data );
            // dummy data only used when driver doesn't support terminate, so don't set terminate flag
            if ( 0 != state->mdriver->send(state->mhandle,tx_bulk_entry->channel,dummy_data,req->append_dummy_data,flags,tx_bulk_entry) )
               continue; // didn't send (for instance, if there were no free slots), try next channel
            req->append_dummy_data = 0;
         }

         // successful
         if ( req->tx_data_bytes == 0 && !req->append_dummy_data )
            tx_bulk_entry->flags |= VCHI_FLAGS_BULK_DATA_QUEUED;

         // Round-robin through the channels. Has the effect of time-multiplexing links with
         // multiple logical channels.
         state->pump_tx_bulk_next_channel = (i+1) % VCHI_MAX_BULK_TX_CHANNELS_PER_CONNECTION;
         return 1;
      }
   }

   return 0;
}


/* ----------------------------------------------------------------------
 * should we expect a bulk auxiliary message for this bulk transfer?
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static bool_t
single_expect_bulk_aux( const SINGLE_CONNECTION_STATE_T *state,
                        const SERVICE_HANDLE_T *service )
{
   return (bool_t) (service->crc ||
          service->unaligned_bulk_rx);
}

/* ----------------------------------------------------------------------
 * figure out which section of a data block is suitable for data reception
 * -------------------------------------------------------------------- */
static void
single_get_bulk_rx_aligned_section( const SINGLE_CONNECTION_STATE_T *state,
                                    void *addr,
                                    uint32_t len,
                                    void **addr_out,
                                    uint32_t *len_out )
{
   // If smaller than the agreed threshold, will omit the bulk transfer AND the request.
   if (len < state->min_bulk_size)
      len = 0;
   // If the data would fit in the BULX message, and it's not Ccp, there's no point doing a
   // bulk transfer at all. Unless we've got ridiculously few, large slots, in which case we
   // lose lots of buffer space... Also check 16-bit limit on data_head_length.
   else if (len <= VCHI_MAX_MSG_SIZE - BULX_HEADER_SIZE &&
       len <= state->peer_total_msg_space / 16 &&
       len < 0x10000)
      len = 0;
   else
   {
      const int alignment = state->mdriver->rx_alignment( state->mhandle, MESSAGE_RX_CHANNEL_BULK );
      uint32_t head_size = 0, tail_size = 0;

      if (!IS_ALIGNED(addr, alignment))
      {
         head_size = alignment - MISALIGNMENT(addr, alignment);
         if (head_size > len)
            head_size = len, len = 0;
         else
            len -= head_size;
      }
      addr = (uint8_t*)addr + head_size;

      if (len % VCHI_BULK_GRANULARITY)
      {
         tail_size = len % VCHI_BULK_GRANULARITY;
         len -= tail_size;
      }
   }
   *addr_out = addr;
   *len_out = len;
}

static void
single_process_bulk_aux( SINGLE_CONNECTION_STATE_T *state )
{
   // Crucial thing here is to maintain completion order for each service.
   // Data can arrive over multiple channels - MeSSI data, BulkAux-only, and in
   // future, maybe some other interface. The completion order on these channels
   // is indeterminate.

   // We have two ways of ensuring order.
   // 1) If we don't get BulkAux for the service, we use completion order on the
   //    channel. That only works as long as one channel is in use at a time. This
   //    is true for this implementation, as we only have one channel.
#if VCHI_MAX_BULK_RX_CHANNELS > 1
#error "See comment above - check Symbian implementation for how to do this"
#endif
   // 2) If we do get BulkAux for the service, we use arrival order of BulkAux.
   //
   // There is a complication here if BulkAux could go on and off for a service.
   // We now rule this out - if we have the possibility of multiple channels
   // being used, and one of them needs BulkAux, BulkAux is always used. Note that
   // "BulkAux-only" is a virtual channel - it instantaneously completes the
   // zero-length data transfer. The corollary of this is that we can only do
   // BulkAux-only if we were going to be doing BulkAux anyway.

   // Case 1 is dealt with in slot_handling_func, case MESSAGE_EVENT_RX_BULK_COMPLETE.
   // This code deals with case 2. Arrived data descriptors are held in per-channel queues.
   // Unfortunately, bulk auxiliary messages are held in single queue, all services multiplexed,
   // which means we need to look-ahead through the queue to find the first BulkAux (hence next
   // transfer due to be processed) for a given service.

   for (;;)
   {
      bool_t processed_data = VC_FALSE;
      int nonempty = 0, i;

      // This is faster than looping over all services - just loops over services that
      // we know wwe have data for.
      for (i = MESSAGE_RX_CHANNEL_MESSAGE; i < MESSAGE_RX_CHANNEL_BULK + VCHI_MAX_BULK_RX_CHANNELS_PER_CONNECTION; i++)
      {
         RX_BULK_SLOTINFO_T *bulk = vchi_mqueue_peek( state->rx_bulk_queue, RX_BULK_DATA_ARRIVED(i), 0);
         if (bulk)
         {
            nonempty++;
            // this will process the first bulk aux for the service at the head of this channel -
            // this will actually look at the channel designated by the bulk aux - it may remove
            // this bulk, or one from another channel
            if (single_process_bulk_aux_for_service( state, bulk->service ))
               processed_data = VC_TRUE;
         }
      }
      if (!processed_data)
         break;
   }
}

/* ----------------------------------------------------------------------
 * Process a BULX message (if any) for a service. Returns VC_TRUE if it
 * matched up a BULX and an arrived data transfer.
 * -------------------------------------------------------------------- */
static bool_t
single_process_bulk_aux_for_service( SINGLE_CONNECTION_STATE_T *state, SERVICE_HANDLE_T *service )
{
   VCHI_MSG_ITER_T it;
   int32_t success;

   os_logging_message("single_process_bulk_aux_for_service[%c%c%c%c]",FOURCC_TO_CHAR(service->service_id));

   if (single_service_look_ahead_msg((VCHI_CONNECTION_SERVICE_HANDLE_T)state->bulk_aux_service, &it, VCHI_FLAGS_NONE))
      return VC_FALSE;

   for (;;)
   {
      void *ptr;
      uint8_t *aux;
      uint32_t auxlen;
      fourcc_t aux_service;
      uint32_t aux_size;
      uint32_t aux_crc;
      uint32_t aux_data_size;
      int16_t  aux_data_shift;
      MESSAGE_RX_CHANNEL_T aux_channel;
      uint16_t aux_head_size;
      uint16_t aux_tail_size;

      if (single_msg_iter_next((VCHI_CONNECTION_SERVICE_HANDLE_T)state->bulk_aux_service, &it, &ptr, &auxlen))
         return VC_FALSE;

      os_assert(auxlen >= BULX_HEADER_SIZE);
      aux = ptr;
      aux_service    = vchi_readbuf_fourcc(&aux[BULX_SERVICE_OFFSET]);
      aux_size       = vchi_readbuf_uint32(&aux[BULX_SIZE_OFFSET]);
      aux_crc        = vchi_readbuf_uint32(&aux[BULX_CRC_OFFSET]);
      aux_data_size  = vchi_readbuf_uint32(&aux[BULX_DATA_SIZE_OFFSET]);
      aux_data_shift = vchi_readbuf_int16(&aux[BULX_DATA_SHIFT_OFFSET]);
      aux_channel    = aux[BULX_CHANNEL_OFFSET];
      aux_head_size  = vchi_readbuf_uint16(&aux[BULX_HEAD_SIZE_OFFSET]);
      aux_tail_size  = vchi_readbuf_uint16(&aux[BULX_TAIL_SIZE_OFFSET]);

      if (aux_service == service->service_id)
      {
         // Found the first BULX msg for this service.
         RX_BULK_SLOTINFO_T *bulk, *b;
         void *data_addr;
         uint32_t data_len;

         os_assert(aux_channel < 1+VCHI_MAX_BULK_RX_CHANNELS_PER_CONNECTION );

         bulk = vchi_mqueue_peek( state->rx_bulk_queue, RX_BULK_DATA_ARRIVED(aux_channel), 0);
         if (bulk == NULL)
            return VC_FALSE;

         if (bulk->service != service)
            return VC_FALSE;

         if (service->unaligned_bulk_rx)
         {
            // Figure out what their actual data channel transfer was
            uint32_t max_len;
            single_get_bulk_rx_aligned_section(state, bulk->addr, bulk->len, &data_addr, &max_len);
            data_len = aux_data_size;
            os_assert(data_len <= max_len); // Invalid BULX data size
         }
         else
         {
            data_addr = bulk->addr;
            data_len = bulk->len;
         }
         os_logging_message("BULX: bulk->addr     =0x%x", bulk->addr);
         os_logging_message("BULX: data_addr      =0x%x", data_addr);
         os_logging_message("BULX: bulk->len      =0x%x", bulk->len);
         os_logging_message("BULX: data_len       =0x%x", data_len);
         os_logging_message("BULX: aux_data_size  =0x%x", aux_data_size);
         os_logging_message("BULX: aux_data_shift =%d",   aux_data_shift);
         os_logging_message("BULX: aux_head_size  =0x%x", aux_head_size);
         os_logging_message("BULX: aux_tail_size  =0x%x", aux_tail_size);

         os_assert ( aux_size == bulk->len ); // Mismatched sizes

         if (service->unaligned_bulk_rx)
         {
            // Shift the transfer into position, patch in head and tail
            os_assert((uint8_t*)data_addr + aux_data_shift >= (uint8_t*)bulk->addr &&
                      (uint8_t*)data_addr + aux_data_shift + data_len <= (uint8_t*)bulk->addr + bulk->len); // Invalid BULX data shift
            os_assert(aux_head_size + aux_tail_size <= bulk->len); // Invalid BULX head and tail size
            if (data_len != 0)
            {
               os_assert((uint8_t*)bulk->addr + aux_head_size >= (uint8_t*)data_addr + aux_data_shift); // Missing BULX head coverage
               os_assert((uint8_t*)bulk->addr + bulk->len - aux_tail_size <= (uint8_t*)data_addr + aux_data_shift + data_len); // Missing BULX tail coverage
            }
            else
            {
               os_assert(aux_head_size+aux_tail_size == bulk->len); // Insufficient BULX-only coverage
            }
            if (data_len != 0 && aux_data_shift != 0)
               memmove((uint8_t*)data_addr + aux_data_shift, data_addr, data_len);
            if (aux_head_size != 0)
               memcpy(bulk->addr, aux+BULX_HEADER_SIZE, aux_head_size);
            if (aux_tail_size != 0)
               memcpy((uint8_t*)bulk->addr + bulk->len - aux_tail_size, aux+BULX_HEADER_SIZE+aux_head_size, aux_tail_size);
         }

         if (service->crc)
            os_assert(aux_crc == single_compute_crc(bulk->addr, bulk->len));

         success = single_msg_iter_remove((VCHI_CONNECTION_SERVICE_HANDLE_T)state->bulk_aux_service, &it);
         os_assert(success == 0);
         b = vchi_mqueue_get( state->rx_bulk_queue, RX_BULK_DATA_ARRIVED(aux_channel), 0 );
         os_assert(bulk == b);
         single_bulk_rx_complete(state, bulk);
         return VC_TRUE;
      }
   }
}

/* ----------------------------------------------------------------------
 * queue a receive slot for bulk data
 *
 * returns: 0 on success; non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_bulk_queue_receive( VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                           void *data_dst,
                           uint32_t data_size,
                           VCHI_FLAGS_T flags,
                           void *bulk_handle )
{
   RX_BULK_SLOTINFO_T *entry;
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;
   OS_SEMAPHORE_T blocking_sem;
   const int block = (flags & (VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)) ? 1 : 0;

   if (flags & (VCHI_FLAGS_ALLOW_PARTIAL|
                VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|
                VCHI_FLAGS_CALLBACK_WHEN_DATA_READ))
      return -2;
   
   if ( !service->want_unaligned_bulk_rx && // check against want_ flag to make sure they're using API properly
        !state->mdriver->buffer_aligned(state->mhandle,0,0,data_dst,data_size) )
   {
      return -2;
   }

   // find a free entry in the bulk rx queue
   entry = vchi_mqueue_get( state->rx_bulk_queue, RX_BULK_FREE, block );
   if ( !entry )
      return -1;

   // now we have a valid entry fill in the information
   entry->len     = data_size;
   entry->addr    = data_dst;
   entry->service = service;
   entry->flags   = flags;
   entry->handle  = bulk_handle;

   // setup for blocking behaviour if required
   if ( flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE ) {
      (void) os_semaphore_create( &blocking_sem, OS_SEMAPHORE_TYPE_SUSPEND );
      (void) os_semaphore_obtain( &blocking_sem );
      entry->blocking = &blocking_sem;
   }

   // add to the pending queue
   vchi_mqueue_put( state->rx_bulk_queue, RX_BULK_PENDING, entry );
   //const fourcc_t sid = service->service_id;
   //os_logging_message( "rx_bulk_queue 0>1 (%p,%d,%d,%c%c%c%c)", entry->addr, entry->len, entry->flags,
   //FOURCC_TO_CHAR(sid) );
   //vchi_mqueue_debug( state->rx_bulk_queue );

   // after we put it on the pending queue, we can no longer refer to entry;
   // it may have been put back onto the free queue and re-used by another task
   entry = NULL;

   single_event_signal( state, SINGLE_EVENTBIT_RX_BULK_SLOT_READY );

   // if requested or because we need to copy the transfer into the proper buffer
   // we will now wait for the signal that the transfer has completed
   if ( flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE ) {
      single_unlock( state );
      (void) os_semaphore_obtain( &blocking_sem );
#ifndef VCHI_ELIDE_BLOCK_EXIT_LOCK
      single_lock( state ); // would be nice to short-circuit this - vchi.c will unlock again immediately
#endif
      (void) os_semaphore_destroy( &blocking_sem );
   }
   return 0;
}

/* ----------------------------------------------------------------------
 * add bulk receive transfers to the dma fifo, if there is room
 * -------------------------------------------------------------------- */
static void
single_pump_rx_bulk( SINGLE_CONNECTION_STATE_T *state )
{
   for (;;) {
      RX_BULK_SLOTINFO_T *entry;
      SERVICE_HANDLE_T *service;
      void *addr;
      uint32_t len;

      // first, see if there is anything pending
      entry = vchi_mqueue_peek( state->rx_bulk_queue, RX_BULK_PENDING, 0 );
      if ( !entry )
         break;

      service = entry->service;
      if ( service->unaligned_bulk_rx )
         single_get_bulk_rx_aligned_section( state, entry->addr, entry->len, &addr, &len );
      else
         addr = entry->addr, len = entry->len;

      if ( len != 0 ) {
         // can we add it to hardware?
         if ( 0 != state->mdriver->add_bulk_rx(state->mhandle, addr, len, entry) )
            break;
      }

      // all good... but before the other side will start transmitting,
      // we need to advertise the fact we added this to our hardware
      // fifo... advertising is done from within pump_tx()
      (void) vchi_mqueue_move( state->rx_bulk_queue, RX_BULK_PENDING, RX_BULK_UNREQUESTED );

      single_event_signal( state, SINGLE_EVENTBIT_TX_SLOT_READY );

   }
}

/* ----------------------------------------------------------------------
 * deal with completion of a bulk reception. Called from slot handling
 * task, either after getting a data transfer (when not expecting bulk aux)
 * or after getting the bulk aux.
 * -------------------------------------------------------------------- */
static void
single_bulk_rx_complete( SINGLE_CONNECTION_STATE_T *state, RX_BULK_SLOTINFO_T *s )
{
   // callbacks come next
   if ( s->flags & VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE )
      single_issue_callback( s->service, VCHI_CALLBACK_BULK_RECEIVED, s->handle );

   // wakeup any waiters
   if ( s->flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE )
     (void) os_semaphore_release( s->blocking );

   // remove the top slot from the DMA queue
   vchi_mqueue_put_head( state->rx_bulk_queue, RX_BULK_FREE, s );
}



/* ----------------------------------------------------------------------
 * fetch handle for the given service (on the given message_driver)
 *
 * if the service exists, return the handle
 *
 * if the given service has not yet been created, return pointer
 * to a new handle (or NULL if no free handles)
 *
 * must be called with semaphore held
 * -------------------------------------------------------------------- */
static SERVICE_HANDLE_T *
single_find_free_service( fourcc_t service_id, SERVICE_TYPE_T client_or_server, SINGLE_CONNECTION_STATE_T *state )
{
   int i;
   SERVICE_HANDLE_T *address = NULL;

   for (i=0; i<VCHI_MAX_SERVICES_PER_CONNECTION; i++)
   {
      // record the first free slot we find
      if ( address == NULL )
      {
         if ( state->services[i].type == SINGLE_UNUSED )
         {
            address = (SERVICE_HANDLE_T *)&state->services[i];
            if ( i > state->highest_service_index )
               state->highest_service_index = i;
         }
      }
      // look for the slot that is for this service
      if ( state->services[i].service_id == service_id /*&& state->services[i].type == client_or_server*/ )
      {
         // Bad news this 4cc is already in use
         address = NULL;
         os_assert(0);
         break;
      }
   }

   os_assert(address != NULL);
   // return the address, which will be NULL if there has been any problem
   return( address );
}


/* ----------------------------------------------------------------------
 * send an event to the slot handling task
 * -------------------------------------------------------------------- */
static void
single_event_signal( SINGLE_CONNECTION_STATE_T *state, int event )
{
   int32_t status;
#ifdef VCHI_MULTIPLE_HANDLER_THREADS
   status = os_eventgroup_signal( &state->slot_event_group, (int32_t) event );
#else
   status = os_eventgroup_signal( &slot_event_group, (int32_t) (event + (NUMBER_OF_EVENTS*state->connection_number)) );
#endif
   os_assert(status == 0);
}

/* ----------------------------------------------------------------------
 * 'single' connection task
 *
 * handles the following:
 *   - message driver events
 *   - requests for message/bulk data to be transmitted
 * -------------------------------------------------------------------- */
static void
slot_handling_func( unsigned argc, void *argv )
{
   int i = 0;
   MESSAGE_EVENT_T event;
   
#ifdef VCHI_MULTIPLE_HANDLER_THREADS
   SINGLE_CONNECTION_STATE_T *state = argv;
   os_logging_message("slot_handling_func, argv=%x, connection=%d", argv, ((SINGLE_CONNECTION_STATE_T*)argv)->connection_number);
#endif
   
   for (;;) {
      // wait for any events
      uint32_t events;
      int32_t status;
#ifdef VCHI_MULTIPLE_HANDLER_THREADS
      status = os_eventgroup_retrieve( &state->slot_event_group, &events );
#else
      status = os_eventgroup_retrieve( &slot_event_group, &events );
#endif
      os_assert( status == 0 );

      //os_logging_message( "slot_handling_task: got events 0x%X",events);

#ifdef VCHI_MULTIPLE_HANDLER_THREADS
      {
#else
      for (i=0; i< (int)num_connections; i++) {

         SINGLE_CONNECTION_STATE_T *state = &single_state[i];
#endif
         int pump_transmit = 0;

         if (!(events & (SINGLE_EVENTMASK_ANY << (i*NUMBER_OF_EVENTS))))
            continue;
         
#ifdef VCHI_COARSE_LOCKING
         os_semaphore_obtain(&state->connection->sem);
#endif

         // We have had an interrupt from the low level driver
         if(events & (SINGLE_EVENTMASK_EVENT_PRESENT << (i*NUMBER_OF_EVENTS))) {
            int events_to_process = VC_TRUE;
            int message_received = VC_FALSE;
            do {
               event.type = MESSAGE_EVENT_NONE;
               state->mdriver->next_event( state->mhandle, &event );
               switch( event.type ) {

               default:
                  os_assert(0);
                  break;

               case MESSAGE_EVENT_NOP:
                  // call again
                  break;

               case MESSAGE_EVENT_NONE:
                  //os_logging_message( "slot_handling_task: MESSAGE_EVENT_NONE");
                  // remove the event
                  events_to_process = VC_FALSE;
                  break;

               case MESSAGE_EVENT_MESSAGE:
                  {
                     //os_logging_message( "slot_handling_task: MESSAGE_EVENT_MESSAGE");

                     // keep track of how much room is being reported by the peer
                     state->peer_slot_count += event.message.slot_delta;

                     single_message_received_action( state,
                                                     event.message.service,
                                                     event.message.slot,
                                                     event.message.addr,
                                                     event.message.len,
                                                     event.message.tx_timestamp,
                                                     event.message.rx_timestamp );
                     message_received = VC_TRUE;

                     if (event.message.slot_delta)
                        pump_transmit = 1;
                  }
                  break;

               case MESSAGE_EVENT_SLOT_COMPLETE:
                  //os_logging_message( "slot_handling_task: MESSAGE_EVENT_SLOT_COMPLETE");
                  single_rx_slot_completed_action( state );
                  single_add_slots_to_hardware( state );
                  if (state->rx_slot_delta)
                     pump_transmit = 1;
                  break;
               case MESSAGE_EVENT_RX_BULK_COMPLETE:
                  {
                     RX_BULK_SLOTINFO_T *s = vchi_mqueue_get( state->rx_bulk_queue, RX_BULK_REQUESTED(MESSAGE_RX_CHANNEL_BULK), 0 );
                     SERVICE_HANDLE_T *service = s->service;
                     os_assert( s && s == event.rx_bulk );
                     os_logging_message("slot_handling_task: MESSAGE_EVENT_RX_BULK_COMPLETE");

                     if (single_expect_bulk_aux( state, service ))
                     {
                        vchi_mqueue_put( state->rx_bulk_queue, RX_BULK_DATA_ARRIVED(MESSAGE_RX_CHANNEL_BULK), s );
                        single_process_bulk_aux( state );
                     }
                     else
                        single_bulk_rx_complete( state, s );

                     // now see if there is anything further to add to the DMA FIFO
                     single_pump_rx_bulk( state );
                  }
                  break;
               case MESSAGE_EVENT_TX_COMPLETE:
                  // should try and queue the next entry into the FIFO
                  pump_transmit = 1;
                  if ( event.tx_channel == MESSAGE_TX_CHANNEL_MESSAGE )
                  {
                     TX_MSG_SLOTINFO_T *tx = event.tx_handle;
                     TX_MSGINFO_T *s = NULL;
                     os_logging_message( "slot_handling_task: MESSAGE_EVENT_TX_COMPLETE(message) {%d:%p:%d}", event.tx_channel, event.message.addr, event.message.len );
                     os_assert( (uint8_t*)event.message.addr >= tx->addr &&
                                (uint8_t*)event.message.addr + event.message.len <= tx->addr + tx->len );

                     // A transmit of a slot (or section of slot) has completed.
                     // Find messages that were contained within it.
                     for (;;)
                     {
                        s = vchi_mqueue_peek( state->tx_msg_queue, TX_MSG_INUSE, 0 );
                        if (!s) break;
                        os_logging_message( "slot_handling_task: check {%p:%d}", s->addr, s->len );
                        if ( (uint8_t*)s->addr >= (uint8_t*)event.message.addr &&
                             (uint8_t*)s->addr < (uint8_t*)event.message.addr + event.message.len )
                        {
                           os_logging_message( "matched" );
                           os_assert( (uint8_t*)s->addr + s->len <= (uint8_t*)event.message.addr + event.message.len );

                           // callbacks come next
                           if ( s->flags & VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE )
                              single_issue_callback( s->service, VCHI_CALLBACK_MSG_SENT, s->handle );

                           // wakeup any waiters
                           if ( s->flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE )
                             (void) os_semaphore_release( s->blocking );

                           // and we can put it back to the free queue
                           (void) vchi_mqueue_move_head( state->tx_msg_queue, TX_MSG_INUSE, TX_MSG_FREE );
                        }
                        else
                           break;
                     }
                     // Decrement the "transmits in progress" count. If that's now zero, and the
                     // slot has been fully read, and the slot is on the in-use queue, then we've finished
                     // with it.
                     if ( --tx->tx_count == 0 && tx->queue == TX_SLOT_INUSE && tx->read_ptr == WPTR(tx->write_ptr) )
                     {
                        TX_MSG_SLOTINFO_T *t = vchi_mqueue_get( state->tx_slots, TX_SLOT_INUSE, 0);
                        // Sanity check: we're taking the right slot
                        os_assert( t == tx  );
                        // Sanity check: if the head of the message queue is inside the slot that's
                        // been retired, we've gone badly wrong.
                        if (s)
                           os_assert( !((uint8_t*)s->addr >= t->addr && (uint8_t*)s->addr < t->addr + t->len) );
                        single_return_free_tx_slot( state, t );
                     }
                  }
                  else // bulk
                  {
                     TX_MSGINFO_T *tx = event.tx_handle;
                     os_logging_message( "slot_handling_task: MESSAGE_EVENT_TX_COMPLETE(bulk) {%d:%p:%d}", event.tx_channel, event.message.addr, event.message.len );
                     // should try and queue the next entry into the FIFO
                     pump_transmit = 1;
                     
                     if ( event.tx_handle == NULL ) // it's a non-final segment of a transmission
                        break;

                     os_logging_message( "slot_handling_task: MESSAGE_EVENT_TX_BULK_COMPLETE");

                     tx->flags |= VCHI_FLAGS_BULK_DATA_COMPLETE;
                     single_process_completed_tx_bulk( state );
                  }
                  break;
               case MESSAGE_EVENT_RX_BULK_PAUSED:
                  os_logging_message("slot_handling_task: rx_bulk_paused" );
                  break;
               case MESSAGE_EVENT_MSG_DISCARDED:
                  os_logging_message("slot_handling_task: msg_discarded" );
                  break;
               }
            } while( events_to_process );
            
#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
            if (message_received)
               single_issue_msg_available_callbacks( state );
#endif
         }

         // a message has been dequeued that has freed a rx msg slot so we will now so if there is room in the FIFO to add it
         if ( events & (SINGLE_EVENTMASK_RX_SLOT_AVAILABLE_EVENT << (i*NUMBER_OF_EVENTS)) ) {
            //os_logging_message( "slot_handling_task: add rx slots to hardware");
            single_add_slots_to_hardware( state );
            if (state->rx_slot_delta)
               pump_transmit = 1;
         }

         // we have added a tx request
         if ( events & (SINGLE_EVENTMASK_TX_BULK_EVENT << (i*NUMBER_OF_EVENTS)) ) {
            //os_logging_message( "slot_handling_task: marshall TX... calling single_marshall_tx_bulk");
            single_marshall_tx_bulk( state, 0 );
         }

         // we have a request to queue a rx bulk
         if ( events & (SINGLE_EVENTMASK_RX_BULK_SLOT_READY << (i*NUMBER_OF_EVENTS)) ) {
            //os_logging_message( "slot_handling_task: queue rx bulk... calling pump_rx_bulk");
            single_pump_rx_bulk( state );
         }

         // we have a request to transmit
         if ( events & (SINGLE_EVENTMASK_TX_SLOT_READY << (i*NUMBER_OF_EVENTS)) ) {
            //os_logging_message( "slot_handling_task: queue TX... calling pump_transmit");
            pump_transmit = 1;
         }

         if ( events & (SINGLE_EVENTMASK_RX_XON_XOFF_EVENT << (i*NUMBER_OF_EVENTS)) ) {
           (void) single_send_xon_xoff_messages( state );
         }

         if ( pump_transmit )
            single_pump_transmit( state );

#ifdef VCHI_COARSE_LOCKING
         os_semaphore_release(&state->connection->sem);
#endif
      }
   }
}

/* ----------------------------------------------------------------------
 * called from our task whenever a message has been found in a slot
 * -------------------------------------------------------------------- */
static void
single_message_received_action( SINGLE_CONNECTION_STATE_T *state,
                                fourcc_t service_id,
                                RX_MSG_SLOTINFO_T *slot,
                                void *addr,
                                uint32_t len,
                                uint32_t tx_timestamp,
                                uint32_t rx_timestamp )
{
   SERVICE_HANDLE_T *service;
   bool_t check_xon_xoff = VC_FALSE;
#ifndef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
   bool_t issue_callback = VC_FALSE;
#endif
   
   os_logging_message("Msg for %c%c%c%c:%d", FOURCC_TO_CHAR(service_id), len);

   // see if we know about this service
   service = single_fourcc_to_service( state, service_id );

   // check to see if we are supposed to handle this service
   if ( service )
   {
      RX_MESSAGE_INFO_T *msg;
#if !defined VCHI_COARSE_LOCKING || defined VCHI_RX_NANOLOCKS
      (void) os_semaphore_obtain(&state->rxq_semaphore);
#endif

      msg = vchi_mqueue_get( state->rx_msg_queue, MESSAGE_FREE, 0 );
      os_assert( msg != NULL );
      
      msg->addr         = addr;
      msg->len          = len;
      msg->slot         = slot;
      msg->tx_timestamp = tx_timestamp;
      msg->rx_timestamp = rx_timestamp;
      
      // if we are making the 0->1 transition then we have blocked another slot
      if(++msg->slot->ref_count[service->index] == 1) {
         service->rx_slots_in_use++;
         check_xon_xoff = VC_TRUE;
      }

      // if this service has been opened then there will be a valid callback - so run it
#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
      service->rx_callback_pending = VC_TRUE;
#else
      issue_callback = VC_TRUE;
#endif
      // move the message onto the end of service's linked list of messages
      // (If no dequeue locks, it could be yanked atomically here).
      vchi_mqueue_put( state->rx_msg_queue, MESSAGE_SERVICE(service->index), msg );
      
#if !defined VCHI_COARSE_LOCKING || defined VCHI_RX_NANOLOCKS
      (void) os_semaphore_release(&state->rxq_semaphore);
#endif
   }
   else
   {
      // we don't have a service for this message so we will just throw it away

      // don't throw an error if there are no services (KJB - why not?)
      if(state->highest_service_index >= 0) {
         os_logging_message( "message for unknown service %c%c%c%c", FOURCC_TO_CHAR(service_id) );
         // it is possible we may receive the initial connect message from the other side before our CTRL service is up and running
         if ( service_id != MAKE_FOURCC("CTRL") )
            os_assert (0);  /// BAD BAD BAD - message for service we don't recognise
      }
   }
   
#ifndef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
   if (issue_callback)
      single_issue_callback( service, VCHI_CALLBACK_MSG_AVAILABLE, NULL );
#endif
      
   if (check_xon_xoff)
      (void) single_send_xon_xoff_message(service);
}

/* ----------------------------------------------------------------------
 * "does all the housekeeping required when a slot completes"
 * -------------------------------------------------------------------- */
static void
single_rx_slot_completed_action( SINGLE_CONNECTION_STATE_T *state )
{
   // expect this slot to be first in 'active' list
   RX_MSG_SLOTINFO_T *s = vchi_mqueue_get( state->rx_slots, RX_SLOT_INUSE, 0 );

   state->rx_slot_count--;

   //os_logging_message( "single_rx_slot_completed_action : slot has completed %p (slot_no %d) Messages %d",s,s->slot_no,s->messages);

#ifndef VCHI_COARSE_LOCKING
   (void) os_semaphore_obtain( &s->sem );
#endif
   // flag that the slot is no longer in the DMA FIFO
   s->active = 0;

   if(s->write_ptr <= s->len && s->msgs_parsed > s->msgs_released)
   {
      // move completed slot to 'pending' list
      vchi_mqueue_put( state->rx_slots, RX_SLOT_PENDING, s );
   }
   else
   {
      // invalid state or the messages have already been dealt with so push it onto free queue
      vchi_mqueue_put_head( state->rx_slots, RX_SLOT_FREE, s );
      //os_logging_message( "single_rx_slot_completed_action: Messages already dealt with");
   }
#ifndef VCHI_COARSE_LOCKING
   (void) os_semaphore_release( &s->sem );
#endif
}

/* ----------------------------------------------------------------------
 * callback from the message driver
 *
 * this means there are events pending in the message driver
 * -------------------------------------------------------------------- */
static void
single_event_completed ( void *_state )
{
   SINGLE_CONNECTION_STATE_T *state = _state;

   single_event_signal( state, SINGLE_EVENTBIT_EVENT_PRESENT );
}

/* ----------------------------------------------------------------------
 * called by the control service when a CONNECT message is received
 * -------------------------------------------------------------------- */
static void
single_connection_info( VCHI_CONNECTION_STATE_T *state_handle,
                        uint32_t protocol_version,
                        uint32_t slot_size,
                        uint32_t num_slots,
                        uint32_t min_bulk_size )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;

   // Tricky bit of fudgery. It's possible to get confused on initial slot count, due to
   // ordering of processing of connect handshake. To avoid this, we arrange for initial
   // connect messages (not replies) to never be accounted for - either they get dropped
   // because the peer isn't up, or the peer gets them, but doesn't include them in delta
   // indications. Taking one off delta here cancels out the delta indication from freeing
   // the slot containing the connect.
   if (!(protocol_version & PROTOCOL_REPLY_FLAG))
      state->rx_slot_delta--;

   state->suspended = VC_FALSE;
   state->peer_slot_count = num_slots;
   state->peer_total_msg_space = num_slots * slot_size;
   state->min_bulk_size = OS_MIN(min_bulk_size, VCHI_MIN_BULK_SIZE);

   // because we now form messages in state->slot_size-sized slots, we're doomed
   // unless that's the size they're expecting
   os_assert(slot_size == state->tx_slot_size);
}

/* ----------------------------------------------------------------------
 * called by the control service when a DISCONNECT message is received
 * -------------------------------------------------------------------- */
static void
single_disconnect( VCHI_CONNECTION_STATE_T *state_handle, uint32_t flags )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   
   os_logging_message("Disconnect(%d) received", flags);
   if (flags & 1)
   {
      // should wait for idle. How? Assert for now.
      TX_MSG_SLOTINFO_T *slot = vchi_mqueue_peek( state->tx_slots, TX_SLOT_PENDING, 0 );
      os_assert(slot == NULL || (WPTR(slot->write_ptr) == slot->read_ptr && slot->next == NULL));
      slot = vchi_mqueue_peek( state->tx_slots, TX_SLOT_INUSE, 0 );
      os_assert(slot == NULL);

      state->suspended = VC_TRUE;
      if (os_suspend() == 0)
      {
         slot = vchi_mqueue_get( state->tx_slots, TX_SLOT_PENDING, 0 );
         if (slot)
            single_return_free_tx_slot( state, slot );
         
         // VMCS_APP will call vchi_connect, which will clear the "suspended" flag
         // But first, need to reinit message driver
         state->mdriver->resumed( state->mhandle );
         // all already used slots have now been dumped
         while ( state->rx_slot_count )
            single_rx_slot_completed_action( state );

         single_prepare_initial_rx( state );
      }
      else
      {
         // send "failed" message here
      }
   }
   else
   {
      os_halt();
   }
}

/* ----------------------------------------------------------------------
 * called by the control service when an xon/xoff message is received
 * -------------------------------------------------------------------- */
static void
single_flow_control( VCHI_CONNECTION_STATE_T *state_handle, fourcc_t service_id, int32_t xoff )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   SERVICE_HANDLE_T *service = single_fourcc_to_service( state, service_id );
   int32_t old_state;

   if ( !service )
   {
      os_assert(0);
      return;
   }

#ifndef VCHI_COARSE_LOCKING
   (void) os_semaphore_obtain(&service->semaphore);
#endif
   old_state = service->tx_xoff_in_progress;
   os_logging_message("%s received", xoff ? "XOFF" : "XON");
   service->tx_xoff_in_progress = xoff;
   if (old_state != xoff && !xoff)
   {
      (void) os_cond_broadcast(&service->tx_xon, VC_TRUE);
      // XXX - this needs to be here in absence of proper space available callback implementation. Yuck.
      // I can only imagine how clients feel. This will wake up pumping task which will try again on bulk aux.
      if (service == state->bulk_aux_service)
         single_event_signal( state, SINGLE_EVENTBIT_TX_SLOT_READY );
   }
#ifndef VCHI_COARSE_LOCKING
   (void) os_semaphore_release(&service->semaphore);
#endif
}

/* ----------------------------------------------------------------------
 * called by the control service when a POWER_CONTROL message is received
 * -------------------------------------------------------------------- */
static void
single_power_control( VCHI_CONNECTION_STATE_T *state_handle, MESSAGE_TX_CHANNEL_T channel, bool_t enable )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   
   state->mdriver->power_control( state->mhandle, channel, enable );
}

/* ----------------------------------------------------------------------
 * called by the control service when an server_available_reply
 * message is received
 * -------------------------------------------------------------------- */
static void
single_server_available_reply( VCHI_CONNECTION_STATE_T *state_handle, fourcc_t service_id, uint32_t flags )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   SERVICE_HANDLE_T *service = single_fourcc_to_service( state, service_id );

   // check to see if we are supposed to handle this service
   if ( service && service->type == SINGLE_CLIENT && !service->connected )
   {
      int32_t success;
      if (flags & AVAILABLE)
      {
         service->crc = (bool_t) ( (flags & WANT_CRC) || single_want_crc(service) );
         service->unaligned_bulk_rx = (bool_t) ( (flags & WANT_UNALIGNED_BULK_TX) || service->want_unaligned_bulk_rx );
         service->unaligned_bulk_tx = (bool_t) ( (flags & WANT_UNALIGNED_BULK_RX) || service->want_unaligned_bulk_tx );
         service->always_tx_bulk_aux = (flags & WANT_BULK_AUX_ALWAYS) ? VC_TRUE : VC_FALSE;
         service->connected = VC_TRUE;
      }
      success = os_semaphore_release(&service->connect_semaphore);
      os_assert(success == 0);
   }
}

/* ----------------------------------------------------------------------
 * called by the bulk auxiliary service when messages have been received
 * -------------------------------------------------------------------- */
static void
single_bulk_aux_received( VCHI_CONNECTION_STATE_T *state_handle )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   single_process_bulk_aux( state );
}

/* ----------------------------------------------------------------------
 * called by the bulk auxiliary service when a message has been transmitted
 * -------------------------------------------------------------------- */
static void
single_bulk_aux_transmitted( VCHI_CONNECTION_STATE_T *state_handle,
                             void *msg_handle )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   TX_MSGINFO_T *bulk = msg_handle;

   bulk->flags |= VCHI_FLAGS_BULK_AUX_COMPLETE;
   single_process_completed_tx_bulk( state );
}

/* ----------------------------------------------------------------------
 * the other side will send CTRL messages to indicate when it has
 * queued a bulk receive slot.
 *
 * this routine is called whenever we receive such a message
 * -------------------------------------------------------------------- */
static void
single_rx_bulk_buffer_added( VCHI_CONNECTION_STATE_T *state_handle,
                             fourcc_t service_id,
                             uint32_t length,
                             MESSAGE_TX_CHANNEL_T channel,
                             uint32_t channel_params,
                             uint32_t data_length,
                             uint32_t data_offset )
{
   SINGLE_CONNECTION_STATE_T *state = (SINGLE_CONNECTION_STATE_T *)state_handle;
   SERVICE_HANDLE_T *service;
   TX_BULK_REQ_T *req;

   service = single_fourcc_to_service( state, service_id );
   if ( !service )
   {
      os_assert(0); // unknown service ID
      return;
   }

   req = vchi_mqueue_get( state->tx_bulk_req_queue, TX_BULK_REQ_FREE, 0 );
   os_assert( req != NULL ); // increase VCHI_MAX_PEER_BULK_REQUESTS if this goes off

   // record the supplied information
   req->service_id = service_id;
   req->length = length;
   // May ultimately need to pass channel_params to message driver at time of queueing,
   // in which case it should be stored in req here for later use. But for now, we just need to
   // note the channel.
   req->channel = channel;
   req->rx_data_bytes = data_length;
   req->rx_data_offset = data_offset;
   if (data_length == 0)
      os_assert(req->channel == MESSAGE_TX_CHANNEL_MESSAGE);
   else
      os_assert(req->channel >= MESSAGE_TX_CHANNEL_BULK && req->channel < MESSAGE_TX_CHANNEL_BULK + VCHI_MAX_BULK_TX_CHANNELS_PER_CONNECTION);

   // move to the pending queue
   vchi_mqueue_put( state->tx_bulk_req_queue, TX_BULK_REQ_PENDING, req );

   // try to find a match
   single_marshall_tx_bulk( state, req->service_id );
}

/* ----------------------------------------------------------------------
 * reports whether or not the requested 4cc is a server on this
 * connection. Also reports our desired connection flags.
 *
 * returns >=0 if server found; -1 otherwise
 * -------------------------------------------------------------------- */
static int32_t
single_server_present( VCHI_CONNECTION_STATE_T *state, fourcc_t service_id, int32_t peer_flags )
{
   SINGLE_CONNECTION_STATE_T *single_state = (SINGLE_CONNECTION_STATE_T *)state;
   SERVICE_HANDLE_T *s = single_fourcc_to_service( single_state, service_id );

   if ( s && s->type == SINGLE_SERVER )
   {
      int32_t our_flags =
         (single_want_crc(s) ? WANT_CRC : 0) |
         (s->want_unaligned_bulk_rx ? WANT_UNALIGNED_BULK_RX : 0) |
         (s->want_unaligned_bulk_tx ? WANT_UNALIGNED_BULK_TX : 0);
      // Update the service flags.
      s->crc = (bool_t) ( (peer_flags & WANT_CRC) || single_want_crc(s) );
      s->unaligned_bulk_rx = (bool_t) ( (peer_flags & WANT_UNALIGNED_BULK_TX) || s->want_unaligned_bulk_rx );
      s->unaligned_bulk_tx = (bool_t) ( (peer_flags & WANT_UNALIGNED_BULK_RX) || s->want_unaligned_bulk_tx );
      s->always_tx_bulk_aux = (peer_flags & WANT_BULK_AUX_ALWAYS) ? VC_TRUE : VC_FALSE;
      return our_flags;
   }

   return -1;
}

/* ----------------------------------------------------------------------
 * returns the total number of RX slots available
 * -------------------------------------------------------------------- */
static int
single_connection_rx_slots_available( const VCHI_CONNECTION_STATE_T *state )
{
   const SINGLE_CONNECTION_STATE_T *single_state = (SINGLE_CONNECTION_STATE_T *)state;

   return (int) single_state->rx_slot_count;
}

/* ----------------------------------------------------------------------
 * returns the RX slot size
 * -------------------------------------------------------------------- */
static uint32_t
single_connection_rx_slot_size( const VCHI_CONNECTION_STATE_T *state )
{
   const SINGLE_CONNECTION_STATE_T *single_state = (SINGLE_CONNECTION_STATE_T *)state;

   return single_state->rx_slot_size;
}

/* ----------------------------------------------------------------------
 * Allocate a buffer of at least 'length' aligned for address and length
 * as required by the connection
 * -------------------------------------------------------------------- */
static void *
single_buffer_allocate( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle, uint32_t *length )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;

   return state->mdriver->allocate_buffer( state->mhandle, length );
}

/* ----------------------------------------------------------------------
 * Free memory allocated by single_buffer_allocate
 * -------------------------------------------------------------------- */
static void
single_buffer_free( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle, void *address )
{
   SERVICE_HANDLE_T *service = (SERVICE_HANDLE_T *)service_handle;
   SINGLE_CONNECTION_STATE_T *state = service->state;

   state->mdriver->free_buffer( state->mhandle, address );
}

#ifdef DEBUG_TX_BULK_INUSE
/* ----------------------------------------------------------------------
 * Dump the TX_BULK_INUSE queue
 * -------------------------------------------------------------------- */
static void
single_dump_tx_bulk_inuse( const SINGLE_CONNECTION_STATE_T *state, const char *heading )
{
   TX_MSGINFO_T *t;
   for ( t = vchi_mqueue_peek( state->tx_bulk_queue, TX_BULK_INUSE, 0 ); t; t = t->next )
   {
      if (heading)
      {
         os_logging_message("%s", heading);
         heading = NULL;
      }
      os_logging_message("%c%c%c%c:%10d [%d] %s %s %s %s %s %s %s %s",
                         FOURCC_TO_CHAR(t->service->service_id),
                         t->len,
                         t->channel,
                         t->flags & VCHI_FLAGS_BULK_AUX_QUEUED    ? "AuxQueued"
                                                                  : "         ",
                         t->flags & VCHI_FLAGS_BULK_AUX_COMPLETE  ? "AuxComp"
                                                                  : "       ",
                         t->flags & VCHI_FLAGS_BULK_DATA_QUEUED   ? "DataQueued"
                                                                  : "          ",
                         t->flags & VCHI_FLAGS_BULK_DATA_COMPLETE ? "DataComp"
                                                                  : "        ",
                         t->flags & VCHI_FLAGS_CALLBACK_WHEN_DATA_READ   ? "CbRead"
                                                                         : "      ",
                         t->flags & VCHI_FLAGS_BLOCK_UNTIL_DATA_READ     ? "BlockRead"
                                                                         : "         ",
                         t->flags & VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE ? "CbOpComp"
                                                                         : "        ",
                         t->flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE   ? "BlockOpComp"
                                                                         : "           ");
   }
}
#endif

/****************************** End of file ***********************************/
