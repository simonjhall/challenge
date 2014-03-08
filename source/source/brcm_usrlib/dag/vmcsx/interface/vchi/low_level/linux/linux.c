/**=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VCHI
Module   :  MPHI Linux videocore message driver

FILE DESCRIPTION:

Implements a MPHI layer which uses a Linux driver for the communications.

All slots based systems is implemented in S/W (unlike the VC3 hardware FIFO system)
and simulates VC style interrupts for transmit and receive events.


In the vcfw mphi driver, there are 3 separate handles which can be opened;
two receive (control, bulk) and transmit.

Note that at this stage this is very much work in progress. The polling system used
for monitoring the MSR will be changed for an interrupt driven approach in time.

=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#include "vcinclude/common.h"
#include "vcfw/drivers/chip/mphi.h"
#include "vcfw/rtos/rtos.h"
#include "vcfw/logging/logging.h"
#include "interface/vchi/os/os.h"


/******************************************************************************
  Local typedefs
 *****************************************************************************/

//#define DEBUG_FUNCTION_ENTRY
//#define DEBUG_RX_DATA
//#define DEBUG_NEXT_EVENT

#define MSR_M  0x40000000
#define MSR_N  0x20000000
#define MSR_T  0x10000000
#define MSR_UF 0x08000000
#define MSR_OF 0x04000000
#define MSR_FS 0x02000000

#define MSR_MSG_LENGTH(a) ( ( (  ((uint32_t)a & 0x01ff0000 )>>1) + ( (uint32_t)a & 0x0000ffff) ) + 1)

#define MPR_FORCE_DATA_FLAG            0x01
#define MPR_FORCE_DMA_TERMINATE_FLAG   0x02
#define MPR_HOST_DMA_FLAG              0x04



#define LINUX_MPHI_MAX_HANDLES 6
#define LINUX_SLOTS_PER_CHANNEL 16


#ifdef LINUX_MPHI_TEST

#define CTRL_DRIVER_NAME "/dev/vc3Simp0"
#define DATA_DRIVER_NAME "/dev/vc3Simp1"

#else

#define CTRL_DRIVER_NAME "/dev/vc03p0"
#define DATA_DRIVER_NAME "/dev/vc03p1"

#endif


typedef enum
{
   INTSTAT_RX0MEND = 0x01,
   INTSTAT_RX0TEND = 0x02,
   INTSTAT_RX1MEND = 0x04,
   INTSTAT_RX1TEND = 0x08,
   INTSTAT_TXTEND = 0x10,
} INTERRUPT_STATE_BITS_T;
   



/// The particular states that a slot can be in.
/// Some are for RX slots, some for TX slots and some for both
typedef enum
{
   SLOT_STATE_EMPTY = 0,             /// rx slot curently empty and unused  
   SLOT_STATE_IN_USE,                /// rx slot can be used  
   SLOT_STATE_COMPLETED,             /// rx slot is full and ready for transfer during next_event  
   SLOT_STATE_TX_CONTROL,            /// used to flag whether a slot contains control message
   SLOT_STATE_TX_DATA,               /// or data message. Also means the slot is ready to be sent
   SLOT_STATE_TX_CONTROL_DMA_TERM,   /// As previous two, but also force a DMA terminate to be set
   SLOT_STATE_TX_DATA_DMA_TERM,      /// " 
   SLOT_STATE_TX_SENT                /// Slot has been sent out.
} SLOT_STATE_T;


/// A slot entry
typedef struct
{
   uint8_t        id;               /// id assigned to the slot
   void           *addr;            /// ptr to data buffer for the slot
   int            len;              /// length in bytes of data buffer
   int            write_idx;        /// Current index to write position in buffer
   SLOT_STATE_T   state;            /// state of the slot
} LINUX_SLOT_T;


// A channel entry
typedef struct
{
   OS_SEMAPHORE_T  latch;                                /// Semaphore to protect access
   int             opened;                               /// flag 1 if open, 0 if closed 
   MPHI_CHANNEL_T  type;                                 /// Type fo the channel (e.g. in or out)   

   // slot info
   LINUX_SLOT_T    slot[ LINUX_SLOTS_PER_CHANNEL ];      /// FIFO of slots in the channel
   int             slot_start, slot_end, slot_entries;   /// FIFO maintenance vars.
} LINUX_MPHI_CHANNEL_T;

/// Structure containing all data for the module/driver
typedef struct
{
   int               ref_count;                          /// Incremented each time a channel is opened. 

   OS_SEMAPHORE_T    latch;                              /// A protection semaphore

   int               ctrl_handle;                        /// Linux handle to control channel   
   int               data_handle;                        /// linux handle to data channel   

   LINUX_MPHI_CHANNEL_T   channel[ LINUX_MPHI_MAX_HANDLES ];  /// All possible channels

   MPHI_EVENT_CALLBACK_T  *event_callback;               /// Callback assigned when channels opened. Only one callback for all channels
   const void*       callback_data;                      /// Data to be sent back in the callback  
   
   uint8_t           waiting_transmit;                   /// The number of slots in the output channel waiting to be transmitted

   OS_EVENTGROUP_T   event;                              /// handles to the event used to fire off HISR on receipt of an 'interrupt'
   uint32_t          interrupt_status;                   /// Local copy of the 'interrupt' register
 
} LINUX_MPHI_STATE_T;



/******************************************************************************
 Static functions forward declarations
 *****************************************************************************/

