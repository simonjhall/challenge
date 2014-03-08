/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  Inter-processor communication
Module   :  FIFO
File     :  $RCSfile:  $
Revision :  $Revision: $

FILE DESCRIPTION:

Software controlled FIFOs for inter processor communication.
=============================================================================*/
#include <stdlib.h>
#include <string.h>

#include "interface/vchi/os/os.h"
#include "software_fifo.h"


/***********************************************************
 * Name: software_fifo_create
 *
 * Arguments: const uint32_t size            - size of FIFO (in bytes)
 *            int alignment                  - required alignment (bytes)
 *            SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *
 * Description: Allocates space for a FIFO and initialises it
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_create( const uint32_t size, int alignment, SOFTWARE_FIFO_HANDLE_T *handle )
{
int32_t success = -1;

   // check that the alignment is a power of 2 or 0
   os_assert((OS_COUNT(alignment) == 1) || (alignment == 0));

   // put the alignment into a form we can use (must be power of 2)
   alignment = alignment ? alignment-1 : 0;

   // check we have a valid pointer
   if(handle)
   {
      memset(handle,0,sizeof(SOFTWARE_FIFO_HANDLE_T));
      // allocate slightly more space than we need so that we can get the required alignment
      handle->malloc_address = (uint8_t *)os_malloc(size+alignment, OS_ALIGN_DEFAULT, "");
      if(handle->malloc_address)
      {
         handle->base_address = (uint8_t *)(((uint32_t)handle->malloc_address + alignment) & ~alignment);
         handle->read_ptr = handle->base_address;
         handle->write_ptr = handle->base_address;
         handle->size = size;
         handle->bytes_read = 0;
         handle->bytes_written = 0;
         success = os_semaphore_create(&handle->software_fifo_semaphore,OS_SEMAPHORE_TYPE_SUSPEND);
         // oops, unable to create the semaphore
         os_assert(success == 0);
      }
   }

   // oops, looks like you've run out of memory
   os_assert(success == 0);
   return(success);
}


/***********************************************************
 * Name: software_fifo_destroy
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T handle - handle to this FIFO
 *
 * Description: De-allocates space for the FIFO
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_destroy( SOFTWARE_FIFO_HANDLE_T *handle )
{
int32_t success = -1;

   // first confirm that the handle has been previously allocated
   if(handle->malloc_address)
   {
      // free the FIFO memory
      free(handle->malloc_address);
      // reset all the variables
      memset(handle,0,sizeof(SOFTWARE_FIFO_HANDLE_T));
      // success!
      success = 0;
   }
   return(success);
}


/***********************************************************
 * Name: software_fifo_read
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *            uint8_t * data - where to read the data to
 *            uint32_t data_size - how much data to read (in bytes)
 *
 * Description: Reads the required amount of data from the FIFO
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_read( SOFTWARE_FIFO_HANDLE_T *handle, void *data, const uint32_t data_size )
{
uint32_t bytes_left;

   os_assert(handle->base_address);
   // check that there is enough data in the FIFO to fulfill this request
   if( software_fifo_data_available(handle) < data_size )
      return(-1);
   bytes_left = (handle->base_address+handle->size) - handle->read_ptr;
   if(bytes_left >= data_size)
   {
      memcpy(data,handle->read_ptr,data_size);
	   handle->read_ptr += data_size;
	   if(handle->read_ptr >= handle->base_address+handle->size)
         handle->read_ptr = handle->base_address;
   }
   else
   {
      memcpy(data,handle->read_ptr,bytes_left);
      handle->read_ptr = handle->base_address;
      memcpy(data,handle->read_ptr,data_size - bytes_left);
	   handle->read_ptr += data_size - bytes_left;
   }
   handle->bytes_read += data_size;
   return(0);
}


/***********************************************************
 * Name: software_fifo_write
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *            uint8_t * data - data to write
 *            uint32_t data_size - how much data to write (in bytes)
 *
 * Description: Writes the required amount of data to the FIFO
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_write( SOFTWARE_FIFO_HANDLE_T *handle, const void *data, const uint32_t data_size )
{
uint32_t bytes_left;

   os_assert(handle->base_address);
   // check that there is enough room in the FIFO to fulfill this request
   if( software_fifo_room_available(handle) < data_size )
      return(-1);
   bytes_left = (handle->base_address+handle->size) - handle->write_ptr;
   if(bytes_left >= data_size)
   {
      memcpy(handle->write_ptr,data,data_size);
	  handle->write_ptr += data_size;
	  if(handle->write_ptr >= handle->base_address+handle->size)
         handle->write_ptr = handle->base_address;
   }
   else
   {
      memcpy(handle->write_ptr,data,bytes_left);
      handle->write_ptr = handle->base_address;
      memcpy(handle->write_ptr,data,data_size - bytes_left);
      handle->write_ptr += data_size - bytes_left;
   }
   handle->bytes_written += data_size;
   return(0);
}


/***********************************************************
 * Name: software_fifo_data_available
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T handle - handle to this FIFO
 *
 * Description: Returns the amount of data available to be read
 *
 * Returns: negative values are errors
 *
 ***********************************************************/
