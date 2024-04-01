/*
 *  newlocale  - create or modify a locale object
 *  duplocale  - duplicate a locale object
 *  freelocale - free resources allocated for a locale object
 *  setlocale  - set program locale (environment global)
 *
 *  uselocale  - missing - use locale in current thread - untouched thread functions
 *
 *  localenew - create new locale object
 *  localemodify - modify existing locale object
 *  Pheonix
 *
 *  Created by Steven Abner on Sun 28 Apr 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 *  moving "locale.c" to here for now. Recreation of code from definition.
 * Verication process I employ.
 *  moving "setlocale.c" to here. lots of duplicate static use. marked retired.
 *  locale.c now marked retired, has some orginal design and cocept notes.
 *
 *  old functions created... now the thread safe, new functions to make use of
 * the data.
 *  concepts:
 * char *
 * setlocale(int category, const char *locale) {
 *  // grab current strings
 *  if (locale == NULL)
 *    return current string of category
 *  if ((category != LC_ALL) && (category != LC_CREATE)) {
 *    if (localemodify(pxUserLocale, locale, category))
 *      return NULL;
 *  } else if (localenew(&pxUserLocale, locale))  <= this was sorta getlocale()
 *    return NULL;
 *  // set those current to prior, set current strings
 * }
 *
 * with localnew new category, lc_create  <= replaces newlocale with base as (locale_t)0
 * NO new category.... pass (locale_t)0 in locale_t *ld, the return pointer '*ld'
 * the setlocale() locale == NULL querry... return current strings
 * localemodify(locale_t, char*, int) <= edits locale, for global pass global
 * localeget(locale_t, int) <= accessors
 * duplocale, freelocale stay same, macros localedup, localefree
 *
 * localeset, localemodify is thread safe WHEN not using the environmental locale.
 * However, what if one thread modify, when another using accessor <= case poor
 *  thread design?
 *    because user/designer in control of the 'locale', not old way of
 *    user/designer at the mercy of global environment usage
 * belief is there shouldn't be enviromental locale… a system', users's locales
 * can be copied, but threads pass info via arguements, or set an 'environment'
 * flag and act on it
 */

/*  the argument...
 const char *locale
 locale must be in this form: [language[_Script]][_TERRITORY][.codeset][@modifier]
 locale conforms to language:  ISO 639-1, ISO 639-3
 locale conforms to territory: ISO 3166
 locale conforms to codeset:   IANA MIME name or IANA Name (hopefully) see code
 @modifier: not to be used, but set up for 15 char for others since they like
 "@Latin" instead of ".ISO-8859-1".
 */

#include "as_locale.h"
#include "as_errno.h"

extern int sscanf(const char *s, const char *format, ... );
extern void *bsearch(const void *, const void *, size_t, size_t,
                     int (*compar)(const void *, const void *));
extern void *memmove(void *s1, const void *s2, size_t n);
extern void *memset(void *, int, size_t);
extern char *stpcpy(char *restrict, const char *restrict);
extern char *strcpy(char *, const char *);
extern int strncmp(const char *, const char *, size_t);
extern char *strncpy(char *restrict, const char *restrict, size_t);
extern int setcs(locale_t, const char *);
extern void *malloc(size_t size);
extern void free(void *);


/* CLDR parsed data... the majority of locale definition */
/* from locale_data.c */
struct _locale_defined {
  uint64_t ident;
  uint8_t *messages;
  void *nsys;        /* pointer to 4 packed 16s: [NRSenum:index][8:8] */
  void *nmrc[2];     /* 2 formats/symbols based on script (rwpair_st) */
  void *monetary;
  void *calendar;    /* Names of months, days, am/pm */
  void *formats;     /* Display of time, ex: "/-1073743992/" */
  void *greg_era;    /* Names of gregorian era */
  void *tsol_era;    /* Names of thai solar era */
  void *jpyr_era;    /* Names of japanese era */
  void *ming_era;    /* Names of mingo era */
};

extern struct _locale_defined mini_locale[];
extern size_t mini_locale_sizeof(void);

/* special Note:
 * older specification:
 *  POSIX requires "define a locale as the default locale, to be invoked
 * when set to the empty string".
 */

