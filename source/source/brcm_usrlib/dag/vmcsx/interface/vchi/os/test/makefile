#/*=============================================================================
#Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.#
#
#Project  :  
#Module   :  
#File     :  $RCSfile: powerman.c,v $
#Revision :  $Revision: #4 $
#
#FILE DESCRIPTION: linux make file for osal_test harness
#=============================================================================*/

cc=gcc
ldflags=-g -lpthread

IPATH=-I../../../../ -I../../../../../../vc3_vmcsx_dev_jnah

ccflags= -g  ${IPATH}


osal_test : osal_test.o linux_os.o
	${cc} ${ldflags} -o osal_test osal_test.o linux_os.o

osal_test.o : osal_test.c
	${cc} ${ccflags} -c osal_test.c

linux_os.o : ../linux/linux_os.c
	${cc} ${ccflags} -c ../linux/linux_os.c
	
	
