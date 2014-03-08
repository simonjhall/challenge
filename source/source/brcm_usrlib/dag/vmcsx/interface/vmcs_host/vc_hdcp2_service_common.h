/*=============================================================================
Copyright (c) 2011 Broadcom Europe Limited.
All rights reserved.

Project  :  VMCS
Module   :  HDCP2 Service host side
File     :  $File: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_hdcp2_service_common.h $
Revision :  $Revision: #1 $

FILE DESCRIPTION
HDCP2 service host side helpers
Dependency: VCHI
=============================================================================*/
#ifndef _VC_HDCP2_SERVICE_COMMON_H_
#define _VC_HDCP2_SERVICE_COMMON_H_
#include "applications/vmcs/hdcp2service/hdcp2_server_common.h"

/**
 * This is the host service state, which is the same for both Tx and Rx.
 * Only the Videocore side service state differs between Tx and Rx.
 */
typedef struct {
   //Generic service stuff
   VCHI_SERVICE_HANDLE_T client_handle[VCHI_MAX_NUM_CONNECTIONS]; //To connect to server on VC
   VCHI_SERVICE_HANDLE_T notify_handle[VCHI_MAX_NUM_CONNECTIONS]; //For incoming notifications
   
   uint32_t              num_connections;

   VCOS_SEMAPHORE_T      sema;
   HDCP2SERVICE_CALLBACK_T notify_fn;
   void                 *notify_data;
   int                   initialised;

} HDCP2_SERVICE_HOST_STATE_T;

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
 * @param actual_length will be set to the actual length of the buffer
 *
 * @return zero if successful, VCHI error code if failed
 */
extern int32_t vc_hdcp2_service_wait_for_reply(VCHI_SERVICE_HANDLE_T client_handle,
                                               VCOS_SEMAPHORE_T *sema,
                                               void *response, uint32_t max_length, uint32_t *actual_length);

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
extern int32_t vc_hdcp2_service_send_command(VCHI_SERVICE_HANDLE_T client_handle,
                                             VCOS_SEMAPHORE_T *sema,
                                             uint32_t command, void *buffer, uint32_t length);

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
extern int32_t vc_hdcp2_service_send_command_reply(VCHI_SERVICE_HANDLE_T client_handle,
                                                   VCOS_SEMAPHORE_T *lock_sema,
                                                   VCOS_SEMAPHORE_T *reply_sema,
                                                   uint32_t command, 
                                                   void *buffer, uint32_t length,
                                                   void *response, uint32_t response_length,
                                                   uint32_t *actual_length);

#define VCOS_LOG_CATEGORY (&hdcp2_log_category)
extern VCOS_LOG_CAT_T hdcp2_log_category;

#endif