/*
 *  The newlocale() function shall create a new locale object or modify an
 * existing one. If the base argument is ( locale_t)0, a new locale object shall
 * be created. It is unspecified whether the locale object pointed to by base
 * shall be modified, or freed and a new locale object created.
 *  The category_mask argument specifies the locale categories to be set or
 * modified. Values for category_mask shall be constructed by a
 * bitwise-inclusive OR of the symbolic constants LC_CTYPE_MASK,
 * LC_NUMERIC_MASK, LC_TIME_MASK , LC_COLLATE_MASK, LC_MONETARY_MASK,
 * and LC_MESSAGES_MASK, or any of the implementation-defined mask values
 * defined in <locale.h>.
 *  For each category with the corresponding bit set in category_mask the data
 * from the locale named by locale shall be used. In the case of modifying an
 * existing locale object, the data from the locale named by locale shall
 * replace the existing data within the locale object. If a completely new
 * locale object is created, the data for all sections not requested by
 * category_mask shall be taken from the default locale.
 *  The following preset values of locale are defined for all settings of
 * category_mask:
 *  "POSIX":
 *    Specifies the minimal environment for C-language translation called the
 *    POSIX locale.
 *  "C":
 *    Equivalent to "POSIX".
 *  "":
 *    Specifies an implementation-defined native environment. This corresponds
 *    to the value of the associated environment variables, LC_* and LANG ; see
 *    XBD Locale and Environment Variables.
 *  If the base argument is not (locale_t)0 and the newlocale() function call
 * succeeds, the contents of base are unspecified. Applications shall ensure
 * that they stop using base as a locale object before calling newlocale(). If
 * the function call fails and the base argument is not (locale_t)0, the
 * contents of base shall remain valid and unchanged.
 *  The behavior is undefined if the base argument is the special locale object
 * LC_GLOBAL_LOCALE, or is not a valid locale object handle and is not
 * (locale_t)0.
 *
 *        RETURN VALUE
 *  Upon successful completion, the newlocale() function shall return a handle
 * which the caller may use on subsequent calls to duplocale(), freelocale(),
 * and other functions taking a locale_t argument.
 *  Upon failure, the newlocale() function shall return (locale_t)0 and set
 * errno to indicate the error.
 *
 *        ERRORS
 *  The newlocale() function shall fail if:
 * [ENOMEM]:
 *    There is not enough memory available to create the locale object or load
 * the locale data.
 * [EINVAL]:
 *    The category_mask contains a bit that does not correspond to a valid
 * category.
 * [ENOENT]:
 *    For any of the categories in category_mask, the locale data is not
 * available.
 *
 *  The newlocale() function may fail if:
 * [EINVAL]:
 *    The locale argument is not a valid string pointer.
 */

static int findIndent(const void *a, const void *b) {
  uint64_t find = *(uint64_t*)a;
  uint64_t query = (((struct _locale_defined *)b)->ident);
  if (query > find)  return -1;
  if (query < find)  return 1;
  return 0;
}

/*
 * Original concept, convert adjusted for numeric regions, and named locales.
 *  All locale_t creation/modify must go thru here!
 * Instead of LC_NAMES for environment debugging, use compressed ids. A utility
 * should be called to display environmental debugging variables. Compression
 * ids are each alpha character of a locale (language/territory) to form id.
 * Should a script be involed, a bit is set to signal a binary search.
 *   Issues solved by 64bit encoding. Script needs 20bits, other 2, 15bits each.
 * This still reduces from 16 byte to 8 byte.
 *   If ISO 639-6 occurs, 35bits would be needed, so 64bit alows mega growth.
 *
 * shi_Tfng_MA  =>
 *
 *                 | ('T' - ('A' - 1)) << 15
 *                 | ('f' - ('a' - 1)) << 10
 *                 | ('n' - ('a' - 1)) <<  5
 *                 | ('g' - ('a' - 1)) <<  0

 * shi_MA       =>   ('s' - ('a' - 1)) << 20
 *                 | ('h' - ('a' - 1)) << 15
 *                 | ('i' - ('a' - 1)) << 10
 *                 | ('M' - ('A' - 1)) <<  5
 *                 | ('A' - ('A' - 1)) <<  0
 *              Note that if ISO 3166-3 then << 10
 * NOTE: can force this to 24 bits
 */

