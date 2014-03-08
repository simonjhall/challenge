/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
/* visualStats.c*/
#include "middleware/khronos/perform/khrn_visual_stats.h"

/*-------ADJUSTABLE CONSTANTS-------*/

/*-----FORMATTING-----*/

#define MAXTIME 50                   // number of most recent counter values stored and maximum x-axis value in the line performance graphs
#define X_INC 10                     // x-axis units between consecutive labels (e.g. 10 means every 10 x-units there is a label on x-axis) 

/* PERFORMANCE OF PRIMITIVES */
#define PRIM_Y_MAX 500               // maximum y-axis value
#define PRIM_Y_INC 50                // y-axis units between consecutive labels (e.g. 50 means every 50 y-units there is a label on y-axis) 

/* PERFORMANCE OF TEXTURE UNITS */
#define TXT_Y_MAX 500                // maximum y-axis value          
#define TXT_Y_INC 50                 // y-axis units between consecutive labels (e.g. 50 means every 50 y-units there is a label on y-axis)                    

/* PERFORMANCE OF PIPELINE */
#define PBE_Y_MAX 500                // maximum y-axis value                   
#define PBE_Y_INC 50                 // y-axis units between consecutive labels (e.g. 50 means every 50 y-units there is a label on y-axis)   

/* DATA FILTERING */
#define FOVCULL_INC 50               // maximum difference between current and previous value (limits maximum change in counter value)  
#define FOVCLIP_INC 50               // maximum difference between current and previous value (limits maximum change in counter value)  
#define REVCULL_INC 50               // maximum difference between current and previous value (limits maximum change in counter value)  
#define NOFEPIX_INC 50               // maximum difference between current and previous value (limits maximum change in counter value)  

#define TXT_CACHE_MISS_INC 50        // maximum difference between current and previous value (limits maximum change in counter value)
#define TXT_CACHE_ACCESS_INC 50      // maximum difference between current and previous value (limits maximum change in counter value)

#define DEPTH_PASS_INC 50            // maximum difference between current and previous value (limits maximum change in counter value)  
#define DEPTH_FAIL_INC 50            // maximum difference between current and previous value (limits maximum change in counter value)  
#define STCL_FAIL_INC 50             // maximum difference between current and previous value (limits maximum change in counter value)  


/*-----COORDINATES-----*/

#define VPUI_X 500                   // VPU I PIE CHART x xoordinate (top-left)
#define VPUI_Y 28                    // VPU I PIE CHART y xoordinate (top-left)
#define VPUI_HEIGHT 208              // VPU I PIE CHART height
#define VPUI_WIDTH 282               // VPU I PIE CHART width

#define VPUII_X 500                  // VPU II PIE CHART x xoordinate (top-left)
#define VPUII_Y 240                  // VPU II PIE CHART y xoordinate (top-left)
#define VPUII_HEIGHT 208             // VPU II PIE CHART height 
#define VPUII_WIDTH 282              // VPU II PIE CHART width  

#define FULL_SCR_X 16                // x coordinate of the top-left corner for BAR & LINE graphs
#define FULL_SCR_Y 28                // y coordinate of the top-left corner for BAR & LINE graphs
#define FULL_SCR_HEIGHT 420          // height of BAR & LINE graphs

#define FULL_SCR_WIDTH 768           // width of LINE graphs (w/o VPU graphs)
#define NFULL_SCR_WIDTH 480          // width of BAR graphs (with VPU graphs)

/*-----TITLES & LABELS-----*/

//VPU graphs
#define VPUI_TITLE "VPU Core I"
#define VPUII_TITLE "VPU Core II"

//PRIM graphs
#define PRIM_GRAPH_TITLE "Primitives: RT performance"
#define PRIM_X_TITLE "Last 50 Time Units"
#define PRIM_Y_TITLE "Count"

#define PRIM_AV_GRAPH_TITLE "Primitives: Average Performance"
#define PRIM_AV_X_TITLE "\0"
#define PRIM_AV_Y_TITLE "Count"

#define FOVCULL "FOVCULL"
#define FOVCLIP "FOVCLIP"
#define REVCULL "REVCULL"
#define NOFEPIX "NOFEPIX"

//TXTR graphs
#define TXT_GRAPH_TITLE "Texture Units: RT Performance"
#define TXT_X_TITLE "Last 50 Time Units"
#define TXT_Y_TITLE "Count"

#define TU0_CACHE_ACCESSES "TU0_CACHE_ACCESSES"
#define TU0_CACHE_MISSES  "TU0_CACHE_MISSES"
#define TU1_CACHE_ACCESSES "TU1_CACHE_ACCESSES"
#define TU1_CACHE_MISSES  "TU1_CACHE_MISSES"

