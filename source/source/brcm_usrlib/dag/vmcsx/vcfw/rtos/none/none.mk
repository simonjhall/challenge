# RTOS=NONE makefile

LIB_NAME  := vcfw_rtos_none
LIB_SRC   := rtos_none.c rtos_malloc.c rtos_none_asm.s rtos_none_latch.c
	     

LIB_IPATH := .. ../../drivers/chip
LIB_DEFINES := 
LIB_CFLAGS  :=
LIB_AFLAGS  :=

ifeq (VIDEOCORE4, $(findstring VIDEOCORE4, $(DEFINES_GLOBAL)))
 LIB_VPATH += ./vciv
else
 ifeq (VIDEOCORE3, $(findstring VIDEOCORE3, $(DEFINES_GLOBAL)))
  LIB_VPATH += ./vciii
 else
  LIB_VPATH += ./vcii
 endif
endif

#always include the default library
LIB_LIBS := vcfw/rtos/common/rtos_common
LIB_LIBS += middleware/rpc/rpc

ifdef VCGF
 LIB_DEFINES += WANT_VCGF
 LIB_NAME := $(call change_name,WANT_VCGF,_vcgf,$(LIB_NAME),$(LIB_DEFINES))
endif
