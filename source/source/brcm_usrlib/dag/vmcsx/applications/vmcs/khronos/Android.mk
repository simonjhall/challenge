ifeq ($(BRCM_BUILD_ALL),true)

ifeq ($(BRCM_V3D_OPT),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += khronos_main.c

HGL_CLIENT_LIBRARIES += libapplications_vmcs_khronos_client
LOCAL_MODULE := libapplications_vmcs_khronos_client
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk


else

ifneq ($(CLIENT_DIRECT), TRUE)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += khronos_client_main.c

HGL_CLIENT_LIBRARIES += libapplications_vmcs_khronos_client
LOCAL_MODULE := libapplications_vmcs_khronos_client
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

###

include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += khronos_server.c khronos_server_main.c

HGL_SERVER_LIBRARIES += libapplications_vmcs_khronos_server
LOCAL_MODULE := libapplications_vmcs_khronos_server
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk
endif # CLIENT_DIRECT

endif # BRCM_V3D_OPT

else # BRCM_BUILD_ALL

ifeq ($(BRCM_V3D_OPT),true)

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libapplications_vmcs_khronos_client.a
HGL_CLIENT_LIBRARIES += libapplications_vmcs_khronos_client
LOCAL_MODULE := libapplications_vmcs_khronos_client
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
else


ifneq ($(CLIENT_DIRECT), TRUE)
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libapplications_vmcs_khronos_client.a
HGL_CLIENT_LIBRARIES += libapplications_vmcs_khronos_client
LOCAL_MODULE := libapplications_vmcs_khronos_client
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

###

include $(CLEAR_VARS)
LOCAL_SRC_FILES := libapplications_vmcs_khronos_server.a

HGL_SERVER_LIBRARIES += libapplications_vmcs_khronos_server
LOCAL_MODULE := libapplications_vmcs_khronos_server
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif # CLIENT_DIRECT

endif # BRCM_V3D_OPT

endif #BRCM_BUILD_ALL
