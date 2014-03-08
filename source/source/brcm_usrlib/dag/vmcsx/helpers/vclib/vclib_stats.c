/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

/* System includes */

/* Project includes */

/******************************************************************************
Public functions written in this module.
Define as extern here.
******************************************************************************/

extern void vclib_stat_uchar (unsigned char *addr, int width, int height, int pitch,
                                 unsigned int *max, unsigned int *min, unsigned int *sum);
/* Check extern defs match above and loads #defines and typedefs */


/******************************************************************************
Extern functions (written in other modules).
Specify through module public interface include files or define
specifically as extern.
******************************************************************************/

extern void vclib_statxyn (unsigned char *addr, int pitch, unsigned int *max,
                              unsigned int *min, unsigned int *sum, int ncols, int nrows);

/******************************************************************************
Private typedefs, macros and constants. May also be defined just before a
function or group of functions that use the declaration.
******************************************************************************/


/******************************************************************************
Private functions in this module.
Define as static.
******************************************************************************/


/******************************************************************************
Data segments - const and variable.
Declare global and static data here.
Include extern definitions for any external global data used by this module.
******************************************************************************/


/******************************************************************************
Global function definitions.
******************************************************************************/


/*===============================================================================
 NAME: vclib_stat_uchar

 SYNOPSIS: void vclib_stat_uchar(unsigned char *addr, int width, int height, int pitch,
                                 unsigned *max, unsigned *min, unsigned *sum)

 FUNCTION: calculate the max / min and the total sum of an array of unsigned char values

 ARGUMENTS: start address, width/height/pitch of data and pointers to max/min/sum for returning values

 RETURN: -
 ================================================================================*/

void vclib_stat_uchar(unsigned char *addr, int width, int height, int pitch,
                      unsigned int *max, unsigned int *min, unsigned int *sum) {

   int ii, jj;
   unsigned char *start;

   for (jj = height; jj >= 16; jj -= 16,  addr += (pitch << 4)) {
      for (ii = width, start = addr; ii >= 16; ii -= 16, start += 16) {
         vclib_statxyn(start, pitch, max, min, sum, 16, 16);
      }

      /* leftover on the right hand side */
      if (ii)
         vclib_statxyn(start, pitch, max, min, sum, ii, 16);
   }

   /* left over at the bottom */
   if (jj) {
      for (ii = width, start = addr; ii >= 16; ii -= 16, start += 16) {
         vclib_statxyn(start, pitch, max, min, sum, 16, jj);
      }

      /* leftover on the lower right hand corner */
      if (ii)
         vclib_statxyn(start, pitch, max, min, sum, ii, jj);
   }

}
