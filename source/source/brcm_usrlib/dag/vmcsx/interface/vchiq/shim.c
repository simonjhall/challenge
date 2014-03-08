#include "interface/vchi/vchi.h"

#include "vchiq.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#ifdef WIN32
#include <process.h>
#endif

typedef enum {
   MODE_NONE,
   MODE_CALLBACK,
   MODE_BLOCKING
} SHIM_BULK_MODE_T;

typedef struct {
   VCHIQ_STATE_T *state;
   int fourcc;

   VCHIU_QUEUE_T queue;

   VCHI_CALLBACK_T callback;
   void *callback_param;

   /*
      bulk blocking stuff
   */

   struct {
      SHIM_BULK_MODE_T mode;
      OS_EVENT_T event;
   } receive;

   struct {
      SHIM_BULK_MODE_T mode;
      OS_EVENT_T event;
   } transmit;
} SHIM_SERVICE_T;

/* ----------------------------------------------------------------------
 * return pointer to the mphi message driver function table
 * -------------------------------------------------------------------- */
#ifdef WIN32
const VCHI_MESSAGE_DRIVER_T *
mphi_get_func_table( void )
{
   return NULL;
}
#endif

/* ----------------------------------------------------------------------
 * return pointer to the mphi message driver function table
 * -------------------------------------------------------------------- */
const VCHI_MESSAGE_DRIVER_T *
vchi_mphi_message_driver_func_table( void )
{
   return NULL;
}

/* ----------------------------------------------------------------------
 * return a pointer to the 'single' connection driver fops
 * -------------------------------------------------------------------- */
const VCHI_CONNECTION_API_T *
single_get_func_table( void )
{
   return NULL;
}

VCHI_CONNECTION_T * vchi_create_connection( const VCHI_CONNECTION_API_T * function_table,
                                            const VCHI_MESSAGE_DRIVER_T * low_level)
{
   return NULL;
}