#define TXT_AV_GRAPH_TITLE "Texture Units: Average Performance"
#define TXT_AV_X_TITLE "\0"
#define TXT_AV_Y_TITLE "Count"

#define TU_0 "TU0"
#define TU_1 "TU1"
#define CACHE_ACCESSES "CACHE_ACCESSES"
#define CACHE_MISSES  "CACHE_MISSES"

//PBE graphs
#define PBE_GRAPH_TITLE "Pipeline: RT Performance"
#define PBE_X_TITLE "Last 50 Time Units"
#define PBE_Y_TITLE "Count"

#define PBE_AV_GRAPH_TITLE "Pipeline: Average Performance"
#define PBE_AV_X_TITLE "\0"
#define PBE_AV_Y_TITLE "Count"

#define DEPTH_TEST_FAIL  "DEPTH_TEST_FAIL"
#define STCL_TEST_FAIL  "STCL_TEST_FAIL"
#define DPTH_STCL_PASS  "DPTH_STCL_PASS"


/* COLORS CAN BE ADJUSTED */
t_Color PIEColors[]= {0xff,0xf0f,0xf8f,0xf50f,0xa0ff,0xddef,0xff8f,0x9b5f,0xff8f,0xcd0f,0xfa5f,0x0aff,0xf05f,0xfaff,0x0fff};
t_Color BARColors[] = {0xff,0xf0f,0xf8f,0xf50f,0xa0ff,0xddef,0xff8f,0x9b5f,0xff8f,0xcd0f,0xfa5f,0x0aff,0xf05f,0xfaff,0x0fff};
t_Color LINEColors[]= {0xff,0xf0f,0xf8f,0xf50f,0xa0ff,0xddef,0xff8f,0x9b5f,0xff8f,0xcd0f,0xfa5f,0x0aff,0xf05f,0xfaff,0x0fff};


//-----------------------------------------------------------------------------------------------------------//

/* FUNCTION DECLARATIONS */

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void VPUtilStats(t_VPUInfo*);

   DESCRIPTION: 
    Draws VPU utilization pie charts.

   ARGUMENT(S): 
    t_VPUInfo* - VPU utilization data

   RETURNED VALUE: -

   SIDE EFFECTS: 
    Two VPU pie chrats are drawn into frame buffer.
   ------------------------------------------------------------------*/
void VPUtilStats(t_VPUInfo*);

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawAVPRIMStats();

   DESCRIPTION: 
    Draws a bar graph showing average performance of primitives.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A bar graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawAVPRIMStats();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawTXTStats();

   DESCRIPTION: 
    Draws a line graph showing real-time performance of texture units.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A line graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawTXTStats();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawPRIMStats();

   DESCRIPTION: 
    Draws a line graph showing real-time performance of primitives.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A line graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawPRIMStats();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawAVTXTStats();

   DESCRIPTION: 
    Draws a bar graph showing average performance of texture units.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A bar graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawAVTXTStats();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawAVPBEStats();

   DESCRIPTION: 
    Draws a bar graph showing average performance of pipeline.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A bar graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawAVPBEStats();

/* ------------------------------------------------------------------
   FUNCTION DECLARATION: 
    void drawPBEStats();

   DESCRIPTION: 
    Draws a line graph showing real-time performance of pipeline.

   ARGUMENT(S): -

   RETURNED VALUE: -

   SIDE EFFECTS: 
    A line graph is drawn into frame buffer.
   ------------------------------------------------------------------*/
void drawPBEStats();


//-----------------------------------------------------------------------------------------------------------//

/* GLOBAL VARIABLES AND DATA STRUCTURES */

t_GraphSettings gs;                                       // graph settings common to all graphs

/* PRIMITIVES */

t_LineDetails PRIMStats;                                  // line graph details
t_Curve PRIMcurves[4];                                    // 4 curves
t_DataPoint PRIMpoints[4][2*MAXTIME];                     // structure where submitted counter values are stored 
uint32_t PRIMsum[4];                                      // structure for keeping total sums for calculating average values

t_BarDetails PRIMStatsAV;                                 // details required for a bar graph 
t_BarSet PRIMbs;                                          // 1 set
t_BarData PRIMbd[4];                                      // 4 bars
char *PRIMbt[4] = {FOVCULL, FOVCLIP,REVCULL,NOFEPIX};     // x-axis labels


/*VPUs*/
t_PieDetails vpuUtil1;                                    // vpu pie chart details
t_PieDetails vpuUtil2;


/*TEXTURES*/

