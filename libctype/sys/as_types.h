/*
 *  sys/types.h - data types
 *  PheonixOS
 *
 *  Created by Steven J Abner on Fri May 28 2004.
 *  Copyright (c) 2004, 2012. All rights reserved.
 *
 */

#ifndef _AS_SYS_TYPES_H_
#define _AS_SYS_TYPES_H_

/* must define a data model */
#ifndef __LP64__
 #define __LP32__  1
#endif
/* found out 24 Jan 2019 from new computer build */
/* gcc does not define __LITTLE_ENDIAN__ or __BIG_ENDIAN__ */
#ifndef __LITTLE_ENDIAN__
 #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #define __LITTLE_ENDIAN__
 #else
  #define __BIG_ENDIAN__
 #endif
#endif

/* definitions used by most systems to prevent redefinition */
#define _INT8_T
#define _UINT8_T
#define _INT16_T
#define _UINT16_T
#define _INT32_T
#define _UINT32_T
#define _INT64_T
#define _UINT64_T
#define _INTMAX_T
#define _UINTMAX_T
#define _INTPTR_T
#define _UINTPTR_T
#define _SIZE_T
#define _SSIZE_T
/* Darwin specific? */
#define _MODE_T
#define _OFF_T
/* header file convience */
#define __standard__(x)

typedef signed char         int8_t;
typedef unsigned char       uint8_t;
typedef short               int16_t;
typedef unsigned short      uint16_t;
typedef int                 int32_t;
typedef unsigned int        uint32_t;
typedef long long           int64_t;
typedef unsigned long long  uint64_t;
#if __SIZEOF_INT128__ == 16
typedef __int128            int128_t;
typedef unsigned __int128   uint128_t;
#endif

typedef union {
#if __SIZEOF_INT128__ == 16
  uint128_t   uo;
#endif
#ifdef __BIG_ENDIAN__
  struct { uint64_t ull1, ull0; } ulls;
  struct { uint32_t ul3, ul2, ul1, ul0; } uls;
  struct { uint16_t us7, us6, us5, us4, us3, us2, us1, us0; } uss;
#else  /* __LITTLE_ENDIAN__ */
  struct { uint64_t ull0, ull1; } ulls;
  struct { uint32_t ul0, ul1, ul2, ul3; } uls;
  struct { uint16_t us0, us1, us2, us3, us4, us5, us6, us7; } uss;
#endif
} octle;

typedef signed char    _int8_t;
typedef   uint8_t     u_int8_t;
typedef   int16_t     _int16_t;
typedef  uint16_t    u_int16_t;
typedef   int32_t     _int32_t;
typedef  uint32_t    u_int32_t;
typedef   int64_t     _int64_t;
typedef  uint64_t    u_int64_t;

/* gcc appears to only defines _LP64 __LP64__ (2019) */
/* only models with 64bit ints */
#if defined (__ILP64__) || defined (__SILP64__)
typedef  int64_t      intmax_t;
typedef uint64_t     uintmax_t;
#elif defined (__LP32__)
typedef  int16_t      intmax_t;
typedef uint16_t     uintmax_t;
#else /* __LP64__ has 32bit int */
typedef  int32_t      intmax_t;
typedef uint32_t     uintmax_t;
#endif

/* these are all register size based */
#if defined (__LP32__) || defined (__ILP32__)
typedef  int32_t      intptr_t;
typedef uint32_t     uintptr_t;
#define __SIZEOF_REGISTER_T__ 4
#define __SIZEOF_SIZE_T__ 4
#else
typedef  int64_t      intptr_t;
typedef uint64_t     uintptr_t;
#define __SIZEOF_REGISTER_T__ 8
#define __SIZEOF_SIZE_T__ 8
#endif

