LIB_NAME := platform_test

LIB_SRC  := main.c thread.c semaphore.c eventgroup.c timer.c library.c

LIB_VPATH  := 
LIB_IPATH  := 
LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS :=

EXTRA_RELEASE_DIRS := 

# auto pak file generation - this will add all the VLL's used by the
# application plus the following resources

PAKFILE_NAME  :=
PAKFILE_DIRS  := 
PAKFILE_FILES := 
PAKFILE_LIST  := 

# Give this host app a sensible name
HOST_APP_NAME := platform_test