t_LineDetails TXTStats;                                   // line graph details
t_Curve TXTcurves[4];                                     // 4 curves   
uint32_t TXTsum[4];                                       // structure for keeping total sums for calculating average values
t_DataPoint TXTpoints[4][2*MAXTIME];                      // structure where submitted counter values are stored 

t_BarDetails TXTStatsAV;                                  // details required for a bar graph 
t_BarSet TXTbs[2];                                        // 2 sets: for cache misses & accesses
t_BarData TXTbd[2][2];                                    // 2 bars in each of 2 sets
char *TXTbt[2] = {TU_0, TU_1};                            // x-axis labels


/*PIPELINE*/

t_LineDetails PBEStats;                                   // line graph details
t_Curve PBEcurves[3];                                     // 3 curves   
uint32_t PBEsum[3];                                       // structure for keeping total sums for calculating average values
t_DataPoint PBEpoints[3][2*MAXTIME];                      // structure where submitted counter values are stored 

t_BarDetails PBEStatsAV;                                  // details required for a bar graph 
t_BarSet PBEbs;                                           // 1 set
t_BarData PBEbd[3];                                       // 3 bars
char *PBEbt[3] = {DEPTH_TEST_FAIL, STCL_TEST_FAIL,DPTH_STCL_PASS}; // x-axis labels


//-----------------------------------------------------------------------------------------------------------//


