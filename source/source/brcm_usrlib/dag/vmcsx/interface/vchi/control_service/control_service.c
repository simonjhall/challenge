/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile:  $
Revision :  $Revision: #6 $

FILE DESCRIPTION:
This module is the control service which needs to operate across multiple
connections.

The initialisation process consists of pushing 'init' messages to the other side.
Once a similar message has been received we declare the connection open.

When a Service Open message is received from the other side we need to signal
the connection layer that this is the case and cause any pending messages to be sent.

Need to be able to query created (but not necessarily opened) services.

The list of created/opened services is held in the connection


=============================================================================*/
#include <stdlib.h>
#include <string.h>

#include "interface/vchi/vchi.h"
#include "interface/vchi/vchi_cfg_internal.h"
#include "interface/vchi/connections/connection.h"
#include "control_service.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi_cfg.h"


/******************************************************************************
Private typedefs
******************************************************************************/

// command codes for CTRL messages
typedef enum {
   CONNECT,               // used to signal opening of the comms
   SERVER_AVAILABLE,      // queries if a server is available
   BULK_TRANSFER_RX,      // informs the other side about an item added to the RX BULK FIFO
   XON,                   // restore comms on a particular service
   XOFF,                  // pause comms on a particular service
   SERVER_AVAILABLE_REPLY,// response to a SERVER_AVAILABLE query
   DISCONNECT,            // used to signal ending of comms (hence shutdown of VideoCore)
   POWER_CONTROL,         // instruct peer to change power mode of a transmitter channel
   POWER_CONTROL_REPLY    // response to a POWER_CONTROL command
} VCHI_COMMANDS_T;

typedef struct 
{
   fourcc_t service_id;
   int32_t reply_to_send;
   uint8_t reply[12];  //12 byte
} control_reply_t;

typedef struct {
   VCHI_CONNECTION_SERVICE_HANDLE_T open_handle;
   VCHI_CONNECTION_T * connection;
   int initialised;
   OS_SEMAPHORE_T connected_semaphore;
   control_reply_t  control_reply[VCHI_MAX_SERVICES_PER_CONNECTION];
   int32_t server_available_replies_to_send;
} CONTROL_SERVICE_INFO_T;



/******************************************************************************
Static data
******************************************************************************/

static uint32_t time_offset;


// function to handle a CTRL message
static void control_callback( void *callback_param, //my service local param
                              VCHI_CALLBACK_REASON_T reason,
                              void *handle ); //for transmitting msg's only


/******************************************************************************
Global functions
******************************************************************************/

