/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited.
All rights reserved.

Project  :  VCHI
Module   :  Endian-aware routines to read/write data

FILE DESCRIPTION

Implements a multiqueue.

The idea behind a multiqueue (MQ) is that we have a base pool of elements.
In addition, a MQ consists of a bunch of linked lists (which are effectively
treated as fifos).  At any point in time, within a MQ an element can be on
one and only one of these individual queues.

The elements will be a struct, externally defined.  The design is such that
arbitrary structs can be used, HOWEVER, we mandate that the first member of
the element struct be a pointer to ->next.

=============================================================================*/

#include <stdlib.h>
#include <string.h>

#include "multiqueue.h"
#include "interface/vchi/os/os.h"
#include "interface/vchi/vchi_cfg_internal.h"


/******************************************************************************
  Register fields
 *****************************************************************************/


/******************************************************************************
  Local typedefs
 *****************************************************************************/

// define this to perform strict queue sanity checking
//#define MQUEUE_VALIDITY_CHECK

struct vchi_mqueue_element {
   // clients may iterate over next without using the semaphore, so again
   // make sure that it's always coherent during vchi_mqueue_put (not safe
   // to do such an iteration if the queue may be changed in any other way).
   struct vchi_mqueue_element * volatile next;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   struct vchi_mqueue_element * volatile prev;
#endif
};

struct opaque_vchi_mqueue_t {
   OS_SEMAPHORE_T *sem;
   const char *name;
   // peek routines have a fast case that reads head[n] or tail[n] without the
   // semaphore. So we have to ensure it's always something sensible, even in
   // locked regions...
   struct vchi_mqueue_element * volatile *head;
   struct vchi_mqueue_element * volatile *tail;
   OS_COND_T *nonempty;
   int threadsafe;
   int nqueues;
   int nelements;
   int orphaned; // count number of orphaned elements
};

#if OS_ALIGN_DEFAULT == 0
#   define alloc_round(x) (((x) + 3) & ~3) // at least force word alignment
#else
#   define alloc_round(x) (((x) + (OS_ALIGN_DEFAULT-1)) & (~(OS_ALIGN_DEFAULT-1)))
#endif


/******************************************************************************
 Extern functions
 *****************************************************************************/


/******************************************************************************
 Static functions
 *****************************************************************************/

// validate the integrity of a multi-queue
#if defined(MQUEUE_VALIDITY_CHECK)
static void mqueue_validate( const VCHI_MQUEUE_T *mq );
#else
#define mqueue_validate(q) do {} while(0)
#endif

static void mqueue_put( VCHI_MQUEUE_T *mq, int to, struct vchi_mqueue_element *element );
static void mqueue_put_head( VCHI_MQUEUE_T *mq, int to, struct vchi_mqueue_element *element );
static int32_t mqueue_element_get( VCHI_MQUEUE_T *mq, int from, struct vchi_mqueue_element *target );


/******************************************************************************
 Static data
 *****************************************************************************/


/******************************************************************************
 Global Functions
 *****************************************************************************/

/* ----------------------------------------------------------------------
 * allocate space for a new multiqueue.  there will be a 'head' and
 * 'tail' pointer per queue, and a total of nelements.
 * -------------------------------------------------------------------- */
