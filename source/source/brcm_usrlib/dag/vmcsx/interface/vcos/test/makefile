
# Makefile for vcos_test

RTOS ?= win32
SRC_ROOT ?= ../../..

ifdef FOR_ARTS
   CFLAGS   := -DARTS=$(ARTS)
endif

ifeq ($(RTOS),win32)
include host.mk
endif

ifeq ($(RTOS),pthreads)
include host.mk
endif

ifeq ($(RTOS),nucleus)
include $(SRC_ROOT)/interface/vcos/test/vc.mk
endif

ifeq ($(RTOS),threadx)
include $(SRC_ROOT)/interface/vcos/test/vc.mk
endif

ifeq ($(RTOS),none)
include $(SRC_ROOT)/interface/vcos/test/vc.mk
endif

