/*
 *  numbers.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Sun 8 Dec 2013.
 *  Copyright (c) 2013. All rights reserved.
 *
 */

/*  8 Jan 2014: set up, checked grabs correct symbol set.
 *              decided unknown future, alter all.
 *
 *  29 Jan 2014: renamed file, removed "lc_number" data for
 *               builds of locales. lc_number.c to hold that data,
 *               numbers.c to hold writting systems.
 *
 *  18 Mar 2014: convert all non-printing zeros to 0xE2 0x81 0x9F,
 *               medium mathmatical space. Need to add to ctype the
 *               added definition of being "Number, Decimal Digit [Nd]".
 *               This is done to avoid other spaces used as grouping and
 *               to have a code defined as mathmatical zero for zero-less
 *               systems, or "void". Easy adjustment should a universal or
 *               individual system decide on a zero to be used.
 */


/*
 *  Now that some design workings how been created, the typecast for
 * input/output looking more concrete.
 *  From:
 * csEnum (*numericWr)(csCnvrt_st *, lc_number_system_st *, encoderFn)
 *  To:
 * csEnum (*numericWr)(csCnvrt_st *, locale_t, uint32_t *)
 *  Thoughts about changing csCnvrt_st, either actual or to one for numerics,
 * are quashed. Force familularity with struct, along with perfect for
 * base input/output needs.
 *  Change to locale_t. This encompasses two different numeric,
 * decimal/financial systems. Also, the seperate en/decoderFn, other than
 * the callers locale_t, is not needed. Such be converting to callers, not
 * transforming from caller's display to someone elses.
 *  Flags became a must. This or change csCnvrt_st, was a must to determine
 * if caller was inputting a signed/unsigned hexadecimal number for conversion.
 * Originally, input of "lc_number_system_st *" deterined if common, native,
 * traditional used. Flag system will send a flag indicating the numbering
 * system to use: common, native, traditional, financial.
 * Flags must be a pointer. In order to return info, a pointer is used. Input
 * of signed/unsigned hexadecimal number, but need caller to know if a
 * minusSign or plusSign used. On negative, a minusSign is used, but caller
 * or "format" may require a plusSign, so caller should be informed.
 *
 */

/* Major clean up after fight with compiler, for lots of notations
 * see back up _bak02. Found compiler error in shifts, once worked, now
 * broken by Intel or gcc?
 */
/*
 *  Optimization of indicize() still needed. Left in fully functional state.
 * Possible size/speed can/should be done.
 *  Preliminary results of old system to this system on base convert,
 * 32bit compare only, was new system, on average, at least twice as fast.
 */
/*
 *  Indices will terminate with (uint8_t)-1. No longer
 * flush right routines.
 */

/*
 *  read in glyphs, convert to indices, run through deindex.
 *  deindex does the math, for up to 128bit numbers.
 *  deindex returns up to 4 vector-like 32bit values, lo to hi.
 */
/* where glyph reading stores indices */
/*  store into indicies left to right, highest left, then terminate 0xFF
 * looks like read grouping set 0 [h][m][l], set 1 [[][][]],[h][m][l]...0xFF */
/* reminder: indices terminates with 0xFF */
/* vector supplied by caller along with number of pieces (uint32_t) */

/* what if pack glyphSz and dZero into numerics, then positional
 * would call single functionPtr.
 * can I apply to Rd?
 */

#include "as_locale.h"    /* as_locale_numeric.h has rw prototypes */
#include "sysdep.h"       /* use of asm and ull_t (endianess) */

/* all Wr functions (input via csCnvrt_st):
 * input of uint32_t[] where uint32_t[0]
 * 128bit:
 * [u32_t << (0 * 32)][u32_t << (1 * 32)][u32_t << (2 * 32)][u32_t << (3 * 32)]
 * 96bit:
 * [u32_t << (0 * 32)][u32_t << (1 * 32)][u32_t << (2 * 32)]
 * 64bit:
 * [u32_t << (0 * 32)][u32_t << (1 * 32)]
 * 32bit:
 * [u32_t << (0 * 32)]
 *
 * a 32 bit can hold 8 full digits + limited range 9th, x10^8 math
 *
 *   Wr functions call indicize(). This converts the input via csCnvrt_st,
 * hexadecimal number, into indices for table look up.
 *   Wr functions would then look up in a table to recieve a code point.
 * Example: a indice of 5 would look in latnTable[5] to get unicode code
 * point of "0x00000035". The Wr function then endocerFn it to output via
 * csCnvrt_st wr info.
 *   Wr functions, via indicize() return, can also format/group/window dress
 * the indices into what caller desired via "flags" input decipher.
 *   Wr functions, via lc_number_system_st *ns, obtain info on dressing. It
 * obtains minusSign, plusSign, group, formats.
 *
 */

static uint64_t
_divlu2r(uint32_t, uint32_t) __attribute__ ((noinline));
static void
_uls_log10_buffer(uint32_t, uint8_t **) __attribute__ ((noinline));
static void
_uls_to_buffer(uint32_t, uint8_t **) __attribute__ ((noinline));
static void
_ulltobuffer(uint32_t, uint32_t, uint8_t *) __attribute__ ((noinline));


static void
_neglu4(uint32_t *uPtr, uint32_t retSz) {

  uPtr[0] = ~uPtr[0];
  if (retSz == 4) {
    uPtr[3] = ~uPtr[3];
    uPtr[2] = ~uPtr[2];
    uPtr[1] = ~uPtr[1];
    if ((uint32_t)(uPtr[0] + 1) < uPtr[0]) {
      if ((uint32_t)(uPtr[1] + 1) < uPtr[1]) {
        if ((uint32_t)(uPtr[2] + 1) < uPtr[2])  uPtr[3] += 1;
        uPtr[2] += 1;  }
      uPtr[1] += 1;  }
  }
  if (retSz == 3) {
    uPtr[2] = ~uPtr[2];
    uPtr[1] = ~uPtr[1];
    if ((uint32_t)(uPtr[0] + 1) < uPtr[0]) {
      if ((uint32_t)(uPtr[1] + 1) < uPtr[1])  uPtr[2] += 1;
      uPtr[1] += 1;  }
  }
  if (retSz == 2) {
    uPtr[1] = ~uPtr[1];
    if ((uint32_t)(uPtr[0] + 1) < uPtr[0])  uPtr[1] += 1;
  }
  uPtr[0] += 1;
}

static uint64_t
_divlu2r(uint32_t hi, uint32_t lo) {

  uint32_t shift;
  uint32_t mConst = 2305843009U;
  uint32_t tConst = 1000000000;
  _ull_t q;  q.uls.hi = hi;  q.uls.lo = lo;
  shift = 32;
  lo |= 1;
  if (hi) {
    CNTLZW(shift, hi);
    if (shift != 0) lo = (hi << (shift)) | (lo >> (32 - shift));  }
  lo = (uint32_t)((lo * (uint64_t)mConst) >> 32);
    /* Intel problem with negative shifts, PPC works. */
  lo = ((shift < 3) ? (lo << (3 - shift)) : (lo >> (shift - 3)));
  q.uls.hi = (uint32_t)(q.ull - ((uint64_t)lo * (uint32_t)tConst));
  if (q.uls.hi >= tConst) {  lo += 1;  q.uls.hi -= tConst;  }
  q.uls.lo = lo;
  return q.ull;
}

/* Version which reduces multiplies less than the standard way of
 * divide by 10, output modulo. This is different from intial
 * form. Intial form was output all 18 digits, remove top
 * zeroes, or place decimal point, etc where 1 out of 44
 * would use this. 128 bit => 1 out of 3 will use this.
 */
/*
 * This will force input of a handle rather than pointer
 * for "buf". Best way to comunicate position between calls.
 */
/*
 * 9 digit:  (1 + (18shifts 9add)) mul  +  (1 + 27) shift (32%)
 * vs           (9 + (18shifts 9add)) mul  +  (1 + 18) shift (68%)
 */
static void
_uls_log10_buffer(uint32_t u32, uint8_t **buf) {

  _ull_t a;
  int n;

  if (u32 < 10) {
    *(*buf)++ = (uint8_t)u32;
    return;
  }
  if (u32 < 100) {
    a.uls.hi = u32 * 26843546;
    *(*buf)++ = (a.uls.hi >> 28);
    *(*buf)++ = ((((a.uls.hi << 4) >> 4) * 10) >> 28);
    return;
  }
  if (u32 < 1000) {
    a.uls.hi = u32 * 2684355;
    *(*buf)++ = (a.uls.hi >> 28);
    a.uls.lo = (((a.uls.hi << 4) >> 4) * 10);
    *(*buf)++ = (a.uls.lo >> 28);
    *(*buf)++ = ((((a.uls.lo << 4) >> 4) * 10) >> 28);
    return;
  }
  if (u32 < 10000) {
    a.uls.hi = u32 * 268436;
    *(*buf)++ = (a.uls.hi >> 28);
    a.uls.lo = (((a.uls.hi << 4) >> 4) * 10);
    *(*buf)++ = (a.uls.lo >> 28);
    a.uls.hi = (((a.uls.lo << 4) >> 4) * 10);
    *(*buf)++ = (a.uls.hi >> 28);
    a.uls.lo = (((a.uls.hi << 4) >> 4) * 10);
    *(*buf)++ = (a.uls.lo >> 28);
    return;
  }
  if (u32 < 100000) {
    a.uls.lo = u32 << 15;
    a.ull = (uint64_t)a.uls.lo * (uint32_t)3518437209U;
    n = 5;
    goto L1;
  }
  if (u32 < 1000000) {
    a.uls.lo = u32 << 12;
    a.ull = (uint64_t)a.uls.lo * (uint32_t)2814749768U;
    n = 6;
    goto L1;
  }
  if (u32 < 10000000) {
    a.uls.lo = u32 << 8;
    a.ull = ((uint64_t)a.uls.lo * (uint32_t)1125899907) << 2;
    n = 7;
    goto L1;
  }
  if (u32 < 100000000) {
    a.uls.lo = u32 << 5;
    a.ull = (uint64_t)a.uls.lo * (uint32_t)3602879702U;
    n = 8;
    goto L1;
  }
  a.uls.lo = u32 << 2;
  a.ull = (uint64_t)a.uls.lo * (uint32_t)2882303762U;
  n = 9;
  goto L1;
  do {
    /* if I remember, jump was lag enough for multiply, then bdz */
    a.uls.hi = (((a.uls.hi << 4) >> 4) * 10) + (a.uls.lo >> 28);
    a.uls.lo <<= 4;
L1: *(*buf)++ = (a.uls.hi >> 28);
  } while ((--n));
  return;
}

static void
_uls_to_buffer(uint32_t u32, uint8_t **buf) {

  _ull_t a;
  int n;

  a.uls.lo = u32 << 2;
  a.ull = (uint64_t)a.uls.lo * (uint32_t)2882303762U;
  n = 9;
  goto L1;
  do {
    a.uls.hi = (((a.uls.hi << 4) >> 4) * 10) + (a.uls.lo >> 28);
    a.uls.lo <<= 4;
L1: *(*buf)++ = (a.uls.hi >> 28);
  } while ((--n));
  return;
}

static void
_ulltobuffer(uint32_t hi, uint32_t lo, uint8_t *buf) {

  _ull_t a, b;
  int n;
  --buf;
  /* in 4.28 fixed point integer, remainder/quotient, last/first */
  /* ((10^8 / 2^28) * 2^32) / 2^2 */
  a.ull = ((uint64_t)(hi << 2)) * (uint32_t)2882303762U;
  b.ull = ((uint64_t)(lo << 2)) * (uint32_t)2882303762U;
  n = 9;
  goto L1;
  do {
    b.uls.hi = (((b.uls.hi << 4) >> 4) * 10) + (b.uls.lo >> 28);
    a.uls.hi = (((a.uls.hi << 4) >> 4) * 10) + (a.uls.lo >> 28);
    b.uls.lo <<= 4;
    a.uls.lo <<= 4;
L1: *++buf = (a.uls.hi >> 28);
    buf[9] = (b.uls.hi >> 28);
  } while ((--n));
  return;
}

/* returning packed value, optimize return use on CPU type */
/* one case for flags include into csCnvrt_st */
static int32_t
indicize(csCnvrt_st *cnvrt, uint32_t *flags) {

  /* trying to use same terms as dbltostr(), possible adaptation? */
  /* putting all ducks into "reg", hi_split, lo_split, etc. */
  typedef union {
    _ull_t reg[5];
    uint32_t u32[10];
  } _sreg;

  _sreg sreg;

  /* put negate lower, removal by shift, if arrays must be larger regs */
  /* use as gpr, use 16bit(char[2]) packed on return */
  uint32_t pR;  /* packedReturn, pReg */

  uint8_t *sPtr = (uint8_t *)cnvrt->cs_wr;
  /* initialize data and place */
  pR = (uint32_t)cnvrt->cs_rdSz;

  sreg.u32[3] = (sreg.u32[2] = (sreg.u32[1] = 0));

  sreg.u32[0] = ((uint32_t *)cnvrt->cs_rd)[0];
  if (pR == 1)  goto J1;
  sreg.u32[1] = ((uint32_t *)cnvrt->cs_rd)[1];
  if (pR == 2)  goto J1;
  sreg.u32[2] = ((uint32_t *)cnvrt->cs_rd)[2];
  if (pR == 3)  goto J1;
  sreg.u32[3] = ((uint32_t *)cnvrt->cs_rd)[3];
    /* added 191115, FILE buffering undo */
  pR = 4;
J1:
  /* adjust regs for signed/unsigned value */
  if ((*flags & SIGNED_VALUE) != 0) {
    uint8_t  cF = (pR - 1) & 1;    /* 1 => 0, 2 => 1, 3 => 0, 4 => 1 */
    uint32_t tR = (pR - 1) >> 1;   /* 1 => 0, 2 => 0, 3 => 1, 4 => 1 */
    *flags &= ~SIGNED_VALUE;
    if (cF != 0)  cF = sreg.reg[tR].uls.hi >> 31;
    else {
      /* sign extend */
      if ((cF = sreg.reg[tR].uls.lo >> 31) != 0)
        sreg.reg[tR].uls.hi = (uint32_t)0xFFFFFFFF;
    }
    /* if high bit set, two's compliment data */
    if (cF != 0) {
      *flags |= SIGNED_VALUE;
      /* assume, for now, gcc handles 64 bits, not function call */
      sreg.reg[0].ull = ~sreg.reg[0].ull;
      sreg.reg[1].ull = ~sreg.reg[1].ull;
      /* proporgate info (overflow) */
      if (((uint32_t)(sreg.reg[0].uls.lo + 1) < sreg.reg[0].uls.lo)
          && ((uint32_t)(sreg.reg[0].uls.hi + 1) < sreg.reg[0].uls.hi))
        sreg.reg[1].ull += 1;
      sreg.reg[0].ull += 1;
    }
  }

  /*
   *  unsigned values:
   *                                 4 294967295 (pR == 1)
   *                      18 446744073 709551615 (pR == 2)
   *            79 228162514 264337593 543950335 (pR == 3)
   * 340 282366920 938463463 374607431 768211455 (pR == 4)
   */

  if (pR == 1) {
    /*         [0][4294967295] => [294967295][4] */
    sreg.reg[3].ull = _divlu2r(0, sreg.u32[0]);
    if (sreg.reg[3].uls.lo != 0) {
      /* bypass function call, single digit */
      *sPtr++ = (uint8_t)sreg.reg[3].uls.lo;
      _uls_to_buffer(sreg.reg[3].uls.hi, &sPtr);
      goto J10;
    }
    pR = 3;
    goto J11;
  }

  /* based on pR, index 0 or 1, do 64 or 128 bit values */
  /*         [0][4294967295] => [294967295][4] */
  sreg.reg[2].ull = _divlu2r(0, sreg.u32[(pR - 1)]);
  /* [294967295][4294967295] => [709551615][1266874889] */
  sreg.reg[3].ull = _divlu2r(sreg.reg[2].uls.hi, sreg.u32[(pR - 2)]);
  /*         [4][1266874889] => [446744073][18] */
  sreg.reg[4].ull = _divlu2r(sreg.reg[2].uls.lo, sreg.reg[3].uls.lo);

  if (sreg.reg[3].uls.hi >= 1000000000) {
    sreg.reg[1].ull = _divlu2r(0, sreg.reg[3].uls.hi);
    sreg.reg[3].uls.hi  = sreg.reg[1].uls.hi;
    sreg.reg[4].uls.hi += sreg.reg[1].uls.lo;
  }
  if (sreg.reg[4].uls.hi >= 1000000000) {
    sreg.reg[1].ull = _divlu2r(0, sreg.reg[4].uls.hi);
    sreg.reg[4].uls.hi  = sreg.reg[1].uls.hi;
    sreg.reg[4].uls.lo += sreg.reg[1].uls.lo;
  }

  if (pR == 2) {
    if (sreg.reg[4].uls.lo != 0) {
      _uls_log10_buffer(sreg.reg[4].uls.lo, &sPtr);
      _ulltobuffer(sreg.reg[4].uls.hi, sreg.reg[3].uls.hi, sPtr);
      sPtr += 18;
      goto J10;
    }
    if (sreg.reg[4].uls.hi != 0) {
      _uls_log10_buffer(sreg.reg[4].uls.hi, &sPtr);
      _uls_to_buffer(sreg.reg[3].uls.hi, &sPtr);
      goto J10;
    }
    pR = 3;
    goto J11;
  }

  /* do 2^96 */
  /* > 64 bits, 64 arranged as above */
  /* [709551615][4294967295] => [543950335][3047500985] */
  sreg.reg[2].ull = _divlu2r(sreg.reg[3].uls.hi, sreg.u32[(pR - 3)]);
  /* [446744073][3047500985] => [264337593][1918751186] */
  sreg.reg[3].ull = _divlu2r(sreg.reg[4].uls.hi, sreg.reg[2].uls.lo);
  /* [18][1918751186] => [228162514][79] */
  sreg.reg[4].ull = _divlu2r(sreg.reg[4].uls.lo, sreg.reg[3].uls.lo);

  if (sreg.reg[2].uls.hi >= 1000000000) {
    sreg.reg[1].ull = _divlu2r(0, sreg.reg[2].uls.hi);
    sreg.reg[2].uls.hi  = sreg.reg[1].uls.hi;
    sreg.reg[3].uls.hi += sreg.reg[1].uls.lo;
  }
  if (sreg.reg[3].uls.hi >= 1000000000) {
    sreg.reg[1].ull = _divlu2r(0, sreg.reg[3].uls.hi);
    sreg.reg[3].uls.hi  = sreg.reg[1].uls.hi;
    sreg.reg[4].uls.hi += sreg.reg[1].uls.lo;
  }

  if (pR == 3) {
    if (sreg.reg[4].uls.lo != 0) {
      _uls_log10_buffer(sreg.reg[4].uls.lo, &sPtr);
      _uls_to_buffer(sreg.reg[4].uls.hi, &sPtr);
      _ulltobuffer(sreg.reg[3].uls.hi, sreg.reg[2].uls.hi, sPtr);
      sPtr += 18;
      goto J10;
    }
    if (sreg.reg[4].uls.hi != 0) {
      _uls_log10_buffer(sreg.reg[4].uls.hi, &sPtr);
      _ulltobuffer(sreg.reg[3].uls.hi, sreg.reg[2].uls.hi, sPtr);
      sPtr += 18;
      goto J10;
    }
    if (sreg.reg[3].uls.hi != 0) {
      _uls_log10_buffer(sreg.reg[3].uls.hi, &sPtr);
      _uls_to_buffer(sreg.reg[2].uls.hi, &sPtr);
      goto J10;
    }
    pR = 2;
    goto J11;
  }

  /* do 2^128 (pR == 4) */
  /* [543950335][4294967295] => [768211455][2336248903] */
  sreg.reg[1].ull = _divlu2r(sreg.reg[2].uls.hi, sreg.u32[(pR - 4)]);
  /* [264337593][2336248903] => [374607431][1135321319] */
  sreg.reg[2].ull = _divlu2r(sreg.reg[3].uls.hi, sreg.reg[1].uls.lo);
  /* [228162514][1135321319] => [938463463][979950536] */
  sreg.reg[3].ull = _divlu2r(sreg.reg[4].uls.hi, sreg.reg[2].uls.lo);
  /* [79][979950536] => [282366920][340] */
  sreg.reg[4].ull = _divlu2r(sreg.reg[4].uls.lo, sreg.reg[3].uls.lo);

  if (sreg.reg[1].uls.hi >= 1000000000) {
    sreg.reg[0].ull = _divlu2r(0, sreg.reg[1].uls.hi);
    sreg.reg[1].uls.hi  = sreg.reg[0].uls.hi;
    sreg.reg[2].uls.hi += sreg.reg[0].uls.lo;
  }
  if (sreg.reg[2].uls.hi >= 1000000000) {
    sreg.reg[0].ull = _divlu2r(0, sreg.reg[2].uls.hi);
    sreg.reg[2].uls.hi  = sreg.reg[0].uls.hi;
    sreg.reg[3].uls.hi += sreg.reg[0].uls.lo;
  }
  if (sreg.reg[3].uls.hi >= 1000000000) {
    sreg.reg[0].ull = _divlu2r(0, sreg.reg[3].uls.hi);
    sreg.reg[3].uls.hi  = sreg.reg[0].uls.hi;
    sreg.reg[4].uls.hi += sreg.reg[0].uls.lo;
  }
  if (sreg.reg[4].uls.hi >= 1000000000) {
    sreg.reg[0].ull = _divlu2r(0, sreg.reg[4].uls.hi);
    sreg.reg[4].uls.hi  = sreg.reg[0].uls.hi;
    sreg.reg[4].uls.lo += sreg.reg[0].uls.lo;
  }

  if (sreg.reg[4].uls.lo != 0) {
    _uls_log10_buffer(sreg.reg[4].uls.lo, &sPtr);
    _ulltobuffer(sreg.reg[4].uls.hi, sreg.reg[3].uls.hi, sPtr);
    _ulltobuffer(sreg.reg[2].uls.hi, sreg.reg[1].uls.hi, (sPtr + 18));
    sPtr += 36;
    goto J10;
  }
  if (sreg.reg[4].uls.hi != 0) {
    _uls_log10_buffer(sreg.reg[4].uls.hi, &sPtr);
    _uls_to_buffer(sreg.reg[3].uls.hi, &sPtr);
    _ulltobuffer(sreg.reg[2].uls.hi, sreg.reg[1].uls.hi, sPtr);
    sPtr += 18;
    goto J10;
  }
  if (sreg.reg[3].uls.hi != 0) {
    _uls_log10_buffer(sreg.reg[3].uls.hi, &sPtr);
    _ulltobuffer(sreg.reg[2].uls.hi, sreg.reg[1].uls.hi, sPtr);
    sPtr += 18;
    goto J10;
  }
  if (sreg.reg[2].uls.hi != 0) {
    _uls_log10_buffer(sreg.reg[2].uls.hi, &sPtr);
    _uls_to_buffer(sreg.reg[1].uls.hi, &sPtr);
    goto J10;
  }
  pR = 1;
J11:
  _uls_log10_buffer(sreg.reg[pR].uls.hi, &sPtr);
J10:
  return (int32_t)((size_t)sPtr - (size_t)cnvrt->cs_wr);
}