void khrn_perform_init()
{
 defaultSettings(&gs);               //retrieving default settings

 //VPUs

 vpuUtil1.size.height = VPUI_HEIGHT; 
 vpuUtil1.size.width = VPUI_WIDTH;
 vpuUtil1.coordinate.x = VPUI_X;
 vpuUtil1.coordinate.y = VPUI_Y;
 vpuUtil1.graphTitle = VPUI_TITLE;
 vpuUtil1.gs = &gs;

 vpuUtil2.size.height = VPUII_HEIGHT; 
 vpuUtil2.size.width = VPUII_WIDTH;
 vpuUtil2.coordinate.x = VPUII_X;
 vpuUtil2.coordinate.y = VPUII_Y;
 vpuUtil2.graphTitle = VPUII_TITLE;
 vpuUtil2.gs = &gs;

//PRIM

 PRIMStats.coordinate.x = FULL_SCR_X; 
 PRIMStats.coordinate.y = FULL_SCR_Y;
 PRIMStats.size.height = FULL_SCR_HEIGHT; 
 PRIMStats.nrOfCurves = 4;
 PRIMStats.gs = &gs;

 PRIMcurves[0].itemName = PRIMbt[0];
 PRIMcurves[0].nrOfPoints = 0; 
 PRIMcurves[0].points =PRIMpoints[0];

 PRIMcurves[1].itemName = PRIMbt[1];
 PRIMcurves[1].nrOfPoints = 0;
 PRIMcurves[1].points =PRIMpoints[1];

 PRIMcurves[2].itemName = PRIMbt[2];
 PRIMcurves[2].nrOfPoints = 0;
 PRIMcurves[2].points =PRIMpoints[2];

 PRIMcurves[3].itemName = PRIMbt[3];
 PRIMcurves[3].nrOfPoints = 0;
 PRIMcurves[3].points =PRIMpoints[3];

 PRIMStats.curves =PRIMcurves;
 PRIMStats.graphTitle = PRIM_GRAPH_TITLE ;
 PRIMStats.x_title = PRIM_X_TITLE;
 PRIMStats.y_title = PRIM_Y_TITLE;
 PRIMStats.autoscaleX = 0;
 PRIMStats.xScale.max = MAXTIME;
 PRIMStats.xScale.incSteps = X_INC;
 PRIMStats.autoscaleY = 0;
 PRIMStats.yScale.max = PRIM_Y_MAX;
 PRIMStats.yScale.incSteps = PRIM_Y_INC;
 PRIMsum[0] = 0;
 PRIMsum[1] = 0;
 PRIMsum[2] = 0;
 PRIMsum[3] = 0;

//PRIM_AV

 PRIMStatsAV.coordinate.x = FULL_SCR_X;  PRIMStatsAV.coordinate.y = FULL_SCR_Y;
 PRIMStatsAV.size.height = FULL_SCR_HEIGHT;
 PRIMStatsAV.graphTitle = PRIM_AV_GRAPH_TITLE;
 PRIMStatsAV.x_title = PRIM_AV_X_TITLE;
 PRIMStatsAV.y_title = PRIM_AV_Y_TITLE;
 PRIMStatsAV.nrOfBarsInSet = 4;
 PRIMStatsAV.autoscaleY = 0;
 PRIMStatsAV.yScale.max = PRIM_Y_MAX;
 PRIMStatsAV.yScale.incSteps = PRIM_Y_INC;
 PRIMStatsAV.xbarTitles = PRIMbt;
 PRIMStatsAV.nrOfSets = 1;
 PRIMStatsAV.barSets = &PRIMbs;
 PRIMbs.bars = PRIMbd;
 PRIMStatsAV.gs = &gs;
 
 //TXT

 TXTStats.coordinate.x = FULL_SCR_X; 
 TXTStats.coordinate.y = FULL_SCR_Y;
 TXTStats.size.height = FULL_SCR_HEIGHT; 
 TXTStats.nrOfCurves = 4;
 TXTStats.gs = &gs;
 


 TXTcurves[0].itemName = TU0_CACHE_MISSES;
 TXTcurves[0].nrOfPoints = 0;
 TXTcurves[0].points = TXTpoints[0];

 TXTcurves[1].itemName = TU0_CACHE_ACCESSES;
 TXTcurves[1].nrOfPoints = 0;
 TXTcurves[1].points = TXTpoints[1];

 TXTcurves[2].itemName = TU1_CACHE_MISSES;
 TXTcurves[2].nrOfPoints = 0;
 TXTcurves[2].points = TXTpoints[2];

 TXTcurves[3].itemName = TU1_CACHE_ACCESSES;
 TXTcurves[3].nrOfPoints = 0;
 TXTcurves[3].points = TXTpoints[3];

 TXTStats.curves =TXTcurves;
 TXTStats.graphTitle = TXT_GRAPH_TITLE;
 TXTStats.x_title = TXT_X_TITLE;
 TXTStats.y_title = TXT_Y_TITLE;
 TXTStats.autoscaleX = 0;
 TXTStats.xScale.max = MAXTIME;
 TXTStats.xScale.incSteps = X_INC;
 TXTStats.autoscaleY = 0;
 TXTStats.yScale.max = TXT_Y_MAX;
 TXTStats.yScale.incSteps = TXT_Y_INC;
 TXTsum[0] = 0;
 TXTsum[1] = 0;
 TXTsum[2] = 0;
 TXTsum[3] = 0;

 //TXT_AV

 TXTStatsAV.coordinate.x = FULL_SCR_X;  TXTStatsAV.coordinate.y = FULL_SCR_Y;
 TXTStatsAV.gs=&gs;
 TXTStatsAV.size.height = FULL_SCR_HEIGHT;
 TXTStatsAV.graphTitle = TXT_AV_GRAPH_TITLE;
 TXTStatsAV.x_title = TXT_AV_X_TITLE;
 TXTStatsAV.y_title = TXT_AV_Y_TITLE;
 TXTStatsAV.nrOfBarsInSet = 2;
 TXTStatsAV.autoscaleY = 0;
 TXTStatsAV.yScale.max = TXT_Y_MAX;
 TXTStatsAV.yScale.incSteps = TXT_Y_INC;
 TXTStatsAV.xbarTitles = TXTbt;
 TXTStatsAV.nrOfSets = 2;
 TXTStatsAV.barSets = TXTbs;
 TXTbs[0].bars = TXTbd[0];
 TXTbs[0].itemName = CACHE_MISSES;
 TXTbs[1].bars = TXTbd[1];
 TXTbs[1].itemName = CACHE_ACCESSES;
 
 //PBE

 PBEStats.coordinate.x = FULL_SCR_X; 
 PBEStats.coordinate.y = FULL_SCR_Y;
 PBEStats.size.height = FULL_SCR_HEIGHT; 
 PBEStats.nrOfCurves = 3;
 PBEStats.gs=&gs;

 PBEcurves[0].itemName = PBEbt[0];
 PBEcurves[0].nrOfPoints = 0;
 PBEcurves[0].points = PBEpoints[0];

 PBEcurves[1].itemName = PBEbt[1];
 PBEcurves[1].nrOfPoints = 0;
 PBEcurves[1].points = PBEpoints[1];

 PBEcurves[2].itemName = PBEbt[2];
 PBEcurves[2].nrOfPoints = 0;
 PBEcurves[2].points = PBEpoints[2];

 PBEStats.curves =PBEcurves;
 PBEStats.graphTitle = PBE_GRAPH_TITLE;
 PBEStats.x_title = PBE_X_TITLE;
 PBEStats.y_title = PBE_Y_TITLE;
 PBEStats.autoscaleX = 0;
 PBEStats.xScale.max = MAXTIME;
 PBEStats.xScale.incSteps = X_INC;
 PBEStats.autoscaleY = 0;
 PBEStats.yScale.max = PBE_Y_MAX;
 PBEStats.yScale.incSteps = PBE_Y_INC;
 PBEsum[0] = 0;
 PBEsum[1] = 0;
 PBEsum[2] = 0;

 //PBE_AV

 PBEStatsAV.coordinate.x = FULL_SCR_X;  PBEStatsAV.coordinate.y = FULL_SCR_Y;
 PBEStatsAV.gs=&gs;
 PBEStatsAV.size.height = FULL_SCR_HEIGHT;
 PBEStatsAV.graphTitle =  PBE_AV_GRAPH_TITLE;
 PBEStatsAV.x_title = PBE_AV_X_TITLE;
 PBEStatsAV.y_title = PBE_AV_Y_TITLE;
 PBEStatsAV.nrOfBarsInSet = 3;
 PBEStatsAV.autoscaleY = 0;
 PBEStatsAV.yScale.max = PBE_Y_MAX;
 PBEStatsAV.yScale.incSteps = PBE_Y_INC;
 PBEStatsAV.xbarTitles = PBEbt;
 PBEStatsAV.nrOfSets = 1;
 PBEStatsAV.barSets = &PBEbs;
 PBEbs.bars = PBEbd;

}




 void khrn_perform_drawStats(t_VPUInfo *vpuInf,uint8_t mode){

   if (vpuInf==0){

    switch(mode){
     case 1: PRIMStats.size.width = FULL_SCR_WIDTH ; drawPRIMStats(); break;
     case 2: TXTStats.size.width = FULL_SCR_WIDTH ; drawTXTStats(); break;
     case 3: PBEStats.size.width = FULL_SCR_WIDTH ; drawPBEStats(); break;
    }
   }
     
   else {

    switch(mode){
     case 1: PRIMStatsAV.size.width = NFULL_SCR_WIDTH; drawAVPRIMStats(); break;
     case 2: TXTStatsAV.size.width = NFULL_SCR_WIDTH; drawAVTXTStats(); break;
     case 3: PBEStatsAV.size.width = NFULL_SCR_WIDTH; drawAVPBEStats(); break;

    }
    VPUtilStats(vpuInf);
   }
}



void khrn_perform_addPRIMStats(uint32_t fovCull, uint32_t fovClip, uint32_t revCull, uint32_t nOfEPix){
 static uint32_t Time = 0;
 static uint32_t offset = 0;
 static uint8_t full =0;
 uint32_t tmp;
 uint32_t prevValue;

 if (Time==MAXTIME) {
  full = 1;
  Time=0;
 }

 if (full){
  
   if(++offset==MAXTIME) offset=0;

   PRIMsum[0]-=PRIMcurves[0].points[0].value;
   PRIMsum[1]-=PRIMcurves[1].points[0].value;
   PRIMsum[2]-=PRIMcurves[2].points[0].value;
   PRIMsum[3]-=PRIMcurves[3].points[0].value;

   PRIMcurves[0].points = &PRIMpoints[0][offset];
   PRIMcurves[1].points = &PRIMpoints[1][offset];
   PRIMcurves[2].points = &PRIMpoints[2][offset];
   PRIMcurves[3].points = &PRIMpoints[3][offset];

 }
 
 else{

  PRIMcurves[0].nrOfPoints++;
  PRIMcurves[1].nrOfPoints++;
  PRIMcurves[2].nrOfPoints++;
  PRIMcurves[3].nrOfPoints++;
 }

 if (Time>0) prevValue = PRIMpoints[0][Time-1].value;
 else
    if (full) prevValue = PRIMpoints[0][MAXTIME-1].value;
    else prevValue=0;

 if (fovCull>prevValue){
   tmp = fovCull - prevValue;
   if (tmp > FOVCULL_INC) 
      if (prevValue + FOVCULL_INC <= PRIM_Y_MAX) fovCull = prevValue + FOVCULL_INC;
      else fovCull = PRIM_Y_MAX;
 }
 else {
     
      if (prevValue > FOVCULL_INC) fovCull = prevValue - FOVCULL_INC;
      else fovCull = 0;

 }
  
 

  PRIMsum[0]+=fovCull;
  PRIMpoints[0][Time].value = fovCull;
  PRIMpoints[0][MAXTIME+Time].value = fovCull;


 if (Time>0) prevValue = PRIMpoints[1][Time-1].value;
 else
    if (full) prevValue = PRIMpoints[1][MAXTIME-1].value;
    else prevValue=0;

 if (fovClip>prevValue){
   tmp = fovClip - prevValue;
   if (tmp > FOVCLIP_INC) 
      if (prevValue + FOVCLIP_INC <= PRIM_Y_MAX) fovClip = prevValue + FOVCLIP_INC;
      else fovClip = PRIM_Y_MAX;
 }
 else {
     
      if (prevValue > FOVCLIP_INC) fovClip = prevValue - FOVCLIP_INC;
      else fovClip = 0;
 
 
 }

  PRIMsum[1]+=fovClip;
  PRIMpoints[1][Time].value = fovClip;
  PRIMpoints[1][MAXTIME+Time].value = fovClip;

   if (Time>0) prevValue = PRIMpoints[2][Time-1].value;
 else
    if (full) prevValue = PRIMpoints[2][MAXTIME-1].value;
    else prevValue=0;

 if (revCull>prevValue){
   tmp = revCull - prevValue;
   if (tmp > REVCULL_INC) 
      if (prevValue + REVCULL_INC <= PRIM_Y_MAX) revCull = prevValue + REVCULL_INC;
      else revCull = PRIM_Y_MAX;
 }
 else {
     
      if (prevValue > REVCULL_INC) revCull = prevValue - REVCULL_INC;
      else revCull = 0;
 
 
 }


  PRIMsum[2]+=revCull;
  PRIMpoints[2][Time].value = revCull;
  PRIMpoints[2][MAXTIME+Time].value = revCull;


  if (Time>0) prevValue = PRIMpoints[3][Time-1].value;
   else
    if (full) prevValue = PRIMpoints[3][MAXTIME-1].value;
    else prevValue=0;

  if (nOfEPix>prevValue){
    tmp = nOfEPix - prevValue;
    if (tmp > NOFEPIX_INC) 
      if (prevValue + NOFEPIX_INC <= PRIM_Y_MAX) nOfEPix = prevValue + NOFEPIX_INC;
      else nOfEPix = PRIM_Y_MAX;
   }
  else {
     
      if (prevValue > NOFEPIX_INC) nOfEPix = prevValue - NOFEPIX_INC;
      else nOfEPix = 0;
 
 
 }

  PRIMsum[3]+=nOfEPix;
  PRIMpoints[3][Time].value= nOfEPix;
  PRIMpoints[3][MAXTIME+Time].value= nOfEPix;
  
  Time++;

}


