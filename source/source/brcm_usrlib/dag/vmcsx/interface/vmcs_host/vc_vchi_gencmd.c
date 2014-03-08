/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Host Request Service (host-side)
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_gencmd.c#4 $
Revision :  $Revision: #4 $

FILE DESCRIPTION
General command service using VCHI
=============================================================================*/
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

#include "vchost.h"

#include "vc_vchi_gencmd.h"
#include "interface/vchi/vchi.h"
#include "interface/vchi/common/endian.h"
#include "interface/vchi/os/os.h"
#include "interface/vmcs_host/vc_gencmd_defs.h"

/******************************************************************************
Local types and defines.
******************************************************************************/
#define GENCMD_MAX_LENGTH 512
typedef struct {
   VCHI_SERVICE_HANDLE_T open_handle[VCHI_MAX_NUM_CONNECTIONS];
   uint32_t              msg_flag[VCHI_MAX_NUM_CONNECTIONS];
   char                  command_buffer[GENCMD_MAX_LENGTH+1];
   char                  response_buffer[GENCMDSERVICE_MSGFIFO_SIZE];
   uint32_t              response_length;  //Length of response minus the error code
   int                   num_connections;
   OS_SEMAPHORE_T        sema;
} GENCMD_SERVICE_T;

static GENCMD_SERVICE_T gencmd_client;
static OS_SEMAPHORE_T gencmd_message_available_semaphore;


/******************************************************************************
Static function.
******************************************************************************/
static void gencmd_callback( void *callback_param,
                             VCHI_CALLBACK_REASON_T reason,
                             void *msg_handle );

static void lock_obtain (void) {
   int32_t success;
   assert(!os_semaphore_obtained(&gencmd_client.sema));
   success = os_semaphore_obtain( &gencmd_client.sema );
   assert(success >= 0);
}
static void lock_release (void) {
   int32_t success;
   assert(os_semaphore_obtained(&gencmd_client.sema));
   success = os_semaphore_release( &gencmd_client.sema );
   assert( success >= 0 );
}

//call vc_vchi_gencmd_init to initialise
int vc_gencmd_init() {
   assert(0);
   return 0;
}

/******************************************************************************
NAME
   vc_vchi_gencmd_init

SYNOPSIS
   void vc_vchi_gencmd_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections )

FUNCTION
   Initialise the general command service for use. A negative return value
   indicates failure (which may mean it has not been started on VideoCore).

RETURNS
   int
******************************************************************************/

