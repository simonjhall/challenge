ifneq ($(BRCM_BUILD_ALL),false)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_C_INCLUDES += $(VMCSX_DIR)/vcfw/drivers/chip/vciv/$(CHIP)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += vcsuspend.c


HGL_LIBRARIES += libhelpers_vcsuspend
LOCAL_MODULE := libhelpers_vcsuspend
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk
else
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libhelpers_vcsuspend.a
HGL_LIBRARIES += libhelpers_vcsuspend
LOCAL_MODULE := libhelpers_vcsuspend
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif

