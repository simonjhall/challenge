/* 
 *
 * $Log: fread.c,v $
 * Revision 1.17  1997/06/24 23:25:01  marcusm
 * clearerr if a tty and EOF/ERR is set.
 * Allows cleaner ctrl-C handling.
 *
 * Revision 1.16  1996/01/05  18:22:04  paulb
 * putting copyright below log message for extracting purposes.
 *
 * Revision 1.15  1995/05/08  21:11:35  mon
 * changed code to take into account that for text mode files, the carriage-
 * return/newline pair is counted as one char, not two, so that the correct
 * amount of data is read when amount is greater than buffer size.
 *
 * Revision 1.14  1994/02/18  02:36:15  marka
 * Remove unnecessary saving and restoring of errno.
 *
 * Revision 1.13  1993/10/05  23:41:54  marka
 * Don't use EINTR on PenPoint.
 *
 * Revision 1.12  1993/08/06  23:26:38  marcusm
 * *** empty log message ***
 *
 * Revision 1.11  1993/07/28  18:01:27  marka
 * Multi-thread changes.
 *
 * Revision 1.10  1993/03/05  18:38:30  budis
 * changed rvalue reference to _fd() to __fileno() (for byte reversal
 * problem in _UPA).
 *
 * Revision 1.9  1992/11/14  21:51:14  marcusm
 * Reset buffer state when doing large READs which bypass the buffer.  Was
 * causing a subsequent fseek to think that the current buffer represented
 * the data one buffer back from the current system file pointer.
 *
 * Revision 1.8  1992/09/07  19:52:55  marcusm
 * Fixed setting of EOF flag when reading less than the desired amount for
 * unbuffered i/o.
 *
 * Revision 1.7  1992/06/30  15:24:21  thomasn
 * minor_rewrite
 *
 * Revision 1.6  1991/11/19  22:38:17  thomasn
 * use_nonrelative_includes
 *
 * Revision 1.5  1991/11/07  20:01:30  thomasn
 * fix_aux_references
 *
 * Revision 1.4  1991/11/04  20:45:52  pickens
 * Don't invoke fread if EOF pending.
 * Don't invoke "read" system call, if last call returned zero.
 *
 * Revision 1.3  1991/10/11  16:38:16  pickens
 * EOF never getting set for buffered calls to fread!
 *
 * Revision 1.2  1991/09/20  21:31:11  thomasn
 * fix char sign
 *
 * Revision 1.1  1991/09/20  16:22:51  thomasn
 * libinstall on file owned by: thomasn
 ***/

/*
 * (c) Copyright (c) 2002 ARC International.
 */

#include <string.h>
#include <fio.h>
#include <errno.h>

#define BUFFER_ALIGN_SIZE	(512)

