/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - win32 test file header 
=============================================================================*/

#include "interface/vcos/vcos_logging.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

extern int verbose;
#define VCOS_LOG(...) if (verbose) VCOS_ALERT(__VA_ARGS__)

#define RUN_TEST(r,p,func) {\
   int ret;(*r)++; ret=(func()); (*p)+=ret;\
   vcos_log("%-20s: %d: %s",#func,ret,ret?"pass":"FAIL");\
   if (!ret) return;\
}