static uint64_t
convertString(char *sPtr) {

  /* each code [0-9,A-Z,a-z] gets 2^6 slot. each offset by 1, '0' used as
   * no code placeholder. Language uses 3 slots (iso639-2),
   * Script uses 4 slots (iso15924), and Territory holds three slots
   * (iso3166-1 alpha-2 or grouping numeric).
   */
  /* value 11 from: adjust so numeric "grouping zones" sort first [0-9] and all
   * values + 1 so 0 indicates no value. No value used when no code goes in
   * assigned slot (ignored slot).
   */

  /* problem: alias use of 'C' and 'POSIX' */
  int idx;
  int language;
  if ((sPtr[0] - 'a') < 0) {
    /* aliases, upper cased */
    if (sPtr[1] == 0)
      return (uint64_t)(sPtr[0] - ('A' - 11));
    return (uint64_t)(  ((sPtr[0] - ('A' - 11)) << 12)
                      | ((sPtr[1] - ('A' - 11)) <<  6)
                      | ((sPtr[2] - ('A' - 11))));
  }

  language = ((sPtr[0] - ('a' - 1)) << 12) | ((sPtr[1] - ('a' - 1)) <<  6);
  if (sPtr[2] == '_') {
    idx = 3;
  } else {
    idx = 4;
    language |= (sPtr[2] - ('a' - 1));
  }
  int script = 0;
  if ((sPtr[(idx + 1)] - 'a') >= 0) {
    /* lower case must be higher value then upper case */
    script = (        ((sPtr[idx] - ('A' - 11)) << 18)
              | ((sPtr[(idx + 1)] - ('a' - 27)) << 12)
              | ((sPtr[(idx + 2)] - ('a' - 27)) <<  6)
              |  (sPtr[(idx + 3)] - ('a' - 27)));
    idx += 5;
  }
  /* can be numeric */
  int territory;
  if ((sPtr[idx] - 'A') < 0) {
    /* numeric grouping of territories */
    territory = (        ((sPtr[idx] - ('0' - 1)) << 12)
                 | ((sPtr[(idx + 1)] - ('0' - 1)) <<  6)
                 | ((sPtr[(idx + 2)] - ('0' - 1))));
    idx += 2;
  } else {
    territory = (        ((sPtr[idx] - ('A' - 11)) << 12)
                 | ((sPtr[(idx + 1)] - ('A' - 11)) <<  6));
    /* shouldn't allow? alpha-3 */
    /* must allow! case: en_US_POSIX, uses '_' which > uppercase == 'g' */
    if (sPtr[(idx += 2)] != 0) {
      territory |= (sPtr[idx] - ('A' - 11));
    }
  }

  return ((script == 0) ?
          (  ((uint64_t)language << 42)
           | ((uint64_t)territory << 24)) :
          (  ((uint64_t)language << 42)
           | ((uint64_t)script << 18)
           | ((uint64_t)territory)));

}

/* locale descriptor = ld */
int
localenew(locale_t *ld, const char *locale) {

    /* parse 'locale'. If invalid, EINVAL. return (locale_t)0 */
  char l_id[16], l_cs[16], l_xtra[16];
  struct _locale_defined *req_loc;
  uint64_t ident;

    /* POSIX says EINVAL, not ENOENT */
  if (locale == NULL)  return (int)EINVAL;
    /* verify locale before creating anything */
  memset(l_id, 0, (size_t)16);
  memset(l_cs, 0, (size_t)16);
    /* define "" for 'locale' */
  ident = pxUserLocale->lc_ident;
  if (*locale != 0)  {
      /* orginal parser had 4 modifers, the 1 of 4 era descriptors */
    int  nid = 0, ncs = 0;
    int matched = sscanf(locale, "%[^.]%n.%[^@]%n%s",
                                              l_id, &nid, l_cs, &ncs, l_xtra);
    if ((matched == 0) || (nid >= 16))  return (errno = EINVAL);
    if (matched > 1) {
        /* verify requested codeset */
      locale_st dummy;
      if (((ncs - nid) > 16) || setcs(&dummy, l_cs))  return (errno = EINVAL);
    }
    ident = convertString(l_id);
  }
  req_loc = (struct _locale_defined *)bsearch(&ident, (void*)mini_locale,
                                  mini_locale_sizeof()/sizeof(mini_locale[0]),
                                  sizeof(mini_locale[0]), &findIndent);
    /* NULL == not a valid 'locale' */
  if (req_loc == NULL)  return (errno = EINVAL);
    /* assign codeset default if not requested */
  if (*l_cs == 0)  strcpy(l_cs, "UTF-8");
    /* 'locale' argument found valid... continue */
    /* if *ld is not (locale_t)0 then replacing data in valid pointer */
  locale_t nloc = *ld;
  if (*ld == (locale_t)0) {
    nloc = (locale_t)malloc(sizeof(locale_st));
    if (nloc == (locale_t)0)  return (errno = ENOMEM);
  }
    /* fill in ident */
  nloc->lc_ident = ident;

    /* LC_ALL */
  setcs(nloc, l_cs);
  nloc->lc_messages.messages = (uint8_t*)req_loc->messages;
  nloc->lc_numeric.nsys = (uint8_t*)req_loc->nsys;
  nloc->lc_numeric.nmrc[0] = (uint8_t*)req_loc->nmrc[0];
  nloc->lc_numeric.nmrc[1] = (uint8_t*)req_loc->nmrc[1];
    /* fill in quick access info, so dont parse when needed */
  nloc->lc_numeric.NRS = (NRSenum)nloc->lc_numeric.nsys[0];
  nloc->lc_numeric.numbers = nloc->lc_numeric.nmrc[nloc->lc_numeric.nsys[1]];
  const rwpair_st *rwFns = rwNRSpairng(nloc->lc_numeric.NRS);
  nloc->lc_numeric.numericRd = rwFns->numericRd;
  nloc->lc_numeric.numericWr = rwFns->numericWr;
    /* back to status quo */
  nloc->lc_monetary.monetary = (uint8_t*)req_loc->monetary;
  nloc->lc_time.calendar = (uint8_t*)req_loc->calendar;
  nloc->lc_time.formats = (uint8_t*)req_loc->formats;
  nloc->lc_time.era = (uint8_t*)req_loc->greg_era;
  nloc->lc_time.era_dates = (uint8_t*)&__greg_date;

  *ld = nloc;
  return (int)ENOERR;
}

