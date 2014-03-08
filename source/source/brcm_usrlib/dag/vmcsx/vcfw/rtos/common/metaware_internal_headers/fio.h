/*********************************************************************
(C) Copyright 2002      ARC International (ARC);  Santa Cruz, CA 95060
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

#include <stdio.h>
#c_include <stdarg.h>	/* for va_list */
#include "libhdr.h"
#include "thread.h"

#ifdef __CPLUSPLUS__
extern "C" {
#endif

#ifndef _UNICHAR_DEFINED
    typedef unsigned short _unichar;
    #define _UNICHAR _unichar
    #define _UNICHAR_DEFINED
#endif

#if !_MSDOS && !_MSNT && !_OS2 && !_NTDOS
    #define _O_RDONLY	O_RDONLY
    #define _O_WRONLY	O_WRONLY
    #define _O_RDWR	O_RDWR

    #define _O_TRUNC	O_TRUNC
    #define _O_APPEND	O_APPEND
    #define _O_CREAT	O_CREAT
#endif

#if _AM29K
    #define _O_FORM	O_FORM
#endif


/* these macros expect a file pointer as an argument */
#define _cnt(fp)	(fp)->_cnt
#define _ptr(fp)	(fp)->_ptr
#define _base(fp)	(fp)->_base
#define _ioflag(fp)	(fp)->_flag
/* convert file pointer to file descriptor */
#define _fd(fp)		(fp)->_file
#if _UPA
    /* _UPA's fp->_file field has bytes stored in reverse order --
     *    use this macro to reverse it if it is _UPA.
     */
    #define _rfd(fd) (((((unsigned short)(fd))>>8)| \
			(((unsigned short)(fd))<<8)) & 0xffff)
#else	/* other */
    #define _rfd(fd) (fd)
#endif

/*
 * _iob_fdattr is an arrary of array of file attributes for each
 * opened file with a file handle; defined in _iob.c.
 * it is added to support _fstat to acquire access mode of a file.
 * dos does not provide a system call that returns file attributes
 * when given a file handle.  the file attributes have to be saved
 * by _open function so that later _fstat can get to these file
 * attributes via file handles.
 */
#if _MSDOS || _NTDOS
    typedef unsigned	_Fdattr;
    extern  _Fdattr	_iob_fdattr[];
    #define _fd_fileattr(fd)	_iob_fdattr[fd]
#endif


/*
 * the auxiliary file information is stored three ways:
 *   _AUX_INDEX_BY_FD
 *     index auxiliary tables by file descsriptor
 *   _AUX_INDEX_BY_FP
 *     index auxiliary tables by file pointer
 *   _AUX_NON_INDEXABLE
 *     the auxiliary info is not in separate tables. it is part of the
 *     FILE entry
 *
 *   the first two indexing methods are required due to the following
 *   circumstances:
 *     - under DOS, the low-level I/O (_open, _read, _write),
 *       the auxiliary tables must be updated using the file
 *       desciptor as an index.  (i.e., we need to know if
 *       we're processing TEXT or BINARY).
 *     - under HOBBIT, file descriptors may be arbitrarily
 *       large, and thus may not be used as indices.
 *     - under _SOL, we address IOB entries in two different
 *       tables, making INDEX_BY_FD easier.
 *
 *   the last method is typically used when there are no compatibility
 *   issues with the FILE type, or when the _IOB is implemented as a
 *   linked list (_IOB_FORMAT == _IOB_LIST).
 *   
 *   note that converting a file pointer (fp) to a file
 *   descriptor (fd) is simple (see _fd, below).  However
 *   the inverse operaton may not easily be performed, as file
 *   handles may be reassigned (see FREOPEN).
 */
#define _AUX_INDEX_BY_FD	1
#define _AUX_INDEX_BY_FP	2
#define _AUX_NON_INDEXABLE	3

#if _MSDOS || _MSNT || _OS2 || _SOL || _NTDOS
    #define _AUX_FORMAT     _AUX_INDEX_BY_FD
#elif (_IOB_FORMAT == _IOB_LIST)
    #define _AUX_FORMAT	    _AUX_NON_INDEXABLE
