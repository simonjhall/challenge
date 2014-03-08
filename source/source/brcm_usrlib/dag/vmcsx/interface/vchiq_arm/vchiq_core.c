/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Core VCHIQ logic.

==============================================================================*/

#include "vchiq_core.h"

#define VCHIQ_SLOT_HANDLER_STACK 8192

#define VCHIQ_TRACE_ENABLED 0
#define VCHIQ_TRACE if (VCHIQ_TRACE_ENABLED) VCOS_TRACE

// FIXME: this should not be needed
#ifdef _VIDEOCORE
#include "vcfw/rtos/common/rtos_common_mem.h"
#endif

static void __inline vchiq_set_service_state(VCHIQ_SERVICE_T *service, int newstate)
{
   VCHIQ_TRACE("  srv %d: %d->%d", service->localport, service->srvstate, newstate);
   service->srvstate = newstate;
}

static void __inline local_mutex_create(LOCAL_MUTEX_T *mutex)
{
   vcos_mutex_create(&mutex->mutex, "vchiq");
}

static void __inline local_mutex_destroy(LOCAL_MUTEX_T *mutex)
{
   vcos_mutex_delete(&mutex->mutex);
}

static int __inline local_mutex_acquire(LOCAL_MUTEX_T *mutex)
{
   return (vcos_mutex_lock(&mutex->mutex) == VCOS_SUCCESS);
}

static void __inline local_mutex_release(LOCAL_MUTEX_T *mutex)
{
   vcos_mutex_unlock(&mutex->mutex);
}

static void __inline local_event_create(LOCAL_EVENT_T *event)
{
   vcos_event_create(&event->event, "vchiq");
}

static void __inline local_event_destroy(LOCAL_EVENT_T *event)
{
   vcos_event_delete(&event->event);
}

static void __inline local_event_signal(LOCAL_EVENT_T *event)
{
   vcos_event_signal(&event->event);
}

static int __inline local_event_wait(LOCAL_EVENT_T *event)
{
   return (vcos_event_wait(&event->event) == VCOS_SUCCESS);
}

static int __inline local_event_try(LOCAL_EVENT_T *event)
{
   return (vcos_event_try(&event->event) == VCOS_SUCCESS);
}

static void __inline remote_event_create(REMOTE_EVENT_T *event)
{
#ifdef VCHIQ_LOCAL
   vcos_event_create(&event->event, "vchiq");
#else
   event->set_count = 0;
   event->clr_count = 0;
   local_event_create(&event->local);
#endif
}

static void __inline remote_event_destroy(REMOTE_EVENT_T *event)
{
#ifdef VCHIQ_LOCAL
   vcos_event_delete(&event->event);
#else
   local_event_destroy(&event->local);
#endif
}

static void __inline remote_event_signal(REMOTE_EVENT_T *event)
{
#ifdef VCHIQ_LOCAL
   vcos_event_signal(&event->event);
#else
   event->set_count++;

#ifdef __VIDEOCORE__
   /* Force a stall until the write completes */
   _vasm("mov %D, %r", *(volatile *)&event->set_count);
#endif

   /* Notify the other side */
   vchiq_ring_doorbell();
#endif
}

static int __inline remote_event_wait(REMOTE_EVENT_T *event)
{
#ifdef VCHIQ_LOCAL
   return (vcos_event_wait(&event->event) == VCOS_SUCCESS);
#else
   return local_event_wait(&event->local);
#endif
}

static void __inline remote_event_signal_local(REMOTE_EVENT_T *event)
{
#ifdef VCHIQ_LOCAL
   vcos_event_signal(&event->event);
#else
   event->clr_count = event->set_count;
   local_event_signal(&event->local);
#endif
}

#ifndef VCHIQ_LOCAL

static void __inline remote_event_poll(REMOTE_EVENT_T *event)
{
   if (event->set_count != event->clr_count)
      remote_event_signal_local(event);
}

void remote_event_pollall(VCHIQ_STATE_T *state)
{
   remote_event_poll(&state->local->trigger);
   remote_event_poll(&state->local->ctrl.remove_event);
}

#endif

static unsigned int __inline calc_stride(unsigned int size)
{
   return (size + (unsigned int)sizeof(VCHIQ_HEADER_T) + 7) & ~7;
}

static VCHIQ_SERVICE_T *get_listening_service(VCHIQ_CHANNEL_T *channel, int fourcc)
{
   int i;

   vcos_assert(fourcc != VCHIQ_FOURCC_INVALID);

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      if ((channel->services[i].fourcc == fourcc) && (channel->services[i].srvstate == VCHIQ_SRVSTATE_LISTENING))
         return &channel->services[i];

   return NULL;
}

#if defined(VCHIQ_VC_SIDE)

static VCHIQ_SERVICE_T *get_connected_service(VCHIQ_CHANNEL_T *channel, unsigned int port)
{
   int i;
   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
   {
      VCHIQ_SERVICE_T *service = &channel->services[i];
      if ((service->srvstate == VCHIQ_SRVSTATE_OPEN) && (service->remoteport == port))
      {
         return service;
      }
   }
   return NULL;
}

#endif

static void *reserve_space(VCHIQ_CHANNEL_T *local, int target)
{
   if (target - local->ctrl.remove > VCHIQ_CHANNEL_SIZE)
   {
      while (1)
      {
         while (local->ctrl.remove != local->ctrl.process) {
            VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(local->ctrl.data + (local->ctrl.remove & VCHIQ_CHANNEL_MASK));

            if (header->fourcc == VCHIQ_FOURCC_INVALID)
               local->ctrl.remove += calc_stride(header->size);
            else
               break;
         }

         if (target - local->ctrl.remove <= VCHIQ_CHANNEL_SIZE)
            break;

         if (!remote_event_wait(&local->ctrl.remove_event))
            return NULL; /* Not now */
      }
   }
   return (void *)(local->ctrl.data + (local->ctrl.insert & VCHIQ_CHANNEL_MASK));
}

