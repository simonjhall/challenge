/*=============================================================================
Copyright (c) 2007 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  OpenMAX IL Component Service (host-side)
File     :  $RCSfile: vcilcs_intern.h,v $
Revision :  $Revision: 1.1.2.1 $

FILE DESCRIPTION
OpenMAX IL Component Service API
Internal functions for the host side IL component service
=============================================================================*/

#ifndef VCILCS_INTERN_H
#define VCILCS_INTERN_H

// used by host functions that implement the IL component API
// to execute a marshalled function on a VideoCore component
VCHPRE_ int VCHPOST_ vc_ilcs_execute_function(IL_FUNCTION_T func, void *data, int len, void *data2, int len2,
                                              void *bulk, int bulk_len, void *resp);
VCHPRE_ OMX_ERRORTYPE VCHPOST_ vc_ilcs_pass_buffer(IL_FUNCTION_T func, void *reference, OMX_BUFFERHEADERTYPE *pBuffer);
VCHPRE_ OMX_BUFFERHEADERTYPE * VCHPOST_ vc_ilcs_receive_buffer(void *call, int clen, OMX_COMPONENTTYPE **pComp);

// functions that implement incoming functions calls
// from VideoCore components to host based components
VCHPRE_ void VCHPOST_ vcil_in_get_state(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_get_parameter(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_set_parameter(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_get_config(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_set_config(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_use_buffer(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_free_buffer(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_empty_this_buffer(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_fill_this_buffer(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_get_component_version(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_get_extension_index(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_in_component_role_enum(void *call, int clen, void *resp, int *rlen);

// functions that implement callbacks from VideoCore
// components to the host core.
// The prefix is vcil_out since they implement part
// of the API that the host uses out to VideoCore
VCHPRE_ void VCHPOST_ vcil_out_event_handler(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_out_empty_buffer_done(void *call, int clen, void *resp, int *rlen);
VCHPRE_ void VCHPOST_ vcil_out_fill_buffer_done(void *call, int clen, void *resp, int *rlen);

#endif // VCILCS_INTERN_H
