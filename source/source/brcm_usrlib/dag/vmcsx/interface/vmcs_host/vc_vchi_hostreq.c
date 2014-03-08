/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Software host interface (host-side)
File     :  $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_hostreq.c#3 $
Revision :  $Revision: #3 $

FILE DESCRIPTION
Hosteq service (VCHI native).
=============================================================================*/
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "vchost_config.h"
#include "vchost.h"
#include "vchostreq.h"
#include "vc_hostreq_defs.h"

#include "interface/vchi/vchi.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/os/os.h"
#include "applications/vmcs/hostreq/hostreqserv.h"

#include "vcfw/drivers/device/pmu.h"

/******************************************************************************
Local types and defines.
******************************************************************************/
//Hostreq service state
typedef struct {
   VCHI_SERVICE_HANDLE_T client_handle[VCHI_MAX_NUM_CONNECTIONS];
   uint8_t               response_buffer[HOSTREQ_FIFO_SIZE];
   uint32_t              response_length;
   void *                message_handle;
   int                   num_connections;
   OS_SEMAPHORE_T        sema;
   VC_HRNOTIFY_CALLBACK_T notify_callbacks[ VC_HRNOTIFY_END ];
   int32_t               initialised;
} HOSTREQ_SERVICE_T;

/******************************************************************************
Static data.
******************************************************************************/
static HOSTREQ_SERVICE_T hostreq_client;
static OS_SEMAPHORE_T    hostreq_client_message_available;
static VCOS_THREAD_T       hostreq_client_task;

/* 2727DK specific data so that time can be retrieved from the PMU */
static const PMU_DRIVER_T *pmu_driver = 0;
static DRIVER_HANDLE_T    pmu_handle;

/******************************************************************************
Static functions.
******************************************************************************/
//Lock the host state
static void lock_obtain (void) {
   assert(hostreq_client.initialised);
   assert(!os_semaphore_obtained(&hostreq_client.sema));
   int success = os_semaphore_obtain(&hostreq_client.sema);
   assert(success >= 0);
}

//Unlock the host state
static void lock_release (void) {
   assert(hostreq_client.initialised);
   assert(os_semaphore_obtained(&hostreq_client.sema));
   int32_t success = os_semaphore_release(&hostreq_client.sema);
   assert( success >= 0 );
}

