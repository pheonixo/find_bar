/*
 *  stdio.h - standard buffered input/output
 *  Pheonix
 *
 *  Created by Steven J Abner on Fri Aug 13 2004.
 *  Copyright (c) 2004, 2012, 2019. All rights reserved.
 *
 */

#ifndef _AS_STDIO_H_
#define _AS_STDIO_H_

#include "sys/as_types.h" /* size_t, NULL, fpos_t */
#include "as_stdarg.h"    /* va_list */
#include "_FILE_t.h"      /* different FILE types per systems, translator */

/* for testing on non double-double machines,
 * replace actual when ready to release */
//#undef __LDBL_MANT_DIG__
//#define __LDBL_MANT_DIG__ 113
//#define __LDBL_MANT_DIG__ 106
//#define __LDBL_MANT_DIG__ 53

#if ((__LDBL_MANT_DIG__ == 106) || (__LDBL_MANT_DIG__ == 113))
 #define LD_SUBST  quadruple
#elif (__LDBL_MANT_DIG__ == 53)
 #define LD_SUBST  double
#else
 #define LD_SUBST  long double
#endif


#define EOF           (-1)      /* End-of-file return value. */
  /* defined in unistd.h, but too useful here in stdio.h */
#define STDERR_FILENO  2  /* File number of stderr. */
#define STDIN_FILENO   0  /* File number of stdin. */
#define STDOUT_FILENO  1  /* File number of stdout. */

    /* fseek() values */
#define SEEK_SET         0      /* Seek relative to start-of-file. */
#define SEEK_CUR         1      /* Seek relative to current position. */
#define SEEK_END         2      /* Seek relative to end-of-file. */
    /* misc limits and values */
#define FOPEN_MAX       32      /* Max streams opened simultaneously. __A(20) */
#define FILENAME_MAX  1024      /* Max bytes of filename string to be opened. */
#define BUFSIZ        4096      /* Default size of buffers. __l(8192) __A(1024)*/
#define UGET_MAX        32      /* Max bytes allowed for ungetc() (sorta) */
#define L_ctermid     1024      /* Maximum size of character array to hold
                                 * ctermid() output. */
    /* FILE _flag values (match fcntl.h)*/
#define _IOMODE      0000017    /* Mask for file access modes (O_ACCMODE) */
#define _IORD        0000000    /* Opened reading (O_RDONLY) */
#define _IOWR        0000001    /* Opened writing (O_WRONLY) */
#define _IORW        0000002    /* Opened for reading and writing (O_RDWR) */
#define _IOAND       0000100    /* (O_APPEND) */
                   /*0000010       (O_SEARCH) dir buffering of dirents ? */
#define _IOCRT       0004000    /* (O_CREAT) */
#define _IOTRC       0010000    /* (O_TRUNC) */
    /* _IOFBF, _IOLBF, _IONBF required */
#ifdef __phix
 #define _IONBF      0000200    /* IO unbuffered. (e.g.: stdout, pipe) */
 #define _IOLBF      0000400    /* IO line buffered. (e.g.: stdin) */
 #define _IOFBF      0000000    /* IO fully buffered. (all, except above) */
 #define _IOMEM      0001000    /* IO memory file */
 #define _IOMM2      0003000    /* IO memory file, with returning info */
#else
 #define _IONBF      0000002    /* Input/output unbuffered. (e.g.: stdout, pipe) */
 #define _IOLBF      0000001    /* Input/output line buffered. (e.g.: stdin) */
 #define _IOFBF      0000000    /* Input/output fully buffered. (most files) */
 #define _IOMBF      0000200    /* malloc()'d buffer */
 #define _IOMEM      0001000    /* IO memory file */
 #define _IOMM2      0003000    /* IO memory file, with returning info */
#endif
#define _IOUTF       0020000    /* orientation, set if non-byte mode */
#define _IOEOF       0040000    /* EOF reached on read */
#define _IOERR       0100000    /* I/O error */
#define _IOSHM       0200000    /* shared resource, set locks around resources */
#define _IOOWN       0400000    /* malloc'd buffer, free on close */

