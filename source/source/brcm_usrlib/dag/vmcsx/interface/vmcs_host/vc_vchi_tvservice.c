/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  TV service
Module   :  Software host interface (host-side)
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_tvservice.c#3 $
Revision :  $Revision: #3 $

FILE DESCRIPTION
TV service host side API implementation
=============================================================================*/
#include <string.h>
#include "vchost_config.h"
#include "vchost.h"

#include "interface/vchi/vchi.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/os/os.h"
#include "vc_tvservice.h"


/******************************************************************************
Local types and defines.
******************************************************************************/
#ifndef _min
#define _min(x,y) (((x) <= (y))? (x) : (y))
#endif
#ifndef _max
#define _max(x,y) (((x) >= (y))? (x) : (y))
#endif

//TV service host side state (mostly the same as Videocore side - TVSERVICE_STATE_T)
typedef struct {
   //Generic service stuff
   VCHI_SERVICE_HANDLE_T client_handle[VCHI_MAX_NUM_CONNECTIONS]; //To connect to server on VC
   VCHI_SERVICE_HANDLE_T notify_handle[VCHI_MAX_NUM_CONNECTIONS]; //For incoming notification
   uint32_t              msg_flag[VCHI_MAX_NUM_CONNECTIONS];
   char                  command_buffer[TVSERVICE_MSGFIFO_SIZE];
   char                  response_buffer[TVSERVICE_MSGFIFO_SIZE];
   uint32_t              response_length;
   uint32_t              notify_buffer[TVSERVICE_MSGFIFO_SIZE/sizeof(uint32_t)];
   uint32_t              notify_length;
   uint32_t              num_connections;
   OS_SEMAPHORE_T        sema;
   TVSERVICE_CALLBACK_T  notify_fn;
   void                 *notify_data;
   int                   initialised;

   //TV stuff
   uint32_t              current_tv_state;
   uint32_t              copy_protect;

   //HDMI specific stuff
   HDMI_RES_GROUP_T      hdmi_current_group;
   HDMI_MODE_T           hdmi_current_mode;
   HDMI_DISPLAY_OPTIONS_T hdmi_options;

   //If client ever asks for supported modes, we store them for quick return
   //The preferred mode is actually duplicated in cea_cache (if it is valid), 
   //but we can fill them in when we get the STANDBY notification
   HDMI_RES_GROUP_T      hdmi_preferred_group; 
   uint32_t              hdmi_preferred_mode;
   uint32_t cea_cache_valid;
   TV_QUERY_SUPPORTED_MODES_RESP_T cea_cache;
   uint32_t dmt_cache_valid;
   TV_SUPPORTED_MODE_T   dmt_supported_modes[TV_MAX_SUPPORTED_MODES];
   uint32_t              num_dmt_modes;

   //SDTV specific stuff
   SDTV_COLOUR_T         sdtv_current_colour;
   SDTV_MODE_T           sdtv_current_mode;
   SDTV_OPTIONS_T        sdtv_options;
   SDTV_CP_MODE_T        sdtv_current_cp_mode;
} TVSERVICE_HOST_STATE_T;

/******************************************************************************
Static data.
******************************************************************************/
static TVSERVICE_HOST_STATE_T tvservice_client;
static OS_SEMAPHORE_T tvservice_message_available_semaphore;
static OS_SEMAPHORE_T tvservice_notify_available_semaphore;
static VCOS_THREAD_T tvservice_notify_task;

/******************************************************************************
Static functions.
******************************************************************************/
//Lock the host state
static void lock_obtain (void) {
   int32_t success;
   vcos_assert(tvservice_client.initialised);
   success = os_semaphore_obtain( &tvservice_client.sema );
   vcos_assert(success >= 0);
}

//Unlock the host state
static void lock_release (void) {
   int32_t success;
   vcos_assert(tvservice_client.initialised);
   vcos_assert(os_semaphore_obtained(&tvservice_client.sema));
   success = os_semaphore_release( &tvservice_client.sema );
   vcos_assert( success >= 0 );
}

//Forward declarations
static void tvservice_client_callback( void *callback_param,
                                      VCHI_CALLBACK_REASON_T reason,
                                      void *msg_handle );

static void tvservice_notify_callback( void *callback_param,
                                      VCHI_CALLBACK_REASON_T reason,
                                      void *msg_handle );

static int32_t tvservice_wait_for_reply(void *response, uint32_t max_length);

static int32_t tvservice_wait_for_bulk_receive(void *buffer, uint32_t max_length);

static int32_t tvservice_send_command( uint32_t command, void *buffer, uint32_t length, uint32_t has_reply);

static int32_t tvservice_send_command_reply( uint32_t command, void *buffer, uint32_t length,
                                             void *response, uint32_t max_length);

static void tvservice_notify_func( unsigned int argc, void *argv );


