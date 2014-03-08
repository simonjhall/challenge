
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl
LOCAL_MODULE_TAGS := optional


LOCAL_CFLAGS += -DLOG_TAG=\"libhgl\"
LOCAL_CFLAGS += -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

LOCAL_CFLAGS += -O3 -D__CC_ARM -D__VIDEOCORE4__ -D__HERA_V3D__ -DBCG_FB_LAYOUT 
LOCAL_CFLAGS += -DBCM_COMP_USE_RELOC_HEAP -DHAVE_PRCTL -DUNIFORM_LOCATION_IS_16_BIT -D_ATHENA_

LOCAL_CFLAGS += -DBRCM_V3D_OPT=1 

LOCAL_CFLAGS += -D_ATHENA_

CHIP:=2708

LOCAL_CFLAGS +=  -DCHIP_REVISION=20
LOCAL_CFLAGS += -DBCM_COMP_ATHENA_AXI2AHB_BUG_WORKAROUND


LOCAL_SHARED_LIBRARIES := libcutils libhardware libutils libui libv3d
LOCAL_LDLIBS := -lpthread -ldl

LOCAL_ARM_MODE := arm

VMCSX_DIR := $(LOCAL_PATH)

LOCAL_C_INCLUDES += $(VMCSX_DIR)
LOCAL_C_INCLUDES += $(VMCSX_DIR)/helpers/vc_image
LOCAL_C_INCLUDES += $(VMCSX_DIR)/interface/vcos/pthreads
#LOCAL_C_INCLUDES += frameworks/base/opengl/include
LOCAL_C_INCLUDES += hardware/libhardware/modules/gralloc
LOCAL_C_INCLUDES += $(VMCSX_DIR)/../v3d_library/inc
LOCAL_C_INCLUDES += $(VMCSX_DIR)/../v3d_library/src
LOCAL_C_INCLUDES += $(VMCSX_DIR)/interface/khronos/include
LOCAL_C_INCLUDES += $(VMCSX_DIR)/middleware/khronos/common
LOCAL_C_INCLUDES += $(VMCSX_DIR)/middleware/khronos/common/2708/platform
LOCAL_C_INCLUDES += $(VMCSX_DIR)/middleware/khronos/vg/2708/platform
LOCAL_C_INCLUDES += $(VMCSX_DIR)/vcfw/drivers/chip
LOCAL_C_INCLUDES += $(VMCSX_DIR)/vcinclude
LOCAL_C_INCLUDES += $(VMCSX_DIR)/vcinclude/dummy



LOCAL_CFLAGS += -DRPC_DIRECT -DRPC_DIRECT_MULTI  -DDIRECT_RENDERING

ifeq ($(TARGET_CPU),Cortex-A9)
	LOCAL_CFLAGS += -D__ARM_NEON__
endif


################################ interface sources #############################


SRC_PATH = interface/khronos/common
SRC32S := \
	khrn_int_hash.c	\
	khrn_int_image.c	\
	khrn_int_util.c \
	khrn_client.c	\
	khrn_client_cache.c	\
	khrn_client_global_image_map.c	\
	khrn_client_pointermap.c	\
	khrn_client_vector.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = interface/khronos/vg
SRC32S := vg_int_mat3x3.c \
	vg_client.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = interface/khronos/egl
SRC32S := \
	egl_client.c	\
	egl_client_config.c	\
	egl_client_context.c	\
	egl_client_surface.c	\
	brcm_egl.c \
	composer.cpp

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = interface/khronos/ext
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


SRC_PATH = interface/khronos/glxx
SRC32S := glxx_client.c
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

SRC_PATH = interface/khronos/common/selfhosted
SRC32S := khrn_client_platform_selfhosted.c
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))
        
SRC_PATH = interface/khronos/common/vcos
SRC32S := khrn_client_platform_vcos.c        
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

SRC_PATH = interface/vcos/generic
SRC32S := \
	vcos_generic_event_flags.c	\
	vcos_generic_named_sem.c	\
	vcos_generic_reentrant_mtx.c	\
	vcos_mem_from_malloc.c		\
	vcos_msgqueue.c		\
	vcos_abort.c	\
	vcos_logcat.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

SRC_PATH = interface/vcos/pthreads
SRC32S := vcos_pthreads.c 
LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))

################################ middleware sources #############################


