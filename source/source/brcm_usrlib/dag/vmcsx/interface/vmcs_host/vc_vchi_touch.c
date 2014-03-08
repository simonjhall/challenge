/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Touch screen and buttons client (host-side)
File     :  $Id: $
Revision :  $Revision:  $

FILE DESCRIPTION
Client to service for controlling the touch screen and videocore-hosted buttons
=============================================================================*/






/*****************************************************************************
 *                                                                           *
 *    Headers                                                                *
 *                                                                           * 
 *****************************************************************************/


#include <string.h>  /*< for memset() */


#include "vchost_config.h"
#include "vchost.h"
#include "vc_vchi_touch.h"

#include "interface/peer/vc_touchsrv_defs.h"
#include "interface/vchi/vchi.h"
#include "interface/vchi/message_drivers/message.h"
#include "interface/vchi/os/os.h"

#include "applications/vmcs/touchserv/touchserv.h"




/*****************************************************************************
 *                                                                           *
 *    Debugging                                                              *
 *                                                                           * 
 *****************************************************************************/


#define DO(_X) _X
#define IGNORE(_X)


#if 1
#define DPRINTF printf
#define DPRINTF_INIT()
#else
#define DPRINTF(arglist...) logging_message(LOGGING_GENERAL, arglist)
#define DPRINTF_INIT() \
   {  logging_init();  \
      logging_level(LOGGING_GENERAL); \
   }
#endif

#define CODEID "touchclient"




/*****************************************************************************
 *                                                                           *
 *    Configuration                                                          *
 *                                                                           * 
 *****************************************************************************/



#define TOUCH_EVENT_BUTTONS_MAX    3
#define TOUCH_EVENT_MSGQ_SIZE_LN2  3  // max of 8 messages in the queue




/*****************************************************************************
 *                                                                           *
 *    Touch Message Event Queue                                              *
 *                                                                           * 
 *****************************************************************************/




#define TMSGQ_SIZE_LN2  TOUCH_EVENT_MSGQ_SIZE_LN2
typedef VC_TOUCH_EVENT_MSG_T TMSG_T;


typedef struct {
   size_t size;
   TMSG_T msg;
} TMSGINFO_T;

typedef struct {
   int size;
   int read;
   int write;
   VCOS_EVENT_T lock;

   TMSGINFO_T storage[1 << TMSGQ_SIZE_LN2];
} TMSGQ_T;




static void
tmsgq_init(TMSGQ_T *queue)
{
   queue->size = 1<<TMSGQ_SIZE_LN2;
   queue->read = 0;
   queue->write = 0;

   vcos_event_create(&queue->lock, "TOUCH EVENT Q LOCK");
   vcos_event_signal(&queue->lock); /* starts with lock available */
}



static int /*bool*/
tmsgq_push(TMSGQ_T *queue, const void *msg, size_t msgsize)
{
   if (msgsize > sizeof(TMSG_T))
      return 0 /*FALSE*/;
   else
   {  int /*bool*/ ok;
      
      vcos_event_wait(&queue->lock);

      if (queue->write == queue->read + queue->size)
         ok = 0; /*FALSE*/
      else
      {  TMSGINFO_T *msginfo = &queue->storage[queue->write & (queue->size-1)];
         msginfo->size = msgsize;
         memcpy(&msginfo->msg, msg, msgsize);
         queue->write++;
         ok = 1; /*TRUE*/
      }
      vcos_event_signal(&queue->lock);

      return ok;
   }
}



static int /*bool*/
tmsgq_pop(TMSGQ_T *queue, void *msg, size_t maxsize, size_t *out_msgsize)
{
   int /*bool*/ok;

   vcos_event_wait(&queue->lock);
   if (queue->write == queue->read)
      ok = 0 /*FALSE*/;
   else
   {  TMSGINFO_T *msginfo = &queue->storage[queue->read & (queue->size - 1)];
      queue->read++;
      *out_msgsize = msginfo->size;
      ok = msginfo->size <= maxsize;
      if (ok)
         memcpy(msg, &msginfo->msg, msginfo->size);
   }
   vcos_event_signal(&queue->lock);
   return ok;
}





/*****************************************************************************
 *                                                                           *
 *    Touch Client Lifetime Control                                          *
 *                                                                           * 
 *****************************************************************************/




#define TOUCH_SERVICE_MAGIC (0x70c4c0de) /* check for pointer correctness */

