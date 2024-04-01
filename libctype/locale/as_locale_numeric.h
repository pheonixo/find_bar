/*
 *  as_locale_numeric.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Sat 30 Nov 2013.
 *  Copyright (c) 2013. All rights reserved.
 *
 */

/* initial thoughts and workings "as_locale_numeric_bak00.h" */

#ifndef _AS_LOCALE_NUMERIC_H_
#define _AS_LOCALE_NUMERIC_H_

/* Format symbols: ($ #,***,##0.00!-/%)
 * ' ' = <nbsp>
 * '!' = <exponential symbol>
 * '#' = digit place holder
 * '$' = <currency sign> either normal or narrow
 * '%' = <percentage symbol>
 * '(' = literal, start of negative format
 * ')' = literal, end of negative format
 * '*' = digit place holder, part of grouping, continous indicates number in group
 * ',' = <grouping symbol>
 * '-' = <minusSign symbol>
 * '.' = <decimal symbol>
 * '/' = <fractional symbol>
 * '0' = digit, must placement
 */

#include "sys/as_types.h"       /* bit types */
#include "as_locale_codeset.h"  /* info on lc_codeset_st */

/* flags defines, communication to numericWr, numericRd */
  /* basic numeric types */
#define SIGNED_VALUE  0x80000000
#define CAST_BIT      0x80000000  /* signed/unsigned value (typecast)  */
#define REAL_BIT      0x40000000  /* use as real unit */
#define OVFL_BIT      0x20000000  /* flag counter, use overflow of real unit */
  /* special formating */
#define JUSTIFY_BIT   0x08000000  /* justify left/right, 0=left (pad side)  */
#define GROUP_BIT     0x04000000  /* group by locale's grouping specs. */

/* digit field filler */
#define FILL_MASK     0x00C00000  /* digit filler,  3 states (mask, jc) */
#define FILL_ZERO     0x00400000  /* 0,1,2 => none, '0', ' ' */
#define FILL_SPACE    0x00800000
/* padding for outside digit field */
#define PAD_MASK      0x00300000  /* pad,  3 states (mask, jc) */
#define PAD_ZERO      0x00100000  /* 0,1,2 => none, '0', ' ' */
#define PAD_SPACE     0x00200000
/* sign designation */
#define SIGN_MASK     0x000F0000  /* sign, 4 states (mask, jc) */
#define SIGN_MIN      0x00010000  /* 0,1,2,4 */
#define SIGN_MUST     0x00020000
#define SIGN_NEG      0x00040000  /* use to force -0 */
/* field sizes */
#define TOTAL_MASK    0x0000FF00
#define TOTAL_FIELD   ((flags & TOTAL_MASK) >> 8)   /* field size (255 limit) */
#define DIGIT_MASK    0x000000FF
#define DIGIT_FIELD   (flags & DIGIT_MASK)         /* precision  (255 limit) */

/* DIGIT_FIELD = 1, PAD_SPACE, FILL_ZERO */
#define FLAG_DEFAULT  (FILL_ZERO | PAD_SPACE | 1)

/* issue: active rwSystem, changing; 'rwSystems' doesn't get altered */
/* establish this the same as codeset, use mask? converted as Enum for id.
 * I believe no. 0 links with active, but dont need unless allow switching,
 * user-defined?
 */
/* Numerical Representation System */
/* This behaves simular to codesets. For a given language, a territory
 * may have up to (but not limited to) four means of representing numerals
 * and its symbols. These enums are ids, used to identify the read/write
 * functions of that numerical system. Symbols for that system, in a given
 * language and territory, are found by matching up the appropriate rwSystem
 * combo. rwSystem is 2 bytes of data, one containing NRS, the other containing
 * 0 or 1, which identifies numbers0 or numbers1 respectively. numbers0 and
 * numbers1 contain the formats and symbols used with the NRS. */
/*  NRS, numericRd/numericWr, and numbers are set to the active means of
 * representing numerics. numbers0, numbers1, and rwSystems are 'const' data
 * that allows switching to a given system of representation.
 *  A program/application can set preferences, altering the 'const' data.
 * This creates allocated memory pointers to the altered data. A program/
 * application can then save/load these preferences, allowing for user-defined
 * locales, or newly created locales.
 */

