/*
 *  as_locale_codeset.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Fri 1 Nov 2013.
 *  Copyright (c) 2013. All rights reserved.
 *
 */

/* codeset is either at device driver level or middle man between device driver
 * and PheonixOS functions. Goes at the "read", "write" of 'files'. UTF-8 used
 * internally. Keyboard read in, or converted to UTF-8. Display 'file' converted
 * from UTF-8 to whatever set up of display driver. Hard-drive and the like
 * get converted to codeset (assume user porting to another system that only
 * works in that codeset). Could actually store UTF-8 on fixed drives, codeset
 * on external or plugable drives.
 */


/* issue: current design didn't work as I wanted.
 * Wanted user to:
 *  inputs: codeset stream (really should have been a void * ? see below) with size in terms of "char" units
 *  inputs: unicode stream (always of size uint32_t with size_t in terms of code uints (32bit count))
 *
 *  code set stream is char * because code set stream and size could be wide chars
 *  which I believe at this point is a compiler setting of "char" defined as uint16_t
 *
 *  outputs: return was pointer to next read or write location of code set stream.
 *           from this one could continue work on locales code set and determine
 *           strlen of code set stream read/written. Error was returned by
 *           either no pointer advancement or unicode stream pointer pointing
 *           to error code indicator value.
 *
 *  design:  locale to locale is locale to unicode, work in system in unicode terms,
 *           then output in other/same locale's usage.
 *           A void to void shouldn't work as this becomes a specific use, non-inclusive.
 *           Code set's stream lengths read or written was pointer subtraction.
 *           Stream endpoints are to be returned for any operation needed, such
 *           as error handling, stream merging.
 *
 *  issue:   (One). Error indicator was only on code set to unicode, but user
 *           does not receive a last write position of unicode.
 *           (Two). Wanted both input streams to be able to determine stream
 *           read/written lengths to be determinable and both being able to pick
 *           up from left off point. This was not handled correctly! This was
 *           shied from because did not want void * use nor handle (**) use. I
 *           had hoped (mistakenly) to get this done in this current prototype.
 *
 *  solutions:  (one) use handles (**) to return both stream end points and return
 *           error code or boolean error occurred.
 *           (two) create a struct return with end points, error condition, and
 *           any future needed data
 * thoughts: on (one), would like handles to be reserved for returned allocations
 *           of objects. Plus that would lead to special user handling of pointers
 *           entry and exit.
 *           on (two), removes solution one's concerns, plus could alter param
 *           return to counts instead of pointers, or do both, or return any
 *           future info created due to design changes, like maybe a scan for
 *           specific code point during read/writes. Errors could become specific,
 *           such as, termination due to buffer size_t limits or invalid code vs
 *           not a unicode code, or termination on EOS (nul). Let's assume I am
 *           wrong about wchar, a state could be returned, largest code set bytes
 *           per code. I do NOT want param in and param out. Like idea of simplicity
 *           since this makes user programming, expansion, and debugging easier and
 *           less error prone.
 *
 * implementation: decided on (two). Need two pointers and error state. Pointers
 *           should be: first slot, Unicode pointer, second slot, code set pointer
 *           then error. This could be instead write pointer first, read pointer
 *           second, just as prototype arguments are write info then read info.
 *           
 *           The write/read setup would allow for special "locale_t" a FILE like
 *           struct where maybe locale to locale shortcuts could be used for
 *           both encoder/decoder operations and possible operations as loading
 *           a locale from file. This would structure a design concept allowing
 *           for easy operation design rather then reading manual to work
 *           each operation.
 *
 *           Still do not want to go with the void * since size_t for Unicode
 *           would always have math to break into "char" units. uint32_t can
 *           lead to optimizations and speciality writers can alter internally
 *           a known just like an unknown. The "char *" stays, rather than
 *           "unsigned char *" for convention, less typing, easy to typecast
 *           internally if needed.
 */

