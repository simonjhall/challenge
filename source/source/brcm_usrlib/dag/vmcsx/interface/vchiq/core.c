#include "vchiq.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _VIDEOCORE
#include "helpers/vcsuspend/vcsuspend.h"
#define defined_alloc(size, description) rtos_prioritymalloc(size, RTOS_ALIGN_256BIT, RTOS_PRIORITY_L1_NONALLOCATING, description)
#else
#define defined_alloc(size, description) malloc(size)
#endif

#ifdef USE_USBHP
extern void usbhp_thread(void *v);
#endif

#define VCHIQ_NUM_DMA                     16

#define VCHIQ_SLOT_HANDLER_STACK          8192
#define VCHIQ_WORKER_STACK                4096

static unsigned int calc_stride(unsigned int size)
{
#ifdef __CC_ARM
   return size + (unsigned int)sizeof(VCHIQ_HEADER_T) - 4 + 15 & ~15;
#else
   return size + (unsigned int)sizeof(VCHIQ_HEADER_T) + 15 & ~15;
#endif
}

static int rx_slot_in_use(VCHIQ_SLOT_T *slot)
{
   int pos = 0;

   while (pos < slot->size) {
      VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(slot->data + pos);

      if (header->u.slot)
         return 1;

      pos += calc_stride(header->size);
   }

   assert(pos == slot->size);

   return 0;
}

static void install_rx_slots(VCHIQ_STATE_T *state)
{
   while (state->ctrl.rx.install != (short)(state->ctrl.rx.process.s.slot + VCHIQ_NUM_RX_SLOTS)) {
      VCHIQ_SLOT_T *slot = &state->ctrl.rx.slots[state->ctrl.rx.install & VCHIQ_NUM_RX_SLOTS - 1];

      if (slot->received != slot->released && rx_slot_in_use(slot))
         break;

      slot->received = 0;
      slot->released = 0;

      /*
         add slot at 'install' to DMA hardware
      */

#ifdef _VIDEOCORE_MPHI
      if (state->driver->add_receive_entry(state->handle, slot->data, VCHIQ_SLOT_SIZE, MPHI_CHANNEL_CTRL) != 0)
         break;
#else
      if (state->ctrl.rx.install == (short)(state->ctrl.rx.remove.s.slot + VCHIQ_NUM_DMA))
         break;
#endif

      state->ctrl.rx.install++;

      if (state->ctrl.tx.install == state->ctrl.tx.setup)
         os_event_signal(&state->worker);
   }
}

static void install_tx_tasks(VCHIQ_STATE_T *state)
{
   while (state->ctrl.tx.install != state->ctrl.tx.setup) {
      VCHIQ_TASK_T *task = &state->ctrl.tx.tasks[state->ctrl.tx.install & VCHIQ_NUM_TX_TASKS - 1];

      /*
         deadlock avoidance - don't queue a transmit into the last available slot on
         the peer unless it comes with news of a newly available receive slot
      */

      if (state->ctrl.rx.peer != state->ctrl.rx.install)
         state->ctrl.rx.peer = state->ctrl.rx.install;
      else
         if (task->slot == (short)(state->ctrl.tx.peer - 1))
            break;

      /*
         update message header with current rx install position
      */

      task->data->u.s.peer = state->ctrl.rx.peer;

      /*
         add task at 'install' to DMA hardware
      */

#ifdef _VIDEOCORE_MPHI
      if (state->driver->add_transmit_entry(state->handle, task->data, task->size, MPHI_CHANNEL_CTRL) != 0)
         break;
#else
      if (state->ctrl.tx.install == (short)(state->ctrl.tx.remove + VCHIQ_NUM_DMA))
         break;
#endif

      state->ctrl.tx.install++;

#ifndef _VIDEOCORE_MPHI
      os_event_signal(&state->ctrl.tx.hardware);
#endif
   }
}

static VCHIQ_SERVICE_T *get_service(VCHIQ_STATE_T *state, int fourcc)
{
   int i;

   assert(fourcc != VCHIQ_FOURCC_INVALID);

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      if (state->services[i].fourcc == fourcc)
         return &state->services[i];

   return NULL;
}

