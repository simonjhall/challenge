ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS) -std=c99

SRC_PATH = .
SRC32S := \
	khrn_dispatch.c	\
	khrn_dispatch_gl11.c	\
	khrn_dispatch_gl20.c	\
	khrn_dispatch_glxx.c	\
	khrn_dispatch_vg.c	\
	khrn_dispatch_egl.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

HGL_SERVER_LIBRARIES += libkhronos_dispatch
LOCAL_MODULE := libkhronos_dispatch
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else # BRCM_BUILD_ALL

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := libkhronos_dispatch.a
HGL_SERVER_LIBRARIES += libkhronos_dispatch
LOCAL_MODULE := libkhronos_dispatch
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

endif # BRCM_BUILD_ALL