/* Touch service state */
typedef struct {
   VCHI_SERVICE_HANDLE_T     client_handle[VCHI_MAX_NUM_CONNECTIONS];
   uint32_t                  response_length;
   void *                    message_handle;
   int                       num_connections;
   OS_SEMAPHORE_T            message_atomicity;
   vc_touch_event_fn_t      *touch_event_callback_fn;
   void                     *touch_event_callback_arg;
   vc_button_event_fn_t     *button_event_callback_fn;
   void                     *button_event_callback_arg;
   vc_touch_properties_t     prop;           /*< cached properties */
   TMSGQ_T                   pending_events; /*< unhandled events */
   VC_TOUCH_EVENT_MSG_T      event_msg;      /*< for "asynchronous" event rx */
   uint32_t                  magic;          /*< sentinal-place at struct end */
} LOCAL_TOUCH_CLIENT_T;


#define _CTIME_ASSERT_WNAME(s,l) int _cassert_failed_line_##l[(s)?1:-1];
#define _CTIME_ASSERT(s,l) _CTIME_ASSERT_WNAME(s,l)
#define CASSERT(s) _CTIME_ASSERT(s, __LINE__)

CASSERT(sizeof(LOCAL_TOUCH_CLIENT_T) <= sizeof(TOUCH_SERVICE_T))




static int32_t /*rc*/
local_touch_client_cast(TOUCH_SERVICE_T *serv,
                         LOCAL_TOUCH_CLIENT_T **out_localserv)
{  *out_localserv = (LOCAL_TOUCH_CLIENT_T *)serv;
   if (out_localserv != NULL && (*out_localserv)->magic == TOUCH_SERVICE_MAGIC)
      return 0;
   else
   {  os_assert((*out_localserv)->magic == TOUCH_SERVICE_MAGIC);
      /* (asserts don't prevent further execution in non-debug code) */
      *out_localserv = NULL;
      return TOUCH_RC_BAD_HANDLE;
   }
}
   




/*! Initialise touch service. Returns it's interface number. This
 *  initialises the host side of the interface, it does not send anything to
 *  VideoCore.
 */
extern int
vc_touch_init(void)
{  int success_rc = 0;
   return success_rc;
}





/*! Handle a message prior to it's being given to our API's main task
 *  (This routine is, nonetheless, called in a task context)
 */
static void
touch_msg_interceptor(void *callback_arg, const VCHI_CALLBACK_REASON_T reason,
                      void *msg_handle)
{
   LOCAL_TOUCH_CLIENT_T *touch_client = (LOCAL_TOUCH_CLIENT_T *)callback_arg;

   switch (reason)
   {
      case VCHI_CALLBACK_MSG_AVAILABLE:
      {  // assume that this has been called to service client_handle [0]
         const VCHI_SERVICE_HANDLE_T handle = touch_client->client_handle[0];
         void *message;
         uint32_t actual_size;
         int rc = vchi_msg_peek(handle, &message, &actual_size,
                                VCHI_FLAGS_NONE);
         if (0 == rc) {
            vc_touch_msg_hdr_t *rxmsg = (vc_touch_msg_hdr_t *)message;

            if (0 != (rxmsg->id & VC_TOUCH_EVENT))
            {  /* take a copy of the message and place it on our queue */
               /*int pushed = */(void)tmsgq_push(&touch_client->pending_events,
                                                 (TMSG_T *)rxmsg, actual_size);
               /* If we failed to push the message - e.g. because the queue is
                * full - we just throw it away here.
                * TODO: discard the oldest events instead
                */

               (void)vchi_msg_remove(handle);
               /* take it off the queue for the client API's task */
            }
         }
         break;
      }         
      default:
         /* ignore unknown reason codes */
         break;
   }
}




/*! Initialise the general command service for use. A negative return value
 *  indicates failure (which may mean it has not been started on VideoCore).
 */
