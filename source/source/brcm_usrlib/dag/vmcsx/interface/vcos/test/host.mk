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
# 
# Makefile for VCOS test harness on a normal host
#
# =============================================================================

.PHONY: libvcos

LIBVCOS = ../$(RTOS)/libvcos.a

VPATH += ../$(RTOS)
COMPILER ?= cl

include common.mk

SRC_ROOT ?= ../../..

IPATH=-I$(SRC_ROOT) -I$(SRC_ROOT)/interface/vcos/$(RTOS)


all: vcos_test$(EXESFX)

include ../$(RTOS)/rules.mk

OBJECTS=$(SOURCES:.c=.$(OBJEXT)) $(CPP_SOURCES:.cpp=.$(OBJEXT))

vcos_test$(EXESFX): $(OBJECTS) $(LIBVCOS)
	echo $(OBJECTS) 
	${link_cmd}


$(LIBVCOS): libvcos
	$(MAKE) -C ../$(RTOS) CFLAGS="$(CFLAGS)" COMPILER=$(COMPILER)


clean:
	$(clean_cmd)

-include *.d