/******************************************************************************
TV service API
******************************************************************************/
/******************************************************************************
NAME
   vc_vchi_tv_init

SYNOPSIS
   void vc_vchi_tv_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections )

FUNCTION
   Initialise the TV service for use. A negative return value
   indicates failure (which may mean it has not been started on VideoCore).

RETURNS
   int
******************************************************************************/
VCHPRE_ void VCHPOST_ vc_vchi_tv_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int32_t success;
   uint32_t i;
   static const HDMI_DISPLAY_OPTIONS_T hdmi_default_display_options =
      {
         HDMI_ASPECT_UNKNOWN,
         VC_FALSE, 0, 0, // No vertical bar information.
         VC_FALSE, 0, 0, // No horizontal bar information.
      };

   // record the number of connections
   memset( &tvservice_client, 0, sizeof(TVSERVICE_HOST_STATE_T) );
   tvservice_client.num_connections = num_connections;
   success = os_semaphore_create( &tvservice_client.sema, OS_SEMAPHORE_TYPE_SUSPEND );
   vcos_assert( success == 0 );
   success = os_semaphore_create( &tvservice_message_available_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   vcos_assert( success == 0 );
   success = os_semaphore_obtain( &tvservice_message_available_semaphore );
   vcos_assert( success == 0 );
   success = os_semaphore_create( &tvservice_notify_available_semaphore, OS_SEMAPHORE_TYPE_SUSPEND );
   vcos_assert( success == 0 );
   success = os_semaphore_obtain( &tvservice_notify_available_semaphore );
   vcos_assert( success == 0 );

   //Initialise any other non-zero bits of the TV service state here
   tvservice_client.sdtv_current_mode = SDTV_MODE_OFF; 
   memcpy(&tvservice_client.hdmi_options, &hdmi_default_display_options, sizeof(HDMI_DISPLAY_OPTIONS_T));
   tvservice_client.sdtv_options.aspect = SDTV_ASPECT_4_3;

   for (i=0; i < tvservice_client.num_connections; i++) {

      // Create a 'Client' service on the each of the connections
      SERVICE_CREATION_T tvservice_parameters = { TVSERVICE_CLIENT_NAME,      // 4cc service code
                                                  connections[i],             // passed in fn ptrs
                                                  0,                          // tx fifo size (unused)
                                                  0,                          // tx fifo size (unused)
                                                  &tvservice_client_callback, // service callback
                                                  &tvservice_message_available_semaphore,  // callback parameter
                                                  VC_FALSE,                   // want_unaligned_bulk_rx
                                                  VC_TRUE,                    // want_unaligned_bulk_tx
                                                  VC_FALSE,                   // want_crc
      };

      SERVICE_CREATION_T tvservice_parameters2 = { TVSERVICE_NOTIFY_NAME,     // 4cc service code
                                                   connections[i],            // passed in fn ptrs
                                                   0,                         // tx fifo size (unused)
                                                   0,                         // tx fifo size (unused)
                                                   &tvservice_notify_callback,// service callback
                                                   &tvservice_notify_available_semaphore,  // callback parameter
                                                   VC_FALSE,                  // want_unaligned_bulk_rx
                                                   VC_FALSE,                  // want_unaligned_bulk_tx
                                                   VC_FALSE,                  // want_crc
      };

      //Create the client to normal TV service 
      success = vchi_service_open( initialise_instance, &tvservice_parameters, &tvservice_client.client_handle[i] );
      vcos_assert( success == 0 );

      //Create the client to the async TV service (any TV related notifications)
      success = vchi_service_open( initialise_instance, &tvservice_parameters2, &tvservice_client.notify_handle[i] );
      vcos_assert( success == 0 );

   }
   
   //Create the notifier task
   success = os_thread_start(&tvservice_notify_task, tvservice_notify_func, &tvservice_client, 2048, "TVSERV Notify");
   vcos_assert( success == 0 );
   
   tvservice_client.initialised = 1;
}

/***********************************************************
 * Name: vc_vchi_tv_stop
 *
 * Arguments:
 *       -
 *
 * Description: Stops the Host side part of TV service
 *
 * Returns: -
 *
 ***********************************************************/
VCHPRE_ void VCHPOST_ vc_vchi_tv_stop( void ) {
   // Wait for the current lock-holder to finish before zapping TV service
   uint32_t i;
   lock_obtain();
   //TODO: there is no API to stop the notifier task at the moment
   for (i=0; i < tvservice_client.num_connections; i++) {
      int32_t result;
      result = vchi_service_close(tvservice_client.client_handle[i]);
      assert( result == 0 );
      result = vchi_service_close(tvservice_client.notify_handle[i]);
      assert( result == 0 );
   }
   tvservice_client.initialised = 0;
   lock_release();
}


/***********************************************************
 * Name: vc_tv_register_callaback
 *
 * Arguments:
 *       callback function, context to be passed when function is called
 *
 * Description: Register a callback function for all TV notifications
 *
 * Returns: -
 *
 ***********************************************************/
VCHPRE_ void VCHPOST_ vc_tv_register_callback(TVSERVICE_CALLBACK_T callback, void *callback_data) {
   lock_obtain();
   tvservice_client.notify_fn = callback;
   tvservice_client.notify_data = callback_data;
   lock_release();
}


/*********************************************************************************
 *
 *  Static functions definitions
 *
 *********************************************************************************/
//TODO: Might need to handle multiple connections later
/***********************************************************
 * Name: tvservice_client_callback
 *
 * Arguments: semaphore, callback reason and message handle
 *
 * Description: Callback when a message is available for TV service
 *
 ***********************************************************/
static void tvservice_client_callback( void *callback_param,
                                       const VCHI_CALLBACK_REASON_T reason,
                                       void *msg_handle ) {

   OS_SEMAPHORE_T *sem = (OS_SEMAPHORE_T *)callback_param;

   if ( reason != VCHI_CALLBACK_MSG_AVAILABLE )
      return;

   if ( sem == NULL )
      return;

   if ( os_semaphore_obtained(sem) ) {
      int32_t success = os_semaphore_release( sem );
      vcos_assert( success >= 0 );
   }

}

/***********************************************************
 * Name: tvservice_notify_callback
 *
 * Arguments: semaphore, callback reason and message handle
 *
 * Description: Callback when a message is available for TV notify service
 *
 ***********************************************************/
