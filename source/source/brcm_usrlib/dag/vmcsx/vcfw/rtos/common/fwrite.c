/* 
 *
 * $Log: fwrite.c,v $
 * Revision 1.13  2000/02/17 02:05:42  marcusm
 * Eliminated the divide at the end of fwrite.
 *
 * Revision 1.12  2000/02/16 03:38:30  marcusm
 * modify return to avoid the divide.
 *
 * Revision 1.11  1996/01/05  18:22:07  paulb
 * putting copyright below log message for extracting purposes.
 *
 * Revision 1.10  1994/02/18  02:37:03  marka
 * Remove unnecessary saving and restoring of errno.
 *
 * Revision 1.9  1993/10/05  23:42:14  marka
 * Don't use EINTR on PenPoint.
 *
 * Revision 1.8  1993/08/06  23:26:40  marcusm
 * *** empty log message ***
 *
 * Revision 1.7  1993/07/28  18:01:27  marka
 * Multi-thread changes.
 *
 * Revision 1.6  1993/03/05  18:48:01  budis
 * changed rvalue reference to _fd() to __fileno() (byte reversal problem in
 * _UPA).
 *
 * Revision 1.5  1992/08/19  20:04:00  marcusm
 * Changed flush logic to call library internal routine so that an actual
 * system call to flush to disk is not invoked unless user calls fflush.
 *
 * Revision 1.4  1992/06/30  15:24:29  thomasn
 * minor_rewrite
 *
 * Revision 1.3  1991/12/02  20:20:56  thomasn
 * cp not correctly bumped
 *
 * Revision 1.2  1991/11/19  22:34:52  thomasn
 * use_nonrelative_includes
 *
 * Revision 1.1  1991/09/20  16:23:02  thomasn
 * libinstall on file owned by: thomasn
 ***/

/*
 * (c) Copyright (c) 2002 ARC International.
 */

#include <internal/fio.h>
#include <string.h>
#include <errno.h>

size_t fwrite(const char *cp, size_t elem, size_t count, FILE *fp) {

    LOCK_FILE(fp)
    if (_ioflag(fp) & _IOERR) {	// Error pending?
	UNLOCK_FILE(fp)
	return 0;
	}

    int amount = count*elem;	// Compute byte count
    int bytes_written = 0;
    int bytes_requested = amount;

    /*
     * If this is the first operation on the file, then we need
     * to initialize it.
     */
    if (!_isinit(fp))
	if (_finit(fp) == -1) {
	    UNLOCK_FILE(fp)
	    return 0;
	    }
    /*
     * If non-buffered then just call write
     */
    if (_ioflag(fp) & _IONBF){
	bytes_written = WRITE(__fileno(fp),(char *)cp,amount);
	if (bytes_written < 0){
	    bytes_written = 0;
#ifndef __PENPOINT__
	    if (errno != EINTR)
#endif
		_ioflag(fp) |= _IOERR;
	    }
	}
    else{

	if (!(_ioflag(fp) & _IOWRT))
	{
	    /* previous operation was not WRITE */
	    _cnt(fp) = _bufsiz(fp);
	    _ptr(fp) = _base(fp);
	    _ioflag(fp) |= _IOWRT;

	    /* ensure file location corresponds with */
	    /* beginning of buffer... else FTELL fails */
	    if (_fioflag(fp) & _FIOAPPEND) {
		long cur_pos = SEEK (__fileno(fp), 0, SEEK_CUR);
		long end_pos = SEEK (__fileno(fp), 0, SEEK_END);
		if (cur_pos > end_pos)
		    SEEK (__fileno(fp), cur_pos, SEEK_SET);
		}
	}

	/* Consider copying some data via the output buffer:
	 * - If we already have data in the output buffer,
	 *   copy write data to the buffer until it is
	 *   full, or we run out of data.
	 * - If the output buffer is empty, and we have less than a buffer's
	 *   worth of data to write, copy our data into the buffer.
	 * - If the output buffer is empty, and we have at least a full buffer
	 *   of data, ignore the buffer.
	 */
	if( (_cnt(fp) < _bufsiz(fp)) || (amount < _bufsiz(fp)))
	{
	    _iob_cnt_t bytes_left_in_buffer = _cnt(fp);
	    if (bytes_left_in_buffer > 0)
	    {
		size_t a = _min(bytes_left_in_buffer,amount);
		memcpy(_ptr(fp),cp,a);
		_ptr(fp) += a;
		_cnt(fp) -= a;
		bytes_written += a;
		cp += a;
		amount -= a;
	    }

	    /* If the buffer is now full, flush it: */
	    if( _cnt(fp) <= 0 )
		_ifflush(fp, internal => 1);
	}

	if (amount > 0)
	{
	    if ( !(_ioflag(fp) & _IOERR) )
	    {
		long cnt;
		if (amount < _bufsiz(fp))
		{
		    cnt = amount;
		    /* after a flush, we always have an empty buffer */
		    memcpy(_ptr(fp),cp,(size_t)cnt);
		    _ptr(fp) += cnt;
		    _cnt(fp) -= cnt;
		}
		else
		{
		    /* bypass buffering if remaining length exceeds buffer size */
		    cnt = WRITE(__fileno(fp),(char *)cp,amount);
		    // Check for error
		    if (cnt < 0){
			cnt = 0;
#ifndef __PENPOINT__
			if (errno != EINTR)
#endif
			    _ioflag(fp) |= _IOERR;
			}
		}
		bytes_written += cnt;
		cp += cnt;
	    } /* ! _IOERR */
	} /* amount > 0 */

	/*
	 * If line-buffered, and the last byte written was a newline,
	 * then flush the buffer.
	 */
	if ( !(_ioflag(fp) & _IOERR) && (_ioflag(fp) & _IOLBF) &&
		(cp[-1] == '\n') && (fp->_ptr > fp->_base) )
	    _ifflush(fp, internal => 1);
	}
    UNLOCK_FILE(fp)
    return elem == 0 ? 0 :
            (bytes_written == bytes_requested ? count : bytes_written/elem);
    }