VCHI_MQUEUE_T *
vchi_mqueue_create( const char *name, int nqueues, int nelements, int element_size, OS_SEMAPHORE_T *sem )
{
   int32_t success;
   int i;
   unsigned char *ptr;
   VCHI_MQUEUE_T *mq;
   struct vchi_mqueue_element *element, *prev;

   // create new multiqueue struct
   uint32_t mq_size = alloc_round( sizeof(VCHI_MQUEUE_T) ); // basic mqueue struct
   if (!sem)
      mq_size += alloc_round( sizeof(OS_SEMAPHORE_T) ); // our semaphore;
   mq_size += alloc_round( nqueues * sizeof(struct vchi_mqueue_element *) ); // ->head array
   mq_size += alloc_round( nqueues * sizeof(struct vchi_mqueue_element *) ); // ->tail array
   mq_size += alloc_round( nqueues * sizeof(OS_COND_T) ); // ->nonempty array
   mq_size += alloc_round( nelements * element_size ); // elements

   mq = (VCHI_MQUEUE_T *)os_malloc( mq_size, OS_ALIGN_DEFAULT, name );
   os_assert( mq );
   if (mq == NULL) return NULL;
   memset( mq, 0, mq_size );
   mq->name = name;

   // setup pointers to the various arrays (little bit of dirty casting)
   ptr = (unsigned char *)mq + alloc_round( sizeof(VCHI_MQUEUE_T) );
   if (sem) {
      mq->sem = sem;
      mq->threadsafe = 0;
   }
   else {
      mq->sem = (OS_SEMAPHORE_T *)ptr; ptr += alloc_round( sizeof(OS_SEMAPHORE_T) );
      mq->threadsafe = 1;
   }
   mq->head = (struct vchi_mqueue_element **)ptr; ptr += alloc_round( nqueues * sizeof(struct vchi_mqueue_element *) );
   mq->tail = (struct vchi_mqueue_element **)ptr; ptr += alloc_round( nqueues * sizeof(struct vchi_mqueue_element *) );
   mq->nonempty = (OS_COND_T *)ptr; ptr += alloc_round( nqueues * sizeof(OS_COND_T) );

   if (!sem) {
      // create a semaphore for protecting this structure
#ifdef VCHI_MQUEUE_NANOLOCKS
      success = os_semaphore_create( mq->sem, OS_SEMAPHORE_TYPE_NANOLOCK );
#else
      success = os_semaphore_create( mq->sem, OS_SEMAPHORE_TYPE_SUSPEND );
#endif
      os_assert( success == 0 );
   }

   // create array of condvars for providing blocking
   for (i=0; i<nqueues; i++) {
      success = os_cond_create( &mq->nonempty[i], mq->sem );
      os_assert( success == 0 );
   }

   // keep track of how many queues and elements we have (for sanity checking)
   mq->nqueues   = nqueues;
   mq->nelements = nelements;

   // finally chain all elements to the queue 0
   element = (struct vchi_mqueue_element *)ptr;
   prev = NULL;
   for (i = 0; i < nelements - 1; i++) {
      element->next = (struct vchi_mqueue_element *)((char *) element + element_size);
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
      element->prev = prev;
      prev = element;
#endif
      element = element->next;
   }
   element->next = NULL;
   mq->head[0] = (struct vchi_mqueue_element *) ptr;
   mq->tail[0] = element;
   mq->orphaned = 0;

   // sanity check state of queue
   mqueue_validate( mq ); // mq is private: no need to take semaphore

   // done!
   return mq;
}

/* ----------------------------------------------------------------------
 * free structures associated with the given multiqueue
 * -------------------------------------------------------------------- */
int32_t
vchi_mqueue_destroy( VCHI_MQUEUE_T *mq )
{
   int i;
   if ( !mq ) return -1;

   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );
   for (i=0; i < mq->nqueues; i++)
      os_cond_destroy( &mq->nonempty[i] );
   if (mq->threadsafe)
      os_semaphore_destroy( mq->sem );
   os_free( mq );
   return 0;
}

/* ----------------------------------------------------------------------
 * return pointer to the first element of queue 'from' within the
 * given MQ
 * -------------------------------------------------------------------- */
void *
vchi_mqueue_peek( VCHI_MQUEUE_T *mq, int from, int block )
{
   struct vchi_mqueue_element *element;

   // sanitise inputs
   os_assert( from < mq->nqueues );

   if ( !block )
      return mq->head[from];

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   while ( (element = mq->head[from]) == NULL ) {
      int32_t success = os_cond_wait( &mq->nonempty[from], mq->sem );
      os_assert(success == 0);
   }

   //os_logging_message( "MQ peek(%d) element = %p", from, element );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return element;
}

/* ----------------------------------------------------------------------
 * return pointer to the last element of queue 'from' within the
 * given MQ
 * -------------------------------------------------------------------- */
void *
vchi_mqueue_peek_tail( VCHI_MQUEUE_T *mq, int from, int block )
{
   struct vchi_mqueue_element *element;

   // sanitise inputs
   os_assert( from < mq->nqueues );

   if ( !block )
      return mq->tail[from];

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   while ( (element = mq->tail[from]) == NULL ) {
      int32_t success = os_cond_wait( &mq->nonempty[from], mq->sem );
      os_assert(success == 0);
   }

   //os_logging_message( "MQ peek_tail(%d) element = %p", from, element );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return element;
}

