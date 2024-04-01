/*
 *  as_locale_monetary.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Sat 7 Dec 2013.
 *  Copyright (c) 2013, 2014. All rights reserved.
 *
 *  Solidified on 17 Feb 2014.
 */

/* TERRITORY based. */
/* decimal, group, minusSign, plusSign belongs to LC_NUMERIC */
/* have not seen, nor know of exception where finance different then numeric */
/* there are fraction symbols, but believe those belong to scientific?
 * (mathmetical sciences) */

/* Going with all offset, no tstr_t */
/*   numericRd/numericWr:
 * Allow or set up an ability for locales that have finicial to set active using
 * NS_FINANCE just as NS_COMMON, etc. DO not reserve slot, rather on rare
 * occurances, use duplocale(), set active, pass this to financial functions.
 */
/*
 *  For consistency, use struct. This should also allow expansion with a
 * backwards compatibility.
 */

#ifndef _AS_LOCALE_MONETARY_H_
#define _AS_LOCALE_MONETARY_H_

typedef struct {
  uint8_t *monetary;
} lc_monetary_st;

#endif  /* !_AS_LOCALE_MONETARY_H_ */
