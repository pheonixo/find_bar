/*
 *  csPTCP154.c
 *  PheonixOS
 *
 *  Created by Steven Abner on Wed 13 Nov 2013.
 *  Copyright (c) 2013, 2022. All rights reserved.
 *
 */

#include "as_ctype.h" /* includes as_locale.h */
/* table dependancies */
#include "csPTCP154.h"
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

#pragma mark #### PTCP154 Encoders

/* Encoder: unicode stream to PTCP154 stream */
csEnum
UStrtoPTCP154Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *PTStr = (uint8_t *)cnvrt->cs_wr;
  size_t    PTSz  = cnvrt->cs_wrSz;
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
    if (cp > 0x2116)  goto rcsIC;

    gcclvalue = (uint16_t)cp;
    found = (uint16_t *)bsearch((const void *)&gcclvalue,
                                (const void *)_unicode_to_csPTCP154,
                                (size_t)64, sizeof(uint32_t), &_compareFn);
    if (found == NULL) {
      if ((cp - (uint32_t)0x0410) >= 0x40)  goto rcsIC;
      cp -= 0x0350;
      goto cntnu;
    }
    cp = found[1];

  cntnu:
    if ((PTSz - (size_t)1) > PTSz)  goto rcsWR;
    *PTStr = (uint8_t)cp;
    PTStr += 1;
    PTSz  -= 1;
    cpStr += 1;
    cpSz  -= sizeof(uint32_t);
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = PTSz;
  cnvrt->cs_wr = (void *)PTStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (PTSz == 0)  cs_code = csRW;
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

#pragma mark #### PTCP154 Decoders

/* Decoder: PTCP154 stream to unicode codepoint stream */
csEnum
PTCP154StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *PTStr = (uint8_t *)cnvrt->cs_rd;
  size_t    PTSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (PTStr == NULL))  goto rcsIE;

  do {
    uint32_t cp;

    if ((PTSz - (size_t)1) > PTSz)  goto rcsRD;
    cp = (uint32_t)*PTStr;

    if (cp >= 0x80) {
      if (cp > 0xFF)  goto rcsIC;
      if (cp > 0xBF)  cp += 0x0350;
      else            cp = (uint32_t)_csPTCP154_to_unicode[(cp - 0x80)];
    }

    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz  -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr += 1;
    PTSz  -= 1;
    PTStr += 1;
    if (cp == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = PTSz;
  cnvrt->cs_rd = (void *)PTStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (PTSz == 0)  cs_code = csRW;
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
