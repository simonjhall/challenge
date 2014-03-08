/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef CLOCK_H_
#define CLOCK_H_

#include "vcinclude/common.h"
#include "vcfw/drivers/driver.h"


// always use this type to pass frequencies around.  units of Hz.
typedef uint32_t frequency_t;


/* ----------------------------------------------------------------------
 * enum of all possible plls (superset of all chips)
 * -------------------------------------------------------------------- */
typedef enum
{
   CLOCK_PLL_MIN,

   CLOCK_PLL_A,
   CLOCK_PLL_B,
   CLOCK_PLL_C,
   CLOCK_PLL_D,
   CLOCK_PLL_H,

   CLOCK_PLL_MAX

} CLOCK_PLL_T;

/* ----------------------------------------------------------------------
 * enum of all possible pll channels
 * -------------------------------------------------------------------- */
typedef enum
{
   CLOCK_PLL_CHAN_MIN,

#if defined(__VIDEOCORE4__)

   // 2708 pll channels
   CLOCK_PLL_CHAN_ACORE,
   CLOCK_PLL_CHAN_APER,
   //CLOCK_PLL_CHAN_ADSI0, // not used on any platforms yet
   CLOCK_PLL_CHAN_ACCP2,

   CLOCK_PLL_CHAN_BARM,

   CLOCK_PLL_CHAN_CCORE0,
   CLOCK_PLL_CHAN_CCORE1,
   CLOCK_PLL_CHAN_CCORE2,
   CLOCK_PLL_CHAN_CPER,

   CLOCK_PLL_CHAN_DCORE,
   CLOCK_PLL_CHAN_DPER,
   CLOCK_PLL_CHAN_DDSI0,
   CLOCK_PLL_CHAN_DDSI1,

   CLOCK_PLL_CHAN_HPIX,
   //CLOCK_PLL_CHAN_HRCAL, // not used on any platforms yet
   CLOCK_PLL_CHAN_HAUX,

#elif defined(__VIDEOCORE3__)

   // 2707 pll channels
   CLOCK_PLL_CHAN_APER,
   CLOCK_PLL_CHAN_ACCP2,
   CLOCK_PLL_CHAN_ADSI,
   CLOCK_PLL_CHAN_ASDC,

   CLOCK_PLL_CHAN_CCORE0,
   CLOCK_PLL_CHAN_CCORE1,

   CLOCK_PLL_CHAN_DPER,
   CLOCK_PLL_CHAN_DCDP,
   CLOCK_PLL_CHAN_DSDC,

   CLOCK_PLL_CHAN_H1,

#else
#   error "unknown architecture"
#endif
   CLOCK_PLL_CHAN_MAX
} CLOCK_PLL_CHAN_T;

/* ----------------------------------------------------------------------
 * enum of all possible clock sources (superset of all chips)
 * -------------------------------------------------------------------- */
typedef enum
{
   CLOCK_SOURCE_MIN,

   CLOCK_SOURCE_NONE,
   CLOCK_SOURCE_FIXED,

   CLOCK_SOURCE_OSC,
   CLOCK_SOURCE_TEST,

   CLOCK_SOURCE_PLLA,
   CLOCK_SOURCE_PLLB,
   CLOCK_SOURCE_PLLC,
   CLOCK_SOURCE_PLLD,
   CLOCK_SOURCE_PLLH,

   CLOCK_SOURCE_CSI1,
   CLOCK_SOURCE_CSI2,
   CLOCK_SOURCE_CDP,
   CLOCK_SOURCE_DSI,
   CLOCK_SOURCE_CCP2TX,

   CLOCK_SOURCE_DSI0_BYTE,
   CLOCK_SOURCE_DSI0_DDR,
   CLOCK_SOURCE_DSI0_DDR_2,
   CLOCK_SOURCE_DSI1_BYTE,
   CLOCK_SOURCE_DSI1_DDR,
   CLOCK_SOURCE_DSI1_DDR_2,

   CLOCK_SOURCE_CAM0_PHY,
   CLOCK_SOURCE_CAM0_PHY_INV,
   CLOCK_SOURCE_CAM1_PHY,
   CLOCK_SOURCE_CAM1_PHY_INV,

   // 2707 adds these
   CLOCK_SOURCE_CAM1_PHY_DDR_2,
   CLOCK_SOURCE_CAM1_PHY_BYTE0,

   CLOCK_SOURCE_MAX

} CLOCK_SOURCE_T;