/* ----------------------------------------------------------------------
 * return pointer to the first and last elements of queue 'from' within
 * the given MQ
 * -------------------------------------------------------------------- */
void
vchi_mqueue_limits( VCHI_MQUEUE_T *mq, int from, void **head, void **tail, int block )
{
   // sanitise inputs
   os_assert( from < mq->nqueues );

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   while ( (*head = mq->head[from]) == NULL && block ) {
      int32_t success = os_cond_wait( &mq->nonempty[from], mq->sem );
      os_assert(success == 0);
   }

   *tail = mq->tail[from];

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );
}

/* ----------------------------------------------------------------------
 * move an element from the head of 'from' to the tail of 'to'.
 * return a pointer to the moved element, or NULL if 'from' is empty
 * -------------------------------------------------------------------- */
void *
vchi_mqueue_move( VCHI_MQUEUE_T *mq, int from, int to )
{
   struct vchi_mqueue_element *element;

   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   // sanitise inputs
   os_assert( from < mq->nqueues && to < mq->nqueues );

   //os_logging_message( "MQ move1 from %d (head=%p tail=%p) to %d (head=%p tail=%p)",
   //from, mq->head[from], mq->tail[from],
   //to, mq->head[to], mq->tail[to] );

   // fetch the element at start of 'from'
   element = mq->head[from];
   if ( !element ) {
      // the 'from' list is empty
      // unlock the data structure
      if (mq->threadsafe)
         os_semaphore_release( mq->sem );
      return NULL;
   }

   // remove from beginning of 'from'
   mq->head[from] = element->next;
   if ( mq->head[from] == NULL ) {
      // we emptied this queue
      mq->tail[from] = NULL;
      //os_logging_message( "MQUEUE: _move --> obtain %d", from );
   }
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   else
      mq->head[from]->prev = NULL;
#endif

   // we will be adding 'element' to the end...
   element->next = NULL;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   element->prev = NULL;
#endif
   mq->orphaned++;

   mqueue_put( mq, to, element );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return element;
}

/* ----------------------------------------------------------------------
 * move an element from the head of 'from' to the tail of 'to'.
 * return a pointer to the moved element, or NULL if 'from' is empty
 * -------------------------------------------------------------------- */
void *
vchi_mqueue_move_head( VCHI_MQUEUE_T *mq, int from, int to )
{
   struct vchi_mqueue_element *element;

   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   // sanitise inputs
   os_assert( from < mq->nqueues && to < mq->nqueues );

   //os_logging_message( "MQ move1 from %d (head=%p tail=%p) to %d (head=%p tail=%p)",
   //from, mq->head[from], mq->tail[from],
   //to, mq->head[to], mq->tail[to] );

   // fetch the element at start of 'from'
   element = mq->head[from];
   if ( !element ) {
      // the 'from' list is empty
      // unlock the data structure
      if (mq->threadsafe)
         os_semaphore_release( mq->sem );
      return NULL;
   }

   // remove from beginning of 'from'
   mq->head[from] = element->next;
   if ( mq->head[from] == NULL ) {
      // we emptied this queue
      mq->tail[from] = NULL;
      //os_logging_message( "MQUEUE: _move --> obtain %d", from );
   }
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   else
      mq->head[from]->prev = NULL;
#endif

   // we will be adding 'element' to the start...
   element->next = NULL;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   element->prev = NULL;
#endif
   mq->orphaned++;

   mqueue_put_head( mq, to, element );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return element;
}

/* ----------------------------------------------------------------------
 * Find the entry 'target' on the 'from' queue and remove it.
 *
 * returns 0 on success, -1 otherwise (eg. if 'target' is not on
 * queue 'from').
 * -------------------------------------------------------------------- */
static int32_t
mqueue_element_get( VCHI_MQUEUE_T *mq, int from, struct vchi_mqueue_element *target )
{
   struct vchi_mqueue_element *element, *prev;

   // sanitise inputs
   os_assert( from < mq->nqueues && target );
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   if ( element->prev )
      element->prev->next = element->next;
   else
      mq->head[from] = element->next;

   if ( element->next )
      element->next->prev = element->prev;
   else
      mq->tail[from] = element->prev;
#else
   element = mq->head[from];
   prev    = NULL;

   // walk the queue looking for target
   while( element ) {
      if ( element == target )
         break;
      prev = element;
      element = element->next;
   }

   if ( !element ) {
      // didn't find the element in this queue
      os_assert(0);
      return -1;
   }

   // remove element from 'from'
   if ( prev )
      prev->next = element->next;
   else
      mq->head[from] = element->next;

   if ( element == mq->tail[from] )
      mq->tail[from] = prev;
#endif

   // element will be orphaned
   element->next = NULL;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   element->prev = NULL;
#endif
   mq->orphaned++;

   // done successfully!
   return 0;
}