static const uint32_t _p10[9] = {
  1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000
};

/* read input of x encoding, set as indices[], convert
 * indices to hexadecimal. */
/* XXX 39 digit max, because of postional type reading... 44 used
 * since start up <zero>s not addressed */
static csEnum
deindex(uint32_t v[4], uint8_t indices[44], uint32_t wrSz) {

  typedef union {
    _ull_t reg[5];
    uint32_t u32[10];
  } _sreg;

  _sreg sreg;
  uint8_t n, idx = 0, rdx = 0, rsdx = 0;

  sreg.u32[0] = 0;
    /* catch pre-<zero>s used on algorithmic positionals */
  do {
    n = indices[idx];
    if (n == 0) {  idx++; continue;  }
    break;
  } while (1);
  do {
    uint32_t temp;
    if ((n = indices[idx++]) == (uint8_t)0xFF)
      break;
    temp = sreg.u32[rdx];
    sreg.u32[rdx] = (temp << 3) + (temp << 1) + n;
    if ((++rsdx) == 9) {
      sreg.u32[(++rdx)] = 0;
      rsdx = 0;
    }
  } while (1);

  if (rdx == 0) {    /* less than 10^9 */
    v[0] = sreg.u32[0];
/* XXX 191101 change == to >= */
    if (wrSz >= 2)  v[1] = 0;
J3: if (wrSz >= 3)  v[2] = 0;
J4: if (wrSz >= 4)  v[3] = 0;
    if (idx != 1)
      return csRD;
    return csIE;  /* no digits found */
  }
  /* wrSz == 1 maxs here */
  if ((rdx == 1) || (wrSz == 1)) {
    sreg.reg[4].ull = (uint64_t)sreg.u32[0] * _p10[rsdx];
    sreg.reg[4].ull += sreg.u32[1];
    v[0] = sreg.reg[4].uls.lo;
    if (wrSz == 1) {
      if (sreg.reg[4].uls.hi != 0)  return csWR;
      return csRD;
    }
    v[1] = sreg.reg[4].uls.hi;
    goto J3;
  }
  if (rdx == 2) {
    sreg.reg[4].ull  = ((uint64_t)sreg.u32[1] * _p10[rsdx]) + sreg.u32[2];
    sreg.reg[3].ull  = (uint64_t)sreg.u32[0] * (uint32_t)1000000000;
    sreg.reg[2].ull  = (uint64_t)sreg.reg[3].uls.hi * _p10[rsdx];
    sreg.reg[1].ull  = (uint64_t)sreg.reg[3].uls.lo * _p10[rsdx];

    sreg.reg[4].ull += sreg.reg[1].ull;
      /* add hi to lo, make use of complier add in of overflow */
    sreg.reg[2].ull += sreg.reg[4].uls.hi;
      /* in 2.ull 4.lo */
    v[0] = sreg.reg[4].uls.lo;
    v[1] = sreg.reg[2].uls.lo;
      /* check for storage overwrite */
    if (wrSz == 2) {
      if (sreg.reg[2].uls.hi != 0)  return csWR;
      return csRD;
    }
    v[2] = sreg.reg[2].uls.hi;
    goto J4;
  }
  /* wrSz == 2 maxs here */
  if ((rdx == 3) || (wrSz == 2)) {
    /*
     *    [a * 10^18][b * 10^9][c][d]
     *    [a * 10^18 * p10] + [b * 10^9 * p10] + [c * p10 + d]
     *                        [b * 10^9 * p10]
     *             [a * 10^18 * p10] + [c * p10 + d]
     */
    sreg.reg[0].ull  = (((uint64_t)sreg.u32[0] * (uint32_t)1000000000)
                        + sreg.u32[1]);
    sreg.reg[1].ull  = ((uint64_t)sreg.u32[2] * _p10[rsdx]) + sreg.u32[3];
    sreg.reg[2].ull  = (uint64_t)sreg.reg[0].uls.lo * (uint32_t)1000000000;
    /* let gcc generate ADDC instuction */
    sreg.reg[1].ull += sreg.reg[2].uls.lo * _p10[rsdx];
    /* sreg[1].lo complete, use hi for [b * 10^9 * p10] addin */
    sreg.reg[1].uls.hi += (uint32_t)(((uint64_t)sreg.reg[2].uls.lo * _p10[rsdx]) >> 32);
    /* use up hi of [b * 10^9 * p10] */
    sreg.reg[2].ull  = (((uint64_t)sreg.reg[0].uls.hi * (uint32_t)1000000000)
                        + sreg.reg[2].uls.hi);
    sreg.reg[3].ull  = (uint64_t)sreg.reg[2].uls.lo * _p10[rsdx];
    sreg.reg[4].ull  = (uint64_t)sreg.reg[2].uls.hi * _p10[rsdx];
    /* use care in hi carry, generate ADDC instruction */
    sreg.reg[2].ull  = (uint64_t)sreg.reg[1].uls.hi + (uint64_t)sreg.reg[3].uls.lo;
/* XXX overflow */
    //sreg.reg[4].uls.lo += sreg.reg[3].uls.hi + sreg.reg[2].uls.hi;
    sreg.reg[4].ull += sreg.reg[3].uls.hi + sreg.reg[2].uls.hi;
    /* in 4, 2, 1 lo */
    v[0] = sreg.reg[1].uls.lo;
    v[1] = sreg.reg[2].uls.lo;
    if (wrSz == 2) {
      if ((sreg.reg[4].uls.lo | sreg.reg[4].uls.hi) != 0)  return csWR;
      return csRD;
    }
    v[2] = sreg.reg[4].uls.lo;
    if (wrSz == 3) {
      if (sreg.reg[4].uls.hi != 0)  return csWR;
      return csRD;
    }
    v[3] = sreg.reg[4].uls.hi;
    return csRD;
  }
  /* rdx == 4   [a*10^27][b*10^18][c*10^9][d][e] */

    /* overflow, create new cs code? */
  if (rsdx > 3)
    return csWR;
//340282366920938463,2208748511463374607,79228162000000000
//340282366920938463
  sreg.reg[0].ull  = (((uint64_t)sreg.u32[0] * (uint32_t)1000000000)
                      + sreg.u32[1]);
  sreg.reg[3].ull  = (((uint64_t)sreg.reg[0].uls.lo * (uint32_t)1000000000)
                      + sreg.u32[2]);
  sreg.reg[4].ull  = (uint64_t)sreg.reg[0].uls.hi * (uint32_t)1000000000;

  /* sreg.reg[0] open, put sreg.u32[4] then sreg.reg[2] and up gprs */
  /* sreg.u32[3] still untouched, (part of sreg.reg[1]) */
  sreg.u32[0] = sreg.u32[4];

  sreg.reg[2].ull  = sreg.reg[4].ull + sreg.reg[3].uls.hi;
  /* values: sreg.reg[3].uls.lo, sreg.reg[2] */
  sreg.reg[3].ull  = (((uint64_t)sreg.reg[3].uls.lo * (uint32_t)1000000000)
                      + sreg.u32[3]);
  /* used sreg.u32[3], sreg.reg[1] open... make [1][2][3] */
  sreg.reg[1].ull  = (uint64_t)sreg.reg[2].uls.lo * (uint32_t)1000000000;
  sreg.reg[2].ull  = (uint64_t)sreg.reg[2].uls.hi * (uint32_t)1000000000;
  /* adjust [1][2][3], generate ADDw/C instructions */
  sreg.reg[1].ull += sreg.reg[3].uls.hi;
  sreg.reg[2].ull += sreg.reg[1].uls.hi;
  /* arrange for understanding */
  sreg.reg[1].uls.hi = sreg.reg[1].uls.lo;
  sreg.reg[1].uls.lo = sreg.reg[3].uls.lo;
  /* terms now: sreg.reg[2], sreg.reg[1]... hi, lo */
  sreg.reg[0].ull  = (uint64_t)sreg.reg[1].uls.lo * _p10[rsdx] + sreg.u32[0];
  sreg.reg[1].ull  = (uint64_t)sreg.reg[1].uls.hi * _p10[rsdx] + sreg.reg[0].uls.hi;
  sreg.reg[3].ull  = (uint64_t)sreg.reg[2].uls.hi * _p10[rsdx];
  sreg.reg[2].ull  = (uint64_t)sreg.reg[2].uls.lo * _p10[rsdx] + sreg.reg[1].uls.hi;
  sreg.reg[3].ull += sreg.reg[2].uls.hi;
  v[0] = sreg.reg[0].uls.lo;
  v[1] = sreg.reg[1].uls.lo;
  v[2] = sreg.reg[2].uls.lo;
  if (wrSz == 3) {
    if (sreg.reg[3].uls.lo != 0)  return csWR;
    return csRD;
  }
  if (((v[3] = sreg.reg[3].uls.lo) != 0) && (sreg.reg[3].uls.hi == 0))
    return csRD;
  return csWR;
}

#define BOUNDS_CHECK(x)    \
do { \
_eval = (x); \
if (_eval > wrSz)  return csWR;  \
wrSz -= _eval;  \
} while (0)

#define HAS_HCNT  0x2000  /* special flag for %C,%F,%G,%Y (Hard-CouNT) */

/* XXX altered and moved to "as_locale_numeric.h" */
/*
typedef enum {
  NS_ALLOCSZ                 =   0,
  NS_DECIMALFORMAT           =   1,
  NS_PERCENTFORMAT           =   2,
  NS_SCIENTIFICFORMAT        =   3,
  NS_ACOUNTINGFORMAT         =   4,
  NS_DECIMAL                 =   5,
  NS_EXPONENTIAL             =   6,
  NS_GROUP                   =   7,
  NS_INFINITY                =   8,
  NS_MINUSSIGN               =   9,
  NS_NAN                     =  10,
  NS_PERMILLE                =  11,
  NS_PERCENTSIGN             =  12,
  NS_PLUSSIGN                =  13,
  NS_SUPERSCRIPTINGEXPONENT  =  14,
  NS_LAST                    =  14
} nsEnum;
*/
static __inline__ uint8_t *
ns_numberinfo(nsEnum item, uint8_t *ndata) {

  uint16_t offset = ((uint16_t *)ndata)[item];
#ifdef __LITTLE_ENDIAN__
  offset = (offset >> 8) | (offset << 8);
#endif
  return (ndata + offset);
}

const rwpair_st _enum_to_rwFns[ ] = {
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  arabRd, arabWr  },
  {  arabextRd, arabextWr  },
  {  armnRd, armnWr  },
  {  armnRd, armnWr  },
  {  latnRd, latnWr  },
  {  bengRd, bengWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  cakmRd, cakmWr  },
  {  latnRd, latnWr  },
  {  cyrlRd, cyrlWr  },
  {  devaRd, devaWr  },
  {  ethiRd, ethiWr  },
  {  latnRd, latnWr  },
  {  georRd, georWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  grekRd, grekWr  },
  {  grekRd, grekWr  },
  {  gujrRd, gujrWr  },
  {  guruRd, guruWr  },
  {  hanidecRd, hanidecWr  },
  {  hanidecRd, hanidecWr  },
  {  hansRd, hansWr  },
  {  hansRd, hansWr  },
  {  hantRd, hantWr  },
  {  hantRd, hantWr  },
  {  hebrRd, hebrWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  javaRd, javaWr  },
  {  jpanRd, jpanWr  },
  {  jpanRd, jpanWr  },
  {  jpanRd, jpanWr  },
  {  latnRd, latnWr  },
  {  khmrRd, khmrWr  },
  {  kndaRd, kndaWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  laooRd, laooWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  mlymRd, mlymWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  mymrRd, mymrWr  },
  {  mymrRd, mymrWr  },
  {  mymrRd, mymrWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  oryaRd, oryaWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  },
  {  tamlRd, tamlWr  },
  {  tamldecRd, tamldecWr  },
  {  teluRd, teluWr  },
  {  thaiRd, thaiWr  },
  {  tibtRd, tibtWr  },
  {  latnRd, latnWr  },
  {  vaiiRd, vaiiWr  },
  {  latnRd, latnWr  },
  {  latnRd, latnWr  }
};

const rwpair_st *
rwNRSpairng(NRSenum nrs) {
  if (nrs >= RW_LAST)
    return &_enum_to_rwFns[RW_DEFAULT];
  return &_enum_to_rwFns[nrs];
}

#pragma mark ### numericWr
#pragma mark #### positional

static csEnum
gposWr(csCnvrt_st *cnvrt, uint8_t *ns,
       uint32_t *rflags, const uint8_t *dZero) {

  uint8_t indices[44];
  int32_t _eval;
  int32_t  d, w, s, df; /* digits, width, sign, digit field */
  uint32_t flags;

  int32_t  wrSz = (int32_t)cnvrt->cs_wrSz;
  uint8_t *sPtr = (uint8_t *)cnvrt->cs_wr;

  /* number of indices (digit glyphs) */
  cnvrt->cs_wr = (void *)indices;
  d = indicize(cnvrt, rflags);

  /* convience, MACROs use "flags" */
  flags = *rflags;

  /* any missing digits? */
  df = (w = DIGIT_FIELD) - d;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display? */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((w + 1) <= d)  s = 1;

  /* padding? */
  if ((w = TOTAL_FIELD - (d + df + s)) < 0)  w = 0;

  /* left field padding (pre-sign) */
  if (w && (flags & PAD_SPACE)) {
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    do {
      uint8_t smblCh = *smblPtr;
      smblPtr++;
      if (smblCh == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* set up for sz 4 not addressed, nor used at this time (only 2/3) */
  /* if <spaces> to be used, then w should be 0; else <zero> used for both */
  if ((flags & FILL_SPACE) != 0) {
    if (df) {
      BOUNDS_CHECK(df);
      do *sPtr++ = ' '; while ((--df));
    }
  } else {
    if ((w += df) != 0) {
      if (dZero[0] == 2) {
        BOUNDS_CHECK((w << 1));
        do {
          *(uint16_t*)sPtr = *(uint16_t*)&dZero[1];
          sPtr += 2;
        } while ((--w));
      } else if (dZero[0] == 3) {
        BOUNDS_CHECK(((w << 1) + w));
        do {
          *(uint16_t*)sPtr = *(uint16_t*)&dZero[1];
          sPtr[2] = dZero[3];
          sPtr += 3;
        } while ((--w));
      } else {
          /* (dZero[0] == 4) */
        BOUNDS_CHECK((w << 2));
        do {
          *(uint32_t*)sPtr = *(uint32_t*)&dZero[1];
          sPtr += 4;
        } while ((--w));
      }
    }
  }

  /* w should == 0 from above */
  /* convert indices to UTF-8 display */
  /* note: a grouping funtion needed */
  if (dZero[0] == 2) {
    BOUNDS_CHECK((d << 1));
    do {
      sPtr[0] =  dZero[1];
      sPtr[1] = (dZero[2] + indices[w++]);
      sPtr += 2;
    } while ((--d));
  } else if (dZero[0] == 3) {
    BOUNDS_CHECK(((d << 1) + d));
    do {
      *(uint16_t*)sPtr = *(uint16_t*)&dZero[1];
      sPtr[2] = (dZero[3] + indices[w++]);
      sPtr += 3;
    } while ((--d));
  } else {
      /* (dZero[0] == 4) */
    BOUNDS_CHECK((d << 2));
    do {
      *(uint32_t*)sPtr = *(uint32_t*)&dZero[1];
      sPtr[3] += indices[w++];
      sPtr += 4;
    } while ((--d));
  }

  /* we terminated because all was read */
  /* return next write position, termination reason */
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

/* ٠١٢٣٤٥٦٧٨٩  Arabic
 * const uint16_t udigit[10] = {
 *   0x0660, 0x0661, 0x0662, 0x0663, 0x0664,
 *   0x0665, 0x0666, 0x0667, 0x0668, 0x0669
 * };
 * add 0x660 to indices, 0xD9,0xA0-A9
 */
csEnum
arabWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x02\xD9\xA0");
}

/* ۰۱۲۳۴۵۶۷۸۹  Persian
 * const uint16_t udigit[10] = {
 *   0x06F0, 0x06F1, 0x06F2, 0x06F3, 0x06F4,
 *   0x06F5, 0x06F6, 0x06F7, 0x06F8, 0x06F9
 * };
 * add 0x6F0 to indices, 0xDB,0xB0-B9
 */
csEnum
arabextWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x02\xDB\xB0");
}

/* ০১২৩৪৫৬৭৮৯  Bengali
 * const uint16_t udigit[10] = {
 *   0x09E6, 0x09E7, 0x09E8, 0x09E9, 0x09EA,
 *   0x09EB, 0x09EC, 0x09ED, 0x09EE, 0x09EF
 * };
 * add 0x9E6 to indices, 0xE0,0xA7,0xA6-AF
 */
csEnum
bengWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA7\xA6");
}

/* 𑄶𑄷𑄸𑄹𑄺𑄻𑄻𑄽𑄾𑄿  Chakma
 * const uint16_t udigit[10] = {
 *   0x2B80, 0x2B81, 0x2B82, 0x2B83, 0x2B84,
 *   0x2B85, 0x2B86, 0x2B87, 0x2B88, 0x2B89
 * };
 * add 0x2B80 to indices, 0xF0,0x91,0x84,0xB6-BF
 */
csEnum
cakmWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x04\xF0\x91\x84\xB6");
}

/* ०१२३४५६७८९  Devanagari
 * const uint16_t udigit[10] = {
 *   0x0966, 0x0967, 0x0968, 0x0969, 0x096A,
 *   0x096B, 0x096C, 0x096D, 0x096E, 0x096F
 * };
 * add 0x0966 to indices, 0xE0,0xA5,0xA6-AF
 */
csEnum
devaWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA5\xA6");
}

/* ૦૧૨૩૪૫૬૭૮૯  Gujarati
 * const uint16_t udigit[10] = {
 *   0x0AE6, 0x0AE7, 0x0AE8, 0x0AE9, 0x0AEA,
 *   0x0AEB, 0x0AEC, 0x0AED, 0x0AEE, 0x0AEF
 * };
 * add 0x0AE6 to indices, 0xE0,0xAB,0xA6-AF
 */
csEnum
gujrWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAB\xA6");
}

/* ੦੧੨੩੪੫੬੭੮੯  Gurmukhī
 * const uint16_t udigit[10] = {
 *   0x0A66, 0x0A67, 0x0A68, 0x0A69, 0x0A6A,
 *   0x0A6B, 0x0A6C, 0x0A6D, 0x0A6E, 0x0A6F
 * };
 * add 0x0A66 to indices, 0xE0,0xA9,0xA6-AF
 */
csEnum
guruWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA9\xA6");
}

/* ꧐꧑꧒꧓꧔꧕꧖꧗꧘꧙  Javanese
 * const uint16_t udigit[10] = {
 *   0xA9D0, 0xA9D1, 0xA9D2, 0xA9D3, 0xA9D4,
 *   0xA9D5, 0xA9D6, 0xA9D7, 0xA9D8, 0xA9D9
 * };
 * add 0xA9D0 to indices, 0xEA,0xA7,0x90-99
 */
csEnum
javaWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xEA\xA7\x90");
}

/* ០១២៣៤៥៦៧៨៩  Khmer
 * const uint16_t udigit[10] = {
 *   0x17E0, 0x17E1, 0x17E2, 0x17E3, 0x17E4,
 *   0x17E5, 0x17E6, 0x17E7, 0x17E8, 0x17E9
 * };
 * add 0x17E0 to indices, 0xE1,0x9F,0xA0-A9
 */
csEnum
khmrWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE1\x9F\xA0");
}

/* ೦೧೨೩೪೫೬೭೮೯  Kannada
 * const uint16_t udigit[10] = {
 *   0x0CE6, 0x0CE7, 0x0CE8, 0x0CE9, 0x0CEA,
 *   0x0CEB, 0x0CEC, 0x0CED, 0x0CEE, 0x0CEF
 * };
 * add 0x0CE6 to indices, 0xE0,0xB3,0xA6-AF
 */
csEnum
kndaWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB3\xA6");
}

/* ໐໑໒໓໔໕໖໗໘໙  Lao
 * const uint16_t udigit[10] = {
 *   0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4,
 *   0x0ED5, 0x0ED6, 0x0ED7, 0x0ED8, 0x0ED9
 * };
 * add 0x0ED0 to indices, 0xE0,0xBB,0x90-99
 */
csEnum
laooWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xBB\x90");
}

/* ൦൧൨൩൪൫൬൭൮൯   Malayalam
 * const uint16_t udigit[10] = {
 *   0x0D66, 0x0D67, 0x0D68, 0x0D69, 0x0D6A,
 *   0x0D6B, 0x0D6C, 0x0D6D, 0x0D6E, 0x0D6F
 * };
 * add 0x0D66 to indices, 0xE0,0xB5,0xA6-AF
 */
