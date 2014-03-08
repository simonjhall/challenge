#/*=============================================================================
#Copyright (c) 2007 Broadcom Europe Limited.
#All rights reserved.
#
#Project  :  vcfw
#Module   :  Top level control makefile
#File     :  $File$
#Revision :  $Revision$
#
#FILE DESCRIPTION
# VCFW control makefile
#=============================================================================*/


LIB_NAME  := vcfw
LIB_SRC   := 

LIB_VPATH := 
LIB_IPATH :=
LIB_DEFINES := 
LIB_CFLAGS  := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS  :=

#include the startup code
LIB_LIBS += vcfw/startup/startup

#include the driver startup code
LIB_LIBS += vcfw/drivers/drivers

#include the module(s) startup - used for middleware, clockman, powerman etc..
LIB_LIBS += vcfw/module/module

#include the chip level drivers
LIB_LIBS += vcfw/drivers/chip/$(VCFW_TARGET_CHIP)

#include the RTOS
LIB_LIBS += vcfw/rtos/$(RTOS)/$(RTOS)

#Include the platform specific code
LIB_LIBS += vcfw/platform/$(VCFW_PLATFORM_MAKEFILE)

#include the univ compatabilty layer
LIB_LIBS += vcfw/univ/univ_vcfw

#include the vclib compatabilty layer
LIB_LIBS += vcfw/vclib/vcfw_vclib

#include this module on the global ipath
IPATH_GLOBAL  += $(SRC_ROOT)/vcfw

# hack hack hack!
LIB_LIBS += helpers/dmalib/dmalib helpers/clockman/clockman helpers/sysman/sysman vcfw/logging/logging vcfw/performance/performance

# Enable new memory allocator throughout
DEFINES_GLOBAL += VCMODS_NEWMEM

#ARTS dependencies
LIB_MODDEPS := middleware/hdmi   \
   interface/vmcs_host

#include the device control makefile
include $(SRC_ROOT)/vcfw/drivers/device/device_drivers.mk

# change the library name if either ENABLE_HOSTLINK_STDIO or WANT_THREADSAFE_IO is defined
ifeq (ENABLE_HOSTLINK_STDIO, $(findstring ENABLE_HOSTLINK_STDIO, $(DEFINES_GLOBAL)))
   LIB_NAME := $(addsuffix _rtc_debugger,$(LIB_NAME))
else
   ifeq (WANT_THREADSAFE_IO, $(findstring WANT_THREADSAFE_IO, $(DEFINES_GLOBAL)))
      LIB_NAME := $(addsuffix _rtc_debugger,$(LIB_NAME))
   endif
endif
