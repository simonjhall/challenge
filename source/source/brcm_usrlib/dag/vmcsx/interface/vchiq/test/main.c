#include "../vchiq.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _DEBUG
#define CHECK(x) assert(x)
#else
#define CHECK(x) (x)
#endif

//#define SPEED_TEST

#ifdef SPEED_TEST
#define NUM_SERVICES    1
#define NUM_SENDERS     4
#else
#define NUM_SERVICES    8
#define NUM_SENDERS     8
#endif

/*
   VCHIQ backend for testbench, comprising a pair of threads which write to and read from
   windows anonymous pipes. This may serve as a useful example of how to write the VCHIQ
   backend for a host architecture.
*/

typedef struct {
   VCHIQ_STATE_T *state;

   HANDLE handle;
} TEST_INFO_T;

static void remove_rx_slot(VCHIQ_STATE_T *state)
{
   VCHIQ_ATOMIC_SHORTS_T remove;

   state->ctrl.rx.slots[state->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1].size = state->ctrl.rx.remove.s.mark;

   remove.atomic = state->ctrl.rx.remove.atomic;
   remove.s.slot++;
   remove.s.mark = 0;
   state->ctrl.rx.remove.atomic = remove.atomic;
}

void test_read_thread(void *v)
{
   TEST_INFO_T *info = (TEST_INFO_T *)v;

   VCHIQ_STATE_T *state = info->state;
   HANDLE read = info->handle;

   while (1) {
      int chan;
      int len;
      int count;

      ReadFile(read, &chan, 4, (LPDWORD)&count, NULL);
      ReadFile(read, &len, 4, (LPDWORD)&count, NULL);

      if (chan) {
         VCHIQ_RX_BULK_T *bulk;

         assert(state->bulk.rx.install != state->bulk.rx.remove);

         bulk = &state->bulk.rx.bulks[state->bulk.rx.remove & VCHIQ_NUM_RX_BULKS - 1];

         assert(bulk->size == len);

         ReadFile(read, bulk->data, len, (LPDWORD)&count, NULL);  

         state->bulk.rx.remove++;
      } else {
         VCHIQ_SLOT_T *slot;

         if (state->ctrl.rx.remove.s.mark + len > VCHIQ_SLOT_SIZE)
            remove_rx_slot(state);

         assert(state->ctrl.rx.install != state->ctrl.rx.remove.s.slot);

         slot = &state->ctrl.rx.slots[state->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1];

         ReadFile(read, slot->data + state->ctrl.rx.remove.s.mark, len, (LPDWORD)&count, NULL);  
         state->ctrl.rx.remove.s.mark += len;
      }

      SetEvent(state->trigger);
   }
}

void test_write_thread(void *v)
{
   TEST_INFO_T *info = (TEST_INFO_T *)v;

   VCHIQ_STATE_T *state = info->state;
   HANDLE write = info->handle;

   while (1) {
      HANDLE handles[] = {state->ctrl.tx.hardware, state->bulk.tx.hardware};
      int count;
         
      WaitForMultipleObjects(2, handles, FALSE, INFINITE);

      {
         int chan = 0;

         while (state->ctrl.tx.install != state->ctrl.tx.remove) {
            VCHIQ_TASK_T *task = &state->ctrl.tx.tasks[state->ctrl.tx.remove & VCHIQ_NUM_TX_TASKS - 1];

            unsigned int size = task->size;

            WriteFile(write, &chan, 4, (LPDWORD)&count, NULL);
            WriteFile(write, &size, 4, (LPDWORD)&count, NULL);
            WriteFile(write, task->data, task->size, (LPDWORD)&count, NULL); 

            state->ctrl.tx.remove++;
         }
      }

      {
         VCHIQ_TX_BULK_T *bulk;
         int chan = 1;

         while (state->bulk.tx.install != state->bulk.tx.remove) {
            bulk = &state->bulk.tx.bulks[state->bulk.tx.remove & VCHIQ_NUM_CURRENT_TX_BULKS - 1];

            WriteFile(write, &chan, 4, (LPDWORD)&count, NULL);
            WriteFile(write, &bulk->size, 4, (LPDWORD)&count, NULL);
            WriteFile(write, bulk->data, bulk->size, (LPDWORD)&count, NULL); 

            state->bulk.tx.remove++;
         }
      }

      SetEvent(state->trigger);
   }
}

#pragma warning (push)
#pragma warning (disable : 4200)

typedef struct {
   int len;
   int defer;
   int seq;
   int seed;

   int data[0];
} TEST_MESSAGE_T;

#pragma warning (pop)

static int seqs[2][8][3];

static HANDLE mutexes[2][8][3];

static void test_init_seqs()
{
   int i, j, k;

   memset(seqs, 0, sizeof(seqs));

   for (i = 0; i < 2; i++)
      for (j = 0; j < 8; j++)
         for (k = 0; k < 3; k++) 
            mutexes[i][j][k] = CreateMutex(NULL, FALSE, NULL);
}

static void reseed()
{
   LARGE_INTEGER time;

   CHECK(QueryPerformanceCounter(&time));
   srand(time.LowPart);
}