/*
 * Since trying to distinguish the code set routines, return
 * struct should prefix with "cs". Suffix has been trying to
 * distinguish by me as "_t" (type) as scalar object, and
 * "_st" (struct type) for non-scalar. So returned object
 * should be "csXxxxx_st". wchar_t was initially used for unicode
 * but depends on user. ISO/IEC TR 19769 is trying for char16_t
 * and char32_t. This object really deals with encoding, decoding
 * so neither "types" should  be in object name.
 * covert or transform is actually the best description for
 * encode/decode. so xform, cvrt ? transform I think mathematics,
 * convert, one state to another. CNVRT.
 *  csCnvrt_st
 */
/* notes:
 * On csSI, does not check if paired. This could be a single
 * code point entry used as a signal, or even a private use on some
 * other encoding scheme.
 * Similarly, csUU terminates on > 10FFFF, a code point greater might
 * be a different encoding scheme, or a signal.
 */
/* cs_rd: if cs_code is an early termination error,
 * cs_rd will point to offending object, byte or sequence.
 * cs_rd will point to read location, or just after the
 * nul stream terminator. If nul is to be included as part of
 * the stream, caller should check if nul termination happened
 * and continue with another call to encode/decode until desired
 * termination condition is met. On nul termination, cs_rd points
 * just after nul, as does cs_wr. So if joining reads into
 * a single write stream, decrement write pointer, change read pointer
 * to next stream to combine. Note that a different encoding scheme
 * can even be joined together. If incomplete code at end of stream,
 * move remaining stream to buffer beginning add rest of stream behind
 * last remaining unit(s) in buffer.
 */
/* On csNUL, behavior defined as:
 *   stream pointers end at nul,
 *   counters are not advanced.
 * Resets placed here, rcsNUL encode/decode exit code match up.
 */

#ifndef _AS_LOCALE_CODESET_H_
#define _AS_LOCALE_CODESET_H_

#include "ianacharset_mib.h" /* MIBenum type */

/* current data needs for CODESET */
/*
typedef struct {
 codeset name     // using MIB enum Numbers, instead
 codeset unit sz  // case where switching to different unit size (wchar_t)
 encoderFn        // Unicode to Codeset
 decoderFn        // Codeset to Unicode
} lc_CODESET_st;

 * I believe that all wchar_t rountines can be accomplished by a wrapper.
 *
 * This makes wchar_t rountines a wrapper allowing wchar_t to be the
 * abstract object encoding scheme for any codeset. This makes routine
 * codeset size largest to wchar_t is converting unit size to wchar_t
 * sizes. A four byte code must byte swap to either a 32bit wchar_t or
 * byte swap to 64bit. Big endian handles differently than little endian.
 * If things are as is, wchar_t defined by POSIX must be at least 8bits, but
 * can be up to "long" which now floats between 32 and 64 bits.
 * If sizeof(wchar_t) is 8bit then UTF-16 and UTF-32 must be converted to
 * 8bit streams. If wchar_t is 32bit then UTF-16 must convert to 32bit.
 *
 * So any wchar_t rountine is:
 *    If wchar_t input, convert wchar_t input to code set units.
 *    If wchar_t output, convert code set units to wchar_t output.
 * Could have for a given machine a table of function pointers
 * converting wchar_t to 8bit, 16bit, 32bit, 64bit
 * converting 8bit, 16bit, 32bit, 64bit to wchar_t
 * these would be machine specific and wchar_t specific.
*/


/*
 * Unicode clause C12a:
 *   When faced with this ill-formed code unit sequence while transforming or
 * interpreting text, a conformant process must treat the first code unit as
 * an illegally terminated code unit sequence—for example, by signaling an
 * error, filtering the code unit out, or representing the code unit with a
 * marker such as U+FFFD (REPLACEMENT CHARACTER).
 *
 *   Currently, encoders/decoders return on "ill-formed code unit"s. The user,
 * a function, or programmer, must decide on how they will handle this code.
 * I believe it is better not to hard code the handling, since I can not
 * foresee all scenarios of all locales, nor the possibility or under certain
 * conditions, a standard might arise.
 */

