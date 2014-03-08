#ifndef COMMON_H
#define COMMON_H

#include "bcmcomp_stdint.h"

#if defined (_MSC_VER) || defined(__CC_ARM)
#define INLINE __inline
#else
#define INLINE inline
#endif

#ifndef NULL
   #define NULL (void *)0
#endif

#if defined(__HIGHC__) || defined(__GNUC__)
   #define BCM_COMP_GNU_VARIADIC_MACROS /* use XXX.../XXX syntax for variadic macros */
#else
   #define BCM_COMP_C99_VARIADIC_MACROS /* use c99 .../__VA_ARGS__ syntax for variadic macros */
#endif

#include "interface/vcos/vcos_assert.h" // todo: microblaze?
#define UNREACHABLE() vcos_assert(0)

#ifdef _MSC_VER
   #define snprintf _snprintf
#else
   // No #define needed.
#endif

#ifdef _MSC_VER
   #ifdef __HIGHC__
      #error "Can't be both?"
   #endif
#endif

#ifndef __cplusplus
   typedef uint8_t bool;
   #define false 0
   #define true 1
#endif

#ifdef _MSC_VER
   #define alignof(T) __alignof(T)
#else
   #define alignof(T) (sizeof(struct {T t; char ch;}) - sizeof(T))
#endif

#endif // COMMON_H
