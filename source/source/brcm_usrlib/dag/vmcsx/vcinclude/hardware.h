/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
All rights reserved.

Project  :  VideoCore
Module   :  VideoCore hardware headers
File     :  $Id$

FILE DESCRIPTION
Public interface definition file for hardware specified registers.

Valid defines are:

   __VC4_BIG_ISLAND__      - VC4_maui, rreleased for big island

   __VIDEOCORE4__          - All VideoCoreIV based products
   
   __VIDEOCORE3__          - All VideoCoreIII based products
   __BCM2707A0__           - BCM2707A0 specific HW registers (assume B0 default)

   __VIDEOCORE2__          - All VideoCoreII based products
    _VIDEOCORE             - All VideoCore based products (really just VC01, but
                             also used to identify VIDEOCORE vs PC)
   _WIN32                  - Any PC build  

=============================================================================*/

#ifndef _HARDWARE_H
#define _HARDWARE_H
#if defined(_ATHENA_)
   #define HW_REGISTER_RW(addr) ((addr))
#else   
   #define HW_REGISTER_RW(addr) (*(volatile unsigned long *)(addr))
#endif   
#define HW_REGISTER_RO(addr) (*(const volatile unsigned long *)(addr))

#define HW_POINTER_TO_ADDRESS(pointer) ((uint32_t)(void *)&(pointer))


#if defined (__VC4_BIG_ISLAND__)
   #include "hardware_vc4_bigisland.h"
#elif defined (__VIDEOCORE4__)
   #include "hardware_vc4.h"
#elif defined (__VIDEOCORE3__)
   #include "hardware_vc3.h"
#elif defined (__VIDEOCORE2__)
   #include "hardware_vc2.h"
#elif defined (_VIDEOCORE)
   #include "hardware_vc.h"
#elif !defined (_WIN32)
   #error "No valid processor defined in hardware.h!"
#endif // __VIDEOCORE4__ && __VIDEOCORE3__ && __VIDEOCORE2__ && _VIDEOCORE && _WIN32


#endif /* _HARDWARE_H */
