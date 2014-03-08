/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited. All rights reserved.

Project  : VMCS
Module   : VCHI

FILE DESCRIPTION:

A wrapper designed to allow some clients that use the VCHIQ API to run
over a system that uses VCHI and has no support for VCHIQ.

This wrapper is not intended to be a complete VCHIQ implementation, it
is only designed to cover the current use made of VCHIQ by ILCS to
enable testing using ILCS on the CTS7a branch that uses VCHI.  It is
also not particularly efficient, for example it is around 80% slower
than the minimum threshold in the 'host_iface2' iltest.

It should not be used for other purposes.
=============================================================================*/

#include "vchi.h"
#include "interface/vchiq/vchiq.h"

#include "vchiq_wrapper.h"

#define SLOT_SIZE 4096
#define SLOT_NUM 16

typedef struct {
   VCHIQ_HEADER_T hdr;
   unsigned char data[SLOT_SIZE];
} SLOT_T;

struct _VCHIQ_WRAPPER_T {
   VCHIQ_STATE_T *state;
   int fourcc;
   VCHI_SERVICE_HANDLE_T vchi_handle;
   VCHIQ_CALLBACK_T callback;
   void *userdata;
   unsigned int slot_free;
   SLOT_T slot[SLOT_NUM];
   struct _VCHIQ_WRAPPER_T *next;
};

typedef struct _VCHIQ_WRAPPER_T VCHIQ_WRAPPER_T;

// we have to be able to search through all the wrappers since
// several vchiq methods don't pass in any service specific state
static VCHIQ_WRAPPER_T *wrapper_list;


void vchiq_wrapper_callback(void *callback_param,
                            VCHI_CALLBACK_REASON_T reason,
                            void *handle)
{
   VCHIQ_WRAPPER_T *st = (VCHIQ_WRAPPER_T *) callback_param;

   switch(reason) {
   case VCHI_CALLBACK_MSG_AVAILABLE:
      {
         void *data;
         uint32_t size;
         int count = 0;

         while(vchi_msg_peek(st->vchi_handle, &data, &size, 0) == 0)
         {
            SLOT_T *s;
            int slot = 0;
            vcos_demand(st->slot_free != 0);
            while(!((1<<slot) & st->slot_free))
               slot++;
            
            s = st->slot+slot;
            st->slot_free &= ~(1<<slot);
            s->hdr.u.slot = (VCHIQ_SLOT_T *) st; // store this away so we can release the slot properly
            s->hdr.size = size;
            
            vcos_demand(s->hdr.size <= SLOT_SIZE);
            memcpy(s->data, data, s->hdr.size);
            vchi_msg_remove(st->vchi_handle);
            
            st->callback(VCHIQ_MESSAGE_AVAILABLE, (VCHIQ_HEADER_T *) s, st->userdata, NULL);
         }
         break;
      }
   case VCHI_CALLBACK_BULK_RECEIVED:
      {
         // this doesn't seem to be used, but leave it in anyway
         st->callback(VCHIQ_BULK_RECEIVE_DONE, NULL, st->userdata, NULL);
         break;
      }
   }
}

int vchiq_wrapper_add_service(VCHIQ_STATE_T *state, void **vconnection, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata)
{
   VCHI_CONNECTION_T **connection = (VCHI_CONNECTION_T **) vconnection;
   VCHI_INSTANCE_T *instance_handle = (VCHI_INSTANCE_T *) state;
   SERVICE_CREATION_T parameters = { fourcc,                  // 4cc service code
                                     0,                       // passed in fn ptrs
                                     0,                       // tx fifo size (unused)
                                     0,                       // tx fifo size (unused)
                                     vchiq_wrapper_callback,  // service callback
                                     0 };                     // callback parameter
   VCHIQ_WRAPPER_T *st = vcos_malloc(sizeof(VCHIQ_WRAPPER_T), "vchiq wrapper");

   if(st == NULL)
      return 0;

   parameters.connection = connection[0];
   parameters.callback_param = st;
   st->state = state;
   st->fourcc = fourcc;
   st->callback = callback;
   st->userdata = userdata;
   st->slot_free = ((1<<SLOT_NUM)-1);

   if(vchi_service_create(*instance_handle, &parameters, &st->vchi_handle) != 0)
   {
      vcos_free(st);
      return 0;
   }

   // add to the global list of wrappers   
   st->next = wrapper_list;
   wrapper_list = st;
   
   return 1;
}

void vchiq_queue_message(VCHIQ_STATE_T *state, int fourcc, const VCHIQ_ELEMENT_T *elements, int count)
{
   VCHIQ_WRAPPER_T *st = wrapper_list;
   VCHI_MSG_VECTOR_EX_T vec[4];
   int i;

   while(st != NULL && (st->state != state || st->fourcc != fourcc))
      st = st->next;

   vcos_demand(st != NULL);
   vcos_demand(count <= 4);

   for(i=0; i<count; i++)
   {
      vec[i].type = VCHI_VEC_POINTER;
      vec[i].u.ptr.vec_base = elements[i].data;
      vec[i].u.ptr.vec_len = elements[i].size;
   }

   vchi_msg_queuev_ex(st->vchi_handle, vec, count, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL);
}

void vchiq_release_message(VCHIQ_STATE_T *state, VCHIQ_HEADER_T *header)
{
   SLOT_T *s = (SLOT_T *) header;
   VCHIQ_WRAPPER_T *st = (VCHIQ_WRAPPER_T *) s->hdr.u.slot;
   int slot = s-st->slot;
   
   vcos_demand(slot < SLOT_NUM);

   st->slot_free |= 1<<slot; 
}


// if handle==MEM_HANDLE_INVALID: data is a pointer, size is length
// if handle!=MEM_HANDLE_INVALID: data is int, offset from handle base, size is length
void vchiq_queue_bulk_transmit(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, const void *data, int size, void *userdata)
{
   VCHIQ_WRAPPER_T *st = wrapper_list;

   while(st != NULL && (st->state != state || st->fourcc != fourcc))
      st = st->next;

   vcos_demand(st != NULL);

   if(handle == MEM_HANDLE_INVALID)
      vchi_bulk_queue_transmit(st->vchi_handle, data, size, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
   else
      vchi_bulk_queue_transmit_reloc(st->vchi_handle, handle, (uint32_t) data, size, VCHI_FLAGS_BLOCK_UNTIL_QUEUED, NULL);
}

// if handle==MEM_HANDLE_INVALID: data is a pointer, size is length
// if handle!=MEM_HANDLE_INVALID: data is int, offset from handle base, size is length
void vchiq_queue_bulk_receive(VCHIQ_STATE_T *state, int fourcc, VCHI_MEM_HANDLE_T handle, void *data, int size, void *userdata)
{
   VCHIQ_WRAPPER_T *st = wrapper_list;

   while(st != NULL && (st->state != state || st->fourcc != fourcc))
      st = st->next;

   vcos_demand(st != NULL);

   if(handle == MEM_HANDLE_INVALID)
      vchi_bulk_queue_receive(st->vchi_handle, data, size, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL);
   else
      vchi_bulk_queue_receive_reloc(st->vchi_handle, handle, (uint32_t) data, size, VCHI_FLAGS_BLOCK_UNTIL_OP_COMPLETE, NULL);

   st->callback(VCHIQ_BULK_RECEIVE_DONE, NULL, st->userdata, NULL);
}

