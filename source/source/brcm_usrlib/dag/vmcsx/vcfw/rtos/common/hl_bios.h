/*********************************************************************
(C) Copyright (c) 2006 ARC International;  Santa Cruz, CA 95060
*********************************************************************/

#ifndef __hl_bios_h__
#define __hl_bios_h__

#ifdef __CPLUSPLUS__
extern "C" {
#endif

#ifdef _HL_IOCHUNK	
    /* You have already specified IO chunk size.  32K is a good choice. */
    #print Using _HL_IOCHUNK as hostlink buffer size.
#elif _PJC
/*
 * Big HL request to reduce # of packets for hostlink request processing.
 * But limit it to below 64K because the length of a string (e.g., IO
 * write) is 2 bytes, not 4.
 */
    #define _HL_IOCHUNK	65000
#else
    #define _HL_IOCHUNK	1024
#endif

extern void _hl_send(volatile void *p);
extern volatile char* _hl_blockedPeek(void);
extern void _hl_delete(void);
extern volatile void *_hl_payload(void);

// normally returns _HL_IOCHUNK but could be more or less if -Hhlsize= used.
extern unsigned _hl_buffer_size(void);

#ifdef __CPLUSPLUS__
}
#endif

#endif /* __hl_bios_h__ */
