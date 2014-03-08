/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/* visualStats.h */
#include "helpers/vc_graphlib/graphlibrary.h"

/* structure represents information about a vpu task */
struct s_VPUtask{
 char *taskName;         // task name depicted in the legned
 float taskLoad;         // portion of vpu resource consumed
};
typedef struct s_VPUtask t_VPUtask;

/* structure represents vpu utilization data */
struct s_VPUInfo{
 uint32_t noOfTasks;    // number of tasks running on vpu
 t_VPUtask *tasks;      // array of tasks
};
typedef struct s_VPUInfo t_VPUInfo;

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void khrn_perform_init();

   DESCRIPTION: 
    Initializes all necessary graph details.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    Global variables are initialized in visualStats.c.
   ------------------------------------------------------------------*/
void khrn_perform_init();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void khrn_perform_drawStats(t_VPUInfo*,uint8_t);

   DESCRIPTION: 
    Draws all necessary graphs describing current performance.

   ARGUMENT(S): 
    t_VPUInfo* - data for vpu utilization charts
    uint8_t    - mode

   BEHAVIOUR:
    IF (vpuInf ==0) 
     - a FULL SCREEN line graph is drawn representing real-time preformance;
    ELSE 
     - VPU utililization graphs are drawn together with a bar graph 
       representing average performance;
   
    Mode determines which performance statistics are to be drawn:
        1 - PRIMITIVES
        2 - TEXTURE UNITS
        3 - PIPELINE

   RETURNED VALUE: -

   SIDE EFFECTS: 
    The frame buffer is updated.
   ------------------------------------------------------------------*/
void khrn_perform_drawStats(t_VPUInfo*,uint8_t);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void khrn_perform_addPRIMStats(uint32_t, uint32_t, uint32_t, uint32_t);

   DESCRIPTION: 
    Submits performance counter values of PRIMITIVES.

   ARGUMENT(S): 
    uint32_t - FOVCULL counter value
    uint32_t - FOVCLIP counter value
    uint32_t - REVCULL counter value
    uint32_t - NOFEPIX counter value

   RETURNED VALUE: -

   SIDE EFFECTS: 
    Values are stored in global data structures defined in visualStats.c.
   ------------------------------------------------------------------*/
void khrn_perform_addPRIMStats(uint32_t,uint32_t,uint32_t,uint32_t);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void khrn_perform_addTXTStats(uint32_t, uint32_t, uint32_t, uint32_t);

   DESCRIPTION: 
    Submits performance counter values of TEXTURE UNITS.

   ARGUMENT(S): 
    uint32_t - TU0_CACHE_MISSES counter value
    uint32_t - TU0_CACHE_ACCESSES counter value
    uint32_t - TU1_CACHE_MISSES counter value
    uint32_t - TU1_CACHE_ACCESSES counter value

   RETURNED VALUE: -

   SIDE EFFECTS: 
    Values are stored in global data structures defined in visualStats.c.
   ------------------------------------------------------------------*/
void khrn_perform_addTXTStats(uint32_t, uint32_t, uint32_t, uint32_t);


/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void khrn_perform_addPBEStats(uint32_t, uint32_t, uint32_t);

   DESCRIPTION: 
    Submits performance counter values of PIPELINE BACKEND.

   ARGUMENT(S): 
    uint32_t - DEPTH_TEST_FAIL counter value
    uint32_t - STCL_TEST_FAIL counter value
    uint32_t - DEPTH_STCL_PASS counter value

   RETURNED VALUE: -

   SIDE EFFECTS: 
    Values are stored in global data structures defined in visualStats.c.
   ------------------------------------------------------------------*/
void khrn_perform_addPBEStats(uint32_t, uint32_t, uint32_t);





