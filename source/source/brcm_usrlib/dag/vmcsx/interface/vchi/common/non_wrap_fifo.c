/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  Inter-processor communication
Module   :  non wrap FIFO
File     :  $RCSfile:  $
Revision :  $Revision$

FILE DESCRIPTION:

Non wrap FIFO, designed to allow data to be read / written in a contiguous fashion.
This may cause some of the memory in the FIFO to be wasted but it will enable
data to be DMA'ed into and out of it in a single transaction. This is useful for
the B0 MPHI reply mechanism. It will also be useful for a multi-FIFO Host Port
implementation.
The FIFO is assumed to be used with whole 'messages' read or written each time
i.e. the length specified in a write request will be the same as the length needed
for the read.
=============================================================================*/

#include <stdlib.h>
#include <string.h>

#include "interface/vchi/os/os.h"
#include "non_wrap_fifo.h"


/******************************************************************************
  Local typedefs
 *****************************************************************************/

typedef enum {
  NON_WRAP_NOT_IN_USE = 0,
  NON_WRAP_WRITING,
  NON_WRAP_READY,
  NON_WRAP_READING
} NON_WRAP_STATE_T;

// need to record the current state of write / read accesses
typedef struct {
   uint8_t          *address;
   uint32_t         length;
   NON_WRAP_STATE_T state;
} NON_WRAP_SLOT_T;

typedef struct {
   const char      *name;
   uint8_t         *malloc_address;
   uint8_t         *base_address;
   uint8_t         *read_ptr;
   uint8_t         *write_ptr;
   uint32_t        size;
   NON_WRAP_SLOT_T *slot_info;
   uint32_t        write_slot;
   uint32_t        read_slot;
   uint32_t        num_of_slots;
   // semaphore to protect this structure
   OS_SEMAPHORE_T  sem;
   // condition variable used to indicate space becoming available
   OS_COND_T       space_available_cond;
} NON_WRAP_FIFO_HANDLE_T;


/******************************************************************************
 Static functions
 *****************************************************************************/
static uint32_t next_read_slot( NON_WRAP_FIFO_HANDLE_T *fifo, uint32_t current_slot );
static uint32_t next_slot( NON_WRAP_FIFO_HANDLE_T *fifo, uint32_t current_slot );


/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * allocate space for a new non-wrap fifo, and return pointer to
 * opaque handle.  returns NULL on failure
 * -------------------------------------------------------------------- */
VCHI_NWFIFO_T *
non_wrap_fifo_create( const char *name, const uint32_t size, int alignment, int no_of_slots )
{
   int32_t success = -1;
   NON_WRAP_FIFO_HANDLE_T *fifo;

   // check that the alignment is a power of 2 or 0
#ifndef WIN32
   os_assert((OS_COUNT(alignment) == 1) || (alignment == 0));
#endif

   // put the alignment into a form we can use (must be power of 2)
   alignment = alignment ? alignment - 1 : 0;

   // create new non-wrap fifo struct
   fifo = (NON_WRAP_FIFO_HANDLE_T *)os_malloc( sizeof(NON_WRAP_FIFO_HANDLE_T), OS_ALIGN_DEFAULT, "" );
   os_assert( fifo );
   if (!fifo) return NULL;
   
   memset( fifo, 0, sizeof(NON_WRAP_FIFO_HANDLE_T) );
   fifo->name = name;

   // allocate slightly more space than we need so that we can get the required alignment
   fifo->malloc_address = (uint8_t *)os_malloc(size+alignment, OS_ALIGN_DEFAULT, "");
   if ( fifo->malloc_address ) {
      fifo->base_address = (uint8_t *)(((size_t)fifo->malloc_address + alignment) & ~alignment);
      fifo->read_ptr = fifo->base_address;
      fifo->write_ptr = fifo->base_address;
      // now allocate the space for the slot_info structures
      fifo->slot_info = (NON_WRAP_SLOT_T *)os_malloc(no_of_slots*sizeof(NON_WRAP_SLOT_T), OS_ALIGN_DEFAULT, "");
      memset( fifo->slot_info, 0, no_of_slots * sizeof(NON_WRAP_SLOT_T) );
      fifo->size = size;
      fifo->num_of_slots = no_of_slots;
      success = os_semaphore_create( &fifo->sem, OS_SEMAPHORE_TYPE_SUSPEND );
      os_assert(success == 0);
      success = os_cond_create( &fifo->space_available_cond, &fifo->sem );
      os_assert(success == 0);
   }
   else {
      os_free(fifo);
      fifo = NULL;
   }

   return (VCHI_NWFIFO_T *)fifo;
}

