LIB_NAME := khronos_client_direct
LIB_NAME := $(call change_name,RSO_FRAMEBUFFER,_rsoframe,$(LIB_NAME),$(DEFINES_GLOBAL))

LIB_SRC  := cache.c client.c config.c context.c egl.c gl.c global_image_map.c \
            platform.c pointermap.c surface.c vector.c vg.c egl_brcm_flush.c \
            egl_brcm_global_image.c egl_khr_image.c egl_khr_lock_surface.c \
            egl_khr_sync.c gl_oes_egl_image.c gl_oes_query_matrix.c gl_oes_framebuffer_object.c

LIB_LIBS :=

LIB_VPATH  := egl direct ext
LIB_IPATH  :=
LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)

DEFINES_GLOBAL += RPC_DIRECT
