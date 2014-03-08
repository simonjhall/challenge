#
# Makefile which builds BCM116x drivers
#

#
# IMPORTANT: The order of these is important.
#
#	Withint this directory, the order that objects are added to obj-y
#	will determine the order that the init functions are called.
#
EXTRA_CFLAGS := -fno-inline 
EXTRA_CFLAGS += -I$(LINUX_DIR)/include/linux/broadcom/omx
EXTRA_CFLAGS += -I$(LINUX_DIR)/include/linux/broadcom/vc03
EXTRA_CFLAGS += -DUSE_MSGIO	# to include user space msgio support (msgfifo)
EXTRA_CFLAGS += -DUSE_FILESYS

obj-$(CONFIG_BCM_VC03_B0) 		+= vc_drv.o 

# If the kernel make is called from the top level make, then CONVBMP will
# already be defined. If we call the kernel make directly, then it won't. So
# we define it for this case
#

CONVBMP ?= $(TOPDIR)/../bin/convbmp

%.c: %.bmp
	@echo Converting $< ...
	@$(CONVBMP) $(CONVBMP_FLAGS) -o $@ $<