/* ----------------------------------------------------------------------
 * Find the entry with 'address' on the 'from' queue and remove it.
 *
 * returns 0 on success, -1 otherwise (eg. if 'address' is not on
 * queue 'from').
 * -------------------------------------------------------------------- */
int32_t
vchi_mqueue_element_get( VCHI_MQUEUE_T *mq, int from, void *address )
{
   int32_t success;

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   success = mqueue_element_get( mq, from, (struct vchi_mqueue_element *)address );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return success;
}

/* ----------------------------------------------------------------------
 * Find the entry with 'address' on the 'from' queue and move it to
 * the 'to' queue.
 *
 * returns 0 on success, -1 otherwise (eg. if 'address' is not on
 * queue 'from').
 * -------------------------------------------------------------------- */
int32_t
vchi_mqueue_element_move( VCHI_MQUEUE_T *mq, int from, int to, void *address )
{
   int32_t success;

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   success = mqueue_element_get( mq, from, (struct vchi_mqueue_element *)address );
   if (success == 0)
      mqueue_put( mq, to, (struct vchi_mqueue_element *)address );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return success;
}

/* ----------------------------------------------------------------------
 * Find the entry with 'address' on the 'from' queue and move it to
 * the head of the 'to' queue.
 *
 * returns 0 on success, -1 otherwise (eg. if 'address' is not on
 * queue 'from').
 * -------------------------------------------------------------------- */
int32_t
vchi_mqueue_element_move_head( VCHI_MQUEUE_T *mq, int from, int to, void *address )
{
   int32_t success;

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   success = mqueue_element_get( mq, from, address );
   if (success == 0)
      mqueue_put_head( mq, to, address );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return success;
}

/* ----------------------------------------------------------------------
 * remove, and return pointer to, the head entry of 'from'.
 *
 * until the entry is re-added (with vchi_mqueue_put), it is
 * effectively orphaned.
 *
 * block == 0 --> non-blocking: if the queue is empty, return NULL
 * block != 0 --> sleep until there is an entry, never return NULL
 * -------------------------------------------------------------------- */
void *
vchi_mqueue_get( VCHI_MQUEUE_T *mq, int from, int block )
{
   struct vchi_mqueue_element *element;

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   while ( (element = mq->head[from]) == NULL && block ) {
      int32_t success = os_cond_wait( &mq->nonempty[from], mq->sem );
      os_assert(success == 0);
   }

   if ( element ) {

      // remove from beginning of 'from'
      mq->head[from] = element->next;
      if ( mq->head[from] == NULL ) {
         // we emptied this queue
         mq->tail[from] = NULL;
         //os_logging_message( "MQUEUE: _get --> obtain %d", from );
      }
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
      else
         mq->head[from]->prev = NULL;
#endif

      // the item will be orphaned
      element->next = NULL;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
      element->prev = NULL;
#endif
      mq->orphaned++;

   }

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );

   return element;
}

/* ----------------------------------------------------------------------
 * add the given entry to the tail of 'to'
 * -------------------------------------------------------------------- */
static void
mqueue_put( VCHI_MQUEUE_T *mq, int to, struct vchi_mqueue_element *element )
{
   int32_t success;

   os_assert(element->next == NULL);
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   os_assert(element->prev == NULL);
#endif

   if ( mq->tail[to] == NULL ) {

      // this list was empty... now will have a single entry
      mq->tail[to] = mq->head[to] = element;
      //os_logging_message( "MQUEUE: _put --> release on %d", to );
   } else {

      os_assert( mq->tail[to]->next == NULL ); // for sanity!
      mq->tail[to]->next = element;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
      element->prev = mq->tail[to];
#endif
      mq->tail[to] = element;
   }

   mq->orphaned--;
   success = os_cond_signal( &mq->nonempty[to], VC_TRUE );
   os_assert(success == 0);
}

/* ----------------------------------------------------------------------
 * add the given entry to the tail of 'to'
 * -------------------------------------------------------------------- */
