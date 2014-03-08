	include $(MSP_HOME)/dag/vmcsx/applications/cc_flags_test.mk

	INCPATH			:=	$(addprefix $(MSP_HOME)/,						\
							$(addprefix dag/vmcsx/,						\
								.										\
								middleware/khronos/common				\
								middleware/khronos/common/2708/platform	\
								helpers/vc_image						\
								vcinclude								\
								vcinclude/dummy							\
								interface/vcos/mobcom_rtos				\
								vcfw									\
							)											\
						)
	INCPATH			+=	$(STD_INCPATH)
					
	COMMON_VPATH	:=	test/khronos

	SRC32S			:=	khronos_test.c

	LIB_SUBDIRS		:=	test/bcmcomp_test	
	LIB_SUBDIRS		+=	test/khronos/cubes			
	LIB_SUBDIRS		+=	test/khronos/gl20test		
	LIB_SUBDIRS		+=	test/khronos/sw4047			
	LIB_SUBDIRS		+=	test/khronos/ogles20_game	
	LIB_SUBDIRS		+=	test/khronos/openvg_test	
	LIB_SUBDIRS		+=	test/khronos/draw_triangles	
	LIB_SUBDIRS		+=	test/khronos/gl20throughput	
	LIB_SUBDIRS		+=	test/khronos/blendtest
	LIB_SUBDIRS		+=	vmcs/khronos
