/*
 *  signal.h - signals
 *  Pheonix
 *
 *  Created by Steven J Abner on Sun May 30 2004.
 *  Copyright (c) 2004, 2012, 2019. All rights reserved.
 *
 */

#ifndef _AS_SIGNAL_H_
#define _AS_SIGNAL_H_

#include "sys/as_types.h"  /* pthread_t, size_t, pid_t, and uid_t defines */

#define SIG_ERR     (void (*)(int))-1 /* Error return value from signal(). */
    /* request for type of signal handling */
#define SIG_DFL     (void (*)(int))0  /* Request for default signal handling. */
#define SIG_IGN     (void (*)(int))1  /* Request that signal be ignored. */
  /* 5 to avoid kernel numbering? */
#define SIG_HOLD    (void (*)(int))5  /* Request that signal be held. */

typedef void (*sighandler_t)(int);
typedef int sig_atomic_t;   /* Possibly volatile-qualified, used by an atomic entity. */

union sigval {
  int    sival_int;     /* Integer signal value. */
  void  *sival_ptr;     /* Pointer signal value. */
};

struct sigevent {
  union sigval  sigev_value;  /* Signal value. */
  int           sigev_signo;  /* Signal number. */
  int          sigev_notify;  /* Notification type. */
  void (*sigev_notify_function)(union sigval);  /* Notification function. */
  pthread_attr_t *sigev_notify_attributes;      /* Notification attributes. */
};

      /* values of sigev_notify */
enum {
  SIGEV_NONE = 0,   /* No asynchronous notification, deliver when event of interest occurs. */
  SIGEV_SIGNAL,     /* A queued signal. */
  SIGEV_THREAD = 3  /* A notification function is called to perform notification. */
};

/* changes here must be done to strsignal() too */
enum {
  SIGUNKN   =    0,    /* needed for new strsignal function ? */
  SIGABRT   =    6,    /* Process abort signal */
  SIGALRM   =   14,    /* Alarm clock */
  SIGBUS    =   10,    /* Access to an undefined portion of a memory object */
  SIGCHLD   =   20,    /* Child process terminated, stopped, or continued */
  SIGCONT   =   19,    /* Continue executing, if stopped */
  SIGEMT    =    7,    /* Emulation trap */
  SIGFPE    =    8,    /* Erroneous arithmetic operation */
  SIGHUP    =    1,    /* Hangup */
  SIGILL    =    4,    /* Illegal instruction */
  SIGINFO   =   29,    /* Information request */
  SIGINT    =    2,    /* Terminal interrupt signal */
  SIGKILL   =    9,    /* Kill (cannot be caught or ignored) */
  SIGPIPE   =   13,    /* Write on a pipe with no one to read it */
  SIGPOLL   =   23,    /* Pollable event (Obsolescent) */
  SIGPROF   =   27,    /* Profiling timer expired (Obsolescent) */
  SIGQUIT   =    3,    /* Terminal quit signal */
  SIGSEGV   =   11,    /* Invalid memory reference */
  SIGSTOP   =   17,    /* Stop executing (cannot be caught or ignored) */
  SIGSYS    =   12,    /* Bad system call */
  SIGTERM   =   15,    /* Termination signal */
  SIGTRAP   =    5,    /* Trace/breakpoint trap */
  SIGTSTP   =   18,    /* Terminal stop signal */
  SIGTTIN   =   21,    /* Background process attempting read */
  SIGTTOU   =   22,    /* Background process attempting write */
  SIGURG    =   16,    /* High bandwidth data is available at a socket */
  SIGUSR1   =   30,    /* User-defined signal 1 */
  SIGUSR2   =   31,    /* User-defined signal 2 */
  SIGVTALRM =   26,    /* Virtual timer expired */
  SIGWINCH  =   28,    /* Window size changed */
  SIGXCPU   =   24,    /* CPU time limit exceeded */
  SIGXFSZ   =   25,    /* File size limit exceeded */
#ifdef __linux
  SIGRTRV1  =   33,
  SIGRTRV2  =   34,
  SIG_LAST  =   SIGRTRV2    /* Last bit. */
#else
  SIG_LAST  =   SIGUSR2     /* Last bit. */
#endif
};

#define NSIG        (sizeof(sigset_t) << 3)
#define SIGRTMIN    ((int)(SIG_LAST + 1))
#define SIGRTMAX    ((int)NSIG)

