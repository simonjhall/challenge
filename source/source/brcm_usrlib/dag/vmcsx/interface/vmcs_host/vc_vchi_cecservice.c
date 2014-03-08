/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  CEC service
Module   :  Software host interface (host-side)
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_cecservice.c#3 $
Revision :  $Revision: #3 $

FILE DESCRIPTION
CEC service host side API implementation
=============================================================================*/
#include <string.h>
#include "vchost_config.h"
#include "vchost.h"

#include "interface/vchi/vchi.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/os/os.h"
#include "vc_cecservice.h"

/******************************************************************************
Local types and defines.
******************************************************************************/
#ifndef _min
#define _min(x,y) (((x) <= (y))? (x) : (y))
#endif
#ifndef _max
#define _max(x,y) (((x) >= (y))? (x) : (y))
#endif

//TV service host side state (mostly the same as Videocore side - TVSERVICE_STATE_T)
typedef struct {
   //Generic service stuff
   VCHI_SERVICE_HANDLE_T client_handle[VCHI_MAX_NUM_CONNECTIONS]; //To connect to server on VC
   VCHI_SERVICE_HANDLE_T notify_handle[VCHI_MAX_NUM_CONNECTIONS]; //For incoming notification
   uint32_t              msg_flag[VCHI_MAX_NUM_CONNECTIONS];
   char                  command_buffer[CECSERVICE_MSGFIFO_SIZE];
   char                  response_buffer[CECSERVICE_MSGFIFO_SIZE];
   uint32_t              response_length;
   uint32_t              notify_buffer[CECSERVICE_MSGFIFO_SIZE/sizeof(uint32_t)];
   uint32_t              notify_length;
   uint32_t              num_connections;
   OS_SEMAPHORE_T        sema;
   CECSERVICE_CALLBACK_T notify_fn;
   void                 *notify_data;
   int                   initialised;

   //CEC state, not much here
   //Most things live on Videocore side
   CEC_RECEIVE_MSG_RESP_T rx_msg;
   VC_CEC_TOPOLOGY_T     *topology; //16-byte aligned for the transfer

} CECSERVICE_HOST_STATE_T;

/******************************************************************************
Static data.
******************************************************************************/
static CECSERVICE_HOST_STATE_T cecservice_client;
static OS_SEMAPHORE_T cecservice_message_available_semaphore;
static OS_SEMAPHORE_T cecservice_notify_available_semaphore;
static VCOS_THREAD_T cecservice_notify_task;

/******************************************************************************
Static functions.
******************************************************************************/
//Lock the host state
static void lock_obtain (void) {
   int32_t success;
   vcos_assert(cecservice_client.initialised);
   success = os_semaphore_obtain( &cecservice_client.sema );
   vcos_assert(success >= 0);
}

//Unlock the host state
static void lock_release (void) {
   int32_t success;
   vcos_assert(cecservice_client.initialised);
   vcos_assert(os_semaphore_obtained(&cecservice_client.sema));
   success = os_semaphore_release( &cecservice_client.sema );
   vcos_assert( success >= 0 );
}

//Forward declarations
static void cecservice_client_callback( void *callback_param,
                                       VCHI_CALLBACK_REASON_T reason,
                                       void *msg_handle );

static void cecservice_notify_callback( void *callback_param,
                                      VCHI_CALLBACK_REASON_T reason,
                                      void *msg_handle );

static int32_t cecservice_wait_for_reply(void *response, uint32_t max_length);

static int32_t cecservice_wait_for_bulk_receive(void *buffer, uint32_t max_length);

static int32_t cecservice_send_command( uint32_t command, const void *buffer, uint32_t length, uint32_t has_reply);

static int32_t cecservice_send_command_reply( uint32_t command, void *buffer, uint32_t length,
                                              void *response, uint32_t max_length);

static void cecservice_notify_func( unsigned int argc, void *argv );

