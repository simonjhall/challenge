/*=========================================================
 Copyright © 2009 Broadcom Europe Limited. All rights reserved.

 Project  :  VMCS Host Apps
 Module   :  Platform framework test application
 File     :  $Id: $

 FILE DESCRIPTION:
 Message queue tests.
 =========================================================*/


#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_msgqueue.h"
#include "vcos_test.h"

typedef struct my_msg_t {
   int i;
} __VCOS_MAY_ALIAS my_msg_t;

#define MSG_N_MY_MSG (VCOS_MSG_N_PRIVATE + 0)

static void server_open(VCOS_MSGQUEUE_T *server)
{
   VCOS_MSG_T msg;
   // no data to send in these messages at present...
   vcos_msg_sendwait(server, VCOS_MSG_N_OPEN, &msg);
}

static void server_close(VCOS_MSGQUEUE_T *server)
{
   VCOS_MSG_T msg;
   // no data to send in these messages at present...
   vcos_msg_sendwait(server, VCOS_MSG_N_CLOSE, &msg);
}

static void *simple_server(void *cxt)
{
   int count = 0;
   int alive = 1;
   int refcount = 0;
   VCOS_MSG_ENDPOINT_T self;

   VCOS_STATUS_T st = vcos_msgq_endpoint_create(&self, "server");
   vcos_assert(st == VCOS_SUCCESS);

   while (alive)
   {
      VCOS_MSG_T *msg = vcos_msg_wait();
      switch (msg->code)
      {
      case VCOS_MSG_N_OPEN:
         refcount++;
         vcos_msg_reply(msg);
         break;
      case VCOS_MSG_N_CLOSE:
         refcount--;
         vcos_msg_reply(msg);
         break;
      case MSG_N_MY_MSG:
         {
            struct my_msg_t *body = VCOS_MSG_DATA(msg);
            body->i++;
            vcos_msg_reply(msg);
            count++;
         }
         break;
      case VCOS_MSG_N_QUIT:
         vcos_log("server: got quit request");
         vcos_assert(refcount == 0);
         vcos_msg_reply(msg);
         alive = 0;
         break;
      default:
         vcos_assert(0);
         // keep things going
         if ((msg->code & VCOS_MSG_REPLY_BIT) == 0)
            vcos_msg_reply(msg);
      }
   }
      
   vcos_msgq_endpoint_delete(&self);
   return (void*)count;
}

static void *simple_client(void *cxt)
{
   int i;
   VCOS_MSGQUEUE_T *q;
   VCOS_MSG_ENDPOINT_T self;

   VCOS_STATUS_T st = vcos_msgq_endpoint_create(&self, "client");
   vcos_assert(st == VCOS_SUCCESS);

   q = vcos_msgq_wait("server");

   server_open(q);

   for (i=0; i<1024; i++)
   {
      VCOS_MSG_T msg;
      struct my_msg_t *body = VCOS_MSG_DATA(&msg);
      body->i = i;
      vcos_msg_sendwait(q, MSG_N_MY_MSG, &msg);
      vcos_assert(body->i == i+1); // did the server bump the count?
   }

   server_close(q);

   vcos_msgq_endpoint_delete(&self);
   return (void*)i;
}


// Create a single server, and some clients. Run the clients, then
// terminate the server.

#define N_CLIENTS 8

static int simple_pingpong(void)
{
   static VCOS_THREAD_T server, clients[N_CLIENTS];
   unsigned long sent, received = 0;
   void *psent = &sent;
   int i;
   VCOS_STATUS_T st;
   VCOS_MSG_ENDPOINT_T self;

   st = vcos_msgq_endpoint_create(&self,"test");

   st = vcos_thread_create(&server, "server", NULL, simple_server, NULL);

   for (i=0; i<N_CLIENTS; i++)
   {
      st = vcos_thread_create(&clients[i], "client", NULL, simple_client, NULL);
   }

   // wait for the clients to quit
   for (i=0; i<N_CLIENTS; i++)
   {
      unsigned long rx;
      void *prx = &rx;
      vcos_thread_join(&clients[i],prx);
      received += rx;
   }

   // tell the server to quit
   {
      VCOS_MSG_T quit;
      VCOS_MSGQUEUE_T *serverq = vcos_msgq_find("server");

      vcos_assert(serverq != NULL);

      vcos_log("client: sending quit");
      vcos_msg_sendwait(serverq, VCOS_MSG_N_QUIT, &quit);
      vcos_thread_join(&server,psent);
   }

   vcos_msgq_endpoint_delete(&self);

   if (sent == received)
      return 1;
   else
   {
      vcos_log("bad: sent %d, received %d", sent, received);
      return 0;
   }
}

void run_msgqueue_tests(int *run, int *passed)
{
   RUN_TEST(run,passed,simple_pingpong);
}