/***********************************************************
 * Name: vchi_msg_peek
 *
 * Arguments:  const VCHI_SERVICE_HANDLE_T handle,
 *             void **data,
 *             uint32_t *msg_size,
 *             VCHI_FLAGS_T flags
 *
 * Description: Routine to return a pointer to the current message (to allow in place processing)
 *              The message can be removed using vchi_msg_remove when you're finished
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_peek( VCHI_SERVICE_HANDLE_T handle,
                       void **data,
                       uint32_t *msg_size,
                       VCHI_FLAGS_T flags )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;
   VCHIQ_HEADER_T *header;

   assert(flags == VCHI_FLAGS_NONE || flags == VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);

   if (flags == VCHI_FLAGS_NONE)
      if (vchiu_queue_is_empty(&service->queue))
         return -1;

   header = vchiu_queue_peek(&service->queue);

   *data = header->data;
   *msg_size = header->size;

   return 0;
}

/***********************************************************
 * Name: vchi_msg_remove
 *
 * Arguments:  const VCHI_SERVICE_HANDLE_T handle,
 *
 * Description: Routine to remove a message (after it has been read with vchi_msg_peek)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_remove( VCHI_SERVICE_HANDLE_T handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;
   VCHIQ_HEADER_T *header;

   header = vchiu_queue_pop(&service->queue);

   vchiq_release_message(service->state, header);

   return 0;
}

/***********************************************************
 * Name: vchi_msg_queue
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             const void *data,
 *             uint32_t data_size,
 *             VCHI_FLAGS_T flags,
 *             void *msg_handle,
 *
 * Description: Thin wrapper to queue a message onto a connection
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_queue( VCHI_SERVICE_HANDLE_T handle,
                        const void * data,
                        uint32_t data_size,
                        VCHI_FLAGS_T flags,
                        void * msg_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

#if  defined(__CC_ARM)
   VCHIQ_ELEMENT_T element;
   element.data = data;
   element.size = data_size;
#else
   VCHIQ_ELEMENT_T element = {data, data_size};
#endif

   assert(flags == VCHI_FLAGS_BLOCK_UNTIL_QUEUED);

   vchiq_queue_message(service->state, service->fourcc, &element, 1);

   return 0;
}

/***********************************************************
 * Name: vchi_bulk_queue_receive
 *
 * Arguments:  VCHI_BULK_HANDLE_T handle,
 *             void *data_dst,
 *             const uint32_t data_size,
 *             VCHI_FLAGS_T flags
 *             void *bulk_handle
 *
 * Description: Routine to setup a rcv buffer
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_bulk_queue_receive( VCHI_SERVICE_HANDLE_T handle,
                                 void * data_dst,
                                 uint32_t data_size,
                                 VCHI_FLAGS_T flags,
                                 void * bulk_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

   void *my_data_dst = data_dst;
   uint32_t my_data_size = data_size;
   VCHI_FLAGS_T my_flags = flags;

   if ((data_size & 0xf) || ((size_t)data_dst & 0xf)) {
      my_data_size = data_size + 15 & ~15;
      my_data_dst = malloc(my_data_size);
      my_flags = VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE;
   }

   switch (my_flags) {
   case VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
      assert(service->receive.mode != MODE_BLOCKING);
      service->receive.mode = MODE_CALLBACK;
      break;
   case VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE:
      assert(service->receive.mode != MODE_CALLBACK);
      service->receive.mode = MODE_BLOCKING;
      break;
   default:
      assert(0);
      break;
   }

   vchiq_queue_bulk_receive(service->state, service->fourcc, VCHI_MEM_HANDLE_INVALID, my_data_dst, my_data_size, bulk_handle);

   if (my_flags == VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)
      os_event_wait(&service->receive.event);

   if (my_data_dst != data_dst) {
      memcpy(data_dst, my_data_dst, data_size);

      free(my_data_dst);
   }

   return 0;
}

#ifdef USE_MEMMGR

/***********************************************************
 * Name: vchi_bulk_queue_receive_reloc
 *
 * Arguments:  VCHI_BULK_HANDLE_T handle,
 *             VCHI_MEM_HANDLE_T h
 *             uint32_t offset
 *             const uint32_t data_size,
 *             VCHI_FLAGS_T flags
 *             void *bulk_handle
 *
 * Description: Routine to setup a relocatable rcv buffer
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_bulk_queue_receive_reloc( const VCHI_SERVICE_HANDLE_T handle,
                                       VCHI_MEM_HANDLE_T h,
                                       uint32_t offset,
                                       uint32_t data_size,
                                       const VCHI_FLAGS_T flags,
                                       void * const bulk_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

   if ((offset & 0xf) ||
       (data_size & 0xf))
   {
      // can't do misaligned bulk transfers
      assert(0);
      return -1;
   }

   switch (flags) {
   case VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
      assert(service->receive.mode != MODE_BLOCKING);
      service->receive.mode = MODE_CALLBACK;
      break;
   case VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE:
      assert(service->receive.mode != MODE_CALLBACK);
      service->receive.mode = MODE_BLOCKING;
      break;
   default:
      assert(0);
      break;
   }

   vchiq_queue_bulk_receive(service->state, service->fourcc, h, (void*)offset, data_size, bulk_handle);

   if (flags == VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)
      os_event_wait(&service->receive.event);

   return 0;
}

#endif

/***********************************************************
 * Name: vchi_bulk_queue_transmit
 *
 * Arguments:  VCHI_BULK_HANDLE_T handle,
 *             const void *data_src,
 *             uint32_t data_size,
 *             VCHI_FLAGS_T flags,
 *             void *bulk_handle
 *
 * Description: Routine to transmit some data
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_bulk_queue_transmit( VCHI_SERVICE_HANDLE_T handle,
                                  const void * data_src,
                                  uint32_t data_size,
                                  VCHI_FLAGS_T flags,
                                  void * bulk_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

   void *my_data_src = (void *)data_src;
   uint32_t my_data_size = data_size;
   VCHI_FLAGS_T my_flags = flags;

   if ((data_size & 0xf) || ((size_t)data_src & 0xf)) {
      my_data_size = data_size + 15 & ~15;
      my_data_src = malloc(my_data_size);
      my_flags = VCHI_FLAGS_BLOCK_UNTIL_DATA_READ;

      memcpy(my_data_src, data_src, data_size);
   }

   switch (my_flags) {
   case VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
      assert(service->transmit.mode != MODE_BLOCKING);
      service->transmit.mode = MODE_CALLBACK;
      break;

   // hack: This blocks too much (although should be correct), and any code with VCHI_FLAGS_BLOCK_UNTIL_QUEUED will suffer a performance penalty
   // and should use either (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED) or VCHI_FLAGS_BLOCK_UNTIL_DATA_READ directly.
   /* FIXME: */ case VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
   case VCHI_FLAGS_BLOCK_UNTIL_DATA_READ:
      assert(service->transmit.mode != MODE_CALLBACK);
      service->transmit.mode = MODE_BLOCKING;
      break;
   default:
      assert(0);
      break;
   }

   vchiq_queue_bulk_transmit(service->state, service->fourcc, VCHI_MEM_HANDLE_INVALID, my_data_src, my_data_size, bulk_handle);

   if (my_flags == VCHI_FLAGS_BLOCK_UNTIL_DATA_READ)
      os_event_wait(&service->transmit.event);

   if (my_data_src != data_src)
      free(my_data_src);

   return 0;
}

