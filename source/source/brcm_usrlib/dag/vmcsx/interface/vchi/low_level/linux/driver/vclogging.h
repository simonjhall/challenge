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

#ifndef VCLOGGING_H
#define VCLOGGING_H

#include <vclogging_int.h>

typedef struct
{
   unsigned int errorCount;
   unsigned int countThisPeriod;
   unsigned int periodExpiresJiffies;

} VC_ERRORlog_t;

int vc_retrieve_assert_log
(
   uint32_t          log_address,
   uint32_t         *time,
   unsigned short   *seqNum,
   char             *filename,
   uint32_t          maxFileNameLen,
   uint32_t         *lineno,
   char             *cond,
   uint32_t          maxCondLen
);

void vc_hostdrv_errlog( char * msg, VC_ERRORlog_t *errorLog );

#endif // VCLOGGING_H
