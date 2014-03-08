/* ============================================================================
Copyright (c) 2008-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos_logging.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static VCOS_MUTEX_T lock;
static int warned_loglevel;             // only warn about invalid log level once

static VCOS_LOG_CAT_T dflt_log_category;
static VCOS_LOG_CAT_T *vcos_logging_categories;
static int inited;

void vcos_logging_init(void)
{
   if (inited)
   {
      // FIXME: should print a warning or something here
      return;
   }

   vcos_mutex_create(&lock, "vcos_log");
   vcos_log_register("default", &dflt_log_category);
   dflt_log_category.flags.want_prefix = 0;
   vcos_assert(!inited);
   inited = 1;
}

/** Read an alphanumeric token, returning True if we succeeded.
  */

static int read_tok(char *tok, size_t toklen, const char **pstr, char sep)
{
   const char *str = *pstr;
   int n = 0;
   int ch;

   // skip past any whitespace
   while (str[0] && isspace((int)(str[0])))
      str++;

   while ((ch = *str) != '\0' &&
          ch != sep &&
          isalnum(ch) &&
          n != toklen-1)
   {
      tok[n++] = ch;
      str++;
   }

   // did it work out?
   if (ch == '\0' || ch == sep)
   {
      if (ch) str++; // move to next token if not at end
      // yes
      tok[n] = '\0';
      *pstr = str;
      return 1;
   }
   else
   {
      // no
      return 0;
   }
}

static int read_level(VCOS_LOG_LEVEL_T *level, const char **pstr, char sep)
{
   char buf[16];
   int ret = 1;
   if (read_tok(buf,sizeof(buf),pstr,sep))
   {
      if (strcmp(buf,"error") == 0)
         *level = VCOS_LOG_ERROR;
      else if (strcmp(buf,"warn") == 0)
         *level = VCOS_LOG_WARN;
      else if (strcmp(buf,"warning") == 0)
         *level = VCOS_LOG_WARN;
      else if (strcmp(buf,"info") == 0)
         *level = VCOS_LOG_INFO;
      else if (strcmp(buf,"trace") == 0)
         *level = VCOS_LOG_TRACE;
      else
      {
         vcos_log("Invalid trace level '%s'\n", buf);
         ret = 0;
      }
   }
   else
   {
      ret = 0;
   }
   return ret;
}

void vcos_log_register(const char *name, VCOS_LOG_CAT_T *category)
{
   const char *env;
   VCOS_LOG_CAT_T *i;
   memset(category, 0, sizeof(*category));

   category->name  = name;
   category->level = VCOS_LOG_ERROR;
   category->flags.want_prefix = 1;

   vcos_mutex_lock(&lock);

   /* is it already registered? */
   for (i = vcos_logging_categories; i ; i = i->next )
   {
      if (i == category)
         break;
   }

   if (!i)
   {
      /* not yet registered */
      category->next = vcos_logging_categories;
      vcos_logging_categories = category;
   }

   vcos_mutex_unlock(&lock);

   /* Check to see if this log level has been enabled. Look for
    * (<category:level>,)*
    *
    * VC_LOGLEVEL=ilcs:info,vchiq:warn
    */

   env = _VCOS_LOG_LEVEL();
   if (env)
   {
      do
      {
         char env_name[64];
         VCOS_LOG_LEVEL_T level;
         if (read_tok(env_name, sizeof(env_name), &env, ':') &&
             read_level(&level, &env, ','))
         {
            if (strcmp(env_name, name) == 0)
            {
               category->level = level;
               break;
            }
         }
         else
         {
            if (!warned_loglevel)
            {
                vcos_log("VC_LOGLEVEL format invalid at %s\n", env);
                warned_loglevel = 1;
            }
            return;
         }
      } while (env[0] != '\0');
   }
}

void vcos_log_unregister(VCOS_LOG_CAT_T *category)
{
   VCOS_LOG_CAT_T **pcat;
   vcos_mutex_lock(&lock);
   pcat = &vcos_logging_categories;
   while (*pcat != category)
   {
      if (!*pcat)
         break;   /* possibly deregistered twice? */
      if ((*pcat)->next == NULL)
      {
         vcos_assert(0); // already removed!
         vcos_mutex_unlock(&lock);
         return;
      }
      pcat = &(*pcat)->next;
   }
   if (*pcat)
      *pcat = category->next;
   vcos_mutex_unlock(&lock);
}

VCOSPRE_ const VCOS_LOG_CAT_T * VCOSPOST_ vcos_log_get_default_category(void)
{
   return &dflt_log_category;
}

void vcos_set_log_options(const char *opt)
{
#ifdef __linux__
   printf("Sorry, %s not yet implemented\n", __FUNCTION__);
#endif
}