void khrn_perform_addTXTStats(uint32_t tU0CacheMisses, uint32_t tU0CacheAccesses, uint32_t tU1CacheMisses, uint32_t tU1CacheAccesses){

 static uint32_t Time = 0;
 static uint32_t offset = 0;
 static uint8_t full =0;
 uint32_t tmp;
 uint32_t prevValue;

 if (Time==MAXTIME) {
  full = 1;
  Time=0;
 }

 if (full){
  
   if(++offset==MAXTIME) offset=0;

   TXTsum[0]-=TXTcurves[0].points[0].value;
   TXTsum[1]-=TXTcurves[1].points[0].value;
   TXTsum[2]-=TXTcurves[2].points[0].value;
   TXTsum[3]-=TXTcurves[3].points[0].value;

   TXTcurves[0].points = &TXTpoints[0][offset];
   TXTcurves[1].points = &TXTpoints[1][offset];
   TXTcurves[2].points = &TXTpoints[2][offset];
   TXTcurves[3].points = &TXTpoints[3][offset];
  
 }
 
 else{
  TXTcurves[0].nrOfPoints++;
  TXTcurves[1].nrOfPoints++;
  TXTcurves[2].nrOfPoints++;
  TXTcurves[3].nrOfPoints++;
 }

 if (Time>0) prevValue = TXTpoints[0][Time-1].value;
 else
    if (full) prevValue = TXTpoints[0][MAXTIME-1].value;
    else prevValue=0;

 if (tU0CacheMisses>prevValue){
   tmp = tU0CacheMisses - prevValue;
   if (tmp > TXT_CACHE_MISS_INC) 
      if (prevValue + TXT_CACHE_MISS_INC <= TXT_Y_MAX) tU0CacheMisses = prevValue + TXT_CACHE_MISS_INC;
      else tU0CacheMisses = TXT_Y_MAX;
 }
 else {
     
      if (prevValue > TXT_CACHE_MISS_INC) tU0CacheMisses = prevValue - TXT_CACHE_MISS_INC;
      else tU0CacheMisses = 0;
 
 
 }
  

 
  TXTsum[0]+=tU0CacheMisses;
  TXTpoints[0][Time].value = tU0CacheMisses;
  TXTpoints[0][MAXTIME+Time].value = tU0CacheMisses;

  if (Time>0) prevValue = TXTpoints[1][Time-1].value;
  else
    if (full) prevValue = TXTpoints[1][MAXTIME-1].value;
    else prevValue=0;

  if (tU0CacheAccesses>prevValue){
   tmp = tU0CacheAccesses - prevValue;
   if (tmp > TXT_CACHE_ACCESS_INC) 
      if (prevValue + TXT_CACHE_ACCESS_INC <= TXT_Y_MAX) tU0CacheAccesses = prevValue + TXT_CACHE_ACCESS_INC;
      else tU0CacheAccesses = TXT_Y_MAX;
  }
  else {
     
       if (prevValue > TXT_CACHE_ACCESS_INC) tU0CacheAccesses = prevValue - TXT_CACHE_ACCESS_INC;
       else tU0CacheAccesses = 0;
  }

  TXTsum[1]+=tU0CacheAccesses;
  TXTpoints[1][Time].value = tU0CacheAccesses;
  TXTpoints[1][MAXTIME+Time].value = tU0CacheAccesses;

  if (Time>0) prevValue = TXTpoints[2][Time-1].value;
  else
    if (full) prevValue = TXTpoints[2][MAXTIME-1].value;
    else prevValue=0;

  if (tU1CacheMisses>prevValue){
   tmp = tU1CacheMisses - prevValue;
   if (tmp > TXT_CACHE_MISS_INC) 
      if (prevValue + TXT_CACHE_MISS_INC <= TXT_Y_MAX) tU1CacheMisses = prevValue + TXT_CACHE_MISS_INC;
      else tU1CacheMisses = TXT_Y_MAX;
  }
  else {
     
       if (prevValue > TXT_CACHE_MISS_INC) tU1CacheMisses = prevValue - TXT_CACHE_ACCESS_INC;
       else tU1CacheMisses = 0;
  }


  TXTsum[2]+=tU1CacheMisses;
  TXTpoints[2][Time].value = tU1CacheMisses;
  TXTpoints[2][MAXTIME+Time].value = tU1CacheMisses;

 if (Time>0) prevValue = TXTpoints[3][Time-1].value;
  else
    if (full) prevValue = TXTpoints[3][MAXTIME-1].value;
    else prevValue=0;

  if (tU1CacheAccesses>prevValue){
   tmp = tU1CacheAccesses - prevValue;
   if (tmp > TXT_CACHE_ACCESS_INC) 
      if (prevValue + TXT_CACHE_ACCESS_INC <= TXT_Y_MAX) tU1CacheAccesses = prevValue + TXT_CACHE_ACCESS_INC;
      else tU1CacheAccesses = TXT_Y_MAX;
  }
  else {
     
       if (prevValue > TXT_CACHE_ACCESS_INC) tU1CacheAccesses = prevValue - TXT_CACHE_ACCESS_INC;
       else tU1CacheAccesses = 0;
  }

  
  TXTsum[3]+=tU1CacheAccesses;
  TXTpoints[3][Time].value= tU1CacheAccesses;
  TXTpoints[3][MAXTIME+Time].value= tU1CacheAccesses;
  
  Time++;
}

