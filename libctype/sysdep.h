/*
 *  sysdependent.h
 *  Pheonix
 *
 *  Created by Steven J Abner on 4 September 2008.
 *  Copyright 2008, 2012, 2020. All rights reserved.
 *
 */

#include "sys/as_types.h"

#ifndef _AS_SYSDEP_H_
#define _AS_SYSDEP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* other compilers translation (only other I've seen, IBM, MS) */
#ifndef __LDBL_MANT_DIG__
#define __LDBL_MANT_DIG__  LDBL_MANT_DIG
#endif

/* endian break down of uint64_t */
#ifdef __BIG_ENDIAN__
  typedef struct {  uint16_t w3; uint16_t w2; uint16_t w1; uint16_t w0;  } _ull_ws;
  typedef struct {  uint32_t hi; uint32_t lo;  } _ull_ls;
#elif defined (__LITTLE_ENDIAN__)
  typedef struct {  uint16_t w0; uint16_t w1; uint16_t w2; uint16_t w3;  } _ull_ws;
  typedef struct {  uint32_t lo; uint32_t hi;  } _ull_ls;
#else
  #error: undefined endianness
#endif
typedef union {  uint64_t ull;  _ull_ls  uls;  _ull_ws  uss;  } _ull_t;

/* interfaces based on semi-dependent of architecture */
int __isinff(float a);
int __isinfd(double a);
int __isinfld(long double a);
int __isnanf(float a);
int __isnand(double a);
int __isnanld(long double a);
#define	ISINF(a) \
  (  (sizeof((a)) == 4) ? __isinff(a) :	\
     (sizeof((a)) == 8) ? __isinfd(a) :	\
                          __isinfld(a)  )
#define	ISNAN(a) \
  (  (sizeof((a)) == 4) ? __isnanf(a) :	\
     (sizeof((a)) == 8) ? __isnand(a) :	\
                          __isnanld(a)  )

  /* count leading zeroes [0-31], to match ppc abilities
   * user responsible for not passing '0' on AMD
   * ppc/generic should return 32
   * return is number of leading zeroes
   * if bit 31 (AMD) set, returns 0.
   * intended use: the shift amount to position MSB in bit 31 position */
#ifdef __ppc__
  #define CNTLZW(x,y) \
    do __asm__("cntlzw %0,%1" : "=r" (x) : "r" (y)); while(0)
#elif defined (__i386__)
  #define CNTLZW(x,y) \
    do {  __asm__("bsr %1,%0" : "=r" (x) : "r" (y));  x = 31 - x;  } while(0)
#elif defined (__x86_64__)
  #define CNTLZW(x,y) \
    do {  __asm__("bsr %1,%0" : "=r" (x) : "r" (y));  x = 31 - x;  } while(0)
  #define CNTLZD(x,y) \
    do {  __asm__("bsr %1,%0" : "=r" (x) : "r" (y));  x = 63 - x;  } while(0)
#else
    /* about 50% slower, 3 instructions vs 31 instructions */
  #define CNTLZW(x,y) \
    do {  unsigned t = y;   \
      t |= t >> 1;          \
      t |= t >> 2;          \
      t |= t >> 4;          \
      t |= t >> 8;          \
      t |= t >> 16;         \
      t = t - ((t >> 1) & 0x55555555);                            \
      t = (t & 0x33333333) + ((t >> 2) & 0x33333333);             \
      x  = 32 - (((t + (t >> 4) & 0xF0F0F0F) * 0x1010101) >> 24); \
    } while (0);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !_AS_SYSDEP_H_ */