typedef struct {
  int           si_signo;   /* Signal number. */
  int            si_code;   /* Signal code. */
  int           si_errno;   /* If non-zero, signal errno, as defined in <errno.h>. */
  pid_t           si_pid;   /* Sending process ID. */
  uid_t           si_uid;   /* Real user ID of sending process. */
  void *         si_addr;   /* Address of faulting instruction. */
  int          si_status;   /* Exit value or signal. */
  union sigval  si_value;   /* Signal value. */
  long           si_band;   /* Band event for SIGPOLL. (Obsolescent) */
} siginfo_t;

struct sigaction {
  union {       /* Pointer to a signal-catching function. */
    void (*__sa_handler)(int);
    void (*__sa_sigaction)(int, siginfo_t *, void *);
  } sigaction_u;
  sigset_t  sa_mask;    /* Blocking signal set. */
  int      sa_flags;    /* Special flags. */
};
#define sa_handler    sigaction_u.__sa_handler
#define sa_sigaction  sigaction_u.__sa_sigaction

  /* XXX not sure others use */
enum {
  SIG_BLOCK    = 1,   /* union of sets. */
  SIG_UNBLOCK,        /* intersection of sets. */
  SIG_SETMASK,        /* Resulting signal set. */

  SA_NOCLDSTOP = 1,   /* Do not generate SIGCHLD. */
  SA_NOCLDWAIT = 2,   /* Implementations not to create zombie processes on child death. */
  SA_SIGINFO   = 4,   /* Extra information to be passed on receipt to signal handlers. */
  SA_ONSTACK   = 0x08000000,  /* Signal delivery to occur on an alternate stack. */
  SA_RESTART   = 0x10000000,  /* Certain functions to become restartable. */
  SA_NODEFER   = 0x40000000,  /* Signal not to be automatically blocked. */
  SA_RESETHAND = 0x80000000,  /* Signal dispositions to be set to SIG_DFL. */

  SS_ONSTACK   = 1,   /* Process is executing on an alternate signal stack. */
  SS_DISABLE          /* Alternate signal stack is disabled. */
};

#define MINSIGSTKSZ      8192  /* Minimum stack size for a signal handler. */
#define SIGSTKSZ        32768  /* Default size in bytes for the alternate signal stack. */
/*#define MINSIGSTKSZ      2048   Minimum stack size for a signal handler. */
/*#define SIGSTKSZ         8192   Default size in bytes for the alternate signal stack. */
/*#define MINSIGSTKSZ     32768   Minimum stack size for a signal handler. */
/*#define SIGSTKSZ       131072   Default size in bytes for the alternate signal stack. */

typedef struct {
  void     *ss_sp;    /* Stack base or pointer. */
  int    ss_flags;    /* Flags. */
  size_t  ss_size;    /* Stack size. */
} stack_t;

struct sigstack {
  int  ss_onstack;    /* Non-zero when signal stack is in use. */
  void     *ss_sp;    /* Signal stack pointer. */
};

  /* si_code that are reasons why the signal was generated. */