static VCHIQ_STATUS_T queue_message(VCHIQ_STATE_T *state, int fourcc, const VCHIQ_ELEMENT_T *elements, int count, int size)
{
   VCHIQ_CHANNEL_T *local;
   VCHIQ_HEADER_T *header;

   unsigned int stride;
   unsigned int pos;

   VCHIQ_TRACE("%d: qm %d", state->id, VCHIQ_MSG_TYPE(fourcc));

   local = state->local;

   stride = calc_stride(size);

   vcos_assert(stride < VCHIQ_CHANNEL_SIZE);

   local_mutex_acquire(&local->ctrl.mutex);

   pos = local->ctrl.insert; /* Remember for rewind */
   if ((local->ctrl.insert & VCHIQ_CHANNEL_MASK) + stride > VCHIQ_CHANNEL_SIZE) {
      int target = (local->ctrl.insert + VCHIQ_CHANNEL_SIZE) & ~VCHIQ_CHANNEL_MASK;

      header = (VCHIQ_HEADER_T *)reserve_space(local, target);
      if (!header)
      {
         local_mutex_release(&local->ctrl.mutex);
         return VCHIQ_RETRY;
      }
      header->fourcc = VCHIQ_FOURCC_INVALID;
      header->size = VCHIQ_CHANNEL_SIZE - sizeof(VCHIQ_HEADER_T) -
         (local->ctrl.insert & VCHIQ_CHANNEL_MASK);

      local->ctrl.insert = target;
   }

   /*
      write into slot
   */

   header = (VCHIQ_HEADER_T *)reserve_space(local, local->ctrl.insert + stride);
   if (!header)
   {
      local->ctrl.insert = pos;
      local_mutex_release(&local->ctrl.mutex);
      return VCHIQ_RETRY;
   }

   header->fourcc = fourcc;
   header->size = size;

   VCHIQ_TRACE("%d: qm %d - %x", state->id, VCHIQ_MSG_TYPE(fourcc), header);

   if (VCHIQ_MSG_TYPE(fourcc) == VCHIQ_MSG_DATA)
   {
      int i;
      for (i = 0, pos = 0; i < (unsigned int)count; pos += elements[i++].size)
         if (elements[i].size)
         {
            if (vchiq_copy_from_user(header->data + pos, elements[i].data, (size_t) elements[i].size) != VCHIQ_SUCCESS)
            {
               header->fourcc = VCHIQ_FOURCC_INVALID;
               local_mutex_release(&local->ctrl.mutex);
               return VCHIQ_ERROR;
            }
         }
   }
   else if (size != 0)
   {
      vcos_assert((count == 1) && (size == elements[0].size));
      memcpy(header->data, elements[0].data, elements[0].size);
   }

   local->ctrl.insert += stride;

   local_mutex_release(&local->ctrl.mutex);

   remote_event_signal(&state->remote->trigger);

   return VCHIQ_SUCCESS;
}

static void notify_rx_bulks(VCHIQ_STATE_T *state)
{
   VCHIQ_CHANNEL_T *local = state->local;

   VCHIQ_TRACE("%d: nrb", state->id);
   while (local->bulk.remove != local->bulk.process) {
      VCHIQ_RX_BULK_T *bulk = &local->bulk.bulks[local->bulk.remove & (VCHIQ_NUM_CURRENT_BULKS - 1)];

      VCHIQ_TRACE("%d: nrb %d %x", state->id, bulk->dstport, local->bulk.remove);

      if (bulk->dstport < VCHIQ_MAX_SERVICES)
      {
         VCHIQ_SERVICE_T *service = &local->services[bulk->dstport];

         if (service->base.callback(bulk->data ? VCHIQ_BULK_RECEIVE_DONE : VCHIQ_BULK_RECEIVE_ABORTED,
                                    NULL, &service->base, bulk->userdata) == VCHIQ_RETRY)
            break; /* Bail out if not ready */
      }

      local->bulk.remove++;

      local_event_signal(&local->bulk.remove_event);
   }
}

static void notify_tx_bulks(VCHIQ_STATE_T *state)
{
   VCHIQ_CHANNEL_T *local = state->local;
   int i;

   VCHIQ_TRACE("%d: ntb", state->id);
   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
   {
      VCHIQ_SERVICE_T *service = &local->services[i];

      if (service->srvstate > VCHIQ_SRVSTATE_LISTENING)
         VCHIQ_TRACE("%d: ntb - srv %d state %d", state->id, service->localport, service->srvstate);

      if (service->terminate)
      {
         if ((service->srvstate == VCHIQ_SRVSTATE_OPENING) ||
             (service->srvstate == VCHIQ_SRVSTATE_OPEN))
         {
            if (queue_message(state,
                              VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, service->localport, service->remoteport),
                              NULL, 0, 0) == VCHIQ_RETRY)
               continue;
         }

         service->terminate = 0;
         vchiq_set_service_state(service, VCHIQ_SRVSTATE_FREE);
      }
      else if (service->srvstate == VCHIQ_SRVSTATE_OPEN)
      {
         while (service->remove != service->process) {
            VCHIQ_TX_BULK_T *bulk = &service->bulks[service->remove & (VCHIQ_NUM_SERVICE_BULKS - 1)];

            VCHIQ_TRACE("%d: ntb %x DONE", state->id, service->remove);

            if (service->base.callback(bulk->data ? VCHIQ_BULK_TRANSMIT_DONE : VCHIQ_BULK_TRANSMIT_ABORTED,
                                       NULL, &service->base, bulk->userdata) == VCHIQ_RETRY)
               break; /* Bail out if not ready */

            service->remove++;

            local_event_signal(&service->remove_event);
         }
      }
   }
}

