/*********************************************************************
(C) Copyright 2002-2007 ARC International (ARC);  Santa Cruz, CA 95060
This program is the unpublished property and trade secret of ARC.   It
is to be  utilized  solely  under  license  from  ARC  and it is to be
maintained on a confidential basis for internal company use only.  The
security  and  protection  of  the program is paramount to maintenance
of the trade secret status.  It is to  be  protected  from  disclosure
to unauthorized parties, both within the Licensee company and outside,
in a manner  not less stringent than  that utilized for Licensee's own
proprietary internal information.  No  copies of  the source or Object
Code are to leave the premises of Licensee's business except in strict
accordance with the license agreement signed by Licensee with ARC.
*********************************************************************/
#include <libhdr.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "hostcom.h"
#include "hl_bios.h"
#include "hl_data.h"

#define ALIGN(x,y) ((x+(y-1))&~(y-1))

#define BREAK_TO_MON_IN_HOSTLINK 1

#if _PPC
    #define BMON_FUNC _ppc_break_to_monitor
    static void BMON_FUNC(int,done d) {
	/* The debugger auto-restarts at instr after the trap */
	_ASM("twge r30, r30");
	}
#elif _I386
    #define BMON_FUNC _x86_break_to_monitor
    extern void BMON_FUNC(int,done);
#elif _PJC
    #define BMON_FUNC pico_break_to_monitor
    static void BMON_FUNC(int,done) {
	/*
	 * The debugger auto-restarts upon breakpoint after hostlink
	 * for both simulator and hardware.
	 */
	_ASM("breakpoint");
	}
#elif _ARM
    #pragma off(check_ltorg)
    /*
     * DO NOT TURN check_ltorg ON AGAIN; otherwise with -O
     * it's as if you never said it.
     * We turn it off to prevent generation of unwanted data
     * items before the _ASM.  This helps in stepping (isi)
     * through this code.
     */
    #define BMON_FUNC arm_break_to_monitor
    static void BMON_FUNC(int,done) {
	/*
	 * The debugger auto-restarts upon breakpoint after hostlink
	 * for both simulator and hardware.
	 */
	#if _THUMB
	    _ASM(".short 0xb100");
	#elif _LE	/* ARM */
	    _ASM(".long	0xe7fddefe");
	#elif _BE	/* ARM */
	    _ASM(".long	0xe7ffdefe");
	#else
	    #error Must define either _LE or _BE for ARM/Thumb.
	#endif
	/*
	 * Causes debugger to recognize these 2 instructions as the "hostlink"
	 * breakpoint.  This causes automatic restart from the debugger.
	 */
	_ASM(".long	0xdeadbeef");
	}
#elif _MIPS
    #define BMON_FUNC _mips_break_to_monitor
    extern void BMON_FUNC(int,done);
#elif __VIDEOCORE3__
    #define BMON_FUNC _vc_break_to_monitor
    #pragma weak _vc_break_to_monitor
    void BMON_FUNC(int,done) {
	*(volatile long *)0x1800B000 = 0; // generate public message
	}
#elif _VIDEOCORE
    #define BMON_FUNC vc_break_to_monitor
    static void BMON_FUNC(int,done) {
	*(volatile long *)0x10000004 = 0; // generate public message
	}
#endif


volatile void *_hl_payload(void) {
    return (volatile void *)&__HOSTLINK__.common_buffer[0];
    }

static int _hl_cursz(volatile void *p) {
    return (volatile char *)p - (volatile char *)&__HOSTLINK__.common_buffer[0];
    }


static void init_packet(packet_header *packet, int size) {
    /* for now, make it as simple as one packet at a time to be serviced */
    packet->packet_id = 1;
    packet->total_size = ALIGN(size,4)+sizeof(packet_header);
    packet->priority = 0;
    packet->type = 0;
    packet->checksum = 0;
    }


void _hl_send(volatile void *p) {
    #ifndef BMON_FUNC
    static char first = 1;
    #endif
    packet_header *pack_hdr = &__HOSTLINK__.pack_header;
    init_packet( pack_hdr, _hl_cursz(p) );
    // have to be careful about the order these fields are assigned.
    // the debugger polls packet_address_for_host.
    __HOSTLINK__.header.common_buffer_address = (unsigned long) pack_hdr;
    __HOSTLINK__.header.common_buffer_size = _hl_buffer_size() + _HL_BUF_SLOP;
    __HOSTLINK__.header.packet_address_for_target = 0xffffffff;
    __HOSTLINK__.header.version = HOSTLINK_VERSION;
    __HOSTLINK__.header.options = BREAK_TO_MON_IN_HOSTLINK;
    #ifdef BMON_FUNC
    __HOSTLINK__.header.BMON_NAME = BMON_FUNC;
    #else
    // cr2314.  We must insure that the bmon handler is initialized.
    //     When the "emon" feature is in effect, the debugger initializes
    //     this field to contain the address of a routine that we call
    //     while waiting for hostlink to be serviced.  When "emon" is active,
    //     the debugger will initialize this field when it gets the first
    //     hostlink request from us that is version 2 or higher.  But we need
    //     to be sure the field is zeroed now since the debugger might not
    //     set the field at all, and even if it does, we are going to go
    //     right into hl_blockedPeek which polls this field and the debugger
    //     won't necessarily have set it by then.  We need the "first" logic
    //     rather than zeroing it every time because the debugger only sets
    //     the field once, and we can't overwrite it with zero after that.
    if (first) {
	__HOSTLINK__.header.BMON_NAME = (bmon_type)0;
	first = 0;
	}
    #endif
    // This tells the debugger we have a command:
    __HOSTLINK__.header.packet_address_for_host =
	    __HOSTLINK__.header.common_buffer_address;
    }


#if 0
static int done_func() {
    return __HOSTLINK__.header.packet_address_for_target != -1;
    }
#endif

volatile char* _hl_blockedPeek(void) {
    volatile _Uncached unsigned long * return_addr = 
	&__HOSTLINK__.header.packet_address_for_target;
    #define check_bmon() \
	if (__HOSTLINK__.header.BMON_NAME) \
	    __HOSTLINK__.header.BMON_NAME(1,return_addr);
    #ifdef _VIDEOCORE
    check_bmon();
    #endif
    while (*return_addr == 0xffffffff) {
	/*
	 * If we're using a debug monitor, we have to break so the
	 * monitor gains control, and then resume after the break.
	 */
	#ifndef _VIDEOCORE
	check_bmon();
	#endif
	/* may be we should have some time out mechanism? */
	}
    return _hl_payload();
    }

void _hl_delete() {
    __HOSTLINK__.header.packet_address_for_target = 0xffffffff;
    }
