#include <stdlib.h>

// Nasty hack: Eben's queue library only with with vchiq, so we have a separate copy for vchi

#ifndef NDEBUG

#define CHECK(x) os_assert(x)

static int is_pow2(int i)
{
   return i && !(i & i - 1);
}

#else
#define CHECK(x)
#endif

#include "interface/vcos/vcos.h"

typedef void VCHIQ_HEADER_T;
typedef struct {
   int size;
   int read;
   int write;

   VCOS_EVENT_T pop;
   VCOS_EVENT_T push;

   VCHIQ_HEADER_T **storage;
} VCHIU_QUEUE_T;

// Routine to create a semaphore

static void vchiu_queue_init(VCHIU_QUEUE_T *queue, int size);
static int vchiu_queue_is_empty(VCHIU_QUEUE_T *queue);
static void vchiu_queue_push(VCHIU_QUEUE_T *queue, VCHIQ_HEADER_T *header);

//static VCHIQ_HEADER_T *vchiu_queue_peek(VCHIU_QUEUE_T *queue);
static VCHIQ_HEADER_T *vchiu_queue_pop(VCHIU_QUEUE_T *queue);


void vchiu_queue_init(VCHIU_QUEUE_T *queue, int size)
{
   os_assert(is_pow2(size));

   queue->size = size;
   queue->read = 0;
   queue->write = 0;

   vcos_event_create(&queue->pop,NULL);
   vcos_event_create(&queue->push,NULL);

   queue->storage = (VCHIQ_HEADER_T**)os_malloc(size * sizeof(VCHIQ_HEADER_T *), OS_ALIGN_DEFAULT, "vchiu_queue");
   CHECK(queue->storage);
}

int vchiu_queue_is_empty(VCHIU_QUEUE_T *queue)
{
   return queue->read == queue->write;
}

void vchiu_queue_push(VCHIU_QUEUE_T *queue, VCHIQ_HEADER_T *header)
{
   os_assert(queue->write != queue->read + queue->size);

   queue->storage[queue->write & queue->size - 1] = header;

   queue->write++;

   vcos_event_signal(&queue->push);
}

/*VCHIQ_HEADER_T *vchiu_queue_peek(VCHIU_QUEUE_T *queue)
{
   while (queue->write == queue->read)
      vcos_event_wait(&queue->push);

   return queue->storage[queue->read & queue->size - 1];
}*/

VCHIQ_HEADER_T *vchiu_queue_pop(VCHIU_QUEUE_T *queue)
{
   VCHIQ_HEADER_T *header;

   while (queue->write == queue->read)
      vcos_event_wait(&queue->push);

   header = queue->storage[queue->read & queue->size - 1];

   queue->read++;

   vcos_event_signal(&queue->pop);

   return header;
}

/* endhack */

