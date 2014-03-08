/* ============================================================================
Copyright (c) 2006-2014, Broadcom Corporation
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
============================================================================ */

// Several helper macros for get_info functions in handlers or codecs.They assume that name contains the
// name of a property desired, and property is a PROPERTY structure to put it in. NAME is compared to
// name, both prefixed by PREFIX and not, and if it matches, property is filled with data and the function
// returns. NUM and STR should be integer or string data. CHILDREN should be a comma separated list of
// sub-properties names.
//
// It is convention that parent properties are prefixed by #.
//
// For instance. prop_grp("#optional_parent.","#sub1.#sub2","#optional_parent.#sub1.#sub2.sub3a","#optional_parent.#sub1.#sub2.sub3b")

#ifndef _GET_INFO_H_
#define _GET_INFO_H_
#include <string.h>

#define PROP_BUF_SIZE 500 //should be large enough for list of exif properties

typedef struct {
   enum {
      PROP_UNKNOWN,
      PROP_PARENT,
      PROP_NUM,
      PROP_STRING,
      PROP_DATA
   } type;
   char ** children; // array of names, last one is ""
   int32_t i;//int or numerator
   int32_t d;//denominator
   char s[PROP_BUF_SIZE+1];//spare byte for terminator
   int more; // indicates how many more values are to come, or -1 for 'unknown, but some'. Used most for large data.
   int count; //only used for data, says how many bytes there are. Also for children if 'more' is used.
} PROPERTY_T;


static inline int property_grp(char *prefix, char *name, char *name2, PROPERTY_T *property, char **children) {
   if (strcasecmp(name, name2) == 0 || ( strcasecmp(name+strlen(prefix),name2)==0 && strncasecmp(name,prefix,strlen(prefix))==0 )) {
      property->type = PROP_PARENT;
      // *** Note there is NO memory allocation here,
      // *** the property merely points to the array
      property->children = children;
      return 0;
   }
   return 1;
}

static inline int property_num(char *prefix, char *name, char *name2, PROPERTY_T * property, int num) {
   if (strcasecmp(name, name2) == 0 || ( strcasecmp(name+strlen(prefix),name2)==0 && strncasecmp(name,prefix,strlen(prefix))==0 )) {
      property->type = PROP_NUM;
      property->i = num;
      return 0;
   }
   return 1;
}

static inline int property_str(char *prefix, char *name, char *name2, PROPERTY_T * property, const char *str) {
   if (strcasecmp(name, name2) == 0 || ( strcasecmp(name+strlen(prefix),name2)==0 && strncasecmp(name,prefix,strlen(prefix))==0 )) {
      property->type = PROP_STRING;
      strncpy(property->s, str, PROP_BUF_SIZE);
      return 0;
   }
   return 1;
}

// Should this be in handler.h? Default_handler_get_info can only used in handlers including get_info stuff, so not defined
// for non-vmcs stuff. Psnprintf could be, but isn't.
#ifdef HANDLER_H
extern int psnprintf(char * buf, size_t n, PROPERTY_T property);
extern int default_handler_get_info(char * name, PROPERTY_T * property, int more, MP_SOURCE_INFO_T source_info, MP_STREAM_INFO_T ** stream_infos);
#endif

#endif
