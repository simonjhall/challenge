/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */


/****************************************************************************
* 
* This file is enabled when rtos.h defines LATCH_LOGGING_ENABLED
*
* There are two latch logging systems implemented here.  The SIMPLE_LOG local
* define is set by default, and chooses the simpler scheme.  This logs all
* acquire and release events to a single list.  There is no locking in this 
* list, so multiple entries may overwrite each other.
*
* The more complex scheme records sequences of latches taken by a
* thread, and complaining if they are taken out-of-order (eg a thread
* takes A then B, and some other thread takes B then A). This is made
* more complex by a couple of complications:
*   1. Latches are sometimes used as signals rather than mutexes, leading to false
*      positives. To avoid these, if a thread ever takes a latch twice without
*      releasing it, or releases a latch that it didn't take, then that latch
*      is marked as an event and not considered for conflicts.
*   2. A latch being taken with rtos_latch_try can cause deadlock when it is the
*      first latch taken, but not when it's the subsequent one.
*        
* Every so often when a latch it released, it will go through all the logged sequences
* checking for conflicts.  It's not done every time because it's fairly slow, and
* because if there's a "conflict" that actually involves an event, delaying the notification
* gives the event a chance to reveal its nature.
*  
* Conflicts are signalled by means of an assert, and the sequences of conflicting latches
* output by printf. 
* 
* In both schemes there are performance implications to this logging,
* the latter scheme has more overhead.
* 
*****************************************************************************/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vcinclude/common.h"
#include "spinlock.h"

#include "vcfw/rtos/rtos.h"

#include "interface/vcos/vcos.h"

#ifdef LATCH_LOGGING_ENABLED

#define SIMPLE_LOG

/******************************************************************************
  Local typedefs
 *****************************************************************************/

#ifdef SIMPLE_LOG

// each record is 28bytes, so 1<<15 records is about 900kb.
#define NUM_RECORDS (1<<15)

typedef enum {
   TAKEN,
   RELEASE,
} LATCH_ACTION_T;

typedef struct {
   LATCH_ACTION_T action;
   RTOS_LATCH_T *latch;
   uint32_t thread;
   const char *tname;
   const char *name;
   int try;
   uint32_t time;
} LATCH_RECORD_T;

unsigned int latch_records_next;
unsigned int latch_records_wrapped;
LATCH_RECORD_T latch_records[NUM_RECORDS+16];
static uint32_t log_latches = 0;
   
void add_record(LATCH_ACTION_T action, RTOS_LATCH_T *latch, const char * name, uint32_t try)
{
   VCOS_THREAD_T *thread;
   int entry = latch_records_next;
   latch_records_next = (latch_records_next+1)&(NUM_RECORDS-1);
   if(!latch_records_next)
      latch_records_wrapped++;

   latch_records[entry].action = action;
   latch_records[entry].thread = rtos_get_thread_id();

   thread = vcos_thread_current();
   latch_records[entry].tname = thread ? vcos_thread_get_name(thread) : NULL;
   latch_records[entry].latch = latch;
   latch_records[entry].name = name;
   latch_records[entry].try = try;
   latch_records[entry].time = thread ? rtos_getmicrosecs() : 0;
}


#else

#define MAX_TAKEN_LATCHES 40
#define MAX_LOGGED_LATCH_CHAINS 1000
#define MAX_LATCH_RECORDS 1024
#define MAX_THREAD_SLOTS 256

typedef struct
{
   RTOS_LATCH_T *latch;
   const char *name;
   uint32_t real;
} LATCH_INFO_T;


typedef struct
{
   uint32_t count;
   uint32_t touched; /* While building up lists, whether the size of the list is increasing
                     *  (if the list is decreasing, then we know we don't need to add it to the
                     *   global list, because if we needed to we'd have done it last time)
                     *  For the global list, it records whether the list has changed since the
                     *   last time they were checked for bad orderings */
   
   struct { LATCH_INFO_T* latch_info; uint32_t try;} taken_latches[MAX_TAKEN_LATCHES];
} LATCHLIST_T;


typedef LATCHLIST_T THREADSTUFF_VALUE_T;

