INCPATH			=	$(addprefix $(MSP_HOME)/,						\
						$(addprefix dag/vmcsx/,						\
							.										\
							helpers/vc_image						\
							vcinclude								\
							vcinclude/dummy							\
							interface/vcos/mobcom_rtos				\
							vcfw									\
						)											\
					)												\
					$(STD_INCPATH)
					
COMMON_VPATH	=	common os/vcos control_service os/vcfw connections/videocore

	SRC32S		:=	vchi.c 
#	SRC32S		+=	single.c 
#	SRC32S		+=	local_mphi.c 
	SRC32S		+=	control_service.c 
	SRC32S		+=	bulk_aux_service.c 
#	SRC32S		+=	mphi_ccp2.c
	SRC32S		+=	software_fifo.c 
	SRC32S		+=	endian.c 
	SRC32S		+=	multiqueue.c 
	SRC32S		+=	non_wrap_fifo.c
	SRC32S		+=	vcfw_os.c 
	SRC32S		+=	vcos_cv.c 
	SRC32S		+=	vcos_os.c

