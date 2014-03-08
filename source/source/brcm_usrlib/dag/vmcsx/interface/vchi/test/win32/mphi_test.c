/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  
Module   :  
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION: OS interface test harness for multiple platforms
=============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// generic header for os abstration layer
#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi.h"

#include "vcfw/drivers/chip/mphi.h"

#define TEST_OK(a, b) {int res; if ((res = (b)) != 0) failed(a,res); else succeeded(a);}
   
#define TERM_EVENT 5

#define TX_BUFFER_SIZE 1024
#define RX_BUFFER_SIZE 16384
#define RX_LEN 512


/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static func forwards
******************************************************************************/

const MPHI_DRIVER_T *linux_mphi_get_func_table( void );
static void event_callback( const void *callback_data );


/******************************************************************************
Static data
******************************************************************************/

static OS_SEMAPHORE_T sem1;
static int flag;

static OS_SEMAPHORE_T event_latch;
int rx_event = 1;


static OS_EVENTGROUP_T eventgroup;
static int current_event;

static const MPHI_DRIVER_T *mphi_driver;
static DRIVER_HANDLE_T handle_control; // rx control
static DRIVER_HANDLE_T handle_data;    // rx bulk
static DRIVER_HANDLE_T handle_out;     // tx shared

static uint8_t *txbuf;
static uint8_t *rxbuf;

static const MPHI_OPEN_T open_control = { MPHI_CHANNEL_IN_CONTROL, event_callback };
static const MPHI_OPEN_T open_data    = { MPHI_CHANNEL_IN_DATA,    event_callback };
static const MPHI_OPEN_T open_out     = { MPHI_CHANNEL_OUT,        event_callback };


static int start_slots;
static int slots_now;
static int tx_slotid = 0x06;
static const int rx_slotid = 0x42;

/******************************************************************************
Global functions
******************************************************************************/

void failed(char *mess, int res)
{
   os_logging_message("%s : FAILED with %d\n", mess, res);
}

void succeeded(char *mess)
{
   os_logging_message("%s : ok\n", mess);
}


static void event_callback( const void *callback_data )
{   
   MPHI_EVENT_TYPE_T event;
   uint32_t pos;
   uint8_t id;
   static int write_ptr = 0;

   os_semaphore_obtain(&event_latch);

   event = mphi_driver->next_event( handle_control, &id, &pos );

   switch (event)   
   {
   case MPHI_EVENT_NONE :
      os_logging_message("Event = NONE\n");
      break;
      
   case MPHI_EVENT_OUT_DMA_COMPLETED :
   {
      os_logging_message("Received a transmit end event\n");

      // All correct, now check the slot ID returned which should match the one sent
      if (tx_slotid != id)
      {
         os_logging_message("Incorrect slot id - expect %d, got %d\n", tx_slotid, id);
      }

      slots_now = mphi_driver->slots_available(handle_out);
      
      if (slots_now != start_slots)
      {
         os_logging_message("Incorrect slot numbers left - expect %d, got %d\n", start_slots, slots_now);
      }

      break;
   }

   case MPHI_EVENT_IN_POSITION :
   {
      os_logging_message("event in position (old pos = %d, pos = %d) - %s\n", write_ptr, pos, &rxbuf[write_ptr]);
      
      // All correct, now check the slot ID returned which should match the one sent
      if (rx_slotid != id)
      {
         os_logging_message("Incorrect slot id - expect %d, got %d\n", rx_slotid, id);
      }
      
      write_ptr = pos;
      
      break;
   }
      
   case MPHI_EVENT_IN_DMA_COMPLETED :
   {
      os_logging_message("Received a event in DMA complete (old pos = %d, pos = %d) - %s\n", write_ptr, pos, &rxbuf[write_ptr]);

      // All correct, now check the slot ID returned which should match the one sent
      if (rx_slotid != id)
      {
         os_logging_message("Incorrect slot id - expect %d, got %d\n", rx_slotid, id);
      }

      // DMA end means the slots is no longer in fifo so re-add it after some confirmations
      slots_now = mphi_driver->slots_available(handle_control);
      
      if (slots_now != start_slots)
         os_logging_message("a) Incorrect slot numbers left - expect %d, got %d\n", start_slots, slots_now);
              
      mphi_driver->add_recv_slot( handle_control, rxbuf, RX_BUFFER_SIZE, rx_slotid );

      slots_now = mphi_driver->slots_available(handle_control);
      
      if (slots_now != start_slots-1)
         os_logging_message("b) Incorrect slot numbers left - expect %d, got %d\n", start_slots, slots_now);
         
      write_ptr = 0;   
   }
   
   default :
      os_logging_message("Unexpected event %d\n", event);
      break;
   }


   os_semaphore_release(&event_latch);
}


