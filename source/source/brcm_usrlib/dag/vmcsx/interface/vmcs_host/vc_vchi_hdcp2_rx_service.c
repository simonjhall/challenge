/*=============================================================================
Copyright (c) 2011 Broadcom Europe Limited.
All rights reserved.

Project  :  HDCP2 Service
Module   :  Common host interface
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_hdcp2_rx_service.c#1 $
Revision :  $Revision: #1 $

FILE DESCRIPTION
HDCP2 host side service API (Rx) implementation
See vc_hdcp2.h typedefs

=============================================================================*/
#include <string.h>
#include "vchost_config.h"
#include "vchost.h"

#include "interface/vcos/vcos_logging.h"
#include "vc_hdcp2_service_common.h"
#include "vc_hdcp2_rx_service.h"

#include "vcfw/logging/logging.h"


/******************************************************************************
Static data.
******************************************************************************/
static HDCP2_SERVICE_HOST_STATE_T hdcp2_rx_service_client = {0};
static VCOS_SEMAPHORE_T hdcp2_rx_service_message_available_semaphore;
static VCOS_SEMAPHORE_T hdcp2_rx_service_notify_available_semaphore;
static VCOS_THREAD_T hdcp2_rx_service_notify_task;

/******************************************************************************
Static functions.
******************************************************************************/
//Lock the host state
static void lock_obtain (void) {
   VCOS_STATUS_T status;
   vcos_assert(hdcp2_rx_service_client.initialised);
   status = vcos_semaphore_wait( &hdcp2_rx_service_client.sema );
   vcos_assert(status == VCOS_SUCCESS);
}

//Unlock the host state
static void lock_release (void) {
   VCOS_STATUS_T status;
   vcos_assert(hdcp2_rx_service_client.initialised);
   status = vcos_semaphore_post( &hdcp2_rx_service_client.sema );
   vcos_assert( status == VCOS_SUCCESS );
}

//Task handling incoming messages from Videocore
static void *hdcp2_rx_service_notify_func( void *argv );

/******************************************************************************
HDCP2 service API
******************************************************************************/

