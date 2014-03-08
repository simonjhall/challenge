/* ============================================================================
Copyright (c) 2010-2014, Broadcom Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifdef BRCM_USE_BMEM

#ifndef _BMEM_IOCTL_H_
#define _BMEM_IOCTL_H_
#include <linux/ioctl.h>	/* needed for the _IOW etc stuff used later */

/*
 * Ioctl definitions
 */
#define BMEM_WRAP_MAGIC  'B'
#define GEMEMALLOC_WRAP_ACQUIRE_BUFFER _IOWR(BMEM_WRAP_MAGIC,  1, unsigned long)
#define GEMEMALLOC_WRAP_RELEASE_BUFFER _IOW(BMEM_WRAP_MAGIC,  2, unsigned long)
#define GEMEMALLOC_WRAP_COPY_BUFFER _IOW(BMEM_WRAP_MAGIC,  3, unsigned long)

#define BMEM_WRAP_MAXNR 15

typedef struct {
	unsigned long busAddress;
	unsigned int size;
} GEMemallocwrapParams;

#ifndef _BMEM_DMA_STRUCT_
#define _BMEM_DMA_STRUCT_
typedef struct {
	unsigned long src_busAddr; 	// input physical addr
	unsigned long dst_busAddr;	// ouptut physical addr
	unsigned int size;			// size to be transferred.
} DmaStruct;
#endif

#endif /* _BMEM_IOCTL_H_ */

#else

#ifndef _GEMEMALLOC_IOCTL_H_
#define _GEMEMALLOC_IOCTL_H_
#include <linux/ioctl.h>	/* needed for the _IOW etc stuff used later */

/*
 * Ioctl definitions
 */
#define GEMEMALLOC_WRAP_MAGIC  'B'
#define GEMEMALLOC_WRAP_ACQUIRE_BUFFER _IOWR(GEMEMALLOC_WRAP_MAGIC,  1, unsigned long)
#define GEMEMALLOC_WRAP_RELEASE_BUFFER _IOW(GEMEMALLOC_WRAP_MAGIC,  2, unsigned long)

#define GEMEMALLOC_WRAP_MAXNR 15

typedef struct {
	unsigned long busAddress;
	unsigned int size;
} GEMemallocwrapParams;

#endif /* _MEMALLOC_IOCTL_H_ */

#endif
