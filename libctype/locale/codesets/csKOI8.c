/*
 *  csKOI8.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Wed 13 Nov 2013.
 *  Copyright (c) 2013, 2022. All rights reserved.
 *
 */

#include "as_ctype.h" /* includes as_locale.h */

/* table dependancies */
#include "csKOI8.h"

/* function dependancies */
extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*compar)(const void *, const void *));

/* internal function */
static int
_compareFn(const void *a, const void *b) {

  uint16_t a0 = ((uint16_t *)a)[0];
  uint16_t b0 = ((uint16_t *)b)[0];
  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  return 0;
}

#pragma mark #### KOI8 Encoders

/* Encoder: unicode stream to KOI8 stream */
static csEnum
_KOI8ncd(csCnvrt_st *cnvrt, const uint16_t *table, uint32_t tSz) {

  csEnum cs_code;
  /* Variables */

  uint8_t  *K8Str = (uint8_t *)cnvrt->cs_wr;
  size_t    K8Sz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;
  uint32_t  cp;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  cp = *cpStr;
  do {
    uint16_t *found, gcclvalue;

    if (cp < 0x80)  goto cntnu;

    if (cp > 0x10FFFF)  goto rcsIU;
    /* highest unicode code point in any of the tables */
    if (cp > 0x25A0)  goto rcsIC;

    gcclvalue = (uint16_t)cp;
    found = (uint16_t *)bsearch((const void *)&gcclvalue,
      (const void *)table, (size_t)(tSz >> 1), sizeof(uint32_t), &_compareFn);
    if (found == NULL)  goto rcsIC;
    cp = found[1];

  cntnu:
    if ((K8Sz - (size_t)1) > K8Sz)  goto rcsWR;
    *K8Str = (uint8_t)cp;
    K8Str += 1;
    K8Sz  -= 1;
    cpStr += 1;
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
  cnvrt->cs_wrSz = K8Sz;
  cnvrt->cs_wr = (void *)K8Str;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (K8Sz == 0)  cs_code = csRW;
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
UStrtoKOI8UStr(csCnvrt_st *cnvrt) {
  return _KOI8ncd(cnvrt, _unicode_to_KOI8U_table, 256);
}

csEnum
UStrtoKOI8RStr(csCnvrt_st *cnvrt) {
  return _KOI8ncd(cnvrt, _unicode_to_KOI8R_table, 256);
}


#pragma mark #### KOI8 Decoders

/* Decoder: KOI8 stream to unicode codepoint stream */
static csEnum
_KOI8dcd(csCnvrt_st *cnvrt, const uint16_t *table) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *K8Str = (uint8_t *)cnvrt->cs_rd;
  size_t    K8Sz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (K8Str == NULL))  goto rcsIE;

  do {
    uint32_t cp;

    if ((K8Sz - (size_t)1) > K8Sz)  goto rcsRD;
    cp = (uint32_t)*K8Str;

    if (cp >= 0x80) {
      if (cp > 0xFF)  goto rcsIC;
      cp = (uint32_t)table[(cp - 0x80)];
    }

    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr = cp, cpStr += 1;
    K8Sz  -= 1, K8Str += 1;
    if (cp == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = K8Sz;
  cnvrt->cs_rd = (void *)K8Str;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (K8Sz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
KOI8UStrtoUStr(csCnvrt_st *cnvrt) {
  return _KOI8dcd(cnvrt, _KOI8U_to_unicode);
}

csEnum
KOI8RStrtoUStr(csCnvrt_st *cnvrt) {
  return _KOI8dcd(cnvrt, _KOI8R_to_unicode);
}
