/*=============================================================================
Copyright (c) 2011 Broadcom Europe Limited.
All rights reserved.

Project  :  VMCS
Module   :  HDCP2 Service host side
File     :  $File: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_hdcp2_service_common.c $
Revision :  $Revision: #1 $

FILE DESCRIPTION
HDCP2 service host side helpers implementation
Dependency: VCHI
=============================================================================*/
#include "vc_hdcp2_service_common.h"

VCOS_LOG_CAT_T hdcp2_log_category;

/**
 * <DFN>vc_hdcp2_service_wait_for_reply</DFN> blocks until a response comes back
 * from Videocore.
 *
 * @param client_handle is the vchi client handle
 * 
 * @param sema is the signalling semaphore indicating a reply
 *
 * @param response is the reponse buffer
 *
 * @param max_length is the maximum length of the buffer
 *
 * @param actual_length will be set to the actual length of the buffer (or zero if error)
 *
 * @return zero if successful, vchi error code otherwise
 */
int32_t vc_hdcp2_service_wait_for_reply(VCHI_SERVICE_HANDLE_T client_handle,
                                        VCOS_SEMAPHORE_T *sema,
                                        void *response, uint32_t max_length, uint32_t *actual_length) {
   int32_t success = 0;
   VCOS_STATUS_T status;
   uint32_t length_read = 0;
   vcos_assert(sema);
   vcos_assert(response && max_length);
   do {
      //TODO : we need to deal with messages coming through on more than one connections properly
      //At the moment it will always try to read the first connection if there is something there
      //Check if there is something in the queue, if so return immediately
      //otherwise wait for the semaphore and read again
      success = vchi_msg_dequeue( client_handle, response, max_length, &length_read, VCHI_FLAGS_NONE );
   } while( length_read == 0 || (status = vcos_semaphore_trywait(sema)) != VCOS_SUCCESS);

   if(vcos_verify(*actual_length)) {
      *actual_length = (success == 0)? length_read : 0;
   }
   return success;
}

/**
 * <DFN>vc_hdcp2_service_send_command</DFN> sends a command which has no reply to Videocore
 * side HDCP2 service.
 *
 * @param client_handle is the vchi client handle
 *
 * @param sema is the locking semaphore to protect the buffer
 * 
 * @param command is the command (VC_HDCP2_CMD_CODE_T in vc_hdcp2service_defs.h)
 *
 * @param buffer is the command buffer to be sent
 *
 * @param length is the size of buffer in bytes
 *
 * @return zero if successful, VCHI error code if failed
 */
int32_t vc_hdcp2_service_send_command(VCHI_SERVICE_HANDLE_T client_handle,
                                      VCOS_SEMAPHORE_T *sema,
                                      uint32_t command, void *buffer, uint32_t length) {
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                  {buffer, length} };
   VCOS_STATUS_T status;
   int32_t success = 0;
   vcos_assert(sema);
   status = vcos_semaphore_wait(sema);
   vcos_assert(status == VCOS_SUCCESS);
   success = vchi_msg_queuev( client_handle, vector, sizeof(vector)/sizeof(vector[0]),
                              VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
   status = vcos_semaphore_post(sema);
   vcos_assert(status == VCOS_SUCCESS);
   return success;
}

/**
 * <DFN>vc_hdcp2_service_send_command_reply</DFN> sends a command and waits for a reply from
 * Videocore side HDCP2 service.
 *
 * @param client_handle is the vchi client handle
 *
 * @param lock_sema is the locking semaphore to protect the buffer
 *
 * @param reply_sema is the signalling semaphore for the reply
 *
 * @param command is the command (VC_HDCP2_CMD_CODE_T in vc_hdcp2service_defs.h)
 *
 * @param buffer is the command buffer to be sent
 *
 * @param length is the size of buffer in bytes
 *
 * @param response is the reponse buffer
 *
 * @param response_length is the maximum length of the response buffer
 *
 * @param actual_length is set to the actual length of the message
 *
 * @return zero if successful, VCHI error code if failed
 */
int32_t vc_hdcp2_service_send_command_reply(VCHI_SERVICE_HANDLE_T client_handle,
                                            VCOS_SEMAPHORE_T *lock_sema,
                                            VCOS_SEMAPHORE_T *reply_sema,
                                            uint32_t command, 
                                            void *buffer, uint32_t length,
                                            void *response, uint32_t response_length,
                                            uint32_t *actual_length) {

   int32_t success = vc_hdcp2_service_send_command(client_handle,
                                                   lock_sema,
                                                   command, buffer, length);

   if(vcos_verify(!success)) {
      VCOS_STATUS_T status = vcos_semaphore_wait(lock_sema);
      vcos_assert(status == VCOS_SUCCESS);
      success = vc_hdcp2_service_wait_for_reply(client_handle,
                                                reply_sema,
                                                response, response_length, actual_length);
      status = vcos_semaphore_post(lock_sema);
      vcos_assert(status == VCOS_SUCCESS);
   }
   return success;
}
                                                   
