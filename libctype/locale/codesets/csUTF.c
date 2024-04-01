/*
 *  csUTF8.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Tue 28 Aug 2012.
 *  Copyright (c) 2012, 2019, 2022. All rights reserved.
 *
 */

/* conversion source:
 * ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt
 * using csUTF8ToUnicode.c
 * same as unicode except changed to 16bit value w/ 128bit cases
 * 93430 vs 128388 bytes, new vs old
 */

#include "as_ctype.h"

  /* code found in ctype_ucs.c */
extern uint32_t   ucs_getUCV(uint32_t);

unsigned UTF8toUnicode(unsigned i);
unsigned UTF32toUnicode(unsigned i);

/* needed for strcoll(), might be useful?. */
unsigned
UTF8toUnicode(unsigned i) {

  if ((i & 0xF8C0C0C0) == 0xF0808080)
    return (((i & 0x7000000) >> 6) | ((i & 0x3F0000) >> 4)
            | ((i & 0x3F00) >> 2) | (i & 0x3F));
  if ((i & 0x80F0C0C0) == 0x00E08080)
    return (((i & 0xF0000) >> 4) | ((i & 0x3F00) >> 2) | (i & 0x3F));
  if ((i & 0x8080E0C0) == 0x0000C080)
    return (((i & 0x1F00) >> 2) | (i & 0x3F));
  if ((i & 0x80808080) == 0x00000000)
    return i;
  /* error occured */
  return 0xF8C0C0C0;
}

unsigned UTF32toUnicode(unsigned i)  {  return i;  }

/*
 *                   0xFEFF
 * __LITTLE__ENDIAN__  |  __BIG__ENDIAN__
 *       FF FE 00 00   |  00 00 FE FF
 *       b0 b1 b2 b3   |  b0 b1 b2 b3
 *             FF FE   |  FE FF
 *             b0 b1   |  b0 b1
 */

#pragma mark #### UTF-8 Encoders

/* unsigned wrap around check */
#define BOUNDS(x)  if ((U8Sz - (size_t)(x)) > U8Sz)  goto rcsWR;