#if defined(VCHIQ_VC_SIDE)

static void resolve_tx_bulks(VCHIQ_STATE_T *state)
{
   VCHIQ_CHANNEL_T *local = state->local;
   VCHIQ_CHANNEL_T *remote = state->remote;
   int retrigger = 0;

   VCHIQ_TRACE("%d: rtb %x %x", state->id, remote->bulk.process, remote->bulk.insert);
   while (remote->bulk.process != remote->bulk.insert) {
      VCHIQ_RX_BULK_T *rx_bulk = &remote->bulk.bulks[remote->bulk.process & (VCHIQ_NUM_CURRENT_BULKS - 1)];
      VCHIQ_SERVICE_T *service;
      VCHIQ_TX_BULK_T *tx_bulk;

      if (VCHIQ_PORT_IS_VALID(rx_bulk->dstport))
      {
         /* Find connected service */
         service = get_connected_service(local, rx_bulk->dstport);

         if (service == NULL)
         {
            rx_bulk->data = NULL; /* Abort */
            VCHIQ_TRACE("%d: rtb %x(%d) ABORTED", state->id, remote->bulk.process, rx_bulk->dstport);
         }
         else
         {
            if (service->process == service->insert)
               break; /* No matching transmit Stall */

            tx_bulk = &service->bulks[service->process & (VCHIQ_NUM_SERVICE_BULKS - 1)];

            if (rx_bulk->size == tx_bulk->size)
            {
               const void *tx_data;

               vcos_assert(rx_bulk->handle == VCHI_MEM_HANDLE_INVALID);

               if (tx_bulk->handle == VCHI_MEM_HANDLE_INVALID)
                  tx_data = tx_bulk->data;
               else
               {
                  tx_data = (const char *)mem_lock(tx_bulk->handle) + (int)tx_bulk->data;
                  tx_bulk->data = tx_data; /* Overwrite to avoid an abort */
               }

               vchiq_copy_bulk_to_host(rx_bulk->data, tx_data, rx_bulk->size);

               if (tx_bulk->handle != VCHI_MEM_HANDLE_INVALID)
                  mem_unlock(tx_bulk->handle);

               VCHIQ_TRACE("%d: rtb %x<->%x(%d)", state->id, service->process, remote->bulk.process, rx_bulk->dstport);
            }
            else
            {
               /* Abort these transfers */
               rx_bulk->data = NULL;
               tx_bulk->data = NULL;

               VCHIQ_TRACE("%d: rtb %x<->%x(%d) ABORTED", state->id, service->process, remote->bulk.process, rx_bulk->dstport);
            }

            retrigger = 1;

            service->process++;
         }
      }

      remote->bulk.process++;

      remote_event_signal(&remote->trigger);
   }

   if (retrigger)
      remote_event_signal_local(&local->trigger);
}

#endif /* defined(VCHIQ_VC_SIDE) */

#if !defined(VCHIQ_ARM_SIDE)

static void resolve_rx_bulks(VCHIQ_STATE_T *state)
{
   VCHIQ_CHANNEL_T *local = state->local;
   VCHIQ_CHANNEL_T *remote = state->remote;
   int retrigger = 0;

   VCHIQ_TRACE("%d: rrb %x %x", state->id, local->bulk.process, local->bulk.insert);
   while (local->bulk.process != local->bulk.insert) {
      VCHIQ_RX_BULK_T *rx_bulk = &local->bulk.bulks[local->bulk.process & (VCHIQ_NUM_CURRENT_BULKS - 1)];
      VCHIQ_SERVICE_T *service;
      VCHIQ_TX_BULK_T *tx_bulk;

      /* Ensure the service isn't being closed */
      if (rx_bulk->dstport < VCHIQ_MAX_SERVICES)
      {
         service = &local->services[rx_bulk->dstport];
         if (service->srvstate == VCHIQ_SRVSTATE_OPEN)
         {
            VCHIQ_SERVICE_T *remote_service = &remote->services[service->remoteport];
            if (remote_service->process == remote_service->insert)
               break; /* No tx bulk - stall */

            tx_bulk = &remote_service->bulks[remote_service->process & (VCHIQ_NUM_SERVICE_BULKS - 1)];

            if (rx_bulk->size == tx_bulk->size)
            {
               const void *tx_data;
               void *rx_data;

#ifdef VCHIQ_VC_SIDE
               vcos_assert(tx_bulk->handle == VCHI_MEM_HANDLE_INVALID);
               tx_data = tx_bulk->data;
#else
               if (tx_bulk->handle == VCHI_MEM_HANDLE_INVALID)
                  tx_data = tx_bulk->data;
               else
               {
                  tx_data = (const char *)mem_lock(tx_bulk->handle) + (int)tx_bulk->data;
                  tx_bulk->data = tx_data; /* Overwrite to avoid an abort */
               }
#endif
               if (rx_bulk->handle == VCHI_MEM_HANDLE_INVALID)
                  rx_data = rx_bulk->data;
               else
               {
                  rx_data = (char *)mem_lock(rx_bulk->handle) + (int)rx_bulk->data;
                  rx_bulk->data = rx_data; /* Overwrite to avoid an abort */
               }

               vchiq_copy_bulk_from_host(rx_data, tx_data, rx_bulk->size);

               if (rx_bulk->handle != VCHI_MEM_HANDLE_INVALID)
                  mem_unlock(rx_bulk->handle);

#ifndef VCHIQ_VC_SIDE
               if (tx_bulk->handle != VCHI_MEM_HANDLE_INVALID)
                  mem_unlock(tx_bulk->handle);
#endif

               VCHIQ_TRACE("%d: rrb %x<->%x(%d)", state->id, local->bulk.process, remote_service->process, service->remoteport);
            }
            else
            {
               /* Abort these non-matching transfers */
               rx_bulk->data = NULL;
               tx_bulk->data = NULL;

               VCHIQ_TRACE("%d: rrb %x<->%x(%d) ABORTED", state->id, local->bulk.process, remote_service->process, service->remoteport);
            }

            remote_service->process++;

            remote_event_signal(&remote->trigger);
         }
         else
         {
            rx_bulk->data = NULL; /* Aborted */
            VCHIQ_TRACE("%d: rrb %x ABORTED", state->id, local->bulk.process);
         }
      }

      local->bulk.process++;

      retrigger = 1;
   }

   if (retrigger)
      remote_event_signal_local(&local->trigger);
}

