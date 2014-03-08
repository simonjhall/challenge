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
# VideoCore OS Abstraction Layer - Threadx makefile
# =============================================================================

LIB_NAME := vcos_threadx
LIB_SRC := vcos_threadx.c vcos_joinable_thread_from_plain.c \
           vcos_generic_reentrant_mtx.c vcos_generic_named_sem.c \
           vcos_generic_tls.c \
           vcos_spinlock_semaphore.c \
           vcos_msgqueue.c vcos_abort.c vcos_dlfcn.c \
           vcos_threadx_quickslow_mutex.c \
           vcos_logcat.c vcos_thread_reaper.c

LIB_VPATH := ../generic
LIB_VPATH += ../../../vcfw/vcos
LIB_VPATH += ../vc

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)

IPATH_GLOBAL += $(SRC_ROOT)/interface/vcos/$(RTOS)
AFLAGS_GLOBAL += -Hasopt=-I$(SRC_ROOT)/interface/vcos/$(RTOS)

