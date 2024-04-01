/*
 *  as_locale_time.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Fri 1 Nov 2013.
 *  Copyright (c) 2013. All rights reserved.
 *
 */

/*
 *  NOTE: POSIX keyword "alt_digits" must become a rountine that
 * accesses lc_numeric, determines what the "alt-digits" are. This
 * is only if backwards compatiability desired.
 *  It should be relabelled "alt-calendars", but will leave as empty string.
 * The "O" modifier of conversion specifier will function, if there is a
 * alternate number system defined. If a locale defines only their script
 * number system, alternate will be latn. Still need to work out details
 * for a 3 number system switching, and alternate calendars.
 *
 * abandoned alt_digits.
 */


#ifndef _AS_LOCALE_TIME_H_
#define _AS_LOCALE_TIME_H_

#include "sys/as_types.h"    /* defines NULL */

typedef struct {
  uint16_t  increment; /* for array type, bytes / string */
  uint16_t  offset;    /* add to tstr_st to get to begining of string(s) */
} tstr_st;

/* POSIX does not specify, PheonixOS specifies:
 * "The era, corresponding to the %EC conversion specification, shall contain
 * isalpha() only characters"
 */
/* In strftime() and in strptime():
 * POSIX also creates a problematic issue. PheonixOS resolves this isse.
 * the words "offset from %EC (year only)" means "the year of the named
 * era (%EC)".
 */

/* ICU provides full, long, medium, short */
/* POSIX locale must get reworked by hand */

/*
 * d_t_fmt:     Standard - Date & Time Combination Formats - full
 * d_fmt:       Standard - Date Formats - short (ISO8601-date style)
 * t_fmt:       Standard - Time Formats - long (ISO8601-time-extended)
 * am_pm:       Day Periods - wide - Formatting Context - am_pm
 * t_fmt_ampm:  Standard - Time Formats - short
 *   of the "era" keywords d_fmt, t_fmt, d_t_fmt are only const char *
 *   era is a set of struct types, but here used as era system name
 *   alt_digits are suppose to be enumerated, here a numbering system name
 *   era_t_format default is "" (%EX = 'ET'HHMM), time based on a formula
 * era:         (Enumeration type) Returned values from data set
 * era_d_fmt:   Flexible - Date Formats - Gy
 * era_t_fmt:   uses zzzz for 'ET'HHMM or longitude for exact 'ET'
 * era_d_t_fmt: Combination Flexible Formats (various)
 *   (various): Standard - Date & Time Combination Formats - medium
 *            + Flexible - Date Formats	- GyMMMEd
 *            + zzzz - 'ET'HHMM... EarthTime
 * alt_digits:  (Enumeration type) Alternate Numbering System, returned values
 *              from a sequential or algorithmic system of numbers.
 *
 *    Examples:
 * d_t_fmt(%c):    "{1} {0}" "y MMMM d, EEEE HH:mm:ss zzzz"
 * d_fmt(%x):      "y-MM-dd"
 * t_fmt(%X):      "HH:mm:ss z"
 * am_pm(%p):      "AM" "PM"
 * t_fmt_ampm(%r): "hh:mm a" or "HH:mm"
 *
 *    Defines:
 * y     - full year                               (%Y) or (%#Y for eras)
 * yy    - last two digits of year                 (%y)
 * M     - single digit months [1-12]              (%#m)
 * MM    - two digit months [01-12]                (%m)
 * MMM   - abbreviated context months [Jan-Dec]    (%b)
 * MMMM  - full context months [January-December]  (%B)
 * d     - single digit days [1-7]                 (%#d)
 * dd    - two digit days [01-07]                  (%d)
 * EEE   - abbreviated days [Sun-Sat]              (%a)
 * EEEE  - full context days [Sunday-Saturday]     (%A)
 * H/h   - single digit hours (24hr/12hr) [0-23/1-12] (%#H/%#I)
 * HH/hh - two digit hours (24hr/12hr) [00-23/01-12]  (%H/%I)
 * m     - single digit minutes [0-59]                (%#M, %2#M) context?
 * mm    - two digit minutes [00-59]                  (%M)
 * s     - single digit seconds [0-60]                (%2#S)
 * ss    - two digit seconds [00-60]                  (%S)
 * z     - abbreviated timezone [GMT, EDT, AEST]      (%Z)
 * zzzz  - Coordinated Time Universal ['UTC'±HHMM]    (%z)... issue larger fields
 * tttt  - Earth Time ['ET'HHMM]
 *     
 *    Eras:
 * era   - gregorian era (Christian Era), Saka Era, Nengō Eras, etc
 * alt_digits - (Japan) default/native = latn (latin),
 *              can use traditional = jpan (japanese)
 *            - (arab) default/native = arab (Eastern Arabic),
 *              can use latn (latin)
 * era_d_fmt: equalivant to %Ey%Ea if not a defined locale sequence (simular to %EY)
 * era_t_fmt: %EX, if not defined use Earthtime, as follows:  (zone Y)
 *            akin to forced 24Hr zzzz, all positive, time starts at
 *            antimeridian or 180th meridian, earth day 0 - 23:59:59
 *            timezone HHmmss [000000-265960], no prior day like UTC
 *            and bonus: for anti-Christian, anti-Western based on 180th meridian
 *            UTC +12 = 24 ET
 *            UTC -12 = 0 ET
 *            formula: UTCx, UTC+12 = 12 -   12 == 24 -  0 = 24 24 hrs of day occurred
 *            formula: UTCx, UTC-12 = 12 -  -12 == 24 - 24 = 0   0 hrs of day occurred
 *            formula: UTCx, UTC+11 = 12 -   11 == 24 -  1 = 23 23 hrs of day occurred
 *            formula: UTCx, UTC+0  = 12 -    0 == 24 - 12 = 12 12 hrs of day occurred
 *            formula: UTCx, UTC-5  = 12 - (-5) == 24 - 17 = 7   7 hrs of day occurred
 *            formula: UTCx, UTC-11 = 12 -(-11) == 24 - 23 = 1   1 hr  of day occurred
 *            formula: UTCx, UTC+14 = 12 -  14) == 24 - -2 = 26 26 hrs of day occurred
 *  longitudinal (exact): 'ET' = 0 @ 180°, counter-clockwise increases age of day
 *            sun @ 0° = 12 ET hours have occurred
 * era_d_t_fmt:
 *            GyMMMEd + 'ET'HHMM
 *            %EC%Ey%a%b%e'ET'%H%M where ET normalized 0-24, 26 => add day - 24hrs
 *            Do Not Add a day, era_t has no day
 *
 *    Eras (strf/ptime notations):
 *
 * %Ea = abbreviated Era Name
 * %EA = full Era Name
 * %Ey = year of the Era (no preceeding <zeroes>)
 * %EY = year and Era name in locale's sequence (era_format)
 * %Ec = era_d_t_format
 * %EC = %Ea
 * %Ex = era_d_fmt
 * %EX = era_t_fmt if default ("") uses formula based on timezone info
 * %O  = alt_digit uses: if "default" different than "native" uses default
 *                       if "default" equals "native" and no traditional, uses latin
 *
 */

