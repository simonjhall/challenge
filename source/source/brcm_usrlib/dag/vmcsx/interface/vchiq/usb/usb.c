#include "../vchiq.h"

#include "tools/usbhp/bcm2727dkb0rev2/hp2727dk/hpapi.h"

// uncomment to drop first 32 bytes of received data (use when VMCS has been booted from SDCard by loader)
//#define USBHP_SKIP_HEADER

#define USBHP_BCM2708

static void remove_rx_slot(VCHIQ_STATE_T *state)
{
   VCHIQ_ATOMIC_SHORTS_T remove;

   state->ctrl.rx.slots[state->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1].size = state->ctrl.rx.remove.s.mark;

   remove.atomic = state->ctrl.rx.remove.atomic;
   remove.s.slot++;
   remove.s.mark = 0;
   state->ctrl.rx.remove.atomic = remove.atomic;
}

void usbhp_thread(void *v)
{
   VCHIQ_STATE_T *state = (VCHIQ_STATE_T *)v;

#ifdef USBHP_SKIP_HEADER
   {
      char dummy[32] = {65};

      unsigned long status;
      int chan;
      int len;

      do {
          hp_get_status(state->handle, &status);
      } while (~status & 0x40000000);

      chan = status & 0x10000000;
      len = (status >> 1 & 0x00ff8000 | status & 0x00007fff) + 1;

      assert(chan == 0);
      assert(len == 32);

      hp_read_host(state->handle, 32, dummy);
   }
#endif

#ifdef USBHP_BCM2708
   {
      // Ensure that BCM2708 MPHI is in 16 bit mode
      int dummy;
      hp_write_host_specified(state->handle, 0x3000000, 0, &dummy);
   }
#endif

   while (1) {
      unsigned long status;
      hp_get_status(state->handle, &status);

      if (status & 0x40000000) {
         VCHIQ_SLOT_T *slot;

         int chan = status & 0x10000000;
         int len = (status >> 1 & 0x00ff8000 | status & 0x00007fff) + 1;

         if (chan) {
            VCHIQ_RX_BULK_T *bulk;

            assert(state->bulk.rx.install != state->bulk.rx.remove);

            bulk = &state->bulk.rx.bulks[state->bulk.rx.remove & VCHIQ_NUM_RX_BULKS - 1];

            assert(bulk->size == len);

            hp_read_host(state->handle, len, bulk->data);
            state->bulk.rx.remove++;
         } else {
            if (state->ctrl.rx.remove.s.mark + len > VCHIQ_SLOT_SIZE)
               remove_rx_slot(state);

            assert(state->ctrl.rx.install != state->ctrl.rx.remove.s.slot);

            slot = &state->ctrl.rx.slots[state->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1];

            hp_read_host(state->handle, len, slot->data + state->ctrl.rx.remove.s.mark);
            state->ctrl.rx.remove.s.mark += len;
         }

         os_event_signal(&state->trigger);
      }

      {
         int dummy = 0;

         while (state->ctrl.tx.install != state->ctrl.tx.remove) {
            VCHIQ_TASK_T *task = &state->ctrl.tx.tasks[state->ctrl.tx.remove & VCHIQ_NUM_TX_TASKS - 1];

            if (task->term) {
               VCHIQ_HEADER_T term;

               term.u.s.fourcc = VCHIQ_FOURCC_TERMINATE;
               term.u.s.peer = task->data->u.s.peer;
               term.size = 0;

               hp_write_host(state->handle, HP_CHANNEL_0 | HP_TERMINATE_DMA, 16, &term);
            }

            hp_write_host(state->handle, HP_CHANNEL_0, task->size, task->data);
            hp_write_host(state->handle, HP_CHANNEL_0, 0, &dummy);

            state->ctrl.tx.remove++;
         }

         os_event_signal(&state->trigger);
      }

      {
         int dummy = 0;

         while (state->bulk.tx.install != state->bulk.tx.remove) {
            VCHIQ_TX_BULK_T *bulk = &state->bulk.tx.bulks[state->bulk.tx.remove & VCHIQ_NUM_CURRENT_TX_BULKS - 1];

            hp_write_host(state->handle, HP_CHANNEL_1, bulk->size, (void*) bulk->data);
            hp_write_host(state->handle, HP_CHANNEL_1, 0, &dummy);

            state->bulk.tx.remove++;
         }

         os_event_signal(&state->trigger);
      }
   }
}