static void tvservice_notify_callback( void *callback_param,
                                       const VCHI_CALLBACK_REASON_T reason,
                                       void *msg_handle ) {
   OS_SEMAPHORE_T *sem = (OS_SEMAPHORE_T *)callback_param;

   if ( reason != VCHI_CALLBACK_MSG_AVAILABLE )
      return;

   if ( sem == NULL )
      return;

   if ( os_semaphore_obtained(sem) ) {
      int32_t success = os_semaphore_release( sem );
      vcos_assert( success >= 0 );
   }

}

/***********************************************************
 * Name: tvservice_wait_for_reply
 *
 * Arguments: response buffer, buffer length
 *
 * Description: blocked until something is in the buffer
 *
 * Returns error code of vchi
 *
 ***********************************************************/
static int32_t tvservice_wait_for_reply(void *response, uint32_t max_length) {
   int32_t success = 0;
   int32_t sem_ok = 0;
   uint32_t length_read = 0;
   do {
      //TODO : we need to deal with messages coming through on more than one connections properly
      //At the moment it will always try to read the first connection if there is something there
      //Check if there is something in the queue, if so return immediately
      //otherwise wait for the semaphore and read again
      success = vchi_msg_dequeue( tvservice_client.client_handle[0], response, max_length, &length_read, VCHI_FLAGS_NONE );
   } while( length_read == 0 && (sem_ok = os_semaphore_obtain( &tvservice_message_available_semaphore)) == 0);

   return success;
}

/***********************************************************
 * Name: tvservice_wait_for_bulk_receive
 *
 * Arguments: response buffer, buffer length
 *
 * Description: blocked until bulk receive
 *
 * Returns error code of vchi
 *
 ***********************************************************/
static int32_t tvservice_wait_for_bulk_receive(void *buffer, uint32_t max_length) {
   vcos_assert(((uint32_t) buffer & 0xf) == 0); //should be 16 byte aligned
   return vchi_bulk_queue_receive( tvservice_client.client_handle[0],
                                   buffer,
                                   max_length,
                                   VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
                                   NULL );
}

/***********************************************************
 * Name: tvservice_send_command
 *
 * Arguments: command, parameter buffer, parameter legnth, has reply? (non-zero means yes)
 *
 * Description: send a command and optionally wait for its single value reponse (TV_GENERAL_RESP_T)
 *
 * Returns: response.ret (currently only 1 int in the wrapped response), endian translated if necessary
 *
 ***********************************************************/

static int32_t tvservice_send_command(  uint32_t command, void *buffer, uint32_t length, uint32_t has_reply) {
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                  {buffer, length} };
   int32_t success = 0;
   TV_GENERAL_RESP_T response;
   lock_obtain();
   success = vchi_msg_queuev( tvservice_client.client_handle[0],
                              vector, sizeof(vector)/sizeof(vector[0]),
                              VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
   vcos_assert( success == 0 );
   if(success == 0 && has_reply) {
      //otherwise only wait for a reply if we ask for one
      success = tvservice_wait_for_reply(&response, sizeof(response));
      response.ret = VC_VTOH32(response.ret);
   } else {
      //No reply expected or failed to send, send the success code back instead
      response.ret = success;
   }
   vcos_assert(success == 0);
   lock_release();
   return response.ret;
}

/***********************************************************
 * Name: tvservice_send_command_reply
 *
 * Arguments: command, parameter buffer, parameter legnth, reply buffer, buffer length
 *
 * Description: send a command and wait for its non-single value reponse (in a buffer)
 *
 * Returns: error code, host app is responsible to do endian translation
 *
 ***********************************************************/
static int32_t tvservice_send_command_reply(  uint32_t command, void *buffer, uint32_t length,
                                              void *response, uint32_t max_length) {
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                   {buffer, length} };

   int32_t success = 0;
   lock_obtain();
   success = vchi_msg_queuev( tvservice_client.client_handle[0],
                               vector, sizeof(vector)/sizeof(vector[0]),
                               VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
   vcos_assert(!success);
   if(success == 0) {
      success = tvservice_wait_for_reply(response, max_length);
   }
   vcos_assert(success == 0);
   lock_release();

   return success;
}

/***********************************************************
 * Name: tvservice_notify_func
 *
 * Arguments: TV service state
 *
 * Description: This is the notification task which receives all TV
 *              service notifications
 *
 * Returns: does not return
 *
 ***********************************************************/