/*  Shift rwEnum, RW_UNSET. On new locale, should use modifier to set
 * numeric system desired. No guessing which is native, traditional, default.
 * This then allows any locale to use non-specific systems like roman numerals.
 */
/*
 *   To create backwards compatibility and encourage expansion, would be best
 * to do four-letter codes which translate to one 32bit unique id. This means
 * early assigned values will always be the same, and filling in new 'enums'
 * can be alphabetically sorted (if I do it correctly).
 *   This would mean code to idx to use.
 */
/*
 *   Enum leaves open others including new rwSystems, which could conflict
 * with me or those I assign licences to, conflicting.
 */
/*   4-letter codes: do I need to worry about endianess. It is still a unique
 * id.
 */
/* what if id were "arabRd"? */
/* Found it! Use ISO639-2 codes. Is more complete and seems to correctly label
 * scripts, versus CLDR's ISO15924.
 */
/* so ISO639-2 codes as base, modifier as code + 1?
 * NOTE: can not use ISO-3, copyright notice forbids.
 * ISO15924 is suppose to be for scripts. But wow how poorly done! To add
 * a variant looks like nightmare. Use mine, let others deal with ISO15924.
 */

/* RW_LATN */
#define RW_DEFAULT  42

  /* those without natural enum are yet to be done.
   * as of now, only explicit numbered are used by locales */
typedef enum {
  RW_ADLM       = RW_DEFAULT,     /* 0 when completed */
  RW_AHOM       = RW_DEFAULT,
  RW_ARAB       = 2,
  RW_ARABEXT    = 3,
  RW_ARMN       = 4,
  RW_ARMNLOW    = RW_ARMN,
  RW_BALI       = RW_DEFAULT,
  RW_BENG       = 7,
  RW_BHKS       = RW_DEFAULT,
  RW_BRAH       = RW_DEFAULT,
  RW_CAKM       = 10,
  RW_CHAM       = RW_DEFAULT,
  RW_CYRL       = 12,
  RW_DEVA       = 13,
  RW_ETHI       = 14,
  RW_FULLWIDE   = RW_DEFAULT,
  RW_GEOR       = 16,
  RW_GONG       = RW_DEFAULT,
  RW_GONM       = RW_DEFAULT,
  RW_GREK       = 19,
  RW_GREKLOW    = RW_GREK,
  RW_GUJR       = 21,
  RW_GURU       = 22,
  RW_HANIDAYS   = 24,             /* forward to be 23 */
  RW_HANIDEC    = 24,
  RW_HANS       = 25,
  RW_HANSFIN    = RW_HANS,
  RW_HANT       = 27,
  RW_HANTFIN    = RW_HANT,
  RW_HEBR       = 29,
  RW_HMNG       = RW_DEFAULT,
  RW_HMNP       = RW_DEFAULT,
  RW_JAVA       = 32,
  RW_JPAN       = 33,
  RW_JPANFIN    = RW_JPAN,
  RW_JPANYEAR   = RW_JPAN,
  RW_KALI       = RW_DEFAULT,
  RW_KHMR       = 37,
  RW_KNDA       = 38,
  RW_LANA       = RW_DEFAULT,
  RW_LANATHAM   = RW_DEFAULT,
  RW_LAOO       = 41,
  RW_LATN       = 42,
  RW_LEPC       = RW_DEFAULT,
  RW_LIMB       = RW_DEFAULT,
  RW_MATHBOLD   = RW_DEFAULT,
  RW_MATHDBL    = RW_DEFAULT,
  RW_MATHMONO   = RW_DEFAULT,
  RW_MATHSANB   = RW_DEFAULT,
  RW_MATHSANS   = RW_DEFAULT,
  RW_MLYM       = 50,
  RW_MODI       = RW_DEFAULT,
  RW_MONG       = RW_DEFAULT,
  RW_MROO       = RW_DEFAULT,
  RW_MTEI       = RW_DEFAULT,
  RW_MYMR       = 55,
  RW_MYMRSHAN   = RW_MYMR,
  RW_MYMRTLNG   = RW_MYMR,
  RW_NEWA       = RW_DEFAULT,
  RW_NKOO       = RW_DEFAULT,
  RW_OLCK       = RW_DEFAULT,
  RW_ORYA       = 61,
  RW_OSMA       = RW_DEFAULT,
  RW_ROHG       = RW_DEFAULT,
  RW_ROMAN      = RW_DEFAULT,
  RW_ROMANLOW   = RW_DEFAULT,
  RW_SAUR       = RW_DEFAULT,
  RW_SHRD       = RW_DEFAULT,
  RW_SIND       = RW_DEFAULT,
  RW_SINH       = RW_DEFAULT,
  RW_SORA       = RW_DEFAULT,
  RW_SUND       = RW_DEFAULT,
  RW_TAKR       = RW_DEFAULT,
  RW_TALU       = RW_DEFAULT,
  RW_TAML       = 74,
  RW_TAMLDEC    = 75,
  RW_TELU       = 76,
  RW_THAI       = 77,
  RW_TIBT       = 78,
  RW_TIRH       = RW_DEFAULT,
  RW_VAII       = 80,
  RW_WARA       = RW_DEFAULT,
  RW_WCHO       = RW_DEFAULT,
  /* all additions must go here, backwards compatibility */
  RW_LAST       = 82
} NRSenum;