void
vchi_mqueue_put( VCHI_MQUEUE_T *mq, int to, void *entry )
{
   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   mqueue_put( mq, to, (struct vchi_mqueue_element *)entry );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );
}

/* ----------------------------------------------------------------------
 * add the given entry to the head of 'to'
 * -------------------------------------------------------------------- */
void
mqueue_put_head( VCHI_MQUEUE_T *mq, int to, struct vchi_mqueue_element *element )
{
   int32_t success;

   os_assert(element->next == NULL);
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   os_assert(element->prev == NULL);
#endif

   if ( mq->tail[to] == NULL ) {

      // this list was empty... now will have a single entry
      mq->tail[to] = mq->head[to] = element;
      //os_logging_message( "MQUEUE: _put --> release on %d", to );
   } else {
      element->next = mq->head[to];
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
      os_assert( mq->head[to]->prev == NULL ); // for sanity!
      mq->head[to]->prev = element;
#endif
      mq->head[to] = element;
   }

   mq->orphaned--;
   success = os_cond_signal( &mq->nonempty[to], VC_TRUE );
   os_assert(success == 0);
}


/* ----------------------------------------------------------------------
 * add the given entry to the head of 'to'
 * -------------------------------------------------------------------- */
void
vchi_mqueue_put_head( VCHI_MQUEUE_T *mq, int to, void *entry )
{
   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   mqueue_put_head( mq, to, entry );

   // sanity check state of queue
   mqueue_validate( mq );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );
}

/* ----------------------------------------------------------------------
 * dump the given multiqueue structure in debug form
 * -------------------------------------------------------------------- */
void
vchi_mqueue_debug( VCHI_MQUEUE_T *mq )
{
   int i;

   // lock the data structure
   if (mq->threadsafe)
      os_semaphore_obtain( mq->sem );

   // sanity check state of queue
   mqueue_validate( mq );

   os_logging_message( "----------" );
   os_logging_message( "%s: nqueues=%d, nelements=%d, orphaned=%d",
                       mq->name, mq->nqueues, mq->nelements, mq->orphaned );

   for (i=0; i<mq->nqueues; i++) {
      int n = 0;
      struct vchi_mqueue_element *element = mq->head[i];
      while( element ) { n++; element = element->next; }
      if ( n == 0 ) continue;

      os_logging_message( "%s: queue %d has %d %s (head=0x%p, tail=0x%p)",
                          mq->name, i, n, n == 1 ? "entry" : "entries", mq->head[i], mq->tail[i] );
   }

   os_logging_message( "----------" );

   // unlock the data structure
   if (mq->threadsafe)
      os_semaphore_release( mq->sem );
}

/* ----------------------------------------------------------------------
 * Check that from each head of a queue you get to the tail
 *
 * expected to be called with queue semaphore held
 * -------------------------------------------------------------------- */
#if defined(MQUEUE_VALIDITY_CHECK)
static void
mqueue_validate( const VCHI_MQUEUE_T *mq )
{
   int i, j, count = 0;
   struct vchi_mqueue_element *element;
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
   struct vchi_mqueue_element *prev;
#endif

   os_assert( mq->orphaned >= 0 && mq->orphaned <= mq->nelements );

   for (i=0; i<mq->nqueues; i++) {

      if ( mq->head[i] ) {
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
         prev = NULL;
#endif
         count++;
         element = mq->head[i];
         while( element->next ) {
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
            os_assert( element->prev = prev );
            prev = element;
#endif
            element = element->next;
            count++;
         }
#ifdef VCHI_MQUEUE_DOUBLY_LINKED
         os_assert( element->prev = prev );
#endif
         os_assert( mq->tail[i] == element );
      } else {
         // if the head is NULL
         os_assert(mq->tail[i] == NULL);
      }

      // make sure queues are separate (ie. no duplicate head and tail pointers)
      for (j=i+1; j<mq->nqueues; j++) {
         os_assert( mq->head[i] != mq->head[j] || (mq->head[i] == NULL && mq->head[j] == NULL) );
         os_assert( mq->tail[i] != mq->tail[j] || (mq->tail[i] == NULL && mq->tail[j] == NULL) );
      }
   }

   os_assert( count + mq->orphaned == mq->nelements );

   // add logging about how many missing entries there currently are

}
#endif

/********************************** End of file ******************************************/
