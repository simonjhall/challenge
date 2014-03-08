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

#if _ARC
    // turn this off for compatibility and because we don't need it here since
    // we have taken care to align and pad the uncached data.
    pragma off(uncached_in_own_section);

    // 24-Feb-05  Moving forward, we'd like to eliminate use of
    // align_to and cache_line_filler and just put hostlink in the
    // .ucdata section.
    // There is some concern from tech support that introducing a .ucdata
    // section from the runtime could cause old customer's linker command
    // files to need changing.  I'm not sure how much of a problem that
    // is, since one would assume that final builds would not have
    // hostlink in them anyways, and simulated builds with hostlink
    // won't care where the .ucdata section is put by the linker.
#endif

#include "hl_data.h"

#if _VIDEOCORE
    pragma Data(".ucdata");
_Huge HOSTLINK __HOSTLINK__ = {
#else
HOSTLINK __HOSTLINK__ = {
#endif
    /* header */
    {HOSTLINK_VERSION, 0xffffffff, }
    // The rest of these fields are initialized to 0.
    // We count on this: we need BMON_NAME to be 0, as hl_send does not
    // initialize it.
    };
#if _ARC
    pragma align_to(MAX_DCACHE_LINE_SIZE, __HOSTLINK__);
#endif
#if _VIDEOCORE
    pragma Data();
#endif

unsigned _hl_buffer_size(void) { return _HL_IOCHUNK; }