int
localemodify(locale_t ld, const char *locale, int category_mask) {

  if ((ld == (locale_t)0) || (locale == NULL))  return (int)ENOENT;

  locale_t nloc = NULL;
  if (localenew(&nloc, locale))  return errno;

  if        ((category_mask & LC_ALL_MASK) == LC_ALL_MASK) {
    memmove(ld, nloc, sizeof(locale_st));
  } else {
    /* if (category_mask & LC_COLLATE) { not implemented }*/
    if (category_mask & LC_CODESET_MASK)
      memmove(&ld->lc_codeset, &nloc->lc_codeset, sizeof(lc_codeset_st));
    if (category_mask & LC_MONETARY_MASK)
      memmove(&ld->lc_monetary, &nloc->lc_monetary, sizeof(lc_monetary_st));
    if (category_mask & LC_NUMERIC_MASK)
      memmove(&ld->lc_numeric, &nloc->lc_numeric, sizeof(lc_numeric_st));
    if (category_mask & LC_TIME_MASK)
      memmove(&ld->lc_time, &nloc->lc_time, sizeof(lc_time_st));
    if (category_mask & LC_MESSAGES_MASK)
      memmove(&ld->lc_messages, &nloc->lc_messages, sizeof(lc_messages_st));
      /* done after LC_NUMERIC_MASK! */
    if (category_mask & LC_NSRW_MASK) {
          /* set one of the 4 number systems */
      int cm = ((category_mask - 1) & LC_NSRW_MASK) >> 5; /* >>= 6 <<= 1 */
      ld->lc_numeric.NRS = ((uint8_t *)nloc->lc_numeric.nsys)[cm];
      cm = ((uint8_t *)nloc->lc_numeric.nsys)[(cm + 1)];
      ld->lc_numeric.numbers = nloc->lc_numeric.nmrc[cm];
      const rwpair_st *rwFns = rwNRSpairng(ld->lc_numeric.NRS);
      ld->lc_numeric.numericRd = rwFns->numericRd;
      ld->lc_numeric.numericWr = rwFns->numericWr;
    }
    if (category_mask & LC_ERAS_MASK) {
      struct _locale_defined *req_loc;
      req_loc = (struct _locale_defined *)bsearch(&ld->lc_ident,
                                                  (void*)mini_locale,
                                    mini_locale_sizeof()/sizeof(mini_locale[0]),
                                          sizeof(mini_locale[0]), &findIndent);
      if ((category_mask & LC_ERAS_MASK) == LC_ERAGREG) {
        ld->lc_time.era = req_loc->greg_era;
      } else if ((category_mask & LC_ERAS_MASK) == LC_ERATSOL) {
        ld->lc_time.era = req_loc->tsol_era;
      } else if ((category_mask & LC_ERAS_MASK) == LC_ERAMING) {
        ld->lc_time.era = req_loc->ming_era;
      } else {
        ld->lc_time.era = req_loc->jpyr_era;
      }
    }
  }
  freelocale(nloc);
  return (int)ENOERR;
}

