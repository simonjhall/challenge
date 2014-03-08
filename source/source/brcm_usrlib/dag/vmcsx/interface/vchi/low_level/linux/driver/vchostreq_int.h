/*****************************************************************************
*  Copyright 2006 - 2007 Broadcom Corporation.  All rights reserved.
*
*  Unless you and Broadcom execute a separate written software license
*  agreement governing use of this software, this software is licensed to you
*  under the terms of the GNU General Public License version 2, available at
*  http://www.gnu.org/copyleft/gpl.html (the "GPL").
*
*  Notwithstanding the above, under no circumstances may you combine this
*  software in any way with any other Broadcom software provided under a
*  license other than the GPL, without Broadcom's express prior written
*  consent.
*
*****************************************************************************/


#ifndef VCHOSTREQ_INT_H
#define VCHOSTREQ_INT_H

/* Initialize the host request service. */
extern int vc_hostreq_init (void);

/* Stop the service from being used. */
extern void vc_hostreq_stop(void);

/* Return the service number (-1 if not running). */
extern int vc_hostreq_inum(void);

/* Poll host request fifo and process any messages there */
extern int vc_hostreq_poll_message_fifo(void);

/* Return the event used to wait for reads. */
extern void * vc_hostreq_read_event(void);

#endif