#ifdef __cplusplus
extern "C" {
#endif

void        clearerr(FILE *);
char *      ctermid(char *);                          __standard__(POSIX.1-2008)
int         dprintf(int, const char *restrict, ...);  __standard__(POSIX.1-2008)
int         fclose(FILE *);
FILE *      fdopen(int, const char *);                __standard__(POSIX.1-2008)
int         feof(FILE *);
int         ferror(FILE *);
int         fflush(FILE *);
int         fflush_unlocked(FILE *);                  __standard__(Pheonix)
int         fgetc(FILE *);
int         fgetpos(FILE *restrict, fpos_t *restrict);
int         fileno(FILE *);                           __standard__(POSIX.1-2008)
char *      fgets(char *restrict, int, FILE *restrict);
void        flockfile(FILE *);                        __standard__(POSIX.1-2008)
FILE *      fmemopen(void *restrict, size_t,
                               const char *restrict); __standard__(POSIX.1-2008)
FILE *      fopen(const char *restrict, const char *restrict);
int         fprintf(FILE *restrict, const char *restrict, ...);
int         fputc(int, FILE *);
int         fputs(const char *restrict, FILE *restrict);
size_t      fread(void *restrict, size_t, size_t, FILE *restrict);
FILE *      freopen(const char *restrict, const char *restrict, FILE *restrict);
int         fscanf(FILE *restrict, const char *restrict, ...);
int         fseek(FILE *, long, int);
int         fseeko(FILE *, off_t, int);               __standard__(POSIX.1-2008)
int         fsetpos(FILE *, const fpos_t *);
long        ftell(FILE *);
off_t       ftello(FILE *);                           __standard__(POSIX.1-2008)
int         ftrylockfile(FILE *);                     __standard__(POSIX.1-2008)
void        funlockfile(FILE *);                      __standard__(POSIX.1-2008)
size_t      fwrite(const void *restrict, size_t, size_t, FILE *restrict);
int         getc(FILE *);
int         getc_unlocked(FILE *);                    __standard__(POSIX.1-2008)
int         getchar(void);
int         getchar_unlocked(void);                   __standard__(POSIX.1-2008)
ssize_t     getdelim(char **restrict, size_t *restrict,
                                int, FILE *restrict); __standard__(POSIX.1-2008)
ssize_t     getline(char **restrict, size_t *restrict,
                                     FILE *restrict); __standard__(POSIX.1-2008)
FILE *      open_memstream(char **, size_t *);        __standard__(POSIX.1-2008)
int         pclose(FILE *);                           __standard__(POSIX.1-2008)
void        perror(const char *);
FILE *      popen(const char *, const char *);        __standard__(POSIX.1-2008)
int         printf(const char *restrict, ...);
int         putc(int, FILE *);
int         putc_unlocked(int, FILE *);               __standard__(POSIX.1-2008)
int         putchar(int);
int         putchar_unlocked(int);                    __standard__(POSIX.1-2008)
int         puts(const char *);
int         remove(const char *);
int         rename(const char *, const char *);
int         renameat(int, const char *, int,
                                     const char *);   __standard__(POSIX.1-2008)
void        rewind(FILE *);
int         scanf(const char *restrict, ...);
void        setbuf(FILE *restrict, char *restrict);
int         setvbuf(FILE *restrict, char *restrict, int, size_t);
int         snprintf(char *restrict, size_t, const char *restrict, ...);
int         sprintf(char *restrict, const char *restrict, ...);
int         sscanf(const char *restrict, const char *restrict, ...);
FILE *      tmpfile(void);
int         ungetc(int, FILE *);
int         ungetc_unlocked(int, FILE *);             __standard__(POSIX.1-2008)
int         vdprintf(int, const char *restrict,
                                            va_list); __standard__(POSIX.1-2008)
int         vfprintf(FILE *restrict, const char *restrict, va_list);
int         vfscanf(FILE *restrict, const char *restrict, va_list);
int         vprintf(const char *restrict, va_list);
int         vscanf(const char *restrict, va_list);
int         vsnprintf(char *restrict, size_t, const char *restrict, va_list);
int         vsprintf(char *restrict, const char *restrict, va_list);
int         vsscanf(const char *restrict, const char *restrict, va_list);

#define quadruple octle
  /* new globals */
float           ftof(FILE *, ssize_t *);        __standard__(Pheonix)
double          ftod(FILE *, ssize_t *);        __standard__(Pheonix)
LD_SUBST        ftold(FILE *, ssize_t *);       __standard__(Pheonix)
quadruple       ftoq(FILE *, ssize_t *);        __standard__(Pheonix)

  /* Obsolescent */
#define TMP_MAX   10000   /* _T#0000.TMP - _T#9999.TMP (DOS uses 8.3 convention) */
#define L_tmpnam     16   /* Max bytes of tmp filename string (DOS)(4 longs)(PATH). */
#ifdef __APPLE__
 #define P_tmpdir   "/var/tmp/"   /* Default directory prefix for tmpnam(). */
#else
 #define P_tmpdir   "/tmp/"       /* Default directory prefix for tmpnam(). */
#endif
char    *gets(char *);
char    *tempnam(const char *, const char *);
char    *tmpnam(char *);

#ifdef __cplusplus
}
#endif
#endif /* !_AS_STDIO_H_ */
