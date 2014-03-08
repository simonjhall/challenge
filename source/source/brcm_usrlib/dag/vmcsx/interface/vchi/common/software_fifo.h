/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  Inter-processor communication
Module   :  FIFO
File     :  $RCSfile:  $
Revision :  $Revision: $

FILE DESCRIPTION
=============================================================================*/

#ifndef SOFTWARE_FIFO_H_
#define SOFTWARE_FIFO_H_

#include "interface/vchi/os/os.h"

typedef struct {
   uint8_t        *malloc_address;
   uint8_t        *base_address;
   uint8_t        *read_ptr;
   uint8_t        *write_ptr;
   uint32_t       size;
   uint32_t       bytes_read;
   uint32_t       bytes_written;
   // semaphore to protect this structure
   OS_SEMAPHORE_T software_fifo_semaphore;
} SOFTWARE_FIFO_HANDLE_T;


int32_t software_fifo_create( const uint32_t size, int alignment, SOFTWARE_FIFO_HANDLE_T *handle );
int32_t software_fifo_destroy( SOFTWARE_FIFO_HANDLE_T *handle );

int32_t software_fifo_read( SOFTWARE_FIFO_HANDLE_T *handle, void *data, const uint32_t data_size );
int32_t software_fifo_write( SOFTWARE_FIFO_HANDLE_T *handle, const void *data, const uint32_t data_size );

int32_t software_fifo_data_available( SOFTWARE_FIFO_HANDLE_T *handle );
int32_t software_fifo_room_available( SOFTWARE_FIFO_HANDLE_T *handle );

int32_t software_fifo_peek( SOFTWARE_FIFO_HANDLE_T *handle, void *data, const uint32_t data_size );
int32_t software_fifo_remove( SOFTWARE_FIFO_HANDLE_T *handle, const uint32_t data_size );

int32_t software_fifo_protect( SOFTWARE_FIFO_HANDLE_T *handle );
int32_t software_fifo_unprotect( SOFTWARE_FIFO_HANDLE_T *handle );

#endif // SOFTWARE_FIFO_H_
