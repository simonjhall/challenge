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

/* libhdr  -- system calls and other implementation specifics */

#ifdef __CPLUSPLUS__
extern "C" {
#endif

#include <sizet.h>

#if _I386 || _PPC || _ARM || _PJC || _UPA || _SOL || _RS6000 || _MIPS || _VIDEOCORE || _ARC
    #define _HAS_LONG_LONG 1
#endif


/* ==>  SEE _na.h for _HAS_WEAK_SYMBOLS */


#if _MSNT || _OS2 || _HOBBIT || _PPC || _ARM || _ARC || _PJC || _RS6000 || _MIPS || _VIDEOCORE
    /* e.g. see std_g/_doprint.c */
    #define _HAS_UNICODE 1
#endif

/* if your compiler and runtime supports thread-local data, indicate this here
   to use __declspec(thread) for thread-local C library data rather than the
   static_t aggregate method.
*/
#if _MSDOS || _ARC || _PPC || _ARM || _MIPS || _VIDEOCORE
    #define _HAS_DECLSPEC_THREAD 1
#endif

#if defined(_REENTRANT) && defined(_HAS_DECLSPEC_THREAD)
    #define _THREAD __declspec(thread)
#else
    #define _THREAD
#endif

#ifndef _UNICHAR_DEFINED
    typedef unsigned short _unichar;
    #define _UNICHAR _unichar
    #define _UNICHAR_DEFINED
#endif

#ifndef _CRTIMP
    #if _MSNT && _DLL
	#define _CRTIMP __declspec(dllimport)
    #else
	#define _CRTIMP
    #endif
#endif

#ifndef _ARMDECL
    /* Obsolete: only used for VCP use of ARM-compiled data */
    #if _VCP
	#define _ARMDECL __declspec(arm)
    #else
	#define _ARMDECL
    #endif
#endif

#ifdef NOCONST
    #ifndef const
	#define const
    #endif
#endif


/* This macro puts out a "directive" section that causes the linker to
   issue the warning passed as string "msg".
   A "pragma link_warn(char *msg)" would be a better solution.
*/

#if _ARM || __DASHG__
// Use of .previous below conflicts with debug info's use of .previous
#define LINK_WARN(msg)
#elif _ARC
#define LINK_WARN(msg) \
    static void zwarn(void) { \
	static void (*dummyref)(void) = zwarn; \
	_ASM(".pushsect .wafter, directive"); \
	_ASM(".string \"\\\"-zwarn=" msg "\\\"\""); \
	_ASM(".popsect");\
	}
#else
#define LINK_WARN(msg) \
    static void zwarn(void) { \
	static void (*dummyref)(void) = zwarn; \
	_ASM(".section .wafter, directive"); \
	_ASM(".string \"\\\"-zwarn=" msg "\\\"\""); \
	_ASM(".previous");\
	}
#endif

/*
 * Macros to provide low level IO access to library.
 *
 *   for AM29K, _read and _write just happen to be the names of
 *   the read/write routines, and are not the _read.c/_write.c
 *   routines that do CR-LF adjustments for MSDOS & I860.
 *
 *   the X_xxx routines are only called from within 
 *   the _xxx routines, and should not be referenced elsewhere.
 *     
 */
#if _MSDOS || _MSNT || _OS2 || _NTDOS
    #define OPEN(A,B,C)		_open(A,B,C)
    #define X_OPEN(A,B,C)	__open(A,B,C)
    #define CLOSE(A)		_close(A)
    #define X_CLOSE(A)		__close(A)
    #define READ(A,B,C)		_read(A,(void *)(B),C)
    #define X_READ(A,B,C)	__read(A,(void *)(B),C)
    #define WRITE(A,B,C)	_write(A,(void *)(B),C)
    #define X_WRITE(A,B,C)	__write(A,(void *)(B),C)
    #define SEEK(A,B,C)		_lseek(A,B,C)
    #define X_SEEK(A,B,C)	__lseek(A,B,C)
    #define FLUSH(A)            __flush(A)
    /* do not define UNLINK for dos, as it deletes file if OPEN */
#elif _I860 && !_NOXLAT && !_ATT
  /* We don't want this stuff for CSPI and Mercury COFF based i860 big endian */
    /* _open, _read and _write support DOS-style textfiles */
    #define OPEN(A,B,C)		_open(A,B,C)
    #define X_OPEN(A,B,C)	open(A,B,C)
    #define CLOSE(A)		close(A)
    #define X_CLOSE(A)		close(A)
    #define READ(A,B,C)		_read(A,(void *)(B),C)
    #define X_READ(A,B,C)	read(A,(void *)(B),C)
    #define WRITE(A,B,C)	_write(A,(void *)(B),C)
    #define X_WRITE(A,B,C)	write(A,(void *)(B),C)
    #define SEEK(A,B,C)		lseek(A,B,C)
    #define X_SEEK(A,B,C)	lseek(A,B,C)
    #define UNLINK(A)		unlink(A)
#elif _AM29K || _ARC || _ARM || _PJC || _PPC || _MIPS || _VIDEOCORE
    #define OPEN(A,B,C)		_open(A,B,C)
    #define X_OPEN(A,B,C)	_open(A,B,C)
    #define CLOSE(A)		_close(A)
    #define X_CLOSE(A)		_close(A)
    #define READ(A,B,C)		_read(A,(void *)(B),C)
    #define X_READ(A,B,C)	_read(A,(void *)(B),C)
    #define WRITE(A,B,C)	_write(A,(void *)(B),C)
    #define X_WRITE(A,B,C)	_write(A,(void *)(B),C)
    #define SEEK(A,B,C)		_lseek(A,B,C)
    #define SEEK64(A,B,C)	_lseek64(A,B,C)
    #define X_SEEK(A,B,C)	_lseek(A,B,C)
	/* don't define UNLINK for embedded targets.  UNLINK is used
	   to remove tmpfiles by opening the file and then immediately
	   "unlink()"-ing.  This is a unix-ism.  For embedded targets that
	   may or may not be hosted on unix, activate the _iob_tmpnam
	   auxiliary table for keeping track of temp file names in
	   a separate table, and explicitly removing the files after
	   fclosing.  This method takes more energy, but will work everywhere.
	*/
#else
   /* Use these defaults for unix-like system */
    #define OPEN(A,B,C)		open(A,B,C)
    #define X_OPEN(A,B,C)	open(A,B,C)
    #define CLOSE(A)		close(A)
    #define X_CLOSE(A)		close(A)
    #define READ(A,B,C)		read(A,(void *)(B),C)
    #define X_READ(A,B,C)	read(A,(void *)(B),C)
    #define WRITE(A,B,C)	write(A,(void *)(B),C)
    #define X_WRITE(A,B,C)	write(A,(void *)(B),C)
    #define SEEK(A,B,C)		lseek(A,B,C)
    #define X_SEEK(A,B,C)	lseek(A,B,C)
    #define UNLINK(A)		unlink(A)
#endif


#if _MSDOS || _MSNT || _OS2  || _NTDOS
    extern int _open(const char *, int ,...);
    extern int _close(int);
    extern int _read(int, char *,unsigned int);
    extern int _write(int, const void *,unsigned int);
    extern long _lseek(int, long,int );
    extern int __flush(int);
#elif _I860
    #if  !_SUN || !defined __sys_fcntlcom_h
	extern int open(char*,int,...);
    #endif
    extern int _open(char*,int,...);
    extern int close(int);
    extern int _read(int, char*, unsigned int);
    extern int read(int, char*, unsigned int);
    extern int _write(int, const void *, unsigned int);
    extern int write(int, void *, unsigned int);
    extern long	lseek(int, long,int);
    extern int unlink(const char*);
#elif _HOBBIT || _BEOS
    extern int open(const char*,int,...);
    extern int close(int);
    extern int read(int, char*, unsigned int);
    extern int write(int, const void *, unsigned int);
    extern long	lseek(int, long,int);
    extern int unlink(const char*);
#elif _AM29K
    extern int _open(char *,int,...);
    extern int _close(int);
    extern int _read(int, char*, int);
    extern int _write(int, void*, int);
    extern long	_lseek(int, long, int);
#elif _ARC || _PJC || _PPC || _MIPS || _VIDEOCORE
    extern int _open(const char *,int,...);
    extern int _close(int);
    extern int _read(int, char*, unsigned int);
    extern int _write(int, const char*, unsigned int);
    extern long	_lseek(int, long, int);
    #if _HAS_LONG_LONG
	extern long long	_lseek64(int, long long, int);
    #endif
#elif _ARM || _ARMVCP
    extern int _open(const char*, int,...);
    extern int _close(int);
    extern int _read(int, char*, unsigned);
    extern int _write(int, const char*, unsigned);
    extern long _lseek(int, long, int); 
    extern int _unlink(const char*);
    extern int _link(const char*, const char*);
#else
    // Don't define "open()" if SunOS fcntlcom.h has been included.
    // It already defines "open" in an incompatible way.
    //
    #if _LINUX || _SOL
	extern int open(const char*,int,...);
    #elif !_SUN && !_ISIS || !defined __sys_fcntlcom_h
	extern int open(char*,int,...);
    #endif
    extern int close(int);
    #if _SOL
    extern int read(int, char*, unsigned);
    extern int write(int, const char *, unsigned);
    #else
    extern int read(int, char*, int);
    extern int write(int, char *, int);
    #endif
    extern long	lseek(int, long,int);
    extern int unlink(const char*);
#endif

/* Unicode system-level functions.  */
extern int 	 _uopen(const _unichar *, int, ...);
extern int 	 _ucreat(const _unichar *, int);

/*
 * Macro to provide low level return to system
 */
#define EXIT(rc)	_exit(rc)
extern void	_exit(int);

/*
 * Macros for isatty() function.  This function is optional, and
 * affects whether line buffering or full buffering is made the default
 * when a file is opened.
 */

#if _MSDOS || _OS2 || _MSNT || _AM29K || _NTDOS || _ARM || _ARC || _PJC || _PPC || _MIPS || _VIDEOCORE
    #define ISATTY(A)		_isatty(A)
#elif _SUN || _ATT4 || (_ATT && !_I860) || _UPA || _SOL
    /* CSPI i860 host link stuff does not have isatty */
    #define ISATTY(A)		isatty(A)
#endif
#ifdef ISATTY
    extern int ISATTY(int);
#endif


/*
 * Macros for strerror() error message strings
 */
#if _ARC || _ARM || _PPC || _PJC || _AM29K || _MSDOS || _MSNT || _OS2 ||_NTDOS || _MIPS || _VIDEOCORE
    #define SYS_NERR		_sys_nerr
    #define SYS_ERRLIST(n)  	_sys_errlist[(n)]
    _CRTIMP _ARMDECL extern char *_sys_errlist[];
    _CRTIMP _ARMDECL extern int _sys_nerr;
#elif __PENPOINT__
    char * _PenPointError(int __errnum);
    #define SYS_NERR		0x7fffffff
    #define SYS_ERRLIST(n)	_PenPointError((n)|0x80000000)
#else
    #define SYS_NERR		sys_nerr
    #define SYS_ERRLIST(n)  	sys_errlist[(n)]
    #if !_LINUX
	extern int sys_nerr;
	extern char *sys_errlist[];
    #endif
#endif


/*
 * Memory Allocation System call definitions.  These are used by malloc().
 * SYSFREE is only used to return the most recent result from SYSALLOC.
 * Also, SYSFREE is never used if SYSALLOC always returns memory in a
 * contiguous sequence.
 */
#if _AM29K
#   define SYSALLOC(size) _sysalloc(size)
#   define SYSFREE(p,size) _sysfree(p,size)
    extern void * SYSALLOC(unsigned size);
#elif _MSDOS || _NTDOS
#   define SYSALLOC(size) _mwexpand_heap(size)
#   define SYSFREE(p,size) _mwcontract_heap(p)
    extern void * SYSALLOC(int size);
#elif _MSNT || _OS2
#   define SYSALLOC(size) _mwexpand_heap(size)
#   define SYSFREE(p,size) _mwcontract_heap(p, size)
    extern void * SYSALLOC(unsigned size);
#elif _ARM || _ARC || _PJC || _PPC || _MIPS || _VIDEOCORE
#   define SYSALLOC(size) _sbrk(size)
#   define SYSFREE(p,size) _brk(p)
    extern void *SYSALLOC(int size);
#else
#   define SYSALLOC(size) sbrk(size)
#   define SYSFREE(p,size) brk(p)
    extern void *SYSALLOC(int size);
#endif
extern void *SYSFREE(void *p, unsigned size);

/*
 * getenv() support.  Define GETENV if you want malloc() to respect the
 * MALLOC_LEVEL environment variable.  GETENV will be called in lazy fashion
 * upon the first call to malloc().
 *
 * getenv is also used by fio_g/_tempnam.c, if GETENV is defined, and by
 * misc_g/_searche.c.  Also misc_g/iprofile.c.
 *
 * Implementing _mwgetenv2 is preferred, as it uses a separate buffer from
 * getenv and thus satisfies ansi behavior that the lib should not call
 * getenv internally and trash the buffer.
 */
#if _MSDOS || _NTDOS || _MSNT || _OS2 || _ARC || _PJC 
#   define GETENV(s) _mwgetenv2(s)
#else
#   define GETENV(s) getenv(s)
#endif
#ifdef GETENV
    extern char *GETENV(const char *s);
#endif

/*
 * _putenv() needs to know the length of _environ[].
 */
#ifndef _ENVIRON_LEN
#define _ENVIRON_LEN	32 
#endif

/*
 * access() function.  This is used for temporary filename generation.
 * See fio_g/_mktemp.c.  Also used by misc_g/_searchs.c for _searchstr(),
 * which is optional in the library.  Currently the library only requires
 * the use of value 0 (F_OK from unistd.h) for the second argument, which
 * indicates if the file exists or not.
 */
#if _MSDOS || _OS2 || _MSNT || _NTDOS || _HOBBIT || _PPC || _PJC || _ARM || _ARC || _MIPS || _VIDEOCORE
    #define ACCESS(A,B) _access(A,B)
#else
    #define ACCESS(A,B) access(A,B)
#endif
extern int   ACCESS(const char *,int);
extern int _uaccess(const _unichar *, int);

/*
 * getpid() function.  This is used for temporary filename generation.
 * See fio_g/_mktemp.c.  If GETPID is not defined, the implementation will
 * substitute a dummy version.
 */
#if _MSDOS || _OS2 || _MSNT || _NTDOS || _HOBBIT || _PPC || _PJC || _ARM || _ARC || _MIPS || _VIDEOCORE
    #define GETPID _getpid
#else
    #define GETPID getpid
#endif
#ifdef GETPID
    #if _SOL || _ARC
    extern long  GETPID(void);
    #else
    extern int   GETPID(void);
    #endif
#endif

/*
 * getcwd() function.  This is used by the _searchs() function.
 * See misc_g/_searchs.c.  If GETCWD is not defined, the implementation
 * will substitute a dummy version.
 */
#if _MSDOS || _OS2 || _MSNT || _NTDOS || _HOBBIT || _PPC || _PJC || _ARM || _ARC || _MIPS || _VIDEOCORE
    #define GETCWD(A,B) _getcwd(A,B)
    extern char *GETCWD(char *, int);
#elif _SOL || _ATT4
    #define GETCWD(A,B) getcwd(A,B)
    extern char *GETCWD(char *, size_t);
#else
    #define GETCWD(A,B) getcwd(A,B)
    extern char *GETCWD(char *, int);
#endif

/*
 * If the TZ environment variable is not set, but it can be constructed
 * from available OS resources, implement this routine.  The return value
 * is a pointer to a valid string for the TZ environment variable.
 */
#if _MSNT
    #define CONSTRUCT_TZ _mwconstruct_tz
    extern char *_mwconstruct_tz(void);
#endif


/*
 * ===>>> System interfaces underlying rename(), remove(), time(), and
 * ===>>> clock() are frequently more esoteric and are handled in the
 *        individual source files.
 */


/*
 *- - - - - - - - - - - - - - - - - - - - - - - - -
 * The rest of this file can probably be used as-is
 *- - - - - - - - - - - - - - - - - - - - - - - - -
 */

/*
 * Macros to provide lowest level machine-dependent allocation.
 * Only used for "old" malloc implementation and can typically be ignored.
 */
#if _AM29K
    #define MEMALLOC(A)		_sysalloc(A)
#else
    #define MEMALLOC(A)		_alloc(A)
    extern void *_alloc(int x);
#endif
#define MEMFREE(A,B)


/*
 * Unicode macros generated by _W(macroname), where _W is defined
 * in str.h, and macroname is one of the macros in the above list.
 */
#define _uOPEN(A,B,C)		_uopen(A,B,C)
#define _uX_OPEN(A,B,C)		__uopen(A,B,C)
#define _uACCESS(A,B)		_uaccess(A,B)

#if _MSDOS || _MSNT || _OS2 || _NTDOS
    #define FTRUNCATE_IS_CHSIZE
    #define FTRUNCATE(handle, newsize) _chsize(handle, newsize)
#elif _ATT && _I386
    #define FTRUNCATE(handle, newsize) chsize(handle, newsize)
#else
    #define FTRUNCATE(handle, newsize) ftruncate(handle, newsize)
#endif
extern FTRUNCATE (int handle, long newsize);

/* TRUNCATE_HANDLE is used by _chsize to truncate a file at the current
 * file position.  It is used only to shorten, never lengthen, a file.
 */
#if _MSDOS
    #define TRUNCATE_HANDLE(handle) WRITE(handle,&handle,0)
#elif !__OS_OPEN
    #define TRUNCATE_HANDLE(handle) _file_truncate(handle)
    extern int _file_truncate(int);
#endif

/* RETURN sets a return code, then to jump to a procedure exit label.
 * This is useful in multi-thread environments, where a lock may
 * need to be released before returning.
 */
#define RETURN(val) { ret = val; goto RETURN; }

#ifdef __CPLUSPLUS__
}
#endif
