/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <string.h>
#include <assert.h>

/******************************************************************************
NAME
   memmem

SYNOPSIS
   void *memmem( const void *needle, size_t needle_length, const void *haystack, size_t haystack_length )

FUNCTION
   Search for the first occurence of a substring within a block of memory, without requiring null termination

RETURNS
   void *  A pointer to the first occurrence of the substring, or NULL if it is not found
******************************************************************************/

void *memmem( const void *needle, size_t needle_length, const void *haystack, size_t haystack_length )
{
   size_t needle_pos = 0;
   char * haystack_ptr = (char*) haystack;
   void * ret_ptr = NULL;


   if ( ( ! needle ) || ( ! haystack ) )
   {
      // either the needle or haystack are NULL
      assert(0);
      return NULL;
   }

   if ( needle_length > haystack_length )
   {
      // Can't find needle if it is bigger than the haystack
      assert(0);
      return NULL;
   }

   while ( haystack_length > 0 )
   {
      if ( *haystack_ptr == ((char*)needle)[ needle_pos ] )
      {
         if ( needle_pos == 0 )
         {
            ret_ptr = (void * )haystack_ptr;
         }

         needle_pos++;

         if ( needle_pos == needle_length )
         {
            return ret_ptr;
         }
         haystack_length--;
         haystack_ptr++;
      }
      else if ( needle_pos != 0 )
      {
         needle_pos = 0;
      }
      else
      {
         haystack_length--;
         haystack_ptr++;
      }
   }
   return NULL;
}

