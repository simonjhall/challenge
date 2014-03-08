/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile:  $
Revision :  $Revision$

FILE DESCRIPTION:
This file provides a local connection on a single processor via memory.
There are two sets of function pointers produced, one called local_server and
one called local_client. This does not restrict what is connected to which -
so clients can use local_server.
This is modelled on the MPHI interface as a test of the design.
The client side is the host and the server is VideoCore

UNDER CONSTRUCTION!

NOT YET IMPLEMENTED:
XON / XOFF signalling
SLOT counting
Doesn't request confirmation of presence of server during client startup
No shutdown / cleanup code
=============================================================================*/

#include <stdlib.h>
#include <string.h>

#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi.h"
#include "interface/vchi/connections/connection.h"
#include "interface/vchi/message_drivers/message.h"
#include "vcfw/rtos/rtos.h"
#include "vcfw/logging/logging.h"
#include "vcfw/rtos/nucleus/rtos_nucleus.h"
#include "thirdyparty/vcfw/rtos/nucleus/smpkernel/nucleus.h"
#include "interface/vchi/common/multiqueue.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/common/software_fifo.h"
#include "interface/vchi/common/non_wrap_fifo.h"
#include "interface/vchi/control_service/control_service.h"


/******************************************************************************
#defines
******************************************************************************/

#define LOCAL_CONNECTION_SYNC 0x50FA
#define VCHI_LOCAL_NUM_SLOTS 32  // how many slots each side has
#define VCHI_LOCAL_ALIGNMENT 1
#define VCHI_LOCAL_LENGTH_ALIGNMENT 1

#define NO_OF_BULK_RX 32    // how many bulk transfer receives can we queue

#define MAX_PENDING_TRANSFERS 10


/******************************************************************************
Private typedefs
******************************************************************************/

// bit mapped record of what has been done with a service
typedef enum {
   UNUSED  = 0x0,
   SERVER  = 0x1,
   CLIENT  = 0x2
} MPHI_STATE_T;

typedef enum {
   FREE = 0,
   IN_USE
} SLOT_STATE_T;

typedef struct {
  SLOT_STATE_T slot_state;
  uint8_t slot[VCHI_SLOT_SIZE];
  int messages;
} SLOT_T;

// information about a single service
typedef struct {
   fourcc_t service_id;
   MPHI_STATE_T state;
   VCHI_CALLBACK_T callback;
   const void *callback_param;
   int queue;
} SERVICE_INFO_T;


typedef struct lnode {
   struct lnode *next;
   void *data_dst;
   uint32_t data_size;
   void *bulk_handle;
   SERVICE_INFO_T *service;
} BULK_TRANSFER_T;


typedef struct {
   int32_t control;
   BULK_TRANSFER_T info;
} TX_INFO_T;

typedef struct
{
   //os semaphore use to protect this structure
   OS_SEMAPHORE_T semaphore;

   OS_SEMAPHORE_T transmit_semaphore;
   SERVICE_INFO_T services[ VCHI_MAX_SERVICES_PER_CONNECTION ];
   // pointers to the start and end of the linked list of unused message slots

   // we define the queues as follows:
   //   0 = free (all elements start on here initially)
   //   1 = queue for services[0]
   //   2 = queue for services[1]
   //   ...etc
   VCHI_MQUEUE_T *rx_mqueue;

   SLOT_T slots[VCHI_LOCAL_NUM_SLOTS];
   SLOT_T * current_slot;
   int write_index;
   uint16_t slot_count;     // how many slots have been added to DMA FIFO since last time we told the other side - in this implementation this is the number of freed slots

   // we need to store up our bulk receive requests
   SOFTWARE_FIFO_HANDLE_T rx_bulk_fifo;

   // server side tx marshalling FIFOs
   SOFTWARE_FIFO_HANDLE_T tx_bulk_fifo;
   NON_WRAP_FIFO_HANDLE_T tx_msg;

   SERVICE_INFO_T * control_service;


} MPHI_SERVICE_STATE_T;

MPHI_SERVICE_STATE_T local_client_state;
MPHI_SERVICE_STATE_T local_server_state;



//BULK_TRANSFER_T pending_client_to_server_transfers[MAX_PENDING_TRANSFERS];
//BULK_TRANSFER_T pending_server_to_client_transfers[MAX_PENDING_TRANSFERS];

VCHI_MQUEUE_T * pending_client_to_server_transfers;
VCHI_MQUEUE_T * pending_server_to_client_transfers;

/******************************************************************************
Static func fowards
******************************************************************************/
// API functions
//routine to init a connection
static int32_t local_client_init( void );
static int32_t local_server_init( void );

//routine to create a service
static int32_t local_client_server_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle );
static int32_t local_server_server_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle );

//routine to destroy a connection
static int32_t local_client_service_destroy( const VCHI_CONNECTION_SERVICE_HANDLE_T service_info );
static int32_t local_server_service_destroy( const VCHI_CONNECTION_SERVICE_HANDLE_T service_info );

//routine to connect to an exisiting service
static int32_t local_client_client_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle );
static int32_t local_server_client_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle );

//routine to disconnect from a service
static int32_t local_client_service_disconnect( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle );
static int32_t local_server_service_disconnect( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle );

//routine to queue a message
static int32_t local_client_service_queue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                               const void *data,
                                               const uint32_t data_size,
                                               const void *msg_handle);
static int32_t local_server_service_queue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                               const void *data,
                                               const uint32_t data_size,
                                               const void *msg_handle);

//routine to dequeue a message
static int32_t local_client_service_dequeue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data,
                                                 const uint32_t max_data_size_to_read,
                                                 uint32_t *actual_msg_size);
static int32_t local_server_service_dequeue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data,
                                                 const uint32_t max_data_size_to_read,
                                                 uint32_t *actual_msg_size);

//routine to peek at a message
static int32_t local_client_service_peek_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                              void **data,
                                              uint32_t *actual_msg_size);
static int32_t local_server_service_peek_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                              void **data,
                                              uint32_t *actual_msg_size);

//routine to remove a message
static int32_t local_client_service_remove_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle );
static int32_t local_server_service_remove_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle );

//routine to transmit bulk data
static int32_t local_client_bulk_queue_transmit( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data_src,
                                                 const uint32_t data_size,
                                                 const VCHI_FLAGS_T flags,
                                                 const void *bulk_handle );
static int32_t local_server_bulk_queue_transmit( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data_src,
                                                 const uint32_t data_size,
                                                 const VCHI_FLAGS_T flags,
                                                 const void *bulk_handle );

//routine to receive data
static int32_t local_client_bulk_queue_receive( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                const void *data_dst,
                                                const uint32_t data_size,
                                                const void *bulk_handle );
static int32_t local_server_bulk_queue_receive( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                const void *data_dst,
                                                const uint32_t data_size,
                                                const void *bulk_handle );

static int32_t local_client_server_present( const fourcc_t service_id );
static int32_t local_server_server_present( const fourcc_t service_id );

static int32_t local_client_format_message( const void * dest, const fourcc_t service_four_cc, const void * data, const uint32_t length );
static int32_t local_server_format_message( const void * dest, const fourcc_t service_four_cc, const void * data, const uint32_t length );

// Callback to indicate that the other side has added a buffer to the rx bulk DMA FIFO
static void local_client_rx_bulk_buffer_added( fourcc_t service, uint32_t length );
static void local_server_rx_bulk_buffer_added( fourcc_t service, uint32_t length );

// Callback to indicate that the other side has added a buffer to the tx bulk DMA FIFO
static void local_client_tx_bulk_buffer_added( fourcc_t service, uint32_t length );
static void local_server_tx_bulk_buffer_added( fourcc_t service, uint32_t length );


// Local functions
static SERVICE_INFO_T *find_free_service( fourcc_t service_id, MPHI_STATE_T client_or_server, const MPHI_SERVICE_STATE_T *state );
//static int32_t write_slot(MPHI_SERVICE_STATE_T *incoming_state,MPHI_SERVICE_STATE_T *outgoing_state,fourcc_t four_cc,
//                          const void * msg,const uint32_t msg_len,const void *msg_handle);
static int32_t remove_message( SLOT_T * slot );

static void transfer_message(MPHI_SERVICE_STATE_T *state, const void * msg, uint32_t msg_len);
static void server_signalling_data_to_be_read(int32_t control, uint32_t length, void * server_side_from);
static int32_t client_to_server_msg_transfer( SERVICE_INFO_T *service,
                                              const void * handle,
                                              const void * msg,
                                              const uint32_t msg_len);
static int32_t marshall_server_transmissions( SERVICE_INFO_T *service,
                                              const void * handle,
                                              const int32_t control,
                                              const void * address,
                                              const uint32_t length);

/******************************************************************************
Static data
******************************************************************************/

static VCHI_CONNECTION_T local_client_functable =
{
   &local_client_init,
   &local_client_server_connect,
   &local_client_service_destroy,
   &local_client_client_connect,
   &local_client_service_disconnect,
   &local_client_service_queue_msg,
   &local_client_service_dequeue_msg,
   &local_client_service_peek_msg,
   &local_client_service_remove_msg,
   &local_client_bulk_queue_transmit,
   &local_client_bulk_queue_receive,
   &local_client_server_present,
   &local_client_format_message,
   &local_client_rx_bulk_buffer_added,
   &local_client_tx_bulk_buffer_added

};
static VCHI_CONNECTION_T local_server_functable =
{
   &local_server_init,
   &local_server_server_connect,
   &local_server_service_destroy,
   &local_server_client_connect,
   &local_server_service_disconnect,
   &local_server_service_queue_msg,
   &local_server_service_dequeue_msg,
   &local_server_service_peek_msg,
   &local_server_service_remove_msg,
   &local_server_bulk_queue_transmit,
   &local_server_bulk_queue_receive,
   &local_server_server_present,
   &local_server_format_message,
   &local_server_rx_bulk_buffer_added,
   &local_server_tx_bulk_buffer_added
};

//static NU_HISR rx_hisr;

/******************************************************************************
Global functions
******************************************************************************/

/***********************************************************
 * Name: local_client_get_func_table
 * Name: local_server_get_func_table
 *
 * Arguments:  void
 *
 * Description: Routine to return the func table
 *
 * Returns: function table address
 *
 ***********************************************************/
VCHI_CONNECTION_T *local_client_get_func_table( void ) { return &local_client_functable; }
VCHI_CONNECTION_T *local_server_get_func_table( void ) { return &local_server_functable; }

/******************************************************************************
Static functions
******************************************************************************/

/***********************************************************
 * Name: local_client_init
 * Name: local_server_init
 *
 * Arguments:  void
 *
 * Description: routine to init a connection
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_init( void )
{
int32_t success = -1;
int i;
uint8_t * slot_ptr = (uint8_t *)NULL;

   // zero out our state information
   memset(&local_client_state, 0, sizeof(MPHI_SERVICE_STATE_T ));
   // initialise the semaphores
   success = os_semaphore_create( &local_client_state.semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   success = os_semaphore_create( &local_client_state.transmit_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   // initialise message queues
   local_client_state.rx_mqueue = vchi_mqueue_create( 1 + VCHI_MAX_SERVICES_PER_CONNECTION, VCHI_MAX_RX_MESSAGES, sizeof(MESSAGE_INFO_T) );
   for (i=0; i<VCHI_MAX_SERVICES_PER_CONNECTION; i++)
      local_client_state.services[i].queue = i + 1;
   // cause the first message to grab a free slot
   local_client_state.write_index = VCHI_SLOT_SIZE;
   // create a FIFO of bulk receive entries
   software_fifo_create(NO_OF_BULK_RX*sizeof(BULK_TRANSFER_T),VCHI_LOCAL_ALIGNMENT,&local_client_state.rx_bulk_fifo);
   // transmit FIFOs - tx_msg ensures that messages / bulk transfers are interleaved properly
   // tx_msg is the FIFO where messages are written into with their headers
   software_fifo_create(NO_OF_BULK_RX*sizeof(BULK_TRANSFER_T),VCHI_LOCAL_ALIGNMENT,&local_client_state.tx_bulk_fifo);
   //  fairly arbitrary size at the moment
   non_wrap_fifo_create(8*VCHI_SLOT_SIZE,VCHI_LOCAL_ALIGNMENT,&local_client_state.tx_msg);

   // local client is the only side to use this
   pending_client_to_server_transfers = vchi_mqueue_create( 2, MAX_PENDING_TRANSFERS, sizeof(BULK_TRANSFER_T) );
   pending_server_to_client_transfers = vchi_mqueue_create( 2, MAX_PENDING_TRANSFERS, sizeof(BULK_TRANSFER_T) );

   return(success);
}
static int32_t local_server_init( void )
{
int32_t success = -1;
int i;

   // zero out our state information
   memset(&local_server_state, 0, sizeof(MPHI_SERVICE_STATE_T ));
   // initialise the semaphores
   success = os_semaphore_create( &local_server_state.semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   success = os_semaphore_create( &local_server_state.transmit_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   // initialise message queues
   local_server_state.rx_mqueue = vchi_mqueue_create( 1 + VCHI_MAX_SERVICES_PER_CONNECTION, VCHI_MAX_RX_MESSAGES, sizeof(MESSAGE_INFO_T) );
   for (i=0; i<VCHI_MAX_SERVICES_PER_CONNECTION; i++)
      local_server_state.services[i].queue = i + 1;
   // cause the first message to grab a free slot
   local_server_state.write_index = VCHI_SLOT_SIZE;
   // create a FIFO of bulk receive entries
   software_fifo_create(NO_OF_BULK_RX*sizeof(BULK_TRANSFER_T),VCHI_LOCAL_ALIGNMENT,&local_server_state.rx_bulk_fifo);
   // transmit FIFOs - tx_msg ensures that messages / bulk transfers are interleaved properly
   // tx_msg is the FIFO where messages are written into with their headers
   software_fifo_create(NO_OF_BULK_RX*sizeof(BULK_TRANSFER_T),VCHI_LOCAL_ALIGNMENT,&local_server_state.tx_bulk_fifo);
   //  fairly arbitrary size at the moment
   non_wrap_fifo_create(8*VCHI_SLOT_SIZE,VCHI_LOCAL_ALIGNMENT,&local_server_state.tx_msg);

   // NB. some fifos are only needed on one side or the other so we will not waste the space allocating them unnecessarily

   return(success);
}


/***********************************************************
 * Name: local_client_service_create
 * Name: local_server_service_create
 *
 * Arguments: const uint32_t rx_fifo_size
 *            const uint32_t tx_fifo_size
 *            VCHI_CONNECTION_SERVICE_INFO_T *service_info
 *
 * Description: routine to create a service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_server_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_info )
{
   int32_t success = 0;
   SERVICE_INFO_T *service;

   // grab the semaphore
   os_semaphore_obtain(&local_client_state.semaphore);

   // this is where we record the service id and associated callback
   service = find_free_service( service_id, SERVER, &local_client_state );
   if(service)
   {
      // record the service ID
      service->service_id = service_id;
      // record that this is aserver
      service->state = SERVER;
      // record the callback
      service->callback = callback;
      // and the callback parameters
      service->callback_param = callback_param;
      // return a pointer to the information about the service
      *service_info = (VCHI_CONNECTION_SERVICE_HANDLE_T)service;
      // if this is the control service record it for when we want to send BULK_TRANSFER_?X messages
      if(service_id == MAKE_FOURCC("CTRL"))
         local_client_state.control_service = service;
      // all fine
      success = 0;
   }

   // release the semaphore
   os_semaphore_release(&local_client_state.semaphore);

   return success;
}
static int32_t local_server_server_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_info )
{
   int32_t success = 0;
   SERVICE_INFO_T *service;

   // grab the semaphore
   os_semaphore_obtain(&local_server_state.semaphore);

   // this is where we record the service id and associated callback
   service = find_free_service( service_id, SERVER, &local_server_state );
   if(service)
   {
      // record the service ID
      service->service_id = service_id;
      // record that this is aserver
      service->state = SERVER;
      // record the callback
      service->callback = callback;
      // and the callback parameters
      service->callback_param = callback_param;
      // return a pointer to the information about the service
      *service_info = (VCHI_CONNECTION_SERVICE_HANDLE_T)service;
      // if this is the control service record it for when we want to send BULK_TRANSFER_?X messages
      if(service_id == MAKE_FOURCC("CTRL"))
         local_server_state.control_service = service;
      // all fine
      success = 0;
   }

   // release the semaphore
   os_semaphore_release(&local_server_state.semaphore);

   return success;
}


/***********************************************************
 * Name: local_client_service_destroy
 * Name: local_server_service_destroy
 *
 * Arguments:  const VCHI_CONNECTION_SERVICE_INFO_T service_info
 *
 * Description: routine to destroy a connection
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_destroy( const VCHI_CONNECTION_SERVICE_HANDLE_T service_info )
{
   int32_t success = 0;
   return success;
}
static int32_t local_server_service_destroy( const VCHI_CONNECTION_SERVICE_HANDLE_T service_info )
{
   int32_t success = 0;
   return success;
}


/***********************************************************
 * Name: local_client_client_connect
 * Name: local_server_client_connect
 *
 * Arguments: fourcc_t service_id
 *            const uint32_t rx_fifo_size
 *            const uint32_t tx_fifo_size
 *            const VCHI_CALLBACK_T callback
 *            const void *callback_param
 *            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle
 *
 * Description: routine to connect to an exisiting service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_client_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle )
{
   int32_t success = -1;
   SERVICE_INFO_T *service;

   // grab the semaphore
   os_semaphore_obtain(&local_client_state.semaphore);
   // this is where we record the service id and associated callback
   service = find_free_service( service_id, CLIENT, &local_client_state );
   if(service)
   {
      // HACK HACK FIXME TBD - call the other side and check the server exists


      // record the service ID
      service->service_id = service_id;
      // record that this is a client
      service->state = CLIENT;
      // record the callback
      service->callback = callback;
      // and the callback parameters
      service->callback_param = callback_param;
      // return a pointer to the information about the service
      *service_handle = (VCHI_CONNECTION_SERVICE_HANDLE_T)service;
      // all fine
      success = 0;
   }
   // release the semaphore
   os_semaphore_release(&local_client_state.semaphore);

   return success;
}
static int32_t local_server_client_connect( fourcc_t service_id,
                                            const uint32_t rx_fifo_size,
                                            const uint32_t tx_fifo_size,
                                            const VCHI_CALLBACK_T callback,
                                            const void *callback_param,
                                            VCHI_CONNECTION_SERVICE_HANDLE_T *service_handle )
{
   int32_t success = -1;
   SERVICE_INFO_T *service;

   // grab the semaphore
   os_semaphore_obtain(&local_server_state.semaphore);
   // this is where we record the service id and associated callback
   service = find_free_service( service_id, CLIENT, &local_server_state );
   if(service)
   {
      // HACK HACK FIXME TBD - call the other side and check the server exists

      // record the service ID
      service->service_id = service_id;
      // record that this is a client
      service->state = CLIENT;
      // record the callback
      service->callback = callback;
      // and the callback parameters
      service->callback_param = callback_param;
      // return a pointer to the information about the service
      *service_handle = (VCHI_CONNECTION_SERVICE_HANDLE_T)service;
      // all fine
      success = 0;
   }
   // release the semaphore
   os_semaphore_release(&local_server_state.semaphore);

   return success;
}


/***********************************************************
 * Name: local_client_service_disconnect
 * Name: local_server_service_disconnect
 *
 * Arguments:  const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle
 *
 * Description: routine to disconnect from a service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_disconnect( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle )
{
   int32_t success = -1;

   // HACK HACK TBD FIXME
   // grab the semaphore

   // remove the service id from our list

   // release the semaphore

   return success;
}
static int32_t local_server_service_disconnect( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle )
{
   int32_t success = -1;

   // HACK HACK TBD FIXME
   // grab the semaphore

   // remove the service id from our list

   // release the semaphore

   return success;
}


/***********************************************************
 * Name: local_client_service_queue_msg
 * Name: local_server_service_queue_msg
 *
 * Arguments: const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
 *            const void *data
 *            const uint32_t data_size
 *            const void *msg_handle
 *
 * Description: routine to queue a message (i.e. send it to the Host)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_queue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                               const void *data,
                                               const uint32_t data_size,
                                               const void *msg_handle)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   // since we know this can just be sent as soon as the bus is free then the buffering need
   // only exist for the lifetime of this function call
   uint8_t outgoing_message[128];
   int32_t new_length;

   new_length = local_client_format_message(outgoing_message, service->service_id, data, data_size);

   // We are sending a message from the client (Host) to the server (VideoCore)
   // this just goes directly - we can assume that VideoCore has some slots ready for us
   if(new_length>0)
      success = client_to_server_msg_transfer(service,msg_handle,outgoing_message, new_length);

   return success;
}
static int32_t local_server_service_queue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                               const void *data,
                                               const uint32_t data_size,
                                               const void *msg_handle)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;

   // We are sending a message from the server (VideoCore) to the client (Host)
   // as this shares a channel with bulk transfers there is a degree of marshalling required
   success = marshall_server_transmissions(service, msg_handle, VC_TRUE, data, data_size);

   return success;
}


/***********************************************************
 * Name: local_client_service_dequeue_msg
 * Name: local_server_service_dequeue_msg
 *
 * Arguments: const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
 *            const void *data,
 *            const uint32_t max_data_size_to_read,
 *            uint32_t *actual_msg_size
 *
 * Description: routine to dequeue a message (i.e. read a message from the host)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_dequeue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data,
                                                 const uint32_t max_data_size_to_read,
                                                 uint32_t *actual_msg_size)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // we have had a callback to say thre is a message for the client (Host) (from VideoCore/server)
   msg = vchi_mqueue_peek( local_client_state.rx_mqueue, service->queue );
   if ( msg == NULL )
      return -1;

   os_assert( msg->len <= max_data_size_to_read );

   // check that there is enough room for the message
   if(msg->len <= max_data_size_to_read)
   {
      // copy the message into the supplied buffer
      memcpy((void *)data,msg->addr,msg->len);
      // record the actual message size
      *actual_msg_size = msg->len;
      // do the house keeping to free the message from the slot
      if(remove_message((SLOT_T *)msg->priv))
         local_client_state.slot_count++;
      // remove this item from the linked list and put it at the end of the free queue
      vchi_mqueue_move( local_client_state.rx_mqueue, service->queue, 0 );
      // we were successful
      success = 0;
   }
   return success;
}
static int32_t local_server_service_dequeue_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data,
                                                 const uint32_t max_data_size_to_read,
                                                 uint32_t *actual_msg_size)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // we have had a callback to say thre is a message for the server (VideoCore) (from Host/client)
   msg = vchi_mqueue_peek( local_server_state.rx_mqueue, service->queue );
   if ( msg == NULL )
      return -1;
   os_assert( msg );

   os_assert( msg->len <= max_data_size_to_read );

   // check that there is enough room for the message
   if(msg->len <= max_data_size_to_read)
   {
      // copy the message into the supplied buffer
      memcpy((void *)data,msg->addr,msg->len);
      // record the actual message size
      *actual_msg_size = msg->len;
      // do the house keeping to free the message from the slot
      if(remove_message((SLOT_T *)msg->priv))
         local_server_state.slot_count++;
      // remove this item from the linked list and put it at the end of the free queue
      vchi_mqueue_move( local_server_state.rx_mqueue, service->queue, 0 );
      // we were successful
      success = 0;
   }
   return success;
}


/***********************************************************
 * Name: local_client_service_peek_msg
 * Name: local_server_service_peek_msg
 *
 * Arguments: const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
 *            const void *data,
 *            uint32_t *actual_msg_size
 *
 * Description: Routine to return the address of the current message
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_peek_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                              void **data,
                                              uint32_t *actual_msg_size)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // We are using the message from the server (VideoCore) in place
   msg = vchi_mqueue_peek( local_client_state.rx_mqueue, service->queue );
   // check that there is something in the linked list
   if(msg != NULL)
   {
      *data = msg->addr;
      *actual_msg_size = msg->len;
      // all is fine
      success = 0;
   }
   return success;
}
static int32_t local_server_service_peek_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                              void **data,
                                              uint32_t *actual_msg_size)
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // We are using the message from the client (Host) in place
   msg = vchi_mqueue_peek( local_server_state.rx_mqueue, service->queue );
   // check that there is something in the linked list
   if(msg != NULL)
   {
      *data = msg->addr;
      *actual_msg_size = msg->len;
      // all is fine
      success = 0;
   }
   return success;
}


/***********************************************************
 * Name: local_client_service_remove_msg
 * Name: local_server_service_remove_msg
 *
 * Arguments: const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle
 *
 * Description: routine to dequeue a message (i.e. read a message from the host)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_service_remove_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle )
{
   int32_t success = 0;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // We are removing a message from the server (VideoCore)

   // read the message information so we know what to remove
   msg = vchi_mqueue_peek( local_client_state.rx_mqueue, service->queue );
   if ( msg == NULL )
      return -1;
   // do the house keeping to free the message from the slot
   if(remove_message((SLOT_T *)msg->priv))
      local_client_state.slot_count++;
   // move from the service queue to the free queue
   vchi_mqueue_move( local_client_state.rx_mqueue, service->queue, 0 );
   return success;
}
static int32_t local_server_service_remove_msg( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle )
{
   int32_t success = 0;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   MESSAGE_INFO_T *msg;

   // We are removing a message from the client (Host)

   // read the message information so we know what to remove
   msg = vchi_mqueue_peek( local_server_state.rx_mqueue, service->queue );
   if ( msg == NULL )
      return -1;
   // do the house keeping to free the message from the slot
   if(remove_message((SLOT_T *)msg->priv))
      local_server_state.slot_count++;
   // move from the service queue to the free queue
   vchi_mqueue_move( local_server_state.rx_mqueue, service->queue, 0 );
   return success;
}


/***********************************************************
 * Name: local_client_bulk_queue_transmit
 * Name: local_server_bulk_queue_transmit
 *
 * Arguments:  const void *data_src,
               const uint32_t data_size,
               const VCHI_FLAGS_T flags,
               const void *bulk_handle
 *
 * Description: routine to transmit bulk data
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_bulk_queue_transmit( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data_src,
                                                 const uint32_t data_size,
                                                 const VCHI_FLAGS_T flags,
                                                 const void *bulk_handle )
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   BULK_TRANSFER_T *bulk_info;
//   int i;

   // We are transmitting bulk data from the client (Host) to the server (Videocore)
   // so we must record that we wish to do so (handle's etc)
   // then we must wait for the message that tells us the other side is set up
   // then we must add this info to our pending queue of transfers
   // hopefully everything stays in sync

   // look at the first entry in the free list
   bulk_info = vchi_mqueue_peek( pending_client_to_server_transfers, 0 );

   if(bulk_info != NULL)
   {
      // fill in the details
      bulk_info->data_dst = (void *)data_src;
      bulk_info->data_size = data_size;
      bulk_info->bulk_handle = (void *)bulk_handle;
      bulk_info->service = service;

      // and move it to the pending queue
      vchi_mqueue_move( pending_client_to_server_transfers, 0, 1 );
      success = 0;
   }
   // check we have run out of room to be information in
   os_assert(success == 0);
   return(success);
}
static int32_t local_server_bulk_queue_transmit( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                 const void *data_src,
                                                 const uint32_t data_size,
                                                 const VCHI_FLAGS_T flags,
                                                 const void *bulk_handle )
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   uint8_t message[12];

   // need to form a message with BULK_TRANSFER_TX, service_id and length
   // BUT it must be transmitted on control service
   vchi_writebuf_uint32(BULK_TRANSFER_TX,&message[0]);
   vchi_writebuf_fourcc(service->service_id,&message[4]);
   vchi_writebuf_uint32(data_size,&message[8]);
   success = local_server_service_queue_msg( (VCHI_CONNECTION_SERVICE_HANDLE_T)local_server_state.control_service,message,12,NULL);

   // We are transmitting from the server (VideoCore) to the client (Host)
   // this goes down one channel together with the messages
   success = marshall_server_transmissions(service,bulk_handle,VC_FALSE,data_src,data_size);

   return success;
}


/***********************************************************
 * Name: local_client_bulk_queue_receive
 * Name: local_server_bulk_queue_receive
 *
 * Arguments:  const void *data_dst,
               const uint32_t data_size,
               const void *bulk_handle
 *
 * Description: routine to receive data
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
static int32_t local_client_bulk_queue_receive( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                const void *data_dst,
                                                const uint32_t data_size,
                                                const void *bulk_handle )
{
   int32_t success = -1;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   BULK_TRANSFER_T *bulk_info;
//   int i;

   // we need to just record that this transfer is pending.
   // The other side will signal when the DMA descriptor has been added to the output FIFO of the MPHI
   // HAT will then be asserted and we will pull off the matching transfer and do it

   // look at the first entry in the free list
   bulk_info = vchi_mqueue_peek( pending_server_to_client_transfers, 0 );

   if(bulk_info != NULL)
   {
      // fill in the details
      bulk_info->data_dst = (void *)data_dst;
      bulk_info->data_size = data_size;
      bulk_info->bulk_handle = (void *)bulk_handle;
      bulk_info->service = service;

      // and move it to the pending queue
      vchi_mqueue_move( pending_server_to_client_transfers, 0, 1 );
      success = 0;
   }
   // check we have run out of room to be information in
   os_assert(success == 0);
   return(success);
}
static int32_t local_server_bulk_queue_receive( const VCHI_CONNECTION_SERVICE_HANDLE_T service_handle,
                                                const void *data_dst,
                                                const uint32_t data_size,
                                                const void *bulk_handle )
{
   int32_t success = -1;
   BULK_TRANSFER_T transfer_info;
   SERVICE_INFO_T *service = (SERVICE_INFO_T *)service_handle;
   uint8_t message[12];

   // We are preparing to receive a bulk transfer from the client (Host)
   // We must first prime the transfer FIFO
   // and then send a message on the CTRL service to say we're ready to go

   // record this info so it can be put on the pending rx transfer FIFO
   transfer_info.data_dst = (void *)data_dst;
   transfer_info.data_size = data_size;
   transfer_info.bulk_handle = (void *)bulk_handle;
   transfer_info.service = service;

   // push the transfer information into the fifo
   software_fifo_protect(&local_server_state.rx_bulk_fifo);
   software_fifo_write(&local_server_state.rx_bulk_fifo,&transfer_info,sizeof(transfer_info));
   software_fifo_unprotect(&local_server_state.rx_bulk_fifo);

   // need to form a message with BULK_TRANSFER_RX, service_id and length
   // BUT it must be transmitted on control service
   vchi_writebuf_uint32(BULK_TRANSFER_RX,&message[0]);
   vchi_writebuf_fourcc(service->service_id,&message[4]);
   vchi_writebuf_uint32(data_size,&message[8]);
   success = local_server_service_queue_msg( (VCHI_CONNECTION_SERVICE_HANDLE_T)local_server_state.control_service,message,12,NULL);

   return success;
}


/***********************************************************
 * Name: find_free_service
 *
 * Arguments: fourcc_t service_id
 *            const MPHI_SERVICE_STATE_T *state
 *
 * Description: Returns the address of a free SERVICE_INFO_T structure
 *              if the service has not been created, the correct one if it has.
 *              NULL is returned if no slot is available.
 *              Must be called with the semaphore locked
 *
 * Returns: pointer to a usuable structure or NULL if none available
 *
 ***********************************************************/