int32_t software_fifo_data_available( SOFTWARE_FIFO_HANDLE_T *handle )
{
int32_t data_available = (int32_t)(handle->bytes_written - handle->bytes_read);

   os_assert(handle->base_address);
   os_assert(data_available <= handle->size);

   return( data_available );
}


/***********************************************************
 * Name: software_fifo_room_available
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T handle - handle to this FIFO
 *
 * Description: Returns the amount of space left in the FIFO
 *
 * Returns: negative values are errors
 *
 ***********************************************************/
int32_t software_fifo_room_available( SOFTWARE_FIFO_HANDLE_T *handle )
{
int32_t room_available = (int32_t)handle->size - (int32_t)((handle->bytes_written - handle->bytes_read));

   os_assert(handle->base_address);
   os_assert((room_available <= handle->size) && (room_available >= 0));

   return( room_available );
}


/***********************************************************
 * Name: software_fifo_peek
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *            uint8_t * data - where to read the data to
 *            uint32_t data_size - how much data to read (in bytes)
 *
 * Description: Reads the required amount of data from the FIFO, but does not advance the read pointer
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_peek( SOFTWARE_FIFO_HANDLE_T *handle, void *data, const uint32_t data_size )
{
uint32_t bytes_left;

   os_assert(handle->base_address);
   // check that there is enough data in the FIFO to fulfill this request
   if( software_fifo_data_available(handle) < data_size )
      return(-1);
   bytes_left = (handle->base_address+handle->size) - handle->read_ptr;
   if(bytes_left >= data_size)
   {
      memcpy(data,handle->read_ptr,data_size);
   }
   else
   {
      memcpy(data,handle->read_ptr,bytes_left);
      memcpy(data,handle->base_address,data_size - bytes_left);
   }
   return(0);
}


/***********************************************************
 * Name: software_fifo_remove
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *            uint32_t data_size - how much data to read (in bytes)
 *
 * Description: Advances the read pointer appropriately
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_remove( SOFTWARE_FIFO_HANDLE_T *handle, const uint32_t data_size )
{
uint32_t bytes_left;

   os_assert(handle->base_address);
   // check that there is enough data in the FIFO to fulfill this request
   if( software_fifo_data_available(handle) < data_size )
      return(-1);
   bytes_left = (handle->base_address+handle->size) - handle->read_ptr;
   if(bytes_left >= data_size)
   {
	   handle->read_ptr += data_size;
	   if(handle->read_ptr >= handle->base_address+handle->size)
         handle->read_ptr = handle->base_address;
   }
   else
   {
      handle->read_ptr = handle->base_address;
	   handle->read_ptr += data_size - bytes_left;
   }
   handle->bytes_read += data_size;
   return(0);
}


/***********************************************************
 * Name: software_fifo_protect
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *
 * Description: Acquires the semaphore used to protect the SOFTWARE_FIFO_HANDLE_T structure
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_protect( SOFTWARE_FIFO_HANDLE_T *handle )
{
   return(os_semaphore_obtain(&handle->software_fifo_semaphore));
}


/***********************************************************
 * Name: software_fifo_unprotect
 *
 * Arguments: SOFTWARE_FIFO_HANDLE_T *handle - handle to this FIFO
 *
 * Description: Releases the semaphore used to protect the SOFTWARE_FIFO_HANDLE_T structure
 *
 * Returns: 0 if successful, any other value is failure
 *
 ***********************************************************/
int32_t software_fifo_unprotect( SOFTWARE_FIFO_HANDLE_T *handle )
{
   return(os_semaphore_release(&handle->software_fifo_semaphore));
}


/* ************************************ The End ***************************************** */
