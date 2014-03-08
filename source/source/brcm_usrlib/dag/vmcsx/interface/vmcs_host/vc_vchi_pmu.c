/*==============================================================================
 Copyright (c) 2010 Broadcom Europe Limited.
 All rights reserved.

 Module   :  Power management unit service (host side)
 $Id: //software/vc4/DEV/interface/vmcs_host/vc_vchi_pmu.c#7 $

 FILE DESCRIPTION
 Host-side service providing interaction with the power management unit,
   for handling battery charging and USB connection and disconnection
==============================================================================*/

#include <string.h>

#include "interface/vmcs_host/vc_vchi_pmu.h"
#include "interface/vchi/os/os.h"
#include "applications/vmcs/pmuserv/pmu_server.h"
#include "interface/vcos/vcos_assert.h"

/*******************************************************************************
 Public functions written in this module.
 Define as extern here.
*******************************************************************************/


/*******************************************************************************
 Extern functions (written in other modules).
 Specify through module include files or define specifically as extern.
*******************************************************************************/

//<module level #includes for functions used by this file, inside ”” symbols>

//<explicit extern declarations for functions or data defined in other files>

/*******************************************************************************
 Private typedefs, macros and constants.
*******************************************************************************/

typedef struct
{
   VCHI_SERVICE_HANDLE_T handle;
   OS_SEMAPHORE_T   msg_available;
   OS_SEMAPHORE_T   response_lock;
   VCOS_THREAD_T    thread;
   PMU_MSG_T        message;
} PMU_CLIENT_T;

/*******************************************************************************
 Private functions in this module.
 Define as static.
*******************************************************************************/

static void pmu_callback
   (void *callback_param, const VCHI_CALLBACK_REASON_T reason, void *msg_handle);
static void pmu_task_func(unsigned argc, void *argv);

/*******************************************************************************
 Data segments - const and variable.
*******************************************************************************/
static PMU_CLIENT_T pmu_client;

/*-----------------------------------------------------------------------------*/

