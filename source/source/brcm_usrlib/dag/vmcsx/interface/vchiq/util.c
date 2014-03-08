#include "util.h"

#include <stdlib.h>

static __inline int is_pow2(int i)
{
   return i && !(i & i - 1);
}

int vchiu_queue_init(VCHIU_QUEUE_T *queue, int size)
{
   assert(is_pow2(size));

   queue->size = size;
   queue->read = 0;
   queue->write = 0;

   os_event_create(&queue->pop);
   os_event_create(&queue->push);

   queue->storage = (VCHIQ_HEADER_T **)malloc(size * sizeof(VCHIQ_HEADER_T *));
   if(queue->storage == NULL)
   {
      assert(queue->storage != NULL);
      return 0;
   }

   return 1;
}

void vchiu_queue_delete(VCHIU_QUEUE_T *queue)
{
   free(queue->storage);
}

int vchiu_queue_is_empty(VCHIU_QUEUE_T *queue)
{
   return queue->read == queue->write;
}

void vchiu_queue_push(VCHIU_QUEUE_T *queue, VCHIQ_HEADER_T *header)
{
   while (queue->write == queue->read + queue->size)
      os_event_wait(&queue->pop);

   queue->storage[queue->write & queue->size - 1] = header;

   queue->write++;

   os_event_signal(&queue->push);
}

VCHIQ_HEADER_T *vchiu_queue_peek(VCHIU_QUEUE_T *queue)
{
   while (queue->write == queue->read)
      os_event_wait(&queue->push);

   return queue->storage[queue->read & queue->size - 1];
}

VCHIQ_HEADER_T *vchiu_queue_pop(VCHIU_QUEUE_T *queue)
{
   VCHIQ_HEADER_T *header;

   while (queue->write == queue->read)
      os_event_wait(&queue->push);

   header = queue->storage[queue->read & queue->size - 1];

   queue->read++;

   os_event_signal(&queue->pop);

   return header;
}
