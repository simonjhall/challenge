/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :
Module   :
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision: #4 $

FILE DESCRIPTION: OS interface for Linux
=============================================================================*/

#include <asm/semaphore.h>
#include <linux/vmalloc.h>
#include <linux/kthread.h>
#include "interface/vchi/os/os.h"

#define MAX_EVENTS_IN_GROUP 32


/******************************************************************************
Private typedefs
******************************************************************************/

typedef struct tag_WAITING_TASK_T
{
   struct semaphore task_semaphore;
   uint32_t event_map;
   struct tag_WAITING_TASK_T *pnext;
} WAITING_TASK_T;

typedef struct
{
   struct semaphore protection_mutex;         // general MT protection
   uint32_t event_map;                       // Bitmap of the events fired
   WAITING_TASK_T *wait_list;                // Pointer to first item in linked list of waiting tasks/semaphores
} EVENT_GROUP_T;

/******************************************************************************
Static data
******************************************************************************/
static OS_SEMAPHORE_T os_semaphore_global;

/******************************************************************************
Static func forwards
******************************************************************************/

/******************************************************************************
Global functions
******************************************************************************/

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
   int32_t success = os_semaphore_create(&os_semaphore_global, OS_SEMAPHORE_TYPE_BUSY_WAIT);
   os_assert(success == 0);

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
#if 0
int32_t os_semaphore_obtain_global( void )
{
   return os_semaphore_obtain(&os_semaphore_global);
}
#endif
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
#if 0
int32_t os_semaphore_release_global( void )
{
   return os_semaphore_release(&os_semaphore_global);
}
#endif

/***********************************************************
 * Name: os_semaphore_create
 *
 * Arguments:  OS_SEMAPHORE_T *semaphore,
               const OS_SEMAPHORE_TYPE_T type
 *
 * Description: Routine to create a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_create(  OS_SEMAPHORE_T *semaphore,
                              const OS_SEMAPHORE_TYPE_T type )
{
   if (semaphore == NULL)
      return -1;

   struct semaphore *sema = os_malloc(sizeof(struct semaphore), 0, "");
   if (sema == NULL)
      return -1;

   sema_init(sema, 1);
   *semaphore = (OS_SEMAPHORE_T)sema;
   return 0;
}

/***********************************************************
 * Name: os_semaphore_destroy
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to destroy a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_destroy( OS_SEMAPHORE_T *semaphore )
{
   os_free( (void *) *semaphore );
   return 0;
}


/***********************************************************
 * Name: os_semaphore_obtain
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to obtain a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_obtain( OS_SEMAPHORE_T *semaphore )
{
   down((struct semaphore *)*semaphore);
   return 0;
}


/***********************************************************
 * Name: os_semaphore_release
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to release a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_semaphore_release( OS_SEMAPHORE_T *semaphore )
{
   up((struct semaphore *)*semaphore);
   return 0;
}


/***********************************************************
 * Name: os_semaphore_obtained
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to release a semaphore
 *
 * Returns: 1 if semaphore has been obtained, 0 if it has been released
 *
 ***********************************************************/
int32_t os_semaphore_obtained( OS_SEMAPHORE_T *semaphore )
{
   // OK, we need to see if this mutex is already in use. We can do
   // this by attempted to trylock it - if we succeed, it was originally unlocked,
   // but we then need to unlock it again.
   int32_t ret = 0;

   if (semaphore)
   {
      int r = down_trylock((struct semaphore *)*semaphore);

      if (r)
      {
         ret = 1;
      }
      else
      {
         // got the lock, but dont need it so release
         up((struct semaphore *)*semaphore);
      }
   }

   return ret;
}


/***********************************************************
 * Name: os_malloc
 *
 * Arguments: const uint32_t size - size of memory in bytes to allocate
 *
 * Description: Routine to allocate memory
 *
 * Returns: void *  ptr to alolocated memory, NULL if failed
 *
 ***********************************************************/
void *os_malloc( const uint32_t size, const uint32_t align, const char *name )
{
   uint32_t myalign = align < sizeof(void *) ? sizeof(void *) : align;
   void   * Start   = vmalloc(size + myalign + sizeof(void *) - 1); // Guarantee space for a void * bytes before an aligned size
   void * * Aligned = (void * *) (((unsigned int) Start + sizeof(void *) + myalign - 1) & ~(myalign - 1));
   os_assert((myalign & (myalign-1)) == 0);
   if (Start == NULL)
      return NULL;

   Aligned[-1] = Start; // We must free the unaligned pointer
   return (void *) Aligned;
}

