LIB_NAME := khronos_server

LIB_SRC  := khronos_server.c egl_brcm_global_image_id.c

LIB_DEFINES :=
LIB_CFLAGS_mw  := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS  :=
LIB_VPATH   := ../../../middleware/khronos/ext

LIB_LIBS    += helpers/vcsuspend/vcsuspend
