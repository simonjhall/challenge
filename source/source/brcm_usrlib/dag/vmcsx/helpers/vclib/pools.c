/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Project includes */
#include "nucleus.h"


/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/



extern int vclib_associate_pools(NU_TASK *task, int pool);
extern NU_MEMORY_POOL *vclib_chk_app_pool(int external);
extern int vclib_associate_child_task(NU_TASK *ptask, NU_TASK *ctask);
extern void vclib_forget_child_task(NU_TASK *task);
extern int pool_create_app_pair(int int_size, int ext_size, char *partlabel );
extern int pool_delete_app_pair(int handle);
extern int pool_delete_app_pair(int handle);
extern int pool_handle(int pool_pair, int external);



/* Check extern defs match above and loads #defines and typedefs */
#include "helpers/vclib/vclib.h"

/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

void *dmalloc (size_t size, int pool);
void *drealloc (void *ret, size_t size, int pool);
void dfree (void* pointer);
void *app_malloc(size_t size, int pool);
int memory_pool_empty(NU_MEMORY_POOL *pool);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/

#define fatal_assert(x) vclib_fatal_assert(x, FATAL_WHITE|FATAL_SUB_BLACK)

#define NBR_TASK_POOLS 20

typedef struct {
   NU_TASK *task;
   int pool_handle;
} TASK_POOL_TABLE_T;

#define NBR_POOL_PAIRS 5
typedef struct {
   int   int_pool;
   int   ext_pool;
   int   in_use;
} POOL_PAIR_T;

// Application created pools
#define NBR_APP_POOLS (2 * NBR_POOL_PAIRS)
typedef struct {
   NU_MEMORY_POOL app_pool;
   void *pool_ptr;
   int in_use;
} APP_POOL_T;




/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/
static int delete_app_pool(int handle);
static int create_app_pool(size_t size, char *label, void *pointer);


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/

static int vclib_current_priority=MEMORY_STALL_THRESHOLD;
static TASK_POOL_TABLE_T task_pool_info[NBR_TASK_POOLS];
static POOL_PAIR_T app_pool_table[NBR_POOL_PAIRS];
static APP_POOL_T pools[NBR_APP_POOLS];


/******************************************************************************
Global function definitions.
******************************************************************************/

/******************************************************************************
NAME
   vclib_associate_pools

SYNOPSIS
   int vclib_associate_pools(task, pool)

   Associate a memory-pool-pair-handle with a task so that all allocations for
   this task and its children are made within the given pair of memory pools.
RETURN
   0 success
   -1 if no room in table

******************************************************************************/
int vclib_associate_pools(NU_TASK *task, int pool)
{
   int retval = -1;
   int i;
   for ( i = 0; i < NBR_TASK_POOLS; i++)
   {
      if ( task_pool_info[i].task == 0 )
      {
         task_pool_info[i].task = task;
         task_pool_info[i].pool_handle = pool;
         retval = 0;
         break;
      }
   }
   return retval;
}

/******************************************************************************
NAME
   vclib_chk_app_pool

SYNOPSIS
   NU_MEMORY_POOL* vclib_chk_app_pool(external)

RETURN
   Valid pool handle for the current task that matches the required
   internal/external selector.
   NULL if no pool pairs associated with current task.

******************************************************************************/
NU_MEMORY_POOL *vclib_chk_app_pool(int external)
{
   NU_MEMORY_POOL *retval = 0;
   int i;

   // Check task id
   NU_TASK *current_task = NU_Current_Task_Pointer();
   for ( i = 0; i < NBR_TASK_POOLS && current_task != NU_NULL; i++)
   {
      if ( task_pool_info[i].task == current_task )
      {
         int pool_hndl = pool_handle(task_pool_info[i].pool_handle, external);
         if ( pool_hndl != -1 )
         {
            retval = &pools[pool_hndl].app_pool;
         }
         break;
      }
   }

   return retval;

}

/******************************************************************************
NAME
   vclib_associate_child_task

SYNOPSIS
   int vclib_associate_child_task(parent_task, child_task)

   Ensures that memory allocations for the given child task are made from the
   same pair of pools as for the parent task.

RETURN
   0 for success
   -1 if no room for association

******************************************************************************/
int vclib_associate_child_task(NU_TASK *ptask, NU_TASK *ctask)
{
   int i, j;
   int retval = -1;
   /* Find parent */
   for ( i = 0; i < NBR_TASK_POOLS; i++)
   {
      if ( task_pool_info[i].task == ptask )
      {
         break;
      }
   }
   if ( i < NBR_TASK_POOLS )
   {
      /* Find empty slot or existing use */
      for ( j = 0; j < NBR_TASK_POOLS; j++)
      {
         if ( task_pool_info[j].task == (NU_TASK *)0 || task_pool_info[j].task == ctask )
         {
            task_pool_info[j].task = ctask;
            task_pool_info[j].pool_handle = task_pool_info[i].pool_handle;
            retval = 0;
            break;
         }
      }

      if ( j ==  NBR_TASK_POOLS )
      {
         retval = -1;
      }
   }
   else
   {
      /* Not finding the parent task is not yet considered a failure as the point
         is to allow the system to operate as before, with the extra pools used if required */
      retval = 0;
   }
   return retval;
}

