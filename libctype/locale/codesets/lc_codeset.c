/*
 *  lc_codeset.c - sets a portion of a locale's ctype
 *  PheonixOS
 *
 *  Created by Steven Abner on Mon 11 Nov 2013.
 *  Copyright (c) 2013, 2022. All rights reserved.
 *
 */
/*
 *  For a given region, one customer might be using Shift_JIS, another
 * EUC-JP. This still does not change the fact that both sort the same,
 * both display time the same, etc. This sets the interpretation for input
 * and output, yet not break the way they format, sort. Another words, you
 * don't have to conform to just what one person says is correct for you.
 *
 */

#include "as_locale.h"

/* function dependencies */
extern int strcmp(const char *, const char *);

/* private prototype for now */
int         setcs(locale_t ld, const char *csPtr);
const char *getcs(locale_t ld);

/* this needs to verify correct user permissions */
/* this will only change the User space locale */

/* String ID, with position match to locale's actual codeset data */
/* XXX: allow bsearch() */
static
struct _csstrpair {
  int        csidx;
  const char *cs_name;
} csList[36] = {
  { 27, "Big5"         },
  { 30, "Big5-HKSCS"   },
  { 12, "EUC-JP"       },
  { 19, "GB18030"      },
  { 26, "GB2312"       },
  { 13, "GB_2312-80"   },
  {  1, "ISO-8859-1"   },
  {  2, "ISO-8859-2"   },
  {  3, "ISO-8859-3"   },
  {  4, "ISO-8859-4"   },
  {  5, "ISO-8859-5"   },
  {  6, "ISO-8859-6"   },
  {  7, "ISO-8859-7"   },
  {  8, "ISO-8859-8"   },
  {  9, "ISO-8859-9"   },
  { 10, "ISO-8859-10"  },
  { 34, "ISO-8859-11"  },
  { 15, "ISO-8859-13"  },
  { 16, "ISO-8859-14"  },
  { 17, "ISO-8859-15"  },
  { 18, "ISO-8859-16"  },
  { 28, "KOI8-R"       },
  { 29, "KOI8-U"       },
  { 11, "Shift_JIS"    },
  { 34, "TIS-620"      },
  {  0, "US-ASCII"     },
  { 14, "UTF-8"        },
  { 22, "UTF-16"       },
  { 20, "UTF-16BE"     },
  { 21, "UTF-16LE"     },
  { 23, "UTF-32"       },
  { 24, "UTF-32BE"     },
  { 25, "UTF-32LE"     },
  { 31, "windows-1251" },
  { 32, "windows-1252" },
  { 33, "windows-1256" }
};

/* XXX: this means all interpeters are loaded, want only ascii, utf8 and
 * user's set loaded */
static const lc_codeset_st csSet[] = {
  { csASCII,              UStrtoASCIIStr,    ASCIIStrtoUStr    }, //  3     0
  { csISOLatin1,          UStrtoISO1Str,     ISO1StrtoUStr     }, //  4     1
  { csISOLatin2,          UStrtoISO2Str,     ISO2StrtoUStr     }, //  5     2
  { csISOLatin3,          UStrtoISO3Str,     ISO3StrtoUStr     }, //  6     3
  { csISOLatin4,          UStrtoISO4Str,     ISO4StrtoUStr     }, //  7     4
  { csISOLatinCyrillic,   UStrtoISO5Str,     ISO5StrtoUStr     }, //  8     5
  { csISOLatinArabic,     UStrtoISO6Str,     ISO6StrtoUStr     }, //  9     6
  { csISOLatinGreek,      UStrtoISO7Str,     ISO7StrtoUStr     }, // 10     7
  { csISOLatinHebrew,     UStrtoISO8Str,     ISO8StrtoUStr     }, // 11     8
  { csISOLatin5,          UStrtoISO9Str,     ISO9StrtoUStr     }, // 12     9
  { csISOLatin6,          UStrtoISO10Str,    ISO10StrtoUStr    }, // 13    10
  { csShiftJIS,           UStrtoSJISStr,     SJISStrtoUStr     }, // 17    11
  { csEUCPkdFmtJapanese,  UStrtoEUCJPStr,    EUCJPStrtoUStr    }, // 18    12
  { csISO58GB231280,      UStrtoGB2312Str,   GB2312StrtoUStr   }, // 57    13
  { csUTF8,               UStrtoUTF8Str,     UTF8StrtoUStr     }, // 106   14
  { csISO885913,          UStrtoISO13Str,    ISO13StrtoUStr    }, // 109   15
  { csISO885914,          UStrtoISO14Str,    ISO14StrtoUStr    }, // 110   16
  { csISO885915,          UStrtoISO15Str,    ISO15StrtoUStr    }, // 111   17
  { csISO885916,          UStrtoISO16Str,    ISO16StrtoUStr    }, // 112   18
  { csGB18030,            UStrtoGB18030Str,  GB18030StrtoUStr  }, // 114   19
  { csUTF16BE,            UStrtoUTF16BEStr,  UTF16BEStrtoUStr  }, // 1013  20
  { csUTF16LE,            UStrtoUTF16LEStr,  UTF16LEStrtoUStr  }, // 1014  21
  { csUTF16,              UStrtoUTF16Str,    UTF16StrtoUStr    }, // 1015  22
  { csUTF32,              UStrtoUTF32Str,    UTF32StrtoUStr    }, // 1017  23
  { csUTF32BE,            UStrtoUTF32BEStr,  UTF32BEStrtoUStr  }, // 1018  24
  { csUTF32LE,            UStrtoUTF32LEStr,  UTF32LEStrtoUStr  }, // 1019  25
  { csGB2312,             UStrtoEUCCNStr,    EUCCNStrtoUStr    }, // 2025  26
  { csBig5,               UStrtoBig5Str,     Big5StrtoUStr     }, // 2026  27
  { csKOI8R,              UStrtoKOI8RStr,    KOI8RStrtoUStr    }, // 2084  28
  { csKOI8U,              UStrtoKOI8UStr,    KOI8UStrtoUStr    }, // 2088  29
  { csBig5HKSCS,          UStrtoHKSCSStr,    HKSCSStrtoUStr    }, // 2101  30
  { cswindows1251,        UStrtocs1251Str,   cs1251StrtoUStr   }, // 2251  31
  { cswindows1252,        UStrtocs1252Str,   cs1252StrtoUStr   }, // 2252  32
  { cswindows1256,        UStrtocs1256Str,   cs1256StrtoUStr   }, // 2256  33
  { csTIS620,             UStrtoISO11Str,    ISO11StrtoUStr    }  // 2259  34
};

