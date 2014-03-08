include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
					
	INCPATH		+=	$(VMCSX_DIR)/vcfw
	INCPATH		+=	$(VMCSX_DIR)/vcfw/drivers/chip

COMMON_VPATH	+=	common
	SRC32S		+=	rtos_common_mem.c

#COMMON_VPATH	+=	none
#	SRC32S		+=	rtos_none_latch.c

