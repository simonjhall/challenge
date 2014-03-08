ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = generic
SRC32S := \
	vcos_generic_event_flags.c	\
	vcos_generic_named_sem.c	\
	vcos_generic_reentrant_mtx.c	\
	vcos_mem_from_malloc.c		\
	vcos_msgqueue.c		\
	vcos_abort.c	\
	vcos_logcat.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

LOCAL_SRC_FILES += pthreads/vcos_pthreads.c

HGL_LIBRARIES += libinterface_vcos
LOCAL_MODULE := libinterface_vcos
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else # BRCM_BUILD_ALL

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libinterface_vcos.a
HGL_LIBRARIES += libinterface_vcos
LOCAL_MODULE := libinterface_vcos
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

endif # BRCM_BUILD_ALL
