/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
Copyright (c) 2002 Alphamosaic Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  Host Request Service (host-side)
File     :  $RCSfile: vcgencmd.h,v $
Revision :  $Revision: #3 $

FILE DESCRIPTION
General command service API.
=============================================================================*/

#ifndef GENCMD_H
#define GENCMD_H

#include "interface/vmcs_host/vchi_local/vchost_config.h"
#include "interface/vchi/vchi.h"

VCHPRE_ void VCHPOST_ vc_vchi_gencmd_init(VCHI_INSTANCE_T initialise_instance, VCHI_CONNECTION_T **connections, uint32_t num_connections );


/* Initialise general command service. Returns it's interface number. This initialises
   the host side of the interface, it does not send anything to VideoCore. */

VCHPRE_ int VCHPOST_ vc_gencmd_init(void);

/* Stop the service from being used. */

VCHPRE_ void VCHPOST_ vc_gencmd_stop(void);

/* Return the service number (-1 if not running). */
VCHPRE_ int VCHPOST_ vc_gencmd_inum(void);

/******************************************************************************
Send commands to VideoCore.
These all return 0 for success. They return VC_MSGFIFO_FIFO_FULL if there is
insufficient space for the whole message in the fifo, and none of the message is
sent.
******************************************************************************/

/*  send command to general command serivce */
VCHPRE_ int VCHPOST_ vc_gencmd_send( const char *format, ... );

/*  get resonse from general command serivce */
VCHPRE_ int VCHPOST_ vc_gencmd_read_response(char *response, int maxlen);

/* convenience function to send command and receive the response */
VCHPRE_ int VCHPOST_ vc_gencmd(char *response, int maxlen, const char *format, ...);

/* read part of a response from the general command service */
VCHPRE_ int VCHPOST_ vc_gencmd_read_response_partial(char *response, int nbytes);

/* if reading with vc_gencmd_read_response_partial end response reads with this */
VCHPRE_ int VCHPOST_ vc_gencmd_close_response_partial(void);

/* get state of reading of response */
VCHPRE_ int VCHPOST_ vc_gencmd_read_partial_state(void);

/******************************************************************************
Utilities to help interpret the responses.
******************************************************************************/

/* Read the value of a property=value type pair from a string (typically VideoCore's
   response to a general command). Return non-zero if found. */
VCHPRE_ int VCHPOST_ vc_gencmd_string_property(char *text, const char *property, char **value, int *length);

/* Read the numeric value of a property=number field from a response string. Return
   non-zero if found. */
VCHPRE_ int VCHPOST_ vc_gencmd_number_property(char *text, const char *property, int *number);

/* Send a command until the desired response is received, the error message is detected, or the timeout */
VCHPRE_ int VCHPOST_ vc_gencmd_until( char        *cmd,
                                      const char  *property,
                                      char        *value,
                                      const char  *error_string,
                                      int         timeout);

#endif
