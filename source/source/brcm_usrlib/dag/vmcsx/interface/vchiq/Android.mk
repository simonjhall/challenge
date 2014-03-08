ifeq ($(BRCM_BUILD_ALL),true)

ifneq ($(CLIENT_DIRECT),TRUE)
#ifeq ($(KHRN_USE_VCHIQ),TRUE)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = .
SRC32S := \
	core.c	\
	util_vchiq.c	\
	shim.c	\
	vcos_os_vchiq.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


LOCAL_SRC_FILES += ../vchi/os/vcfw/vcfw_os.c
LOCAL_SRC_FILES += ../vchi/os/vcos/vcos_os.c


ifdef VCHI_LOCAL_HOST_PORT
	LOCAL_SRC_FILES += local/local.c
endif


HGL_LIBRARIES += libinterface_vchiq
LOCAL_MODULE := libinterface_vchiq
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk
endif # CLIENT_DIRECT

else # BRCM_BUILD_ALL
ifneq ($(CLIENT_DIRECT),TRUE)
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := libinterface_vchiq.a
HGL_LIBRARIES += libinterface_vchiq
LOCAL_MODULE := libinterface_vchiq
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif
endif # BRCM_BUILD_ALL
