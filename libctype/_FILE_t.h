/*
 *  _FILE_t.h
 *  Pheonix
 *
 *  Created by Steven Abner on Tue 6 Aug 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 *  Dedicated to Erin JoLynn Abner.
 *  Special thanks to God!
 *
 *   Currently: except for maybe LBF, see no need for locks. Only shared
 *  data is <fildes>. On LBF, it is an assumption that the 'write' portion
 *  of the buffer is shared, however the read of LBF is private. Could be
 *  issue of 'fflush' or 'write' filling buffer during a read operation.
 *  Possible solution: copy state pre-operation on LBF. This would almost be
 *  a near identical non-buffered system.
 *   If on FBF, it was a concern of buffered data not instantenously being up to
 *  date with <fildes>, shouldn't be buffering. Could solve by after a read of
 *  buffered data, invalidate buffered data, reread <fildes> before reading
 *  buffered. Again this means, going to a non-buffered system.
 *   If concerned <fildes> sharing and one needs 'private' writes to for reading,
 *  A system could be invented of write/read to a dup file, then replace <fildes>.
 *  However concider LBF, private <fildes> defeats the use of LBF.
 *
 *  Found out __CYGWIN__ switched to __APPLE__ like FILE 2019/08/14
 *  Design of LBF was to expand input buffer. Believe now that is incorrect
 *    approach, should return error instead (EFBIG).
 *
 *  Definitions: (re-review, POSIX states diferent buffering)
 *
 *  stdin - fileno 0 - a file opened as 'r', sorta
 *    Used as input from 'keyboard', read by processes.
 *    This means only system can write, all can read.
 *    May not be used for process communication.
 *    Buffered input (write from system), with file writing 'fflush' occurring
 *    on a <newline> character being read. Semi-buffered output (read), with
 *    read occuring from beginning of non-terminated, <newline> terminator, line
 *    being buffered.
 *  stdout - fileno 1 - a file opened as 'w'
 *    Used as output to users, 'console', for viewing.
 *    Non-buffered output to 'file'. Writes (input) directly to 'console'.
 *  stderr - fileno 2 - a file opened as 'a+'
 *    Used as output for diagnostic information. I have ideas for as possible
 *    replacement for errno, but... . buffered input (writes to) and buffered
 *    output (reads from). This is to comply with POSIX. Originally was a
 *    write to file.
 *  others - a file opened by user specifation.
 *    open as 'r' buffered reading
 *    open as 'w' buffered writing
 *    open as 'a' buffered writing with all to file writes at end of file
 *    opened with '+' buffered read/write with 'a' still writing to end of file
 *    can user by alter of flags change LBF, NBF, FBF ?
 *  memory - a file opened by user specifation
 *    direct read/write operation, FILE * used to keep accounting, with user
 *    expecting 'file' operations with no expections of physical disk storage
 */

/*      malloc buffered system (32 + BUFSIZ) or (32 + user supplied)
      ____________
  ^  |____________| <---- rdend/wrend   (malloc(d) + 32 + BUFSIZ - 1)
  |  |____________|
  |  |____________|
read/|____________| <---- base malloc(d) or malloc end if buffer supplied
write|____________|
  |  |____________| } enuf for 8 wchar_t / 32 char
  V  |____________|
unget|____________| <---- start unget  (struct + UNGET_MAX + 15) & ~15 - UNGET_MAX
     |____________| <---- end of fPtr struct
     |____________|
     |____________| <---- malloc fPtr struct

Note: unget area varies depending on use, but will have at least 32 char buffer

_p => where to write
_r => readable amount, rdPtr = (_p - _w) - _r + _u
_w => unwritten data, to file ptr = _p - _w

            PHEONIX                         APPLE
  fcntl      stdio   octal     stat
            _IOFBF  0000000
 // modes
O_RDONLY    _IORD   0000000
O_WRONLY    _IOWR   0000001
O_RDWR      _IORW   0000002
O_EXEC              0000004
O_SEARCH            0000010
                    0000020
                    0000040
  // flags
O_APPEND    _IOAND  0000100
O_NONBLOCK  _IONBF  0000200
O_RSYNC     _IOLBF  0000400
O_SYNC      _IOMEM  0001000
O_DSYNC             0002000
O_CREAT     _IOCRT  0004000
O_TRUNC     _IOTRC  0010000  S_IFIFO
O_EXCL      _IOUTF  0020000  S_IFCHR
O_NOCTTY    _IOEOF  0040000  S_IFDIR
                    0060000  S_IFBLK
O_CLOEXEC   _IOERR  0100000  S_IFREG    
                    0120000  S_IFLNK
                    0140000  S_IFSOCK
                    0170000  S_IFMT
O_DIRECTORY _IOSHM  0200000  S_IARC2    not allowed on linux stdio > 0xFFFF
O_NOFOLLOW  _IOOWN  0400000  S_IARC1
O_TTY_INIT         01000000
*/