static int32_t             linux_mphi_init( void );
static int32_t             linux_mphi_exit( void );
static int32_t             linux_mphi_info( const char **driver_name,
                                            uint32_t *version_major,
                                            uint32_t *version_minor,
                                            DRIVER_FLAGS_T *flags );
static int32_t             linux_mphi_open( const MPHI_OPEN_T *params,
                                            DRIVER_HANDLE_T *handle );
static int32_t             linux_mphi_close( const DRIVER_HANDLE_T handle );
static int32_t             linux_mphi_add_recv_slot( const DRIVER_HANDLE_T handle, const void *addr, uint32_t len, uint8_t slot_id );
static int32_t             linux_mphi_out_queue_message( const DRIVER_HANDLE_T handle, uint8_t control, const void *addr, size_t len, uint8_t msg_id, const MPHI_FLAGS_T flags );
static int                 linux_mphi_slots_available( DRIVER_HANDLE_T handle );
static MPHI_EVENT_TYPE_T   linux_mphi_next_event( const DRIVER_HANDLE_T handle, uint8_t *slot_id, uint32_t *pos );
static int32_t             linux_mphi_enable( const DRIVER_HANDLE_T handle );
static int32_t             linux_mphi_set_power( const DRIVER_HANDLE_T handle, int drive_mA, int slew );
static void                linux_mphi_debug( const DRIVER_HANDLE_T handle );


static void                linux_hisr_interrupt( unsigned a, void *b);


/******************************************************************************
 Static data
 *****************************************************************************/

//The driver function table
static const MPHI_DRIVER_T linux_mphi_func_table =
{
   // common driver api
   &linux_mphi_init,
   &linux_mphi_exit,
   &linux_mphi_info,
   &linux_mphi_open,
   &linux_mphi_close,

   // linux_mphi-specific api
   &linux_mphi_add_recv_slot,
   &linux_mphi_out_queue_message,
   &linux_mphi_slots_available,
   &linux_mphi_next_event,
   &linux_mphi_enable,
   &linux_mphi_set_power,
   &linux_mphi_debug
};

///  Static state structure
static LINUX_MPHI_STATE_T linux_mphi_state = {0};

//the automatic registering of the driver - DO NOT REMOVE! 
// Actually, only needed on VC side of I/F. This is host side.
//#pragma Data(DATA, ".drivers") //$linux_mphi
static const MPHI_DRIVER_T *linux_mphi_func_table_ptr = &linux_mphi_func_table;
//#pragma Data()


static uint32_t interrupt_state;



/******************************************************************************
 Global Functions
 *****************************************************************************/

/**
 * Global function to return a ptr to the mphi driver function table
 *
 * @return Pointer to a mphi function table
 */
const MPHI_DRIVER_T *linux_mphi_get_func_table( void )
{
#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something asked for function table...\n");   
#endif

   return linux_mphi_func_table_ptr;
}


/******************************************************************************
 Static  Functions
 *****************************************************************************/

/*
 *  Adds the specified bit to the 'interrupt' flag, and sets off the HISR
 *
 */
static void simulate_interrupt(int type)
{
   interrupt_state |= type;  
    
   os_eventgroup_signal ( &linux_mphi_state.event, 0);
}


/**
 * Function to send the specified setup data to the message parameter register
 *
 * @param val Value of data to send to the output setup port
 */
 static void setup_out_port( uint16_t val)
{
    write(linux_mphi_state.ctrl_handle, &val, sizeof(val));
}


/**
 * Function to send the specifed buffer of data to the VC data port
 *
 * @param addr  start address to read data to be output
 * @param len   length of the data block in bytes
 * @return number of bytes written to the port
 */
static int copy_data_to_port(void *addr, int32_t len_bytes)
{
   int bytes_written = write(linux_mphi_state.data_handle, addr, len_bytes );
   
   return bytes_written;
}

/**
 * Function to copy data from VC to the specifed buffer port
 *
 * @param addr  start address to read data to be output
 * @param len   length of the data block in bytes
 * @return number of bytes read from the port
 */
static int copy_data_from_port(void *addr, int32_t len_bytes)
{
   int bytes_read = read(linux_mphi_state.data_handle, addr, len_bytes );

#ifdef DEBUG_RX_DATA
int i;   

os_logging_message("Port Read : ");   
for (i = 0;i<bytes_read;i++)
{
   if (i%16 == 0) 
      os_logging_message("\n");   

   os_logging_message("%02x ", ((uint8_t*)addr)[i]);
}
os_logging_message("\n");   
#endif
   
   return bytes_read;
}

/**
 * Function to read from VC host address via Linux file descriptor
 *
 * @param add  Flag to determine data or control channel. 0 = control, 1 = data address
 */
static int16_t read_address(int add)
{
   int16_t val;
   
   if (add)
   {
      read(linux_mphi_state.data_handle, &val, sizeof(val) );
   }
   else
   {
      read( linux_mphi_state.ctrl_handle, &val, sizeof(val) );
   }      

   return val;   
}

/**
 * Function to read the 32bit message status register
 *
 * There is a special case here - if the host sees the M flag 
 * transition from 0 to 1, then we need to read a second time, 
 * just to ensure the MSR is read correctly - its a timing 
 * things with the VCHI
 * 
 * @return 32 bit value of the Message status register from VC
 */
 
 static int32_t read_msr()
{
   // stores last know state of M bit in MSR
   static int8_t last_M = 0;

   uint32_t res, M_flag;
   
   while (1)
   {
      uint32_t a = 0xffff, b;
   
      // We must read until top bit is zero to ensure this is the first read of the cycle.
      while (a & 0x8000)
         a = read_address(0);
         
      // now get the second part of the data   
      b = read_address(0);
   
      res = (b << 16) + a;
      
      M_flag = res & MSR_M;

      // We can drop out straight away if M is zero or 
      // transition was not 0-1
      if (!M_flag || (M_flag && last_M)  )
      {
         last_M = M_flag ? 1 : 0;
         break;
      }

      last_M = M_flag ? 1 : 0;
   }
   
   return res;
}