SRC_PATH = middleware/khronos/common/$(CHIP)
SRC32S := \
	bf_common.c	\
	khrn_nmem_4.c	\
	khrn_copy_buffer_4.c	\
	khrn_fmem_4.c	\
	khrn_hw_4.c	\
	khrn_image_4.c	\
	khrn_interlock_4.c	\
	khrn_pool_4.c	\
	khrn_prod_4.c	\
	khrn_render_state_4.c	\
	khrn_worker_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/gl11/$(CHIP)
SRC32S := \
	gl11_shader_4.c	\
	gl11_shadercache_4.c	\
	gl11_support_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/gl20/$(CHIP)
SRC32S := \
	gl20_shader_4.c	\
	gl20_support_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/glsl/$(CHIP)
SRC32S := \
	glsl_allocator_4.c	\
	glsl_fpu_4.c	\
	glsl_qdisasm_4.c	\
	glsl_scheduler_4.c	\
	glsl_verify_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/glxx/$(CHIP)
SRC32S := \
	glxx_attr_sort_4.c	\
	glxx_hw_4.c	\
	glxx_inner_4.c	\
	glxx_shader_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/vg/$(CHIP)
SRC32S := \
	bf_2708.c	\
	vg_hw_4.c	\
	vg_path_4.c	\
	vg_segment_lengths_4.c	\
	vg_shader_fd_4.c	\
	vg_shader_md_4.c	\
	vg_tess_4.c	\
	vg_tess_fill_shader_4.c	\
	vg_tess_init_shader_4.c	\
	vg_tess_stroke_shader_4.c	\
	vg_tess_term_shader_4.c	\

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/egl/android
SRC32S := \
	platform_egl.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/common
SRC32S := \
	khrn_color.c	\
	khrn_fleaky_map.c	\
	khrn_image.c	\
	khrn_interlock.c	\
	khrn_llat.c	\
	khrn_map.c	\
	khrn_map_64.c	\
	khrn_math.c	\
	khrn_misc.c	\
	khrn_tformat.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/egl
SRC32S := \
	egl_disp.c	\
	server_egl.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/ext
SRC32S := \
	egl_brcm_global_image.c	\
	egl_brcm_global_image_id.c	\
	egl_brcm_perf_monitor.c	\
	egl_khr_image.c	\
	egl_openmaxil.c	\
	gl_oes_egl_image.c	\
	egl_brcm_driver_monitor.c	\
	gl_oes_query_matrix.c	\
	gl_oes_draw_texture.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/gl11
SRC32S := \
	gl11_matrix.c	\
	gl11_server.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/gl20
SRC32S := \
	gl20_program.c	\
	gl20_server.c	\
	gl20_shader.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/glsl
SRC32S := \
	glsl_ast.c	\
	glsl_ast_print.c	\
	glsl_ast_visitor.c	\
	glsl_builders.c	\
	glsl_compiler.c	\
	glsl_const_functions.c	\
	glsl_dataflow.c	\
	glsl_dataflow_print.c	\
	glsl_dataflow_visitor.c	\
	glsl_errors.c	\
	glsl_fastmem.c	\
	glsl_globals.c	\
	glsl_header.c	\
	glsl_intern.c	\
	glsl_map.c	\
	glsl_mendenhall.c	\
	glsl_stringbuilder.c	\
	glsl_symbols.c	\
	lex.yy.c	\
	y.tab.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/glsl/prepro
SRC32S := \
	glsl_prepro_directive.c	\
	glsl_prepro_eval.c	\
	glsl_prepro_expand.c	\
	glsl_prepro_macro.c	\
	glsl_prepro_token.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/glxx
SRC32S := \
	glxx_buffer.c	\
	glxx_server.c	\
	glxx_server_cr.c	\
	glxx_shared.c	\
	glxx_texture.c	\
	glxx_framebuffer.c	\
	glxx_renderbuffer.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = middleware/khronos/vg
SRC32S := \
	vg_bf.c	\
	vg_font.c	\
	vg_hw.c	\
	vg_image.c	\
	vg_mask_layer.c	\
	vg_paint.c	\
	vg_path.c	\
	vg_ramp.c	\
	vg_scissor.c	\
	vg_segment_lengths.c	\
	vg_server.c	\
	vg_set.c	\
	vg_tess.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


################################ applications sources #############################

LOCAL_SRC_FILES += applications/vmcs/khronos/khronos_main.c

################################ vcfw sources #############################

SRC_PATH = vcfw/drivers/chip/vciv/$(CHIP)
SRC32S := v3d.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = vcfw/rtos/common
SRC32S := rtos_common_mem.cpp

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


LOCAL_MODULE:= libGLES_hgl

include $(BUILD_SHARED_LIBRARY)