locale_t
newlocale(int category_mask, const char *locale, locale_t base) {

  /*  Create a new locale_st with these contents base on:
   * (locale_t)0:
   *    all contents of the named 'locale' argument. The category_mask
   *    is ignored, returned is the full 'locale' named.
   * all others (assumes valid locobj in base):
   *    contents of the named 'locale' argument replacing a copy of the
   *    LC_GLOBAL_LOCALE locobj in the newly created locobj, specified by
   *    category_mask as to what gets replaced.
   */

  locale_t nloc = NULL;
  if (localenew(&nloc, locale))
    return (locale_t)0;
  if (base == (locale_t)0)  return nloc;

    /* based on category_mask, replace not requested by LC_GLOBAL_LOCALE */
  category_mask ^= LC_ALL_MASK;
    /* LC_COLLATE is not implemented at this time */
    /* the 'global' locale */
  locale_t gloc = pxUserLocale;
  if ((category_mask & LC_CODESET_MASK) != 0)
    memmove(&nloc->lc_codeset, &gloc->lc_codeset, sizeof(lc_codeset_st));
  if ((category_mask & LC_MESSAGES_MASK) != 0)
    memmove(&nloc->lc_messages, &gloc->lc_messages, sizeof(lc_messages_st));
  if ((category_mask & LC_MONETARY_MASK) != 0)
    memmove(&nloc->lc_monetary, &gloc->lc_monetary, sizeof(lc_monetary_st));
  if ((category_mask & LC_NUMERIC_MASK) != 0)
    memmove(&nloc->lc_numeric, &gloc->lc_numeric, sizeof(lc_numeric_st));
  if ((category_mask & LC_TIME_MASK) != 0)
    memmove(&nloc->lc_time, &gloc->lc_time, sizeof(lc_time_st));

  return nloc;
}

locale_t
duplocale(locale_t locobj) {

    /* here for now, incase LC_GLOBAL_LOCALE changes to hide 'pxUserLocale' */
  if (locobj == LC_GLOBAL_LOCALE)  locobj = pxUserLocale;

  locale_t nloc = (locale_t)malloc(sizeof(locale_st));
  if (nloc == (locale_t)NULL) {
    errno = ENOMEM;
    return nloc;
  }
  memmove(nloc, locobj, sizeof(locale_st));
  return nloc;
}

void
freelocale(locale_t locobj) {
    /* currently no add ons/ins */
  free(locobj);
}


/* this is for return values, non-thread safe! */
static char str_ALL[6*16] = { 0 };
static char str_setlocale[6*16] = { 0 };
//static char * const str_COLLATE      = &str_setlocale[ 0];
//static char * const str_CODESET      = &str_setlocale[16];
//static char * const str_MONETARY     = &str_setlocale[32];
//static char * const str_NUMERIC      = &str_setlocale[48];
//static char * const str_TIME         = &str_setlocale[64];
//static char * const str_MESSAGES     = &str_setlocale[80];
static char str_prior_ALL[6*16] = { 0 };
static char str_prior_setlocale[6*16] = { 0 };
//static char * const str_prior_COLLATE  = &str_prior_setlocale[ 0];
//static char * const str_prior_CODESET  = &str_prior_setlocale[16];
//static char * const str_prior_MONETARY = &str_prior_setlocale[32];
//static char * const str_prior_NUMERIC  = &str_prior_setlocale[48];
//static char * const str_prior_TIME     = &str_prior_setlocale[64];
//static char * const str_prior_MESSAGES = &str_prior_setlocale[80];