/***********************************************************
 * Name: vchi_control_service_init
 *
 * Arguments:  VCHI_CONNECTION_T *connections,
 *             const uint32_t num_connections
 *
 * Description: Routine to init the control service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_control_service_init( VCHI_CONNECTION_T **connections,
                                   const uint32_t num_connections,
                                   void **state )
{
   int32_t success = 0;
   uint32_t count = 0;
   CONTROL_SERVICE_INFO_T *control_info = (CONTROL_SERVICE_INFO_T *)*state;

   os_assert(num_connections <= VCHI_MAX_NUM_CONNECTIONS);

   if (!control_info)
   {
      control_info = (CONTROL_SERVICE_INFO_T *)os_malloc( VCHI_MAX_NUM_CONNECTIONS * sizeof(CONTROL_SERVICE_INFO_T), 0, "vchi:control_info" );
      memset( control_info, 0,VCHI_MAX_NUM_CONNECTIONS * sizeof(CONTROL_SERVICE_INFO_T) );

      for( count = 0; count < num_connections; count++ )
      {
#ifdef VCHI_COARSE_LOCKING
         os_semaphore_obtain(&connections[count]->sem);
#endif
         // create and obtain the semaphore used to signal when we have connected
         success += os_semaphore_create( &control_info[count].connected_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
         os_assert( success == 0 );

         // record the connection info
         control_info[count].connection = connections[count];
         // create the server (this is a misnomer as the the CTRL service acts as both client and server
         success += (connections[count])->api->service_connect((connections[count])->state,MAKE_FOURCC("CTRL"),0,0,VC_TRUE,control_callback,&control_info[count],VC_FALSE,VC_FALSE,VC_FALSE,&control_info[count].open_handle);
         os_assert(success == 0);
#ifdef VCHI_COARSE_LOCKING
         os_semaphore_release(&connections[count]->sem);
#endif
      }
      // start timestamps from zero
      time_offset = 0;
      time_offset -= vchi_control_get_time();
   }
   else
   {
      for ( count = 0; count < num_connections; count++ )
      {
         os_assert(control_info[count].connection == connections[count]);
      }
   }

   // because we can have both ends of a connection running on one processor we need to send down the CONNECTION command after all
   // the connections have been connected
   for( count = 0; count < num_connections; count++ )
   {
#ifdef VCHI_COARSE_LOCKING
      os_semaphore_obtain(&connections[count]->sem);
#endif
      // record that we have not yet handled an INIT request
      control_info[count].initialised = VC_FALSE;
      success = os_semaphore_obtain( &control_info[count].connected_semaphore );
      os_assert( success == 0 );
      success = vchi_control_queue_connect( &control_info[count], VC_FALSE );
      os_assert(success == 0);
#ifdef VCHI_COARSE_LOCKING
      os_semaphore_release(&connections[count]->sem);
#endif
   }
   // now we must wait for the connections to have their INIT returned
   for( count = 0; count < num_connections; count++ )
   {
      success = os_semaphore_obtain( &control_info[count].connected_semaphore );
      os_assert( success == 0 );
      success = os_semaphore_release( &control_info[count].connected_semaphore );
      os_assert( success == 0 );
   }
   *state = control_info;
   return success;
}


/***********************************************************
 * Name: vchi_control_service_close
 *
 * Arguments:  VCHI_CONNECTION_T *connections,
 *             const uint32_t num_connections
 *
 * Description: Routine to init the control service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_control_service_close( void )
{
   int32_t success = 0;
#if 1
   os_assert(0);
#else
   uint32_t count = 0;

   for( count = 0; count < VCHI_MAX_NUM_CONNECTIONS; count++ )
   {
      if( control_info[count].initialised == VC_TRUE)
      {
         // we have previously initialised this connection
         // so set it all as uninitialised
         // HACK HACK TBD FIXME - do we need to signal to the other end of the connections that we are closing this end??????
         // similarly we would need to respond to such requests ourselves......
         control_info[count].initialised = VC_FALSE;
         control_info[count].connection = NULL;
         success += os_semaphore_release( &control_info[count].connected_semaphore );
      }

   }
#endif
   // close the timer
   os_time_term();
   // reset the timer offset
   time_offset = 0;
   return(success);
}


/***********************************************************
 * Name: vchi_initialise_time
 *
 * Arguments: -
 *
 * Description: Routine to initialise the timer stuff
 *
 * Returns: current time
 *
 ***********************************************************/
void vchi_initialise_time(void)
{
   // open up the VCFW driver to get the time values for the messages
   time_offset = 0;

   os_time_init();
}


/***********************************************************
 * Name: vchi_control_get_time
 *
 * Arguments: -
 *
 * Description: Routine to return the current time
 *
 * Returns: current time
 *
 ***********************************************************/
uint32_t vchi_control_get_time(void)
{
   return os_get_time_in_usecs() + time_offset;
}


/***********************************************************
 * Name: vchi_control_update_time
 *
 * Arguments: uint32_t time
 *
 * Description: Keeps our clock in sync with the Host clock
 *
 * Returns: -
 *
 ***********************************************************/
void vchi_control_update_time(uint32_t time)
{
   int32_t delta;
   
   // ignore 0 timestamps - some dummy messages may be sent
   // untimestamped
   if (time == 0)
      return;
   
   delta = vchi_control_get_time() - time;

   // see if we need to increase our offset to keep the clocks in sync
   if ( delta < 0 )
      time_offset += 1 - delta;
}


fourcc_t vchi_control_reply_pick_service(void *handle, uint8_t *message)
{
  CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
  uint32_t ii;
  
  if (!service_info->server_available_replies_to_send)
     return 0;

  for (ii=0;ii<VCHI_MAX_SERVICES_PER_CONNECTION;ii++)
  {
     if (service_info->control_reply[ii].reply_to_send != 0)
     { 
        memcpy(message, service_info->control_reply[ii].reply,12);
        return service_info->control_reply[ii].service_id;
     }
   }
   return 0;

}

void vchi_control_reply_sub_service(void *handle, fourcc_t service_id)
{
  CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
  uint32_t ii;

  for (ii=0;ii<VCHI_MAX_SERVICES_PER_CONNECTION;ii++)
  {
     if (service_info->control_reply[ii].service_id == service_id)
     { 
        service_info->control_reply[ii].reply_to_send--;
        os_assert(service_info->control_reply[ii].reply_to_send >=0);
        service_info->server_available_replies_to_send--;
        os_assert(service_info->server_available_replies_to_send >=0);
     }
   }

}

