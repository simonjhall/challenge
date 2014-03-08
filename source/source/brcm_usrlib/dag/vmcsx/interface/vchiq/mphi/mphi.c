// Copyright (c) 2008-9 Broadcom Corporation. All rights reserved.

#include "interface/vchiq/vchiq.h"
#include "mphi/Mphi.h"
#include "interface/vchiq/mphi/mphi.h"

//#define TRACE_READ
//#define TRACE_WRITE
#if defined(TRACE_READ) || defined(TRACE_WRITE)
  #include <linux/kernel.h> // printk
#endif


static void remove_rx_slot( VCHIQ_STATE_T * Vchi, uint_t Bytes )
{
#ifdef TRACE_READ
  printk( KERN_ERR "%s: Moving to next slot (%08x)\n", __PRETTY_FUNCTION__,
    (char *) Vchi->ctrl.rx.slots[ Vchi->ctrl.rx.remove.s.slot + 1 & VCHIQ_NUM_RX_SLOTS - 1 ].data );
#endif
  VCHIQ_ATOMIC_SHORTS_T Remove;
  Vchi->ctrl.rx.slots[ Vchi->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1 ].size = Vchi->ctrl.rx.remove.s.mark;
  Remove.atomic = Vchi->ctrl.rx.remove.atomic;
  ++Remove.s.slot;
  Remove.s.mark = Bytes;
  Vchi->ctrl.rx.remove.atomic = Remove.atomic;
}


extern void VchiMphiControlReadCallback( void * Context, uint_t Offset, uint_t Bytes );
extern void VchiMphiBulkReadCallback( void * Context );

void VchiMphiControlReadCallback( void * Context, uint_t Offset, uint_t Bytes )
{
  VCHIQ_STATE_T * Vchi = (VCHIQ_STATE_T *) Context;
#ifdef TRACE_READ
  VCHIQ_SLOT_T * Slot = &Vchi->ctrl.rx.slots[ Vchi->ctrl.rx.remove.s.slot & VCHIQ_NUM_RX_SLOTS - 1 ];
  printk( KERN_ERR "%s: Done receive of %4x bytes @ offset %4x (to %08x, mark %4x)\n",
    __PRETTY_FUNCTION__, Bytes, Offset, Slot == 0 ? 0 : (char *) Slot->data + Offset, (int) Vchi->ctrl.rx.remove.s.mark );
#endif
  if (Vchi->ctrl.rx.remove.s.mark + Bytes > VCHIQ_SLOT_SIZE)
  {
    ASSERT_EQ( Offset, 0 );
    remove_rx_slot( Vchi, Bytes );
  }
  else
  {
    ASSERT_EQ( Offset, Vchi->ctrl.rx.remove.s.mark );
    Vchi->ctrl.rx.remove.s.mark += Bytes;
  }
  os_event_signal( &Vchi->trigger );
}

void VchiMphiBulkReadCallback( void * Context )
{
  VCHIQ_STATE_T * Vchi = (VCHIQ_STATE_T *) Context;
#ifdef TRACE_READ
  printk( KERN_ERR "%s: Done\n", __PRETTY_FUNCTION__ );
#endif
  ++Vchi->bulk.rx.remove;
  os_event_signal( &Vchi->trigger );
}

extern void VchiMphiControlWriteCallback( void * Context );
void VchiMphiControlWriteCallback( void * Context )
{
  VCHIQ_STATE_T * Vchi = (VCHIQ_STATE_T *) Context;
#ifdef TRACE_WRITE
  VCHIQ_TASK_T  * Task = &Vchi->ctrl.tx.tasks[Vchi->ctrl.tx.remove & VCHIQ_NUM_TX_TASKS - 1];
  printk( KERN_ERR "%s: Completing write @ %u, %p +%4x\n", __PRETTY_FUNCTION__, Vchi->ctrl.tx.remove, Task->data, (uint_t) Task->size );
#endif

  ++Vchi->ctrl.tx.remove;
  os_event_signal( &Vchi->trigger );
}

extern void VchiMphiBulkWriteCallback( void * Context );
void VchiMphiBulkWriteCallback( void * Context )
{
  VCHIQ_STATE_T * Vchi = (VCHIQ_STATE_T *) Context;
#ifdef TRACE_WRITE
  printk( KERN_ERR "%s: Done\n", __PRETTY_FUNCTION__ );
#endif
  ++Vchi->bulk.tx.remove;
  os_event_signal( &Vchi->trigger );
}