/*    fcntl flags         stdio flags   APPLE fcntl   APPLE stdio  linux fcntl
 * O_RDONLY    0000000      0000000       0000000       0x0004       0000000
 * O_WRONLY    0000001      0000001       0000001       0x0008       0000001
 * O_RDWR      0000002      0000002       0000002       0x0010       0000002
 * O_APPEND    0000100      0000100       0000010       0x0100       0000010
 * O_CREAT     0004000      0004000       0001000         NA         0001000
 * O_TRUNC     0010000      0010000       0002000         NA         0002000
 * _IONBF                   0000200                     0000002
 * _IOLBF                   0000400                     0000001
 * _IOFBF   never test (test NBF/LBF) else its FBF
 */

#ifdef __APPLE__
 #define _IO_ALTERED_RW 000004
 #define _IO_APPEND    0000010
 #define _IO_CREATE    0001000
 #define _IO_TRUNCATE  0002000
 #define _IO_NOBUFFER  0000002
 #define _IO_LINEBUFF  0000001
#elif defined(__linux)
 #define _IO_ALTERED_RW 000004
 #define _IO_APPEND    0002000
 #define _IO_CREATE    0000100
 #define _IO_TRUNCATE  0001000
 #define _IO_NOBUFFER  0000002
 #define _IO_LINEBUFF  0000001
#else
 #define _IO_ALTERED_RW _IORW
 #define _IO_APPEND    _IOAND
 #define _IO_CREATE    _IOCRT
 #define _IO_TRUNCATE  _IOTRC
 #define _IO_NOBUFFER  _IONBF
 #define _IO_LINEBUFF  _IOLBF
#endif

/*
 *  Gathering location for the different systems usage of FILE*
 */
/* currently cant be used without Pheonix code */
#define DATA_SHIFT
//#define __PHIX_LOCKS

#ifndef _AS__FILE_T_H_
#define _AS__FILE_T_H_

/* to be incorporated into pxUserSpace.c? but here so can use individual
 * file functions. This is used on exit(). */
extern void * _iofiles[ ];
extern size_t _iolocks;

/* As of Aug 2019 this looking like possible FILE, bufsize max 2^31 */
#if 0
typedef struct _IOFILE {
  int32_t     _flags;     (32)     consider const
  int32_t         _r;     (32)     read
  int32_t         _w;     (32)     write
  int16_t         _u;     (16)     unget (need 5 bits)
  int16_t    _fileno;     (16)     fileno we attached to, consider const
  uint8_t *       _p;     (64/32)  needed for malloc
  uint8_t *    _base;     (64/32)  needed for malloc
  uint32_t   _bfsize;     (32)     malloc(d) size
  pid_t         _pid;     (32)     used for locks, threads, pipes
  int64_t    _offset;     (64)
    /* want pointer here to structure, to hold extras others need */
    /* like glibc _IO_MAGIC, doesn't belong as flag (not binary) */
  iolock_t     _lock;     () struct for pthread, or posix requirements
} FILE;
#endif

#ifdef __APPLE__

  /* note matches FILE's unsigned char *_up; */
struct __sFILEX {
	unsigned char	*_up;         /* saved _p when _p is doing ungetc data */
	pthread_mutex_t	fl_mutex;   /* used for MT-safety */
	pthread_t	fl_owner;         /* current owner */
	int		fl_count;             /* recursive lock count */
	int		orientation:2;        /* orientation for fwide() */
	int		counted:1;            /* stream counted against STREAM_MAX */
    /* no clue how they defined, nor do i want all info from wchar.h */
    /* use this for now */
  struct {
    char __wchb[8];
  } mbstate;
/* 	added member to allow working popen() */
  pid_t _pid;
};

