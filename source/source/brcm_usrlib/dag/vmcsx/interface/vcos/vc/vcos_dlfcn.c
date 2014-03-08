/*=============================================================================
Copyright (c) 2010 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  vcos
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VCOS - abstraction over dynamic library opening
=============================================================================*/

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_dlfcn.h"
#include "middleware/dlloader/dlfcn.h"
#include "filesystem/filesys.h"

void *vcos_dlopen(const char *name, int mode)
{
   void *vll_handle;

   //filesys_register();

   vll_handle = dlopen(name, mode);
   
   return vll_handle;
}

void (*vcos_dlsym(void *handle, const char *name))()
{
   void (*symbol)();
   
   symbol = dlsym(handle, name);

   // Note that we intentionally call dldone here to encourage
   // VLLs with only a single entry-point! If you find this a 
   // problem you, should use dlshared_vll_load instead.
   dldone(handle);
   
   return symbol;
}

int vcos_dlclose (void *handle)
{
   dlclose(handle);
   return 0;
}

int vcos_dlerror(int *err, char *buf, size_t buflen)
{
   // not really threadsafe!
   const char *errmsg = dlerror();

   vcos_assert(buflen > 0);

   if (errmsg)
   {
      *err = -1;
      strncpy(buf, errmsg, buflen);
      buf[buflen-1] = '\0';
   }
   else
   {
      *err = 0;
      buf[0] = '\0';
   }
   return 0;
}