#endif /* !defined(VCHIQ_ARM_SIDE) */

static void parse_rx_slots(VCHIQ_STATE_T *state)
{
   VCHIQ_CHANNEL_T *remote = state->remote;
   VCHIQ_CHANNEL_T *local = state->local;

   while (remote->ctrl.process != remote->ctrl.insert) {
      VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(remote->ctrl.data + (remote->ctrl.process & VCHIQ_CHANNEL_MASK));
      VCHIQ_SERVICE_T *service = NULL;
      unsigned int stride = calc_stride(header->size);
      int type = VCHIQ_MSG_TYPE(header->fourcc);

      VCHIQ_TRACE("%d: prs %d (%d,%d)", state->id, type, VCHIQ_MSG_DSTPORT(header->fourcc), VCHIQ_MSG_SRCPORT(header->fourcc));

      switch (type)
      {
      case VCHIQ_MSG_OPEN:
         vcos_assert(VCHIQ_MSG_DSTPORT(header->fourcc) == 0);
         if (vcos_verify(header->size == 4))
         {
            VCHIQ_HEADER_T *reply;
            unsigned short remoteport = VCHIQ_MSG_SRCPORT(header->fourcc);
            int target;
            service = get_listening_service(local, *(int *)header->data);

            local_mutex_acquire(&local->ctrl.mutex);

            target = local->ctrl.insert + sizeof(VCHIQ_HEADER_T);
            reply = reserve_space(local, target);
            if (!reply)
            {
               local_mutex_release(&local->ctrl.mutex);
               return; /* Bail out */
            }

            if (service && (service->srvstate == VCHIQ_SRVSTATE_LISTENING))
            {
               /* A matching, listening service exists - attempt the OPEN */
               VCHIQ_STATUS_T status;
               vchiq_set_service_state(service, VCHIQ_SRVSTATE_OPEN); /* Proceed as if the connection will be accepted */
               status = service->base.callback(VCHIQ_SERVICE_OPENED, NULL, &service->base, NULL);
               if (status == VCHIQ_SUCCESS)
               {
                  /* The open was accepted - acknowledge it */
                  reply->fourcc = VCHIQ_MAKE_MSG(VCHIQ_MSG_OPENACK, service->localport, remoteport);
                  service->remoteport = remoteport;
               }
               else
               {
                  vchiq_set_service_state(service, VCHIQ_SRVSTATE_LISTENING);

                  if (status == VCHIQ_RETRY)
                     return; /* Bail out if not ready */

                  /* The open was rejected - send a close */
                  reply->fourcc = VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, 0, remoteport);
               }
            }
            else
            {
               /* No matching, available service - send a CLOSE */
               reply->fourcc = VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, 0, remoteport);
            }
            reply->size = 0;

            local->ctrl.insert = target;

            local_mutex_release(&local->ctrl.mutex);

            remote_event_signal(&remote->trigger);
         }
         break;
      case VCHIQ_MSG_OPENACK:
         {
            unsigned int localport = VCHIQ_MSG_DSTPORT(header->fourcc);
            unsigned int remoteport = VCHIQ_MSG_SRCPORT(header->fourcc);
            service = &local->services[localport];
            if (vcos_verify((localport < VCHIQ_MAX_SERVICES) &&
                            (service->srvstate == VCHIQ_SRVSTATE_OPENING)))
            {
               service->remoteport = remoteport;
               vchiq_set_service_state(service, VCHIQ_SRVSTATE_OPEN);
               local_event_signal(&service->remove_event);
            }
         }
         break;
      case VCHIQ_MSG_CLOSE:
         {
            unsigned int localport = VCHIQ_MSG_DSTPORT(header->fourcc);
            unsigned int remoteport = VCHIQ_MSG_SRCPORT(header->fourcc);
            service = &local->services[localport];
            vcos_assert(header->size == 0); /* There should be no data */

            if (vcos_verify(localport < VCHIQ_MAX_SERVICES))
            {
               switch (service->srvstate)
               {
               case VCHIQ_SRVSTATE_OPEN:
                  if (service->remoteport != remoteport)
                     break;
                  /* Return the close */
                  if (queue_message(state,
                                    VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, service->localport, service->remoteport),
                                    NULL, 0, 0) == VCHIQ_RETRY)
                     return; /* Bail out if not ready */

                  vchiq_set_service_state(service, VCHIQ_SRVSTATE_CLOSESENT);
                  /* Drop through... */
               case VCHIQ_SRVSTATE_CLOSESENT:
                  if (service->remoteport != remoteport)
                     break;
                  vchiq_set_service_state(service, VCHIQ_SRVSTATE_CLOSING);
               /* Drop through... */
               case VCHIQ_SRVSTATE_CLOSING:
                  if (service->remoteport == remoteport)
                  {
                     /* Start the close procedure */
                     if (vchiq_close_service_internal(service) == VCHIQ_RETRY)
                        return; /* Bail out if not ready */
                  }
                  break;
               case VCHIQ_SRVSTATE_OPENING:
                  /* A client is mid-open - this is a rejection, so just fail the open */
                  vchiq_set_service_state(service, VCHIQ_SRVSTATE_CLOSEWAIT);
                  local_event_signal(&service->remove_event);
                  break;
               default:
                  break;
               }
            }
         }
         break;
      case VCHIQ_MSG_DATA:
         {
            unsigned int localport = VCHIQ_MSG_DSTPORT(header->fourcc);
            unsigned int remoteport = VCHIQ_MSG_SRCPORT(header->fourcc);
            service = &local->services[localport];
            if (vcos_verify((localport < VCHIQ_MAX_SERVICES) &&
                            (service->remoteport == remoteport)) &&
                (service->srvstate == VCHIQ_SRVSTATE_OPEN))
            {
               if (service->base.callback(VCHIQ_MESSAGE_AVAILABLE, header, &service->base, NULL) == VCHIQ_RETRY)
                  return; /* Bail out if not ready */
               header = NULL; /* Don't invalidate this message - defer till vchiq_release */
            }
         }
         break;
      case VCHIQ_MSG_CONNECT:
         vcos_event_signal(&state->connect);
         break;
      case VCHIQ_MSG_INVALID:
      default:
         break;
      }

      remote->ctrl.process += stride;
      if (header != NULL)
      {
         /* Invalidate it */
         header->fourcc = VCHIQ_FOURCC_INVALID;
         /* Notify the other end there is some space */
         remote_event_signal(&remote->ctrl.remove_event);
      }
   }
}

