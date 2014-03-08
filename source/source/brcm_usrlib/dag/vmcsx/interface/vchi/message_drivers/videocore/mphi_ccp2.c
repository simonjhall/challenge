/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VCHI
Module   :  MPHI videocore message driver

FILE DESCRIPTION:

The vcfw mphi driver provides a set of low-level routines for talking
to the hardware.

Of particular note is how message receiving is handled: the vcfw
driver needs to be passed a set of 'slots' into which incoming
messages are placed.  Callbacks are 'slot has completed' and 'position
within slot has advanced' (this latter is message-end-interrupt driven).
This module is responsible for translating these into a set of 'message
received' callbacks to the next layer up.

For transmitting messages, the abstraction is easier: again, we pass
'slots' to the vcfw driver.
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vcinclude/hardware.h"

#include "vcfw/drivers/chip/mphi.h"
#include "vcfw/drivers/chip/cache.h"
#ifdef VCHI_SUPPORT_CCP2TX
#include "vcfw/drivers/chip/ccp2tx.h"
#include "helpers/sysman/sysman.h"
#endif

#include "interface/vchi/vchi.h"
#include "interface/vchi/vchi_cfg_internal.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/common/multiqueue.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/control_service/control_service.h"


/******************************************************************************
  Local typedefs
 *****************************************************************************/

// define NULL-terminated list of messages we're interested in dumping
//#define DUMP_MESSAGES { "GCMD", "ILCS", "DISP", "KHRN", "HREQ", "FSRV", NULL }
//#define DUMP_MESSAGES { "KHRN", NULL }

// Mapping of public channel indexes to hardware
#define MESSAGE_TX_CHANNEL_MPHI_CONTROL MESSAGE_TX_CHANNEL_MESSAGE
#define MESSAGE_TX_CHANNEL_MPHI_DATA    MESSAGE_TX_CHANNEL_BULK
#define MESSAGE_TX_CHANNEL_CCP2(n)      (MESSAGE_TX_CHANNEL_BULK+1+(n))
#define IS_MPHI_TX_CHANNEL(n)           ((n) == MESSAGE_TX_CHANNEL_MPHI_CONTROL || (n) == MESSAGE_TX_CHANNEL_MPHI_DATA)
#define IS_CCP2_TX_CHANNEL(n)           ((n) >= MESSAGE_TX_CHANNEL_CCP2(0) && (n) <= MESSAGE_TX_CHANNEL_CCP2(7))

#define CCP2_TX_FIFO_EMPTY(state) (!(state)->tx_ccp2_slot[(state)->tx_ccp2_id_tail].inuse)
#define CCP2_TX_FIFO_FULL(state) ((state)->tx_ccp2_slot[(state)->tx_ccp2_id_head].inuse)

#define MESSAGE_RX_CHANNEL_MPHI_CONTROL MESSAGE_RX_CHANNEL_MESSAGE
#define MESSAGE_RX_CHANNEL_MPHI_DATA    MESSAGE_RX_CHANNEL_BULK

// how many times can we call ->open
#define MPHI_MAX_INSTANCES 3

// maximum number of messages we can queue up
#define NUMBER_OF_MESSAGES 128

// parameters of the protocol
#define VCHI_MSG_SYNC      0xAF05
#define VCHI_MSG_TERM      0x50FA
#define VCHI_LEN_ALIGN         16

// offsets for the message format
#define SYNC_OFFSET             0
#define LENGTH_OFFSET           2
#define FOUR_CC_OFFSET          4
#define TIMESTAMP_OFFSET        8
#define SLOT_COUNT_OFFSET      12
#define PAYLOAD_LENGTH_OFFSET  14
#define PAYLOAD_ADDRESS_OFFSET 16

#ifndef VCHI_CCP2TX_MANUAL_POWER
#  if VCHI_CCP2TX_IDLE_TIMEOUT >= 0 || VCHI_CCP2TX_OFF_TIMEOUT >= 0
#    define VCHI_CCP2TX_AUTOMATIC_POWER
#  endif
#endif

#ifdef VCHI_MSG_RX_OVERRUN
/* We will set up message receive slots to overrun, so the host can
 * automatically proceed from one slot to the next. But 2727B0 and 2727B1
 * have a bug (HW-1428) - if the MPHI hardware has advanced to the next
 * slot due to overrun, then the host sends a message marker, the MPHI
 * hardware may (or may not) jump a slot. To avoid this, the host must
 * never send a message marker at an overrun boundary. This is achieved
 * by the host always sending a 16-byte trailer at the end of every slot.
 * Our receive slots are set up skewed so that this trailer goes after
 * the slot boundary and into the start of the next slot:
 * 
 *    TX slots                RX slots
 * +-------------           +------------          (+ = message boundary)
 * +++++++++++++-          -+++++++++++++
 * +---+---+-----          -+---+---+----
 * +-------------          -+------------
 * ++++-------++-          -++++-------++
 *                         -
 */
#define VCHI_OVERRUN_BUG
#endif

typedef struct {
   int   inuse;
   MESSAGE_TX_CHANNEL_T channel;
   void *handle;
   void *addr;
   uint32_t len;
} FIFO_ENTRY_T;

typedef struct {

   // index into array of state (handles)
   int instance;


   // handles to mphi driver
   const MPHI_DRIVER_T *mphi_driver;
   DRIVER_HANDLE_T      handle_control;   // control rx
   DRIVER_HANDLE_T      handle_data;      // bulk rx
   DRIVER_HANDLE_T      handle_out;       // control and bulk tx

#ifdef VCHI_SUPPORT_CCP2TX
   // handle to the ccp2tx driver
   const CCP2TX_DRIVER_T *ccp2tx_driver;
   DRIVER_HANDLE_T        handle_ccp2tx_bulk; // bulk tx over CCP2
   SYSMAN_HANDLE_T        sysman_h;
#endif
   
#ifdef __VIDEOCORE4__
   const CACHE_DRIVER_T  *cache_driver;
   DRIVER_HANDLE_T        cache_h;
#endif
   // callback to the next higher level
   void (*event_callback)( void *cb_data );
   void *event_callback_data;

   // There are 3 identical 16 deep DMA FIFOs in the MPHI peripheral;
   // we need to keep track of what is in them so that we can pass back
   // the correct information when each finishes.
   RX_MSG_SLOTINFO_T  *rx_msg_slot [ MPHI_RX_MSG_SLOTS  ];
   RX_BULK_SLOTINFO_T *rx_bulk_slot[ MPHI_RX_BULK_SLOTS ];
   FIFO_ENTRY_T        tx_mphi_slot[ MPHI_TX_SLOTS      ];

#ifdef VCHI_SUPPORT_CCP2TX
   // This FIFO is for the CCP2TX bulk transmit
   FIFO_ENTRY_T        tx_ccp2_slot[ CCP2TX_QUEUE_SIZE   ];
   bool_t              tx_ccp2_frame_in_progress[ 8 ];
#endif

   // parse state information
   RX_MSG_SLOTINFO_T *parse_slot;
   int parse_slot_completed;
   
#ifdef VCHI_OVERRUN_BUG
   bool_t added_first_slot;
#endif

   // To confirm that things are happening in order we will use an 8-bit
   // id which is returned upon completion (except for the TX side where
   // we just see an interrupt)
   uint8_t rx_msg_slot_id;
   uint8_t rx_bulk_id;
   uint8_t tx_mphi_id;

   // parameters of the driver
   int    rx_addr_align_cached;
   int    rx_addr_align_uncached;
   int    tx_addr_align_cached;
   int    tx_addr_align_uncached;
   bool_t tx_supports_terminate;

#ifdef VCHI_SUPPORT_CCP2TX
   // index into tx_ccp2_slot
   uint8_t tx_ccp2_id_head;
   uint8_t tx_ccp2_id_tail;
   
   // CCP2 power management
   CCP2TX_POWER_T tx_ccp2_power;
#ifdef VCHI_CCP2TX_AUTOMATIC_POWER
   uint32_t tx_ccp2_last_tx_time;
#endif
#endif

   OS_HISR_T hisr;

} MPHI_MSG_DRIVER_STATE_T;


