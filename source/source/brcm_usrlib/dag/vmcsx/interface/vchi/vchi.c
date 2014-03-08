/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile:  $
Revision :  $Revision: #7 $

FILE DESCRIPTION:

Top level API for passing messages and data between Videocore and a host processor.
There are potentially multiple hosts each running a multiplicity of services. The
services are based on a client/server arrangement. Servers are created using
'vchi_service_create' on the processor the code runs on. Both sides must open the
same service to be able to use it. The control service is started
during 'vchi_initialise' and is used to handle all low level housekeeping.

A service (client or server) can only be opened/created once per connection.

=============================================================================*/

#include <string.h>

#include "interface/vchi/vchi.h"
#include "interface/vchi/os/os.h"
#include "interface/vchi/connections/connection.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/control_service/control_service.h"
#include "interface/vchi/control_service/bulk_aux_service.h"

/******************************************************************************
Private typedefs
******************************************************************************/

#define VCHI_MAX_NUM_SERVICES (VCHI_MAX_SERVICES_PER_CONNECTION*VCHI_MAX_NUM_CONNECTIONS)

// There is code around that transfers relocatable memory across VCHI, and there's
// not much can be done about it right now.
#define ALLOW_RELOC_XFERS  1

// is this 4cc a server or a client?
typedef enum {
   UNUSED  = 0x0,
   SERVER  = 0x1,
   CLIENT  = 0x2
} VCHI_SERVICE_STATE_T;

typedef struct opaque_vchi_service_t
{
   // what is the current state of this slot
   VCHI_SERVICE_STATE_T state;

   // which service is it?
   fourcc_t service_id;

   // which connection is it?
   VCHI_CONNECTION_T *connection;

   // pointer to the info the connection layer needs for this service
   VCHI_CONNECTION_SERVICE_HANDLE_T service_handle;

} VCHI_SERVICE_T;


typedef struct
{
   // os semaphore use to protect this structure
   OS_SEMAPHORE_T semaphore;

   // information about each service / connection pair
   VCHI_SERVICE_T services[ VCHI_MAX_NUM_SERVICES ];

   // need some static state for bulk and control services
   void *bulk_aux_service_state;
   void *control_service_state;

} VCHI_STATE_T;


/******************************************************************************
Static data
******************************************************************************/

//static VCHI_STATE_T vchi_state;

/******************************************************************************
Static func forwards
******************************************************************************/

// find a free service slot and check that we are not trying to open something multiple times
static VCHI_SERVICE_T *vchi_find_free_service( fourcc_t service_id, VCHI_CONNECTION_T * connection, const VCHI_STATE_T *state );

#ifdef VCHI_COARSE_LOCKING
static void vchi_lock(VCHI_CONNECTION_T *connection)
{
   int32_t s = os_semaphore_obtain(&connection->sem);
   os_assert(s == 0);
}

static void vchi_unlock(VCHI_CONNECTION_T *connection)
{
   int32_t s = os_semaphore_release(&connection->sem);
   os_assert(s == 0);
}
#else
#define vchi_lock(c)            ((void)0)
#define vchi_unlock(c)          ((void)0)
#endif
#define vchi_unlock_return(c,r) (vchi_unlock(c),(r))


/******************************************************************************
Global functions
******************************************************************************/

VCHI_CONNECTION_T connections[VCHI_MAX_NUM_CONNECTIONS];

