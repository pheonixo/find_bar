/*
 *  locale.h - category macros
 *  Pheonix
 *
 *  Created by Steven Abner on 2/2/11.
 *  Copyright (c) 2011, 2012. All rights reserved.
 *
 */
/* the new way thread safe way is:
 new_locale = duplocale(usrloc);
 alter new_locale.
 now pass to any *_l function or if wish to use as default uselocale(new_locale).
 if uselocale() called, the new_locale is duplicated and retained as the user's
 process any call to usrloc will get this data, earlier calls prior to
 uselocale() still have valid pointers/data to old usrloc

 This to be part of environment, (stdlib/environment), system sets system_locale,
 when passes to login user, user's locale loads (becomes part of process' info),
 on fork() the new process gets a copy of parent. This is to be unlike getenv()
 in that getenv() is a communication setup where a child sets a value, all
 parents can see this value is set, yet any prior calls by parent with pointers
 are still valid pointers, but a new call will get a different pointer.
 locale will change only by the process itself, child A can change its copy,
 child B can change its copy, parent has no knowledge of A or B, B cant see A,
 A cant see B. if want to communicate changes, you pass locales to each, or set
 an env variable, use getenv(variable) in a different thread, alter the getenv()
 callers locale or if not to pass onto newly spawned children, use getenv()s
 data privately.

 */
/*
 *   OPINION: no need for per-thread environment locales. This would protect
 * pxUserLocale, but I believe a static copy of pxUserLocale would be best for
 * programmer abuse purposes. This way programmers can still be lazy and not
 * create a locale for thier threads.
 */

#ifndef _AS_LOCALE_H_
#define _AS_LOCALE_H_

#include "sys/as_types.h"    /* defines NULL */

/* When using CLDR:
 * 'international' only differs form 'monetary' by symbols.
 * So to make useful information, the 'int_' represents finanical or
 * accounting format. To fully accommodate a few members were added to
 * the lconv structure.
 *  Note: char * == char [], needed for thread-safe version
 * Also it's just the right way to do it!
 */

/* Issue: when using FILE unit without Pheonix' locale unit,
 * can't create binary compatibity with char* == char[], even when
 * 64bit pointers. So set up a POSIX struct lconv which uses new
 * system. Then can access new system directly, or with systems (embedded type)
 * can use new without worry of compatibity.
 * For linux, when not using CTYPE unit, need old 'lconv', with unit needed
 * new definition. */

struct lvalues {
  char    decimal_point[8];
  char    mon_decimal_point[8];
  char    thousands_sep[8];
  char    mon_thousands_sep[8];
  char    grouping[8];
  char    int_grouping[8];
  char    mon_grouping[8];
  char    int_curr_symbol[8];
  char    currency_symbol[8];
  char    positive_sign[8];
  char    negative_sign[8];
  char    bidi[8];
  char    frac_digits;
  char    int_frac_digits;
  char    mon_frac_digits;
  char    p_cs_precedes;
  char    n_cs_precedes;
  char    p_sep_by_space;
  char    n_sep_by_space;
  char    p_sign_posn;
  char    n_sign_posn;
  char    int_p_cs_precedes;
  char    int_n_cs_precedes;
  char    int_p_sep_by_space;
  char    int_n_sep_by_space;
  char    int_p_sign_posn;
  char    int_n_sign_posn;
};

struct lconv {
  char    *decimal_point;
  char    *thousands_sep;
  char    *grouping;
  char    *int_curr_symbol;
  char    *currency_symbol;
  char    *mon_decimal_point;
  char    *mon_thousands_sep;
  char    *mon_grouping;
  char    *positive_sign;
  char    *negative_sign;
  char    int_frac_digits;
  char    frac_digits;
  char    p_cs_precedes;
  char    p_sep_by_space;
  char    n_cs_precedes;
  char    n_sep_by_space;
  char    p_sign_posn;
  char    n_sign_posn;
  char    int_p_cs_precedes;
  char    int_p_sep_by_space;
  char    int_n_cs_precedes;
  char    int_n_sep_by_space;
  char    int_p_sign_posn;
  char    int_n_sign_posn;
  struct  lvalues _lvals;
};

    /* macros for setlocale() function (almost all set like this) */
#define LC_ALL                    0
#define LC_COLLATE                1
#define LC_CTYPE                  2
#define LC_CODESET                2
#define LC_MONETARY               3
#define LC_NUMERIC                4
#define LC_TIME                   5
#define LC_MESSAGES               6

