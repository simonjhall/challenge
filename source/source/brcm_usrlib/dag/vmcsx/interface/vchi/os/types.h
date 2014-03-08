#ifndef OS_TYPES_H_
#define OS_TYPES_H_

#ifdef __SYMBIAN32__
#include "interface/vchi/os/symbian/symbian_types.h"
#elif defined WIN32
#include "interface/vchi/os/win32/os_native_types.h"
#else
#include "interface/vchi/os/vcfw/os_native_types.h"
#endif

#endif /*OS_TYPES_H_*/