typedef struct
{
   uint32_t thread_id;
   THREADSTUFF_VALUE_T value;
} THREADSTUFF_T;

typedef struct
{
   uint32_t count;
   LATCHLIST_T latchlists[MAX_LOGGED_LATCH_CHAINS];
} LOCK_ORDERING_T;






/******************************************************************************
  Static data
 *****************************************************************************/


static THREADSTUFF_T threadstuff[MAX_THREAD_SLOTS]; // Limited number of thread slots for now
static uint32_t used_thread_slots = 0;
static spinlock_t threadstuff_lock;

static uint32_t log_latches = 0;


static LATCH_INFO_T all_latches[MAX_LATCH_RECORDS];
static uint32_t all_latches_count = 0;
static spinlock_t all_latches_lock;


// Set these to lock addresses you're interested in logging more detailed gets/puts for.
static uint32_t l1 = 0x00000000;
static uint32_t l2 = 0x00000000;

static LOCK_ORDERING_T lock_ordering;
static spinlock_t lock_ordering_lock;


#endif




/******************************************************************************
  Static Functions
 *****************************************************************************/
 
static void log_taken_latch( RTOS_LATCH_T *latch, const char * name , uint32_t try);
static void log_released_latch( RTOS_LATCH_T *latch, const char * name );

#ifndef SIMPLE_LOG

static void report_latch_conflicts();

static void compact_latchlists();
static void copy_latchlists(LATCHLIST_T *dst, LATCHLIST_T *src);
static uint32_t bad_ordering(LATCHLIST_T * list1, LATCHLIST_T * list2);
static void log_sequence(LATCHLIST_T * seq);
static uint32_t latchlist_cmp(LATCHLIST_T *a, LATCHLIST_T *b);


static THREADSTUFF_VALUE_T * get_threadstuff_ptr();
static LATCH_INFO_T * get_latch_info(RTOS_LATCH_T * latch);

#endif

/******************************************************************************
  Global Functions
 *****************************************************************************/


/***********************************************************
* Name: rtos_latch_logging_latch_get
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to get
*       const char *name    - file/line of code calling this function
*
* Description: Routine to get a latch, and log it.
*
* Returns: void
*
***********************************************************/

void rtos_latch_logging_latch_get( RTOS_LATCH_T *latch, const char * name )
{
   log_taken_latch(latch, name, 0);
   rtos_latch_get_real(latch);
}


/***********************************************************
* Name: latch_logging_rtos_latch_put
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to put
*       const char *name    - file/line of code calling this function
*
* Description: Routine to put / release a latch, and log it.
*
* Returns: void
*
***********************************************************/
void rtos_latch_logging_latch_put( RTOS_LATCH_T *latch, const char * name )
{
   if ( NULL != latch )
   {
      log_released_latch(latch, name);
      rtos_latch_put_real(latch);
   }
}


/***********************************************************
* Name: latch_logging_rtos_latch_try
*
* Arguments:
*       RTOS_LATCH_T *latch - latch to get
*       const char *name    - file/line of code calling this function
*
* Description: Routine to attempt to get a latch, and if successful logs it.
*
* Returns: int - 0==got the lock
*
***********************************************************/
int32_t rtos_latch_logging_latch_try( RTOS_LATCH_T *latch, const char *name )
{  
   int32_t failed = rtos_latch_try_real(latch);
     
   if ( 0 == failed && log_latches)
   {
      log_taken_latch(latch, name, 1);
   }
   return failed;
}

/***********************************************************
* Name: rtos_latch_logging_init
*
* Arguments: (none)
*
* Description: Initialise the latch logging.
*
* Returns: void
*
***********************************************************/


void rtos_latch_logging_init()
{
#ifndef SIMPLE_LOG
   spin_lock_init( &threadstuff_lock );
   spin_lock_init( &lock_ordering_lock );
   spin_lock_init(&all_latches_lock);
   
   memset( &threadstuff, 0, sizeof( threadstuff ) );
   uint32_t i;
   for (i = 0 ; i < MAX_THREAD_SLOTS ; ++i)
   {
      threadstuff[i].thread_id = RTOS_INVALID_THREAD_ID;
   }
   memset( &lock_ordering, 0, sizeof( lock_ordering ) );
   memset( &all_latches, 0, sizeof( all_latches ));
#endif
}


