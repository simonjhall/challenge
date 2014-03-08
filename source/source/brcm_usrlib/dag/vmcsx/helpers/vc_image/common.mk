include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	CC32FLAGS		+=	-D__STDC__
	CC32FLAGS		+=	-D__STDC_VERSION__=199901L
	CC32FLAGS		+=	--cpp 

	INCPATH			+=	$(VMCSX_DIR)/helpers
				  
	SRC32S			+= 	vc_image.c
	SRC32S			+= 	vc_image_dummy.c
	SRC32S			+= 	vc_image_helper.c
	SRC32S			+= 	vc_image_im_utils.c
	SRC32S			+= 	vc_image_metadata.c
	SRC32S			+= 	vc_image_rgba32.c
	SRC32S			+= 	yuv_to_rgb.c