extern VCHI_CONNECTION_T * vchi_create_connection( const VCHI_CONNECTION_API_T * function_table,
                                                   const VCHI_MESSAGE_DRIVER_T * low_level)
{
static int32_t connection_number = 0;
VCHI_CONNECTION_T * ptr;
int32_t success;
   success = os_semaphore_obtain_global();
   os_assert(success == 0);
   // move on to next entry in table
   ptr = &connections[connection_number++];
   // confirm we are not trying to open too many connections
   os_assert(connection_number < VCHI_MAX_NUM_CONNECTIONS);
   success = os_semaphore_release_global();
   os_assert(success == 0);

#ifdef VCHI_COARSE_LOCKING
   // create the big semaphore
   success = os_semaphore_create(&ptr->sem, OS_SEMAPHORE_TYPE_SUSPEND);
   os_assert(success == 0);
#endif
   // record the api
   ptr->api = function_table;
   // record the state associated with the low level driver
   ptr->state = function_table->init(ptr, low_level);
   return(ptr);
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
int32_t vchi_initialise( VCHI_INSTANCE_T *instance_handle )
{
   int32_t success = -1;
//   uint32_t count = 0;
   VCHI_STATE_T * init_instance;

   // initialise the timestamping stuff for the messages
   vchi_initialise_time();
   // malloc the memory for the state information
   init_instance = (VCHI_STATE_T *)os_malloc(sizeof(VCHI_STATE_T), OS_ALIGN_DEFAULT, "" );

   //reset my state
   memset( init_instance, 0, sizeof( VCHI_STATE_T ) );

   //create a semaphore
   success = os_semaphore_create( &init_instance->semaphore, OS_SEMAPHORE_TYPE_BUSY_WAIT );
   os_assert( success == 0 );

   // the initialisation of connections will be done when generating connection handles
//   for( count = 0; count < num_connections; count++ )
//   {
//      if( NULL != connections[count] )
//      {
         //ports are allowed to create tasks etc.. if they want.
         //when init exits, all comms are setup
//         success = (*(&connections[count]))->init();
//         os_assert( success == 0 );
//      }
//      else
//      {
//         os_assert( 0 );
//      }
//   }

   *instance_handle = (VCHI_INSTANCE_T )init_instance;
   return success;
}


/***********************************************************
 * Name: vchi_connect
 *
 * Arguments: VCHI_CONNECTION_T **connections
 *            const uint32_t num_connections
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
   VCHI_STATE_T *state = (VCHI_STATE_T *)instance_handle;
   int32_t success = -1;

   success = vchi_bulk_aux_service_init( connections, num_connections, &state->bulk_aux_service_state );
   os_assert( success >= 0 );

// Initialise the control service on each of the connections
   // This will wait for the other side to respond before returning
   success = vchi_control_service_init( connections, num_connections, &state->control_service_state );
   os_assert( success >= 0 );


   return success;
}


/***********************************************************
 * Name: vchi_disconnect
 *
 * Arguments: VCHI_INSTANCE_T instance_handle
 *
 * Description:
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
int32_t vchi_disconnect( VCHI_INSTANCE_T instance_handle )
{
   int32_t success = -1;

   success = vchi_bulk_aux_service_close();
   os_assert( success >= 0 );

   success = vchi_control_service_close();
   os_assert( success >= 0 );

   return success;
}

/***********************************************************
 * Name: vchi_crc_control
 *
 * Arguments: VCHI_CONNECTION_T *connection
 *            VCHI_CRC_CONTROL_T control
 *
 * Description:
 *
 * Returns: 0 if successful, failure otherwise
 *
 ***********************************************************/
int32_t vchi_crc_control( VCHI_CONNECTION_T *connection,
                          VCHI_CRC_CONTROL_T control )
{
   int32_t success = -1;
   if ( connection )
   {
      vchi_lock(connection);
      success = connection->api->crc_control( connection->state, control );
      vchi_unlock(connection);
   }

   return success;
}


/***********************************************************
 * Name: vchi_service_create
 *
 * Arguments: VCHI_INSTANCE_T *instance_handle
 *            SERVICE_CREATION_T *setup
 *            VCHI_SERVICE_CREATE_HANDLE_T *handle
 *
 * Description: Routine to create a named service (this will be a server)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_service_create( VCHI_INSTANCE_T instance_handle,
                             SERVICE_CREATION_T *setup,
                             VCHI_SERVICE_HANDLE_T *handle )
{
   int32_t success = -1;
   VCHI_STATE_T * init_instance = (VCHI_STATE_T *)instance_handle;

   if( (NULL != setup->connection) && (NULL != handle) )
   {
      VCHI_SERVICE_T *service;

      // protect our service structure
      os_semaphore_obtain(&init_instance->semaphore);

      // find a free slot to record this information
      service = vchi_find_free_service( setup->service_id, setup->connection, init_instance );

      os_assert( service );

      if( NULL != service )
      {
         vchi_lock(setup->connection);
         //make the port create the service
         success = setup->connection->api->service_connect( setup->connection->state,    // pointer to connection state
                                                            setup->service_id,           // 4CC service code
                                                            setup->rx_fifo_size,         // RX fifo size (unused)
                                                            setup->tx_fifo_size,         // RX fifo size (unused)
                                                            VC_TRUE,                     // SERVER
                                                            setup->callback,             // callback routine
                                                            setup->callback_param,       // callback parameter
                                                            setup->want_crc,
                                                            setup->want_unaligned_bulk_rx,
                                                            setup->want_unaligned_bulk_tx,
                                                            &service->service_handle  );

         if( success == 0 )
         {
            //store the connection
            service->connection = setup->connection;
            //mark the service as in use
            service->state = SERVER;
            // record the service_id
            service->service_id = setup->service_id;
            //return this handle
            *handle = (VCHI_SERVICE_HANDLE_T)service;
            //success!
            success = 0;
         }
         else
         {
            os_assert( 0 );
         }
         vchi_unlock(setup->connection);
      }
      // we can free the lock around the structure
      os_semaphore_release(&init_instance->semaphore);
   }

   return success;
}


/***********************************************************
 * Name: vchi_service_destroy
 *
 * Arguments:  const VCHI_CONNECTION_SERVICE_INFO_T service_info
 *
 * Description: Routine to destroy a named service
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_service_destroy( const VCHI_SERVICE_HANDLE_T service )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service_details = (VCHI_SERVICE_T *)service;

   // make sure that the service exists and that it was created (i.e. it is a server)
   if(( service_details != NULL ) && (service_details->state == SERVER))
   {
      vchi_lock(service_details->connection);
      // Remove the service from this connection (may still exist on others)
      success = service_details->connection->api->service_disconnect( service_details->service_handle );
      os_assert( success >= 0 );
      vchi_unlock(service_details->connection);

      // mark the slot as free
      service_details->state = UNUSED;
      service_details->service_id = 0;
      service_details->connection = NULL;
   }

   return success;
}


/***********************************************************
 * Name: vchi_service_open
 *
 * Arguments: VCHI_INSTANCE_T *instance_handle
 *            SERVICE_CREATION_T *setup,
 *            VCHI_SERVICE_HANDLE_T *handle
 *
 * Description: Routine to open a service (as a client)
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t vchi_service_open( VCHI_INSTANCE_T instance_handle,
                           SERVICE_CREATION_T *setup,
                           VCHI_SERVICE_HANDLE_T *handle)
{
   int32_t success = -1;
   VCHI_STATE_T * init_instance = (VCHI_STATE_T *)instance_handle;

   if( (NULL != setup->connection) && (NULL != handle) )
   {
      // check that the server is available at the other end
//      success = vchi_control_server_available( setup->service_id, setup->connection );
      success = 0; // HACK HACK need to fix this sometime soon!!!!

      if( success >= 0 )
      {
         VCHI_SERVICE_T *service;

         // protect our service structure
         os_semaphore_obtain(&init_instance->semaphore);
         service = vchi_find_free_service( setup->service_id, setup->connection, init_instance );

         os_assert(service);

         if( NULL != service )
         {
            vchi_lock(setup->connection);
            success = setup->connection->api->service_connect( setup->connection->state,    // pointer to connection state
                                                               setup->service_id,           // 4CC service code
                                                               setup->rx_fifo_size,         // RX fifo size (unused)
                                                               setup->tx_fifo_size,         // RX fifo size (unused)
                                                               VC_FALSE,                    // CLIENT
                                                               setup->callback,             // callback routine
                                                               setup->callback_param,       // callback parameter
                                                               setup->want_crc,
                                                               setup->want_unaligned_bulk_rx,
                                                               setup->want_unaligned_bulk_tx,
                                                               &service->service_handle  );
            if( success >= 0 )
            {
               //store the details
               service->state = CLIENT;

               service->connection = setup->connection;

               //return the handle
               *handle = (VCHI_SERVICE_HANDLE_T)service;
            }
            vchi_unlock(setup->connection);
         }
         // we can free the lock around the structure
         os_semaphore_release(&init_instance->semaphore);
      }
   }

   return success;
}


/***********************************************************
 * Name: vchi_service_close
 *
 * Arguments: const VCHI_SERVICE_HANDLE_T handle
 *
 * Description: Routine to close a service
 *
 * Returns: int32_t - 0 == success
 *
 ***********************************************************/
int32_t vchi_service_close( const VCHI_SERVICE_HANDLE_T handle )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if( service != NULL && (service->state == CLIENT))
   {
      vchi_lock(service->connection);
      success = service->connection->api->service_disconnect( service->service_handle );
      os_assert( success >= 0 );

      // mark as no longer being open
      service->state = UNUSED;
      service->service_id = 0;
      service->connection = NULL;
      vchi_unlock(service->connection);
   }

   return success;
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
                        const void *data,
                        uint32_t data_size,
                        VCHI_FLAGS_T flags,
                        void *msg_handle )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   os_assert(ALLOW_RELOC_XFERS || !mem_is_relocatable(data)); // can't safely transfer relocatable memory; use handle instead

   if ( service != NULL )
   {
      vchi_lock(service->connection);
      os_assert( service->service_handle );
      success = service->connection->api->service_queue_msg( service->service_handle,
                                                             data,
                                                             data_size,
                                                             (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                             msg_handle );
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)))
#endif
         vchi_unlock(service->connection);
   }

   return success;
}

