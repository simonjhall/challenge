/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
User-space library frontend for Linux VCHIQ driver.

==============================================================================*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>

#include "vchiq.h"
#include "vchiq_ioctl.h"
#include "interface/vcos/vcos.h"

#define VCHIQ_MAX_SERVICES 32

#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))

#define RETRY(r,x) do { r = x; } while ((r == -1) && (errno == EINTR))

typedef struct vchiq_service_struct
{
   VCHIQ_SERVICE_BASE_T base;
   int handle;
   int fd;
} VCHIQ_SERVICE_T;

static struct vchiq_instance_struct
{
   int fd;
   int initialised;
   int connected;
   void *map;
   VCOS_THREAD_T completion_thread;
   VCOS_MUTEX_T mutex;
   VCHIQ_SERVICE_T services[VCHIQ_MAX_SERVICES];
} vchiq_instance;

static void *vchiq_completion_thread(void *);

static int __inline is_valid_instance(VCHIQ_INSTANCE_T instance)
{
   return (instance == &vchiq_instance) && instance->initialised;
}

VCHIQ_STATUS_T vchiq_initialise(VCHIQ_INSTANCE_T *pinstance)
{
   VCHIQ_INSTANCE_T instance = &vchiq_instance;

   vcos_global_lock();

   if (instance->initialised == 0)
   {
      instance->fd = open("/dev/vchiq", O_RDWR);
      if (instance->fd >= 0)
      {
         instance->map = mmap(0, VCHIQ_CHANNEL_SIZE, PROT_READ, MAP_SHARED, instance->fd, 0);
         if (instance->map >= 0)
         {
            int i;
            for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
            {
               instance->services[i].handle = VCHIQ_INVALID_HANDLE;
            }
            vcos_mutex_create(&instance->mutex, "VCHIQ instance");
            instance->initialised = 1;
         }
         else
         {
            close(instance->fd);
            instance = NULL;
         }
      }
      else
      {
         instance = NULL;
      }
   }

   vcos_global_unlock();

   *pinstance = instance;
   return (instance != NULL) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_shutdown(VCHIQ_INSTANCE_T instance)
{
   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->mutex);

   if (instance->initialised == 1)
   {
      int i;

      instance->initialised = -1; /* Enter limbo */

      /* Remove all services */

      for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      {
         if (instance->services[i].handle != VCHIQ_INVALID_HANDLE)
         {
            vchiq_remove_service(&instance->services[i].base);
            instance->services[i].handle = VCHIQ_INVALID_HANDLE;
         }
      }

      if (instance->connected)
      {
         int ret;
         RETRY(ret, ioctl(instance->fd, VCHIQ_IOC_SHUTDOWN, 0));
         vcos_assert(ret == 0);
         vcos_thread_join(&instance->completion_thread, NULL);
         instance->connected = 0;
      }

      munmap(instance->map, VCHIQ_CHANNEL_SIZE);
      close(instance->fd);
      instance->fd = -1;
   }

   vcos_mutex_unlock(&instance->mutex);

   vcos_global_lock();

   if (instance->initialised == -1)
   {
      vcos_mutex_delete(&instance->mutex);
      instance->initialised = 0;
   }

   vcos_global_unlock();

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_connect(VCHIQ_INSTANCE_T instance)
{
   VCHIQ_STATUS_T status;

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->mutex);

   if (!instance->connected)
   {
      status = ioctl(instance->fd, VCHIQ_IOC_CONNECT, 0);
      if (vcos_verify(status == 0))
      {
         VCOS_THREAD_ATTR_T attrs;
         instance->connected = 1;
         vcos_thread_attr_init(&attrs);
         vcos_thread_create(&instance->completion_thread, "VCHIQ completion",
                            &attrs, vchiq_completion_thread, instance);
      }
   }

   vcos_mutex_unlock(&instance->mutex);

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_add_service(VCHIQ_INSTANCE_T instance, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata, VCHIQ_SERVICE_HANDLE_T *pservice)
{
   VCHIQ_SERVICE_T *service = NULL;
   int ret = -1;
   int i;

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->mutex);

   /* Find a free service */
   for (i = (VCHIQ_MAX_SERVICES - 1); i >= 0; i--)
   {
      VCHIQ_SERVICE_T *srv = &instance->services[i];
      if (srv->handle == VCHIQ_INVALID_HANDLE)
      {
         service = srv;
      }
      else if ((srv->base.fourcc == fourcc) &&
               (srv->base.callback != callback))
      {
         /* There is another server using this fourcc which doesn't match */
         service = NULL;
         break;
      }
   }

   if (service)
   {
      VCHIQ_ADD_SERVICE_T args;
      service->base.fourcc = fourcc;
      service->base.callback = callback;
      service->base.userdata = userdata;
      args.fourcc = fourcc;
      args.service_userdata = service;
      args.handle = -1; /* OUT parameter */
      RETRY(ret, ioctl(instance->fd, VCHIQ_IOC_ADD_SERVICE, &args));
      service->handle = args.handle;
   }
   if (ret >= 0)
   {
      service->fd = instance->fd;
      *pservice = &service->base;
   }
   else
   {
      *pservice = NULL;
   }

   vcos_mutex_unlock(&instance->mutex);

   return vcos_verify(ret >= 0) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_open_service(VCHIQ_INSTANCE_T instance, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata, VCHIQ_SERVICE_HANDLE_T *pservice)
{
   VCHIQ_SERVICE_T *service = NULL;
   int ret = -1;
   int i;

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->mutex);

   /* Find a free service */
   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
   {
      if (instance->services[i].handle == VCHIQ_INVALID_HANDLE)
      {
         service = &instance->services[i];
         break;
      }
   }

   if (service)
   {
      VCHIQ_OPEN_SERVICE_T args;
      service->fd = instance->fd;
      service->base.fourcc = fourcc;
      service->base.callback = callback;
      service->base.userdata = userdata;
      args.fourcc = fourcc;
      args.service_userdata = service;
      args.handle = -1; /* OUT parameter */
      RETRY(ret, ioctl(instance->fd, VCHIQ_IOC_OPEN_SERVICE, &args));
      service->handle = args.handle;
   }
   if (ret >= 0)
   {
      service->fd = instance->fd;
      *pservice = &service->base;
   }
   else
   {
      *pservice = NULL;
   }

   vcos_mutex_unlock(&instance->mutex);

   return vcos_verify(ret >= 0) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_remove_service(VCHIQ_SERVICE_HANDLE_T handle)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   int ret;
   RETRY(ret,ioctl(service->fd, VCHIQ_IOC_REMOVE_SERVICE, service->handle));

   if (!vcos_verify(ret == 0))
      return VCHIQ_ERROR;

   service->handle = VCHIQ_INVALID_HANDLE;
   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_queue_message(VCHIQ_SERVICE_HANDLE_T handle, const VCHIQ_ELEMENT_T *elements, int count)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_QUEUE_MESSAGE_T args;
   int ret;
   args.handle = service->handle;
   args.elements = elements;
   args.count = count;
   RETRY(ret, ioctl(service->fd, VCHIQ_IOC_QUEUE_MESSAGE, &args));

   return vcos_verify(ret >= 0) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

void vchiq_release_message(VCHIQ_SERVICE_HANDLE_T handle, VCHIQ_HEADER_T *header)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_RELEASE_MESSAGE_T args;
   int ret;
   args.handle = service->handle;
   args.header = header;
   RETRY(ret, ioctl(service->fd, VCHIQ_IOC_RELEASE_MESSAGE, &args));

   vcos_assert(ret == 0);
}

