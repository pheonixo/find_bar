/*
 *  csPTCP154.h
 *  PheonixOS
 *
 *  Created by Steven Abner on Wed 13 Nov 2013.
 *  Copyright (c) 2013. All rights reserved.
 *
 */

const uint16_t _csPTCP154_to_unicode[64] = {
  0x0496, 0x0492, 0x04EE, 0x0493, 0x201E, 0x2026, 0x04B6, 0x04AE,
  0x04B2, 0x04AF, 0x04A0, 0x04E2, 0x04A2, 0x049A, 0x04BA, 0x04B8,
  0x0497, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
  0x04B3, 0x04B7, 0x04A1, 0x04E3, 0x04A3, 0x049B, 0x04BB, 0x04B9,
  0x00A0, 0x040E, 0x045E, 0x0408, 0x04E8, 0x0498, 0x04B0, 0x00A7,
  0x0401, 0x00A9, 0x04D8, 0x00AB, 0x00AC, 0x04EF, 0x00AE, 0x049C,
  0x00B0, 0x04B1, 0x0406, 0x0456, 0x0499, 0x04E9, 0x00B6, 0x00B7,
  0x0451, 0x2116, 0x04D9, 0x00BB, 0x0458, 0x04AA, 0x04AB, 0x049D
};

const uint16_t _unicode_to_csPTCP154[128] = {
  0x00A0,0x00A0, 0x00A7,0x00A7, 0x00A9,0x00A9, 0x00AB,0x00AB,
  0x00AC,0x00AC, 0x00AE,0x00AE, 0x00B0,0x00B0, 0x00B6,0x00B6,
  0x00B7,0x00B7, 0x00BB,0x00BB, 0x0401,0x00A8, 0x0406,0x00B2,
  0x0408,0x00A3, 0x040E,0x00A1, 0x0451,0x00B8, 0x0456,0x00B3,
  0x0458,0x00BC, 0x045E,0x00A2, 0x0492,0x0081, 0x0493,0x0083,
  0x0496,0x0080, 0x0497,0x0090, 0x0498,0x00A5, 0x0499,0x00B4,
  0x049A,0x008D, 0x049B,0x009D, 0x049C,0x00AF, 0x049D,0x00BF,
  0x04A0,0x008A, 0x04A1,0x009A, 0x04A2,0x008C, 0x04A3,0x009C,
  0x04AA,0x00BD, 0x04AB,0x00BE, 0x04AE,0x0087, 0x04AF,0x0089,
  0x04B0,0x00A6, 0x04B1,0x00B1, 0x04B2,0x0088, 0x04B3,0x0098,
  0x04B6,0x0086, 0x04B7,0x0099, 0x04B8,0x008F, 0x04B9,0x009F,
  0x04BA,0x008E, 0x04BB,0x009E, 0x04D8,0x00AA, 0x04D9,0x00BA,
  0x04E2,0x008B, 0x04E3,0x009B, 0x04E8,0x00A4, 0x04E9,0x00B5,
  0x04EE,0x0082, 0x04EF,0x00AD, 0x2013,0x0096, 0x2014,0x0097,
  0x2018,0x0091, 0x2019,0x0092, 0x201C,0x0093, 0x201D,0x0094,
  0x201E,0x0084, 0x2022,0x0095, 0x2026,0x0085, 0x2116,0x00B9
};