/* ----------------------------------------------------------------------
 * Thread which loops and polls the VCiii message status reg.
 *
 * It then gets the data and will fake an interrupt kick off the standard
 * event handling mechanism
 *
 * Loop also acts a bit like a DMA thingy, checking to see if data is ready to be output
 * and sending it to the port if there is.
 *
 * Eventually this loop will be fired by the HAT interrupt, negating the need to poll
 * ---------------------------------------------------------------------- */

static void message_status_monitor(unsigned a, void *b)
{
   static int total_sent = 0 ;
   
   
   while (1)
   {
      uint32_t msr = read_msr();

      // Check for M bit set, which will kick off the message rxed system
      if (msr & MSR_M)
      {
         INTERRUPT_STATE_BITS_T rxmend, rxtend;
         
         uint32_t data_len = MSR_MSG_LENGTH(msr);
         
         // OK, there is message data waiting, find out what sort
         int bulk = msr & MSR_T;
        
         LINUX_MPHI_CHANNEL_T *pchannel;
         
         // Determine which channel data is for, and also which 'interrupts' will be fired
         if (bulk)
         {
            // OK, its bulk data on MPHI_CHANNEL_IN_DATA
            pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_DATA];
            rxmend = INTSTAT_RX1MEND;
            rxtend = INTSTAT_RX1TEND;
         }
         else
         {
            pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_CONTROL];
            rxmend = INTSTAT_RX0MEND;
            rxtend = INTSTAT_RX0TEND;
        }
         
         // Loop out input to slots until no more data or slots run out (error)
         // We dont allow data from a message to span multiple slots.
         // However, we can TXEND any slots in the FIFO that are too small for this
         // new data. 
         

         while (data_len)
         {
            os_semaphore_obtain( &pchannel->latch );

            // Find slot in-use, or if not, find the first empty
            int slot_id = pchannel->slot_start;
            int in_use = -1, empty = -1;
            int i;
              
            for (i=0;i<pchannel->slot_entries;i++)
            {
               if (in_use == -1 && pchannel->slot[slot_id].state == SLOT_STATE_IN_USE)
                  in_use = slot_id;
   
               if (empty == -1 && pchannel->slot[slot_id].state == SLOT_STATE_EMPTY)
                  empty = slot_id;
   
               // and update our slot ptr
               if ( ++slot_id == LINUX_SLOTS_PER_CHANNEL )
                     slot_id = 0;
            }
            
            if (in_use != -1) 
               slot_id = in_use;
            else if (empty != -1)
               slot_id = empty;
            else
            {
               // No in use or empty slots. This is bad
               os_assert(0);
               break;
            }   
   
            LINUX_SLOT_T *pslot = &pchannel->slot[slot_id];

            // This slot is defo in use now
            pslot->state = SLOT_STATE_IN_USE;
            
            // Find out how much space left in the slot
            int left = pslot->len - pslot->write_idx;
            
            // Layers above should not let this happen
            os_assert(left > 0);
            
            // If the data len is less than the amount of space left
            // then we flag a transmit end interrupt, then go on to the next slot
            
            int32_t interrupt_required = 0;
            
            if (data_len > left)
           	{
           	   // Flag as DMA completed. Note that this will invoke a next_event call
           	   // but there will be no message data. According to SL, passing back the write_ptr
           	   // as same value as previously will mean that the event will be ignored.
           	   // Since the write_ptr has not been incremented, it should indeed be the same 
           	   // as the last event call.
           	   
//os_logging_message("Slot is now full - issueing TEND\n");           	   
           	   
               pslot->state = SLOT_STATE_COMPLETED;
               interrupt_required = rxtend;
          	}	
				else
				{
               // copy the data to our available slot, then fire our simulated interrupt 
               uint8_t *addr = ((uint8_t*)pslot->addr) + pslot->write_idx;

               int read = copy_data_from_port(addr, data_len);
               
               // update write_ptr to reflect new data
               pslot->write_idx += data_len;
               left -= data_len;

               // And update the data_len - this flags our end of transmit
               data_len = 0;
               
               // Determine how much space is now left in the slot, if none
               // ie No space for any futher messages use TEND, otherwise MEND
               if (left == 0 )
               {
                  interrupt_required = rxtend;          
                  // Update the slot state if it is now full, or transmission is over
                  pslot->state = SLOT_STATE_COMPLETED;
               }
               else
               {
                  interrupt_required = rxmend;             
                  // no change to state - still in use
               }
            }       
            
            // release latch in case our simualte interrupt calls back and needs it.
            os_semaphore_release( &pchannel->latch );

            if (interrupt_required)
            {
               simulate_interrupt(interrupt_required);
            }
         }
      }


      // OK, now MSR has been handled, see if we have any data to output
      // This outputs ALL waiting data, over all waiting slots
      if (linux_mphi_state.waiting_transmit)
      {
         // Find the next tx to go. Search slots until find one
         LINUX_MPHI_CHANNEL_T *pchannel;
         
         pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_OUT];

         os_semaphore_obtain( &pchannel->latch );

         LINUX_SLOT_T *slot;
         int slot_id;

         while (linux_mphi_state.waiting_transmit)
         {
            int i;
            slot = NULL;
            slot_id = pchannel->slot_start;
            
            for (i=0;i<pchannel->slot_entries;i++)
            {
               if (pchannel->slot[slot_id].state == SLOT_STATE_TX_CONTROL ||
                   pchannel->slot[slot_id].state == SLOT_STATE_TX_DATA || 
                   pchannel->slot[slot_id].state == SLOT_STATE_TX_CONTROL_DMA_TERM ||
                   pchannel->slot[slot_id].state == SLOT_STATE_TX_DATA_DMA_TERM)
               {
                  slot = &(pchannel->slot[slot_id]);
                  break;
               }
   
               // and update our end ptr
               if ( ++slot_id >= LINUX_SLOTS_PER_CHANNEL )
                     slot_id = 0;
            }
            
            // must have been one at least.
            os_assert(slot);

            // Setting up port also finishes off the previous message send
            switch (slot->state)
            {
               case SLOT_STATE_TX_DATA :
                  setup_out_port( MPR_FORCE_DATA_FLAG)  ; 
                  break;

               case SLOT_STATE_TX_DATA_DMA_TERM :
             
                  os_assert(0);//should never occur

                  setup_out_port( MPR_FORCE_DMA_TERMINATE_FLAG |  MPR_FORCE_DATA_FLAG)  ; 
                  break;
                  
               case SLOT_STATE_TX_CONTROL:
                  setup_out_port( 0)  ;
                  break;

               case SLOT_STATE_TX_CONTROL_DMA_TERM:
               {
                  uint8_t term[16];

// offsets for the message format
#define SYNC_OFFSET             0
#define LENGTH_OFFSET           2
#define FOUR_CC_OFFSET          4
#define TIMESTAMP_OFFSET        8
#define SLOT_COUNT_OFFSET      12
#define PAYLOAD_LENGTH_OFFSET  14
#define PAYLOAD_ADDRESS_OFFSET 16

#define MAKE_FOURCC(x) ( (x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3] )

                  vchi_writebuf_uint16( term + SYNC_OFFSET,    0xaf05 );
                  vchi_writebuf_uint16( term + LENGTH_OFFSET,  16 );
                  vchi_writebuf_fourcc( term + FOUR_CC_OFFSET, MAKE_FOURCC("CTRL") );
                  vchi_writebuf_uint32( term + TIMESTAMP_OFFSET,  0  );
                  vchi_writebuf_uint16( term + SLOT_COUNT_OFFSET, 0 );
                  vchi_writebuf_uint16( term + PAYLOAD_LENGTH_OFFSET, 0 );

                  //os_logging_message("outputing DMA terminate flag on control\n");
                  setup_out_port( MPR_FORCE_DMA_TERMINATE_FLAG);
                  copy_data_to_port(term, 16);
                  setup_out_port( 0)  ;
                  break;
               }
            }

//total_sent += slot->len;

//os_logging_message("Outputing data to port from slot %d, total sent= %d\n", slot_id, total_sent);
               
            copy_data_to_port(slot->addr, slot->len);
            
            slot->state = SLOT_STATE_TX_SENT;
            
            linux_mphi_state.waiting_transmit--;
         }
         
         setup_out_port(0); // This completes final xfer 

         os_semaphore_release( &pchannel->latch );

         // OK, make an interrupt happen which simulates this completion
         simulate_interrupt(INTSTAT_TXTEND);
      
      } 
      
      // Dont want to hog the processor. One loop per millisec should be ok for testing
      os_delay(1);
   }
}