csEnum
mlymWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB5\xA6");
}

/* ၀၁၂၃၄၅၆၇၈၉  Myanmar (Burmese)
 * const uint16_t udigit[10] = {
 *   0x1040, 0x1041, 0x1042, 0x1043, 0x1044,
 *   0x1045, 0x1046, 0x1047, 0x1048, 0x1049
 * };
 * add 0x1040 to indices, 0xE1,0x81,0x80-89
 */
csEnum
mymrWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE1\x81\x80");
}

/* ୦୧୨୩୪୫୬୭୮୯  Oriya
 * const uint16_t udigit[10] = {
 *   0x0B66, 0x0B67, 0x0B68, 0x0B69, 0x0B6A,
 *   0x0B6B, 0x0B6C, 0x0B6D, 0x0B6E, 0x0B6F
 * };
 * add 0x0B66 to indices, 0xE0,0xAD,0xA6-AF
 */
csEnum
oryaWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAD\xA6");
}

/* ௦௧௨௩௪௫௬௭௮௯  Tamil
 * const uint16_t udigit[10] = {
 *   0x0BE6, 0x0BE7, 0x0BE8, 0x0BE9, 0x0BEA,
 *   0x0BEB, 0x0BEC, 0x0BED, 0x0BEE, 0x0BEF
 * };
 * add 0x0BE6 to indices, 0xE0,0xAF,0xA6-AF
 */
csEnum
tamldecWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAF\xA6");
}

/* ౦౧౨౩౪౫౬౭౮౯  Telugu
 * const uint16_t udigit[10] = {
 *   0x0C66, 0x0C67, 0x0C68, 0x0C69, 0x0C6A,
 *   0x0C6B, 0x0C6C, 0x0C6D, 0x0C6E, 0x0C6F
 * };
 * add 0x0C66 to indices, 0xE0,0xB1,0xA6-AF
 */
csEnum
teluWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB1\xA6");
}

/* ๐๑๒๓๔๕๖๗๘๙  Thai
 * const uint16_t udigit[10] = {
 *   0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54,
 *   0x0E55, 0x0E56, 0x0E57, 0x0E58, 0x0E59
 * };
 * add 0x0E50 to indices, 0xE0,0xB9,0x90-99
 */
csEnum
thaiWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB9\x90");
}

/* ༠༡༢༣༤༥༦༧༨༩  Tibetan
 * const uint16_t udigit[10] = {
 *   0x0F20, 0x0F21, 0x0F22, 0x0F23, 0x0F24,
 *   0x0F25, 0x0F26, 0x0F27, 0x0F28, 0x0F29
 * };
 * add 0x0F20 to indices, 0xE0,0xBC,0xA0-A9
 */
csEnum
tibtWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xBC\xA0");
}

/* ꘠꘡꘢꘣꘤꘥꘦꘧꘨꘩  Vai
 * const uint16_t udigit[10] = {
 *   0xA620, 0xA621, 0xA622, 0xA623, 0xA624,
 *   0xA625, 0xA626, 0xA627, 0xA628, 0xA629
 * };
 * add 0xA620 to indices, 0xEA,0x98,0xA0-A9
 */
csEnum
vaiiWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposWr(cnvrt, ns, rflags, (const uint8_t *)"\x03\xEA\x98\xA0");
}

#pragma mark #### algorithmic

csEnum
armnWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Armenian
   * ԱԲԳԴԵԶԷԸԹ, 1-9           0xD4,0xB1-B9
   * ԺԻԼԽԾԿՀՁՂ, 10 - 90       0xD4,0xBA-BF 0xD5,0x80-82
   * ՃՄՅՆՇՈՉՊՋ, 100 - 900     0xD5,0x83-8B
   * ՌՍՎՏՐՑՒՓՔ, 1000 - 9000   0xD5,0x8C-94
   * Ա̅ = 10000  combining overline == 0xCC 0x85
   * unknown when >= 100000000, assume prefix multiples of 9999.
   */

  const uint8_t  u8[4][18] = {
    { 0xD4,0xB1, 0xD4,0xB2, 0xD4,0xB3, 0xD4,0xB4, 0xD4,0xB5,
      0xD4,0xB6, 0xD4,0xB7, 0xD4,0xB8, 0xD4,0xB9  },
    { 0xD4,0xBA, 0xD4,0xBB, 0xD4,0xBC, 0xD4,0xBD, 0xD4,0xBE,
      0xD4,0xBF, 0xD5,0x80, 0xD5,0x81, 0xD5,0x82  },
    { 0xD5,0x83, 0xD5,0x84, 0xD5,0x85, 0xD5,0x86, 0xD5,0x87,
      0xD5,0x88, 0xD5,0x89, 0xD5,0x8A, 0xD5,0x8B  },
    { 0xD5,0x8C, 0xD5,0x8D, 0xD5,0x8E, 0xD5,0x8F, 0xD5,0x90,
      0xD5,0x91, 0xD5,0x92, 0xD5,0x93, 0xD5,0x94  }
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t wrSz;
  uint8_t *sPtr, idx;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* because of nurmerals like 50 being 1 glyph "Ծ",
   * must count d digits differently for padding.
   */
  df = 0;
  if (indices[0] == 0)  df++;
  else {
    uint8_t bzero = 4;
    w = 0;
    do {
      if (indices[w++] != 0) df++;
      else if ((w > bzero) && (*(uint32_t*)&indices[(w - 4)] == 0))
        df++, bzero += 4;
    } while (w < d);
  }
  /* use digits found before converting to missing digit field */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }
  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }
  /* complete digit field, delete missing, use space, use zero */
  /* problem with <space>, strptime must eat up white space, voiding attempt as
   * field count */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* <zero> == <mathematical space> */
  if (indices[0] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
  } else {
    /* convert indices to UTF-8 display */
    /* must do >= 10000 first, has combining overline */
    w = 0;
    if (d > 4) {
      /* obtain first grouping of multiple 4 */
      if ((s = d & 3) == 0)  s = 4;
      do {
        d -= s;
        do {
          if ((s == 4) && (*(uint32_t*)&indices[w] == 0)) {
            s -= 4, w += 4;
            BOUNDS_CHECK(5);
            *sPtr++ = 0xE2;
            *(uint32_t*)sPtr = cconst32_t('\x81','\x9F','\xCC','\x85');
            sPtr += 4;
            break;
          }
          --s;
          if ((idx = indices[w++]) != 0) {
            const uint8_t *uPtr = &u8[s][((idx - 1) << 1)];
            BOUNDS_CHECK(4);
            *sPtr++ = (uint8_t)*uPtr++;  *sPtr++ = (uint8_t)*uPtr;
            *sPtr++ = (uint8_t)0xCC;     *sPtr++ = (uint8_t)0x85;
          }
        } while (s);
        s = 4;
      } while (d > 4);
    }
    /* only 1 group of 4 or less */
    do {
      --d;
      if ((idx = indices[w++]) != 0) {
        const uint8_t *uPtr = &u8[d][((idx - 1) << 1)];
        BOUNDS_CHECK(2);
        *sPtr++ = (uint8_t)*uPtr++;  *sPtr++ = (uint8_t)*uPtr;
      }
    } while (d);
  }
  /* we terminated because all was read */
  /* return next write position, termination reason */
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
cyrlWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* special note: original design added <space to combing mark. This was
   * believed needed as mac only displayed correctly with. After more research
   * and running on ryzen, this is mac problem, not actual coding problem.
   * Altering for correct coding, 19-06-12
   */
  /*
   * Additive system  Church Slavic (Cryrillic)
   *  авгдєѕзиѳ,  1 - 9      0xD0 B0,B2,B3,B4 0xD1 94,95 0xD0 B7,B8 0xD1 B3
   *  АВГДЄЅЗИѲ
   *  іклмнѯопч,  10 - 90    0xD1 96 0xD0 BA,BB,BC,BD 0xD1 AF 0xD0 BE,BF 0xD1 87
   *  ІКЛМНѮѺПЧ  optional 90?(Ҁ)
   *  рстѵфхѱѽц,  100 - 900  0xD1 80,81,82 0xD1 B5 0xD1 84,85 0xD1 B1,BD 0xD1 86
   *  РСТѴФХѰѾЦ
   *
   * exception: 11-19 the tens(1) follows the single digit
   *
   *  CYRILLIC  A,VE,GHE,DE,ukrainian IE,DZE,ZE,I,FITA
   *  CYRILLIC  DOTTED I,KA,EL,EM,EN,KSI,O,PE,CHE
   *  CYRILLIC  ER,ES,TE,IZHITSA,EF,KHA,PSI,OMEGA with TITLO,TSE
   *
   *  single digit, or letter considered digit combining mark titlo
   *   ҃  0xD2 83
   *  thousands prefix
   *  ҂  0xD2 82
   *  powers combining marks
   *  powers 10^4,10^5,10^6,10^7,10^8,10^9
   *  ^4  ^5  ^6  ^7  ^8  ^9
   *    ⃝ ,   ҈,   ҉,   ꙰,   ꙱,   ꙲   0xE2 0x83 9D, 0xD2 88,89, 0xEA 0x99 B0,B1,B2
   *  0 = mathematical space = \xe2 \x81 \x9f
   */
  const uint8_t  u8[3][18] = {
    { 0xD0,0xB0, 0xD0,0xB2, 0xD0,0xB3, 0xD0,0xB4, 0xD1,0x94,
      0xD1,0x95, 0xD0,0xB7, 0xD0,0xB8, 0xD1,0xB3 },
    { 0xD1,0x96, 0xD0,0xBA, 0xD0,0xBB, 0xD0,0xBC, 0xD0,0xBD,
      0xD1,0xAF, 0xD0,0xBE, 0xD0,0xBF, 0xD1,0x87 },
    { 0xD1,0x80, 0xD1,0x81, 0xD1,0x82, 0xD1,0xB5, 0xD1,0x84,
      0xD1,0x85, 0xD1,0xB1, 0xD1,0xBD, 0xD1,0x86 },
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field/find */
  int32_t  wrSz;
  uint8_t *sPtr;
  uint8_t n;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* every 3 adds a 4th, but when request a field of 4 the field needs 5...
   * but the zero delays til end on short fields */
  if (indices[0] == 0)
    df = 1;
  else {
    df = 0;
    /* every number above 999 has addition gylph. thousands have '',
     others have combining glyph ' ', needed to combine the two. */
    /* zeroes have no glyph */
    w = 0;
    do {
      if (indices[w] != 0) df++;
    } while ((++w) < d);
  }

  /* df has digits found according to extra add on glyphs */
  /* can be larger or smaller than d. make use of this before lose */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  /* if missing digits, add to padding (only if padding) */
  if ((w - s) > 0) {
    w = (w - s) + df;
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
    df = 0;
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* <zero> == <mathematical space> */
  if (indices[0] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    goto done;
  }

  /* glyph conversion time */
  /* marks occur in groups of 3 */
  w = 0;
  if (d == 1) {
      /* single digits have a 'nurmeric' mark */
    n = indices[0];
    n = (n - 1) << 1;
    BOUNDS_CHECK(4);
    *sPtr++ = u8[0][n];
    *sPtr++ = u8[0][(n + 1)];
    *sPtr++ = 0xD2;
    *sPtr++ = 0x83;
    goto done;
  }
  if (d > 3) {
    do {
      --d;
      if ((n = indices[w++]) != 0) {
        n = (n - 1) << 1;
        if (d >= 9) {
          BOUNDS_CHECK(6);
          *sPtr++ = u8[0][n];
          *(uint32_t*)sPtr = cconst32_t('\x20','\xEA','\x99','\xB2');
            /* overwrite <space>, originally mac display fix */
          *sPtr = u8[0][(n + 1)];
          sPtr += 4;
        } else if (d == 8) {
          BOUNDS_CHECK(5);
          *sPtr++ = u8[0][n];
          *(uint32_t*)sPtr = cconst32_t('\x20','\xEA','\x99','\xB1');
          *sPtr = u8[0][(n + 1)];
          sPtr += 4;
        } else if (d == 7) {
          BOUNDS_CHECK(5);
          *sPtr++ = u8[0][n];
          *(uint32_t*)sPtr = cconst32_t('\x20','\xEA','\x99','\xB0');
          *sPtr = u8[0][(n + 1)];
          sPtr += 4;
        } else if (d == 6) {
          BOUNDS_CHECK(4);
          *sPtr++ = u8[0][n];
          *sPtr++ = u8[0][(n + 1)];
          *(uint16_t*)sPtr = cconst16_t('\xD2','\x89');
          sPtr += 2;
        } else if (d == 5) {
          BOUNDS_CHECK(4);
          *sPtr++ = u8[0][n];
          *sPtr++ = u8[0][(n + 1)];
          *(uint16_t*)sPtr = cconst16_t('\xD2','\x88');
          sPtr += 2;
        } else if (d == 4) {
          BOUNDS_CHECK(5);
          *sPtr++ = u8[0][n];
          *(uint32_t*)sPtr = cconst32_t('\x20','\xE2','\x83','\x9D');
          *sPtr = u8[0][(n + 1)];
          sPtr += 4;
        } else {
          BOUNDS_CHECK(4);
          *(uint16_t*)sPtr = cconst16_t('\xD2','\x82');
          sPtr += 2;
          /* first symbol than numeral */
          *sPtr++ = u8[(d - 3)][n];
          *sPtr++ = u8[(d - 3)][(n + 1)];
        }
      }
    } while (d > 3);
  }
  while (d) {
    --d;
    if ((n = indices[w++]) != 0) {
        /* 11,12-19 exchange places, read like 11,21-91 */
      if ((d == 1) && (n == 1) && (indices[w] != 0)) {
        BOUNDS_CHECK(4);
        n = (n - 1) << 1;
        sPtr[2] = u8[1][n];
        sPtr[3] = u8[1][(n + 1)];
        n = (indices[w] - 1) << 1;
        sPtr[0] = u8[0][n];
        sPtr[1] = u8[0][(n + 1)];
        sPtr += 4;
        break;
      } else {
        BOUNDS_CHECK(2);
        n = (n - 1) << 1;
        *sPtr++ = u8[d][n];
        *sPtr++ = u8[d][(n + 1)];
      }
    }
  }
  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
ethiWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Ethiopic 0x1369-0x137C
   * ፩፪፫፬፭፮፯፰፱,   x1      0xE1,0x8D,0xA9-B1  (1-9)
   * ፲፳፴፵፶፷፸፹፺,  x10      0xE1,0x8D,0xB2-BA  (10 - 90)
   * ፻,         x100     0xE1,0x8D,0xBB     (suffix)
   * ፼,         x10000   0xE1,0x8D,0xBC     (suffix)
   * As with others, group multiplies for higher numbers
   * assume repeating 10000
   */
  /* To get around variable digit length, must make <zero>
   * different than padding <space>. Going to 0xE2 0x81 0x9F to
   * represent <zero>.
   */

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df, gf; /* digits, width, sign, digit field, glyph field */
  int32_t  wrSz;
  uint8_t *sPtr;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* because of nurmerals like 50 being 1 glyph "፶",
   * 2100 being 3 glyphs, 9910 being 4 but 9911 = 5,
   * 10000 being 2 (1 x 10000)... must actually do glyph convert
   * to calculate digit field.
   * idea: replace indices with 0xA9-0xBA, use 09-1A for
   * suffix with 100, use 29-3A for suffix 10000.
   * 0xA9-0xBA count as 1 glyph, 09-1A.. count 2,
   * 29-3A count 2
   */

  /* every hundred of a set of 4 has a glyph unless 0 */
  /* evey set of 4 has a ten thousand glyph */
  gf = (df = 0);
  if (indices[0] == 0)  gf++, df++;
  else {
    uint8_t d_copy = d, h;
    w = 0;
    if (d_copy > 4) {
      if ((s = (d_copy & 3)) == 0) s = 4;
      do {
        h = 0, d_copy -= s;
        do {
          if (indices[w++] != 0)  h = 1, gf++, df++;
          if (((--s) == 2) && h)  h = 0, gf++;
        } while (s);
        s = 4, gf++;
      } while (d_copy > 4);
    }
    h = 0;
    do {
      if (indices[w++] != 0)       h = 1, gf++, df++;
      if (((--d_copy) == 2) && h)  h = 0, gf++;
    } while (d_copy);
  }

  /* df has digits found according to extra add on glyphs */
  /* can be larger or smaller than d. make use of this before lose */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)gf > DIGIT_FIELD) ? (gf - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* <zero> == <mathematical space> */
  if (indices[0] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
  } else {
    /* convert indices to UTF-8 display */
    /* must do >= 10000 first */
    uint8_t h;
    w = 0;
    if (d > 4) {
      /* obtain first grouping of multiple 4 */
      if ((s = d & 3) == 0)  s = 4;
      do {
        h = 0;
        d -= s;
        do {
          uint8_t n = indices[w++];
          if (n != 0) {
            h = 1;
            BOUNDS_CHECK(3);
            *sPtr++ = (uint8_t)0xE1;
            *sPtr++ = (uint8_t)0x8D;
            *sPtr++ = n + ((s & 1) ? (uint8_t)0xA8 : (uint8_t)0xB1);
          }
          if (((--s) == 2) && h) {
            /* mark off x100 */
            h = 0;
            BOUNDS_CHECK(3);
            *sPtr++ = (uint8_t)0xE1;  *sPtr++ = (uint8_t)0x8D;  *sPtr++ = (uint8_t)0xBB;
          }
        } while (s);
        s = 4;
        /* mark off x10000 */
        BOUNDS_CHECK(3);
        *sPtr++ = (uint8_t)0xE1;  *sPtr++ = (uint8_t)0x8D;  *sPtr++ = (uint8_t)0xBC;
        /* check if last 4 are all zero */
      } while (d > 4);
    }
    /* only 1 group of 4 or less */
    h = 0;
    do {
      uint8_t n = indices[w++];
      if (n != 0) {
        h = 1;
        BOUNDS_CHECK(3);
        *sPtr++ = (uint8_t)0xE1;
        *sPtr++ = (uint8_t)0x8D;
        *sPtr++ = n + ((d & 1) ? (uint8_t)0xA8 : (uint8_t)0xB1);
      }
      if (((--d) == 2) && h) {
        /* mark off x100 */
        BOUNDS_CHECK(3);
        *sPtr++ = (uint8_t)0xE1;  *sPtr++ = (uint8_t)0x8D;  *sPtr++ = (uint8_t)0xBB;
      }
    } while (d);
  }

  /* we terminated because all was read */
  /* return next write position, termination reason */
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
georWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Georgian
   * აბგდევზჱთ,   1-9            0xE1,0x83,[0x90-96 - 0xB1 - 0x97]
   * იკლმნჲოპჟ,   10 - 90        0xE1,0x83,[0x98-9C - 0xB2 - 0x9D-9F]
   * რსტუფქღყშ,   100 - 900      0xE1,0x83,[0xA0-A8]
   * ჩცძწჭხჴჯჰ,   1000 - 9000    0xE1,0x83,[0xA9-AE - 0xB4 - 0xAF-B0]
   * ჵ,           10000          0xE1,0x83,0xB5
   * ჳ additional 400            0xE1,0x83,0xB3
   * assume repeating 10000
   */
  /* group glyph is <nbsp>, need new <zero> */
  /* E2 80 88 Punctuation Space to be used */

  const uint8_t  u8[4][9] = {
    { 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0xB1, 0x97 },
    { 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0xB2, 0x9D, 0x9E, 0x9F },
    { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8 },
    { 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xB4, 0xAF, 0xB0 },
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  if (indices[0] == 0)
    df = 1;
  else {
    df = 0;
    if (d >= 5)  /* add in ten thousands glyph */
      df = (d - 1) >> 2;
    /* zeroes have no glyph */
    w = 0;
    do {
      if (indices[w] != 0) df++;
    } while ((++w) < d);
  }

  /* df has digits found according to extra add on glyphs */
  /* can be larger or smaller than d. make use of this before lose */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* <zero> == <mathematical space> */
  if (indices[0] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    goto done;
  }

  w = 0;
  /* must do >= 10000 first, repeating */
  if (d > 4) {
    /* obtain first grouping of multiple 4 */
    if ((s = d & 3) == 0)  s = 4;
    do {
      uint8_t n = 0;
      d -= s;
      do {
  if ((s == 4) && (*(uint32_t*)&indices[w] == 0)) {
    s -= 4, w += 4;
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    break;
  }
        --s;
        if ((n = indices[w++]) != 0) {
          BOUNDS_CHECK(3);
          *sPtr++ = (uint8_t)0xE1;
          *sPtr++ = (uint8_t)0x83;
          *sPtr++ = u8[s][(n - 1)];
        }
      } while (s != 0);
      BOUNDS_CHECK(3);
      *sPtr++ = (uint8_t)0xE1;
      *sPtr++ = (uint8_t)0x83;
      *sPtr++ = (uint8_t)0xB5;
      s = 4;
    } while (d > 4);
  }
  /* only 1 group of 4 or less */
  do {
    uint8_t n;
    --d;
    if ((n = indices[w++]) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = (uint8_t)0xE1;
      *sPtr++ = (uint8_t)0x83;
      *sPtr++ = u8[d][(n - 1)];
    }
  } while (d);

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
grekWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Greek
   * ΑΒΓΔΕϚΖΗΘ,   1-9            0xCE,[0x91-95]  0xCF,0x9A  0xCE,[0x96-98]
   * ΙΚΛΜΝΞΟΠϞ,   10 - 90        0xCE,[0x99-A0]  0xCF,0x9E
   * ΡΣΤΥΦΧΨΩϠ,   100 - 900      0xCE,0xA1  0xCE,[0xA3-A9]  0xCF,0xA0
   * ͵             1000 prefix   (0x0374) 0xCD,0xB5
   * ʹ            numeric suffix (0x0375) 0xCD.0xB4
   * ͵Ϡ == 900,000
   * ͵͵ == 1,000,000
   * ͵͵Ϡ == 900,000,000
   * ͵͵͵ == 1,000,000,000
   */
  const uint8_t  u8[3][18] = {
    { 0xCE,0x91, 0xCE,0x92, 0xCE,0x93, 0xCE,0x94, 0xCE,0x95,
      0xCF,0x9A, 0xCE,0x96, 0xCE,0x97, 0xCE,0x98 },
    { 0xCE,0x99, 0xCE,0x9A, 0xCE,0x9B, 0xCE,0x9C, 0xCE,0x9D,
      0xCE,0x9E, 0xCE,0x9F, 0xCE,0xA0, 0xCF,0x9E },
    { 0xCE,0xA1, 0xCE,0xA3, 0xCE,0xA4, 0xCE,0xA5, 0xCE,0xA6,
      0xCE,0xA7, 0xCE,0xA8, 0xCE,0xA9, 0xCF,0xA0 },
  };
  const uint8_t   thou[2] = { 0xCD, 0xB5 };
  const uint8_t   nEnd[2] = { 0xCD, 0xB4 };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field/find */
  int32_t  wrSz;
  uint8_t *sPtr;
  uint8_t lr, lq;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* zeroes have no glyph, marks not counted as digit glyphs */
  if (indices[0] == 0)
    df = 1;
  else {
    df = 0;
    w = 0;
    do {
      if (indices[w] != 0) df++;
    } while ((++w) < d);
  }

  /* df has digits found according to extra add on glyphs */
  /* can be larger or smaller than d. make use of this before lose */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  if (df != 0) {
    BOUNDS_CHECK(df);
    do *sPtr++ = ' '; while ((--df));
  }

/* zero is a number, but not part of system; suffix with numeric ender? */
  /* <zero> == <mathematical space> */
  if (indices[0] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    goto done;
  }

  /* need remainder 3, in loop just subtract/test */
  s = (((uint32_t)d * (uint16_t)21846) >> 16);
  lr = d - ((s << 1) + s);
  lq = s;

  w = 0;
  do {
    uint8_t n;
    --d;
    if (lr == 0) {
      lr = 2;
      if (lq != 0) --lq;
    } else --lr;
    if ((n = indices[w++]) != 0) {
      const uint8_t *uPtr = &u8[lr][((n - 1) << 1)];
      /* create 1000 prefix first, if needed (floor) */
      /* s is quotient of divide by 3 */
      if (lq != 0) {
        s = lq;
        do {
          BOUNDS_CHECK(2);
          *(uint16_t *)sPtr = *(uint16_t *)thou;
          sPtr += 2;
        } while ((--s));
      }
      BOUNDS_CHECK(2);
      *sPtr++ = uPtr[0];
      *sPtr++ = uPtr[1];
    }
  } while (d);
  /* add numeric terminator */
  BOUNDS_CHECK(2);
  *(uint16_t *)sPtr = *(uint16_t *)nEnd;
  sPtr += 2;

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
hanidecWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* 〇一二三四五六七八九  Chinese decimal variation
   * const uint16_t udigit[10] = {
   *   0x3007, 0x4E00, 0x4E8C, 0x4E09, 0x56DB,
   *   0x4E94, 0x516D, 0x4E03, 0x516B, 0x4E5D
   * };
   *
   */
  const uint8_t u8[10][3] = {
    0xE3,0x80,0x87, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
    0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
    0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
  };

  const uint8_t  dZero[3] = { 0xE3, 0x80, 0x87 };
  uint8_t indices[44];
  int32_t _eval;
  int32_t  d, w, s, df; /* width, sign, digit field */
  uint32_t flags;
  int32_t  wrSz;
  uint8_t *sPtr;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  /* number of indices (digit glyphs) */
  cnvrt->cs_wr = (void *)indices;
  d = indicize(cnvrt, rflags);

  /* convience */
  flags = *rflags;

  /* any missing digits? */
  df = (w = DIGIT_FIELD) - d;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display? */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((w + 1) <= d)  s = 1;

  /* padding? */
  if ((w = TOTAL_FIELD - (d + df + s)) < 0)  w = 0;

  /* left field padding (pre-sign) */
  if (w && (flags & PAD_SPACE)) {
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    do {
      uint8_t smblCh = *smblPtr;
      smblPtr++;
      if (smblCh == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* if <spaces> to be used, then w should be 0; else <zero> used for both */
  if ((flags & FILL_SPACE) != 0) {
    if (df) {
      BOUNDS_CHECK(df);
      do *sPtr++ = ' '; while ((--df));
    }
  } else {
    if ((w += df) != 0) {
      BOUNDS_CHECK(((w << 1) + w));
      do {
        sPtr[0] = dZero[0];
        sPtr[1] = dZero[1];
        sPtr[2] = dZero[2];
        sPtr += 3;
      } while ((--w));
    }
  }

  /* convert indices to UTF-8 display */
  BOUNDS_CHECK(((d << 1) + d));
  do {
    s = indices[w++];
    *sPtr++ = u8[s][0];
    *sPtr++ = u8[s][1];
    *sPtr++ = u8[s][2];
  } while ((--d));

  /* we terminated because all was read */
  /* return next write position, termination reason */
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

/*
 using newly written jpan as basis for hans
 chinese writes 1s before suffix, but sometimes not on 10's or teens.
 the ten, teens are some not all, considered dialect
 */
csEnum
hansWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Simplified Chinese (same as hant, save power of tens)
   * 零一二三四五六七八九
   * 十百千万亿 10,10^2,10^3,10^4,10^8
   * 兆京垓秭穰沟涧  12,16,20,24,28,32,36  2^128 goes to 38
   */
  const uint8_t u8[10][3] = {
    0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
    0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
    0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
  };
  const uint8_t p8[12][3] = {
    0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE4,0xB8,0x87,
    0xE4,0xBA,0xBF, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
    0xE7,0xA7,0xAD, 0xE7,0xA9,0xB0, 0xE6,0xB2,0x9F, 0xE6,0xB6,0xA7
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int8_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr;
  uint8_t n, pN;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  n = 0;
  if (d == 1)  df = 1;
      /* teen's get implied 1 */
  else if ((d == 2) && (indices[0] == 1)) {
    df = 1;
      /* flag for a '10' terminator */
    if (indices[1] == 0)  n = 1;
  }
  else {
    df = 0, w = 0;
      /* count non-zeroes, each above '0' get a suffix */
    do {  if ((n = indices[w]) >= 1)  df++;  } while ((++w) < d);
    if (d > 4) {
      if      ((d & 3) != 0)   df++;
      else if ((d >> 3) != 0)  df++;
    }
  }
    /* use digits found before converting to missing digit field */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

    /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

    /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  /* problem with <space>, strptime must eat up white space, voiding attempt as
   * field count */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* special case:  where zero could print, value == 0 */
  if (d == 1) {
    const uint8_t *uPtr = u8[(indices[0])];
    BOUNDS_CHECK(3);
    *sPtr++ = uPtr[0], *sPtr++ = uPtr[1], *sPtr++ = uPtr[2];
    goto done;
  }
  if ((d == 2) && (indices[0] == 1)) {
    BOUNDS_CHECK(3);
    *sPtr++ = p8[0][0], *sPtr++ = p8[0][1], *sPtr++ = p8[0][2];
    if (n == 1) {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
    } else {
      n = indices[1];
      BOUNDS_CHECK(3);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
    }
    goto done;
  }

  /* groups of <= 10000, d * 10 ^3, d * 10^2 + d * 10^1 + d
   * if more than 1 group, suffix with p8[], power of 10s
   */

  pN = 0;
  w = 0;
  d -= 1;
  s = d >> 2;
  d &= 3;

  if (d == 0)  goto J1;
  if (d == 1)  goto J10;
  if (d == 2)  goto J100;

  do {
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[2][0], *sPtr++ = p8[2][1], *sPtr++ = p8[2][2];
    }
  J100:
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[1][0], *sPtr++ = p8[1][1], *sPtr++ = p8[1][2];
    }
  J10:
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[0][0], *sPtr++ = p8[0][1], *sPtr++ = p8[0][2];
    }
  J1:
    if ((n = indices[w++]) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
    }
    if (s == 0)  break;
    if ((pN | n) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = p8[(s + 2)][0];
      *sPtr++ = p8[(s + 2)][1];
      *sPtr++ = p8[(s + 2)][2];
    }
    --s;
  } while (1);

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
hantWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Traditional Chinese (same as hans, save powers of 10)
   * 零一二三四五六七八九
   * 十百 千  10,10^2, 10^3(E5 8D 83)
   * 萬億兆京垓秭穰溝澗  4,8,12,16,20,24,28,32,36  2^128 goes to 38
   */
  const uint8_t u8[10][3] = {
    0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
    0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
    0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
  };
  const uint8_t p8[12][3] = {
    0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE8,0x90,0xAC,
    0xE5,0x84,0x84, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
    0xE7,0xA7,0xAD, 0xE7,0xA9,0xB0, 0xE6,0xBA,0x9D, 0xE6,0xBE,0x97
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int8_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr;
  uint8_t n, pN;

    /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
    /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  if (d == 1)  df = 1;
    /* teen's get implied 1 */
  else if ((d == 2) && (indices[0] == 1))
    df = 1;
  else {
    df = 0, w = 0;
      /* count non-zeroes, each above '0' get a suffix */
    do {  if ((n = indices[w]) >= 1)  df++;  } while ((++w) < d);
    if (d > 4) {
      if      ((d & 3) != 0)   df++;
      else if ((d >> 3) != 0)  df++;
    }
  }
    /* use digits found before converting to missing digit field */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

    /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

    /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

    /* left field padding (pre-sign) */
    /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

    /* arithmetic sign display */
    /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  /* problem with <space>, strptime must eat up white space, voiding attempt as
   * field count */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

    /* special case:  where zero could print, value == 0 */
  if (d == 1) {
    const uint8_t *uPtr = u8[(indices[0])];
    BOUNDS_CHECK(3);
    *sPtr++ = uPtr[0], *sPtr++ = uPtr[1], *sPtr++ = uPtr[2];
    goto done;
  }
  if ((d == 2) && (indices[0] == 1)) {
    BOUNDS_CHECK(6);
    *sPtr++ = p8[0][0], *sPtr++ = p8[0][1], *sPtr++ = p8[0][2];
    if ((n = indices[1]) != 0)
          *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
    else  *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
    goto done;
  }

  /* groups of <= 10000, d * 10 ^3, d * 10^2 + d * 10^1 + d
   * if more than 1 group, suffix with p8[], power of 10s
   */

  pN = 0;
  w = 0;
  d -= 1;
  s = d >> 2;
  d &= 3;

  if (d == 0)  goto J1;
  if (d == 1)  goto J10;
  if (d == 2)  goto J100;

  do {
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[2][0], *sPtr++ = p8[2][1], *sPtr++ = p8[2][2];
    }
  J100:
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[1][0], *sPtr++ = p8[1][1], *sPtr++ = p8[1][2];
    }
  J10:
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(6);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      *sPtr++ = p8[0][0], *sPtr++ = p8[0][1], *sPtr++ = p8[0][2];
    }
  J1:
    if ((n = indices[w++]) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
    }
    if (s == 0)  break;
    if ((pN | n) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = p8[(s + 2)][0];
      *sPtr++ = p8[(s + 2)][1];
      *sPtr++ = p8[(s + 2)][2];
    }
    --s;
  } while (1);

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

/* does not use GERESH or GERSHAYIM at this time, need flag to signal use */
/* groupbit to be used, 0 or default is contextual; 1 or set is non-contextual */
csEnum
hebrWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive  Hebrew   0xD7 prefix
   * אבגדהוזחט  1 - 9
   * יכלמנסעפצ  10 - 90
   * קרשת       100 - 400
   * ךםןףץ      500 - 900  not traditionally used
   * ׳          thousands/millions group (suffix)
   * ״          last digit (1s) prefix with 2 or more digits 0xD7,0xB4
   */

  const uint8_t  u8[2][9] = {
    { 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98 },
    { 0x99,0x9B,0x9C,0x9E,0xA0,0xA1,0xA2,0xA4,0xA6 }
  };
  const uint8_t  h8[9] = {
    0xA7,0xA8,0xA9,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  uint16_t jn, jr;
  int32_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr, pN;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* get actual glyph print from digits */
  jn = 0;
  jr = (df = 1);
  if (d > 1) {
    /* divide by 3 for remainder */
    jn = ((uint16_t)d * (uint16_t)21846) >> 16;
    jr = d - ((jn << 1) + jn);
    w = (df = 0);
    if (jr == 2)  goto H2;
    if (jr == 1)  goto H1;
    do {
      /* case hundreds */
      s = indices[w++];
      if (s != 0) {
        if     (s == 9)  df += 3;
        else if (s > 4)  df += 2;
        else             df += 1;
      }
    H2:
      /* case tens */
      if (indices[w++] != 0)   df += 1;
      /* case ones */
    H1:
      if (indices[w++] != 0)   df += 1;
    } while (w < d);
  }

  /* use digits found before converting to missing digit field */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)df > DIGIT_FIELD) ? (df - DIGIT_FIELD) : 0);

  /* any missing digits? */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* Super additive, must do groups of thousands */
  /* exceptions for 15 and 16 */
  /* <zero> == <mathematical space> */
  if (indices[(w = 0)] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    goto done;
  }

  pN = 0;
  s = 0;
  /* divide by 3 for remainder (done in df counting) */
  if (jr == 2)  goto has2;
  if (jr == 1)  goto has1;
  --jn;
  do {
    uint8_t n;
    pN |= (n = indices[w++]);
    if (n != 0) {
      BOUNDS_CHECK(2);
      *sPtr++ = (uint8_t)0xD7;  *sPtr++ = h8[(n - 1)];
      df++;
      if (n > 4) {
        BOUNDS_CHECK(2);
        *sPtr++ = (uint8_t)0xD7;  *sPtr++ = h8[(n - 5)];
        df++;
        if (n == 9) {
          BOUNDS_CHECK(2);
          *(uint16_t*)sPtr = cconst16_t('\xD7','\xA7');  sPtr += 2;
          df++;
    } } }
  has2:
    pN |= (n = indices[w++]);
    if (n != 0) {
      s = (n == 1) ? 2 : 0;
      BOUNDS_CHECK(2);
      *sPtr++ = (uint8_t)0xD7;  *sPtr++ = u8[1][(n - 1)];
      df++;
    }
  has1:
    pN |= (n = indices[w++]);
    if (n != 0) {
      if ((n == 5) || (n == 6))  s |= 1;
      BOUNDS_CHECK(2);
      *sPtr++ = (uint8_t)0xD7;  *sPtr++ = u8[0][(n - 1)];
      df++;
    }
    if (jn == 0) {
        /* see if was last digit, check 15,16, then add gershayim */
      if (df >= 2) {
          /* has number not ending on geresh */
        if (s == 3) {
            /* case: 15 or 16 */
          uint8_t swap = sPtr[-1];  sPtr[-1] = sPtr[-3];  sPtr[-3] = swap;
        }
        if ((sPtr[-3] != (uint8_t)0xB3) && (sPtr[-1] != (uint8_t)0xB3)) {
            /* case: 2 numbers adjacent, add gershayim */
          BOUNDS_CHECK(2);
          *(uint16_t*)sPtr = *(uint16_t*)&sPtr[-2];
          sPtr[-1] = (uint8_t)0xB4;
          sPtr += 2;
        }
      }
      break;
    }
      /* now add thousands mark, ׳, geresh */
    if (pN != 0) {
      pN = jn;
      do {
        BOUNDS_CHECK(2);
        *(uint16_t*)sPtr = cconst16_t('\xD7','\xB3');
        sPtr += 2;
      } while ((--pN) != 0);
    }
    s = (pN = 0);
    jn--;
  } while (1);

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

