/*=============================================================================
Copyright (c) 2005 Broadcom Europe Limited. All rights reserved.

Project  :  Powerman
Module   :  Interface to the power manager
File     :  $RCSfile: powerman.h,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION:
Contains the protypes for the power manager functions.
=============================================================================*/

#ifndef POWERMAN_H
#define POWERMAN_H

#include "vcinclude/common.h"
#include "powerman_display.h"

/******************************************************************************
 Global Types
 *****************************************************************************/

typedef struct POWERMAN_USER_INFO_S * POWERMAN_HANDLE_T;

typedef enum
{
   POWERMAN_REQUEST_MIN,

   /* The parameter gives the min clock required */
   POWERMAN_REQUEST_MIN_CLOCK,

   /* Ignore the parameter with the mac clock command */
   POWERMAN_REQUEST_MAX_CLOCK,

   /* The param gives the clock speed to kick the powerman upto */
   POWERMAN_REQUEST_KICK_CLOCK,

   /* The parameter gives the headroom above 'auto' mode */
   POWERMAN_REQUEST_AUTO_CLOCK,

   POWERMAN_REQUEST_MAX

} POWERMAN_REQUEST_T;

//The max number of users that can have a say in the clock management
#define POWERMAN_MAX_NUM_USERS      10

//the max number of extra params allowed on the stats display
#define POWERMAN_MAX_EXTRA_STATS    1

/******************************************************************************
 Global Funcs
 *****************************************************************************/

//Only include the real functions if we want powerman for this platform
#ifdef WANT_POWERMAN

   /* Routine to initialise the power manager */
   extern int powerman_initialise( void );


   /* Routine used to register for a say in the clock freq */
   extern int powerman_register_user( POWERMAN_HANDLE_T *powerman_handle );

   /* Routine used to deregister from the powerman */
   extern int powerman_deregister_user( const POWERMAN_HANDLE_T powerman_handle );

   /* Routine to request the users clock requirements */
   extern int powerman_set_user_request(  const POWERMAN_HANDLE_T powerman_handle,
                                          const POWERMAN_REQUEST_T request,
                                          const uint32_t param );


   /* Routine to get the current status */
   extern int powerman_get_status(  uint32_t *clk_mhz,
                                    uint32_t *free_mhz);

   /* Routine to show the stats */
   extern int powerman_show_stats(  const uint8_t display_number,
                                    const uint16_t x,
                                    const uint16_t y,
                                    const uint16_t width,
                                    const uint16_t height,
                                    const POWERMAN_DISPLAY_METRIC_T metric,
                                    const POWERMAN_DISPLAY_TYPE_T type );

   /* Routine to set the panic stat in the powerman */
   extern int powerman_set_overrun_stat( const uint32_t overrun_stat );

   /* routine to start logging */
   extern int powerman_start_logging( const char *logfilename );

   /* routine to stop logging */
   extern int powerman_stop_logging( void );

#else

   #define powerman_initialise( ) (0)

   #define powerman_register_user( a ) (0)

   #define powerman_deregister_user( a ) (0)

   #define powerman_set_user_request( a, b, c ) (0)

   #define powerman_get_status( a, b ) (0)

   #define powerman_show_stats( a, b, c, d, e ) (0)

   #define powerman_set_overrun_stat( a ) (0)

   #define powerman_start_logging( a ) (0)

   #define powerman_stop_logging( ) (0)

#endif

#endif /* POWERMAN_H */

/****************************** End of file **********************************/
