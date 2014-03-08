/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: $
Revision :  $Revision: #5 $

FILE DESCRIPTION:

=============================================================================*/

#ifndef BULK_AUX_SERVICE_H_
#define BULK_AUX_SERVICE_H_

#include "interface/vchi/os/types.h"

/******************************************************************************
 Global defs
 *****************************************************************************/

#define BULX_SERVICE_OFFSET     0 // service ID
#define BULX_SIZE_OFFSET        4 // size of entire transfer
#define BULX_CHUNKSIZE_OFFSET   8 // needed for some interfaces
#define BULX_CRC_OFFSET        12 // CRC of entire transfer (after reassembly)
#define BULX_DATA_SIZE_OFFSET  16 // amount of data actually transferred over data channel
#define BULX_DATA_SHIFT_OFFSET 20 // how many bytes receiver should shift that data by (signed)
#define BULX_CHANNEL_OFFSET    22 // which channel data is being sent on
#define BULX_RESERVED_OFFSET   23 //  reserved byte
#define BULX_HEAD_SIZE_OFFSET  24 // how many bytes of head data [0..HEAD_SIZE) enclosed
#define BULX_TAIL_SIZE_OFFSET  26 // how many bytes of tail data [SIZE-TAIL_SIZE..SIZE) enclosed
#define BULX_HEADER_SIZE       28


/******************************************************************************
 Global functions
 *****************************************************************************/

// Create the bulk aux services for our connection
int32_t vchi_bulk_aux_service_init( VCHI_CONNECTION_T **connections,
                                    const uint32_t num_connections,
                                    void **state );

// Close the connections we have previously init'ed
int32_t vchi_bulk_aux_service_close( void );

// Fill in a header structure, returning the length
size_t vchi_bulk_aux_service_form_header( void *dest,
                                          size_t dest_len,
                                          fourcc_t service_id,
                                          MESSAGE_TX_CHANNEL_T channel,
                                          uint32_t total_size,
                                          uint32_t chunk_size,
                                          uint32_t crc,
                                          uint32_t data_size,
                                          int16_t data_shift,
                                          uint16_t head_bytes,
                                          uint16_t tail_bytes );
                                         
#endif /* BULK_AUX_SERVICE_H_ */

/****************************** End of file **********************************/
