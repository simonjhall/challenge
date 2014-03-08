include $(MSP_HOME)/dag/vmcsx/applications/cc_flags_test.mk

	LIB_SUBDIRS	:=	helpers
	LIB_SUBDIRS	+=  middleware
	LIB_SUBDIRS	+=  vcfw
	LIB_SUBDIRS	+=  interface
	LIB_SUBDIRS	+=  applications
	LIB_SUBDIRS	+=  thirdparty
	LIB_SUBDIRS	+=  opensrc

ifeq ($(INC_QPU_DEBUG),TRUE)
	LIB_SUBDIRS	+=  v3d
endif                
