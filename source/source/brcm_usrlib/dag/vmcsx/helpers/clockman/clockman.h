/* ============================================================================
Copyright (c) 2005-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef CLOCKMAN_H
#define CLOCKMAN_H

#include "vcinclude/common.h"
#include "vcfw/drivers/chip/clock.h"
#include "vcfw/drivers/chip/power.h"

/********************************************************************************
 Global Types
 ********************************************************************************/

/******************
* Enum of voltage request commands
*******************/
typedef enum
{
   CLOCKMAN_VOLTAGE_REQUEST_MIN,

   CLOCKMAN_VOLTAGE_REQUEST_NONE, /* no request at the moment */
   CLOCKMAN_VOLTAGE_REQUEST_MIN_REQUIRED, /* The param is the min voltage I need at the moment */

   CLOCKMAN_VOLTAGE_REQUEST_MAX

} CLOCKMAN_VOLTAGE_REQUEST_T;

#define CHECK_CLOCKMAN_VOLTAGE_REQUEST( voltage_req ) (( (voltage_req) > CLOCKMAN_VOLTAGE_REQUEST_MIN) && ( (voltage_req) < CLOCKMAN_VOLTAGE_REQUEST_MAX) )


/******************
* Enum of the power states
*******************/
typedef enum
{
   CLOCKMAN_POWERSTATE_MIN,

   CLOCKMAN_POWERSTATE_RUN,
   CLOCKMAN_POWERSTATE_HIBERNATE,
   CLOCKMAN_POWERSTATE_OFF,

   CLOCKMAN_POWERSTATE_MAX

} CLOCKMAN_POWERSTATE_T;

#define CHECK_CLOCKMAN_POWERSTATE_T( state ) ( ( (state) > CLOCKMAN_POWERSTATE_MIN) && ( (state) < CLOCKMAN_POWERSTATE_MAX) )



/******************
* Enum for the callback, to say what state a particular clock is in
*******************/
typedef enum
{
   CLOCKMAN_ASYN_CLOCK_STATE_MIN,

   CLOCKMAN_ASYN_CLOCK_STATE_PRE_CHANGE,
   CLOCKMAN_ASYN_CLOCK_STATE_POST_CHANGE,

   CLOCKMAN_ASYN_CLOCK_STATE_MAX

} CLOCKMAN_ASYN_CLOCK_STATE_T;

#define CHECK_CLOCKMAN_ASYN_CLOCK_STATE( state ) ( ( (state) > CLOCKMAN_ASYN_CLOCK_STATE_MIN) && ( (state) < CLOCKMAN_ASYN_CLOCK_STATE_MAX) )


/******************
* Callback - used when a particular pll freq has changed
*******************/
typedef int (*CLOCKMAN_CLOCK_FREQ_CHANGED_CALLBACK_T)(const CLOCK_OUTPUT_T clock_changed,
                                                      const CLOCKMAN_ASYN_CLOCK_STATE_T clock_state,
                                                      const uint32_t new_clock_freq );

/********************************************************************************
 Global Funcs
 ********************************************************************************/

   /* Routine to initialise the clockmananger */
   extern int clockman_initialise( const uint32_t initial_core_freq_in_hz );


   /* Routine to set the core clock freq */
   extern uint32_t clockman_set_core_freq( uint32_t freq,
                                           uint32_t blocking );


   /* Routine to register interest in a pll freq */
   // SL 2009-12-08: only used by filesystem/media/sdcard/sdcard.c
   extern int clockman_register_clock_change_callback( const CLOCK_OUTPUT_T clock,
                                                   CLOCKMAN_CLOCK_FREQ_CHANGED_CALLBACK_T callback );


   /* Routine to deregister interest in a pll freq */
   // SL 2009-12-08: not actually used anywhere
   extern int clockman_deregister_clock_change_callback( const CLOCK_OUTPUT_T clock,
                                                         const CLOCKMAN_CLOCK_FREQ_CHANGED_CALLBACK_T callback );

   // setup mux (for outputs with only a mux)
   int clockman_setup_mux( const CLOCK_OUTPUT_T clock,
                           const CLOCK_SOURCE_T source );

   /* Routine to set which async clock uses which pll */
   extern int clockman_setup_clock( const CLOCK_OUTPUT_T clock,
                                    const uint32_t ideal_clock_freq_in_hz,
                                    const uint32_t allowable_leeway_in_hz,
                                    uint32_t *actual_clock_freq_in_hz );


   /* Routine to get an async clock freq */
   extern uint32_t clockman_get_clock( const CLOCK_OUTPUT_T clock );

   /* Routine to measure clock using on-chip counters */
   uint32_t clockman_measure_clock( const CLOCK_OUTPUT_T clock );

#if 0 // SL 2009-12-08: not used anywhere
   /* Routine to tell clock manager about a change in power state */
   extern int clockman_power_control( const CLOCKMAN_POWERSTATE_T power_state );
#endif

   /* Routine to register an interest in to the core voltage */
   // SL 2009-12-08: only used in 2707/tu.c
   extern int clockman_register_core_voltage_request( void **core_voltage_handle );

   /* Routine to deregister an interest in to the core voltage */
   // SL 2009-12-08: only used in 2707/tu.c
   extern int clockman_deregister_core_voltage_request( const void *core_voltage_handle );

   /* Routine to set a party's request for a the core voltage */
   /* voltage request is in millivolts, so 1.2V == 1200 */
   // SL 2009-12-08: only used in 2707/tu.c
   extern int clockman_set_core_voltage_request(const void *core_voltage_handle,
                                                const CLOCKMAN_VOLTAGE_REQUEST_T request,
                                                const voltage_t request_param );

   // SL 2009-12-08: only used in middleware/powerman/powerman_display.c
   extern const char * clockman_get_core_voltage_text( void );

   extern int clockman_enable_perfmon( int enable );


#endif /* CLOCKMAN_H */

/****************************** End of file *******************************************/