/******************************************************************************
CEC service API
******************************************************************************/
/******************************************************************************
NAME
   vc_vchi_cec_init

SYNOPSIS
  void vc_vchi_cec_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections )

FUNCTION
  Initialise the CEC service for use.  A negative return value
   indicates failure (which may mean it has not been started on VideoCore).

RETURNS
   int
******************************************************************************/
VCHPRE_ void VCHPOST_ vc_vchi_cec_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int32_t success = -1;
   uint32_t i;

   // record the number of connections
   memset( &cecservice_client, 0, sizeof(CECSERVICE_HOST_STATE_T) );
   cecservice_client.num_connections = num_connections;
   success = os_semaphore_create( &cecservice_client.sema, OS_SEMAPHORE_TYPE_SUSPEND );
   vcos_assert( success == 0 );
   success = os_semaphore_create( &cecservice_message_available_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   vcos_assert( success == 0 );
   success = os_semaphore_obtain( &cecservice_message_available_semaphore );
   vcos_assert( success == 0 );
   success = os_semaphore_create( &cecservice_notify_available_semaphore, OS_SEMAPHORE_TYPE_SUSPEND );
   vcos_assert( success == 0 );
   success = os_semaphore_obtain( &cecservice_notify_available_semaphore );
   vcos_assert( success == 0 );

   cecservice_client.topology = os_malloc(sizeof(VC_CEC_TOPOLOGY_T), 16, "CEC topology");
   vcos_assert(cecservice_client.topology);

   for (i=0; i < cecservice_client.num_connections; i++) {
   
      // Create a 'Client' service on the each of the connections
      SERVICE_CREATION_T cecservice_parameters = { CECSERVICE_CLIENT_NAME,     // 4cc service code
                                                   connections[i],             // passed in fn ptrs
                                                   0,                          // tx fifo size (unused)
                                                   0,                          // tx fifo size (unused)
                                                   &cecservice_client_callback,// service callback
                                                   &cecservice_message_available_semaphore,  // callback parameter
                                                   VC_FALSE,                   // want_unaligned_bulk_rx
                                                   VC_FALSE,                   // want_unaligned_bulk_tx
                                                   VC_FALSE,                   // want_crc
      };

      SERVICE_CREATION_T cecservice_parameters2 = { CECSERVICE_NOTIFY_NAME,    // 4cc service code
                                                    connections[i],            // passed in fn ptrs
                                                    0,                         // tx fifo size (unused)
                                                    0,                         // tx fifo size (unused)
                                                    &cecservice_notify_callback,// service callback
                                                    &cecservice_notify_available_semaphore,  // callback parameter
                                                    VC_FALSE,                  // want_unaligned_bulk_rx
                                                    VC_FALSE,                  // want_unaligned_bulk_tx
                                                    VC_FALSE,                  // want_crc
      };

      //Create the client to normal CEC service 
      success = vchi_service_open( initialise_instance, &cecservice_parameters, &cecservice_client.client_handle[i] );
      vcos_assert( success == 0 );

      //Create the client to the async CEC service (any CEC related notifications)
      success = vchi_service_open( initialise_instance, &cecservice_parameters2, &cecservice_client.notify_handle[i] );
      vcos_assert( success == 0 );

   }

   //Create the notifier task
   success = os_thread_start(&cecservice_notify_task, cecservice_notify_func, &cecservice_client, 2048, "CEC Notify");
   vcos_assert( success == 0 );

   cecservice_client.initialised = 1;
}

/***********************************************************
 * Name: vc_vchi_cec_stop
 *
 * Arguments:
 *       -
 *
 * Description: Stops the Host side part of CEC service
 *
 * Returns: -
 *
 ***********************************************************/
VCHPRE_ void VCHPOST_ vc_vchi_cec_stop( void ) {
   // Wait for the current lock-holder to finish before zapping TV service
   uint32_t i;
   lock_obtain();
   //TODO: there is no API to stop the notifier task at the moment
   for (i=0; i < cecservice_client.num_connections; i++) {
      int32_t result;
      result = vchi_service_close(cecservice_client.client_handle[i]);
      assert( result == 0 );
      result = vchi_service_close(cecservice_client.notify_handle[i]);
      assert( result == 0 );
   }
   cecservice_client.initialised = 0;
   lock_release();
}

/***********************************************************
 * Name: vc_cec_register_callaback
 *
 * Arguments:
 *       callback function, context to be passed when function is called
 *
 * Description: Register a callback function for all CEC notifications
 *
 * Returns: -
 *
 ***********************************************************/
VCHPRE_ void VCHPOST_ vc_cec_register_callback(CECSERVICE_CALLBACK_T callback, void *callback_data) {
   lock_obtain();
   cecservice_client.notify_fn   = callback;
   cecservice_client.notify_data = callback_data;
   lock_release();
}

