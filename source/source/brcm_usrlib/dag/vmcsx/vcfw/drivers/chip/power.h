/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef POWER_H_
#define POWER_H_

#include "vcinclude/common.h"
#include "vcfw/drivers/driver.h"
#include "helpers/sysman/sysman.h"


/******************************************************************************
 Global definitions
 *****************************************************************************/

//enum of the pads
typedef enum
{
   POWER_PAD_MIN,

   POWER_PAD_SDRAM,
   POWER_PAD_HOSTPORT,
   POWER_PAD_GPIO_BANK0,
   POWER_PAD_GPIO_BANK1,
   POWER_PAD_GPIO_BANK2,
   POWER_PAD_JTAG_HDMI_I2C,
   POWER_PAD_SLIMBUS,

   POWER_PAD_MAX

} POWER_PAD_T;

/*
 * enum of all possible domains on all chips, to be implemented as bitmask
 *
 * note that sysman will always enable domains from lowest to highest,
 * and disable from highest to lowest.  or in other words, it enforces
 * correct sequencing of power domains - provided that the bitfields,
 * below, are ordered correctly!
 */

#if defined(__VIDEOCORE4__)
typedef enum
{
   // 2708 has several domains, plus a bunch of ldo's
   POWER_DOMAIN_AUDIO      = 0x00000001,
   //
   POWER_DOMAIN_IMAGE      = 0x00000002,
   POWER_DOMAIN_IMAGE_H264 = 0x00000004,
   POWER_DOMAIN_IMAGE_ISP  = 0x00000008,
   POWER_DOMAIN_IMAGE_PERI = 0x00000010,
   //
   POWER_DOMAIN_GRAFX      = 0x00000020,
   POWER_DOMAIN_GRAFX_V3D  = 0x00000040,
   //
   POWER_DOMAIN_PROC       = 0x00000080,
   POWER_DOMAIN_PROC_ARM   = 0x00000100,
   //
   POWER_DOMAIN_PX_LDO     = 0x00000200,
   POWER_DOMAIN_HDMI_LDO   = 0x00000400,
   POWER_DOMAIN_CAM0_LDO   = 0x00000800,
   POWER_DOMAIN_CAM1_LDO   = 0x00001000,
   POWER_DOMAIN_CCP2TX_LDO = 0x00002000,
   POWER_DOMAIN_DSI0_LDO   = 0x00004000,
   POWER_DOMAIN_DSI1_LDO   = 0x00008000,
   POWER_DOMAIN_USB_LDO    = 0x00010000,
   //
   POWER_DOMAIN_CE_VPU1    = 0x00020000,
   // sysman_disable must come last!
   POWER_DOMAIN_SYSMAN_DISABLE = 0x00040000,
} POWER_DOMAIN_T;

#define POWER_DOMAIN_FIRST    POWER_DOMAIN_AUDIO
#define POWER_DOMAIN_LAST     POWER_DOMAIN_SYSMAN_DISABLE

#elif defined(__VIDEOCORE3__)

typedef enum
{

   // 2707 only has two domains, plus a bunch of ldo's
   POWER_DOMAIN_MINI       = 0x1,
   POWER_DOMAIN_RUN        = 0x2,
   //
   POWER_DOMAIN_CE_VPU1    = 0x4,
   POWER_DOMAIN_CE_HDMI    = 0x8,
   POWER_DOMAIN_CE_H264    = 0x10,
   POWER_DOMAIN_CE_ISP     = 0x20,
   POWER_DOMAIN_CE_V3D     = 0x40,
   POWER_DOMAIN_CE_VEC     = 0x80,
   // FIXME: should the ldo's go before clock enables?
   // fyi old sysman behaviour was clock enables first, ldo's last
   POWER_DOMAIN_RLDO       = 0x100,
   POWER_DOMAIN_MLDO       = 0x200,
   // sysman_disable must come last!
   POWER_DOMAIN_SYSMAN_DISABLE = 0x400,
} POWER_DOMAIN_T;

#else
#   error "unknown architecture"
#endif

// we will natively measure voltage in mV
typedef uint16_t voltage_t;

/******************************************************************************
 API
 *****************************************************************************/

//routine to enable a given power domain
typedef int32_t (*POWER_ENABLE_DOMAIN)( DRIVER_HANDLE_T handle,
                                        POWER_DOMAIN_T domain );

//routine to disable a given power domain
typedef int32_t (*POWER_DISABLE_DOMAIN)( DRIVER_HANDLE_T handle,
                                         POWER_DOMAIN_T domain );

//routine to map between a sysman block and power domain(s)
typedef POWER_DOMAIN_T (*POWER_SYSMAN_DOMAINS)( DRIVER_HANDLE_T handle,
                                                SYSMAN_BLOCK_T block );

//routine to set the core voltage (in units of mV)
typedef int32_t (*POWER_SET_CORE_VOLTAGE)( DRIVER_HANDLE_T handle,
                                           voltage_t mv,
                                           int wait_for_voltage_to_settle );

//routine to get the core voltage (in units of mV)
typedef voltage_t (*POWER_GET_CORE_VOLTAGE)( DRIVER_HANDLE_T handle );

//routine to get the core voltage for a specific core frequency (units of mV, Hz)
typedef voltage_t(*POWER_GET_CORE_VOLTAGE_FOR_FREQUENCY_X)( DRIVER_HANDLE_T handle,
                                                            uint32_t frequency_in_hz,
                                                            uint32_t ringosc_freq);

//routine to control the pads
typedef int32_t (*POWER_PAD_CONTROL)( DRIVER_HANDLE_T handle,
                                      POWER_PAD_T pad,
                                      uint32_t slew,
                                      uint32_t enable_hysteresis,
                                      uint32_t drive_strength );

//routine which indicates whether VPU1 has been started
typedef int32_t (*POWER_VPU1_IS_AWAKE)(DRIVER_HANDLE_T handle);

/******************************************************************************
 System driver struct
 *****************************************************************************/

typedef struct
{
   //include the common driver definitions - this is a macro
   COMMON_DRIVER_API(void const *unused)

   //routine to enable the given power domain
   POWER_ENABLE_DOMAIN enable_domain;

   //routine to disable the given power domain
   POWER_DISABLE_DOMAIN disable_domain;

   //routine to map between a sysman block and power domain(s)
   POWER_SYSMAN_DOMAINS sysman_domains;

   //routine to set the core voltage (mV)
   POWER_SET_CORE_VOLTAGE  set_core_voltage;

   //routine to get the core voltage (mV)
   POWER_GET_CORE_VOLTAGE  get_core_voltage;

   //routine to get the core voltage (mV) for a specific core frequency (Hz)
   POWER_GET_CORE_VOLTAGE_FOR_FREQUENCY_X  get_core_voltage_for_frequency_x;

   //routine to control the pads
   POWER_PAD_CONTROL pad_control;

   //routine which indicates whether VPU1 has been started
   POWER_VPU1_IS_AWAKE vpu1_is_awake;

} POWER_DRIVER_T;

/******************************************************************************
 Global Functions
 *****************************************************************************/

const POWER_DRIVER_T *power_get_func_table( void );

#endif // POWER_H_
