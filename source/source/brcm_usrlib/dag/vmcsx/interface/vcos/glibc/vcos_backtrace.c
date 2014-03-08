/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  osal
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
Print out a backtrace 
=============================================================================*/

#include <interface/vcos/vcos.h>
#ifdef __linux__
#include <execinfo.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

void vcos_backtrace_self(void)
{
#ifdef __linux__
   void *stack[64];
   int depth = backtrace(stack, sizeof(stack)/sizeof(stack[0]));
   char **names = backtrace_symbols(stack, depth);
   int i;
   if (names)
   {
      for (i=0; i<depth; i++)
      {
         printf("%s\n", names[i]);
      }
      free(names);
   }
#endif
}