static int32_t vchi_control_reply_add_service(CONTROL_SERVICE_INFO_T *service_info, fourcc_t service_id,uint8_t *data)
{
  uint32_t ii;

  for (ii=0;ii<VCHI_MAX_SERVICES_PER_CONNECTION;ii++)
  {
     if (service_info->control_reply[ii].service_id == 0 || service_info->control_reply[ii].service_id == service_id)
     { 
        service_info->control_reply[ii].service_id = service_id;
        service_info->control_reply[ii].reply_to_send++;
        service_info->server_available_replies_to_send++;
        memcpy(service_info->control_reply[ii].reply,data,12);
        return 0;
     }
    }

   return -1;
}

/***********************************************************
 * Name: control_callback
 *
 * Arguments: void *callback_param
 *            const VCHI_CALLBACK_REASON_T reason
 *            const void *handle
 *
 * Description: Handles callbacks for received messages
 *
 * Returns: -
 *
 ***********************************************************/
static void control_callback( void *callback_param, //my service local param
                              const VCHI_CALLBACK_REASON_T reason,
                              void *handle )
{
CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)callback_param;
uint8_t *message;
uint8_t output_message[128];
uint32_t message_length;
int32_t return_value;
fourcc_t service_id;
uint32_t flags;
VCHI_FLAGS_T output_flags = VCHI_FLAGS_NONE;

   switch(reason)
   {
      case VCHI_CALLBACK_MSG_AVAILABLE:
      {
         // handle the message in place
         void *message_handle;
#if defined VCHI_COARSE_LOCKING
         os_semaphore_obtain(&service_info->connection->sem);
#endif
#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
         do {
         message_handle = NULL;
#endif
         return_value = service_info->connection->api->service_hold_msg(service_info->open_handle,(void**)&message,&message_length,VCHI_FLAGS_NONE,&message_handle);
         if(return_value == 0)
         {
            if (message_length>0)
            {
               // this is a valid message - read the command
               uint32_t command = vchi_readbuf_uint32(&message[0]);
               int32_t reply_required = VC_FALSE;
               switch(command)
               {
               case CONNECT:
                  os_logging_message("CTRL SERVICE: Connect message");
                  if(service_info->initialised ==  VC_FALSE)
                  {
                     uint32_t protocol_version;
                     uint32_t slot_size;
                     uint32_t num_slots;
                     uint32_t min_bulk_size;

                     return_value = os_semaphore_release( &service_info->connected_semaphore );
                     os_assert( return_value == 0 );
                     // record that we have been initialised
                     service_info->initialised = VC_TRUE;
                     // extract the values we need to pass back to the service - principally slot size for the moment
                     protocol_version = vchi_readbuf_uint32(&message[4]);
                     slot_size        = vchi_readbuf_uint32(&message[8]);
                     num_slots        = vchi_readbuf_uint32(&message[12]);
                     min_bulk_size    = message_length >= 20 ? vchi_readbuf_uint32(&message[16]) : 0;
                     //os_assert(0);
                     service_info->connection->api->connection_info(service_info->connection->state,protocol_version, slot_size, num_slots, min_bulk_size);
                     // we are going to reply with 'CONNECT'
                     return_value = vchi_control_queue_connect( service_info, VC_TRUE );
                     os_assert( return_value == 0);
                  }
                  break;
               case SERVER_AVAILABLE:
               {
                  VCHI_SERVICE_FLAGS_T peer_flags = (VCHI_SERVICE_FLAGS_T)(message_length >= 12 ? vchi_readbuf_uint32(&message[8]) : 0);
                  int32_t our_flags;
                  service_id = vchi_readbuf_fourcc(&message[4]);
                  os_logging_message("CTRL SERVICE: Server available (%c%c%c%c:0x%x)",FOURCC_TO_CHAR(service_id),peer_flags);
                  message_length = 12;
                  // we are replying
                  reply_required = VC_TRUE;
                  // first part of the reply is the command
                  vchi_writebuf_uint32( &output_message[0], SERVER_AVAILABLE_REPLY );
                  // then the requested server ID
                  vchi_writebuf_fourcc( &output_message[4], service_id );
                  // check if this fourcc is in our list of servers on this connection
                  our_flags = service_info->connection->api->server_present(service_info->connection->state, service_id, peer_flags);
                  if(our_flags >= 0)
                     vchi_writebuf_uint32( &output_message[8], AVAILABLE | our_flags );
                  else
                     vchi_writebuf_uint32( &output_message[8], 0 );
                  break;
               }
               case SERVER_AVAILABLE_REPLY:
                  // Connection can take ownership of the message - signalled by returning true.
                  service_id = vchi_readbuf_fourcc(&message[4]);
                  flags = vchi_readbuf_uint32(&message[8]);
                  service_info->connection->api->server_available_reply(service_info->connection->state, service_id, flags);
                  break;
               case BULK_TRANSFER_RX:
               {
                  uint32_t length, channel_params, data_length, data_offset;
                  MESSAGE_TX_CHANNEL_T channel;
                  // extract the service and length and pass it to the low level driver
                  service_id = vchi_readbuf_fourcc(&message[4]);
                  length = vchi_readbuf_uint32(&message[8]);
                  channel = (MESSAGE_TX_CHANNEL_T)(message_length >= 20 ? message[12] : MESSAGE_TX_CHANNEL_BULK);
                  channel_params = message_length >= 20 ? vchi_readbuf_uint32(&message[16]) : 0;
                  data_length = message_length >= 28 ? vchi_readbuf_uint32(&message[20]) : length;
                  data_offset = message_length >= 28 ? vchi_readbuf_uint32(&message[24]) : 0;
                  os_logging_message("CTRL SERVICE: Bulk transfer rx (%c%c%c%c), channel %d, %u Bytes (core=%u, offset=%u)",FOURCC_TO_CHAR(service_id), channel, length, data_length, data_offset);
                  service_info->connection->api->rx_bulk_buffer_added(service_info->connection->state,service_id,length,channel,channel_params,data_length,data_offset);
                  break;
               }
               case XON:
                  service_id = vchi_readbuf_fourcc(&message[4]);
                  os_logging_message("CTRL SERVICE: Xon (%c%c%c%c)",FOURCC_TO_CHAR(service_id));
                  service_info->connection->api->flow_control(service_info->connection->state, service_id, VC_FALSE);
                  break;
               case XOFF:
                  service_id = vchi_readbuf_fourcc(&message[4]);
                  os_logging_message("CTRL SERVICE: Xoff (%c%c%c%c)",FOURCC_TO_CHAR(service_id));
                  service_info->connection->api->flow_control(service_info->connection->state, service_id, VC_TRUE);
                  break;
               case DISCONNECT:
                  flags = message_length >= 8 ? vchi_readbuf_uint32(&message[4]) : 0;
                  service_info->connection->api->disconnect(service_info->connection->state, flags);
                  break;
               case POWER_CONTROL:
               {
                  MESSAGE_TX_CHANNEL_T channel = vchi_readbuf_uint32(&message[4]);
                  bool_t enable = vchi_readbuf_uint32(&message[8]);
                  uint32_t cookie = vchi_readbuf_uint32(&message[12]);
                  os_logging_message("CTRL SERVICE: Power (%d -> %d; %d)", channel, enable, cookie);
                  // connection should synchronously perform the power change, then we reply
                  // don't currently allow for any sort of error (protocol doesn't allow for it)
                  service_info->connection->api->power_control(service_info->connection->state, channel, enable);
                  // we are replying
                  reply_required = VC_TRUE;
                  message_length = 16;
                  // reply is the same as the message, except for the command code
                  vchi_writebuf_uint32( &output_message[0], POWER_CONTROL_REPLY );
                  memcpy(output_message+4, message+4, 12);
                  break;
               }
               default:
                  os_logging_message("CTRL SERVICE: Unknown message (0x%x)", command);
                  os_assert(0);
                  break;
               }
               // transmit the reply (if needed)
               if(reply_required)
               {
                  //attempt to send
                  return_value = service_info->connection->api->service_queue_msg(service_info->open_handle,output_message,message_length,output_flags,NULL);
                  if (return_value != 0) //failed because tx slots are full or whatever
                  {
                     //save the msg and send it later
                     return_value = vchi_control_reply_add_service(service_info,service_id,output_message);
                     os_assert(return_value == 0);
                  }
               }
            }
            return_value = service_info->connection->api->held_msg_release(service_info->open_handle, message_handle);
            os_assert(return_value == 0);
         }
#ifdef VCHI_FEWER_MSG_AVAILABLE_CALLBACKS
         } while (message_handle);
#endif
#if defined VCHI_COARSE_LOCKING
         os_semaphore_release(&service_info->connection->sem);
#endif
         break;
      }
      case VCHI_CALLBACK_MSG_SENT:
         break;
         
      case VCHI_CALLBACK_BULK_RECEIVED:
      case VCHI_CALLBACK_BULK_DATA_READ:
      case VCHI_CALLBACK_BULK_SENT:      // control service doesn't use bulk transfers
      case VCHI_CALLBACK_SENT_XOFF:
      case VCHI_CALLBACK_SENT_XON:       // control service must never be XOFFed
         os_assert(0);
         break;
   }
}

