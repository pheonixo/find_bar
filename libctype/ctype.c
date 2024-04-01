/*
 *  ctype - classification of characters
 *  Pheonix
 *
 *  Created by Steven J Abner on Fri May 28 2004.
 *  Copyright (c) 2004, 2012, 2013, 2019, 2024. All rights reserved.
 *
 *  Dedicated to Erin JoLynn Abner.
 *  Special thanks to God!
 *
 *  Major rewrite Mon 11 Nov 2013, not yet finished on rewrite
 *
 *  2 March 24: after re-read of POSIX and need for UTF8 support,
 *  started rework on isXXX() functions. Need this to only
 *  handle sizeof(unsigned char) for isXXX() and sizeof(wint_t)
 *  for iswXXX().
 */

#include "as_ctype.h"

extern uint32_t ucs_getUCV(uint32_t);

static int
_ctypeIS(uint32_t c, locale_t ld, ctEnum r) {

  uint32_t ct_value;

  /* special handle 0x0000 properties */
  if (c == 0) {
    if (r == ct_isset)  return 1;
    return ((r & 0x1000) ? c : (int)(0x0080 & r));
  } else {

    csEnum cs_code = csNUL;
    csCnvrt_st cnvrt;
      // limit 'int' to wchar_t or Unicode character size
    uint32_t response = c;

      // Definition states:
      // "character of class" (ctEnum) "in the current locale"
    if (ld->lc_codeset.MIB != csUTF32) {
      cnvrt.cs_wr   = (void *)&response;
      cnvrt.cs_wrSz = sizeof(uint32_t);
      cnvrt.cs_rd   = (void *)&c;
      cnvrt.cs_rdSz = sizeof(uint32_t);

      cs_code = ld->lc_codeset.decoderFn(&cnvrt);
      if (((cs_code != csWN) && (cs_code > csRW))
          || (cs_code == csNUL))
        return 0;
    }
    if (response <= 0x3400)  ct_value = ucs_table[response];
    else                     ct_value = ucs_getUCV(response);
    if ((ct_value & 0x8000) == 0x8000) {
      uint32_t tc;
      tc = (r & 0x1000) ? (r & 3) : 3;
      ct_value = ucs_cases_table[(((ct_value & 0x7FFF) << 2) + tc)];
      if (tc != 3) {
          /* do not overwrite "c", may not be a part of cs */
        cnvrt.cs_wr   = (void *)&tc;
        cnvrt.cs_wrSz = sizeof(uint32_t);
        cnvrt.cs_rd   = (void *)&ct_value;
        cnvrt.cs_rdSz = 1;
          /* errors can occur, might not be part of character set */
        cs_code = ld->lc_codeset.encoderFn(&cnvrt);
          /* do not assign case change if error */
        if (cs_code <= csRW)
          c = tc;
      }
    }
  }
  /* XXXXX see "as_ctype.h" for comment on flag issues */
  return ((r & 0x1000) ? c : (ct_value & r));
}

// POSIX states 'unsigned char' but elsewhere describes 0x00-0xFF
// assume POSIX unaware that 'unsigned char' can be more than 8 bits
#define UC(x) ((uint32_t)((uint8_t)x))

int   isset_l(int c, locale_t ld)    {  return _ctypeIS(UC(c), ld, ct_isset);  }
int   isalnum_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_alnum);  }
int   isalpha_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_alpha);  }
int   isblank_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_blank);  }
int   iscmark_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_cmark);  }
int   iscntrl_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_cntrl);  }
int   isdigit_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_digit);  }
int   isgraph_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_graph);  }
int   isletter_l(int c, locale_t ld) {  return _ctypeIS(UC(c), ld, ct_letter); }
int   islower_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_lower);  }
int   isprint_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_print);  }
int   ispunct_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_punct);  }
int   isspace_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_space);  }
int   isupper_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_upper);  }
int   isxdigit_l(int c, locale_t ld) {  return _ctypeIS(UC(c), ld, ct_xdigit); }
int   tolower_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_cnvtl);  }
int   toupper_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_cnvtu);  }
int   totitle_l(int c, locale_t ld)  {  return _ctypeIS(UC(c), ld, ct_cnvtt);  }

#undef UC

