/*
 *  csJISX0213.c - Japanese standard
 *  Pheonix
 *
 *  Created by Steven Abner on Sat 18 May 2019.
 *  Copyright (c) 2019, 2022. All rights reserved.
 *
 *  Original Thu 7 Nov 2013
 */

#include "as_ctype.h"
/* table dependancies */
#include "csJISX0213.h"
/* function dependancies */
extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*compar)(const void *, const void *));

#pragma mark #### JISX0213 Encoders

static int
uFind(const void *a, const void *b) {

  uint16_t a0 = *(uint16_t *)a;
  uint16_t b0 = ((uint16_t *)b)[1];

  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  return 0;
}

static int
cuFind(const void *a, const void *b) {

  uint16_t a0 = *(uint16_t *)a;
  uint16_t b0 = ((uint16_t *)b)[1];

  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  if (((uint16_t *)b)[2] != 0)  return -1;
  return 0;
}

/* Encoder: unicode stream to JISX0213 stream */
csEnum
UStrtoJISX0213Str(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *JIStr = (uint8_t *)cnvrt->cs_wr;
  size_t    JISz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {

    if (cp == EOSmark)  goto cntnu;

    uint16_t gcclvalue = cp;
    struct _jis *found;
    if (cp == gcclvalue) {
      found = bsearch(&gcclvalue, _unicode_to_jisx0213_table,
                                    (size_t)10884, sizeof(struct _jis), &uFind);
      if (found != NULL) {
        cp = found->jis;
        goto cntnu;
      }
    }
      /* cases: 0xFFFE, 0xFFFF, not valid */
    found = bsearch(&gcclvalue, _sip_to_jisx0213_table,
                                      (size_t)303, sizeof(struct _jis), &uFind);
    if (found != NULL) {
      cp = found->jis;
      goto cntnu;
    }

    struct _cjis *cfound;
    cfound = bsearch(&gcclvalue, _doubleword_to_jisx0213_table,
                                    (size_t)46, sizeof(struct _cjis), &cuFind);
    if (cfound == NULL)  goto rcsNC;
    cp = cfound->jis;
    if (cfound->uni >= 0x304B) {
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[1] == 0x309A)  cp = (cfound + 1)->jis, cpStr += 1, cpSz -= 4;
    } else if (cfound->uni == 0x00E6) {
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[1] == 0x0300)  cp = (cfound + 1)->jis, cpStr += 1, cpSz -= 4;
    } else if (cfound->uni == 0x02E5) {
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[1] == 0x02E9)  cp = (cfound + 1)->jis, cpStr += 1, cpSz -= 4;
    } else if (cfound->uni == 0x02E9) {
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[1] == 0x02E5)  cp = (cfound + 1)->jis, cpStr += 1, cpSz -= 4;
    } else {
      if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsRN;
      if (cpStr[1] == 0x0300)  cp = (cfound + 1)->jis, cpStr += 1, cpSz -= 4;
      if (cpStr[1] == 0x0301)  cp = (cfound + 2)->jis, cpStr += 1, cpSz -= 4;
    }

    /* Update stream info */
  cntnu:
#ifdef __LITTLE_ENDIAN__
    cp = (uint16_t)((uint8_t)(cp >> 8)) | ((uint16_t)((uint8_t)cp) << 8);
#endif
    if ((JISz - (size_t)2) > JISz)  goto rcsWR;
    JISz  -= 2;
    *(uint16_t*)JIStr = (uint16_t)cp;
    JIStr += 2;
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
  cnvrt->cs_wrSz = JISz;
  cnvrt->cs_wr = (void *)JIStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (JISz == 0)  cs_code = csRW;
  goto r_state;