//Send replies/commands across to VMCS
static int32_t hostreq_send_command(uint32_t command, void *buffer, uint32_t length) {
   int success = 0, i;
   VCHI_MSG_VECTOR_T vector[] = { {&command, sizeof(command)},
                                  {buffer, length} };

   assert(hostreq_client.initialised);
   lock_obtain();
   for( i=0; i<hostreq_client.num_connections; i++ ) {
      success = vchi_msg_queuev(hostreq_client.client_handle[i],
                                vector,
                                sizeof(vector)/sizeof(vector[0]),
                                VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
      assert(!success);
      if(success != 0)
         break;
   }
   lock_release();
   return success;

}

time_t vchi_hostreq_time(void)
{
   int32_t failure;
   PMU_RTC_T rtc;
   time_t t = 0;
   int y, m, days;

   /* if needed get a handle to the PMU & open */
   if(!pmu_driver) {
      pmu_driver = PMU_DRIVER_GET_FUNC_TABLE();

      /* open handle to the driver */
      if(pmu_driver) {
         failure = pmu_driver->open(0, &pmu_handle);
         assert(!failure);

         if(failure)
            pmu_driver = 0;
      }
   }

   /* if the pmu driver failed to open */
   if(pmu_driver) {
      /* try to get the time from the RTC */
      failure = pmu_driver->get_rtc(pmu_handle, &rtc);
      assert(!failure);

      /* convert to time_t time */
      if(!failure) {
         /* calculate the days since 0 AD */
         m = (rtc.month + 9) % 12;
         y = rtc.year - m / 10;
         days = 365 * y + y/4 - y/100 + y/400 + (m*306 + 5) / 10 + (rtc.calender_day) - 1;
         /* bias days to be relative to 1st Jan 1970 */
         days -= 719468;
         t = ((((days * 24) + rtc.hours) * 60) + rtc.minutes) * 60 + rtc.seconds;
      }
   }

   return t;
}

//Request handler
static void hostreq_request_handler(int inum, HOSTREQ_SERVICE_T *state){
   assert(state->response_length);
   uint32_t *data = (uint32_t*)state->response_buffer;
   uint32_t command = *data;
   int32_t success = 0;
   data ++;
   state->response_length -= sizeof(uint32_t);

   switch(command) {
   case VC_HOSTREQ_TIME:
      {
         //VC asks for time
         //HOST specific implementation
         //We just return zero for time for now
         time_t dummytime = vchi_hostreq_time();
         int success = hostreq_send_command(VC_HOSTREQ_TIME, &dummytime, sizeof(dummytime));
         assert(success == 0);
      break;
      }
   case VC_HOSTREQ_NOTIFY:
      {
         //Host notify
         assert(state->response_length == 2*sizeof(uint32_t));
         VC_HRNOTIFY_T reason = (VC_HRNOTIFY_T) *data;
         data ++;
         uint32_t param = *data;
         if(hostreq_client.notify_callbacks[reason]) {
            (*hostreq_client.notify_callbacks[reason])(reason, param);
         }

         break;
      }
   case VC_HOSTREQ_READMEM:
      {
         //params are hostaddr, len, vc addr
         uint32_t *param = data;
         assert(param[0]);
         success = vchi_bulk_queue_transmit(hostreq_client.client_handle[0],
                                            (void *)param[0],
                                            param[1],
                                            VCHI_FLAGS_NONE,
                                            NULL);
         assert(success == 0);
         break;
      }
   case VC_HOSTREQ_WRITEMEM:
      {
         //params are hostaddr, len, vc addr
         uint32_t *param = data;
         assert(param[0]);
         success = vchi_bulk_queue_receive(hostreq_client.client_handle[0],
                                           (void *)param[0],
                                           param[1],
                                           VCHI_FLAGS_NONE,
                                           NULL);
         assert(success == 0);
         break;
      }

   default:
      assert(!"unhandled request");
   }
}

//Receiver thread
static void hostreqclient_func(unsigned int argc, void *argv) {
   int32_t success;
   int32_t i, nothing;

   while(1) { //Run forever waiting for a callback to happen
      // wait for the semaphore to say that there is a message
      success = os_semaphore_obtain(&hostreq_client_message_available);
      assert( success == 0 );

      while(1) { //Read until we can read no more on all interfaces
         nothing = 0;
         for(i=0;i<hostreq_client.num_connections;i++) {
            success = vchi_msg_dequeue(hostreq_client.client_handle[i], hostreq_client.response_buffer, sizeof(hostreq_client.response_buffer), &hostreq_client.response_length, VCHI_FLAGS_NONE);
            if(success == 0) {
               hostreq_request_handler(i, &hostreq_client);
            } else {
               nothing++;
            }
         }
         //When there is nothing on all interfaces, wait for a callback
         if(nothing == hostreq_client.num_connections) {
            break;
         }
      }
   }
}

/***********************************************************
 * Name: hostreq_client_callback
 *
 * Arguments: semaphore, callback reason and message handle
 *
 * Description: VCHI callback for the HOSTREQ service
 *
 ***********************************************************/
static void hostreq_client_callback( void *callback_param,
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
vc hostreq API
******************************************************************************/
//Not used, call vc_vchi_hostreq_init instead
VCHPRE_ int VCHPOST_ vc_hostreq_init( void ){
   assert(0);
   return 0;
}

/******************************************************************************
NAME
   vc_vchi_hostreq_init

SYNOPSIS
   void vc_vchi_hostreq_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections )

FUNCTION
   Initialise hostreq service

RETURNS
   error code
******************************************************************************/

void vc_vchi_hostreq_init (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections ) {
   int success, i;

   // record the number of connections
   memset( &hostreq_client, 0, sizeof(HOSTREQ_SERVICE_T) );
   hostreq_client.num_connections = num_connections;
   success = os_semaphore_create( &hostreq_client.sema, OS_SEMAPHORE_TYPE_SUSPEND );
   assert( success == 0 );
   success = os_semaphore_create( &hostreq_client_message_available, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   assert( success == 0 );
   success = os_semaphore_obtain(&hostreq_client_message_available);
   assert( success == 0 );

   for (i=0; i<hostreq_client.num_connections; i++) {

      // Create a 'Client' service on the each of the connections
      SERVICE_CREATION_T hostreq_parameters = { HOSTREQ_NAME,     // 4cc service code
                                                connections[i],           // passed in fn ptrs
                                                0,                        // tx fifo size (unused)
                                                0,                        // tx fifo size (unused)
                                                &hostreq_client_callback, // service callback
                                                &hostreq_client_message_available }; // callback parameter

      success = vchi_service_open( initialise_instance, &hostreq_parameters, &hostreq_client.client_handle[i] );
      assert( success == 0 );


   }

   success = os_thread_start(&hostreq_client_task, &hostreqclient_func, NULL, 2048, "HOSTREQ task");
   assert(!success);
   if(!success) {
      hostreq_client.initialised = 1;
      //Send a time across immediately
      time_t dummytime = 0; //We need to get the real time on a real host
      success = hostreq_send_command(VC_HOSTREQ_TIME, &dummytime, sizeof(dummytime));
      assert( success == 0 );
   }
}

/******************************************************************************
NAME
   vc_hostreq_stop

SYNOPSIS
   void vc_hostreq_stop()

FUNCTION
   This tells us that the hostreq service has stopped, thereby preventing
   any of the functions from doing anything.

RETURNS
   void
******************************************************************************/

void vc_hostreq_stop () {
   // Don't want anyone to be using this when we zap it.
   int i, success;
   lock_obtain();
   for(i = 0; i < hostreq_client.num_connections; i++) {
      success = vchi_service_close(hostreq_client.client_handle[i]);
      assert(success == 0);
   }
   lock_release();
   memset(&hostreq_client, 0, sizeof(hostreq_client));

}

/******************************************************************************
NAME
   vc_hostreq_inum

SYNOPSIS
   int vc_hostreq_inum()

FUNCTION
   Obsolete

RETURNS
   int
******************************************************************************/

int vc_hostreq_inum () {
   return -1;
}

/******************************************************************************
NAME
   vc_hostreq_set_notify

SYNOPSIS
   int32_t vc_hostreq_set_notify( const VC_HRNOTIFY_T notify_event, VC_HRNOTIFY_CALLBACK_T notify_callback )

FUNCTION
   Sets a user notify function to be called for the given notify event

RETURNS
   0 on success, < 0 on failure (-1 invalid event, -2 hostreq not initialised)
******************************************************************************/
int32_t vc_hostreq_set_notify( const VC_HRNOTIFY_T notify_event, VC_HRNOTIFY_CALLBACK_T notify_callback )
{
   if (!hostreq_client.initialised)
      return -2;

   if ( (notify_event > VC_HRNOTIFY_START) && (notify_event < VC_HRNOTIFY_END) )
   {
      hostreq_client.notify_callbacks[notify_event] = notify_callback;
   }
   else
      return -1;
   return 0;
}

/******************************************************************************
NAME
   vchostreq_writemem

SYNOPSIS
   VC_RESP_CODE_T vchostreq_writemem( void* host_addr, void *vc_addr, int len, int channel )

FUNCTION
   Writes a block of data from Videocore into the host
   memory area specified by host_addr
   Obsolete now

RETURNS
   Success  : 0 - all the data was copied
   Fail     : 1 - error in input parameters or operation
******************************************************************************/
int vchostreq_writemem( void* host_addr, void *vc_addr, int len, int channel /* not used */ ) {
   uint32_t param[] = {(uint32_t) host_addr, len, (uint32_t) vc_addr};
   int success = hostreq_send_command(VC_HOSTREQ_WRITEMEM, param, sizeof(param));
   assert(success == 0);
   assert(0); // should not be calling this function
   success =  vchi_bulk_queue_receive(hostreq_client.client_handle[0],
                                      host_addr,
                                      len,
                                      VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
                                      NULL);
   assert(success == 0);
   return (success == 0) ? 0 : 1;
}

/******************************************************************************
NAME
   vchostreq_readmem

SYNOPSIS
   VC_RESP_CODE_T vchostreq_readmem( void* host_addr, void *vc_addr, int len )

FUNCTION
   Allows Videocore to read a block of memory from a
   memory area on the host specified by host_addr
   Obsolete now

RETURNS
   Success  : 0 - all the data was copied
   Fail     : 1 - error in input parameters or operation
******************************************************************************/
int vchostreq_readmem( void* host_addr, void *vc_addr, int len ) {
   uint32_t param[] = {(uint32_t) host_addr, len, (uint32_t) vc_addr};
   int success = hostreq_send_command(VC_HOSTREQ_READMEM, param, sizeof(param));
   assert(success == 0);
   assert(0); // should not be calling this function
   success =  vchi_bulk_queue_transmit(hostreq_client.client_handle[0],
                                       host_addr,
                                       len,
                                       VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE,
                                       NULL);
   assert(success == 0);
   return (success == 0) ? 0 : 1;
}
