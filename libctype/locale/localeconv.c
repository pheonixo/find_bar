/*
 *  localeconv - return locale-specific information
 *  Pheonix
 *
 *  Created by Steven Abner on Sat 27 Apr 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 *  As near as I can make out, The 'International' format
 *  is the 'currency' format with the symbols swapped. The CLDR
 *  'accounting' format is not the "International' nor is it used by
 *  localeconv().
 */


#include "as_locale.h"

extern char *strchr(const char *, int);
extern char *strncpy(char *restrict, const char *restrict, size_t);
/* debugging */
extern int printf(const char *restrict, ...);

/*
 *  The members of the structure with type char * are pointers to strings, any
 * of which (except decimal_point) can point to "", to indicate that the value
 * is not available in the current locale or is of zero length. The members
 * with type char are non-negative numbers, any of which can be {CHAR_MAX} to
 * indicate that the value is not available in the current locale.
 *  The members include the following:
 * char *decimal_point
 *    The radix character used to format non-monetary quantities.
 * char *thousands_sep
 *    The character used to separate groups of digits before the decimal-point
 *    character in formatted non-monetary quantities.
 * char *grouping
 *    A string whose elements taken as one-byte integer values indicate the size
 *    of each group of digits in formatted non-monetary quantities.
 * char *int_curr_symbol
 *    The international currency symbol applicable to the current locale. The
 *    first three characters contain the alphabetic international currency
 *    symbol in accordance with those specified in the ISO 4217:1995 standard.
 *    The fourth character (immediately preceding the null byte) is the
 *    character used to separate the international currency symbol from the
 *    monetary quantity.
 * char *currency_symbol
 *    The local currency symbol applicable to the current locale.
 * char *mon_decimal_point
 *    The radix character used to format monetary quantities.
 * char *mon_thousands_sep
 *    The separator for groups of digits before the decimal-point in formatted
 *    monetary quantities.
 * char *mon_grouping
 *    A string whose elements taken as one-byte integer values indicate the size
 *    of each group of digits in formatted monetary quantities.
 * char *positive_sign
 *    The string used to indicate a non-negative valued formatted monetary
 *    quantity.
 * char *negative_sign
 *    The string used to indicate a negative valued formatted monetary quantity.
 * char int_frac_digits
 *    The number of fractional digits (those after the decimal-point) to be
 *    displayed in an internationally formatted monetary quantity.
 * char frac_digits
 *    The number of fractional digits (those after the decimal-point) to be
 *    displayed in a formatted monetary quantity.
 * char p_cs_precedes
 *    Set to 1 if the currency_symbol precedes the value for a non-negative
 *    formatted monetary quantity. Set to 0 if the symbol succeeds the value.
 * char p_sep_by_space
 *    Set to a value indicating the separation of the currency_symbol, the sign
 *    string, and the value for a non-negative formatted monetary quantity.
 * char n_cs_precedes
 *    Set to 1 if the currency_symbol precedes the value for a negative
 *    formatted monetary quantity. Set to 0 if the symbol succeeds the value.
 * char n_sep_by_space
 *    Set to a value indicating the separation of the currency_symbol, the sign
 *    string, and the value for a negative formatted monetary quantity.
 * char p_sign_posn
 *    Set to a value indicating the positioning of the positive_sign for a
 *    non-negative formatted monetary quantity.
 * char n_sign_posn
 *    Set to a value indicating the positioning of the negative_sign for a
 *    negative formatted monetary quantity.
 * char int_p_cs_precedes
 *    Set to 1 or 0 if the int_curr_symbol respectively precedes or succeeds the
 *    value for a non-negative internationally formatted monetary quantity.
 * char int_n_cs_precedes
 *    Set to 1 or 0 if the int_curr_symbol respectively precedes or succeeds the
 *    value for a negative internationally formatted monetary quantity.
 * char int_p_sep_by_space
 *    Set to a value indicating the separation of the int_curr_symbol, the sign
 *    string, and the value for a non-negative internationally formatted
 *    monetary quantity.
 * char int_n_sep_by_space
 *    Set to a value indicating the separation of the int_curr_symbol, the sign
 *    string, and the value for a negative internationally formatted monetary
 *    quantity.
 * char int_p_sign_posn
 *    Set to a value indicating the positioning of the positive_sign for a
 *    non-negative internationally formatted monetary quantity.
 * char int_n_sign_posn
 *    Set to a value indicating the positioning of the negative_sign for a
 *    negative internationally formatted monetary quantity.
 *
 *  The elements of grouping and mon_grouping are interpreted according to the
 * following:
 *    {CHAR_MAX}:
 *      No further grouping is to be performed.
 *    0:
 *      The previous element is to be repeatedly used for the remainder of the
 *      digits.
 *    other:
 *      The integer value is the number of digits that comprise the current
 *      group. The next element is examined to determine the size of the next
 *      group of digits before the current group.
 *
 *  The values of p_sep_by_space, n_sep_by_space, int_p_sep_by_space, and
 * int_n_sep_by_space are interpreted according to the following:
 *    0:
 *      No space separates the currency symbol and value.
 *    1:
 *      If the currency symbol and sign string are adjacent, a space separates
 *      them from the value; otherwise, a space separates the currency symbol
 *      from the value.
 *    2:
 *      If the currency symbol and sign string are adjacent, a space separates
 *      them; otherwise, a space separates the sign string from the value.
 *  For int_p_sep_by_space and int_n_sep_by_space, the fourth character of
 * int_curr_symbol is used instead of a space.
 *
 *  The values of p_sign_posn, n_sign_posn, int_p_sign_posn, and int_n_sign_posn
 * are interpreted according to the following:
 *    0:
 *      Parentheses surround the quantity and currency_symbol or
 *      int_curr_symbol.
 *    1:
 *      The sign string precedes the quantity and currency_symbol or
 *      int_curr_symbol.
 *    2:
 *      The sign string succeeds the quantity and currency_symbol or
 *      int_curr_symbol.
 *    3:
 *      The sign string immediately precedes the currency_symbol or
 *      int_curr_symbol.
 *    4:
 *      The sign string immediately succeeds the currency_symbol or
 *      int_curr_symbol.
 *
 *  The implementation shall behave as if no function in this volume of
 * POSIX.1-2017 calls localeconv().
 *
 *  The localeconv() function need not be thread-safe.
 *
 *        RETURN VALUE
 *  The localeconv() function shall return a pointer to the filled-in object.
 * The application shall not modify the structure to which the return value
 * points, nor any storage areas pointed to by pointers within the structure.
 * The returned pointer, and pointers within the structure, might be invalidated
 * or the structure or the storage areas might be overwritten by a subsequent
 * call to localeconv(). In addition, the returned pointer, and pointers within
 * the structure, might be invalidated or the structure or the storage areas
 * might be overwritten by subsequent calls to setlocale() with the categories
 * LC_ALL, LC_MONETARY, or LC_NUMERIC, or by calls to uselocale() which change
 * the categories LC_MONETARY or LC_NUMERIC. The returned pointer, pointers
 * within the structure, the structure, and the storage areas might also be
 * invalidated if the calling thread is terminated.
 *
 *      ERRORS
 *  No errors are defined.
 */