/* needed to read another code to determine */
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
UStrtoSJISStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *SJIStr = (uint8_t *)cnvrt->cs_wr;
  size_t    SJISz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {

      /* single bytes of SJIS */
    if (cp < 0x080) {
      if (cp == 0x005C)  {  cp = 0x0815F; goto cntnu;  }
      if (cp == 0x007E)  {  cp = 0x081B0; goto cntnu;  }
      if ((SJISz - (size_t)1) > SJISz)  goto rcsWR;
      SJISz  -= 1;
      *SJIStr = (uint8_t)cp;
      SJIStr += 1;
      goto cntnu;
    }
      /* rest double byte, retrieve then convert to SJIS */
    if ((SJISz - (size_t)2) > SJISz)  goto rcsWR;
      /* convert to JIS */
    uint8_t jis[2];
    csCnvrt_st jis_st = {
      .cs_wr = jis,
      .cs_wrSz = 2,
      .cs_rd = cpStr,
      .cs_rdSz = cpSz
    };
    cs_code = UStrtoJISX0213Str(&jis_st);
    if (cs_code != csNC) {
      if (jis_st.cs_rdSz == 0) {
        cpSz = sizeof(uint32_t);
        cpStr++;
      }
    }
    if (cs_code == csNC) {
      if (cp == 0x0FFE3) {
        *(uint16_t*)SJIStr = cconst16_t('\x81','\x50');
        SJIStr += 2;
        SJISz  -= 2;
        goto cntnu;
      }
      if (cp == 0x0FFE5) {
        *(uint16_t*)SJIStr = cconst16_t('\x81','\x8F');
        SJIStr += 2;
        SJISz  -= 2;
        goto cntnu;
      }
      if ((cp > 0x0FF60) && (cp < 0x0FFA0)) {
        cp -= 0x0FEC0;
        *SJIStr = (uint8_t)cp;
        SJIStr += 1;
        SJISz  -= 1;
        goto cntnu;
      }
      if ((cp > 0x0FF00) && (cp < 0x0FF61)) {
        cp -= 0x0FEE0;
        jis_st.cs_wr = jis;
        jis_st.cs_wrSz = 2;
        jis_st.cs_rd = &cp;
        jis_st.cs_rdSz = sizeof(uint32_t);
        cs_code = UStrtoJISX0213Str(&jis_st);
      }
    }
      /* jis to sjis */
    if (jis[0] < 0x0A0) {
        /* plane 1 */
      jis[1] = ((jis[0] & 1) == 0) ?
                     (jis[1] + 0x7E) : ((jis[1] + 0x1F) + jis[1]/96);
      jis[0] = ((jis[0] + 1) >> 1) + ((jis[0] < 0x5F) ? 0x70 : 0xB0);
      cp = ((uint16_t)jis[0] << 8) | (uint16_t)jis[1];
      if (cp == 0x08150) {
        *SJIStr = (uint8_t)0x07E;
        SJIStr += 1;
        SJISz  -= 1;
        goto cntnu;
      }
      if (cp == 0x0818F) {
        *SJIStr = (uint8_t)0x05C;
        SJIStr += 1;
        SJISz  -= 1;
        goto cntnu;
      }
    } else {
      if ((jis[0] & 1) == 1) jis[1] -= ((jis[1] < 0xE0) ? 0x61 : 0x60);
      else jis[1] -= 2;
      switch (jis[0]) {
        case 0xFE:
          cp = (((uint16_t)jis[0] - 0x02) << 8) | (uint16_t)jis[1];
          break;
        case 0xFD:
          cp = (((uint16_t)jis[0] - 0x01) << 8) | (uint16_t)jis[1];
          break;
        case 0xFC:
          cp = (((uint16_t)jis[0] - 0x01) << 8) | (uint16_t)jis[1];
          break;
        case 0xFB:
          cp = (((uint16_t)jis[0] + 0x00) << 8) | (uint16_t)jis[1];
          break;
        case 0xFA:
          cp = (((uint16_t)jis[0] + 0x00) << 8) | (uint16_t)jis[1];
          break;
        case 0xF9:
          cp = (((uint16_t)jis[0] + 0x01) << 8) | (uint16_t)jis[1];
          break;
        case 0xF8:
          cp = (((uint16_t)jis[0] + 0x01) << 8) | (uint16_t)jis[1];
          break;
        case 0xF7:
          cp = (((uint16_t)jis[0] + 0x02) << 8) | (uint16_t)jis[1];
          break;
        case 0xF6:
          cp = (((uint16_t)jis[0] + 0x02) << 8) | (uint16_t)jis[1];
          break;
        case 0xF5:
          cp = (((uint16_t)jis[0] + 0x03) << 8) | (uint16_t)jis[1];
          break;
        case 0xF4:
          cp = (((uint16_t)jis[0] + 0x03) << 8) | (uint16_t)jis[1];
          break;
        case 0xF3:
          cp = (((uint16_t)jis[0] + 0x04) << 8) | (uint16_t)jis[1];
          break;
        case 0xF2:
          cp = (((uint16_t)jis[0] + 0x04) << 8) | (uint16_t)jis[1];
          break;
        case 0xF1:
          cp = (((uint16_t)jis[0] + 0x05) << 8) | (uint16_t)jis[1];
          break;
        case 0xF0:
          cp = (((uint16_t)jis[0] + 0x05) << 8) | (uint16_t)jis[1];
          break;
        case 0xEF:
          cp = (((uint16_t)jis[0] + 0x06) << 8) | (uint16_t)jis[1];
          break;
        case 0xEE:
          cp = (((uint16_t)jis[0] + 0x06) << 8) | (uint16_t)jis[1];
          break;
/* 1,3,4,5,8,12,13,14,15 */
        case 0xAF: /* 3E */
          cp = (((uint16_t)jis[0] + 0x45) << 8) | (uint16_t)jis[1];
          break;
        case 0xAE:
          cp = (((uint16_t)jis[0] + 0x45) << 8) | (uint16_t)jis[1];
          break;
        case 0xAD:
          cp = (((uint16_t)jis[0] + 0x46) << 8) | (uint16_t)jis[1];
          break;
        case 0xAC:
          cp = (((uint16_t)jis[0] + 0x46) << 8) | (uint16_t)jis[1];
          break;
        case 0xA8:
          cp = (((uint16_t)jis[0] + 0x48) << 8) | (uint16_t)jis[1];
          break;
        case 0xA5:
          cp = (((uint16_t)jis[0] + 0x4D) << 8) | (uint16_t)jis[1];
          break;
        case 0xA4:
          cp = (((uint16_t)jis[0] + 0x4D) << 8) | (uint16_t)jis[1];
          break;
        case 0xA3:
          cp = (((uint16_t)jis[0] + 0x4E) << 8) | (uint16_t)jis[1];
          break;
        case 0xA1:
          cp = (((uint16_t)jis[0] + 0x4F) << 8) | (uint16_t)jis[1];
          break;
        default:
          goto rcsIE;
      }
    }

#ifdef __LITTLE_ENDIAN__
    cp = (uint16_t)((uint8_t)(cp >> 8)) | ((uint16_t)((uint8_t)cp) << 8);
#endif
    *(uint16_t*)SJIStr = (uint16_t)cp;
    SJIStr += 2;
    SJISz  -= 2;
    /* Update stream info */
  cntnu:
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
  cnvrt->cs_wrSz = SJISz;
  cnvrt->cs_wr = (void *)SJIStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (SJISz == 0)  cs_code = csRW;
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
UStrtoEUCJPStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint8_t  *JIStr = (uint8_t *)cnvrt->cs_wr;
  size_t    JISz  = cnvrt->cs_wrSz;
  uint32_t *cpStr = (uint32_t *)cnvrt->cs_rd;
  size_t    cpSz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (cpStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsRD;
  uint32_t cp = *cpStr;

  do {

    /* single bytes of EUC */
    if (cp < 0x0A0) {
      if ((JISz - (size_t)1) > JISz)  goto rcsWR;
      *JIStr = (uint8_t)cp;
      JIStr += 1;
      JISz  -= 1;
      goto cntnu;
    }
    if ((JISz - (size_t)2) > JISz)  goto rcsWN;
    /* now return is mininum of 2 byte codes */
    if ((cp >= 0x0FF61) && (cp <= 0x0FF9F)) {
      cp = 0x08EA1 + (cp - 0x0FF61);
#ifdef __LITTLE_ENDIAN__
      cp = (uint16_t)((uint8_t)(cp >> 8)) | ((uint16_t)((uint8_t)cp) << 8);
#endif
      *(uint16_t*)JIStr = (uint16_t)cp;
      JIStr += 2;
      JISz  -= 2;
      goto cntnu;
    }
    uint8_t jis[2];
    csCnvrt_st jis_st = {
      .cs_wr = jis,
      .cs_wrSz = 2,
      .cs_rd = cpStr,
      .cs_rdSz = cpSz
    };
    if ((cp >= 0x0FF01) && (cp <= 0x0FF5E)) {
      cp -= 0x0FEE0;
      jis_st.cs_rd = &cp;
      jis_st.cs_rdSz = sizeof(uint32_t);
      cs_code = UStrtoJISX0213Str(&jis_st);
      cpStr += 1;
      cpSz  -= sizeof(uint32_t);
    } else {
      cs_code = UStrtoJISX0213Str(&jis_st);
      cpStr = jis_st.cs_rd;
      cpSz  = jis_st.cs_rdSz;
    }
    if (jis[0] >= 0xA1) {
      if ((JISz - (size_t)3) > JISz)  goto rcsWN;
      *JIStr = 0x8F;
      JIStr += 1;
      JISz  -= 1;
      *(uint16_t*)JIStr = *(uint16_t*)jis;
    } else {
      *(uint16_t*)JIStr = (*(uint16_t*)jis + 0x08080);
    }
    JIStr += 2;
    JISz  -= 2;
    if (cp == EOSmark)  goto rcsNUL;
    if (cpSz == 0)  goto rcsRD;
    cp = *cpStr;
    continue;

    /* Update stream info */
  cntnu:
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
  cnvrt->cs_wrSz = JISz;
  cnvrt->cs_wr = (void *)JIStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (JISz == 0)  cs_code = csRW;
  goto r_state;
rcsWN:
  cs_code = csWN;
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

#pragma mark #### JISX0213 Decoders

static int
cFind(const void *a, const void *b) {

  uint16_t a0 = *(uint16_t *)a;
  uint16_t b0 = ((struct _cjis *)b)->jis;

  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  return 0;
}

static int
sFind(const void *a, const void *b) {

  uint16_t a0 = *(uint16_t *)a;
  uint16_t b0 = ((struct _jis *)b)->jis;

  if (a0 < b0)  return -1;
  if (a0 > b0)  return 1;
  return 0;
}

/* Decoder: JISX0213 stream to unicode codepoint stream */
csEnum
JISX0213StrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *JIStr = (uint8_t *)cnvrt->cs_rd;
  size_t    JISz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (JIStr == NULL))  goto rcsIE;

  do {
    uint16_t jis;
    uint32_t cp;
      /* check if JISz is equal 0 */
    if ((JISz - 2) > JISz)  goto rcsRD;
    if ((cp = *JIStr) == EOSmark)  goto cntnu;
      /* read data, all 16bit but sent as byte stream */
    jis = *(uint16_t*)JIStr;
#ifdef __LITTLE_ENDIAN__
    jis = (uint16_t)((uint8_t)(jis >> 8)) | (jis << 8);
#endif
      /* 0xFEFE last valid code (includes unmapped), none prior to 0x2121 */
    if ((uint16_t)(jis -= 0x2121) > (uint16_t)0x0DDDD)  goto rcsIE;
      /* plane 1 [2121-7E7E], straight indexing, verify no bad input areas */
    if (jis <= 0x05D5D) {
      if ((uint8_t)jis > 0x05D)  goto rcsIE;
      uint16_t idx = ((jis >> 8) * 94) + (uint16_t)((uint8_t)jis);
      cp = _jisx0213_to_unicode_table[idx];
      if (cp == 0x0FFFF)  goto rcsNC;
      if (cp == 0x0FFFE) {
        jis += 0x02121;
        struct _cjis *found = bsearch(&jis, _jisx0213_to_doubleword_table,
                                      (size_t)25, sizeof(struct _cjis), &cFind);
        if (found == NULL) {
          struct _jis *sfind = bsearch(&jis, _jisx0213_to_sip_table,
                                      (size_t)303, sizeof(struct _jis), &sFind);
          cp = (uint32_t)sfind->uni | 0x00020000;
          goto cntnu;
        }
        cp = found->cmb;
        if ((cpSz -= sizeof(uint32_t)) == 0)  goto rcsWN;
        *cpStr = found->uni;
        cpStr += 1;
      }
      goto cntnu;
    }
      /* plane 2, no double-uni */
    if ((cp == 0x0A2) || (cp == 0x0A6) || (cp == 0x0A7) ||
        (cp == 0x0A9) || (cp == 0x0AA) || (cp == 0x0AB) ||
        ((cp > 0x0AF) && (cp < 0x0EE)))
      goto rcsIE;
    uint32_t idx;
    if ((uint16_t)(jis -= 0x8080) > (uint16_t)0x05D5D)  goto rcsIE;
    if ((uint8_t)jis >= 0x5E)  goto rcsIE;
    idx = ((uint16_t)((uint8_t)(jis >> 8)) + 94) * 94;
    /* 0*94 + 94 */
    if (jis < 0x5E) {  /* A1 [A1-FE] */
      cp = _jisx0213_to_unicode_table[(idx + jis)];
    } else {
      idx -= 1*94;
      if (idx < 98*94) {  /* A3-A5 */
        cp = _jisx0213_to_unicode_table[(idx + (uint8_t)jis)];
      } else {
        idx -= 2*94;  /* A6-A7 */
        if (idx < 99*94) {  /* A8 */
          cp = _jisx0213_to_unicode_table[(idx + (uint8_t)jis)];
        } else {
          idx -= 3*94;  /* A9-AB */
          if (idx < 103*94) { /* AC-AF */
            cp = _jisx0213_to_unicode_table[(idx + (uint8_t)jis)];
          } else {
            idx -= 62*94;  /* B0-ED */
            cp = _jisx0213_to_unicode_table[(idx + (uint8_t)jis)];
    } } } }
    if (cp == 0x0FFFF)  goto rcsNC;
    if (cp == 0x0FFFE) {
      jis = *(uint16_t*)JIStr;
#ifdef __LITTLE_ENDIAN__
      jis = (uint16_t)((uint8_t)(jis >> 8)) | (jis << 8);
#endif
      struct _jis *sfind = bsearch(&jis, _jisx0213_to_sip_table,
                                      (size_t)303, sizeof(struct _jis), &sFind);
      cp = (uint32_t)sfind->uni | 0x00020000;
    }
    /* Update stream info */
  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz  -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr += 1;
    JIStr += 2;
    JISz  -= 2;
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = JISz;
  cnvrt->cs_rd = (void *)JIStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (JISz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cpSz == 0)  cs_code = csRW;
  goto r_state;
rcsWN:
  cs_code = csWN;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

csEnum
SJISStrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *SJIStr = (uint8_t *)cnvrt->cs_rd;
  size_t    SJISz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (SJIStr == NULL))  goto rcsIE;

  do {
    uint16_t jis;
    uint32_t cp;
      /* check if JISz is equal 0 */
    if ((SJISz - 1) > SJISz)  goto rcsRD;
    if ((jis = *SJIStr) < 0x080) {
      cp = jis;
      if (jis == 0x005C)  cp = 0x000A5;
      if (jis == 0x007E)  cp = 0x0203E;
      goto cntnu;
    }
    if ((jis > 0x0A0) && (jis < 0x0E0)) {
      cp = jis + 0x0FEC0;
      goto cntnu;
    }
    if ((jis == 0x080) || (jis == 0x0A0) || (jis > 0x0FC)) goto rcsIE;
      /* all 2 byte, convert to JIS */
    if ((SJISz - (size_t)2) > SJISz)  goto rcsRN;
    jis = *(uint16_t*)SJIStr;
#ifdef __LITTLE_ENDIAN__
    jis = (uint16_t)((uint8_t)(jis >> 8)) | (jis << 8);
#endif
    uint8_t j1 = jis >> 8;
    uint8_t j2 = (uint8_t)jis;
    if (j1 == 0xA0)  goto rcsIE;
    if ((j2 < 0x040) || (j2 == 0x07F) || (j2 > 0x0FC))  goto rcsIE;
    if (jis < 0x0F000) {
        /* if code in plane 1 */
      if (j1 >= 0x0E0)  j1 -= 0x040;
      if (j2 >= 0x080)  --j2;
      j1 = ((j1 - 0x081) << 1 ) + 0x021;
      j2 = (j2 - 0x040) + 0x21;
      if (j2 > 0x07E) {  j1++;  j2 -= 0x05E;  }
#ifdef __LITTLE_ENDIAN__
      jis = ((uint16_t)j2 << 8) | (uint16_t)j1;
#else
      jis = ((uint16_t)j1 << 8) | (uint16_t)j2;
#endif
    } else {
      if (j2 < 0x040)  goto rcsIE;
      switch (j1) {
        case 0xFC:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 + 0x02) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 + 0x01) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xFB:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 + 0x01) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xFA:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x01) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF9:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x01) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x02) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF8:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x02) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x03) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF7:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x03) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x04) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF6:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x04) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x05) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF5:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x05) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x06) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF4:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x06) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x45) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF3:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x45) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x46) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF2:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x46) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x4D) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF1:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x4D) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x4E) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        case 0xF0:
          if (j2 > 0x09E)
            jis = (((uint16_t)j1 - 0x48) << 8) | (uint16_t)(j2 + 2);
          else
            jis = (((uint16_t)j1 - 0x4F) << 8) |
                                  (uint16_t)(j2 + ((j2 <= 0x7E) ? 0x61 : 0x60));
          break;
        default:
          goto rcsIE;
      }