/******************************************************************************
NAME
   vclib_forget_child_task

SYNOPSIS
   void vclib_forget_child_task(task)

   Remove task from pool info. Ignore if missing.

******************************************************************************/
void vclib_forget_child_task(NU_TASK *task)
{
   int i;
   for ( i = 0; i < NBR_TASK_POOLS; i++)
   {
      if ( task_pool_info[i].task == task )
      {
         task_pool_info[i].task = (NU_TASK *)0;
         task_pool_info[i].pool_handle = -1;
         break;
      }
   }
}

/******************************************************************************
NAME
   pool_create_app_pair

SYNOPSIS
   int pool_create_app_pair(size_t int_size, size_t ext_size, const char *partlabel )

   If either or both internal and external sizes  are greater than zero
   allocate a new pool of the given size and identify by label.
   Returns a pool 'handle' to be used when deleting the pool pair.

******************************************************************************/
int pool_create_app_pair(int int_size, int ext_size, char *partlabel )
{
   int      pair_handle;
   void     *pool_ptr;
   int      len = strlen(partlabel);;

   /* find free app pool pair*/
   for (pair_handle=0;pair_handle< NBR_POOL_PAIRS;pair_handle++)
   {
      if (!app_pool_table[pair_handle].in_use)
      {
         break;
      }
   }
   // NU call expects a label of 7 or less characters. The gencmd should have enforced this.
   if ( pair_handle < NBR_POOL_PAIRS && len > 0 && len < 8)
   {
      /* either internal,external or both pools may be created */
      if ( int_size > 0 )
      {
         pool_ptr = malloc_priority(int_size, VCLIB_ALIGN_DEFAULT, vclib_current_priority, "App Pool");
         if (!pool_ptr) {
            fatal_assert(0);
            return -1;
         }
         app_pool_table[pair_handle].int_pool = create_app_pool(int_size, partlabel, pool_ptr);
         if ( app_pool_table[pair_handle].int_pool == -1 )
         {
            free(pool_ptr);
            return -1;
         }
      }
      if ( ext_size > 0 )
      {
         pool_ptr = malloc_priority(ext_size, VCLIB_ALIGN_DEFAULT, vclib_current_priority-1, "App Pool");
         if (!pool_ptr) {
            fatal_assert(0);
            return -1;
         }
         app_pool_table[pair_handle].ext_pool = create_app_pool(ext_size, partlabel, pool_ptr);
         if ( app_pool_table[pair_handle].ext_pool == -1 )
         {
            pool_delete_app_pair(pair_handle);
            return -1;
         }
      }
      app_pool_table[pair_handle].in_use = 1;
   }
   else
   {
      pair_handle = -1;
   }
   return pair_handle;
}

/******************************************************************************
NAME
   pool_delete_app_pair

SYNOPSIS
   int pool_delete_app_pair(int handle)


   Return 0 for success, -1 if a pool being deleted was not empty

******************************************************************************/
int pool_delete_app_pair(int handle)
{
   int err = 0;

   assert(handle < NBR_POOL_PAIRS && handle >= 0);

   if ( app_pool_table[handle].in_use )
   {
      if ( (0 != delete_app_pool(app_pool_table[handle].int_pool) ) ||
            (0 != delete_app_pool(app_pool_table[handle].ext_pool) )   )
      {
         err = -1;
      }
      app_pool_table[handle].in_use = 0;
   }
   return err;
}
/******************************************************************************
NAME
   pool_handle

SYNOPSIS
   int pool_handle(int pool_pair, int external)

******************************************************************************/
int pool_handle(int pool_pair, int external)
{
   int retval = -1;
   if ( pool_pair != -1)
   {
      if (external)
      {
         retval = app_pool_table[pool_pair].ext_pool;
      } else {
         retval = app_pool_table[pool_pair].int_pool;
      }
   }
   return retval;
}


static int create_app_pool(size_t size, char *label, void *pool_ptr)
{
   int      handle;
   STATUS   status;

   /* find free app pool pair*/
   for (handle=0;handle< NBR_APP_POOLS;handle++)
   {
      if (!pools[handle].in_use)
      {
         break;
      }
   }
   if ( handle < NBR_APP_POOLS )
   {
      status = NU_Create_Memory_Pool(&pools[handle].app_pool, label, pool_ptr, size, 4, NU_FIFO);
      if ( status == NU_SUCCESS )
      {
         pools[handle].pool_ptr = pool_ptr;
         pools[handle].in_use = 1;
      }
      else
      {
         handle = -1;
      }
   }
   else
   {
      handle = -1;
   }
   return handle;
}

static int delete_app_pool(int handle)
{
   int err = 0;
   if ( pools[handle].in_use )
   {
      if (!memory_pool_empty(&pools[handle].app_pool) )
      {
         err = -1;
      }
      NU_Delete_Memory_Pool( &pools[handle].app_pool);
      free(pools[handle].pool_ptr);
      pools[handle].in_use = 0;
   }
   return err;
}
