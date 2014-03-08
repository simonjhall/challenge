LIB_NAME := khronos_main
LIB_NAME := $(call change_name,EGL_SERVER_SMALLINT,_smallint,$(LIB_NAME),$(DEFINES_GLOBAL))

LIB_LIBS := \
   middleware/khronos/khronos_int \
   helpers/bcmcomp/qcomp/qcomp

ifeq ($(KHRONOS_EGL_PLATFORM),)
 KHRONOS_EGL_PLATFORM := dispmanx
 $(warning KHRONOS_EGL_PLATFORM not defined, defaulting to $(KHRONOS_EGL_PLATFORM))
endif

ifeq (,$(filter $(DEFINES_GLOBAL),__VIDEOCORE4__))
   # the 2708 implementation of bf will work for us too...
   LIB_VPATH += common/2707b common/2708
   LIB_SRC += \
      khrn_hw_3.c \
      khrn_image_3.c \
      khrn_interlock_3.c \
      khrn_bf_4.c \
      khrn_bf_4.s

   LIB_VPATH += gl11/2707b
   LIB_SRC += \
      gl11_assembler_3.c \
      gl11_atemplates_3.s \
      gl11_hw_3.c \
      gl11_innerloop_3.s \
      gl11_ltemplates_3.s \
      gl11_support_3.s \
      gl11_templates_3.s \
      gl11_vassembler_3.c \
      gl11_vtemplates_3.s

   LIB_VPATH += gl20/2707b
   LIB_SRC += \
      gl20_aassembler_3.c \
      gl20_atemplates_3.s \
      gl20_hw_3.c \
      gl20_innerloop_3.s \
      gl20_support_3.s

   LIB_VPATH += glsl/2707b
   LIB_SRC += \
      glsl_allocator_3.c \
      glsl_emit_3.c \
      glsl_generator_3.c \
      glsl_scheduler_3.c

   LIB_VPATH += glxx/2707b
   LIB_SRC += \
      glxx_hw_3.c \
      glxx_install_3.c \
      glxx_texture_3.c \
      glxx_transmit_3.c

   LIB_VPATH += vg/2707b
   LIB_SRC += \
      vg_bf_3.c \
      vg_hw_3.c \
      vg_memstack_3.c \
      vg_path_3.c \
      vg_path_interpolate_coords_3.s \
      vg_path_interpolate_segments_3.s \
      vg_prod_3.c \
      vg_prod_3.s \
      vg_q_3.c \
      vg_q_alloc_bcmds_cvg_fill_3.s \
      vg_q_alloc_bcmds_stroke_3.s \
      vg_q_bin_small_3.s \
      vg_q_run_tiles_inner_loop_3.s \
      vg_segment_lengths_3.c \
      vg_shader_c_3.s \
      vg_shader_fd_3.c \
      vg_shader_fd_3.s \
      vg_shader_md_3.s \
      vg_tess_3.c \
      vg_tess_arc_to_cubics_3.c \
      vg_tess_fill_fan_3.s \
      vg_tess_fill_flatten_cubics_3.s \
      vg_tess_fill_prepare_3.s \
      vg_tess_gen_3.s \
      vg_tess_stroke_dash_3.c \
      vg_tess_stroke_flatten_cubics_3.s \
      vg_tess_stroke_prepare_3.s \
      vg_tess_stroke_tri_3.s \
      vg_tess_tri_clip_3.c