/***********************************************************
 * Name: vchi_msg_queuev
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             VCHI_MSG_VECTOR_EX_T *vector
 *             uint32_t count
 *             VCHI_FLAGS_T flags,
 *             void *msg_handle
 *
 * Description: Thin wrapper to queue an array of messages onto a connection
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
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if ( service != NULL )
   {
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
         os_assert(0);
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
            copy[i].vec_base = (uint8_t*)mem_lock((VCHI_MEM_HANDLE_T)v->u.handle.handle) + v->u.handle.offset;
            orig[i] = (uint8_t *)copy[i].vec_base;
            copy[i].vec_len = v->u.handle.vec_len;
            break;
         case VCHI_VEC_LIST:
            os_assert(0); // FIXME: implement this
            break;
         default:
            os_assert(0);
         }
      }
      vchi_lock(service->connection);
      os_assert( service->service_handle );
      success = service->connection->api->service_queue_msgv( service->service_handle,
                                                              copy,
                                                              count,
                                                              (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                              msg_handle );
      os_assert(success == 0);
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)))
#endif
         vchi_unlock(service->connection);

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
            mem_unlock((VCHI_MEM_HANDLE_T)v->u.handle.handle);
            if (flags & VCHI_FLAGS_ALLOW_PARTIAL)
            {
               const uint8_t *old = orig[i];
               uint32_t change = (const uint8_t*)copy[i].vec_base-old;
               v->u.handle.offset += change;
               v->u.handle.vec_len -= change;
            }
            break;
         default:
            os_assert(0);
         }
      }
   }

   return success;
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
                         VCHI_MSG_VECTOR_T *vector,
                         uint32_t count,
                         VCHI_FLAGS_T flags,
                         void *msg_handle )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if ( service != NULL )
   {
      vchi_lock(service->connection);
      os_assert( service->service_handle );
      success = service->connection->api->service_queue_msgv( service->service_handle,
                                                              vector,
                                                              count,
                                                              (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                              msg_handle );
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)))
#endif
         vchi_unlock(service->connection);
   }

   return success;
}


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
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if( service != NULL )
   {
#ifndef VCHI_RX_NANOLOCKS
      vchi_lock(service->connection);
#endif
      success = service->connection->api->service_dequeue_msg( service->service_handle,
                                                               data,
                                                               max_data_size_to_read,
                                                               actual_msg_size,
                                                               (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL));
#ifndef VCHI_RX_NANOLOCKS
     vchi_unlock(service->connection);
#endif
   }

   return success;
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
extern int32_t vchi_msg_peek( VCHI_SERVICE_HANDLE_T handle,
                              void **data,
                              uint32_t *msg_size,
                              VCHI_FLAGS_T flags )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if ( service ) {
#ifndef VCHI_RX_NANOLOCKS
#ifdef VCHI_AVOID_PEEK_LOCKS
      if ( !(flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) )
#endif
         vchi_lock(service->connection);
#endif
      success = service->connection->api->service_peek_msg( service->service_handle,
                                                            data,
                                                            msg_size,
                                                            (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL));
#ifndef VCHI_RX_NANOLOCKS
#ifdef VCHI_AVOID_PEEK_LOCKS
      if ( !(flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE) )
#endif
         vchi_unlock(service->connection);
#endif
   }

   return success;
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
extern int32_t vchi_msg_hold( VCHI_SERVICE_HANDLE_T handle,
                              void **data,
                              uint32_t *msg_size,
                              VCHI_FLAGS_T flags,
                              VCHI_HELD_MSG_T *message_handle )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if ( service && message_handle ) {
#ifndef VCHI_RX_NANOLOCKS
      vchi_lock(service->connection);
#endif

      message_handle->service = service;
      success = service->connection->api->service_hold_msg( service->service_handle,
                                                            data,
                                                            msg_size,
                                                            (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                            &message_handle->message );
      
#ifndef VCHI_RX_NANOLOCKS
      vchi_unlock(service->connection);
#endif
   }

   return success;
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
extern int32_t vchi_msg_remove( VCHI_SERVICE_HANDLE_T handle )
{
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;
   int32_t success = -1;
   void *message;

   if ( service ) {
#ifndef VCHI_RX_NANOLOCKS
      vchi_lock(service->connection);
#endif

      success = service->connection->api->service_hold_msg( service->service_handle,
                                                            NULL,
                                                            0,
                                                            VCHI_FLAGS_NONE,
                                                            &message );

      if (success == 0)
         success = service->connection->api->held_msg_release( service->service_handle,
                                                               message );
      
#ifndef VCHI_RX_NANOLOCKS
      vchi_unlock(service->connection);
#endif
   }

   return success;
}


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
extern int32_t vchi_held_msg_release( VCHI_HELD_MSG_T *message )
{
   int32_t success = -1;

   if ( message && message->service && message->message ) {
#ifndef VCHI_RX_NANOLOCKS
      vchi_lock(message->service->connection);
#endif
      success = message->service->connection->api->held_msg_release( message->service->service_handle,
                                                                     message->message );
#ifndef VCHI_RX_NANOLOCKS
      vchi_unlock(message->service->connection);
#endif
   }

   if ( success == 0 )
      message->message = NULL;

   return success;
}

/***********************************************************
 * Name: vchi_held_msg_ptr
 *
 * Arguments:  const VCHI_HELD_MSG_T *message
 *
 * Description: Routine to get the address of a held message
 *
 * Returns: Address of the held message (NULL on failure)
 *
 ***********************************************************/