#ifdef __LITTLE_ENDIAN__
      jis = (uint16_t)((uint8_t)(jis >> 8)) | (jis << 8);
#endif
    }
    csCnvrt_st jis_st = {
      .cs_wr   = &cp,
      .cs_wrSz = sizeof(uint32_t),
      .cs_rd   = &jis,
      .cs_rdSz = 2
    };
    cs_code = JISX0213StrtoUStr(&jis_st);
    if (cs_code != csRW) {
      if (cs_code == csIE)  goto r_state;
      if (cs_code == csNC)  goto r_state;
      if (cs_code == csWN) {
        if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsWN;
        jis_st.cs_wr   = cpStr;
        jis_st.cs_wrSz = cpSz;
        jis_st.cs_rd   = &jis;
        jis_st.cs_rdSz = 2;
        cs_code = JISX0213StrtoUStr(&jis_st);
        cpStr  += 2;
        cpSz   -= (2 * sizeof(uint32_t));
        SJIStr += 2;
        SJISz  -= 2;
        if (cp == EOSmark)  goto rcsNUL;
        if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
        continue;
      }
    }
    if (cp == 0x0203E)  cp = 0x0FFE3;
    if (cp == 0x000A5)  cp = 0x0FFE5;
    if (cp < 0x080) {
      if ((cp != 0x05C) && (cp != 0x07E))
        cp += 0x0FEE0;
    }
    SJIStr += 1;
    SJISz  -= 1;

  cntnu:
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
    cpSz   -= sizeof(uint32_t);
    *cpStr = cp;
    cpStr  += 1;
    SJIStr += 1;
    SJISz  -= 1;
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = SJISz;
  cnvrt->cs_rd = (void *)SJIStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (SJISz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cpSz == 0)  cs_code = csRW;
  goto r_state;
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsWN:
  cs_code = csWN;
  goto r_state;
