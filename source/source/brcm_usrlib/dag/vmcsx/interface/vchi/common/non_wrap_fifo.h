/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  Inter-processor communication
Module   :  non wrap FIFO
File     :  $RCSfile:  $
Revision :  $Revision: $

FILE DESCRIPTION
=============================================================================*/

#ifndef _VCHI_NON_WRAP_FIFO_H_
#define _VCHI_NON_WRAP_FIFO_H_


typedef struct opaque_vchi_mqueue_t VCHI_NWFIFO_T;


VCHI_NWFIFO_T *non_wrap_fifo_create( const char *name, const uint32_t size, int alignment, int no_of_slots );
int32_t non_wrap_fifo_destroy( VCHI_NWFIFO_T *fifo );
int32_t non_wrap_fifo_read( VCHI_NWFIFO_T *fifo, void *data, const uint32_t max_data_size );
int32_t non_wrap_fifo_write( VCHI_NWFIFO_T *fifo, const void *data, const uint32_t data_size );
int32_t non_wrap_fifo_request_write_address( VCHI_NWFIFO_T *fifo, void **address, uint32_t size, int block );
int32_t non_wrap_fifo_write_complete( VCHI_NWFIFO_T *fifo, void *address );
int32_t non_wrap_fifo_request_read_address( VCHI_NWFIFO_T *fifo, const void **address, uint32_t *length );
int32_t non_wrap_fifo_read_complete( VCHI_NWFIFO_T *fifo, void *address );
int32_t non_wrap_fifo_remove( VCHI_NWFIFO_T *fifo, void *address );
void    non_wrap_fifo_debug( VCHI_NWFIFO_T *fifo );


#endif // _VCHI_NON_WRAP_FIFO_H_

/********************************** End of file ******************************************/
