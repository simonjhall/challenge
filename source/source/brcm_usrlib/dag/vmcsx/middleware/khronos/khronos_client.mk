LIB_NAME := khronos_client
ifeq (RPC_DIRECT,$(filter $(LIB_DEFINES),RPC_DIRECT RPC_LIBRARY))
   LIB_NAME := $(LIB_NAME)_direct
else
   LIB_NAME := $(call change_name,RPC_LIBRARY,_lib,$(LIB_NAME),$(LIB_DEFINES))
endif
LIB_NAME := $(call change_name,EGL_SERVER_SMALLINT,_smallint,$(LIB_NAME),$(DEFINES_GLOBAL))

LIB_LIBS := \
   middleware/khronos/khronos_int

ifeq (RPC_DIRECT,$(filter $(LIB_DEFINES),RPC_DIRECT RPC_LIBRARY))
   LIB_VPATH += ../../interface/khronos/common/direct
   LIB_SRC += \
      khrn_client_platform_direct.c
else
   LIB_VPATH += ../../interface/khronos/common/selfhosted ../../interface/khronos/common/vcos
   LIB_SRC += \
      khrn_client_platform_selfhosted.c \
      khrn_client_platform_vcos.c
   ifeq (,$(filter $(LIB_DEFINES),RPC_LIBRARY))
      LIB_SRC += khrn_client_rpc_selfhosted.c
   endif
endif

LIB_VPATH += ../../interface/khronos/common
LIB_SRC += \
   khrn_client.c \
   khrn_client_cache.c \
   khrn_client_global_image_map.c \
   khrn_client_pointermap.c \
   khrn_client_vector.c

LIB_VPATH += ../../interface/khronos/egl
LIB_SRC += \
   egl_client.c \
   egl_client_config.c \
   egl_client_context.c \
   egl_client_surface.c

LIB_VPATH += ../../interface/khronos/ext
LIB_SRC += \
   egl_brcm_driver_monitor_client.c \
   egl_brcm_flush_client.c \
   egl_brcm_global_image_client.c \
   egl_brcm_perf_monitor_client.c \
   egl_khr_image_client.c \
   egl_khr_lock_surface_client.c \
   egl_khr_sync_client.c \
   egl_openmaxil_client.c \
   gl_oes_egl_image_client.c \
   gl_oes_query_matrix.c \
   gl_oes_framebuffer_object.c \
   gl_oes_draw_texture.c 

LIB_VPATH += ../../interface/khronos/glxx
LIB_SRC += \
   glxx_client.c

LIB_VPATH += ../../interface/khronos/vg
LIB_SRC += \
   vg_client.c

LIB_VPATH += ../../interface/khronos/wf
LIB_SRC += \
   wfc_client.c \
   wfc_client_stream.c \
   wfc_client_utils.c

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)
