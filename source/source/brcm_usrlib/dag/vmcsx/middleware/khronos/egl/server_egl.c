/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */
#ifdef ANDROID

extern void *memory_pool_base;
extern unsigned int memory_pool_size;

#define MEMPOOL_BASE	memory_pool_base
#define MEMPOOL_SIZE	memory_pool_size

#else // ANDROID

#define MM_DATA_INCLUDED
//#include "nandsdram_memmap.h"
//#include "osheap.h"

#if defined( MM_DATA_INCLUDED )
extern int Image$$ER_MMDATA$$Base;

#define MEMPOOL_BASE	 (((int)(&Image$$ER_MMDATA$$Base) + 31)&~31)
#define MEMPOOL_SIZE 	 (MM_DATA_SIZE-32)

#else

#define MEMPOOL_BASE	((((int)(void*)OSHEAP_GetBytePoolByName("mm_mem_heap")+31)&~31))
#define MEMPOOL_SIZE	(0x02000000-32)

#endif // MM_DATA_INCLUDED
#endif // ANDROID

#include "egl_server.c"