/**
 * Initialises the linux_mphi driver
 * this is part of the common driver api
 *
 * @return 0 on success; all other values are failures
 *
 */
static int32_t linux_mphi_init( void )
{
   OS_THREAD_T ti;
   
#ifdef DEBUG_FUNCTION_ENTRY   
os_logging_message("Something called linux_mphi_init...\n");   
#endif   
   
   // just in case
   memset( &linux_mphi_state, 0, sizeof(linux_mphi_state) );
   
   // Create latch to protect the state structure
   os_semaphore_create(&linux_mphi_state.latch, OS_SEMAPHORE_TYPE_SUSPEND);

   // Create event for firing off hisr
   os_eventgroup_create(&linux_mphi_state.event, "linux_mphi1");

   // Open the linux kernel driver
   linux_mphi_state.ctrl_handle  = open( CTRL_DRIVER_NAME, O_RDWR );
   linux_mphi_state.data_handle  = open( DATA_DRIVER_NAME, O_RDWR );

   os_assert(linux_mphi_state.ctrl_handle > 0);
   os_assert(linux_mphi_state.data_handle > 0);
   
   // start up the HISR
   os_thread_start(&ti, linux_hisr_interrupt, NULL, 1024, NULL);

   return 0;
}

/**
 * exit the linux_mphi driver
 * this is part of the common driver api
 *
 * @return 0 on success; all other values are failures 
 */
static int32_t linux_mphi_exit( void )
{
#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something called linux_mphi_exit...\n");   
#endif

   if (linux_mphi_state.latch)
   {
      os_semaphore_destroy(&linux_mphi_state.latch);
   }
   
   if (linux_mphi_state.data_handle != -1)
   {
      close(&linux_mphi_state.data_handle);
   }
      
   if (linux_mphi_state.ctrl_handle != -1)
   {
      close(&linux_mphi_state.ctrl_handle);
   }
   
   if (linux_mphi_state.event)
   {
      os_eventgroup_destroy(&linux_mphi_state.event);
   }
      
   return 0;
}