extern int32_t /*rc*/
vc_vchi_touch_client_init(TOUCH_SERVICE_T *new_client_state,
                          VCHI_INSTANCE_T initialise_instance,
                          VCHI_CONNECTION_T **connections,
                          uint32_t num_connections)
{  int rc = 0;
   int i;
   LOCAL_TOUCH_CLIENT_T *touch_client =
      (LOCAL_TOUCH_CLIENT_T *)new_client_state;
   /* compile-time assert above guarantees that the sizes are OK */

   /* record the number of connections */
   memset(touch_client, 0, sizeof(LOCAL_TOUCH_CLIENT_T));

   touch_client->num_connections = num_connections;
   assert(num_connections != 0);
   
   rc = os_semaphore_create(&touch_client->message_atomicity,
                            OS_SEMAPHORE_TYPE_SUSPEND);
   assert(rc == 0);

   for (i=0; i<touch_client->num_connections; i++)
   {
      /* Create a 'Client' service on the each of the connections */
      SERVICE_CREATION_T touch_parameters = 
      {  TOUCH_NAME,               /*< 4cc service code         */
         connections[i],           /*< passed in fn ptrs        */
         0,                        /*< tx fifo size (unused)    */
         0,                        /*< tx fifo size (unused)    */
         &touch_msg_interceptor,   /*< service callback         */
         (void *)touch_client,     /*< callback parameter       */
      }; 

      rc = vchi_service_open(initialise_instance, &touch_parameters,
                             &touch_client->client_handle[i]);
      assert(rc == 0);
   }

   if (rc == 0)
   {
      tmsgq_init(&touch_client->pending_events);
      touch_client->magic = TOUCH_SERVICE_MAGIC;
      //rc = vc_touch_properties(new_client_state, &touch_client->prop);
      assert(rc == 0);
   }
   return rc;
}




/*! Stop the service from being used.
 */
extern void
vc_touch_end(void)
{
}



/*****************************************************************************
 *                                                                           *
 *    Touch Service Access                                                   *
 *                                                                           * 
 *****************************************************************************/




/* The functions below send and receive messages to the touchscreen service.
   All of them send a request and then suspend until a response is received.
   If an event message occurs before the response it is ignored and
   handled by a service callback.

   Each of the request/response functions provides a return code that indicates
   succesful operation of the service access protocol.  (Zero is success,
   non-zero indicates a problem.)
*/






static int32_t /*rc*/
touch_handle_event(LOCAL_TOUCH_CLIENT_T *localserv,
                   vc_touch_msg_hdr_t *msg, size_t rxlen)
{  int rc = 0;

   /* must have at least enough of a message to identify it! */
   if (rxlen < sizeof(vc_touch_msg_hdr_t))
   {  rc = -3; /* event messsage ridiculously small */
      assert(rc == 0);
      
   } else switch (msg->id) {

      case VC_TOUCH_ID_FINGER_EVENT:
      {
         VC_TOUCH_MSG_FINGER_EVENT_T *ev = (VC_TOUCH_MSG_FINGER_EVENT_T *)msg;
         if (rxlen < sizeof(ev))
         {  rc = -2; /* event message smaller than expected */
            assert(rc == 0);
         } else if (NULL != localserv->touch_event_callback_fn)
         {  (*localserv->touch_event_callback_fn)
                     (localserv->touch_event_callback_arg, ev->type, ev->arg);
         }
         break;
      }

      case VC_TOUCH_ID_BUTTON_EVENT:
      {
         VC_TOUCH_MSG_BUTTON_EVENT_T *ev = (VC_TOUCH_MSG_BUTTON_EVENT_T *)msg;
         if (rxlen < sizeof(ev))
         {  rc = -2; /* event message smaller than expected */
            assert(rc == 0);
         } else if (NULL != localserv->button_event_callback_fn)
         {  (*localserv->button_event_callback_fn)
                    (localserv->button_event_callback_arg, ev->type,
                     ev->button_id, ev->arg);
         }
         break;
      }

      default:
         rc = -1; /* unknown event */
         assert(rc == 0);
         break;
   }

   return rc;
}





extern int32_t /*rc*/
vc_touch_poll(TOUCH_SERVICE_T *touchserv)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int /*bool*/ some_dequeued = 0;
#if 1
      TMSG_T evmsg;
      size_t actual_evsize;
      
      // now we have released the lock we can process any events that
      // are pending
      while (rc == 0 &&
             tmsgq_pop(&localserv->pending_events, &evmsg, sizeof(evmsg),
                       &actual_evsize))
         if (0 == (rc = touch_handle_event(localserv, &evmsg.finger_ev.hdr,
                                           actual_evsize)))
            some_dequeued = 1/*TRUE*/;
      
