
COMPILER         ?= mw
PLATFORM         ?= 2727dk
DEBUG            ?= TRUE
SRC_ROOT         ?= ../../..
MEDIA            := sdcard
FILESYSTEM       := none

# ------------------------------------------------------------------------------ #

include $(SRC_ROOT)/interface/vcos/test/common.mk

NAME             := vcos_test_$(RTOS)
ifeq ($(RTOS),none)
SOURCE           := $(SOURCES_none) $(CPP_SOURCES)
else
SOURCE           := $(SOURCES) $(CPP_SOURCES)
endif

LIBS             += interface/vcos/vcos vcfw/logging/logging

include $(SRC_ROOT)/makefiles/add_application.mk
include $(SRC_ROOT)/makefiles/common.mk
