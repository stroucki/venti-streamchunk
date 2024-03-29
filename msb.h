// $Id$

/*
 *
 * Copyright (C) 1998 David Mazieres (dm@uun.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 *
 */

#ifndef _MSB_H_
#define _MSB_H_ 1

#include <sys/types.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else /* !__cplusplus */
#define EXTERN_C extern
#endif /* !__cplusplus */

/*
 * Routines for calculating the most significant bit of an integer.
 */

EXTERN_C const char bytemsb[];

/* Find last set (most significant bit) */
// from /usr/src/linux/include/asm-generic/bitops/fls.h
static inline unsigned int fls32 (u_int32_t x)
{
        unsigned int r = 32;

        if (!x)
                return 0;
        if (!(x & 0xffff0000u)) {
                x <<= 16;
                r -= 16;
        }
        if (!(x & 0xff000000u)) {
                x <<= 8;
                r -= 8;
        }
        if (!(x & 0xf0000000u)) {
                x <<= 4;
                r -= 4;
        }
        if (!(x & 0xc0000000u)) {
                x <<= 2;
                r -= 2;
        }
        if (!(x & 0x80000000u)) {
                x <<= 1;
                r -= 1;
        }
        return r;
}

/* Ceiling of log base 2 */
//static inline int
//  log2c32 (u_int32_t v)
//  {
//    return v ? (int) fls32 (v - 1) : -1;
//  }

//static inline unsigned_int fls64 (u_int64_t) __attribute__ ((const));
static inline char fls64 (u_int64_t v)
{
  u_int32_t h;
  if ((h = v >> 32))
    return 32 + fls32 (h);
  else
    return fls32 ((u_int32_t) v);
}

static inline int log2c64 (u_int64_t) __attribute__ ((const));
static inline int
log2c64 (u_int64_t v)
{
  return v ? (int) fls64 (v - 1) : -1;
}

#define fls(v) (sizeof (v) > 4 ? fls64 (v) : fls32 (v))
#define log2c(v) (sizeof (v) > 4 ? log2c64 (v) : log2c32 (v))

/*
 * For symmetry, a 64-bit find first set, "ffs," that finds the least
 * significant 1 bit in a word.
 */

EXTERN_C const char bytelsb[];

static inline unsigned int
ffs32 (u_int32_t v)
{
  int vv;
  if (v & 0xffff) {
    if ((vv = (v & 0xff)))
      return bytelsb[vv];
    else
      return 8 + bytelsb[v >> 8 & 0xff];
  }
  else if ((vv = (v & 0xff0000)))
    return 16 + bytelsb[vv >> 16];
  else if (v)
    return 24 + bytelsb[v >> 24 & 0xff];
  else
    return 0;
}

static inline unsigned int
ffs64 (u_int64_t v)
{
  u_int32_t l;
  if ((l = v & 0xffffffff))
    return fls32 (l);
  else if ((l = v >> 32))
    return 32 + fls32 (l);
  else
    return 0;
}

#define ffs(v) (sizeof (v) > 4 ? ffs64 (v) : ffs32 (v))

#undef EXTERN_C

#endif /* _MSB_H_  */