/*********************************************************************************
 *
 *  Static functions definitions
 *
 *********************************************************************************/
//TODO: Might need to handle multiple connections later
/***********************************************************
 * Name: cecservice_client_callback
 *
 * Arguments: semaphore, callback reason and message handle
 *
 * Description: Callback when a message is available for CEC service
 *
 ***********************************************************/
static void cecservice_client_callback( void *callback_param,
                                        const VCHI_CALLBACK_REASON_T reason,
                                        void *msg_handle ) {

   OS_SEMAPHORE_T *sem = (OS_SEMAPHORE_T *)callback_param;

   if ( reason != VCHI_CALLBACK_MSG_AVAILABLE )
      return;

   if ( sem == NULL )
      return;

   if ( os_semaphore_obtained(sem) ) {
      int32_t success = os_semaphore_release( sem );
      vcos_assert( success >= 0 );
   }

}

/***********************************************************
 * Name: cecservice_notify_callback
 *
 * Arguments: semaphore, callback reason and message handle
 *
 * Description: Callback when a message is available for CEC notify service
 *
 ***********************************************************/
static void cecservice_notify_callback( void *callback_param,
                                        const VCHI_CALLBACK_REASON_T reason,
                                        void *msg_handle ) {
   OS_SEMAPHORE_T *sem = (OS_SEMAPHORE_T *)callback_param;

   if ( reason != VCHI_CALLBACK_MSG_AVAILABLE )
      return;

   if ( sem == NULL )
      return;

   if ( os_semaphore_obtained(sem) ) {
      int32_t success = os_semaphore_release( sem );
      vcos_assert( success >= 0 );
   }

}

/***********************************************************
 * Name: cecservice_wait_for_reply
 *
 * Arguments: response buffer, buffer length
 *
 * Description: blocked until something is in the buffer
 *
 * Returns error code of vchi
 *
 ***********************************************************/
static int32_t cecservice_wait_for_reply(void *response, uint32_t max_length) {
   int32_t success = 0;
   int32_t sem_ok = 0;
   uint32_t length_read = 0;
   do {
      //TODO : we need to deal with messages coming through on more than one connections properly
      //At the moment it will always try to read the first connection if there is something there
      //Check if there is something in the queue, if so return immediately
      //otherwise wait for the semaphore and read again
      success = vchi_msg_dequeue( cecservice_client.client_handle[0], response, max_length, &length_read, VCHI_FLAGS_NONE );
   } while( length_read == 0 && (sem_ok = os_semaphore_obtain( &cecservice_message_available_semaphore)) == 0);
   
   return success;
}

/***********************************************************
 * Name: cecservice_wait_for_bulk_receive
 *
 * Arguments: response buffer, buffer length
 *
 * Description: blocked until bulk receive
 *
 * Returns error code of vchi
 *
 ***********************************************************/
static int32_t cecservice_wait_for_bulk_receive(void *buffer, uint32_t max_length) {
   vcos_assert(((uint32_t) buffer & 0xf) == 0); //should be 16 byte aligned
   return vchi_bulk_queue_receive( cecservice_client.client_handle[0],
                                   buffer,
                                   max_length,
                                   VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
                                   NULL );
}

/***********************************************************
 * Name: cecservice_send_command
 *
 * Arguments: command, parameter buffer, parameter legnth, has reply? (non-zero means yes)
 *
 * Description: send a command and optionally wait for its single value reponse (TV_GENERAL_RESP_T)
 *
 * Returns: response.ret (currently only 1 int in the wrapped response), endian translated if necessary
 *
 ***********************************************************/

static int32_t cecservice_send_command(  uint32_t command, const void *buffer, uint32_t length, uint32_t has_reply) {
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                  {buffer, length} };
   int32_t success = 0;
   int32_t response;
   lock_obtain();
   success = vchi_msg_queuev(cecservice_client.client_handle[0],
                             vector, sizeof(vector)/sizeof(vector[0]),
                             VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
   vcos_assert( success == 0 );
   if(success == 0 && has_reply) {
      //otherwise only wait for a reply if we ask for one
      success = cecservice_wait_for_reply(&response, sizeof(response));
      response = VC_VTOH32(response);
   } else {
      //No reply expected or failed to send, send the success code back instead
      response = success;
   }
   vcos_assert(success == 0);
   lock_release();
   return response;
}

