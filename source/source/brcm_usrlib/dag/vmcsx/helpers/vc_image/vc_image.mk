LIB_NAME := vc_image

LIB_SRC  :=   vc_image.c vc_image_helper.c vc_region.c      vc_image_1bpp.c               \
	      vc_image_resize_bytes.c     vc_image_resize_rgb565.c      \
	      vc_image_resize_yuv.c       vc_image_tmpbuf.c             \
	      vc_image_yuv.c              vc_image_rgb888.c             \
	      vc_image_rgb565.c           vc_image_alphablend.c        \
	      im_utils_asm.s              vc_image_im_utils.c           \
	      vc_image_1bpp_asm.s             vc_image_asm.s            \
	      vc_image_resize_bytes_2d.s      vc_image_resize_bytes_hoz.s     vc_image_resize_bytes_vert.s \
	      vc_image_resize_rgb565_2d.s     vc_image_resize_rgb565_hoz.s    vc_image_resize_rgb565_vert.s \
	      vc_image_rgb565_asm.s           vc_image_rgb888_asm.s 	        vc_image_yuv_asm.s \
	      alphablend.asm yuv16.s \
	      vc_image_aa_lines.c vc_image_line_asm.s vc_image_metadata.c

###LIB_LIBS := helpers/vclib/vclib helpers/vc_pool/vc_pool

ifeq ($(findstring USE_ALL_VC_IMAGE_TYPES,$(DEFINES_GLOBAL)),USE_ALL_VC_IMAGE_TYPES)
DEFINES_GLOBAL += USE_RGB565
DEFINES_GLOBAL += USE_RGB666
DEFINES_GLOBAL += USE_RGB888
DEFINES_GLOBAL += USE_RGBA32
DEFINES_GLOBAL += USE_RGBA16
DEFINES_GLOBAL += USE_RGBA565
DEFINES_GLOBAL += USE_YUV420
DEFINES_GLOBAL += USE_YUV422
DEFINES_GLOBAL += USE_YUV422PLANAR
DEFINES_GLOBAL += USE_3D32
DEFINES_GLOBAL += USE_4BPP
DEFINES_GLOBAL += USE_8BPP
DEFINES_GLOBAL += USE_RGB2X9
DEFINES_GLOBAL += USE_TFORMAT
DEFINES_GLOBAL += USE_BGR888
DEFINES_GLOBAL += USE_YUV_UV

# these appear in the source but don't seem to change the name of the library:
DEFINES_GLOBAL += USE_1BPP
DEFINES_GLOBAL += USE_48BPP
endif

ifeq ($(findstring USE_RGB2X9,$(DEFINES_GLOBAL)),USE_RGB2X9)
LIB_SRC += vc_image_rgb2x9_asm.s
endif

ifeq ($(findstring USE_RGB666,$(DEFINES_GLOBAL)),USE_RGB666)
LIB_SRC += vc_image_rgb666_asm.s
endif

ifneq (,                                           \
   $or(                                            \
      $(findstring USE_8BPP,$(DEFINES_GLOBAL)),    \
      $(findstring USE_YUV420,$(DEFINES_GLOBAL)),  \
      $(findstring USE_YUV422,$(DEFINES_GLOBAL))   \
   )                                               \
)
LIB_SRC +=  vc_image_8bpp.c  vc_image_8bpp_asm.s vc_image_palette8_asm.s
endif

ifeq ($(findstring USE_4BPP,$(DEFINES_GLOBAL)),USE_4BPP)
LIB_SRC +=  vc_image_4bpp.c  vc_image_4bpp_asm.s
endif

ifeq ($(findstring USE_RGBA32,$(DEFINES_GLOBAL)),USE_RGBA32)
LIB_SRC +=  vc_image_rgba32.c  vc_image_rgba32_asm.s
endif

ifeq ($(findstring USE_RGBA16,$(DEFINES_GLOBAL)),USE_RGBA16)
LIB_SRC +=  vc_image_rgba16.c  vc_image_rgba16_asm.s
endif

ifeq ($(findstring USE_RGBA565,$(DEFINES_GLOBAL)),USE_RGBA565)
LIB_SRC +=  vc_image_rgba565.c  vc_image_rgba565_asm.s
endif

