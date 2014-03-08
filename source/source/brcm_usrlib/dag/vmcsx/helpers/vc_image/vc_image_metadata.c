/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "vcfw/rtos/rtos.h"
#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_assert.h"
#include "vc_image_metadata.h"
#ifdef __VIDEOCORE__
#include "vcfw/vclib/vclib.h"
#else
#define ALIAS_DIRECT(x) ((void *)(x))
#endif
#include "vcinclude/hardware.h"

/******************************************************************************
Private defines.
******************************************************************************/

#define NOLOG
#ifdef NOLOG
#define LOG_MSG(fmt,...) (void)0
#else
#define CH(x) ((x) < ' ' || (x) > '~' ? ' ' : (x))
#define FCC(x) CH(x>>24), CH((x>>16)&0xff), CH((x>>8)&0xff), CH(x&0xff)
#define LOG_MSG(fmt,...) vcos_log(fmt,__VA_ARGS__)
#endif

/******************************************************************************
Public function definitions.
******************************************************************************/
int vc_metadata_initialise(VC_METADATA_HEADER_T *header, int size)
{
   int retcode = -1;
   vcos_assert(size > 0);

   if (header)
   {
      //vc_metadata_lock(header);
      header->size = size - sizeof(VC_METADATA_HEADER_T);
      header->offset_next = ((sizeof(VC_METADATA_HEADER_T) + VC_METADATA_ALIGN) & ~VC_METADATA_ALIGN);
      header->readonly = 0;
      header->latch = rtos_latch_unlocked();
      //vc_metadata_unlock(header);
      retcode = 0;
   }

   return retcode;
}

int vc_metadata_add(VC_METADATA_HEADER_T *header, void *data, VC_METADATA_TYPE_T type, int len)
{
   int retcode = -1;
   VC_METADATA_ITEM_T *item=NULL;

   vc_metadata_lock(header);

   LOG_MSG("vc_metadata:adding '%c%c%c%c' to %x, readonly=%d, offset=%d",
           FCC(type), header, header->readonly, header->offset_next);

   if(vcos_verify_msg(header->readonly == 0, "vc_metadata_add: metadata buffer is locked to read-only, so vc_metadata_add not allowed"))
   {
      item = (VC_METADATA_ITEM_T *)((unsigned)header + header->offset_next);
      vcos_assert(item && header);

      if (vcos_verify_msg(header->size >= ((sizeof(VC_METADATA_ITEM_T)+len+VC_METADATA_ALIGN)&~VC_METADATA_ALIGN), 
                      "vc_metadata_add: not enough buffer space for metadata item"))
      {
         int item_len;
         item->type = type;
         item->len = len;

         item_len = sizeof(VC_METADATA_ITEM_T) + len;
         /* Round up to a multiple of 4 */
         item_len = (item_len+VC_METADATA_ALIGN) & ~VC_METADATA_ALIGN;

         header->offset_next += item_len;
         header->size -= item_len;

         vc_metadata_unlock(header);
         LOG_MSG("vc_metadata:adding '%c%c%c%c' at %x, len %d, offset=%d",
                 FCC(type), item, item_len, header->offset_next);

         if (data)  // data can be NULL to get uninitialised metadata
            memcpy((void *)((unsigned)item+sizeof(VC_METADATA_ITEM_T)), data, len);

         retcode = 0;
      }
      else
      {
         vc_metadata_unlock(header);
         retcode = -1;
      }
   }
   else
   {
      vc_metadata_unlock(header);
   }

   LOG_MSG("vc_metadata:adding '%c%c%c%c', returning %d", FCC(type), retcode);

   return retcode;
}

int vc_metadata_set_readonly(VC_METADATA_HEADER_T *header, int value)
{
   int retcode = -1;

   if (vcos_verify(header))
   {
      vc_metadata_lock(header);
      header->readonly = !!value;
      vc_metadata_unlock(header);
      retcode = 0;
   }

   return retcode;
}

void *vc_metadata_get(void *dest, VC_METADATA_HEADER_T *header, VC_METADATA_TYPE_T type, int *retcode)
{
   return vc_metadata_get_with_length(dest, header, type, retcode, NULL);
}