/* convenience marco, moved from lc_numeric, only used by locale.c */
/* normally "active" is set to common, Macros for setting active */
#define LC_NSCOMMON             0000100   /* also considered default */
#define LC_NSNATIVE             0000200
#define LC_NSTRADITIONAL        0000300
#define LC_NSFINANCIAL          0000400
/* ability to change eras, currently only 4 solar, with same base as gregorian */
#define LC_ERAGREG              0001000   /* also considered default */
#define LC_ERATSOL              0002000
#define LC_ERAMING              0003000
#define LC_ERAJPYR              0004000

/* since LC_CTYPE is unused, replace by LC_CODESET */
    /* macros for newlocale() function */
#define LC_COLLATE_MASK         0000001
#define LC_CODESET_MASK         0000002
#define LC_CTYPE_MASK           0000002
#define LC_MESSAGES_MASK        0000004
#define LC_MONETARY_MASK        0000010
#define LC_NUMERIC_MASK         0000020
#define LC_TIME_MASK            0000040
/* use LC_NS* for setting active number system */
/* can affect symbols, some locales change with a given system */
/* common-financial not actually a  'mask', 1 of 4 systems */
#define LC_NSCOMMON_MASK        LC_NSCOMMON
#define LC_NSNATIVE_MASK        LC_NSNATIVE
#define LC_NSTRADITIONAL_MASK   LC_NSTRADITIONAL
#define LC_NSFINANCIAL_MASK     LC_NSFINANCIAL
#define LC_ERAS_MASK            0007000
#define LC_NSRW_MASK            0000700
#define LC_ALL_MASK             0000077

/* access, create, alter to locale members */
            /* category */
typedef enum {
  LCAT_CALENDARNAMES   = 00000000,
  LCAT_CALENDARFORMAT  = 00000001,
  LCAT_CALENDARERA     = 00000002,
  LCAT_NUMERIC         = 00000004,
  LCAT_NUMERICSYSTEM   = 00000010,
  LCAT_MONETARY        = 00000020,
  LCAT_MESSAGES        = 00000040
} lcatEnum;

/* can be used as indicies, rearranging breaks backwards */
          /* components */
typedef enum {
    /* all categories, 0 is AllocSz */
  LCPT_ALLOCSZ        = 0,
    /* LCAT_CALENDAR_NAMES */
  LCPT_ABDAY,
  LCPT_DAY,
  LCPT_ABMON,
  LCPT_MON,
  LCPT_AMPM,
    /* LCAT_CALENDAR_FORMATS */
  LCPT_FMT_DT         = 1,
  LCPT_FMT_D,
  LCPT_FMT_T,
  LCPT_FMT_AMPM,
  LCPT_FMT_EDT,
  LCPT_FMT_ED,
  LCPT_FMT_ET,
  LCPT_FMT_EY,
    /* LCAT_CALENDAR_ERAS */
    /* NOTE: names behave like TZ, belongs to struct */
  LCPT_ERA_DATES     = 0,
  LCPT_ERA_ABNAME,
  LCPT_ERA_NAME,
    /* LCAT_NUMERIC_SYSTEMS */
  LCPT_RW_ACTIVE      = 1,
  LCPT_RW_DEFAULT     = 1,
  LCPT_RW_NATIVE,
  LCPT_RW_TRADITIONAL,
  LCPT_RW_FINICIAL,
    /* LCAT_NUMERIC_FORMATS */
  LCPT_DECIMALFORMAT  = 1,
  LCPT_PERCENTFORMAT,
  LCPT_SCIENTIFICFORMAT,
  LCPT_ACCOUNTINGFORMAT,
  LCPT_CURRENCYFORMAT,
    /* LCAT_NUMERIC_SYMBOLS */
  LCPT_SYMBOLS_DECIMAL,
  LCPT_SYMBOLS_EXPONENTIAL,
  LCPT_SYMBOLS_GROUP,
  LCPT_SYMBOLS_INFINITY,
  LCPT_SYMBOLS_MINUSSIGN,
  LCPT_SYMBOLS_NAN,
  LCPT_SYMBOLS_PERMILLE,
  LCPT_SYMBOLS_PERCENTSIGN,
  LCPT_SYMBOLS_PLUSSIGN,
  LCPT_SYMBOLS_SUPERSCRIPTINGEXPONENT,
  LCPT_DECIMAL        = 0,
  LCPT_EXPONENTIAL,
  LCPT_GROUP,
  LCPT_INFINITY,
  LCPT_MINUSSIGN,
  LCPT_NAN,
  LCPT_PERMILLE,
  LCPT_PERCENTSIGN,
  LCPT_PLUSSIGN,
  LCPT_SUPERSCRIPTINGEXPONENT,
  LCPT_NUMERIC_LAST   = LCPT_SUPERSCRIPTINGEXPONENT,
    /* LCAT_MONETARY */
  LCPT_ISO4217      = 1,/* (code, "USD") */
  LCPT_CURRENYSIGN,     /* (sign, territorial code, "US$") */
  LCPT_CURRENCYSYMBOL,  /* (absign, territorial shorthand "$") */
  LCPT_DISPLAYNAME,     /* ("United States Dollar") */
  LCPT_SINGULAR,        /* ("dollar") */
  LCPT_PLURAL,          /* ("dollars") */
    /* LCAT_MESSAGES */
  LCPT_YSTR           = 1,
  LCPT_YABR,
  LCPT_NSTR,
  LCPT_NABR,
  LCPT_MOREINFO,
  LCPT_ERROR
} lcptEnum;

