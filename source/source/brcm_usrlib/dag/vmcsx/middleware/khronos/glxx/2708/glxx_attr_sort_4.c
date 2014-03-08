/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include "interface/khronos/common/khrn_int_common.h"
#include "interface/khronos/glxx/glxx_int_attrib.h"
#include "interface/khronos/glxx/glxx_int_config.h"
#include "middleware/khronos/glxx/2708/glxx_attr_sort_4.h"

#include <stdio.h>

struct entry{
   struct entry *prev;
   struct entry *next;
   uint32_t index;
};

typedef struct entry GLXX_ATTR_SORT_ENTRY_T;

/*************************************************************
 Defines
 *************************************************************/


/*************************************************************
 Static data
 *************************************************************/

static GLXX_ATTR_SORT_ENTRY_T order[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];


/*************************************************************
 Static function forwards
 *************************************************************/



/*************************************************************
 Global Functions
 *************************************************************/

//python pseudo code
//def do_sort():
//   global mergeable_attributes, order, enabled
//   i = 0
//   order = []
//   count_enabled = 0
//   for i in range(8): 
//      if enabled[i]:
//         count_enabled +=1
//         order += [i]
//   
//   i = 0   
//   while i<count_enabled:
//      done_swap = False
//      for j in range(i,count_enabled):
//         a = order[i]
//         b = order[j]
//         if a in mergeable_attributes[b]:
//            #j should go just before i
//            order = order[:i] + [b] + order[i:j] + order[j+1:]
//            done_swap = True
//            break               
//         if b in mergeable_attributes[a] and j>i+1:
//            #i should go just before j
//            order = order[:i+1] + [b] + order[i+1:j] + order[j+1:]
//            done_swap = True
//            break            
//      if not done_swap:
//         i+=1