/* combine rwSystems with numbers, hold [rwEnum/uint8_t *] */
/* numericRd/Wr:
 *    uint8_t * passed is symbols, a "const" type, either defined by
 *   static uint8_t array or loaded because of user changes which should
 *   be considered "const".
 */

typedef enum {
  NS_ALLOCSZ = 0,
  NS_DECIMALFORMAT,
  NS_PERCENTFORMAT,
  NS_SCIENTIFICFORMAT,
  NS_ACOUNTINGFORMAT,
  NS_CURRENCYFORMAT,
  NS_DECIMAL,
  NS_EXPONENTIAL,
  NS_GROUP,
  NS_INFINITY,
  NS_MINUSSIGN,
  NS_NAN,
  NS_PERMILLE,
  NS_PERCENTSIGN,
  NS_PLUSSIGN,
  NS_SUPERSCRIPTINGEXPONENT,
  NS_LAST = NS_SUPERSCRIPTINGEXPONENT
} nsEnum;

typedef struct _locale_rwpair {
  csEnum  (*numericRd)(csCnvrt_st *, uint8_t *, uint32_t *);
  csEnum  (*numericWr)(csCnvrt_st *, uint8_t *, uint32_t *);
} rwpair_st;

    /* even thou CLDR defines 4 scripts, only 2 numbers used by 4 scripts */
typedef struct {
  uint8_t *nsys;        /* pointer to 4 packed 16s: [NRSenum:index][8:8] */
  uint8_t *nmrc[2];  /* 2 formats/symbols based on script (rwpair_st) */
    /* these set to default, initially, rwSystems[LCPT_RW_DEFAULT] */
  NRSenum  NRS;
  csEnum  (*numericRd)(csCnvrt_st *, uint8_t *, uint32_t *);
  csEnum  (*numericWr)(csCnvrt_st *, uint8_t *, uint32_t *);
  uint8_t *numbers;     /* formats/symbols */
} lc_numeric_st;

#ifdef __cplusplus
extern "C" {
#endif

csEnum arabRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum arabWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum arabextRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum arabextWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum armnRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum armnWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum bengRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum bengWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum cakmRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum cakmWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum cyrlRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum cyrlWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum devaRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum devaWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum ethiRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum ethiWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum georRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum georWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum grekRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum grekWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum gujrRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum gujrWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum guruRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum guruWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hanidecRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hanidecWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hansRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hansWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hantRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hantWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hebrRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum hebrWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum javaRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum javaWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum jpanRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum jpanWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum khmrRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum khmrWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum kndaRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum kndaWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum laooRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum laooWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum latnRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum latnWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum mlymRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum mlymWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum mymrRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum mymrWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum oryaRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum oryaWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tamlRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tamlWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tamldecRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tamldecWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum teluRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum teluWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum thaiRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum thaiWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tibtRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum tibtWr(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum vaiiRd(csCnvrt_st *, uint8_t *, uint32_t *);
csEnum vaiiWr(csCnvrt_st *, uint8_t *, uint32_t *);

const rwpair_st *rwNRSpairng(NRSenum);

#ifdef __cplusplus
}
#endif

#endif	/* !_AS_LOCALE_NUMERIC_H_ */
