/*****************************************************************************
*  Copyright 2001 - 2007 Broadcom Corporation.  All rights reserved.
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


/*
*
*****************************************************************************
*
*  vchost_int.h
*
*  PURPOSE:
*
*   This file contains private definitions to be shared amongst the VC02
*   Host Interface driver files.
*
*  NOTES:
*
*****************************************************************************/


#if !defined( VCHOST_INT_H )
#define VCHOST_INT_H

/* ---- Include Files ---------------------------------------------------- */

#include <linux/list.h>
#include <cfg_global.h>

#include <vc3host-port.h>

/* ---- Constants and Types ---------------------------------------------- */

typedef struct
{
    unsigned            magic;
    int                 index;
    struct list_head    node;
    wait_queue_head_t   waitQ;
    volatile int        flag;

#if VC_EVENT_DEBUG
    const char         *createFileName;
    int                 createLineNum;
    const char         *fileName;
    char                fileNameBuf[ 200 ];
    int                 lineNum;
#endif

} Event_t;

typedef struct
{
    unsigned            magic;
    struct list_head    node;
    struct semaphore    sem;

#if VC_LOCK_DEBUG
    const char         *fileName;
    char                fileNameBuf[ 200 ];
    int                 lineNum;
#endif

} Lock_t;

/* ---- Variable Externs ------------------------------------------------- */

extern   int   gLcdPrintStats;

extern   int   gVcGmtOffset;

/* ---- Function Prototypes ---------------------------------------------- */

void *vc_lock_create_on( struct list_head *list );

void vc_lock_free_list( struct list_head *list );


void vc_event_free_list( struct list_head *list );

#if VC_EVENT_DEBUG
#define vc_event_create_on( list )    vc_event_create_on_dbg( list, __FILE__, __LINE__ )

void *vc_event_create_on_dbg( struct list_head *list, const char *fileName, int lineNum );
#else
void *vc_event_create_on( struct list_head *list );

#define  vc_event_create_on_dbg( list, fileName, lineNum )  vc_create_on( list ) 

#endif

void vc_adjust_thread_priority( int *requestedPriority );


#endif  // VCHOST_INT_H