/* Encoder: unicode stream to UTF-8 stream */
csEnum
UStrtoUTF8Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *U8Str = (uint8_t *)cnvrt->cs_wr;
  size_t    U8Sz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;
  uint32_t  cp;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  cp = *cpStr;
  do {

    if (cp == EOSmark) {
      BOUNDS(1);
      U8Sz  -= 1;
      *U8Str = (uint8_t)cp;
      U8Str += 1;
      cpStr += 1;
      cpSz  -= sizeof(uint32_t);
      goto rcsNUL;
    }
    if (cp <= 0x007F) {
      BOUNDS(1);
      U8Sz -= 1;
      *U8Str++ = (uint8_t)cp;
      goto cntnu;
    }
    if (cp <= 0x07FF) {
      BOUNDS(2);
      U8Sz -= 2;
      *U8Str++ = 0xC0 | (cp >> 6);
      *U8Str++ = 0x80 | (cp & 0x3F);
      goto cntnu;
    }
    if (cp < 0xFFFE) {
      if ((cp - (uint32_t)0xD800) <= (uint32_t)(0xDFFF - 0xD800))  goto rcsSI;
    non_wr:
      BOUNDS(3);
      U8Sz -= 3;
      *U8Str++ = 0xE0 | (cp >> 12);
      *U8Str++ = 0x80 | ((cp >> 6) & 0x3F);
      *U8Str++ = 0x80 | (cp & 0x3F);
      goto cntnu;
    }
    if (cp < 0x10000) {
      if ((cpSz != sizeof(uint32_t)) || (U8Sz != 3))  goto rcsNC;
      goto non_wr;
    }
    if (cp > 0x10FFFF)  goto rcsIU;
    if (((uint32_t)(cp & (uint32_t)0xFFFF) - (uint32_t)0xFFFE) < 2) {
      if ((cpSz != sizeof(uint32_t)) || (U8Sz != 4))  goto rcsNC;
    }
    BOUNDS(4);
    U8Sz -= 4;
    *U8Str++ = 0xF0 | (cp >> 18);
    *U8Str++ = 0x80 | ((cp >> 12) & 0x3F);
    *U8Str++ = 0x80 | ((cp >> 6) & 0x3F);
    *U8Str++ = 0x80 | (cp & 0x3F);
  cntnu:
    ++cpStr;
    cpSz -= sizeof(uint32_t);
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = U8Sz;
  cnvrt->cs_wr = (void *)U8Str;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (U8Sz == 0)  goto rcsRW;
  goto r_state;
rcsRW:
  cs_code = csRW;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
rcsIU:
  cs_code = csIU;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### UTF-8 Decoders

/* Decoder: UTF-8 stream to unicode codepoint stream */
csEnum
UTF8StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;
  /* Variables, adjust as needed */
  uint8_t  cs0, cs1, cs2, cs3;
  size_t   cscnt;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *U8Str = (uint8_t *)cnvrt->cs_rd;
  size_t    U8Sz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (U8Str == NULL))  goto rcsIE;

  /* Initialize variables, as needed */
  cs3 = (cs2 = (cs1 = 0));
  cscnt = 0;

  do {
    uint32_t cp;

    if ((U8Sz - (cscnt + 1)) > U8Sz)  goto rcsRD;
    cs0 = U8Str[cscnt];

    /* Assume most codes <= 0x7F */
    if (cs0 <= 0x7F) {
      if (cscnt != 0) {
        if (cs0 == EOSmark)  goto rcsRN;
        goto rcsIC;
      }
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      cpSz -= sizeof(uint32_t);
      *cpStr = (uint32_t)cs0, cpStr++;
      U8Sz -= 1, U8Str += 1;
      if (cs0 == EOSmark)  goto rcsNUL;
      if (cpSz == 0)  goto rcsWR;
      continue;
    }

    /* update variables */
    cscnt++;
    if (cs3 != 0) {
      /* Have 4 unit code. If (larger) rcIC */
      /* cs0 comes after 3 higher bytes */
      if ((cs0 & (uint8_t)0xC0) != (uint8_t)0x80)  goto rcsIC;
      if ((cs1 & (uint8_t)0xF8) == (uint8_t)0xF0) {
        cp = (((uint32_t)cs1 & 0x07) << 18)
           | (((uint32_t)cs2 & 0x3F) << 12)
           | (((uint32_t)cs3 & 0x3F) << 6)
           |  ((uint32_t)cs0 & 0x3F);
        if (cp < 0x10000)  goto rcsIC;
        if (cp > 0x10FFFF)  goto rcsIU;
        if (((uint32_t)(cp & (uint32_t)0xFFFF) - (uint32_t)0xFFFE) < 2) {
          if ((cpSz != sizeof(uint32_t)) || (U8Sz != 4))  goto rcsNC;
        }
        goto cpo;
      }
      goto rcsIC;
    }
    if (cs2 != 0) {
      /* Have 3 unit code? */
      /* cs0 comes after 2 higher bytes */
      if ((cs0 & (uint8_t)0xC0) != (uint8_t)0x80)  goto rcsIC;
      if ((cs1 & (uint8_t)0xF0) == (uint8_t)0xE0) {
        cp = (((uint32_t)cs1 & 0x0F) << 12)
           | (((uint32_t)cs2 & 0x3F) << 6)
           |  ((uint32_t)cs0 & 0x3F);
        if (cp < 0x800)  goto rcsIC;
        if ((cp - (uint32_t)0xD800) <= (uint32_t)(0xDFFF - 0xD800))  goto rcsSI;
        if ((cp >= 0xFFFE) && ((cpSz != sizeof(uint32_t)) || (U8Sz != 3)))  goto rcsNC;
        goto cpo;
      }
      cs3 = cs0;
      continue;
    }
    if (cs1 != 0) {
      /* Have 2 unit code ? */
      /* cs0 comes after high byte */
      if ((cs0 & (uint8_t)0xC0) != (uint8_t)0x80)  goto rcsIC;
      if ((cs1 & (uint8_t)0xE0) == (uint8_t)0xC0) {
        cp = (((uint32_t)cs1 & 0x1F) << 6)
           |  ((uint32_t)cs0 & 0x3F);
        if (cp < 0x80)  goto rcsIC;
  cpo:  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
        *cpStr++ = cp;
        U8Sz  -= cscnt;
        U8Str += cscnt;
        if ((cpSz -= sizeof(uint32_t)) == 0)  goto rcsWR;
        cs3 = (cs2 = (cs1 = 0));
        cscnt = 0;
        continue;
      }
      cs2 = cs0;
      continue;
    }
    /* high byte */
    cs1 = cs0;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = U8Sz;
  cnvrt->cs_rd = (void *)U8Str;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;
rcsWR:
  cs_code = csWR;
  if (U8Sz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cscnt != 0) goto rcsIC;
  goto r_state;
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIU:
  cs_code = csIU;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### UTF-16

typedef enum {  wr2,  wr1  } wr16_et;
typedef enum {  rd2,  rd1  } rd16_et;

static csEnum
_U16ncd(csCnvrt_st *cnvrt, wr16_et ws) {

  uint32_t cp;
  uint16_t c0, c1;
  csEnum cs_code;

  uint32_t *rStr = (uint32_t *)cnvrt->cs_rd;
  size_t    rSz  = cnvrt->cs_rdSz;
  uint16_t *wStr = (uint16_t *)cnvrt->cs_wr;
  size_t    wSz  = cnvrt->cs_wrSz;

  if ((rSz == 0) || (rStr == NULL)) goto rcsIE;
  if ((rSz - sizeof(uint32_t)) > rSz)  goto rcsRD;

  do {
    cp = *rStr;
    c1 = (uint16_t)(cp >> 16);
    c0 = (uint16_t)(cp & (uint32_t)0xFFFF);

    if (cp >= (uint32_t)0xFFFE) {
      if (c1 > (uint16_t)0x10)  goto rcsIU;
      if ((c0 - (uint16_t)0xFFFE) < 2) {
          /* write out 2 codes */
        if  (wSz >  (2 * sizeof(uint16_t)))  goto rcsNC;
        if ((wSz == (2 * sizeof(uint16_t))) && (c1 == 0))  goto rcsNC;
        if ((wSz <  (2 * sizeof(uint16_t))) && (c1 == 0))  goto cpo;
      }
      if ((wSz < (2 * sizeof(uint16_t))) && (c1 != 0))  goto rcsSV;
      c1 = (uint16_t)0xD7C0 + (uint16_t)(cp >> 10);
      c0 = 0xDC00 | (c0 & 0x3FF);
    suro:
      if (ws == wr1) {
        c1 = (c1 << 8) | (c1 >> 8);
        c0 = (c0 << 8) | (c0 >> 8);
      }
      *wStr++ = c1;
      *wStr++ = c0;
      wSz  -= (2 * sizeof(uint16_t));
      rStr += 2, rSz -= (2 * sizeof(uint32_t));
      if ((rSz - sizeof(uint32_t)) > rSz)  goto rcsRD;
      continue;
    }

    if ((c1 = (c0 - (uint16_t)0xD800)) < (uint16_t)0x0800) {
      if (c1 >= (uint16_t)0x0400)  goto rcsSI;
      if (((rSz - (2 * sizeof(uint32_t))) > rSz) ||
          ((wSz - (2 * sizeof(uint16_t))) > wSz))  goto rcsSV;
      if ((rStr[1] - (uint32_t)0xDC00) >= (uint32_t)0x0400)  goto rcsSI;
      c1 = c0;
      c0 = (uint16_t)rStr[1];
      goto suro;
    }

  cpo:
    if ((wSz - sizeof(uint16_t)) > wSz)  goto rcsWR;
    wSz -= sizeof(uint16_t);
    if (ws == wr1)
      c0 = ((c0 << 8) | (c0 >> 8));
    *wStr = c0, wStr += 1;
    rStr += 1, rSz -= sizeof(uint32_t);
    if (c0 == (uint16_t)EOSmark)  goto rcsNUL;
    if ((rSz - sizeof(uint32_t)) > rSz)  goto rcsRD;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = rSz;
  cnvrt->cs_rd = (void *)rStr;
  cnvrt->cs_wrSz = wSz;
  cnvrt->cs_wr = (void *)wStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (wSz == 0)  goto rcsRW;
  goto r_state;
rcsRW:
  cs_code = csRW;
  goto r_state;
rcsIU:
  cs_code = csIU;
  goto r_state;
rcsSV:
  cs_code = csSV;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

static csEnum
_U16dcd(csCnvrt_st *cnvrt, rd16_et rs) {

  csEnum cs_code;

  uint32_t *wStr  = (uint32_t *)cnvrt->cs_wr;
  size_t    wSz   = cnvrt->cs_wrSz;  // will be in bytes, sizeof()
  uint16_t *rStr = (uint16_t *)cnvrt->cs_rd;
  size_t    rSz  = cnvrt->cs_rdSz;  // will be in bytes, as like file size

  if ((wSz == 0) || (rStr == NULL))  goto rcsIE;

  do {
    uint32_t cp;
    uint16_t c0, c1;

      /* Assume most not surogate pair */
    if ((rSz - sizeof(uint16_t)) > rSz)  goto rcsRD;

    c0 = *rStr;
    if (rs == rd1)
      c0 = ((c0 << 8) | (c0 >> 8));

    if ((c0 - (uint32_t)0xD800) < (uint32_t)0x0800) goto suro;

    if ((c0 >= 0xFFFE) &&
        ((wSz != sizeof(uint32_t)) || (rSz != sizeof(uint16_t))))  goto rcsNC;

    cp = (uint32_t)c0;
  cpo:
    if ((wSz - sizeof(uint32_t)) > wSz)  goto rcsWR;
    wSz -= sizeof(uint32_t);
    *wStr++ = cp;
    rSz  -= sizeof(uint16_t);
    rStr += 1;
    if (cp == (uint32_t)EOSmark)  goto rcsNUL;
    if (wSz == 0)  goto rcsWR;
    continue;

  suro:
    if (c0 >= (uint32_t)0xDC00)  goto rcsSI;
    if ((rSz - (2 * sizeof(uint16_t))) > rSz)  goto rcsSV;

    c1 = rStr[1];
    if (rs == rd1)
      c1 = ((c1 << 8) | (c1 >> 8));
    cp = (((uint32_t)(c0 - (uint16_t)0xD7C0)) << 10)
         | (uint32_t)(c1 - (uint16_t)0xDC00);

    if (((uint32_t)(cp & (uint32_t)0xFFFF) - (uint32_t)0xFFFE) < (uint32_t)2)
      if ((wSz != sizeof(uint32_t)) || (rSz != sizeof(uint16_t)))  goto rcsNC;

    rSz  -= sizeof(uint16_t);
    rStr += 1;
    goto cpo;

  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = rSz;
  cnvrt->cs_rd = (void *)rStr;
  cnvrt->cs_wrSz = wSz;
  cnvrt->cs_wr = (void *)wStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (rSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  goto r_state;
rcsSV:
 cs_code = csSV;
 goto r_state;
rcsSI:
 cs_code = csSI;
 goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### UTF-16 Encoders

/* Encoder: unicode stream to UTF16 stream */
/* Unicode Stream in machine (internal use) */
/* UTF16 byte stream, endian sensitive casts */

/* Encoder: unicode codepoint stream to UTF-16BE stream */
csEnum
UStrtoUTF16BEStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U16ncd(cnvrt, wr1);
#elif defined (__BIG_ENDIAN__)
  return _U16ncd(cnvrt, wr2);
#else
  #error: undefined endianness
#endif
}
/* Encoder: unicode codepoint stream to UTF-16LE stream */
csEnum
UStrtoUTF16LEStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U16ncd(cnvrt, wr2);
#elif defined (__BIG_ENDIAN__)
  return _U16ncd(cnvrt, wr1);
#else
  #error: undefined endianness
#endif
}

/* Encoder: unicode codepoint stream to UTF-16 stream */
csEnum
UStrtoUTF16Str(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U16ncd(cnvrt, wr1);
#elif defined (__BIG_ENDIAN__)
  return _U16ncd(cnvrt, wr2);
#else
  #error: undefined endianness
#endif
}

#pragma mark #### UTF-16 Decoders

/* Decoder: UTF16 stream to unicode codepoint stream */
/* UTF16 byte stream, endian sensitive casts */
/* Unicode Stream in machine (internal use) */

/* Decoder: UTF-16BE stream to unicode codepoint stream */
csEnum
UTF16BEStrtoUStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U16dcd(cnvrt, rd1);
#elif defined (__BIG_ENDIAN__)
  return _U16dcd(cnvrt, rd2);
#else
  #error: undefined endianness
#endif
}

/* Decoder: UTF-16LE stream to unicode codepoint stream */
csEnum
UTF16LEStrtoUStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U16dcd(cnvrt, rd2);
#elif defined (__BIG_ENDIAN__)
  return _U16dcd(cnvrt, rd1);
#else
  #error: undefined endianness
#endif
}

/* Decoder: UTF-16 stream to unicode codepoint stream */
csEnum
UTF16StrtoUStr(csCnvrt_st *cnvrt) {

  size_t    u16Sz  = cnvrt->cs_rdSz;
  uint16_t *u16Str  = (uint16_t *)cnvrt->cs_rd;
  uint16_t  cp;

  if ((u16Sz == 0) || (u16Str == NULL))  return csIE;

  cp = *u16Str;
#ifdef __LITTLE_ENDIAN__
  if (cp == (uint16_t)0xFEFF)  return _U16dcd(cnvrt, rd2);
  return _U16dcd(cnvrt, rd1);
#elif defined (__BIG_ENDIAN__)
  if (cp == (uint16_t)0xFFFE)  return _U16dcd(cnvrt, rd1);
  return _U16dcd(cnvrt, rd2);
#else
  #error: undefined endianness
#endif
}


#pragma mark #### UTF-32

/* actual encoder/decoder work horse */

typedef enum _wr32 {  wr32,  wr8  } wr32_et;
typedef enum _rd32 {  rd32,  rd8  } rd32_et;

static csEnum
_U32out(csCnvrt_st *cnvrt, rd32_et rs, wr32_et ws) {

  uint32_t cp;
  size_t n;
  csEnum cs_code;

  uint32_t *rStr = (uint32_t *)cnvrt->cs_rd;
  size_t    rSz  = cnvrt->cs_rdSz;
  uint32_t *wStr = (uint32_t *)cnvrt->cs_wr;
  size_t    wSz  = cnvrt->cs_wrSz;

  if ((rSz == 0) || (rStr == NULL)) goto rcsIE;
  n = 0;
  cs_code = csRW;

  if (wSz < rSz) {
    cs_code = csWR;
  } else if (wSz > rSz) {
    cs_code = csRD;
    wSz = rSz;
  }

  /* wanted jump on condition instead of cmp/jmpc */
  if ((n |= wSz) == 0)  goto rcsIE;

  /* conditional cases: 00, 01, 10, not 11 */
  do {
    cp = *rStr;
    /* branch on conditional flag */
    if (rs == rd8)
      /* rlwm, rrwm, or; rotl and rotr and or; */
      cp = (((cp >> 8) | (cp << 24)) & 0xFF00FF00)
         | (((cp << 8) | (cp >> 24)) & 0x00FF00FF);

    if (cp > (uint32_t)0x10FFFF)  goto rcsIU;
    if ((cp - (uint32_t)0xD800) <= (uint32_t)(0xDFFF - 0xD800))  goto rcsSI;
    if (((uint32_t)(cp & (uint32_t)0xFFFF) - (uint32_t)0xFFFE) < 2)
      if ((wSz != sizeof(uint32_t)) || (cnvrt->cs_wrSz != cnvrt->cs_rdSz))  goto rcsNC;

    if (ws == wr8)
      cp = (((cp >> 8) | (cp << 24)) & 0xFF00FF00)
         | (((cp << 8) | (cp >> 24)) & 0x00FF00FF);

    if ((wSz - sizeof(uint32_t)) > wSz)  goto r_state;
    wSz -= sizeof(uint32_t);
    *wStr = cp;
    wStr += 1;
    rStr += 1;
    if (cp == EOSmark)  goto rcsNUL;
    if (wSz == 0)  goto r_state;
  } while (1);

rcsNUL:
  cs_code = csNUL;
nadj:
  n -= wSz;
r_state:
  cnvrt->cs_rdSz -= n;
  cnvrt->cs_rd = (void *)rStr;
  cnvrt->cs_wrSz -= n;
  cnvrt->cs_wr = (void *)wStr;
  return cs_code;

rcsSI:
  cs_code = csSI;
  goto nadj;
rcsIU:
  cs_code = csIU;
  goto nadj;
rcsNC:
  cs_code = csNC;
  goto nadj;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### UTF-32 Encoders

/* Encoder: unicode codepoint stream to UTF-32BE stream */
csEnum
UStrtoUTF32BEStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  /* unicode(LEmachine) to UTF32BE */
  return _U32out(cnvrt, rd32, wr8);
#elif defined (__BIG_ENDIAN__)
  /* unicode(BEmachine) to UTF32BE */
  return _U32out(cnvrt, rd32, wr32);
#else
  #error: undefined endianness
#endif
}
/* Encoder: unicode codepoint stream to UTF-32LE stream */
csEnum
UStrtoUTF32LEStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  /* unicode(LEmachine) to UTF32LE */
  return _U32out(cnvrt, rd32, wr32);
#elif defined (__BIG_ENDIAN__)
  /* unicode(BEmachine) to UTF32LE */
  return _U32out(cnvrt, rd32, wr8);
#else
  #error: undefined endianness
#endif
}

/* Encoder: unicode codepoint stream to UTF-32 stream */
csEnum
UStrtoUTF32Str(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  /* unicode(LEmachine) to UTF32BE */
  return _U32out(cnvrt, rd32, wr8);
#elif defined (__BIG_ENDIAN__)
  /* unicode(BEmachine) to UTF32BE */
  return _U32out(cnvrt, rd32, wr32);
#else
  #error: undefined endianness
#endif
}

#pragma mark #### UTF-32 Decoders

/* Decoder: UTF-32BE stream to unicode codepoint stream */
csEnum
UTF32BEStrtoUStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U32out(cnvrt, rd8, wr32);
#elif defined (__BIG_ENDIAN__)
  return _U32out(cnvrt, rd32, wr32);
#else
  #error: undefined endianness
#endif
}
/* Decoder: UTF-32LE stream to unicode codepoint stream */
csEnum
UTF32LEStrtoUStr(csCnvrt_st *cnvrt) {

#ifdef __LITTLE_ENDIAN__
  return _U32out(cnvrt, rd32, wr32);
#elif defined (__BIG_ENDIAN__)
  return _U32out(cnvrt, rd8, wr32);
#else
  #error: undefined endianness
#endif
}
/* Decoder: UTF-32 stream to unicode codepoint stream */
csEnum
UTF32StrtoUStr(csCnvrt_st *cnvrt) {

  uint32_t cp;

  if ((cnvrt->cs_wrSz == 0) || (cnvrt->cs_rdSz == 0)
      || (cnvrt->cs_rd == NULL))  return csIE;

  cp = *(uint32_t *)cnvrt->cs_rd;
#ifdef __LITTLE_ENDIAN__
  /* UTF32LE to unicode (in LE so 32bit reads) */
  if (cp == (uint32_t)0xFEFF)  return _U32out(cnvrt, rd32, wr32);
  /* case cp == 0xFEFF0000  UTF32BE to unicode (in LE so byte reads)
     neither so             UTF32BE to unicode (in LE so byte reads) */
  return _U32out(cnvrt, rd8, wr32);
#elif defined (__BIG_ENDIAN__)
  /* UTF32LE to unicode (in BE so byte reads) */
  if (cp == 0xFFFE0000)  return _U32out(cnvrt, rd8, wr32);
  /* case cp == 0x0000FEFF  UTF32BE to unicode (in BE so 32bit reads)
     neither so             UTF32BE to unicode (in BE so 32bit reads) */
  return _U32out(cnvrt, rd32, wr32);
#else
  #error: undefined endianness
#endif
}
