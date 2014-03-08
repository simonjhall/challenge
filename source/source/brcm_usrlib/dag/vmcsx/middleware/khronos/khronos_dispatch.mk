LIB_NAME := khronos_dispatch

LIB_VPATH += dispatch
LIB_SRC += \
   khrn_dispatch.c \
   khrn_dispatch_egl.c \
   khrn_dispatch_gl11.c \
   khrn_dispatch_gl20.c \
   khrn_dispatch_glxx.c \
   khrn_dispatch_vg.c

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)
