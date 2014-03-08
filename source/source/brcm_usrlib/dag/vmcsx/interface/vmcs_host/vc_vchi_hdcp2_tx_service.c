/*=============================================================================
Copyright (c) 2011 Broadcom Europe Limited.
All rights reserved.

Project  :  HDCP2 Service
Module   :  Common host interface
File     :  $File: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_hdcp2_tx_service.c $
Revision :  $Revision: #1 $

FILE DESCRIPTION
HDCP2 host side service API (Tx) implementation
See vc_hdcp2.h typedefs

=============================================================================*/
#include <string.h>
#include "vchost_config.h"
#include "vchost.h"

#include "interface/vcos/vcos_logging.h"
#include "vc_hdcp2_service_common.h"
#include "vc_hdcp2_tx_service.h"

#include "vcfw/logging/logging.h"

/******************************************************************************
Static data.
******************************************************************************/
static HDCP2_SERVICE_HOST_STATE_T hdcp2_tx_service_client = {0};
static VCOS_SEMAPHORE_T hdcp2_tx_service_message_available_semaphore;
static VCOS_SEMAPHORE_T hdcp2_tx_service_notify_available_semaphore;
static VCOS_THREAD_T hdcp2_tx_service_notify_task;

/******************************************************************************
Static functions.
******************************************************************************/
//Lock the host state
static void lock_obtain (void) {
   VCOS_STATUS_T status;
   vcos_assert(hdcp2_tx_service_client.initialised);
   status = vcos_semaphore_wait( &hdcp2_tx_service_client.sema );
   vcos_assert(status == VCOS_SUCCESS);
}

//Unlock the host state
static void lock_release (void) {
   VCOS_STATUS_T status;
   vcos_assert(hdcp2_tx_service_client.initialised);
   status = vcos_semaphore_post( &hdcp2_tx_service_client.sema );
   vcos_assert( status == VCOS_SUCCESS );
}

//Task handling incoming messages from Videocore
static void *hdcp2_tx_service_notify_func( void *argv );

/******************************************************************************
HDCP2 service API
******************************************************************************/

/**
 * <DFN>vc_vchi_hdcp2_tx_init</DFN> is called at the beginning of the application
 * to initialise the client to Tx side HDCP2 service
 * 
 * @param initialise_instance is the VCHI instance
 *
 * @param connections is the array of pointers of connections
 * 
 * @param num_connections is the length of the array
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_vchi_hdcp2_tx_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int32_t success;
   uint32_t i;
   VCOS_STATUS_T status;
   const uint32_t stack_size = 4096;
   void *stack;

   vcos_assert(!hdcp2_tx_service_client.initialised);
   vcos_log_register("VCOS_LOG_TRACE", &hdcp2_log_category);

   // record the number of connections
   memset( &hdcp2_tx_service_client, 0, sizeof(HDCP2_SERVICE_HOST_STATE_T) );
   hdcp2_tx_service_client.num_connections = num_connections;
   //Semaphore controlling access to HDCP2 Tx service
   status = vcos_semaphore_create( &hdcp2_tx_service_client.sema, "HDCP2 Tx client sema", 1);
   vcos_assert(status == VCOS_SUCCESS);

   //These semaphores are initially locked, to be signalled by incoming replies/asychronous messages
   status = vcos_semaphore_create( &hdcp2_tx_service_message_available_semaphore, "HDCP2 Tx client reply", 0);
   vcos_assert(status == VCOS_SUCCESS);
   status = vcos_semaphore_create( &hdcp2_tx_service_notify_available_semaphore, "HDCP2 Tx client notify", 0);
   vcos_assert(status == VCOS_SUCCESS);
  
   for (i=0; i < hdcp2_tx_service_client.num_connections; i++) {

      // Create a 'Client' service on the each of the connections
      SERVICE_CREATION_T hdcp2_tx_service_parameters = { HDCP2SERVICE_TX_CLIENT_NAME,      // 4cc service code
                                                         connections[i],             // passed in fn ptrs
                                                         0,                          // tx fifo size (unused)
                                                         0,                          // tx fifo size (unused)
                                                         &hdcp2_service_callback,    // service callback (reused the one on Videocore side)
                                                         &hdcp2_tx_service_message_available_semaphore,  // callback parameter
                                                         VC_FALSE,                   // want_unaligned_bulk_rx
                                                         VC_FALSE,                   // want_unaligned_bulk_tx
                                                         VC_FALSE,                   // want_crc
      };

      SERVICE_CREATION_T hdcp2_tx_service_parameters2 = { HDCP2SERVICE_TX_NOTIFY_NAME,     // 4cc service code
                                                          connections[i],            // passed in fn ptrs
                                                          0,                         // tx fifo size (unused)
                                                          0,                         // tx fifo size (unused)
                                                          &hdcp2_service_callback,    // service callback (reused the one on Videocore side)
                                                          &hdcp2_tx_service_notify_available_semaphore,  // callback parameter
                                                          VC_FALSE,                  // want_unaligned_bulk_rx
                                                          VC_FALSE,                  // want_unaligned_bulk_tx
                                                          VC_FALSE,                  // want_crc
      };

      //Create the client to normal TV service 
      success = vchi_service_open( initialise_instance, &hdcp2_tx_service_parameters, &hdcp2_tx_service_client.client_handle[i] );
      vcos_assert( success == 0 );

       vcos_log_trace("HDCP2 Tx host side service opened");

      //Create the client to the async TV service (any TV related notifications)
      success = vchi_service_open( initialise_instance, &hdcp2_tx_service_parameters2, &hdcp2_tx_service_client.notify_handle[i] );
      vcos_assert( success == 0 );

       vcos_log_trace("HDCP2 Tx host side async service opened");

   }
 
   stack = vcos_malloc(stack_size, "HDCP Tx ARM service");
 
   if(vcos_verify(stack)) {
      //Create the notifier task
      success = vcos_thread_create_classic(&hdcp2_tx_service_notify_task, "HDCP2 Tx ARM", hdcp2_tx_service_notify_func,
                                           &hdcp2_tx_service_client, stack, stack_size,/*prio*/10,/*slice*/1,VCOS_START);
      vcos_assert(success == 0);
   }
   
   hdcp2_tx_service_client.initialised = 1;
}