#ifdef USE_MEMMGR

int32_t vchi_bulk_queue_transmit_reloc( VCHI_SERVICE_HANDLE_T handle,
                                        VCHI_MEM_HANDLE_T h_src,
                                        uint32_t offset,
                                        uint32_t data_size,
                                        VCHI_FLAGS_T flags,
                                        void * const bulk_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

   if ((data_size & 0xf) || (offset & 0xf)) {
      // can't do misaligned bulk transfers
      assert(0);
      return -1;
   }

   switch (flags) {
   case VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
      assert(service->transmit.mode != MODE_BLOCKING);
      service->transmit.mode = MODE_CALLBACK;
      break;

   // hack: This blocks too much (although should be correct), and any code with VCHI_FLAGS_BLOCK_UNTIL_QUEUED will suffer a performance penalty
   // and should use either (VCHI_FLAGS_CALLBACK_WHEN_OP_COMPLETE | VCHI_FLAGS_BLOCK_UNTIL_QUEUED) or VCHI_FLAGS_BLOCK_UNTIL_DATA_READ directly.
   /* FIXME: */ case VCHI_FLAGS_BLOCK_UNTIL_QUEUED:
   case VCHI_FLAGS_BLOCK_UNTIL_DATA_READ:
      assert(service->transmit.mode != MODE_CALLBACK);
      service->transmit.mode = MODE_BLOCKING;
      break;
   default:
      assert(0);
      break;
   }

   vchiq_queue_bulk_transmit(service->state, service->fourcc, h_src, (void*)offset, data_size, bulk_handle);

   if (flags == VCHI_FLAGS_BLOCK_UNTIL_DATA_READ)
      os_event_wait(&service->transmit.event);

   return 0;
}

#endif