/* ----------------------------------------------------------------------
 * enum of all possible clock outputs (superset of all chips)
 * -------------------------------------------------------------------- */
typedef enum
{
   CLOCK_OUTPUT_MIN,

   CLOCK_OUTPUT_CORE,
#if defined(__VIDEOCORE4__)
   CLOCK_OUTPUT_SDRAM500, // 500 MHz family of frequencies
   CLOCK_OUTPUT_SDRAM400, // 400 MHz family of frequencies
#else
   CLOCK_OUTPUT_SDRAM332, // 332 MHz family of frequencies
   CLOCK_OUTPUT_SDRAM300, // 300 MHz family of frequencies
#endif

   CLOCK_OUTPUT_DPI,
   CLOCK_OUTPUT_DSI0_PHY,
   CLOCK_OUTPUT_DSI1_PHY,
   CLOCK_OUTPUT_DSI0,
   CLOCK_OUTPUT_DSI1,
   CLOCK_OUTPUT_DSI0_ESC,
   CLOCK_OUTPUT_DSI1_ESC,
   CLOCK_OUTPUT_HDMI,
   CLOCK_OUTPUT_VEC,
   CLOCK_OUTPUT_CPI,
   CLOCK_OUTPUT_CCP2_TX_PHY,
   CLOCK_OUTPUT_CCP2_TX,
   CLOCK_OUTPUT_CCP2_RX,
   CLOCK_OUTPUT_CSI2_ESC,
   CLOCK_OUTPUT_GPCLK0,
   CLOCK_OUTPUT_GPCLK1,
   CLOCK_OUTPUT_GPCLK2,
   CLOCK_OUTPUT_CDP,
   CLOCK_OUTPUT_UART,
   CLOCK_OUTPUT_PCM,
   CLOCK_OUTPUT_SMI,
   CLOCK_OUTPUT_PWM,
   CLOCK_OUTPUT_OTP,
   CLOCK_OUTPUT_SMPS,
   CLOCK_OUTPUT_SYSTIMER,
   CLOCK_OUTPUT_H264,
   CLOCK_OUTPUT_PIXEL,

   CLOCK_OUTPUT_CAM0_LANE0,
   CLOCK_OUTPUT_CAM0_LANE1,
   CLOCK_OUTPUT_CAM0_BIT,
   CLOCK_OUTPUT_CAM1_LANE0,
   CLOCK_OUTPUT_CAM1_LANE1,
   CLOCK_OUTPUT_CAM1_LANE2,
   CLOCK_OUTPUT_CAM1_LANE3,
   CLOCK_OUTPUT_CAM1_BIT,

   CLOCK_OUTPUT_CAM0_LP,
   CLOCK_OUTPUT_CAM0_HS,
   CLOCK_OUTPUT_CAM1_LP,
   CLOCK_OUTPUT_CAM1_HS,

   CLOCK_OUTPUT_ISP,

   CLOCK_OUTPUT_USB,
   CLOCK_OUTPUT_V3D,
   CLOCK_OUTPUT_SLIMBUS,
   CLOCK_OUTPUT_ARM,
   CLOCK_OUTPUT_EMMC,

   CLOCK_OUTPUT_TD0,
   
   CLOCK_OUTPUT_MAX

} CLOCK_OUTPUT_T;