/******************************************************************************
 Extern functions
 *****************************************************************************/


/******************************************************************************
 Static functions
 *****************************************************************************/

// API functions
static VCHI_MDRIVER_HANDLE_T *mphi_msg_open( VCHI_MESSAGE_DRIVER_OPEN_T *params, void *state );
static int32_t mphi_msg_suspending( VCHI_MDRIVER_HANDLE_T *handle );
static int32_t mphi_msg_resumed( VCHI_MDRIVER_HANDLE_T *handle );
static int32_t mphi_msg_power_control( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel, bool_t enable );
static int32_t mphi_msg_add_msg_slot( VCHI_MDRIVER_HANDLE_T *handle, RX_MSG_SLOTINFO_T *slot );
static int32_t mphi_msg_add_bulk_rx( VCHI_MDRIVER_HANDLE_T *handle, void *data, uint32_t len, RX_BULK_SLOTINFO_T *slot );
static int32_t mphi_msg_send_message( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel, const void *data, uint32_t len, VCHI_MSG_FLAGS_T flags, void *send_handle );
static void    mphi_msg_next_event( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_EVENT_T *slot );
static int32_t mphi_msg_enable( VCHI_MDRIVER_HANDLE_T *handle );
static int32_t mphi_msg_form_message( VCHI_MDRIVER_HANDLE_T *handle, fourcc_t service_id, VCHI_MSG_VECTOR_T *vector, uint32_t count, void *address, uint32_t available_length, uint32_t max_total_length, bool_t pad_to_fill, bool_t allow_partial );

static int32_t mphi_msg_update_message( VCHI_MDRIVER_HANDLE_T *handle, void *dest, int16_t *slot_count );
static int32_t mphi_msg_buffer_aligned( VCHI_MDRIVER_HANDLE_T *handle, int tx, int uncached, const void *address, const uint32_t length );
static void *  mphi_msg_allocate_buffer( VCHI_MDRIVER_HANDLE_T *handle, uint32_t *length );
static void    mphi_msg_free_buffer( VCHI_MDRIVER_HANDLE_T *handle, void *address );
static int     mphi_msg_rx_slot_size( VCHI_MDRIVER_HANDLE_T *handle, int msg_size );
static int     mphi_msg_tx_slot_size( VCHI_MDRIVER_HANDLE_T *handle, int msg_size );

static bool_t  mphi_msg_tx_supports_terminate( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel );
static uint32_t mphi_msg_tx_bulk_chunk_size( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel );
static int     mphi_msg_tx_alignment( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel );
static int     mphi_msg_rx_alignment( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_RX_CHANNEL_T channel );
static void    mphi_msg_form_bulk_aux( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel, const void *data, uint32_t len, uint32_t chunk_size, const void **aux_data, int32_t *aux_len );

// helper functions
static void    mphi_msg_callback( const void *callback_data );
static void    mphi_hisr0( void );
static void    mphi_hisr1( void );
static void    mphi_hisr2( void );
static int32_t mphi_parse( RX_MSG_SLOTINFO_T *slot, MESSAGE_EVENT_T *event );
static void    mphi_msg_debug( VCHI_MDRIVER_HANDLE_T *handle );
static void    mphi_check_ccp2tx_idle( MPHI_MSG_DRIVER_STATE_T *state );

#if defined(DUMP_MESSAGES)
static void dump_message( const char *reason, const unsigned char *addr, int len );
#else
#define dump_message(a,b,c) do { ; } while(0)
#endif
#ifdef VCHI_SUPPORT_CCP2TX
static CCP2TX_CALLBACK ccp2_msg_callback;
#endif


/******************************************************************************
 Static data
 *****************************************************************************/

//The driver function table
static const VCHI_MESSAGE_DRIVER_T mphi_msg_fops =
{
   mphi_msg_open,
   mphi_msg_suspending,
   mphi_msg_resumed,
   mphi_msg_power_control,
   mphi_msg_add_msg_slot,
   mphi_msg_add_bulk_rx,
   mphi_msg_send_message,
   mphi_msg_next_event,
   mphi_msg_enable,
   mphi_msg_form_message,

   mphi_msg_update_message,
   mphi_msg_buffer_aligned,
   mphi_msg_allocate_buffer,
   mphi_msg_free_buffer,
   mphi_msg_rx_slot_size,
   mphi_msg_tx_slot_size,

   mphi_msg_tx_supports_terminate,
   mphi_msg_tx_bulk_chunk_size,
   mphi_msg_tx_alignment,
   mphi_msg_rx_alignment,
   mphi_msg_form_bulk_aux,

   mphi_msg_debug
};


static MPHI_MSG_DRIVER_STATE_T mphi_msg_state[ MPHI_MAX_INSTANCES ];

// vcfw driver open params
//static const MPHI_OPEN_T mphi_open_control = { MPHI_CHANNEL_IN_CONTROL, mphi_msg_callback };
//static const MPHI_OPEN_T mphi_open_data    = { MPHI_CHANNEL_IN_DATA,    mphi_msg_callback };
//static const MPHI_OPEN_T mphi_open_out     = { MPHI_CHANNEL_OUT,        mphi_msg_callback };


/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * return pointer to the mphi message driver function table
 * -------------------------------------------------------------------- */
const VCHI_MESSAGE_DRIVER_T *
vchi_mphi_message_driver_func_table( void )
{
   return &mphi_msg_fops;
}


/******************************************************************************
 Static  Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * open the vcfw mphi driver (several times; rx control, rx data and tx)
 *
 * can be called multiple times (eg. local host apps)
 *
 * returns handle to this instance on success; NULL on failure
 * -------------------------------------------------------------------- */
