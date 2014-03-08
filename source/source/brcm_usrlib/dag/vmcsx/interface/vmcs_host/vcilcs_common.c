/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  VMCS-X
Module   :  OpenMAX IL Component Service

FILE DESCRIPTION
OpenMAX IL Component Service functions
Common Host side functions
=============================================================================*/

#include "interface/vmcs_host/khronos/IL/OMX_Component.h"

#include "interface/vmcs_host/vcilcs.h"
#include "interface/vmcs_host/vcilcs_common.h"

static IL_FN_T vcilcs_fns[] = {NULL, // response
                               NULL, // create component

                               vcil_in_get_component_version,
                               NULL, // send command
                               vcil_in_get_parameter,
                               vcil_in_set_parameter,
                               vcil_in_get_config,
                               vcil_in_set_config,
                               vcil_in_get_extension_index,
                               vcil_in_get_state,
                               NULL, // tunnel request
                               vcil_in_use_buffer,
                               NULL, // use egl image
                               NULL, // allocate buffer
                               vcil_in_free_buffer,
                               vcil_in_empty_this_buffer,
                               vcil_in_fill_this_buffer,
                               NULL, // set callbacks
                               NULL, // component role enum

                               NULL, // deinit

                               vcil_out_event_handler,
                               vcil_out_empty_buffer_done,
                               vcil_out_fill_buffer_done,

                               NULL, // component name enum
                               NULL, // get debug information

                               NULL
};

static ILCS_COMMON_T *vcilcs_common_init(ILCS_SERVICE_T *ilcs)
{
   ILCS_COMMON_T *st;

   st = vcos_malloc(sizeof(ILCS_COMMON_T), "ILCS_HOST_COMMON");
   if(!st)
      return NULL;

   if(vcos_semaphore_create(&st->component_lock, "ILCS", 1) != VCOS_SUCCESS)
   {
      vcos_free(st);
      return NULL;
   }

   st->ilcs = ilcs;
   st->component_list = NULL;
   return st;
}

static void vcilcs_common_deinit(ILCS_COMMON_T *st)
{
   vcos_semaphore_delete(&st->component_lock);

   while(st->component_list)
   {
      VC_PRIVATE_COMPONENT_T *comp = st->component_list;
      st->component_list = comp->next;
      vcos_free(comp);
   }

   vcos_free(st);
}

static void vcilcs_thread_init(ILCS_COMMON_T *st)
{
}

static unsigned char *vcilcs_mem_lock(OMX_BUFFERHEADERTYPE *buffer)
{
   return buffer->pBuffer;
}

static void vcilcs_mem_unlock(OMX_BUFFERHEADERTYPE *buffer)
{
}


void vcilcs_config(ILCS_CONFIG_T *config)
{
   config->fns = vcilcs_fns;
   config->ilcs_common_init = vcilcs_common_init;
   config->ilcs_common_deinit = vcilcs_common_deinit;
   config->ilcs_thread_init = vcilcs_thread_init;
   config->ilcs_mem_lock = vcilcs_mem_lock;
   config->ilcs_mem_unlock = vcilcs_mem_unlock;
}