else
   LIB_VPATH += common/2708
   LIB_SRC += \
      khrn_bf_4.c \
      khrn_bf_4.s \
      khrn_copy_buffer_4.c \
      khrn_fmem_4.c \
      khrn_hw_4.c \
      khrn_image_4.c \
      khrn_interlock_4.c \
      khrn_nmem_4.c \
      khrn_pool_4.c \
      khrn_prod_4.c \
      khrn_prod_4.s \
      khrn_render_state_4.c \
      khrn_worker_4.c

   LIB_VPATH += gl11/2708
   LIB_SRC += \
      gl11_shader_4.c \
      gl11_shadercache_4.c \
      gl11_support_4.c

   LIB_VPATH += gl20/2708
   LIB_SRC += \
      gl20_shader_4.c \
      gl20_support_4.c

   LIB_VPATH += glsl/2708
   LIB_SRC += \
      glsl_allocator_4.c \
      glsl_fpu_4.c \
      glsl_qdisasm_4.c \
      glsl_scheduler_4.c \
      glsl_verify_4.c

   LIB_VPATH += glxx/2708
   LIB_SRC += \
      glxx_attr_sort_4.c \
      glxx_hw_4.c \
      glxx_inner_4.c \
      glxx_shader_4.c

   LIB_VPATH += vg/2708
   LIB_SRC += \
      vg_bf_4.c \
      vg_hw_4.c \
      vg_path_4.c \
      vg_segment_lengths_4.c \
      vg_shader_fd_4.c \
      vg_shader_md_4.c \
      vg_tess_4.c \
      vg_tess_fill_shader_4.c \
      vg_tess_init_shader_4.c \
      vg_tess_stroke_shader_4.c \
      vg_tess_term_shader_4.c

   # VC_QDEBUG=1 on the make line to enable shader debugging
   ifneq (,$(VC_QDEBUG))
      LIB_NAME := $(LIB_NAME)_qdebug
      LIB_LIBS += v3d/verification/tools/2760sim/qdebug
   endif
endif

LIB_VPATH += egl/$(KHRONOS_EGL_PLATFORM)
LIB_SRC += \
   egl_platform_$(KHRONOS_EGL_PLATFORM).c

LIB_VPATH += common
LIB_SRC += \
   khrn_color.c \
   khrn_fleaky_map.c \
   khrn_image.c \
   khrn_interlock.c \
   khrn_llat.c \
   khrn_map.c \
   khrn_map_64.c \
   khrn_math.c \
   khrn_misc.c \
   khrn_stats.c \
   khrn_tformat.c

LIB_VPATH += egl
LIB_SRC += \
   egl_server.c
ifeq (,$(filter $(DEFINES_GLOBAL),EGL_DISP_FPS))
   LIB_SRC += egl_disp.c
else
   LIB_SRC += egl_disp_fps.c # egl_disp_fps.c just includes egl_disp.c
endif

LIB_VPATH += ext
LIB_SRC += \
   egl_brcm_driver_monitor.c \
   egl_brcm_global_image.c \
   egl_brcm_perf_monitor.c \
   egl_khr_image.c \
   egl_openmaxil.c \
   gl_oes_egl_image.c \
   gl_oes_query_matrix.c \
   gl_oes_draw_texture.c

LIB_VPATH += gl11
LIB_SRC += \
   gl11_matrix.c \
   gl11_server.c

LIB_VPATH += gl20
LIB_SRC += \
   gl20_program.c \
   gl20_server.c \
   gl20_shader.c

LIB_VPATH += glsl glsl/prepro
LIB_SRC += \
   glsl_ast.c \
   glsl_ast_print.c \
   glsl_ast_visitor.c \
   glsl_builders.c \
   glsl_compiler.c \
   glsl_const_functions.c \
   glsl_dataflow.c \
   glsl_dataflow_print.c \
   glsl_dataflow_visitor.c \
   glsl_errors.c \
   glsl_fastmem.c \
   glsl_globals.c \
   glsl_header.c \
   glsl_intern.c \
   glsl_map.c \
   glsl_mendenhall.c \
   glsl_stringbuilder.c \
   glsl_symbols.c \
   lex.yy.c \
   y.tab.c \
   glsl_prepro_directive.c \
   glsl_prepro_eval.c \
   glsl_prepro_expand.c \
   glsl_prepro_macro.c \
   glsl_prepro_token.c

LIB_VPATH += glxx
LIB_SRC += \
   glxx_buffer.c \
   glxx_server.c \
   glxx_server_cr.c \
   glxx_shared.c \
   glxx_texture.c \
   glxx_framebuffer.c \
   glxx_renderbuffer.c
  
   
#LIB_VPATH += perform
#LIB_SRC += \
#   khrn_visual_stats.c

LIB_VPATH += vg
LIB_SRC += \
   vg_bf.c \
   vg_font.c \
   vg_hw.c \
   vg_image.c \
   vg_mask_layer.c \
   vg_paint.c \
   vg_path.c \
   vg_ramp.c \
   vg_scissor.c \
   vg_segment_lengths.c \
   vg_server.c \
   vg_set.c \
   vg_tess.c

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)
