/*=============================================================================
Copyright (c) 2010 Broadcom Europe Ltd. All rights reserved.

Project  :  ARM
Module   :  vchiq_arm

FILE DESCRIPTION:
Header file for vchiq_test.

==============================================================================*/

#ifndef VCHIQ_TEST_H
#define VCHIQ_TEST_H

#include "vchiq.h"

#define VERBOSE_TRACE 0

#define FUNC_FOURCC VCHIQ_MAKE_FOURCC('f','u','n','c')
#define FUN2_FOURCC VCHIQ_MAKE_FOURCC('f','u','n','2')

#define SERVICE1_DATA_SIZE 1024
#define SERVICE2_DATA_SIZE 2048
#define FUN2_MAX_DATA_SIZE 16384

#if VERBOSE_TRACE

#define EXPECT(_e, _v) if (_e != _v) { VCOS_TRACE("%d: " #_e " != " #_v, __LINE__); VCOS_BKPT; goto error_exit; } else { VCOS_TRACE("%d: " #_e " == " #_v, __LINE__); }

#define START_CALLBACK(_r, _u) \
   if (++callback_index == callback_count) { \
      if (reason != _r) { \
         VCOS_TRACE("%d: expected callback reason " #_r ", got %d", __LINE__, reason); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)VCHIQ_GET_SERVICE_USERDATA(service) != _u) { \
         VCOS_TRACE("%d: expected userdata %d, got %d", __LINE__, _u, (int)VCHIQ_GET_SERVICE_USERDATA(service)); VCOS_BKPT; goto error_exit; \
      } \
      else \
      { \
         VCOS_TRACE("%d: " #_r ", " #_u, __LINE__); \
      }

#define START_BULK_CALLBACK(_r, _u, _bu)   \
   if (++bulk_index == bulk_count) {  \
      if (reason != _r) { \
         VCOS_TRACE("%d: expected callback reason " #_r ", got %d", __LINE__, reason); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)VCHIQ_GET_SERVICE_USERDATA(service) != _u) { \
         VCOS_TRACE("%d: expected userdata %d, got %d", __LINE__, _u, (int)VCHIQ_GET_SERVICE_USERDATA(service)); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)bulk_userdata != _bu) { \
         VCOS_TRACE("%d: expected bulk_userdata %d, got %d", __LINE__, _bu, (int)bulk_userdata); VCOS_BKPT; goto error_exit; \
      } \
      else \
      { \
         VCOS_TRACE("%d: " #_r ", " #_u ", " #_bu, __LINE__); \
      }

#else

#define EXPECT(_e, _v) if (_e != _v) { VCOS_TRACE("%d: " #_e " != " #_v, __LINE__); VCOS_BKPT; goto error_exit; }

#define START_CALLBACK(_r, _u) \
   if (++callback_index == callback_count) { \
      if (reason != _r) { \
         VCOS_TRACE("%d: expected callback reason " #_r ", got %d", __LINE__, reason); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)VCHIQ_GET_SERVICE_USERDATA(service) != _u) { \
         VCOS_TRACE("%d: expected userdata %d, got %d", __LINE__, _u, (int)VCHIQ_GET_SERVICE_USERDATA(service)); VCOS_BKPT; goto error_exit; \
      }

#define START_BULK_CALLBACK(_r, _u, _bu)   \
   if (++bulk_index == bulk_count) {  \
      if (reason != _r) { \
         VCOS_TRACE("%d: expected callback reason " #_r ", got %d", __LINE__, reason); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)VCHIQ_GET_SERVICE_USERDATA(service) != _u) { \
         VCOS_TRACE("%d: expected userdata %d, got %d", __LINE__, _u, (int)VCHIQ_GET_SERVICE_USERDATA(service)); VCOS_BKPT; goto error_exit; \
      } \
      else if ((int)bulk_userdata != _bu) { \
         VCOS_TRACE("%d: expected bulkuserdata %d, got %d", __LINE__, _bu, (int)bulk_userdata); VCOS_BKPT; goto error_exit; \
      }

#endif

#define END_CALLBACK(_s) \
      return _s; \
   }

extern void vchiq_test_start_services(VCHIQ_INSTANCE_T instance);

#endif