int test_form_message(TEST_MESSAGE_T *message)
{
   int i;

#ifdef SPEED_TEST
   message->len = 0;
   message->defer = 1;
#else
   message->len = rand() & 0xff;
   message->defer = rand() & 1;
#endif

   reseed();

   message->seed = rand();

   srand(message->seed);

   for (i = 0; i < message->len; i++)
      message->data[i] = rand();

   return message->len * 4 + sizeof(TEST_MESSAGE_T);
}

VCHIQ_STATE_T client;
VCHIQ_STATE_T server;

static char bulks[256][4096];
static int position;
static HANDLE mutex;
static HANDLE semaphore;

static void test_init_bulks()
{
   position = 0;
   mutex = CreateMutex(NULL, FALSE, NULL);
   semaphore = CreateSemaphore(NULL, 256, 256, NULL);
}

void test_send_thread(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;

   char buffer[4096];

   TEST_MESSAGE_T *message = (TEST_MESSAGE_T *)buffer;

   while (1) {
      int service = rand() & NUM_SERVICES - 1;
      int len = test_form_message(message);

      VCHIQ_ELEMENT_T element = {buffer, len};

      CHECK(WaitForSingleObject(mutexes[state == &server][service][message->defer], INFINITE) == WAIT_OBJECT_0);

      message->seq = seqs[state == &server][service][message->defer]++;
      vchiq_queue_message(state, VCHIQ_MAKE_FOURCC('S', 'V', 'C', '0' + service), &element, 1);

      CHECK(ReleaseMutex(mutexes[state == &server][service][message->defer]));
#ifndef SPEED_TEST
      if (message->defer && message->len < 4) {
         int i;

         for (i = 0; i < message->len; i++) {
            HANDLE handles[] = {mutex, semaphore};

            TEST_MESSAGE_T *bulk;

            CHECK(WaitForMultipleObjects(2, handles, TRUE, INFINITE) == WAIT_OBJECT_0);
            bulk = (TEST_MESSAGE_T *)bulks[position & 0xff];
            position++;
            CHECK(ReleaseMutex(mutex));

            test_form_message(bulk);

            CHECK(WaitForSingleObject(mutexes[state == &server][service][2], INFINITE) == WAIT_OBJECT_0);

            bulk->seq = seqs[state == &server][service][2]++;
            bulk->defer = 2;
            vchiq_queue_bulk_transmit(state, VCHIQ_MAKE_FOURCC('S', 'V', 'C', '0' + service), MEM_INVALID_HANDLE, bulk, 4096, NULL);

            CHECK(ReleaseMutex(mutexes[state == &server][service][2]));
         }
      }
#endif
   }
}

#define TEST_QUEUE_SIZE 16

typedef struct {
   VCHIQ_STATE_T *state;

   VCHIU_QUEUE_T queue;

   int fourcc;

   int immediate;       // next id for immediate processing messages
   int deferred;        // next id for deferred processing messages

   char *bulk[16];

   int read;
   int write;

   HANDLE sem;
} TEST_QUEUE_T;

void test_init_queue(TEST_QUEUE_T *queue, VCHIQ_STATE_T *state, int fourcc)
{
   int i;

   queue->state = state;

   vchiu_queue_init(&queue->queue, TEST_QUEUE_SIZE);

   queue->fourcc = fourcc;
   queue->immediate = 0;
   queue->deferred = 0;

   for (i = 0; i < 16; i++)
      queue->bulk[i] = (void *)((size_t)malloc(4096 + 0xf) + 0xf & ~0xf);

   queue->read = 0;
   queue->write = 0;

   queue->sem = CreateSemaphore(NULL, 16, 16, NULL);
}

void test_check_message(VCHIQ_HEADER_T *header, TEST_MESSAGE_T *message, TEST_QUEUE_T *queue)
{
   int i;

   assert(!header || message->len * 4 + sizeof(TEST_MESSAGE_T) == header->size);

   switch (message->defer) {
   case 0:
      assert(message->seq == queue->immediate);

      if ((queue->immediate++ & 0xfff) == 0) {
#ifndef SPEED_TEST
         printf("i");
#endif
      }
      break;
   case 1:
      assert(message->seq == queue->deferred);

      if ((queue->deferred++ & 0xfff) == 0) {
#ifndef SPEED_TEST
         printf("d");
#endif
      }
      break;
   case 2:
      assert(message->seq == queue->read);

      if ((queue->read++ & 0xff) == 0) {
#ifndef SPEED_TEST
         printf("b");
#endif
      }
      break;
   default:
      assert(0);
      break;
   }

   srand(message->seed);

   for (i = 0; i < message->len; i++)
      assert(message->data[i] == rand());
}