/* ----------------------------------------------------------------------
 * tear down data structures, and free memory, associated with
 * the given non-wrap fifo handle
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_destroy( VCHI_NWFIFO_T *_fifo )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;

   // first confirm that the handle has been previously allocated
   if ( fifo->base_address ) {
      // free the FIFO memory
      free( fifo->malloc_address );
      // free the slot info memory
      free( fifo->slot_info );
      // destroy the os signalling
      os_cond_destroy( &fifo->space_available_cond );
      os_semaphore_destroy( &fifo->sem );
      // free the structure
      free( fifo );
      // success!
      success = 0;
   }
   return success;
}

#if 0
/* ----------------------------------------------------------------------
 * read data from the given non-wrap fifo, by copying out of the
 * fifo to the given address
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_read( VCHI_NWFIFO_T *_fifo, void *data, const uint32_t max_data_size )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t  success = -1;
   uint8_t *address = NULL;
   uint32_t length;

   os_assert(0); // FIXME: will deadlock
   os_semaphore_obtain( &fifo->sem );
   os_assert( fifo->base_address );
   // request the address and length for this transfer
   if ( non_wrap_fifo_request_read_address(fifo,&address,&length) == 0 ) {
      if ( length <= max_data_size ){
         // there is enough space and the data has been written so we can copy it out
         memcpy(data,address,length);
         // now tidy up our information about what is in the FIFO
         if ( non_wrap_fifo_read_complete(fifo,address) == 0 ) {
            // success!
            success = 0;
         }
      }
   }
   os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * write data to the given non-wrap fifo, by copying from the given
 * address
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_write( VCHI_NWFIFO_T *_fifo, const void *data, const uint32_t data_size )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t  success = -1;
   uint8_t *address = NULL;

   os_assert( fifo->base_address );

   os_assert(0); // FIXME: will deadlock
   os_semaphore_obtain( &fifo->sem );
   // request the address and length for this transfer
   if ( non_wrap_fifo_request_write_address(fifo,&address,data_size) == 0 ) {
      // there is enough space so we can write the data
      memcpy(address,data,data_size);
      // now tidy up our information about what is in the FIFO
      if ( non_wrap_fifo_write_complete(fifo,address) == 0 ) {
         // success!
         success = 0;
      }
   }
   os_semaphore_release( &fifo->sem );
   return success;
}
#endif

/* ----------------------------------------------------------------------
 * returns address of free space within the given non-wrap fifo
 * that can take the requested amount of data.
 *
 * intended to be used with non_wrap_fifo_write_complete, below
 *
 * NOTE: on success, this routine will leave the fifo locked!
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_request_write_address( VCHI_NWFIFO_T *_fifo, void **address, uint32_t size, int block )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;
   uint32_t slot;
   uint32_t space;
   uint8_t *address1;
   uint8_t *address2 = 0;
   int wrapped;

   os_semaphore_obtain( &fifo->sem );

   os_assert( fifo->base_address );

   do {
      // before we start need to confirm that the write slot does not currently hold some data
      if ( fifo->slot_info[fifo->write_slot].state != NON_WRAP_NOT_IN_USE ) {
         os_semaphore_release( &fifo->sem );
         return -1;
      }

      slot = fifo->read_slot;

      // To simplify the logic we will only write messages contiguously, so we will sometimes lose space at the end
      // when the message is too big for the space available
      address1 = fifo->slot_info[slot].address;
      if ( address1 == NULL )
         address1 = fifo->base_address;
      // if this slot does not contain anything then the buffer is empty and we can start again from the beginning
      if ( fifo->slot_info[slot].state == NON_WRAP_NOT_IN_USE ) {
         *address = fifo->base_address;
         // check that there is enough room
         if ( size <= fifo->size )
            success = 0;
      }
      else
      {
         // need to find out how much room there is
         wrapped = VC_FALSE;
         while( slot != fifo->write_slot ) {
            address2 = fifo->slot_info[slot].address + fifo->slot_info[slot].length;
            // move slot on, taking care to wrap
            slot = next_read_slot( fifo, slot );
            if ( fifo->slot_info[slot].state != NON_WRAP_NOT_IN_USE ) {
               if ( fifo->slot_info[slot].address < address2 )
                  wrapped = VC_TRUE;
            }
         }
         // now we can work out which address and how much space is available
         if ( wrapped ) {
            space = (uint32_t)(address1 - address2);
            if ( space >= size ) {
               // set the required address
               *address = address2;
               // record success
               success = 0;
            }
         }
         else
         {
            // is there enough space at the end of the buffer
            if ( (uint32_t)(fifo->base_address + fifo->size - address2) >= size ) {
               *address = address2;
               success = 0;
            } else {
               // now see if there is enough room if we do a wrap
               if ( (uint32_t)(address1 - fifo->base_address) >= size ) {
                  *address = fifo->base_address;
                  success = 0;
               }
            }
         }
      }
      if (success != 0 && block)
      {
         os_cond_wait( &fifo->space_available_cond, &fifo->sem );
      }
   } while (success != 0 && block);

   // record the write info in our list of slots
   if ( success == 0 ) {
      fifo->slot_info[fifo->write_slot].state   = NON_WRAP_WRITING;
      fifo->slot_info[fifo->write_slot].address = (uint8_t *)*address;
      fifo->slot_info[fifo->write_slot].length  = size;
   }

   // on success, the fifo is left locked
   if ( success != 0 )
      os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * used in conjunction with non_wrap_fifo_request_write_address, to
 * signify completion
 *
 * NOTE: this routine will unlock the fifo!
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_write_complete( VCHI_NWFIFO_T *_fifo, void *address )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;

   os_assert( fifo->base_address );

   //os_semaphore_obtain( &fifo->sem );
   // it should be the case that the writes will complete sequentially
   // but we will verify it
   if ( fifo->slot_info[fifo->write_slot].address == address ) {
      // mark that this message has been written
      fifo->slot_info[fifo->write_slot].state = NON_WRAP_READY;
      // point to the next message
      fifo->write_slot = next_slot( fifo, fifo->write_slot );
      // success!
      success = 0;
   }
   os_assert( success == 0 );

   os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * fetch the address and length of the oldest message in the given
 * non-wrap fifo
 *
 * NOTE: on success, this routine will leave the fifo locked!
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_request_read_address( VCHI_NWFIFO_T *_fifo, const void **address, uint32_t *length )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;

   os_semaphore_obtain( &fifo->sem );
   os_assert( fifo->base_address );

   // check that this slot is ready to be read from
   if ( fifo->slot_info[fifo->read_slot].state == NON_WRAP_READY ) {

      *address = fifo->slot_info[fifo->read_slot].address;
      *length  = fifo->slot_info[fifo->read_slot].length;

      // mark this slot as being in the process of being read
      fifo->slot_info[fifo->read_slot].state = NON_WRAP_READING;

      // success!
      success = 0;
   }

   // on success, the fifo is left locked
   if ( success != 0 )
      os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * used in conjunction with non_wrap_fifo_request_read_address, to
 * signify completion
 *
 * NOTE: this routine will unlock the fifo!
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_read_complete( VCHI_NWFIFO_T *_fifo, void *address )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;

   //os_semaphore_obtain( &fifo->sem );
   os_assert( fifo->base_address );
   // it should be the case that the reads will complete sequentially
   // but we will verify it
   if ( fifo->slot_info[fifo->read_slot].address == address ) {
      // mark that this message has been read
      fifo->slot_info[fifo->read_slot].state = NON_WRAP_NOT_IN_USE;
      // point to the next message
      fifo->read_slot = next_read_slot( fifo, fifo->read_slot );
      // success!
      success = os_cond_broadcast( &fifo->space_available_cond, VC_TRUE );
   }
   
   os_assert( success == 0 );
   os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * remove the slot with the given address from the given non-wrap fifo.
 * intended for out-of-order operations
 *
 * the given address must belong to a slot between the read and write
 * slots, and be in the 'ready' state
 *
 * returns 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
int32_t
non_wrap_fifo_remove( VCHI_NWFIFO_T *_fifo, void *address )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   int32_t success = -1;
   uint32_t slot;

   os_semaphore_obtain( &fifo->sem );

   slot = fifo->read_slot;
   os_assert( fifo->base_address );

   while( slot != fifo->write_slot ) {

      if ( fifo->slot_info[slot].address == address && fifo->slot_info[slot].state == NON_WRAP_READY ) {
         // mark this as no longer being in use
         fifo->slot_info[slot].state = NON_WRAP_NOT_IN_USE;
         success = 0;
         break;
      }
      // increment and wrap our slot number
      slot = next_slot( fifo, slot );
   }

   // if the entry to be removed was the current read slot then we need to move on
   if ( slot == fifo->read_slot && success == 0 )
      fifo->read_slot = next_read_slot( fifo, slot );

   os_semaphore_release( &fifo->sem );
   return success;
}

/* ----------------------------------------------------------------------
 * return the next read slot in the given non-wrap fifo.
 *
 * handles the case where a slot has been removed by skipping over it.
 * -------------------------------------------------------------------- */
