/*
 *  string.h - string operations
 *  Pheonix
 *
 *  Created by Steven J Abner on Wed Aug 04 2004.
 *  Copyright (c) 2004-2012, 2019. All rights reserved.
 *
 */

#ifndef _AS_STRING_H_
#define _AS_STRING_H_

#include "sys/as_types.h"    /* defines size_t, NULL & other defines */

#ifdef __cplusplus
extern "C" {
#endif

void *          memccpy(void *restrict, const void *restrict, int, size_t);
void *          memchr(const void *, int, size_t);
int             memcmp(const void *, const void *, size_t);
void *          memcpy(void *restrict, const void *restrict, size_t);
void *          memmove(void *, const void *, size_t);
char *          memrchr(const char *, int, size_t);                 __standard__(Pheonix)
void *          memset(void *, int, size_t);
char *          stpccpy(char *s1, const char *s2, int c, size_t n); __standard__(Pheonix)
char *          stpcpy(char *restrict, const char *restrict);     __standard__(POSIX.1-2008)
char *          stpncpy(char *restrict, const char *restrict, size_t); __standard__(POSIX.1-2008)
char *          strcat(char *restrict, const char *restrict);
char *          strchr(const char *, int);
int             strcmp(const char *, const char *);
char *          strcpy(char *restrict, const char *restrict);
size_t          strcspn(const char *, const char *);
char *          strdup(const char *);
char *          strerror(int);
int             strerror_r(int, char *, size_t);
size_t          strlen(const char *);
char *          strncat(char *restrict, const char *restrict, size_t);
int             strncmp(const char *, const char *, size_t);
char *          strncpy(char *restrict, const char *restrict, size_t);
char *          strndup(const char *, size_t);                    __standard__(POSIX.1-2008)
size_t          strnlen(const char *, size_t);                    __standard__(POSIX.1-2008)
char *          strpbrk(const char *, const char *);
char *          strrchr(const char *, int);
size_t          strspn(const char *, const char *);
char *          strstr(const char *, const char *);
char *          strtok(char *restrict, const char *restrict);
char *          strtok_r(char *restrict, const char *restrict, char **restrict);
/* to be done locale sensitive */
int             strcoll(const char *, const char *);
size_t          strxfrm(char *restrict, const char *restrict, size_t);
/* memcpy() replaced, memmove() safer and not that much difference in speed */
#define memcpy memmove

/* XXXX to be done... just prototypes and shell include */
#include "as_locale.h"		/* define the locale_t type */
char *          strsignal(int);
int             strcoll_l(const char *, const char *, locale_t);
char *          strerror_l(int, locale_t);
size_t          strxfrm_l(char *restrict, const char *restrict, size_t, locale_t);

#ifdef __cplusplus
}
#endif
#endif /* !_AS_STRING_H_ */
