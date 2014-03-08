/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Service creation for vchiq_test.

==============================================================================*/

#include "vcfw/logging/logging.h"

#include "vchiq_test.h"

#define MAX_SERVICES 8

typedef struct service_struct {
   VCHIQ_SERVICE_HANDLE_T handle;
   char name[5];
   char *bulk_data;
   int bulk_size;
   int iters;
   int count;
} SERVICE_T;

static SERVICE_T g_services[MAX_SERVICES];

static VCHIQ_INSTANCE_T srvr_instance;
static VCHIQ_SERVICE_HANDLE_T func_service_handle;
static VCHIQ_SERVICE_HANDLE_T fun2_service_handle;
static char srvr_service1_data[SERVICE1_DATA_SIZE];
static char srvr_service2_data[SERVICE2_DATA_SIZE];

static VCHIQ_STATUS_T srvr_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                                    VCHIQ_SERVICE_HANDLE_T handle, void *bulk_userdata)
{
   SERVICE_T *service = (SERVICE_T *)VCHIQ_GET_SERVICE_USERDATA(handle);
   switch (reason)
   {
   case VCHIQ_SERVICE_OPENED:
      VCOS_TRACE("  Service '%s' opened", service->name);
      break;
   case VCHIQ_SERVICE_CLOSED:
      VCOS_TRACE("  Service '%s' closed", service->name);
      free(service->bulk_data);
      service->bulk_data = NULL;
      break;
   case VCHIQ_MESSAGE_AVAILABLE:
      {
         VCHIQ_ELEMENT_T element;

         /* Reconfiguring the server with a new size */
         if (header->size != 8)
            element.data = "Wrong message length";
         else
         {
            if (service->bulk_data)
                free(service->bulk_data);
            service->bulk_size = ((int *)header->data)[0];
            service->bulk_data = malloc(service->bulk_size);
            if (!service->bulk_data)
               element.data = "Out of memory";
            else
            {
               service->iters = ((int *)header->data)[1];
               service->count = 0;
               vchiq_queue_bulk_receive(handle, service->bulk_data, service->bulk_size, 0);
               element.data = NULL;
            }
         }
         element.size = element.data ? (strlen(element.data) + 1) : 0;
         vchiq_queue_message(handle, &element, 1);
         vchiq_release_message(handle, header);
      }
      break;
   case VCHIQ_BULK_RECEIVE_DONE:
      //VCOS_TRACE("  BULK_RECEIVE_DONE(%s,%s)", service->name, service->bulk_data);
      if (vchiq_queue_bulk_transmit(handle, service->bulk_data, service->bulk_size, bulk_userdata) != VCHIQ_SUCCESS)
      {
         VCOS_TRACE("    transmit failed - aborting");
      }
      break;
   case VCHIQ_BULK_TRANSMIT_DONE:
      //VCOS_TRACE("  BULK_TRANSMIT_DONE(%s,%s)", service->name, service->bulk_data);
      vcos_assert(service->count == (int)bulk_userdata);
      service->count++;
      if (service->count != service->iters)
         vchiq_queue_bulk_receive(handle, service->bulk_data,
                                  service->bulk_size, (void *)service->count);
      break;
   case VCHIQ_BULK_RECEIVE_ABORTED:
      VCOS_TRACE("  BULK_RECEIVE_ABORTED(%s,?)", service->name);
      break;
   case VCHIQ_BULK_TRANSMIT_ABORTED:
      VCOS_TRACE("  BULK_TRANSMIT_ABORTED(%s,%s)", service->name, service->bulk_data);
      break;
   default:
      VCOS_TRACE("  BULK_TRANSMIT_ABORTED(%s,%s)", service->name, service->bulk_data);
      break;
   }

   return VCHIQ_SUCCESS;
}