extern void *vchi_held_msg_ptr( const VCHI_HELD_MSG_T *message )
{
   void *ptr = NULL;
   if ( message && message->service && message->message )
      message->service->connection->api->held_msg_info( message->service->service_handle,
                                                        message->message,
                                                        &ptr,
                                                        NULL,
                                                        NULL,
                                                        NULL );

   return ptr;
}

/***********************************************************
 * Name: vchi_held_msg_size
 *
 * Arguments:  const VCHI_HELD_MSG_T *message
 *
 * Description: Routine to get the size of a held message
 *
 * Returns: Size of the held message (-1 on failure)
 *
 ***********************************************************/
extern int32_t vchi_held_msg_size( const VCHI_HELD_MSG_T *message )
{
   int32_t size = -1;
   if ( message && message->service && message->message )
      message->service->connection->api->held_msg_info( message->service->service_handle,
                                                        message->message,
                                                        NULL,
                                                        &size,
                                                        NULL,
                                                        NULL );

   return size;
}

/***********************************************************
 * Name: vchi_held_msg_tx_timestamp
 *
 * Arguments:  const VCHI_HELD_MSG_T *message
 *
 * Description: Routine to get the transmit timestamp of a
 *              held message
 *
 * Returns: Timestamp of the held message (0 on failure)
 *
 ***********************************************************/