void main_host(unsigned a, void *b)
{
   const char *name;
   uint32_t maj, min;
   DRIVER_FLAGS_T flags;
   int len;
   
   printf("main_host\n");

   os_semaphore_create(  &event_latch, OS_SEMAPHORE_TYPE_SUSPEND);
   
   // Get ptr to functions
   mphi_driver = linux_mphi_get_func_table();
   
   TEST_OK("info", mphi_driver->info(&name, &maj, &min, &flags));
   
   os_logging_message("Driver name : %s   version %d.%d\n", name, maj, min);

   os_delay(500);

   
   TEST_OK("init", mphi_driver->init());
   
   mphi_driver->debug(0);

   os_delay(500);

   os_logging_message("Attempting to open the drivers\n");

   TEST_OK("open in control", mphi_driver->open( &open_control, &handle_control ));
   TEST_OK("open in data",    mphi_driver->open( &open_data,    &handle_data    ));
   TEST_OK("open outgoing",   mphi_driver->open( &open_out,     &handle_out     ));

   mphi_driver->debug(0);

   os_delay(500);

   os_logging_message("Allocating memory buffers\n");

   // allocate space for rx and tx buffers
   txbuf = malloc( TX_BUFFER_SIZE);
   memset( txbuf, 0, TX_BUFFER_SIZE );

   rxbuf = malloc( RX_BUFFER_SIZE);
   memset( rxbuf, 0, RX_BUFFER_SIZE );

   // Find out how many slots present
   start_slots = mphi_driver->slots_available(handle_control);

   // add receive slot.  0x42 is cookie we get back on completion
   mphi_driver->add_recv_slot( handle_control, rxbuf, RX_BUFFER_SIZE, rx_slotid );

   slots_now = mphi_driver->slots_available(handle_control);

   TEST_OK("Slot count decreased on adding", slots_now != start_slots - 1);

   mphi_driver->enable(handle_control);

   os_delay(500);

   os_logging_message("Queueing outgoing message\n");

   // And fill up the transmit buffer with some fun data.
   // Must be padded to 16 bytes
   strcpy(txbuf, "Hello VideoCore, how are you today? Padded to 16");
   len = (strlen(txbuf) + 15) & ~0xf;
   
   // record the number of slots before we add to the queue
   start_slots = mphi_driver->slots_available(handle_out);
   
   // Send it to the control channel (<>0 in parameter 2)
   TEST_OK("Output data", mphi_driver->out_queue_message( handle_out, 1, txbuf, len, tx_slotid, MPHI_FLAGS_TERMINATE_DMA));

   // Now wait for an event to show its been sent - TXEND or similar.

   os_logging_message("waiting 2s for event to arrive\n");

   // Wait for 5 secs for events to arrive
   os_delay(2000); //  2 sec
   

   // Close the drivers
   TEST_OK("close control", mphi_driver->close(handle_control));
   TEST_OK("close data", mphi_driver->close(handle_data));
   TEST_OK("close out", mphi_driver->close(handle_out));
   
   os_logging_message("Tests complete\n\n");

   // gives a chance for the output to make it to the terminal before any other messages appear   
   os_delay(500);
}


void main_vc(unsigned a, void *b)
{
   int32_t success = -1; //fail by default
   VCHI_INSTANCE_T initialise_instance;
   VCHI_CONNECTION_T *connection[1];
   const uint32_t *pointer = NULL;

   printf("main_vc\n");

   // initialise the vchi layer
   success = vchi_initialise( &initialise_instance);
   assert( success == 0 );

   connection[0] = vchi_create_connection(single_get_func_table(),
                                          vchi_mphi_message_driver_func_table());

   // now turn on the control service and initialise the connection
   success = vchi_connect( &connection[0], 1, initialise_instance);
   assert( success == 0 );
}


int main(void)
{
   int32_t success;
   VCOS_THREAD_T thread_vc, thread_host;
   const int STACK=16*1024;

   printf("Testing MPHI driver layer\n");
   printf("=========================\n"); 
   
   // System init
   TEST_OK("os_init", os_init());

   success = os_thread_start( &thread_host, main_host, NULL, STACK, "main_host" );
   assert(success==0);
   success = os_thread_start( &thread_vc, main_vc, NULL, STACK, "main_vc" );
   assert(success==0);
   
   //while (1) //os_thread_is_running(thread_vc) || os_thread_is_running(thread_host))
   //   {}

   return 0;
}
