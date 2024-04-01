/*
 *  csBig5.c - Big5 conversions
 *  PheonixOS
 *
 *  Created by Steven Abner on Thu 14 Nov 2013.
 *  Copyright (c) 2013, 2019, 2022. All rights reserved.
 *
 *  New tables created from tw.gov and add in of honk kongs.
 *  So hkscs runs, if values not included, get it from big5.
 *  Variants are to replace "Reserved for user-defined characters"
 *  however, doesn't state replaces entire areas, and hkscs even
 *  maps some outside big5 definition.
 *
 *  hkscs also has some double mappings so round trip defines are not 100%.
 */

#include "as_ctype.h" /* includes as_locale.h */
/* table dependancies */
#include "csBig5.h"
#include "csBig5_hkscs.h"

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

static int
_hkscsFn(const void *a, const void *b) {
  int32_t key = *(int32_t*)a;
  int32_t cp = (int32_t)(((struct _hkscs*)b)->uni);
  return (int)(key - cp);
}

static int
_nFind(const void *a, const void *b) {
  return (int)(*(uint16_t*)a - *(uint16_t*)b);
}

#pragma mark #### Big5 Encoders

/* Encoder: unicode stream to Big5 stream */
csEnum
UStrtoBig5Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint16_t *B5Str = (uint16_t *)cnvrt->cs_wr;
  size_t    B5Sz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;

  do {
    uint32_t cp;
    struct _b5 *found;
    uint16_t gcclvalue;

    cp = *cpStr;

      /* Americanize */
    if (cp < 0x80) {
      if ((B5Sz - (size_t)1) > B5Sz)  goto rcsWR;
      char *bptr = (char*)B5Str;
      *bptr = cp;
      B5Str = (uint16_t*)(bptr + 1);
      B5Sz  -= 1;
      cpStr += 1;
      cpSz -= sizeof(uint32_t);
      if (cp == 0)  goto rcsNUL;
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
      continue;
    }

      /* bound table input */
    if (cp > 0xFFE5)  goto rcsIC;

      /* Unicode to Big5 code point conversion */
    gcclvalue = cp;
    found = (struct _b5*)bsearch(&gcclvalue, _unicode_to_Big5_table,
                            sizeof(_unicode_to_Big5_table)/sizeof(struct _b5),
                                              sizeof(struct _b5), &_compareFn);
    if (found == NULL)  goto rcsIC;
    gcclvalue = found->b5;
#ifdef __LITTLE_ENDIAN__
    gcclvalue = (gcclvalue >> 8) | (gcclvalue << 8);
#endif
      /* Big5 is 2-byte system */
    if ((B5Sz - (size_t)2) > B5Sz)  goto rcsWR;
    *B5Str = gcclvalue;
      /* Update stream info */
    B5Str += 1;
    B5Sz  -= 2;
    cpStr += 1;
    cpSz -= sizeof(uint32_t);
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = B5Sz;
  cnvrt->cs_wr = (void *)B5Str;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (B5Sz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

/* Encoder: unicode stream to Big5_hkscs stream */
csEnum
UStrtoHKSCSStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint16_t *B5Str = (uint16_t *)cnvrt->cs_wr;
  size_t    B5Sz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;

  do {
    uint32_t cp;
    uint16_t gcclvalue16;
    struct _hkscs *found_hkscs;
    uint32_t gcclvalue32;

    cp = *cpStr;
    if (cp == 0)  goto cntnu;
//      /* Americanize */
//    if (cp < 0x80) {
//      if ((B5Sz - (size_t)1) > B5Sz)  goto rcsWR;
//      char *bptr = (char*)B5Str;
//      *bptr = cp;
//      cpStr += 1;
//      B5Str = (uint16_t*)(bptr + 1);
//      B5Sz  -= 1;
//      cpSz -= sizeof(uint32_t);
//      if (cp == 0)  goto rcsNUL;
//      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
//      continue;
//    }

    /* bound table input, big5 bind only */
    /*if (cp > 0xFFE5)  goto rcsIC;*/
    /* check out of Big5 first */
    if ((gcclvalue16 = cp) == cp) {
      uint16_t *found16 = bsearch(&gcclvalue16, non_b5,
                                            sizeof(non_b5)/sizeof(uint16_t),
                                                    sizeof(uint16_t), &_nFind);
      if (found16 != NULL) {
        cp = *found16;
        goto cntnu;
      }
    }
      /* Unicode to Big5 code point conversion */
    gcclvalue32 = cp;
    found_hkscs = (struct _hkscs*)bsearch(&gcclvalue32, _unicode_to_hkscs_table,
                            sizeof(_unicode_to_hkscs_table)/sizeof(struct _hkscs),
                                              sizeof(struct _hkscs), &_hkscsFn);
    if (found_hkscs == NULL) {
       struct _b5 *found_b5;
       /* bound table input, big5 bind only */
      if (cp > 0xFFE5)  goto rcsIC;
      found_b5 = (struct _b5*)bsearch(&gcclvalue16, _unicode_to_Big5_table,
                              sizeof(_unicode_to_Big5_table)/sizeof(struct _b5),
                                                sizeof(struct _b5), &_compareFn);
      if (found_b5 == NULL)  goto rcsIC;
      cp = found_b5->b5;
      goto cntnu;
    }
    cp = found_hkscs->scs;
    if (found_hkscs->scs == 0x0FFFE) {
        /* need to check second uni to determine return */
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[0] == 0x00CA)
            cp = 0x8866;
      else  cp = 0x88A7;
      if      (cpStr[1] == 0x0304) {  cp -= 4, cpStr += 1, cpSz  -= 4;  }
      else if (cpStr[1] == 0x030C) {  cp -= 2, cpStr += 1, cpSz  -= 4;  }
    }

  cntnu:
    if ((B5Sz - sizeof(uint16_t)) > B5Sz)  goto rcsWR;
#ifdef __LITTLE_ENDIAN__
    cp = ((uint16_t)cp >> 8) | (cp << 8);
#endif
    *B5Str = cp;
      /* Update stream info */
    B5Str += 1;
    B5Sz  -= sizeof(uint16_t);
    cpStr += 1;
    cpSz -= sizeof(uint32_t);
    if (cp == 0)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = B5Sz;
  cnvrt->cs_wr = (void *)B5Str;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (B5Sz == 0)  cs_code = csRW;
  goto r_state;
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### Big5 Decoders

/* Decoder: Big5 stream to unicode codepoint stream */
csEnum
Big5StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;
  uint16_t  cs0;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *B5Str = (uint8_t *)cnvrt->cs_rd;
  size_t    B5Sz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (B5Str == NULL))  goto rcsIE;

  do {

      /* Americanize */
    if ((B5Sz - (size_t)1) > B5Sz)  goto rcsRD;
    if ((cs0 = (uint16_t)*B5Str) < 0x80) {
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      cpSz -= sizeof(uint32_t);
      *cpStr = (uint32_t)cs0, cpStr += 1;
      B5Str += 1, B5Sz  -= 1;
      if (cs0 == 0)  goto rcsNUL;
      if (cpSz == 0)  goto rcsWR;
      continue;
    }

      /* Big5 is 2-byte system */
    if ((B5Sz - (size_t)2) > B5Sz)  goto rcsRD;
    cs0 = *(uint16_t*)B5Str;
#ifdef __LITTLE_ENDIAN__
    cs0 = ((cs0 & 0x0FF) << 8) | cs0 >> 8;
#endif
    /* get rid of NC and user-defined */
    if (((uint8_t)(cs0 >> 8) - (uint8_t)0x81) >= ((uint8_t)0xFE - (uint8_t)0x81))
      goto rcsIC;
    if (((uint8_t)(cs0 & 0xFF) < (uint8_t)0x40) ||
                                ((uint8_t)(cs0 & 0xFF) == (uint8_t)0xFF))
      goto rcsIC;
    if (((uint8_t)(cs0 & 0xFF) > (uint8_t)0x7E) &&
                                ((uint8_t)(cs0 & 0xFF) < (uint8_t)0xA1))
      goto rcsIC;
    /* now excluded table areas, valid but unused */
    if ((cs0 > (uint16_t)0x0875C) && (cs0 < (uint16_t)0x08E40))
      goto rcsNC;
    if ((cs0 > (uint16_t)0x0C680) && (cs0 < (uint16_t)0x0C940))
      goto rcsNC;

    /* now all code points are mapped by table */
    /* convert code point to index value used by table */
    cs0 = ((uint16_t)((cs0 - 0x8100) >> 8) * (uint16_t)0xA0) +
          (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                ((uint8_t)cs0 - (uint8_t)0x040) :
                ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
    /* subtract out the non-defined areas, see 'data' for full hex write up */
    if (cs0 > 11103)  cs0 -= 416;
    if (cs0 > 988)    cs0 -= 1091;

    /* derived index obtained, use it */
    cs0 = _Big5_to_unicode_table[cs0];
    /* should get back non-mapped codes, non-characters done in preamble */
    if (cs0 == 0xFFFF)  goto rcsIC;

    /* Assign valid Big5 code point to Unicode stream */
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz  -= sizeof(uint32_t);
    *cpStr = (uint32_t)cs0, cpStr += 1;
    B5Str += 2, B5Sz  -= 2;
    if (cs0 == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = B5Sz;
  cnvrt->cs_rd = (void *)B5Str;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (B5Sz == 0)  cs_code = csRW;
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

/* Decoder: Big5_hkscs stream to unicode codepoint stream */
csEnum
HKSCSStrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;
    /* note: these can return > uint16_t */
  uint32_t  cs0;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *B5Str = (uint8_t *)cnvrt->cs_rd;
  size_t    B5Sz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (B5Str == NULL))  goto rcsIE;

  do {
    uint16_t *fnb5;

      /* Big5 is 2-byte system */
    if ((B5Sz - (size_t)2) > B5Sz)  goto rcsRD;
    cs0 = (uint32_t)*(uint16_t*)B5Str;
#ifdef __LITTLE_ENDIAN__
    cs0 = ((cs0 & 0x0FF) << 8) | cs0 >> 8;
#endif
    if (cs0 == EOSmark)  goto cntnu;

      /* test for outside Big5 definition points firstly */
    fnb5 = (uint16_t*)bsearch(&cs0, non_b5, sizeof(non_b5)/sizeof(uint16_t),
                                              sizeof(uint16_t), &_compareFn);
      /* in non_b5 searched for returns exact number searched, cs0 == *fnb5 */
    if (fnb5 != NULL)  goto cntnu;

//      /* Americanize (Big5 difference because it includes 'prefixes' < 0x80 */
//    if (((uint8_t)cs0 < 0x080) && ((uint8_t)(cs0 >> 8) < 0x080)) {
//      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
//      cpSz -= sizeof(uint32_t);
//      *cpStr = (uint8_t)cs0, cpStr += 1;
//      B5Str += 1, B5Sz  -= 1;
//      if (cs0 == EOSmark)  goto rcsNUL;
//      if (cpSz == 0)  goto rcsWR;
//      continue;
//    }
      /* now all Big5 encodings */
      /* get rid of NC and user-defined */
    if (((uint8_t)(cs0 >> 8) - (uint8_t)0x81) >= ((uint8_t)0xFF - (uint8_t)0x81))
      goto rcsNC;
    if (((uint8_t)(cs0 & 0xFF) < (uint8_t)0x40) ||
        ((uint8_t)(cs0 & 0xFF) == (uint8_t)0xFF))
      goto rcsNC;
    if (((uint8_t)(cs0 & 0xFF) > (uint8_t)0x7E) &&
        ((uint8_t)(cs0 & 0xFF) < (uint8_t)0xA1))
      goto rcsNC;
    /* now excluded table areas, valid but unused */
    /* however, hkscs might define these areas */

    /* these in niether hkscs nor big5 */
    if ((cs0 > (uint16_t)0x088AA) && (cs0 < (uint16_t)0x08940))
      goto rcsNC;
    /* these only in big5... Note: use 8100, not hkscs 8700 */
    if (cs0 < (uint16_t)0x08740) {
      cs0 = ((uint16_t)((cs0 - 0x8100) >> 8) * (uint16_t)0xA0) +
            (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                  ((uint8_t)cs0 - (uint8_t)0x040) :
                  ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
      cs0 = _Big5_to_unicode_table[cs0];
      if (cs0 == 0xFFFF)  goto rcsIC;
      goto cntnu;
    }
    if ((cs0 > (uint16_t)0x0A13F) && (cs0 < (uint16_t)0x0C67F)) {
      cs0 = ((uint16_t)((cs0 - 0x8100) >> 8) * (uint16_t)0xA0) +
            (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                  ((uint8_t)cs0 - (uint8_t)0x040) :
                  ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
      cs0 -= 1091;
      cs0 = _Big5_to_unicode_table[cs0];
      if (cs0 == 0xFFFF)  goto rcsIC;
      goto cntnu;
    }
    if ((cs0 > (uint16_t)0x0C93F) && (cs0 < (uint16_t)0x0F9D6)) {
      cs0 = ((uint16_t)((cs0 - 0x8100) >> 8) * (uint16_t)0xA0) +
            (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                  ((uint8_t)cs0 - (uint8_t)0x040) :
                  ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
      cs0 -= 1507;
      cs0 = _Big5_to_unicode_table[cs0];
      if (cs0 == 0xFFFF)  goto rcsIC;
      goto cntnu;
    }
    /* now search hkscs for code, if not found check big5 */
    /* < 8740 addressed, block [88AB-88FE] missing addressed,
     * block [A140-C67E] missing addressed, block [C940-F9D5]
     * missing addressed. Just exclusion from hkscs search math needed */

    /* now all code points are mapped by table */
    /* convert code point to index value used by table (8700 start) */
    cs0 = ((uint16_t)((cs0 - 0x8700) >> 8) * (uint16_t)0xA0) +
          (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                ((uint8_t)cs0 - (uint8_t)0x040) :
                ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
      /* subtract out the non-defined areas, see 'data' for full hex write up */
    if (cs0 >= 18358)  cs0 -= 7798; /* 18240+150,  7680+118 (80-40)(D6-A0) */
      /* [8740-C640], 63 blocks * 160 + 7E-40 == 5920+64 @ 5985 */
    if (cs0 >= 10145)  cs0 -= 5985;
    if (cs0 > 234)     cs0 -= 85;     /*2 blocks / 85 missing in second*/

    /* derived index obtained, use it */
    cs0 = _hkscs_to_unicode_table[cs0];
    /* check for flagged double unicode codes */
    if (cs0 == 0x0FFFE) {
      /* use knowledge instead of table */
      (void)extbl;
      cs0 = *(uint16_t*)B5Str;
#ifdef __LITTLE_ENDIAN__
      cs0 = ((cs0 & 0x0FF) << 8) | cs0 >> 8;
#endif
      if (cs0 <= 0x8866) {
        if (cs0 == 0x8866) {  cs0 = 0x00CA;  goto cntnu;  }
        if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsWN;
        *cpStr = (uint32_t)0x00CA;
        cpStr += 1, cpSz -= sizeof(uint32_t);
        cs0 = (cs0 == 0x8864) ? 0x030C : 0x0304;
        goto cntnu;
      }
      cs0 = 0x00EA;
#ifdef __LITTLE_ENDIAN__
      if (*(uint16_t*)B5Str == 0xA788)  goto cntnu;
#else
      if (*(uint16_t*)B5Str == 0x88A7)  goto cntnu;
#endif
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsWN;
      *cpStr = cs0;
      cpStr += 1, cpSz -= sizeof(uint32_t);
#ifdef __LITTLE_ENDIAN__
      cs0 = (*(uint16_t*)B5Str == 0xA588) ? 0x030C : 0x0304;
#else
      cs0 = (*(uint16_t*)B5Str == 0x88A5) ? 0x030C : 0x0304;
#endif
    }
    /* should get back non-mapped codes, non-characters done in preamble */
    else if (cs0 == 0xFFFF) {
      /* since not found in hkscs, try big5 */
      /* indexing different, cs0 not valid on _Big5_to_unicode_table */
      cs0 = (uint32_t)*(uint16_t*)B5Str;
#ifdef __LITTLE_ENDIAN__
      cs0 = ((cs0 & 0x0FF) << 8) | cs0 >> 8;
#endif
      cs0 = ((uint16_t)((cs0 - 0x8100) >> 8) * (uint16_t)0xA0) +
            (((uint8_t)cs0 < (uint8_t)0x0A0) ?
                  ((uint8_t)cs0 - (uint8_t)0x040) :
                  ((uint8_t)cs0 - (uint8_t)0x0A0 + (uint8_t)0x040));
      /* subtract out the non-defined areas, see 'data' for full hex write up */
      if (cs0 > 11103)  cs0 -= 416;
      if (cs0 > 988)    cs0 -= 1091;
      /* derived index obtained, use it */
      cs0 = _Big5_to_unicode_table[cs0];
      /* should get back non-mapped codes, non-characters done in preamble */
      if (cs0 == 0xFFFF)  goto rcsIC;
    }

    /* Assign valid Big5 code point to Unicode stream */
  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr = (uint32_t)cs0, cpStr += 1;
    B5Str += 2;  B5Sz -= 2;
    if (cs0 == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = B5Sz;
  cnvrt->cs_rd = (void *)B5Str;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (B5Sz == 0)  cs_code = csRW;
  goto r_state;

rcsRD:
  cs_code = csRD;
  goto r_state;
rcsWN:
  cs_code = csWN;
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