void *vc_metadata_get_with_length(void *dest, VC_METADATA_HEADER_T *header, VC_METADATA_TYPE_T type, int *retcode, int *len)
{
   VC_METADATA_ITEM_T *item = NULL;
   void *data = NULL, *end_ptr = NULL;
   *retcode = -1;

   if (header)
   {
      vc_metadata_lock(header);
      item = (VC_METADATA_ITEM_T *)((unsigned)header + sizeof(VC_METADATA_HEADER_T));
      end_ptr = (void *)ALIAS_DIRECT((unsigned)header + header->offset_next);
      vc_metadata_unlock(header);

      while ( ALIAS_DIRECT(item) < end_ptr )
      {
         LOG_MSG("vc_metadata:getting '%c%c%c%c', found '%c%c%c%c' at %x (offset %d)",
                 FCC(type), FCC(item->type), item, (uint8_t *) item - (uint8_t *) header);

         if (item->type == type)
         {
            data = (void *)((unsigned)item + sizeof(VC_METADATA_ITEM_T));
            if(len)
               *len = item->len;
            *retcode = 0;
            break;
         }
         /* move to the next item */
         if(!vcos_verify_msg(item->len >= 0, "vc_metadata_get_with_length: metadata header may be corrupt"))
         {
            break;
         }
         item = (VC_METADATA_ITEM_T *)(((unsigned)item + sizeof(VC_METADATA_ITEM_T) + item->len +
                                        VC_METADATA_ALIGN) & ~VC_METADATA_ALIGN);
      }

      if (data && dest)
         memcpy(dest, data, item->len);
   }

   LOG_MSG("vc_metadata:getting '%c%c%c%c', returning %x", FCC(type), data);

   return data;
}

int vc_metadata_clear(VC_METADATA_HEADER_T *header)
{
   int retcode = -1;
   int buffer_size = -1;

   if (header)
   {
      vc_metadata_lock(header);
      buffer_size = header->size + header->offset_next - sizeof(VC_METADATA_HEADER_T);
      header->size = buffer_size;
      header->offset_next = ((sizeof(VC_METADATA_HEADER_T) + VC_METADATA_ALIGN) & ~VC_METADATA_ALIGN);
      header->readonly = 0;
      vc_metadata_unlock(header);

      LOG_MSG("vc_metadata:clear %x, setting offset=%d", header, header->offset_next);

      retcode = 0;
   }

   return retcode;
}

int vc_metadata_copy(VC_METADATA_HEADER_T *dest, VC_METADATA_HEADER_T *src)
{
   void *from, *to;
   int retcode = -1, size_src;

   vcos_assert(dest && src);

   vc_metadata_lock(dest);
   vc_metadata_lock(src);
   to = (void *)((unsigned)dest + dest->offset_next);
   from = (void *)((unsigned)src + sizeof(VC_METADATA_HEADER_T));
   size_src =  src->offset_next - sizeof(VC_METADATA_HEADER_T);

   if (size_src <= dest->size)
   {
      dest->offset_next = ((dest->offset_next + size_src + VC_METADATA_ALIGN) & ~VC_METADATA_ALIGN);
      dest->size -= size_src;
      vc_metadata_unlock(dest);
      vc_metadata_unlock(src);

      memcpy(to, from, size_src);
      retcode = 0;
   }
   else
   {
      vc_metadata_unlock(dest);
      vc_metadata_unlock(src);
   }

   return retcode;
}

int vc_metadata_overwrite(VC_METADATA_HEADER_T *header, void *data, VC_METADATA_TYPE_T type)
{
   int retcode = -1;
   VC_METADATA_ITEM_T *item_hdr;
   void *item = NULL;

   item = vc_metadata_get(NULL, header, type, &retcode);

   if (retcode == 0)
   {
      item_hdr = (VC_METADATA_ITEM_T *)((unsigned)item - sizeof(VC_METADATA_ITEM_T));
      memcpy(item, data, item_hdr->len);
   }

   return retcode;
}

int vc_metadata_cache_flush(VC_METADATA_HEADER_T *header)
{
   int retcode = 0;

#ifdef __VIDEOCORE__   
   int  buffer_size;
   vc_metadata_lock(header);
   buffer_size = header->size + header->offset_next;
   vclib_dcache_flush_range(header, buffer_size);
   vc_metadata_unlock(header);
#endif
   return retcode;
}

int vc_metadata_lock(VC_METADATA_HEADER_T *header)
{
#ifdef __VIDEOCORE__ 
   if (header)
   {
      rtos_latch_get(&header->latch);
   }
#endif
   return 0;
}

int vc_metadata_unlock(VC_METADATA_HEADER_T *header)
{
#ifdef __VIDEOCORE__
   if (header)
   {
      rtos_latch_put(&header->latch);
   }
#endif
   return 0;
}