csEnum
jpanWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Japanese
   * 零一二三四五六七八九
   * 十百 千  10,10^2, 10^3(E5 8D 83)
   * 万億兆京垓𥝱,秭穣溝澗  4,8,12,16,20,24,24,28,32,36  2^128 goes to 38
   *
   * must have power delimiter (invis punct when ends on power)
   * must have '1' begining delimiter (invis punct when start is without digit)
   * all jpan consider element as "digit/power"
   */
  const uint8_t u8[10][3] = {
    0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
    0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
    0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
  };
  const uint8_t p8[12][3] = {
    0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE4,0xB8,0x87,
    0xE5,0x84,0x84, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
    0xE7,0xA7,0xAD, 0xE7,0xA9,0xA3, 0xE6,0xBA,0x9D, 0xE6,0xBE,0x97
  };

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int8_t d, w, s, df, gf; /* digits, width, sign, digit field, glyphs found */
  int32_t  wrSz;
  uint8_t *sPtr;
  uint8_t n, pN;

    /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  df = 1, gf = 1;
  if (d > 1) {
    w = 0, df = 0, gf = 0;
    if ((s = d & 3) != 0)
      do {
        if (indices[w] != 0)  gf++;
        if (indices[w] > 1)  df++;
      } while ((++w) < s);
    if ((n = d >> 2) != 0) {
      if (w != 0)  gf++;  /* had digits prior, add myriad mark */
      do {
          /* look ahead, greater than 4, check if will recieve 10000 mark */
        if ((n > 1) && (*(uint32_t*)&indices[w] != 0))  gf++;
          /* as above, check the end or intermediate group of 4 */
        do {
          if (indices[w] != 0)  gf++;
          if (indices[w] > 1)  df++;
        } while ((((++w) - s) & 3) != 0);
      } while ((--n) != 0);   /* update myriad number */
    }
    while (indices[(--w)] == 0) if (w == 0)  break;
    if (indices[w] == 1)
      df++;
  }
  if (df == 0)  df = 1;

    /* use glyphs found, power glyphs take up space */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)gf > DIGIT_FIELD) ? (gf - DIGIT_FIELD) : 0);

    /* any missing digits?, differs from above in this is 'nelem' */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

    /* arithmetic sign display?, match latin's power of ten, instead of glyph */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((int32_t)(DIGIT_FIELD + 1) <= d)  s = 1;

    /* left field padding (pre-sign) */
    /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

    /* arithmetic sign display */
    /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  /* complete digit field, delete missing, use space, use zero */
  /* problem with <space>, strptime must eat up white space, voiding attempt as
   * field count */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  }

  /* special case:  where zero could print, value == 0 */
  if (d == 1) {
    const uint8_t *uPtr = u8[(indices[0])];
    BOUNDS_CHECK(3);
    *sPtr++ = uPtr[0];
    *sPtr++ = uPtr[1];
    *sPtr++ = uPtr[2];
    goto done;
  }

  /* groups of <= 10000, d * 10 ^3, d * 10^2 + d * 10^1 + d
   * if more than 1 group, suffix with p8[], power of 10s
   */

  pN = 0;
  w = 0;
  d -= 1;
  s = d >> 2;
  d &= 3;

    /* first digit '1' is implied */
  if ((n = indices[w]) == 1) {
    BOUNDS_CHECK(3);
      /* invisible punctuation */
    *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
  }

  if (d == 0)  goto J1;
  if (d == 1)  goto J10;
  if (d == 2)  goto J100;

  do {
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      }
      BOUNDS_CHECK(3);
      *sPtr++ = p8[2][0], *sPtr++ = p8[2][1], *sPtr++ = p8[2][2];
    }
  J100:
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      }
      BOUNDS_CHECK(3);
      *sPtr++ = p8[1][0], *sPtr++ = p8[1][1], *sPtr++ = p8[1][2];
    }
  J10:
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      }
      BOUNDS_CHECK(3);
      *sPtr++ = p8[0][0], *sPtr++ = p8[0][1], *sPtr++ = p8[0][2];
    }
  J1:
    n = indices[w++];
    if (n != 0) {
      if ((n > 1) || (pN != 0) || (s == 0)) {
        BOUNDS_CHECK(3);
        *sPtr++ = u8[n][0], *sPtr++ = u8[n][1], *sPtr++ = u8[n][2];
      }
    }
    if (s == 0)  break;
    if ((pN | n) != 0) {
      BOUNDS_CHECK(3);
      *sPtr++ = p8[(s + 2)][0];
      *sPtr++ = p8[(s + 2)][1];
      *sPtr++ = p8[(s + 2)][2];
    }
    --s;
  } while (1);

    /* break on last, n has last digit read */
    /* if ended with power mark, add punctuation */
  if (n == 0) {
    BOUNDS_CHECK(3);
      /* invisible punctuation */
    *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
  }

  /* we terminated because all was read */
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

/* some optimization (BOUNDS), single uint8_t zero */
csEnum
latnWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr   = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  /* any missing digits? */
  df = (w = DIGIT_FIELD) - d;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

  /* arithmetic sign display? */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((w + 1) <= d)  s = 1;

  /* padding? */
  if ((w = TOTAL_FIELD - (d + df + s)) < 0)  w = 0;

  /* does it fit buffer? need char based, not glyph */
  BOUNDS_CHECK(w + s + d + df);

/* adding in justify left, right padding */
if ((flags & JUSTIFY_BIT) == 0)
  /* left field padding (pre-sign) */
  /* currently no international pad ' ' */
  if (w && (flags & PAD_SPACE))  do *sPtr++ = ' '; while ((--w));

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    /* a bounds check for 1 uint8_t already done */
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }
if ((flags & JUSTIFY_BIT) == 0)
  /* left field padding (post-sign => <zeros> normally) */
  if (w)  do *sPtr++ = '0'; while ((--w));
  /* complete digit field, delete missing, use space, use zero */
  if (df) {
    uint8_t fch = (flags & FILL_SPACE) ? ' ' : '0';
    do *sPtr++ = fch; while ((--df));
  }
s = 0;
  do *sPtr++ = indices[s++] + '0'; while ((--d));
  /* w == 0, convert indices to encoding display */
/*  do *sPtr++ = indices[w++] + '0'; while ((--d));*/

if ((flags & JUSTIFY_BIT) != 0)
  if (w)  do *sPtr++ = ' '; while ((--w));

  /* we terminated because all was read */
  /* return next write position, termination reason */
  cnvrt->cs_wr  = (void *)sPtr;
  return csRD;
}