static void *slot_handler_func(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;
   VCHIQ_CHANNEL_T *local = state->local;

   while (1) {
      remote_event_wait(&local->trigger);

      parse_rx_slots(state);

#if !defined(VCHIQ_ARM_SIDE)
      resolve_rx_bulks(state);
#endif

#if defined(VCHIQ_VC_SIDE)
      resolve_tx_bulks(state);
#endif

      notify_rx_bulks(state);
      notify_tx_bulks(state);
   }
   return NULL;
}

void vchiq_init_channel(VCHIQ_CHANNEL_T *channel)
{
   int i;

   channel->ctrl.remove = 0;
   channel->ctrl.process = 0;
   channel->ctrl.insert = 0;

   remote_event_create(&channel->ctrl.remove_event);
   local_mutex_create(&channel->ctrl.mutex);

   channel->bulk.remove = 0;
   channel->bulk.process = 0;
   channel->bulk.insert = 0;

   local_event_create(&channel->bulk.remove_event);
   local_mutex_create(&channel->bulk.mutex);

   remote_event_create(&channel->trigger);

   /*
      initialize services
   */

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
   {
      VCHIQ_SERVICE_T *service = &channel->services[i];
      service->srvstate = VCHIQ_SRVSTATE_FREE;
      service->localport = i;
      local_event_create(&service->remove_event);
   }
}

static __inline int is_pow2(int i)
{
   return i && !(i & (i - 1));
}

void vchiq_init_state(VCHIQ_STATE_T *state, VCHIQ_CHANNEL_T *local, VCHIQ_CHANNEL_T *remote)
{
   VCOS_THREAD_ATTR_T attrs;
   char threadname[8];
   static int id = 0;

   vcos_assert(is_pow2(VCHIQ_CHANNEL_SIZE));

   vcos_assert(is_pow2(VCHIQ_NUM_CURRENT_BULKS));
   vcos_assert(is_pow2(VCHIQ_NUM_SERVICE_BULKS));

   vcos_assert(sizeof(VCHIQ_HEADER_T) == 8);    /* we require this for consistency between endpoints */

   memset(state, 0, sizeof(VCHIQ_STATE_T));
   state->id = id++;

   /*
      initialize events and mutex
   */

   vcos_event_create(&state->connect, "vchiq");
   vcos_mutex_create(&state->mutex, "vchiq");

   /*
      initialize channel pointers
   */

   state->local  = local;
   state->remote = remote;
   vchiq_init_channel(local);

   /*
      bring up slot handler thread
   */

   vcos_thread_attr_init(&attrs);
   vcos_thread_attr_setstacksize(&attrs, VCHIQ_SLOT_HANDLER_STACK);
   vcos_thread_attr_setpriority(&attrs, 5); /* FIXME: should not be hardcoded */
   vcos_thread_attr_settimeslice(&attrs, 20); /* FIXME: should not be hardcoded */
   strcpy(threadname, "VCHIQ-0");
   threadname[6] += state->id % 10;
   vcos_thread_create(&state->slot_handler_thread, threadname,
                      &attrs, slot_handler_func, state);

   /* Indicate readiness to the other side */
   local->initialised = 1;
}

