/* 
 *
 * $Log: setvbuf.c,v $
 * Revision 1.15  1999/01/21 21:40:01  marcusm
 * Update for libhdr.h changes
 *
 * Revision 1.14  1997/11/12  23:48:34  marcusm
 * Thread stuff
 *
 * Revision 1.13  1996/01/05  18:22:18  paulb
 * putting copyright below log message for extracting purposes.
 *
 * Revision 1.12  1995/02/06  18:52:14  marka
 * Fix for new OS/2 errno implementation.
 *
 * Revision 1.11  1994/03/10  11:54:23  marcusm
 * no DECL_STATIC_PTR for NT
 *
 * Revision 1.10  1994/02/18  02:37:20  marka
 * Optimize the use of errno in multi-thread library.
 *
 * Revision 1.9  1993/10/21  20:04:34  marcusm
 * Cleared prior buffering mode bits prior to setting new one.
 * Was causing changing from line buffered to full buffering to fail.
 *
 * Revision 1.8  1993/07/28  18:01:27  marka
 * Multi-thread changes.
 *
 * Revision 1.7  1992/10/29  04:04:05  marcusm
 * *** empty log message ***
 *
 * Revision 1.6  1992/06/30  15:24:32  thomasn
 * minor_rewrite
 *
 * Revision 1.5  1992/06/02  07:46:27  tom
 * bufsiz_endp
 *
 * Revision 1.4  1991/12/09  18:43:59  thomasn
 * added logic for _FIOALLOCATED
 *
 * Revision 1.3  1991/11/19  22:32:07  thomasn
 * use_nonrelative_includes
 *
 * Revision 1.2  1991/11/08  16:11:21  thomasn
 * fix_defines
 *
 * Revision 1.1  1991/09/20  16:23:26  thomasn
 * libinstall on file owned by: thomasn
 ***/

/*
 * (c) Copyright (c) 2002 ARC International.
 */


#include <fio.h>
#include <stdlib.h>
#include <errno.h>

#include "vcfw/rtos/rtos.h"
#include "vcfw/drivers/chip/cache.h"
#include "interface/vcos/vcos.h"

int setvbuf(FILE *fp, char *buf, int type, size_t size) {
    int ret;

    /* If already buffered, then error
     */
    LOCK_FILE(fp)
    if (_base(fp)!=NULL){
        errno = EIO;
        RETURN(-1)
        }
    switch(type & (_IONBF|_IOFBF|_IOLBF)){
        case _IONBF: 
            _base(fp) = _ptr(fp) = NULL;
            _cnt(fp) = 0;
            break;
        case _IOFBF:
        case _IOLBF:
            if (buf == NULL){
                if (size<=0) {
                    errno = EINVAL;
                    RETURN(-1)
                    }
                if (_ioflag(fp) & _IOSHRMEM)
                    buf = _smalloc(size);
                else
                    buf = rtos_prioritymalloc(size, RTOS_ALIGN_256BIT, RTOS_PRIORITY_L1_NONALLOCATING, "setvbuf");
                if (buf == NULL){
                    errno = ENOMEM;
                    RETURN(-1)
                    }
                _ioflag(fp) |= _IOMYBUF;
                } else {
#ifdef __VIDEOCORE4__
                    // This buffer is read/written through DMA, so best to align to cache line sizes
                    char *aligned_start = (char *)ALIGN_UP(buf, 32);
                    char *aligned_end   = (char *)ALIGN_DOWN(buf+size, 32);
                    size = aligned_end - aligned_start;
                    buf = aligned_start;
                    // We are better off being a multiple of sector size
                    if (size > 512) size = ALIGN_DOWN(size, 512);
                    if (size<=0) {
                        errno = EINVAL;
                        RETURN(-1)
                        }
                    if (!RTOS_IS_ALIAS_NOT_L1(buf)) {
                        static const CACHE_DRIVER_T *cache_driver = NULL;
                        static DRIVER_HANDLE_T cache_handle = NULL;
                        //get a handle to the cache driver
                        if (!cache_driver) {
                            int32_t success;
                            cache_driver = cache_get_func_table();
                            vcos_demand( NULL != cache_driver );
                            //open the intctrlr and get a handle
                            success = cache_driver->open( NULL, &cache_handle );
                            vcos_demand( success >= 0 );
                        }
                        cache_driver->invalidate_range( cache_handle, CACHE_DEST_L1, buf, size );
                        buf = RTOS_ALIAS_L1_NONALLOCATING(buf);
                   }
#endif
				}
            _base(fp) = (void *)buf;
            _ptr(fp) = (void *)buf;
            #if (_IOB_BUFTYP == _IOB_BUFSIZ)
                _bufsiz(fp) = size;
            #else
                _bufend(fp) = _base(fp) + size;
                #if (_IOB_BUFTYP == _IOB_BUFSIZ_ENDP)
                    _bufsiz(fp) = size;
                #endif
            #endif
            break;
        default:
            errno = EINVAL;
            RETURN(-1)
        }
            
    _ioflag(fp) &= ~(_IONBF|_IOFBF|_IOLBF); /* clear any prior buffering mode */
    _ioflag(fp) |= type;
    ret = 0;
RETURN:
    UNLOCK_FILE(fp)
    return ret;
    }