/***********************************************************
 * Name: cecservice_send_command_reply
 *
 * Arguments: command, parameter buffer, parameter legnth, reply buffer, buffer length
 *
 * Description: send a command and wait for its non-single value reponse (in a buffer)
 *
 * Returns: error code, host app is responsible to do endian translation
 *
 ***********************************************************/
static int32_t cecservice_send_command_reply(  uint32_t command, void *buffer, uint32_t length,
                                               void *response, uint32_t max_length) {
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                  {buffer, length} };
   
   int32_t success = 0;
   lock_obtain();
   success = vchi_msg_queuev( cecservice_client.client_handle[0],
                              vector, sizeof(vector)/sizeof(vector[0]),
                              VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
   vcos_assert(!success);
   if(success == 0) {
      success = cecservice_wait_for_reply(response, max_length);
   }
   vcos_assert(success == 0);
   lock_release();

   return success;
}

/***********************************************************
 * Name: cecservice_notify_func
 *
 * Arguments: CEC service state
 *
 * Description: This is the notification task which receives all CEC
 *              service notifications
 *
 * Returns: does not return
 *
 ***********************************************************/
static void cecservice_notify_func( unsigned int argc, void *argv ) {
   int32_t success;
   CECSERVICE_HOST_STATE_T *state = (CECSERVICE_HOST_STATE_T *) argv;

   //first send a dummy message to the Videocore to let it know we are ready
   cecservice_send_command(VC_CEC_END_OF_LIST, NULL, 0, 0);

   while(1) {
      success = os_semaphore_obtain(&cecservice_notify_available_semaphore);
      vcos_assert(!success && state->initialised);

      do {
         uint32_t reason, param1, param2;
         //Get all notifications in the queue
         success = vchi_msg_dequeue( state->notify_handle[0], state->notify_buffer, sizeof(state->notify_buffer), &state->notify_length, VCHI_FLAGS_NONE );
         if(success != 0 || state->notify_length < sizeof(uint32_t)*3 ) {
            continue;
         }

         lock_obtain();
         //Check what notification it is and update ourselves accordingly before notifying the host app
         //All notifications are of format: reason, param1, param2 (all 32-bit unsigned int)
         reason = VC_VTOH32(state->notify_buffer[0]), param1 = VC_VTOH32(state->notify_buffer[1]), param2 = VC_VTOH32(state->notify_buffer[2]);

         //Unlike the TV service we have nothing to store here, so just call the callback

         lock_release();

         //Now callback the host app
         if(state->notify_fn) {
            (*state->notify_fn)(state->notify_data, reason, param1, param2);
         }
      } while(success == 0 && state->notify_length >= sizeof(uint32_t)*3); //read the next message if any
   } //while (1)
}