size_t fread(char *cp, size_t elem, size_t count, FILE *fp)
{
    size_t ret;

    LOCK_FILE(fp)
    if (_ioflag(fp) & (_IOERR|_IOEOF)) {	// Error or EOF pending?
	#ifdef ISATTY
	    // If reading from console, clear prior EOF or other ERR status.
	    // On NT for example EOF gets set if ctrl-C is typed while in
	    // the ReadFile system call--ReadFile returns 0 bytes read.
	    if (ISATTY(__fileno(fp)))
		clearerr(fp);
	    else
		RETURN(0)
	#else
	    RETURN(0)
	#endif
	}

    int amount = count*elem;	// Compute byte count
    int bytes_read = 0;

    /* initialize if needed */
    if (!_isinit(fp)) 
	if (_finit(fp) == -1)
	    RETURN(0)

    /* see if unget buffer has anything in it */
    _iob_unget_t *unget = &_unget(fp);
    if (unget->_ptr > unget->_base) {
	bytes_read = _min(amount, (unget->_ptr - unget->_base));
	int i;
	for (i = 0; i < bytes_read; i++)
	    *cp++ = *--unget->_ptr & 0xff;
	if (unget->_ptr == unget->_base)
	    _cnt(fp) = unget->_iob_cnt;
	amount -= bytes_read;
	if (amount == 0)
	    goto DONE;
	}

    /* if non-buffered, then just call READ */
    if (_ioflag(fp) & _IONBF){
	int read_cnt;
	read_cnt = READ(__fileno(fp), cp, amount);

	if ( read_cnt < 0 ) {
#ifndef __PENPOINT__
	    if (errno != EINTR)
#endif
		{
		_ioflag(fp) |= _IOEOF;
		_ioflag(fp) |= _IOERR;
		}
	    }
	else {
	    bytes_read += read_cnt;
	    if ( read_cnt < amount )
		_ioflag(fp) |= _IOEOF;
	    }
	}
    else
    {
	if (!(_ioflag(fp) & _IOREAD)) {
	    /* previous operation was not READ */
	    _ptr(fp) = _base(fp);
	    _cnt(fp) = 0;
	    _ioflag(fp) |= _IOREAD;
	    }

	/* read rest of input buffer */
	_iob_cnt_t bytes_left_in_buffer = _cnt(fp);
	if (bytes_left_in_buffer > 0){
	    size_t a = _min(bytes_left_in_buffer,amount);
	    memcpy(cp,_ptr(fp),a);
	    _ptr(fp) += a;
	    _cnt(fp) -= a;
	    amount -= a;
	    bytes_read += a;
	    cp += a;
	    }

	/* if we just emptied the buffer and more bytes remain, then fill it */
	if (amount > 0)
	{
	    /* if we need a partial buffer, then fill it */
	    if (amount < _bufsiz(fp))	// JV: < because whole buffer done directly
	    {
		// We may be attempting to fill the buffer at a mis-aligned location.
		// Attempt to align the buffer if that looks like it will help.
		// If we already read some data, assume that buffer is already aligned.
		// 
		// Buffer is empty, but take care if we are at EOF.
		// How is EOF detected ?
		//
		// After reading the data upto EOF, the file position may not be sector
		// aligned.  If the file position is NOT sector aligned, we will seek
		// back to the sector boundary before EOF, and attempt to _fill() the
		// buffer from there.
		//
		// This _fill() will (re-)read data in the last sector of the file,
		// and so will not detect EOF.  We will discard this data.
		//
		// EOF will then be recognised in the "if( ( amount > 0 && !feof(fp)) )"
		// block below.
	
	        if ( 
		    // If we have read data from the buffer, assume that buffer
		    // is already aligned.

		    // Note: bytes_read will be 0 if we have already hit EOF.
		    (bytes_read == 0) &&

		    // Sane, valid align size:
		    (BUFFER_ALIGN_SIZE > 0) && (BUFFER_ALIGN_SIZE <= _bufsiz(fp))
		    && ((_bufsiz(fp) & (BUFFER_ALIGN_SIZE-1)) == 0 )

		    // Avoid unbuffered or line-buffered modes, EOF, and errors:
		    && ! (_ioflag(fp) & (_IOLBF | _IONBF | _IOEOF | _IOERR))
                #if _NEWLINE_XLATE
		    // Don't try to be clever in text mode:
		    && ! (_fioflag(fp) & _FIOTEXT)
                #endif
		   )
		{
		    int fd = __fileno(fp);
		    long long cur_pos = SEEK64(fd, 0ll, SEEK_CUR);
		    int cur_align = (long)cur_pos & (BUFFER_ALIGN_SIZE-1l);
		    if(
			// Seek failed - possibly unseekable device:
			(cur_pos < 0)
			||
			// Current position is already aligned enough:
			cur_align == 0
		    )
		    {
			// Fall through to "normal" fill processing:
			;
		    }
		    else
		    {
			long long new_pos =  SEEK64(fd, (long long)(-cur_align), SEEK_CUR );

			// On some systems, it's possible we have attempted to seek
			// to the location of a "hole" in the file

			if( (new_pos >= 0) && ((new_pos + cur_align) == cur_pos) )
			{
			    // Seek seems to have worked !

			    if (_fill(fp) == 0)  /* if couldn't read from OS, return */
				goto DONE;

			    // Hopefully, have read aligned buffer.
			    // Now skip to misaligned start of requested read data:

			    size_t a = _min(_cnt(fp), cur_align );
			    _ptr(fp) += a;
			    _cnt(fp) -= a;

			    goto done_fill;
			}
		    }

		}
		
		if (_fill(fp) == 0)  /* if couldn't read from OS, return */
		    goto DONE;

	    done_fill:
		/* now grab bytes from buffer */
		if (_cnt(fp) > 0){
		    size_t a = _min(_cnt(fp),amount);
		    memcpy(cp,_ptr(fp),a);
		    _ptr(fp) += a;
		    _cnt(fp) -= a;
		    amount -= a;
		    bytes_read += a;
		    cp += a;
		    }
	    }

	    /* if we have more bytes remaining than can fit in a buffer */
	    /* then bypass the buffering, unless, of course, we're at   */
	    /* end of file */

	    if( ( amount > 0 && !feof(fp)) )
	    {
		int buffering_offset_from_start;
		long long end_pos;

		// any active buffer is no longer current.
		_ptr(fp) = _base(fp);
		_cnt(fp) = 0;

		// For fully bufferred reads of size _bufsiz(fp) or more,
		// where the end of the transfer is not aligned to BUFFER_ALIGN_SIZE,
		// we break the transfer into two parts, and extend it if necessary, so
		// that it ends on the first BUFFER_ALIGN_SIZE boundary at or above
		// the end of the requested data.
		//
		// The first part of this - possibly extended - transfer starts at the current
		// position in the file, and continues to the last BUFFER_ALIGN_SIZE boundary
		// which is not above the end of the requested data.
		// This part is read() directly into the caller's buffer.
		//
		// The second part of the extended transfer starts at the end of the first part,
		// and has size either zero or BUFFER_ALIGN_SIZE.  This part is copied into the
		// stdio read buffer.  The requested part is then memcpy()'ed into the users buffer,
		// possibly leaving unread bytes in the stdio buffer.
		//
		// The purpose of this is to avoid double read()s of single sectors from storage media
		// on successive "large" fread()s which do not start & end on sector boundaries.
		// "Large" roughly means bigger than _bufsiz(fp).
		// 
		// The exact condition for the first read is:
		//    amount >= ( _bufsiz(fp) + _cnt(fp) + (unget->_ptr - unget->_base) )
		//
		// These large reads bypassed stdio buffering, after any buffered data was copied to user
		// buffers.

		if ( 
		    // Sane, valid align size:
		    (BUFFER_ALIGN_SIZE > 0) && (BUFFER_ALIGN_SIZE <= _bufsiz(fp))
		    && ((_bufsiz(fp) & (BUFFER_ALIGN_SIZE-1)) == 0 )

		    // We are (still) trying to do a read at least as big
		    // as _bufsiz(fp) - we have not already read to EOF in _fill():
		    && (amount >= _bufsiz(fp))

		    // Avoid unbuffered or line-buffered modes, EOF, and errors:
		    && ! (_ioflag(fp) & (_IOLBF | _IONBF | _IOEOF | _IOERR))
		#if _NEWLINE_XLATE
		    // Don't try to be clever in text mode:
		    && ! (_fioflag(fp) & _FIOTEXT)
		#endif
		   )
		{
		    if( bytes_read > 0 )
		    {
			// We got some bytes out of the buffer - so assume that the end
			// of the data in the buffer is aligned, because we've already aligned it.

			// This guess saves a seek(), but if we have a random access device where
			// read()s routinely return less data than requested, we lose alignment.
			// Also, if extra unget()s have happened,  we lose alignment.

			buffering_offset_from_start = amount &~ (BUFFER_ALIGN_SIZE-1l);
		    }
		    else
		    {
			long long cur_pos = SEEK64( __fileno(fp), 0ll, SEEK_CUR);

			if( cur_pos >= 0 )
			{
			    // Seek succeeded:
			    end_pos = (cur_pos + amount) &~ (BUFFER_ALIGN_SIZE-1l);
			    buffering_offset_from_start = end_pos - cur_pos;
			}
			else
			{
			    // Seek failed - possibly unseekable device:
			    buffering_offset_from_start = amount;
			}
		    }
		}
		else
		    buffering_offset_from_start = amount;

		buffering_offset_from_start += bytes_read;

		do // while ( amount > 0 && !feof(fp));
		{
		    // note that READ() may return a number less than amount
		    // and that's because the carriage return('\r')/newline('\n')
		    // pair is counted as one char in the read operation for
		    // files opened in text mode on DOS.
		    // so after each READ(), the cnt must be check.  if cnt
		    // is less than amount, then this while loop must be
		    // executed again to read the total amount of data specified
		    // by the caller.

		    // Read() can always return fewer bytes than requested,
		    // and that does /not/ indicate an error.

		    int cnt;
		    if( bytes_read < buffering_offset_from_start )
		    {
			// Reading up to possible early break - or we're not
			// trying to be clever with buffer alignment:
			cnt = READ(__fileno(fp),cp,buffering_offset_from_start - bytes_read);
		    }
		    else if( bytes_read > buffering_offset_from_start )
		    {
			// We have read past an aligned boundary,
			// have not hit error or EOF, 
			cnt = READ(__fileno(fp),cp,amount);
		    }
		    else // bytes_read == buffering_offset_from_start
		    {
			// We have arrived at a sector-aligned boundary -
			// switch to stdio buffer, so we cache the whole
			// final sector.

			#if _SYSTEM_NEWLINE_XLATE
			    /* record where buffer begins so next ftell will work */
			    _bufpos(fp) = SEEK(__fileno(fp), 0, SEEK_CUR);
			#endif
			
			cnt =  READ(__fileno(fp),_base(fp),BUFFER_ALIGN_SIZE);

			if( cnt > 0 )
			{
			    size_t a = _min(cnt,amount);
			    memcpy(cp,_ptr(fp),a);
			    _ptr(fp) = _base(fp) + a;
			    _cnt(fp) = cnt - a;

			    // cp incremented below.

		            cnt = a;  // Read() may not have supplied all the data requested.
			}
		    }

		    // Let EOF be caught on next read
		    if (cnt <= 0){
#ifndef __PENPOINT__
			if (cnt == 0 || errno != EINTR)
#endif
			    {
			    if (cnt < 0)
				_ioflag(fp) |= _IOERR;
			    _ioflag(fp) |= _IOEOF;
			    }
			cnt = 0;
			amount = 0;
			}
		    else {
			bytes_read += cnt;
			amount -= cnt;
			cp += cnt;
			}
		}
		while ( amount > 0 && !feof(fp));
	    }
	}
    }
DONE:
    ret = elem==0 ? 0 : bytes_read/elem;
RETURN:
    UNLOCK_FILE(fp)
    return ret;
}