ifeq ($(findstring USE_TFORMAT,$(DEFINES_GLOBAL)),USE_TFORMAT)
LIB_SRC +=  vc_image_tformat.c  vc_image_tformat_asm.s vc_image_tfresize_asm.s etc1encode.c etc1encode_asm.s
LIB_SRC +=  vc_image_tfsub2ut_8bpp.s vc_image_tfsub2ut_16bpp.s vc_image_tfsub2ut_32bpp.s
endif

ifeq ($(findstring USE_BGR888,$(DEFINES_GLOBAL)),USE_BGR888)
LIB_SRC +=  vc_image_bgr888.c  vc_image_bgr888_asm.asm
endif

ifeq ($(findstring USE_YUV_UV,$(DEFINES_GLOBAL)),USE_YUV_UV)
LIB_SRC +=  vc_image_yuvuv128.c  vc_image_yuvuv128_asm.s
endif

ifneq (,                                                \
   $or(                                                 \
      $(findstring USE_YUV420,$(DEFINES_GLOBAL)),       \
      $(findstring USE_YUV422PLANAR,$(DEFINES_GLOBAL)), \
      $(findstring USE_YUV_UV,$(DEFINES_GLOBAL))        \
   )                                                    \
)
LIB_SRC +=  vc_image_rgb_to_yuv.c vc_image_rgb_to_yuv_asm.s
endif

ifeq ($(findstring __VIDEOCORE2__,$(DEFINES_GLOBAL)),__VIDEOCORE2__)
LIB_LIBS +=  display/display

LIB_SRC +=  yuv_to_rgb.c       yuv_to_rgb_asm.s
LIB_SRC += genconv_asm.s
LIB_SRC += striperesize_asm.s
LIB_SRC += vc_image_atext.c vc_image_atext_asm.s vc_image_atext_data.c
else
ifeq ($(findstring __VIDEOCORE3__,$(DEFINES_GLOBAL)),__VIDEOCORE3__)
LIB_SRC +=  yuv_to_rgb.c       yuv_to_rgb_asm.s
LIB_SRC += genconv_asm.s
LIB_SRC += striperesize_asm.s
LIB_SRC += vc_image_atext.c vc_image_atext_asm.s vc_image_atext_data.c
else
LIB_SRC += vc_image_atext_vc01.c vc_image_atext_data.c
endif
endif

LIB_VPATH  :=
LIB_IPATH  := .. ###../vcmaths
LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS :=

# Rules about what to call the library
# ====================================
#
# The vc_image library name is appended with the image format types
# it can support:
#
# _565    : can support RGB_565 images     : USE_RGB565
# _888    : can support RGB_888 images     : USE_RGB888
# _a32    : can support RGBA32 images      : USE_RGBA32
# _a565   : can support RGBA565 images     : USE_RGBA565
# _yuv    : can support YUV420 images      : USE_YUV420
# _yuv422 : can support YUV422 images      : USE_YUV422
# _3d32   : can support the v3d format     : USE_3D32
# _4bpp   : general support for 4-bit palettised images : USE_4BPP
# _8bpp   : general support for 4-bit palettised images : USE_8BPP
# _2x9    : can support RGB2x9 images      : USE_RGB2X9
# _tf     : can support T-format images    : USE_TFORMAT
#
# The platform's LCD will require a particular image type (e.g. 24-bit)
# and the application will also make a requirement (e.g. 3D game). These
# requirements are indicated through globally defined symbols in $(DEFINES)
#
# e.g. vc_image_888_3d32.a would be produced for a 3D game on a platform
#      that uses a 24-bit LCD
#

LIB_NAME := $(call change_name,RGB888,_redlsb,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGB565,_565,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGB666,_666,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGB888,_888,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGBA32,_a32,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGBA16,_a16,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGBA565,_a565,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_YUV420,_yuv,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_YUV422,_yuv422,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_YUV422PLANAR,_yuv422planar,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_3D32,_3d32,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_4BPP,_4bpp,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_8BPP,_8bpp,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_RGB2X9,_2x9,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_TFORMAT,_tf,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_BGR888,_bgr,$(LIB_NAME),$(DEFINES_GLOBAL))
LIB_NAME := $(call change_name,USE_YUV_UV,_yuvuv,$(LIB_NAME),$(DEFINES_GLOBAL))

# ARTS module dependency
LIB_MODDEPS := helpers/vc_pool