VCHIQ_STATUS_T vchiq_queue_bulk_transmit(VCHIQ_SERVICE_HANDLE_T handle, const void *data, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_QUEUE_BULK_TRANSMIT_T args;
   int ret;
   args.handle = service->handle;
   args.data = data;
   args.size = size;
   args.userdata = userdata;
   RETRY(ret, ioctl(service->fd, VCHIQ_IOC_QUEUE_BULK_TRANSMIT, &args));

   return vcos_verify(ret >= 0) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_queue_bulk_receive(VCHIQ_SERVICE_HANDLE_T handle, void *data, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_QUEUE_BULK_RECEIVE_T args;
   int ret;
   args.handle = service->handle;
   args.data = data;
   args.size = size;
   args.userdata = userdata;
   RETRY(ret, ioctl(service->fd, VCHIQ_IOC_QUEUE_BULK_RECEIVE, &args));

   return vcos_verify(ret >= 0) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_queue_bulk_transmit_handle(VCHIQ_SERVICE_HANDLE_T handle, VCHI_MEM_HANDLE_T memhandle, void *offset, int size, void *userdata)
{
   vcos_assert(memhandle == VCHI_MEM_HANDLE_INVALID);

   return vchiq_queue_bulk_transmit(handle, offset, size, userdata);
}

VCHIQ_STATUS_T vchiq_queue_bulk_receive_handle(VCHIQ_SERVICE_HANDLE_T handle, VCHI_MEM_HANDLE_T memhandle, void *offset, int size, void *userdata)
{
   vcos_assert(memhandle == VCHI_MEM_HANDLE_INVALID);

   return vchiq_queue_bulk_receive(handle, offset, size, userdata);
}

static void *vchiq_completion_thread(void *arg)
{
   VCHIQ_INSTANCE_T instance = (VCHIQ_INSTANCE_T)arg;
   VCHIQ_AWAIT_COMPLETION_T args;
   VCHIQ_COMPLETION_DATA_T completions[8];

   args.count = ARRAY_LEN(completions);
   args.buf = completions;

   while (1)
   {
      int ret, i;

      RETRY(ret, ioctl(instance->fd, VCHIQ_IOC_AWAIT_COMPLETION, &args));
         
      if (ret <= 0)
         break;

      for (i = 0; i < ret; i++)
      {
         VCHIQ_COMPLETION_DATA_T *completion = &completions[i];
         VCHIQ_SERVICE_BASE_T *service = (VCHIQ_SERVICE_BASE_T *)completion->service_userdata;
         (*service->callback)(completion->reason, completion->header,
                              service,
                              completion->bulk_userdata);
      }
   }
   return NULL;
}
