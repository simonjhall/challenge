/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/******************************************************************************
Includes (general).
******************************************************************************/

#include <assert.h>
#include <stdlib.h>

#include "helpers/vclib/vclib.h"
#include "vcfw/rtos/rtos.h"

/******************************************************************************
Includes (VMCS).
******************************************************************************/

/******************************************************************************
Includes (MediaPlayer).
******************************************************************************/

#include "list.h"

/******************************************************************************
Private defines.
******************************************************************************/

/******************************************************************************
Private types.
******************************************************************************/

/******************************************************************************
Static data.
******************************************************************************/

/******************************************************************************
Static functions.
******************************************************************************/

/******************************************************************************
Global data.
******************************************************************************/

/******************************************************************************
Global functions.
******************************************************************************/


/*
** Frame list handling functions
**
** This function is used to move frames around the linked lists safely.  The short circuit cases are:
** Remove a frame from the start of a list
** Add a frame to the end of a list
** Should be thread / multiprocessor safe
*/

struct VCLIB_LIST_S_
{
   RTOS_LATCH_T lock;          // Lock for whole list

   VCLIB_LIST_ELEMENT_T *head;    // Head of list (head->prev is last item)
   unsigned int count;         // Count of items in list (useful to keep)
};

/* List creation
**
** This function is used to create a list of n-frames
*/
VCLIB_LIST_T vclib_list_create(int count, size_t size, const char *name)
{
   VCLIB_LIST_T list;
   unsigned char *memblock;
   int i;

   memblock = vclib_prioritycalloc(sizeof(struct VCLIB_LIST_S_) + count * size, VCLIB_ALIGN_DEFAULT, VCLIB_PRIORITY_UNIMPORTANT, name);
   if (memblock == NULL)
      return NULL;

   list = (VCLIB_LIST_T)memblock;

   list->lock  = rtos_latch_unlocked();
   list->head  = NULL;
   list->count = 0;

   // Add the items to the list
   for (i = 0; i < count; i++)
   {
      VCLIB_LIST_ELEMENT_T *ptr = (VCLIB_LIST_ELEMENT_T *)(memblock + sizeof(struct VCLIB_LIST_S_) + i*size);
      vclib_list_move(ptr, NULL, list);
   }
   return list;
}

/* List destruction
**
*/
void vclib_list_destroy(VCLIB_LIST_T list)
{
   rtos_latch_get(&list->lock);

   free(list);
}


/* Move list
**
** This function is used to move items from a src_list to the dst_list
**
** If src_list is not NULL frame will be removed from it (it must be in the list)
** If dst_list is not NULL frame will be added to it (it must not be in the list)
**
** The short circuit cases are frames being added to the end of the list and frame being
** removed from the start of the list
**
** The list is organised in a complete loop with the head pointer pointing to the first item
** so head->prev is the end of the list
*/
void vclib_list_move(VCLIB_LIST_ELEMENT_T *element, VCLIB_LIST_T src_list, VCLIB_LIST_T dst_list)
{

   if (src_list)
   {
      rtos_latch_get(&src_list->lock);

      assert(element->next);
      assert(element->prev);

      assert(src_list->head);

#ifndef NDEBUG
      {
         VCLIB_LIST_ELEMENT_T *p;
         int i = 0, c = 0;

         p = src_list->head;
         do
         {
            c++;
            if (p == element)
               i++;
            p = p->next;
         }
         while (p != src_list->head);
         // The element MUST be in the src_list
         assert(i == 1);
         assert(c == src_list->count);
      }
#endif

      // if this is the only element in the list
      if (element->next == element)
      {
         src_list->head = NULL;
      }
      else
      {
         // remove frame
         element->prev->next = element->next;
         element->next->prev = element->prev;

         // Move head pointer if required
         if (src_list->head == element)
            src_list->head = element->next;
      }

      element->next = NULL;
      element->prev = NULL;

      src_list->count --;

      rtos_latch_put(&src_list->lock);
   }

   if (dst_list)
   {
      rtos_latch_get(&dst_list->lock);

      assert(element->next == NULL);
      assert(element->prev == NULL);

      if (dst_list->head == NULL)
      {
         dst_list->head = element;
         element->next = element;
         element->prev = element;
      }
      else
      {
         VCLIB_LIST_ELEMENT_T *last = dst_list->head->prev;

         element->prev = last;
         element->next = dst_list->head;
         last->next = element;
         dst_list->head->prev = element;
      }

#ifndef NDEBUG
      {
         VCLIB_LIST_ELEMENT_T *p;
         int i = 0, c = 0;
         p = dst_list->head;
         do
         {
            c++;
            if (p == element)
               i++;
            p = p->next;
         }
         while (p != dst_list->head);
         // The element MUST be in the dst_list
         assert(i == 1);
         assert(c == dst_list->count + 1);
      }
#endif


      dst_list->count ++;

      rtos_latch_put(&dst_list->lock);
   }
}

// Function to return the head item
VCLIB_LIST_ELEMENT_T * vclib_list_head(VCLIB_LIST_T list)
{
   return list->head;
}

VCLIB_LIST_ELEMENT_T * vclib_list_next(VCLIB_LIST_T list, VCLIB_LIST_ELEMENT_T * item)
{
   if (item == list->head->prev)
      return NULL;
   else
      return item->next;
}

int vclib_list_count(VCLIB_LIST_T list)
{
   return list->count;
}

/******************************************************************************
Static functions defined below.
******************************************************************************/