extern uint32_t vchi_held_msg_tx_timestamp( const VCHI_HELD_MSG_T *message )
{
   uint32_t t = 0;
   if ( message && message->service && message->message )
      message->service->connection->api->held_msg_info( message->service->service_handle,
                                                        message->message,
                                                        NULL,
                                                        NULL,
                                                        &t,
                                                        NULL );

   return t;
}

/***********************************************************
 * Name: vchi_held_msg_rx_timestamp
 *
 * Arguments:  const VCHI_HELD_MSG_T *message
 *
 * Description: Routine to get the receive timestamp of a
 *              held message
 *
 * Returns: Timestamp of the held message (0 on failure)
 *
 ***********************************************************/
extern uint32_t vchi_held_msg_rx_timestamp( const VCHI_HELD_MSG_T *message )
{
   uint32_t t = 0;
   if ( message && message->service && message->message )
      message->service->connection->api->held_msg_info( message->service->service_handle,
                                                        message->message,
                                                        NULL,
                                                        NULL,
                                                        NULL,
                                                        &t );

   return t;
}

/***********************************************************
 * Name: vchi_msg_iter_has_next
 *
 * Arguments:  VCHI_MSG_ITER_T *iter,
 *
 * Description: Routine to return a pointer to the next message
 *              The message can be removed using
 *              vchi_msg_iter_remove when you're finished
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern bool_t vchi_msg_iter_has_next( const VCHI_MSG_ITER_T *iter )
{
   bool_t result = VC_FALSE;
   if ( iter && iter->service )
   {
      VCHI_SERVICE_T *service = iter->service;

      result = service->connection->api->msg_iter_has_next( service->service_handle,
                                                            iter );
   }

   return result;
}

/***********************************************************
 * Name: vchi_msg_iter_next
 *
 * Arguments:  VCHI_MSG_ITER_T *iter,
 *             void **data,
 *             uint32_t *msg_size,
 *
 * Description: Routine to return a pointer to the next message
 *              The message can be removed using
 *              vchi_msg_iter_remove when you're finished
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern int32_t vchi_msg_iter_next( VCHI_MSG_ITER_T *iter,
                                   void **data,
                                   uint32_t *msg_size )
{
   int32_t success = -1;
   if ( iter && iter->service )
   {
      success = iter->service->connection->api->msg_iter_next( iter->service->service_handle,
                                                               iter,
                                                               data,
                                                               msg_size );
   }

   return success;
}

/***********************************************************
 * Name: vchi_msg_iter_remove
 *
 * Arguments:  VCHI_MSG_ITER_T *iter
 *
 * Description: Routine to remove the last message returned
 *              by the iterator
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern int32_t vchi_msg_iter_remove( VCHI_MSG_ITER_T *iter )
{
   int32_t success = -1;
   if ( iter && iter->service )
   {
      vchi_lock(iter->service->connection);
      success = iter->service->connection->api->msg_iter_remove( iter->service->service_handle,
                                                                 iter );
      vchi_unlock(iter->service->connection);
   }

   return success;
}

/***********************************************************
 * Name: vchi_msg_iter_hold
 *
 * Arguments:  VCHI_MSG_ITER_T *iter,
 *             VCHI_HELD_MSG_T *message_handle
 *
 * Description: Routine to hold the last message returned
 *              by the iterator
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern int32_t vchi_msg_iter_hold( VCHI_MSG_ITER_T *iter,
                                   VCHI_HELD_MSG_T *message_handle )
{
   int32_t success = -1;
   if ( iter && iter->service && message_handle )
   {
      vchi_lock(iter->service->connection);
      message_handle->service = iter->service;
      success = iter->service->connection->api->msg_iter_hold( iter->service->service_handle,
                                                               iter,
                                                               &message_handle->message );
      vchi_unlock(iter->service->connection);
   }

   return success;
}

/***********************************************************
 * Name: vchi_msg_iter_hold_next
 *
 * Arguments:  VCHI_MSG_ITER_T *iter,
 *             void **data,
 *             uint32_t *msg_size,
 *             VCHI_HELD_MSG_T *message_handle
 *
 * Description: Routine to get the next message and hold it
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern int32_t vchi_msg_iter_hold_next( VCHI_MSG_ITER_T *iter,
                                        void **data,
                                        uint32_t *msg_size,
                                        VCHI_HELD_MSG_T *message_handle )
{
   int32_t success = -1;
   if ( iter && iter->service && message_handle )
   {
#ifndef VCHI_RX_NANOLOCKS
      vchi_lock(iter->service->connection);
#endif
      success = iter->service->connection->api->msg_iter_next( iter->service->service_handle,
                                                               iter,
                                                               NULL,
                                                               NULL );
      if ( success == 0 )
      {
         message_handle->service = iter->service;
         success = iter->service->connection->api->msg_iter_hold( iter->service->service_handle,
                                                                  iter,
                                                                  &message_handle->message );
         os_assert( success == 0 ); // really shouldn't fail - if it does, we've done half an operation - bad API
      }
#ifndef VCHI_RX_NANOLOCKS
      vchi_unlock(iter->service->connection);
#endif
  }

   return success;
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
                                 void *data_dst,
                                 uint32_t data_size,
                                 VCHI_FLAGS_T flags,
                                 void *bulk_handle )
{
   int32_t success = -1;

   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   os_assert(data_dst);

   os_assert(ALLOW_RELOC_XFERS || !mem_is_relocatable(data_dst)); // Can't safely do transfers of relocatable memory

   if( service != NULL )
   {
      vchi_lock(service->connection);
      success = service->connection->api->bulk_queue_receive(service->service_handle,
                                                             data_dst,
                                                             data_size,
                                                             (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                             bulk_handle);
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)))
#endif
         vchi_unlock(service->connection);
   }

   return success;
}

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
                                       const uint32_t data_size,
                                       const VCHI_FLAGS_T flags,
                                       void * const bulk_handle )
{
   int32_t success = -1;

   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if( service != NULL )
   {
      // FIXME - this will tend to deadlock if the sending thread is
      // trying to call mem_alloc() !

      void *data_dst = (uint8_t*)mem_lock((VCHI_MEM_HANDLE_T)h) + offset;
      vchi_lock(service->connection);
      success = service->connection->api->bulk_queue_receive(service->service_handle,
                                                             data_dst,
                                                             data_size,
                                                             (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                             bulk_handle);
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE)))
#endif
         vchi_unlock(service->connection);
      mem_unlock((VCHI_MEM_HANDLE_T)h);
   }

   return success;
}

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
                                  const void *data_src,
                                  uint32_t data_size,
                                  VCHI_FLAGS_T flags,
                                  void *bulk_handle )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   os_assert(data_src);
   os_assert(ALLOW_RELOC_XFERS || !mem_is_relocatable(data_src)); // Can't safely do transfers of relocatable memory

   if( service != NULL )
   {
      vchi_lock(service->connection);
      success = service->connection->api->bulk_queue_transmit(service->service_handle,
                                                              data_src,
                                                              data_size,
                                                              (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                              bulk_handle);
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & (VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE))))
#endif
         vchi_unlock(service->connection);
   }

   return success;
}

int32_t vchi_bulk_queue_transmit_reloc( VCHI_SERVICE_HANDLE_T handle,
                                               VCHI_MEM_HANDLE_T h_src,
                                               uint32_t offset,
                                               uint32_t data_size,
                                               VCHI_FLAGS_T flags,
                                               void *transfer_handle )
{
   // FIXME - this will tend to deadlock if the receiving thread is
   // trying to call mem_alloc() !
   uint8_t *data_src = (uint8_t*)mem_lock((VCHI_MEM_HANDLE_T)h_src) + offset;
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if( service != NULL )
   {
      vchi_lock(service->connection);
      success = service->connection->api->bulk_queue_transmit(service->service_handle,
                                                              data_src,
                                                              data_size,
                                                              (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL),
                                                              transfer_handle);
#ifdef VCHI_ELIDE_BLOCK_EXIT_LOCK
      if (!(success == 0 && (flags & (VCHI_FLAGS_BLOCK_UNTIL_DATA_READ|VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE))))
#endif
         vchi_unlock(service->connection);
   }

   mem_unlock((VCHI_MEM_HANDLE_T)h_src);

   return success;
}

/***********************************************************
 * Name: vchi_msg_look_ahead
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle,
 *             VCHI_MSG_ITER_T *iter
 *             VCHI_FLAGS_T flags,
 *
 * Description: Routine to initialise a received message
 *              iterator.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
extern int32_t vchi_msg_look_ahead( VCHI_SERVICE_HANDLE_T handle,
                                    VCHI_MSG_ITER_T *iter,
                                    VCHI_FLAGS_T flags )
{
   int32_t success = -1;
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   if ( service != NULL && iter != NULL )
   {
      vchi_lock(service->connection);
      iter->service = service;
      // remaining fields of VCHI_MSG_ITER_T are there for the connection to use as they see fit
      success = service->connection->api->service_look_ahead_msg(service->service_handle,
                                                                 iter,
                                                                 (VCHI_FLAGS_T)(flags &~ VCHI_FLAGS_INTERNAL));
      vchi_unlock(service->connection);
   }

   return success;
}


/***********************************************************
 * Name: vchi_find_free_service
 *
 * Arguments:  const VCHI_STATE_T *state
 *
 * Description: Returns the address of a free entry, checks that the 4cc is not already in use
 *              Must be called with the semaphore locked
 *
 * Returns: pointer to free structure or NULL if none available
 *
 ***********************************************************/
