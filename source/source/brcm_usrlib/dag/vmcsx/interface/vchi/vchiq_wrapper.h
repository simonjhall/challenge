/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited. All rights reserved.

Project  : VMCS
Module   : VCHI

FILE DESCRIPTION:

A wrapper designed to allow some clients that use the VCHIQ API to run
over a system that uses VCHI and has no support for VCHIQ.

=============================================================================*/

#ifndef VCGIQ_WRAPPER_H
#define VCHIQ_WRAPPER_H

extern int vchiq_wrapper_add_service(VCHIQ_STATE_T *state, void **connection, int fourcc, VCHIQ_CALLBACK_T callback, void *userdata);

#endif