static void tvservice_notify_func( unsigned int argc, void *argv ) {
   int32_t success;
   TVSERVICE_HOST_STATE_T *state = (TVSERVICE_HOST_STATE_T *) argv;

   //first send a dummy message to the Videocore to let it know we are ready
   tvservice_send_command(VC_TV_END_OF_LIST, NULL, 0, 0);

   while(1) {
      success = os_semaphore_obtain(&tvservice_notify_available_semaphore);
      vcos_assert(!success && state->initialised);

      do {
         uint32_t reason, param1, param2;
         //Get all notifications in the queue
         success = vchi_msg_dequeue( state->notify_handle[0], state->notify_buffer, sizeof(state->notify_buffer), &state->notify_length, VCHI_FLAGS_NONE );
         if(success != 0 || state->notify_length < sizeof(uint32_t)*3 ) {
            continue;
         }

         lock_obtain();
         
         //Check what notification it is and update ourselves accordingly before notifying the host app
         //All notifications are of format: reason, param1, param2 (all 32-bit unsigned int)
         reason = VC_VTOH32(state->notify_buffer[0]), param1 = VC_VTOH32(state->notify_buffer[1]), param2 = VC_VTOH32(state->notify_buffer[2]);
         switch(reason) {
         case VC_HDMI_UNPLUGGED:
            if(state->current_tv_state & (VC_HDMI_HDMI|VC_HDMI_DVI|VC_HDMI_STANDBY)) {
               state->copy_protect = 0;
            }
            state->current_tv_state &= ~(VC_HDMI_HDMI|VC_HDMI_DVI|VC_HDMI_STANDBY|VC_HDMI_HDCP_AUTH);
            state->current_tv_state |= (VC_HDMI_UNPLUGGED | VC_HDMI_HDCP_UNAUTH);
            state->cea_cache_valid = 0;
            state->dmt_cache_valid = 0;
            break;
            
         case VC_HDMI_STANDBY:
            if(state->current_tv_state & (VC_HDMI_HDMI|VC_HDMI_DVI)) {
               state->copy_protect = 0;
            }
            state->current_tv_state &=  ~(VC_HDMI_HDMI|VC_HDMI_DVI|VC_HDMI_UNPLUGGED|VC_HDMI_HDCP_AUTH);
            state->current_tv_state |= VC_HDMI_STANDBY;
            state->hdmi_preferred_group = (HDMI_RES_GROUP_T) param1;
            state->hdmi_preferred_mode = param2;
            break;
            
         case VC_HDMI_DVI:
            state->current_tv_state &= ~(VC_HDMI_HDMI|VC_HDMI_STANDBY|VC_HDMI_UNPLUGGED);
            state->current_tv_state |= VC_HDMI_DVI;
            state->hdmi_current_mode = HDMI_MODE_DVI;
            state->hdmi_current_group = (HDMI_RES_GROUP_T) param1;
            state->hdmi_current_mode = param2;
            break;
            
         case VC_HDMI_HDMI:
            state->current_tv_state &= ~(VC_HDMI_DVI|VC_HDMI_STANDBY|VC_HDMI_UNPLUGGED);
            state->current_tv_state |= VC_HDMI_HDMI;
            state->hdmi_current_mode = HDMI_MODE_HDMI;
            state->hdmi_current_group = (HDMI_RES_GROUP_T) param1;
            state->hdmi_current_mode = param2;
            break;
            
         case VC_HDMI_HDCP_UNAUTH:
            state->current_tv_state &= ~VC_HDMI_HDCP_AUTH;
            state->current_tv_state |= VC_HDMI_HDCP_UNAUTH;
            state->copy_protect = 0;
            //Do we care about the reason for HDCP unauth in param1?
            break;
            
         case VC_HDMI_HDCP_AUTH:
            state->current_tv_state &= ~VC_HDMI_HDCP_UNAUTH;
            state->current_tv_state |= VC_HDMI_HDCP_AUTH;
            state->copy_protect = 1;
            break;
            
         case VC_HDMI_HDCP_KEY_DOWNLOAD:
         case VC_HDMI_HDCP_SRM_DOWNLOAD:
            //Nothing to do here, just tell the host app whether it is successful or not (in param1)
            break;
            
         case VC_SDTV_UNPLUGGED: //Currently we don't get this
            if(state->current_tv_state & (VC_SDTV_PAL | VC_SDTV_NTSC)) {
               state->copy_protect = 0;
            }
            state->current_tv_state &= ~(VC_SDTV_STANDBY | VC_SDTV_PAL | VC_SDTV_NTSC);
            state->current_tv_state |= (VC_SDTV_UNPLUGGED | VC_SDTV_CP_INACTIVE);
            state->sdtv_current_mode = SDTV_MODE_OFF;
            break;
            
         case VC_SDTV_STANDBY: //Currently we don't get this either
            state->current_tv_state &= ~(VC_SDTV_UNPLUGGED | VC_SDTV_PAL | VC_SDTV_NTSC);
            state->current_tv_state |= VC_SDTV_STANDBY;
            state->sdtv_current_mode = SDTV_MODE_OFF;
            break;
            
         case VC_SDTV_NTSC:
            state->current_tv_state &= ~(VC_SDTV_UNPLUGGED | VC_SDTV_STANDBY | VC_SDTV_PAL);
            state->current_tv_state |= VC_SDTV_NTSC;
            state->sdtv_current_mode = (SDTV_MODE_T) param1;
            state->sdtv_options.aspect = (SDTV_ASPECT_T) param2;
            if(param1 & SDTV_COLOUR_RGB) {
               state->sdtv_current_colour = SDTV_COLOUR_RGB;
            } else if(param1 & SDTV_COLOUR_YPRPB) {
               state->sdtv_current_colour = SDTV_COLOUR_YPRPB;
            } else {
               state->sdtv_current_colour = SDTV_COLOUR_UNKNOWN;
            }
            break;
            
         case VC_SDTV_PAL:
            state->current_tv_state &= ~(VC_SDTV_UNPLUGGED | VC_SDTV_STANDBY | VC_SDTV_NTSC);
            state->current_tv_state |= VC_SDTV_PAL;
            state->sdtv_current_mode = (SDTV_MODE_T) param1;
            state->sdtv_options.aspect = (SDTV_ASPECT_T) param2;
            if(param1 & SDTV_COLOUR_RGB) {
               state->sdtv_current_colour = SDTV_COLOUR_RGB;
            } else if(param1 & SDTV_COLOUR_YPRPB) {
               state->sdtv_current_colour = SDTV_COLOUR_YPRPB;
            } else {
               state->sdtv_current_colour = SDTV_COLOUR_UNKNOWN;
            }
            break;
            
         case VC_SDTV_CP_INACTIVE:
            state->current_tv_state &= ~VC_SDTV_CP_ACTIVE;
            state->current_tv_state |= VC_SDTV_CP_INACTIVE;
            state->copy_protect = 0;
            state->sdtv_current_cp_mode = SDTV_CP_NONE;
            break;
            
         case VC_SDTV_CP_ACTIVE:
            state->current_tv_state &= ~VC_SDTV_CP_INACTIVE;
            state->current_tv_state |= VC_SDTV_CP_ACTIVE;
            state->copy_protect = 1;
            state->sdtv_current_cp_mode = (SDTV_CP_MODE_T) param1;
            break;
         }
         
         lock_release();
         
         //Now callback the host app
         if(state->notify_fn) {
            (*state->notify_fn)(state->notify_data, reason, param1, param2);
         }
      } while(success == 0 && state->notify_length >= sizeof(uint32_t)*3); //read the next message if any
   } //while (1)
}

