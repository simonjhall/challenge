include $(MSP_HOME)/dag/vmcsx/cc_flags.mk

	CC32FLAGS	+=  --c99

	SRC32S		+= khrn_dispatch.c
	SRC32S		+= khrn_dispatch_gl11.c
	SRC32S		+= khrn_dispatch_gl20.c
	SRC32S		+= khrn_dispatch_glxx.c
	SRC32S		+= khrn_dispatch_vg.c
	SRC32S		+= khrn_dispatch_egl.c
#	SRC32S		+= khrn_dispatch_wf.c