static VCHIQ_STATUS_T func_srvr_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                                        VCHIQ_SERVICE_HANDLE_T service, void *bulk_userdata)
{
   static VCHIQ_SERVICE_HANDLE_T func_service2_handle;
   static int callback_count = 0, bulk_count = 0;
   int callback_index = 0, bulk_index = 0;

   if (reason < VCHIQ_BULK_TRANSMIT_DONE)
   {
      callback_count++;

      START_CALLBACK(VCHIQ_SERVICE_OPENED, 1)
      // Reject this one for fun
      END_CALLBACK(VCHIQ_ERROR)

      START_CALLBACK(VCHIQ_SERVICE_OPENED, 1)
      EXPECT(vchiq_add_service(srvr_instance, FUNC_FOURCC, func_srvr_callback, (void *)2, &func_service2_handle), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_SERVICE_OPENED, 2)
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_SERVICE_CLOSED, 2)
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_SERVICE_OPENED, 2)
      EXPECT(vchiq_queue_bulk_transmit(service, srvr_service2_data, sizeof(srvr_service2_data), (void *)0x201), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_MESSAGE_AVAILABLE, 1)
      {
         VCHIQ_ELEMENT_T element;
         element.data = header->data;
         element.size = header->size;
         EXPECT(vchiq_queue_message(service, &element, 1), VCHIQ_SUCCESS);
      }
      vchiq_release_message(service, header);
      EXPECT(vchiq_queue_bulk_receive(service, srvr_service1_data, sizeof(srvr_service1_data), (void *)0x101), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_SERVICE_CLOSED, 1)
      EXPECT(vchiq_remove_service(service), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_CALLBACK(VCHIQ_SERVICE_CLOSED, 2)
      /* Reset for the next time */
      EXPECT(vchiq_remove_service(service), VCHIQ_SUCCESS);
      EXPECT(vchiq_add_service(srvr_instance, FUNC_FOURCC, func_srvr_callback, (void*)1, &func_service_handle), VCHIQ_SUCCESS);
      callback_count = 0;
      bulk_count = 0;
      END_CALLBACK(VCHIQ_SUCCESS)
   }
   else
   {
      bulk_count++;

      START_BULK_CALLBACK(VCHIQ_BULK_RECEIVE_DONE, 1, 0x101)
      memcpy(srvr_service2_data, srvr_service1_data, sizeof(srvr_service1_data));
      memcpy(srvr_service2_data + sizeof(srvr_service1_data), srvr_service1_data, sizeof(srvr_service1_data));
      EXPECT(vchiq_queue_bulk_transmit(service, srvr_service2_data, sizeof(srvr_service1_data), (void *)0x102), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_BULK_CALLBACK(VCHIQ_BULK_TRANSMIT_ABORTED, 1, 0x102)
      EXPECT(vchiq_queue_bulk_transmit(service, srvr_service2_data, sizeof(srvr_service2_data), (void *)0x103), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_BULK_CALLBACK(VCHIQ_BULK_TRANSMIT_DONE, 1, 0x103)
      EXPECT(vchiq_queue_bulk_receive(service, srvr_service1_data, sizeof(srvr_service1_data), (void *)0x104), VCHIQ_SUCCESS);
      EXPECT(vchiq_queue_message(service, NULL, 0), VCHIQ_SUCCESS);
      END_CALLBACK(VCHIQ_SUCCESS)

      START_BULK_CALLBACK(VCHIQ_BULK_RECEIVE_ABORTED, 1, 0x104)
      END_CALLBACK(VCHIQ_SUCCESS)

      START_BULK_CALLBACK(VCHIQ_BULK_TRANSMIT_ABORTED, 2, 0x201)
      END_CALLBACK(VCHIQ_SUCCESS)
   }

error_exit:
   callback_count = 0;
   bulk_count = 0;

   return VCHIQ_ERROR;
}

static VCHIQ_STATUS_T fun2_srvr_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header,
                                         VCHIQ_SERVICE_HANDLE_T service, void *bulk_userdata)
{
   static int datalen = 0;
#ifdef USE_MEMMGR
   static MEM_HANDLE_T memhandle = MEM_HANDLE_INVALID;
   uint8_t *databuf;
#else
   static uint8_t databuf[32 + FUN2_MAX_DATA_SIZE + 32 ];
#endif
   uint8_t *prologue, *data, *epilogue;
   int success;
   int i;

   switch (reason)
   {
   case VCHIQ_SERVICE_OPENED:
   case VCHIQ_SERVICE_CLOSED:
      break;

   case VCHIQ_MESSAGE_AVAILABLE:
      vcos_assert(header->size == sizeof(int));
      datalen = ((int *)header->data)[0];
      vcos_assert(datalen <= FUN2_MAX_DATA_SIZE);
      vchiq_release_message(service, header);
#ifdef USE_MEMMGR
      if (memhandle == MEM_HANDLE_INVALID)
      {
         memhandle = mem_alloc(32 + FUN2_MAX_DATA_SIZE + 32, 32, MEM_FLAG_COHERENT, "fun2_srvr_databuf");
         vcos_assert(memhandle != MEM_HANDLE_INVALID);
      }
      databuf = mem_lock(memhandle);
#endif
      memset(databuf, 0xff, 32 + datalen + 32);
#ifdef USE_MEMMGR
      mem_unlock(memhandle);
      vchiq_queue_bulk_receive_handle(service, memhandle, (void *)32, datalen, 0);
#else
      vchiq_queue_bulk_receive(service, data, datalen, 0);
#endif
      break;

   case VCHIQ_BULK_RECEIVE_DONE:
      success = 1;
#ifdef USE_MEMMGR
      databuf = mem_lock(memhandle);
#endif
      prologue = databuf;
      data = prologue + 32;
      epilogue = data + datalen;
      for (i = 0; i < 32; i++)
      {
         if (prologue[i] != '\xff')
         {
            VCOS_TRACE("Prologue corrupted at %d (datalen %d)", i, datalen);
            VCOS_BKPT;
            success = 0;
            break;
         }
         if (epilogue[i] != '\xff')
         {
            VCOS_TRACE("Epilogue corrupted at %d (datalen %d)", i, datalen);
            VCOS_BKPT;
            success = 0;
            break;
         }
      }

      if (success)
      {
         for (i = 0; i < (datalen - 1); i++)
         {
            if ((i & 0x1f) == 0)
            {
               if (data[i] != (uint8_t)(i >> 5))
               {
                  success = 0;
                  break;
               }
            }
            else if (data[i] != (uint8_t)i)
            {
               success = 0;
               break;
            }
         }
         if (success && (databuf[32 + i] != 0))
            success = 0;
      }

      if (!success)
      {
         VCOS_TRACE("Data corrupted at %d (datalen %d)", i, datalen);
         VCOS_BKPT;
      }
#ifdef USE_MEMMGR
      mem_unlock(memhandle);
      vchiq_queue_bulk_transmit_handle(service, memhandle, (void *)32, datalen, 0);
#else
      vchiq_queue_bulk_transmit(service, data, datalen, 0);
#endif
      break;

   case VCHIQ_BULK_TRANSMIT_DONE:
      break;

   default:
      vcos_assert(0);
      break;
   }

   return VCHIQ_SUCCESS;
}

void vchiq_test_start_services(VCHIQ_INSTANCE_T instance)
{
   int i;

   srvr_instance = instance;

   // Client set-up
   for (i = 0; i < MAX_SERVICES; i++)
   {
      SERVICE_T *service = &g_services[i];
      int fourcc;
      strcpy(service->name, "echo");
      fourcc = VCHIQ_MAKE_FOURCC(service->name[0], service->name[1], service->name[2], service->name[3]);
      service->bulk_data = NULL;
      vchiq_add_service(srvr_instance, fourcc, srvr_callback, service, &service->handle);
   }
   vchiq_add_service(srvr_instance, FUNC_FOURCC, func_srvr_callback, (void*)1, &func_service_handle);
   vchiq_add_service(srvr_instance, FUN2_FOURCC, fun2_srvr_callback, NULL, &fun2_service_handle);
}