/* sets locale's codeset based on string ID */
int
setcs(locale_t ld, const char *csPtr) {

  int i = 0;
  do {
    if (!strcmp(csPtr, csList[i].cs_name))  break;
    if ((++i) == 36)  return 1;
  } while (1);

  i = csList[i].csidx;
  ld->lc_codeset.MIB       = csSet[i].MIB;
  ld->lc_codeset.encoderFn = csSet[i].encoderFn;
  ld->lc_codeset.decoderFn = csSet[i].decoderFn;

  return 0;
}

/* Translate MIB to string ID => position of csSet */
const char *
getcs(locale_t ld) {

  switch (ld->lc_codeset.MIB) {
    case csASCII:             return  csList[0].cs_name; //  3
    case csISOLatin1:         return  csList[1].cs_name; //  4
    case csISOLatin2:         return  csList[2].cs_name; //  5
    case csISOLatin3:         return  csList[3].cs_name; //  6
    case csISOLatin4:         return  csList[4].cs_name; //  7
    case csISOLatinCyrillic:  return  csList[5].cs_name; //  8
    case csISOLatinArabic:    return  csList[6].cs_name; //  9
    case csISOLatinGreek:     return  csList[7].cs_name; // 10
    case csISOLatinHebrew:    return  csList[8].cs_name; // 11
    case csISOLatin5:         return  csList[9].cs_name; // 12
    case csISOLatin6:         return csList[10].cs_name; // 13
    case csShiftJIS:          return csList[17].cs_name; // 17
    case csEUCPkdFmtJapanese: return csList[18].cs_name; // 18
    case csISO58GB231280:     return csList[19].cs_name; // 57
    case csUTF8:              return csList[21].cs_name; // 106
    case csISO885913:         return csList[13].cs_name; // 109
    case csISO885914:         return csList[14].cs_name; // 110
    case csISO885915:         return csList[15].cs_name; // 111
    case csISO885916:         return csList[16].cs_name; // 112
    case csGB18030:           return csList[20].cs_name; // 114
    case csUTF16BE:           return csList[23].cs_name; // 1013
    case csUTF16LE:           return csList[24].cs_name; // 1014
    case csUTF16:             return csList[22].cs_name; // 1015
    case csUTF32:             return csList[25].cs_name; // 1017
    case csUTF32BE:           return csList[26].cs_name; // 1018
    case csUTF32LE:           return csList[27].cs_name; // 1019
    case csGB2312:            return csList[28].cs_name; // 2025
    case csBig5:              return csList[29].cs_name; // 2026
    case csKOI8R:             return csList[30].cs_name; // 2084
    case csKOI8U:             return csList[31].cs_name; // 2088
    case csBig5HKSCS:         return csList[32].cs_name; // 2101
    case cswindows1251:       return csList[33].cs_name; // 2251
    case cswindows1252:       return csList[34].cs_name; // 2252
    case cswindows1256:       return csList[35].cs_name; // 2256
    case csTIS620:            return csList[12].cs_name; // 2259
    default:                  return NULL;
  }
}