#define EOSmark 0  /* EndOfStream indicator */

typedef enum {
  csNUL = 0,  /* nul termination occurred */
  csWR  = 1,  /* write size termination */
  csRD  = 2,  /* read size termination */
  csRW  = 3,  /* read size & write size termination */
  /* ALL codes below signal errors */
  csRN  = 4,  /* read early nul terminated, was valid code point until terminated */
  csWN  = 5,  /* write buffer terminated, was valid code point until terminated */
  csIC  = 6,  /* invalid code, bytes read don't form code point */
  csIU  = 7,  /* invalid unicode code point (> 0x10FFFF), or cs would need more bytes
               than encoding scheme allows, undeterminable code points */
  csSV  = 8,  /* use of 0xD800 - 0xDC00, missing pair on read, pair unknown, terminated by read sz  */
  csSI  = 9,  /* use of 0xDC00 - 0xDFFF, or use of 0xD800 - 0xDFFF, invalid for code set*/
  csNC  = 10, /* use of 0xnFFFE - 0xnFFFF, invalid for code set, valid for internal use, (flag only) */
  csIE  = 11  /* invalid entry, wrSz = 0, rdPtr == NULL, or rdSz == 0 or
               not large enough to read a code point */
} csEnum;

/* csNC:
 *  Handled differently. To get Encoder/Decoder to encode/decode, must use
 * Encoder/Decoder in single mode. A knowledge or table is required
 * to unlock this flag. Once the correct input key is used, the Encoder/Decoder
 * should return csRW. Remember, this is a flag for a closed system, so these
 * extra steps to avoid security breaches are needed.
 *  If encountered from external unicode stream, it can be handled easily, just
 * step over the code point. If from external code set stream, a knowledge is
 * still needed, of space the flag takes in it's scheme. The external code set
 * stream was not in compliance, or a hacker is trying to mess with you. You
 * can easily send retransmit or other error codes to sender.
 */
/*
 *  I believe there is a need. I believe a handler for returning more
 * information from offending (csSI, csIC, csIU, csNC) is needed. See above, or
 * refer to any rountine used to test. The hoops are too much for end user to
 * remember, or research, considering they don't care about the code set they
 * just want it to work.
 *  Something like getEn/DeInfo(cnvrt_t, csEnum); might be what I want.
 */
typedef struct {
  void   *cs_wr;       /* object's next write location */
  size_t  cs_wrSz;     /* object's number of elements */
  void   *cs_rd;       /* object's next read location */
  size_t  cs_rdSz;     /* object's number of elements */
} csCnvrt_st, *csCnvrt_t;

typedef struct {
  MIBenum   MIB;
  csEnum   (*encoderFn)(csCnvrt_st *);
  csEnum   (*decoderFn)(csCnvrt_st *);
} lc_codeset_st;

/* Chinese GB18030:2005 */  /* COMPLETED and TESTED */
extern csEnum  UStrtoGB18030Str(csCnvrt_st *);
extern csEnum  GB18030StrtoUStr(csCnvrt_st *);
/* Doesn't exclude all of 18030 codes, but roundtrip encoding of GB2312 */
extern csEnum  UStrtoGB2312Str(csCnvrt_st *);
extern csEnum  GB2312StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoEUCCNStr(csCnvrt_st *);
extern csEnum  EUCCNStrtoUStr(csCnvrt_st *);

/* UTF-8 */         /* COMPLETED and MOST TESTED */
extern csEnum  UStrtoUTF8Str(csCnvrt_st *);
extern csEnum  UTF8StrtoUStr(csCnvrt_st *);

