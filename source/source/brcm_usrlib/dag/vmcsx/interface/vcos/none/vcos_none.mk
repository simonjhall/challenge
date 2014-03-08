# =============================================================================
# Copyright (c) 2009 Broadcom Europe Limited.
# All rights reserved.
#
# Project  :  vcfw
# Module   :  vcos
# File     :  $RCSfile: $
# Revision :  $Revision: $
#
# FILE DESCRIPTION
# VideoCore OS Abstraction Layer - no-RTOS makefile
# =============================================================================

LIB_NAME := vcos_none
LIB_SRC := vcos_none.c vcos_generic_event_flags.c vcos_generic_named_sem.c \
           vcos_generic_reentrant_mtx.c vcos_generic_tls.c \
           vcos_joinable_thread_from_plain.c vcos_spinlock_semaphore.c \
           vcos_msgqueue.c vcos_abort.c vcos_logcat.c

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_VPATH :=
LIB_VPATH += ../generic ../../../vcfw/vcos

IPATH_GLOBAL += $(SRC_ROOT)/interface/vcos/$(RTOS)
AFLAGS_GLOBAL += -Hasopt=-I$(SRC_ROOT)/interface/vcos/$(RTOS)

