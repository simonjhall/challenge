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
# VideoCore OS Abstraction Layer - Nucleus makefile
# =============================================================================

LIB_NAME := vcos_nucleus
LIB_SRC  := vcos_nucleus.c vcos_joinable_thread_from_plain.c \
            vcos_generic_reentrant_mtx.c vcos_generic_named_sem.c \
            vcos_generic_tls.c \
            vcos_msgqueue.c vcos_abort.c
LIB_VPATH :=
LIB_VPATH += ../generic

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)

IPATH_GLOBAL += $(SRC_ROOT)/interface/vcos/$(RTOS)

AFLAGS_GLOBAL += -Hasopt=-I$(SRC_ROOT)/interface/vcos/$(RTOS)


