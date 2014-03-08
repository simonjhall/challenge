/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VCFW
Module   :  Message-Passing Host Interface driver
File     :  $RCSfile: mphi.c,v $
Revision :  $Revision: #7 $

FILE DESCRIPTION

=============================================================================*/

#include <stdlib.h>
#include <string.h>

#include "vcinclude/hardware.h"
#include "interface/vchi/os/os.h"
#include "interface/vchi/common/endian.h"
#include "tools/usbhp/bcm2727dkb0rev2/hp2727dk/hpapi.h"



#include "vcfw/drivers/chip/mphi.h"
#include "vcfw/drivers/chip/mphi_cfg.h"

/******************************************************************************
  Local typedefs
 *****************************************************************************/

typedef enum {
   EVENT_NONE,
   EVENT_TXEND,
   EVENT_RX0TEND,
   EVENT_RX0MEND,
   EVENT_RX1TEND
} EMU_EVENT_T;

typedef struct {
   OS_SEMAPHORE_T	latch;

   int	refcount;

   // emulated event state

   EMU_EVENT_T emu_event;
   uint32_t emu_event_idx;
   uint32_t emu_event_pos;

   // the hardware-emulation thread takes this semaphore to prevent multiple
   // writes to the emulated event state

   OS_SEMAPHORE_T emu_latch;
   OS_SEMAPHORE_T	emu_latch_handled;

   // the lisr will call this to indicate there are events which need handling
   void (*event_callback)( const void *cb_data );
   const void *event_callback_data;
} MPHI_STATE_T;


typedef struct {
   OS_SEMAPHORE_T	latch;

   int              refcount;

   // copy of ->open params
   MPHI_CHANNEL_T   channel;
   //MPHI_CALLBACK_T *callback;
} MPHI_HANDLE_T;


/******************************************************************************
 Extern functions
 *****************************************************************************/

/******************************************************************************
 Static functions
 *****************************************************************************/

//Routine used to initilise the driver
static int32_t mphi_init( void );

//Routine used to exit the driver
static int32_t mphi_exit( void );

//Routine to get the driver info
static int32_t mphi_info( const char **driver_name,
                          uint32_t *version_major,
                          uint32_t *version_minor,
                          DRIVER_FLAGS_T *flags );

//Routine used to open the driver (returns a handle)
static int32_t mphi_open( const MPHI_OPEN_T *params,
                          DRIVER_HANDLE_T *handle );

//Routine used to close the driver
static int32_t mphi_close( const DRIVER_HANDLE_T handle );

// local (chip-level) read and write.  can be overridden by platform
static MPHI_OPEN_FUNC mphi_local_open;
static MPHI_CLOSE_FUNC mphi_local_close;

static void mphi_thread( void * );

static int32_t  mphi_add_recv_slot( const DRIVER_HANDLE_T handle, const void *addr, uint32_t len, uint8_t slot_id );
static int32_t  mphi_out_queue_message( const DRIVER_HANDLE_T handle, uint8_t control, const void *addr, size_t len, uint8_t msg_id, const MPHI_FLAGS_T flags );
//static int      mphi_fifo_used( const DRIVER_HANDLE_T handle );
//static int      mphi_fifo_available( const DRIVER_HANDLE_T handle );

static int      mphi_slots_available( DRIVER_HANDLE_T handle );
static int      mphi_channel_available( MPHI_CHANNEL_T channel );
static int      mphi_channel_used( MPHI_CHANNEL_T channel );

static MPHI_EVENT_TYPE_T mphi_next_event( const DRIVER_HANDLE_T handle, uint8_t *slot_id, uint32_t *pos );

// Routine used to enable the logic and interrupts
static int32_t mphi_enable( const DRIVER_HANDLE_T handle );

// Set drive power (mA) and slew (0=high power, 1=slew, low power)
static int32_t mphi_set_power( const DRIVER_HANDLE_T handle, int drive_mA, int slew );

// Get address alignment requirement
static int     mphi_alignment( const DRIVER_HANDLE_T handle );

// dump debug info to logging
static void     mphi_debug( const DRIVER_HANDLE_T handle );

/******************************************************************************
 Static data
 *****************************************************************************/