/***********************************************************
 Actual CEC service API starts here
***********************************************************/
/***********************************************************
 * Name: vc_cec_register_command
 *
 * Arguments:
 *       opcode to be registered
 *
 * Description
 *       Register an opcode to be forwarded as VC_CEC_RX notification
 *       The following opcode cannot be registered:
 *       <User Control Pressed>, <User Control Released>, 
 *       <Vendor Remote Button Down>, <Vendor Remote Button Up>,
 *       <Feature Abort>, <Abort>
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_register_command(CEC_OPCODE_T opcode) {
   int success = -1;
   uint32_t param = VC_HTOV32(opcode);
   success = cecservice_send_command( VC_CEC_REGISTER_CMD, &param, sizeof(param), 0);
   return success;
}

/***********************************************************
 * Name: vc_cec_register_all
 *
 * Arguments:
 *       None
 *
 * Description
 *       Register all commands except <Abort>
 *       Button presses/release will still be forwarded as 
 *       BUTTON_PRESSED/BUTTON_RELEASE notifications
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_register_all( void ) {
   return cecservice_send_command( VC_CEC_REGISTER_ALL, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_cec_deregister_command
 *
 * Arguments:
 *       opcode to be deregistered
 *
 * Description
 *       Deregister an opcode to be forwarded as VC_CEC_RX notification
 *       The following opcode cannot be deregistered:
 *       <User Control Pressed>, <User Control Released>, 
 *       <Vendor Remote Button Down>, <Vendor Remote Button Up>,
 *       <Feature Abort>, <Abort>
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_deregister_command(CEC_OPCODE_T opcode) {
   int success = -1;
   uint32_t param = VC_HTOV32(opcode);
   success = cecservice_send_command( VC_CEC_DEREGISTER_CMD, &param, sizeof(param), 0);
   return success;
}

/***********************************************************
 * Name: vc_cec_deregister_all
 *
 * Arguments:
 *       None
 *
 * Description
 *       Remove all commands to be forwarded. This does not affect
 *       the button presses which are always forwarded
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_deregister_all( void ) {
   return cecservice_send_command( VC_CEC_DEREGISTER_ALL, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_cec_send_message
 *
 * Arguments:
 *       Follower's logical address
 *       Message payload WITHOUT the header byte (can be NULL)
 *       Payload length WITHOUT the header byte (can be zero)
 *       VC_TRUE if the message is a reply to an incoming message
 *       (For poll message set payload to NULL and length to zero)
 *
 * Description
 *       Remove all commands to be forwarded. This does not affect
 *       the button presses which are always forwarded
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          If the command is successful, there will be a Tx callback
 *          in due course to indicate whether the message has been
 *          acknowledged by the recipient or not
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_message(uint32_t follower,
                                         const uint8_t *payload,
                                         uint32_t length,
                                         bool_t is_reply) {
   int success = -1;  
   CEC_SEND_MSG_PARAM_T param;
   vcos_assert(length <= CEC_MAX_XMIT_LENGTH);
   param.follower = VC_HTOV32(follower);
   param.length = VC_HTOV32(length);
   param.is_reply = VC_HTOV32(is_reply);
   memset(param.payload, 0, sizeof(param.payload));
   if(length > 0 && vcos_verify(payload)) {
      memcpy(param.payload, payload, _min(length, CEC_MAX_XMIT_LENGTH));
   }
   success = cecservice_send_command( VC_CEC_SEND_MSG, &param, sizeof(param), 1);
   return success;
}

/***********************************************************
 * Name: vc_cec_receive_message
 *
 * Arguments:
 *       pointer to initiator
 *       pointer to follower
 *       buffer to hold payload (max. 15 bytes)
 *       pointer to length
 *
 * Description
 *       Retrieve the most recent message, if there is no
 *       pending message, both initiator and follower will 
 *       be set to 0xF, otherwise length is set to length 
 *       of message minus the header byte.
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          command is deemed successful even if there is no
 *          pending message
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_receive_message(uint32_t *initiator,
                                            uint32_t *follower,
                                            uint8_t *buffer,
                                            uint32_t *length) {
   int success = -1;  
   vcos_assert(initiator && follower && buffer && length);
   success = cecservice_send_command_reply( VC_CEC_RECEIVE_MSG, NULL, 0, 
                                            &cecservice_client.rx_msg, sizeof(CEC_RECEIVE_MSG_RESP_T));
   if(success == 0) {
      cecservice_client.rx_msg.initiator = VC_VTOH32(cecservice_client.rx_msg.initiator);
      cecservice_client.rx_msg.follower  = VC_VTOH32(cecservice_client.rx_msg.follower);
      cecservice_client.rx_msg.length    = VC_VTOH32(cecservice_client.rx_msg.length);
      *initiator = cecservice_client.rx_msg.initiator;
      *follower  = cecservice_client.rx_msg.follower;
      *length    = cecservice_client.rx_msg.length;
      if(*length > 0) {
         memcpy(buffer, cecservice_client.rx_msg.payload, *length);
      }
   } else {
      *initiator = (uint32_t) CEC_AllDevices_eUnRegistered;
      *follower  = (uint32_t) CEC_AllDevices_eUnRegistered;
      *length    = 0L;
   }
   return success;
}

/***********************************************************
 * Name: vc_cec_get_logical_address
 *
 * Arguments:
 *       pointer to logical address
 *
 * Description
 *       Get the logical address, if one is being allocated
 *       0xF (unregistered) will be returned
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          logical_address is not modified if command failed
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_get_logical_address(CEC_AllDevices_T *logical_address) {
   uint32_t response;
   int32_t success = cecservice_send_command_reply( VC_CEC_GET_LOGICAL_ADDR, NULL, 0, 
                                                    &response, sizeof(response));
   if(success == 0) {
      *logical_address = (CEC_AllDevices_T)(VC_VTOH32(response) & 0xF);
   }
   return success;
}

/***********************************************************
 * Name: vc_cec_alloc_logical_address
 *
 * Arguments:
 *       None
 *
 * Description
 *       Start the allocation of a logical address. The host only
 *       needs to call this if the initial allocation failed
 *       (logical address being 0xF and physical address is NOT 0xFFFF
 *        from VC_CEC_LOGICAL_ADDR notification), or if the host explicitly
 *       released its logical address.
 * 
 * Returns: if the command is successful (zero) or not (non-zero)
 *         If successful, there will be a callback notification
 *         VC_CEC_LOGICAL_ADDR. The host should wait for this before
 *         calling this function again.
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_alloc_logical_address( void ) {
   return cecservice_send_command( VC_CEC_ALLOC_LOGICAL_ADDR, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_cec_release_logical_address
 *
 * Arguments:
 *       None
 *
 * Description
 *       Release our logical address. This effectively disables CEC.
 *       The host will need to allocate a new logical address before
 *       doing any CEC calls (send/receive message, get topology, etc.). 
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *         The host should get a callback VC_CEC_LOGICAL_ADDR with
 *         0xF being the logical address and 0xFFFF being the physical address.
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_release_logical_address( void ) {
   return cecservice_send_command( VC_CEC_RELEASE_LOGICAL_ADDR, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_cec_get_topology
 *
 * Arguments:
 *       pointer to topology struct
 *
 * Description
 *       Get the topology
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *         If successful, the topology will be set, otherwise it is unchanged
 *         A topology with 1 device (us) means CEC is not supported
 *         If there is no topology available, this also returns a failure.
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_get_topology( VC_CEC_TOPOLOGY_T* topology) {
   int32_t success = cecservice_send_command( VC_CEC_GET_TOPOLOGY, NULL, 0, 1);
   if(success == 0) {
      success = cecservice_wait_for_bulk_receive(cecservice_client.topology, sizeof(VC_CEC_TOPOLOGY_T));
   }
   if(success == 0) {
      int i;
      cecservice_client.topology->active_mask = VC_VTOH16(cecservice_client.topology->active_mask);
      cecservice_client.topology->num_devices = VC_VTOH16(cecservice_client.topology->num_devices);
      for(i = 0; i < 15; i++) {
         cecservice_client.topology->device_attr[i] = VC_VTOH32(cecservice_client.topology->device_attr[i]);
      }
      memcpy(topology, cecservice_client.topology, sizeof(VC_CEC_TOPOLOGY_T));
   }
   return success;
}

/***********************************************************
 * Name: vc_cec_set_vendor_id
 *
 * Arguments:
 *       24-bit IEEE vendor id
 *
 * Description
 *       Set the response to <Give Device Vendor ID>
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_set_vendor_id( uint32_t id ) {
   uint32_t vendor_id = VC_HTOV32(id);
   return cecservice_send_command( VC_CEC_SET_VENDOR_ID, &vendor_id, sizeof(vendor_id), 0);
}

/***********************************************************
 * Name: vc_cec_set_osd_name
 *
 * Arguments:
 *       OSD name (14 byte array)
 *
 * Description
 *       Set the response to <Give OSD Name>
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_set_osd_name( const char* name ) {
   return cecservice_send_command( VC_CEC_SET_OSD_NAME, name, OSD_NAME_LENGTH, 0); 
}

/***********************************************************
 * Name: vc_cec_get_physical_address
 *
 * Arguments:
 *       pointer to physical address (returned as 16-bit packed value)
 *
 * Description
 *       Get the physical address
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          If failed, physical address argument will not be changed
 *          A physical address of 0xFFFF means CEC is not supported
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_get_physical_address(uint16_t *physical_address) {
   uint32_t response;
   int32_t success = cecservice_send_command_reply( VC_CEC_GET_PHYSICAL_ADDR, NULL, 0, 
                                                    &response, sizeof(response));
   if(success == 0) {
      *physical_address = (uint16_t)(VC_VTOH32(response) & 0xFFFF);
   }
   return success;
}

/***********************************************************
 API for some common CEC messages, uses the API above to 
 actually send the message
***********************************************************/