static VCHI_MDRIVER_HANDLE_T *
mphi_msg_open( VCHI_MESSAGE_DRIVER_OPEN_T *params, void *callback_data )
{
   int32_t success = -1; // fail by default
   MPHI_MSG_DRIVER_STATE_T *state;
   MPHI_OPEN_T open_params;
   const char *driver_name;
   uint32_t driver_version_major, driver_version_minor;
   DRIVER_FLAGS_T driver_flags;
#ifdef VCHI_SUPPORT_CCP2TX
   CCP2TX_OPEN_T ccp2_open = { 0 };
   CCP2TX_SETUP_T ccp2_setup = { 0 };
#endif
   static int32_t mphi_num_instances;

   // get new state handle
   if ( mphi_num_instances >= MPHI_MAX_INSTANCES ) return NULL;

   // could get called from more than one thread
   success = os_semaphore_obtain_global();
   os_assert(success == 0);

   state = &mphi_msg_state[ mphi_num_instances ];
   memset( state, 0, sizeof(MPHI_MSG_DRIVER_STATE_T) );
   state->instance = (int) mphi_num_instances++;

   success = os_semaphore_release_global();
   os_assert(success == 0);

   // record the callbacks
   state->event_callback = params->event_callback;
   state->event_callback_data = callback_data;

   // open (vcfw) mphi driver and get handle
#ifdef VCHI_LOCAL_HOST_PORT
{
   extern const MPHI_DRIVER_T *local_mphi_get_func_table( void );
   state->mphi_driver = local_mphi_get_func_table();
}
#else
   state->mphi_driver = mphi_get_func_table();
#endif

   success = state->mphi_driver->info(&driver_name,
                                      &driver_version_major,
                                      &driver_version_minor,
                                      &driver_flags);
   if ( success != 0) return NULL;

   // The master transmitter supports terminate. A slave transmitter does not, but signals length up front.
   state->tx_supports_terminate = (bool_t) ( strstr(driver_name, "_slave") == 0 );

   open_params.channel       = MPHI_CHANNEL_IN_CONTROL;
   open_params.callback      = mphi_msg_callback;
   open_params.callback_data = state;
   open_params.flags         = MPHI_CHANNEL_FLAGS_IN_POSITION_EVENTS;
#ifdef VCHI_MSG_RX_OVERRUN
   open_params.flags        |= MPHI_CHANNEL_FLAGS_IN_OVERRUN;
#endif
   // open the driver for the control channel
   success = state->mphi_driver->open( &open_params, &state->handle_control );
   if ( success != 0 ) return NULL;

   // open the driver for the data (bulk) channel
   open_params.channel = MPHI_CHANNEL_IN_DATA;
   open_params.flags   = MPHI_CHANNEL_FLAGS_NONE;
   success = state->mphi_driver->open( &open_params, &state->handle_data );
   if ( success != 0 ) return NULL;

   // open the driver for the output channel
   open_params.channel = MPHI_CHANNEL_OUT;
   open_params.flags   = MPHI_CHANNEL_FLAGS_NONE;
   success = state->mphi_driver->open( &open_params, &state->handle_out );
   if ( success != 0 ) return NULL;

   // set current (12mA) and slew (off in this case)
   success = state->mphi_driver->set_power( state->handle_control, 12, 0 );
   if ( success != 0 ) return NULL;

   // get the alignment requirements
   state->rx_addr_align_cached = OS_MAX( state->mphi_driver->alignment( state->handle_control, 0 ),
                                         state->mphi_driver->alignment( state->handle_data, 0 ) );
   state->rx_addr_align_uncached = OS_MAX( state->mphi_driver->alignment( state->handle_control, 1 ),
                                           state->mphi_driver->alignment( state->handle_data, 1 ) );
   state->tx_addr_align_cached = state->mphi_driver->alignment( state->handle_out, 0 );
   state->tx_addr_align_uncached = state->mphi_driver->alignment( state->handle_out, 1 );

   // create the HISR that is used to signal the next higher level that an event has occured
   if ( state->instance == 0 )
      success = os_hisr_create( &state->hisr, mphi_hisr0, "MPHI HISR0" );
   else if ( state->instance == 1 )
      success = os_hisr_create( &state->hisr, mphi_hisr1, "MPHI HISR1" );
   else if ( state->instance == 2 )
      success = os_hisr_create( &state->hisr, mphi_hisr2, "MPHI HISR2" );
   else
      success = -1;
   if ( success != 0 ) return NULL;

#ifdef VCHI_SUPPORT_CCP2TX
   // open (vcfw) ccp2tx driver and get handle
   state->ccp2tx_driver = ccp2tx_get_func_table();

   ccp2_open.port = CCP2TX_PORT_0;
   ccp2_open.setup = &ccp2_setup;
   ccp2_setup.bitrate_khz = 650000;
   ccp2_setup.clock_mode = CCP2TX_CLOCK_MODE_DATA_STROBE;
   ccp2_setup.format = CCP2TX_DATA_TYPE_FSP;
   ccp2_setup.line_blanking_period = 1;
#if defined VCHI_CCP2TX_AUTOMATIC_POWER || defined VCHI_CCP2TX_MANUAL_POWER
   ccp2_open.initial_power_off = VC_TRUE;
   state->tx_ccp2_power = CCP2TX_POWER_OFF;
#else
   ccp2_open.initial_power_off = VC_FALSE;
   state->tx_ccp2_power = CCP2TX_POWER_ON;
#endif

   success = state->ccp2tx_driver->open( &ccp2_open, &state->handle_ccp2tx_bulk );
   if ( success != 0 ) return NULL;
   success = state->ccp2tx_driver->register_callback ( state->handle_ccp2tx_bulk,
                                                       ccp2_msg_callback,
                                                       state );
   if ( success != 0 ) return NULL;

   success = state->ccp2tx_driver->set_queue_threshold( state->handle_ccp2tx_bulk,
                                                        0 );
   if ( success != 0 ) return NULL;

   // hard-coded knowledge that CCP2TX needs 16-byte alignment. VCFW driver should export this
   state->tx_addr_align_uncached = OS_MAX( state->tx_addr_align_uncached, 16 );
   state->tx_addr_align_cached = OS_MAX( state->tx_addr_align_cached, 32 );

   success = sysman_register_user(&state->sysman_h);
   if (success != 0) return NULL;
#endif
   
#ifdef __VIDEOCORE4__
   state->cache_driver = cache_get_func_table();
   success = state->cache_driver->open( NULL, &state->cache_h );
   if ( success != 0 ) return NULL;
#endif
   // various parts of this message driver will fail if this condition isn't met -
   // will need more care on message formation.
   os_assert( state->tx_addr_align_uncached <= VCHI_LEN_ALIGN );

   return (VCHI_MDRIVER_HANDLE_T *)state;
}