#if !defined (__phix) && defined (__APPLE__)
extern struct __sFILEX usual_extra[ ];
#endif

struct __sbuf {
  unsigned char *_base;
  int            _size;
};

typedef struct __sFILE_mac {
  unsigned char *_p;        /* current position in (some) buffer */
  int            _r;        /* read space left for getc() */
  int            _w;        /* write space left for putc() */
  short      _flags;        /* flags, below; this FILE is free if 0 */
  short       _file;        /* fileno, if Unix descriptor, else -1 */
  struct __sbuf _bf;        /* the buffer (at least 1 byte, if !NULL) */
  int      _lbfsize;        /* 0 or -_bf._size, for inline putc */

        /* operations, note different size_t on _read, _write */
  void     *_cookie;        /* cookie passed to io functions (self) */
  int      (*_close)(void *);
  ssize_t  (*_read)(void *, char *, size_t);
  fpos_t   (*_seek)(void *, fpos_t, int);
  int      (*_write)(void *, char const *, size_t);

        /* separate buffer for long sequences of ungetc() */
  struct __sbuf  _ub;       /* ungetc buffer */
    /* redefined? struct __sFILEX *  */
  struct __sFILEX *_extra;  /* additions to FILE to not break ABI */
#define _up _extra->_up
  /* unsigned char *_up;       saved _p when _p is doing ungetc data */
  int            _ur;       /* saved _r when _r is counting ungetc data */

        /* tricks to meet minimum requirements even when malloc() fails */
  unsigned char _ubuf[3];   /* guarantee an ungetc() buffer */
  unsigned char _nbuf[1];   /* guarantee a getc() buffer */

        /* separate buffer for fgetln() when line crosses buffer boundary */
  struct __sbuf  _lb;       /* buffer for fgetln() */

        /* Unix stdio files get aligned to block boundaries on fseek() */
  int       _blksize;       /* stat.st_blksize (may be != _bf._size) */
    /* This is offset of where read to begin. In unbuffered, it is an index,
     * for buffered it is the same, when combined with the amount read in buffer */
  fpos_t     _offset;       /* current lseek offset (see WARNING, lol where?) */
} FILE_mac;

extern FILE_mac __sF[];
#define stdin   ((FILE *)(&__sF[0]))
#define stdout  ((FILE *)(&__sF[1]))
#define stderr  ((FILE *)(&__sF[2]))

#define FILE FILE_mac

#elif defined __linux  /* !__APPLE__ */

/*#define _IO_MAGIC 0xFBAD0000          Magic number */
/*#define _OLD_STDIO_MAGIC 0xFABC0000   Emulate old stdio. */
/*#define _IO_MAGIC_MASK 0xFFFF0000     Access magic */
/*#define _IO_USER_BUF 1                User owns buffer; don't delete it on close. */
/*#define _IO_UNBUFFERED 2 */
/*#define _IO_NO_READS   4  Reading not allowed */
/*#define _IO_NO_WRITES  8  Writing not allowd */
/*#define _IO_EOF_SEEN   0x10 */
/*#define _IO_ERR_SEEN   0x20 */
/*#define _IO_DELETE_DONT_CLOSE 0x40  Don't call close(_fileno) on cleanup. */
/*#define _IO_LINKED            0x80  Set if linked (using _chain) to streambuf::_list_all.*/
/*#define _IO_IN_BACKUP         0x100 */
/*#define _IO_LINE_BUF          0x200 */
/*#define _IO_TIED_PUT_GET      0x400 Set if put and get pointer logicly tied. */
/*#define _IO_CURRENTLY_PUTTING 0x800 */
/*#define _IO_IS_APPENDING      0x1000 */
/*#define _IO_IS_FILEBUF        0x2000 */
/*#define _IO_BAD_SEEN          0x4000 */
/*#define _IO_USER_LOCK         0x8000 */

  /* cant use my off_t, always 64bit */
typedef long int            __off_t;
typedef long int            __off64_t;
/* Forward declarations.  */
struct _IO_FILE;
struct _IO_marker;

struct _IO_marker {
  struct _IO_marker *_next;
  struct _IO_FILE *  _sbuf;
  int                 _pos;
};