VCHPRE_ int32_t VCHPOST_ vc_pmu_enable_usb(uint32_t enable)
// Enable (or disable) USB stack (and high current charging mode)
{
   int32_t success = -1;
   uint8_t msg[2] = {PMU_MSG_ENABLE_USB, 0xFF};

   msg[1] = enable;
   success = vchi_msg_queue(pmu_client.handle, msg, 2, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   vcos_assert(success == 0);

   // Wait to receive response
   success = os_semaphore_obtain(&pmu_client.response_lock);
   vcos_assert(success == 0);

   if(vcos_verify(pmu_client.message.msg_id == PMU_MSG_ENABLE_USB))
      {success = pmu_client.message.data;}

   return success;

} // vc_pmu_enable_usb()

/*-----------------------------------------------------------------------------*/

VCHPRE_ int32_t VCHPOST_ vc_pmu_usb_connected(void)
// Returns true if USB is plugged in
{
   int32_t success = -1;
   uint8_t msg[2] = {PMU_MSG_USB_CONNECTED, 0xFF};
   int32_t usb_connected = 0;

   success = vchi_msg_queue(pmu_client.handle, msg, 2, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   vcos_assert(success == 0);

   // Wait to receive response
   success = os_semaphore_obtain(&pmu_client.response_lock);
   vcos_assert(success == 0);

   if(vcos_verify(pmu_client.message.msg_id == PMU_MSG_USB_CONNECTED))
      {usb_connected = pmu_client.message.data;}

   return usb_connected;

} // vc_pmu_usb_connected()

/*-----------------------------------------------------------------------------*/

VCHPRE_ uint32_t VCHPOST_ vc_pmu_get_battery_status(void)
{
   int32_t success = -1;
   uint8_t msg[2] = {PMU_MSG_GET_BATTERY_STATUS, 0xFF};
   uint32_t battery_status = 0;

   success = vchi_msg_queue(pmu_client.handle, msg, 2, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   vcos_assert(success == 0);

   // Wait to receive response
   success = os_semaphore_obtain(&pmu_client.response_lock);
   vcos_assert(success == 0);

   if(vcos_verify(pmu_client.message.msg_id == PMU_MSG_GET_BATTERY_STATUS))
      {battery_status = pmu_client.message.data;}

   return battery_status;
} // vc_pmu_get_battery_status()

/*******************************************************************************
NAME
   <name of function>

SYNOPSIS
   <function synopsis>

FUNCTION
   <description of arguments, operation and implementation of function>

RETURNS
   <details of data returned by function>
*******************************************************************************/

VCHPRE_ void VCHPOST_ vc_vchi_pmu_init
   (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections)
{
   int32_t success;

   memset(&pmu_client, 0, sizeof(PMU_CLIENT_T));

   vcos_assert(num_connections == 1);

   // Set up message available semaphore
   success = os_semaphore_create(&pmu_client.msg_available, OS_SEMAPHORE_TYPE_BUSY_WAIT);
   vcos_assert(success == 0);
   success = os_semaphore_obtain(&pmu_client.msg_available);
   vcos_assert(success == 0);

   // Set up response lock semaphore
   success = os_semaphore_create(&pmu_client.response_lock, OS_SEMAPHORE_TYPE_BUSY_WAIT);
   vcos_assert(success == 0);
   success = os_semaphore_obtain(&pmu_client.response_lock);
   vcos_assert(success == 0);

   SERVICE_CREATION_T pmu_params =
   {
      PMU_NAME,     // 4cc service code
      connections[0],           // passed in fn ptrs
      0,                        // tx fifo size (unused)
      0,                        // tx fifo size (unused)
      &pmu_callback,            // service callback
      &pmu_client.msg_available,  // callback parameter
      VC_FALSE,                  // want_unaligned_bulk_rx
      VC_FALSE,                  // want_unaligned_bulk_tx
      VC_FALSE,                  // want_crc
   }; // pmu_params

   success = vchi_service_open(initialise_instance, &pmu_params, &pmu_client.handle);
   vcos_assert( success == 0 );

   success = os_thread_start(&pmu_client.thread, pmu_task_func, NULL, 4000, "PMU_HOST");
   vcos_assert( success == 0 );

} // vc_vchi_pmu_init()

/*-----------------------------------------------------------------------------*/

static void pmu_callback
   (void *callback_param, const VCHI_CALLBACK_REASON_T reason, void *msg_handle)
// Callback function for PMU service
{
   int32_t success;
   OS_SEMAPHORE_T *msg_available;

   switch(reason)
   {
      case VCHI_CALLBACK_MSG_AVAILABLE:
         // Message available, so release semaphore
         msg_available = (OS_SEMAPHORE_T *) callback_param;
         vcos_assert(msg_available != NULL);
         if((msg_available != NULL) && (os_semaphore_obtained(msg_available)))
         {
            success = os_semaphore_release(msg_available);
            vcos_assert(success >= 0);
         } // if
         break;
      default:
         ;
   } // switch

} // pmu_callback()

/*-----------------------------------------------------------------------------*/

static void pmu_task_func(unsigned argc, void *argv)
// Main function for PMU thread
{
   int32_t success;
   uint8_t *message;
   uint32_t msg_len;

   for(;;)
   {
      // Wait for the semaphore to say that there's a message
      success = os_semaphore_obtain(&pmu_client.msg_available);
      vcos_assert(success == 0);

      // Peek message
      success = vchi_msg_peek(pmu_client.handle, (void **) &message, &msg_len, VCHI_FLAGS_NONE);
      vcos_assert(success == 0);

      if(success == 0)
      {
         pmu_client.message.msg_id = message[0];
         pmu_client.message.data = message[1];

         // Remove message from queue
         vchi_msg_remove(pmu_client.handle);

         // Signal any pending functions which will be waiting for response
         if(vcos_verify(os_semaphore_obtained(&pmu_client.response_lock)))
         {
            success = os_semaphore_release(&pmu_client.response_lock);
            vcos_assert(success == 0);
         } // if
      } // if
   } // for

} // pmu_task_func()

/*-----------------------------------------------------------------------------*/