#else
      VC_TOUCH_EVENT_MSG_T *event_msg = &localserv->event_msg;
      vc_touch_msg_hdr_t *rxmsg = &event_msg->finger_ev.hdr;
      size_t rxlen_max = sizeof(*event_msg);
      int connection;
   
      for (connection=0;
           rc == 0 && connection < localserv->num_connections;
           connection++)
      {
         uint32_t actual_size;
         
         rxmsg->rc = (uint16_t)-1;
         rc = vchi_msg_dequeue(localserv->client_handle[connection],
                               rxmsg, rxlen_max, &actual_size, VCHI_FLAGS_NONE);
         if (rc == 0)
         {  some_dequeued = 1;
            if (0 != (rxmsg->id & VC_TOUCH_EVENT))
               rc = touch_handle_event(localserv, rxmsg, actual_size);
            else
            {  /* we're not expecting any traffic on this connection
                * (that isn't picked up during an RPC-style transaction)
                * other than events */
               assert(0 != (rxmsg->id & VC_TOUCH_EVENT));
            }
         } else
            rc = 0; /* it's OK for no messages to be read on a connection */
      }
#endif
      if (rc == 0 && !some_dequeued)
         rc = -2; /* no work done on this poll */
   }
   
   return rc;
}





static int32_t
touch_receive(LOCAL_TOUCH_CLIENT_T *serv, int connection, uint16_t msgid,
              vc_touch_msg_hdr_t *rxmsg, size_t rxlen_max, size_t rxlen_expect,
              int16_t *out_rpc_rc)
{
   uint32_t actual_size;
   int32_t rc;

   /* wait for a reply but receive messages that are events and handle them
    * immediately */
   do {
      rxmsg->rc = (uint16_t)-1;
      rc = vchi_msg_dequeue(serv->client_handle[connection],
                            rxmsg, rxlen_max,
                            &actual_size, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE);
      os_assert(rc == 0);
      if (rc == 0)
      {  os_assert(0 == (rxmsg->id & VC_TOUCH_EVENT));
         // no EVENTs should get past the interception callback routine
         if (0 != (rxmsg->id & VC_TOUCH_EVENT))
         {  int /*bool*/ pushed = tmsgq_push(&serv->pending_events,
                                             (TMSG_T *)rxmsg, actual_size);
            // Because we have the serialization lock (message_atomicity) if we
            // call back here the called function can't use most operations in
            // this API.  We fix this by queuing these events and handling then
            // once we've sent the corresponding reply.
         }
      }
   } while (rc == 0 && 0 != (rxmsg->id & VC_TOUCH_EVENT));
   
   os_assert(actual_size == rxlen_expect);
   if (actual_size != rxlen_expect)
      rc = TOUCH_RC_RXLEN;
   os_assert(0 != (rxmsg->id & VC_TOUCH_REPLY));
   if (0 == (rxmsg->id & VC_TOUCH_REPLY))
      rc = TOUCH_RC_NOTRSP;
   os_assert(rxmsg->id == (msgid | VC_TOUCH_REPLY));
   if (rxmsg->id != (msgid | VC_TOUCH_REPLY))
      rc = TOUCH_RC_NOTMINE;
   *out_rpc_rc = rxmsg->rc;
   
   return rc;
}