/* typedef void _IO_lock_t; */
/* typedef struct { int lock; int cnt; void *owner; } _IO_lock_t; */
typedef struct {
  pthread_mutex_t	fl_mutex;
  pthread_t	      fl_owner;
  int		          fl_count;
} _IO_lock_t;

struct _IO_FILE {
  int _flags;             /* High-order word is _IO_MAGIC; rest is flags. */
  /* The following pointers correspond to the C++ streambuf protocol. */
  char *_IO_read_ptr;     /* Current read pointer */
  char *_IO_read_end;     /* End of get area. */
  char *_IO_read_base;    /* Start of putback+get area. */
  char *_IO_write_base;   /* Start of put area. */
  char *_IO_write_ptr;    /* Current put pointer. */
  char *_IO_write_end;    /* End of put area. */
  char *_IO_buf_base;     /* Start of reserve area. */
  char *_IO_buf_end;      /* End of reserve area. */
  /* The following fields are used to support backing up and undo. */
  char *_IO_save_base;    /* Pointer to start of non-current get area. */
  char *_IO_backup_base;  /* Pointer to first valid character of backup area */
  char *_IO_save_end;     /* Pointer to end of non-current get area. */
  struct _IO_marker *_markers;
  struct _IO_FILE *   _chain;
  int                _fileno;
  int                _flags2;
  __off_t        _old_offset;   /* using for memory file _eof */
  unsigned short _cur_column;
  signed char _vtable_offset;
  char          _shortbuf[1];
  _IO_lock_t *         _lock;
    /* same as APPLE definition,
     * offset + read chars is actual position for buffered */
  __off64_t          _offset;
  void *              __pad1;
  void *              __pad2;
  void *              __pad3;
  void *              __pad4;
  size_t              __pad5;
  int                  _mode;   /* using for pid_t _pid; */
  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];
};

typedef struct _IO_FILE _IO_FILE;

/* Standard streams. */
extern _IO_FILE _IO_2_1_stdin_;
extern _IO_FILE _IO_2_1_stdout_;
extern _IO_FILE _IO_2_1_stderr_;
#define stdin   ((FILE*)&_IO_2_1_stdin_)
#define stdout  ((FILE*)&_IO_2_1_stdout_)
#define stderr  ((FILE*)&_IO_2_1_stderr_)

#define FILE _IO_FILE

#else    /* !__APPLE__ !__CYGWIN__ !__linux */
  #error: unknown FILE structure
#endif  /* FILE * */

  /* Translation unit XXX was initialized trying to do 2 buffers */

#ifdef __APPLE__
 #define __SRD    0x0004    /* OK to read */
 #define __SWR    0x0008    /* OK to write */
 #define __SRW    0x0010    /* open for reading & writing */
 #define __SEOF   0x0020    /* found EOF */
 #define __SERR   0x0040    /* found error */
 #define __SAPP   0x0100    /* fdopen()ed in append mode */
    /* fileno */
 #define _io_fileno(x)              ((x)->_file)
    /* modes */
 #define _io_writable(x)            (!!((x)->_flags & (__SWR|__SRW)))
 #define _io_readable(x)            (!!((x)->_flags & (__SRD|__SRW)))
 #define _io_buffered(x)            (!((x)->_flags & _IONBF))
 #define _io_fbf(x)                 (((x)->_flags & (_IOLBF|_IONBF)) == 0)
 #define _io_lbf(x)                 (!!((x)->_flags & _IOLBF))
 #define _io_append(x)              (!!((x)->_flags & __SAPP))
 #define _io_unget(x)               (!!(x)->_ur)
 #define _io_shared(x)              (1)
 //#define _io_shared(x)              (!!((x)->_flags & _IOSHM))
    /* memory files */
 #define _io_mem(x)                 (!!((x)->_flags & _IOMEM))
 #define _io_mm2(x)                 (((x)->_flags & _IOMM2) == _IOMM2)
 #define _io_set_meof(x,y)          ((x)->_lb._size = (int)(y))
 #define _io_get_meof(x)            ((x)->_lb._size)
    /* flags */
 #define _io_eof(x)                 (!!((x)->_flags & __SEOF))
 #define _io_set_eof(x)             ((x)->_flags |= __SEOF)
 #define _io_clear_eof(x)           ((x)->_flags &= ~__SEOF)
 #define _io_err(x)                 (!!((x)->_flags & __SERR))
 #define _io_set_err(x)             ((x)->_flags |= __SERR)
 #define _io_clearerr(x)            ((x)->_flags &= ~(__SERR | __SEOF))
 #define _io_set_utf(x)             ((x)->_flags |= _IOUTF)
 #define _io_clearutf(x)            ((x)->_flags &= ~_IOUTF)
 #define _io_set_shm(x)             ((x)->_flags |= _IOSHM)
 #define _io_clearshm(x)            ((x)->_flags &= ~_IOSHM)
 #define _io_vbuf(x)                (!!((x)->_flags & _IOOWN))
    /* fixes for FILE's */
 #define _io_fix0(x)                \
    if (_io_fileno(x) == STDIN_FILENO) ((x)->_flags |= (_IOLBF | _IOSHM)); \
    if (_io_fileno(x) == STDOUT_FILENO) ((x)->_flags |= (__SAPP | _IOLBF | _IOSHM));