/******************************************************************************
 some handy macros
 *****************************************************************************/

typedef struct
{
   CLOCK_OUTPUT_T   output;
   CLOCK_SOURCE_T   source;
   CLOCK_PLL_CHAN_T chan;
   frequency_t      freq;
   int              mash;
} CLOCK_SOURCES_FOR_OUTPUT_T;


/******************************************************************************
 API
 *****************************************************************************/

// routine to set the output mux for the given clock
typedef int32_t (*CLOCK_SET_MUX)( const DRIVER_HANDLE_T handle,
                                  const CLOCK_OUTPUT_T clk_output,
                                  const CLOCK_SOURCE_T clk_src );

// routine to set a pll's vco frequency
typedef int32_t (*CLOCK_SET_PLL_VCO)( const DRIVER_HANDLE_T handle,
                                      const CLOCK_PLL_T pll,
                                      const frequency_t freq_in_hz );

// routine to query a pll's vco frequency
typedef frequency_t (*CLOCK_PROBE_VCO)( const DRIVER_HANDLE_T handle,
                                        const CLOCK_PLL_T pll );

// routine to set (integer) divider for a pll channel
typedef int32_t (*CLOCK_SET_PLL_CHAN)( const DRIVER_HANDLE_T handle,
                                       const CLOCK_PLL_CHAN_T pll_chan,
                                       const uint32_t div );

// routine to set the source for a particular clock output
typedef int32_t (*CLOCK_SET_CLOCK)( const DRIVER_HANDLE_T handle,
                                    const CLOCK_OUTPUT_T clk_output,
                                    const CLOCK_SOURCE_T clk_src,
                                    const CLOCK_PLL_CHAN_T pll_chan,
                                    float div, uint32_t mash );

// CM_SDC is owned by the sdram driver, so provide a routine to map
// CLOCK_OUTPUT_SDRAM* to 'src' mux setting
typedef uint32_t (*CLOCK_SDRAM_SOURCE)( const DRIVER_HANDLE_T handle,
                                        const CLOCK_OUTPUT_T clock );

// routine to measure the given clock using on-chip countersf
typedef int32_t (*CLOCK_MEASURE)( const DRIVER_HANDLE_T handle,
                                  const CLOCK_OUTPUT_T clk_output );

// routine to query what sources are available for a given output clock
typedef int32_t (*CLOCK_GET_SOURCES)( const DRIVER_HANDLE_T handle,
                                      const CLOCK_OUTPUT_T clk_output,
                                      const frequency_t desired_output_freq,
                                      CLOCK_SOURCES_FOR_OUTPUT_T *sources,
                                      uint32_t *number_clock_sources,
                                      uint32_t *is_fractional_divider );


/******************************************************************************
 System driver struct
 *****************************************************************************/

typedef struct
{
   //include the common driver definitions - this is a macro
   COMMON_DRIVER_API(void const *unused)

   // routine to set the output mux for the given clock
   CLOCK_SET_MUX       set_mux;

   // routine to set a pll's vco frequency
   CLOCK_SET_PLL_VCO   set_pll_vco;

   // routine to query a pll's vco frequency
   CLOCK_PROBE_VCO     probe_vco;

   // routine to set (integer) divider for a pll channel
   CLOCK_SET_PLL_CHAN  set_pll_chan;

   // routine to set the source for a particular clock output
   CLOCK_SET_CLOCK     set_clock;

   // return mux value suitable for writing to cm_sdc
   CLOCK_SDRAM_SOURCE  sdram_source;

   // routine to measure the given clock using on-chip counters
   CLOCK_MEASURE       measure;

   // routine to query what sources are available for a give output clock
   CLOCK_GET_SOURCES   get_sources;

} CLOCK_DRIVER_T;


/******************************************************************************
 Global Functions
 *****************************************************************************/

const CLOCK_DRIVER_T *clock_get_func_table( void );


#endif // CLOCK_H_