/**
 * <DFN>vc_vchi_hdcp2_tx_stop</DNF> stops the Tx host side HDCP2 service
 *
 * @param none
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_vchi_hdcp2_tx_stop( void ) {
   uint32_t i;
   lock_obtain();
   //TODO: there is no API to stop the notifier task at the moment
   for (i=0; i < hdcp2_tx_service_client.num_connections; i++) {
      int32_t result;
      result = vchi_service_close(hdcp2_tx_service_client.client_handle[i]);
      vcos_assert( result == 0 );
      result = vchi_service_close(hdcp2_tx_service_client.notify_handle[i]);
      vcos_assert( result == 0 );
   }
   hdcp2_tx_service_client.initialised = 0;
   lock_release();
}

/**
 * Host applications must call <DFN>vc_hdcp2_tx_register_callback</DNF> at 
 * the beginning to register a function to handle all outgoing messages
 * from HDCP2 Tx state machine. Note that notifications from 
 * state machine also come in the form of outgoing message, so you have
 * to look at the message type.
 *
 * @param callback function
 *
 * @param callback_data is the context to be passed when function is called
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_hdcp2_tx_register_callback(HDCP2SERVICE_CALLBACK_T callback, void *callback_data) {
   lock_obtain();
   hdcp2_tx_service_client.notify_fn = callback;
   hdcp2_tx_service_client.notify_data = callback_data;
   lock_release();
}

/*********************************************************************************
 *
 *  Static functions definitions
 *
 *********************************************************************************/
//TODO: Might need to handle multiple connections later

/***********************************************************
 * Name: hdcp2_tx_service_notify_func
 *
 * Arguments: HDCP2 Tx service state
 *
 * Description: This is the notification task which receives all 
 *              messages coming from Videocore
 *
 * Returns: does not return
 *
 ***********************************************************/
