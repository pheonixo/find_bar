/*
 *  ianacharset_mib.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Sat 9 Nov 2013.
 *  Copyright on http://www.ietf.org/copyrights/ianamib.html
 *  Source: http://www.iana.org/assignments/ianacharset-mib
 *  LAST-UPDATED    "201110300000Z"
 *
 *  Hand added addition, placed as 998, SLS standard
 *
 *
 * Copyright (C) The IETF Trust (YEAR OF PUBLICATION).
 * An IANA MIB module and translations of it may be copied and furnished to
 * others, and derivative works that comment on or otherwise explain it or
 * assist in its implementation may be prepared, copied, published and
 * distributed, in whole or in part, without restriction of any kind, provided
 * that the above copyright notice and this paragraph are included on all such
 * copies and derivative works. However, this MIB module itself may not be
 * modified in any way, such as by removing the copyright notice or references
 * to the IETF Trust or other Internet organizations, except as needed for the
 * purpose of developing Internet standards in which case the procedures for
 * copyrights defined in the Internet Standards process must be followed, or as
 * required to translate it into languages other than English.
 */

#ifndef _AS_MIBenum_H_
#define _AS_MIBenum_H_

typedef enum {
  binary = 0,                     /* data not encoded, byte-oriented */
  other = 1,                      /* used if the designated character set is
                                   * not currently registered by IANA */
  unknown = 2,                    /* used as a default value */
  csASCII = 3,
  csISOLatin1 = 4,                /* ISO-8859-1 */
  csISOLatin2 = 5,                /* ISO-8859-2 */
  csISOLatin3 = 6,                /* ISO-8859-3 */
  csISOLatin4 = 7,                /* ISO-8859-4 */
  csISOLatinCyrillic = 8,         /* ISO-8859-5 */
  csISOLatinArabic = 9,           /* ISO-8859-6 */
  csISOLatinGreek = 10,           /* ISO-8859-7 */
  csISOLatinHebrew = 11,          /* ISO-8859-8 */
  csISOLatin5 = 12,               /* ISO-8859-9 */
  csISOLatin6 = 13,               /* ISO-8859-10 */
  csISOTextComm = 14,
  csHalfWidthKatakana = 15,
  csJISEncoding = 16,
  csShiftJIS = 17,                /* Shift_JIS */
  csEUCPkdFmtJapanese = 18,       /* EUC-JP */
  csEUCFixWidJapanese = 19,
  csISO4UnitedKingdom = 20,
  csISO11SwedishForNames = 21,
  csISO15Italian = 22,
  csISO17Spanish = 23,
  csISO21German = 24,
  csISO60DanishNorwegian = 25,
  csISO69French = 26,
  csISO10646UTF1 = 27,
  csISO646basic1983 = 28,
  csINVARIANT = 29,
  csISO2IntlRefVersion = 30,
  csNATSSEFI = 31,
  csNATSSEFIADD = 32,
  csNATSDANO = 33,
  csNATSDANOADD = 34,
  csISO10Swedish = 35,
  csKSC56011987 = 36,
  csISO2022KR = 37,               /* ISO-2022-KR */
  csEUCKR = 38,                   /* EUC-KR */
  csISO2022JP = 39,               /* ISO-2022-JP */
  csISO2022JP2 = 40,              /* ISO-2022-JP-2 */
  csISO13JISC6220jp = 41,
  csISO14JISC6220ro = 42,
  csISO16Portuguese = 43,
  csISO18Greek7Old = 44,
  csISO19LatinGreek = 45,
  csISO25French = 46,
  csISO27LatinGreek1 = 47,
  csISO5427Cyrillic = 48,
  csISO42JISC62261978 = 49,
  csISO47BSViewdata = 50,
  csISO49INIS = 51,
  csISO50INIS8 = 52,
  csISO51INISCyrillic = 53,
  csISO54271981 = 54,
  csISO5428Greek = 55,
  csISO57GB1988 = 56,             /* GB_1988-80 */
  csISO58GB231280 = 57,           /* GB_2312-80 */
  csISO61Norwegian2 = 58,
  csISO70VideotexSupp1 = 59,
  csISO84Portuguese2 = 60,
  csISO85Spanish2 = 61,
  csISO86Hungarian = 62,
  csISO87JISX0208 = 63,           /* JIS_X0208-1983 */
  csISO88Greek7 = 64,
  csISO89ASMO449 = 65,
  csISO90 = 66,
  csISO91JISC62291984a = 67,
  csISO92JISC62991984b = 68,
  csISO93JIS62291984badd = 69,
  csISO94JIS62291984hand = 70,
  csISO95JIS62291984handadd = 71,
  csISO96JISC62291984kana = 72,
  csISO2033 = 73,
  csISO99NAPLPS = 74,
  csISO102T617bit = 75,
  csISO103T618bit = 76,
  csISO111ECMACyrillic = 77,      /* KOI8-E */
  csa71 = 78,
  csa72 = 79,
  csISO123CSAZ24341985gr = 80,
  csISO88596E = 81,
  csISO88596I = 82,
  csISO128T101G2 = 83,
  csISO88598E = 84,
  csISO88598I = 85,
  csISO139CSN369103 = 86,
  csISO141JUSIB1002 = 87,
  csISO143IECP271 = 88,
  csISO146Serbian = 89,
  csISO147Macedonian = 90,
  csISO150 = 91,
  csISO151Cuba = 92,
  csISO6937Add = 93,
  csISO153GOST1976874 = 94,
  csISO8859Supp = 95,
  csISO10367Box = 96,
  csISO158Lap = 97,
  csISO159JISX02121990 = 98,      /* JIS_X0212-1990 */
  csISO646Danish = 99,
  csUSDK = 100,
  csDKUS = 101,
  csKSC5636 = 102,
  csUnicode11UTF7 = 103,
  csISO2022CN = 104,
  csISO2022CNEXT = 105,
  csUTF8 = 106,                  /* UTF-8 */
  csISO885913 = 109,             /* ISO-8859-13 */
  csISO885914 = 110,             /* ISO-8859-14 */
  csISO885915 = 111,             /* ISO-8859-15 */
  csISO885916 = 112,             /* ISO-8859-16 */
  csGBK = 113,                   /* GBK */
  csGB18030 = 114,               /* GB18030 */
  csOSDEBCDICDF0415 = 115,
  csOSDEBCDICDF03IRV = 116,
  csOSDEBCDICDF041 = 117,
  csISO115481 = 118,
  csKZ1048 = 119,
  csISO646TA = 998,             /* SLS-1326 */ /*SLS 1326:2008*/
  csUnicode = 1000,
  csUCS4 = 1001,
  csUnicodeASCII = 1002,
  csUnicodeLatin1 = 1003,
  csUnicodeJapanese = 1004,
  csUnicodeIBM1261 = 1005,
  csUnicodeIBM1268 = 1006,
  csUnicodeIBM1276 = 1007,
  csUnicodeIBM1264 = 1008,
  csUnicodeIBM1265 = 1009,
  csUnicode11 = 1010,
  csSCSU = 1011,
  csUTF7 = 1012,                  /* UTF-7 */
  csUTF16BE = 1013,               /* UTF-16BE */
  csUTF16LE = 1014,               /* UTF-16LE */
  csUTF16 = 1015,                 /* UTF-16 */
  csCESU8 = 1016,                 /* CESU-8 */
  csUTF32 = 1017,                 /* UTF-32 */
  csUTF32BE = 1018,               /* UTF-32BE */
  csUTF32LE = 1019,               /* UTF-32LE */
  csBOCU1 = 1020,
  csWindows30Latin1 = 2000,
  csWindows31Latin1 = 2001,
  csWindows31Latin2 = 2002,
  csWindows31Latin5 = 2003,
  csHPRoman8 = 2004,
  csAdobeStandardEncoding = 2005,
  csVenturaUS = 2006,
  csVenturaInternational = 2007,
  csDECMCS = 2008,
  csPC850Multilingual = 2009,
  csPCp852 = 2010,
  csPC8CodePage437 = 2011,
  csPC8DanishNorwegian = 2012,
  csPC862LatinHebrew = 2013,
  csPC8Turkish = 2014,
  csIBMSymbols = 2015,
  csIBMThai = 2016,
  csHPLegal = 2017,
  csHPPiFont = 2018,
  csHPMath8 = 2019,
  csHPPSMath = 2020,
  csHPDesktop = 2021,
  csVenturaMath = 2022,
  csMicrosoftPublishing = 2023,
  csWindows31J = 2024,
  csGB2312 = 2025,               /* GB2312 */
  csBig5 = 2026,                 /* Big5 */
  csMacintosh = 2027,
  csIBM037 = 2028,
  csIBM038 = 2029,
  csIBM273 = 2030,
  csIBM274 = 2031,
  csIBM275 = 2032,
  csIBM277 = 2033,
  csIBM278 = 2034,
  csIBM280 = 2035,
  csIBM281 = 2036,
  csIBM284 = 2037,
  csIBM285 = 2038,
  csIBM290 = 2039,
  csIBM297 = 2040,
  csIBM420 = 2041,
  csIBM423 = 2042,
  csIBM424 = 2043,
  csIBM500 = 2044,
  csIBM851 = 2045,
  csIBM855 = 2046,
  csIBM857 = 2047,
  csIBM860 = 2048,
  csIBM861 = 2049,
  csIBM863 = 2050,
  csIBM864 = 2051,
  csIBM865 = 2052,
  csIBM868 = 2053,
  csIBM869 = 2054,
  csIBM870 = 2055,
  csIBM871 = 2056,
  csIBM880 = 2057,
  csIBM891 = 2058,
  csIBM903 = 2059,
  csIBBM904 = 2060,
  csIBM905 = 2061,
  csIBM918 = 2062,
  csIBM1026 = 2063,
  csIBMEBCDICATDE = 2064,
  csEBCDICATDEA = 2065,
  csEBCDICCAFR = 2066,
  csEBCDICDKNO = 2067,
  csEBCDICDKNOA = 2068,
  csEBCDICFISE = 2069,
  csEBCDICFISEA = 2070,
  csEBCDICFR = 2071,
  csEBCDICIT = 2072,
  csEBCDICPT = 2073,
  csEBCDICES = 2074,
  csEBCDICESA = 2075,
  csEBCDICESS = 2076,
  csEBCDICUK = 2077,
  csEBCDICUS = 2078,
  csUnknown8BiT = 2079,
  csMnemonic = 2080,
  csMnem = 2081,
  csVISCII = 2082,
  csVIQR = 2083,
  csKOI8R = 2084,                 /* KOI8-R */
  csHZGB2312 = 2085,
  csIBM866 = 2086,
  csPC775Baltic = 2087,
  csKOI8U = 2088,                 /* KOI8-U */
  csIBM00858 = 2089,
  csIBM00924 = 2090,
  csIBM01140 = 2091,
  csIBM01141 = 2092,
  csIBM01142 = 2093,
  csIBM01143 = 2094,
  csIBM01144 = 2095,
  csIBM01145 = 2096,
  csIBM01146 = 2097,
  csIBM01147 = 2098,
  csIBM01148 = 2099,
  csIBM01149 = 2100,
  csBig5HKSCS = 2101,             /* Big5-HKSCS */
  csIBM1047 = 2102,
  csPTCP154 = 2103,               /* PTCP154 */
  csAmiga1251 = 2104,
  csKOI7switched = 2105,
  csBRF = 2106,
  csTSCII = 2107,
  csCP51932 = 2108,
  cswindows874 = 2109,
  cswindows1250 = 2250,
  cswindows1251 = 2251,          /* windows-1251 */
  cswindows1252 = 2252,          /* windows-1252 */
  cswindows1253 = 2253,
  cswindows1254 = 2254,
  cswindows1255 = 2255,
  cswindows1256 = 2256,          /* windows-1256 */
  cswindows1257 = 2257,
  cswindows1258 = 2258,
  csTIS620 = 2259,               /* ISO-8859-11 */
  cs50220 = 2260,
  reserved = 3000
} MIBenum;

#endif	/* !_AS_MIBenum_H_ */