//The driver function table
static const MPHI_DRIVER_T mphi_func_table =
{
   // common driver api
   &mphi_init,
   &mphi_exit,
   &mphi_info,
   &mphi_open,
   &mphi_close,

   // mphi-specific api
   &mphi_add_recv_slot,
   &mphi_out_queue_message,
   &mphi_slots_available,
   &mphi_next_event,
   &mphi_enable,
   &mphi_set_power,
   &mphi_alignment,
   &mphi_debug
};

static MPHI_STATE_T mphi_state;

static MPHI_HANDLE_T mphi_handle[ MPHI_CHANNEL_MAX ];

//the automatic registering of the driver - DO NOT REMOVE!
static const MPHI_DRIVER_T *mphi_func_table_ptr = &mphi_func_table;

/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * return pointer to the mphi driver function table
 * -------------------------------------------------------------------- */
const MPHI_DRIVER_T *
mphi_get_func_table( void )
{
   return mphi_func_table_ptr;
}


/******************************************************************************
 Static  Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * initialise the mphi driver
 * this is part of the common driver api
 * return 0 on success; all other values are failures
 * -------------------------------------------------------------------- */
static int32_t
mphi_init( void )
{
   int32_t success = -1; // fail by default

   int i;

   memset( &mphi_state, 0, sizeof(mphi_state) );

   // Create latch to protect the state structure
   success = os_semaphore_create(&mphi_state.latch,
								  OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert(success == 0);

   success = os_semaphore_create(&mphi_state.emu_latch,
                               OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert(success == 0);

   success = os_semaphore_create(&mphi_state.emu_latch_handled,
                               OS_SEMAPHORE_TYPE_SUSPEND );
   os_assert(success == 0);



   for (i=0; i<MPHI_CHANNEL_MAX; i++) {
      MPHI_HANDLE_T *m = &mphi_handle[i];
      memset( m, 0, sizeof(MPHI_HANDLE_T) );

	  success = os_semaphore_create(&m->latch,
									OS_SEMAPHORE_TYPE_SUSPEND );
   }

   return 0;
}

/* ----------------------------------------------------------------------
 * exit the mphi driver
 * this is part of the common driver api
 * return 0 on success; all other values are failures
 * -------------------------------------------------------------------- */
static int32_t
mphi_exit( void )
{
   int32_t success = -1; // fail by default

   // can we exit this driver?
   os_assert( 0 );

   return success;
}

/* ----------------------------------------------------------------------
 * get the mphi driver info
 * this is part of the common driver api
 * return 0 on success; all other values are failures
 * -------------------------------------------------------------------- */
static int32_t
mphi_info( const char **driver_name,
           uint32_t *version_major,
           uint32_t *version_minor,
           DRIVER_FLAGS_T *flags )
{
   int32_t success = -1; // fail by default

   // return the driver name
   if ( driver_name && version_major && version_minor && flags ) {

      *driver_name = "mphi";
      *version_major = 0;
      *version_minor = 1;
      *flags = 0;

      // success!
      success = 0;
   }

   return success;
}

/* ----------------------------------------------------------------------
 * open the driver and obtain a handle
 * this is part of the common driver api
 * return 0 on success; all other values are failures
 * -------------------------------------------------------------------- */
static int32_t
mphi_open( const MPHI_OPEN_T *params, DRIVER_HANDLE_T *handle )
{
   VCOS_THREAD_T ti;
   int32_t success = 0;

   //WaitForSingleObject(mphi_state.latch, INFINITE);
   os_semaphore_obtain(&mphi_state.latch);

   if ( mphi_state.refcount == 0 ) {
      // launch dispatch thread
      os_thread_start(&ti, ((OS_THREAD_FUNC_T)mphi_thread), NULL, 1024, NULL);
      mphi_state.refcount++;
   }

   // now try to open the given channel
   if ( params && params->channel < MPHI_CHANNEL_MAX && params->channel != MPHI_CHANNEL_IN_HOST_SPECIFIED ) {
      MPHI_HANDLE_T *mphi = &mphi_handle[ params->channel ];

      //WaitForSingleObject(mphi->latch, INFINITE);
	  os_semaphore_obtain(&mphi->latch);

      // permit each channel to be opened once
      if ( mphi->refcount != 0 ) success = -1; // failure
      mphi->refcount++;

      // copy open params info
      mphi->channel  = params->channel;
      //mphi->callback = params->callback;

      if ( mphi_state.event_callback || mphi_state.event_callback_data ) {
         os_assert( mphi_state.event_callback == params->callback ); // can have only one
         os_assert( mphi_state.event_callback_data == params->callback_data );
      } else {
         mphi_state.event_callback = params->callback;
         mphi_state.event_callback_data = params->callback_data;
      }


      //ReleaseMutex(mphi->latch);
	  os_semaphore_release(&mphi->latch);

      // finally fill in return values
      if ( success == 0 )
         *handle = (DRIVER_HANDLE_T) mphi;
   }

   // release the latch
   //ReleaseMutex(mphi_state.latch);
   os_semaphore_release(&mphi_state.latch);
   os_assert( success == 0 );
   return success;
}

/* ----------------------------------------------------------------------
 * close a handle to the driver
 * this is part of the common driver api
 *
 * return 0 on success; all other values are failures
 * -------------------------------------------------------------------- */
static int32_t
mphi_close( const DRIVER_HANDLE_T handle )
{
   int32_t success = -1; // fail by default
   MPHI_HANDLE_T *mphi = (MPHI_HANDLE_T *) handle;

   // close the given handle...
   //WaitForSingleObject(mphi_state.latch, INFINITE);
   os_semaphore_obtain(&mphi_state.latch);

   // should only be opened once
   //WaitForSingleObject(mphi->latch, INFINITE);
   os_semaphore_obtain(&mphi->latch);

   if ( mphi->refcount == 1 ) success = 0;
   mphi->refcount--;

   os_semaphore_release(&mphi->latch);

   // ...then perform generic module closing...
   if ( success == 0 ) {
      mphi_state.refcount--;

      mphi_state.event_callback = NULL;
      mphi_state.event_callback_data = NULL;
   }

   //ReleaseMutex(mphi_state.latch );
   os_semaphore_release(&mphi_state.latch);

   os_assert( success == 0 );

   return success;
}

/* ----------------------------------------------------------------------
 * each of the two input channels has its own dma fifo, each
 * containing a number of slots.  as incoming messages are received,
 * they will be placed in the next available slot from this fifo.
 *
 * this routine tries to fill in a slot with the given address/len
 * conbination.  if there are no free slots, this routine returns
 * immediately with MPHI_ERROR_FIFO_FULL.
 *
 * when a message is received, if no entry is available in the FIFO,
 * the message is dropped, and a MPHI_EVENT_IN_MESSAGE_DISCARDED
 * is generated.
 *
 * if a DMA entry is available, then the message is placed into memory
 * and MPHI_EVENT_IN_DMA_COMPLETED is generated.
 * -------------------------------------------------------------------- */

#define EMU_SLOT_COUNT  16

typedef struct {
   uint8_t *addr;
   uint32_t chan;
   uint32_t term;
   uint32_t idx;
   uint32_t len;
   uint32_t pos;
} EMU_SLOT_T;

typedef struct {
   uint32_t read;
   uint32_t write;

   EMU_SLOT_T slots[EMU_SLOT_COUNT];
} EMU_FIFO_T;

static void emu_push_slot(EMU_FIFO_T *fifo, const void *addr, uint32_t chan, uint32_t term, uint32_t idx, uint32_t len)
{
   EMU_SLOT_T *slot = &fifo->slots[fifo->write & EMU_SLOT_COUNT-1];

   os_assert(fifo->write - fifo->read < EMU_SLOT_COUNT);

   slot->addr = (uint8_t *)addr;
   slot->chan = chan;
   slot->term = term;
   slot->idx = idx;
   slot->len = len;
   slot->pos = 0;

   fifo->write++;
}

static EMU_SLOT_T *emu_peek_slot(EMU_FIFO_T *fifo)
{
   os_assert(fifo->write - fifo->read > 0);

   return &fifo->slots[fifo->read & EMU_SLOT_COUNT-1];
}

static void emu_pop_slot(EMU_FIFO_T *fifo)
{
   os_assert(fifo->write - fifo->read > 0);

   fifo->read++;
}

static EMU_FIFO_T emu_in[2];
static EMU_FIFO_T emu_out;

static int32_t
mphi_add_recv_slot( const DRIVER_HANDLE_T handle, const void *addr, uint32_t len, uint8_t slot_id )
{
   MPHI_HANDLE_T *mphi = (MPHI_HANDLE_T *) handle;
   // NOTE: this routine is now internal only to this driver.
   // we are called from:
   //   1) mphi_open(), before receive interrupts are turned-on
   //   2) mphi_interrupt(), in lisr context, to chain the next slot
   MPHI_CHANNEL_T chan = mphi->channel;

   //os_logging_message( "mphi_add_recv_slot slot_id = %d, addr = 0x%X, free entries in FIFO = %d",
   //slot_id, (int)addr, mphi_channel_available(chan) );

   if ( chan != MPHI_CHANNEL_IN_CONTROL && chan != MPHI_CHANNEL_IN_DATA ) return -1;
   if ( (len & 0xf) || len == 0 || len > 0x1000000 ) return -1;

   if ( mphi_channel_available(chan) == 0 )
      return MPHI_ERROR_FIFO_FULL;

   if ( chan == MPHI_CHANNEL_IN_CONTROL ) {
      emu_push_slot(&emu_in[0],
                    addr,
                    0,
                    0,
                    slot_id,
                    len);
   } else {
      emu_push_slot(&emu_in[1],
                    addr,
                    1,
                    0,
                    slot_id,
                    len);
   }
   return 0;
}

/* ----------------------------------------------------------------------
 * queue a message for transmission.  the two output channels share a
 * single fifo.
 *
 * if the output fifo is full, this routine returns immediately with
 * MPHI_ERROR_FIFO_FULL.
 *
 * when the message has been fully read from memory,
 * MPHI_EVENT_OUT_DMA_COMPLETED is generated.  note that this does
 * not mean that the host has read the message; the message is placed
 * in a message fifo and held until the host responds to its interrupt
 * and issues the appropriate number of read cycles.
 * -------------------------------------------------------------------- */
static int32_t
mphi_out_queue_message( const DRIVER_HANDLE_T handle, uint8_t control, const void *addr, size_t len, uint8_t slot_id, const MPHI_FLAGS_T flags )
{
   MPHI_HANDLE_T *mphi = (MPHI_HANDLE_T *) handle;

   if ( mphi->channel != MPHI_CHANNEL_OUT ) return -3;
   if ( (len & 0xf) || len == 0 || len > 0x1000000 ) return -5;

   //WaitForSingleObject(mphi->latch, INFINITE);
   os_semaphore_obtain(&mphi->latch);

   if (emu_out.write - emu_out.read == EMU_SLOT_COUNT) {
      os_logging_message( "mphi_out_queue_message: no free space" );
      //ReleaseMutex(mphi->latch);
      os_semaphore_release(&mphi->latch);
	  return MPHI_ERROR_FIFO_FULL;
   }

   emu_push_slot(&emu_out,
                 addr,
                 control ? 0 : 1,
                 flags & MPHI_FLAGS_TERMINATE_DMA ? 1 : 0,
                 slot_id,
                 (uint32_t)len);

   // release the latch
   //ReleaseMutex(mphi->latch);
   os_semaphore_release(&mphi->latch);
   return 0;
}

/* ----------------------------------------------------------------------
 * how many free slots are there on this channel?
 *
 * the first function is publically exported via the ->ops table
 *
 * the other two functions are internal
 * -------------------------------------------------------------------- */
static int
mphi_slots_available( DRIVER_HANDLE_T handle )
{
   MPHI_HANDLE_T *mphi = (MPHI_HANDLE_T *) handle;
   return mphi_channel_available( mphi->channel );
}

static int
mphi_channel_available( MPHI_CHANNEL_T channel )
{
   switch( channel ) {

   case MPHI_CHANNEL_IN_CONTROL:
      return MPHI_RX_MSG_SLOTS - mphi_channel_used( channel );

   case MPHI_CHANNEL_IN_DATA:
      return MPHI_RX_BULK_SLOTS - mphi_channel_used( channel );

   case MPHI_CHANNEL_IN_HOST_SPECIFIED:
      // doesn't really match the other channels, but may as well return something
      return 16 - mphi_channel_used( channel );

   case MPHI_CHANNEL_OUT:
      return MPHI_TX_SLOTS - mphi_channel_used( channel );

   default:
      os_assert(0);
      return 0;
   }
}

static int
mphi_channel_used( MPHI_CHANNEL_T channel )
{
   switch( channel ) {

   case MPHI_CHANNEL_IN_CONTROL:
      return emu_in[0].write - emu_in[0].read;

   case MPHI_CHANNEL_IN_DATA:
      return emu_in[1].write - emu_in[1].read;

   case MPHI_CHANNEL_IN_HOST_SPECIFIED:
      // doesn't really match the other channels, so assert
      os_assert(0);
      return 0;

   case MPHI_CHANNEL_OUT:
      return emu_out.write - emu_out.read;

   default:
      os_assert(0);
      return 0;
   }
}

static void
mphi_thread( void *v )
{
	HP_HANDLE_T hp = hp_open(NULL, 0, HPDT_ANY, 1);

   if (hp == NULL)
      os_assert(0);

   while (1) {
      /*
         receive an available incoming message
      */

      uint32_t status;
      hp_get_status(hp, (unsigned long *)&status);

      if (status & 0x40000000) {
         uint32_t channel = status >> 28 & 1;
         uint32_t length = (status >> 1 & 0x00ff8000 | status & 0x00007fff) + 1;

label:
         if (emu_in[channel].write - emu_in[channel].read > 0) {
            EMU_SLOT_T *slot = emu_peek_slot(&emu_in[channel]);

            if (slot->pos + length > slot->len) {
               /*
                  need to turf the slot!
               */

               os_assert(!channel);

               emu_pop_slot(&emu_in[channel]);

               os_semaphore_obtain(&mphi_state.emu_latch_handled);

			      os_semaphore_obtain(&mphi_state.emu_latch);
               mphi_state.emu_event_idx = slot->idx;
               mphi_state.emu_event_pos = slot->pos;
               mphi_state.emu_event = EVENT_RX0TEND;
               os_semaphore_release(&mphi_state.emu_latch);

               if ( mphi_state.event_callback )
                  mphi_state.event_callback( mphi_state.event_callback_data);

               goto label;
            }

            hp_read_host(hp, length, slot->addr + slot->pos);

            slot->pos += length;

            os_assert(slot->pos <= slot->len);

            os_semaphore_obtain(&mphi_state.emu_latch_handled);

			   os_semaphore_obtain(&mphi_state.emu_latch);
            mphi_state.emu_event_idx = slot->idx;
            mphi_state.emu_event_pos = slot->pos;

            if (channel) {
               os_assert(slot->pos == slot->len);

               emu_pop_slot(&emu_in[channel]);

               mphi_state.emu_event = EVENT_RX1TEND;
            } else {
               if (slot->pos == slot->len) {
                  emu_pop_slot(&emu_in[channel]);

                  mphi_state.emu_event = EVENT_RX0TEND;
               } else
                  mphi_state.emu_event = EVENT_RX0MEND;
            }
            os_semaphore_release(&mphi_state.emu_latch);

            if ( mphi_state.event_callback )
               mphi_state.event_callback( mphi_state.event_callback_data);
         }
      }

      /*
         transmit outstanding write messages
      */

      while (emu_out.read != emu_out.write) {
         EMU_SLOT_T *slot = emu_peek_slot(&emu_out);

         int dummy = 0;

         if (!slot->chan)
            os_assert(*(unsigned short *) slot->addr == 0xAF05);
         hp_write_host(hp, (slot->chan ? HP_CHANNEL_1 : HP_CHANNEL_0) | (slot->term ? HP_TERMINATE_DMA : 0) , slot->len, slot->addr);
         hp_write_host(hp, 0, 0, &dummy);

         os_semaphore_obtain(&mphi_state.emu_latch_handled);
	      os_semaphore_obtain(&mphi_state.emu_latch);
         mphi_state.emu_event_idx = slot->idx;

         mphi_state.emu_event = EVENT_TXEND;

         emu_pop_slot(&emu_out);
         os_semaphore_release(&mphi_state.emu_latch);

         if ( mphi_state.event_callback )
            mphi_state.event_callback( mphi_state.event_callback_data);
      }
   }
}

/* ----------------------------------------------------------------------
 * look at stored copy of mphi interrupt status register, and return
 * the next event that is indicated.
 *
 * re-enable interrupts if no more events.
 *
 * intended to be called from hisr context: keep calling this function
 * until it returns MPHI_EVENT_NONE (at which point, mphi interrupts
 * will be re-enabled, below)
 * -------------------------------------------------------------------- */
static MPHI_EVENT_TYPE_T
mphi_next_event( const DRIVER_HANDLE_T handle, uint8_t *slot_id, uint32_t *pos )
{
   EMU_EVENT_T event;

   os_semaphore_obtain(&mphi_state.emu_latch);
   event = mphi_state.emu_event;

   mphi_state.emu_event = EVENT_NONE;

   // sensible default return values
   *slot_id = 0;
   *pos = 0;

   switch (event) {
   case EVENT_NONE:
      break;
   case EVENT_TXEND:
   {
      *slot_id = (uint8_t)mphi_state.emu_event_idx;
	   os_semaphore_release(&mphi_state.emu_latch);

      os_semaphore_release(&mphi_state.emu_latch_handled);
      return MPHI_EVENT_OUT_DMA_COMPLETED;
   }
   case EVENT_RX0TEND:
   {
      *slot_id = (uint8_t)mphi_state.emu_event_idx;
      *pos = mphi_state.emu_event_pos;
      os_semaphore_release(&mphi_state.emu_latch);

      os_semaphore_release(&mphi_state.emu_latch_handled);
      return MPHI_EVENT_IN_CONTROL_DMA_COMPLETED;
   }
   case EVENT_RX0MEND:
   {
      // message transfer end: as far as the midlayer is concerned,
      // the incoming write pointer has changed

      *slot_id = (uint8_t)mphi_state.emu_event_idx;
      *pos = mphi_state.emu_event_pos;
	   os_semaphore_release(&mphi_state.emu_latch);

      os_semaphore_release(&mphi_state.emu_latch_handled);
      return MPHI_EVENT_IN_CONTROL_POSITION;
   }
   case EVENT_RX1TEND:
   {
      *slot_id = (uint8_t)mphi_state.emu_event_idx;
      *pos = mphi_state.emu_event_pos;
      os_semaphore_release(&mphi_state.emu_latch);

      os_semaphore_release(&mphi_state.emu_latch_handled);
      return MPHI_EVENT_IN_DATA_DMA_COMPLETED;
   }
   default:
      os_assert(0);
      break;
   }

   os_semaphore_release(&mphi_state.emu_latch);
   return MPHI_EVENT_NONE;
}

/* ----------------------------------------------------------------------
 * Once we have primed the DMA FIFOs we can turn on the interrupts.
 * The MPHI does not have a 'transfer enable' so the only thing we can
 * do is enable the interrupts.  This may result in an initial (spurious)
 * discarded data interrupt etc.
 *
 * return 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
mphi_enable( const DRIVER_HANDLE_T handle )
{
   return 0;
}

/* ----------------------------------------------------------------------
 * MPHI peripheral defaults to a fairly low power state (4mA, slew on).
 * Expose an API to speed it up.
 *
 * return 0 on success, non-0 otherwise
 * -------------------------------------------------------------------- */
static int32_t
mphi_set_power( const DRIVER_HANDLE_T handle, int drive_mA, int slew )
{
   return 0;
}

/* ----------------------------------------------------------------------
 * no alignment requirements for Windows/Linux implementation
 * -------------------------------------------------------------------- */
static int
mphi_alignment( const DRIVER_HANDLE_T handle )
{
   return 1;
}

/* ----------------------------------------------------------------------
 * dump debug info
 * -------------------------------------------------------------------- */
static void
mphi_debug( const DRIVER_HANDLE_T handle )
{
   os_logging_message( "mphi_debug:" );

   os_logging_message( "tx used=%d", mphi_channel_used( MPHI_CHANNEL_OUT ) );
   os_logging_message( "rx slots inuse=%d", mphi_channel_used(MPHI_CHANNEL_IN_CONTROL) );
   os_logging_message( "rx bulk slot inuse=%d", mphi_channel_used(MPHI_CHANNEL_IN_DATA) );
}

/********************************** End of file ******************************************/
