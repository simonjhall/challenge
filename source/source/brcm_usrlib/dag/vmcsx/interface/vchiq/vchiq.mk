# ======================================================================
# Copyright (c) 2007-2008 Broadcom Europe Limited.
# All rights reserved.
#
# Project  :  vchi
# Module   :  Top level control makefile
# File     :  $RCSfile:  $
# Revision :  $Revision:  $
#
# FILE DESCRIPTION
#    VCHIQ control makefile
# ======================================================================

ifdef USE_VCHI
LIB_NAME := vchiq_dummy
LIB_LIBS := interface/vchi/vchi
LIB_SRC :=
else

LIB_NAME := vchiq
LIB_SRC := core.c util.c shim.c vcfw_os.c vcos_os.c vcos_os_vchiq.c

LIB_VPATH  := . ../vchi/os/vcfw ../vchi/os/vcos
LIB_IPATH  :=
#LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS :=

ifdef VCHI_LOCAL_HOST_PORT
   LIB_VPATH += local
   LIB_SRC += local.c
   LIB_NAME := $(LIB_NAME)_local
endif

endif

