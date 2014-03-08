include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	CC32FLAGS	+=  --c99

COMMON_VPATH	:= common
	SRC32S		:= khrn_client.c
	SRC32S		+= khrn_client_cache.c
	SRC32S		+= khrn_client_global_image_map.c
	SRC32S		+= khrn_client_pointermap.c
	SRC32S		+= khrn_client_vector.c

COMMON_VPATH	+= egl
	SRC32S		+= egl_client.c
	SRC32S		+= egl_client_config.c
	SRC32S		+= egl_client_context.c
	SRC32S		+= egl_client_surface.c

COMMON_VPATH	+= ext
	SRC32S		+= egl_brcm_flush_client.c
	SRC32S		+= egl_brcm_global_image_client.c
	SRC32S		+= egl_brcm_perf_monitor_client.c
	SRC32S		+= egl_khr_image_client.c
	SRC32S		+= egl_khr_lock_surface_client.c
	SRC32S		+= egl_khr_sync_client.c
	SRC32S		+= egl_openmaxil_client.c
	SRC32S		+= gl_oes_egl_image_client.c
	SRC32S		+= egl_brcm_driver_monitor_client.c
	SRC32S		+= gl_oes_draw_texture_client.c
	SRC32S		+= gl_oes_query_matrix.c

COMMON_VPATH	+= glxx
	SRC32S		+= glxx_client.c

COMMON_VPATH	+= vg
	SRC32S		+= vg_client.c

#khronos_int
#COMMON_VPATH	+= common
	SRC32S		+= khrn_int_hash.c
	SRC32S		+= khrn_int_image.c
	SRC32S		+= khrn_int_util.c

COMMON_VPATH	+= vg
	SRC32S		+= vg_int_mat3x3.c

ifeq ($(CLIENT_DIRECT), TRUE)
    ifeq ($(CLIENT_DIRECT_MULTI),TRUE)
        COMMON_VPATH	+=	common/selfhosted
        SRC32S		+=	khrn_client_platform_selfhosted.c
        
        COMMON_VPATH	+=	common/vcos
        SRC32S		+=	khrn_client_platform_vcos.c        
    else
        COMMON_VPATH	+=	common/direct
        INCPATH		+=	$(VMCSX_DIR)/interface/khronos/common/direct
        SRC32S		+=	khrn_client_platform_direct.c
    endif
else
    COMMON_VPATH	+=	common/selfhosted
    INCPATH		+=	$(VMCSX_DIR)/interface/khronos/common/vcos
    SRC32S		+=	khrn_client_platform_selfhosted.c
    
    CLIENT_IPC	?= FALSE
    CLIENT_IPC_NO_VCHIQ ?= FALSE
    ifeq ($(CLIENT_IPC), TRUE)
        SRC32S		+=	khrn_client_rpc_selfhosted_client.c        
        ifneq ($(CLIENT_IPC_NO_VCHIQ),TRUE)
            SRC32S		+=	khrn_client_rpc_selfhosted_server.c  
        endif
    else
        SRC32S		+=	khrn_client_rpc_selfhosted.c
    endif
  
    COMMON_VPATH	+=	common/vcos
    SRC32S		+=	khrn_client_platform_vcos.c
endif

COMMON_VPATH	+=	common/mobcom
#	SRC32S		+=	khrn_client_platform_mobcom.c