/*
                         =                   1
w                ௰      =                  10
m                ௱      =                 100
   t             ௲      =               1,000
w  t            ௰௲     =              10,000
m  t            ௱௲     =             100,000 (lakh)
wm t           ௰௱௲    =           1,000,000 (10 lakhs)

    mmt        ௱௱௲    =          10,000,000 (crore)
w   mmt       ௰௱௱௲   =         100,000,000 (10 crore)
m   mmt       ௱௱௱௲   =       1,000,000,000 (100 crore)
  t mmt       ௲௱௱௲   =      10,000,000,000 (thousand crore)
w t mmt      ௰௲௱௱௲  =     100,000,000,000 (10 thousand crore)
m t mmt      ௱௲௱௱௲  =   1,000,000,000,000 (lakh crore)
wmt mmt     ௰௱௲௱௱௲ =  10,000,000,000,000 (10 lakhs crore)

    mmt mmt	௱௱௲௱௱௲ = 100,000,000,000,000 (crore crore)

  this is wrong.. add t
 wmmt != 1wmt + 1mt != wmm(t) 10crore != 10lakhs/lakh
wmt  digit + B0,B1,(B2)
m t  digit +    B1,(B2)
w t  digit + B0,   (B2)
  t  digit +       (B2)
  add t    +        B2
m    digit +    B1
w    digit + B0
     digit

 */
csEnum
tamlWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* http://www.unicode.org/notes/tn21/tamil_numbers.pdf */
  /*   Archaic Tamil (multiplicative)
   * ௧ ௨ ௩ ௪ ௫ ௬ ௭ ௮ ௯    0xE0,0xAF,0xA7-AF
   * ௰ ௱ ௲  10,100,1000   0xE0,0xAF,0xB0-B2
   *
   * except for single digit, a 'digit' is its prefix plus its
   * multiplier suffix(es). a '1' prefix is implied.
   */

  const uint8_t gadd[7] = {  3, 2, 2, 1, 1, 1, 0  };
  uint32_t flags;
  uint8_t indices[44];
  int32_t _eval;
  int32_t d, w, s, df; /* digits, width, sign, digit field */
  int32_t  wrSz;
  uint8_t *sPtr;
  int32_t gf;
  uint8_t addin, n, pN, r7;
  uint16_t q7;

  /* early return conditional (supports up to 128bit numbers) */
  if ((((uint32_t)cnvrt->cs_rdSz) - (uint32_t)1) > (uint32_t)3)  return csIE;

  wrSz = (int32_t)cnvrt->cs_wrSz;
  sPtr = (uint8_t *)cnvrt->cs_wr;

  cnvrt->cs_wr = (void *)indices;
  /* number of indices (digit glyphs) */
  d = indicize(cnvrt, rflags);
  flags = *rflags;

  gf = 0;
  addin = 0;
  df = 0;
  w = d;
  s = d - 7;
    /* the 'single' treated differently */
  if (indices[(--w)] != 0)  df++, gf++;
  if (w != 0)
    do {
      if (w == s) {
        s -= 7;
        addin += 3;
        if (s <= 7)  gf += addin;
        else {
          if ((*(uint32_t*)&indices[(w - 7)] != 0)
                 || (*(uint32_t*)&indices[(w - 4)] != 0))
            gf += addin;
        }
      }
      if (indices[(--w)] != 0)   df++, gf += (indices[w] > 1) + gadd[(w - s)];
    } while (w != 0);
  if (df == 0)  df = 1, gf++;

    /* use glyphs found, power glyphs take up space */
  w = (TOTAL_FIELD - DIGIT_FIELD)
    - (((uint32_t)gf > DIGIT_FIELD) ? (gf - DIGIT_FIELD) : 0);

    /* any missing digits?, differs from above in this is 'nelem' */
  df = DIGIT_FIELD - df;
  if ((df < 0) || ((flags & FILL_MASK) == 0))  df = 0;

    /* arithmetic sign display? */
  s = ((flags & (SIGNED_VALUE | SIGN_NEG | SIGN_MUST)) != 0);
  if ((s == 0) && (flags & SIGN_MIN))
    if ((w + 1) <= d)  s = 1;

  /* left field padding (pre-sign) */
  /* adjust padding according to sign info */
  if ((w - s) > 0) {
    w = (w - s);
    BOUNDS_CHECK(w);
    do *sPtr++ = ' '; while ((--w));
  }

  /* arithmetic sign display */
  /* obtain plus/minusSign from ld->lc_numeric */
  if (s) {
    nsEnum  item = ((flags & (SIGNED_VALUE | SIGN_NEG)) ?
                    NS_MINUSSIGN : NS_PLUSSIGN);
    uint8_t *smblPtr = ns_numberinfo(item, ns);
    BOUNDS_CHECK(1);
    *sPtr++ = *smblPtr;
    do {
      uint8_t smblCh;
      if ((smblCh = *++smblPtr) == 0)  break;
      BOUNDS_CHECK(1);
      *sPtr++ = smblCh;
    } while (1);
  }

  w = 0;
  /* complete digit field, delete missing, use space, use zero */
  /* problem with <space>, strptime must eat up white space, voiding attempt as
   * field count */
  if (df != 0) {
    do {
      BOUNDS_CHECK(3);
      *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    } while ((--df));
  } else if ((d > 1) && (indices[w] == 1)) {
    BOUNDS_CHECK(3);
    /* invisible punctuation */
    *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
  }

  /* no zero mutiples 10,000,000,000 kinda */
  /* <zero> == <mathematical space> */
  if (indices[w] == 0) {
    BOUNDS_CHECK(3);
    *sPtr++ = 0xE2,  *sPtr++ = 0x81,  *sPtr++ = 0x9F;
    goto done;
  }

  pN = 0;
  q7 = 0;
  if (d == 1)   goto J1;
  if (d == 2)   goto J10;
  if (d == 3)   goto J100;
  q7 = ((uint16_t)d * (uint16_t)37450) >> 18;
  r7 = d - ((q7 << 3) - q7);
  if (r7 == 0)  q7--;
  else {
    if (r7 == 1)  goto J1;
    if (r7 == 2)  goto J10;
    if (r7 == 3)  goto J100;
    if (r7 == 4)  goto J1000;
    if (r7 == 5)  goto J10000;
    if (r7 == 6)  goto J100000;
  }

  do {
      /* wmt  ௰௱௲ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(9);
      *(uint32_t*)&sPtr[0] = cconst32_t('\xE0','\xAF','\xB0','\xE0');
      *(uint32_t*)&sPtr[4] = cconst32_t('\xAF','\xB1','\xE0','\xAF');
      sPtr[8] = 0xB2;
      sPtr += 9;
    }
  J100000:
      /*  mt  ௱௲ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(6);
      *(uint16_t*)sPtr     = cconst16_t('\xE0','\xAF');
      *(uint32_t*)&sPtr[2] = cconst32_t('\xB1','\xE0','\xAF','\xB2');
      sPtr += 6;
    }
  J10000:
      /*  wt  ௰௲ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(6);
      *(uint16_t*)sPtr     = cconst16_t('\xE0','\xAF');
      *(uint32_t*)&sPtr[2] = cconst32_t('\xB0','\xE0','\xAF','\xB2');
      sPtr += 6;
    }
  J1000:
      /*  t   ௲ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(3);
      *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
      sPtr[2] = 0xB2;
      sPtr += 3;
    }
  J100:
      /*  m  ௱ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(3);
      *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
      sPtr[2] = 0xB1;
      sPtr += 3;
    }
  J10:
      /*  w  ௰ */
    pN |= (n = indices[w++]);
    if (n != 0) {
      if (n > 1) {
        BOUNDS_CHECK(3);
        *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
        sPtr[2] = (uint8_t)0xA6 + n;
        sPtr += 3;
      }
      BOUNDS_CHECK(3);
      *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
      sPtr[2] = 0xB0;
      sPtr += 3;
    }
  J1:
      /* single */
    pN |= (n = indices[w++]);
    if ((n > 1) || ((q7 == 0) && (n != 0))) {
      BOUNDS_CHECK(3);
      *(uint16_t*)sPtr = cconst16_t('\xE0','\xAF');
      sPtr[2] = (uint8_t)0xA6 + n;
      sPtr += 3;
    }

    if (q7 == 0)  break;
      /* mmt  ௱௱௲  B1,B1,B2 */
    if (pN != 0) {
      pN = q7;
      do {
        BOUNDS_CHECK(9);
        *(uint32_t*)&sPtr[0] = cconst32_t('\xE0','\xAF','\xB1','\xE0');
        *(uint32_t*)&sPtr[4] = cconst32_t('\xAF','\xB1','\xE0','\xAF');
        sPtr[8] = 0xB2;
        sPtr += 9;
      } while ((--pN) != 0);
    }
    pN = 0;
    q7--;
  } while (1);

  /* we terminated because all was read */
    /* break on last, n has last digit read */
    /* if ended with power mark, add punctuation */
  if (n == 0) {
    BOUNDS_CHECK(3);
      /* invisible punctuation */
    *sPtr++ = 0xE2, *sPtr++ = 0x81, *sPtr++ = 0xA3;
  }
  /* return next write position, termination reason */
done:
  cnvrt->cs_wr = (void *)sPtr;
  return csRD;
}

#pragma mark ### numericRd

/* standardize positional nurmeric pre-digit parsing */
/* allows embedded to delete, or ease of understanding */

static void
preNumericCheck(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  uint8_t *rdPtr = (uint8_t *)cnvrt->cs_rd;
  uint8_t  rdCh  = *rdPtr;
  /* Note: hi 16 has min digit field, lo 16 holds formated requested size */
  uint32_t nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  uint8_t *smblPtr, smblCh, sdx;

  /* test for format padding before sign */
  while (rdCh == ' ') {
    if ((--nelem) == 0)  goto reset;
    rdCh = *++rdPtr;
  }

  /* plusSign[8] */
  smblPtr = ns_numberinfo(NS_PLUSSIGN, ns);
  smblCh   = *smblPtr;
  sdx = 0;
  do {
    if (smblCh != rdCh) {
      /* minusSign[8] */
      rdCh = rdPtr[(sdx = 0)];
      smblPtr = ns_numberinfo(NS_MINUSSIGN, ns);
      smblCh  = *smblPtr;
      do {
        if (smblCh != rdCh)  goto reset;
        smblCh = smblPtr[(++sdx)];
        if (smblCh == 0) {
          *rflags |= SIGNED_VALUE;
          break;
        }
        rdCh = rdPtr[sdx];
      } while (1);
      break;
    }
    smblCh = smblPtr[(++sdx)];
    if (smblCh == 0) {
      *rflags &= ~SIGNED_VALUE;
      break;
    }
    rdCh = rdPtr[sdx];
  } while (1);

  if ((*rflags & HAS_HCNT) != 0)  ++nelem;
  rdPtr += sdx;
  --nelem;

  /* found symbol, reload cnvrt for Rd function */
reset:
  cnvrt->cs_rd   = (void *)rdPtr;
  cnvrt->cs_rdSz = ((uint32_t)cnvrt->cs_rdSz & ~0xFFFF) | (nelem & 0xFFFF);
}

#pragma mark #### positional

static csEnum
gposRd(csCnvrt_st *cnvrt, uint8_t *ns,
       uint32_t *rflags, const uint8_t *dZero) {

  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr;
  uint8_t w;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  w = 0;
  if (*dZero == 2) {
    /* cant use 16bit subtraction, endianess problem */
    do {
      /* for 2-byte, 16bit match? */
      if ((rdPtr[0] == dZero[1])
          && ((uint8_t)(rdPtr[1] - dZero[2]) <= (uint8_t)9)) {
        indices[w++] = rdPtr[1] - dZero[2];
        rdPtr += 2;
        --nelem;
        if ((uint32_t)(nelem - (uint32_t)1) > nelem)  break;
      } else {
        uint8_t smblCh, sdx;
        /* verify grouping else done */
        if ((*rflags & GROUP_BIT) == 0)  goto gathered;
        smblCh  = smblPtr[(sdx = 0)];
        do {
          if (smblCh != rdPtr[sdx])  goto gathered;
          smblCh = smblPtr[(++sdx)];
        } while (smblCh != 0);
        /* grouping should not be part of nelem count */
        rdPtr += sdx;
      }
    } while (1);
  } else if (*dZero == 3) {
    const uint16_t dBase = *((uint16_t *)&dZero[1]);
    const uint8_t  dZ = dZero[3];
    do {
      /* for 3-byte, 16bit value match then 8 bit? */
      /* 2 reads, cmp, subtract */
      if ((*(uint16_t *)rdPtr == dBase)
          && ((uint8_t)(rdPtr[2] - dZ) <= (uint8_t)9)) {
        indices[w++] = rdPtr[2] - dZ;
        rdPtr += 3;
        --nelem;
        if ((uint32_t)(nelem - (uint32_t)1) > nelem)  break;
      } else {
        uint8_t smblCh, sdx;
        /* verify grouping else done */
        if ((*rflags & GROUP_BIT) == 0)  goto gathered;
        smblCh  = smblPtr[(sdx = 0)];
        do {
          if (smblCh != rdPtr[sdx])  goto gathered;
          smblCh = smblPtr[(++sdx)];
        } while (smblCh != 0);
        /* grouping should not be part of nelem count */
        rdPtr += sdx;
      }
    } while (1);
  } else {
      /* if (*dZero == 4) */
    uint8_t  dZ;
    uint32_t dBase = *(uint32_t *)&dZero[1];
#ifdef __LITTLE_ENDIAN__
    dBase = (dBase >> 16) | (dBase << 16);
    dBase = ((dBase & 0xFF00FF00) >> 8) | ((dBase & 0x00FF00FF) << 8);
#endif
    dZ = (uint8_t)dBase;
    do {
#ifdef __LITTLE_ENDIAN__
      uint32_t rdFix = *(uint32_t *)rdPtr;
      rdFix = (rdFix >> 16) | (rdFix << 16);
      rdFix = ((rdFix & 0xFF00FF00) >> 8) | ((rdFix & 0x00FF00FF) << 8);
      if ((uint32_t)(rdFix - dBase) <= (uint32_t)9)
#else
      if ((uint32_t)(*(uint32_t *)rdPtr - dBase) <= (uint32_t)9)
#endif
      {
        indices[w++] = rdPtr[3] - dZ;
        rdPtr += 4;
        --nelem;
        if ((uint32_t)(nelem - (uint32_t)1) > nelem)  break;
      } else {
        uint8_t smblCh, sdx;
        /* verify grouping else done */
        if ((*rflags & GROUP_BIT) == 0)  goto gathered;
        smblCh  = smblPtr[(sdx = 0)];
        do {
          if (smblCh != rdPtr[sdx])  goto gathered;
          smblCh = smblPtr[(++sdx)];
        } while (smblCh != 0);
        /* grouping should not be part of nelem count */
        rdPtr += sdx;
      }
    } while (1);
  }

gathered:
  indices[w] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

/* ٠١٢٣٤٥٦٧٨٩  Arabic
 * const uint16_t udigit[10] = {
 *   0x0660, 0x0661, 0x0662, 0x0663, 0x0664,
 *   0x0665, 0x0666, 0x0667, 0x0668, 0x0669
 * };
 * add 0x660 to indices, 0xD9,0xA0-A9
 */
csEnum
arabRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x02\xD9\xA0");
}

/* ۰۱۲۳۴۵۶۷۸۹  Persian
 * const uint16_t udigit[10] = {
 *   0x06F0, 0x06F1, 0x06F2, 0x06F3, 0x06F4,
 *   0x06F5, 0x06F6, 0x06F7, 0x06F8, 0x06F9
 * };
 * add 0x6F0 to indices, 0xDB,0xB0-B9
 */
csEnum
arabextRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x02\xDB\xB0");
}

/* ০১২৩৪৫৬৭৮৯  Bengali
 * const uint16_t udigit[10] = {
 *   0x09E6, 0x09E7, 0x09E8, 0x09E9, 0x09EA,
 *   0x09EB, 0x09EC, 0x09ED, 0x09EE, 0x09EF
 * };
 * add 0x9E6 to indices, 0xE0,0xA7,0xA6-AF
 */
csEnum
bengRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA7\xA6");
}

/* 𑄶𑄷𑄸𑄹𑄺𑄻𑄻𑄽𑄾𑄿  Chakma
 * const uint16_t udigit[10] = {
 *   0x2B80, 0x2B81, 0x2B82, 0x2B83, 0x2B84,
 *   0x2B85, 0x2B86, 0x2B87, 0x2B88, 0x2B89
 * };
 * add 0x2B80 to indices, 0xF0,0x91,0x84,0xB6-BF
 */
csEnum
cakmRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x04\xF0\x91\x84\xB6");
}

/* ०१२३४५६७८९  Devanagari
 * const uint16_t udigit[10] = {
 *   0x0966, 0x0967, 0x0968, 0x0969, 0x096A,
 *   0x096B, 0x096C, 0x096D, 0x096E, 0x096F
 * };
 * add 0x0966 to indices, 0xE0,0xA5,0xA6-AF
 */
csEnum
devaRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA5\xA6");
}

/* ૦૧૨૩૪૫૬૭૮૯  Gujarati
 * const uint16_t udigit[10] = {
 *   0x0AE6, 0x0AE7, 0x0AE8, 0x0AE9, 0x0AEA,
 *   0x0AEB, 0x0AEC, 0x0AED, 0x0AEE, 0x0AEF
 * };
 * add 0x0AE6 to indices, 0xE0,0xAB,0xA6-AF
 */
csEnum
gujrRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAB\xA6");
}

/* ੦੧੨੩੪੫੬੭੮੯  Gurmukhī
 * const uint16_t udigit[10] = {
 *   0x0A66, 0x0A67, 0x0A68, 0x0A69, 0x0A6A,
 *   0x0A6B, 0x0A6C, 0x0A6D, 0x0A6E, 0x0A6F
 * };
 * add 0x0A66 to indices, 0xE0,0xA9,0xA6-AF
 */
csEnum
guruRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xA9\xA6");
}

/* ០១២៣៤៥៦៧៨៩  Khmer
 * const uint16_t udigit[10] = {
 *   0x17E0, 0x17E1, 0x17E2, 0x17E3, 0x17E4,
 *   0x17E5, 0x17E6, 0x17E7, 0x17E8, 0x17E9
 * };
 * add 0x17E0 to indices, 0xE1,0x9F,0xA0-A9
 */
csEnum
khmrRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE1\x9F\xA0");
}

/* ꧐꧑꧒꧓꧔꧕꧖꧗꧘꧙  Javanese
 * const uint16_t udigit[10] = {
 *   0xA9D0, 0xA9D1, 0xA9D2, 0xA9D3, 0xA9D4,
 *   0xA9D5, 0xA9D6, 0xA9D7, 0xA9D8, 0xA9D9
 * };
 * add 0xA9D0 to indices, 0xEA,0xA7,0x90-99
 */
csEnum
javaRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xEA\xA7\x90");
}

/* ೦೧೨೩೪೫೬೭೮೯  Kannada
 * const uint16_t udigit[10] = {
 *   0x0CE6, 0x0CE7, 0x0CE8, 0x0CE9, 0x0CEA,
 *   0x0CEB, 0x0CEC, 0x0CED, 0x0CEE, 0x0CEF
 * };
 * add 0x0CE6 to indices, 0xE0,0xB3,0xA6-AF
 */
csEnum
kndaRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB3\xA6");
}

/* ໐໑໒໓໔໕໖໗໘໙  Lao
 * const uint16_t udigit[10] = {
 *   0x0ED0, 0x0ED1, 0x0ED2, 0x0ED3, 0x0ED4,
 *   0x0ED5, 0x0ED6, 0x0ED7, 0x0ED8, 0x0ED9
 * };
 * add 0x0ED0 to indices, 0xE0,0xBB,0x90-99
 */
csEnum
laooRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xBB\x90");
}

/* ൦൧൨൩൪൫൬൭൮൯   Malayalam
 * const uint16_t udigit[10] = {
 *   0x0D66, 0x0D67, 0x0D68, 0x0D69, 0x0D6A,
 *   0x0D6B, 0x0D6C, 0x0D6D, 0x0D6E, 0x0D6F
 * };
 * add 0x0D66 to indices, 0xE0,0xB5,0xA6-AF
 */
csEnum
mlymRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB5\xA6");
}

/* ၀၁၂၃၄၅၆၇၈၉  Myanmar (Burmese)
 * const uint16_t udigit[10] = {
 *   0x1040, 0x1041, 0x1042, 0x1043, 0x1044,
 *   0x1045, 0x1046, 0x1047, 0x1048, 0x1049
 * };
 * add 0x1040 to indices, 0xE1,0x81,0x80-89
 */
csEnum
mymrRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE1\x81\x80");
}

/* ୦୧୨୩୪୫୬୭୮୯  Oriya
 * const uint16_t udigit[10] = {
 *   0x0B66, 0x0B67, 0x0B68, 0x0B69, 0x0B6A,
 *   0x0B6B, 0x0B6C, 0x0B6D, 0x0B6E, 0x0B6F
 * };
 * add 0x0B66 to indices, 0xE0,0xAD,0xA6-AF
 */
csEnum
oryaRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAD\xA6");
}

/* ௦௧௨௩௪௫௬௭௮௯  Tamil
 * const uint16_t udigit[10] = {
 *   0x0BE6, 0x0BE7, 0x0BE8, 0x0BE9, 0x0BEA,
 *   0x0BEB, 0x0BEC, 0x0BED, 0x0BEE, 0x0BEF
 * };
 * add 0x0BE6 to indices, 0xE0,0xAF,0xA6-AF
 */
csEnum
tamldecRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xAF\xA6");
}

/* ౦౧౨౩౪౫౬౭౮౯  Telugu
 * const uint16_t udigit[10] = {
 *   0x0C66, 0x0C67, 0x0C68, 0x0C69, 0x0C6A,
 *   0x0C6B, 0x0C6C, 0x0C6D, 0x0C6E, 0x0C6F
 * };
 * add 0x0C66 to indices, 0xE0,0xB1,0xA6-AF
 */
csEnum
teluRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB1\xA6");
}