void khrn_perform_addPBEStats(uint32_t depthTestFail, uint32_t stclTestFail, uint32_t depthTestPass){

 static uint32_t Time = 0;
 static uint32_t offset = 0;
 static uint8_t full =0;
 uint32_t tmp;
 uint32_t prevValue;

 if (Time==MAXTIME) {
  full = 1;
  Time=0;
 }

 if (full){
  
   if(++offset==MAXTIME) offset=0;

 
   PBEsum[0]-=PBEcurves[0].points[0].value;
   PBEsum[1]-=PBEcurves[1].points[0].value;
   PBEsum[2]-=PBEcurves[2].points[0].value;
   
   
   PBEcurves[0].points = &PBEpoints[0][offset];
   PBEcurves[1].points = &PBEpoints[1][offset];
   PBEcurves[2].points = &PBEpoints[2][offset];
  
 }
 
 else{
  PBEcurves[0].nrOfPoints++;
  PBEcurves[1].nrOfPoints++;
  PBEcurves[2].nrOfPoints++;
 }

   if (Time>0) prevValue = PBEpoints[0][Time-1].value;
   else
    if (full) prevValue = PBEpoints[0][MAXTIME-1].value;
    else prevValue=0;

   if (depthTestFail>prevValue){
   tmp = depthTestFail - prevValue;
   if (tmp > DEPTH_FAIL_INC) 
      if (prevValue + DEPTH_FAIL_INC <= PBE_Y_MAX) depthTestFail = prevValue + DEPTH_FAIL_INC;
      else depthTestFail = PBE_Y_MAX;
  }
 else {
     
      if (prevValue > DEPTH_FAIL_INC) depthTestFail = prevValue - DEPTH_FAIL_INC;
      else depthTestFail = 0;
 
 
 }
 
  PBEsum[0]+=depthTestFail;
  PBEpoints[0][Time].value = depthTestFail;
  PBEpoints[0][MAXTIME+Time].value = depthTestFail;

   if (Time>0) prevValue = PBEpoints[1][Time-1].value;
   else
    if (full) prevValue = PBEpoints[1][MAXTIME-1].value;
    else prevValue=0;

   if (stclTestFail>prevValue){
   tmp = stclTestFail - prevValue;
   if (tmp > STCL_FAIL_INC) 
      if (prevValue + STCL_FAIL_INC <= PBE_Y_MAX) stclTestFail = prevValue + DEPTH_FAIL_INC;
      else stclTestFail = PBE_Y_MAX;
   }
 else {
     
      if (prevValue > STCL_FAIL_INC) stclTestFail = prevValue - STCL_FAIL_INC;
      else stclTestFail = 0;
 
 
 }

  PBEsum[1]+=stclTestFail;
  PBEpoints[1][Time].value = stclTestFail;
  PBEpoints[1][MAXTIME+Time].value = stclTestFail;

   if (Time>0) prevValue = PBEpoints[2][Time-1].value;
   else
    if (full) prevValue = PBEpoints[2][MAXTIME-1].value;
    else prevValue=0;

   if (depthTestPass>prevValue){
   tmp = depthTestPass - prevValue;
   if (tmp > DEPTH_PASS_INC) 
      if (prevValue + DEPTH_PASS_INC <= PBE_Y_MAX) depthTestPass = prevValue + DEPTH_PASS_INC;
      else depthTestPass = PBE_Y_MAX;
  }
 else {
     
      if (prevValue > STCL_FAIL_INC) depthTestPass = prevValue - STCL_FAIL_INC;
      else depthTestPass = 0;
 
 
 }

  PBEsum[2]+=depthTestPass;
  PBEpoints[2][Time].value = depthTestPass;
  PBEpoints[2][MAXTIME+Time].value = depthTestPass;

   
  Time++;
}




