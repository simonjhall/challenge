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
#include "hl_bios.h"

typedef volatile unsigned long *done;
typedef void (*bmon_type)(int version,done);
#define BMON_NAME _hl_break_to_monitor_parm

// the hostlink data buffer should have some extra slop bytes for the
// data packing overhead
#define _HL_BUF_SLOP 100
#define _HL_MAX_SCRATCH_BUF _HL_IOCHUNK+_HL_BUF_SLOP

#if _ARC
    #define HOSTLINK_VERSION 2
#else
    // Old non-ARC debuggers don't like hostlink != 1.
    #define _Uncached
    #define HOSTLINK_VERSION 1
#endif

typedef volatile struct {
    unsigned long version;
    unsigned long packet_address_for_host;
    // NONE OF THE FIELDS after this point ARE VALID until
    // first hostlink request (when packet_address_for_host != -1).  
    // The reason for waiting is that data initialization at
    // startup initializes fields in an unpredictable order, so we can't
    // count on valid values until hl_send has filled them in.
    unsigned long packet_address_for_target;
    unsigned long common_buffer_address;
    unsigned long common_buffer_size;
    unsigned long options;	// if version==2: for future options bits.
    bmon_type BMON_NAME;
    } hostlink_header;

typedef volatile struct {
    unsigned long packet_id;
    unsigned long total_size;
    unsigned long priority;
    unsigned long type;
    unsigned long checksum;
    } packet_header;


typedef volatile struct {
    hostlink_header header;
    packet_header pack_header;
    char common_buffer[_HL_MAX_SCRATCH_BUF];
    #if _ARC
    // on ARC, all volatile data must be aligned on a cache line boundary.
    // Since the ARC cache line length is configurable, we have to assume the
    // worst and pick the largest supported size.
    #define MAX_DCACHE_LINE_SIZE 256
    char cache_line_filler[MAX_DCACHE_LINE_SIZE -
	    (sizeof(hostlink_header) + 
	     sizeof(packet_header) +
	     _HL_MAX_SCRATCH_BUF) % MAX_DCACHE_LINE_SIZE ];
    #endif
    } HOSTLINK;


#ifdef _VIDEOCORE
extern _Huge HOSTLINK __HOSTLINK__;
#else
extern HOSTLINK __HOSTLINK__;
#endif