#else
    #define _AUX_FORMAT     _AUX_INDEX_BY_FP
#endif

#if (_AUX_FORMAT == _AUX_INDEX_BY_FD)

    /* these macros expect a file pointer as an argument */
    #define _fioflag(fp)  	_iob_fioflag[__fileno(fp)]
    #ifndef UNLINK 
	#define _tmp(fp)	_iob_tmpnam[__fileno(fp)]
    #endif
    #define _unget(fp)		_iob_unget[__fileno(fp)]
    #define _bufpos(fp)		_iob_bufpos[__fileno(fp)]

    /* these macros expect a file descriptor as an argument */
    #define _fioflag_fd(fd)	_iob_fioflag[fd]
    #ifndef UNLINK 
	#define _tmp_fd(fd)	_iob_tmpnam[fd]
    #endif
    #define _unget_fd(fd)	_iob_unget[fd]
    #define _bufpos_fd(fd)	_iob_bufpos[fd]

#elif (_AUX_FORMAT == _AUX_INDEX_BY_FP)

    /* these macros expect a file pointer as an argument */
    #define _fioflag(fp)  	_iob_fioflag[fp - _IOB]
    #ifndef UNLINK 
	#define _tmp(fp)	_iob_tmpnam[fp - _IOB]
    #endif
    #define _unget(fp)		_iob_unget[fp - _IOB]
    #define _bufpos(fp)		_iob_bufpos[fp - _IOB]

#elif (_AUX_FORMAT == _AUX_NON_INDEXABLE)

    /* these macros expect a file pointer as an argument */
    #define _fioflag(fp)  	(fp)->_iob_fioflag
    #ifndef UNLINK 
	#define _tmp(fp)	(fp)->_iob_tmpnam
    #endif
    #define _unget(fp)		(fp)->_iob_unget
    #define _bufpos(fp)		(fp)->_iob_bufpos

#endif


/*
 *    _isinit     returns TRUE if the file is open and has been
 *                initialized.  This may be detected by examining
 *                IONBF (no buffer) or _base (buffer pointer).
 *                If a file has been opened and initialized, one
 *                of these must be TRUE.
 *    _isopen     returns TRUE if the file is open.  If the file
 *                flags are not zero, the file is open.
 *                
 */
#define _isinit(fp)	((_ioflag(fp) & _IONBF) || _base(fp) != NULL)

#if 0
#define _isopen(fp)	(_fd(fp) || fp == _IOB)
#endif

#define _isopen(fp)	(_ioflag(fp))




/* 
 *  rules of the road:
 *    if (_IOB_BUFTYP == _IOB_BUFSIZ)
 *        legal sources:  	_bufsiz
 *        legal destinations:  	_bufsiz
 *    elif _IOB_BUFTYP == _IOB_BUFSIZ_ENDP
 *        legal sources:  	_bufsiz _bufend
 *        legal destinations:  	_bufend _bufend
 *    else
 *        legal sources:  	_bufsiz _bufend
 *        legal destinations:  	_bufend
 *    fi
 */
#if (_IOB_BUFTYP == _IOB_BUFSIZ)
    #define _bufsiz(fp)	(fp)->_bufsiz
#elif (_IOB_BUFTYP == _IOB_BUFENDP)
    #define _bufend(fp)	(fp)->_bufendp
    #define _bufsiz(fp)	(_bufend(fp) - _base(fp))
#elif (_IOB_BUFTYP == _IOB_BUFENDTAB)
    #if (_IOB_FORMAT == _IOB_EXTENSION)
	#define _bufend(fp) *(__fileno(fp) < _FOPEN_MAX1 ? \
	   &_bufendtab[__fileno(fp)]:&_bufendtab_more[__fileno(fp)-_FOPEN_MAX1])
    #elif _UPA
	#define _bufend(fp)	__bufendtab[__fileno(fp)]
    #else
	#if (_AUX_FORMAT == _AUX_INDEX_BY_FD)
	    #define _bufend(fp)	_bufendtab[__fileno(fp)]
	#elif (_AUX_FORMAT == _AUX_INDEX_BY_FP)
	    #define _bufend(fp)	_bufendtab[fp - _IOB]
	#else
	    #error Error in compilation: _bufendtab not configured in fio.h
	#endif
    #endif
    #define _bufsiz(fp)	(_bufend(fp) - _base(fp))
