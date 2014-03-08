# Include a file that isn't in CVS where local changes can be made, so that
# people don't trip each other up...
SRC_ROOT ?= ../..
-include $(SRC_ROOT)/applications/vmcs/local.mkinc

COMPILER ?= mw
PLATFORM ?= 2763db
RTOS     ?= yes
FILESYSTEM ?= filesys
DEBUG    ?= TRUE
OPT      ?= SIZE
PROFILE  ?= FALSE
MEDIA    ?= sdcard pakfs

# ---------------------------------------------------------------------------- #
# 1. Common config (see next block for VMCS application definition)

DEFAULT_APPS   ?= vmcs

# Make sure the components and variant directories get released
EXTRA_RELEASE_DIRS := components variants

# Note : DO NOT include func.c in the source list!
BASE_SOURCE    :=   vmcs.c vmcs_main.c

# Note: Do NOT include nucleus_ati/Plus here - use the RTOS variable above
#       Also do not include filesys/filesys here either; use FILESYSTEM
#       variable above
BASE_LIBS      :=   applications/vmcs/apps/handlers/handler_file        \
                    applications/vmcs/apps/handlers/handlers            \
                    applications/vmcs/bufman/bufman_server              \
                    applications/vmcs/cecservice/cec_server             \
                    applications/vmcs/dispserve/display_server          \
                    applications/vmcs/fileserv/file_server              \
                    applications/vmcs/gencmd/gencmd_server              \
                    applications/vmcs/hostreq/hostreq_server            \
                    applications/vmcs/ilcs/ilcs_server                  \
                    applications/vmcs/pmuserv/pmu_server                \
                    applications/vmcs/touchserv/touch_server            \
                    applications/vmcs/tvservice/tv_server               \
                    codecs/codecs                                       \
                    containers/containers_core                          \
                    helpers/pwl/pwl                                     \
                    helpers/vc_audio/vc_audio                           \
                    helpers/vc_image/vc_image                           \
                    helpers/vc_pool/image/vc_pool_image                 \
                    helpers/vc_pool/vc_pool                             \
                    helpers/vclib/vclib                                 \
                    helpers/vcmaths/vcmaths                             \
                    middleware/audioplus/audioplus                      \
                    middleware/dispmanx/dispmanx                        \
                    middleware/dlloader/dlloader                        \
                    middleware/ISP/static/isp_static                    \
                    middleware/openmaxil/il_core                        \
                    middleware/powerman/powerman                        \
                    middleware/rpc/rpc                                  


ifeq ($(findstring _vchiqa,$(PLATFORM)),_vchiqa)
USE_VCHIQ_ARM := yes
endif

ifeq ($(findstring _linux,$(PLATFORM)),_linux)
USE_VCHIQ_ARM  := yes
DEFINES_GLOBAL += WANT_ARM_LINUX SDCARD_PREFER_INTERRUPTS=0
MEDIA += host
BASE_LIBS      += helpers/arm_loader/arm_loader
BASE_LIBS      += interface/vchiq_arm/vchiq_vc interface/vchi/os/os
BASE_LIBS      += middleware/khronos/khronos
else
ifdef USE_VCHIQ_ARM
BASE_LIBS      += interface/vchiq_arm/vchiq_local interface/vchi/os/os
else
BASE_LIBS      += interface/vchiq/vchiq
endif
BASE_LIBS      += applications/vmcs/khronos/khronos_server
endif

ifeq ($(findstring WANT_RTSP,$(DEFINES_GLOBAL)),WANT_RTSP)
   BASE_LIBS += applications/vmcs/dataservice/dataservice
endif


CFLAGS         :=
LFLAGS_mw      := -Hnothread

VPATH          :=
IPATH          :=


# ------------------------------------------------------------------------------ #
# 2. Normal VMCS with all applications and codecs as VLLs
#
NAME           := vmcs
VARIANT        ?= all
SOURCE         := $(BASE_SOURCE)
LIBS           := $(BASE_LIBS)
# Embed the username in the build info
DEFINES        := USERNAME=$(USERNAME)
DEFINES_GLOBAL += FOR_VMCS
# Would be nice to remove this (just for FlashFX?)
DEFINES_GLOBAL += NAND_NOFORMAT
DEFINES_GLOBAL +=
DEFINES_GLOBAL += USE_YUV420 USE_RGBA565 USE_RGBA32

#ifdef LOGGING
DEFINES_GLOBAL += LOGGING
#endif

ifdef HOST_APP
VCHI_LOCAL_HOST_PORT := TRUE
DEFINES_GLOBAL += VCHI_LOCAL_HOST_PORT
endif

ifdef WANT_VCHI_TESTS
 DEFINES_GLOBAL += WANT_VCHI_TESTS
endif

ifdef WANT_BUFMAN
 DEFINES_GLOBAL += WANT_BUFMAN
endif

DEFINES_GLOBAL += TV_SERVICE CEC_SERVICE

# ARTS module dependency
APP_MODDEPS := middleware/slimbus


include $(SRC_ROOT)/makefiles/add_application.mk


# ------------------------------------------------------------------------------ #
# 3. Define all the possible VMCS applications, handlers and codecs

include $(SRC_ROOT)/applications/vmcs/components/audio.mk
include $(SRC_ROOT)/applications/vmcs/components/codecs.mk
include $(SRC_ROOT)/applications/vmcs/components/containers.mk
include $(SRC_ROOT)/applications/vmcs/components/handlers.mk
include $(SRC_ROOT)/applications/vmcs/components/il.mk
include $(SRC_ROOT)/applications/vmcs/components/imaging.mk
include $(SRC_ROOT)/applications/vmcs/components/other.mk


#------------------------------------------------------------------------------
# Common stuff below
#------------------------------------------------------------------------------

include $(SRC_ROOT)/makefiles/common.mk
# The End
