ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_C_INCLUDES += \
                      $(TOP)/hardware/common/opencore/codec/include

LOCAL_CFLAGS := $(CC32FLAGS)
LOCAL_CFLAGS += -DBCM_COMP_PLATFORM_2708DB

SRC_PATH = qcomp
SRC32S := \
	alloc.c	\
	conversion_luts.c	\
	generate_clif.c	\
	qpu_control_db.c	\
	qcomp_b0.c	\
	history.c	\
	fast_alloc.c	\
	rect.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = util
SRC32S := \
	util.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


HGL_LIBRARIES += libbrcmcomp
LOCAL_MODULE := libbrcmcomp
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else # BRCM_BUILD_ALL

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libbrcmcomp.a
HGL_LIBRARIES += libbrcmcomp
LOCAL_MODULE := libbrcmcomp
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

endif # BRCM_BUILD_ALL