static SERVICE_INFO_T *find_free_service( fourcc_t service_id, MPHI_STATE_T client_or_server, const MPHI_SERVICE_STATE_T *state )
{
int i;
SERVICE_INFO_T * address = (SERVICE_INFO_T *)NULL;

   for(i=0;i<VCHI_MAX_SERVICES_PER_CONNECTION;i++)
   {
      // record the first free slot we find
      if(address == (SERVICE_INFO_T *)NULL)
      {
         if(state->services[i].state == UNUSED)
            address = (SERVICE_INFO_T *)&state->services[i];
      }
      // look for the slot that is for this service
      if((state->services[i].service_id == service_id) && (state->services[i].state == client_or_server))
      {
         // Bad news this 4cc is already in use for this purpose
         address = (SERVICE_INFO_T *)NULL;
         os_assert(0);
         break;
      }
   }

   // return the address, which will be NULL if there has been any problem
   return( address );
}


/***********************************************************
 * Name: local_client_videocore_report_server
 * Name: local_server_videocore_report_server
 *
 * Arguments: const int service_number        // nth server requested
 *            const fourcc_t * service_id     // server found
 *
 * Description: Reports the nth server (used to generate list of servers
 *              for the SERVERS_AVAILABLE command
 *
 * Returns: 0 if server found, no service found otherwise
 *
 ***********************************************************/