/* ----------------------------------------------------------------------
 * VideoCore is about to suspend
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_suspending( VCHI_MDRIVER_HANDLE_T *handle )
{
#ifdef VCHI_CCP2TX_MANUAL_POWER
   // A shut-down of CCP2 is implicit when disconnecting. If we don't do this now,
   // CCP2 will come back on when we desuspend.
   return mphi_msg_power_control( handle, MESSAGE_TX_CHANNEL_CCP2(0), 0 );
#else
   return 0;
#endif
}

/* ----------------------------------------------------------------------
 * VideoCore has been resumed, so MPHI will have been trashed.
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_resumed( VCHI_MDRIVER_HANDLE_T *handle )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   int id;
   for ( id = 0; id < MPHI_RX_MSG_SLOTS; id++ )
       state->rx_msg_slot[ id ] = NULL;
   
   state->rx_msg_slot_id = 0;
#ifdef VCHI_OVERRUN_BUG
   state->added_first_slot = VC_FALSE;
#endif
   
   return 0;
}

/* ----------------------------------------------------------------------
 * Power request received from peer
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_power_control( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel, bool_t enable )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   int32_t success = 0;
   
#ifdef VCHI_CCP2TX_MANUAL_POWER
   // Only support CCP2TX changes, for which we use channel 0 as the selector
   if (channel == MESSAGE_TX_CHANNEL_CCP2(0))
   {
      CCP2TX_POWER_T ccp2_power = enable ? CCP2TX_POWER_ON : CCP2TX_POWER_OFF;

      os_assert(CCP2_TX_FIFO_EMPTY(state));
      
      success = state->ccp2tx_driver->set_power( state->handle_ccp2tx_bulk,
                                                 ccp2_power );
      os_assert(success == 0);
      state->tx_ccp2_power = ccp2_power;
      success = sysman_set_user_request(state->sysman_h, SYSMAN_BLOCK_CCP2_TX, enable, SYSMAN_WAIT_BLOCKING);
      os_assert(success == 0);
   }
#endif
   
   return success;
}

/* ----------------------------------------------------------------------
 * add a slot to the rx message (= control) fifo
 *
 * return 0 if the given slot was successfully added, non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_add_msg_slot( VCHI_MDRIVER_HANDLE_T *handle, RX_MSG_SLOTINFO_T *slot )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   uint8_t id = state->rx_msg_slot_id;
   uint8_t *addr;
   uint32_t len;
   int32_t success = -1; // fail by default

   if ( state->rx_msg_slot[id] != NULL ) {
      //os_logging_message("mphi_msg_add_msg_slot: no free slots");
      return -1; // no free slots
   }
   os_assert( slot->active == 0 );
   // we need to setup, in advance, for vcfw success case (avoid lisr race)
   state->rx_msg_slot[ id ] = slot;
   addr = slot->addr;
   len = slot->len;
#ifdef VCHI_OVERRUN_BUG
   if (!state->added_first_slot) {
       addr += VCHI_LEN_ALIGN;
       slot->read_ptr += VCHI_LEN_ALIGN;
       len -= VCHI_LEN_ALIGN;
   }
   slot->read_ptr = VCHI_LEN_ALIGN;
#else
   slot->read_ptr = 0;
#endif
   slot->active = 1;
   
   success = state->mphi_driver->add_recv_slot( state->handle_control, addr, len, id );
   if ( success != 0 ) {
      // vcfw call didn't succeed; undo setup
      slot->active = 0;
      state->rx_msg_slot[ id ] = NULL;
      //os_logging_message( "mphi_msg_add_msg_slot failed" );
   } else {
#ifdef VCHI_OVERRUN_BUG
      state->added_first_slot = VC_TRUE;
#endif
      state->rx_msg_slot_id = (id + 1) % MPHI_RX_MSG_SLOTS;
      //os_logging_message( "mphi_msg_add_msg_slot: id = %d", id );
   }
   return success;
}


/* ----------------------------------------------------------------------
 * add a slot to the rx bulk (= data) fifo
 *
 * return 0 if the given slot was successfully added, non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_add_bulk_rx( VCHI_MDRIVER_HANDLE_T *handle, void *addr, uint32_t len, RX_BULK_SLOTINFO_T *slot )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   uint8_t id = state->rx_bulk_id;
   int32_t success = -1; // fail by default

   if ( state->rx_bulk_slot[id] != NULL )
      return -1; // no free slots
   // we need to setup, in advance, for vcfw success case (avoid lisr race)
   state->rx_bulk_slot[ id ] = slot;
   success = state->mphi_driver->add_recv_slot( state->handle_data, addr, len, id );
   if ( success != 0 ) {
      // vcfw call didn't succeed; undo setup
      state->rx_bulk_slot[ id ] = NULL;
      os_logging_message( "mphi_msg_add_msg_slot (bulk) failed" );
   } else {
      state->rx_bulk_id = (id + 1) % MPHI_RX_BULK_SLOTS;
      //os_logging_message( "mphi_msg_add_bulk_rx: id = %d", id );
   }
   return success;
}

/* ----------------------------------------------------------------------
 * add a slot to the tx fifo
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_send_message( VCHI_MDRIVER_HANDLE_T * handle,
                       MESSAGE_TX_CHANNEL_T channel,
                       const void *addr,
                       uint32_t len,
                       VCHI_MSG_FLAGS_T flags,
                       void * send_handle )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   uint8_t id;
   int32_t success = -1; // fail by default

   os_logging_message("mphi_msg_send_message %x:%d", addr, len);

   if( IS_MPHI_TX_CHANNEL(channel) )
   {
      id = state->tx_mphi_id;

      if ( state->tx_mphi_slot[id].inuse ) {
         // no room for message
   //      os_assert(0);
         return -1;
      }

      // we need to setup, in advance, for vcfw success case (avoid lisr race)
      state->tx_mphi_slot[ id ].inuse = 1;
      state->tx_mphi_slot[ id ].handle = send_handle;
      state->tx_mphi_slot[ id ].channel = channel;
      state->tx_mphi_slot[ id ].addr = (void *)addr;
      state->tx_mphi_slot[ id ].len = len;

      //dump_message( "mphi_msg_send_message", addr, len );
      success = state->mphi_driver->out_queue_message( state->handle_out,
                                                       channel == MESSAGE_TX_CHANNEL_MPHI_CONTROL,
                                                       addr,
                                                       len,
                                                       id,
                                                       flags & VCHI_MSG_FLAGS_TERMINATE_DMA ?
                                                                 MPHI_FLAGS_TERMINATE_DMA :
                                                                 MPHI_FLAGS_NONE );
      if ( success != 0 ) {
         // vcfw call didn't succeed; undo setup
         state->tx_mphi_slot[ id ].inuse = 0;
         os_logging_message( "mphi_msg_send_message: mphi tx failed (%d), id = %d", success, id );
//         os_assert(0);
      } else {
         state->tx_mphi_id = (id + 1) % MPHI_TX_SLOTS;
         os_logging_message( "mphi_msg_send_message: id = %d", id );
      }
   }
#ifdef VCHI_SUPPORT_CCP2TX
   else if( IS_CCP2_TX_CHANNEL(channel) )
   {
      int lcn = channel - MESSAGE_TX_CHANNEL_CCP2(0);
      id = state->tx_ccp2_id_head;

      if ( state->tx_ccp2_slot[id].inuse ) {
         // no room for message
   //      os_assert(0);
         return -1;
      }

      // we need to setup, in advance, for vcfw success case (avoid lisr race)
      state->tx_ccp2_slot[ id ].inuse = 1;
      state->tx_ccp2_slot[ id ].handle = send_handle;
      state->tx_ccp2_slot[ id ].channel = channel;
      state->tx_ccp2_slot[ id ].addr = (void *)addr;
      state->tx_ccp2_slot[ id ].len = len;
      
      // Ensure CCP2TX is fully operational
      if (state->tx_ccp2_power != CCP2TX_POWER_ON)
      {
         if (state->tx_ccp2_power == CCP2TX_POWER_OFF)
            success = sysman_set_user_request(state->sysman_h, SYSMAN_BLOCK_CCP2TX, 1, SYSMAN_WAIT_BLOCKING);
         else
            success = 0;
         if (success == 0)
         {
            success = state->ccp2tx_driver->set_power( state->handle_ccp2tx_bulk,
                                                       CCP2TX_POWER_ON );
            if (success == 0)
               state->tx_ccp2_power = CCP2TX_POWER_ON;
         }
      }
      else
         success = 0;
      
      os_assert(success == 0);
      
      if (success == 0) {
         success = state->ccp2tx_driver->queue_line( state->handle_ccp2tx_bulk, //const DRIVER_HANDLE_T handle,
                                                     lcn,         //int logical_channel
                                                     !state->tx_ccp2_frame_in_progress[ lcn ], //bool_t frame_start,
                                                     (flags & VCHI_MSG_FLAGS_TERMINATE_DMA) != 0, //bool_t frame_end,
                                                     addr,        //const void *data,
                                                     len          //size_t length
                                                   );
      }
      if ( success != 0 ) {
         // vcfw call didn't succeed; undo setup
         state->tx_ccp2_slot[ id ].inuse = 0;
         os_logging_message( "mphi_msg_send_message: ccp2tx failed (%d), id = %d", success, id );
         os_assert(0);
      } else {
         state->tx_ccp2_frame_in_progress[ lcn ] = !( flags & VCHI_MSG_FLAGS_TERMINATE_DMA );
         state->tx_ccp2_id_head = (id + 1) % CCP2TX_QUEUE_SIZE;
         //os_logging_message( "mphi_msg_send_message: id = %d", id );
      }
      //logging_message( LOGGING_GENERAL, "mphi_msg_send_message: id = %d", id );
      //os_logging_message( "mphi_msg_send_message: id = %d", id );
   }
#endif
   else
      os_assert(0);

   return success;
}

/* ----------------------------------------------------------------------
 * at this point, we get a callback from the lisr to say there are
 * some events (at the vcfw level) which need handling.
 *
 * the processing chain is to wake up our local hisr...
 * -------------------------------------------------------------------- */