char *
setlocale(int category, const char *locale) {

  if (locale == NULL) {
    if (category == LC_ALL)
          return str_ALL;
    else  return &str_setlocale[((category - 1) << 4)];
  }
    /* grab current strings */
  char bufA[6*16];
  memmove(bufA, str_setlocale, (size_t)(6*16));
  if (category == LC_ALL) {
    if (localenew(&pxUserLocale, locale))  return NULL;
      /* altered environment global, set strings to reflect data */
    memmove(str_prior_setlocale, bufA, (size_t)(6*16));
    for (category = LC_COLLATE; category <= LC_MESSAGES; category++)
      strncpy(&str_setlocale[((category - 1) << 4)], locale, (size_t)16);
    strncpy(str_ALL, locale, (size_t)(6*16));
      /* altered environment global, set return strings to reflect data */
      /* current set, now set prior, the return info */
    int diff = 0;
    for (category = LC_CODESET; category <= LC_MESSAGES; category++)
      diff += strncmp(&str_setlocale[0],
                            &str_setlocale[((category - 1) << 4)], (size_t)16);
    if (diff == 0) strncpy(str_prior_ALL, &str_setlocale[0], (size_t)(6*16));
    else {
      char *nilPtr = stpcpy(str_prior_ALL, &str_setlocale[0]);
      for (category = LC_CODESET; category <= LC_MESSAGES; category++) {
        *nilPtr = ';', nilPtr++;
        stpcpy(nilPtr, &str_setlocale[((category - 1) << 4)]);
      }
    }
    return str_prior_ALL;
  }  /* if (category == LC_ALL) */

    /* modify global environment locale with 'locale' for the 'category' */
  int mask;
  if      (category == LC_COLLATE)   mask = LC_COLLATE_MASK;
  else if (category == LC_CODESET)   mask = LC_CODESET_MASK;
  else if (category == LC_MONETARY)  mask = LC_MONETARY_MASK;
  else if (category == LC_NUMERIC)   mask = LC_NUMERIC_MASK;
  else if (category == LC_TIME)      mask = LC_TIME_MASK;
  else if (category == LC_MESSAGES)  mask = LC_MESSAGES_MASK;
  else {
/* XXX unhandle error? */
    mask = LC_ALL;
  }
  if (localemodify(pxUserLocale, locale, mask))
    return NULL;
  memmove(str_prior_setlocale, bufA, (size_t)(6*16));
  strncpy(&str_setlocale[((category - 1) << 4)], locale, (size_t)16);
  return &str_prior_setlocale[((category - 1) << 4)];
}

void *
lc_getIndexItemPtr(lcatEnum category, lcptEnum item,
                   locale_t ld, uint32_t idx) {
  (void)category;
  /* make use that only ".calendar" only uses arrays */
  tstr_st *info = &((tstr_st *)ld->lc_time.calendar)[item];
  /* offset & increment 16bit */
  uint16_t offset    = info->offset;
  uint16_t increment = info->increment;
#ifdef __LITTLE_ENDIAN__
  offset = (offset >> 8) | (offset << 8);
  increment = (increment >> 8) | (increment << 8);
#endif
  return (ld->lc_time.calendar + offset + (idx * increment));
}

/* NOTE:
 *  There is a special version in numbers
 * inline static uint8_t *
 * ns_numberinfo(nsEnum item, uint8_t *ndata)
 *   and
 * in strptime().
 */

/* to be moved into LC_MESSAGES? 'REPLACEMENT CHARACTER' U+FFFD */
static const uint8_t error_condition[4] = { 0xEF, 0xBF, 0xBD, 0x00 };

void *
lc_getItemPtr(lcatEnum category, lcptEnum item, locale_t ld) {

  uint8_t *data;
  if      (category == LCAT_CALENDARNAMES) {
    /* this returns tstr_st (only ".calendar") */
    return &((tstr_st *)ld->lc_time.calendar)[item];
  }
  if (category == LCAT_CALENDARFORMAT)
    data = ld->lc_time.formats;
  else if (category == LCAT_CALENDARERA) {
    if (item == LCPT_ERA_DATES)
      return (uint8_t*)ld->lc_time.era_dates;
    data = (uint8_t*)ld->lc_time.era;
  } else if (category == LCAT_NUMERIC)
    data = ld->lc_numeric.numbers;
  else if (category == LCAT_NUMERICSYSTEM)
    data = (uint8_t*)ld->lc_numeric.nsys;
  else if (category == LCAT_MONETARY)
    data = ld->lc_monetary.monetary;
  else {
    data = ld->lc_messages.messages;
    if (category != LCAT_MESSAGES)
      return (void *)error_condition;
  }
  uint16_t offset = ((uint16_t*)data)[item];
#ifdef __LITTLE_ENDIAN__
  offset = (offset >> 8) | (offset << 8);
#endif
  return (data + offset);
}