/******************************************************************************
  Static Functions
 *****************************************************************************/


static void log_taken_latch( RTOS_LATCH_T *latch, const char * name , uint32_t try)
{
#ifdef SIMPLE_LOG

   add_record(TAKEN, latch, name, try);

#else
   if ( (uint32_t)latch == l1 || (uint32_t)latch == l2)
   {
      uint32_t thread_id = rtos_get_thread_id();
      printf("Thread %u got lock %p", thread_id, latch);
   }

   assert( !strncmp(name, "Loc:", 4) );
   LATCH_INFO_T * latch_info = get_latch_info(latch);
   if (!latch_info->real) return;
   THREADSTUFF_VALUE_T * latches = get_threadstuff_ptr();
   uint32_t i;
   for ( i = 0 ; i < latches->count ; ++i) // If we already have it, it's probably being used as an event, so mark it as not real.
   {
      if (latches->taken_latches[i].latch_info->latch == latch)
      {
         latches->taken_latches[i].latch_info->real = 0;
         return;
      }
   }
   if (latches->count == MAX_TAKEN_LATCHES)
   {
      assert(0);
   }
   else
   {
      latches->taken_latches[latches->count].latch_info = latch_info;
      latches->taken_latches[latches->count].try = try;
      latches->taken_latches[latches->count++].latch_info->name = name;
      latches->touched = 1;
   }
#endif
}


static uint32_t lists_added = 0;
static uint32_t latch_count = 0;

static void log_released_latch( RTOS_LATCH_T *latch, const char * name )
{
#ifdef SIMPLE_LOG

   add_record(RELEASE, latch, name, 0);

#else
   assert(latch);

   if ( (uint32_t)latch == l1 || (uint32_t)latch == l2 )
   {
      uint32_t thread_id = rtos_get_thread_id();
      printf( "Thread %u put lock %p", thread_id, latch);
   }
   
   assert( !strncmp(name, "Loc:", 4) );
   THREADSTUFF_VALUE_T * latches = get_threadstuff_ptr();
   if ( 0 == latches->count )
   {
      get_latch_info(latch)->real = 0;
      return;
   }


   latch_count++;
   if (latch_count % 16384 == 0)
   {
      report_latch_conflicts();
   }
   if ( latches->touched && latches->count > 1 && get_latch_info(latch)->real)
   {

      assert( latches->count );
      latches->touched = 0;
      spin_lock( &lock_ordering_lock );
      assert(lock_ordering.count < MAX_LOGGED_LATCH_CHAINS);
      if ( lock_ordering.count < MAX_LOGGED_LATCH_CHAINS )
      {
         uint32_t i;
         for ( i = 0 ; i < lock_ordering.count ; ++i )
         {
            uint32_t cmp = latchlist_cmp(&lock_ordering.latchlists[i], latches );
            if ( cmp )
            {
               if (cmp == 2)
               {
                  copy_latchlists(&lock_ordering.latchlists[i], latches);
                  lock_ordering.latchlists[i].touched = 1;
               }
               break;
            }
         }
         if ( i == lock_ordering.count)
         {
            // Didn't find one
            if ( ++lists_added % 128 == 0)
            {
               compact_latchlists();
            }
            copy_latchlists(&lock_ordering.latchlists[lock_ordering.count++], latches);
            lock_ordering.latchlists[lock_ordering.count - 1].touched = 1;
         }
      }
      spin_unlock( &lock_ordering_lock );
   }
   RTOS_LATCH_T * last_latch = latches->taken_latches[latches->count-1].latch_info->latch;
   if ( last_latch != latch )
   {
      // Latches being released out-of-order, or latches being used as events, or we didn't notice the latch being taken.
      int32_t i;
      for ( i = latches->count - 2 ; i >= 0; --i )
      {
         if ( latch == latches->taken_latches[i].latch_info->latch )
         {
            int32_t j;
            for ( j = i ; j < latches->count - 1 ; ++j)
            {
               latches->taken_latches[j] = latches->taken_latches[j+1];
            }
            latches->count--;
            return;
         }
      } 
      // We don't have this latch, so it's probably an event.
      get_latch_info(latch)->real = 0;

   }
   else
   {
      latches->count--;
   }   
#endif 
}