static void
mphi_msg_callback( const void *callback_data )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)callback_data;

   //logging_message (LOGGING_MINIMAL, "VCHI timing : mphi_msg_callback : %d", vchi_current_time(0));

   (void) os_hisr_activate( &state->hisr );
}

#ifdef VCHI_SUPPORT_CCP2TX
static void
ccp2_msg_callback (CCP2TX_EVENT_T events, void *userdata)
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)userdata;
   os_hisr_activate( &state->hisr );
}
#endif

/* ----------------------------------------------------------------------
 * ...which simply does a callback (in hisr context) to the next level
 * up (= vchi connection driver)...
 * -------------------------------------------------------------------- */
 static void
mphi_hisr0( void )
{
   MPHI_MSG_DRIVER_STATE_T *state = &mphi_msg_state[0];

   //logging_message (LOGGING_MINIMAL, "VCHI timing : mphi_hisr0 : %d", vchi_current_time(0));

   state->event_callback( state->event_callback_data );
}

static void
mphi_hisr1( void )
{
   MPHI_MSG_DRIVER_STATE_T *state = &mphi_msg_state[1];
   state->event_callback( state->event_callback_data );
}

static void
mphi_hisr2( void )
{
   MPHI_MSG_DRIVER_STATE_T *state = &mphi_msg_state[2];
   state->event_callback( state->event_callback_data );
}

/* ----------------------------------------------------------------------
 * ...at which point, the connection driver will call into us, in
 * task context, to find out exactly which event(s) happened.
 *
 * the idea is that the task should spin in a loop, calling this
 * function until it returns MESSAGE_EVENT_NONE.  closely coupled
 * to the vcfw driver ->next_event method, see that for low-level
 * details.
 *
 * return values is set in event->type as follows:
 *
 *    MESSAGE_EVENT_NONE - no more events pending.
 *
 *    MESSAGE_EVENT_NOP - no event on this call, because only part
 *       of a message was received.  call again.
 *
 *    MESSAGE_EVENT_MESSAGE - a message was received.  information
 *       about the payload (address, len, service 4cc, slot) will
 *       be stored in event->message.*
 *
 *    MESSAGE_EVENT_SLOT_COMPLETE - a message receive slot completed.
 *       pointer to the slot stored in event->rx_msg.  note that we
 *       guarantees we will return MESSAGE_EVENT_MESSAGE for all
 *       messages in a completed slot before returning
 *       MESSAGE_EVENT_SLOT_COMPLETE
 *
 *    MESSAGE_EVENT_RX_BULK_PAUSED - FIXME: do we even need this?
 *
 *    MESSAGE_EVENT_RX_BULK_COMPLETE - a bulk receive slot completed.
 *       pointer to the slot stored in event->rx_bulk
 *
 *    MESSAGE_EVENT_TX_COMPLETE - a transmit (message) slot
 *       completed.  pointer to the slot stored in event->tx
 *
 *    MESSAGE_EVENT_TX_BULK_COMPLETE - a transmit bulk slot
 *       completed.  pointer to the slot stored in event->tx
 *
 *    MESSAGE_EVENT_MSG_DISCARDED - FIXME: do we need this?
 *
 * -------------------------------------------------------------------- */
static void
mphi_msg_next_event( VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_EVENT_T *event )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   uint8_t slot_id;
   uint32_t pos;
   MPHI_EVENT_TYPE_T reason;

   event->type = MESSAGE_EVENT_NONE;

   // if we are busy parsing a slot, keep returning more messages
   // until we run out of data
   if ( state->parse_slot ) {
      if ( mphi_parse(state->parse_slot,event) )
         return;
      if ( state->parse_slot_completed ) {
         state->parse_slot_completed = 0;
         event->type = MESSAGE_EVENT_SLOT_COMPLETE;
         event->rx_msg = state->parse_slot;
         state->parse_slot = NULL;
         return;
      }
      state->parse_slot = NULL;
   }

#ifdef VCHI_SUPPORT_CCP2TX
   // OS abstraction layer doesn't have any sort of timers - instead, just do a sort of
   // polled check here - will work pretty well, assuming there's at least some activity
   // on the MPHI bus...
   mphi_check_ccp2tx_idle(state);
#endif

   // fetch the next event from vcfw driver
   // should get cleverer, and only check on channels that have signalled. Note
   // that original code, which checked events only on handle_control, relied on
   // a bogus implementation of next_event(). It's been broken from the start,
   // returning any event on any channel. If the channels mean anything, they have
   // to return independent event streams. local.c now has independent channels,
   // mphi.c still has coupled channels. For now, this works with either.
   reason = state->mphi_driver->next_event( state->handle_control, &slot_id, &pos );
   if (reason == MPHI_EVENT_NONE)
      reason = state->mphi_driver->next_event( state->handle_out, &slot_id, &pos );
   if (reason == MPHI_EVENT_NONE)
      reason = state->mphi_driver->next_event( state->handle_data, &slot_id, &pos );

   switch( reason ) {

   default:
      os_assert(0);
   case MPHI_EVENT_NONE:
#ifdef VCHI_SUPPORT_CCP2TX
   {
      // MPHI driver has no more events, check CCP2TX for events
      CCP2TX_MSG_EVENT_T ccp2tx_reason;

      ccp2tx_reason = state->ccp2tx_driver->next_event ( state->handle_ccp2tx_bulk );

      switch( ccp2tx_reason ) {
         default:
            os_assert(0);

         case CCP2TX_MSG_EVENT_NONE:
            event->type = MESSAGE_EVENT_NONE;
            return; // finished

         case CCP2TX_MSG_EVENT_TX_COMPLETED:
            // a transmission from VideoCore has completed
            slot_id = state->tx_ccp2_id_tail;
            if(!state->tx_ccp2_slot[ slot_id ].inuse)
            {
               os_assert(0); // Slot not in use
               return;
            }
            
#ifdef VCHI_CCP2TX_AUTOMATIC_POWER
            state->tx_ccp2_last_tx_time = os_get_time_in_usecs();
#endif

            event->tx_handle = state->tx_ccp2_slot[ slot_id ].handle;
            event->tx_channel = state->tx_ccp2_slot[ slot_id ].channel;
            event->message.addr = state->tx_ccp2_slot[ slot_id ].addr;
            event->message.len = state->tx_ccp2_slot[ slot_id ].len;
            //logging_message(LOGGING_MINIMAL, "CCP2TX_MSG_EVENT_TX_COMPLETED, event->tx_handle=%x", event->tx_handle);
            os_logging_message("CCP2TX_MSG_EVENT_TX_COMPLETED, event->tx_handle=%x", event->tx_handle);
            state->tx_ccp2_slot[ slot_id ].inuse = 0;
            event->type = MESSAGE_EVENT_TX_COMPLETE;

            state->tx_ccp2_id_tail = (slot_id + 1) % CCP2TX_QUEUE_SIZE;
            mphi_check_ccp2tx_idle(state);
            return;
      }
   }
#else
      event->type = MESSAGE_EVENT_NONE;
      return; // finished
#endif

   case MPHI_EVENT_IN_CONTROL_POSITION:
      // the given slot_id has received some data
      os_logging_message("IN_POS: %d:%d:%d", state->instance, slot_id, pos);
      os_assert( slot_id < MPHI_RX_MSG_SLOTS );
      state->parse_slot = state->rx_msg_slot[ slot_id ];
      state->parse_slot->write_ptr = pos;

      if ( mphi_parse(state->parse_slot,event) )
         return; // we will loop looking for more messages

      state->parse_slot = NULL; // stop parsing
      event->type = MESSAGE_EVENT_NOP;
      return;

   case MPHI_EVENT_IN_CONTROL_DMA_COMPLETED:
      // the given slot_id has completed
      os_logging_message("IN_COMP: %d:%d:%d", state->instance, slot_id, pos);
      os_assert( slot_id < MPHI_RX_MSG_SLOTS );
      state->parse_slot = state->rx_msg_slot[ slot_id ];
      state->parse_slot->write_ptr = pos;
      state->rx_msg_slot[ slot_id ] = NULL;

      state->parse_slot_completed = 1;
      if ( mphi_parse(state->parse_slot,event) )
         return; // we will loop looking for more messages

      state->parse_slot_completed = 0;
      event->type = MESSAGE_EVENT_SLOT_COMPLETE;
      event->rx_msg = state->parse_slot;
      state->parse_slot = NULL; // stop parsing
      return;

   case MPHI_EVENT_IN_DATA_POSITION:
      // nothing required at the moment
      // must likely to happen during long transfers when the host
      // needs to check the VC->Host direction
      event->type = MESSAGE_EVENT_RX_BULK_PAUSED;
      return;

   case MPHI_EVENT_IN_DATA_DMA_COMPLETED:
      // a transmission from the host has completed
      os_assert( slot_id < MPHI_RX_BULK_SLOTS );
      event->rx_bulk = state->rx_bulk_slot[ slot_id ];
      state->rx_bulk_slot[ slot_id ] = NULL;
      event->type = MESSAGE_EVENT_RX_BULK_COMPLETE;
      return;

   case MPHI_EVENT_OUT_DMA_COMPLETED:
      // a transmission from VideoCore has completed
      //os_logging_message( "mphi_hisr : dma out complete slot_id = %d", slot_id );
      os_logging_message("OUT_DONE: %d:%d", state->instance, slot_id);
      event->tx_handle = state->tx_mphi_slot[ slot_id ].handle;
      event->tx_channel = state->tx_mphi_slot[ slot_id ].channel;
      event->message.addr = state->tx_mphi_slot[ slot_id ].addr;
      event->message.len = state->tx_mphi_slot[ slot_id ].len;
      state->tx_mphi_slot[ slot_id ].inuse = 0;
      event->type = MESSAGE_EVENT_TX_COMPLETE;
      return;

   case MPHI_EVENT_IN_MESSAGE_DISCARDED:
      event->type = MESSAGE_EVENT_MSG_DISCARDED;
      return;

   }
}