/* UTF-16 */        /* COMPLETED and LIGHT TESTED */
extern csEnum  UStrtoUTF16Str(csCnvrt_st *);
extern csEnum  UTF16StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoUTF16BEStr(csCnvrt_st *);
extern csEnum  UTF16BEStrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoUTF16LEStr(csCnvrt_st *);
extern csEnum  UTF16LEStrtoUStr(csCnvrt_st *);

/* UTF-32 */        /* COMPLETED and LIGHT TESTED */
extern csEnum  UStrtoUTF32Str(csCnvrt_st *);
extern csEnum  UTF32StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoUTF32BEStr(csCnvrt_st *);
extern csEnum  UTF32BEStrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoUTF32LEStr(csCnvrt_st *);
extern csEnum  UTF32LEStrtoUStr(csCnvrt_st *);

/* Japanese */        /* COMPLETED and TESTED */
extern csEnum  UStrtoJISX0213Str(csCnvrt_st *cnvrt);
extern csEnum  JISX0213StrtoUStr(csCnvrt_st *cnvrt);
extern csEnum  UStrtoSJISStr(csCnvrt_st *cnvrt);
extern csEnum  SJISStrtoUStr(csCnvrt_st *cnvrt);
extern csEnum  UStrtoEUCJPStr(csCnvrt_st *cnvrt);
extern csEnum  EUCJPStrtoUStr(csCnvrt_st *cnvrt);

/* ISO-8859 */        /* COMPLETED and NOT TESTED */
extern csEnum  UStrtoASCIIStr(csCnvrt_st *);
extern csEnum  ASCIIStrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO1Str(csCnvrt_st *);
extern csEnum  ISO1StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO2Str(csCnvrt_st *);
extern csEnum  ISO2StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO3Str(csCnvrt_st *);
extern csEnum  ISO3StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO4Str(csCnvrt_st *);
extern csEnum  ISO4StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO5Str(csCnvrt_st *);
extern csEnum  ISO5StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO6Str(csCnvrt_st *);
extern csEnum  ISO6StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO7Str(csCnvrt_st *);
extern csEnum  ISO7StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO8Str(csCnvrt_st *);
extern csEnum  ISO8StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO9Str(csCnvrt_st *);
extern csEnum  ISO9StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO10Str(csCnvrt_st *);
extern csEnum  ISO10StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO11Str(csCnvrt_st *);
extern csEnum  ISO11StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO13Str(csCnvrt_st *);
extern csEnum  ISO13StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO14Str(csCnvrt_st *);
extern csEnum  ISO14StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO15Str(csCnvrt_st *);
extern csEnum  ISO15StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoISO16Str(csCnvrt_st *);
extern csEnum  ISO16StrtoUStr(csCnvrt_st *);

/* cswindows */    /* COMPLETED and TESTED */
extern csEnum  UStrtocs1251Str(csCnvrt_st *);
extern csEnum  cs1251StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtocs1252Str(csCnvrt_st *);
extern csEnum  cs1252StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtocs1256Str(csCnvrt_st *);
extern csEnum  cs1256StrtoUStr(csCnvrt_st *);

/* KOI8 */         /* COMPLETED and TESTED */
extern csEnum  UStrtoKOI8UStr(csCnvrt_st *);
extern csEnum  KOI8UStrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoKOI8RStr(csCnvrt_st *);
extern csEnum  KOI8RStrtoUStr(csCnvrt_st *);

/* PTCP154 */     /* COMPLETED and TESTED */
extern csEnum  UStrtoPTCP154Str(csCnvrt_st *);
extern csEnum  PTCP154StrtoUStr(csCnvrt_st *);

/* Big5 */        /* COMPLETED and TESTED */
extern csEnum  UStrtoBig5Str(csCnvrt_st *);
extern csEnum  Big5StrtoUStr(csCnvrt_st *);
extern csEnum  UStrtoHKSCSStr(csCnvrt_st *);
extern csEnum  HKSCSStrtoUStr(csCnvrt_st *);

    
#endif	/* !_AS_LOCALE_CODESET_H_ */
