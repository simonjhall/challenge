/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/vcos/vcos.h"
#include <string.h>

/**
  * \file
  *
  * Named semaphores, primarily for VCFW.
  *
  * Does not actually work across processes; merely emulate the API.
  *
  * The client initialises a VCOS_NAMED_SEMAPHORE_T, but this merely
  * points at the real underlying VCIS_NAMED_SEMAPHORE_IMPL_T.
  *
  *      semaphore_t  ---\
  *                       ----- semaphore_impl_t
  *      semaphore_t  ---/
  *                     /
  *      semaphore_t  -/
  *
  */

/* Maintain a simple array of actual semaphores */
#define MAX_SEMS  10

/** Each actual real semaphore is stored in one of these. Clients just
  * get a structure with a pointer to this in it.
  */
typedef struct VCOS_NAMED_SEMAPHORE_IMPL_T
{
   VCOS_SEMAPHORE_T sem;                        /**< Actual underlying semaphore */
   char name[VCOS_NAMED_SEMAPHORE_NAMELEN];     /**< Name of semaphore, copied */
   unsigned refs;                               /**< Reference count */
} VCOS_NAMED_SEMAPHORE_IMPL_T;

static VCOS_MUTEX_T lock;
static VCOS_NAMED_SEMAPHORE_IMPL_T sems[MAX_SEMS];

VCOS_STATUS_T _vcos_named_semaphore_init(void)
{
   memset(sems,0,sizeof(sems));
   return vcos_mutex_create(&lock,"vcosnmsem");
}

void _vcos_named_semaphore_deinit(void)
{
   vcos_mutex_delete(&lock);
}

VCOS_STATUS_T
vcos_generic_named_semaphore_create(VCOS_NAMED_SEMAPHORE_T *sem, const char *name, VCOS_UNSIGNED count)
{
   VCOS_STATUS_T status;
   int i, name_len;

   vcos_mutex_lock(&lock);
   
   /* do we already have this semaphore? */

   for (i=0; i<MAX_SEMS; i++)
   {
      if (sems[i].refs && strcmp(name, sems[i].name) == 0)
      {
         sems[i].refs++;
         sem->actual = sems+i;
         sem->sem = &sems[i].sem;
         vcos_mutex_unlock(&lock);
         return VCOS_SUCCESS;
      }
   }

   /* search for unused semaphore */

   name_len = strlen(name);

   if (name_len >= VCOS_NAMED_SEMAPHORE_NAMELEN)
   {
      vcos_assert(0);
      vcos_mutex_unlock(&lock);
      return VCOS_EINVAL;
   }

   for (i=0; i<MAX_SEMS; i++)
   {
      if (sems[i].refs == 0)
      {
         status = vcos_semaphore_create(&sems[i].sem, name, count);
         if (status == VCOS_SUCCESS)
         {
            sems[i].refs = 1;
            strcpy(sems[i].name, name); /* already checked length! */
            sem->actual = sems+i;
            sem->sem = &sems[i].sem;
         }
         vcos_mutex_unlock(&lock);
         return status;
      }
   }

   vcos_mutex_unlock(&lock);

   return VCOS_ENOSPC;
}


void vcos_named_semaphore_delete(VCOS_NAMED_SEMAPHORE_T *sem)
{
   VCOS_NAMED_SEMAPHORE_IMPL_T *actual = sem->actual;
   vcos_mutex_lock(&lock);

   vcos_assert(actual->refs); /* if this fires, the semaphore has already been deleted */

   if (--actual->refs == 0)
   {
      vcos_semaphore_delete(&actual->sem);
   }
   vcos_mutex_unlock(&lock);
}