#ifndef SIMPLE_LOG

static LATCH_INFO_T * get_latch_info(RTOS_LATCH_T * latch)
{
   uint32_t i = (uint32_t)latch % MAX_LATCH_RECORDS;
   spin_lock(&all_latches_lock);
   while (all_latches[i].latch)
   {
      if (all_latches[i].latch == latch)
      {
         spin_unlock(&all_latches_lock);
         return &all_latches[i];
      }
      i++;
      if (i == MAX_LATCH_RECORDS)
      {
         i = 0;
      }
   }
   assert(all_latches_count < MAX_LATCH_RECORDS);
   LATCH_INFO_T * ret = &all_latches[i];
   all_latches_count++;
   ret->latch = latch;
   ret->name = NULL;
   ret->real = 1; // benefit of the doubt
   spin_unlock(&all_latches_lock);
   return ret;
}


static void log_sequence(LATCHLIST_T * seq)
{
   uint32_t j;
   for ( j = 0; j < seq->count ; ++j )
   {
      const char * n = seq->taken_latches[j].latch_info->name;
      if( strncmp(n, "Loc:", 4) )
      {
         n = "(location lost)";
      }
      printf("    %p (%d) [%s] (%d)\n", seq->taken_latches[j].latch_info->latch, *(seq->taken_latches[j].latch_info->latch), n, seq->taken_latches[j].latch_info->real);
   }
}

static uint32_t bad_ordering(LATCHLIST_T * list1, LATCHLIST_T * list2)
{
   int32_t pos = -1;
   uint32_t i;
   for ( i = 0 ; i < list1->count ; ++i)
   {
      if (list1->taken_latches[i].latch_info->real != 1)
      {
         continue;
      }
      int32_t j;
      if (!list1->taken_latches[i].try)
      {
         for ( j = 0 ; j < pos ; ++j)
         {
            if (list2->taken_latches[j].latch_info->real == 1 && list2->taken_latches[j].latch_info->latch == list1->taken_latches[i].latch_info->latch)
            {
               list2->taken_latches[j].latch_info->real++; // Only report once. Really, should just kill the pairing, but it's easier to kill an entire latch
               return 1;
            }
         }
      }
      for ( j = pos + 1 ; j < list2->count ; ++j )
      {
         if (list2->taken_latches[j].latch_info->real == 1 && !list2->taken_latches[j].try && list2->taken_latches[j].latch_info->latch == list1->taken_latches[i].latch_info->latch)
         {
            pos = j;
            break;
         }
      }
   }
   return 0;
}

static uint32_t latchlist_cmp(LATCHLIST_T *a, LATCHLIST_T *b) // Returns 1 if a==b, 2 if a is prefix of b, 0 otherwise. Ignores entries where real=0.
{
   assert(a && b);

   uint32_t a_pos = 0;
   uint32_t b_pos = 0;
   for (;;)
   {
      while (a_pos < a->count && !a->taken_latches[a_pos].latch_info->real)
      {
         a_pos++;
      }
      while (b_pos < b->count && !b->taken_latches[b_pos].latch_info->real)
      {
         b_pos++;
      }
      
      if (a_pos == a->count)
      {
         if (b_pos == b->count) return 1;
         return 2;
      }
      if (b_pos == b->count) return 0;
      
      if (a->taken_latches[a_pos].latch_info->latch != b->taken_latches[b_pos].latch_info->latch || a->taken_latches[a_pos].try != b->taken_latches[b_pos].try)
      {
         return 0;
      }
      
      a_pos++;
      b_pos++;
      
   }
}


static void copy_latchlists(LATCHLIST_T *dst, LATCHLIST_T *src)
{
   assert(dst);
   assert(src);
   uint32_t i;
   uint32_t real_count = 0;
   for (i = 0; i < src->count ; ++i)
   {
      if (src->taken_latches[i].latch_info->real)
      {
         dst->taken_latches[real_count++] = src->taken_latches[i];
      }
   }
   dst->count = real_count;
   dst->touched = src->touched;
}

