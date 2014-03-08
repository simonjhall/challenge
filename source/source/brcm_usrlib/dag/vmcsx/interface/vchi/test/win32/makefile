#/*=============================================================================
#Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.#
#
#Project  :  
#Module   :  
#File     :  $File$
#Revision :  $Revision$
#
#FILE DESCRIPTION: win32 make file for win32 mphi test harnesses for VC03B0
#
#  If using a vcms build VC03B0, use make comms_test.
#  mphi_test requires custom VC03B0 software
#=============================================================================*/

# You will need to change this compiler setting if not using the standard cl
CC ?= cl

all: comms_test.exe mphi_test.exe

IPATH=-I../../../ -I../../../../ -I../../../vmcs_host -I../../../vmcs_host/win32
IPATH+=-I../../../vcos/win32

# This redefines the mphi func table to be the on in win32.c (linux_mphi_get_func_table)
DEFINES=-DNDEBUG -Dmphi_get_func_table=local_mphi_get_func_table -DVCHI_LOCAL_HOST_PORT -DWIN32 -DUSE_DISPLAY -DVCHI_TEST_SERVER=1

OBJECTS=comms_test.obj\
   win32_os.obj\
   mphi_local.obj\
   endian.obj\
   vchi.obj\
   control_service.obj\
   bulk_aux_service.obj\
   single.obj\
   mphi_ccp2.obj\
   multiqueue.obj\
   non_wrap_fifo.obj\
   test_long.obj\
   test_shrt.obj\
   test_cli.obj\
   test_svr.obj\
   test_tstb.obj\
   test_tstd.obj\
   test_tst0.obj\
   test_dc4.obj\
   long_short.obj\
   client_server.obj\
   bulkdata.obj\
   bulk.obj\
   dc4.obj\
   vcos_os.obj\
   vcos_win32.obj\
   vcos_cv.obj\
   vcos_generic_event_flags.obj \
   vcos_msgqueue.obj \
   vcos_mem_from_malloc.obj \
   vcos_joinable_thread_from_plain.obj \
   vcos_abort.obj \
   vcos_logcat.obj


VPATH=../../os/win32:../../common:../../connections/videocore:../../control_service:../../message_drivers/videocore:../../../../host_applications/framework/vmcs:../..:../../../vmcs_host:../../../vmcs_host/win32:..:../../../../applications/vmcs/vchi/test:../../../../host_applications/vmcs/test_apps/vchi_host_test:../../../../vcfw/drivers/chip/vciii/2707
VPATH+=../../os/vcos
VPATH+=../../../vcos/win32
VPATH+=../../../vcos/generic

#../../low_level/win32:


comms_test.exe : $(OBJECTS)
	$(link_cmd_nolibs)

mphi_test.exe : $(OBJECTS)
	$(link_cmd_nolibs)

include ../../../vcos/win32/rules.mk

# Too many warnings for now to be able to compiler with -Werror!
ifeq ($(COMPILER),gcc)
  CFLAGS += -Wno-error
endif


clean:
	rm -f *.obj *.d *.exe *.pdb
	$(clean_cmd)