// can't do a qsort as not all pairs of attributes have comparisons
//
void glxx_sort_attributes(GLXX_ATTRIB_T *attrib,
   uint32_t cattribs_live,
   uint32_t vattribs_live,
   uint32_t * mergeable_attribs,
   uint32_t * cattribs_order,
   uint32_t * vattribs_order
   )
{
   uint32_t i,j,m,last_vattrib = -1;
   uint32_t n = 0;
   GLXX_ATTR_SORT_ENTRY_T *a, *head;
   uint32_t cattribs_extra[GLXX_CONFIG_MAX_VERTEX_ATTRIBS];
   uint32_t num_cattribs_extra = 0;
   uint32_t dn_vattr = 0, dn_cattr = 0, dn_vvcd = 0, dn_cvcd = 0;
/* uint32_t dn_vcde =0; */
   
#ifdef GLXX_WANT_ATTRIBUTE_SORTING
   uint32_t done_swap;
   GLXX_ATTR_SORT_ENTRY_T *b;
#endif   
   
   //setup the order
   for(i=0;i<GLXX_CONFIG_MAX_VERTEX_ATTRIBS;i++)
   {
      if(vattribs_live & 15<<(4*i) && attrib[i].enabled)
      {
         if(n>0)
         {
            order[n-1].next = &order[n];
            order[n].prev = &order[n-1];
         }
         else
            order[n].prev = NULL;
            
         order[n].index = i;
         order[n].next = NULL;
         n++;
      }
   }
   
   head = &order[0];

#ifdef GLXX_WANT_ATTRIBUTE_SORTING
   //now sort for merging   
   if(n>0)
   {
      a = head;
      while(a!=NULL)
      {
         done_swap = 0;
         for(b = a->next;b != NULL; b = b->next)
         {
            if(mergeable_attribs[b->index] == a->index)
            {
               //b should go just before a
               
               //take b out of its current position
               vcos_assert(b != head);
               b->prev->next = b->next;
               if(b->next)
                  b->next->prev = b->prev;
               
               //put b in new position
               b->prev = a->prev;
               b->next = a;
                  
               if(a->prev)
               {
                  vcos_assert(a != head);
                  a->prev->next = b;
               }
               else
               {
                  vcos_assert(a == head);
                  head = b;
               }
               
               a->prev = b;
               
               done_swap = 1;
               break;
            }
            else if(mergeable_attribs[a->index] == b->index && b->prev!=a)
            {
               //a should go just before b
               
               //take b out of its current position
               vcos_assert(b != head);
               b->prev->next = b->next;
               if(b->next)
                  b->next->prev = b->prev;
               
               //put b in new position
               b->prev = a;
               b->next = a->next;
                  
               vcos_assert(a->next);
               a->next->prev = b;
               a->next = b;   

               done_swap = 1;
               break;
            }
         }
         if(!done_swap)
            a = a->next;
      }
   }
#endif
   
   //we double the length of this ordering array
   //but the total merged length should still be
   //<GLXX_CONFIG_MAX_VERTEX_ATTRIBS
   for(i=0;i<GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2;i++)
   {
      vattribs_order[i] = ~0;
      cattribs_order[i] = ~0;
   }
   
   a = head;
   for(i=0;i<GLXX_CONFIG_MAX_VERTEX_ATTRIBS;i++)
   {
      if(i<n)
      {
         vcos_assert(a != NULL);
         vattribs_order[i] = a->index;
         a = a->next;
         
         dn_vattr++;
      }
   }
   
   
   
   //coordinate attribs are a subset of vertex attribs
   //fill this in below by simulating the merge
   for (j = 0; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS; j++) {
      uint32_t jstart = j;
      i = vattribs_order[j];
      
      if (i!=(uint32_t)~0) {
         bool cattrib_match = !!(cattribs_live & 15<<(4*i));
         
         last_vattrib = j;
         
#ifdef GLXX_WANT_ATTRIBUTE_MERGING         
         while(j<GLXX_CONFIG_MAX_VERTEX_ATTRIBS-1 && vattribs_order[j+1]!=(uint32_t)~0 && mergeable_attribs[vattribs_order[j]] == vattribs_order[j+1])
         {
            //merge vattribs
            j++;   
            cattrib_match = !(cattribs_live & 15<<(4*vattribs_order[j])) ? false : cattrib_match;
            last_vattrib = j;
         }
#endif         

         dn_vvcd++;

         if (cattrib_match) //want all the vattribs from this iteration in cattribs
         {
            dn_cvcd++;
            for(m = jstart;m<=last_vattrib;m++)
            {
               cattribs_order[m] = vattribs_order[m];
               
               dn_cattr++;
            }
         }
         else
         {
            //don't use this merged attribute for coordinate shader
            //collect any of the merged attribs that were needed by the coordinate shader
            for(m = jstart;m<=last_vattrib;m++)
            {
               cattribs_order[m] = ~0;
               if (cattribs_live & 15<<(4*vattribs_order[m]))
               {
                  cattribs_extra[num_cattribs_extra++] = vattribs_order[m];
               }
            }
         }
      }
   }
   //now push any cattribs that didn't match the merged vattribs on the end of cattribs
   vcos_assert((last_vattrib+1+num_cattribs_extra)<=GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2);
   for (j = 0; j < num_cattribs_extra; j++) {
      cattribs_order[last_vattrib+1+j] = cattribs_extra[j];
      dn_cattr++;
   }
   
   //for debugging only simulate cattribs merge
//   for (j = last_vattrib+1; j < GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2; j++) {
//            
//      i = cattribs_order[j];
//      
//      if (i!=~0)
//      {
//#ifdef GLXX_WANT_ATTRIBUTE_MERGING                     
//         while(j<GLXX_CONFIG_MAX_VERTEX_ATTRIBS*2-1 && cattribs_order[j+1]!=~0  && mergeable_attribs[cattribs_order[j]] == cattribs_order[j+1])
//         {
//            //merge cattribs
//            j++;   
//         }
//#endif         
//         dn_cvcd++;
//         dn_vcde++;
//      }
//   }
//   
//   printf("%d %d %d %d %d\n",dn_vattr, dn_cattr, dn_vvcd, dn_cvcd, dn_vcde);
}
