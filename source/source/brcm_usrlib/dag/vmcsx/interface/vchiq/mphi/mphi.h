// Copyright (c) 2009 Broadcom Corporation. All rights reserved.

#ifndef VCHIQ_MPHI_H_
#define VCHIQ_MPHI_H_

#include <CommonTypes.h>


extern void VchiMphiControlReadCallback(  void * Context, uint_t Offset, uint_t Bytes );
extern void VchiMphiBulkReadCallback(     void * Context );
extern void VchiMphiControlWriteCallback( void * Context );
extern void VchiMphiBulkWriteCallback(    void * Context );


#endif /* ifndef VCHIQ_MPHI_H_ */