static int32_t local_client_server_present( const fourcc_t service_id )
{
int i;

   for(i=0;i<VCHI_MAX_SERVICES_PER_CONNECTION;i++)
   {
      if(local_client_state.services[i].state == SERVER)
      {
         if(service_id == local_client_state.services[i].service_id)
             return(0);
      }
   }
   return(-1);
}
static int32_t local_server_server_present( const fourcc_t service_id )
{
int i;

   for(i=0;i<VCHI_MAX_SERVICES_PER_CONNECTION;i++)
   {
      if(local_server_state.services[i].state == SERVER)
      {
         if(service_id == local_server_state.services[i].service_id)
            return(0);
      }
   }
   return(-1);
}


/***********************************************************
 * Name: remove_message
 *
 * Arguments: SLOT_INFO * slot
 *
 * Description: Handles the removal of a message from a slot
 *
 * Returns: 1 if a slot is freed, 0 otherwise
 *
 ***********************************************************/
static int32_t remove_message(SLOT_T * slot)
{
   slot->messages--;
   if(slot->messages == 0)
   {
      slot->slot_state = FREE;
      return(1);
   }
   return(0);
}


/***********************************************************
 * Name: local_client_format_message
 * Name: local_server_format_message
 *
 * Arguments: const void * dest
 *            fourcc_t service_four_cc
 *            void * data
 *            uint32 * length
 *
 * Description: Formats a header on to the front of a message and pads it as necessary
 *
 * NB. this needs to be local to the connection so it can enforce (or at least check)
 *     alignment / length requirements and allow a per connection
 *     'Sync' magic value.
 *
 * Returns: length of the formated message
 *
 ***********************************************************/
