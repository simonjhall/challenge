/*=============================================================================
Copyright (c) 2006 Broadcom Europe Limited.
Copyright (c) 2002 Alphamosaic Limited.
All rights reserved.

Project  :  VideoCore Software Host Interface (Host-side functions)
Module   :  File system (host-side)
File     :  $RCSfile $
Revision :  $Revision $

FILE DESCRIPTION
File service command enumeration.
=============================================================================*/

#ifndef VC_VCHI_FILESERVICE_DEFS_H
#define VC_VCHI_FILESERVICE_DEFS_H

#include "interface/vchi/vchi.h"

/* Definitions (not used by API) */

/* structure used by both side to communicate */
#define FILESERV_MAX_BULK_SECTOR  128   //must be power of two

#define FILESERV_SECTOR_LENGTH  512

#define FILESERV_MAX_BULK (FILESERV_MAX_BULK_SECTOR*FILESERV_SECTOR_LENGTH)

#define FILESERV_4CC  MAKE_FOURCC("FSRV")

typedef enum FILESERV_EVENT_T
{
   FILESERV_BULK_RX = 0,
   FILESERV_BULK_TX,
   FILESERV_BULK_RX_0,
   FILESERV_BULK_RX_1
}FILESERV_EVENT_T;
//this following structure has to equal VCHI_MAX_MSG_SIZE
#define FILESERV_MAX_DATA	(VCHI_MAX_MSG_SIZE - 40) //(VCHI_MAX_MSG_SIZE - 24)

typedef struct{
	uint32_t xid;		    //4 // transaction's ID, used to match cmds with response
   uint32_t cmd_code;    //4
   uint32_t params[4];   //16
   char  data[FILESERV_MAX_DATA];
}FILESERV_MSG_T;

typedef enum
{
   FILESERV_RESP_OK,
   FILESERV_RESP_ERROR,
   FILESERV_BULK_READ,
   FILESERV_BULK_WRITE,

} FILESERV_RESP_CODE_T;


/* Protocol (not used by API) version 1.2 */



#endif
