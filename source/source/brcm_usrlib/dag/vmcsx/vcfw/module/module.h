/* ============================================================================
Copyright (c) 2007-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#ifndef MODULE_H_
#define MODULE_H_

#include "vcinclude/common.h"

/******************************************************************************
 Function defines
 *****************************************************************************/

//Generic handle passed used by all drivers
typedef struct opaque_module_handle_t *MODULE_HANDLE_T;

//Routine to initialise some module library
typedef int32_t (*MODULE_INIT_T)( void );

//Routine to shutdown some module library
typedef int32_t (*MODULE_EXIT_T)( void );

//Routine to return a drivers info (name, version etc..)
typedef int32_t (*MODULE_INFO_T)(  const char **module_name,
                                   uint32_t *version_major,
                                   uint32_t *version_minor );

/******************************************************************************
 Driver struct definition
 *****************************************************************************/

/* Basic driver structure, as implemented by all module that can be loaded by the vcfw rtos layer */
#define COMMON_MODULE_API \
   /*Function to initialize the module. This is used at the start of day*/ \
   MODULE_INIT_T   init; \
   \
   /*Function to exit the module. This shuts down the library*/ \
   MODULE_EXIT_T   exit; \
   \
   /*Function to get the module name + version*/ \
   MODULE_INFO_T   info;

typedef struct
{
   //just include the basic module api
   COMMON_MODULE_API

} MODULE_T;


#endif // MODULE_H_
