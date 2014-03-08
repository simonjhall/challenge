ifneq ($(BRCM_BUILD_ALL),false)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = .
SRC32S := \
	glues.c	\
	glutes.c	\
	glutes_shape.c	\
	glutes_teapot.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


HGL_LIBRARIES += libkhronos_glutes
LOCAL_MODULE := libkhronos_glutes
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libkhronos_glutes.a
HGL_LIBRARIES += libkhronos_glutes
LOCAL_MODULE := libkhronos_glutes
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif
