include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	CC32FLAGS		+=	-D__STDC__
	CC32FLAGS		+=	-D__STDC_VERSION__=199901L
	CC32FLAGS		+=	--cpp 

	SRC32S			+= vc_pool.c