typedef  intptr_t   register_t;
typedef  intptr_t    _intptr_t;
typedef uintptr_t   u_intptr_t;
typedef  intptr_t      ssize_t;   /* Used for a count of bytes or an error indication. */
typedef uintptr_t       size_t;   /* Used for sizes of objects. */
typedef  intptr_t    ptrdiff_t;   /* Used for subtracting two pointers. */
typedef uintptr_t      clock_t;   /* Used for system times in clock ticks or CLOCKS_PER_SEC . */
typedef  intptr_t       time_t;   /* Used for time in seconds. */

#define  SInt8    int8_t
#define  UInt8   uint8_t
#define SInt16   int16_t
#define UInt16  uint16_t
#define SInt32   int32_t
#define UInt32  uint32_t
#define SInt64   int64_t
#define UInt64  uint64_t

#define PTR_SIZE    (sizeof(intptr_t))
#define NULL        ((void *)0)
#define ALIGN32(p)  (((UInt32)(p) + (UInt32)3) & ~(UInt32)3)
#define ALIGN64(p)  (((UInt64)(p) + (UInt64)7) & ~(UInt64)7)
/*#define ALIGN_PTR   ALIGN32*/
/*#define ALIGN_INT   ALIGN32*/

        /* absolute sizes */
        /* file system */
typedef SInt64 blkcnt_t;    /* Used for file block counts. */
typedef SInt32 blksize_t;   /* Used for block sizes. */
typedef UInt64 fsblkcnt_t;  /* Used for file system block counts. */
typedef UInt64 fsfilcnt_t;  /* Used for file system file counts. */
typedef UInt32 ino_t;       /* Used for file serial numbers. */
typedef UInt32 mode_t;      /* Used for some file attributes. (can't be 16 bit) */
typedef UInt16 nlink_t;     /* Used for link counts. */
typedef SInt64 off_t;       /* Used for file sizes. */
typedef off_t  fpos_t;
typedef UInt32 wchar_t;     /* Used for representation of distinct wide-character codes. */

        /* time system */
typedef SInt32 clockid_t;   /* Used for clock ID type in the clock and timer functions. */
typedef SInt32 timer_t;     /* Used for timer ID returned by timer_create().  */
typedef SInt32 suseconds_t; /* Used for time in microseconds.  */
typedef UInt32 useconds_t;  /* Used for time in microseconds.  */

        /* identify system */
typedef SInt32 dev_t;   /* Used for device IDs. */
typedef UInt32 id_t;    /* Used as a general identifier */
typedef UInt32 gid_t;   /* Used for group IDs. */
typedef SInt32 pid_t;   /* Used for process IDs and process group IDs. */
typedef UInt32 uid_t;   /* Used for user IDs. */

        /* sizes */
typedef SInt32 key_t;   /* Used for interprocess communication. */

  /* this is to avoid circular and multi-file definitions */
  /* 32-bit won't have access, so different than what I've tried to do */
typedef unsigned long sigset_t;  /* Used to represent sets of signals. */
struct timespec {
 time_t   tv_sec;   /* Seconds. */
 long     tv_nsec;  /* Nanoseconds. */
};
#define _SIGSET_T
#define _STRUCT_TIMESPEC

        /* C langauge - string literals */
#ifdef __BIG_ENDIAN__
 #define cconst16_t(a,b)     (uint16_t)(((uint8_t)(a) << 8) | (uint8_t)(b))
 #define cconst32_t(a,b,c,d) (uint32_t)(((uint8_t)(a) << 24) | ((uint8_t)(b) << 16) | ((uint8_t)(c) << 8) | (uint8_t)(d))
#else
 #define cconst16_t(a,b)     (uint16_t)(((uint8_t)(b) << 8) | (uint8_t)(a))
 #define cconst32_t(a,b,c,d) (uint32_t)(((uint8_t)(d) << 24) | ((uint8_t)(c) << 16) | ((uint8_t)(b) << 8) | (uint8_t)(a))
#endif

        /* thread system, __APPLE__ listing, but looks wrong, all of it */
