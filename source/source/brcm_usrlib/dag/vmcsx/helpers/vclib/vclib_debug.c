/* ============================================================================
Copyright (c) 2009-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include "vcinclude/common.h"
#include "helpers/vclib/vclib.h"
#include "vcfw/rtos/rtos.h"


/******************************************************************************
Public functions written in this module.
******************************************************************************/

unsigned long vclib_VRF_hash(uint32_t tmp[32]);

int vclib_checkpoint_init(VCLIB_CHECKPOINT_STATE_T *state, void *buffer, unsigned int length, int generate);
int vclib_checkpoint_check_VRF(VCLIB_CHECKPOINT_STATE_T *state);
int vclib_checkpoint_check_buffer(VCLIB_CHECKPOINT_STATE_T *state, void const *buffer, unsigned int length);
int vclib_checkpoint_breakpoint(VCLIB_CHECKPOINT_STATE_T *state, uint32_t progress);
int vclib_checkpoint_length(VCLIB_CHECKPOINT_STATE_T *state);
int vclib_checkpoint_dump(VCLIB_CHECKPOINT_STATE_T *state, char const *filename, VCLIB_CHECKPOINT_DUMP_FORMAT_T format);
int vclib_checkpoint_set_default(VCLIB_CHECKPOINT_STATE_T *default_state);
int vclib_checkpoint_clear_default(VCLIB_CHECKPOINT_STATE_T *current_default_state);


/******************************************************************************
Private stuff in this module.
******************************************************************************/

#define CHECKPOINT_STATE_MAGIC 0x1d352837
#define CHECKPOINT_BUFFER_MAGIC 0x25cb2c22

#define CHECKPOINT_POLY 0x04c11db7L

static inline void checkpoint_break(VCLIB_CHECKPOINT_STATE_T const *state);

static VCLIB_CHECKPOINT_STATE_T *vclib_default_checkpoint_state;
static VCLIB_CRC32_STATE_T crc32_state;
static int crc32_initialised = 0;


/*******************************************************************************
Public functions
*******************************************************************************/

/*******************************************************************************
NAME
   vclib_VRF_hash

SYNOPSIS
   void vclib_VRF_hash(
      uint32_t                   tmp[32]);

FUNCTION
   digest the state of the VRF into a 32-bit hash

RETURNS
   a hash
*******************************************************************************/

unsigned long vclib_VRF_hash(uint32_t tmp[32])
{
   static const uint32_t twiddle[16] = { 32771, 49121, 65449, 32779, 49123, 65479, 32783, 49139, 65497, 32789, 49157, 65519, 32797, 49169, 65521, 49171, };
   int i, j;

   _vasm("v32st H32(0++,0), (%r+=%r) REP 2", tmp, 64);
   _vasm("v32eor H32(0,0), H32(0,0), -1");

   /* TODO: preserve the flags */

   for (i = 0; i < 64; i++)
   {
      if (_Usually(i))
         _vasm("veor H32(0,0), H32(0,0), H32(0,0)+%r", i << 6);

      for (j = 0; j < 32; j++)
      {
         _vasm("vshl H32(0,0), H32(0,0), 31 SETF");
         _vasm("veor H32(0,0), H32(0,0), %r IFC", CHECKPOINT_POLY);
      }
   }
   _vasm("v16ld H16(1,0), (%r)", twiddle);

   _vasm("v16mulhd.uu H16(0,0), H16(1,0), H16(0,0)");
   _vasm("v16mulhd.uu H16(0,32), H16(1,0), H16(0,32)");
   i = _vasm("v32ror -, H32(0,0), H8(1,0) USUM %D");

   /* TODO: restore the flags */

   _vasm("v32ld H32(0++,0), (%r+=%r) REP 2", tmp, 64);
   return i;
}


/*******************************************************************************
NAME
   vclib_checkpoint_init

SYNOPSIS
   void vclib_checkpoint_init(
      VCLIB_CHECKPOINT_STATE_T  *state,
      void                      *buffer,
      unsigned int               length,
      int                        generate);

FUNCTION
   init

RETURNS
   non-zero if parameters are good, zero if there is an error.
*******************************************************************************/

