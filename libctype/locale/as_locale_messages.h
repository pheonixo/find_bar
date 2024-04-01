/*
 *  as_locale_messages.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Mon 17 Feb 2014.
 *  Copyright (c) 2014. All rights reserved.
 *
 */
/* to house the traditional yes/no strings, but will
 * also include "more information" request, and "list" symbol.
 */

#ifndef _AS_LOCALE_MESSAGES_H_
#define _AS_LOCALE_MESSAGES_H_

#include "sys/as_types.h"

/* done the same as monetary, loads of expansion, offsets */
typedef struct {
  uint8_t *messages;
} lc_messages_st;

#endif /* !_AS_LOCALE_MESSAGES_H_ */
