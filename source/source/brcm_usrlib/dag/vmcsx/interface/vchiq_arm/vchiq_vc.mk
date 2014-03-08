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

LIB_NAME := vchiqarm_vc
LIB_SRC := vchiq_core.c vchiq_util.c vchiq_vc.c vchiq_shim.c vchiq_test_services.c

DEFINES_GLOBAL += VCHIQ_VC_SIDE USE_VCHIQ_ARM VCHI_BULK_ALIGN=1 VCHI_BULK_GRANULARITY=1

LIB_IPATH  :=
LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS :=