void vc_vchi_gencmd_init (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int32_t success;
   int i;

   // record the number of connections
   memset( &gencmd_client, 0, sizeof(GENCMD_SERVICE_T) );
   gencmd_client.num_connections = (int) num_connections;
   success = os_semaphore_create( &gencmd_client.sema, OS_SEMAPHORE_TYPE_SUSPEND );
   assert( success == 0 );
   success = os_semaphore_create( &gencmd_message_available_semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   assert( success == 0 );
   success = os_semaphore_obtain( &gencmd_message_available_semaphore );
   assert( success == 0 );

   for (i=0; i<gencmd_client.num_connections; i++) {

      // Create a 'LONG' service on the each of the connections
      SERVICE_CREATION_T gencmd_parameters = { MAKE_FOURCC("GCMD"),      // 4cc service code
                                               connections[i],           // passed in fn ptrs
                                               0,                        // tx fifo size (unused)
                                               0,                        // tx fifo size (unused)
                                               &gencmd_callback,         // service callback
                                               &gencmd_message_available_semaphore, // callback parameter
                                               VC_FALSE,                 // want_unaligned_bulk_rx
                                               VC_FALSE,                 // want_unaligned_bulk_tx
                                               VC_FALSE                  // want_crc
                                               };

      success = vchi_service_open( initialise_instance, &gencmd_parameters, &gencmd_client.open_handle[i] );
      assert( success == 0 );
   }

}

/******************************************************************************
NAME
   gencmd_callback

SYNOPSIS
   void gencmd_callback( void *callback_param,
                     const VCHI_CALLBACK_REASON_T reason,
                     const void *msg_handle )

FUNCTION
   VCHI callback

RETURNS
   int
******************************************************************************/
static void gencmd_callback( void *callback_param,
                             const VCHI_CALLBACK_REASON_T reason,
                             void *msg_handle ) {

   OS_SEMAPHORE_T *sem = (OS_SEMAPHORE_T *)callback_param;

   if ( reason != VCHI_CALLBACK_MSG_AVAILABLE )
      return;

   if ( sem == NULL )
      return;

   if ( os_semaphore_obtained(sem) ) {
      int32_t success = os_semaphore_release( sem );
      assert( success >= 0 );
   }

}

/******************************************************************************
NAME
   vc_gencmd_stop

SYNOPSIS
   int vc_gencmd_stop()

FUNCTION
   This tells us that the generak command service has stopped, thereby preventing
   any of the functions from doing anything.

RETURNS
   int
******************************************************************************/

void vc_gencmd_stop () {
   // Assume a "power down" gencmd has been sent and the lock is held. There will
   // be no response so this should be called instead.
   int32_t success,i;

   for(i = 0; i< (int32_t)gencmd_client.num_connections; i++) {
      success = vchi_service_close( gencmd_client.open_handle[i]);
      assert(success == 0);
   }
}

/******************************************************************************
NAME
   vc_gencmd_send_string

SYNOPSIS
   int vc_gencmd_send_string(char *command)

FUNCTION
   Send a string to general command service.
   Return

RETURNS
   int
******************************************************************************/

static int vc_gencmd_send_string(char *command)
{
   int success = 0, i;
   for( i=0; i<gencmd_client.num_connections; i++ ) {
      success += vchi_msg_queue( gencmd_client.open_handle[i],
                                 command,
                                 strlen(command)+1,
                                 VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL );
      assert(!success);
      if(success != 0)
         break;
   }
   lock_release();
   return success;
}

/******************************************************************************
NAME
   vc_gencmd_send

SYNOPSIS
   int vc_gencmd_send( const char *format, ... )

FUNCTION
   Behaves exectly as vc_gencmd_send_string, except allows varargs.

RETURNS
   int
******************************************************************************/
// original function broken into two parts so that a higher level interface
// can be implemented and re-use the code
int vc_gencmd_send_list ( const char *format, va_list a )
{
   int length;

   // Obtain the lock and keep it so no one else can butt in while we await the response.
   lock_obtain();

   if (gencmd_client.open_handle[0] == 0) {
      lock_release();
      return -1;
   }

   length = vsnprintf( gencmd_client.command_buffer, GENCMD_MAX_LENGTH, format, a );

   if ( (length < 0) || (length >= GENCMD_MAX_LENGTH ) )
   {
      assert(0);
      lock_release();
      return -1;
   }

   /* send message back to caller */
   return vc_gencmd_send_string (gencmd_client.command_buffer);
}

int vc_gencmd_send ( const char *format, ... )
{
   va_list a;
   int     rv;

   va_start ( a, format );
   rv = vc_gencmd_send_list( format, a );
   va_end ( a );
   return rv;
}

/******************************************************************************
NAME
   vc_gencmd_read_response

SYNOPSIS
   int vc_gencmd_read_response

FUNCTION
   Block until something comes back

RETURNS
   Error code from dequeue message
******************************************************************************/
int vc_gencmd_read_response (char *response, int maxlen) {
   int i = 0;
   int success = 0;
   int ret_code = 0;
   int32_t sem_ok = 0;

   //Note this will ALWAYS reset response buffer and overwrite any partially read responses

   do {
      //TODO : we need to deal with messages coming through on more than one connections properly
      //At the moment it will always try to read the first connection if there is something there
      for(i = 0; i < gencmd_client.num_connections; i++) {
         //Check if there is something in the queue, if so return immediately
         //otherwise wait for the semaphore and read again
         success = (int) vchi_msg_dequeue( gencmd_client.open_handle[i], gencmd_client.response_buffer, sizeof(gencmd_client.response_buffer), &gencmd_client.response_length, VCHI_FLAGS_NONE);
         if(success == 0) {
            ret_code = VC_VTOH32( *(int *)gencmd_client.response_buffer );
            break;
         } else {
            gencmd_client.response_length = 0;
         }
      }
   } while(!gencmd_client.response_length && (sem_ok = os_semaphore_obtain( &gencmd_message_available_semaphore)) == 0);

   if(gencmd_client.response_length && sem_ok == 0) {
      gencmd_client.response_length -= sizeof(int); //first word is error code
      memcpy(response, gencmd_client.response_buffer+sizeof(int), (size_t) OS_MIN((int)gencmd_client.response_length, (int)maxlen));
   }
   // If we read anything, return the VideoCore code. Error codes < 0 mean we failed to
   // read anything...
   //How do we let the caller know the response code of gencmd?
   //return ret_code;
   return success;
}

/******************************************************************************
NAME
   vc_gencmd

SYNOPSIS
   int vc_gencmd(char *response, int maxlen, const char *format, ...)

FUNCTION
   Send a gencmd and receive the response as per vc_gencmd read_response.

RETURNS
   int
******************************************************************************/
int vc_gencmd(char *response, int maxlen, const char *format, ...) {
   va_list args;
   int ret;

   va_start(args, format);
   ret = vc_gencmd_send_list(format, args);
   va_end (args);

   if (ret < 0)
      return ret;

   return vc_gencmd_read_response(response, maxlen);
}

/******************************************************************************
NAME
   vc_gencmd_string_property

SYNOPSIS
   int vc_gencmd_string_property(char *text, char *property, char **value, int *length)

FUNCTION
   Given a text string, containing items of the form property=value,
   look for the named property and return the value. The start of the value
   is returned, along with its length. The value may contain spaces, in which
   case it is enclosed in double quotes. The double quotes are not included in
   the return parameters. Return non-zero if the property is found.

RETURNS
   int
******************************************************************************/

int vc_gencmd_string_property(char *text, const char *property, char **value, int *length) {
#define READING_PROPERTY 0
#define READING_VALUE 1
#define READING_VALUE_QUOTED 2
   int state = READING_PROPERTY;
   int delimiter = 1, match = 0, len = (int)strlen(property);
   char *prop_start=text, *value_start=text;
   for (; *text; text++) {
      int ch = *text;
      switch (state) {
      case READING_PROPERTY:
         if (delimiter) prop_start = text;
         if (isspace(ch)) delimiter = 1;
         else if (ch == '=') {
            delimiter = 1;
            match = (text-prop_start==len && strncmp(prop_start, property, (size_t)(text-prop_start))==0);
            state = READING_VALUE;
         }
         else delimiter = 0;
         break;
      case READING_VALUE:
         if (delimiter) value_start = text;
         if (isspace(ch)) {
            if (match) goto success;
            delimiter = 1;
            state = READING_PROPERTY;
         }
         else if (delimiter && ch == '"') {
            delimiter = 1;
            state = READING_VALUE_QUOTED;
         }
         else delimiter = 0;
         break;
      case READING_VALUE_QUOTED:
         if (delimiter) value_start = text;
         if (ch == '"') {
            if (match) goto success;
            delimiter = 1;
            state = READING_PROPERTY;
         }
         else delimiter = 0;
         break;
      }
   }
   if (match) goto success;
   return 0;
success:
   *value = value_start;
   *length = text - value_start;
   return 1;
}

/******************************************************************************
NAME
   vc_gencmd_number_property

SYNOPSIS
   int vc_gencmd_number_property(char *text, char *property, int *number)

FUNCTION
   Given a text string, containing items of the form property=value,
   look for the named property and return the numeric value. If such a numeric
   value is successfully found, return 1; otherwise return 0.

RETURNS
   int
******************************************************************************/

int vc_gencmd_number_property(char *text, const char *property, int *number) {
   char *value, temp;
   int length, retval;
   if (vc_gencmd_string_property(text, property, &value, &length) == 0)
      return 0;
   temp = value[length];
   value[length] = 0;
   retval = sscanf(value, "0x%x", (unsigned int*)number);
   if (retval != 1)
      retval = sscanf(value, "%d", number);
   value[length] = temp;
   return retval;

}

/******************************************************************************
NAME
   vc_gencmd_until

SYNOPSIS
   int vc_gencmd_until(const char *cmd, const char *error_string, int timeout);

FUNCTION
   Sends the command repeatedly, until one of the following situations occurs:
   The specified response string is found within the gencmd response.
   The specified error string is found within the gencmd response.
   The timeout is reached.

   The timeout is a rough value, do not use it for precise timing.

RETURNS
   0 if the requested response was detected.
   1 if the error string is detected or the timeout is reached.
******************************************************************************/
int vc_gencmd_until( char        *cmd,
                     const char  *property,
                     char        *value,
                     const char  *error_string,
                     int         timeout) {
   char response[128];
   int length;
   char *ret_value;

   for (;timeout > 0; timeout -= 10) {
      vc_gencmd(response, (int)sizeof(response), cmd);
      if (strstr(response,error_string)) {
         return 1;
      }
      else if (vc_gencmd_string_property(response, property, &ret_value, &length) &&
               strncmp(value,ret_value,(size_t)length)==0) {
         return 0;
      }
      vc_sleep(10);
   }
   // Timed out
   return 1;
}