/* calendar: header=>allocsz+tstr_st; varied arrays of strings */
/* formats:  header=>allocsz+offsets; varied length strings */
/* era:      header=>allocsz+offsets; varied length strings + struct */

/* era header: allocsz + offset(name1) + struct offset;
 * end of struct begins
struct _era {
  int32_t  year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  yr_offset;
  uint8_t  yr_direction;  // 0 == increases as moves away, 1 == decreases
  uint16_t next_struct;
  uint16_t abera;
 // data for era name
};
 */

/* what data will parse into for era code handling */
/* data stored year:[[month:op][day]]:year_offset:abidx:idx (1-32,4-16) */
typedef struct {
  int32_t   year;
  uint8_t   month;
  uint8_t   day;
  uint16_t  yr_offset;
  uint8_t   yr_operation;  /* 0 == increases as moves away, 1 == decreases, 2,3 reserved */
  uint16_t  abera_idx;
  uint16_t  era_idx;
} edate_st;

/*
struct _era {
  edate_st *era_dates;
  uint8_t  *era_names;
};
*/

/* when calendar changes, era should, fmts maybe */
/* era should be seperated, what if others don't wish to support eras (embedded) */
/* essential is calendar, format along with base strf/ptime should be butchered */
/* Japanese years is an offical means of displaying years. Japanese years should
 * be NULL and allocated on request of %J. For static embedded system, use NULL
 * but on request use const pointer which can't be freed.
 */
/* altering due to Japanese era names and dates. This also yields smaller
 * code size. */
typedef struct {
  uint8_t *calendar;    /* Names of months, days, am/pm */
  uint8_t *formats;     /* Display of time, ex: "%m/%d/%y" */
  uint8_t *era;         /* Names of an era, 1 of 4 set, default gregorian */
  uint8_t *era_dates;   /* Dates to match occurances of above */
/*  uint8_t *era_names; */
} lc_time_st;

extern const uint8_t *__tsol_date;
extern const uint8_t *__greg_date;
extern const uint8_t *__roc_date;
extern const uint8_t *__jpyr_date;

#endif	/* !_AS_LOCALE_TIME_H_ */
