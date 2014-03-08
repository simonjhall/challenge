/*==============================================================================
 Copyright (c) 2010 Broadcom Europe Limited.
 All rights reserved.

 Module   :  Power management unit service (host side)

 $Id: //software/customers/broadcom/mobcom/vc4/rel/interface/vmcs_host/vc_vchi_pmu.h#1 $

 FILE DESCRIPTION
 Host-side service providing interaction with the power management unit,
   for handling battery charging and USB connection and disconnection
==============================================================================*/

#ifndef VC_VCHI_PMU_H
#define VC_VCHI_PMU_H

#include "vchost_config.h"
#include "interface/vchi/vchi.h"

#if !defined(VCHPRE_)
#error VCHPRE_ not defined!
#endif
#if !defined(VCHPOST_)
#error VCHPOST_ not defined!
#endif

/*-----------------------------------------------------------------------------*/
// Values returned by vc_pmu_get_battery_status()
typedef enum
{
   BATT_NONE,
   BATT_LOW,
   BATT_VERY_LOW,
   BATT_OK,
   BATT_CHARGED
} PMU_BATTERY_STATE_T;

/*-----------------------------------------------------------------------------*/

VCHPRE_ void VCHPOST_ vc_vchi_pmu_init
   (VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections);

VCHPRE_ int32_t VCHPOST_ vc_pmu_enable_usb(uint32_t enable);
VCHPRE_ int32_t VCHPOST_ vc_pmu_usb_connected(void);
VCHPRE_ uint32_t VCHPOST_ vc_pmu_get_battery_status(void);

#endif /* VC_VCHI_PMU_H */
/* End of file */
/*-----------------------------------------------------------------------------*/