static int32_t
mphi_parse( RX_MSG_SLOTINFO_T *slot, MESSAGE_EVENT_T *event )
{
   int32_t available;
   uint8_t *addr;
   int32_t len;
   uint16_t sync;

   //logging_message (LOGGING_MINIMAL, "VCHI timing : mphi_parse : %d", vchi_current_time(0));

   // trap badness from the hardware
   if ( slot->write_ptr > slot->len ) {
      os_assert(0);
      return 0; // ignore any further messages in this slot
   }

   available = slot->write_ptr - slot->read_ptr;
   os_assert( available >= 0 ); // this would be bad

   // we require to see at least the header:
   if ( available < PAYLOAD_ADDRESS_OFFSET )
      return 0;

   addr = slot->addr + slot->read_ptr;
   sync = vchi_readbuf_uint16( addr + SYNC_OFFSET );
   if ( sync == VCHI_MSG_TERM )
      return 0;
   
   os_assert( sync == VCHI_MSG_SYNC );

   len = vchi_readbuf_uint16( addr + LENGTH_OFFSET );
   // confirm that we have seen the whole message
   if ( available < len )
      return 0;

   // verify that the values we have read are valid
   if ( len < PAYLOAD_ADDRESS_OFFSET || slot->read_ptr + len > slot->len ) {
      os_assert(0); // must have valid 'length' and 'service id' fields
      return 0; // ignore any further messages in this slot
   }

   dump_message( "mphi_parse", addr, len );

   // also need to record how many messages there are in the slot so we know when it comes free
   slot->msgs_parsed++; // FIXME: put this into next layer?

   // synchronise clocks with the other side
   vchi_control_update_time( vchi_readbuf_uint32(addr + TIMESTAMP_OFFSET) );

   // fill in ->event fields
   event->type = MESSAGE_EVENT_MESSAGE;
   event->message.addr        = addr + PAYLOAD_ADDRESS_OFFSET;
   event->message.slot_delta  = vchi_readbuf_uint16( addr + SLOT_COUNT_OFFSET );
   event->message.len         = vchi_readbuf_uint16( addr + PAYLOAD_LENGTH_OFFSET );
   event->message.slot        = slot;
   event->message.service     = vchi_readbuf_fourcc( addr + FOUR_CC_OFFSET );
   event->message.tx_timestamp= vchi_readbuf_uint32( addr + TIMESTAMP_OFFSET );
   event->message.rx_timestamp= vchi_control_get_time();

   // ready to parse any further messages in this slot
   slot->read_ptr += len;

   return 1; // we found a message!
}

/* ----------------------------------------------------------------------
 * the routine is called during initialisation, after the connection
 * driver has queued all rx slots.
 *
 * we simply need to call the vcfw ->enable method
 *
 * return 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_enable( VCHI_MDRIVER_HANDLE_T *handle )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   return state->mphi_driver->enable( state->handle_control );
}

/* ----------------------------------------------------------------------
 * Calculate the total length of vector v[n].
 * -------------------------------------------------------------------- */
static size_t vec_length(const VCHI_MSG_VECTOR_T *v, int n)
{
   size_t length=0;
   for ( ; n; v++, n-- )
      if (v->vec_len < 0)
          length += vec_length(v->vec_base, (int) -v->vec_len);
      else
          length += v->vec_len;

   return length;
}

/* ----------------------------------------------------------------------
 * Copy vector v[n] to dest. If limit is non-NULL, it will stop at limit,
 * and will update v to indicate consumed data.
 * -------------------------------------------------------------------- */
