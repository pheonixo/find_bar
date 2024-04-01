/*
 *  errno.h - system error numbers
 *  Pheonix
 *
 *  Created by Steven J Abner on Fri May 28 2004.
 *  Copyright (c) 2004, 2012, 2019. All rights reserved.
 *
 * If adding errors, need to update string/strerror.c also.
 */


#ifndef _AS_ERRNO_H_
#define _AS_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* PROBLEM: POSIX.1-2008 requires these as macros */
typedef enum {
  ENOERR = 0,
  EPERM,                /* Operation not permitted */
  ENOENT,               /* No such file or directory */
  ESRCH,                /* No such process */
  EINTR,                /* Interrupted function */
  EIO,                  /* I/O error */
  ENXIO,                /* No such device or address */
  E2BIG,                /* Argument list too long */
  ENOEXEC,              /* Executable file format error */
  EBADF,                /* Bad file descriptor */
  ECHILD,               /* No child processes */
  EDEADLK,              /* Resource deadlock would occur */
  ENOMEM,               /* Not enough space */
  EACCES,               /* Permission denied */
  EFAULT,               /* Bad address */
  ENOTBLK,              /* Block device required NOT POSIX */
  EBUSY,                /* Device or resource busy */
  EEXIST,               /* File exists */
  EXDEV,                /* Cross-device link */
  ENODEV,               /* No such device */
  ENOTDIR,              /* Not a directory */
  EISDIR,               /* Is a directory */
  EINVAL,               /* Invalid argument */
  ENFILE,               /* Too many files open in system */
  EMFILE,               /* Too many open files */
  ENOTTY,               /* Inappropriate I/O control operation */
  ETXTBSY,              /* Text file busy */
  EFBIG,                /* File too large */
  ENOSPC,               /* No space left on device */
  ESPIPE,               /* Invalid seek */
  EROFS,                /* Read-only file system */
  EMLINK,               /* Too many links */
  EPIPE,                /* Broken pipe */
    /* math software */
  EDOM,                 /* Mathematics argument out of domain of function */
  ERANGE,               /* Result too large */
    /* non-blocking and interrupt i/o */
  EAGAIN,			/* Resource unavailable, try again */
  EWOULDBLOCK = EAGAIN,	/* Operation would block */
  EINPROGRESS,          /* Operation in progress */
  EALREADY,             /* Connection already in progress */
    /* ipc/network software -- argument errors */
  ENOTSOCK,             /* Not a socket */
  EDESTADDRREQ,         /* Destination address required */
  EMSGSIZE,             /* Message too large */
  EPROTOTYPE,           /* Protocol wrong type for socket */
  ENOPROTOOPT,          /* Protocol not available */
  EPROTONOSUPPORT,      /* Protocol not supported */
  ESOCKTNOSUPPORT,      /* Socket type not supported NOT POSIX */
  ENOTSUP,              /* Not supported */
  EOPNOTSUPP = ENOTSUP,	/* Operation not supported on socket */
  EPFNOSUPPORT,         /* Protocol family not supported NOT POSIX */
  EAFNOSUPPORT,         /* Address family not supported */
  EADDRINUSE,           /* Address in use */
  EADDRNOTAVAIL,        /* Address not available */
    /* ipc/network software -- operational errors */
  ENETDOWN,             /* Network is down */
  ENETUNREACH,          /* Network unreachable */
  ENETRESET,            /* Connection aborted by network */
  ECONNABORTED,         /* Connection aborted */
  ECONNRESET,           /* Connection reset */
  ENOBUFS,              /* No buffer space available */
  EISCONN,              /* Socket is connected */
  ENOTCONN,             /* The socket is not connected */
  ESHUTDOWN,            /* Can't send after socket shutdown NOT POSIX */
  ETOOMANYREFS,         /* Too many references: can't splice NOT POSIX */
  ETIMEDOUT,            /* Connection timed out */
  ECONNREFUSED,         /* Connection refused */
  ELOOP,                /* Too many levels of symbolic links */
  ENAMETOOLONG,         /* Filename too long */
  EHOSTDOWN,            /* Host is down NOT POSIX */
  EHOSTUNREACH,         /* Host is unreachable */
  ENOTEMPTY,            /* Directory not empty */
  EPROCLIM,             /* Too many processes NOT POSIX */
  EUSERS,               /* Too many users NOT POSIX */
  EDQUOT,               /* Reserved - Disc quota exceeded */
    /* Network File System */
  ESTALE,               /* Reserved - Stale NFS file handle */
  EREMOTE,              /* Too many levels of remote in path NOT POSIX */
  EBADRPC,              /* RPC struct is bad NOT POSIX */
  ERPCMISMATCH,         /* RPC version wrong NOT POSIX */
  EPROGUNAVAIL,         /* RPC prog. not avail NOT POSIX */
  EPROGMISMATCH,        /* Program version wrong NOT POSIX */
  EPROCUNAVAIL,         /* Bad procedure for program NOT POSIX */
  ENOLCK,               /* No locks available */
  ENOSYS,               /* Function not supported */
  EFTYPE,               /* Inappropriate file type or format NOT POSIX */
  EAUTH,                /* Authentication error NOT POSIX */
  ENEEDAUTH,            /* Need authenticator NOT POSIX */
  EIDRM,                /* Identifier removed */
  ENOMSG,               /* No message of the desired type */
  EOVERFLOW,            /* Value too large to be stored in data type */
  ECANCELED,            /* Operation canceled */
  EILSEQ,               /* Illegal byte sequence */
  EBADMSG,              /* Bad message */
  EMULTIHOP,            /* Reserved */
  ENOLINK,              /* Reserved */
  EPROTO,               /* Protocol error */
  ENODATA,              /* No message is available on the STREAM head read queue */
  ENOSR,                /* No STREAM resources */
  ENOSTR,               /* Not a STREAM */
  ETIME,                /* Stream ioctl() timeout */
  ENOATTR,              /* Attribute not found NOT POSIX */
    /* POSIX.1-2008 additions */
  ENOTRECOVERABLE,      /* State not recoverable */
  EOWNERDEAD,           /* Previous owner died */
    /* END POSIX.1-2008 */
  EDOOFUS,              /* Programming error NOT POSIX */
  ELAST = EDOOFUS       /* Must be equal largest errno NOT POSIX */

} _error_msg_;

/* For a user dependent errors and errorstr */
/* If a usr passes strerror() an error > ELAST and provides
 * _usr_strerr_func, the strerror() will pass error to this hook,
 * else EINVAL is set and an error message of EDOOFUS is returned */
typedef char *(*_usr_strerr_hook)(int errnum);
extern _usr_strerr_hook _usr_strerr_func;


#define errno (*__error())
int *__error(void);

void	err_report(const char *, ...);
void	err_msg(const char *, ...);
void	err_smsg(const char *, ...);
void	err_semsg(int, const char *, ...);
void	err_ret(const char *, ...)        __attribute__((noreturn));
void	err_sret(const char *, ...)       __attribute__((noreturn));
void	err_seret(int, const char *, ...) __attribute__((noreturn));
void	err_dump(const char *, ...)       __attribute__((noreturn));

#ifdef __cplusplus
}
#endif
#endif /* !_AS_ERRNO_H_ */

