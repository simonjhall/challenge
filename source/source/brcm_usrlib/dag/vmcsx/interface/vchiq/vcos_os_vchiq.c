/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

=============================================================================*/

#include "interface/vchiq/os.h"


/***********************************************************
 * Name: os_thread_begin
 *
 * Arguments:  VCOS_THREAD_T *thread
 *             OS_THREAD_FUNC_T func
 *             int stack
 *             void *data
 *             const char *name
 *
 * Description: Routine to create and start a thread or task for VCHIQ
 * Thread function has different arguments (void*) to os_thread_start (int,void*).
 * 
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
void os_thread_begin(VCOS_THREAD_T *thread, void (*func)(void*), int stack, void *data, const char *name) 
{
   VCOS_THREAD_ATTR_T attrs;
   VCOS_STATUS_T status;
   vcos_thread_attr_init(&attrs);
   vcos_thread_attr_setstacksize(&attrs, stack);
   vcos_thread_attr_setpriority(&attrs, 5); /* FIXME: should not be hardcoded */
   vcos_thread_attr_settimeslice(&attrs, 20); /* FIXME: should not be hardcoded */

   status = vcos_thread_create(thread, name, &attrs, (VCOS_THREAD_ENTRY_FN_T)func, data);
   vcos_assert(status == VCOS_SUCCESS);
}

void os_cond_init(void)
{
}
