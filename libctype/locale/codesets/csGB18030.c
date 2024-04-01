/*
 *  csGB18030.c - encode/decode GB18030 strings
 *  Pheonix
 *
 *  Created by Steven Abner on Mon 13 May 2019.
 *  Copyright (c) 2019, 2022. All rights reserved.
 *
 *  Originally created on Sun 26 Aug 2012.
 *  See archives for original.
 */

#include "as_ctype.h"
/* table dependancies */
#include "csGB18030.h"
/* function dependancies */
extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*compar)(const void *, const void *));
/* for error testing */
int printf(const char *restrict, ...);


/* returns found or closest to key */
static void *
nearch(const void *key, const void *base, size_t nel, size_t width,
        int (*compar)(const void *, const void *)) {

  const void *p = NULL;
  size_t low = 0, high = nel;
  if (high)
    do {
      size_t mid = low + ((high - low) >> 1);
      p = (const void *)((const char *)base + (width * mid));
      int c = (*compar)(key, p);
      if (!c) return (void *)p;
      if (c > 0) low = ++mid;
      else high = mid;
    } while (low < high);
  return (void *)p;
}

#pragma mark #### GB18030 Encoders

static int
gFind(const void *a, const void *b) {

  uint16_t key = *(uint16_t*)a;
  uint16_t query = ((struct _b2pts *)b)->uni;
  uint16_t addin = ((struct _b2pts *)b)->length;

  if (key == query)  return 0;
  if ((key >= query) && (key < (query + addin)))
    return 0;
  if (key < query) return -1;
  return 1;
}

/* Encoder: unicode stream to GB18030 stream */
csEnum
UStrtoGB18030Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_wr;
  size_t    GBSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;

    /* reading in 32bit unicode codes */
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {

    if (cp < 0x80) {
        /* test if can write to GBStr */
      if ((GBSz - (size_t)1) > GBSz)  goto rcsWR;
      *GBStr = (uint8_t)cp;
      GBStr += 1, GBSz -= 1;
      cpStr += 1, cpSz -= sizeof(uint32_t);
      if (cp == EOSmark)  goto rcsNUL;
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
      cp = *cpStr;
      continue;
    }
      /* these use to be completely indexed */
    if (cp > 0xFFFF) {
      if ((GBSz - (size_t)2) > GBSz)  goto rcsWR;
      if (cp == 0x20087) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x51');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      if (cp == 0x20089) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x52');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      if (cp == 0x200CC) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x53');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      if (cp == 0x215D7) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x6C');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      if (cp == 0x2298F) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x76');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      if (cp == 0x241FE) {
        *(uint16_t*)GBStr = cconst16_t('\xFE','\x91');
        GBStr += 2, GBSz -= 2;
        goto next_code;
      }
      cp += 0x1e248;
      goto calc4;
    }
      /* these are found by tables */
    unsigned idx, base = 0x080;
    uint16_t key = (uint16_t)cp;
    struct _b2pts *found =
        nearch(&key, indexgb18030, (size_t)207, sizeof(struct _b2pts), &gFind);
    if ((key == found->uni) ||
        ((key >= found->uni) && (key < (found->uni + found->length)))) {
      uint16_t rVal;
      if (key >= 0x0D800) {
          /* void area */
        if (key < 0x0E000)  goto rcsSI;
        base = 0x0880;
      }
        /* GBK areas */
      idx = found->sum - found->length - base;
      idx += key - found->uni;
      rVal = _unicode_to_GB18030_table[idx];
#ifdef __LITTLE_ENDIAN__
      rVal = (uint16_t)((uint8_t)(rVal >> 8)) | ((uint16_t)((uint8_t)rVal) << 8);
#endif
      if ((GBSz - (size_t)2) > GBSz)  goto rcsWR;
      GBSz -= 2;
      *(uint16_t*)GBStr = rVal;
      GBStr += 2;
      goto next_code;
    }
    unsigned nearch_sum;
    if (found->uni > key)  nearch_sum = found->sum - found->length;
    else                   nearch_sum = found->sum;
    uint32_t key32 = key;
    if (key32 > 0x01E3F) key32++;
    cp = key32 - nearch_sum;
    if (key32 == 0x0E7C8) /* 0x0E7C7 + key++ */
      cp = 0x01D21;
    if (key32 > 0x0E7C8) cp--; /* undo previous */

  calc4:
    if ((GBSz - (size_t)4) > GBSz)  goto rcsWR;
    GBSz -= 4;
    GBStr[3] = 0x030 +  (cp % 10);
    GBStr[2] = 0x081 + ((cp / 10) % 126);
    GBStr[1] = 0x030 + ((cp / 1260) % 10);
    GBStr[0] = 0x081 +  (cp / 12600);
    GBStr += 4;

  next_code:
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
  cnvrt->cs_wrSz = GBSz;
  cnvrt->cs_wr = (void *)GBStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