/* ๐๑๒๓๔๕๖๗๘๙  Thai
 * const uint16_t udigit[10] = {
 *   0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54,
 *   0x0E55, 0x0E56, 0x0E57, 0x0E58, 0x0E59
 * };
 * add 0x0E50 to indices, 0xE0,0xB9,0x90-99
 */
csEnum
thaiRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xB9\x90");
}

/* ༠༡༢༣༤༥༦༧༨༩  Tibetan
 * const uint16_t udigit[10] = {
 *   0x0F20, 0x0F21, 0x0F22, 0x0F23, 0x0F24,
 *   0x0F25, 0x0F26, 0x0F27, 0x0F28, 0x0F29
 * };
 * add 0x0F20 to indices, 0xE0,0xBC,0xA0-A9
 */
csEnum
tibtRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xE0\xBC\xA0");
}

/* ꘠꘡꘢꘣꘤꘥꘦꘧꘨꘩  Vai
 * const uint16_t udigit[10] = {
 *   0xA620, 0xA621, 0xA622, 0xA623, 0xA624,
 *   0xA625, 0xA626, 0xA627, 0xA628, 0xA629
 * };
 * add 0xA620 to indices, 0xEA,0x98,0xA0-A9
 */
csEnum
vaiiRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {
  return gposRd(cnvrt, ns, rflags, (const uint8_t *)"\x03\xEA\x98\xA0");
}

#pragma mark #### algorithmic

csEnum
armnRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Armenian
   * ԱԲԳԴԵԶԷԸԹ, 1-9           0xD4,0xB1-B9
   * ԺԻԼԽԾԿՀՁՂ, 10 - 90       0xD4,0xBA-BF 0xD5,0x80-82
   * ՃՄՅՆՇՈՉՊՋ, 100 - 900     0xD5,0x83-8B
   * ՌՍՎՏՐՑՒՓՔ, 1000 - 9000   0xD5,0x8C-94
   * Ա̅ = 10000  combining overline == 0xCC 0x85
   * unknown when >= 100000000, assume prefix multiples of 9999.
   */

  const uint8_t  dZero[2] = { 0xE2, 0x81 };
  /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t rdCh, tthou, gdx;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* <zero> == <mathematical space> */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }
  /* for location into indices, groups of 10000s have to be read */
  /* each advance of iPtr, (4), must be accompanied by "zeroing" of group */
  iPtr = indices;
  indices[3] = (indices[2] = (indices[1] = (indices[0] = 0)));
  /* for determining thous mode, conditional flag */
  tthou = 0;
  /* index value, power of ten */
  gdx = 0;

  if ((rdCh == (uint8_t)'\xE2') &&
                      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  goto gathered;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
                      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));

smbl_checked:
  if (rdPtr[0] == 0)  goto gathered;
  /* grab 2nd uint8_t, value that determines index */
  rdCh = rdPtr[1];
  /* two prefix groups */
  /* 1 - 60 */
  if ((rdPtr[0] == 0xD4)
      && ((uint8_t)(rdCh - (uint8_t)0xB1) <= (uint8_t)0x0E)) {
    if ((uint8_t)(rdCh - (uint8_t)0xB1) <= (uint8_t)0x08) {  /* 1 - 9 */
      gdx = 3;
      rdCh -= 0xB1 - 0x01;
      if (iPtr[3] != 0)
        goto has_previous_entry;
      goto J10000;
    }
    /* 10 - 60 */
    gdx = 2;
    rdCh -= 0xBA - 0x01;
    if ((iPtr[3] | iPtr[2]) != 0)
      goto has_previous_entry;
    goto J10000;
  }
  /* 70 - 9000 */
  if ((rdPtr[0] == 0xD5)
      && ((uint8_t)(rdCh - (uint8_t)0x80) <= (uint8_t)0x14)) {
    if ((uint8_t)(rdCh - (uint8_t)0x80) <= (uint8_t)0x02) {  /* 70, 80, 90 */
      gdx = 2;
      rdCh -= 0x80 - 0x07;
      /* make sure a lower power of ten not entered yet */
      if ((iPtr[3] | iPtr[2]) != 0)
        goto has_previous_entry;
      goto J10000;
    }
    if ((uint8_t)(rdCh - (uint8_t)0x83) <= (uint8_t)0x08) {  /* 100 - 900 */
      gdx = 1;
      rdCh -= 0x83 - 0x01;
      /* make sure a lower power of ten not entered yet */
      if ((iPtr[3] | iPtr[2] | iPtr[1]) != 0)
        goto has_previous_entry;
      goto J10000;
    }
    /* 1000 - 9000 */
    gdx = 0;
    rdCh -= 0x8C - 0x01;
    if ((iPtr[3] | iPtr[2] | iPtr[1] | iPtr[0]) != 0) {
has_previous_entry:
      if (tthou == 0)  goto gathered;
      /* switch to singles, or another set of tens of thousands */
      iPtr += 4;  /* ADVANCE_RDPTR; */
      iPtr[3] = (iPtr[2] = (iPtr[1] = (iPtr[0] = 0)));
      tthou = 0;
    }
J10000:
    /* iPtr set to index placement for group of 10000s */
    if (tthou == 1) {
      if (rdPtr[2] != 0xCC) {
        /* end group before placement */
        iPtr += 4;
        iPtr[3] = (iPtr[2] = (iPtr[1] = (iPtr[0] = 0)));
        tthou = 0;
      }
    }
    iPtr[gdx] = rdCh;
    /* check for ten thousands mark */
    if (rdPtr[2] == 0) {
      rdPtr += 2;
      goto gathered;
    }
    if ((rdPtr[2] == 0xCC) && (rdPtr[3] == 0x85)) {
      rdPtr += 4;
      tthou = 1;
      goto glyph_read;
    }
    /* did not have ten thousands mark, advance read */
    rdPtr += 2;
    tthou = 0;
glyph_read:
    --nelem;
    if ((uint32_t)(nelem - (uint32_t)1) > nelem)  goto gathered;
    goto smbl_checked;
  }
  /* verify grouping else done */
  if ((*rflags & GROUP_BIT) != 0) {
    uint8_t smblCh, sdx;
    smblCh  = smblPtr[(sdx = 0)];
    do {
      if (smblCh != rdPtr[sdx])  goto gathered;
      smblCh = smblPtr[(++sdx)];
    } while (smblCh != 0);
    /* grouping should not be part of nelem count */
    rdPtr += sdx;
    goto smbl_checked;
  }
  /* <zero> == <mathematical space> */
  if ((*(uint16_t *)rdPtr == ((uint16_t *)dZero)[0]) && (rdPtr[2] == 0x9F)) {
    rdPtr += 3;
    /* check for ten thousands mark */
    if ((rdPtr[0] == 0xCC) && (rdPtr[1] == 0x85)) {
      rdPtr += 2;
      if (tthou) {
        iPtr += 4;
        iPtr[3] = (iPtr[2] = (iPtr[1] = (iPtr[0] = 0)));
        iPtr += 4;
        iPtr[3] = (iPtr[2] = (iPtr[1] = (iPtr[0] = 0)));
        tthou = 0;
      }
      goto glyph_read;
    }
  }

