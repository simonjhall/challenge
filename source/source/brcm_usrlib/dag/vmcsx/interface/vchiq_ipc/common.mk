#include $(MSP_HOME)/dag/vmcsx/cc_flags.mk
#CXXFLAGS	+=  --c99

#INCPATH			=	$(addprefix $(MSP_HOME)/,						\
#						$(addprefix dag/vmcsx/,						\
#							.										\
#							helpers/vc_image						\
#							vcinclude								\
#							vcinclude/dummy							\
#							interface/vcos/mobcom_rtos				\
#							interface/vchi/os/vcfw					\
#							interface/vchi/os/vcos					\
#							vcfw									\
#						)											\
#					)												\
#					$(STD_INCPATH)									\

COMMON_VPATH	+=	../vchi/os/vcfw
COMMON_VPATH	+=	../vchi/os/vcos

SRC32S		:=	vcfw_os.c vcos_os.c vcos_os_vchiq.c
SRC32S		+=	vchiq_ipc.c 

