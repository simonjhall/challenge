/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
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

/*****************************************************************************
*  cacheops.h
*
*  PURPOSE:
*
*     This file defines the API for cache operation and interface with 
*     /dev/cache-ops file		 
*
*****************************************************************************/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef LOG_TAG
#define LOG_TAG "Cacheops"
#endif

#define BRCM_CACHE "/dev/cache-ops"
#define MAG_NUM                 'c'
#define CACHE_OPS       _IO(MAG_NUM,1)

/* interface for passing structures between user
 space and kernel space easily */
struct cache_interface {
        char                                    i_f;            // cache invalidate or flush
        unsigned int                            pstart;
        unsigned int                            vstart;
        unsigned int                            size;
        int                                     out_rc;         // return code from the test
};
typedef struct cache_interface cache_interface_t;

/*------------------------------------------------------------------------------
    Function name   : Cacheops
    Description     : Cache flush and clean

    Return type     : errno or 0 - pass

    Argument        : F/I, pstart, vastart, size
------------------------------------------------------------------------------*/
static inline int cacheops(char in_flush, unsigned int pstart,unsigned int vstart,unsigned int size)
{
#if 0
	int fd_cacheops = -1,ret;
    	cache_interface_t cache_data;

    	fd_cacheops = open(BRCM_CACHE, O_RDWR);
    	if(fd_cacheops < 0)
    	{
        	LOGV("Error in opening the cache device!!!!\n");
        	return errno;
    	}
    	else
    	{
        	cache_data.i_f=in_flush;
        	cache_data.pstart=pstart;
        	cache_data.vstart=vstart;
        	cache_data.size=size;
        	ret=ioctl(fd_cacheops,CACHE_OPS,&cache_data);
        	if(ret < 0)
        	{
           		LOGE("Error CACHE_OPS ioctl failed %d\n",ret);
        	}
        	close(fd_cacheops);
        	fd_cacheops = -1;
        	return ret;
     	}
#endif
}


