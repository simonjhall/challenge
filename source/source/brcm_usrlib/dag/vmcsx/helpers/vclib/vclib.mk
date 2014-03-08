LIB_NAME    := vclib


LIB_SRC     := textlib.c         vclib_tmpbuf.c                                                    \
               imagelib.s        font16.s          font20.s         font32.s        smallfont.s    \
               median.s          motionest.s       textasm.s        softtext.s                     \
               vclib_stats_asm.s vclib_stats.c     memcpy.s         vclib_misc.c                   \
               memchr.s          c_library.c       memcmp.s         list.c                         \
               vclib_crc.c       vclib_crc_asm.s   vclib_hash.c                                    \
               vclib_rand.c      vclib_rand_asm.s  vclib_spread.c   vclib_spread_asm.s             \
               vclib_debug.c     vclib_debug_asm.s vclib_misc_asm.s vclib_interleave_asm.s

LIB_LIBS    := 

LIB_IPATH   :=
LIB_VPATH   :=
LIB_DEFINES := 
LIB_CFLAGS  := $(WARNINGS_ARE_ERRORS)
LIB_AFLAGS  :=

DEFINES_GLOBAL += VCLIB_AVAILABLE

ifeq ($(findstring VCLIB_SECURE_HASHES,$(DEFINES_GLOBAL)),VCLIB_SECURE_HASHES)
LIB_SRC     += hmac.c
LIB_LIBS    += middleware/crypto/crypto
endif
LIB_NAME := $(call change_name,VCLIB_SECURE_HASHES,_sechash,$(LIB_NAME),$(DEFINES_GLOBAL))