/***********************************************************
 * Name: vchi_control_queue_connect
 *
 * Arguments: void *handle
 *
 * Description: Queues a connect message
 *
 * Returns: -
 *
 ***********************************************************/
int32_t
vchi_control_queue_connect( void *handle, bool_t reply )
{
   CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
   int slots = service_info->connection->api->connection_rx_slots_available(service_info->connection->state);
   uint32_t slot_size = service_info->connection->api->connection_rx_slot_size(service_info->connection->state);
   uint8_t message[20];
   uint32_t protocol = PROTOCOL_VERSION;
   if (reply) protocol |= PROTOCOL_REPLY_FLAG;
   
   vchi_writebuf_uint32(&message[0],  CONNECT             );
   vchi_writebuf_uint32(&message[4],  protocol            );
   vchi_writebuf_uint32(&message[8],  slot_size           );
   vchi_writebuf_uint32(&message[12], slots               );
   vchi_writebuf_uint32(&message[16], VCHI_MIN_BULK_SIZE  );

   return service_info->connection->api->service_queue_msg(service_info->open_handle, message, 20, (VCHI_FLAGS_T)(VCHI_FLAGS_BLOCK_UNTIL_QUEUED|VCHI_FLAGS_ALIGN_SLOT), NULL);
}

/***********************************************************
 * Name: vchi_control_queue_xon_xoff
 *
 * Arguments: void *handle
 *            fourcc_t id
 *            bool_t xon
 *
 * Description: Queues an xon or xoff message
 *
 * Returns: -
 *
 ***********************************************************/