static int32_t
touch_sendmsg(LOCAL_TOUCH_CLIENT_T *serv, int connection, uint16_t msgid,
              vc_touch_msg_hdr_t *txmsg, size_t txlen,
              vc_touch_msg_hdr_t *rxmsg, size_t rxlen_max, size_t rxlen_expect,
              int16_t *out_rpc_rc)
{  int rc = os_semaphore_obtain(&serv->message_atomicity);
   
   os_assert(rc == 0);
   if (rc == 0)
   {
      txmsg->id = msgid;
      txmsg->rc = 0;
      rc = vchi_msg_queue(serv->client_handle[connection], txmsg, txlen,
                          VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
      os_assert(rc == 0);
      if (rc == 0)
      {  rc = touch_receive(serv, connection, msgid,
                            rxmsg, rxlen_max, rxlen_expect, out_rpc_rc);
         os_assert(rc == 0);
      }
      
      {  int semrc = os_semaphore_release(&serv->message_atomicity);

         os_assert(semrc == 0);

      }
   }
   return rc;
}




/*! send a request and recieve the corresponding response
 *  Expect transmit and reception sizes for this kind of message
 *  Provide exclusive access within this client to the server for this RPC
 */
#define VC_TOUCH_RPC(_serv, _rc, _rpc_rc, _msgid, _txmsg, _rxmsg, _rxtype)     \
   {  _rc = touch_sendmsg(_serv, 0, _msgid,                                    \
                          &(_txmsg).req.hdr, sizeof((_txmsg).req),             \
                          &(_rxmsg)._rxtype.rsp.hdr,                           \
                          sizeof((_rxmsg)), sizeof((_rxmsg)._rxtype.rsp),      \
                          &_rpc_rc);                                           \
   }







/*****************************************************************************
 *                                                                           *
 *    Touchscreen Fingers                                                    *
 *                                                                           * 
 *****************************************************************************/








/*! Return the properties of the touch device
 */
extern int /*rc*/
vc_touch_properties(TOUCH_SERVICE_T *touchserv,
                    vc_touch_properties_t *out_properties)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_PROP_T msg;
      VC_TOUCH_MSG_T rxmsg;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_PROP_REQ, msg, rxmsg, prop);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  vc_touch_properties_t *prop = &localserv->prop;
            
            prop->screen_width = rxmsg.prop.rsp.screen_width;
            prop->screen_height = rxmsg.prop.rsp.screen_height;
            prop->rightward_x = rxmsg.prop.rsp.rightward_x;
            prop->rising_y = rxmsg.prop.rsp.rising_y;
            prop->finger_count = rxmsg.prop.rsp.finger_count;
            prop->max_pressure = rxmsg.prop.rsp.max_pressure;
            prop->gesture_box_width = rxmsg.prop.rsp.gesture_box_width;
            prop->gesture_box_height = rxmsg.prop.rsp.gesture_box_height;
            
            memcpy(out_properties, prop, sizeof(*prop));

            /* we keep our own local copy of the scale */
            out_properties->scaled_width = rxmsg.prop.rsp.scaled_width;
            out_properties->scaled_height = rxmsg.prop.rsp.scaled_height;
            
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}




/* currently only sets the local scale for this client */
extern int
vc_touch_set_scale(TOUCH_SERVICE_T *touchserv, 
                   int scaled_width, int scaled_height)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      localserv->prop.scaled_width = scaled_width;
      localserv->prop.scaled_height = scaled_height;
   }
   return rc;
}




/*! Return the locations of the fingers currently in contact with the  
 *  touchscreen.
 *  Up to \c max_fingers finger positions will be returned - further finger
 *  positions will be ignored.  Finger records that are returned with pressure
 *  zero are unused.  Present fingers (pressure != 0) will always be recorded
 *  earlier in the array of values than absent ones.
 */
int /*rc*/
vc_touch_finger_pos(TOUCH_SERVICE_T *touchserv, 
                    vc_touch_point_t *out_posns, int max_fingers)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_FINGERPOS_T msg;
      VC_TOUCH_MSG_T rxmsg;

      memset(out_posns, 0, max_fingers*sizeof(out_posns[0]));

      if (max_fingers > VC_TOUCHSERV_FINGERS_MAX)
         max_fingers = VC_TOUCHSERV_FINGERS_MAX;

      msg.req.fingers = max_fingers;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_FINGERPOS_REQ, msg, rxmsg, fingerpos);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  int i;

            for (i=0; i<max_fingers; i++)
            {  VC_TOUCH_POSN_T *rxposn = &rxmsg.fingerpos.rsp.posn[i];
               out_posns[i].x = rxposn->x;
               out_posns[i].y = rxposn->y;
               out_posns[i].pressure = rxposn->pressure;
            }
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}   




static uint32_t
vc_touch_scale_x(vc_touch_properties_t *prop, unsigned raw_x)
{  uint32_t max_x = prop->screen_width;
   if (prop->rightward_x)
      return (prop->scaled_width*raw_x)/max_x;
   else
      return (prop->scaled_width*(max_x-raw_x))/max_x;
}



static uint32_t
vc_touch_scale_y(vc_touch_properties_t *prop, unsigned raw_y)
{  uint32_t max_y = prop->screen_height;
   if (prop->rising_y)
      return (prop->scaled_height*raw_y)/max_y;
   else
      return (prop->scaled_height*(max_y-raw_y))/max_y;
}