/***********************************************************
 * Name: vc_cec_send_FeatureAbort
 *
 * Arguments:
 *       follower, rejected opcode, reject reason, reply or not
 *
 * Description
 *       send <Feature Abort> for a received command
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_FeatureAbort(uint32_t follower, 
                                              CEC_OPCODE_T opcode, 
                                              CEC_ABORT_REASON_T reason) { 
   uint8_t tx_buf[3];
   tx_buf[0] = CEC_Opcode_FeatureAbort;    // <Feature Abort>
   tx_buf[1] = opcode;
   tx_buf[2] = reason;
   return vc_cec_send_message(follower,
                              tx_buf,
                              sizeof(tx_buf),
                              VC_TRUE);
}

/***********************************************************
 * Name: vc_cec_send_ActiveSource
 *
 * Arguments:
 *       physical address, reply or not
 *
 * Description
 *       send <Active Source> 
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_ActiveSource(uint16_t physical_address, 
                                              bool_t is_reply) {
   uint8_t tx_buf[3];
   tx_buf[0] = CEC_Opcode_ActiveSource;    // <Active Source>
   tx_buf[1] = physical_address >> 8;      // physical address msb
   tx_buf[2] = physical_address & 0x00FF;  // physical address lsb
   return vc_cec_send_message(CEC_BROADCAST_ADDR, // This is a broadcast only message
                              tx_buf,
                              sizeof(tx_buf),
                              is_reply);
}

/***********************************************************
 * Name: vc_cec_send_ImageViewOn
 *
 * Arguments:
 *       follower, reply or not
 * Description
 *       send <Image View On> 
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_ImageViewOn(uint32_t follower, 
                                             bool_t is_reply) {
   uint8_t tx_buf[1];
   tx_buf[0] = CEC_Opcode_ImageViewOn;      // <Image View On> no param required
   return vc_cec_send_message(follower,
                              tx_buf,
                              sizeof(tx_buf),
                              is_reply);
}

/***********************************************************
 * Name: vc_cec_send_SetOSDString
 *
 * Arguments:
 *       follower, display control, string (char[13]), reply or not
 *
 * Description
 *       send <Image View On> 
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_SetOSDString(uint32_t follower, 
                                              CEC_DISPLAY_CONTROL_T disp_ctrl,                               
                                              const char* string,
                                              bool_t is_reply) {
   uint8_t tx_buf[CEC_MAX_XMIT_LENGTH];
   tx_buf[0] = CEC_Opcode_SetOSDString;     // <Set OSD String>
   tx_buf[1] = disp_ctrl;
   memset(&tx_buf[2], 0, sizeof(tx_buf)-2);
   memcpy(&tx_buf[2], string, _min(strlen(string), CEC_MAX_XMIT_LENGTH-2));
   return vc_cec_send_message(follower,
                              tx_buf,
                              sizeof(tx_buf),
                              is_reply);
}

/***********************************************************
 * Name: vc_cec_send_Standby
 *
 * Arguments:
 *       follower, reply or not
 *
 * Description
 *       send <Standby>. Turn other devices to standby 
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_Standby(uint32_t follower, bool_t is_reply) {
   uint8_t tx_buf[1];
   tx_buf[0] = CEC_Opcode_Standby;           // <Standby>
   return vc_cec_send_message(follower,
                              tx_buf,
                              sizeof(tx_buf),
                              is_reply);
}

/***********************************************************
 * Name: vc_cec_send_MenuStatus
 *
 * Arguments:
 *       follower, menu state, reply or not
 *
 * Description
 *       send <Menu Status> (response to <Menu Request>)
 *       menu state is either CEC_MENU_STATE_ACTIVATED or CEC_MENU_STATE_DEACTIVATED
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          Tx callback if successful
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_cec_send_MenuStatus(uint32_t follower, 
                                            CEC_MENU_STATE_T menu_state, 
                                            bool_t is_reply) {
   uint8_t tx_buf[2];
   vcos_assert(menu_state < CEC_MENU_STATE_QUERY);
   tx_buf[0] = CEC_Opcode_MenuStatus;        // <Menu Status>
   tx_buf[1] = menu_state;
   return vc_cec_send_message(follower,
                              tx_buf,
                              sizeof(tx_buf),
                              is_reply);
}