static uint32_t
next_read_slot( NON_WRAP_FIFO_HANDLE_T *fifo, uint32_t current_slot )
{
   do {
      current_slot++;
      if ( current_slot >= fifo->num_of_slots )
         current_slot = 0;
   } while( fifo->slot_info[current_slot].state == NON_WRAP_NOT_IN_USE && current_slot != fifo->write_slot );

   return current_slot;
}

/* ----------------------------------------------------------------------
 * return the next slot in the given non-wrap fifo.
 * -------------------------------------------------------------------- */
static uint32_t
next_slot( NON_WRAP_FIFO_HANDLE_T *fifo, uint32_t current_slot )
{
   current_slot++;
   if ( current_slot >= fifo->num_of_slots )
      current_slot = 0;
   return current_slot;
}

/* ----------------------------------------------------------------------
 * dump debug info on the given non-wrap fifo
 *
 * expects the fifo to be already locked
 * -------------------------------------------------------------------- */
void
non_wrap_fifo_debug( VCHI_NWFIFO_T *_fifo )
{
   NON_WRAP_FIFO_HANDLE_T *fifo = (NON_WRAP_FIFO_HANDLE_T *)_fifo;
   uint32_t i;

   os_logging_message( "----------------------------------------" );
   //os_semaphore_obtain( &fifo->sem );
   os_logging_message( "nwfifo_debug: %s", fifo->name );
   os_logging_message( "read_slot=%d, write_slot=%d", fifo->read_slot, fifo->write_slot );

   for (i=0; i<fifo->num_of_slots; i++) {
      NON_WRAP_SLOT_T *slot = &fifo->slot_info[ i ];
      uint8_t *addr = slot->address;
      os_logging_message( "slot %d (%p,%d=%d)", i, addr, slot->length, slot->state );
      if ( (size_t)addr < 0x1000 ) continue;
      if ( slot->length == 0 ) continue;
      if ( slot->state == 0 ) continue;
      os_logging_message( "--> %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7],
                          addr[8], addr[9], addr[10], addr[11], addr[12], addr[13], addr[14], addr[15] );
   }
   //os_semaphore_release( &fifo->sem );
   os_logging_message( "----------------------------------------" );
}


/* ************************************ The End ***************************************** */
