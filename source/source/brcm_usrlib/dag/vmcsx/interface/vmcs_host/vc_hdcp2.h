/*=========================================================
 Copyright © 2010 Broadcom Europe Limited. All rights reserved.

 Project  :  HDCP2
 Module   :  HDCP2 common constants for both host and vc
 File     :  $File: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_hdcp2.h $
 Revision :  $Revision: #2 $

 FILE DESCRIPTION:
 HDCP2 constants common to both host and VC
 =========================================================*/
#ifndef _VC_HDCP2_H_
#define _VC_HDCP2_H_
#include "vcinclude/common.h"

/**
 *\file This file contains constants shared by both host side and Videocore side HDCP2
 */

/**
 * HDCP2 notifications, only 1 so far, note that hotplug is sent as a separate message
 * Reason           remark                           param1    param2
 * VC_HDCP2_AUTH    authentication state             success?  reason if failed  (success = 1 if authenticated)
 */
typedef enum {
   VC_HDCP2_AUTH = 0,
} VC_HDCP2_NOTIFY_T;

/**
 * HDCP2 hotplug state
 */
typedef enum {
   VC_HDCP2_HP_DETACHED = 0,
   VC_HDCP2_HP_ATTACHED = 1
} VC_HDCP2_HP_STATUS_T;


/**
 * HDCP2 errors
 */
typedef enum {
   HDCP2_ERROR_NONE = 0,            /**< Process completed successfully. */
   HDCP2_ERROR_INVALID_STREAM,      /**< Invalid stream (input counters) */
   HDCP2_ERROR_MISALIGNED,          /**< Misaligned buffer */
   HDCP2_ERROR_NOT_READY,           /**< Not ready for encryption */
   HDCP2_ERROR_CALLBACKS_EXCEEDED,  /**< Too many callbacks exceeded */
   HDCP2_ERROR_OUT_OF_MEMORY,       /**< Out of memory */
   HDCP2_ERROR_MAX_DEVICES_EXCEEDED,/**< Too many devices */
   HDCP2_ERROR_MAX_DEPTH_EXCEEDED,  /**< Too many levels */
   HDCP2_ERROR_INVALID_CERT,        /**< Invalid certificate */
   HDCP2_ERROR_RSA_ERROR,           /**< Error in RSA library */
   HDCP2_ERROR_RX_DETACHED,         /**< Rx detached */
   HDCP2_ERROR_H_MISMATCH,          /**< H mismatch */
   HDCP2_ERROR_L_MISMATCH,          /**< Locality check failed */
   HDCP2_ERROR_V_MISMATCH,          /**< receiver id list verify failed */
   HDCP2_ERROR_SRM_CORRUPTED,       /**< SRM integrity failed */
   HDCP2_ERROR_ID_REVOKED,          /**< receiver id revoked */
   HDCP2_ERROR_SEND_ERROR,          /**< Failed to send message */
   HDCP2_ERROR_RECEIVE_ERROR,       /**< Failed to receive message */
   HDCP2_ERROR_RNG_ERROR,           /**< rng failed */
   HDCP2_ERROR_TIMER,               /**< timer error */
   HDCP2_ERROR_TIMEOUT,             /**< Time out */
   HDCP2_ERROR_L_TIMEOUT,           /**< L timeout */
   HDCP2_ERROR_MAX                  /**< [Never returned - placeholder.] */
} HDCP2_ERROR_T;

#endif