/**
 * Get the linux_mphi driver info
 * this is part of the common driver api
 *
 * @param[out] driver_name On suuccessful return contains name of the driver
 * @param[out] version_major On suuccessful return contains major part of version number
 * @param[out] version_minor On suuccessful return contains minor part of version number
 * @param[out] flags On suuccessful return contains flags used to start driver. 
 * @return 0 on success; all other values are failures
 */
static int32_t linux_mphi_info( const char **driver_name,
                                 uint32_t *version_major,
                                 uint32_t *version_minor,
                                 DRIVER_FLAGS_T *flags )
{
   int32_t success = -1; // fail by default

#ifdef DEBUG_FUNCTION_ENTRY   
os_logging_message("Something called linux_mphi_info...\n");   
#endif

   // return the driver name
   if ( driver_name && version_major && version_minor && flags )
   {
      *driver_name = "linux_mphi";
      *version_major = 0;
      *version_minor = 1;
      *flags = 0;

      // success!
      success = 0;
   }

   return success;
}

/**
 * open the driver and obtain a handle
 * this is part of the common driver api
 * 
 * @params[in] ptr to paramter block used to set up driver
 * @params[out] On success, Returns handle to the driver opened
 * @return 0 on success; all other values are failures
 */
static int32_t linux_mphi_open( const MPHI_OPEN_T *params, DRIVER_HANDLE_T *handle )
{
#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something called linux_mphi_open...\n");
#endif
   
   os_delay(100);   

   int32_t success = 0;
   
   // check params
   if ( !params )
      return -1;
      
   MPHI_CHANNEL_T channel_type = params->channel;
   
   if ( channel_type != MPHI_CHANNEL_OUT && channel_type != MPHI_CHANNEL_IN_CONTROL && channel_type != MPHI_CHANNEL_IN_DATA )
      return -1;

   os_semaphore_obtain( &linux_mphi_state.latch );

   linux_mphi_state.ref_count++;

   LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[channel_type];
   
   // See if this channel has already been opened
   if (pchannel->opened)
   {
      os_assert(0);
      success = -1;
   }
   else
   {
      // mark this entry as in use
      pchannel->opened = 1;
      pchannel->type = channel_type;
      pchannel->slot_entries = pchannel->slot_start = pchannel->slot_end = 0; // no slots in queue to begin
      
      os_semaphore_create(&pchannel->latch, OS_SEMAPHORE_TYPE_SUSPEND);
      
      // set callback for this handle. Check to ensure any passed in value is same as any previous one
      if ( linux_mphi_state.event_callback)
         os_assert( linux_mphi_state.event_callback == params->callback ); // can have only one
      else
      {
         linux_mphi_state.event_callback = params->callback;
         linux_mphi_state.callback_data = params->callback_data;
      }
      
   
      // finally fill in return values
      *handle = (DRIVER_HANDLE_T) pchannel;
   }
   
   // release the latch
   os_semaphore_release( &linux_mphi_state.latch );

   return success;
}

/**
 * close a handle to the driver
 * this is part of the common driver api
 *
 * @param Handle to driver to close
 * @return 0 on success; all other values are failures
 **/
static int32_t linux_mphi_close( const DRIVER_HANDLE_T handle )
{
   LINUX_MPHI_CHANNEL_T *channel = (LINUX_MPHI_CHANNEL_T *) handle;

#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something called linux_mphi_close...\n");   
#endif

   os_semaphore_obtain( &linux_mphi_state.latch );

   // Check that the channel specified is actually open
   os_assert(channel->opened);
   
   // Destroy the channel semaphore
   os_semaphore_destroy(&channel->latch);
     
   // and close it...   
   channel->opened = 0;

   os_semaphore_release( &linux_mphi_state.latch );
   
   return 0;
}



/**
 * Adds a new slot to the receive FIFO
 *
 * @param handle handle to channel to add slot to
 * @param addr address of slot buffer
 * @param len length in bytes of buffer
 * @param slot_id slot identifier to be assigned to slot.
 *
 * @return non-zero if failed to add the slot. MPHI_ERROR_FIFO_FULL if no available FIFO's 
 */
static int32_t linux_mphi_add_recv_slot( const DRIVER_HANDLE_T handle, const void *addr, uint32_t len, uint8_t slot_id )
{
#ifdef DEBUG_FUNCTION_ENTRY   
  os_logging_message("Something called linux_mphi_add_recv_slot...\n");   
#endif
   
   LINUX_MPHI_CHANNEL_T *channel = (LINUX_MPHI_CHANNEL_T *) handle;

   os_semaphore_obtain( &channel->latch);

   // Check we are an input channel of some description
   if ( channel->type != MPHI_CHANNEL_IN_CONTROL && channel->type != MPHI_CHANNEL_IN_DATA )
   {
      os_semaphore_release( &channel->latch );
      return -1;
   }

   // See if we have any free slots
   if ( channel->slot_entries == LINUX_SLOTS_PER_CHANNEL )
   {
      os_semaphore_release( &channel->latch );
      return MPHI_ERROR_FIFO_FULL;
   }

   // add our slot to the end of the slot FIFO
   LINUX_SLOT_T *slot = &channel->slot[ channel->slot_end ];
   
   slot->id   = slot_id;
   slot->addr = (void *)addr;
   slot->len  = len;
   slot->write_idx = 0;
   slot->state = SLOT_STATE_EMPTY;

   // and update our end ptr
   if ( ++(channel->slot_end) >= LINUX_SLOTS_PER_CHANNEL )
       channel->slot_end = 0;
   
   channel->slot_entries++;

   os_semaphore_release( &channel->latch );
   
   return 0;
}