static int32_t local_client_format_message( const void * dest, const fourcc_t service_four_cc, const void * data, const uint32_t length )
{
int32_t new_length = -1;

   // make sure we are meeting the alignment requirement
   if(((uint32_t)dest % VCHI_LOCAL_ALIGNMENT) != 0)
   {
      os_assert(0);
      return(new_length);
   }
   // now we have the alignment issue out of the way we can proceed with writing the message
   new_length = vchi_control_format_message(dest, LOCAL_CONNECTION_SYNC,local_client_state.slot_count,
                                            VCHI_LOCAL_LENGTH_ALIGNMENT,service_four_cc,data,length);
   // now we've informed the other side we need to reset the value
   local_client_state.slot_count = 0;

   return(new_length);
}
static int32_t local_server_format_message( const void * dest, const fourcc_t service_four_cc, const void * data, const uint32_t length )
{
int32_t new_length = -1;

   // make sure we are meeting the alignment requirement
   if(((uint32_t)dest % VCHI_LOCAL_ALIGNMENT) != 0)
   {
      os_assert(0);
      return(new_length);
   }
   // now we have the alignment issue out of the way we can proceed with writing the message
   new_length = vchi_control_format_message(dest, LOCAL_CONNECTION_SYNC,local_server_state.slot_count,
                                            VCHI_LOCAL_LENGTH_ALIGNMENT,service_four_cc,data,length);
   // now we've informed the other side we need to reset the value
   local_server_state.slot_count = 0;

   return(new_length);
}


/***********************************************************
 * Name: local_client_rx_bulk_buffer_added
 * Name: local_server_rx_bulk_buffer_added
 *
 * Arguments: fourcc_t service
 *            uint32_t length
 *
 * Description: Routine is called from the control service when
 *              BULK_TRANSFER_RX is received
 *
 * Returns: -
 *
 ***********************************************************/
