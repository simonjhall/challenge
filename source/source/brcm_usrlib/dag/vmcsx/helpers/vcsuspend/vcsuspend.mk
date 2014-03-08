LIB_NAME    := vcsuspend_$(RTOS)
LIB_SRC     := 
LIB_LIBS    := helpers/sysman/sysman

LIB_IPATH   := 
LIB_CFLAGS  := $(WARNINGS_ARE_ERRORS)

LIB_SRC     += vcsuspend.c

ifneq (,$(VCSUSPEND_WANT_TEST_THREAD))
   LIB_SRC     += vcsuspend_test.c
   LIB_DEFINES += VCSUSPEND_WANT_TEST_THREAD
   LIB_NAME := $(call change_name,VCSUSPEND_WANT_TEST_THREAD,_test_thread,$(LIB_NAME),$(LIB_DEFINES))
endif

ifneq (,$(VCLIB_SECURE_HASHES))
	DEFINES_GLOBAL += VCLIB_SECURE_HASHES
	LIB_NAME := $(call change_name,VCLIB_SECURE_HASHES,_sechash,$(LIB_NAME),$(DEFINES_GLOBAL))
endif

ifeq (vciii/2707/2707,$(VCFW_TARGET_CHIP))
   VCSUSPEND_ASM_FLAVOUR ?= vc3
endif

ifeq (vciv/2708/2708,$(VCFW_TARGET_CHIP))
   VCSUSPEND_ASM_FLAVOUR ?= vc4
endif

# if we didn't set a flavour by the mechanism above, simply stub out the secure assembler function
VCSUSPEND_ASM_FLAVOUR ?= stub

LIB_SRC += vcsuspend_asm_$(VCSUSPEND_ASM_FLAVOUR).s
