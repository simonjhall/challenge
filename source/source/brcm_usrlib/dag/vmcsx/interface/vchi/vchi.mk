# ======================================================================
# Copyright (c) 2007-2008 Broadcom Europe Limited.
# All rights reserved.
#
# Project  :  vchi
# Module   :  Top level control makefile
# File     :  $RCSfile:  $
# Revision :  $Revision$
#
# FILE DESCRIPTION
#    VCHI control makefile
# ======================================================================


LIB_NAME := vchi

LIB_SRC := vchi.c single.c control_service.c bulk_aux_service.c mphi_ccp2.c
LIB_SRC += software_fifo.c endian.c multiqueue.c non_wrap_fifo.c # common stuff
LIB_SRC += vcfw_os.c vcos_cv.c vcos_os.c
LIB_SRC += vchiq_wrapper.c util.c


LIB_VPATH  := message_drivers/videocore common os/vcfw connections/videocore control_service ../vchiq
LIB_VPATH  += os/vcos
LIB_IPATH  :=
LIB_CFLAGS :=
LIB_AFLAGS :=

# Need to build different local and non-local versions
LIB_NAME := $(call change_name,VCHI_LOCAL_HOST_PORT,_local,$(LIB_NAME),$(DEFINES_GLOBAL))

# Distinguish ccp2-capable and not, just in case - shouldn't be necessary, as this
# should be set up by the platform.mk
LIB_NAME := $(call change_name,VCHI_SUPPORT_CCP2TX,_ccp2,$(LIB_NAME),$(DEFINES_GLOBAL))
