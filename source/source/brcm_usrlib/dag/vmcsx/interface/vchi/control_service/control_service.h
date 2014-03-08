/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: $
Revision :  $Revision: #4 $

FILE DESCRIPTION:

=============================================================================*/

#ifndef CONTROL_SERVICE_H_
#define CONTROL_SERVICE_H_

#include "interface/vchi/os/types.h"
#include "interface/vchi/message_drivers/message.h"

/******************************************************************************
 Global defs
 *****************************************************************************/

// flags in SERVER_AVAILABLE and SERVER_AVAILABLE_REPLY messages
typedef enum {
   AVAILABLE=1,
   WANT_CRC=2,
   WANT_UNALIGNED_BULK_TX=4,
   WANT_UNALIGNED_BULK_RX=8,
   WANT_BULK_AUX_ALWAYS=16
} VCHI_SERVICE_FLAGS_T;

// Fields in CONNECT protocol field
#define PROTOCOL_VERSION               1
#define PROTOCOL_VERSION_MASK 0x000000FFu
#define PROTOCOL_REPLY_FLAG   0x80000000u


/******************************************************************************
 Global functions
 *****************************************************************************/

// Communicate with control services on the other end of the connection
int32_t vchi_control_service_init( VCHI_CONNECTION_T **connections,
                                   const uint32_t num_connections,
                                   void **state );

// Close the connections we have previously init'ed
int32_t vchi_control_service_close( void );

// Query whether or not a server is available at the other end of a connection
int32_t vchi_control_server_available( uint32_t service_id, VCHI_CONNECTION_T *connection );

// Routine to initialise the time functions
void vchi_initialise_time(void);

// Routine to get the current time for messages
uint32_t vchi_control_get_time(void);

// Routine to update the local time based on the time reported by the Host
void vchi_control_update_time(uint32_t time);

// For the following calls, handle is the callback parameter that CTRL used
// when opening itself. Yuck. Doesn't seem to be any other easy way of giving the
// necessary context when calling from connection layer to control service.

// Routine to queue a connect message (blocking)
int32_t vchi_control_queue_connect( void *handle, bool_t reply );

// Routine to queue an xon/xoff message (non-blocking)
int32_t vchi_control_queue_xon_xoff( void *handle, fourcc_t id, bool_t xon );

// Routine to queue a bulk transfer rx message (non-blocking)
int32_t vchi_control_queue_bulk_transfer_rx( void *handle,
                                             fourcc_t service_id,
                                             uint32_t total_len,
                                             MESSAGE_RX_CHANNEL_T channel,
                                             uint32_t channel_params,
                                             uint32_t data_len,
                                             uint32_t data_offset );

// Routine to queue a server available query message (blocking)
int32_t vchi_control_queue_server_available( void *handle,
                                             fourcc_t service_id,
                                             VCHI_SERVICE_FLAGS_T flags );

#endif /* CONTROL_SERVICE_H_ */

/****************************** End of file **********************************/