int   isset(int c)    {  return isset_l(c, pxUserLocale);    }
int   isalnum(int c)  {  return isalnum_l(c, pxUserLocale);  }
int   isalpha(int c)  {  return isalpha_l(c, pxUserLocale);  }
int   isblank(int c)  {  return isblank_l(c, pxUserLocale);  }
int   iscmark(int c)  {  return iscmark_l(c, pxUserLocale);  }
int   iscntrl(int c)  {  return iscntrl_l(c, pxUserLocale);  }
int   isdigit(int c)  {  return isdigit_l(c, pxUserLocale);  }
int   isgraph(int c)  {  return isgraph_l(c, pxUserLocale);  }
int   isletter(int c) {  return isletter_l(c, pxUserLocale); }
int   islower(int c)  {  return islower_l(c, pxUserLocale);  }
int   isprint(int c)  {  return isprint_l(c, pxUserLocale);  }
int   ispunct(int c)  {  return ispunct_l(c, pxUserLocale);  }
int   isspace(int c)  {  return isspace_l(c, pxUserLocale);  }
int   isupper(int c)  {  return isupper_l(c, pxUserLocale);  }
int   isxdigit(int c) {  return isxdigit_l(c, pxUserLocale); }
int   tolower(int c)  {  return tolower_l(c, pxUserLocale);  }
int   toupper(int c)  {  return toupper_l(c, pxUserLocale);  }
int   totitle(int c)  {  return totitle_l(c, pxUserLocale);  }

int   iswset_l(wint_t c, locale_t ld)    {  return _ctypeIS(c, ld, ct_isset);  }
int   iswalnum_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_alnum);  }
int   iswalpha_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_alpha);  }
int   iswblank_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_blank);  }
int   iswcmark_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_cmark);  }
int   iswcntrl_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_cntrl);  }
int   iswdigit_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_digit);  }
int   iswgraph_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_graph);  }
int   iswletter_l(wint_t c, locale_t ld) {  return _ctypeIS(c, ld, ct_letter); }
int   iswlower_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_lower);  }
int   iswprint_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_print);  }
int   iswpunct_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_punct);  }
int   iswspace_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_space);  }
int   iswupper_l(wint_t c, locale_t ld)  {  return _ctypeIS(c, ld, ct_upper);  }
int   iswxdigit_l(wint_t c, locale_t ld) {  return _ctypeIS(c, ld, ct_xdigit); }
wint_t  towlower_l(wint_t c, locale_t ld) {  return _ctypeIS(c, ld, ct_cnvtl); }
wint_t  towupper_l(wint_t c, locale_t ld) {  return _ctypeIS(c, ld, ct_cnvtu); }
wint_t  towtitle_l(wint_t c, locale_t ld) {  return _ctypeIS(c, ld, ct_cnvtt); }

int   iswset(wint_t c)      {  return iswset_l(c, pxUserLocale);    }
int   iswalnum(wint_t c)    {  return iswalnum_l(c, pxUserLocale);  }
int   iswalpha(wint_t c)    {  return iswalpha_l(c, pxUserLocale);  }
int   iswblank(wint_t c)    {  return iswblank_l(c, pxUserLocale);  }
int   iswcmark(wint_t c)    {  return iswcmark_l(c, pxUserLocale);  }
int   iswcntrl(wint_t c)    {  return iswcntrl_l(c, pxUserLocale);  }
int   iswdigit(wint_t c)    {  return iswdigit_l(c, pxUserLocale);  }
int   iswgraph(wint_t c)    {  return iswgraph_l(c, pxUserLocale);  }
int   iswletter(wint_t c)   {  return iswletter_l(c, pxUserLocale); }
int   iswlower(wint_t c)    {  return iswlower_l(c, pxUserLocale);  }
int   iswprint(wint_t c)    {  return iswprint_l(c, pxUserLocale);  }
int   iswpunct(wint_t c)    {  return iswpunct_l(c, pxUserLocale);  }
int   iswspace(wint_t c)    {  return iswspace_l(c, pxUserLocale);  }
int   iswupper(wint_t c)    {  return iswupper_l(c, pxUserLocale);  }
int   iswxdigit(wint_t c)   {  return iswxdigit_l(c, pxUserLocale); }
wint_t  towlower(wint_t c)  {  return towlower_l(c, pxUserLocale);  }
wint_t  towupper(wint_t c)  {  return towupper_l(c, pxUserLocale);  }
wint_t  towtitle(wint_t c)  {  return towtitle_l(c, pxUserLocale);  }
