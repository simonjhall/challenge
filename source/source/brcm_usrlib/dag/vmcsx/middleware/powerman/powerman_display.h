/*=============================================================================
Copyright (c) 2005 Broadcom Europe Limited. All rights reserved.

Project  :  Powermanager
Module   :  Interface to the power manager display file
File     :  $RCSfile: powerman_display.h,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION:
Contains the protypes for the power manager display functions.
=============================================================================*/

#ifndef POWERMAN_DISPLAY_H
#define POWERMAN_DISPLAY_H

#include "vcinclude/common.h"

/******************************************************************************
 Global Types
 *****************************************************************************/
typedef struct POWERMAN_DISPLAY_S * POWERMAN_DISPLAY_HANDLE_T;

//metrics
typedef enum
{
   POWERMAN_DISPLAY_METRIC_MIN,

   POWERMAN_DISPLAY_METRIC_MHZ = 1,
   POWERMAN_DISPLAY_METRIC_PERCENT,

   POWERMAN_DISPLAY_METRIC_MAX

} POWERMAN_DISPLAY_METRIC_T;

//macro to check the range of the metric
#define POWERMAN_DISPLAY_METRIC_CHECK( metric ) (( (metric) > POWERMAN_DISPLAY_METRIC_MIN) && ( (metric) < POWERMAN_DISPLAY_METRIC_MAX) )


//value types
typedef enum
{
   POWERMAN_DISPLAY_TYPE_MIN,

   POWERMAN_DISPLAY_TYPE_USED = 1,
   POWERMAN_DISPLAY_TYPE_FREE,
   POWERMAN_DISPLAY_TYPE_CORE,

   POWERMAN_DISPLAY_TYPE_MAX

} POWERMAN_DISPLAY_TYPE_T;

//macro to check the range of the metric
#define POWERMAN_DISPLAY_TYPE_CHECK( type ) (( (type) > POWERMAN_DISPLAY_TYPE_MIN) && ( (type) < POWERMAN_DISPLAY_TYPE_MAX) )

/******************************************************************************
 Global Funcs
 *****************************************************************************/

/* Routine to create a powerman display */
extern int powerman_display_create( POWERMAN_DISPLAY_HANDLE_T *powerman_display_handle );

/* Routine to delete a powerman display */
extern int powerman_display_delete( const POWERMAN_DISPLAY_HANDLE_T powerman_display_handle );

/* Routine to set the display region for the powerman stats */
extern int powerman_display_set_region(const POWERMAN_DISPLAY_HANDLE_T powerman_display_handle,
                                       const uint8_t display_number,
                                       const uint16_t x,
                                       const uint16_t y,
                                       const uint16_t width,
                                       const uint16_t height,
                                       const POWERMAN_DISPLAY_METRIC_T metric,
                                       const POWERMAN_DISPLAY_TYPE_T type );

/* routine to set the overrun stat */
int powerman_display_set_overrun_stat( const POWERMAN_DISPLAY_HANDLE_T powerman_display_handle,
                                       const uint32_t overrun_stat );

/* Routine to update the stats in the display */
extern int powerman_display_update_stats( const POWERMAN_DISPLAY_HANDLE_T powerman_display_handle,
                                          const uint32_t core_mhz, const uint32_t free );

#endif /* POWERMAN_DISPLAY_H */

/****************************** End of file **********************************/