#elif (_IOB_BUFTYP == _IOB_BUFSIZ_ENDP)
    #define _bufsiz(fp)	(fp)->_bufsiz
    #define _bufend(fp)	(fp)->_bufendp
#endif

/* system dependent support routine */
#if _AM29K
    extern  char 	*_tmpnam(char *);
#endif

#if _TEXTFILES
    /* file mode, set by _setmode */
    extern int _fmode;
#endif

#if _MSDOS || _MSNT || _OS2 || _NTDOS
    /* protection mode, set by _umask */
    extern int _pmode;

    extern int __open   (const char *name, int oflags, int permits);
    extern int __uopen  (const _unichar *name, int oflags, int permits);
    extern int __close  (int fd);
    extern int __write  (int fd, const void *buf, unsigned int len);
    extern int __read   (int fd, void *buf, unsigned int len);
    extern int __dup    (int fd);
    extern int __dup2   (int fd1, int fd2);
    extern long __lseek (int fd, long pos, int whence);
#endif

/* fopen.c */
		/*use_fd is used as a file handle if "name" is 0 */
extern FILE *_dofopen(const char *name, const char *mode, FILE *fp, int use_fd,
	int share_flags);

/* generic support routines */
extern int      _finit(FILE *f);
extern int      _fill(FILE *f);
extern int	_ifflush(FILE *fp, int internal);
extern int	_ifclose(FILE *fp);
extern int	_fclose_exit(void);
extern int      (*_fclose_exit_p)(void);  // to avoid linking stdio from exit()
extern void	_freefp(FILE *fp);
extern int	_fd_shrd(int fd);
extern void     _stdio_linked(void);

/* print/scan support  (std) */
extern  int             _doprint(int (*fw)(), void *prm, 
                                const char *f, va_list parms);
extern  int             _doscan(int (*fw)(), void *prm, 
                                const char *f, va_list parms);

extern  int             _doprintnf(int (*fw)(), void *prm,
                                const char *f, va_list parms);
extern  int             _doscannf(int (*fw)(), void *prm,
                                const char *f, va_list parms);

/* Unicode fopen/printf/scanf support */

extern FILE *	_udofopen(const _unichar *name, const _unichar *mode, FILE *fp,
			int use_fd, int share_flags);
extern int	_udoprint(int (*fw)(), void *prm, 
                	const _unichar *f, va_list parms);
extern int      _udoscan(int (*fw)(), void *prm, 
                        const _unichar *f, va_list parms);


/* Semaphores for multi-thread environments (OS/2 and NT)
 */
#if _THREAD_MODEL == _THREAD_MODEL_LOCKS
    extern int  _mwinit_file_lock(FILE *stream);
    extern int  _mwdelete_file_lock(FILE *stream);
    extern void _mwlock_file(FILE *);
    extern void _mwunlock_file(FILE *);
    extern void _mwlock_handle(int);
    extern void _mwunlock_handle(int);
    
    #define INIT_FILE_LOCK(stream) _mwinit_file_lock(stream);
    #define DELETE_FILE_LOCK(stream) _mwdelete_file_lock(stream);
    #define LOCK_FILE(stream)	_mwlock_file(stream);
    #define UNLOCK_FILE(stream) _mwunlock_file(stream);
    #define LOCK_HANDLE(handle)	_mwlock_handle(handle);
    #define UNLOCK_HANDLE(handle) _mwunlock_handle(handle);
#else
    #define INIT_FILE_LOCK(stream)
    #define DELETE_FILE_LOCK(stream)
    #define LOCK_FILE(stream)
    #define UNLOCK_FILE(stream)
    #define LOCK_HANDLE(handle)
    #define UNLOCK_HANDLE(handle)
#endif

/* Function to convert an HPFS/Unix filename to a FAT 8.3 filename
 * and call a function to operate on the converted filename.
 */
void _mwfat_name(const char *iname, void doit(const char *)!);

#ifdef __CPLUSPLUS__
}
#endif