/**
 * <DFN>vc_vchi_hdcp2_rx_init</DFN> is called at the beginning of the application
 * to initialise the client to Rx side HDCP2 service
 * 
 * @param initialise_instance is the VCHI instance
 *
 * @param connections is the array of pointers of connections
 * 
 * @param num_connections is the length of the array
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_vchi_hdcp2_rx_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int32_t success;
   uint32_t i;
   VCOS_STATUS_T status;
   const uint32_t stack_size = 4096;
   void *stack;

   vcos_assert(!hdcp2_rx_service_client.initialised);
   vcos_log_register("VCOS_LOG_TRACE", &hdcp2_log_category);

   // record the number of connections
   memset( &hdcp2_rx_service_client, 0, sizeof(HDCP2_SERVICE_HOST_STATE_T) );
   hdcp2_rx_service_client.num_connections = num_connections;
   //Semaphore controlling access to HDCP2 Rx service
   status = vcos_semaphore_create( &hdcp2_rx_service_client.sema, "HDCP2 Rx client sema", 1);
   vcos_assert(status == VCOS_SUCCESS);

   //These semaphores are initially locked, to be signalled by incoming replies/asychronous messages
   status = vcos_semaphore_create( &hdcp2_rx_service_message_available_semaphore, "HDCP2 Rx client reply", 0);
   vcos_assert(status == VCOS_SUCCESS);
   status = vcos_semaphore_create( &hdcp2_rx_service_notify_available_semaphore, "HDCP2 Rx client notify", 0);
   vcos_assert(status == VCOS_SUCCESS);
  
   for (i=0; i < hdcp2_rx_service_client.num_connections; i++) {

      // Create a 'Client' service on the each of the connections
      SERVICE_CREATION_T hdcp2_rx_service_parameters = { HDCP2SERVICE_RX_CLIENT_NAME,      // 4cc service code
                                                         connections[i],             // passed in fn ptrs
                                                         0,                          // tx fifo size (unused)
                                                         0,                          // tx fifo size (unused)
                                                         &hdcp2_service_callback,    // service callback (reused the one on Videocore side)
                                                         &hdcp2_rx_service_message_available_semaphore,  // callback parameter
                                                         VC_FALSE,                   // want_unaligned_bulk_rx
                                                         VC_FALSE,                   // want_unaligned_bulk_tx
                                                         VC_FALSE,                   // want_crc
      };

      SERVICE_CREATION_T hdcp2_rx_service_parameters2 = { HDCP2SERVICE_RX_NOTIFY_NAME,     // 4cc service code
                                                          connections[i],            // passed in fn ptrs
                                                          0,                         // tx fifo size (unused)
                                                          0,                         // tx fifo size (unused)
                                                          &hdcp2_service_callback,    // service callback (reused the one on Videocore side)
                                                          &hdcp2_rx_service_notify_available_semaphore,  // callback parameter
                                                          VC_FALSE,                  // want_unaligned_bulk_rx
                                                          VC_FALSE,                  // want_unaligned_bulk_tx
                                                          VC_FALSE,                  // want_crc
      };

      //Create the client to normal TV service 
      success = vchi_service_open( initialise_instance, &hdcp2_rx_service_parameters, &hdcp2_rx_service_client.client_handle[i] );
      vcos_assert( success == 0 );

      vcos_log_trace("HDCP2 Rx host side service opened");

      //Create the client to the async TV service (any TV related notifications)
      success = vchi_service_open( initialise_instance, &hdcp2_rx_service_parameters2, &hdcp2_rx_service_client.notify_handle[i] );
      vcos_assert( success == 0 );

       vcos_log_trace("HDCP2 Rx host side service opened");

   }
 
   stack = vcos_malloc(stack_size, "HDCP Rx ARM service");
 
   if(vcos_verify(stack)) {
      //Create the notifier task
#ifdef HDCP2_RX_USE_CPU1
      success = vcos_thread_create_classic(&hdcp2_rx_service_notify_task, "HDCP2 Rx ARM", hdcp2_rx_service_notify_func,
                                           &hdcp2_rx_service_client, stack, stack_size, VCOS_THREAD_PRI_NORMAL | VCOS_AFFINITY_CPU1, 1,VCOS_START);
#else
      success = vcos_thread_create_classic(&hdcp2_rx_service_notify_task, "HDCP2 Rx ARM", hdcp2_rx_service_notify_func,
                                           &hdcp2_rx_service_client, stack, stack_size, VCOS_THREAD_PRI_NORMAL | VCOS_AFFINITY_CPU0, 1,VCOS_START);
#endif  
      vcos_assert(success == 0);
   }
   
   hdcp2_rx_service_client.initialised = 1;
}
/**
 * <DFN>vc_vchi_hdcp2_rx_stop</DNF> stops the Tx host side HDCP2 service
 *
 * @param none
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_vchi_hdcp2_rx_stop( void ) {
   uint32_t i;
   lock_obtain();
   //TODO: there is no API to stop the notifier task at the moment
   for (i=0; i < hdcp2_rx_service_client.num_connections; i++) {
      int32_t result;
      result = vchi_service_close(hdcp2_rx_service_client.client_handle[i]);
      vcos_assert( result == 0 );
      result = vchi_service_close(hdcp2_rx_service_client.notify_handle[i]);
      vcos_assert( result == 0 );
   }
   hdcp2_rx_service_client.initialised = 0;
   lock_release();
}

/**
 * Host applications must call <DFN>vc_hdcp2_rx_register_callback</DNF> at 
 * the beginning to register a function to handle all outgoing messages
 * from HDCP2 Rx state machine. Note that notifications from 
 * state machine also come in the form of outgoing message, so you have
 * to look at the message type.
 *
 * @param callback function
 *
 * @param callback_data is the context to be passed when function is called
 *
 * @return void
 */
VCHPRE_ void VCHPOST_ vc_hdcp2_rx_register_callback(HDCP2SERVICE_CALLBACK_T callback, void *callback_data) {
   lock_obtain();
   hdcp2_rx_service_client.notify_fn = callback;
   hdcp2_rx_service_client.notify_data = callback_data;
   lock_release();
}

/*********************************************************************************
 *
 *  Static functions definitions
 *
 *********************************************************************************/
//TODO: Might need to handle multiple connections later

/***********************************************************
 * Name: hdcp2_rx_service_notify_func
 *
 * Arguments: HDCP2 Rx service state
 *
 * Description: This is the notification task which receives all 
 *              messages coming from Videocore
 *
 * Returns: does not return
 *
 ***********************************************************/