int vclib_checkpoint_init(VCLIB_CHECKPOINT_STATE_T *state, void *buffer, unsigned int length, int generate)
{
   int result = 1;

   if (state == NULL) state = vclib_default_checkpoint_state;

   assert((((intptr_t)buffer | (intptr_t)state) & 3) == 0 && (generate & ~1) == 0);
   if (((intptr_t)buffer & 3) != 0 || ((intptr_t)state & 3) != 0 || (generate & ~1) != 0)
      return 0;

   memset(state, 0, sizeof(*state));
   state->checkpoints = buffer;
   state->length = length;
   state->progress = sizeof(uint32_t);
   state->generate = (generate != 0);
   state->lockable = 0;
   state->magic = CHECKPOINT_STATE_MAGIC;

   if (state->generate) *(uint32_t *)state->checkpoints = CHECKPOINT_BUFFER_MAGIC;
   else        result = *(uint32_t *)state->checkpoints == CHECKPOINT_BUFFER_MAGIC;

   return result;
}


/*******************************************************************************
NAME
   vclib_checkpoint_check_VRF

SYNOPSIS
   int vclib_checkpoint_check_VRF(
      VCLIB_CHECKPOINT_STATE_T  *state);

FUNCTION
   blah

RETURNS
   zero if there is a checkpoint mismatch, overflow, or another fault, non-zero
   otherwise.
*******************************************************************************/

int vclib_checkpoint_check_VRF(VCLIB_CHECKPOINT_STATE_T *state)
{
   uint32_t *ptr;
   uint32_t check;
   int result = 1;

   if (state == NULL) state = vclib_default_checkpoint_state;
   if (state == NULL) return 1;

   assert(state != NULL && state->magic == CHECKPOINT_STATE_MAGIC);
   if (state == NULL || state->magic != CHECKPOINT_STATE_MAGIC)
      return 0;

   assert(state->progress < state->length);
   if (state->progress >= state->length)
      return 0;

   check = vclib_VRF_hash(state->vrf_save);

   ptr = (void *)((char *)state->checkpoints + state->progress);
   state->progress += sizeof(uint32_t);

   if (state->generate) *ptr = check;
   else        result = *ptr == check;

   if (state->progress == state->breakpoint)
      checkpoint_break(state);

   return result;
}


/*******************************************************************************
NAME
   vclib_checkpoint_check_buffer

SYNOPSIS
   int vclib_checkpoint_check_buffer(
      VCLIB_CHECKPOINT_STATE_T  *state,
      void                const *buffer,
      unsigned int               length);

FUNCTION
   blah

RETURNS
   zero if there is a checkpoint mismatch, overflow, or another fault, non-zero
   otherwise.
*******************************************************************************/

int vclib_checkpoint_check_buffer(VCLIB_CHECKPOINT_STATE_T *state, void const *buffer, unsigned int length)
{
   uint32_t *ptr;
   uint32_t check;
   int result = 1;

   if (state == NULL) state = vclib_default_checkpoint_state;
   if (state == NULL) return 1;

   assert(state != NULL && state->magic == CHECKPOINT_STATE_MAGIC);
   if (state == NULL || state->magic != CHECKPOINT_STATE_MAGIC)
      return 0;

   assert(state->progress < state->length);
   if (state->progress >= state->length)
      return 0;

   /* TODO: preserve the VRF and flags */

   if (crc32_initialised == 0)
   {
      vclib_crc32_init(&crc32_state, CHECKPOINT_POLY);
      crc32_initialised = 1;
   }

   check = vclib_crc32(&crc32_state, -1, buffer, length);

   /* TODO: restore the VRF and flags */

   ptr = (void *)((char *)state->checkpoints + state->progress);
   state->progress += sizeof(uint32_t);

   if (state->generate) *ptr = check;
   else        result = *ptr == check;

   if (state->progress == state->breakpoint)
      checkpoint_break(state);

   return 1;
}


/*******************************************************************************
NAME
   vclib_checkpoint_breakpoint

SYNOPSIS
   int vclib_checkpoint_breakpoint(
      VCLIB_CHECKPOINT_STATE_T  *state,
      uint32_t                   progress);

FUNCTION
   indicate that we want a _bkpt() call just before we reach \a progress
*******************************************************************************/

int vclib_checkpoint_breakpoint(VCLIB_CHECKPOINT_STATE_T *state, uint32_t progress)
{
   if (state == NULL) state = vclib_default_checkpoint_state;

   assert(state != NULL && state->magic == CHECKPOINT_STATE_MAGIC);
   if (state == NULL || state->magic != CHECKPOINT_STATE_MAGIC)
      return 0;

   assert(state->progress < state->length);
   if (state->progress >= state->length)
      return 0;

   assert(progress - sizeof(uint32_t) > state->progress);
   if (progress - sizeof(uint32_t) <= state->progress)
      return 0;

   assert(state->breakpoint == 0);
   if (state->breakpoint != 0)
      return 0;

   state->breakpoint = progress - sizeof(uint32_t);
   return 1;
}