/***********************************************************
 * Name: vchi_msg_dequeue
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             void *data,
 *             uint32_t max_data_size_to_read,
 *             uint32_t *actual_msg_size
 *             VCHI_FLAGS_T flags
 *
 * Description: Routine to dequeue a message into the supplied buffer
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_dequeue( VCHI_SERVICE_HANDLE_T handle,
                          void *data,
                          uint32_t max_data_size_to_read,
                          uint32_t *actual_msg_size,
                          VCHI_FLAGS_T flags )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;
   VCHIQ_HEADER_T *header;

   assert(flags == VCHI_FLAGS_NONE || flags == VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);

   if (flags == VCHI_FLAGS_NONE)
      if (vchiu_queue_is_empty(&service->queue))
         return -1;

   header = vchiu_queue_pop(&service->queue);

   memcpy(data, header->data, header->size < max_data_size_to_read ? header->size : max_data_size_to_read);

   *actual_msg_size = header->size;

   vchiq_release_message(service->state, header);

   return 0;
}

/***********************************************************
 * Name: vchi_msg_queuev
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             const void *data,
 *             uint32_t data_size,
 *             VCHI_FLAGS_T flags,
 *             void *msg_handle
 *
 * Description: Thin wrapper to queue a message onto a connection
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_queuev( VCHI_SERVICE_HANDLE_T handle,
                         VCHI_MSG_VECTOR_T * vector,
                         uint32_t count,
                         VCHI_FLAGS_T flags,
                         void *msg_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;

   assert(sizeof(VCHI_MSG_VECTOR_T) == sizeof(VCHIQ_ELEMENT_T));
   assert(offsetof(VCHI_MSG_VECTOR_T, vec_base) == offsetof(VCHIQ_ELEMENT_T, data));
   assert(offsetof(VCHI_MSG_VECTOR_T, vec_len) == offsetof(VCHIQ_ELEMENT_T, size));

   assert(flags == VCHI_FLAGS_BLOCK_UNTIL_QUEUED);

   vchiq_queue_message(service->state, service->fourcc, (const VCHIQ_ELEMENT_T *)vector, count);

   return 0;
}

#ifdef USE_MEMMGR

/***********************************************************
 * Name: vchi_msg_queuev_ex
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             VCHI_MSG_VECTOR_EX_T *vector
 *             uint32_t count
 *             VCHI_FLAGS_T flags,
 *             void *msg_handle
 *
 * Description: Thin wrapper to queue an array of messages onto a connection
 * Supports resolving MEM_HANDLE's at last possible moment to avoid deadlocks.
 *
 * Currently just a shim, so deadlocks are still possible!
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_queuev_ex( const VCHI_SERVICE_HANDLE_T handle,
                            VCHI_MSG_VECTOR_EX_T * const vector,
                            const uint32_t count,
                            const VCHI_FLAGS_T flags,
                            void * const msg_handle )
{
   int32_t success = -1;
   // For now, we don't actually support sending anything other than
   // a pointer, so handles have to be patched up; this is likely
   // to cause deadlocks. This code is not designed to be either
   // pretty, efficient, or deadlock-free.

   #define max_vecs 16
   VCHI_MSG_VECTOR_T copy[max_vecs];
   const uint8_t *orig[max_vecs];

   int i;
   if (count > sizeof(copy)/sizeof(copy[0]))
   {
      assert(0);
      return -1;
   }

   for (i=0; i<count; i++)
   {
      VCHI_MSG_VECTOR_EX_T *v = vector+i;

      switch (vector[i].type)
      {
      case VCHI_VEC_POINTER:
         copy[i].vec_base = v->u.ptr.vec_base;
         copy[i].vec_len =  v->u.ptr.vec_len;
         break;
      case VCHI_VEC_HANDLE:
         assert(v->u.handle.offset+v->u.handle.vec_len <= mem_get_size(v->u.handle.handle));
         copy[i].vec_base = (uint8_t*)mem_lock(v->u.handle.handle) + v->u.handle.offset;
         orig[i] = copy[i].vec_base;
         copy[i].vec_len = v->u.handle.vec_len;
         break;
      case VCHI_VEC_LIST:
         assert(0); // FIXME: implement this
         break;
      default:
         assert(0);
      }
   }
   success = vchi_msg_queuev( handle,
                              copy,
                              count,
                              flags &~ VCHI_FLAGS_INTERNAL,
                              msg_handle );
   assert(success == 0);

   // now we need to patch up the vectors if any have been only partially consumed, and
   // unlock memory handles.

   for (i=0; i<count; i++)
   {
      VCHI_MSG_VECTOR_EX_T *v = vector+i;

      switch (vector[i].type)
      {
      case VCHI_VEC_POINTER:
         if (flags & VCHI_FLAGS_ALLOW_PARTIAL)
         {
            v->u.ptr.vec_base = copy[i].vec_base;
            v->u.ptr.vec_len  = copy[i].vec_len;
         }
         break;
      case VCHI_VEC_HANDLE:
         mem_unlock(v->u.handle.handle);
         if (flags & VCHI_FLAGS_ALLOW_PARTIAL)
         {
            const uint8_t *old = orig[i];
            uint32_t change = (const uint8_t*)copy[i].vec_base-old;
            v->u.handle.offset += change;
            v->u.handle.vec_len -= change;
         }
         break;
      default:
         assert(0);
      }
   }

   return success;
}

#endif

/***********************************************************
 * Name: vchi_held_msg_release
 *
 * Arguments:  VCHI_HELD_MSG_T *message
 *
 * Description: Routine to release a held message (after it has been read with vchi_msg_hold)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_held_msg_release( VCHI_HELD_MSG_T *message )
{
   vchiq_release_message((VCHIQ_STATE_T *)message->service, (VCHIQ_HEADER_T *)message->message);

   return 0;
}

/***********************************************************
 * Name: vchi_msg_hold
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             void **data,
 *             uint32_t *msg_size,
 *             VCHI_FLAGS_T flags,
 *             VCHI_HELD_MSG_T *message_handle
 *
 * Description: Routine to return a pointer to the current message (to allow in place processing)
 *              The message is dequeued - don't forget to release the message using
 *              vchi_held_msg_release when you're finished
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_msg_hold( VCHI_SERVICE_HANDLE_T handle,
                       void **data,
                       uint32_t *msg_size,
                       VCHI_FLAGS_T flags,
                       VCHI_HELD_MSG_T *message_handle )
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)handle;
   VCHIQ_HEADER_T *header;

   assert(flags == VCHI_FLAGS_NONE || flags == VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);

   if (flags == VCHI_FLAGS_NONE)
      if (vchiu_queue_is_empty(&service->queue))
         return -1;

   header = vchiu_queue_pop(&service->queue);

   *data = header->data;
   *msg_size = header->size;

   message_handle->service = (struct opaque_vchi_service_t *)service->state;
   message_handle->message = header;

   return 0;
}

/***********************************************************
 * Name: vchi_initialise
 *
 * Arguments: VCHI_INSTANCE_T *instance_handle
 *            VCHI_CONNECTION_T **connections
 *            const uint32_t num_connections
 *
 * Description: Initialises the hardware but does not transmit anything
 *              When run as a Host App this will be called twice hence the need
 *              to malloc the state information
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/

#ifdef VCHI_LOCAL_HOST_PORT
#define SHIM_NUM_STATES 2
extern void local_init(VCHIQ_STATE_T *client, VCHIQ_STATE_T *server);
#else
#define SHIM_NUM_STATES 1
#endif

static VCHIQ_STATE_T states[SHIM_NUM_STATES];
static int next_state = 0;

int32_t vchi_initialise( VCHI_INSTANCE_T *instance_handle )
{
   os_semaphore_obtain_global();

   assert(next_state < SHIM_NUM_STATES);

   vchiq_init_state(&states[next_state]);

#ifdef VCHI_LOCAL_HOST_PORT
   if (next_state == 1)
      local_init(&states[0], &states[1]);
#endif

   *instance_handle = (VCHI_INSTANCE_T)&states[next_state++];

   os_semaphore_release_global();
   return 0;
}

/***********************************************************
 * Name: vchi_connect
 *
 * Arguments: VCHI_CONNECTION_T **connections
 *            const uint32_t num_connections
 *            VCHI_INSTANCE_T instance_handle )
 *
 * Description: Starts the command service on each connection,
 *              causing INIT messages to be pinged back and forth
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
int32_t vchi_connect( VCHI_CONNECTION_T **connections,
                      const uint32_t num_connections,
                      VCHI_INSTANCE_T instance_handle )
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)instance_handle;

   vchiq_connect(state);

   return 0;
}


/***********************************************************
 * Name: vchi_disconnect
 *
 * Arguments: VCHI_INSTANCE_T instance_handle
 *
 * Description: Stops the command service on each connection,
 *              causing DE-INIT messages to be pinged back and forth
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
int32_t vchi_disconnect( VCHI_INSTANCE_T instance_handle )
{
   //YTI
   return 0;
}


/***********************************************************
 * Name: vchi_service_open
 * Name: vchi_service_create
 *
 * Arguments: VCHI_INSTANCE_T *instance_handle
 *            SERVICE_CREATION_T *setup,
 *            VCHI_SERVICE_HANDLE_T *handle
 *
 * Description: Routine to open a service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

static int shim_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header, void *service_user, void *bulk_user)
{
   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)service_user;

   switch (reason) {
   case VCHIQ_MESSAGE_AVAILABLE:
      vchiu_queue_push(&service->queue, header);

      if (service->callback)
         service->callback(service->callback_param, VCHI_CALLBACK_MSG_AVAILABLE, NULL);
      break;
   case VCHIQ_BULK_TRANSMIT_DONE:
      switch (service->transmit.mode) {
      case MODE_CALLBACK:
         if (service->callback)
            service->callback(service->callback_param, VCHI_CALLBACK_BULK_SENT, bulk_user);
         break;
      case MODE_BLOCKING:
         os_event_signal(&service->transmit.event);
         break;
      }
      break;
   case VCHIQ_BULK_RECEIVE_DONE:
      switch (service->receive.mode) {
      case MODE_CALLBACK:
         if (service->callback)
            service->callback(service->callback_param, VCHI_CALLBACK_BULK_RECEIVED, bulk_user);
         break;
      case MODE_BLOCKING:
         os_event_signal(&service->receive.event);
         break;
      }
      break;
   default:
      assert(0);
      break;
   }

   return 1;
}

static int32_t service_create_internal(VCHI_INSTANCE_T instance_handle,
                           SERVICE_CREATION_T *setup,
                           VCHI_SERVICE_HANDLE_T *handle)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)instance_handle;

   SHIM_SERVICE_T *service = (SHIM_SERVICE_T *)malloc(sizeof(SHIM_SERVICE_T));

   int success;

   service->state = state;
   service->fourcc = setup->service_id;

#ifdef ANDROID
   vchiu_queue_init(&service->queue, 256);
#else
   vchiu_queue_init(&service->queue, 64);
#endif

   service->callback = setup->callback;
   service->callback_param = setup->callback_param;

   service->receive.mode = MODE_NONE;
   os_event_create(&service->receive.event);

   service->transmit.mode = MODE_NONE;
   os_event_create(&service->transmit.event);

   success = vchiq_add_service(state, setup->service_id, shim_callback, service);

   if(vcos_verify(success)) {
      *handle = (VCHI_SERVICE_HANDLE_T)service;
      return 0;
   } else {
      //No free slot for our new service
      free(service);
      *handle = (VCHI_SERVICE_HANDLE_T) 0;
      return -1;
   }
}

int32_t vchi_service_open( VCHI_INSTANCE_T instance_handle,
                           SERVICE_CREATION_T *setup,
                           VCHI_SERVICE_HANDLE_T *handle)
{
   return service_create_internal(instance_handle, setup, handle);
}

int32_t vchi_service_create( VCHI_INSTANCE_T instance_handle,
                             SERVICE_CREATION_T *setup,
                             VCHI_SERVICE_HANDLE_T *handle )
{
   return service_create_internal(instance_handle, setup, handle);
}

int32_t vchi_service_close( const VCHI_SERVICE_HANDLE_T handle )
{
   // YTI??
   return 0;
}

/* ----------------------------------------------------------------------
 * read a uint32_t from buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
uint32_t
vchi_readbuf_uint32( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24);
}

/* ----------------------------------------------------------------------
 * write a uint32_t to buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
void
vchi_writebuf_uint32( void *_ptr, uint32_t value )
{
   unsigned char *ptr = (unsigned char *)_ptr;
   ptr[0] = (unsigned char)((value >> 0)  & 0xFF);
   ptr[1] = (unsigned char)((value >> 8)  & 0xFF);
   ptr[2] = (unsigned char)((value >> 16) & 0xFF);
   ptr[3] = (unsigned char)((value >> 24) & 0xFF);
}

/* ----------------------------------------------------------------------
 * read a uint16_t from buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
uint16_t
vchi_readbuf_uint16( const void *_ptr )
{
   const unsigned char *ptr = (const unsigned char *)_ptr;
   return ptr[0] | (ptr[1] << 8);
}

/* ----------------------------------------------------------------------
 * write a uint16_t into the buffer.
 * network format is defined to be little endian
 * -------------------------------------------------------------------- */
void
vchi_writebuf_uint16( void *_ptr, uint16_t value )
{
   unsigned char *ptr = (unsigned char *)_ptr;
   ptr[0] = (value >> 0)  & 0xFF;
   ptr[1] = (value >> 8)  & 0xFF;
}

/***********************************************************
 * Name: vchi_service_use
 *
 * Arguments: const VCHI_SERVICE_HANDLE_T handle
 *
 * Description: Routine to increment refcount on a service
 *
 * Returns: void
 *
 ***********************************************************/
int32_t vchi_service_use( const VCHI_SERVICE_HANDLE_T handle )
{
   //not used on videocore
   return 0;
}

/***********************************************************
 * Name: vchi_service_release
 *
 * Arguments: const VCHI_SERVICE_HANDLE_T handle
 *
 * Description: Routine to decrement refcount on a service
 *
 * Returns: void
 *
 ***********************************************************/
int32_t vchi_service_release( const VCHI_SERVICE_HANDLE_T handle )
{
   //not used on videocore
   return 0;
}