//    if (_io_fileno(x) == STDOUT_FILENO) ((x)->_flags |= (__SAPP | _IOSHM));
  /* must keep until __APPLE__ no longer creates stdin/stdout/stderr */
 #define _io_fix1(x)                \
    if ((_io_get_rdbase((x)) == NULL) && (_io_buffered((x)))) {           \
      _io_set_base((x), ((unsigned char*)malloc((size_t)(UGET_MAX + BUFSIZ)) + UGET_MAX)); \
      _io_set_buffersize((x), BUFSIZ);                                    \
      _io_reset_ptr(x);                                                   \
      _io_set_rd_buffered(x, 0);                                          \
      _io_set_wr_buffered(x, 0);                                          \
      _io_set_ug_buffered(x, 0);                                          \
    }
    /* initializing malloc(d), try to insulate and isolate */
 #define _io_get_base(x)            ((x)->_bf._base)
 #define _io_set_base(x,y)          ((x)->_bf._base = (unsigned char*)(y))
 #define _io_get_buffersize(x)      ((size_t)((x)->_bf._size))
 #define _io_set_buffersize(x,y)    ((x)->_bf._size = (int)(y))
 #define _io_reset_ptr(x)           ((x)->_p = (x)->_bf._base)
 #define _io_reset_wrPtr(x)         ((x)->_p = (x)->_bf._base)
 #define _io_multptr_init(x)
    /* read information */
 #define _io_get_rdbase(x)          ((x)->_bf._base)
#ifdef DATA_SHIFT
      #define _io_get_wrbase(x)          ((x)->_p - (x)->_w)
      #define _io_get_rdPtr(x)           \
          ((x)->_p - _io_get_wr_buffered(x) - _io_get_rd_buffered(x))
      #define _io_set_rdPtr(x,y)
      #define _io_set_wrPtr(x,y)         ((x)->_p = (unsigned char*)(y))
        /* read buffer read by 'y' read amount (output) */
      #define _io_update_rdInfo(x,y)     ((x)->_r -= (int)(y))
#else
      #define _io_get_wrbase(x)          ((x)->_bf._base)
      #define _io_get_rdPtr(x)           ((x)->_p)
      #define _io_set_rdPtr(x,y)         ((x)->_p = (unsigned char*)(y))
      #define _io_set_wrPtr(x,y)
        /* read buffer read by 'y' read amount (output) */
      #define _io_update_rdInfo(x,y)     ((x)->_p += (y), (x)->_r -= (int)(y))
#endif
 #define _io_get_rd_buffered(x)     ((size_t)((x)->_r))
 #define _io_set_rd_buffered(x,y)   ((x)->_r = (int)(y))
 #define _io_add_rd_buffered(x,y)   ((x)->_r += (int)(y))
    /* read buffer filled with y amount, set file data (input) */
 #define _io_get_ug_buffered(x)     ((x)->_ur)
 #define _io_set_ug_buffered(x,y)   ((x)->_ur = (int)(y))
 #define _io_add_ug_buffered(x,y)   ((x)->_ur += (int)(y))
    /* write information, one has a pointer, other imaginary */
 #define _io_get_wr_buffered(x)     ((size_t)(x)->_w)
 #define _io_set_wr_buffered(x,y)   ((x)->_w = (int)(y))
 #define _io_add_wr_buffered(x,y)   ((x)->_w += (int)(y))
    /* wrote into buffer 'y' amount (input) */
 #define _io_get_wrPtr(x)           ((x)->_p)
 #define _io_adjust_wrPtr(x,y)      ((x)->_p += (y))
 #define _io_update_wrInfo(x,y)     ((x)->_p += (y), (x)->_w += (int)(y))
    /* initialize */
 #define _io_set_wrInfo(x,y)        \
    ((x)->_p = _io_get_wrbase((x)), (x)->_w = (int)(y))
    /* popen() */
 #define _io_set_pid(x,y)           ((x)->_extra->_pid = (y))
 #define _io_get_pid(x)             ((x)->_extra->_pid)
    /* locking */
 #define _io_lock_owner(x)          ((x)->_extra->fl_owner)
 #define _io_lock_count(x)          ((x)->_extra->fl_count)
 #define _io_lock_mutex(x)          ((x)->_extra->fl_mutex)

