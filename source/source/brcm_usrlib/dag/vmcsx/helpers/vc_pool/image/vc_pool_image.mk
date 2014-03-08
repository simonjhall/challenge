LIB_NAME  := vc_pool_image

LIB_SRC   := vc_pool_image.c

LIB_LIBS  := helpers/vc_pool/vc_pool helpers/vc_image/vc_image
LIB_VPATH :=
LIB_IPATH :=
LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)

LIB_NAME := $(call change_name,VC_POOL_IMAGE_SCRUB,_scrub,$(LIB_NAME),$(DEFINES_GLOBAL))