VCHIQ_SERVICE_T *vchiq_add_service_internal(VCHIQ_STATE_T *state, int fourcc, VCHIQ_CALLBACK_T callback,
                                            void *userdata, int srvstate, VCHIQ_INSTANCE_T instance)
{
   VCHIQ_CHANNEL_T *local = state->local;
   VCHIQ_SERVICE_T *service = NULL;
   int i;

   if (srvstate == VCHIQ_SRVSTATE_OPENING)
   {
      for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      {
         VCHIQ_SERVICE_T *srv = &local->services[i];
         if (srv->srvstate == VCHIQ_SRVSTATE_FREE)
         {
            service = srv;
            break;
         }
      }
   }
   else
   {
      for (i = (VCHIQ_MAX_SERVICES - 1); i >= 0; i--)
      {
         VCHIQ_SERVICE_T *srv = &local->services[i];
         if (srv->srvstate == VCHIQ_SRVSTATE_FREE)
         {
            service = srv;
         }
         else if ((srv->fourcc == fourcc) &&
                  ((srv->instance != instance) || (srv->base.callback != callback)))
         {
            /* There is another server using this fourcc which doesn't match */
            service = NULL;
            break;
         }
      }
   }

   if (service)
   {
      service->base.fourcc = fourcc;
      service->base.callback = callback;
      service->base.userdata = userdata;
      vchiq_set_service_state(service, srvstate);
      service->fourcc = (srvstate == VCHIQ_SRVSTATE_OPENING) ? VCHIQ_FOURCC_INVALID : fourcc;
      service->state = state;
      service->instance = instance;
      service->remoteport = VCHIQ_PORT_FREE;
      service->remove = 0;
      service->process = 0;
      service->insert = 0;

      while (local_event_try(&service->remove_event))
         continue;
   }

   return service;
}

VCHIQ_STATUS_T vchiq_open_service_internal(VCHIQ_SERVICE_T *service)
{
   VCHIQ_ELEMENT_T body = { &service->base.fourcc, sizeof(service->base.fourcc) };
   VCHIQ_STATUS_T status = queue_message(service->state,
                                         VCHIQ_MAKE_MSG(VCHIQ_MSG_OPEN, service->localport, 0),
                                         &body, 1, sizeof(service->base.fourcc));
   if (status == VCHIQ_SUCCESS)
   {
      if (!local_event_wait(&service->remove_event))
      {
         status = VCHIQ_RETRY;
      }
      else if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      {
         VCHIQ_TRACE("%d: osi - srvstate = %d", service->state->id, service->srvstate);
         vcos_assert(service->srvstate == VCHIQ_SRVSTATE_CLOSEWAIT);
         status = VCHIQ_ERROR;
      }
   }
   return status;
}

VCHIQ_STATUS_T vchiq_close_service_internal(VCHIQ_SERVICE_T *service)
{
   /* This is the first half of the close process, the remainder handled by
      notify_tx_bulks */
   VCHIQ_CHANNEL_T *local = service->state->local;
   int pos;
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
   int reason;

   VCHIQ_TRACE("%d: csi %d - i %x, p %x, r %x/i %x, p %x, r %x", service->state->id, service->localport, service->insert, service->process, service->remove, local->bulk.insert, local->bulk.process, local->bulk.remove);

   /* Complete any outstanding bulk receives */

   reason = VCHIQ_BULK_RECEIVE_DONE;

   for (pos = local->bulk.remove; pos != local->bulk.insert; pos++)
   {
      VCHIQ_RX_BULK_T *bulk = &local->bulk.bulks[pos & (VCHIQ_NUM_CURRENT_BULKS - 1)];

      if (pos == local->bulk.process)
         reason = VCHIQ_BULK_RECEIVE_ABORTED;

      VCHIQ_TRACE("%d: csi %d - %x %d", service->state->id, service->localport, pos, bulk->dstport);

      if (bulk->dstport == service->localport)
      {
         VCHIQ_SERVICE_T *service = &local->services[bulk->dstport];

         status = service->base.callback(bulk->data ? reason : VCHIQ_BULK_RECEIVE_ABORTED, NULL, &service->base, bulk->userdata);
         VCHIQ_TRACE("%d: csi %d - %x callback(%d)->%d", service->state->id, service->localport, pos, reason, status);
         if (status == VCHIQ_RETRY)
            break; /* Bail out if not ready */

         bulk->dstport = VCHIQ_PORT_FREE; /* Avoid a second callback */
      }
   }

   if (status == VCHIQ_SUCCESS)
   {
      /* Complete any outstanding bulk transmits */

      reason = VCHIQ_BULK_TRANSMIT_DONE;

      while (service->remove != service->insert)
      {
         VCHIQ_TX_BULK_T *bulk = &service->bulks[service->remove & (VCHIQ_NUM_SERVICE_BULKS - 1)];

         if (service->remove == service->process)
            reason = VCHIQ_BULK_TRANSMIT_ABORTED;

         status = service->base.callback(reason, NULL, &service->base, bulk->userdata);
         VCHIQ_TRACE("%d: csi %d - %x callback(%d)->%d", service->state->id, service->localport, service->remove, reason, status);
         if (status == VCHIQ_RETRY)
            break;

         if (service->remove == service->process)
            service->process++;
         service->remove++;
      }

      if (service->remove == service->insert)
      {
         int oldstate = service->srvstate;
         vcos_assert(service->process == service->insert);
         vchiq_set_service_state(service, (service->fourcc == VCHIQ_FOURCC_INVALID) ? VCHIQ_SRVSTATE_CLOSEWAIT : VCHIQ_SRVSTATE_LISTENING);

         status = service->base.callback(VCHIQ_SERVICE_CLOSED, NULL, &service->base, NULL);
         VCHIQ_TRACE("%d: csi %d - callback(SERVICE_CLOSED)->%d", service->state->id, service->localport, status);
         if (status == VCHIQ_RETRY)
            vchiq_set_service_state(service, oldstate);
         else
         {
            if (status == VCHIQ_ERROR)
            {
               /* Signal an error (fatal, since the other end will probably have closed) */
               vchiq_set_service_state(service, VCHIQ_SRVSTATE_OPEN); 
            }
            local_event_signal(&service->remove_event);
         }
      }
   }

   VCHIQ_TRACE("%d: csi - i %x, p %x, r %x -> %d", service->state->id, service->insert, service->process, service->remove, status);

   return status;
}

