LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

USE_BMEM:= true

ifeq ($(strip $(BOARD_USES_BRALLOC)),true)
	USE_BRALLOC:= true
endif


LOCAL_CFLAGS:= -DLOG_TAG=\"V3d_Library\" -DBRCM_USE_BMEM

LOCAL_SHARED_LIBRARIES := libutils

LOCAL_SRC_FILES:= src/ghw_allocator_impl.cpp src/ghw_memblock.cpp src/ghw_composer_impl.cpp

ifeq ($(USE_BMEM),true)
	LOCAL_CFLAGS+= -DUSE_BMEM
endif

LOCAL_C_INCLUDES += $(LOCAL_PATH)/inc $(LOCAL_PATH)/src 

ifeq ($(USE_BRALLOC),true)
	LOCAL_CFLAGS+= -DUSE_BRALLOC -DUSE_HW_THREAD_SYNC -DSUPPORT_V3D_WORKLIST 
	LOCAL_SHARED_LIBRARIES += libbralloc
	LOCAL_C_INCLUDES += hardware/broadcom/bralloc hardware/broadcom/v3d/driver 
endif


LOCAL_MODULE:= libv3d

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
#include $(call first-makefiles-under,$(LOCAL_PATH))