/* POSIX.1-2008 */
/* Define the locale_t type, representing a locale object. */
#include "locale/as_locale_codeset.h"  /* info on lc_codeset_st */
#include "locale/as_locale_messages.h" /* info on lc_messages_st */
#include "locale/as_locale_numeric.h"  /* info on lc_numeric_st */
#include "locale/as_locale_monetary.h" /* info on lc_monetary_st */
#include "locale/as_locale_time.h"     /* info on lc_time_st */

/* no LC_TYPE unknown exceptions per locale? */
/* LC_COLLATE => exceptions listings or functionPtrs? */

/* Reminder, each user including system admin has own locale
 (static or dynamic on load). As for per thread, not sure why it would
 be needed. Either dup and use or if to change parent so be it. */

/* since name strings are debug only, moved to ID. Create ID to
 * string or let besearch() handle.
 * reduces > 128 byte to 8 bytes (per locale). Increases binary storage by
 * 497 locales * 8 bytes = <4K.
 */
/* if altered, only affects pxUser, fakeLd in strptime atm. */
typedef struct {
  uint64_t  lc_ident;
  lc_codeset_st   lc_codeset;
  /*lc_collate_st   lc_collate;*/
  lc_messages_st  lc_messages;
  lc_numeric_st   lc_numeric;
  lc_monetary_st  lc_monetary;
  lc_time_st      lc_time;
} locale_st, *locale_t;  /*(981K 30 Jan 2014)(1214 17 Feb)*/

/* make global?, have to... POSIX requires  */
extern locale_t pxUserLocale;
/* Define LC_GLOBAL_LOCALE, a special locale object descriptor used by the
 * duplocale() and uselocale() functions */
#define LC_GLOBAL_LOCALE pxUserLocale

/* used for return of localeconv, shows unthread-safe */
/* passed as argument to thread-safe version */
extern struct lconv glconv;

#ifdef __cplusplus
extern "C" {
#endif

locale_t        duplocale(locale_t);                              __standard__(POSIX.1-2008)
void            freelocale(locale_t);                             __standard__(POSIX.1-2008)
  /* localeconv_r(glconv, pxUserLocale) */
struct lconv *  localeconv(void);
struct lvalues *localevalues(struct lvalues *, locale_t);         __standard__(Pheonix)
locale_t        newlocale(int, const char *, locale_t);           __standard__(POSIX.1-2008)
char *          setlocale(int, const char *);
locale_t        uselocale(locale_t);                              __standard__(POSIX.1-2008)

/* replacement and/or wrappers of old functions */
int             localenew(locale_t *, const char *);              __standard__(Pheonix)
int             localemodify(locale_t, const char *, int);        __standard__(Pheonix)

/* PheonixOS function, newlocale() is wrapper to this */
int             getlocale(locale_t *, const char *, int);         __standard__(Pheonix)
/* Accessor functions, array/non-array */
/* Array currently only LCAT_CALENDARNAMES */
void *          lc_getIndexItemPtr(lcatEnum, lcptEnum, locale_t, uint32_t); __standard__(Pheonix)
void *          lc_getItemPtr(lcatEnum, lcptEnum, locale_t);      __standard__(Pheonix)

  /* NEED and want a locale modifier function. This function would
   * allow changes to symbols, time strings, etc. It must alter a
   * locale by allocation of extra memory to store the const data
   * of a locale. It must also change names, user-defined is beyond
   * 8 char allotment, think of great name.
   *  Intended use is applications storing modified data to preferences.
   * Creation of new locales from an existing one should be easy.
   */

#ifdef __cplusplus
}
#endif

#endif	/* !_AS_LOCALE_H_ */