static void compact_latchlists()
{

   uint32_t cur, new;
   cur = 0;
   new = 0;
   while (cur < lock_ordering.count)
   {
      uint32_t real_count = 0;
      uint32_t i;
      for (i = 0 ; i < lock_ordering.latchlists[cur].count ; ++i)
      {
         if (lock_ordering.latchlists[cur].taken_latches[i].latch_info->real)
         {
            real_count++;
         }
      }
      if (real_count >= 2)
      {
         uint32_t dup = 0;
         for (i = 0 ; i < new ; ++i)
         {
            if (latchlist_cmp(&lock_ordering.latchlists[cur], &lock_ordering.latchlists[i]) == 1)
            {
               dup = 1;
               break;
            }
         }
         if (!dup)
         {
            copy_latchlists(&lock_ordering.latchlists[new], &lock_ordering.latchlists[cur]);
            new++;
         }
      }
      cur++;
   }
   lock_ordering.count = new;
}

static void report_latch_conflicts()
{

   // O(n^2 m^2). Lovely.
   uint32_t ii,jj;
   for ( ii = 0 ; ii < lock_ordering.count ; ++ii)
   {
      if (!lock_ordering.latchlists[ii].touched)
      {
         continue;
      }
      for (jj = 0 ; jj < lock_ordering.count ; ++jj)
      {
         if ( lock_ordering.latchlists[jj].touched && jj <= ii) // Don't check this time for things that will be checked the other way round
         {
            continue;
         }
         if ( bad_ordering( &lock_ordering.latchlists[ii], &lock_ordering.latchlists[jj] ))
         {
            printf("Sequence:\n");
            log_sequence(&lock_ordering.latchlists[ii]);
            printf( "Conflicts with sequence:\n");
            log_sequence(&lock_ordering.latchlists[jj]);
            assert(0);
         }
      }
   }
   for ( ii = 0 ; ii < lock_ordering.count ; ++ii)
   {
      lock_ordering.latchlists[ii].touched = 0;
   }
}

static THREADSTUFF_VALUE_T * get_threadstuff_ptr()
{
   const uint32_t id = rtos_get_thread_id();
   uint32_t i = id % MAX_THREAD_SLOTS;
   
   spin_lock( &threadstuff_lock );
   while (threadstuff[i].thread_id != RTOS_INVALID_THREAD_ID)
   {
      if ( threadstuff[i].thread_id == id )
      {
         spin_unlock( &threadstuff_lock );
         return &threadstuff[i].value;
      }
      i++;
      if (i == MAX_THREAD_SLOTS)
      {
         i = 0;
      }
   }

   assert(used_thread_slots < MAX_THREAD_SLOTS);
   used_thread_slots++;
   threadstuff[i].thread_id = id;
   spin_unlock( &threadstuff_lock );
   return &threadstuff[i].value;
   
}
#endif

#endif // LATCH_LOGGING_ENABLED


// For binary compatability with stuff that doesn't have the new macros

#undef rtos_latch_get
#undef rtos_latch_put
#undef rtos_latch_try

#ifdef LATCH_LOGGING_ENABLED

void rtos_latch_get(RTOS_LATCH_T * latch)
{
   rtos_latch_logging_latch_get(latch, "Loc: (unknown)");
}
void rtos_latch_put(RTOS_LATCH_T * latch)
{
   rtos_latch_logging_latch_put(latch, "Loc: (unknown)");
}
int32_t rtos_latch_try(RTOS_LATCH_T * latch)
{
   return rtos_latch_logging_latch_try(latch, "Loc: (unknown)");
}

#else

void rtos_latch_get(RTOS_LATCH_T * latch)
{
   rtos_latch_get_real(latch);
}
void rtos_latch_put(RTOS_LATCH_T * latch)
{
   rtos_latch_put_real(latch);
}
int32_t rtos_latch_try(RTOS_LATCH_T * latch)
{
   return rtos_latch_try_real(latch);
}

#endif