/***********************************************************
 Actual TV service API starts here
***********************************************************/

/***********************************************************
 * Name: vc_tv_get_state
 *
 * Arguments:
 *       Pointer to tvstate structure
 *
 * Description: Get the current TV state
 *
 * Returns: if the command is successful (zero) or not (non-zero)
 *          If the command fails to be sent, passed in state is unchanged
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_get_state(TV_GET_STATE_RESP_T *tvstate) {
   int success = -1;
   if(vcos_verify(tvstate)) {
      success = tvservice_send_command_reply( VC_TV_GET_STATE, NULL, 0, 
                                              tvstate, sizeof(TV_GET_STATE_RESP_T));
      if(success == 0) {
         tvstate->state = VC_VTOH32(tvstate->state);
         tvstate->width = VC_VTOH32(tvstate->width);
         tvstate->height = VC_VTOH32(tvstate->height);
         tvstate->frame_rate = VC_VTOH16(tvstate->frame_rate);
         tvstate->scan_mode = VC_VTOH16(tvstate->scan_mode);
         
         lock_obtain();
         tvservice_client.current_tv_state = tvstate->state;
         lock_release();
      }
   }
   return success;
}

/***********************************************************
 * Name: vc_tv_hdmi_power_on_preferred
 *
 * Arguments:
 *       none
 *
 * Description: Power on HDMI at preferred resolution
 *              Analogue TV will be powered down if on (same for the following
 *              two HDMI power on functions below)
 * 
 * Returns: single value interpreted as HDMI_RESULT_T (zero means success)
 *          if successful, there will be a callback when the power on is complete
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_power_on_preferred( void ) {
   return (int) tvservice_send_command( VC_TV_HDMI_ON_PREFERRED, NULL, 0, 1);
}

/***********************************************************
 * Name: vc_tv_hdmi_power_on_best
 *
 * Arguments:
 *       screen width, height, frame rate, scan_mode (HDMI_NONINTERLACED / HDMI_INTERLACED)
 *       match flags
 *
 * Description: Power on HDMI at best matched resolution based on passed in parameters
 *
 * Returns: single value interpreted as HDMI_RESULT_T (zero means success)
 *          if successful, there will be a callback when the power on is complete
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_power_on_best(uint32_t width, uint32_t height, uint32_t frame_rate, 
                                              HDMI_INTERLACED_T scan_mode, EDID_MODE_MATCH_FLAG_T match_flags) {
   TV_HDMI_ON_BEST_PARAM_T param;
   int success;
   param.width = VC_HTOV32(width);
   param.height = VC_HTOV32(height);
   param.frame_rate = VC_HTOV32(frame_rate);
   param.scan_mode = VC_HTOV32(scan_mode);
   param.match_flags = VC_HTOV32(match_flags);
   
   success = tvservice_send_command( VC_TV_HDMI_ON_BEST, &param, sizeof(TV_HDMI_ON_BEST_PARAM_T), 1);
   return success;
}

/***********************************************************
 * Name: vc_tv_hdmi_power_on_explicit
 *
 * Arguments:
 *       mode (HDMI_MODE_HDMI/HDMI_MODE_DVI), group (HDMI_RES_GROUP_CEA/HDMI_RES_GROUP_DMT), code
 *
 * Description: Power on HDMI at explicit mode
 *              If Videocore has EDID, this will still be subject to EDID restriction,
 *              otherwise HDMI will be powered on at the said mode
 *
 * Returns: single value interpreted as HDMI_RESULT_T (zero means success)
 *          if successful, there will be a callback when the power on is complete
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_power_on_explicit(HDMI_MODE_T mode, HDMI_RES_GROUP_T group, uint32_t code) {
   TV_HDMI_ON_EXPLICIT_PARAM_T param;
   int success;
   param.hdmi_mode = VC_HTOV32(mode);
   param.group = VC_HTOV32(group);
   param.mode = VC_HTOV32(code);

   success = tvservice_send_command( VC_TV_HDMI_ON_EXPLICIT, &param, sizeof(TV_HDMI_ON_EXPLICIT_PARAM_T), 1);
   return success;
}

/***********************************************************
 * Name: vc_tv_sdtv_power_on
 *
 * Arguments:
 *       SDTV mode, options (currently only aspect ratio)
 *
 * Description: Power on SDTV at required mode and aspect ratio (default 4:3)
 *              HDMI will be powered down if currently on
 *
 * Returns: single value (zero means success)
 *          if successful, there will be a callback when the power on is complete
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_sdtv_power_on(SDTV_MODE_T mode, SDTV_OPTIONS_T *options) {
   TV_SDTV_ON_PARAM_T param;
   int success;
   param.mode = VC_HTOV32(mode);
   param.aspect = (options)? VC_HTOV32(options->aspect) : VC_HTOV32(SDTV_ASPECT_4_3);

   success = tvservice_send_command( VC_TV_SDTV_ON, &param, sizeof(TV_SDTV_ON_PARAM_T), 1);
   return success;
}

/***********************************************************
 * Name: vc_tv_power_off
 *
 * Arguments:
 *       none
 *
 * Description: Power off whatever is on at the moment, no effect if nothing is on
 *
 * Returns: whether command is succcessfully sent (and callback for HDMI)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_power_off( void ) {
   return tvservice_send_command( VC_TV_OFF, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_tv_hdmi_get_supported_modes
 *
 * Arguments:
 *       group (HDMI_RES_GROUP_CEA/HDMI_RES_GROUP_DMT), array of TV_SUPPORT_MODE_T struct
 *       length of array, pointer to preferred group, pointer to prefer mode code
 *
 * Description: Get supported modes for a particular standard, always return the preferred
 *              resolution, the length of array limits no. of modes returned
 *
 * Returns: Returns the number of modes actually written
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_get_supported_modes(HDMI_RES_GROUP_T group, 
                                                    TV_SUPPORTED_MODE_T *supported_modes,
                                                    int max_supported_modes,
                                                    HDMI_RES_GROUP_T *preferred_group,
                                                    uint32_t *preferred_mode) {
   TV_QUERY_SUPPORTED_MODES_PARAM_T param = {VC_HTOV32(group)};
   TV_QUERY_SUPPORTED_MODES_RESP_T *response = NULL;

   int success = 0;
   int modes_copied = 0;

   vcos_assert(supported_modes && max_supported_modes);

   //See if we have cached it already, if so, reply with the cache, otherwise fetch it from VC
   if(group == HDMI_RES_GROUP_CEA && tvservice_client.cea_cache_valid) {
      modes_copied = _min(max_supported_modes, tvservice_client.cea_cache.num_supported_modes);
      memcpy(supported_modes, tvservice_client.cea_cache.supported_modes, modes_copied*sizeof(TV_SUPPORTED_MODE_T));
   } else if(group == HDMI_RES_GROUP_DMT && tvservice_client.dmt_cache_valid) {
      modes_copied = _min(max_supported_modes, tvservice_client.num_dmt_modes);
      memcpy(supported_modes, tvservice_client.dmt_supported_modes, modes_copied*sizeof(TV_SUPPORTED_MODE_T));
   } else { //We ask Videocore
      //response needs to be 16 bytes aligned
      response = os_malloc(sizeof(TV_QUERY_SUPPORTED_MODES_RESP_T), 16, "VC_TV response");

      if(vcos_verify(response)) {
         success = tvservice_send_command( VC_TV_QUERY_SUPPORTED_MODES, &param, sizeof(TV_QUERY_SUPPORTED_MODES_PARAM_T), 0);
         if(success == 0) {
            success = tvservice_wait_for_bulk_receive(response, sizeof(TV_QUERY_SUPPORTED_MODES_RESP_T));
         }
      } else {
         success = -1;
      }
   
      if(success == 0) {
         int i;
         response->num_supported_modes = VC_VTOH32(response->num_supported_modes);
         //Update our own cache
         if(group == HDMI_RES_GROUP_CEA) {
            tvservice_client.cea_cache_valid = 1;
            for(i = 0; i < response->num_supported_modes; i++) {
               tvservice_client.cea_cache.supported_modes[i].scan_mode  = VC_VTOH16(response->supported_modes[i].scan_mode);
               tvservice_client.cea_cache.supported_modes[i].native     = VC_VTOH16(response->supported_modes[i].native);
               tvservice_client.cea_cache.supported_modes[i].code       = VC_VTOH16(response->supported_modes[i].code);
               tvservice_client.cea_cache.supported_modes[i].frame_rate = VC_VTOH16(response->supported_modes[i].frame_rate);
               tvservice_client.cea_cache.supported_modes[i].width      = VC_VTOH16(response->supported_modes[i].width);
               tvservice_client.cea_cache.supported_modes[i].height     = VC_VTOH16(response->supported_modes[i].height);
            }
            tvservice_client.cea_cache.num_supported_modes = response->num_supported_modes;
            tvservice_client.cea_cache.preferred_group     = VC_VTOH32(response->preferred_group);
            tvservice_client.cea_cache.preferred_mode      = VC_VTOH32(response->preferred_mode);
            modes_copied = _min(max_supported_modes, tvservice_client.cea_cache.num_supported_modes);
            memcpy(supported_modes, tvservice_client.cea_cache.supported_modes, modes_copied*sizeof(TV_SUPPORTED_MODE_T));
         } else if(group == HDMI_RES_GROUP_DMT) {
            tvservice_client.dmt_cache_valid = 1;
            for(i = 0; i < response->num_supported_modes; i++) {
               tvservice_client.dmt_supported_modes[i].scan_mode  = VC_VTOH16(response->supported_modes[i].scan_mode);
               tvservice_client.dmt_supported_modes[i].native     = VC_VTOH16(response->supported_modes[i].native);
               tvservice_client.dmt_supported_modes[i].code       = VC_VTOH16(response->supported_modes[i].code);
               tvservice_client.dmt_supported_modes[i].frame_rate = VC_VTOH16(response->supported_modes[i].frame_rate);
               tvservice_client.dmt_supported_modes[i].width      = VC_VTOH16(response->supported_modes[i].width);
               tvservice_client.dmt_supported_modes[i].height     = VC_VTOH16(response->supported_modes[i].height);
            }
            tvservice_client.num_dmt_modes = response->num_supported_modes;
            modes_copied = _min(max_supported_modes, tvservice_client.num_dmt_modes);
            memcpy(supported_modes, tvservice_client.dmt_supported_modes, modes_copied*sizeof(TV_SUPPORTED_MODE_T));
         }
         tvservice_client.hdmi_preferred_group = VC_VTOH32(response->preferred_group);
         tvservice_client.hdmi_preferred_mode  = VC_VTOH32(response->preferred_mode);
      }

      if(response) {
         os_free(response);
      }
   }

   if(preferred_group && preferred_mode) {
      *preferred_group = tvservice_client.hdmi_preferred_group;
      *preferred_mode = tvservice_client.hdmi_preferred_mode;
   }
   return modes_copied;
}

/***********************************************************
 * Name: vc_tv_hdmi_mode_supported
 *
 * Arguments:
 *       resolution standard (CEA/DMT), mode code
 *
 * Description: Query if a particular mode is supported
 *
 * Returns: single value return > 0 means supported, 0 means unsupported, < 0 means error
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_mode_supported(HDMI_RES_GROUP_T group,
                                               uint32_t mode) {
   TV_QUERY_MODE_SUPPORT_PARAM_T param = {VC_HTOV32(group), VC_HTOV32(mode)};
   return tvservice_send_command( VC_TV_QUERY_MODE_SUPPORT, &param, sizeof(TV_QUERY_MODE_SUPPORT_PARAM_T), 1);
}

/***********************************************************
 * Name: vc_tv_hdmi_audio_supported
 *
 * Arguments:
 *       audio format (EDID_AudioFormat + EDID_AudioCodingExtension), 
 *       no. of channels (1-8), 
 *       sample rate (EDID_AudioSampleRate except "refer to header"), 
 *       bit rate (or sample size if pcm)
 *       use EDID_AudioSampleSize as sample size argument
 *
 * Description: Query if a particular audio format is supported
 *
 * Returns: single value return which will be flags in EDID_AUDIO_SUPPORT_FLAG_T
 *          zero means everything is supported, < 0 means error
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_audio_supported(uint32_t audio_format, uint32_t num_channels, 
                                                EDID_AudioSampleRate fs, uint32_t bitrate) {
   TV_QUERY_AUDIO_SUPPORT_PARAM_T param = { VC_HTOV32(audio_format),
                                            VC_HTOV32(num_channels),
                                            VC_HTOV32(fs),
                                            VC_HTOV32(bitrate) };
  
  vcos_assert(num_channels > 0 && num_channels <= 8);
  vcos_assert(fs != EDID_AudioSampleRate_eReferToHeader);

  return tvservice_send_command( VC_TV_QUERY_AUDIO_SUPPORT, &param, sizeof(TV_QUERY_AUDIO_SUPPORT_PARAM_T), 1);
}

/***********************************************************
 * Name: vc_tv_enable_copyprotect
 *
 * Arguments:
 *       copy protect mode (only used for SDTV), time out in milliseconds
 *
 * Description: Enable copy protection (either HDMI or SDTV must be powered on)
 *
 * Returns: single value return 0 means success, additional result via callback
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_enable_copyprotect(uint32_t cp_mode, uint32_t timeout) {
   TV_ENABLE_COPY_PROTECT_PARAM_T param = {VC_HTOV32(cp_mode), VC_HTOV32(timeout)};
   return tvservice_send_command( VC_TV_ENABLE_COPY_PROTECT, &param, sizeof(TV_ENABLE_COPY_PROTECT_PARAM_T), 1);
}

/***********************************************************
 * Name: vc_tv_disable_copyprotect
 *
 * Arguments:
 *       none
 *
 * Description: Disable copy protection (either HDMI or SDTV must be powered on)
 *
 * Returns: single value return 0 means success, additional result via callback
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_disable_copyprotect( void ) {
   return tvservice_send_command( VC_TV_DISABLE_COPY_PROTECT, NULL, 0, 1);
}

/***********************************************************
 * Name: vc_tv_show_info
 *
 * Arguments:
 *       show (1) or hide (0) info screen
 *
 * Description: Show or hide info screen, only works in HDMI at the moment
 *
 * Returns: zero if command is successfully sent
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_show_info(uint32_t show) {
   TV_SHOW_INFO_PARAM_T param = {VC_HTOV32(show)};
   return tvservice_send_command( VC_TV_SHOW_INFO, &param, sizeof(TV_SHOW_INFO_PARAM_T), 0);
}

/***********************************************************
 * Name: vc_tv_hdmi_get_av_latency
 *
 * Arguments:
 *       none
 *
 * Description: Get the AV latency (in ms) for HDMI (lipsync), only valid if
 *              HDMI is currently powered on, otherwise you get zero
 *
 * Returns: latency (zero if error or latency is not defined), < 0 if failed to send command)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_get_av_latency( void ) {
   return tvservice_send_command( VC_TV_GET_AV_LATENCY, NULL, 0, 1);
}

/***********************************************************
 * Name: vc_tv_hdmi_set_hdcp_key
 *
 * Arguments:
 *       key block, whether we wait (1) or not (0) for the key to download
 *
 * Description: Download HDCP key 
 *
 * Returns: single value return indicating download status 
 *          (or queued status if we don't wait)
 *          Callback indicates the validity of key
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_set_hdcp_key(const uint8_t *key) {
   vcos_assert(key);
   TV_HDCP_SET_KEY_PARAM_T param;
   memcpy(param.key, key, HDCP_KEY_BLOCK_SIZE);
   return tvservice_send_command( VC_TV_HDCP_SET_KEY, &param, sizeof(param), 0);
}

/***********************************************************
 * Name: vc_tv_hdmi_set_hdcp_revoked_list
 *
 * Arguments:
 *       list, size of list
 *
 * Description: Download HDCP revoked list
 *
 * Returns: single value return indicating download status 
 *          (or queued status if we don't wait)
 *          Callback indicates the number of keys set (zero if failed, unless you are clearing the list)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_set_hdcp_revoked_list(const uint8_t *list, uint32_t num_keys) {
   TV_HDCP_SET_SRM_PARAM_T param = {VC_HTOV32(num_keys)};
   int success = tvservice_send_command( VC_TV_HDCP_SET_SRM, &param, sizeof(TV_HDCP_SET_SRM_PARAM_T), 0);
   if(success == 0 && num_keys && list) { //Set num_keys to zero if we are clearing the list
      //Sent the command, now download the list
      lock_obtain();
      success = vchi_bulk_queue_transmit( tvservice_client.client_handle[0],
                                          list,
                                          num_keys * HDCP_KSV_LENGTH,
                                          VCHI_FLAGS_BLOCK_UNTIL_QUEUED,
                                          0 );
      lock_release();
   }
   return success;
}

/***********************************************************
 * Name: vc_tv_hdmi_set_spd
 *
 * Arguments:
 *       manufacturer, description, product type (HDMI_SPD_TYPE_CODE_T)
 *
 * Description: Set SPD
 *
 * Returns: whether command was sent successfully (zero means success)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_set_spd(const char *manufacturer, const char *description, HDMI_SPD_TYPE_CODE_T type) {
   TV_SET_SPD_PARAM_T param;
   vcos_assert(manufacturer && description);
   memcpy(param.manufacturer, manufacturer, TV_SPD_NAME_LEN);
   memcpy(param.description, description, TV_SPD_DESC_LEN);
   param.type = VC_HTOV32(type);
   return tvservice_send_command( VC_TV_SET_SPD, &param, sizeof(TV_SET_SPD_PARAM_T), 0);
}

/***********************************************************
 * Name: vc_tv_hdmi_set_display_options
 *
 * Arguments:
 *       aspect ratio (HDMI_ASPECT_T enum), left/right bar width, top/bottom bar height
 *
 * Description: Set active area for HDMI (bar width/height should be set to zero if absent)
 *
 * Returns: whether command was sent successfully (zero means success)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_set_display_options(HDMI_ASPECT_T aspect, 
                                                    uint32_t left_bar_width, uint32_t right_bar_width, 
                                                    uint32_t top_bar_height, uint32_t bottom_bar_height,
                                                    uint32_t overscan_flags) {
   TV_SET_DISPLAY_OPTIONS_PARAM_T param;
   param.aspect = VC_HTOV32(aspect);
   param.vertical_bar_present = VC_HTOV32((left_bar_width || right_bar_width)? VC_TRUE : VC_FALSE);
   param.left_bar_width = VC_HTOV32(left_bar_width);
   param.right_bar_width = VC_HTOV32(right_bar_width);
   param.horizontal_bar_present = VC_HTOV32((top_bar_height || bottom_bar_height)? VC_TRUE : VC_FALSE);
   param.top_bar_height = VC_HTOV32(top_bar_height);
   param.bottom_bar_height = VC_HTOV32(bottom_bar_height);
   param.overscan_flags = VC_HTOV32(overscan_flags);
   return tvservice_send_command( VC_TV_SET_DISPLAY_OPTIONS, &param, sizeof(TV_SET_DISPLAY_OPTIONS_PARAM_T), 0);
}

/***********************************************************
 * Name: vc_tv_test_mode_start
 *
 * Arguments:
 *       24-bit colour, test mode (TV_TEST_MODE_T enum)
 *
 * Description: Power on HDMI to test mode, HDMI must be off to start with
 *
 * Returns: whether command was sent successfully (zero means success)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_test_mode_start(uint32_t colour, TV_TEST_MODE_T test_mode) {
   TV_TEST_MODE_START_PARAM_T param = {VC_HTOV32(colour), VC_HTOV32(test_mode)};

   return tvservice_send_command( VC_TV_TEST_MODE_START, &param, sizeof(TV_TEST_MODE_START_PARAM_T), 0);
}

/***********************************************************
 * Name: vc_tv_test_mode_stop
 *
 * Arguments:
 *       none
 *
 * Description: Stop test mode and power down HDMI
 *
 * Returns: whether command was sent successfully (zero means success)
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_test_mode_stop( void ) {
   return tvservice_send_command( VC_TV_TEST_MODE_STOP, NULL, 0, 0);
}

/***********************************************************
 * Name: vc_tv_hdmi_ddc_read
 *
 * Arguments:
 *       offset, length to read, pointer to buffer, must be 16 byte aligned
 *
 * Description: ddc read over i2c (HDMI only at the moment)
 *
 * Returns: length of data read (so zero means error) and the buffer will be filled
 *          only if no error
 *
 ***********************************************************/
VCHPRE_ int VCHPOST_ vc_tv_hdmi_ddc_read(uint32_t offset, uint32_t length, uint8_t *buffer) {
   int success;
   TV_DDC_READ_PARAM_T param = {VC_HTOV32(offset), VC_HTOV32(length)};

   vcos_assert(buffer && (((uint32_t) buffer) % 16) == 0);
   success = tvservice_send_command( VC_TV_DDC_READ, &param, sizeof(TV_DDC_READ_PARAM_T), 1);

   if(success == 0) {
      success = tvservice_wait_for_bulk_receive(buffer, length);
   }
   return (success == 0)? length : 0; //Either return the whole block or nothing
}
