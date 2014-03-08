/*=============================================================================
Copyright (c) 2009 Broadcom Europe Limited.
All rights reserved.

Project  :  vcfw
Module   :  chip driver
File     :  $RCSfile: $
Revision :  $Revision: $

FILE DESCRIPTION
VideoCore OS Abstraction Layer - logging support
=============================================================================*/

#ifndef VCOS_LOGGING_H
#define VCOS_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>

#include "interface/vcos/vcos_types.h"
#include "vcos_platform.h"

/**
 * \file
 *
 * Logging support
 *
 * This provides categorised logging. Clients register
 * a category, and then get a number of logging levels for
 * that category.
 *
 * The logging level flag is tested using a flag *before* the
 * function call, which makes logging very fast when disabled - there
 * is no function call overhead just to find out that this log
 * message is disabled.
 *
 * \section VCOS_LOG_CATEGORY
 *
 * As a convenience, clients define VCOS_LOG_CATEGORY to point to
 * their category; the various vcos_log_xxx() macros then expand to
 * use this.
 *
 * e.g.
 *
 *     #define VCOS_LOG_CATEGORY (&my_category)
 *
 *     #include <interface/vcos/vcos.h>
 *
 *     VCOS_LOG_CATEGORY my_category;
 *
 *     ....
 *
 *     vcos_log_trace("Stuff happened: %d", n_stuff);
 *
 */

/** Logging levels */
typedef enum VCOS_LOG_LEVEL_T
{
   VCOS_LOG_NEVER,
   VCOS_LOG_ERROR,
   VCOS_LOG_WARN,
   VCOS_LOG_INFO,
   VCOS_LOG_TRACE,
} VCOS_LOG_LEVEL_T;

/** A registered logging category.
  */
typedef struct VCOS_LOG_CAT_T
{
   VCOS_LOG_LEVEL_T level;      /** Which levels are enabled for this category */
   const char *name;            /** Name for this category. */
   struct VCOS_LOG_CAT_T *next;
   struct {
      int want_prefix:1;
   } flags;
} VCOS_LOG_CAT_T;



/** Log a message. Basic API. Normal code should not use this.
  * The platform needs to implement this function.
  */
VCOSPRE_ void VCOSPOST_ vcos_log_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, ...);

/** Log a message using a varargs parameter list. Normal code should
  * not use this.
  */
VCOSPRE_ void VCOSPOST_ vcos_vlog_impl(const VCOS_LOG_CAT_T *cat, const char *fmt, va_list args);

/*
 * Initialise the logging subsystem. This is called from
 * vcos_init() so you don't normally need to call it.
 */
VCOSPRE_ void VCOSPOST_ vcos_logging_init(void);

/** Register a logging category.
  *
  * @param name the name of this category.
  * @param category the category to register.
  */
VCOSPRE_ void VCOSPOST_ vcos_log_register(const char *name, VCOS_LOG_CAT_T *category);

/** Unregister a logging category.
  */
VCOSPRE_ void VCOSPOST_ vcos_log_unregister(VCOS_LOG_CAT_T *category);

/** Return a default logging category, for people too lazy to create their own.
  *
  * Using the default category will be slow (there's an extra function
  * call overhead). Don't do this in normal code.
  */
VCOSPRE_ const VCOS_LOG_CAT_T * VCOSPOST_ vcos_log_get_default_category(void);

VCOSPRE_ void VCOSPOST_ vcos_set_log_options(const char *opt);

/** Set the logging level for a category at run time. Without this, the level
  * will be that set by vcos_log_register from a platform-specific source.
  *
  * @param category the category to modify.
  * @param level the new logging level for this category.
  */
VCOS_STATIC_INLINE void vcos_log_set_level(VCOS_LOG_CAT_T *category, VCOS_LOG_LEVEL_T level)
{
   category->level = level;
}

/* VCOS_TRACE() - deprecated macro which just outputs in a debug build and
 * is a no-op in a release build.
 *
 * _VCOS_LOG_X() - internal macro which outputs if the current level for the
 * particular category is higher than the supplied message level.
 */

#define VCOS_LOG_DFLT_CATEGORY vcos_log_get_default_category()

#define _VCOS_LEVEL(x) (x)

#if defined(_VCOS_METAWARE) || defined(__GNUC__)

# ifndef NDEBUG
#  define _VCOS_LOG_X(cat, _level, fmt...) do { if ((_VCOS_LEVEL(cat->level)) >= (_VCOS_LEVEL(_level))) vcos_log_impl(cat,fmt); } while (0)
#  define _VCOS_VLOG_X(cat, _level, fmt, ap) do { if (_VCOS_LEVEL(cat->level) >= _VCOS_LEVEL(_level)) vcos_vlog_impl(cat,fmt,ap); } while (0)
# else
#  define _VCOS_LOG_X(cat, _level, fmt...) (void)0
#  define _VCOS_VLOG_X(cat, _level, fmt, ap) (void)0
# endif


# define vcos_log_error(fmt...)   _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_ERROR, fmt)
# define vcos_log_warn(fmt...)    _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_WARN, fmt)
# define vcos_log_info(fmt...)    _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_INFO, fmt)
# define vcos_log_trace(fmt...)   _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_TRACE, fmt)

# define vcos_vlog_error(fmt,ap)  _VCOS_VLOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_ERROR, fmt, ap)
# define vcos_vlog_warn(fmt,ap)   _VCOS_VLOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_WARN, fmt, ap)
# define vcos_vlog_info(fmt,ap)   _VCOS_VLOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_INFO, fmt, ap)
# define vcos_vlog_trace(fmt,ap)  _VCOS_VLOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_TRACE, fmt, ap)

# define vcos_log(fmt...)   _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_INFO, fmt)
# define vcos_vlog(fmt...)  _VCOS_VLOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_INFO, fmt)
# define VCOS_ALERT(fmt...) _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_ERROR, fmt)
# define VCOS_TRACE(fmt...) _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_INFO, fmt)

/*
 * MS Visual Studio - pre 2005 does not grok variadic macros
 */
#elif defined(_MSC_VER)

# if _MSC_VER >= 1400

#  ifndef NDEBUG
#   define _VCOS_LOG_X(cat, _level, fmt,...) do { if (cat->level >= _level) vcos_log_impl(cat, fmt, __VA_ARGS__); } while (0)
#  else
#   define _VCOS_LOG_X(cat, _level, fmt,...) (void)0
#  endif

# define vcos_log_error(fmt,...)   _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_ERROR, fmt, __VA_ARGS__)
# define vcos_log_warn(fmt,...)    _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_WARN, fmt, __VA_ARGS__)
# define vcos_log_info(fmt,...)    _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_INFO, fmt, __VA_ARGS__)
# define vcos_log_trace(fmt,...)   _VCOS_LOG_X(VCOS_LOG_CATEGORY, VCOS_LOG_TRACE, fmt, __VA_ARGS__)

# define vcos_log(fmt,...)   _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_INFO, fmt)
# define VCOS_ALERT(fmt,...) _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_ERROR, fmt)
# define VCOS_TRACE(fmt,...) _VCOS_LOG_X(VCOS_LOG_DFLT_CATEGORY, VCOS_LOG_INFO, fmt)

# else /* _MSC_VER >= 1400 */

/* do not define these */

# endif /* _MSC_VER >= 1400 */

#endif


#ifdef __cplusplus
}
#endif
#endif /* VCOS_LOGGING_H */