static void *hdcp2_rx_service_notify_func( void *argv ) {
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
      status = vcos_semaphore_wait(&hdcp2_rx_service_notify_available_semaphore);
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
         //Only three message types are expected here:
         //Notifications, outgoing messages and hotplug to Tx 
         //We ignore the rest of the messages
         if((reason == VC_HDCP2_CMD_MSG || reason ==  VC_HDCP2_CMD_NOTIFY || reason == VC_HDCP2_CMD_HPD)) {
            if(state->notify_fn) {
               (*state->notify_fn)(state->notify_data, message_data, message_size);
            } else {
               vcos_log_trace("HDCP2 Rx service: no callback for message %d coming from Videocore", reason);
            }
         } else {
            vcos_log_trace("HDCP2 Rx service: unexpected message %d coming from Videocore", reason);
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
 * Use <DFN>vc_hdcp2_rx_send_message</DFN> to send message [from Tx] to
 * HDCP2 Rx state machine. You should only use this to send genuine 
 * HDCP2 message. Use <DFN>vc_hdcp2_rx_send_loopback_message</DFN> to
 * send loopback test data.
 *
 * @param buffer is the message buffer
 *
 * @param buffer_size is the size
 *
 * @return zero if message is successfully transmitted, non-zero otherwise
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_rx_send_message(const void *buffer, uint32_t buffer_size) {
   int success = -1;
   static VC_HDCP2_MSG_PARAM_T param;
   if(vcos_verify(buffer && buffer_size)) {
      param.msg_size = buffer_size;
      memcpy(&param.msg, buffer, buffer_size); //param.msg is a variable size message
      success = vc_hdcp2_service_send_command(hdcp2_rx_service_client.client_handle[0], 
                                              &hdcp2_rx_service_client.sema,
                                              VC_HDCP2_CMD_MSG, &param, buffer_size + sizeof(param.msg_size)); //buffer size does not include msg_size member
   }
   return success;
}

/**
 * <DFN>vc_hdcp2_rx_is_authenticated</DFN> returns whether there is 
 * currently an authenticated link or not. Note that the result here does
 * not necessarily agree with the result from <DFN>vc_hdcp2_tx_is_authenticated</DFN>
 * because Rx is passive in the context of authentication.
 *
 * @param pointer to auth [in], non-zero if the link is authenticated, zero otherwise
 *
 * @return zero if the command is sent successfully, auth is also set to zero if 
 *         command failed 
 *
 */
VCHPRE_ int VCHPOST_ vc_hdcp2_rx_is_authenticated(uint32_t *auth) {
   int response = -1;
   uint32_t response_size;
   int success = vc_hdcp2_service_send_command_reply(hdcp2_rx_service_client.client_handle[0], 
                                                     &hdcp2_rx_service_client.sema,
                                                     &hdcp2_rx_service_message_available_semaphore,
                                                     VC_HDCP2_CMD_GET_AUTH_STATE, NULL, 0, 
                                                     &response, sizeof(response), &response_size);
   vcos_assert(auth);
   if(success < 0) {
      *auth = 0; //failed to send command
   } else {
      *auth = response;
      success = 0;
   }
   return success;
}

/**
 * Use <DFN>vc_hdcp2_rx_send_loopback_message</DFN> to send loopback message 
 * [from Tx] to HDCP2 Rx service. This message does NOT travel to
 * HDCP2 Rx state machine. This function is only available if you are running loopback 
 * test.
 *
 * @param streamCtr is the stream counter
 *
 * @param inputCtr is the input counter
 * 
 * @param encrypted_buffer is the encrypted buffer
 *
 * @param buffer_size if the length of encrypted buffer
 *
 * @param plaintext_buffer is the expected data (same size as encrypted buffer)
 *
 * @param last_packet is non-zero if this is the last packet
 *
 * @return 1 if message is successfully transmitted and loopback message verified, 
 *         0 is message mismatched, -1 if message failed to send to Videocore
 */
#ifdef ENABLE_HDCP2_LOOPBACK_TEST
VCHPRE_ int VCHPOST_ vc_hdcp2_rx_send_loopback_message(uint32_t streamCtr, uint64_t inputCtr,
                                                       const uint8_t *encrypted_buffer,
                                                       uint32_t buffer_size,
                                                       const uint8_t *plaintext_buffer,
                                                       uint32_t last_packet) {
   int response;
   uint32_t response_size;
   static VC_HDCP2_LOOPBACK_PARAM_T param;
   param.buffer_size = buffer_size;
   param.streamCtr = streamCtr;
   param.inputCtr = inputCtr;
   memcpy(param.encrypted_buffer, encrypted_buffer, buffer_size);
   memcpy(param.plaintext_buffer, plaintext_buffer, buffer_size);
   param.last_packet = last_packet;

   if(vc_hdcp2_service_send_command_reply(hdcp2_rx_service_client.client_handle[0], 
                                          &hdcp2_rx_service_client.sema,
                                          &hdcp2_rx_service_message_available_semaphore,
                                          VC_HDCP2_CMD_LOOPBACK, &param,
                                          sizeof(VC_HDCP2_LOOPBACK_PARAM_T),
                                          &response, sizeof(response), &response_size) == 0) {
      return response;
   }
   return -1;
}
#endif