/*
rcsIU:
  cs_code = csIU;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
*/
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoGB2312Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_wr;
  size_t    GBSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;

    /* reading in 32bit unicode codes */
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {
    uint16_t gb;

    if ((GBSz - (size_t)2) > GBSz)  goto rcsWR;
    if (cp == EOSmark) {
      *(uint16_t*)GBStr = (uint16_t)cp;
      GBStr += 2, GBSz -= 2;
      cpStr += 1, cpSz -= sizeof(uint32_t);
      goto rcsNUL;
    }
    if (cp == 0x02015) {
      gb = cconst16_t('\x21','\x2A');
      goto cntnu;
    }
    if (cp == 0x030FB) {
      gb = cconst16_t('\x21','\x24');
      goto cntnu;
    }
      /* these are found by tables */
    unsigned idx, base = 0x080;
    uint16_t key = (uint16_t)cp;
    struct _b2pts *found =
        nearch(&key, indexgb18030, (size_t)207, sizeof(struct _b2pts), &gFind);
    if ((key == found->uni) ||
        ((key >= found->uni) && (key < (found->uni + found->length)))) {
      if (key >= 0x0D800) {
          /* void area */
        if (key < 0x0E000)  goto rcsSI;
        base = 0x0880;
      }
        /* GBK areas */
      idx = found->sum - found->length - base;
      idx += key - found->uni;
      gb = _unicode_to_GB18030_table[idx];
#ifdef __LITTLE_ENDIAN__
      gb = (uint16_t)((uint8_t)(gb >> 8)) | ((uint16_t)((uint8_t)gb) << 8);
#endif
      gb -= 0x8080;
    } else {
      printf("encoder error %d\n", __LINE__);
      gb = 0;
    }

  cntnu:
    if ((GBSz - (size_t)2) > GBSz)  goto rcsWR;
    *(uint16_t*)GBStr = gb;
    GBStr += 2, GBSz -= 2;
    cpStr += 1, cpSz -= sizeof(uint32_t);
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = GBSz;
  cnvrt->cs_wr = (void *)GBStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
/*
rcsNC:
  cs_code = csNC;
  goto r_state;
*/
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoEUCCNStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_wr;
  size_t    GBSz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;

    /* reading in 32bit unicode codes */
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {
    uint16_t gb;

      /* has ASCII */
    if ((GBSz - (size_t)1) > GBSz)  goto rcsWR;
    if (cp < 0x080) {
      *GBStr = (uint8_t)cp;
      GBStr += 1, GBSz -= 1;
      cpStr += 1, cpSz -= sizeof(uint32_t);
      if (cp == EOSmark)  goto rcsNUL;
      continue;
    }
    if (cp == 0x02015) {
      gb = cconst16_t('\xA1','\xAA');
      goto cntnu;
    }
    if (cp == 0x030FB) {
      gb = cconst16_t('\xA1','\xA4');
      goto cntnu;
    }
    unsigned idx, base = 0x080;
    uint16_t key = (uint16_t)cp;
    struct _b2pts *found =
        nearch(&key, indexgb18030, (size_t)207, sizeof(struct _b2pts), &gFind);
    if ((key == found->uni) ||
        ((key >= found->uni) && (key < (found->uni + found->length)))) {
      if (key >= 0x0D800) {
          /* void area */
        if (key < 0x0E000)  goto rcsSI;
        base = 0x0880;
      }
        /* GBK areas */
      idx = found->sum - found->length - base;
      idx += key - found->uni;
      gb = _unicode_to_GB18030_table[idx];
#ifdef __LITTLE_ENDIAN__
      gb = (uint16_t)((uint8_t)(gb >> 8)) | ((uint16_t)((uint8_t)gb) << 8);
#endif
    } else {
      printf("encoder error %d\n", __LINE__);
      gb = 0;
    }

  cntnu:
    if ((GBSz - (size_t)2) > GBSz)  goto rcsWR;
    *(uint16_t*)GBStr = gb;
    GBStr += 2, GBSz -= 2;
    cpStr += 1, cpSz -= sizeof(uint32_t);
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
    cp = *cpStr;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = cpSz;
  cnvrt->cs_rd = (void *)cpStr;
  cnvrt->cs_wrSz = GBSz;
  cnvrt->cs_wr = (void *)GBStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsSI:
  cs_code = csSI;
  goto r_state;
/*
rcsNC:
  cs_code = csNC;
  goto r_state;
*/
rcsIE:
  cs_code = csIE;
  return cs_code;
}

#pragma mark #### GB18030 Decoders

static int
gTFind(const void *a, const void *b) {

  uint16_t a0 = *(uint16_t*)a;
  uint16_t b0 = *(uint16_t*)b;

  return (int)(a0 - b0);
}

static int
uFind(const void *a, const void *b) {

  uint16_t key   = *(uint16_t*)a;
  uint16_t query = ((struct _b2pts *)b)->uni;
  uint16_t addin = ((struct _b2pts *)b)->length;
  uint16_t sum   = ((struct _b2pts *)b)->sum;

  key += sum - addin; /* cp without indexing */
  /* query if value without indexing */
  /* key + addin is top of range covered */
  if (key == query) return 0;
  if ((key >= query) && ((key + addin) <= query))
    return 0;
  if (key < query) return -1;
  return 1;
}

/* Decoder: GB18030 stream to unicode codepoint stream */
csEnum
GB18030StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;
  uint8_t  cs0, cs1, cs2, cs3;
  size_t   cscnt;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_rd;
  size_t    GBSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (GBStr == NULL))  goto rcsIE;

  cs3 = (cs2 = (cs1 = 0));
  cscnt = 0;
  do {
    uint32_t cp;

      /* check if GBSz is equal 0 */
    if ((GBSz - (cscnt + 1)) > GBSz)  goto rcsRD;
      /* read data */
    cs0 = GBStr[cscnt];

    if (cs0 == EOSmark) {
      if (cscnt != 0)  goto rcsRN;
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      cpSz -= sizeof(uint32_t);
      *cpStr = (uint32_t)cs0;
      cpStr++;
      GBSz  -= 1;
      GBStr += 1;
      goto rcsNUL;
    }
      /* not in codeset */
    if (cs0 == 0xFF)  goto rcsIC;
      /* update variables */
    cscnt++;

    if (cs1 != 0) {
      /* check for double byte code */
      /* cs0 can only be byte0 or byte2 */
      if (cs0 < 0x30)  goto rcsIC;
      if (cs0 > 0x39) {
        /* must be a 2 byte code */
        if (cs0 == 0x7F)  goto rcsIC;
        uint16_t i = (((uint16_t)cs1) << 8) | (uint16_t)cs0;
        uint16_t *found16 =
                    bsearch(&i, _GB18030_to_unicode_table, (size_t)23940,
                                                sizeof(uint32_t), &gTFind);
        if (found16 == NULL) {
          printf("decoder error %d\n", __LINE__);
        }
        cp = found16[1];
        if (cp == 0x0FFFE) {
          if      (i == 0x0FE51) cp = 0x20087;
          else if (i == 0x0FE52) cp = 0x20089;
          else if (i == 0x0FE53) cp = 0x200CC;
          else if (i == 0x0FE6C) cp = 0x215D7;
          else if (i == 0x0FE76) cp = 0x2298F;
          else if (i == 0x0FE91) cp = 0x241FE;
        }
        goto cntnu;
      }
      cs3 = cs1, cs2 = cs0;
      if ((GBSz - (cscnt + 2)) > GBSz)  goto rcsRD;
      if ((cs1 = GBStr[(cscnt++)]) == 0xFF)  goto rcsIC;
      if ((cs0 = GBStr[(cscnt++)]) == 0xFF)  goto rcsIC;
      if ((cs1 < 0x81) || ((uint8_t)(cs0 - 0x30) > 9))  goto rcsIC;
      cp = ((cs3 - 0x081) * 12600) +
           ((cs2 - 0x030) * 1260) +
           ((cs1 - 0x081) * 10) +
            (cs0 - 0x030);
      if (cs3 >= 0x090) {
        if (cp > 0x12e247)  goto rcsIU;
        cp -= 0x1e248;
        goto cntnu;
      }
      if (cp == 0x01D21) {  cp = 0x0E7C7; goto cntnu;  }
      if ((cp > 0x1E3F) && (cp < 0x82BD)) cp--;
      struct _b2pts *found =
          nearch(&cp, indexgb18030, (size_t)207, sizeof(struct _b2pts), &uFind);
      unsigned nearch_sum;
      unsigned idx = found->uni - (found->sum - found->length);
      if (idx > cp)  nearch_sum = found->sum - found->length;
      else           nearch_sum = found->sum;
      if (idx == 0x82BD)
        cp -= 0x5B;
      if (((cp += nearch_sum) >= 0x10000) && (cs3 < 0x090))
        goto rcsIC;
      goto cntnu;
    }
    if (cs0 > 0x7F) {
        /* push onto stack */
      cs1 = cs0;
      continue;
    }
      /* ASCII */
    cp = (uint32_t)cs0;
  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr++ = cp;
    GBSz  -= cscnt;
    GBStr += cscnt;
    if (cpSz == 0)  goto rcsWR;
      /* reset for new reading */
    cs3 = (cs2 = (cs1 = 0));
    cscnt = 0;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = GBSz;
  cnvrt->cs_rd = (void *)GBStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cscnt != 0) goto rcsIC;
  goto r_state;
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIU:
  cs_code = csIU;
  goto r_state;
