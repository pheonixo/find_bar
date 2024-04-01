/*
 *  pxlocale.c
 *  Pheonix
 *
 *  Created by Steven Abner on Mon 16 Sep 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 */

#include "as_locale.h"

extern csEnum latnWr(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags);
extern csEnum latnRd(csCnvrt_st *cnvrt, uint8_t *ns, uint32_t *rflags);
extern const uint8_t *__latn_default_messages;
extern const uint8_t *__latn_default_nsys;
extern const uint8_t *__am_ET_latn_numbers;
extern const uint8_t *__en_US_monetary;
extern const uint8_t *__en_150_calendar;
extern const uint8_t *__en_AG_formats;
extern const uint8_t *__en_001_greg_era;
extern const uint8_t *__greg_date;

  /* en_US */
locale_st _pxUserLocale = {
  0x14E01F740000000ULL,
  { csUTF8,                    /* lc_codeset */
    UStrtoUTF8Str,
    UTF8StrtoUStr
  },
  {
    (uint8_t *)&__latn_default_messages
  },
  {
    (uint8_t *)&__latn_default_nsys,
    (uint8_t *)&__am_ET_latn_numbers,
    (uint8_t *)&__am_ET_latn_numbers,
    RW_LATN,
    latnRd,
    latnWr,
    (uint8_t *)&__am_ET_latn_numbers
  },
  {
    (uint8_t *)&__en_US_monetary
  },
  {
    (uint8_t *)&__en_150_calendar,
    (uint8_t *)&__en_AG_formats,
    (uint8_t *)&__en_001_greg_era,
    (uint8_t *)&__greg_date
  }
};

locale_t pxUserLocale = (locale_t)&_pxUserLocale;

/* Japanese years, when user uses %J, check here if loaded information.
 * Should it not be NULL, then loaded.
 * This assumes, if a User utilizes %J, that the User will so again and that
 * it should remain until log out of User.
 */