/***********************************************************
 * Name: os_free
 *
 * Arguments: const void *ptr - ptr to free
 *
 * Description: Routine to free memory allocated by os_malloc
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_free( void *ptr )
{
   if (ptr == NULL)
      return 0;
   vfree( ((void * *) ptr)[-1] );
   return 0;
}

/***********************************************************
 * Name: os_delay
 *
 * Arguments: const uint32_t time_in_ms - time to delay in ms
 *
 * Description: Routine to delay execution of the current thread for x ms
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
#include <linux/delay.h>

int32_t os_delay( const uint32_t time_in_ms )
{
   mdelay(time_in_ms); // Technically we shoudl busy wait, but that's a daft idea for milliseconds ..
   return 0;
}
int32_t os_sleep( const uint32_t time_in_ms )
{
   mdelay(time_in_ms);
   return 0;
}


/***********************************************************
 * Name: os_time_init
 *
 * Arguments: void
 *
 * Description: Routine to initialize OS microsecond timer
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_time_init( void )
{
   int32_t success = 0;

   // Linux HT Timer needs no init.

   return success;
}

/***********************************************************
 * Name: os_time_term
 *
 * Arguments: void
 *
 * Description: Routine to terminate OS microsecond timer
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_time_term( void )
{
   int32_t success = 0;

   return success;
}

/***********************************************************
 * Name: os_get_time_in_usecs
 *
 * Arguments: void
 *
 * Description: Routine to get current time in microseconds
 *
 * Returns: uint32_t - current time in microseconds
 *
 ***********************************************************/
#include <linux/time.h>

uint32_t os_get_time_in_usecs( void )
{
   struct timeval tv;
   uint32_t tm;

   do_gettimeofday(&tv);
   tm =  (tv.tv_sec*1000000) + tv.tv_usec;
   return tm;
}

/***********************************************************
 * Name: os_logging_message
 *
 * Arguments: int level - which logging channel to use
              const char *format - printf-style formatting string
              ... - varargs
 *
 * Description: Routine to write message to log
 *
 * Returns: void
 *
 ***********************************************************/

void os_logging_message(const char *format, ...)
{
/*  // No logging level - hence logging messages can't be turned off - disabling until fixed

   va_list args;
   va_start(args, format);
   vprintk(format, args);
   va_end(args);
*/
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

typedef struct
{
   OS_THREAD_FUNC_T func;
   void *arg;
} FUNC_INFO_T;

static int forward_to_thread_func(void *v)
{
   FUNC_INFO_T *infop = v, info = *infop;
   os_free(infop);
   info.func(0, info.arg);
   return 0;
}


int32_t os_thread_start( VCOS_THREAD_T *thread, OS_THREAD_FUNC_T func, void *arg, uint32_t stack_size, const char *name )
{
   struct task_struct * task;
   FUNC_INFO_T *info = os_malloc(sizeof *info, 0, "forward_to_thread_func");
   if (!info)
      return -1;

   info->func = func;
   info->arg = arg;

   // We have to forward this on because thread function required by pthread
   // requires a different number of parameters to the function being passed in.
   task = kthread_run(forward_to_thread_func, info, name);
   if (IS_ERR(task))
      return -1;
   *thread = (VCOS_THREAD_T) task;
   return 0;
}

/***********************************************************
 * Name: os_thread_set_priority
 *
 * Arguments:  VCOS_THREAD_T thread
 *             uint32_t priority
 *
 * Description: Routine to set thread priority. priority should be < 16; lower numbers mean higher priority
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_thread_set_priority( VCOS_THREAD_T thread, uint32_t priority )
{
   /* todo */

   return 0;
}

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
 * Name: os_hisr_destroy
 *
 * Arguments:  OS_HISR_T *hisr
 *
 * Description: Routine to destroy a HISR
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_hisr_destroy ( OS_HISR_T *hisr )
{
   *hisr = (OS_HISR_T)NULL;

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
   if ( hisr != NULL )
   {
      ((OS_HISR_FUNC_T)*hisr)();
   }

   return 0;      // always succeeds
}