static void local_client_rx_bulk_buffer_added(fourcc_t service, uint32_t length)
{
   // the client (Host) side has just been informed that a DMA descriptor
   // has been added to the rx bulk FIFO of the MPHI
   // Now look through the list of pending transmissions and when we match
   // service and length do the actual transfer
   BULK_TRANSFER_T *transfer_info;
   BULK_TRANSFER_T transfer;

//   int i;

   // get the first entry from the queue
   transfer_info = vchi_mqueue_peek( pending_client_to_server_transfers, 1 );

   while(transfer_info != NULL)
   {
      if((transfer_info->service->service_id == service) && (transfer_info->data_size == length))
      {
         // remove the entry from the queue
         if(vchi_mqueue_element_free( pending_client_to_server_transfers, 1, transfer_info ) != 0)
            os_assert(0);
         // read out the next entry in the server rx_bulk FIFO
         software_fifo_protect(&local_server_state.rx_bulk_fifo);
         software_fifo_read(&local_server_state.rx_bulk_fifo,&transfer,sizeof(transfer));
         software_fifo_unprotect(&local_server_state.rx_bulk_fifo);
         // now do the transfer
         memcpy(transfer.data_dst,transfer_info->data_dst,length);
         // now run both sides callbacks
         // client side
         transfer_info->service->callback((void *)transfer_info->service->callback_param,VCHI_CALLBACK_DATA_SENT,transfer_info->bulk_handle);
         // server side
         transfer.service->callback((void *)transfer.service->callback_param,VCHI_CALLBACK_DATA_ARRIVED,transfer.bulk_handle);
          return;
      }
      transfer_info = transfer_info->next;
   }
   // we didn't find the corresponding entry
   os_assert(0);
}
static void local_server_rx_bulk_buffer_added(fourcc_t service, uint32_t length)
{
   // the server side does not need to know what is going on on the client side
   os_assert(0);
}


/***********************************************************
 * Name: local_client_tx_bulk_buffer_added
 * Name: local_server_tx_bulk_buffer_added
 *
 * Arguments: fourcc_t service
 *            uint32_t length
 *
 * Description: Routine is called from the control service when
 *              BULK_TRANSFER_TX is received
 *
 * Returns: -
 *
 ***********************************************************/
static void local_client_tx_bulk_buffer_added(fourcc_t service, uint32_t length)
{
   BULK_TRANSFER_T *transfer_info;
  // BULK_TRANSFER_T transfer;

//   int i;

   // get the first entry from the queue
   transfer_info = vchi_mqueue_peek( pending_server_to_client_transfers, 1 );


   while(transfer_info != NULL)
   {
      if((transfer_info->service->service_id == service) && (transfer_info->data_size == length))
      {
         // remove the entry from the queue
         if(vchi_mqueue_element_free( pending_server_to_client_transfers, 1, transfer_info ) != 0)
            os_assert(0);
         // write information into FIFO for later use
         software_fifo_protect(&local_client_state.rx_bulk_fifo);
         software_fifo_write(&local_client_state.rx_bulk_fifo,transfer_info,sizeof(BULK_TRANSFER_T));
         software_fifo_unprotect(&local_client_state.rx_bulk_fifo);


         // read out the next entry in the server rx_bulk FIFO
//         software_fifo_protect(&local_server_state.rx_bulk_fifo);
//         software_fifo_read(&local_server_state.rx_bulk_fifo,&transfer,sizeof(transfer));
//         software_fifo_unprotect(&local_server_state.rx_bulk_fifo);
         // now do the transfer
//         memcpy(transfer.data_dst,transfer_info->data_dst,length);
         // now run both sides callbacks
         // client side
//         transfer_info->service->callback((void *)transfer_info->service->callback_param,VCHI_CALLBACK_DATA_SENT,transfer_info->bulk_handle);
         // server side
//         transfer.service->callback((void *)transfer.service->callback_param,VCHI_CALLBACK_DATA_ARRIVED,transfer.bulk_handle);
         return;
      }
      transfer_info = transfer_info->next;
   }
   // we didn't find the corresponding entry
   os_assert(0);
}
static void local_server_tx_bulk_buffer_added(fourcc_t service, uint32_t length)
{
   // the server side does not need to know what is going on on the client side
   os_assert(0);
}


/***********************************************************
 * Name: marshall_server_transmissions
 *
 * Arguments: SERVICE_INFO_T *service
 *            const void * handle,
 *            const int32_t control,
 *            const void * address,
 *            const uint32_t length
 *
 * Description: This routine replicates the behaviour needed on the client side
 *              to handle control and data traffic on the single back channel
 *
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
static int32_t marshall_server_transmissions(SERVICE_INFO_T *service,
                                             const void * handle,
                                             const int32_t control,
                                             const void * address,
                                             const uint32_t length)
{
void *address_to_write_msg;
int32_t success;
int32_t new_length;
TX_INFO_T tx_info;

   os_semaphore_obtain(&local_client_state.transmit_semaphore);

   tx_info.control = control;
   tx_info.info.data_dst = (void *)address;
   tx_info.info.data_size = length;
   tx_info.info.bulk_handle = (void *)handle;
   tx_info.info.service = service;

   if(control)
   {
      // transferring a message
      success = non_wrap_fifo_request_write_address( &local_server_state.tx_msg, &address_to_write_msg, length+32 );
      os_assert(success == 0);
      // form message in non-wrap FIFO
      new_length = local_server_format_message(address_to_write_msg, service->service_id, address, length);
      os_assert(success == 0);
      // let the fifo know we've completed
      success = non_wrap_fifo_write_complete( &local_server_state.tx_msg, address_to_write_msg );
      os_assert(success == 0);
      // record our new address and length for the message
      tx_info.info.data_dst = address_to_write_msg;
      tx_info.info.data_size = new_length;
   }
   // record the info in our transmission FIFO
   software_fifo_write(&local_server_state.tx_bulk_fifo,&tx_info,sizeof(TX_INFO_T));

   // we've finished writing to the FIFO, asynchronous reads may still be a problem though
   // because of the CTRL service calling back into to here as a result of the CONNECT message
   // we can't keep the semaphore for longer
   // NOT an issue when running with proper interrupt
   os_semaphore_release(&local_client_state.transmit_semaphore);

   // so we need now to read the FIFO to see what is pending
   software_fifo_read(&local_server_state.tx_bulk_fifo,&tx_info,sizeof(TX_INFO_T));

   // pretend we are running an interrupt on the client (Host) side
   // HACK HACK FIXME TBD - run this in a HISR?????
   server_signalling_data_to_be_read(tx_info.control, tx_info.info.data_size, tx_info.info.data_dst);


   // can now signal that the data has been transmitted or the message - whichever is appropriate
   if(tx_info.control)
   {
      // message
      tx_info.info.service->callback((void *)tx_info.info.service->callback_param,VCHI_CALLBACK_MSG_SENT,tx_info.info.bulk_handle);
   }
   else
   {
      // bulk transfer
      tx_info.info.service->callback((void *)tx_info.info.service->callback_param,VCHI_CALLBACK_DATA_SENT,tx_info.info.bulk_handle);
   }

   return(0);
}


/***********************************************************
 * Name: client_to_server_msg_transfer
 *
 * Arguments: SERVICE_INFO_T *service
 *            const void * handle
 *            const void * msg
 *            const uint32_t msg_len
 *
 * Description: Do the memcpy and run the callback
 *
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
static int32_t client_to_server_msg_transfer( SERVICE_INFO_T *service,
                                              const void * handle,
                                              const void * msg,
                                              const uint32_t msg_len )
{
   transfer_message(&local_server_state, msg, msg_len);
   // run the client side callback
   service->callback((void *)service->callback_param,VCHI_CALLBACK_MSG_SENT,handle);
   return(0);
}


/***********************************************************
 * Name: server_signalling_data_to_be_read
 *
 * Arguments: int32_t control
 *            uint32_t length
 *            void * server_side_from
 *
 * Description: This routine replicates the response to the setting of the HAT pin
 *              on the MPHI interface. Obviously on the real system server_side_from
 *              is not actually available
 *
 * Returns: -
 *
 ***********************************************************/
