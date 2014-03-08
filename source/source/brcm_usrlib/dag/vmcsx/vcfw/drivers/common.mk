include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

ifeq ($(BASEBAND_CHIP), ATHENA)
INCPATH += $(MSP_HOME)/soc/chal/athena/bsp/inc
endif
					
COMMON_VPATH	=	chip/vciv/2708

SRC32S			=	v3d.c											\

