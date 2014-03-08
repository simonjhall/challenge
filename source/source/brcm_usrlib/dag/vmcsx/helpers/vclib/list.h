/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef VCLIB_LIST_HEADER
#define VCLIB_LIST_HEADER

/******************************************************************************
Includes.
******************************************************************************/
#include <stddef.h>

/******************************************************************************
Defines.
******************************************************************************/

// This item _MUST_ be put as the first two elements in the structure that needs
// to be placed in the list
#define VCLIB_LIST_STRUCT_ITEMS  \
   struct VCLIB_LIST_ELEMENT_S_ * prev;   \
   struct VCLIB_LIST_ELEMENT_S_ * next;

/******************************************************************************
Types (enums).
******************************************************************************/


/******************************************************************************
Types (structs).
******************************************************************************/

typedef struct VCLIB_LIST_ELEMENT_S_ {
   VCLIB_LIST_STRUCT_ITEMS;
} VCLIB_LIST_ELEMENT_T;
typedef struct VCLIB_LIST_S_ * VCLIB_LIST_T;

/******************************************************************************
Global functions.
******************************************************************************/

VCLIB_LIST_T vclib_list_create(int count, size_t item_size, const char *name);
void vclib_list_destroy(VCLIB_LIST_T list);
void vclib_list_move(VCLIB_LIST_ELEMENT_T *, VCLIB_LIST_T src_list, VCLIB_LIST_T dst_list);
VCLIB_LIST_ELEMENT_T * vclib_list_head(VCLIB_LIST_T list);
VCLIB_LIST_ELEMENT_T * vclib_list_next(VCLIB_LIST_T list, VCLIB_LIST_ELEMENT_T *item);
void vclib_list_lock(VCLIB_LIST_T list);
void vclib_list_unlock(VCLIB_LIST_T list);
int vclib_list_count(VCLIB_LIST_T list);

/* Note:
** Check out entry in NOTES for information on how to use the linked list
*/

#endif