/*  Assume this refers to pxUserSpace locale. Which a process should have a copy
 * on which we obtain data on. This will be different then System, maybe?, and/or
 * different then any other users on the system.
 *  Fill in the static struct for return.
 */

struct lconv glconv = { 0 };

#ifdef __LITTLE_ENDIAN__
#define REORDER(x) x = ((uint8_t)(x >> 8) | (uint8_t)(x << 8))
#else
#define REORDER(x)
#endif

struct lvalues *
localevalues(struct lvalues *rlconv, locale_t locale) {

  char dparse[32];  /* decimal */
  char aparse[32];  /* accounting (international) */
  char cparse[48];  /* currency */

  char *token;/*, *xtoken; */
  uint16_t offset, idx, jdx;

  if (locale == (locale_t)0)  return NULL;

  uint8_t *gPtr = locale->lc_numeric.numbers;

    /* decimal_point, mon_decimal_point */
  offset = *(uint16_t *)&gPtr[(LCPT_SYMBOLS_DECIMAL << 1)];
  REORDER(offset);
  strncpy(rlconv->decimal_point, (char*)&gPtr[offset], (size_t)8);
  strncpy(rlconv->mon_decimal_point, (char*)&gPtr[offset], (size_t)8);

    /* thousands_sep, mon_thousands_sep */
  offset = *(uint16_t *)&gPtr[(LCPT_SYMBOLS_GROUP << 1)];
  REORDER(offset);
  strncpy(rlconv->thousands_sep, (char*)&gPtr[offset], (size_t)8);
  strncpy(rlconv->mon_thousands_sep, (char*)&gPtr[offset], (size_t)8);

    /* positive_sign, negative_sign  determine later if change to nil */
  offset = *(uint16_t *)&gPtr[(LCPT_SYMBOLS_MINUSSIGN << 1)];
  REORDER(offset);
  strncpy(rlconv->negative_sign, (char*)&gPtr[offset], (size_t)8);
  offset = *(uint16_t *)&gPtr[(LCPT_SYMBOLS_PLUSSIGN << 1)];
  REORDER(offset);
  strncpy(rlconv->positive_sign, (char*)&gPtr[offset], (size_t)8);

    /* grab formats, for grouping, mon_grouping, and non-pointer lconv data */
  offset = *(uint16_t *)&gPtr[(LCPT_DECIMALFORMAT << 1)];
  REORDER(offset);
  strncpy(dparse, (char*)&gPtr[offset], (size_t)32);
  offset = *(uint16_t *)&gPtr[(LCPT_ACCOUNTINGFORMAT << 1)];
  REORDER(offset);
  strncpy(aparse, (char*)&gPtr[offset], (size_t)32);
  offset = *(uint16_t *)&gPtr[(LCPT_CURRENCYFORMAT << 1)];
  REORDER(offset);
  strncpy(cparse, (char*)&gPtr[offset], (size_t)48);

    /* switch into monetary info */
  gPtr = locale->lc_monetary.monetary;

    /* int_curr_symbol, currency_symbol */
  offset = *(uint16_t *)&gPtr[(LCPT_ISO4217 << 1)];
  REORDER(offset);
    /* alter int_curr_symbol info when parsing */
  strncpy(rlconv->int_curr_symbol, (char*)&gPtr[offset], (size_t)8);
  offset = *(uint16_t *)&gPtr[(LCPT_CURRENCYSYMBOL << 1)];
  REORDER(offset);
  strncpy(rlconv->currency_symbol, (char*)&gPtr[offset], (size_t)8);

      /* all data to be copied from locale obtained */
  /* the signal for non-use of '+' is "" (nil) */
  *(uint64_t*)rlconv->grouping = 0;
  *(uint64_t*)rlconv->int_grouping = 0;
  *(uint64_t*)rlconv->mon_grouping = 0;


  /* handle decimal format first, 'grouping' */
  /* parse to ','. if second comma, then thats first group, else will
   * parse to '.'. */
  idx = 0, jdx = 0;
  if ((token = strchr(dparse, ',')) != NULL) {
    token++;
    while (token[(++idx)] != '.')
      if (token[idx] == ',') {
        rlconv->grouping[jdx] = idx, jdx++;
        token = &token[idx] + 1, idx = 0;
      }
  }
  rlconv->grouping[jdx] = idx;
  token = strchr(dparse, '.');
  idx = 1, jdx = 0;
  while (token[idx] != 0)
    idx++, jdx++;
  rlconv->frac_digits = jdx;

    /* no case of a '+' in any locale */
  if (strchr(aparse, '+') != NULL)
    printf("ERROR: '+' found, must redo parser at @d\n", __LINE__);
  *(uint64_t*)rlconv->positive_sign = 0;

                            /*  accounting (international) */

  char ppos[8];  *(uint64_t*)ppos = 0;
  char npos[8];  *(uint64_t*)npos = 0;
  char *posPtr = ppos;

  idx = 0, jdx = 1;
  *(uint64_t*)rlconv->bidi = 0;
  if (*aparse == '\xe2') {
    *(uint16_t*)rlconv->bidi = cconst16_t('\xe2','\x80');
    rlconv->bidi[2] = aparse[2];
    idx = 3;
  }
  while (aparse[idx] != 0) {
    if (aparse[idx] == ';')     {  posPtr = npos;          idx++; continue;  }
    if (aparse[idx] == '\xa4')  {  posPtr[0] = jdx, jdx++, idx++; continue;  }
    if (aparse[idx] == '\xa0')  {  posPtr[1] = jdx, jdx++, idx++; continue;  }
    if (aparse[idx] == '(')     {  posPtr[3] = jdx, jdx++, idx++; continue;  }
    if (aparse[idx] == '-')     {  posPtr[4] = jdx, jdx++, idx++; continue;  }
    if (aparse[idx] == ',') {
      if (*rlconv->int_grouping == 0) {
        unsigned count = 0, pdx = 0;
        do {
          if (aparse[(++idx)] == ',') {
            idx++;
            rlconv->int_grouping[pdx] = count, pdx++, count = 0;
          }
          if (aparse[idx] == '.')  break;
          count++;
        } while (1);
        rlconv->int_grouping[pdx] = count;
      } else {
        while (aparse[idx] != '.')  idx++;
      }
    }
    if (aparse[idx] == '.') {
      posPtr[2] = jdx, jdx++;
      if (posPtr == ppos) {
        unsigned count = 0;
        while (aparse[(idx + 1)] == '0') idx++, count++;
        rlconv->int_frac_digits = count;
      }
    }
    idx++;
  }
  if (posPtr == ppos)
    *(uint64_t*)npos = *(uint64_t*)ppos;
  if ((ppos[1] | npos[1]) != 0)
    *(uint16_t*)&rlconv->int_curr_symbol[3] = cconst16_t('\xc2','\xa0');
  rlconv->int_p_cs_precedes  = (char)(ppos[0] < ppos[2]);
  rlconv->int_n_cs_precedes  = (char)(npos[0] < npos[2]);
  rlconv->int_p_sep_by_space = (char)(ppos[1] != 0);
  rlconv->int_n_sep_by_space = (char)(npos[1] != 0);
  rlconv->int_p_sign_posn    = 1;  /* any number except 0, don't use parentheses */
  rlconv->int_n_sign_posn    = (char)(npos[3] == 0);
  if (npos[3] == 0) {
    if (npos[4] != 0) {
      if (npos[1] != 0)
        rlconv->int_n_sep_by_space += (char)((npos[4] < npos[1]) && (npos[2] > npos[1]));
      if (npos[4] < npos[2])
            rlconv->int_n_sign_posn  = 4;
      else  rlconv->int_n_sign_posn  = 2;
    }
  }

                                  /*  currency  */

  *(uint64_t*)ppos = 0;
  *(uint64_t*)npos = 0;
  posPtr = ppos;

  idx = 0, jdx = 1;
  if (*cparse == '\xe2') {
    *(uint16_t*)rlconv->bidi = cconst16_t('\xe2','\x80');
    rlconv->bidi[2] = cparse[2];
    idx = 3;
  }
  while (cparse[idx] != 0) {
    if (cparse[idx] == ';')     {  posPtr = npos;          idx++; continue;  }
    if (cparse[idx] == '\xa4')  {  posPtr[0] = jdx, jdx++, idx++; continue;  }
    if (cparse[idx] == '\xa0')  {  posPtr[1] = jdx, jdx++, idx++; continue;  }
    if (cparse[idx] == '(')     {  posPtr[3] = jdx, jdx++, idx++; continue;  }
    if (cparse[idx] == '-')     {  posPtr[4] = jdx, jdx++, idx++; continue;  }
    if (cparse[idx] == ',') {
      if (*rlconv->mon_grouping == 0) {
        unsigned count = 0, pdx = 0;
        do {
          if (cparse[(++idx)] == ',') {
            idx++;
            rlconv->mon_grouping[pdx] = count, pdx++, count = 0;
          }
          if (cparse[idx] == '.')  break;
          count++;
        } while (1);
        rlconv->mon_grouping[pdx] = count;
      } else {
        while (cparse[idx] != '.')  idx++;
      }
    }
    if (cparse[idx] == '.') {
      posPtr[2] = jdx, jdx++;
      if (posPtr == ppos) {
        unsigned count = 0;
        while (cparse[(idx + 1)] == '0') idx++, count++;
        rlconv->mon_frac_digits = count;
      }
    }
    idx++;
  }
  if (posPtr == ppos)
    *(uint64_t*)npos = *(uint64_t*)ppos;
  rlconv->p_cs_precedes  = (char)(ppos[0] < ppos[2]);
  rlconv->n_cs_precedes  = (char)(npos[0] < npos[2]);
  rlconv->p_sep_by_space = (char)(ppos[1] != 0);
  rlconv->n_sep_by_space = (char)(npos[1] != 0);
  rlconv->p_sign_posn    = 1;  /* any number except 0, don't use parentheses */
  rlconv->n_sign_posn    = (char)(npos[3] == 0);
  if (npos[3] == 0) {
    if (npos[4] != 0) {
      if (npos[1] != 0)
        rlconv->n_sep_by_space += (char)((npos[4] < npos[1]) && (npos[2] > npos[1]));
      if (npos[4] < npos[2])
            rlconv->n_sign_posn  = 4;
      else  rlconv->n_sign_posn  = 2;
    }
  }

  return rlconv;
}

struct lconv *
localeconv(void) {
    /* return struct lvalues */
  localevalues(&glconv._lvals, pxUserLocale);
  glconv.decimal_point     = glconv._lvals.decimal_point;
  glconv.thousands_sep     = glconv._lvals.thousands_sep;
  glconv.grouping          = glconv._lvals.grouping;
  glconv.int_curr_symbol   = glconv._lvals.int_curr_symbol;
  glconv.currency_symbol   = glconv._lvals.currency_symbol;
  glconv.mon_decimal_point = glconv._lvals.mon_decimal_point;
  glconv.mon_thousands_sep = glconv._lvals.mon_thousands_sep;
  glconv.mon_grouping      = glconv._lvals.mon_grouping;
  glconv.positive_sign     = glconv._lvals.positive_sign;
  glconv.negative_sign     = glconv._lvals.negative_sign;
  return &glconv;
}