#else

#define __SRD    0x0008    /* OK to read __l(not write)*/
#define __SWR    0x0004    /* OK to write __l(not read)*/
/* #define __SRW    0x0000 */
#define __SRW    0x000C    /* open for reading & writing (mask) */
#define __SEOF   0x0010    /* found EOF */
#define __SERR   0x0020    /* found error */
#define __SAPP   0x1000    /* fdopen()ed in append mode */
#define __SLBF   0x0200
#define __SFBF   0x2000
#define __SNBF   0X0002
/* fileno */
 #define _io_fileno(x)              ((x)->_fileno)
    /* modes */
 #define _io_writable(x)            \
    ((((x)->_flags & __SRW) == 0) || (!!((x)->_flags & __SWR)))
 #define _io_readable(x)            \
    ((((x)->_flags & __SRW) == 0) || (!!((x)->_flags & __SRD)))
// #define _io_writable(x)            (!!((x)->_flags & __SWR))
// #define _io_readable(x)            (!!((x)->_flags & __SRD))
 #define _io_buffered(x)            (!((x)->_flags & __SNBF))
 #define _io_fbf(x)                 (!!((x)->_flags & __SFBF))
 #define _io_append(x)              (!!((x)->_flags & __SAPP))
        /* NULL pointers, used as counter */
 #define _io_unget(x)               ((x)->_IO_save_base != (x)->_IO_save_end)
 #define _io_shared(x)              (1)
    /* see below */
 //#define _io_shared(x)              (!!((x)->_flags & _IOSHM))
    /* memory files */
 #define _io_mem(x)                 (!!((x)->_flags & _IOMEM))
 #define _io_mm2(x)                 (((x)->_flags & _IOMM2) == _IOMM2)
 #define _io_set_meof(x,y)          ((x)->_old_offset = (off_t)(y))
 #define _io_get_meof(x)            ((size_t)((x)->_old_offset))
    /* flags */
 #define _io_eof(x)                 (!!((x)->_flags & __SEOF))
 #define _io_set_eof(x)             ((x)->_flags |= __SEOF)
 #define _io_clear_eof(x)           ((x)->_flags &= ~__SEOF)
 #define _io_err(x)                 (!!((x)->_flags & __SERR))
 #define _io_set_err(x)             ((x)->_flags |= __SERR)
 #define _io_clearerr(x)            ((x)->_flags &= ~(__SERR | __SEOF))
    /* these have to go to _flag2 unused atm */
 #define _io_set_utf(x)             ((x)->_flags |= _IOUTF)
 #define _io_clearutf(x)            ((x)->_flags &= ~_IOUTF)
 #define _io_set_shm(x)             ((x)->_flags |= _IOSHM)
 #define _io_clearshm(x)            ((x)->_flags &= ~_IOSHM)
