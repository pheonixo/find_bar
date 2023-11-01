/*
 *  memrchr - find last occurrance of a character
 *  Pheonix
 *
 *  Created by Steven Abner on Tue 24 Sep 2019.
 *  Copyright (c) 2019. All rights reserved.
 *
 */

#include <string.h>
#include <stdint.h>

#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif

char *
memrchr(const char *s, int c, size_t n) {

  size_t a = n;
  unsigned char *src = (unsigned char *)s + n;
  uint32_t t1, r0, r1;
  uint32_t  lend;

  if (!(n & ~7)) {
    do if (*--src == (unsigned char)c) goto rsrc; while ((--n));
    goto rnull;
  }
  lend = (unsigned char)c * (r0 = 0x01010101);
  r1 = 0x80808080;
  src -= 4, t1 = *(uint32_t *)src;
  t1 ^= lend;
  if (!((t1 - r0) & (~t1 & r1))) {
    uint32_t t2;
    if (!(a & (size_t)3)) a -= 4;
    src += (-a & (size_t)3);
    n = a >> 2;
    do {
      src -= 4, t1 = *(uint32_t *)src;  t1 ^= lend;
    } while ((!(t2 = ((t1 - r0) & (~t1 & r1)))) && ((--n)));
    if (!t2)  goto rnull;
  }
  src += 3;
#if __GNUC_PREREQ(4,5)
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
 #ifdef __LITTLE_ENDIAN__
  if (!(t1 & 0xff000000)) goto rsrc;  src--;
  if (!(t1 & 0x00ff0000)) goto rsrc;  src--;
  if (!(t1 & 0x0000ff00)) goto rsrc;  src--;
 #else
  if (!(t1 & 0x000000ff)) goto rsrc;  src--;
  if (!(t1 & 0x0000ff00)) goto rsrc;  src--;
  if (!(t1 & 0x00ff0000)) goto rsrc;  src--;
 #endif
#if __GNUC_PREREQ(4,5)
#pragma GCC diagnostic pop
#endif
rsrc:
  return (char *)src;
rnull:
  return NULL;
}

