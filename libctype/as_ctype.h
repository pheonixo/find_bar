/*
 *  ctype.h - character types
 *  PheonixOS
 *
 *  Created by Steven J Abner on Thu May 27 2004.
 *  Copyright (c) 2004, 2012, 2019, 2024. All rights reserved.
 *
 */

/* switch to smaller, old "ctype" for embedded systems */
/* #define EMBEDDED */

#ifndef _AS_CTYPE_H_
#define _AS_CTYPE_H_

#include "as_locale.h"  /* includes sys/as_types.h */

extern const uint16_t ucs_table[];
extern const uint32_t ucs_cases_table[];

typedef enum {
  ct_nopcs  = 0x0000,  /* non-portable character set, no character */
  ct_upper  = 0x0001,
  ct_lower  = 0x0002,
  ct_title  = 0x0004,
  ct_letter = 0x0008,
  ct_digit  = 0x0010,
  ct_punct  = 0x0020,
  ct_space  = 0x0040,
  ct_cntrl  = 0x0080,
  ct_xdigit = 0x0100,
  ct_blank  = 0x0200,
  ct_print  = 0x0400,
  ct_cmark  = 0x0800,
  ct_alpha  = 0x000f,  /* upper | lower | title | letter */
  ct_alnum  = 0x001f,  /* upper | lower | title | letter | digit */
  ct_graph  = 0x003f,  /* upper | lower | title | letter | digit | punct */
  /* these are used to access secondary array data */
/*XXXX redo tables so values align with ORing */
  ct_cnvtt  = 0x1000,  /* convert to title */
  ct_cnvtl  = 0x1001,  /* convert to lower */
  ct_cnvtu  = 0x1002,  /* convert to upper */
  /*ct_cnvtc  = 0x1003  //  convert to ctype properties */
  ct_isset  = 0x1003
} ctEnum;

#ifdef __cplusplus
extern "C" {
#endif

int             isalnum(int);   /* alphanumeric character 30-39,41-5A,61-7A */
int             isalnum_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isalpha(int);   /* alphabetic character 41-5A,61-7A */
int             isalpha_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isblank(int);   /* blank character 9,20 */
int             isblank_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             iscntrl(int);   /* control character 0-1F,7F */
int             iscntrl_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isdigit(int);   /* decimal digit 30-39 */
int             isdigit_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isgraph(int);   /* visible character */
int             isgraph_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             islower(int);   /* lowercase letter 61-7A */
int             islower_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isprint(int);   /* printable character */
int             isprint_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             ispunct(int);   /* punctuation character 21-2F,3A-40,5B-60,7B-7E */
int             ispunct_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isspace(int);   /* white-space character 9-D,20 */
int             isspace_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isupper(int);   /* uppercase letter 41-5A  */
int             isupper_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             isxdigit(int);	/* hexadecimal digit 30-39,41-46,61-66 */
int             isxdigit_l(int, locale_t);                        __standard__(POSIX.1-2008)
      /* translation */
int             tolower(int);
int             tolower_l(int, locale_t);                         __standard__(POSIX.1-2008)
int             toupper(int);
int             toupper_l(int, locale_t);                         __standard__(POSIX.1-2008)

      /* Pheonix added */
int             iscmark(int);                                     __standard__(Pheonix)
int             iscmark_l(int, locale_t);                         __standard__(Pheonix)
int             isletter(int);                                    __standard__(Pheonix)
int             isletter_l(int, locale_t);                        __standard__(Pheonix)
int             isset(int);                                       __standard__(Pheonix)
int             isset_l(int, locale_t);                           __standard__(Pheonix)
int             totitle(int);                                     __standard__(Pheonix)
int             totitle_l(int, locale_t);                         __standard__(Pheonix)

      /* Obsolescent */
#define isascii(c)      (int)((unsigned)(c) <= 0177)
#define toascii(c)      ((c) & 0x7F)
#define _tolower(c)     tolower(c)
#define _toupper(c)     toupper(c)

#ifndef _AS_WCTYPE_CTYPE_H_
#define _AS_WCTYPE_CTYPE_H_

#ifndef _WINT_T
#define _WINT_T
typedef wchar_t  wint_t;
#endif

int             iswalnum(wint_t);
int             iswalnum_l(wint_t, locale_t);
int             iswalpha(wint_t);
int             iswalpha_l(wint_t, locale_t);
int             iswblank(wint_t);
int             iswblank_l(wint_t, locale_t);
int             iswcntrl(wint_t);
int             iswcntrl_l(wint_t, locale_t);
int             iswdigit(wint_t);
int             iswdigit_l(wint_t, locale_t);
int             iswgraph(wint_t);
int             iswgraph_l(wint_t, locale_t);
int             iswlower(wint_t);
int             iswlower_l(wint_t, locale_t);
int             iswprint(wint_t);
int             iswprint_l(wint_t, locale_t);
int             iswpunct(wint_t);
int             iswpunct_l(wint_t, locale_t);
int             iswspace(wint_t);
int             iswspace_l(wint_t, locale_t);
int             iswupper(wint_t);
int             iswupper_l(wint_t, locale_t);
int             iswxdigit(wint_t);
int             iswxdigit_l(wint_t, locale_t);
      /* translation */
wint_t          towlower(wint_t);
wint_t          towlower_l(wint_t, locale_t);
wint_t          towupper(wint_t);
wint_t          towupper_l(wint_t, locale_t);

      /* Pheonix added, match ctype */
int             iswset(wint_t);
int             iswset_l(wint_t, locale_t);
int             iswcmark(wint_t);
int             iswcmark_l(wint_t, locale_t);
int             iswletter(wint_t);
int             iswletter_l(wint_t, locale_t);
wint_t          towtitle(wint_t);
wint_t          towtitle_l(wint_t, locale_t);

#endif /* !_AS_WCTYPE_CTYPE_H_ */

#ifdef __cplusplus
}

#endif
#endif /* !_AS_CTYPE_H_ */