static void server_signalling_data_to_be_read(int32_t control, uint32_t length, void * server_side_from)
{
   if(control)
   {
      // a message is ready to be handled - this should always work
      transfer_message(&local_client_state, server_side_from, length);
   }
   else
   {
      // this is a bulk data request
      BULK_TRANSFER_T transfer;

      // get the currently queued transfer (if there is one)
      if(software_fifo_read(&local_client_state.rx_bulk_fifo,&transfer,sizeof(BULK_TRANSFER_T)) == 0)
      {
         // confirm that at least the size of the transfer matches
         os_assert(transfer.data_size == length);
         // do the copy
         memcpy(transfer.data_dst,server_side_from,length);
         // run the callback with the supplied 'cookie'
         transfer.service->callback((void *)transfer.service->callback_param,VCHI_CALLBACK_DATA_ARRIVED,transfer.bulk_handle);
      }
      else
         os_assert(0);  // the setting up of the transfer on the client (Host) side has gone awry - or we've queued up something without it being requested...
   }
}


/***********************************************************
 * Name: transfer_message
 *
 * Arguments: MPHI_SERVICE_STATE_T *state
 *            const void * msg
 *            uint32_t msg_len
 *
 * Description: This routine handles the passing of messages between the two sides
 *
 * Returns: -
 *
 ***********************************************************/
static void transfer_message(MPHI_SERVICE_STATE_T *state, const void * msg, uint32_t msg_len)
{
// information extracted from the message
uint16_t slot_count;
fourcc_t service_four_cc;
void *   msg_address;
uint32_t msg_length;
// need to convert from 4cc to a pointer to all the info about that service
SERVICE_INFO_T * service = (SERVICE_INFO_T *)NULL;
int i;
MESSAGE_INFO_T *msg_info;

   // obtain the semaphore
   os_semaphore_obtain(&state->semaphore);

   // see if we need to change slot
   if(state->write_index + msg_len >= VCHI_SLOT_SIZE)
   {
      // make sure we can identify if we have not found a free slot
      state->current_slot = (SLOT_T *)NULL;
      // change to another slot
      for(i=0;i<VCHI_LOCAL_NUM_SLOTS;i++)
      {
         if(state->slots[i].slot_state == FREE)
         {
            state->slots[i].slot_state = IN_USE;
            state->current_slot = &state->slots[i];
            break;
         }
      }
      if(state->current_slot == (SLOT_T *)NULL)
      {
         os_assert(0);
         return;
      }
      // reset the write pointer
      state->write_index = 0;
   }
   // copy the message
   memcpy(&state->current_slot->slot[state->write_index],msg,msg_len);

   // parse the message, pull out the info we need so we can add it to the correct services queue
   vchi_control_extract_message(&state->current_slot->slot[state->write_index],
                                LOCAL_CONNECTION_SYNC, &slot_count, &service_four_cc, &msg_address,&msg_length);

   // need to find the service pointer
   for(i=0;i<VCHI_MAX_SERVICES_PER_CONNECTION;i++)
   {
      if(service_four_cc == state->services[i].service_id)
      {
         service = &state->services[i];
         break;
      }
   }
   if(service == (SERVICE_INFO_T *)NULL)
   {
      // this service is not available!
      // FATAL assert
      os_assert(0);
   }

   // get a pointer to the first element of the free queue
   msg_info = vchi_mqueue_peek( state->rx_mqueue, 0 );
   os_assert( msg_info ); // this would be bad - no free elements!

   msg_info->addr = msg_address;
   msg_info->len = msg_length;
// HACK HACK HACK FIXME TBD - no longer need to have service recorded because we know this from the queue
//   msg_info->service = service_four_cc;
   msg_info->priv = state->current_slot;
   // move the revised entry to the queue for this service
   vchi_mqueue_move( state->rx_mqueue, 0, service->queue );
   // increment our count of the number of messages in this slot
   state->current_slot->messages++;
   // keep track of where we are writing to
   state->write_index += msg_len;
   // release the semaphore
   os_semaphore_release(&state->semaphore);
   // run the callback indicating that a message has come in
   service->callback((void *)service->callback_param,VCHI_CALLBACK_REASON_DATA_AVAILABLE,NULL);
}


/****************************** End of file ***********************************/