int32_t
vchi_control_queue_xon_xoff( void *handle, fourcc_t service_id, bool_t xon )
{
   CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
   uint8_t message[8];
   //printf("%s %c%c%c%c\n", xon ? "XON" : "XOFF", FOURCC_TO_CHAR(service_id));
   
   vchi_writebuf_uint32(&message[0], xon ? XON : XOFF );
   vchi_writebuf_fourcc(&message[4], service_id );
   
   return service_info->connection->api->service_queue_msg(service_info->open_handle, message, 8, VCHI_FLAGS_NONE, NULL);
}

/***********************************************************
 * Name: vchi_control_queue_bulk_transfer_rx
 *
 * Arguments: void *handle
 *            fourcc_t service_id
 *            uint32_t total_len
 *            uint32_t interface
 *            uint32_t interface_params
 *            uint32_t data_len
 *            uint32_t data_offset
 *
 * Description: Queues an xon or xoff message
 *
 * Returns: -
 *
 ***********************************************************/
int32_t
vchi_control_queue_bulk_transfer_rx( void *handle,
                                     fourcc_t service_id,
                                     uint32_t total_len,
                                     MESSAGE_RX_CHANNEL_T channel,
                                     uint32_t channel_params,
                                     uint32_t data_len,
                                     uint32_t data_offset )
{
   CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
   uint8_t message[28];
   
   vchi_writebuf_uint32( &message[0], BULK_TRANSFER_RX );
   vchi_writebuf_fourcc( &message[4], service_id );
   vchi_writebuf_uint32( &message[8], total_len );
   message[12] = channel;
   message[13] = message[14] = message[15] = 0;
   vchi_writebuf_uint32( &message[16], channel_params );
   vchi_writebuf_uint32( &message[20], data_len );
   vchi_writebuf_uint32( &message[24], data_offset );

   return service_info->connection->api->service_queue_msg(service_info->open_handle, message, 28, VCHI_FLAGS_NONE, NULL );
}

/***********************************************************
 * Name: vchi_control_queue_server_available
 *
 * Arguments: void *handle
 *            fourcc_t service_id
 *            VCHI_SERVICE_FLAGS_T flags
 *
 * Description: Queues a server available query
 *
 * Returns: -
 *
 ***********************************************************/
int32_t
vchi_control_queue_server_available( void *handle,
                                     fourcc_t service_id,
                                     VCHI_SERVICE_FLAGS_T flags )
{
   CONTROL_SERVICE_INFO_T * service_info = (CONTROL_SERVICE_INFO_T *)handle;
   uint8_t message[12];
   
   vchi_writebuf_uint32( &message[0], SERVER_AVAILABLE );
   vchi_writebuf_fourcc( &message[4], service_id );
   vchi_writebuf_uint32( &message[8], flags );

   return service_info->connection->api->service_queue_msg(service_info->open_handle, message, 12, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
}
/****************************** End of file ***********************************/