static void *hdcp2_tx_service_notify_func( void *argv ) {
   int32_t success;
   VCOS_STATUS_T status;
   HDCP2_SERVICE_HOST_STATE_T *state = (HDCP2_SERVICE_HOST_STATE_T *) argv;
   void *message_data;
   uint32_t message_size;

   //Let Videocore side asynchronous service know we have connected by sending a message
   success = vc_hdcp2_service_send_command(state->client_handle[0], 
                                           &state->sema,
                                           VC_HDCP2_CMD_END_OF_LIST, NULL, 0);
   vcos_assert(success == 0);

   while(1) {
      status = vcos_semaphore_wait(&hdcp2_tx_service_notify_available_semaphore);
      vcos_assert(status == VCOS_SUCCESS && state->initialised);

      do {
         uint32_t reason;
         //Get all notifications in the queue, smallest message coming from Videocore Tx is
         //VC_HDCP2_NOTIFY_PARAM_T which has 4 words including the command
         success = vchi_msg_peek( state->notify_handle[0], &message_data, &message_size, VCHI_FLAGS_NONE );
         if(success != 0 || message_size < sizeof(uint32_t)*2 ) {
            continue;
         }

         reason = VC_VTOH32(*(uint32_t *)message_data);
         //Only two message types are expected here:
         //Notifications and outgoing messages to Rx (and loopback messages during loopback test)
         //We ignore the rest of the messages
         //Note that it is up to the host to translate authentication failure into ABORT messages
         //and send to Rx
         if((reason == VC_HDCP2_CMD_MSG || reason ==  VC_HDCP2_CMD_NOTIFY
             #ifdef ENABLE_HDCP2_LOOPBACK_TEST
             || reason == VC_HDCP2_CMD_LOOPBACK
             #endif
             )) {
            if(state->notify_fn) {
               (*state->notify_fn)(state->notify_data, message_data, message_size);
            } else {
               vcos_log_trace("HDCP2 Tx service: no callback for message %d coming from Videocore", reason);
            }
         } else {
            vcos_log_trace("HDCP2 Tx service: unexpected message %d coming from Videocore", reason);
         }
         vchi_msg_remove( state->notify_handle[0] );
      } while(success == 0 && message_size >= sizeof(uint32_t)*4);
   } //while(1)
   return NULL;
}