/**
 * Function to queue up a mesage ready for sending
 *
 * @param handle    Handle to driver channel to use
 * @param control   0 if data is a bulk message, non-0 if its a control message
 * @param addr      Address of the data to send
 * @param len       length of data to send
 * @param slot_id   slot id to be assigned to the data being sent
 *
 * @return non-zero if failed
         -3 if not an output channel
         -4 if incoming data not on a 16 byte boundary
         -5 if data length not padded to 16 byte boundary
         MPHI_ERROR_FIFO_FULL if no available FIFO entry
 **/
static int32_t linux_mphi_out_queue_message( const DRIVER_HANDLE_T handle, uint8_t control, const void *addr, size_t len, uint8_t slot_id, const MPHI_FLAGS_T flags )
{
   // make sure we're trying to send on the correct handle
   LINUX_MPHI_CHANNEL_T *channel = (LINUX_MPHI_CHANNEL_T *) handle;

#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something called linux_queue_message...addr = %p, len = %d, slot = %d flag = %d\n", addr, len, slot_id, flags);   
#endif

   // Cull out some bad params   
   if ( channel->type != MPHI_CHANNEL_OUT )
   {
      os_assert(0); // this would be bad
      return -3;
   }

   // Ensure 16 byte boundaries. Probably not necessary for Linux - check this.
   
   // Nope, not necesary on host side
   //if ( (uintptr_t)addr & 0xf )
   //    return -4;
       
   // Check for length padded to 16. Need to do this as required by vc     
   if ( (len & 0xf) || len == 0 || len > 0x1000000 )
       return -5;

   // Get protection
   os_semaphore_obtain( &channel->latch );

   // How many tx's slots are left
   os_assert( channel->slot_entries <= LINUX_SLOTS_PER_CHANNEL );

   if ( channel->slot_entries == LINUX_SLOTS_PER_CHANNEL )
   {
      os_logging_message( "mphi_out_queue_message: no free space" );
      os_semaphore_release( &channel->latch  );
      
      return MPHI_ERROR_FIFO_FULL;
   }

   // Add to slots
   LINUX_SLOT_T *slot = &channel->slot[channel->slot_end];
   
//os_logging_message("linux_queue_message...addr = %p, len = %d, slot = %d, phys slot = %d\n", addr, len, slot_id, channel->slot_end);   
   
   slot->id = slot_id;
   slot->addr = (void *)addr;
   slot->len = len;
   slot->write_idx = 0;
   
   if (flags & MPHI_FLAGS_TERMINATE_DMA)
   {
//os_logging_message("DMA terminate requried\n");   
      slot->state = control ? SLOT_STATE_TX_CONTROL_DMA_TERM : SLOT_STATE_TX_DATA_DMA_TERM ;
   }
   else
   {
      slot->state = control ? SLOT_STATE_TX_CONTROL : SLOT_STATE_TX_DATA;
   }   

   if ( ++(channel->slot_end) >= LINUX_SLOTS_PER_CHANNEL )
       channel->slot_end = 0;

   // We have added another slot to the output, ready to tx
   channel->slot_entries++;

   os_semaphore_release( &channel->latch );


   // ANd the overall state has a flag for number to transmit which needs incrementing
   os_semaphore_obtain( &linux_mphi_state.latch );
   linux_mphi_state.waiting_transmit++;
   os_semaphore_release( &linux_mphi_state.latch );

   return 0;
}

/**
 * Determine many free slots are there on this channel
 *
 * @param handle handle to channel to check
 *
 * @return The number of free FIFO slots on the specified channel
 **/
static int linux_mphi_slots_available( DRIVER_HANDLE_T handle )
{
   LINUX_MPHI_CHANNEL_T *channel = (LINUX_MPHI_CHANNEL_T *) handle;

#ifdef DEBUG_FUNCTION_ENTRY   
   os_logging_message("Something called linux_slots_available...\n");   
#endif
   
   if (handle)
   {
      os_semaphore_obtain( &channel->latch );
      int res = LINUX_SLOTS_PER_CHANNEL - channel->slot_entries;
      os_semaphore_release( &channel->latch );
      
      return res;
   }
   
   return -1;  // what should this return if it goes wrong?
}

/**
 * Function to return the next event from the driver.
 * Intended to be called from hisr context: keep calling this function
 * until it returns MPHI_EVENT_NONE 
 *
 * @param handle        handle to channel to check for an event. Ignored.
 * @param[out] slot_id  slot ID of the event.
 * @param[out] pos      position in slot buffer of the event
 *
 * @return The type of the event found
 **/
