#include "../vchiq.h"
#include "helpers/vclib/vclib.h" /* for vclib_memcpy */
#include "vcfw/vclib/vclib.h" /* for vclib_obtain_VRF/vclib_release_VRF */

typedef struct
{
   VCHIQ_STATE_T *src;
   VCHIQ_STATE_T *dst;

   OS_EVENT_T started;
} THREAD_PARAMS_T;

static void remove_rx_slot(VCHIQ_STATE_T *state)
{
   VCHIQ_ATOMIC_SHORTS_T remove;

   state->ctrl.rx.slots[state->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1].size = state->ctrl.rx.remove.s.mark;

   remove.atomic = state->ctrl.rx.remove.atomic;
   remove.s.slot++;
   remove.s.mark = 0;
   state->ctrl.rx.remove.atomic = remove.atomic;
}

static void ctrl_func(void *v)
{
   THREAD_PARAMS_T *params = (THREAD_PARAMS_T *)v;

   VCHIQ_STATE_T *src = params->src;
   VCHIQ_STATE_T *dst = params->dst;

   os_event_signal(&params->started);

   while (1) {
      VCHIQ_TASK_T *task;
      VCHIQ_SLOT_T *slot;

      os_event_wait(&src->ctrl.tx.hardware);

      while (src->ctrl.tx.install != src->ctrl.tx.remove) {
         task = &src->ctrl.tx.tasks[src->ctrl.tx.remove & VCHIQ_NUM_TX_TASKS - 1];

         if (dst->ctrl.rx.remove.s.mark + task->size > VCHIQ_SLOT_SIZE)
            remove_rx_slot(dst);

         assert(dst->ctrl.rx.install != dst->ctrl.rx.remove.s.slot);

         slot = &dst->ctrl.rx.slots[dst->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1];

         vclib_obtain_VRF(1);
         vclib_memcpy(slot->data + dst->ctrl.rx.remove.s.mark, task->data, task->size);
         vclib_release_VRF();

         src->ctrl.tx.remove++;
         dst->ctrl.rx.remove.s.mark += task->size;
      }

      os_event_signal(&src->trigger);
      os_event_signal(&dst->trigger);
   }
}

static void bulk_func(void *v)
{
   THREAD_PARAMS_T *params = (THREAD_PARAMS_T *)v;

   VCHIQ_STATE_T *src = params->src;
   VCHIQ_STATE_T *dst = params->dst;

   os_event_signal(&params->started);

   while (1) {
      VCHIQ_TX_BULK_T *tx_bulk;
      VCHIQ_RX_BULK_T *rx_bulk;

      os_event_wait(&src->bulk.tx.hardware);

      while (src->bulk.tx.install != src->bulk.tx.remove) {
         tx_bulk = &src->bulk.tx.bulks[src->bulk.tx.remove & VCHIQ_NUM_CURRENT_TX_BULKS - 1];

         assert(dst->bulk.rx.install != dst->bulk.rx.remove);

         rx_bulk = &dst->bulk.rx.bulks[dst->bulk.rx.remove & VCHIQ_NUM_RX_BULKS - 1];

         assert(rx_bulk->size == tx_bulk->size);

         assert(rx_bulk->data != NULL);
         vclib_obtain_VRF(1);
         vclib_memcpy(rx_bulk->data, tx_bulk->data, tx_bulk->size);
         vclib_release_VRF();

         src->bulk.tx.remove++;
         dst->bulk.rx.remove++;
      }

      os_event_signal(&src->trigger);
      os_event_signal(&dst->trigger);
   }
}

typedef void (*LOCAL_FUNC_T)(void *);

#define LOCAL_STACK 4096

static void start_thread(VCOS_THREAD_T *thread, LOCAL_FUNC_T func, VCHIQ_STATE_T *src, VCHIQ_STATE_T *dst)
{
#ifdef __CC_ARM
   THREAD_PARAMS_T params;
   params.src = src;
   params.dst = dst;
#else
   THREAD_PARAMS_T params = {src, dst};
#endif

   os_event_create(&params.started);
   os_thread_begin(thread, func, LOCAL_STACK, &params, "VCHIQ data pump");
   os_event_wait(&params.started);
   os_event_destroy(&params.started);
}

static VCOS_THREAD_T c2s_ctrl;
static VCOS_THREAD_T c2s_bulk;
static VCOS_THREAD_T s2c_ctrl;
static VCOS_THREAD_T s2c_bulk;

void local_init(VCHIQ_STATE_T *client, VCHIQ_STATE_T *server)
{
   start_thread(&c2s_ctrl, ctrl_func, client, server);
   start_thread(&c2s_bulk, bulk_func, client, server);
   start_thread(&s2c_ctrl, ctrl_func, server, client);
   start_thread(&s2c_bulk, bulk_func, server, client);
}
