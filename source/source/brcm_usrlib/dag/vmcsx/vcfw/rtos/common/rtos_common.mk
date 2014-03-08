# RTOS Common makefile

LIB_NAME  := vcfw_rtos_common

#include the C-runtime library startup code
LIB_SRC   :=  rtos_common.c rtos_timer.c rtos_common_malloc.c rtos_common_secure.c rtos_common_secureasm.s rtos_common_asm.s rtos_common_mem.c rtos_common_mempool.c rtos_latch_logging.c rtos_resource_checking.c rtos_common_lib.c rtos_common_lib_asm.s fread.c setvbuf.c fwrite.c

LIB_VPATH :=
LIB_IPATH :=  .. ../../drivers/chip metaware_internal_headers
LIB_DEFINES :=
LIB_CFLAGS  := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS  :=

ifeq (VIDEOCORE4, $(findstring VIDEOCORE4, $(DEFINES_GLOBAL)))
 LIB_VPATH += ./vciv
 LIB_SRC  += hl_bios.c hl_data.c
else
 ifeq (VIDEOCORE3, $(findstring VIDEOCORE3, $(DEFINES_GLOBAL)))
  LIB_VPATH += ./vciii
 else
  LIB_VPATH += ./vcii
 endif
endif

LIB_LIBS    += interface/vcos/vcos

ifdef MEM_SHUFFLE
 ifneq (none, $(RTOS))
  LIB_DEFINES += SHUFFLE_INTERVAL_MSECS=$(MEM_SHUFFLE)
  LIB_NAME := $(LIB_NAME:%=%_shuffle$(MEM_SHUFFLE))
 endif
endif

ifdef MEM_PROTECT
 ifneq (,$(strip $(filter DEBUG debug,$(TARGET))))
  LIB_DEFINES += MEM_FREESPACE_PROTECT=$(MEM_PROTECT)
  LIB_NAME := $(LIB_NAME:%=%_protect$(MEM_PROTECT))
 endif
endif

ifdef MEM_CHECK_FREE
  LIB_DEFINES += MEM_AUTO_VALIDATE MEM_FILL_FREE_SPACE
  LIB_NAME := $(LIB_NAME:%=%_checkfree)
else
 ifdef MEM_CHECK
   LIB_DEFINES += MEM_AUTO_VALIDATE
   LIB_NAME := $(LIB_NAME:%=%_check)
 endif
endif
