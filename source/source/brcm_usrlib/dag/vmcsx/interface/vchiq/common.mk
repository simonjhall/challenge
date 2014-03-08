INCPATH			=	$(addprefix $(MSP_HOME)/,						\
						$(addprefix dag/vmcsx/,						\
							.										\
							helpers/vc_image						\
							vcinclude								\
							vcinclude/dummy							\
							interface/vcos/mobcom_rtos				\
							interface/vchi/os/vcfw					\
							interface/vchi/os/vcos					\
							vcfw									\
						)											\
					)												\
					$(STD_INCPATH)									\

COMMON_VPATH	:=	local
COMMON_VPATH	+=	../../applications/vmcs/vchi/test
COMMON_VPATH	+=	../../host_applications/vmcs/test_apps/vchi_host_test
COMMON_VPATH	+=	test/mobcom
COMMON_VPATH	+=	../vchi/os/vcfw
COMMON_VPATH	+=	../vchi/os/vcos

	SRC32S		:=	core.c util_vchiq.c shim.c vcfw_os.c vcos_os.c vcos_os_vchiq.c
	SRC32S		+=	local.c 
	SRC32S		+=	comms_test.c test_dc4.c dc4.c
