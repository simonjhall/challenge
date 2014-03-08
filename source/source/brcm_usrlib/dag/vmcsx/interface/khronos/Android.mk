ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = common
SRC32S := \
	khrn_int_hash.c	\
	khrn_int_image.c	\
	khrn_int_util.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = vg
SRC32S := \
	vg_int_mat3x3.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


HGL_CLIENT_LIBRARIES += libinterface_khronos_common
ifneq ($(BRCM_V3D_OPT),true)
HGL_SERVER_LIBRARIES += libinterface_khronos_common
endif
LOCAL_MODULE := libinterface_khronos_common
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

###

include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)


SRC_PATH = common
SRC32S := \
	khrn_client.c	\
	khrn_client_cache.c	\
	khrn_client_global_image_map.c	\
	khrn_client_pointermap.c	\
	khrn_client_vector.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = egl
SRC32S := \
	egl_client.c	\
	egl_client_config.c	\
	egl_client_context.c	\
	egl_client_surface.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = ext
SRC32S := \
	egl_brcm_flush_client.c	\
	egl_brcm_global_image_client.c	\
	egl_brcm_perf_monitor_client.c	\
	egl_khr_image_client.c	\
	egl_khr_lock_surface_client.c	\
	egl_khr_sync_client.c	\
	egl_openmaxil_client.c	\
	gl_oes_egl_image_client.c	\
	egl_brcm_driver_monitor_client.c	\
	gl_oes_draw_texture_client.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = glxx
SRC32S := \
	glxx_client.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = vg
SRC32S := \
	vg_client.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


ifeq ($(CLIENT_DIRECT), TRUE)
ifeq ($(CLIENT_DIRECT_MULTI),TRUE)
SRC_PATH = common/selfhosted
SRC32S := khrn_client_platform_selfhosted.c
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))
        
SRC_PATH = common/vcos
SRC32S := khrn_client_platform_vcos.c        
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

else # CLIENT_DIRECT_MULTI

SRC_PATH = common/direct
SRC32S := khrn_client_platform_direct.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

endif # CLIENT_DIRECT_MULTI

else # CLIENT_DIRECT

SRC_PATH = common/selfhosted

ifeq ($(RPC_TO_VCHI), TRUE)
SRC32S := \
	khrn_client_platform_selfhosted.c	\
	khrn_client_rpc_selfhosted_client_android.c
else # RPC_TO_VCHI
SRC32S := \
	khrn_client_platform_selfhosted.c	\
	khrn_client_rpc_selfhosted.c
endif # RPC_TO_VCHI

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

endif # CLIENT_DIRECT


HGL_CLIENT_LIBRARIES += libinterface_khronos
LOCAL_MODULE := libinterface_khronos
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk


###

ifneq ($(CLIENT_DIRECT), TRUE)
ifeq ($(RPC_TO_VCHI), TRUE)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES +=	\
	common/selfhosted/khrn_client_rpc_selfhosted_server_android.c

HGL_SERVER_LIBRARIES += libinterface_khronos_client_rpc_selfhosted_server
LOCAL_MODULE := libinterface_khronos_client_rpc_selfhosted_server
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

endif # RPC_TO_VCHI
endif # CLIENT_DIRECT

###

ifeq ($(DIRECT_RENDERING), TRUE)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES += $(TARGET_PROJECT_INCLUDES) $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += egl/brcm_egl.c

HGL_CLIENT_LIBRARIES += libinterface_khronos_egl_brcm
LOCAL_MODULE := libinterface_khronos_egl_brcm
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk
endif # DIRECT_RENDERING


######################
else  # BRCM_BUILD_ALL
######################


LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

HGL_CLIENT_LIBRARIES += libinterface_khronos_common
ifneq ($(BRCM_V3D_OPT),true)
HGL_SERVER_LIBRARIES += libinterface_khronos_common
endif
LOCAL_MODULE := libinterface_khronos_common
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := libinterface_khronos_common.a
LOCAL_MODULE := libinterface_khronos_common
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

###

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libinterface_khronos.a
HGL_CLIENT_LIBRARIES += libinterface_khronos
LOCAL_MODULE := libinterface_khronos
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)

###

ifneq ($(CLIENT_DIRECT), TRUE)
ifeq ($(RPC_TO_VCHI), TRUE)
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := libinterface_khronos_client_rpc_selfhosted_server.a
HGL_SERVER_LIBRARIES += libinterface_khronos_client_rpc_selfhosted_server
LOCAL_MODULE := libinterface_khronos_client_rpc_selfhosted_server
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif # RPC_TO_VCHI
endif # CLIENT_DIRECT
###

ifeq ($(DIRECT_RENDERING), TRUE)
LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := libinterface_khronos_egl_brcm.a
HGL_CLIENT_LIBRARIES += libinterface_khronos_egl_brcm
LOCAL_MODULE := libinterface_khronos_egl_brcm
LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
include $(BUILD_PREBUILT)
endif # DIRECT_RENDERING


endif # BRCM_BUILD_ALL
