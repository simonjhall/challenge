include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	CC32FLAGS	+=	-c99

COMMON_VPATH	:= common/2708
   #SRC32S		:= khrn_bf_4.c
    SRC32S		:= bf_common.c
   #SRC32S		+= khrn_bf_4.s
	SRC32S		+= khrn_nmem_4.c
	SRC32S		+= khrn_copy_buffer_4.c
	SRC32S		+= khrn_fmem_4.c
    SRC32S		+= khrn_hw_4.c
	SRC32S		+= khrn_image_4.c
	SRC32S		+= khrn_interlock_4.c
	SRC32S		+= khrn_pool_4.c
	SRC32S		+= khrn_prod_4.c
   #SRC32S		+= khrn_prod_4.s
	SRC32S		+= khrn_render_state_4.c
	SRC32S		+= khrn_worker_4.c

COMMON_VPATH	+= gl11/2708
	SRC32S		+= gl11_shader_4.c
	SRC32S		+= gl11_shadercache_4.c
	SRC32S		+= gl11_support_4.c

COMMON_VPATH	+= gl20/2708
	SRC32S		+= gl20_shader_4.c
	SRC32S		+= gl20_support_4.c

COMMON_VPATH	+= glsl/2708
	SRC32S		+= glsl_allocator_4.c
	SRC32S		+= glsl_fpu_4.c
	SRC32S		+= glsl_qdisasm_4.c
	SRC32S		+= glsl_scheduler_4.c
	SRC32S		+= glsl_verify_4.c

COMMON_VPATH	+= glxx/2708
	SRC32S		+= glxx_attr_sort_4.c
	SRC32S		+= glxx_hw_4.c
	SRC32S		+= glxx_inner_4.c
	SRC32S		+= glxx_shader_4.c
	SRC32S		+= glxx_framebuffer.c
	SRC32S		+= glxx_renderbuffer.c

COMMON_VPATH	+= vg/2708
   #SRC32S		+= vg_bf_4.c
	SRC32S		+= bf_2708.c
	SRC32S		+= vg_hw_4.c
	SRC32S		+= vg_path_4.c
	SRC32S		+= vg_segment_lengths_4.c
	SRC32S		+= vg_shader_fd_4.c
	SRC32S		+= vg_shader_md_4.c
	SRC32S		+= vg_tess_4.c
	SRC32S		+= vg_tess_fill_shader_4.c
	SRC32S		+= vg_tess_init_shader_4.c
	SRC32S		+= vg_tess_stroke_shader_4.c
	SRC32S		+= vg_tess_term_shader_4.c

COMMON_VPATH	+= egl/mobcom
#COMMON_VPATH	+= egl/tga
	SRC32S		+= platform_egl.c

COMMON_VPATH	+= common
	SRC32S		+= khrn_color.c
	SRC32S		+= khrn_fleaky_map.c
	SRC32S		+= khrn_image.c
	SRC32S		+= khrn_interlock.c
	SRC32S		+= khrn_llat.c
	SRC32S		+= khrn_map.c
	SRC32S		+= khrn_map_64.c
	SRC32S		+= khrn_math.c
	SRC32S		+= khrn_misc.c
	SRC32S		+= khrn_tformat.c

COMMON_VPATH	+= egl
	SRC32S		+= egl_disp.c
	SRC32S		+= egl_server.c
#	SRC32S		+= server_egl.c

COMMON_VPATH	+= ext
	SRC32S		+= egl_brcm_global_image.c
	SRC32S		+= egl_brcm_global_image_id.c
	SRC32S		+= egl_brcm_perf_monitor.c
	SRC32S		+= egl_khr_image.c
    SRC32S		+= egl_openmaxil.c
	SRC32S		+= gl_oes_egl_image.c
	SRC32S		+= egl_brcm_driver_monitor.c
	SRC32S		+= gl_oes_query_matrix.c
	SRC32S		+= gl_oes_draw_texture.c


COMMON_VPATH	+= gl11
	SRC32S		+= gl11_matrix.c
	SRC32S		+= gl11_server.c

COMMON_VPATH	+= gl20
	SRC32S		+= gl20_program.c
	SRC32S		+= gl20_server.c
	SRC32S		+= gl20_shader.c

COMMON_VPATH	+= glsl
	SRC32S		+= glsl_ast.c
	SRC32S		+= glsl_ast_print.c
	SRC32S		+= glsl_ast_visitor.c
	SRC32S		+= glsl_builders.c
	SRC32S		+= glsl_compiler.c
	SRC32S		+= glsl_const_functions.c
	SRC32S		+= glsl_dataflow.c
	SRC32S		+= glsl_dataflow_print.c
	SRC32S		+= glsl_dataflow_visitor.c
	SRC32S		+= glsl_errors.c
	SRC32S		+= glsl_fastmem.c
	SRC32S		+= glsl_globals.c
	SRC32S		+= glsl_header.c
	SRC32S		+= glsl_intern.c
	SRC32S		+= glsl_map.c
	SRC32S		+= glsl_mendenhall.c
	SRC32S		+= glsl_stringbuilder.c
	SRC32S		+= glsl_symbols.c
	SRC32S		+= lex.yy.c
	SRC32S		+= y.tab.c

COMMON_VPATH	+= glsl/prepro
	SRC32S		+= glsl_prepro_directive.c
	SRC32S		+= glsl_prepro_eval.c
	SRC32S		+= glsl_prepro_expand.c
	SRC32S		+= glsl_prepro_macro.c
	SRC32S		+= glsl_prepro_token.c

COMMON_VPATH	+= glxx
	SRC32S		+= glxx_buffer.c
	SRC32S		+= glxx_server.c
	SRC32S		+= glxx_server_cr.c
	SRC32S		+= glxx_shared.c
	SRC32S		+= glxx_texture.c

COMMON_VPATH	+= vg
	SRC32S		+= vg_bf.c
	SRC32S		+= vg_font.c
	SRC32S		+= vg_hw.c
	SRC32S		+= vg_image.c
	SRC32S		+= vg_mask_layer.c
	SRC32S		+= vg_paint.c
	SRC32S		+= vg_path.c
	SRC32S		+= vg_ramp.c
	SRC32S		+= vg_scissor.c
	SRC32S		+= vg_segment_lengths.c
	SRC32S		+= vg_server.c
	SRC32S		+= vg_set.c
	SRC32S		+= vg_tess.c

ifneq ($(CLIENT_DIRECT),TRUE)
	LIB_SUBDIRS	+= dispatch
endif