int test_callback(VCHIQ_REASON_T reason, VCHIQ_HEADER_T *header, void *service_user, void *bulk_user)
{
   TEST_QUEUE_T *queue = (TEST_QUEUE_T *)service_user;

   switch (reason) {
   case VCHIQ_MESSAGE_AVAILABLE:
   {
      TEST_MESSAGE_T *message = (TEST_MESSAGE_T *)header->data;

      if (message->defer) {
         vchiu_queue_push(&queue->queue, header);
      
         return 1;
      } else {
         test_check_message(header, message, queue);

         return 0;
      }
      break;
   }
   case VCHIQ_BULK_TRANSMIT_DONE:
      CHECK(ReleaseSemaphore(semaphore, 1, NULL));
      break;
   case VCHIQ_BULK_RECEIVE_DONE:
   {
      TEST_MESSAGE_T *message = (TEST_MESSAGE_T *)queue->bulk[queue->read & 0xf];

      test_check_message(NULL, message, queue);

      CHECK(ReleaseSemaphore(queue->sem, 1, NULL));
      break;
   }
   default:
      assert(0);
      break;
   }

   return 0;
}

void test_receive_thread(void *v)
{
   TEST_QUEUE_T *queue = (TEST_QUEUE_T *)v;

   while (1) {
      VCHIQ_HEADER_T *header = vchiu_queue_pop(&queue->queue);
      TEST_MESSAGE_T *message = (TEST_MESSAGE_T *)header->data;

      test_check_message(header, message, queue);
#ifndef SPEED_TEST
      if (message->len < 4) {
         int i;

         for (i = 0; i < message->len; i++) {
            CHECK(WaitForSingleObject(queue->sem, INFINITE) == WAIT_OBJECT_0);
            vchiq_queue_bulk_receive(queue->state, queue->fourcc, MEM_INVALID_HANDLE, queue->bulk[queue->write & 0xf], 4096, NULL);
            queue->write++;
         }
      }
#endif
      vchiq_release_message(queue->state, header);
   }
}

void test_connect_thread(void *v)
{
   TEST_INFO_T *info = (TEST_INFO_T *)v;

   vchiq_connect(info->state);

   SetEvent(info->handle);
}

#define USE_LOCAL
#ifdef USE_LOCAL
extern void local_init(VCHIQ_STATE_T *client, VCHIQ_STATE_T *server);
#endif

int main(int argc, char **argv)
{
   TEST_QUEUE_T client_queues[8];
   TEST_QUEUE_T server_queues[8];

   HANDLE c2s_read, c2s_write;
   HANDLE s2c_read, s2c_write;

   BOOL result1 = CreatePipe(&c2s_read, &c2s_write, NULL, 0);
   BOOL result2 = CreatePipe(&s2c_read, &s2c_write, NULL, 0);

   TEST_INFO_T client_read_info = {&client, s2c_read};
   TEST_INFO_T client_write_info = {&client, c2s_write};
   TEST_INFO_T server_read_info = {&server, c2s_read};
   TEST_INFO_T server_write_info = {&server, s2c_write};

   HANDLE client_event = CreateEvent(NULL, FALSE, FALSE, NULL);
   HANDLE server_event = CreateEvent(NULL, FALSE, FALSE, NULL);

   TEST_INFO_T client_connect_info = {&client, client_event};
   TEST_INFO_T server_connect_info = {&server, server_event};

   HANDLE events[] = {client_event, server_event};

   int i;

   /*
      set up client
   */

   vchiq_init_state(&client);

   for (i = 0; i < NUM_SERVICES; i++) {
      int fourcc = VCHIQ_MAKE_FOURCC('S', 'V', 'C', '0' + i);

      test_init_queue(&client_queues[i], &client, fourcc);

      _beginthread(test_receive_thread, 0, &client_queues[i]);

      vchiq_add_service(&client, fourcc, test_callback, &client_queues[i]);
   }

   /*
      set up server
   */

   vchiq_init_state(&server);

   for (i = 0; i < NUM_SERVICES; i++) {
      int fourcc = VCHIQ_MAKE_FOURCC('S', 'V', 'C', '0' + i);

      test_init_queue(&server_queues[i], &server, fourcc);

      _beginthread(test_receive_thread, 0, &server_queues[i]);

      vchiq_add_service(&server, fourcc, test_callback, &server_queues[i]);
   }

   /*
      start data transfer threads
   */

#ifdef USE_LOCAL
   local_init(&client, &server);
#else
   _beginthread(test_read_thread, 0, &client_read_info);
   _beginthread(test_write_thread, 0, &client_write_info);
   _beginthread(test_read_thread, 0, &server_read_info);
   _beginthread(test_write_thread, 0, &server_write_info);
#endif

   /*
      connect
   */

   _beginthread(test_connect_thread, 0, &client_connect_info);
   _beginthread(test_connect_thread, 0, &server_connect_info);

   WaitForMultipleObjects(2, events, TRUE, INFINITE);

   test_init_seqs();
   test_init_bulks();

   printf("connected\n");

   for (i = 0; i < NUM_SENDERS; i++) {
      _beginthread(test_send_thread, 0, &client);
      _beginthread(test_send_thread, 0, &server);
   }

   while (1) {
      Sleep(10000);
#ifdef SPEED_TEST
      printf("%d\n", client_queues[0].deferred);
#endif
   }
}
