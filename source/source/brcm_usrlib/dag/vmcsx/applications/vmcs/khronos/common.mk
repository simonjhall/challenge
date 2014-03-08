	include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	INCPATH			:=	$(addprefix $(MSP_HOME)/,						\
							$(addprefix dag/vmcsx/,						\
								.										\
								helpers/vc_image						\
								vcinclude								\
								vcinclude/dummy							\
								interface/vcos/mobcom_rtos				\
								middleware/common						\
								middleware/khronos/common				\
								middleware/khronos/common/2708/platform	\
								middleware/khronos/vg/2708/platform		\
								middleware/khronos/include/EGL			\
								middleware/khronos/include/VG			\
								middleware/khronos/include/GLES			\
							)											\
						)												\
						$(STD_INCPATH)

	SRC32S			=	khronos_server.c khronos_main.c

ifeq ($(CLIENT_DIRECT), TRUE)
	INCPATH			+=	$(MSP_HOME)/dag/vmcsx/middleware/khronos/client/direct
else
	INCPATH			+=	$(MSP_HOME)/dag/vmcsx/middleware/khronos/client/vcos
endif