static VCHI_SERVICE_T *vchi_find_free_service( fourcc_t service_id, VCHI_CONNECTION_T * connection, const VCHI_STATE_T *state )
{
   int i;
   const VCHI_SERVICE_T *address = NULL;

   for(i=0;i<VCHI_MAX_NUM_SERVICES;i++)
   {
      // record the first free slot address
      if(address == NULL)
      {
         if(state->services[i].state == UNUSED)
            address = &state->services[i];
      }
      // now check if we have a match on the service_id and the connection
      if((state->services[i].service_id == service_id) &&
         (connection == state->services[i].connection))
      {
         // we have a match on 4cc and connection then this service has been opened previously
         // this is bad
         os_assert(0);
         address = NULL;
         break;
      }
   }

   os_assert( address );
   // return the address, which will be NULL if there has been any problem
   return (VCHI_SERVICE_T *) address;
}


/***********************************************************
 * Name: vchi_allocate_buffer
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle
 *             uint32_t *length
 *
 * Description: Allocates memory that is address and length aligned suitable for use
 *              with the connection associated with the service
 *
 * Returns: pointer to memory (or NULL)
 *
 ***********************************************************/
extern void * vchi_allocate_buffer( VCHI_SERVICE_HANDLE_T handle, uint32_t *length )
{
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   return service->connection->api->buffer_allocate(service->service_handle, length);
}


/***********************************************************
 * Name: vchi_free_buffer
 *
 * Arguments:  VCHI_SERVICE_HANDLE_T handle
 *             void *address
 *
 * Description: Frees memory allocated by vchi_allocate_buffer
 *
 * Returns: pointer to memory (or NULL)
 *
 ***********************************************************/
extern void vchi_free_buffer(VCHI_SERVICE_HANDLE_T handle, void *address)
{
   VCHI_SERVICE_T *service = (VCHI_SERVICE_T *)handle;

   service->connection->api->buffer_free(service->service_handle, address);
}

/***********************************************************
 * Name: vchi_current_time
 *
 * Arguments:  VCHI_INSTANCE_T instance_handle
 *
 * Description: Returns the current time, adjusted to
 *              sync with the peer(s).
 *
 * Returns: time in milliseconds
 *
 ***********************************************************/
extern uint32_t vchi_current_time(VCHI_INSTANCE_T instance_handle)
{
   return vchi_control_get_time();
}

/****************************** End of file ***********************************/
