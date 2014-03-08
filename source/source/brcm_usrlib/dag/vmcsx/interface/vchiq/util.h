#ifndef VCHIQ_UTIL_H
#define VCHIQ_UTIL_H

#include "core.h"

typedef struct {
   int size;
   int read;
   int write;

   OS_EVENT_T pop;
   OS_EVENT_T push;

   VCHIQ_HEADER_T **storage;
} VCHIU_QUEUE_T;

// returns 1 on success, 0 on failure
extern int vchiu_queue_init(VCHIU_QUEUE_T *queue, int size);

extern void vchiu_queue_delete(VCHIU_QUEUE_T *queue);

extern int vchiu_queue_is_empty(VCHIU_QUEUE_T *queue);

extern void vchiu_queue_push(VCHIU_QUEUE_T *queue, VCHIQ_HEADER_T *header);

extern VCHIQ_HEADER_T *vchiu_queue_peek(VCHIU_QUEUE_T *queue);
extern VCHIQ_HEADER_T *vchiu_queue_pop(VCHIU_QUEUE_T *queue);

#endif