typedef enum {
        /* SIGILL */
  ILL_ILLOPC = 1, /* Illegal opcode. */
  ILL_ILLOPN,     /* Illegal operand. */
  ILL_ILLADR,     /* Illegal addressing mode. */
  ILL_ILLTRP,     /* Illegal trap. */
  ILL_PRVOPC,     /* Privileged opcode. */
  ILL_PRVREG,     /* Privileged register. */
  ILL_COPROC,     /* Coprocessor error. */
  ILL_BADSTK,     /* Internal stack error. */
        /* SIGFPE */
  FPE_INTDIV = 1, /* Integer divide by zero. */
  FPE_INTOVF,     /* Integer overflow. */
  FPE_FLTDIV,     /* Floating-point divide by zero. */
  FPE_FLTOVF,     /* Floating-point overflow. */
  FPE_FLTUND,     /* Floating-point underflow. */
  FPE_FLTRES,     /* Floating-point inexact result. */
  FPE_FLTINV,     /* Invalid floating-point operation. */
  FPE_FLTSUB,     /* Subscript out of range. */
        /* SIGSEGV */
  SEGV_MAPERR = 1,  /* Address not mapped to object. */
  SEGV_ACCERR,      /* Invalid permissions for mapped object. */
        /* SIGBUS */
  BUS_ADRALN = 1, /* Invalid address alignment. */
  BUS_ADRERR,     /* Nonexistent physical address. */
  BUS_OBJERR,     /* Object-specific hardware error. */
        /* SIGTRAP */
  TRAP_BRKPT = 1, /* Process breakpoint. */
  TRAP_TRACE,     /* Process trace trap. */
        /* SIGCHLD */
  CLD_EXITED = 1, /* Child has exited. */
  CLD_KILLED,     /* Child has terminated abnormally and did not create a core file. */
  CLD_DUMPED,     /* Child has terminated abnormally and created a core file. */
  CLD_TRAPPED,    /* Traced child has trapped. */
  CLD_STOPPED,    /* Child has stopped. */
  CLD_CONTINUED,  /* Stopped child has continued. */
        /* SIGPOLL (Obsolescent) */
  POLL_IN = 1,    /* Data input available. */
  POLL_OUT,       /* Output buffers available. */
  POLL_MSG,       /* Input message available. */
  POLL_ERR,       /* I/O error. */
  POLL_PRI,       /* High priority input available. */
  POLL_HUP,       /* Device disconnected. */
        /* Any */
  SI_USER = 0x10001,  /* Signal sent by kill(). */
  SI_QUEUE,       /* Signal sent by the sigqueue(). */
  SI_TIMER,       /* Generated by expiration of a timer set by timer_settime(). */
  SI_ASYNCIO,     /* Generated by completion of an asynchronous I/O request. */
  SI_MESGQ        /* Generated by arrival of a message on an empty message queue. */

} si_code_tag;

/* obsolescent <ucontext.h> header moved here, POSIX.1-2008 */
/* should be processor specific ? or at least some configuration */
typedef struct mcontext *mcontext_t;
struct mcontext {
  uintptr_t  gpr[32];  /* general purpose registers */
  uintptr_t  fpr[64];  /* if 32-bit processor 32 x 64-bit == 64 x 32-bit */
  uintptr_t  vpr[128]; /* if 32-bit processor 32 x 128-bit == 128 x 32-bit */
  uintptr_t  mpr[8];   /* misc. regs status, count, condition, etc */
};

typedef struct ucontext *ucontext_t;
struct ucontext {
  ucontext_t *uc_link;     /* Resumed context when this context returns. */
  sigset_t    uc_sigmask;  /* Signals set that are blocked when this context is active. */
  stack_t     uc_stack;    /* The stack used by this context. */
  mcontext_t  uc_mcontext; /* Context to describe whole processor state. */
};

#ifdef __cplusplus
extern "C" {
#endif

int             kill(pid_t, int);
int             raise(int);
int             sigaction(int, const struct sigaction *restrict,
                                                  struct sigaction *restrict);
int             sigaddset(sigset_t *, int);
int             sigdelset(sigset_t *, int);
int             sigemptyset(sigset_t *);
int             sigfillset(sigset_t *);
int             sigismember(const sigset_t *, int);
int             sigpending(sigset_t *);
int             sigprocmask(int, const sigset_t *restrict, sigset_t *restrict);
int             sigqueue(pid_t, int, const union sigval);
int             sigsuspend(const sigset_t *);
int             sigwait(const sigset_t *restrict, int *restrict);
int             sigwaitinfo(const sigset_t *restrict, siginfo_t *restrict);

int             sigtimedwait(const sigset_t *restrict, siginfo_t *restrict,
                                              const struct timespec *restrict);
/* X/Open System Interfaces */
int             killpg(pid_t, int);
int             sigaltstack(const stack_t *restrict, stack_t *restrict);
/* POSIX.1-2008 */
void            psiginfo(const siginfo_t *, const char *);
void            psignal(int, const char *);
int             pthread_kill(pthread_t, int);
int             pthread_sigmask(int, const sigset_t *restrict,
                                                          sigset_t *restrict);
/* Obsolescent */
int             sighold(int);
int             sigignore(int);
int             siginterrupt(int, int);
sighandler_t    signal(int, sighandler_t);
int             sigpause(int);
int             sigrelse(int);
sighandler_t    sigset(int, sighandler_t);

#ifdef __cplusplus
}
#endif
#endif /* !_AS_SIGNAL_H_ */