static MPHI_EVENT_TYPE_T linux_mphi_next_event( const DRIVER_HANDLE_T handle, uint8_t *slot_id, uint32_t *pos )
{
   static int last_returned_slot_id = -1;
   static int last_returned_pos = -1;
   
    
   // sensible default return values
   *slot_id = 0;
   *pos = 0;

   // transmit section
   
   if ( linux_mphi_state.interrupt_status & INTSTAT_TXTEND )
   {
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_OUT];

      os_semaphore_obtain( &pchannel->latch );

//os_logging_message("next_event transmit slot data before: start=%d, end=%d size=%d\n", pchannel->slot_start, pchannel->slot_end, pchannel->slot_entries);

      // Check that the first item is in the sent state (which caused the interrupt)      
      if (pchannel->slot_entries == 0 || pchannel->slot[pchannel->slot_start].state != SLOT_STATE_TX_SENT)
      {
         // No more completed data, so clear interrupt.
         linux_mphi_state.interrupt_status &= ~INTSTAT_TXTEND;
         os_semaphore_release( &pchannel->latch  );
      }
      else
      {
         // OK we have a completed slot, so send that data back to the layer calling us
         *slot_id = pchannel->slot[pchannel->slot_start].id;
         
         // Get rid of this slot from the FIFO
         if ( ++(pchannel->slot_start) >= LINUX_SLOTS_PER_CHANNEL )
            pchannel->slot_start = 0;
            
         pchannel->slot_entries--;

//os_logging_message("next_event transmit slot data after: start=%d, end=%d size=%d, virt = %d\n\n", pchannel->slot_start, pchannel->slot_end, pchannel->slot_entries, *slot_id);

         os_semaphore_release( &pchannel->latch  );

#ifdef DEBUG_NEXT_EVENT
         os_logging_message("Something called linux_mphi_next_event...OUT_DMA_COMPLETE slot = %d\n", *slot_id);   
#endif       
         return MPHI_EVENT_OUT_DMA_COMPLETED;
      }
   }


   // Receive control message section

   // Check for transmission end on control channel
   if ( linux_mphi_state.interrupt_status & INTSTAT_RX0TEND )
   {
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_CONTROL];

      os_semaphore_obtain( &pchannel->latch );

      if (pchannel->slot_entries == 0 || pchannel->slot[pchannel->slot_start].state != SLOT_STATE_COMPLETED )
      {
         // We have no completed slots, so clear interrupt
         // Dont think this should happen but check anyway
         linux_mphi_state.interrupt_status &= ~INTSTAT_RX0TEND;

         os_semaphore_release( &pchannel->latch  );
      }
      else
      {
         // OK we have a completed slot, so send that data back to the layer calling us
         *slot_id = last_returned_slot_id = pchannel->slot[pchannel->slot_start].id;
         *pos = last_returned_pos = pchannel->slot[pchannel->slot_start].write_idx;
         
         // Get rid of this slot from the FIFO
         if ( ++(pchannel->slot_start) >= LINUX_SLOTS_PER_CHANNEL )
            pchannel->slot_start = 0;
            
         pchannel->slot_entries--;
         
         os_semaphore_release( &pchannel->latch  );

#ifdef DEBUG_NEXT_EVENT
os_logging_message("Something called linux_mphi_next_event...IN_DMA_COMPLETE\n");   
#endif
         return MPHI_EVENT_IN_DMA_COMPLETED;
      }
   }      


   // Check for message end on control channel
   if ( linux_mphi_state.interrupt_status & INTSTAT_RX0MEND )
   {
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_CONTROL];

      os_semaphore_obtain( &pchannel->latch );

      // We also drop out here if we have already sent this data
      if ( pchannel->slot_entries == 0 || 
           pchannel->slot[pchannel->slot_start].state != SLOT_STATE_IN_USE ||
          (pchannel->slot[pchannel->slot_start].id == last_returned_slot_id && 
           pchannel->slot[pchannel->slot_start].write_idx == last_returned_pos ))
      {
         // We have no in use slots, so clear interrupt
         linux_mphi_state.interrupt_status &= ~INTSTAT_RX0MEND;

         os_semaphore_release( &pchannel->latch  );
      }
      else
      {
         // OK we have a partially completed slot, so send that data back to the layer calling us
         // Keep the slot in the FIFO
         *slot_id = last_returned_slot_id = pchannel->slot[pchannel->slot_start].id;
         *pos = last_returned_pos = pchannel->slot[pchannel->slot_start].write_idx;
         
         os_semaphore_release( &pchannel->latch  );

#ifdef DEBUG_NEXT_EVENT
os_logging_message("Something called linux_mphi_next_event...IN_POSITION slot_id = %d, write_ptr = %d\n", *slot_id, *pos);   
#endif         
         return MPHI_EVENT_IN_POSITION;
      }
   }      

   // Check for transmission end on data channel
   if ( linux_mphi_state.interrupt_status & INTSTAT_RX1TEND )
   {
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_DATA];

      os_semaphore_obtain( &pchannel->latch );

      if (pchannel->slot_entries == 0 || pchannel->slot[pchannel->slot_start].state != SLOT_STATE_COMPLETED)
      {
         // We have no completed slots, so clear interrupt
         // Dont think this should happen but check anyway
         linux_mphi_state.interrupt_status &= ~INTSTAT_RX1TEND;

         os_semaphore_release( &pchannel->latch  );
      }
      else
      {
         // OK we have a completed slot, so send that data back to the layer calling us
         *slot_id = last_returned_slot_id = pchannel->slot[pchannel->slot_start].id;
         *pos = last_returned_pos = pchannel->slot[pchannel->slot_start].write_idx;
         
         // Get rid of this slot from the FIFO
         if ( ++(pchannel->slot_start) >= LINUX_SLOTS_PER_CHANNEL )
            pchannel->slot_start = 0;
            
         pchannel->slot_entries--;
         
         os_semaphore_release( &pchannel->latch  );

#ifdef DEBUG_NEXT_EVENT
os_logging_message("Something called linux_mphi_next_event...IN_DATA_DMA_COMPLETE slot_id = %d, write_ptr = %d\n", *slot_id, *pos);  
#endif         
         return MPHI_EVENT_IN_DATA_DMA_COMPLETED;
      }
   }      


   // Check for message end on data channel
   if ( linux_mphi_state.interrupt_status & INTSTAT_RX1MEND )
   {
      // Check the receive slots and return back first in use
      
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[MPHI_CHANNEL_IN_DATA];

//os_logging_message("next_event : obtain sem (e)\n");

      os_semaphore_obtain( &pchannel->latch );

      if (pchannel->slot_entries == 0 || 
           pchannel->slot[pchannel->slot_start].state != SLOT_STATE_IN_USE ||
          (pchannel->slot[pchannel->slot_start].id == last_returned_slot_id && 
           pchannel->slot[pchannel->slot_start].write_idx == last_returned_pos ))
      {
         // First in FIFO is not an in use slot, so clear interrupt
         // Dont think this should happen but check anyway
         linux_mphi_state.interrupt_status &= ~INTSTAT_RX1MEND;

         os_semaphore_release( &pchannel->latch  );
      }
      else
      {
         // OK we have a partially completed slot, so send that data back to the layer calling us
         // Keep the slot in the FIFO
         *slot_id = last_returned_slot_id = pchannel->slot[pchannel->slot_start].id;
         *pos = last_returned_pos = pchannel->slot[pchannel->slot_start].write_idx;
         
         os_semaphore_release( &pchannel->latch  );
         
#ifdef DEBUG_NEXT_EVENT
os_logging_message("Something called linux_mphi_next_event...IN_DATA_IN_POSITION slot_id = %d, write_ptr = %d\n", *slot_id, *pos);    
#endif
         return MPHI_EVENT_IN_DATA_IN_POSITION;
      }
   }      

   return MPHI_EVENT_NONE; // no more messages
}