static void *vec_copy(void *dest, VCHI_MSG_VECTOR_T *v, int n, const void *limit)
{
   uint8_t *p = dest;
   const uint8_t *e = limit;
   for ( ; n && p != e; v++, n--)
   {
      if (v->vec_len < 0)
         p = vec_copy(p, (VCHI_MSG_VECTOR_T *) v->vec_base, (int) -v->vec_len, e);
      else if (v->vec_len > 0)
      {
         size_t len = (e == NULL || e - p > (int) v->vec_len) ? (size_t) v->vec_len : (size_t) (e - p);
         memcpy(p, v->vec_base, len);
         p += len;
         if (e)
         {
            v->vec_base = (uint8_t*)v->vec_base + len;
            v->vec_len -= len;
         }
      }
   }

   return p;
}
/* ----------------------------------------------------------------------
 * copy the message to the local memory with the correct formating,
 * padding etc.
 *
 * return -1 on error (message is too long for a slot);
 *         0 if fifo is full (ie. caller needs to wait for space to
 *                            become free)
 *        >0 returns the number of bytes written
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_form_message( VCHI_MDRIVER_HANDLE_T *handle,
                       fourcc_t service_id,
                       VCHI_MSG_VECTOR_T *vector,
                       uint32_t count,
                       void *address,
                       uint32_t available_length,
                       uint32_t max_total_length,
                       bool_t pad_to_fill,
                       bool_t allow_partial )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   int padding;
   uint8_t *ptr;

   // calculate length of all vectors
   uint16_t msg_length, len;
   msg_length = vec_length(vector, (int) count);
   len = msg_length;

   // add space for payload and alignment (if required)
   len += PAYLOAD_ADDRESS_OFFSET;
   padding = (int)len % VCHI_LEN_ALIGN;
   if ( padding )
      padding = VCHI_LEN_ALIGN - padding;
   len += padding;

   if ( allow_partial && len > available_length ) {
      os_assert(available_length % VCHI_LEN_ALIGN == 0);
      len = available_length;
      msg_length = len - PAYLOAD_ADDRESS_OFFSET;
      padding = 0;
   }

   // it would be bad if this didn't fit in a slot
   if ( len > max_total_length )
      return -1;

   // not enough room in this slot
   if ( len > available_length )
      return 0; // calling function needs to find another slot

   if ( pad_to_fill ) {
      padding += available_length - len;
      len = available_length;
   }

   ptr = address;
   os_assert( (((size_t)ptr) % state->tx_addr_align_uncached) == 0 );

   vchi_writebuf_uint16( ptr + SYNC_OFFSET,    VCHI_MSG_SYNC );
   vchi_writebuf_uint16( ptr + LENGTH_OFFSET,  len );
   vchi_writebuf_fourcc( ptr + FOUR_CC_OFFSET, service_id );
   vchi_writebuf_uint16( ptr + PAYLOAD_LENGTH_OFFSET, msg_length );
   // write the message (if present)
   ptr += PAYLOAD_ADDRESS_OFFSET;
   ptr = vec_copy( ptr, vector, (int) count, allow_partial ? ptr + msg_length : NULL );
   // memset to 0 any padding required
   if ( padding )
      memset( ptr, 0, (size_t) padding );

   return len;
}

/* ----------------------------------------------------------------------
 * Add slot info and timestamp to a message
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_update_message( VCHI_MDRIVER_HANDLE_T *handle, void *dest, int16_t *slot_count )
{
   uint8_t *ptr = (uint8_t *)dest;
   uint32_t timestamp = vchi_control_get_time();
   uint16_t delta;

   // Negative deltas can exist transiently - cope with this
   if ( *slot_count > 0 )
   {
      delta = *slot_count;
      *slot_count = 0;
   }
   else
      delta = 0;

   vchi_writebuf_uint32( ptr + TIMESTAMP_OFFSET,  timestamp  );
   vchi_writebuf_uint16( ptr + SLOT_COUNT_OFFSET, delta );

   //os_logging_message( "Outgoing timestamp = %u", timestamp );

   return vchi_readbuf_uint16( ptr + LENGTH_OFFSET );
}

/* ----------------------------------------------------------------------
 * Return VC_TRUE if buffer is suitably aligned for this connection
 * -------------------------------------------------------------------- */
static int32_t
mphi_msg_buffer_aligned( VCHI_MDRIVER_HANDLE_T *handle, int tx, int uncached, const void *address, const uint32_t length )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   int align = uncached ? (tx ? state->tx_addr_align_uncached : state->rx_addr_align_uncached )
		                : (tx ? state->tx_addr_align_cached : state->rx_addr_align_cached );
   if ((size_t)address % align != 0 || length % VCHI_LEN_ALIGN != 0)
      return VC_FALSE;
   else
      return VC_TRUE;
}

/* ----------------------------------------------------------------------
 * Allocate memory with suitable alignment and length for this connection
 * -------------------------------------------------------------------- */
static void *
mphi_msg_allocate_buffer( VCHI_MDRIVER_HANDLE_T *handle, uint32_t *length )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   void *addr;
   uint32_t align = OS_MAX( state->tx_addr_align_cached, state->rx_addr_align_cached );
   *length = (*length + (VCHI_LEN_ALIGN-1)) & ~(VCHI_LEN_ALIGN-1);

   // check that the alignments are powers of 2
   os_assert(OS_COUNT(align) == 1);
   os_assert(OS_COUNT(VCHI_LEN_ALIGN) == 1);
   os_assert(align > 0);
   os_assert((*length % VCHI_LEN_ALIGN) == 0);
   
#ifdef __VIDEOCORE4__
   // want at least 32-byte (cache-line) alignment for the buffer
   align = OS_MAX( align, 32 );
#endif

   addr = os_malloc( *length, align, "MPHI_BUFFER" );
#ifdef __VIDEOCORE4__
   state->cache_driver->flush_range( state->cache_h, CACHE_DEST_L1, addr, *length );
   addr = ALIAS_L1_NONALLOCATING( addr );
#endif
   memset( addr, 0, *length );
   return addr;
}

/* ----------------------------------------------------------------------
 * Free previously allocated memory
 * -------------------------------------------------------------------- */
static void
mphi_msg_free_buffer( VCHI_MDRIVER_HANDLE_T *handle, void *address )
{
   (void) os_free( address );
}

/* ----------------------------------------------------------------------
 * How much big does the receive slot have to be for a given max msg size?
 * -------------------------------------------------------------------- */
static int
mphi_msg_rx_slot_size( VCHI_MDRIVER_HANDLE_T *handle, int msg_size )
{
#ifdef VCHI_OVERRUN_BUG
   // Allow one unit worth of dummy data at the start - the transmitter
   // will send this as an overrunning trailer from the previous slot.
   msg_size += VCHI_LEN_ALIGN;
#endif
   return ((PAYLOAD_ADDRESS_OFFSET + msg_size) + (VCHI_LEN_ALIGN-1)) &~ (VCHI_LEN_ALIGN-1);
}

/* ----------------------------------------------------------------------
 * How much big does the transmit slot have to be for a given max msg size?
 * -------------------------------------------------------------------- */
static int
mphi_msg_tx_slot_size( VCHI_MDRIVER_HANDLE_T *handle, int msg_size )
{
   return ((PAYLOAD_ADDRESS_OFFSET + msg_size) + (VCHI_LEN_ALIGN-1)) &~ (VCHI_LEN_ALIGN-1);
}

/* ----------------------------------------------------------------------
 * Returns VC_TRUE if our peer will automatically handle advancing to the
 * next slot, and we don't send "terminate DMA" messages.
 * Returns VC_FALSE if our peer won't automatically advance to the next
 * slot, so we have to send "terminate DMA" messages.
 * -------------------------------------------------------------------- */
static bool_t
mphi_msg_tx_supports_terminate( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel )
{
   const MPHI_MSG_DRIVER_STATE_T *state = (const MPHI_MSG_DRIVER_STATE_T *)handle;

   if (IS_MPHI_TX_CHANNEL(channel))
      return state->tx_supports_terminate;
   else // CCP2
      return VC_TRUE;
}

/* ----------------------------------------------------------------------
 * Returns size of chunk to use for bulk transmission, or 0 if bulk
 * transmits shouldn't be chunked.
 * -------------------------------------------------------------------- */
static uint32_t
mphi_msg_tx_bulk_chunk_size( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel )
{
   const MPHI_MSG_DRIVER_STATE_T *state = (const MPHI_MSG_DRIVER_STATE_T *)handle;

   if (IS_MPHI_TX_CHANNEL(channel))
      return VCHI_MAX_BULK_CHUNK_SIZE_MPHI;
   else // CCP2
      return VCHI_MAX_BULK_CHUNK_SIZE_CCP2;
}

/* ----------------------------------------------------------------------
 * Returns alignment requirement for transmit buffers.
 * -------------------------------------------------------------------- */
static int
mphi_msg_tx_alignment( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_TX_CHANNEL_T channel )
{
   const MPHI_MSG_DRIVER_STATE_T *state = (const MPHI_MSG_DRIVER_STATE_T *)handle;

   return channel == MESSAGE_TX_CHANNEL_BULK ? state->tx_addr_align_cached : state->tx_addr_align_uncached;
}

/* ----------------------------------------------------------------------
 * Returns alignment requirement for receive buffers.
 * -------------------------------------------------------------------- */