extern int /*rc*/
vc_touch_finger_scaled_pos(TOUCH_SERVICE_T *touchserv, 
                           vc_touch_point_t *out_posns, int max_fingers)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      vc_touch_properties_t *prop = &localserv->prop;
      rc = vc_touch_finger_pos(touchserv, out_posns, max_fingers);

      if (prop->scaled_width != 0 && prop->scaled_height != 0)
      {  int i;
         
         if (max_fingers > VC_TOUCHSERV_FINGERS_MAX)
            max_fingers = VC_TOUCHSERV_FINGERS_MAX;

         for (i=0; i<max_fingers; i++)
         {  out_posns[i].x = vc_touch_scale_x(prop, out_posns[i].x);
            out_posns[i].y = vc_touch_scale_y(prop, out_posns[i].y);
         }
      }
   }
   return rc;
}





/*! Begin generating events from the touchscreen at a rate no greater than
 *  once every \c period_ms.  The events result in the callback of the \c event
 *  function which is supplied with \c open_arg.
 *  Note that the context in which \c event is called may not be suitable for
 *  sustained processing operations.
 *  If period_ms == 0 stop generating events from the touchscreen.
 */
extern int /*rc*/ 
vc_touch_finger_events_open(TOUCH_SERVICE_T *touchserv, 
                            int period_ms, int max_fingers,
                            vc_touch_event_fn_t *event, void *open_arg)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      if ((period_ms <= 0) == (event == NULL))
      {
         int16_t rpc_rc;
         VC_TOUCH_MSG_FINGEROPEN_T msg;
         VC_TOUCH_MSG_T rxmsg;

         localserv->touch_event_callback_fn = event;
         localserv->touch_event_callback_arg = open_arg;

         msg.req.period_ms = period_ms;
         msg.req.max_fingers = max_fingers;

         VC_TOUCH_RPC(localserv, rc, rpc_rc,
                      VC_TOUCH_ID_FINGEROPEN_REQ, msg, rxmsg, fingeropen);

         if (rc == 0)
         {  if (rpc_rc != 0)
               rc = TOUCH_RC_BADRC;
         }
         if (rc != 0)
         {  localserv->touch_event_callback_fn = NULL;
            localserv->touch_event_callback_arg = NULL;
         }
      } else
      {
         /* if the event is NULL the period must be zero, and if non-null
            it should be non-zero
         */
         os_assert(((period_ms <= 0) == (event == NULL)));
         rc = TOUCH_RC_OPEN_INVAL;
      }
   }
   return rc;
}   




 
/*****************************************************************************
 *                                                                           *
 *    Buttons                                                                *
 *                                                                           * 
 *****************************************************************************/








/*! Make an existing hardware button available for use.
 *  The \c button_id integer identifies the hardware button to use.
 */
extern int /*rc*/
vc_button_new(TOUCH_SERVICE_T *touchserv, 
              vc_button_handle_t *button_handle,
              vc_button_id_t button_id,
              vc_key_name_t key_name)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_BUTTNEW_KEY_T msg;
      VC_TOUCH_MSG_T rxmsg;

      msg.req.button_id = button_id;
      msg.req.key_name = key_name;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_BUTTNEW_KEY_REQ, msg, rxmsg, buttnew_key);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  if (NULL != button_handle)
               *button_handle = rxmsg.buttnew_key.rsp.button_handle;
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}





/*! Set the repeat rate for all key buttons (not touch or IR buttons)
 */
int /*rc*/
vc_buttons_repeat_rate(TOUCH_SERVICE_T *touchserv, uint32_t repeat_rate_in_ms,
                       uint32_t *out_old_repeat_rate_ms)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_KEYRATE_T msg;
      VC_TOUCH_MSG_T rxmsg;

      msg.req.period_ms = repeat_rate_in_ms;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_KEYRATE_REQ, msg, rxmsg, keyrate);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  if (NULL != out_old_repeat_rate_ms)
               *out_old_repeat_rate_ms = rxmsg.keyrate.rsp.period_ms;
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}





/*! Create a new soft button operated via Infra Red and name it
 *  with the \c button_id integer.
 *  This name will be returned by the server in order to identify the new
 *  button.
 */
int /*rc*/
vc_ir_button_new(TOUCH_SERVICE_T *touchserv, 
                 vc_button_handle_t *button_handle,
                 vc_button_id_t button_id /* other IR arguments? */)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_BUTTNEW_IR_T msg;
      VC_TOUCH_MSG_T rxmsg;

      msg.req.button_id = button_id;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_BUTTNEW_IR_REQ, msg, rxmsg, buttnew_ir);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  if (NULL != button_handle)
               *button_handle = rxmsg.buttnew_ir.rsp.button_handle;
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}