/*
rcsNC:
  cs_code = csNC;
  goto r_state;
*/
rcsIE:
  cs_code = csIE;
  goto r_state;
}

csEnum
EUCJPStrtoUStr(csCnvrt_st *cnvrt) {

  csEnum cs_code;

  uint32_t *cpStr = (uint32_t *)cnvrt->cs_wr;
  size_t    cpSz  = cnvrt->cs_wrSz;
  uint8_t  *JIStr = (uint8_t *)cnvrt->cs_rd;
  size_t    JISz  = cnvrt->cs_rdSz;

  if ((cpSz == 0) || (JIStr == NULL))  goto rcsIE;
  if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;

  do {
    uint32_t cp;
    uint16_t jis;

      /* handle single byte, or prefix 0x8E,0x8F */
    if ((JISz - 1) > JISz)  goto rcsRD;
    jis = *JIStr;
    if ((jis == 0x0A0) || (jis == 0x0FF))  goto rcsNC;
    if (jis < 0x0A0) {
      cp = jis;
      if (jis == 0x08E) {
        if ((JISz - 2) > JISz) {
          *cpStr = cp;
          cpStr += 1;
          JIStr += 1;
          JISz  -= 1;
          cpSz  -= sizeof(uint32_t);
          goto rcsRD;
        }
        jis = *(uint16_t*)JIStr;
#ifdef __LITTLE_ENDIAN__
        jis = (uint16_t)((uint8_t)(jis >> 8)) | (jis << 8);
#endif
        if (((uint8_t)jis < 0x0A1) || ((uint8_t)jis == 0x0FF)) {
          *cpStr = cp;
          cpStr += 1;
          cpSz -= sizeof(uint32_t);
          JIStr += 1;
          JISz  -= 1;
          if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
          continue;
        }
        if ((jis >= 0x08EA1) && (jis <= 0x08EDF)) {
          cp = (jis - 0x08EA1) + 0x0FF61;
          goto cntnu;
        }
        /*else if ((jis >= 0x08EE0) && (jis <= 0x08EFE)) */
        goto rcsNC;
      }
      if (jis == 0x008F) {
        if ((JISz - 3) > JISz) {
          /* unchecked 2 byte, but valid ASCII code of 0x8F */
          *cpStr = jis;
          cpStr += 1;
          cpSz -= sizeof(uint32_t);
          JIStr += 1;
          JISz  -= 1;
          if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
          continue;
        }
        if ((jis = *(uint16_t*)JIStr) == cp) {
    prefix_only:
          *cpStr = cp;
          cpStr += 1;
          JIStr += 1;
          JISz  -= 1;
          cpSz  -= sizeof(uint32_t);
          goto rcsRD;
        }
          /* skip 0x8F prefix */
        jis = *(uint16_t*)&JIStr[1];

        /* EUC-JP voids these, could add if needed */
          /* for test exclude for sure, want included, alternative is:
           * a 8F prefix, 21 byte, 21 byte, vs jis shift sequence */
        if ((((jis & 0x0FF) << 8) | ((jis & 0x0FF00) >> 8)) < (uint16_t)0x0A1A1) {
          cs_code = csIE;
          goto prefix_only;
        }
          /* plane 2, no double uni */
          /* note didn't rotate since in correct sequence for byte read */
        csCnvrt_st jis_st = {
          .cs_wr   = &cp,
          .cs_wrSz = sizeof(uint32_t),
          .cs_rd   = &jis,
          .cs_rdSz = 2
        };
        if ((cs_code = JISX0213StrtoUStr(&jis_st)) == csIE) {
            /* only prefix is valid */
          goto prefix_only;
        }
        JIStr += 1;
        JISz  -= 1;
        goto cntnu;
      }
      if (jis == 0x0A0)  goto rcsNC;
        /* rest are single byte character/control codes */
      *cpStr = jis;
      cpStr += 1;
      JIStr += 1;
      JISz  -= 1;
      cpSz  -= sizeof(uint32_t);
      if (cp == EOSmark)  goto rcsNUL;
      if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
      continue;
    }
      /* rest all two byte */
    if ((JISz - 2) > JISz)  goto rcsRN;
    jis = *(uint16_t*)JIStr - 0x08080;
    /* plane 1 */
    csCnvrt_st jis_st = {
      .cs_wr   = &cp,
      .cs_wrSz = sizeof(uint32_t),
      .cs_rd   = &jis,
      .cs_rdSz = 2
    };
    cs_code = JISX0213StrtoUStr(&jis_st);
    if (cp < 0x080)
      cp += 0x0FEE0;
    if (cs_code != csRW) {
      if (cs_code == csIE)  goto r_state;
      if (cs_code == csNC)  goto r_state;
      if (cs_code == csWN) {
        if ((cpSz - (2 * sizeof(uint32_t))) > cpSz)  goto rcsWN;
        jis_st.cs_wr   = cpStr;
        jis_st.cs_wrSz = cpSz;
        jis_st.cs_rd   = &jis;
        jis_st.cs_rdSz = 2;
        cs_code = JISX0213StrtoUStr(&jis_st);
        cpStr  += 2;
        cpSz   -= (2 * sizeof(uint32_t));
        JIStr += 2;
        JISz  -= 2;
        if (cp == EOSmark)  goto rcsNUL;
        if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
        continue;
      }
    }
    /* Update stream info */
  cntnu:
    *cpStr = cp;
    cpStr += 1;
    JIStr += 2;
    JISz  -= 2;
    cpSz  -= sizeof(uint32_t);
    if (cp == EOSmark)  goto rcsNUL;
    if ((cpSz - sizeof(uint32_t)) > cpSz)  goto rcsWR;
  } while (1);

rcsNUL:
  cs_code = csNUL;
r_state:
  cnvrt->cs_rdSz = JISz;
  cnvrt->cs_rd = (void *)JIStr;
  cnvrt->cs_wrSz = cpSz;
  cnvrt->cs_wr = (void *)cpStr;
  return cs_code;

rcsWR:
  cs_code = csWR;
  if (JISz == 0)  cs_code = csRW;
  goto r_state;
rcsRD:
  cs_code = csRD;
  if (cpSz == 0)  cs_code = csRW;
  goto r_state;
rcsRN:
  cs_code = csRN;
  goto r_state;
rcsWN:
  cs_code = csWN;
  goto r_state;
rcsNC:
  cs_code = csNC;
  goto r_state;
rcsIE:
  cs_code = csIE;
  return cs_code;
}