gathered:
  if (tthou) {
    iPtr += 4;
    iPtr[3] = (iPtr[2] = (iPtr[1] = (iPtr[0] = 0)));
  }
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
cyrlRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* special note: original design added <space to combing mark. This was
   * believed needed as mac only displayed correctly with. After more research
   * and running on ryzen, this is mac problem, not actual coding problem.
   * Altering for correct coding, 19-06-12
   */
  /*
   * Additive system  Church Slavic (Cryrillic)
   *  авгдєѕзиѳ,  1 - 9      0xD0 B0,B2,B3,B4 0xD1 94,95 0xD0 B7,B8 0xD1 B3
   *  АВГДЄЅЗИѲ
   *  іклмнѯопч,  10 - 90    0xD1 96 0xD0 BA,BB,BC,BD 0xD1 AF 0xD0 BE,BF 0xD1 87
   *  ІКЛМНѮѺПЧ  optional 90?(Ҁ) 0xD2 81
   *  рстѵфхѱѽц,  100 - 900  0xD1 80,81,82 0xD1 B5 0xD1 84,85 0xD1 B1,BD 0xD1 86
   *  РСТѴФХѰѾЦ
   *
   * exception: 11-19 the і follows the single digit
   *
   *  CYRILLIC  A,VE,GHE,DE,ukrainian IE,DZE,ZE,I,FITA
   *  CYRILLIC  DOTTED I,KA,EL,EM,EN,KSI,O,PE,CHE
   *  CYRILLIC  ER,ES,TE,IZHITSA,EF,KHA,PSI,OMEGA with TITLO,TSE
   *
   *  single digit, or letter considered digit combining mark titlo
   *   ҃  0xD2 83
   *  thousands prefix
   *  ҂  0xD2 82
   *  powers combining marks
   *  powers 10^4,10^5,10^6,10^7,10^8
   *  ^4  ^5  ^6  ^7  ^8  ^9
   *    ⃝ ,   ҈,   ҉,   ꙰,   ꙱,   ꙲   0xE2 0x83 9D, 0xD2 88,89, 0xEA 0x99 B0,B1,B2
   *  0 = mathematical space = \xe2 \x81 \x9f
   */

    /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh;

    /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

    /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* <zero> == <mathematical space> */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  /* for location into indices, groups of 10000s have to be read */
  /* each advance of iPtr, (4), must be accompanied by "zeroing" of group */

  /* will encounter combining marked glyphs [mark,glyph] */
  /* system reads as single digits w/ or w/o multipliers.
   * generally reads highest to lowest, it says */
  /* system w/o exponents ranges [1-999*10^9]. 12 digits.
   * clear 12 */
  iPtr = indices;
  *(uint32_t*)&indices[8] = 0;
  *(uint32_t*)&indices[4] = 0;
  *(uint32_t*)&indices[0] = 0;

    /* mathematical space == '0' */
  if ((rdCh == (uint8_t)'\xE2') &&
                      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  goto done;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
                      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));

  do {
    uint8_t numeral, power;
    power = 0, numeral = 0;
    if ((uint8_t)(*rdPtr - 0xD0) > (uint8_t)1) {
      if (*rdPtr == 0)  goto done;
      if (*(uint16_t*)rdPtr == cconst16_t('\xD2','\x82')) { /* ҂ thousand prefix */
        power = 3;
        rdPtr += 2;
      } else if ((*rflags & GROUP_BIT) != 0) {    /* grouping */
        uint8_t smblCh, sdx;
        smblCh  = smblPtr[(sdx = 0)];
        do {
          if (smblCh != rdPtr[sdx])  goto done;
          smblCh = smblPtr[(++sdx)];
        } while (smblCh != 0);
          /* grouping should not be part of nelem count */
        rdPtr += sdx;
        continue;
      } else {
        goto done;
      }
    }
      /* only digits (firstly) */
    if (*rdPtr == 0xD0) {
        /* powers 1,0  power += since could be prefixed */
      if ((uint8_t)(rdPtr[1] - 0xB0) < (uint8_t)9) {
          /* авгд  зи */ /* 1234  78   x1 */
        numeral = rdPtr[1] - 0xB0;
        if (numeral == 0)  numeral++;
        if ((numeral == 5) || (numeral == 6))  goto done;  /* case power 3 */
        power += 0;
      } else if ((uint8_t)(rdPtr[1] - 0xBA) < (uint8_t)6) {
          /*  клмн оп  */ /*  2345 78   x10 */
        numeral = rdPtr[1] - 0xB8 + (rdPtr[1] > 0xBD);
        power += 1;
      } else {
        goto done;
      }
    } else {
        /* powers 2,1,0  power += since could be prefixed */
      if ((uint8_t)(rdPtr[1] - 0x80) < (uint8_t)8) {
          /* рст фх  ц */ /* 123 56  9  x100 */ /* ч */ /* 9  x10 */
        numeral = rdPtr[1] - 0x7F;
        power += 2;
        if (numeral == 4)  goto done;  /* case power 3 */
        if (numeral == 7)  numeral += 2;
        if (numeral == 8)  numeral += 1, power--;
      } else if ((uint8_t)(rdPtr[1] - 0x94) < (uint8_t)3) {
          /* єѕ */ /* 56  x1 */ /* і */ /* 1  x10 */
        numeral = rdPtr[1] - 0x8F;
        if (numeral == 7)  numeral = 1, power += 1;
      } else if (rdPtr[1] == 0xB1) {  /* ѱ */ /* 7  x100 */
        numeral = 7, power += 2;
      } else if (rdPtr[1] == 0xB3) {  /* ѳ */ /* 9  x1 */
        numeral = 9, power += 0;
      } else if (rdPtr[1] == 0xB5) {  /* ѵ */ /* 4  x100 */
        numeral = 4, power += 2;
      } else if (rdPtr[1] == 0xBD) {  /* ѽ */ /* 8  x100 */
        numeral = 8, power += 2;
      } else if (rdPtr[1] == 0xAF) {  /* ѯ */ /* 6  x10 */
        numeral = 6, power += 1;
      } else if (*(uint16_t*)rdPtr == cconst16_t('\xD2','\x81')) {  /*  optional 90 (Ҁ)  */
        numeral = 9, power += 1;
      } else {
        goto done;
      }
    }
      /* only here after digit parsed */
    rdPtr += 2;
    nelem--;
      /* combining marks (powers), prefixed 1000 checked prior */
    if (power >= 3) {
      iPtr[(11 - (3 + 0))] = numeral;
      continue;
    }
    if (*rdPtr == 0xD2) {
      if (*(uint16_t*)rdPtr == cconst16_t('\xD2','\x83')) {  /* single mark */
        iPtr[11] = numeral;
        rdPtr += 2;
        goto done;
      }
      if (*(uint16_t*)rdPtr == cconst16_t('\xD2','\x88')) {  /*   ҈  ^5 */
        iPtr[(11 - (5 + power))] = numeral;
        rdPtr += 2;
        continue;
      }
      if (*(uint16_t*)rdPtr == cconst16_t('\xD2','\x89')) {  /*   ҉ ^6 */
        iPtr[(11 - (6 + power))] = numeral;
        rdPtr += 2;
        continue;
      }
      nelem = 0;
      goto bad_code;
    }
    if (*(uint16_t*)rdPtr == cconst16_t('\xE2','\x83')) {
      if (rdPtr[2] == 0x9D) {  /*   ⃝  ^4 */
        iPtr[(11 - (4 + power))] = numeral;
        rdPtr += 3;
        continue;
      }
      nelem = 0;
      goto bad_code;
    }
    if (*(uint16_t*)rdPtr == cconst16_t('\xEA','\x99')) {
      if ((uint8_t)(rdPtr[2] - 0xB0) < (uint8_t)3) {  /*    ꙰,   ꙱,   ꙲ ^7  ^8  ^9 */
        iPtr[(11 - ((rdPtr[2] - 0xA9) + power))] = numeral;
        rdPtr += 3;
        continue;
      }
      nelem = 0;
    }
bad_code:
    iPtr[(11 - power)] = numeral;
  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  iPtr[12] = 0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
    /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
ethiRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*
   * ፩፪፫፬፭፮፯፰፱,   x1      0xE1,0x8D,0xA9-B1  (1-9)
   * ፲፳፴፵፶፷፸፹፺,  x10      0xE1,0x8D,0xB2-BA  (10 - 90)
   * ፻,         x100     0xE1,0x8D,0xBB     (suffix)
   * ፼,         x10000   0xE1,0x8D,0xBC     (suffix)
   *
   * 1-9, 10-90, x100, x10000
   * x100   => iPtr += 2
   * x10000 => iPtr += 4
   *
   * 9900:
   * 90 iPtr[0++], 9 iPtr[1(0++++)], x100 iPtr += 2 @ 4, iPtr = 0xFF;
   * 9900\xFF
   */

  /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* has <zero>, so error (csWR)? */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  /* for location into indices, groups of 10000s have to be read */
  /* each advance of iPtr, (4), must be accompanied by "zeroing" of group */
  iPtr = indices;
  indices[3] = (indices[2] = (indices[1] = (indices[0] = 0)));

  if ((rdCh == (uint8_t)'\xE2') &&
                (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F'))) {
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  goto gathered;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
                (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));
  }

  /* read into 10's/1's slots, if suffix move read into 1000's/100's */
  while (rdPtr[0] != 0) {
    if ((*(uint16_t *)rdPtr == cconst16_t('\xE1','\x8D'))
        && ((uint8_t)(rdPtr[2] - (uint8_t)0xA9) <= (uint8_t)0x13)) {
        /* all numeric glyphs in this range */
      rdCh = rdPtr[2] - (uint8_t)0xA9 + 0x01;
      if        (rdCh < 10) {
        if (iPtr[3] != 0)  goto gathered;
        iPtr[3] = rdCh;                 /* 1-9 */
      } else if (rdCh < 19) {
        if (iPtr[2] != 0)  goto gathered;
        iPtr[2] = rdCh - 9;             /* 1-9 x10 */
      } else if (rdCh == 19) {
          /* x100 glyph                    ፻ */
        *(uint16_t*)&iPtr[0] = *(uint16_t*)&iPtr[2];
        *(uint32_t*)&iPtr[2] = 0;
        /* counter the deincrement below */
        nelem++;
      } else {
          /* x10000 glyph                   ፼ */
        *(uint32_t*)&iPtr[4] = 0;
        iPtr += 4;
        /* counter the deincrement below */
        nelem++;
      }
    } else if ((*(uint16_t *)rdPtr == cconst16_t('\xE2','\x81'))
                                            && (rdPtr[2] == 0x9F)) {
        /* zero, <zero> == <mathematical space> */
      /* fall thru */
    } else {
      uint8_t smblCh, sdx;
        /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto gathered;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto gathered;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
      /* common with all digits */
    rdPtr += 3;
    --nelem;
    if ((uint32_t)(nelem - (uint32_t)1) > nelem) {
      /* cases: suffix glyph */
      while ((*(uint16_t *)rdPtr == cconst16_t('\xE1','\x8D'))
          && ((rdCh = (rdPtr[2] - (uint8_t)0xBB)) <= (uint8_t)0x1)) {
        if (rdCh == 0) {
          /* x100 glyph                    ፻ */
          *(uint16_t*)&iPtr[0] = *(uint16_t*)&iPtr[2];
          *(uint32_t*)&iPtr[2] = 0;
        } else {
          /* x10000 glyph                   ፼ */
          *(uint32_t*)&iPtr[4] = 0;
          iPtr += 4;
        }
        rdPtr += 3;
      }
      break;
    }
  }

gathered:
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
georRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Georgian
   * აბგდევზჱთ,   1-9            0xE1,0x83,[0x90-96 - 0xB1 - 0x97]
   * იკლმნჲოპჟ,   10 - 90        0xE1,0x83,[0x98-9C - 0xB2 - 0x9D-9F]
   * რსტუფქღყშ,   100 - 900      0xE1,0x83,[0xA0-A8]
   * ჩცძწჭხჴჯჰ,   1000 - 9000    0xE1,0x83,[0xA9-AE - 0xB4 - 0xAF-B0]
   * ჵ,           10000          0xE1,0x83,0xB5  (suffix, can be solo)
   * ჳ additional 400            0xE1,0x83,0xB3
   * assume repeating 10000
   */

  /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, power;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* <zero> == <mathematical space> */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  /* for location into indices, groups of 10000s have to be read */
  /* each advance of iPtr, (4), must be accompanied by "zeroing" of group */
  iPtr = indices;
  indices[3] = (indices[2] = (indices[1] = (indices[0] = 0)));

  if ((rdCh == (uint8_t)'\xE2') &&
              (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F'))) {
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  goto done;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
              (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));
  }

  power = 4;
  do {
      /* grouping */
    if ((*rflags & GROUP_BIT) != 0) {
      uint8_t smblCh, sdx;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto done;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
    }
      /* all glyphs must be in this range */
    if ((rdPtr[0] != 0xE1) || (rdPtr[1] != 0x83) ||
            ((uint8_t)(rdCh = rdPtr[2] - 0x90) > 0x25))  goto done;
      /* 10^0 digits */
    if (rdCh < 8) {    /* [90 - 97] - 0x90 */
      if ((power == 0) && (iPtr[3] != 0))  goto done;
      power = 0;
      if (rdCh == 7)  iPtr[3] = rdCh + 2;
      else            iPtr[3] = rdCh + 1;
      goto cntnu;
    }
    if (rdCh == 33) {  /* [B1] - 0x90 */
      if ((power == 0) && (iPtr[3] != 0))  goto done;
      power = 0;
      iPtr[3] = 8;
      goto cntnu;
    }
      /* 10^1 digits */
    if (rdCh < 16) {  /* [98-9F] - 0x90 */
      if ((power <= 1) || (iPtr[2] != 0))  goto done;
      power = 1;
      if (rdCh < 13)  iPtr[2] = rdCh - 7;
      else            iPtr[2] = rdCh - 6;
      goto cntnu;
    }
    if (rdCh == 34) {  /* [B2] - 0x90 */
      if ((power <= 1) || (iPtr[2] != 0))  goto done;
      power = 1;
      iPtr[2] = 6;
      goto cntnu;
    }
      /* 10^2 digits */
    if (rdCh < 25) {  /* [A0-A8] - 0x90 */
      if ((power <= 2) || (iPtr[1] != 0))  goto done;
      power = 2;
      iPtr[1] = rdCh - 15;
      goto cntnu;
    }
      /* 10^3 digits */
    if (rdCh < 33) {  /* [A9-B0] - 0x90 */
      if ((power <= 3) || (iPtr[0] != 0))  goto done;
      power = 3;
      if (rdCh < 31)  iPtr[0] = rdCh - 24;
      else            iPtr[0] = rdCh - 23;
      goto cntnu;
    }
    if (rdCh == 36) {  /* [B4] - 0x90 */
      if ((power <= 3) || (iPtr[0] != 0))  goto done;
      power = 3;
      iPtr[0] = 7;
      goto cntnu;
    }
      /* 10^2 0ptional 400 digit */
    if (rdCh == 35) {  /* [B3] - 0x90 */
      if ((power <= 2) || (iPtr[1] != 0))  goto done;
      power = 2;
      iPtr[1] = 4;
      goto cntnu;
    }
      /* 10^4 suffix or solo rdCh == 37 */
    if ((iPtr == indices) && (*(uint32_t*)iPtr == 0)) {
      iPtr[3] = 1;
        /* cancel the cancel of element counter */
      nelem--;
    }
    power = 4;
    iPtr += 4;
    *(uint32_t*)iPtr = 0;
      /* cancel element counter */
    nelem++;
  cntnu:
    rdPtr += 3;
    nelem--;
  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
grekRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive system  Greek
   * ΑΒΓΔΕϚΖΗΘ,   1 - 9          0xCE,[0x91-95]  0xCF,0x9A  0xCE,[0x96-98]
   * ΙΚΛΜΝΞΟΠϞ,   10 - 90        0xCE,[0x99-A0]  0xCF,0x9E
   * ΡΣΤΥΦΧΨΩϠ,   100 - 900      0xCE,0xA1  0xCE,[0xA3-A9]  0xCF,0xA0
   * ͵             1000 prefix   (0x0374) 0xCD,0xB5
   * ʹ            numeric suffix (0x0375) 0xCD.0xB4
   */

  const uint8_t   thou[2] = { 0xCD, 0xB5 };
  const uint8_t   nEnd[2] = { 0xCD, 0xB4 };
  const uint8_t  dZero[2] = { 0xE2, 0x81 };
  /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, tIncr;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  /* all numbers end with "ʹ" numeric end, increase nelem by 1 */
  ++nelem;
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  /* for location into indices, groups of 1000s have to be read */
  /* iPtr to point to tail, tIncr used to index group of thousands */
  iPtr = &indices[3];
  indices[2] = (indices[1] = (indices[0] = 0));
  tIncr = 3;

  /* pass 1, the max 1000 prefix will be first parsed */
  /* thousands prefix */
  if (*(uint16_t *)rdPtr == *(uint16_t *)thou) {
    do {
      rdPtr += 2;
      if ((rdPtr[0] == 0) || ((tIncr += 3) > 44))  goto gathered;
      /* first pass difference */
      iPtr[2] = (iPtr[1] = (iPtr[0] = 0));
      iPtr += 3;
    } while (*(uint16_t *)rdPtr == *(uint16_t *)thou);
  }

smbl_checked:
  if (rdPtr[0] == 0)  goto gathered;
  rdCh = rdPtr[1];
  if (rdPtr[0] == 0xCE) {
    if ((uint8_t)(rdCh - (uint8_t)0x91) <= (uint8_t)0x07) {
      rdCh -= (uint8_t)0x91 - (uint8_t)0x01;
      if (rdCh < 6)  iPtr[(2 - (int32_t)tIncr)] = rdCh;
      else           iPtr[(2 - (int32_t)tIncr)] = rdCh + 0x01;
      goto glyph_read;
    }
    if ((uint8_t)(rdCh - (uint8_t)0x99) <= (uint8_t)0x07) {
      rdCh -= (uint8_t)0x99 - (uint8_t)0x01;
      iPtr[(1 - (int32_t)tIncr)] = rdCh;
      goto glyph_read;
    }
    if ((uint8_t)(rdCh - (uint8_t)0xA1) <= (uint8_t)0x08) {
      rdCh -= (uint8_t)0xA1 - (uint8_t)0x01;
      if (rdCh == 1) {
        iPtr[(0 - (int32_t)tIncr)] = rdCh;
        goto glyph_read;
      }
      if (rdCh == 2)  goto gathered;
      iPtr[(0 - tIncr)] = rdCh - 0x01;
      goto glyph_read;
    }
  }
  if (rdPtr[0] == 0xCF) {
    if (rdCh == 0x9A) iPtr[(2 - (int32_t)tIncr)] = 6;
    if (rdCh == 0x9E) iPtr[(1 - (int32_t)tIncr)] = 9;
    if (rdCh == 0xA0) iPtr[(0 - (int32_t)tIncr)] = 9;
  glyph_read:
    rdPtr += 2;
    --nelem;
    if ((uint32_t)(nelem - (uint32_t)1) > nelem)  goto gathered;
    tIncr = 3;
    goto smbl_checked;
  }
  if (*(uint16_t *)rdPtr == *(uint16_t *)thou) {
    /* thousands prefix */
    do {
      rdPtr += 2;
      /* error checking */
      if ((rdPtr[0] == 0) || ((tIncr += 3) > 43))  goto gathered;
    } while (*(uint16_t *)rdPtr == *(uint16_t *)thou);
    goto smbl_checked;
  }
  if (*(uint16_t *)rdPtr == *(uint16_t *)nEnd) {
    /* numeric suffix, end of number */
    rdPtr += 2;
    goto gathered;
  }
  /* verify grouping else done */
  if ((*rflags & GROUP_BIT) != 0) {
    uint8_t smblCh, sdx;
    smblCh  = smblPtr[(sdx = 0)];
    do {
      if (smblCh != rdPtr[sdx])  goto gathered;
      smblCh = smblPtr[(++sdx)];
    } while (smblCh != 0);
    /* grouping should not be part of nelem count */
    rdPtr += sdx;
    goto smbl_checked;
  }
  /* <zero> == <mathematical space> */
  if ((*(uint16_t *)rdPtr == ((uint16_t *)dZero)[0]) && (rdPtr[2] == 0x9F))
    rdPtr += 3;

gathered:
  iPtr[0] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
hanidecRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* 〇一二三四五六七八九  Chinese decimal variation (latn style)
   * const uint8_t u8[10][3] = {
   *  0xE3,0x80,0x87, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
   *  0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
   *  0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
   * };
   */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr;
  uint8_t  w;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  w = 0;
  do {
    if (rdPtr[0] == 0xE3) {
      if ((rdPtr[1] != 0x80) || (rdPtr[2] != 0x87))  goto check_symbol;
      indices[w++] = 0;
      goto glyph_read;
    } else if (rdPtr[0] == 0xE4) {
      if (rdPtr[1] == 0xB8) {
        if (rdPtr[2] == 0x80) {  indices[w++] = 1;  goto glyph_read;  }
        if (rdPtr[2] == 0x83) {  indices[w++] = 7;  goto glyph_read;  }
        if (rdPtr[2] == 0x89) {  indices[w++] = 3;  goto glyph_read;  }
        goto check_symbol;
      }
      if (rdPtr[1] == 0xB9) {
        if (rdPtr[2] == 0x9D) {  indices[w++] = 9;  goto glyph_read;  }
        goto check_symbol;
      }
      if (rdPtr[1] == 0xBA) {
        if (rdPtr[2] == 0x8C) {  indices[w++] = 2;  goto glyph_read;  }
        if (rdPtr[2] == 0x94) {  indices[w++] = 5;  goto glyph_read;  }
        goto check_symbol;
      }
    } else if (rdPtr[0] == 0xE5) {
      if (rdPtr[1] == 0x85) {
        if (rdPtr[2] == 0xAB) {  indices[w++] = 8;  goto glyph_read;  }
        if (rdPtr[2] == 0xAD) {  indices[w++] = 6;  goto glyph_read;  }
        goto check_symbol;
      }
      if ((rdPtr[1] != 0x9B) || (rdPtr[2] != 0x9B))  goto check_symbol;
      indices[w++] = 4;
      goto glyph_read;
    } else {
      uint8_t smblCh, sdx;
  check_symbol:
      /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto gathered;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto gathered;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
      /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
  glyph_read:
    rdPtr += 3;
    --nelem;
    if ((uint32_t)(nelem - (uint32_t)1) > nelem)  break;
  } while (1);

gathered:
  indices[w] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
hansRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Simplified Chinese
   * 零一二三四五六七八九
   * 十百千万亿 10,10^2,10^3,10^4,10^8
   * 兆京垓秭穰沟涧  12,16,20,24,28,32,36  2^128 goes to 38
   */

  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, inner_power, major_power;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

    /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* has <zero>, so error (csWR)? */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  iPtr = indices;
    /* start in indices[0-3] group 1 */
  *(uint64_t*)iPtr = 0;

  if ((rdCh == (uint8_t)'\xE2') &&
                        (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  return csRD;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
                        (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));

  inner_power = 4;
  major_power = 38;
  do {
    uint8_t err1 = 0, err2 = 0;
    uint8_t numeral = 1;
    uint8_t power = major_power;

    if ((uint8_t)(rdPtr[0] - 0xE4) >= 4) {
      uint8_t smblCh, sdx;
        /* invisible punctuation for '10' differeniation */
      if ((rdPtr[0] == 0xE2) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))) {
        rdPtr += 3;
        goto done;
      }
        /* <zero> */
      if ((rdPtr[0] == 0xE9) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\xB6'))) {
        if ((iPtr == indices) && (*(uint32_t*)iPtr == 0))  rdPtr += 3;
        goto done;
      }
        /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto done;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto done;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
    /* parse numeral
     const uint8_t u8[10][3] = {
       0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
       0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
       0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x80'))  numeral = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x83'))  numeral = 7;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x89'))  numeral = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB9','\x9D'))  numeral = 9;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x8C'))  numeral = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x94'))  numeral = 5;
      else  err1 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAB'))  numeral = 8;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAD'))  numeral = 6;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\x9B'))  numeral = 4;
      else  err1 = 1;
    } else {
      err1 = 1;
    }
    if (err1 == 0)  rdPtr += 3, --nelem;

    /* parse power
     const uint8_t p8[12][3] = {
       0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE4,0xB8,0x87,
       0xE4,0xBA,0xBF, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
       0xE7,0xA7,0xAD, 0xE7,0xA9,0xB0, 0xE6,0xB2,0x9F, 0xE6,0xB6,0xA7
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x87'))  power = 4;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\xAC'))  power = 16;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\xBF'))  power = 8;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\x86'))  power = 12;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x81'))  power = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x83'))  power = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9E','\x93'))  power = 20;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE6) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB2','\x9F'))  power = 32;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB6','\xA7'))  power = 36;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE7) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x99','\xBE'))  power = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA7','\xAD'))  power = 24;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA9','\xB0'))  power = 28;
      else  err2 = 1;
    } else {
      err2 = 1;
    }
    if (err2 == 0)  rdPtr += 3;

    if (power < 4) {
      if (power >= inner_power) {
          /* p >= i, return parsed */
        rdPtr -= 6; /* put digit/power back into rdBuf */
        goto done;
      }
      inner_power = power;
      iPtr[(3 - power)] = numeral;
      /*if (err1 != 0)  nelem--;*/
    } else {
      if (power >= major_power) {
          /* case: can't use either read */
        if (inner_power == 0)  {
          if (err1 != 0)  rdPtr -= 3;  /* return numeral */
          rdPtr -= 3;                  /* return power */
          goto done;
        }
          /* cases: can't use read power, but may place into singles */
        if (err2 == 0)  rdPtr -= 3;    /* return power */
          /* need signal to counter exit test */
        if (major_power != 38)
          iPtr += (major_power - 4), major_power = 38;
        if (err1 == 0)  iPtr[3] = numeral;
        goto done;
      }
        /* valid major power */
      if (major_power == 38) {
          /* first time use of major_power, initialize */
          /* determine if implied numeral 1 */
        if (err1 != 0) {
            /* implied 1 or is a group */
          if (*(uint32_t*)indices == 0) {
              /* implied 1, need to count as an element */
            /*iPtr[3] = 1, nelem--;*/
            iPtr[3] = 1;
          } else {
              /* group, move iPtr, done below */
          }
        } else {
            /* found a numeral prefix */
          if (iPtr[3] != 0) {
              /* error condition, return both numeral and power */
            rdPtr -= 6;
            goto done;
          }
          iPtr[3] = numeral;
        }
        major_power = power;
        while (power != 0) *(uint32_t*)&indices[power] = 0, power -= 4;
          /* major power signaled first group complete */
        iPtr = &indices[4];
          /* reset group of 4 positional */
        inner_power = 4;
      } else if ((major_power - 4) == power) {
        major_power = power;
        if (err1 == 0) {
          if (iPtr[3] != 0) {  rdPtr -= 6; goto done;  }
          iPtr[3] = numeral;
        }
          /* move iPtr */
        iPtr += 4;
        inner_power = 4;
      } else {
        uint32_t moving;
          /* group of 4 moves to new location */
        if (err1 == 0) {
          if (iPtr[3] != 0) {
              /* not a group move, placement of numeral at new location */
            iPtr += major_power - power;
            iPtr[-1] = numeral;
            continue;
          }
            /* set numeral, then move group */
          iPtr[3] = numeral;
        }
        moving = *(uint32_t*)iPtr;
        *(uint32_t*)iPtr = 0;
        iPtr += major_power - power - 4;
        *(uint32_t*)iPtr = moving;
        iPtr += 4;
      }
    }
      /* rinse and repeat */
      /* check no inner greater than last */
      /* can be major after first, must be lower */
  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  if (major_power != 38)
    iPtr += (major_power - 4);
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? (NEGateLongUnsigned4, 128bit) */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
hantRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Traditional Chinese (same as hans, save powers of 10)
   * 零一二三四五六七八九
   * 十百 千  10,10^2, 10^3(E5 8D 83)
   * 萬億兆京垓秭穰溝澗  4,8,12,16,20,24,28,32,36  2^128 goes to 38
   */

  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, inner_power, major_power;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

    /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* has <zero>, so error (csWR)? */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }
  if ((rdCh == (uint8_t)'\xE2') &&
                        (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  return csRD;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
                        (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));

  iPtr = indices;
    /* start in indices[0-3] group 1 */
  *(uint64_t*)iPtr = 0;

  inner_power = 4;
  major_power = 38;
  do {
    uint8_t err1 = 0, err2 = 0;
    uint8_t numeral = 1;
    uint8_t power = major_power;

    if ((uint8_t)(rdPtr[0] - 0xE4) >= 5) {
      uint8_t smblCh, sdx;
        /* invisible punctuation for '10' differeniation */
      if ((rdPtr[0] == 0xE2) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))) {
        rdPtr += 3;
        goto done;
      }
        /* <zero> */
      if ((rdPtr[0] == 0xE9) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\xB6'))) {
        if ((iPtr == indices) && (*(uint32_t*)iPtr == 0))  rdPtr += 3;
        goto done;
      }
        /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto done;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto done;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
    /* parse numeral
     const uint8_t u8[10][3] = {
       0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
       0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
       0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x80'))  numeral = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x83'))  numeral = 7;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x89'))  numeral = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB9','\x9D'))  numeral = 9;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x8C'))  numeral = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x94'))  numeral = 5;
      else  err1 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAB'))  numeral = 8;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAD'))  numeral = 6;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\x9B'))  numeral = 4;
      else  err1 = 1;
    } else {
      err1 = 1;
    }
    if (err1 == 0)  rdPtr += 3, --nelem;

    /* parse power
     const uint8_t p8[12][3] = {
       0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE8,0x90,0xAC,
       0xE5,0x84,0x84, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
       0xE7,0xA7,0xAD, 0xE7,0xA9,0xB0, 0xE6,0xBA,0x9D, 0xE6,0xBE,0x97
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\xAC'))  power = 16;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x84','\x84'))  power = 8;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\x86'))  power = 12;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x81'))  power = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x83'))  power = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9E','\x93'))  power = 20;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE6) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x9D'))  power = 32;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBE','\x97'))  power = 36;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE7) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x99','\xBE'))  power = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA7','\xAD'))  power = 24;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA9','\xB0'))  power = 28;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE8) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x90','\xAC'))  power = 4;
      else  err2 = 1;
    } else {
      err2 = 1;
    }
    if (err2 == 0)  rdPtr += 3;

    if (power < 4) {
      if (power >= inner_power) {
          /* p >= i, return parsed */
        rdPtr -= 6; /* put digit/power back into rdBuf */
        goto done;
      }
      inner_power = power;
      iPtr[(3 - power)] = numeral;
    } else {
      if (power >= major_power) {
          /* case: can't use either read */
        if (inner_power == 0)  {
          if (err1 != 0)  rdPtr -= 3;  /* return numeral */
          rdPtr -= 3;                  /* return power */
          goto done;
        }
          /* cases: can't use read power, but may place into singles */
        if (err2 == 0)  rdPtr -= 3;    /* return power */
          /* need signal to counter exit test */
        if (major_power != 38)
          iPtr += (major_power - 4), major_power = 38;
        if (err1 == 0)  iPtr[3] = numeral;
        goto done;
      }
        /* valid major power */
      if (major_power == 38) {
          /* first time use of major_power, initialize */
          /* determine if implied numeral 1 */
        if (err1 != 0) {
            /* implied 1 or is a group */
          if (*(uint32_t*)indices == 0) {
              /* implied 1, need to count as an element */
            /*iPtr[3] = 1, nelem--;*/
            iPtr[3] = 1;
          } else {
              /* group, move iPtr, done below */
          }
        } else {
            /* found a numeral prefix */
          if (iPtr[3] != 0) {
              /* error condition, return both numeral and power */
            rdPtr -= 6;
            goto done;
          }
          iPtr[3] = numeral;
        }
        major_power = power;
        while (power != 0) *(uint32_t*)&indices[power] = 0, power -= 4;
          /* major power signaled first group complete */
        iPtr = &indices[4];
          /* reset group of 4 positional */
        inner_power = 4;
      } else if ((major_power - 4) == power) {
        major_power = power;
        if (err1 == 0) {
          if (iPtr[3] != 0) {  rdPtr -= 6; goto done;  }
          iPtr[3] = numeral;
        }
          /* move iPtr */
        iPtr += 4;
        inner_power = 4;
      } else {
        uint32_t moving;
          /* group of 4 moves to new location */
        if (err1 == 0) {
          if (iPtr[3] != 0) {
              /* not a group move, placement of numeral at new location */
            iPtr += major_power - power;
            iPtr[-1] = numeral;
            continue;
          }
            /* set numeral, then move group */
          iPtr[3] = numeral;
        }
        moving = *(uint32_t*)iPtr;
        *(uint32_t*)iPtr = 0;
        iPtr += major_power - power - 4;
        *(uint32_t*)iPtr = moving;
        iPtr += 4;
      }
    }
      /* rinse and repeat */
      /* check no inner greater than last */
      /* can be major after first, must be lower */
  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  if (major_power != 38)
    iPtr += (major_power - 4);
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? (NEGateLongUnsigned4, 128bit) */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

/*XXXX add reading of GERESH and GERSHAYIM, even though not yet used in Wr */
/* groupbit to be used, 0 or default is contextual; 1 or set is non-contextual */
csEnum
hebrRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* Additive  Hebrew   0xD7 prefix
   * אבגדהוזחט  1 - 9
   * יכלמנסעפצ  10 - 90
   * קרשת       100 - 400
   * ךםןףץ      500 - 900  not traditionally used
   * ׳          thousands/millions group (suffix)
   * ״          last digit (1s) prefix with 2 or more digits 0xD7,0xB4
   */

  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, major_power, power;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

    /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
        /* has <zero>, so error (csWR)? */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  /* nelem, should be based on position, wr counted 500 < n < 1000 as individuals */
  iPtr = indices;
    /* start in indices[0-2] group 1 */
  *(uint64_t*)iPtr = 0;

  major_power = 38;
  power = 38;

  if ((rdCh == (uint8_t)'\xE2') &&
      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
        /* this is hebrew <zero> also */
      if ((--nelem) == 0)  goto done;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
             (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));
    /* hebrew on 3 group system */
  do {
    uint8_t numeral;

    if (rdPtr[0] != 0xD7) {
      uint8_t smblCh, sdx;
        /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto done;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto done;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
    /* parse numeral/power
     const uint8_t  u8[2][9] = {
      { 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98 },  ones
      { 0x99,0x9B,0x9C,0x9E,0xA0,0xA1,0xA2,0xA4,0xA6 }   tens
     };
     const uint8_t  h8[9] = {
        0xA7,0xA8,0xA9,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA };  hundreds
     */
    if ((uint8_t)(numeral = (rdPtr[1] - 0x90)) > (uint8_t)0x24)  goto done;
    if ((++numeral) > 9) {
        /* 10,12,13,15,17,18,19,21,23  24,25,26,27 */
        /* 0xB3,0xB4 punctuation, 36,37 after math */
      if (numeral >= 24) {
        if ((numeral -= 23) < 5) {
            /* 1,2,3,4 hundreds */
          iPtr[0] += numeral;
          if (iPtr[0] < 5)  nelem--;
          rdPtr += 2;
        } else {
            /*  13,14 marks */
          if ((uint8_t)(numeral -= 13) > (uint8_t)1)  goto done;
            /* 0,1 power, last digit follows */
          if (numeral == 0) {
    power_marks:
            power = 3;
            while (*(uint16_t*)&rdPtr[2] == cconst16_t('\xD7','\xB3'))
              *(uint32_t*)&iPtr[power] = 0, rdPtr += 2, power += 3;
            *(uint32_t*)&iPtr[power] = 0;
            if ((major_power == 38) || (power == (major_power - 3)))
              major_power = power, iPtr += 3, rdPtr += 2;
            else if (power >= major_power) {
                /* error condition, return stuff */
              while (power)  rdPtr -= 2, power -= 3;
                /* return numeral its attached to */
              rdPtr -= 2;
              goto done;
            } else {
              iPtr += major_power - power;
              power = major_power;
              rdPtr += 2;
            }
          } else {
            nelem = 1;
            rdPtr += 2;
          }
        }
      } else {
          /* tens */
        if (iPtr[1] != 0)  goto done;
        if      (numeral == 10)  numeral = 1;
        else if (numeral == 12)  numeral = 2;
        else if (numeral == 13)  numeral = 3;
        else if (numeral == 15)  numeral = 4;
        else if (numeral == 17)  numeral = 5;
        else if (numeral == 18)  numeral = 6;
        else if (numeral == 19)  numeral = 7;
        else if (numeral == 21)  numeral = 8;
        else if (numeral == 23)  numeral = 9;
        else {  return csIE;  }
        iPtr[1] = numeral;
        nelem--;
        rdPtr += 2;
      }
    } else {
        /* 1-9 singles */
      if (iPtr[2] != 0)  goto done;
        /* special case: 15,16 has switched positionals */
      if ((numeral == 5) || (numeral == 6)) {
          /* if last digit mark */
        if (*(uint16_t*)&rdPtr[2] == cconst16_t('\xD7','\xB4')) {
          if (rdPtr[5] != 0x99)  return csIE;
          if (*(uint16_t*)&iPtr[1] != 0)  goto done;
          iPtr[1] = 1, iPtr[2] = numeral;
          rdPtr += 6;
          goto done;
        }
      }
      /* place in iPtr[2], set for next round, unless was last parse */
      iPtr[2] = numeral;
      nelem--;
      rdPtr += 2;
    }

    if ((uint32_t)(nelem - (uint32_t)1) > nelem) {
      if ((*rdPtr == 0xD7) && (rdPtr[1] == 0xB3))  goto power_marks;
      break;
    }

  } while (1);

done:
    /* case: termination without shift of final 3 to correct slot */
  if ((power != 38) && (power > 3)) {
    uint32_t moving = *(uint32_t*)iPtr;
    *(uint32_t*)iPtr = 0;
    *(uint32_t*)&iPtr[(power - 3)] = moving;
    iPtr = &iPtr[(power - 3)];
    major_power = (power = 3);
  }
  iPtr[3] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
jpanRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /*   Japanese
   * 零一二三四五六七八九
   * 十百千  10,10^2, 10^3(E5 8D 83)
   * 万億兆京垓𥝱,秭穣溝澗  4,8,12,16,20,24,24,28,32,36  2^128 goes to 38
   * sequence reads left to right, or top to bottom.
   * there is an optional <zero> used as spot holder between numbers
   * invisible punctuation added in special cases for formatting
   */

  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, inner_power, major_power;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

    /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
        /* has <zero>, so error (csWR)? */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }
  if ((rdCh == (uint8_t)'\xE2') &&
      (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')))
    do {
      rdPtr += 3;
      if ((--nelem) == 0)  return csRD;
    } while ((*rdPtr == (uint8_t)'\xE2') &&
             (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));

  /* each is group of 4, in the 4 i have 10^0, 10^1(十), 10^2(百), 10^3(千)
   * groups of 4 have thier powers:
   *  10^4(万), 10^8(億), 10^12(兆), 10^16(京), 10^20(垓), 10^24(𥝱), 10^24(秭),
   *   10^28(穣), 10^32(溝), 10^36(澗)... etc. ^36 as far as number set
   */
  iPtr = indices;
    /* start in indices[0-3] group 1 */
  *(uint64_t*)iPtr = 0;

    /* invisible punctuation for power implied 1 of first  */
  if ((rdPtr[0] == 0xE2) &&
          (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))) {
    rdPtr += 3;
  }

  inner_power = 4;
  major_power = 38;
  do {
    uint8_t err1 = 0, err2 = 0;
    uint8_t numeral = 1;
    uint8_t power = major_power;

    if ((uint8_t)(rdPtr[0] - 0xE4) >= 4) {
      uint8_t smblCh, sdx;
        /* invisible punctuation for '10' power differeniation */
      if ((rdPtr[0] == 0xE2) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))) {
        rdPtr += 3;
        goto done;
      }
        /* <zero> */
      if ((rdPtr[0] == 0xE9) &&
              (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\xB6'))) {
        if ((iPtr == indices) && (*(uint32_t*)iPtr == 0))  rdPtr += 3;
        goto done;
      }
        /* verify grouping else done */
      if ((*rflags & GROUP_BIT) == 0)  goto done;
      smblCh  = smblPtr[(sdx = 0)];
      do {
        if (smblCh != rdPtr[sdx])  goto done;
        smblCh = smblPtr[(++sdx)];
      } while (smblCh != 0);
        /* grouping should not be part of nelem count */
      rdPtr += sdx;
      continue;
    }
    /* parse numeral
     const uint8_t u8[10][3] = {
       0xE9,0x9B,0xB6, 0xE4,0xB8,0x80, 0xE4,0xBA,0x8C, 0xE4,0xB8,0x89,
       0xE5,0x9B,0x9B, 0xE4,0xBA,0x94, 0xE5,0x85,0xAD, 0xE4,0xB8,0x83,
       0xE5,0x85,0xAB, 0xE4,0xB9,0x9D
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x80'))  numeral = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x83'))  numeral = 7;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x89'))  numeral = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB9','\x9D'))  numeral = 9;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x8C'))  numeral = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x94'))  numeral = 5;
      else  err1 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAB'))  numeral = 8;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\xAD'))  numeral = 6;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9B','\x9B'))  numeral = 4;
      else  err1 = 1;
    } else {
      err1 = 1;
    }
    if (err1 == 0)  rdPtr += 3, --nelem;

    /* parse power
     const uint8_t p8[12][3] = {
       0xE5,0x8D,0x81, 0xE7,0x99,0xBE, 0xE5,0x8D,0x83, 0xE4,0xB8,0x87,
       0xE5,0x84,0x84, 0xE5,0x85,0x86, 0xE4,0xBA,0xAC, 0xE5,0x9E,0x93,
       0xE7,0xA7,0xAD, 0xE7,0xA9,0xA3, 0xE6,0xBA,0x9D, 0xE6,0xBE,0x97
     }; */
    if        (rdPtr[0] == 0xE4) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\x87'))  power = 4;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xB8','\xAC'))  power = 16;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE5) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x84','\x84'))  power = 8;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x85','\x86'))  power = 12;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x81'))  power = 1;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x8D','\x83'))  power = 3;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x9E','\x93'))  power = 20;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE6) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBA','\x9D'))  power = 32;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xBE','\x97'))  power = 36;
      else  err2 = 1;
    } else if (rdPtr[0] == 0xE7) {
      if      (*(uint16_t*)&rdPtr[1] == cconst16_t('\x99','\xBE'))  power = 2;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA7','\xAD'))  power = 24;
      else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\xA9','\xA3'))  power = 28;
      else  err2 = 1;
    } else {
      err2 = 1;
    }
    if (err2 == 0)  rdPtr += 3;

    if (power < 4) {
      if (power >= inner_power) {
          /* p >= i, return parsed */
        rdPtr -= 6; /* put digit/power back into rdBuf */
        goto done;
      }
      inner_power = power;
      iPtr[(3 - power)] = numeral;
      if ((nelem == 0) && (rdPtr[0] == 0xE2)) {
        if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))
          rdPtr += 3;
        goto done;
      }
    } else {
      if (power >= major_power) {
          /* case: can't use either read */
        if (inner_power == 0)  {
          if (err1 != 0)  rdPtr -= 3;  /* return numeral */
          rdPtr -= 3;                  /* return power */
          goto done;
        }
          /* cases: can't use read power, but may place into singles */
        if (err2 == 0)  rdPtr -= 3;    /* return power */
          /* need signal to counter exit test */
        if (major_power != 38)
          iPtr += (major_power - 4), major_power = 38;
        if (err1 == 0)  iPtr[3] = numeral;
        goto done;
      }
        /* valid major power */
      if (major_power == 38) {
          /* first time use of major_power, initialize */
          /* determine if implied numeral 1 */
        if (err1 != 0) {
            /* implied 1 or is a group */
          if (*(uint32_t*)indices == 0) {
              /* implied 1, need to count as an element */
            iPtr[3] = 1;
          } else {
              /* group, move iPtr, done below */
          }
        } else {
            /* found a numeral prefix */
          if (iPtr[3] != 0) {
              /* error condition, return both numeral and power */
            rdPtr -= 6;
            goto done;
          }
          iPtr[3] = numeral;
        }
        major_power = power;
        while (power != 0) *(uint32_t*)&indices[power] = 0, power -= 4;
          /* major power signaled first group complete */
        iPtr = &indices[4];
          /* reset group of 4 positional */
        inner_power = 4;
      } else if ((major_power - 4) == power) {
        major_power = power;
        if (err1 == 0) {
          if (iPtr[3] != 0) {  rdPtr -= 6; goto done;  }
          iPtr[3] = numeral;
        }
          /* move iPtr */
        iPtr += 4;
        inner_power = 4;
      } else {
        uint32_t moving;
          /* group of 4 moves to new location */
        if (err1 == 0) {
          if (iPtr[3] != 0) {
              /* not a group move, placement of numeral at new location */
            iPtr += major_power - power;
            iPtr[-1] = numeral;
            continue;
          }
            /* set numeral, then move group */
          iPtr[3] = numeral;
        }
        moving = *(uint32_t*)iPtr;
        *(uint32_t*)iPtr = 0;
        iPtr += major_power - power - 4;
        *(uint32_t*)iPtr = moving;
        iPtr += 4;
      }
    }
      /* rinse and repeat */
      /* check no inner greater than last */
      /* can be major after first, must be lower */
  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  if (major_power != 38)
    iPtr += (major_power - 4);
  iPtr[4] = (uint8_t)0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
    /* need a negate? (NEGateLongUnsigned4, 128bit) */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