/*! Create a new soft button defined by the given coordinates and name it
 *  with the \c button_id integer.
 *  This name will be returned by the server in order to identify the new
 *  button.
 */
int /*rc*/
vc_touch_button_new(TOUCH_SERVICE_T *touchserv, 
                    vc_button_handle_t *button_handle,
                    vc_button_id_t button_id,
                    vc_touch_button_type_t kind,
                    int left_x, int bottom_y, 
                    int width, int height)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_BUTTNEW_TOUCH_T msg;
      VC_TOUCH_MSG_T rxmsg;

      msg.req.button_id = button_id;
      msg.req.kind      = kind;
      msg.req.bottom_y  = bottom_y;
      msg.req.left_x    = left_x;
      msg.req.width     = width;
      msg.req.height    = height;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_BUTTNEW_TOUCH_REQ, msg, rxmsg, buttnew_touch);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  if (NULL != button_handle)
               *button_handle = rxmsg.buttnew_touch.rsp.button_handle;
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}





/*! Remove a button from further use
 */
int /*rc*/
vc_button_delete(TOUCH_SERVICE_T *touchserv, 
                 vc_button_handle_t button_handle)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_BUTTDEL_T msg;
      VC_TOUCH_MSG_T rxmsg;

      msg.req.button_handle = button_handle;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_BUTTDEL_REQ, msg, rxmsg, buttdel);

      if (rc == 0)
      {  if (rpc_rc != 0)
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}





/*! Retrieve the state of all the buttons so far defined
 */
extern int /*rc*/
vc_buttons_get_state(TOUCH_SERVICE_T *touchserv, 
                     vc_button_state_t *out_state)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      int16_t rpc_rc;
      VC_TOUCH_MSG_BUTTSTATE_T msg;
      VC_TOUCH_MSG_T rxmsg;

      VC_TOUCH_RPC(localserv, rc, rpc_rc,
                   VC_TOUCH_ID_BUTTSTATE_REQ, msg, rxmsg, buttstate);

      if (rc == 0)
      {  if (rpc_rc == 0)
         {  out_state->active = rxmsg.buttstate.rsp.active_button_set;
            out_state->depressed = rxmsg.buttstate.rsp.depressed_button_set;
         } else
            rc = TOUCH_RC_BADRC;
      }
   }
   return rc;
}



 
/*! Begin generating events from the buttons at a rate no greater than
 *  once every \c period_ms.  The events result in the callback of the \c event
 *  function which is supplied with \c open_arg.
 *  A new event will be generated whenever button state changes.
 *  Note that the context in which \c event is called may not be suitable for
 *  sustained processing operations.
 *  If period_ms == 0 then stop generating callback events.
 */
extern int /*rc*/
vc_buttons_events_open(TOUCH_SERVICE_T *touchserv, int period_ms,
                       vc_button_event_fn_t *event, void *open_arg)
{  LOCAL_TOUCH_CLIENT_T *localserv;
   int32_t rc = local_touch_client_cast(touchserv, &localserv);
   
   if (rc == TOUCH_RC_NONE)
   {
      if ((period_ms <= 0) == (event == NULL))
      {
         int16_t rpc_rc;
         VC_TOUCH_MSG_BUTTOPEN_T msg;
         VC_TOUCH_MSG_T rxmsg;

         localserv->button_event_callback_fn = event;
         localserv->button_event_callback_arg = open_arg;

         msg.req.period_ms = period_ms;

         VC_TOUCH_RPC(localserv, rc, rpc_rc,
                      VC_TOUCH_ID_BUTTOPEN_REQ, msg, rxmsg, buttopen);

         if (rc == 0)
         {  if (rpc_rc != 0)
               rc = TOUCH_RC_BADRC;
         }
         if (rc != 0)
         {  localserv->button_event_callback_fn = NULL;
            localserv->button_event_callback_arg = NULL;
         }
      } else
      {
         /* if the event is NULL the period must be zero, and if non-null
            it should be non-zero
         */
         os_assert(((period_ms <= 0) == (event == NULL)));
         rc = TOUCH_RC_OPEN_INVAL;
      }
   }
   return rc;
}