static void resolve_tx_bulks(VCHIQ_STATE_T *state)
{
   while (state->bulk.tx.resolve != state->bulk.tx.setup) {
      VCHIQ_TX_BULK_T *current_bulk = &state->bulk.tx.bulks[state->bulk.tx.resolve & VCHIQ_NUM_CURRENT_TX_BULKS - 1];
      VCHIQ_SERVICE_T *service = get_service(state, current_bulk->fourcc);
      VCHIQ_TX_BULK_T *service_bulk;

      assert(service!=NULL);

      if (service->install == service->setup)
         break;

      service_bulk = &service->bulks[service->install & VCHIQ_NUM_SERVICE_TX_BULKS - 1];

#ifdef USE_MEMMGR
      current_bulk->handle = service_bulk->handle;
#endif
      current_bulk->data = service_bulk->data;
      current_bulk->userdata = service_bulk->userdata;

      assert(current_bulk->size == service_bulk->size);

      state->bulk.tx.resolve++;

      service->install++;

      os_event_signal(&service->software);
   }
}

static void install_tx_bulks(VCHIQ_STATE_T *state)
{
   while (state->bulk.tx.install != state->bulk.tx.resolve) {
      VCHIQ_TX_BULK_T *bulk = &state->bulk.tx.bulks[state->bulk.tx.install & VCHIQ_NUM_CURRENT_TX_BULKS - 1];
      const void *data;

#ifdef USE_MEMMGR
      if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
         data = (const void *)((size_t)mem_lock(bulk->handle) + (size_t)bulk->data);
      else
#endif
         data = bulk->data;

#ifdef _VIDEOCORE_MPHI
      if (state->driver->add_transmit_entry(state->handle, data, bulk->size, MPHI_CHANNEL_BULK) != 0) {
#else
      if (state->bulk.tx.install == (short)(state->bulk.tx.remove + VCHIQ_NUM_DMA)) {
#endif
#ifdef USE_MEMMGR
         if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
            mem_unlock(bulk->handle);
#endif
         break;
      } else
         bulk->data = data;

      state->bulk.tx.install++;

#ifndef _VIDEOCORE_MPHI
      os_event_signal(&state->bulk.tx.hardware);
#endif
   }
}

static void notify_tx_bulks(VCHIQ_STATE_T *state)
{
#ifdef _VIDEOCORE_MPHI
   MPHI_CHANNEL_T channel;

   while (state->driver->get_transmit_status(state->handle, &channel) == 0) {
      if (channel != MPHI_CHANNEL_BULK)
         continue;
#else
   while (state->bulk.tx.notify != state->bulk.tx.remove) {
#endif
      VCHIQ_TX_BULK_T *bulk = &state->bulk.tx.bulks[state->bulk.tx.notify & VCHIQ_NUM_CURRENT_TX_BULKS - 1];
      VCHIQ_SERVICE_T *service = get_service(state, bulk->fourcc);

      assert(service!=NULL);

#ifdef USE_MEMMGR
      if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
         mem_unlock(bulk->handle);
#endif

      service->callback(VCHIQ_BULK_TRANSMIT_DONE, NULL, service->userdata, bulk->userdata);

      state->bulk.tx.notify++;
   }
}

static void install_rx_bulks(VCHIQ_STATE_T *state)
{
   while (state->bulk.rx.install != state->bulk.rx.setup) {
      VCHIQ_RX_BULK_T *bulk = &state->bulk.rx.bulks[state->bulk.rx.install & VCHIQ_NUM_RX_BULKS - 1];
      void *data;

#ifdef USE_MEMMGR
      if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
         data = (void *)((size_t)mem_lock(bulk->handle) + (size_t)bulk->data);
      else
#endif
         data = bulk->data;

#ifdef _VIDEOCORE_MPHI
      if (state->driver->add_receive_entry(state->handle, data, bulk->size, MPHI_CHANNEL_BULK) != 0) {
#else
      if (state->bulk.rx.install == (short)(state->bulk.rx.remove + VCHIQ_NUM_DMA)) {
#endif
#ifdef USE_MEMMGR
         if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
            mem_unlock(bulk->handle);
#endif
         break;
      } else
         bulk->data = data;

      state->bulk.rx.install++;

      os_event_signal(&state->worker);
   }
}

static void notify_rx_bulks(VCHIQ_STATE_T *state)
{
#ifdef _VIDEOCORE_MPHI
   int complete;
   int position;

   while (state->driver->get_receive_status(state->handle, &complete, &position, MPHI_CHANNEL_BULK) == 0 && complete) {
#else
   while (state->bulk.rx.notify != state->bulk.rx.remove) {
#endif
      VCHIQ_RX_BULK_T *bulk = &state->bulk.rx.bulks[state->bulk.rx.notify & VCHIQ_NUM_RX_BULKS - 1];
      VCHIQ_SERVICE_T *service = get_service(state, bulk->fourcc);

      assert(service!=NULL);

#ifdef USE_MEMMGR
      if (bulk->handle != VCHI_MEM_HANDLE_INVALID)
         mem_unlock(bulk->handle);
#endif

      service->callback(VCHIQ_BULK_RECEIVE_DONE, NULL, service->userdata, bulk->userdata);

      state->bulk.rx.notify++;

      os_event_signal(&state->bulk.rx.software);
   }
}

static int parse_rx_slot(VCHIQ_STATE_T *state, VCHIQ_SLOT_T *slot, unsigned int pos, unsigned int end)
{
   while (pos < end) {
      VCHIQ_HEADER_T *header = (VCHIQ_HEADER_T *)(slot->data + pos);
      VCHIQ_SERVICE_T *service = NULL;
      unsigned int stride = calc_stride(header->size);

      if (pos + stride > end)
         break;

      slot->received++;

      if (state->ctrl.tx.peer != header->u.s.peer) {
         state->ctrl.tx.peer = header->u.s.peer;

         os_event_signal(&state->ctrl.tx.software);
      }

      switch (header->u.s.fourcc) {
         /* TODO: this suspend/req/ack/fin stuff is a temporary fix
            for SW-4690.  It must be reviewed and perhaps replaced */
      case VCHIQ_FOURCC_SUSPENDREQ:
         assert(!state->suspendreq);
         state->suspendreq = 1;
         os_event_signal(&state->suspend);
         break;
      case VCHIQ_FOURCC_SUSPENDACK:
         os_event_signal(&state->suspendack);
         break;
      case VCHIQ_FOURCC_SUSPENDFIN:
         assert(!state->suspendfin);
         state->suspendfin = 1;
         os_event_signal(&state->suspend);
         break;

      case VCHIQ_FOURCC_HEARTBEAT:
      case VCHIQ_FOURCC_TERMINATE:
         break;
      case VCHIQ_FOURCC_CONNECT:
         os_event_signal(&state->connect);
         break;
      case VCHIQ_FOURCC_BULK:
      {
         VCHIQ_BULK_BODY_T *body = (VCHIQ_BULK_BODY_T *)header->data;
         VCHIQ_TX_BULK_T *bulk = &state->bulk.tx.bulks[state->bulk.tx.setup & VCHIQ_NUM_CURRENT_TX_BULKS - 1];

         assert(state->bulk.tx.setup != (short)(state->bulk.tx.notify + VCHIQ_NUM_CURRENT_TX_BULKS));

         bulk->fourcc = body->fourcc;
         bulk->data = NULL;
         bulk->size = body->size;

         state->bulk.tx.setup++;
         break;
      }
      default:
         service = get_service(state, header->u.s.fourcc);

         assert(service!=NULL);
         break;
      }

      header->u.slot = slot;

      if (!service || !service->callback(VCHIQ_MESSAGE_AVAILABLE, header, service->userdata, NULL)) {
         header->u.slot = NULL;
         slot->released++;
      }

      pos += stride;
   }

   assert(pos <= end);

   return pos;
}

#ifndef _VIDEOCORE_MPHI
static int get_receive_status(VCHIQ_STATE_T *state, int *complete, int *position)
{
   VCHIQ_ATOMIC_SHORTS_T remove;

   remove.atomic = state->ctrl.rx.remove.atomic;

   if (remove.s.slot == state->ctrl.rx.process.s.slot) {
      *complete = 0;
      *position = remove.s.mark;
   } else {
      *complete = 1;
      *position = state->ctrl.rx.slots[state->ctrl.rx.process.s.slot & VCHIQ_NUM_RX_SLOTS - 1].size;
   }

   return 0;
}
#endif

static void parse_rx_slots(VCHIQ_STATE_T *state)
{
   int complete;
   int position;

#ifdef _VIDEOCORE_MPHI
   while (state->driver->get_receive_status(state->handle, &complete, &position, MPHI_CHANNEL_CTRL) == 0) {
#else
   while (get_receive_status(state, &complete, &position) == 0) {
#endif
      VCHIQ_SLOT_T *slot = &state->ctrl.rx.slots[state->ctrl.rx.process.s.slot & VCHIQ_NUM_RX_SLOTS - 1];

      position = parse_rx_slot(state, slot, state->ctrl.rx.process.s.mark, position);

      if (complete) {
         slot->size = position;

         state->ctrl.rx.process.s.slot++;
         state->ctrl.rx.process.s.mark = 0;
      } else {
         state->ctrl.rx.process.s.mark = position;
         break;
      }
   }
}

static void slot_handler_func(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;

   while (1) {
      os_event_wait(&state->trigger);
      notify_tx_bulks(state);       // also responsible for emptying MPHIQ fake transmit FIFO

      parse_rx_slots(state);
      install_rx_slots(state);
      install_tx_tasks(state);

      resolve_tx_bulks(state);
      install_tx_bulks(state);
      install_rx_bulks(state);
      notify_rx_bulks(state);
   }
}

static void worker_func(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;

   while (1) {
      os_event_wait(&state->worker);

      if (state->bulk.rx.message != state->bulk.rx.install)
         while (state->bulk.rx.message != state->bulk.rx.install) {
            VCHIQ_RX_BULK_T *bulk = &state->bulk.rx.bulks[state->bulk.rx.message & VCHIQ_NUM_RX_BULKS - 1];
#if  defined(__CC_ARM)
            VCHIQ_BULK_BODY_T body;
            VCHIQ_ELEMENT_T element;
			body.fourcc = bulk->fourcc;
			body.size = bulk->size;
			element.data = &body;
			element.size = (int) sizeof(body);
#else
            VCHIQ_BULK_BODY_T body = {bulk->fourcc, bulk->size};
            VCHIQ_ELEMENT_T element = {&body, (int) sizeof(body)};
#endif

            state->bulk.rx.message++;

            vchiq_queue_message(state, VCHIQ_FOURCC_BULK, &element, 1);
         }
      else {
         VCHIQ_ELEMENT_T element = {NULL, 0};

         vchiq_queue_message(state, VCHIQ_FOURCC_HEARTBEAT, &element, 1);
      }
   }
}

/* This is the suspend task - it's job is to answer REQ with ACK and
   hold off the transmit queue to prevent further messages going down
   the pipe.  TODO: This needs to be reviewed and perhaps replaces as
   part of SW-4690 */
#ifndef VCHI_LOCAL_HOST_PORT
static void suspend_func(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;
   VCHIQ_ELEMENT_T element = {NULL, 0};
   
   while (1) {
      os_event_wait(&state->suspend);

      if(state->suspendreq) {
         /* Suspend request received.  Ought to send ACK message as _last_ message, 
            but I'll be tolerant of a message or two behind it for now! */
         
         state->suspendreq = 0;
         /* Send out the suspend ACK message to tell the other side that we are
            ready to suspend. Contrary to the above comment, the ACK message
            should be the LAST message that we send, otherwise we risk losing
            any messages that are sent after the ACK message.
            To prevent any more messages from being sent out after the ACK
            message, the ctrl.tx.mutex is not released from the following
            function call. */         
		vchiq_queue_message(state, VCHIQ_FOURCC_SUSPENDACK, &element, 1);
		 
      }

      if(state->suspendfin) {
         /* FIN received.  Let stuff go again */

         state->suspendfin = 0;

         os_mutex_release(&state->ctrl.tx.mutex);
      }
   }
}
#endif

static __inline int is_pow2(int i)
{
   return i && !(i & i - 1);
}

void vchiq_init_state(VCHIQ_STATE_T *state)
{
   int i;

   assert(is_pow2(VCHIQ_NUM_RX_SLOTS));                     // we require these to make various circular buffers work
   assert(is_pow2(VCHIQ_NUM_TX_SLOTS));
   assert(is_pow2(VCHIQ_NUM_RX_BULKS));
   assert(is_pow2(VCHIQ_NUM_CURRENT_TX_BULKS));
   assert(is_pow2(VCHIQ_NUM_SERVICE_TX_BULKS));
   assert(is_pow2(VCHIQ_NUM_TX_TASKS));

   assert(VCHIQ_SLOT_SIZE < 32768);                         // we require this as markers are represented by signed shorts


   assert(VCHIQ_NUM_TX_SLOTS >= VCHIQ_NUM_DMA);             // we require this to prevent trashing outgoing slot data
   assert(VCHIQ_NUM_CURRENT_TX_BULKS >= 2 * VCHIQ_NUM_DMA); // we require this to prevent trashing bulks before notifying completion

   memset(state, 0, sizeof(VCHIQ_STATE_T));

   /*
      initialize events and mutex
   */

   os_event_create(&state->connect);
   os_event_create(&state->trigger);
   os_event_create(&state->worker);
   os_mutex_create(&state->mutex);

   /* Inter-thread communications for SW-4690.  TODO: Review and perhaps replace */
   os_event_create(&state->suspend);
   os_event_create(&state->suspendack);
   state->suspendreq = 0;
   state->suspendfin = 0;

   /*
      open MPHI or USB driver
   */

#ifdef _VIDEOCORE_MPHI
   state->driver = mphi_get_func_table();

   MPHI_OPEN_T open_params = {&state->trigger};

   CHECK(state->driver->open(&open_params, &state->handle) == 0);
#endif
#ifdef USE_USBHP
   state->handle = hp_open(NULL, 0, HPDT_ANY, 1);

   assert(state->handle != NULL);
#endif

   /*
      initialize control receiver state
   */

   for (i = 0; i < VCHIQ_NUM_RX_SLOTS; i++)
      state->ctrl.rx.slots[i].data = (char *)defined_alloc(VCHIQ_SLOT_SIZE, "rx slot data");

   state->ctrl.rx.peer = VCHIQ_NUM_DMA;

   install_rx_slots(state);

   assert(state->ctrl.rx.install == VCHIQ_NUM_DMA);

   /*
      initialize control transmitter state
   */

   for (i = 0; i < VCHIQ_NUM_TX_SLOTS; i++)
      state->ctrl.tx.slots[i].data = (char *)defined_alloc(VCHIQ_SLOT_SIZE, "tx slot data");

   state->ctrl.tx.peer = VCHIQ_NUM_DMA;

   os_event_create(&state->ctrl.tx.software);
#ifndef _VIDEOCORE_MPHI
   os_event_create(&state->ctrl.tx.hardware);
#endif
   os_mutex_create(&state->ctrl.tx.mutex);

   /*
      initialize bulk receiver state
   */

   os_event_create(&state->bulk.rx.software);
   os_mutex_create(&state->bulk.rx.mutex);

   /*
      initialize bulk transmitter state
   */

#ifndef _VIDEOCORE_MPHI
   os_event_create(&state->bulk.tx.hardware);
#endif

   /*
      initialize services
   */

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      os_event_create(&state->services[i].software);

   /*
      bring up slot handler thread
   */

   os_thread_begin(&state->slot_handler_thread, slot_handler_func, VCHIQ_SLOT_HANDLER_STACK, state, "VCHIQ slot handler");

   /*
      enable MPHI or USB driver
   */

#ifdef _VIDEOCORE_MPHI
   CHECK(state->driver->enable(state->handle) == 0);
#endif
#ifdef USE_USBHP
   os_thread_begin( &state->usb_thread, usbhp_thread, 0, state, "VCHIQ USB");
#endif
}

int vchiq_add_service(VCHIQ_STATE_T *state, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata)
{
   int i;

   os_mutex_acquire(&state->mutex);

   assert(!get_service(state, fourcc));

   for (i = 0; i < VCHIQ_MAX_SERVICES; i++)
      if (state->services[i].fourcc == VCHIQ_FOURCC_INVALID) {
         state->services[i].callback = callback;
         state->services[i].userdata = userdata;
         state->services[i].fourcc = fourcc;

         os_mutex_release(&state->mutex);
         return 1;
      }

   os_mutex_release(&state->mutex);
   return 0;
}

void vchiq_queue_bulk_transmit(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, const void *data, int size, void *userdata)
{
   VCHIQ_SERVICE_T *service = get_service(state, fourcc);
   VCHIQ_TX_BULK_T *bulk;

#ifdef _VIDEOCORE_MPHI
   assert( handle != VCHI_MEM_HANDLE_INVALID || (data && !((size_t)data & 0xf)) );
#endif
   assert(!(size & 0xf));
   assert(service!=NULL);

   while (service->setup == (short)(service->install + VCHIQ_NUM_SERVICE_TX_BULKS))
      os_event_wait(&service->software);

   bulk = &service->bulks[service->setup & VCHIQ_NUM_SERVICE_TX_BULKS - 1];
#ifdef USE_MEMMGR
   bulk->handle = handle;
#endif
   bulk->data = data;
   bulk->size = size;
   bulk->userdata = userdata;

   service->setup++;

   os_event_signal(&state->trigger);
}

void vchiq_queue_bulk_receive(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, void *data, int size, void *userdata)
{
   VCHIQ_RX_BULK_T *bulk;
#ifdef _VIDEOCORE_MPHI
   assert( handle != VCHI_MEM_HANDLE_INVALID || (data && !((size_t)data & 0xf)) );
#endif
   assert(!(size & 0xf));

   os_mutex_acquire(&state->bulk.rx.mutex);

   while(state->bulk.rx.setup == (short)(state->bulk.rx.notify + VCHIQ_NUM_RX_BULKS))
      os_event_wait(&state->bulk.rx.software);

   bulk = &state->bulk.rx.bulks[state->bulk.rx.setup & VCHIQ_NUM_RX_BULKS - 1];
#ifdef USE_MEMMGR
   bulk->handle = handle;
#endif
   bulk->fourcc = fourcc;
   bulk->data = data;
   bulk->size = size;
   bulk->userdata = userdata;

   state->bulk.rx.setup++;

   os_mutex_release(&state->bulk.rx.mutex);

   os_event_signal(&state->trigger);
}

void vchiq_queue_message(VCHIQ_STATE_T *state, int fourcc, const VCHIQ_ELEMENT_T *elements, int count)
{
   unsigned int size = 0;
   unsigned int term = 0;
   unsigned int stride;
   unsigned int i, pos;

   VCHIQ_HEADER_T *header;
   VCHIQ_TASK_T *task;

   for (i = 0; i < (unsigned int)count; i++)
      size += elements[i].size;

   stride = calc_stride(size);

   assert(fourcc != VCHIQ_FOURCC_INVALID);
   assert(stride <= VCHIQ_SLOT_SIZE);

   /*
      Mutex is already held when sending the suspend REQ message so we do not 
      need to acquire it when sending the suspend FIN message. Of course, this
      implies that suspend REQ is the last message we send before suspending,
      and suspend FIN is the first message when resuming.
   */
   if (fourcc != VCHIQ_FOURCC_SUSPENDFIN)
   {
      os_mutex_acquire(&state->ctrl.tx.mutex);
   }
   
   if (state->ctrl.tx.fill.s.mark + stride > VCHIQ_SLOT_SIZE) {
      term = state->ctrl.tx.fill.s.mark < VCHIQ_SLOT_SIZE;

      state->ctrl.tx.fill.s.slot++;
      state->ctrl.tx.fill.s.mark = 0;
   }

   if (state->ctrl.tx.fill.s.mark == 0)
      while (state->ctrl.tx.fill.s.slot == state->ctrl.tx.peer)
         os_event_wait(&state->ctrl.tx.software);

   /*
      write into slot
   */

   header = (VCHIQ_HEADER_T *)(state->ctrl.tx.slots[state->ctrl.tx.fill.s.slot & VCHIQ_NUM_TX_SLOTS - 1].data + state->ctrl.tx.fill.s.mark);
   header->u.s.fourcc = fourcc;
   header->size = size;

   for (i = 0, pos = 0; i < (unsigned int)count; pos += elements[i++].size)
      if (elements[i].data)
         memcpy(header->data + pos, elements[i].data, (size_t) elements[i].size);

   state->ctrl.tx.fill.s.mark += stride;

   /*
      create task
   */

   task = &state->ctrl.tx.tasks[state->ctrl.tx.setup & VCHIQ_NUM_TX_TASKS - 1];
   task->data = header;
   task->size = stride;
   task->slot = state->ctrl.tx.fill.s.slot;
   task->term = term;

   state->ctrl.tx.setup++;

   os_event_signal(&state->trigger);

   /*
     Do not release the mutex if we are queuing the suspend REQ or ACK message
     because it should be the very last message that we queue to send out
     before going into suspend.
   */
   if ((fourcc != VCHIQ_FOURCC_SUSPENDACK) &&
       (fourcc != VCHIQ_FOURCC_SUSPENDREQ))
   {
      os_mutex_release(&state->ctrl.tx.mutex);
   }
}

void vchiq_release_message(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header)
{
   assert(header->u.slot != NULL);
   assert(header->u.slot->released < header->u.slot->received);

   header->u.slot->released++;
   header->u.slot = NULL;

   os_event_signal(&state->trigger);
}

/* vcsuspend callback for SW-4690.  TODO: Review / Replace */
#ifndef VCHI_LOCAL_HOST_PORT
#ifdef _VIDEOCORE
static int32_t vchiq_do_suspend_resume(uint32_t up, VCSUSPEND_CALLBACK_ARG_T priv)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)priv;

   assert(state);

   VCHIQ_ELEMENT_T element = {&up, sizeof(up)};

   if(!up) {
      /* suspend */

      /* tell other side to stop sending */
      vchiq_queue_message(state, VCHIQ_FOURCC_SUSPENDREQ, &element, 1);

      os_event_wait(&state->suspendack);
   }
   else {
      /* resume */
      /* tell other side to send again */
      vchiq_queue_message(state, VCHIQ_FOURCC_SUSPENDFIN, &element, 1);
   }

   return 0;
}
#endif // _VIDEOCORE
#endif // !VCHI_LOCAL_HOST_PORT

void vchiq_connect(VCHIQ_STATE_T *state)
{
#ifdef __CC_ARM
   VCHIQ_ELEMENT_T element = {NULL, VCHIQ_SLOT_SIZE - (int)sizeof(VCHIQ_HEADER_T) - 4};
#else
   VCHIQ_ELEMENT_T element = {NULL, VCHIQ_SLOT_SIZE - (int)sizeof(VCHIQ_HEADER_T)};
#endif

   vchiq_queue_message(state, VCHIQ_FOURCC_CONNECT, &element, 1);
   os_event_wait(&state->connect);
   vchiq_queue_message(state, VCHIQ_FOURCC_CONNECT, &element, 1);

   /*
      bring up worker thread
   */

   os_thread_begin(&state->worker_thread, worker_func, VCHIQ_WORKER_STACK, state, "VCHIQ worker");

   /* TODO: review this code for suitability as SW-4690 fix */
#ifndef VCHI_LOCAL_HOST_PORT
   /* don't bother if it's all local anyway */
#ifdef _VIDEOCORE
   /* only videocore can suspend */
   {
      static int firsttime = 1;

      assert(firsttime);

      vcsuspend_register(VCSUSPEND_RUNLEVEL_VCHIQ, vchiq_do_suspend_resume, (VCSUSPEND_CALLBACK_ARG_T)state);
      firsttime = 0;
   }
#endif
   /* this is wasteful, but I just want to get _some_ sort of hack to work... :) */
   os_thread_begin(&state->suspend_thread, suspend_func, 1024, state, "VCHIQ suspend func");
#endif
}
