LIB_NAME := khronos_int

LIB_VPATH += ../../interface/khronos/common
LIB_SRC += \
   khrn_int_hash.c \
   khrn_int_image.c \
   khrn_int_util.c

LIB_VPATH += ../../interface/khronos/vg
LIB_SRC += \
   vg_int_mat3x3.c

LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)
