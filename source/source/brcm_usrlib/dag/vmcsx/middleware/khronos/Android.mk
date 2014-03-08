ifeq ($(BRCM_BUILD_ALL),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_C_INCLUDES += \
                     $(TOP)/hardware/common/opencore/codec/include
LOCAL_CFLAGS := $(CC32FLAGS)

SRC_PATH = common/$(CHIP)
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


SRC_PATH = gl11/$(CHIP)
SRC32S := \
	gl11_shader_4.c	\
	gl11_shadercache_4.c	\
	gl11_support_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = gl20/$(CHIP)
SRC32S := \
	gl20_shader_4.c	\
	gl20_support_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = glsl/$(CHIP)
SRC32S := \
	glsl_allocator_4.c	\
	glsl_fpu_4.c	\
	glsl_qdisasm_4.c	\
	glsl_scheduler_4.c	\
	glsl_verify_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = glxx/$(CHIP)
SRC32S := \
	glxx_attr_sort_4.c	\
	glxx_hw_4.c	\
	glxx_inner_4.c	\
	glxx_shader_4.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = vg/$(CHIP)
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


SRC_PATH = egl/android
SRC32S := \
	platform_egl.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = common
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


SRC_PATH = egl
SRC32S := \
	egl_disp.c	\
	server_egl.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = ext
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


SRC_PATH = gl11
SRC32S := \
	gl11_matrix.c	\
	gl11_server.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = gl20
SRC32S := \
	gl20_program.c	\
	gl20_server.c	\
	gl20_shader.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = glsl
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


SRC_PATH = glsl/prepro
SRC32S := \
	glsl_prepro_directive.c	\
	glsl_prepro_eval.c	\
	glsl_prepro_expand.c	\
	glsl_prepro_macro.c	\
	glsl_prepro_token.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = glxx
SRC32S := \
	glxx_buffer.c	\
	glxx_server.c	\
	glxx_server_cr.c	\
	glxx_shared.c	\
	glxx_texture.c	\
	glxx_framebuffer.c	\
	glxx_renderbuffer.c

LOCAL_SRC_FILES += $(addprefix $(SRC_PATH)/, $(SRC32S))


SRC_PATH = vg
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


HGL_SERVER_LIBRARIES += libkhronos
LOCAL_MODULE := libkhronos
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

###

include $(CLEAR_VARS)

include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
LOCAL_C_INCLUDES := $(INCPATH)
LOCAL_CFLAGS := $(CC32FLAGS)

LOCAL_SRC_FILES += egl/android/nativewindow.cpp

LOCAL_SHARED_LIBRARIES := libsurfaceflinger libui
LOCAL_PRELINK_MODULE := false
HGL_SERVER_LIBRARIES += libnativewindow
LOCAL_MODULE := libnativewindow
LOCAL_MODULE_TAGS := optional
include build/core/copy_static_library.mk

else # BRCM_BUILD_ALL

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libkhronos.a
LOCAL_MODULE := libkhronos
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
HGL_SERVER_LIBRARIES += libkhronos
include $(BUILD_PREBUILT)

LOCAL_PATH := $(TARGET_PREBUILT_LIBS)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := libnativewindow.a
LOCAL_MODULE := libnativewindow
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MODULE_SUFFIX := .a
HGL_SERVER_LIBRARIES += libnativewindow
include $(BUILD_PREBUILT)

endif # BRCM_BUILD_ALL