void vchiq_terminate_service_internal(VCHIQ_SERVICE_T *service)
{
   VCHIQ_CHANNEL_T *remote = service->state->remote;
   int remove, fourcc;

   /* Release any unreleased messages */
   remove = remote->ctrl.remove;

   VCHIQ_TRACE("%d: tsi - (%d<->%d) i %x, p %x, r %x", service->state->id, service->localport, service->remoteport, remote->ctrl.insert, remote->ctrl.process, remove);

   fourcc = VCHIQ_MAKE_MSG(VCHIQ_MSG_DATA, service->remoteport, service->localport);
   while (remove != remote->ctrl.insert)
   {
      VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(remote->ctrl.data + (remove & VCHIQ_CHANNEL_MASK));

      remove += calc_stride(header->size);

      if (header->fourcc == fourcc)
         header->fourcc = VCHIQ_FOURCC_INVALID;
   }

   /* Mark the service for termination by the slot handler... */
   service->terminate = 1;

   /* ... and ensure the slot handler runs. */
   remote_event_signal_local(&service->state->local->trigger);
}

VCHIQ_STATUS_T vchiq_connect_internal(VCHIQ_STATE_T *state, VCHIQ_INSTANCE_T instance)
{
   VCHIQ_CHANNEL_T *local = state->local;
   int i;

   /* Find all services registered to this client and enable them. */
   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      if (local->services[i].instance == instance)
      {
         if (local->services[i].srvstate == VCHIQ_SRVSTATE_HIDDEN)
            vchiq_set_service_state(&local->services[i], VCHIQ_SRVSTATE_LISTENING);
      }

   if (!state->connected)
   {
      if (queue_message(state, VCHIQ_MAKE_MSG(VCHIQ_MSG_CONNECT, 0, 0), NULL, 0, 0) == VCHIQ_RETRY)
         return VCHIQ_RETRY;
      vcos_event_wait(&state->connect);
      state->connected = 1;
   }

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_remove_service(VCHIQ_SERVICE_HANDLE_T handle)
{
   /* Unregister the service */
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_STATE_T *state = service->state;
   VCHIQ_STATUS_T status = VCHIQ_SUCCESS;

   switch (service->srvstate)
   {
   case VCHIQ_SRVSTATE_OPENING:
   case VCHIQ_SRVSTATE_OPEN:
      {
         int oldstate = service->srvstate;

         /* Start the CLOSE procedure */
         vchiq_set_service_state(service, VCHIQ_SRVSTATE_CLOSESENT);
         status = queue_message(state,
                                VCHIQ_MAKE_MSG(VCHIQ_MSG_CLOSE, service->localport, service->remoteport),
                                NULL, 0, 0);

         if (status != VCHIQ_SUCCESS)
            vchiq_set_service_state(service, oldstate);
      }
      break;

   case VCHIQ_SRVSTATE_HIDDEN:
   case VCHIQ_SRVSTATE_LISTENING:
   case VCHIQ_SRVSTATE_CLOSING:
   case VCHIQ_SRVSTATE_CLOSEWAIT:
      break;

   default:
      status = VCHIQ_ERROR;
      break;
   }

   while ((service->srvstate == VCHIQ_SRVSTATE_CLOSING) ||
          (service->srvstate == VCHIQ_SRVSTATE_CLOSESENT))
   {
      if (!local_event_wait(&service->remove_event))
      {
         status = VCHIQ_RETRY;
         break;
      }
   }

   if (status == VCHIQ_SUCCESS)
   {
      if (service->srvstate == VCHIQ_SRVSTATE_OPEN)
         status = VCHIQ_ERROR;
      else
         vchiq_set_service_state(service, VCHIQ_SRVSTATE_FREE);
   }

   return status;
}

VCHIQ_STATUS_T vchiq_queue_bulk_transmit(VCHIQ_SERVICE_HANDLE_T handle, const void *data, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_TX_BULK_T *bulk;
   VCHIQ_STATE_T *state;

   vcos_assert((service != NULL) && (data != NULL));

   state = service->state;

   VCHIQ_TRACE("%d: qbt %d", state->id, service->localport);

   if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      return VCHIQ_ERROR; /* Must be connected */

   while (service->insert == service->remove + VCHIQ_NUM_SERVICE_BULKS)
      if (!local_event_wait(&service->remove_event))
         return VCHIQ_RETRY;

   bulk = &service->bulks[service->insert & (VCHIQ_NUM_SERVICE_BULKS - 1)];
   bulk->dstport = service->remoteport;
   bulk->handle = VCHI_MEM_HANDLE_INVALID;
   bulk->data = data;
   bulk->size = size;
   bulk->userdata = userdata;

   VCHIQ_TRACE("%d: qbt %d %x", state->id, service->localport, service->insert);

   service->insert++;

#ifdef VCHIQ_VC_SIDE
   remote_event_signal_local(&state->local->trigger);
#else
   remote_event_signal(&state->remote->trigger);
#endif

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_queue_bulk_receive(VCHIQ_SERVICE_HANDLE_T handle, void *data, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_STATE_T *state;
   VCHIQ_CHANNEL_T *local;
   VCHIQ_RX_BULK_T *bulk;

   vcos_assert((service != NULL) && (data != NULL));

   state = service->state;
   local = state->local;

   VCHIQ_TRACE("%d: qbr %d", state->id, service->localport);

   if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      return VCHIQ_ERROR; /* Must be connected (receives are processed in order of submission) */

   local_mutex_acquire(&local->bulk.mutex);

   while(local->bulk.insert == local->bulk.remove + VCHIQ_NUM_CURRENT_BULKS)
      local_event_wait(&local->bulk.remove_event);

   bulk = &local->bulk.bulks[local->bulk.insert & (VCHIQ_NUM_CURRENT_BULKS - 1)];
   bulk->dstport = service->localport;
   bulk->handle = VCHI_MEM_HANDLE_INVALID;
   bulk->data = data;
   bulk->size = size;
   bulk->userdata = userdata;

   VCHIQ_TRACE("%d: qbr %d %x", state->id, service->localport, local->bulk.insert);

   local->bulk.insert++;

   local_mutex_release(&local->bulk.mutex);

#ifdef VCHIQ_ARM_SIDE
   remote_event_signal(&state->remote->trigger);
#else
   remote_event_signal_local(&local->trigger);
#endif

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_queue_bulk_transmit_handle(VCHIQ_SERVICE_HANDLE_T handle, VCHI_MEM_HANDLE_T memhandle, void *offset, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_TX_BULK_T *bulk;
   VCHIQ_STATE_T *state;

   vcos_assert(service != NULL);
#ifndef USE_MEMMGR
   vcos_assert(memhandle == VCHI_MEM_HANDLE_INVALID);
#endif

   state = service->state;

   VCHIQ_TRACE("%d: qbth %d", state->id, service->localport);

   if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      return VCHIQ_ERROR; /* Must be connected */

   while (service->insert == service->remove + VCHIQ_NUM_SERVICE_BULKS)
      if (!local_event_wait(&service->remove_event))
         return VCHIQ_RETRY;

   bulk = &service->bulks[service->insert & (VCHIQ_NUM_SERVICE_BULKS - 1)];
   bulk->dstport = service->remoteport;
   bulk->handle = memhandle;
   bulk->data = offset;
   bulk->size = size;
   bulk->userdata = userdata;

   VCHIQ_TRACE("%d: qbth %d %x", state->id, service->localport, service->insert);

   service->insert++;

#ifdef VCHIQ_VC_SIDE
   remote_event_signal_local(&state->local->trigger);
#else
   remote_event_signal(&state->remote->trigger);
#endif

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_queue_bulk_receive_handle(VCHIQ_SERVICE_HANDLE_T handle, VCHI_MEM_HANDLE_T memhandle, void *offset, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;
   VCHIQ_STATE_T *state;
   VCHIQ_CHANNEL_T *local;
   VCHIQ_RX_BULK_T *bulk;

   vcos_assert(service != NULL);
#ifndef USE_MEMMGR
   vcos_assert(memhandle == VCHI_MEM_HANDLE_INVALID);
#endif

   state = service->state;
   local = state->local;

   VCHIQ_TRACE("%d: qbrh %d", state->id, service->localport);

   if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      return VCHIQ_ERROR; /* Must be connected (receives are processed in order of submission) */

   local_mutex_acquire(&local->bulk.mutex);

   while(local->bulk.insert == local->bulk.remove + VCHIQ_NUM_CURRENT_BULKS)
      local_event_wait(&local->bulk.remove_event);

   bulk = &local->bulk.bulks[local->bulk.insert & (VCHIQ_NUM_CURRENT_BULKS - 1)];
   bulk->dstport = service->localport;
   bulk->handle = memhandle;
   bulk->data = offset;
   bulk->size = size;
   bulk->userdata = userdata;

   VCHIQ_TRACE("%d: qbrh %d %x", state->id, service->localport, local->bulk.insert);

   local->bulk.insert++;

   local_mutex_release(&local->bulk.mutex);

#ifdef VCHIQ_ARM_SIDE
   remote_event_signal(&state->remote->trigger);
#else
   remote_event_signal_local(&local->trigger);
#endif

   return VCHIQ_SUCCESS;
}

VCHIQ_STATUS_T vchiq_queue_message(VCHIQ_SERVICE_HANDLE_T handle, const VCHIQ_ELEMENT_T *elements, int count)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;

   unsigned int size = 0;
   unsigned int i;

   if (service->srvstate != VCHIQ_SRVSTATE_OPEN)
      return VCHIQ_ERROR;

   for (i = 0; i < (unsigned int)count; i++)
      size += elements[i].size;

   if (calc_stride(size) > VCHIQ_CHANNEL_SIZE)
      return VCHIQ_ERROR;

   return queue_message(service->state,
                        VCHIQ_MAKE_MSG(VCHIQ_MSG_DATA, service->localport, service->remoteport),
                        elements, count, size);
}

void vchiq_release_message(VCHIQ_SERVICE_HANDLE_T handle, VCHIQ_HEADER_T *header)
{
   VCHIQ_SERVICE_T *service = (VCHIQ_SERVICE_T *)handle;

   vcos_assert(header->fourcc != VCHIQ_FOURCC_INVALID);
   header->fourcc = VCHIQ_FOURCC_INVALID;
   remote_event_signal(&service->state->remote->ctrl.remove_event);
}
