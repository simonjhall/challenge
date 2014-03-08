#/*=============================================================================
#Copyright (c) 2009 Broadcom Europe Limited. All rights reserved.#
#
#Project  :  
#Module   :  
#File     :  $$
#Revision :  $$
#
#FILE DESCRIPTION: Make rules for either gcc (mingw) or VCDEV
#
#=============================================================================*/

# Note: automatic dependency checking only works if using gcc.

# You will need to change this compiler setting is not using the standard cl
COMPILER ?= cl
#COMPILER = gcc

RTOS=win32

IPATH+=-I../../.. -I../win32

ifeq ($(COMPILER),cl)
OBJEXT=obj
else
OBJEXT=o
endif

#
# Rules for MSVC
#

ifeq ($(COMPILER),cl)
CC=cl
ldflags=/Zi
CFLAGS=/Zi /MDd /c ${DEFINES} ${IPATH} -Dmphi_get_func_table=local_mphi_get_func_table
define compile_cmd
	$(CC) $(CFLAGS) /c /Fo$@ $<
endef
define link_cmd
	${CC} ${ldflags} /Fe$@ $(OBJECTS) /link ../$(RTOS)/libvcos.a
endef
define link_cmd_nolibs
	${CC} ${ldflags} /Fe$@ $(OBJECTS)
endef

#
# Rules for gcc
#

else

CC=gcc
ldflags=-g -mno-cygwin -L../$(RTOS)
# This redefines the mphi func table to be the on in win32.c (linux_mphi_get_func_table)
CFLAGS= -Werror -MD -g -c $(DEFINES) ${IPATH} -O2 -mno-cygwin -Wall -Dmphi_get_func_table=local_mphi_get_func_table
CPPFLAGS= -Werror -MD -g -c $(DEFINES) ${IPATH} -O2 -mno-cygwin -Wall -Dmphi_get_func_table=local_mphi_get_func_table

ifdef NDEBUG
CFLAGS += -O2 -DNDEBUG
else
CFLAGS += -O1
endif

define compile_cmd
	$(CC) -c -o $@ $< $(CFLAGS)
endef
define link_cmd
	${CC} ${ldflags} -o $@ $(OBJECTS) -lvcos
endef
define link_cmd_nolibs
	${CC} ${ldflags} -o $@ $(OBJECTS)
endef

-include *.d

endif

%.obj: %.c
	$(compile_cmd)

%.obj: %.cpp
	$(compile_cmd)

define clean_cmd
	rm -f *.o *.d
	rm -f *.obj
	rm -f *.pdb
	rm -f *.ilk
	rm -f *.suo
	rm -f comms_test.exe
endef





