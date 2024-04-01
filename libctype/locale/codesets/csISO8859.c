/*
 *  csISO8859.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Sat 9 Nov 2013.
 *  Copyright (c) 2013, 2022. All rights reserved.
 *
 */

/*
 * To be used in locale's ctype handling.
 * Not restricted to ctype handling.
 * csEnum, csCnvrt_st defined in as_locale_ctype.h.
 * Template design (find and replace):
 *  replace ISO8859
 *  replace uint8_t
 * at this point, if using same prefix
 * for Str and Sz replace iso, DO NOT USE cp
 *  replace isoStr
 *  replace isoSz
 */

#include "as_ctype.h" /* includes as_locale.h */
/* table dependancies */
#include "csISO8859.h"
/* function dependancies */
extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*compar)(const void *, const void *));

/* broken fix */
/*unsigned getISO8859IndexValue(unsigned i, request r);*/

#pragma mark #### ISO8859 Encoders

/* internal function */
static int
_compareFn(const void *a, const void *b) {

  uint16_t a0 = ((uint16_t *)a)[0];
  uint16_t b0 = ((uint16_t *)b)[0];
  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  return 0;
}

/* Encoder: unicode stream to ISO8859 stream */
static csEnum
_ISO8859ncd(csCnvrt_st *cnvrt, const uint16_t *table, uint32_t tSz) {

  csEnum cs_code;
  /* Variables */

  uint8_t *isoStr = (uint8_t *)cnvrt->cs_wr;
  size_t   isoSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;
  uint32_t  cp;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  cp = *cpStr;

  do {
    uint16_t *found, gcclvalue;

    if (cp <= 0xA0)  goto cntnu;

    if (cp > 0x10FFFF)  goto rcsIU;
    /* highest unicode code point in any of the tables */
    if (cp > 0x2116)  goto rcsIC;

    gcclvalue = (uint16_t)cp;
    found = (uint16_t *)bsearch((const void *)&gcclvalue,
        (const void *)table, (size_t)(tSz >> 1), sizeof(uint32_t), &_compareFn);
    if (found == NULL)  goto rcsIC;
    cp = found[1];

  cntnu:
    if ((isoSz - (size_t)1) > isoSz)  goto rcsWR;
    isoSz  -= 1;
    *isoStr = (uint8_t)cp;
    isoStr += 1;
    cpStr  += 1;
    cpSz   -= sizeof(uint32_t);
    if (cp == EOSmark)  goto rcsNUL;
    if (cpSz - sizeof(uint32_t) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = isoSz;
  cnvrt->cs_wr = (void *)isoStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (isoSz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
 cs_code = csIC;
 goto r_state;
rcsIU:
  cs_code = csIU;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoASCIIStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t *isoStr = (uint8_t *)cnvrt->cs_wr;
  size_t   isoSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;
  uint32_t  cp;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  cp = *cpStr;
  do {
    if (cp >= 0x80)  goto rcsIC;
    if ((isoSz - (size_t)1) > isoSz)  goto rcsWR;
    isoSz  -= 1;
    *isoStr = (uint8_t)cp;
    isoStr += 1;
    cpStr  += 1;
    cpSz   -= sizeof(uint32_t);
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = isoSz;
  cnvrt->cs_wr = (void *)isoStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (isoSz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
 cs_code = csIC;
 goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoISO1Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t *isoStr = (uint8_t *)cnvrt->cs_wr;
  size_t   isoSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;
  uint32_t  cp;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  cp = *cpStr;
  do {
    if (cp >= 0x100)  goto rcsIC;
    if ((isoSz - (size_t)1) > isoSz)  goto rcsWR;
    isoSz  -= 1;
    *isoStr = (uint8_t)cp;
    isoStr += 1;
    cpStr  += 1;
    cpSz -= sizeof(uint32_t);
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = isoSz;
  cnvrt->cs_wr = (void *)isoStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (isoSz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
 cs_code = csIC;
 goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoISO2Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatin2_table, 192);
}

csEnum
UStrtoISO3Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatin3_table, 178);
}

csEnum
UStrtoISO4Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatin4_table, 192);
}

csEnum
UStrtoISO5Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatinCyrillic_table, 192);
}

csEnum
UStrtoISO6Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatinArabic_table, 102);
}

csEnum
UStrtoISO7Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatinGreek_table, 186);
}

csEnum
UStrtoISO8Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatinHebrew_table, 120);
}

csEnum
UStrtoISO9Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatin5_table, 192);
}

csEnum
UStrtoISO10Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISOLatin6_table, 192);
}

csEnum
UStrtoISO11Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csTIS620_table, 176);
}

csEnum
UStrtoISO13Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISO885913_table, 192);
}

csEnum
UStrtoISO14Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISO885914_table, 192);
}

csEnum
UStrtoISO15Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISO885915_table, 192);
}

csEnum
UStrtoISO16Str(csCnvrt_st *cnvrt) {
  return _ISO8859ncd(cnvrt, _unicode_to_csISO885916_table, 192);
}

#pragma mark #### ISO8859 Decoders

/* Decoder: ISO8859 stream to unicode codepoint stream */
static csEnum
_ISO8859dcd(csCnvrt_st *cnvrt, const uint16_t *table) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t *isoStr = (uint8_t *)cnvrt->cs_rd;
  size_t   isoSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (isoStr == NULL))  goto rcsIE;

  do {
    uint32_t cp;

    if ((isoSz - (size_t)1) > isoSz)  goto rcsRD;
    cp = (uint32_t)*isoStr;

    if (cp > 0xA0) {
      cp = (uint32_t)table[(cp - 0xA0)];
      if (cp == 0xFFFF)  goto rcsNC;
    }

    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr  += 1;
    isoSz  -= 1;
    isoStr += 1;
    if (cp == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = isoSz;
  cnvrt->cs_rd = (void *)isoStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (isoSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
ASCIIStrtoUStr(csCnvrt_st *cnvrt) {
  return ISO1StrtoUStr(cnvrt);
}

csEnum
ISO1StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t *isoStr = (uint8_t *)cnvrt->cs_rd;
  size_t   isoSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (isoStr == NULL))  goto rcsIE;

  do {
    uint8_t cs0;
    if ((isoSz - (size_t)1) > isoSz)  goto rcsRD;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    cs0 = *isoStr;
    *cpStr = (uint32_t)cs0;
    cpStr  += 1;
    isoSz  -= 1;
    isoStr += 1;
    if (cs0 == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = isoSz;
  cnvrt->cs_rd = (void *)isoStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (isoSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
ISO2StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatin2_to_unicode_table);
}

csEnum
ISO3StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatin3_to_unicode_table);
}

csEnum
ISO4StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatin4_to_unicode_table);
}

csEnum
ISO5StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatinCyrillic_to_unicode_table);
}

csEnum
ISO6StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatinArabic_to_unicode_table);
}

csEnum
ISO7StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatinGreek_to_unicode_table);
}

csEnum
ISO8StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatinHebrew_to_unicode_table);
}

csEnum
ISO9StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatin5_to_unicode_table);
}

csEnum
ISO10StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISOLatin6_to_unicode_table);
}

csEnum
ISO11StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csTIS620_to_unicode_table);
}

csEnum
ISO13StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISO885913_to_unicode_table);
}

csEnum
ISO14StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISO885914_to_unicode_table);
}

csEnum
ISO15StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISO885915_to_unicode_table);
}

csEnum
ISO16StrtoUStr(csCnvrt_st *cnvrt) {
  return _ISO8859dcd(cnvrt, _csISO885916_to_unicode_table);
}