/**
 * Enables the driver
 * Just starts up the monitor thread 
 */
static int32_t linux_mphi_enable( const DRIVER_HANDLE_T handle )
{
   static int8_t started = 0;
   
#ifdef DEBUG_FUNCTION_ENTRY
   os_logging_message("Something called linux_mphi_enable...\n");   
#endif
   
   if (!started)
   {
      OS_THREAD_T ti;

      // Start up the status register monitor thread
      os_thread_start(&ti, message_status_monitor, NULL, 2048, NULL);
      started = 1; 
   }

   return 0;
}

/**
 * Sets the power status of the driver
 * no action for Linux implementation
 **/
static int32_t linux_mphi_set_power( const DRIVER_HANDLE_T handle, int drive_mA, int slew )
{
#ifdef DEBUG_FUNCTION_ENTRY
   os_logging_message("Something called linux_mphi_set_power...\n");   
#endif
   return 0;
}

/**
 * Dumps debug info to os_logging_message
 *
 * @param handle Handle to channel of data to dump
 **/
static void linux_mphi_debug( const DRIVER_HANDLE_T handle )
{
   MPHI_CHANNEL_T channels[] = {MPHI_CHANNEL_OUT, MPHI_CHANNEL_IN_CONTROL, MPHI_CHANNEL_IN_DATA};
   char *channel_name[]      = {"OUT",            "CONTROL IN",            "DATA IN"};
   
   char *slot_state[] = {"empty", "RX - In use", "RX - completed (full)", "TX - Control message", "TX - Data message", "TX - Sent"};
   
   int i;
   
   os_logging_message( "Linux_mphi_debug:" );
   
   os_logging_message("state : waiting_transmit = %d\n", linux_mphi_state.waiting_transmit);
   
   for (i=0;i<sizeof(channels)/sizeof(channels[0]);i++)
   {
      LINUX_MPHI_CHANNEL_T *pchannel = &linux_mphi_state.channel[ channels[i] ];
      
      os_logging_message("Channel : %s - ", channel_name[i]);

      if (pchannel->opened)
      {
         int j, idx;
         
         os_logging_message("open\n");
         os_logging_message("Slots : start=%d, end=%d, entries=%d\n", pchannel->slot_start, pchannel->slot_end, pchannel->slot_entries);
         
         idx = pchannel->slot_start;
         for (j=0;j<pchannel->slot_entries;++j)
         {
            LINUX_SLOT_T *pslot = &pchannel->slot[pchannel->slot_start];
            os_logging_message("Slot %d : id = %d, size = %d, state = %s\n ", idx, pslot->id, pslot->len, slot_state[pslot->state]);
            
            if ( ++idx == LINUX_SLOTS_PER_CHANNEL )
               idx = 0;
         }
      }
      else
         os_logging_message("closed\n");
   }
}






/* ----------------------------------------------------------------------
 * interrupt handler.  this is called from lisr context
 * Current this is simulated until linux driver has interrupts implemented
 * -------------------------------------------------------------------- */
static void linux_hisr_interrupt( unsigned a, void *b)
{
   while (1)
   {
      // wait on the event
      uint32_t events;

      // wait on our event
      os_eventgroup_retrieve (&linux_mphi_state.event, &events );
      
      os_semaphore_obtain( &linux_mphi_state.latch );
      
      linux_mphi_state.interrupt_status = interrupt_state;
      
      interrupt_state = 0;
      
      os_semaphore_release( &linux_mphi_state.latch );
   
      if ( linux_mphi_state.event_callback )
      {
         linux_mphi_state.event_callback(linux_mphi_state.callback_data); // signal above that something happened
      }
      else
      {
         // drop this interrupt on the floor?
      }
   }
}

/********************************** End of file ******************************************/