/*
rcsNC:
  cs_code = csNC;
  goto r_state;
*/
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
GB2312StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_rd;
  size_t    GBSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (GBStr == NULL))  goto rcsIE;

  do {
    uint32_t cp;
    union {
      uint16_t c16;
      uint8_t  cs[2];
    } gb;

      /* check if GBSz is equal 0, 2 byte, no ASCII */
    if ((GBSz - 2) > GBSz)  goto rcsRD;
    gb.cs[0] = GBStr[0];
    gb.cs[1] = GBStr[1];
    if (gb.cs[0] == EOSmark) {
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      cpSz -= sizeof(uint32_t);
      *cpStr = gb.cs[0];
      cpStr += 1;
      GBStr += 2, GBSz  -= 2;
      goto rcsNUL;
    }
    if ((gb.cs[0] < 0x21) && (gb.cs[0] > 0x77))  goto rcsIC;
    if ((gb.cs[1] < 0x21) && (gb.cs[1] > 0x7E))  goto rcsIC;
    if (gb.cs[0] == 0x21) {
      if (gb.cs[1] == 0x24) {  cp = 0x030FB;  goto cntnu;  }
      if (gb.cs[1] == 0x2A) {  cp = 0x02015;  goto cntnu;  }
    }
    gb.c16 += 0x8080;
    csCnvrt_st gb_st = {
      .cs_wr   = &cp,
      .cs_wrSz = sizeof(uint32_t),
      .cs_rd   = gb.cs,
      .cs_rdSz = 2
    };
    cs_code = GB18030StrtoUStr(&gb_st);
    if (cs_code >= csIC)  return cs_code;

  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr += 1;
    GBStr += 2, GBSz  -= 2;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = GBSz;
  cnvrt->cs_rd = (void *)GBStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cpSz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
EUCCNStrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *GBStr = (uint8_t *)cnvrt->cs_rd;
  size_t    GBSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (GBStr == NULL))  goto rcsIE;

  do {
    uint32_t cp;
    union {
      uint16_t c16;
      uint8_t  cs[2];
    } gb;

      /* check if GBSz, readable, has ASCII */
    if ((GBSz - 1) > GBSz)  goto rcsRD;
    if ((gb.cs[0] = GBStr[0]) < 0xA1) {
      if (gb.cs[0] > 0x7F)  goto rcsIC;
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      cpSz -= sizeof(uint32_t);
      *cpStr = gb.cs[0];
      cpStr += 1;
      GBStr += 2, GBSz  -= 2;
      if (gb.cs[0] == EOSmark)  goto rcsNUL;
      if (cpSz == 0)  goto rcsWR;
      continue;
    }
    if ((GBSz - 2) > GBSz)  goto rcsRD;
    if ((gb.cs[1] = GBStr[1]) < 0xA1)  goto rcsIC;
    if (gb.cs[0] > 0xF7)  goto rcsIC;
    if (gb.cs[1] == 0xFF)  goto rcsIC;
    if (gb.cs[0] == 0xA1) {
      if (gb.cs[1] == 0xA4) {  cp = 0x030FB;  goto cntnu;  }
      if (gb.cs[1] == 0xAA) {  cp = 0x02015;  goto cntnu;  }
    }
    gb.c16 = (uint16_t)((uint8_t)gb.c16) << 8 | (uint16_t)((uint8_t)(gb.c16 >> 8));
    uint16_t *found16 =
                  bsearch(&gb.c16, _GB18030_to_unicode_table, (size_t)23940,
                                                    sizeof(uint32_t), &gTFind);
    if (found16 == NULL) {
      printf("decoder error %d\n", __LINE__);
    }
    cp = found16[1];

  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr += 1;
    GBStr += 2, GBSz  -= 2;
    if (cpSz == 0)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = GBSz;
  cnvrt->cs_rd = (void *)GBStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (GBSz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cpSz == 0)  cs_code = csRW;
  goto r_state;
rcsIC:
  cs_code = csIC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

