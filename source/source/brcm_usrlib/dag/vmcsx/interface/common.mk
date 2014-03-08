include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	LIB_SUBDIRS	:=	vcos
ifeq ($(CLIENT_IPC_NO_VCHIQ),TRUE)
	LIB_SUBDIRS	+=	vchiq_ipc
else
  ifneq ($(CLIENT_DIRECT_MULTI),TRUE)
	  LIB_SUBDIRS	+=	vchiq
	endif
endif	
	LIB_SUBDIRS	+=	khronos
