/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
VideoCore frontend to VCHIQ module.

==============================================================================*/

#include "vchiq_core.h"
#include "helpers/dmalib/dmalib.h"

#ifndef VCHIQ_LOCAL
#include "vcfw/drivers/chip/arm.h"
#ifndef ARM_LINUX_MEMBASE
      #error ARM_LINUX_MEMBASE not defined - check your platform
#endif
#endif

typedef struct vchiq_instance_struct
{
   VCHIQ_STATE_T state;
   int connected;
} VCHIQ_INSTANCE_STRUCT_T;

#ifdef VCHIQ_LOCAL

#define NUM_INSTANCES 2
static VCHIQ_CHANNEL_T vchiq_channels[2];

#else /* VCHIQ_LOCAL */

#define NUM_INSTANCES 1

#define PAGE_SIZE 4096
#define CACHE_LINE_SIZE 32
#define PAGELIST_WRITE  0
#define PAGELIST_READ   1
#define PAGELIST_READ_WITH_FRAGMENTS   2

typedef struct pagelist_struct
{
   unsigned int length;
   unsigned short type;
   unsigned short offset;
   unsigned long userdata;
   unsigned long addrs[1];
} PAGELIST_T;

typedef struct fragments_struct
{
   char headbuf[CACHE_LINE_SIZE];
   char tailbuf[CACHE_LINE_SIZE];
} FRAGMENTS_T;

static FRAGMENTS_T *vchiq_fragments;
static const ARM_DRIVER_T *arm_driver;
static DRIVER_HANDLE_T arm_handle;

#endif /* VCHIQ_LOCAL */

static VCHIQ_INSTANCE_STRUCT_T vchiq_instances[NUM_INSTANCES];
static int vchiq_num_instances;

static int is_valid_instance(VCHIQ_INSTANCE_T instance)
{
   int i;
   for (i = 0; i < vchiq_num_instances; i++)
   {
      if (instance == &vchiq_instances[i])
         return instance->state.initialised;
   }
   return 0;
}

VCHIQ_STATUS_T vchiq_initialise(VCHIQ_INSTANCE_T *instance)
{
   VCHIQ_INSTANCE_T inst = NULL;
   int i;

   vcos_global_lock();

#ifdef VCHIQ_LOCAL
   if (vchiq_num_instances < 2)
   {
      vchiq_init_state(&vchiq_instances[vchiq_num_instances].state,
                       &vchiq_channels[vchiq_num_instances],
                       &vchiq_channels[vchiq_num_instances ^ 1]);
      if (vchiq_num_instances == 1)
      {
         /* This state initialisation may have erased a signal - signal anyway
            to be sure. This is a bit of a hack, caused by the desire for the
            server threads to be started on the same core as the calling thread. */
         vcos_event_signal(&vchiq_channels[vchiq_num_instances].trigger.event);
      }
      vchiq_num_instances++;
   }
#endif /* VCHIQ_LOCAL */

   for (i = 0; i < vchiq_num_instances; i++)
   {
      if (!vchiq_instances[i].state.initialised)
      {
         inst = &vchiq_instances[i];
         inst->connected = 0;
         inst->state.id = i;
         inst->state.initialised = 1;
         break;
      }
   }

   vcos_global_unlock();

   *instance = inst;

   return (inst != NULL) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_shutdown(VCHIQ_INSTANCE_T instance)
{
   VCHIQ_CHANNEL_T *local = instance->state.local;
   int i;

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   /* Remove all services */

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
   {
      if (local->services[i].srvstate != VCHIQ_SRVSTATE_FREE)
      {
         vchiq_remove_service(&local->services[i].base);
      }
   }

   /* Release the state/instance */
   instance->state.initialised = 0;

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_connect(VCHIQ_INSTANCE_T instance)
{
   VCHIQ_STATUS_T status;

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->state.mutex);
   status = vchiq_connect_internal(&instance->state, instance);
   if (status == VCHIQ_SUCCESS)
      instance->connected = 1;
   vcos_mutex_unlock(&instance->state.mutex);
   return status;
}

VCHIQ_STATUS_T vchiq_add_service(VCHIQ_INSTANCE_T instance, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata, VCHIQ_SERVICE_HANDLE_T *pservice)
{
   VCHIQ_SERVICE_T *service = NULL;

   vcos_assert(fourcc != VCHIQ_FOURCC_INVALID);
   vcos_assert(callback != NULL);

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   vcos_mutex_lock(&instance->state.mutex);

   service = vchiq_add_service_internal(&instance->state, fourcc, callback, userdata,
                                        instance->connected ? VCHIQ_SRVSTATE_LISTENING :  VCHIQ_SRVSTATE_HIDDEN,
                                        instance);

   vcos_mutex_unlock(&instance->state.mutex);

   *pservice = &service->base;
   return (service != NULL) ? VCHIQ_SUCCESS : VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_open_service(VCHIQ_INSTANCE_T instance, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata, VCHIQ_SERVICE_HANDLE_T *pservice)
{
   VCHIQ_STATE_T *state = &instance->state;
   VCHIQ_SERVICE_T *service;
   VCHIQ_STATUS_T status = VCHIQ_ERROR;

   vcos_assert(fourcc != VCHIQ_FOURCC_INVALID);
   vcos_assert(callback != NULL);

   if (!is_valid_instance(instance))
      return VCHIQ_ERROR;

   if (!instance->connected)
      return VCHIQ_ERROR;

   vcos_mutex_lock(&state->mutex);

   service = vchiq_add_service_internal(state, fourcc, callback, userdata, VCHIQ_SRVSTATE_OPENING, instance);

   vcos_mutex_unlock(&state->mutex);

   if (service)
   {
      status = vchiq_open_service_internal(service);
      if (status != VCHIQ_SUCCESS)
      {
         vchiq_remove_service(&service->base);
         service = NULL;
      }
   }
   *pservice = &service->base;
   return status;
}

#ifdef VCHIQ_LOCAL

VCHIQ_STATUS_T vchiq_copy_from_user(void *dst, const void *src, int size)
{
   if (size > 64)
      dma_memcpy(dst, src, size);
   else
      memcpy(dst, src, size);
   return VCHIQ_SUCCESS;
}

void vchiq_copy_bulk_from_host(void *dst, const void *src, int size)
{
   if (size > 64)
      dma_memcpy(dst, src, size);
   else
      memcpy(dst, src, size);
}

void vchiq_copy_bulk_to_host(void *dst, const void *src, int size)
{
   if (size > 64)
      dma_memcpy(dst, src, size);
   else
      memcpy(dst, src, size);
}

#else /* VCHIQ_LOCAL */

VCHIQ_STATUS_T vchiq_copy_from_user(void *dst, const void *src, int size)
{
   if (size > 64)
      dma_memcpy(dst, src, size);
   else
      memcpy(dst, src, size);
   return VCHIQ_SUCCESS;
}

void vchiq_copy_bulk_from_host(void *dst, const void *src, int size)
{
   PAGELIST_T *pagelist;
   char *vcptr = dst;
   int offset, i;
   int prev_pfn = 0x10000;
   int run = 0;

   pagelist = (PAGELIST_T *)src;

   vcos_assert((src >= (const void *)RTOS_ALIAS_DIRECT(ARM_LINUX_MEMBASE)) &&
               (src < (const void *)RTOS_ALIAS_DIRECT(ARM_LINUX_MEMBASE + ARM_LINUX_MEMSIZE)) &&
               (pagelist->type == PAGELIST_WRITE) &&
               (size == pagelist->length));

   offset = pagelist->offset;

   for (i = 0; size > 0; i++)
   {
      unsigned long addr = pagelist->addrs[i];

      /* The ARM should be sending bus addresses which correspond to the VC memory map */
      const char *armptr = (const char *)(addr & ~(PAGE_SIZE - 1)) + offset;
      int bytes = ((addr & (PAGE_SIZE - 1)) + 1) * PAGE_SIZE - offset;
      if (bytes > size)
         bytes = size;
      if (bytes > 64)
         dma_memcpy(vcptr, armptr, bytes);
      else
         memcpy(vcptr, armptr, bytes);
      vcptr += bytes;
      size -= bytes;
      offset = 0;
   }
}

void vchiq_copy_bulk_to_host(void *dst, const void *src, int size)
{
   PAGELIST_T *pagelist;
   const char *vcptr = src;
   int offset, i;
   int first_pfn = -1;
   int run = 0;

   pagelist = (PAGELIST_T *)dst;

   vcos_assert((dst >= RTOS_ALIAS_DIRECT(ARM_LINUX_MEMBASE)) &&
               (dst < RTOS_ALIAS_DIRECT(ARM_LINUX_MEMBASE + ARM_LINUX_MEMSIZE)) &&
               (pagelist->type != PAGELIST_WRITE) &&
               (size == pagelist->length));

   offset = pagelist->offset;

   i = 0;

   if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS)
   {
      FRAGMENTS_T *fragments = vchiq_fragments + (pagelist->type - PAGELIST_READ_WITH_FRAGMENTS);
      int head_bytes, tail_bytes;
      head_bytes = (CACHE_LINE_SIZE - offset) & (CACHE_LINE_SIZE - 1);
      if (head_bytes)
      {
         memcpy(fragments->headbuf, vcptr, head_bytes);
         vcptr += head_bytes;
         size -= head_bytes;
         offset += head_bytes;
         if (offset == PAGE_SIZE)
         {
            offset = 0;
            i = 1;
         }
      }
      tail_bytes = (offset + size) & (CACHE_LINE_SIZE - 1);
      if (tail_bytes)
      {
         size -= tail_bytes;
         memcpy(fragments->tailbuf, vcptr + size, tail_bytes);
      }
   }

   for (; size > 0; i++)
   {
      unsigned long addr = pagelist->addrs[i];

      /* The ARM should be sending bus addresses which correspond to the VC memory map */
      char *armptr = (char *)(addr & ~(PAGE_SIZE - 1)) + offset;
      int bytes = ((addr & (PAGE_SIZE - 1)) + 1) * PAGE_SIZE - offset;
      if (bytes > size)
         bytes = size;
      if (bytes > 64)
         dma_memcpy(armptr, vcptr, bytes);
      else
         memcpy(armptr, vcptr, bytes);
      vcptr += bytes;
      size -= bytes;
      offset = 0;
   }
}

VCHIQ_STATUS_T vchiq_vc_initialise(const ARM_DRIVER_T *arm, DRIVER_HANDLE_T handle, uint32_t channelbase)
{
   VCHIQ_CHANNEL_T *channels;

   vcos_assert(vchiq_num_instances == 0);
   channels = (VCHIQ_CHANNEL_T *)channelbase;

   arm->set_doorbell_isr(handle, 2, (ARM_DOORBELL_ISR)remote_event_pollall, &vchiq_instances[0].state);

   vchiq_init_state(&vchiq_instances[0].state, channels, channels + 1);

   vchiq_fragments = (FRAGMENTS_T *)(((uint32_t)(channels + 2) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1));

   arm_driver = arm;
   arm_handle = handle;

   vchiq_num_instances = 1;

   return VCHIQ_SUCCESS;
}

void vchiq_ring_doorbell(void)
{
   arm_driver->doorbell_ring(arm_handle, 0, 1);
}

#endif /* VCHIQ_LOCAL */
