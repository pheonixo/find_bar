/*
 *  sys/wait.h - declarations for waiting
 *  Pheonix
 *
 *  Created by Steven Abner on Sat 6 Jul 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 */

#ifndef _AS_SYS_WAIT_H_
#define _AS_SYS_WAIT_H_

#include "sys/as_types.h"
#include "as_signal.h"

#ifndef __APPLE__

/* waitpid() */
#define WNOHANG     0x00000001  /* Do not hang if no status is available;
                                 * return immediately. */
#define WUNTRACED   0x00000002  /* Report status of stopped child process. */
#define WCONTINUED  0x00000008  /* Report status of continued child process. */
/* options argument to waitid() */
#define WSTOPPED    0x00000002  /* Status is returned for any child that has
                                 * stopped upon receipt of a signal. */
#define WEXITED     0x00000004  /* Wait for processes that have exited. */
#define WNOWAIT     0x01000000  /* Keep the process whose status is returned in
                                 * infop in a waitable state. */

/* analysis of process status values */
  /* Return exit status. */
#define WEXITSTATUS(status)   (((status) >> 8) & 0x0ff)
  /* Return signal number that caused process to stop. */
#define WSTOPSIG(status)      WEXITSTATUS(status)
  /* Return signal number that caused process to terminate. */
#define WTERMSIG(status)      ((status) & 0x07f)
  /* Return signal number that caused process to terminate. */
#define WCOREDUMP(status)     ((status) & 0x080)
  /* True if child has been continued. */
#define WIFCONTINUED(status)  ((status) == 0x0ffff)
  /* True if child exited normally. */
#define WIFEXITED(status)     (!WTERMSIG(status))
  /* True if child is currently stopped. */
#define WIFSTOPPED(status)    (((status) & 0x0ff) == 0x07f)
  /* True if child exited due to uncaught signal. */
#define WIFSIGNALED(status)   \
    ((((signed char)(((status) & 0x7f) + 1)) >> 1) > 0)

#else
 #define WNOHANG     0x00000001
 #define WUNTRACED   0x00000002
 #define WEXITED     0x00000004
 #define WSTOPPED    0x00000008
 #define WCONTINUED  0x00000010
 #define WNOWAIT     0x00000020
 #define WEXITSTATUS(x)  (((x) >> 8) & 0x0ff)
 #define WSTOPSIG(x)     ((x) >> 8)
 #define WTERMSIG(x)     ((x) & 0177)
 #define WCOREDUMP(x)    ((x) & 0200)
 #define WIFCONTINUED(x) ((((x) & 0177) == 0177) && (WSTOPSIG(x) == 0x13))
 #define WIFEXITED(x)    (((x) & 0177) == 0)
 #define WIFSTOPPED(x)   ((((x) & 0177) == 0177) && (WSTOPSIG(x) != 0x13))
 #define WIFSIGNALED(x)  ((((x) & 0177) != 0177) && (((x) & 0177) != 0))
#endif

typedef enum {
  P_ALL  = 0,
  P_PGID,
  P_PID
} idtype_t;

#ifdef __cplusplus
extern "C" {
#endif

pid_t               wait(int *);
int                 waitid(idtype_t, id_t, siginfo_t *, int);
pid_t               waitpid(pid_t, int *, int);

#ifdef __cplusplus
}
#endif
#endif /* !_AS_SYS_WAIT_H_ */