void drawPRIMStats(){

 gs.colors = LINEColors;
 gs.legendEnable=1;
 lineGraph(&PRIMStats);
}


void drawAVPRIMStats(){

 if (PRIMcurves[0].nrOfPoints==0)return;

 gs.colors = BARColors;
 gs.legendEnable=0;
 gs.maxBarWidth = 64;


PRIMbd[0].value = PRIMsum[0]/(float)PRIMcurves[0].nrOfPoints; 
PRIMbd[1].value = PRIMsum[1]/(float)PRIMcurves[1].nrOfPoints; 
PRIMbd[2].value = PRIMsum[2]/(float)PRIMcurves[2].nrOfPoints; 
PRIMbd[3].value = PRIMsum[3]/(float)PRIMcurves[3].nrOfPoints; 

barGraph(&PRIMStatsAV);
}



void drawPBEStats(){

gs.colors = LINEColors;
gs.legendEnable=1;
lineGraph(&PBEStats);
}

void drawAVPBEStats(){

if (PBEcurves[0].nrOfPoints==0)return;

gs.colors = BARColors;
gs.legendEnable=0;
gs.maxBarWidth=80;

PBEbd[0].value = PBEsum[0]/(float)PBEcurves[0].nrOfPoints; 
PBEbd[1].value = PBEsum[1]/(float)PBEcurves[1].nrOfPoints; 
PBEbd[2].value = PBEsum[2]/(float)PBEcurves[2].nrOfPoints; 


barGraph(&PBEStatsAV);
}

void drawTXTStats(){

 gs.colors = LINEColors;
 gs.legendEnable=1;
 lineGraph(&TXTStats);
}

void drawAVTXTStats(){

if (TXTcurves[0].nrOfPoints==0)return;

 gs.colors = BARColors;
 gs.legendEnable=1;
 gs.maxBarWidth = 48;


TXTbd[0][0].value = TXTsum[0]/(float)TXTcurves[0].nrOfPoints; 
TXTbd[0][1].value = TXTsum[2]/(float)TXTcurves[2].nrOfPoints; 
TXTbd[1][0].value = TXTsum[1]/(float)TXTcurves[1].nrOfPoints; 
TXTbd[1][1].value = TXTsum[3]/(float)TXTcurves[3].nrOfPoints; 

 barGraph(&TXTStatsAV);
}


/*VPUs*/
void VPUtilStats(t_VPUInfo *data){

 gs.colors=PIEColors;
 vpuUtil1.nrOfItems = data[0].noOfTasks;
 vpuUtil1.data = ( t_PieData *)data[0].tasks;
 pieChart(&vpuUtil1);

 vpuUtil2.nrOfItems = data[1].noOfTasks;
 vpuUtil2.data = ( t_PieData *)data[1].tasks;
 pieChart(&vpuUtil2);

}