/***********************************************************
 * Name: os_yield
 *
 * Arguments:  void
 *
 * Description: Routine to yield to other threads
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_yield( void )
{
   yield();
   return 0;
}



/***********************************************************
 * Name: os_eventgroup_create
 *
 * Arguments:  OS_EVENTGROUP_T eventgroup
 *
 * Description: Routine to create an event group
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_eventgroup_create ( OS_EVENTGROUP_T *eventgroup, const char *name )
{
   int success = -1;

   EVENT_GROUP_T *event_group = os_malloc(sizeof(EVENT_GROUP_T), OS_ALIGN_DEFAULT, "");

   if (event_group)
   {
      // Create a mutex to protect the group
      sema_init(&event_group->protection_mutex, 1);

      // Clear all the events
      event_group->event_map = 0 ;

      event_group->wait_list = NULL;

      success = 0;

      *eventgroup = (OS_EVENTGROUP_T)event_group;
   }

   return success;
}

/***********************************************************
 * Name: os_eventgroup_destroy
 *
 * Arguments:  OS_EVENTGROUP_T eventgroup
 *
 * Description: Routine to destroy an event group
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_eventgroup_destroy ( OS_EVENTGROUP_T *eventgroup )
{
   WAITING_TASK_T *wt;

   if (eventgroup)
   {
      EVENT_GROUP_T *pevent = (EVENT_GROUP_T*)*eventgroup;

      down(&pevent->protection_mutex);

      // needs to check to see if anything is waiting I suppose...then delete
      wt = pevent->wait_list;
      while (wt)
      {
         WAITING_TASK_T *tmp = wt->pnext;

         // Ensure it sets off..
         up(&wt->task_semaphore);

         // get rid of it.
         os_free(wt);
         wt = tmp;
      };

      up(&pevent->protection_mutex);

      os_free(*eventgroup);
   }

   return 0;      // always succeeds, even if passed in event_group invalid
}

/***********************************************************
 * Name: os_eventgroup_signal
 *
 * Arguments:  OS_EVENTGROUP_T eventgroup
               int32_t index
 *
 * Description: Routine to signal a single element of an event group
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_eventgroup_signal ( OS_EVENTGROUP_T *eventgroup, int32_t index )
{
   os_assert(index >= 0 && index <= MAX_EVENTS_IN_GROUP);

   if (eventgroup)
   {
      WAITING_TASK_T *wt;
      EVENT_GROUP_T *pevent = (EVENT_GROUP_T*)*eventgroup;

      down(&pevent->protection_mutex);

      // Set the event index
      pevent->event_map |= (1 << index);

      // Check to see if any task(s) waiting at the moment.
      // and post them.
      wt = pevent->wait_list;

      while (wt)
      {
         wt->event_map = pevent->event_map;

         up(&wt->task_semaphore);
         wt = wt->pnext;
      }

      // We only reset the signal if it was consumed (ie there were task waiting)
      // otherwise we store it up ready

      if (pevent->wait_list)
         pevent->event_map &= !(1 << index);

      up(&pevent->protection_mutex);
   }

   return 0;      // always succeeds
}

/***********************************************************
 * Name: os_eventgroup_retrieve
 *
 * Arguments:  OS_EVENTGROUP_T eventgroup
               uint32_t *events
 *
 * Description: Routine to wait for one or more elements of an event group to trigger
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/

int32_t os_eventgroup_retrieve ( OS_EVENTGROUP_T *eventgroup, uint32_t *events )
{
   int success = -1;

   if (eventgroup)
   {
      WAITING_TASK_T *wt;
      EVENT_GROUP_T *pevent = (EVENT_GROUP_T*)*eventgroup;

      down(&pevent->protection_mutex);

      // First check to see if there are ANY events already waiting - if so return imediately

      if (pevent->event_map)
      {
         *events = pevent->event_map;

         pevent->event_map = 0;

         up(&pevent->protection_mutex);

//os_logging_message("os_eventgroup_retrieve - returned imediately()\n");

         success = 0;
      }
      else
      {
         // OK, no events at this stage, so need to wait for something to happen.
         // Put a WAITING_TASK instance for this task in to the wait list, then
         // wait on its semaphore.

         wt = vmalloc(sizeof(WAITING_TASK_T));

         if ( NULL == wt )
         {
            up(&pevent->protection_mutex);
         }
         else
         {
            sema_init(&wt->task_semaphore, 0);
            {
               WAITING_TASK_T **p;

               // Add object to the list
               wt->pnext = pevent->wait_list;
               pevent->wait_list = wt;

               // Finshed with updateing the event group
               up(&pevent->protection_mutex);

               // Now wait on the semaphore we just created.
               down(&wt->task_semaphore);

               // Now its happened, remove the wait object from the list
               down(&pevent->protection_mutex);

               // need to loop through, as the list may have changed during the wait.
               for ( p = (WAITING_TASK_T **)&pevent->wait_list; *p; p = &(*p)->pnext )
               {
                  if ( *p == wt )
                  {
                     // Found our item, unlink it out of list and break out - no further looping required
                     *p = (*p)->pnext;
                     break;
                  }
               }

               // Get rid of the semaphore in the wait object
//               sem_destroy(&wt->task_semaphore);

               // Get the event bit flag and assign to return
               *events = wt->event_map;

               up(&pevent->protection_mutex);

               // Get rid of our wait object
               vfree(wt);

               success = 0;
            }
         }
      }
   }

 //  os_logging_message("os_eventgroup_retrieve()\n");

   return success;
}

/***********************************************************
 * Name: os_count_semaphore_create
 *
 * Arguments:  OS_SEMAPHORE_T *semaphore,
               const OS_SEMAPHORE_TYPE_T type
 *
 * Description: Routine to create a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_count_semaphore_create(  OS_COUNT_SEMAPHORE_T *semaphore,
                                    const int32_t count,
                                    const OS_SEMAPHORE_TYPE_T type )
{
   int32_t success = -1;

   if( NULL != semaphore )
   {
      struct semaphore *sema = os_malloc(sizeof(struct semaphore), 0, "");

      if (sema)
      {
         sema_init(sema, count);
         *semaphore = (OS_COUNT_SEMAPHORE_T)sema;
         success = 0;
      }
   }

   return success;
}

/***********************************************************
 * Name: os_semaphore_destroy
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to destroy a counting semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_count_semaphore_destroy( OS_COUNT_SEMAPHORE_T *semaphore )
{
   if (!semaphore)
      return -1;

   os_free(*semaphore);
   return 0;
}


/***********************************************************
 * Name: os_count_semaphore_obtain
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to obtain a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_count_semaphore_obtain( OS_COUNT_SEMAPHORE_T *semaphore,
                                   const int32_t block )
{
   int32_t success = -1;

   if (semaphore)
   {
      if (block)
      {
         down((struct semaphore*)*semaphore);
         success = 0;
      }
      else
         success = down_trylock((struct semaphore*)*semaphore);
   }

   return success;
}


/***********************************************************
 * Name: os_count_semaphore_release
 *
 * Arguments:  OS_SEMAPHORE_T semaphore
 *
 * Description: Routine to release a semaphore
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_count_semaphore_release( OS_COUNT_SEMAPHORE_T *semaphore )
{
   int32_t success = -1;

   if (semaphore)
   {
      up((struct semaphore*)*semaphore);
      success = 0;
   }

   return success;
}

/***********************************************************
 * Name: os_cond_create
 *
 * Arguments:  OS_COND_T *condition
 *             OS_SEMAPHORE_T *semaphore
 *
 * Description: Routine to create a condition variable
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_create( OS_COND_T *condition, OS_SEMAPHORE_T *semaphore )
{
   return -1;
}

/***********************************************************
 * Name: os_cond_destroy
 *
 * Arguments:  OS_COND_T cond
 *
 * Description: Routine to destroy a condition variable
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_destroy( OS_COND_T *cond )
{
   return -1;
}

/***********************************************************
 * Name: os_cond_signal
 *
 * Arguments:  OS_COND_T *cond
 *
 * Description: Routine to signal at least one thread
 *              waiting on a condition variable. The
 *              caller must say whether they have obtained
 *              the semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_signal( OS_COND_T *cond, bool_t sem_claimed )
{
   return -1;
}

/***********************************************************
 * Name: os_cond_broadcast
 *
 * Arguments:  OS_COND_T *cond
 *             bool_t sem_claimed
 *
 * Description: Routine to signal all threads waiting on
 *              a condition variable. The caller must
 *              say whether they have obtained the semaphore.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_broadcast( OS_COND_T *cond, bool_t sem_claimed )
{
   return -1;
}

/***********************************************************
 * Name: os_cond_wait
 *
 * Arguments:  OS_COND_T *cond,
 *             OS_SEMAPHORE_T *semaphore
 *
 * Description: Routine to wait for a condition variable
 *              to be signalled. Semaphore is released
 *              while waiting. The same semaphore must
 *              always be used.
 *
 * Returns: int32_t - success == 0
 *
 ***********************************************************/
int32_t os_cond_wait( OS_COND_T *cond, OS_SEMAPHORE_T *semaphore )
{
   return -1;
}

/****************************** End of file ***********************************/