#define __P_MAGIC__          0xfadedead
#ifdef __LP64__
 #define __P_SIZE__           1168
 #define __P_ATTR_SIZE__      56
 #define __MUTEX_SIZE__       56  /* 68 + 8? !56*/
 #define __MUTEXATTR_SIZE__   8
 #define __COND_SIZE__        40
 #define __CONDATTR_SIZE__    8
 #define __RWLOCK_SIZE__      192
 #define __RWLOCKATTR_SIZE__  16
 #define __ONCE_SIZE__        8
#else
 #define __P_SIZE__           596
 #define __P_ATTR_SIZE__      36
 #define __MUTEX_SIZE__       40  /* 40 + 4? but have overwrite */
 #define __MUTEXATTR_SIZE__   8
 #define __COND_SIZE__        24
 #define __CONDATTR_SIZE__    4
 #define __RWLOCK_SIZE__      124
 #define __RWLOCKATTR_SIZE__  12
 #define __ONCE_SIZE__        4
#endif
#define __SPINLOCK_SIZE__    12
/* Used to identify a thread. */
struct __pthread_rec {
  void  (*__routine)(void *); /* Routine to call */
  void  *__arg;               /* Argument to pass */
  struct __pthread_rec *__next;
};
typedef struct _pthread_t {  long tdid; struct __pthread_rec *__cleanup_stack; char opaque[__P_SIZE__];  } *pthread_t;
/* Used to identify a thread attribute object.  */
/* temp removed "conflict"*/
typedef struct _pthread_attr_t {  long tdid; char opaque[__P_ATTR_SIZE__];  } *pthread_attr_t;
/* Used for mutexes. */
typedef struct _pthread_mutex_t {  long tdid; char opaque[__MUTEX_SIZE__];  } pthread_mutex_t;
/* Used to identify a mutex attribute object. */
typedef struct _pthread_mutexattr_t {  long tdid; char opaque[__MUTEXATTR_SIZE__];  } *pthread_mutexattr_t;
/* Used for condition variables. */
typedef struct _pthread_cond_t {  long tdid; char opaque[__COND_SIZE__];  } *pthread_cond_t;
/* Used to identify a condition attribute object. */
typedef struct _pthread_condattr_t {  long tdid; char opaque[__CONDATTR_SIZE__];  } *pthread_condattr_t;
/* Used for read-write locks. */
typedef struct _pthread_rwlock_t {  long tdid; char opaque[__RWLOCK_SIZE__];  } *pthread_rwlock_t;
/* Used for read-write lock attributes. */
typedef struct _pthread_rwlockattr_t {  long tdid; char opaque[__RWLOCKATTR_SIZE__];  } *pthread_rwlockattr_t;
/* Used for dynamic package initialization. */
typedef struct _pthread_once_t {  long tdid; char opaque[__ONCE_SIZE__];  } *pthread_once_t;
/* Used to identify a spin lock.  */
typedef struct _pthread_spinlock_t {  long tdid; char opaque[__SPINLOCK_SIZE__];  } *pthread_spinlock_t;
/* Used for thread-specific data keys. */
typedef UInt32 pthread_key_t;

/*
pthread_barrier_t        Used to identify a barrier.
pthread_barrierattr_t    Used to define a barrier attributes object.
trace_id_t               Used to identify a trace stream.
trace_attr_t             Used to identify a trace stream attributes object.
trace_event_id_t         Used to identify a trace event type.
trace_event_set_t        Used to identify a trace event type set.
*/

        /* compiler convience macro */
#ifndef __GNUC_PREREQ
 #if defined __GNUC__ && defined __GNUC_MINOR__
  #define __GNUC_PREREQ(major,minor) \
    (((__GNUC__ << 16) + __GNUC_MINOR__) >= (((major) << 16) + (minor)))
 #else
  #define __GNUC_PREREQ(major,minor) 0
 #endif
#endif  /* __GNUC_PREREQ */

#if !defined __cplusplus
 #if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
  #define __restrict	restrict
 #elif !__GNUC_PREREQ(2,95)
  #define __restrict	restrict
 #else
  #define __restrict
 #endif
#else
 #define __restrict
#endif

#endif /* !_AS_SYS_TYPES_H_ */