/***********************************************************
 Actual HDCP2 service API starts here
***********************************************************/
/**
 * Use <DFN>vc_hdcp2_tx_send_message</DFN> to send message [from Rx] to
 * HDCP2 Tx state machine. You should only use this to send genuine 
 * HDCP2 message.
 *
 * @param buffer is the message buffer
 *
 * @param buffer_size is the size
 *
 * @return zero if message is successfully transmitted, non-zero otherwise
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_send_message(const void *buffer, uint32_t buffer_size) {
   int success = -1;
   static VC_HDCP2_MSG_PARAM_T param;
   if(vcos_verify(buffer && buffer_size)) {
      param.msg_size = buffer_size;
      memcpy(&param.msg, buffer, buffer_size); //param.msg is a variable size message
      success = vc_hdcp2_service_send_command(hdcp2_tx_service_client.client_handle[0], 
                                              &hdcp2_tx_service_client.sema,
                                              VC_HDCP2_CMD_MSG, &param, buffer_size + sizeof(param.msg_size)); //buffer size does not include msg_size member
   }
   return success;
}

/**
 * Use <DFN>vc_hdcp2_tx_set_hpd</DFN> to set the hotplug status on Tx side.
 * HDCP2 can only state if hotplug is asserted. This function is only provided
 * to the host on Tx. Rx is assumed to generate this message on Videocore
 * and propagate the message to Tx host side.
 *
 * @param hotplug status (non-zero means attached, zero means detached)
 * 
 * @return zero if command is successfully sent, non-zero otherwise
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_set_hpd(VC_HDCP2_HP_STATUS_T hpd_status) {
   VC_HDCP2_HPD_PARAM_T param = {(uint32_t) hpd_status};
   return vc_hdcp2_service_send_command(hdcp2_tx_service_client.client_handle[0], 
                                        &hdcp2_tx_service_client.sema,
                                        VC_HDCP2_CMD_HPD, &param, sizeof(VC_HDCP2_HPD_PARAM_T)); 
}

/**
 * Use <DFN>vc_hdcp2_tx_begin_authentication</DFN> to start authentication.
 * If an authentication has already started, this will restart it.
 * If the link is already authenticated, this will reset the authentication
 * and start a new one. A notify callback will happen to indicate the 
 * authentication result.
 *
 * @param none
 *
 * @return zero if authentication successfully start, non-zero otherwise
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_begin_authentication(void) {
   int response = -1;
   uint32_t response_size;
   int success = vc_hdcp2_service_send_command_reply(hdcp2_tx_service_client.client_handle[0], 
                                                     &hdcp2_tx_service_client.sema,
                                                     &hdcp2_tx_service_message_available_semaphore,
                                                     VC_HDCP2_CMD_START, NULL, 0, 
                                                     &response, sizeof(response), &response_size);
   return (success == 0)? response : -1;
}

/**
 * <DFN>vc_hdcp2_tx_is_authenticated</DFN> returns whether there is 
 * currently an authenticated link or not.
 *
 * @param pointer to auth [in], set to 1 if the link is authenticated, zero otherwise
 *
 * @return zero if the command is sent successfully, non-zero if command failed to transmit
 *         and auth will not be changed in that case
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_is_authenticated(uint32_t *auth) { 
   int response = -1;
   uint32_t response_size;
   int success = vc_hdcp2_service_send_command_reply(hdcp2_tx_service_client.client_handle[0], 
                                                     &hdcp2_tx_service_client.sema,
                                                     &hdcp2_tx_service_message_available_semaphore,
                                                     VC_HDCP2_CMD_GET_AUTH_STATE, NULL, 0, 
                                                     &response, sizeof(response), &response_size);
   vcos_assert(auth);
   if(success) {
      *auth = 0; //failed to send command
   } else {
      *auth = response;
   }
   return success;
}

/**
 * <DFN>vc_hdcp2_tx_reset_ctr</DFN> resets the counter of a particular stream.
 * stream_id is the stream whose inputCtr you want to change, NOT streamCtr. This
 * function is only for debug. 
 *
 * @param stream_id is the stream id (little endian)
 *
 * @param inputCtr is the counter value (64-bit big endian)
 *
 * @return zero if command is sent successfully
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_reset_ctr(uint32_t stream_id, uint64_t inputCtr) {
   VC_HDCP2_RESET_CTR_PARAM_T param = {stream_id, inputCtr};
   int response = -1;
   uint32_t response_size;
   int success = vc_hdcp2_service_send_command_reply(hdcp2_tx_service_client.client_handle[0], 
                                                     &hdcp2_tx_service_client.sema,
                                                     &hdcp2_tx_service_message_available_semaphore,
                                                     VC_HDCP2_CMD_RESET_CTR, &param, sizeof(VC_HDCP2_RESET_CTR_PARAM_T), 
                                                     &response, sizeof(response), &response_size);
   return success;
}

/**
 * <DFN>vc_hdcp2_tx_get_ctr</DFN> returns the current counter of a particular stream.
 * stream_id is converted into big endian streamCtr internally. This
 * function is only for debug.
 *
 * @param stream_id is the stream id [in]
 *
 * @param pointer to inputCtr is the counter value (64-bit big endian) [out]
 *
 * @return zero if command is sent successfully, if failed, inputCtr is not modified
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_get_ctr(uint32_t stream_id, uint64_t *inputCtr) {
   VC_HDCP2_GET_CTR_PARAM_T param = {stream_id};
   uint64_t response;
   uint32_t response_size;
   int success = vc_hdcp2_service_send_command_reply(hdcp2_tx_service_client.client_handle[0], 
                                                     &hdcp2_tx_service_client.sema,
                                                     &hdcp2_tx_service_message_available_semaphore,
                                                     VC_HDCP2_CMD_RESET_CTR, &param, sizeof(VC_HDCP2_GET_CTR_PARAM_T),
                                                     &response, sizeof(response), &response_size);
   if(success == 0 && vcos_verify(inputCtr)) {
      *inputCtr = response;
   }
   return success;
}

/**
 * <DFN>vc_hdcp2_tx_shutdown</DFN> explicitly shutdown an authenticated link.
 * By default an authenticated link will stay authenticated until Tx side
 * is notified the Rx is no longer there. You will get a notification callback
 * with no error
 *
 * @param none
 *
 * @return zero if command is sent successfully,
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_shutdown(void) {
   return vc_hdcp2_service_send_command(hdcp2_tx_service_client.client_handle[0], 
                                        &hdcp2_tx_service_client.sema,
                                        VC_HDCP2_CMD_SHUTDOWN, NULL, 0);
}

/**
 * <DFN>vc_hdcp2_tx_expunge_id</DFN> deletes a receiver id from Tx pairing store.
 *
 * @param receiver id
 *
 * @return zero if id is deleted successfully
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_tx_expunge_id(const uint8_t *id) {
   VC_HDCP2_EXPUNGE_PARAM_T param;
   int response, success;
   uint32_t response_size;
   memcpy(&param.id[0], id, HDCP2_ID_SIZE);
   success = vc_hdcp2_service_send_command_reply(hdcp2_tx_service_client.client_handle[0], 
                                                 &hdcp2_tx_service_client.sema,
                                                 &hdcp2_tx_service_message_available_semaphore,
                                                 VC_HDCP2_CMD_EXPUNGE, &param, sizeof(VC_HDCP2_EXPUNGE_PARAM_T),
                                                 &response, sizeof(response), &response_size);
   return (success == 0)? response : success;
}
