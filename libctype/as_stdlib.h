/*
 *  stdlib.h - standard library definitions
 *  Pheonix
 *
 *  Created by Steven J Abner on Wed Sep 5 2007.
 *  Copyright (c) 2007, 2012, 2019. All rights reserved.
 *
 *  Dedicated to Erin JoLynn Abner.
 *  Special thanks to God!
 *
 */
/*
 *   These functions are core and replacements of all random functions.
 * int and long return based functions are 32bit functions typecast to
 * whatever size int and long might be. All long long types are 64bit
 * results typecast to whatever long long represents. This provides
 * maximum portability... 32 bit machine answers the same as 64 bit machine,
 * and 64 bit answers same on 32 bit machines.
 */
/*
long                rng(unsigned long long *);                     __standard__(Pheonix)
long long           rngll(unsigned long long *);                   __standard__(Pheonix)
double              rngcos(unsigned long long *);                  __standard__(Pheonix)
long                rngmax(unsigned long long *, unsigned long);   __standard__(Pheonix)
*/

#ifndef _AS_STDLIB_H_
#define _AS_STDLIB_H_

#include "sys/as_types.h"    /* defines NULL & other defines */
#include "sys/as_wait.h"

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


#define EXIT_FAILURE  1  /* Unsuccessful termination for exit(). */
#define EXIT_SUCCESS  0  /* Successful termination for exit(). */

typedef	struct {
  int  quot;          /* Quotient */
  int  rem;           /* Remainder */
} div_t;

typedef	struct {
  long   quot;        /* Quotient */
  long   rem;         /* Remainder */
} ldiv_t;

typedef	struct {
  long long   quot;   /* Quotient */
  long long   rem;    /* Remainder */
} lldiv_t;

#define RAND_MAX    0x7fffffff

/* locale related.. not handled as of yet.. so bogus definition */
/* Integer expression whose value is the maximum number of bytes in a character
 * specified by the current locale.  */
#define MB_CUR_MAX  4

#ifdef __cplusplus
extern "C" {
#endif

#define quadruple octle

void            _Exit(int);
long            a64l(const char *);                               __standard__(X/Open)
long long       a64ll(const char *s);                             __standard__(Pheonix)
void            abort(void);
int             abs(int);
int             atexit(void (*)(void));
double          atof(const char *);
int             atoi(const char *);
long            atol(const char *);
long long       atoll(const char *);
void *          bsearch(const void *, const void *, size_t, size_t,
                        int (*)(const void *, const void *));
void *          calloc(size_t, size_t);
div_t           div(int, int);
double          drand48(void);                                    __standard__(X/Open)
double          erand48(unsigned short [3]);                      __standard__(X/Open)
void            exit(int) __attribute__((noreturn));
void            free(void *);
char *          getenv(const char *);
int             getsubopt(char **, char *const *, char **);
int             grantpt(int);                                     __standard__(X/Open)
char *          initstate(unsigned, char *, size_t);              __standard__(X/Open)
long            jrand48(unsigned short [3]);                      __standard__(X/Open)
char *          l64a(long);                                       __standard__(X/Open)
long            labs(long);
void            lcong48(unsigned short [7]);                      __standard__(X/Open)
ldiv_t          ldiv(long, long);
char *          ll64a(long long value);                           __standard__(Pheonix)
long long       llabs(long long);
lldiv_t         lldiv(long long, long long);
long            lrand48(void);                                    __standard__(X/Open)
void *          malloc(size_t);
int             mblen(const char *, size_t);
size_t          mbstowcs(wchar_t *restrict, const char *restrict, size_t);
int             mbtowc(wchar_t *restrict, const char *restrict, size_t);
char *          mkdtemp(char *);                                  __standard__(POSIX.1-2008)
int             mkstemp(char *);                                  __standard__(POSIX.1-2008)
long            mrand48(void);                                    __standard__(X/Open)
long            nrand48(unsigned short [3]);                      __standard__(X/Open)
int             posix_memalign(void **, size_t, size_t);          __standard__(Advisory)
int             posix_openpt(int);                                __standard__(X/Open)
char *          ptsname(int);                                     __standard__(X/Open)
int             putenv(char *);                                   __standard__(X/Open)
void            qsort(void *, size_t, size_t, int (*)(const void *, const void *));
int             rand(void);
long            random(void);                                     __standard__(X/Open)
void *          realloc(void *, size_t);
char *          realpath(const char *restrict, char *restrict);   __standard__(X/Open)
long            rng(unsigned long long *);                        __standard__(Pheonix)
long long       rngll(unsigned long long *);                      __standard__(Pheonix)
double          rngcos(unsigned long long *);                     __standard__(Pheonix)
long            rngmax(unsigned long long *, unsigned long);      __standard__(Pheonix)
int             rpath(const char *restrict file_name, char **resolved_name); __standard__(Pheonix)
unsigned short *seed48(unsigned short [3]);                       __standard__(X/Open)
int             setenv(const char *, const char *, int);          __standard__(POSIX.1-2008)
void            setkey(const char *);                             __standard__(X/Open)
char *          setstate(char *);                                 __standard__(X/Open)
void            srand(unsigned);
void            srand48(long);                                    __standard__(X/Open)
void            srandom(unsigned);                                __standard__(X/Open)
double          strtod(const char *restrict, char **restrict);
float           strtof(const char *restrict, char **restrict);
long            strtol(const char *restrict, char **restrict, int);
LD_SUBST        strtold(const char *restrict, char **restrict);
long long       strtoll(const char *restrict, char **restrict, int);
unsigned long   strtoul(const char *restrict, char **restrict, int);
unsigned long long
                strtoull(const char *restrict, char **restrict, int);
int             system(const char *);
int             unlockpt(int);                                    __standard__(X/Open)
int             unsetenv(const char *);                           __standard__(POSIX.1-2008)
size_t          wcstombs(char *restrict, const wchar_t *restrict, size_t);
int             wctomb(char *, wchar_t);

  /* new globals */
double          stod(const char *restrict, ssize_t *);  __standard__(Pheonix)
float           stof(const char *restrict, ssize_t *);  __standard__(Pheonix)
LD_SUBST        stold(const char *restrict, ssize_t *); __standard__(Pheonix)
quadruple       stoq(const char *restrict, ssize_t *);  __standard__(Pheonix)
quadruple       strtoq(const char *restrict, char **);  __standard__(Pheonix)

/* Obsolescent */
int             rand_r(unsigned *);

#ifdef __cplusplus
}
#endif
#endif /* !_AS_STDLIB_H_ */