static int
mphi_msg_rx_alignment( const VCHI_MDRIVER_HANDLE_T *handle, MESSAGE_RX_CHANNEL_T channel )
{
   const MPHI_MSG_DRIVER_STATE_T *state = (const MPHI_MSG_DRIVER_STATE_T *)handle;

   return channel == MESSAGE_RX_CHANNEL_BULK ? state->rx_addr_align_cached : state->rx_addr_align_uncached;
}


#ifdef VCHI_SUPPORT_CCP2TX
/* ----------------------------------------------------------------------
 * Rapu CCP2 receiver has an apparent fault - its 3-byte pipeline for
 * CCP2 FSP decoding isn't reset at start of line. This means that if the
 * first 1 or 2 bytes look like the tail of a 3-byte escapable sequence,
 * it may drop the next byte, misaligning the entire remainder of the line.
 * This also means the last few bytes may not get written, due to the
 * resulting misalignment. We create an auxiliary data structure that will
 * allow the receiver to detect and repair the error here.
 * -------------------------------------------------------------------- */
struct ccp_bulk_aux_line_info
{
   uint32_t detect; // bits 0..7 = check byte, bits 8..31 = offset within of check byte
   uint8_t  dropped;
   uint8_t  tail[7];
};

static void mphi_construct_aux_for_line(struct ccp_bulk_aux_line_info *info, const uint8_t *data, size_t len)
{
   size_t drop_pos, i;

   os_assert(len >= 8 && len < 0x1000000);

   memcpy(info->tail, data+len-sizeof info->tail, sizeof info->tail);

   if (data[0] == 0)
   {
      /* 2nd byte may be dropped */
      info->dropped = data[1];
      drop_pos = 1;
   }
   else if (data[1] == 0 && (data[0] & data[0]+1) == 0)
   {
      /* 3nd byte may be dropped */
      info->dropped = data[2];
      drop_pos = 2;
   }
   else
   {
      /* Nothing should be dropped */
      info->dropped = 0xA5;
      info->detect = data[0] | (0 << 8);
      return;
   }

   /* Find a byte in the packet that will change value if data[drop_pos] gets
    * dropped.
    */
   for (i = drop_pos; i < len-4; i++)
   {
      if (data[i+1] != data[i])
      {
         info->detect = data[i] | (i << 8);
         return;
      }
   }

   /* No such byte found: every byte from drop_pos to len-4 has the same value.
    * Given the potential loss of the last 3 bytes, this means the error is
    * undetectable. So we construct a false check that always makes the receiver
    * regard the packet as "wrong", reinsert the droppable byte, and rewrite the
    * tail. The net transformations in good and bad cases are thus:
    * ZZ DD DD DD DD TT TT TT            ZZ DD DD DD -- -- -- --
    *         becomes                           becomes
    * ZZ DD DD DD DD TT TT TT            ZZ DD DD DD DD TT TT TT
    */
   info->detect = (data[i] ^ 0xFF) | (0 << 8);
}
#endif

/* ----------------------------------------------------------------------
 * Return the message-driver-specific bulk auxiliary data for this message.
 * Message driver is responsible for allocation. The data will be copied
 * away before this call is made a second time, so a single static
 * buffer can be used.
 * -------------------------------------------------------------------- */
static void
mphi_msg_form_bulk_aux( VCHI_MDRIVER_HANDLE_T *handle,
                        MESSAGE_TX_CHANNEL_T channel,
                        const void *addr,
                        uint32_t len,
                        uint32_t chunk_size,
                        const void **aux_data,
                        int32_t *aux_len )
{
   *aux_data = NULL;
   *aux_len = 0;

#ifdef VCHI_SUPPORT_CCP2TX
   if (len > 0 && IS_CCP2_TX_CHANNEL(channel))
   {
      struct ccp_bulk_aux_line_info info;
      static uint8_t buffer[2048];
      uint8_t *p = buffer;
      
      os_assert( (len + chunk_size-1) / chunk_size * (5 + sizeof info.tail) <= sizeof buffer );

      while (len > 0)
      {
         uint32_t this_chunk = len < chunk_size ? len : chunk_size;
         mphi_construct_aux_for_line(&info, addr, this_chunk);

         vchi_writebuf_uint32(&p[0], info.detect);
         p[4] = info.dropped;
         memcpy(p+5, info.tail, sizeof info.tail);
         
         p += 5+sizeof info.tail;
         len -= this_chunk;
         addr = (uint8_t*)addr + this_chunk;
      }
      // XXX and what about dummy data? Shouldn't happen, I guess, as long as we support terminate

      *aux_data = buffer;
      *aux_len = p - buffer;
   }
#endif
}

#ifdef VCHI_SUPPORT_CCP2TX
/* ----------------------------------------------------------------------
 * Power management for CCP2TX interface
 * -------------------------------------------------------------------- */
void
mphi_check_ccp2tx_idle( MPHI_MSG_DRIVER_STATE_T *state )
{
#ifdef VCHI_CCP2TX_AUTOMATIC_POWER
   if (!CCP2_TX_FIFO_EMPTY(state) || state->tx_ccp2_power == CCP2TX_POWER_OFF)
      return;

   uint32_t t = os_get_time_in_usecs() - state->tx_ccp2_last_tx_time;
   int32_t success;
   if (VCHI_CCP2TX_OFF_TIMEOUT >= 0 && t > VCHI_CCP2TX_OFF_TIMEOUT*1000)
   {
      success = state->ccp2tx_driver->set_power( state->handle_ccp2tx_bulk,
                                                 CCP2TX_POWER_OFF );
      if (success == 0)
          state->tx_ccp2_power = CCP2TX_POWER_OFF;
      success = sysman_set_user_request(state->sysman_h, SYSMAN_BLOCK_CCP2TX, 0, SYSMAN_WAIT_BLOCKING);
      os_assert(success == 0);
   }
   else if (VCHI_CCP2TX_IDLE_TIMEOUT >= 0 && t > VCHI_CCP2TX_IDLE_TIMEOUT*1000 && state->tx_ccp2_power == CCP2TX_POWER_ON)
   {
      success = state->ccp2tx_driver->set_power( state->handle_ccp2tx_bulk,
                                                 CCP2TX_POWER_IDLE );
      if (success == 0)
          state->tx_ccp2_power = CCP2TX_POWER_IDLE;
   }
#endif
}
#endif

static void
mphi_msg_debug( VCHI_MDRIVER_HANDLE_T *handle )
{
   MPHI_MSG_DRIVER_STATE_T *state = (MPHI_MSG_DRIVER_STATE_T *)handle;
   os_logging_message( "  mphi_msg_debug:" );
   state->mphi_driver->debug( state->handle_control );
}

#if defined(DUMP_MESSAGES)
static void
dump_message( const char *reason, const unsigned char *addr, int len )
{
   static char msg[256];
   int i, m = OS_MIN( 60, len - 16 );
   char *ptr = msg;

   // NULL-terminated list of services we're interested in
   const char *filter[] = DUMP_MESSAGES;
   for (i=0; filter[i]; i++)
      if ( memcmp(filter[i],addr+4,4) == 0 )
         break;
   if ( !filter[i] ) return;

   ptr += sprintf( ptr, "{%p:%d} %04x %04x (%c%c%c%c) %d (sc=%d len=%d)",
                   addr, len,
                   vchi_readbuf_uint16(addr),
                   vchi_readbuf_uint16(addr+2),
                   addr[4], addr[5], addr[6], addr[7],
                   vchi_readbuf_uint32(addr+8),
                   vchi_readbuf_uint16(addr+12),
                   vchi_readbuf_uint16(addr+14) );

   for (i=0; i<m; i++)
      ptr += sprintf( ptr, " %02x", addr[i + 16] );
   if ( len > m )
      sprintf( ptr, " ..." );
   os_logging_message( "%s: %s", reason, msg );
}
#endif

/********************************** End of file ******************************************/
