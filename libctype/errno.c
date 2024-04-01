/*
 *  errno - declaration or definition for errno
 *  Pheonix
 *
 *  Created by Steven J Abner on Fri Feb 01 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 *  Dedicated to Erin JoLynn Abner.
 *  Special thanks to God!
 *
 */

/* since global to a process, include into pxUserSpace */

#include "sys/as_types.h"
#include "as_stdarg.h"
#include "as_stdio.h"
#include "as_stdlib.h"
#include "as_string.h"


void	err_report(const char *, ...);
void	err_msg(const char *, ...);
void	err_smsg(const char *, ...);
void	err_semsg(int, const char *, ...);
void	err_ret(const char *, ...)        __attribute__((noreturn));
void	err_sret(const char *, ...)       __attribute__((noreturn));
void	err_seret(int, const char *, ...) __attribute__((noreturn));
void	err_dump(const char *, ...)       __attribute__((noreturn));


#ifdef __linux
extern __thread int errno;
#else
extern int errno;
extern int *__error(void);
#endif

int *
__error(void) {  return &errno;  }

/*

#define _PTHREAD_SIG			0x54485244  // 'THRD'


extern pthread_t pthread_self(void);

int *__error(void) {

  pthread_t self = pthread_self();
  // If we're not a detached pthread, just return the global errno
  if ((self == (pthread_t)0) || (self->tdid != _PTHREAD_SIG)) {
    return &errno;
  }
  return &self->err_no;
}
*/

//#endif

/*
All 'msg' increment static counter __err_cnt
___________________________
err_msg()     put(msg)
err_smsg()    put(msg, strerror(errno))
err_semsg()   put(msg, strerror(err))
err_ret()     put(msg),                   exit(1)
err_sret()    put(msg, strerror(errno)),  exit(1)
err_seret()   put(msg, strerror(err)),    exit(1)
err_dump()    put(msg, strerror(errno)),  abort()
err_report()  put(passed or failed based on __err_cnt)  special err_msg()
___________________________
*/

#define MAXLINE 4096
static int __err_cnt = 0;

static void
__err_heart(int eflag, int error, const char *fmt, va_list ap) {

  char bufmsg[MAXLINE];

  vsnprintf(bufmsg, (size_t)(MAXLINE-1), fmt, ap);
  if (eflag) {
    size_t bufsz = strlen(bufmsg);
    char *nPtr = bufmsg + bufsz;
    snprintf(nPtr, MAXLINE - bufsz - 1, ": %s", strerror(error));
  }
  strcat(bufmsg, "\n");
  fflush(stdout);	/* in case stdout and stderr are the same */
  fputs(bufmsg, stderr);
  fflush(stderr);
}

void
err_report(const char *fmt, ...) {

  char bufmsg[MAXLINE];
  size_t bufsz;
  char *nPtr;
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(bufmsg, (size_t)(MAXLINE-1), fmt, ap);
  nPtr = bufmsg + (bufsz = strlen(bufmsg));
  snprintf(nPtr, MAXLINE - bufsz - 1, " %s",
           ((__err_cnt == 0) ? "PASSED" : "FAILED"));
  va_end(ap);
}

void
err_msg(const char *fmt, ...) {

  va_list ap;

  __err_cnt++;
  va_start(ap, fmt);
  __err_heart(0, 0, fmt, ap);
  va_end(ap);
}

void
err_smsg(const char *fmt, ...) {

  va_list ap;

  __err_cnt++;
  va_start(ap, fmt);
  __err_heart(1, errno, fmt, ap);
  va_end(ap);
}

void
err_semsg(int error, const char *fmt, ...) {

  va_list ap;

  __err_cnt++;
  va_start(ap, fmt);
  __err_heart(1, error, fmt, ap);
  va_end(ap);
}

void
err_ret(const char *fmt, ...) {

  va_list ap;

  va_start(ap, fmt);
  __err_heart(0, 0, fmt, ap);
  va_end(ap);
  exit(1);
}

void
err_sret(const char *fmt, ...) {

  va_list ap;

  va_start(ap, fmt);
  __err_heart(1, errno, fmt, ap);
  va_end(ap);
  exit(1);
}

void
err_seret(int error, const char *fmt, ...) {

  va_list ap;

  va_start(ap, fmt);
  __err_heart(1, error, fmt, ap);
  va_end(ap);
  exit(1);
}

void
err_dump(const char *fmt, ...) {

  va_list ap;

  va_start(ap, fmt);
  __err_heart(1, errno, fmt, ap);
  va_end(ap);
  abort();
  exit(1);
}

