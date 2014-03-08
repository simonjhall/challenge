/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

=============================================================================*/

#include "interface/vchi/os/os.h"

extern void os_cond_init(void);

typedef void (*LEGACY_ENTRY_T)(int argc, void *argv);

/******************************************************************************
Static data
******************************************************************************/

/* Note that os_semaphore_global_XXX() actually refers to a *mutex* - os_semaphore's
 * are uncounted mutexes, not counted semaphores.
 */
static OS_SEMAPHORE_T os_mutex_global;
#ifndef NDEBUG
static int vcos_os_inited;
#endif

/***********************************************************
 * Name: os_init
 *
 * Arguments: void
 *
 * Description: Routine to init the local OS stack - called from vchi_init
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_init( void )
{
   int success;

   vcos_init();
   os_cond_init();


   os_assert(!vcos_os_inited++);   /* should only be called once now */
   success = os_semaphore_create(&os_mutex_global, OS_SEMAPHORE_TYPE_SUSPEND);
   os_assert(success == 0);

   return success;
}

/***********************************************************
 * Name: os_thread_start
 *
 * Arguments:  VCOS_THREAD_T *thread
 *             OS_THREAD_FUNC_T func
 *             void *arg
 *             uint32_t stack_size
 *             const char *name
 *
 * Description: Routine to create and start a thread or task
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_thread_start( VCOS_THREAD_T *thread, OS_THREAD_FUNC_T func, void *arg, uint32_t stack_size, const char *name ) 
{
   VCOS_STATUS_T st;
   VCOS_THREAD_ATTR_T attrs;
   vcos_thread_attr_init(&attrs);
   vcos_thread_attr_setstacksize(&attrs, stack_size);
   vcos_thread_attr_settimeslice(&attrs, 1);

   /* Os_threads assume (argc, argv) as arguments, with argc=0. The default vcos thread
    * just passes in argv as the first argument. Use this to change it. This kludge
    * needs to be removed once os_thread_start() is completely withdrawn.
    */
   _vcos_thread_attr_setlegacyapi(&attrs,1);
   st = vcos_thread_create(thread, name, &attrs, (VCOS_THREAD_ENTRY_FN_T)func, arg);
   return (st==VCOS_SUCCESS?0:-1);
}

int32_t os_time_init( void )
{
   /* nothing to do - done in vcos_init() */
   return 0;
}

extern int32_t os_time_term( void )
{
   return 0;
}

/***********************************************************
 * Name: os_semaphore_obtain_global
 *
 * Arguments: void
 *
 * Description: obtain a global semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_obtain_global( void )
{
   return os_semaphore_obtain(&os_mutex_global);
}

/***********************************************************
 * Name: os_semaphore_release_global
 *
 * Arguments: void
 *
 * Description: release a global semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_release_global( void )
{
   return os_semaphore_release(&os_mutex_global);
}

#if VCOS_HAVE_LEGACY_ISR

#include "vcfw/rtos/rtos.h"
/***********************************************************
 * Name: os_hisr_create
 *
 * Arguments:  OS_HISR_T *hisr
 *             OS_HISR_FUNC_T func
 *             char *name
 *
 * Description: Routine to create a HISR
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_hisr_create ( OS_HISR_T *hisr, OS_HISR_FUNC_T func, const char *name )
{
   int success = -1;

   VCOS_HISR_T *nu_hisr = os_malloc(sizeof(VCOS_HISR_T)+VCOS_LEGACY_HISR_STACK_SIZE, OS_ALIGN_DEFAULT, name);

   if (nu_hisr) {
      void *stack = nu_hisr+1;
      if (vcos_legacy_hisr_create( nu_hisr, name, func, VCOS_LEGACY_HISR_PRI, stack, VCOS_LEGACY_HISR_STACK_SIZE ) == VCOS_SUCCESS)
         success = 0;

      *hisr = (OS_HISR_T)nu_hisr;
   }

   return success;
}

/***********************************************************
 * Name: os_hisr_activate
 *
 * Arguments:  OS_HISR_T *hisr
 *
 * Description: Routine to activate
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_hisr_activate ( OS_HISR_T *hisr )
{
   vcos_legacy_hisr_activate( (VCOS_HISR_T *)*hisr );

   return 0;
}

#else

/***********************************************************
 * Name: os_hisr_create
 *
 * Arguments:  OS_HISR_T *hisr
 *             OS_HISR_FUNC_T func
 *             char *name
 *
 * Description: Routine to create a HISR
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_hisr_create ( OS_HISR_T *hisr, OS_HISR_FUNC_T func, const char *name )
{
   *hisr = (OS_HISR_T)func;

   return 0;      // always succeeds
}

/***********************************************************
 * Name: os_hisr_create
 *
 * Arguments:  OS_HISR_T *thread
 *             OS_HISR_FUNC_T func
 *             char *name
 *
 * Description: Routine to activ
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_hisr_activate ( OS_HISR_T *hisr )
{
   ((OS_HISR_FUNC_T)*hisr)();

   return 0;      // always succeeds
}

#endif
