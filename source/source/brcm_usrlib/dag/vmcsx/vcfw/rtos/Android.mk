ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = common
SRC32S := \
	rtos_common_mem.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


HGL_LIBRARIES += libvcfw_rtos
LOCAL_MODULE := libvcfw_rtos
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else # BRCM_BUILD_ALL

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libvcfw_rtos.a
HGL_LIBRARIES += libvcfw_rtos
LOCAL_MODULE := libvcfw_rtos
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

endif # BRCM_BUILD_ALL