/* NOTE: set up only for whole number reading, with ignore group symbol */
/* special note: monetary should handle same, both have "group". */
/* monetary issues: different minus notations */
/* not optimize for latn, design as template */
/* XXXX in all states */
/* XXX 191105 adapting to real number parsing. Scenerio: padded with
 * <zero>s. Don't want these to count as part of total digits to
 * deindex. They should be part of nelem count thou.
 * Moved read of ptr to make use of 2 cycle read */
csEnum
latnRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  csEnum  cs_code;
  uint8_t indices[44];
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *realPtr, *smblPtr, rdCh;
    /* allow up to 2^16 digits ? */
  uint16_t  w;
  int16_t dp;
  int8_t direction;

  smblPtr = NULL, realPtr = NULL, dp = 0, direction = 0;
    /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  indices[(w = 0)] = (uint8_t)0xFF;
  indices[43]      = (uint8_t)0xFF;
    /* FILE buffering has discontinous buffers,
     * encountered more data to append to previous.
     * Reconstruct previous to continue. */
  if ((*rflags & OVFL_BIT) != 0) {
    csCnvrt_st invrs;
    invrs.cs_rd = (void*)cnvrt->cs_wr;
    invrs.cs_rdSz = (size_t)cnvrt->cs_wrSz;
    invrs.cs_wr = (void*)indices;
    invrs.cs_wrSz = (size_t)44;
    w = indicize(&invrs, rflags);
      /* case: .000 or 0 */
    if ((w == 1) && (indices[0] == 0))
      w = 0;
      /* had <decimal point> ? */
    if (((*rflags & REAL_BIT) == 0) && (cnvrt->cs_wrSz == 5)) {
      dp = ((int32_t*)cnvrt->cs_wr)[4];
      ((int32_t*)cnvrt->cs_wr)[4] = 0;
      direction = -1;
    }
  }

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;    /* number of gylphs */
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;

  if ((*rflags & GROUP_BIT) != 0)
    smblPtr = ns_numberinfo(NS_GROUP, ns);
  if ((*rflags & REAL_BIT) != 0) {
      /* note: only pre-<decimal>, post pre-<zero> grouping allowed */
    realPtr = ns_numberinfo(NS_DECIMAL, ns);
initial_test:
    if (rdCh == *realPtr) {
      if (realPtr[1] == 0)
        {  nelem += (direction = -1), rdPtr += 1, realPtr = NULL, smblPtr = NULL;
           *rflags = (*rflags & ~REAL_BIT) | OVFL_BIT;
           if ((w == 1) && (indices[0] == 0))  w = 0;  }
      else if (realPtr[1] == rdPtr[1])
        {  nelem += (direction = -1), rdPtr += 2, realPtr = NULL, smblPtr = NULL;
           *rflags = (*rflags & ~REAL_BIT) | OVFL_BIT;
           if ((w == 1) && (indices[0] == 0))  w = 0;  }
      if ((uint32_t)(nelem - (uint32_t)1) > nelem)  goto gathered;
      rdCh = *rdPtr;
    }
  }
    /* <zero> devour, note pre-<zero> group not allowed, nor fractional */
  if (rdCh == (uint8_t)0x30) {
    do {
      if (w < 43)
        indices[w] = 0;   /* must assign, case of reconstruction '(w != 0)' */
      rdPtr++;            /* rdCh used */
      --nelem;            /* is there a next readable? */
      if ((uint32_t)(nelem - (uint32_t)1) > nelem) {
        w++;
        dp += direction;
        goto gathered;
      }
      rdCh = *rdPtr;
      dp += direction;
      if (w != 0)  w++;  /* case: reconstructed, and previous digits */
    } while (rdCh == (uint8_t)0x30);
    if ((uint8_t)(rdCh - (uint8_t)0x30) > (uint8_t)9) {
      if (w == 0) w++;
      if (realPtr != NULL)  goto initial_test;
    }
  }

  do {
      /* WTF?! GCC does not understand unsign - unsign => unsign */
    if ((uint8_t)(rdCh - (uint8_t)0x30) <= (uint8_t)9) {
      if (w < 43)
        indices[w] = rdCh - (uint8_t)0x30;
      w++;
      ++rdPtr;
      dp += direction;
    } else {
        /* check for real decimal */
      if ((realPtr != NULL) && (rdCh == *realPtr)) {
          /* note: turned NULL if already encountered */
        if (realPtr[1] == 0)
          {  direction = -1, rdPtr += 1, realPtr = NULL, smblPtr = NULL;
            *rflags = (*rflags & ~REAL_BIT) | OVFL_BIT; goto smbl_found;  }
        else if (realPtr[1] == rdPtr[1])
          {  direction = -1, rdPtr += 2, realPtr = NULL, smblPtr = NULL;
            *rflags = (*rflags & ~REAL_BIT) | OVFL_BIT; goto smbl_found;  }
      }
        /* check if grouping */
      if ((smblPtr != NULL) && (rdCh == *smblPtr)) {
        if (rdCh == *smblPtr) {
          if      (smblPtr[1] == 0)        {  rdCh = *(rdPtr += 1); continue;  }
          else if (smblPtr[1] == rdPtr[1]) {  rdCh = *(rdPtr += 2); continue;  }
        }
      }
        /* rdCh doesn't match */
      break;
    }
  smbl_found:
    --nelem;
    if ((uint32_t)(nelem - (uint32_t)1) > nelem)  break;
    rdCh = *rdPtr;
  } while (1);

gathered:
  cnvrt->cs_rdSz -= (size_t)((void *)rdPtr - cnvrt->cs_rd);
  cnvrt->cs_rd = (void *)rdPtr;
  if ((uint16_t)(w - 1) < (uint16_t)43) {
    indices[w]  = (uint8_t)0xFF;
  } else if (w == 0) {
    if (direction == 0)  return csIE;
    if (indices[0] == 0xFF)  return csNC;   /* . only */
    *(int16_t*)indices = cconst16_t('\x00','\xFF');
  }
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  if (((*rflags & (REAL_BIT | OVFL_BIT)) != 0) && (cnvrt->cs_wrSz == 5)) {
    if (cs_code == csWR) {
        /* handle overflow condition
         * note: lose 4 bit accuracy, still 12 bits beyond quad */
      indices[38] = (uint8_t)0xFF;
      deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
      dp += w - 38;
    }
    ((uint32_t*)cnvrt->cs_wr)[4] += dp;
  }
    /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

csEnum
tamlRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags) {

  /* http://www.unicode.org/notes/tn21/tamil_numbers.pdf */
  /*   Archaic Tamil (multiplicative)
   * ௧ ௨ ௩ ௪ ௫ ௬ ௭ ௮ ௯    0xE0,0xAF,0xA7-AF
   * ௰ ௱ ௲  10,100,1000   0xE0,0xAF,0xB0-B2
   */
  /* Basically all but digit '1' get a suffix */
  /* Loop is of 7, before reduction in size */
  /* ௰, ௱, ௲, ௰௲, ௰௲, ௱௲, ௰௱௲, ௱௱௲, ௰௱௱௲, ௱௱௱௲, ௲௱௱௲, .... */

    /* increased by 4, due to 128bit and 4byte/10000s reads */
  uint8_t indices[44];
  csEnum  cs_code;
  uint32_t nelem;    /* number of gylphs */
  uint8_t *rdPtr, *smblPtr, *iPtr;
  uint8_t  rdCh, major_power;
  int8_t power, last_idx;

  /* check for <spaces> formatting, and arithmetic sign */
  preNumericCheck(cnvrt, ns, rflags);

  nelem = (uint32_t)cnvrt->cs_rdSz & 0xFFFF;
  rdPtr = (uint8_t *)cnvrt->cs_rd;
  rdCh = *rdPtr;
  smblPtr = ns_numberinfo(NS_GROUP, ns);

  /* if was signed, any formatting <zeroes> spaces get gobbled */
  if (rdCh == ' ') {
    do {
      rdCh = *++rdPtr;
      /* this is error! <zero> == <mathematical space> */
      if ((--nelem) == 0)  return csRD;
    } while (rdCh == ' ');
  }

  iPtr = indices;
  power = 0;
  *(uint32_t*)iPtr = 0;
  major_power = 3;  /*  */
  last_idx = -1;

    /* if zero precedes then will not include punctuation */
  if (*rdPtr == 0xE2) {
    if (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F'))
      do {
        rdPtr += 3;
        if ((--nelem) == 0)  goto done;
      } while ((*rdPtr == (uint8_t)'\xE2') &&
               (*(uint16_t *)&rdPtr[1] == cconst16_t('\x81','\x9F')));
    else if (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))
      rdPtr += 3;
  }

  do {
    uint8_t smblCh, sdx;
      /* parse digits/power glyphs */
    if ((*rdPtr == 0xE0) && (rdPtr[1] == 0xAF)) {
      uint8_t n, numeral;
      if ((uint8_t)(rdCh = rdPtr[2] - 0xA7) > (uint8_t)11)  goto done;
        /* digits */
      rdPtr += 3, nelem--;
      if ((++rdCh) < 10)  numeral = rdCh, power = 0;
      else {              numeral = 1, power = rdCh - 9;
        rdCh = power - 1;
        goto bypass_first_test;
      }
        /* powers */
    more_power:
      if ((*rdPtr == 0xE0) && (rdPtr[1] == 0xAF))
        if ((uint8_t)(rdCh = rdPtr[2] - 0xB0) <= (uint8_t)2) {
          power += rdCh + 1, rdPtr += 3;
    bypass_first_test:
            /* 'w' all permitted, except 'w' */
          if (rdCh == 0) {
            if ((*rdPtr == 0xE0) && (rdPtr[1] == 0xAF) && (rdPtr[2] == 0xB0))
              goto assign;
            goto more_power;
          }
          if (rdCh == 1) {
            /* if 'm', 't' or 'mt' only exceptable */
              /* mw not allowed */
            if ((*rdPtr == 0xE0) && (rdPtr[1] == 0xAF) && (rdPtr[2] == 0xB0))
              goto assign;
              /* mt allowed */
            if ((*rdPtr == 0xE0) && (rdPtr[1] == 0xAF) && (rdPtr[2] == 0xB2))
              goto more_power;
     /* case entered from bypass_first_test with 'm' or from powers
      * m mmt instead of just mmt */
              /* mmt test, but already had 'm' added to power */
            if ((  (*rdPtr == 0xE0) && (rdPtr[1] == 0xAF) && (rdPtr[2] == 0xB1)) &&
                ((rdPtr[3] == 0xE0) && (rdPtr[4] == 0xAF) && (rdPtr[5] == 0xB2))) {
              power -= 2;
              rdPtr += 6;
              power += 7;
              /* fall into mmt testing */
            }
          }
            /* if 't', only 'mmt' permitted... if (rdCh == 2)  */
      mmt_test:
          if ((  (*rdPtr == 0xE0) && (rdPtr[1] == 0xAF) && (rdPtr[2] == 0xB1)) &&
              ((rdPtr[3] == 0xE0) && (rdPtr[4] == 0xAF) && (rdPtr[5] == 0xB1)) &&
              ((rdPtr[6] == 0xE0) && (rdPtr[7] == 0xAF) && (rdPtr[8] == 0xB2))) {
            power += 7, rdPtr += 9;
            goto mmt_test;
          }
          /* fall into "put it together, and move on" */
        }
        /* put it together, and move on */
      if (major_power <= power) {
        n = major_power;
        do {
          *(uint32_t*)&iPtr[((n + 3) & ~3)] = 0;
        } while ((n += 3) < power);
        if (*(uint32_t*)indices == 0)  major_power = power;
        else                           major_power += power;
      }
    assign:
      if (last_idx < (major_power - power))  last_idx = major_power - power;
      else {
        if (power > 0)  rdPtr -= 3;
        if (numeral > 1)  rdPtr -= 3;
        goto done;
      }
      iPtr[last_idx] = numeral;
        /* check if power termination */
      if ((rdPtr[0] == 0xE2) &&
            (*(uint16_t*)&rdPtr[1] == cconst16_t('\x81','\xA3'))) {
        rdPtr += 3;
        goto done;
      }
      if (last_idx == major_power)  goto done;
      continue;
    }
      /* only other allowed glyph, grouping */
    if ((*rflags & GROUP_BIT) == 0)  goto done;
    smblCh  = smblPtr[(sdx = 0)];
    do {
      if (smblCh != rdPtr[sdx])  goto done;
      smblCh = smblPtr[(++sdx)];
    } while (smblCh != 0);
      /* grouping should not be part of nelem count */
    rdPtr += sdx;

  } while ((uint32_t)(nelem - (uint32_t)1) <= nelem);

done:
  iPtr[(major_power + 1)] = 0xFF;
  cnvrt->cs_rd = (void *)rdPtr;
  cs_code = deindex(cnvrt->cs_wr, indices, (uint32_t)cnvrt->cs_wrSz);
  /* need a negate? */
  if ((*rflags & SIGNED_VALUE) != 0)
    _neglu4((uint32_t *)cnvrt->cs_wr, (uint32_t)cnvrt->cs_wrSz);
  return cs_code;
}