/*******************************************************************************
NAME
   vclib_checkpoint_length

SYNOPSIS
   int vclib_checkpoint_length(
      VCLIB_CHECKPOINT_STATE_T  *state,
      void                      *buffer,
      unsigned int               length,
      int                        generate);

RETURNS
   returns the length (in bytes) of the checkpoint buffer used/tested so far.
*******************************************************************************/

int vclib_checkpoint_length(VCLIB_CHECKPOINT_STATE_T *state)
{
   if (state == NULL) state = vclib_default_checkpoint_state;

   assert(state != NULL && state->magic == CHECKPOINT_STATE_MAGIC);
   if (state == NULL || state->magic != CHECKPOINT_STATE_MAGIC)
      return 0;

   return state->progress;
}


/*******************************************************************************
NAME
   vclib_checkpoint_dump

SYNOPSIS
   int vclib_checkpoint_dump(
      VCLIB_CHECKPOINT_STATE_T  *state,
      char                const *filename,
      VCLIB_CHECKPOINT_DUMP_FORMAT_T format);

FUNCTION
   blah

RETURNS
   non-zero if parameters are good, zero if there is an error.
*******************************************************************************/

int vclib_checkpoint_dump(VCLIB_CHECKPOINT_STATE_T *state, char const *filename, VCLIB_CHECKPOINT_DUMP_FORMAT_T format)
{
   FILE *fptr;
   int const *ptr, *endptr;
   int col;

   if (state == NULL) state = vclib_default_checkpoint_state;

   assert(state != NULL && state->magic == CHECKPOINT_STATE_MAGIC);
   if (state == NULL || state->magic != CHECKPOINT_STATE_MAGIC)
      return 0;

   if (state->generate == 0)
      return 1; /* return OK, but do nothing */

   if (filename)
   {
      if ((fptr = fopen(filename, format == VCLIB_CHECKPOINT_DUMP_RAW ? "wb" : "wt")) == NULL)
         return 0;
   }
   else
      fptr = stdout;

   switch (format)
   {
   default:
   case VCLIB_CHECKPOINT_DUMP_RAW:
      fwrite(state->checkpoints, 1, state->progress, fptr);
      break;

   case VCLIB_CHECKPOINT_DUMP_C_SOURCE:
      fprintf(fptr, "unsigned int checkpoint_data[%d] = {\n", (state->progress + sizeof(int) - 1) / sizeof(int));

      ptr = state->checkpoints;
      endptr = (void *)((char *)ptr + state->progress);

      fprintf(fptr, "   ");
      col = 3;

      while (ptr < endptr)
      {
         if (col >= 72)
         {
            fprintf(fptr, "\n   ");
            col = 3;
         }

         col += fprintf(fptr, "0x%08x,", *ptr++);
      }

      fprintf(fptr, " };\n");
      break;
   }

   if (fptr != stdout)
      fclose(fptr);

   return 1;
}


/*******************************************************************************
NAME
   vclib_checkpoint_set_default

SYNOPSIS
   void vclib_checkpoint_set_default(
      VCLIB_CHECKPOINT_STATE_T  *default_state);

FUNCTION
   Set the checkpoint state buffer to be used when NULL is passed.

RETURNS
   non-zero if parameters are good, zero if there is an error.
*******************************************************************************/

int vclib_checkpoint_set_default(VCLIB_CHECKPOINT_STATE_T *default_state)
{
   assert(vclib_default_checkpoint_state == NULL);
   if (vclib_default_checkpoint_state != NULL)
      return 0;

   vclib_default_checkpoint_state = default_state;
   return 1;
}


/*******************************************************************************
NAME
   vclib_checkpoint_clear_default

SYNOPSIS
   void vclib_checkpoint_init(
      VCLIB_CHECKPOINT_STATE_T  *current_default_state);

FUNCTION
   Allow another caller to set a default state buffer.

RETURNS
   non-zero if parameters are good, zero if there is an error.
*******************************************************************************/

int vclib_checkpoint_clear_default(VCLIB_CHECKPOINT_STATE_T *current_default_state)
{
   assert(vclib_default_checkpoint_state == current_default_state);
   if (vclib_default_checkpoint_state != current_default_state)
      return 0;

   vclib_default_checkpoint_state = NULL;
   return 1;
}


/*******************************************************************************
Public functions
*******************************************************************************/

static inline void checkpoint_break(VCLIB_CHECKPOINT_STATE_T const *state)
{
   _bkpt();

   /* We stop here when the progress counter in a checkpoint reaches the value
    * set by a call to vclib_checkpoint_breakpoint().  From here you probably
    * want to step out and single-step over the subsequent code -- presumably
    * the next checkpoint is going to fail and you want to see why.
    */
   _nop();

   (void)state;
}

