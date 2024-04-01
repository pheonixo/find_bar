/*
 *  cswindows.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Wed 13 Nov 2013.
 *  Copyright (c) 2013, 2022. All rights reserved.
 *
 */

#include "as_ctype.h" /* includes as_locale.h */

/* table dependancies */
#include "cswindows.h"

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

#pragma mark #### cswindows Encoders

/* Encoder: unicode stream to cswindows stream */
static csEnum
_cswindowsncd(csCnvrt_st *cnvrt, const uint16_t *table, uint32_t tSz) {

  csEnum cs_code;
  /* Variables */

  uint8_t *wndStr = (uint8_t *)cnvrt->cs_wr;
  size_t   wndSz  = cnvrt->cs_wrSz;
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
    if (cp > 0x2122)  goto rcsIC;

    gcclvalue = (uint16_t)cp;
    found = (uint16_t *)bsearch((const void *)&gcclvalue,
      (const void *)table, (size_t)(tSz >> 1), sizeof(uint32_t), &_compareFn);
    if (found == NULL)  goto rcsIC;
    cp = found[1];

  cntnu:
    if ((wndSz - (size_t)1) > wndSz)  goto rcsWR;
    *wndStr = (uint8_t)cp;
    wndStr += 1;
    wndSz  -= 1;
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
  cnvrt->cs_wrSz = wndSz;
  cnvrt->cs_wr = (void *)wndStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (wndSz == 0)  cs_code = csRW;
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
UStrtocs1251Str(csCnvrt_st *cnvrt) {
  return _cswindowsncd(cnvrt, _unicode_to_cswindows1251_table, 254);
}

csEnum
UStrtocs1252Str(csCnvrt_st *cnvrt) {
  return _cswindowsncd(cnvrt, _unicode_to_cswindows1252_table, 246);
}

csEnum
UStrtocs1256Str(csCnvrt_st *cnvrt) {
  return _cswindowsncd(cnvrt, _unicode_to_cswindows1256_table, 256);
}


#pragma mark #### cswindows Decoders

/* Decoder: cswindows stream to unicode codepoint stream */
static csEnum
_cswindowsdcd(csCnvrt_st *cnvrt, const uint16_t *table) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t *wndStr = (uint8_t *)cnvrt->cs_rd;
  size_t   wndSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (wndStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
  do {
    uint32_t cp;

    if ((wndSz - (size_t)1) > wndSz)  goto rcsRD;
    cp = (uint32_t)*wndStr;

    if (cp >= 0x80) {
      if (cp > 0xFF)  goto rcsIC;
      cp = (uint32_t)table[(cp - 0x80)];
      if (cp == 0xFFFF)  goto rcsNC;
    }

    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz  -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr  += 1;
    wndSz  -= 1;
    wndStr += 1;
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = wndSz;
  cnvrt->cs_rd = (void *)wndStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (wndSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
cs1251StrtoUStr(csCnvrt_st *cnvrt) {
  return _cswindowsdcd(cnvrt, _cswindows1251_to_unicode_table);
}

csEnum
cs1252StrtoUStr(csCnvrt_st *cnvrt) {
  return _cswindowsdcd(cnvrt, _cswindows1252_to_unicode_table);
}

csEnum
cs1256StrtoUStr(csCnvrt_st *cnvrt) {
  return _cswindowsdcd(cnvrt, _cswindows1256_to_unicode_table);
}
