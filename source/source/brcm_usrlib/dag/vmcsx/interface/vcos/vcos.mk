# =============================================================================
# Copyright (c) 2009 Broadcom Europe Limited.
# All rights reserved.
# 
# Project  :  vcfw
# Module   :  osal
# File     :  $RCSfile: $
# Revision :  $Revision: $
# 
# FILE DESCRIPTION
# VideoCore OS Abstraction Layer - makefile
# =============================================================================

LIB_NAME := vcos

LIB_LIBS += interface/vcos/$(RTOS)/vcos_$(RTOS)

IPATH_GLOBAL += $(SRC_ROOT)/interface/vcos/$(RTOS)

