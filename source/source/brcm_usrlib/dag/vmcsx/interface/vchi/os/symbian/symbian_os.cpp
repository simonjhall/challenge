/*=============================================================================
Copyright (c) 2008 Broadcom Europe Limited. All rights reserved.

Project  :  
Module   :  
File     :  $RCSfile: powerman.c,v $
Revision :  $Revision$

FILE DESCRIPTION:
=============================================================================*/

#include "interface/vchi/os/os.h"

#include <kernel.h>
#include <kern_priv.h>

#include "../vcos/vcos_os.c"
#include "../vcos/vcos_cv.c"

/******************************************************************************
Private typedefs
******************************************************************************/

/******************************************************************************
Static data
******************************************************************************/

/******************************************************************************
Static func forwards
******************************************************************************/

/******************************************************************************
Global functions
******************************************************************************/


/***********************************************************
 * Name: os_logging_message
 *
 * Arguments: int level - which logging channel to use
              const char *format - printf-style formatting string
              ... - varargs
 *
 * Description: Routine to write message to log
 *
 * Returns: void
 *
 ***********************************************************/

void os_logging_message(const char *format, ...)
{
#ifdef _DEBUG
    // Aarghh - no Kern::VPrintf
    TBuf8<256> printBuf;
    VA_LIST list;
    VA_START(list,format);
    Kern::AppendFormat(printBuf,format,list);
    Kern::Printf("VC: %S", &printBuf);
#endif
}

static void my_sprintf(TDes8 &aBuf, const char *aFormat, ...)
{
    VA_LIST list;
    VA_START(list, aFormat);
    Kern::AppendFormat(aBuf, aFormat, list);
    aBuf.Append(TChar(0));
}

/***********************************************************
 * Name: os_assert_failure
 * 
 * Arguments: const char *msg - message for log (normally condition text)
 *            const char *file - filename of translation unit
 *            const char *func - name of function (NULL if not available)
 *            int line - line number
 */
void os_assert_failure(const char *msg, const char *file, const char *func, int line)
{
   TBuf8<256> printBuf;
   if (func)
       my_sprintf(printBuf, "VC: Assertion failed: %s, function %s, file %s, line %d", msg, func, file, line);
   else
       my_sprintf(printBuf, "VC: Assertion failed: %s, file %s, line %d", msg, file, line);
       
   Kern::Fault(reinterpret_cast<const char *>(printBuf.Ptr()), KErrGeneral);
}

/***********************************************************
 * Name: rand
 *
 * Arguments:  None
 *
 * Description: Generate a random number
 *
 * Returns: int
 *
 ***********************************************************/
int rand(void)
{
   NKern::LockSystem();
   TUint32 r = Kern::Random();
   NKern::UnlockSystem();
   return int(r);
}




/****************************** End of file ***********************************/
