#/*=============================================================================
#Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.#
#
#Project  :  
#Module   :  
#File     :  $$
#Revision :  $$
#
#FILE DESCRIPTION: linux make file for linux mphi test harnesses for VC03B0
#
#  If using a vcms build VC03B0, use make comms_test.
#  mphi_test requires custom VC03B0 software
#=============================================================================*/

# You will need to change this toolchain folder to where ever your gcc (cross)compiler is
TOOLCHAIN_DIR=/home/jamesh/projects/mpglinux-2.0-prealpha3/tools/le_arm11_20080723/bin/

# You will need to change this compiler setting is not using the standard arm-linux-gcc 
CC=${TOOLCHAIN_DIR}arm-linux-gcc

ldflags=-g -lpthread

IPATH=-I../../ -I../../../ -I../../vmcs_host -I../../vmcs_host/linux

# This redefines the mphi func table to be the on in linux.c (linux_mphi_get_func_table)
CFLAGS= -g  ${IPATH} -Dlinux_mphi_get_func_table=mphi_get_func_table

VPATH=../low_level/linux:../os/linux:../common:../connections/videocore:../control_service:../message_drivers/videocore:../../../host_applications/framework/vmcs:..:../../vmcs_host:../../vmcs_host/linux

MPHI_OBJECTS=mphi_test.o linux_os.o linux.o 

COMMSTEST_OBJECTS=comms_test.o linux_os.o linux.o endian.o multiqueue.o non_wrap_fifo.o software_fifo.o single.o mphi.o vchi.o control_service.o host_msgfifo_wrapper.o vchost.o vcgencmd.o vcos.o vcmsgfifo.o vciface.o vcih.o


mphi_test : $(MPHI_OBJECTS)
	${CC} ${ldflags} -o mphi_test $(MPHI_OBJECTS)


comms_test : $(COMMSTEST_OBJECTS)
	${CC} ${ldflags} -o comms_test $(COMMSTEST_OBJECTS)

