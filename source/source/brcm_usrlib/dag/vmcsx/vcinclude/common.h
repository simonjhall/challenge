/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
Copyright (c) 2005 Alphamosaic Limited.
All rights reserved.

Project  :  VideoCore
Module   :  VideoCore specific header (common)
File     :  $RCSfile: common.h,v $
Revision :  $Revision$

FILE DESCRIPTION
Common types and helper macros for C/C++.
=============================================================================*/

#ifndef __VC_INCLUDE_COMMON_H__
#define __VC_INCLUDE_COMMON_H__

#if defined(__HIGHC__)                     // This is only available with MW
#include <vc/intrinsics.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
#ifdef __SYMBIAN32__
# ifndef INLINE
#  define INLINE __inline
# endif

typedef signed   char      int8_t;
typedef unsigned char      uint8_t;

typedef signed   short     int16_t;
typedef unsigned short     uint16_t;

typedef int16_t            int_least16_t;

typedef signed   long      int32_t;
typedef unsigned long      uint32_t;

typedef signed long long   int64_t;
typedef unsigned long long uint64_t;

typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;

typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;

/* We have so many rectangle types; let's try to introduce a common one. */
typedef struct tag_VC_RECT_T {
   int32_t x;
   int32_t y;
   int32_t width;
   int32_t height;
} VC_RECT_T;

/* Align a pointer/integer by rounding up/down */
#define ALIGN_DOWN(p, n)   ((uint32_t)(p) - ( (uint32_t)(p) % (uint32_t)(n) ))
#define ALIGN_UP(p, n)     ALIGN_DOWN((uint32_t)(p) + (uint32_t)(n) - 1, (n))

# else // __SYMBIAN32__

/*{{{ C99 types  - THIS WHOLE SECTION IS INCOMPATIBLE WITH C99. IT SHOULD RESIDE IN A STDINT.H SINCE THIS FILE GETS USED ON HOST SIDE */

#if defined( __STDC__ ) && __STDC_VERSION__ >= 199901L

#include <stdint.h>

#elif defined( __GNUC__ )

#include <stdint.h>

#elif defined(_MSC_VER)                     // Visual C define equivalent types

#include <stddef.h> // Avoids intptr_t being defined in vadefs.h

typedef signed   __int8    int8_t;
typedef unsigned __int8    uint8_t;

typedef          __int16   int16_t;
typedef unsigned __int16   uint16_t;

typedef          __int32   int32_t;
typedef unsigned __int32   uint32_t;

typedef          __int64   int64_t;
typedef unsigned __int64   uint64_t;
typedef uint32_t           uintptr_t;
typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;
typedef int16_t            int_least16_t;

#elif defined (VCMODS_LCC)
#include <limits.h>

typedef signed   char      int8_t;
typedef unsigned char      uint8_t;

typedef signed   short     int16_t;
typedef unsigned short     uint16_t;

typedef signed   long      int32_t;
typedef unsigned long      uint32_t;

typedef signed   long      int64_t; //!!!! PFCD, this means code using 64bit numbers will be broken on the VCE
typedef unsigned long      uint64_t; // !!!! PFCD

#define INT8_MIN SCHAR_MIN
#define INT8_MAX SCHAR_MAX
#define UINT8_MAX UCHAR_MAX
#define INT16_MIN SHRT_MIN
#define INT16_MAX SHRT_MAX
#define UINT16_MAX USHRT_MAX
#define INT32_MIN LONG_MIN
#define INT32_MAX LONG_MAX
#define UINT32_MAX ULONG_MAX
#define INT64_MIN LONG_MIN // !!!! PFCD
#define INT64_MAX LONG_MAX // !!!! PFCD
#define UINT64_MAX ULONG_MAX // !!!! PFCD

typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;
typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;
typedef int16_t            int_least16_t;

#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define INTMAX_MIN INT64_MIN
#define INTMAX_MIN INT64_MIN
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST16_MAX INT16_MAX

#else
#include <limits.h>
typedef signed   char      int8_t;
typedef unsigned char      uint8_t;

typedef signed   short     int16_t;
typedef unsigned short     uint16_t;

typedef signed   long      int32_t;
typedef unsigned long      uint32_t;

typedef signed   long long int64_t;
typedef unsigned long long uint64_t;

#define INT8_MIN SCHAR_MIN
#define INT8_MAX SCHAR_MAX
#define UINT8_MAX UCHAR_MAX
#define INT16_MIN SHRT_MIN
#define INT16_MAX SHRT_MAX
#define UINT16_MAX USHRT_MAX
#define INT32_MIN LONG_MIN
#define INT32_MAX LONG_MAX
#define UINT32_MAX ULONG_MAX
#define INT64_MIN LLONG_MIN
#define INT64_MAX LLONG_MAX
#define UINT64_MAX ULLONG_MAX

typedef int32_t            intptr_t;
typedef uint32_t           uintptr_t;
typedef int64_t            intmax_t;
typedef uint64_t           uintmax_t;
typedef int16_t            int_least16_t;

#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define INT_LEAST16_MAX INT16_MAX
#define INT_LEAST16_MAX INT16_MAX
#endif


/*}}}*/

/* Fixed-point types */
typedef unsigned short uint8p8_t;
typedef signed short sint8p8_t;
typedef unsigned short uint4p12_t;
typedef signed short sint4p12_t;
typedef signed short sint0p16_t;
typedef signed char sint8p0_t;
typedef unsigned char uint0p8_t;

/*{{{ Common typedefs */

typedef enum bool_e
{
   VC_FALSE = 0,
   VC_TRUE = 1,
} bool_t;

typedef uint32_t fourcc_t; // the preferred form
typedef uint32_t FOURCC_T; // kept for compatibility

/* We have so many rectangle types; let's try to introduce a common one. */
typedef struct tag_VC_RECT_T {
   int32_t x;
   int32_t y;
   int32_t width;
   int32_t height;
} VC_RECT_T;

/*}}}*/

/*{{{ Common macros */

/* Length of constant arrays and strings. Lower case to match sizeof()... */
#define countof(arr) (sizeof(arr) / sizeof(arr[0]))
#define endof(arr) ((arr) + countof(arr))
#define lenof(str) (sizeof(str) - 1)

/* Align a pointer/integer by rounding up/down */
#define ALIGN_DOWN(p, n)   ((uintptr_t)(p) - ( (uintptr_t)(p) % (uintptr_t)(n) ))
#define ALIGN_UP(p, n)     ALIGN_DOWN((uintptr_t)(p) + (uintptr_t)(n) - 1, (n))

#define CLIP(lower, n, upper) _min((upper), _max((lower), (n)))

/*}}}*/

/*{{{ Debugging and profiling macros */

#if 0
/* There's already an assert_once in <logging/logging.h> */
#ifdef DEBUG
#define assert_once(x) \
   { \
      static uint8_t ignore = 0; \
      if(!ignore) \
      { \
         assert(x); \
         ignore++; \
      } \
   }
#else
#define assert_once(x) (void)0
#endif
#endif /* 0 */

#if defined(__HIGHC__) && !defined(NDEBUG)
/* HighC lacks a __FUNCTION__ preproc symbol... :( */
#define profile_rename(name) _ASM(".global " name "\n" name ":\n")
#else
#define profile_rename(name) (void)0
#endif

# endif // SYMBIAN
/*}}}*/
#ifdef __cplusplus
 }
#endif
#endif /* __VCINCLUDE_COMMON_H__ */