/* XXX 200218 found missing, needs verify */
 #define _io_vbuf(x)                (!!((x)->_flags & _IOOWN))
    /* fixes for FILE's */
 #define _io_fix0(x)                \
    if (_io_fileno(x) == STDIN_FILENO) ((x)->_flags |= (_IOLBF | _IOSHM)); \
    if (_io_fileno(x) == STDOUT_FILENO) ((x)->_flags |= (_IOLBF | __SAPP | _IOSHM));
 #define _io_fix1(x)                \
    if ((_io_get_base((x)) == NULL) && (_io_buffered((x)))) {             \
      _io_set_base((x), ((unsigned char*)malloc((size_t)(UGET_MAX + BUFSIZ)) + UGET_MAX)); \
      _io_set_buffersize((x), BUFSIZ);                                    \
      _io_multptr_init(x);                                                \
      (x)->_offset = 0;                                                   \
    }
    /* initializing malloc(d), try to insulate and isolate */
 #define _io_get_base(x)            ((unsigned char*)(x)->_IO_buf_base)
 #define _io_set_base(x,y)          ((x)->_IO_buf_base = (char*)(y))
 #define _io_get_buffersize(x)      ((size_t)((x)->_IO_buf_end - (x)->_IO_buf_base))
 #define _io_set_buffersize(x,y)    \
    ((x)->_IO_buf_end = (x)->_IO_buf_base + (y))
 #define _io_reset_ptr(x)           ((x)->_IO_read_ptr = (x)->_IO_read_base)
 #define _io_reset_wrPtr(x)         ((x)->_IO_write_ptr = (x)->_IO_write_base)
 #define _io_multptr_init(x)        \
    ((x)->_IO_read_base = (x)->_IO_buf_base, \
     (x)->_IO_write_base = (x)->_IO_buf_base, \
     (x)->_IO_read_ptr = (x)->_IO_buf_base, \
     (x)->_IO_write_ptr = (x)->_IO_buf_base, \
     (x)->_IO_read_end = (x)->_IO_buf_base, \
     (x)->_IO_write_end = (x)->_IO_buf_end)
    /* read information */
 #define _io_get_rdbase(x)          ((unsigned char*)(x)->_IO_read_base)
 #define _io_get_wrbase(x)          ((unsigned char*)(x)->_IO_write_base)
 #define _io_get_rdPtr(x)           ((unsigned char*)(x)->_IO_read_ptr)
 #define _io_get_wrPtr(x)           ((unsigned char*)(x)->_IO_write_ptr)
 #define _io_set_rdPtr(x,y)         ((x)->_IO_read_ptr = (char*)(y))
 #define _io_set_wrPtr(x,y)         ((x)->_IO_write_ptr = (char*)(y))
    /* read buffer read by 'y' read amount (output) */
 #define _io_update_rdInfo(x,y)     ((x)->_IO_read_ptr += (y))

 #define _io_get_rd_buffered(x)     \
    ((size_t)((x)->_IO_read_end) - (size_t)((x)->_IO_read_ptr))
 #define _io_set_rd_buffered(x,y)   ((x)->_IO_read_end = (x)->_IO_read_ptr + (y))
 #define _io_add_rd_buffered(x,y)   ((x)->_IO_read_ptr -= (y))
 #define _io_get_ug_buffered(x)     ((x)->_IO_save_end - (x)->_IO_save_base)
 #define _io_set_ug_buffered(x,y)   ((x)->_IO_save_end = (x)->_IO_save_base + (y))
 #define _io_add_ug_buffered(x,y)   ((x)->_IO_save_end += (y))
    /* write information, one has a pointer, other imaginary */
 #define _io_get_wr_buffered(x)     \
    ((size_t)((x)->_IO_write_ptr) - (size_t)((x)->_IO_read_end))
 #define _io_set_wr_buffered(x,y)   \
    ((x)->_IO_write_ptr = (x)->_IO_write_base + (int)(y))
/* XXX 200218 found was missing, not verified */
 #define _io_add_wr_buffered(x,y)   _io_set_wr_buffered(x,y)
    /* wrote into buffer 'y' amount (input) */
 #define _io_adjust_wrPtr(x,y)      ((x)->_IO_write_ptr += (y))
 #define _io_update_wrInfo(x,y)     ((x)->_IO_write_ptr += (y))
    /* initialize */
 #define _io_set_wrInfo(x,y)        \
    ((x)->_IO_write_ptr = (x)->_IO_write_base)
    /* popen() */
 #define _io_set_pid(x,y)           ((x)->_mode = (y))
 #define _io_get_pid(x)             ((x)->_mode)
    /* locking */
 #define _io_lock_owner(x)          ((x)->_lock->fl_owner)
 #define _io_lock_count(x)          ((x)->_lock->fl_count)
 #define _io_lock_mutex(x)          ((x)->_lock->fl_mutex)

#endif

#endif  /* !_AS__FILE_T_H_ */
