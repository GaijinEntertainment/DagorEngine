/*
  Copyright (c) 2013 - 2014, 2016 Mark Adler, Robert Vazan, Max Vysokikh

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software
  in a product, an acknowledgment in the product documentation would be
  appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "crc32c.h"
#include "crc32c_data.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <stdint.h>
#include <stddef.h>

#define POLY 0x82f63b78
#define LONG_SHIFT 8192
#define SHORT_SHIFT 256



#if _TARGET_PC_WIN

#include <intrin.h>
#include <nmmintrin.h>

#define CRC32_INLINE __inline
#if defined(__clang__) && !defined(__SSE4_2__)
#define CRC32_HW_SUPPORT 0
#else
#define CRC32_HW_SUPPORT 1

static __inline int crc32c_hw_available()
{
  int info[4];
  __cpuid(info, 1);
  return (info[2] >> 20) & 1;
}


static __inline unsigned wrap_crc32_u8(unsigned crc, uint8_t val)
{
  return _mm_crc32_u8(crc, val);
}

static __inline unsigned wrap_crc32_u32(unsigned crc, uint32_t val)
{
  return _mm_crc32_u32(crc, val);
}

#if _TARGET_64BIT
static __inline uint64_t wrap_crc32_u64(uint64_t crc, uint64_t val)
{
  return _mm_crc32_u64(crc, val);
}
#endif

#endif
#elif _TARGET_PC && !_TARGET_SIMD_NEON


#define CRC32_INLINE inline
#define CRC32_HW_SUPPORT 1


static inline int crc32c_hw_available()
{
  uint32_t eax, ecx;
  eax = 1;
  __asm__("cpuid"
          : "=c"(ecx)
          : "a"(eax)
          : "%ebx", "%edx");
  return (ecx >> 20) & 1;
}


static inline unsigned wrap_crc32_u8(unsigned crc, uint8_t val)
{
  __asm__("crc32b\t" "(%1), %0"
          : "=r"(crc)
          : "r"(&val), "0"(crc), "m"(val));
  return crc;
}

static inline unsigned wrap_crc32_u32(unsigned crc, uint32_t val)
{
  __asm__("crc32l\t" "(%1), %0"
          : "=r"(crc)
          : "r"(&val), "0"(crc), "m"(val));
  return crc;
}

#if _TARGET_64BIT

static inline uint64_t wrap_crc32_u64(uint64_t crc, uint64_t val)
{
  __asm__("crc32q\t" "(%1), %0"
          : "=r"(crc)
          : "r"(&val), "0"(crc), "m"(val));
  return crc;
}

#endif


#else


#define CRC32_INLINE inline
#define CRC32_HW_SUPPORT 0


#endif



typedef const unsigned char *buffer;


/* Table-driven software version as a fall-back.  This is about 15 times slower
   than using the hardware instructions.  This assumes little-endian integers,
   as is the case on Intel processors that the assembler code here is for. */
unsigned crc32c_append_sw(unsigned crci, buffer input, int length)
{
    buffer next = input;
#if _TARGET_64BIT
    uint64_t crc;
    uint64_t high;
#else
    uint32_t crc;
    uint32_t high;
    uint32_t high2;
#endif

    crc = crci ^ 0xffffffff;
#if _TARGET_64BIT
    while (length && ((uintptr_t)next & 7) != 0)
    {
        crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        --length;
    }
    while (length >= 16)
    {
        crc ^= *(uint64_t *)next;
        high = *(uint64_t *)(next + 8);
        crc = table[15][crc & 0xff]
            ^ table[14][(crc >> 8) & 0xff]
            ^ table[13][(crc >> 16) & 0xff]
            ^ table[12][(crc >> 24) & 0xff]
            ^ table[11][(crc >> 32) & 0xff]
            ^ table[10][(crc >> 40) & 0xff]
            ^ table[9][(crc >> 48) & 0xff]
            ^ table[8][crc >> 56]
            ^ table[7][high & 0xff]
            ^ table[6][(high >> 8) & 0xff]
            ^ table[5][(high >> 16) & 0xff]
            ^ table[4][(high >> 24) & 0xff]
            ^ table[3][(high >> 32) & 0xff]
            ^ table[2][(high >> 40) & 0xff]
            ^ table[1][(high >> 48) & 0xff]
            ^ table[0][high >> 56];
        next += 16;
        length -= 16;
    }
#else
    while (length && ((uintptr_t)next & 3) != 0)
    {
        crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        --length;
    }
    while (length >= 12)
    {
        crc ^= *(uint32_t *)next;
        high = *(uint32_t *)(next + 4);
        high2 = *(uint32_t *)(next + 8);
        crc = table[11][crc & 0xff]
            ^ table[10][(crc >> 8) & 0xff]
            ^ table[9][(crc >> 16) & 0xff]
            ^ table[8][crc >> 24]
            ^ table[7][high & 0xff]
            ^ table[6][(high >> 8) & 0xff]
            ^ table[5][(high >> 16) & 0xff]
            ^ table[4][high >> 24]
            ^ table[3][high2 & 0xff]
            ^ table[2][(high2 >> 8) & 0xff]
            ^ table[1][(high2 >> 16) & 0xff]
            ^ table[0][high2 >> 24];
        next += 12;
        length -= 12;
    }
#endif
    while (length)
    {
        crc = table[0][(crc ^ *next++) & 0xff] ^ (crc >> 8);
        --length;
    }
    return (uint32_t)crc ^ 0xffffffff;
}

/* Apply the zeros operator table to crc. */
static CRC32_INLINE uint32_t shift_crc(uint32_t shift_table[][256], uint32_t crc)
{
    return shift_table[0][crc & 0xff]
        ^ shift_table[1][(crc >> 8) & 0xff]
        ^ shift_table[2][(crc >> 16) & 0xff]
        ^ shift_table[3][crc >> 24];
}


#if CRC32_HW_SUPPORT


/* Compute CRC-32C using the Intel hardware instruction. */
unsigned crc32c_append_hw(unsigned crc, buffer buf, int len)
{
    buffer next = buf;
    buffer end;
#if _TARGET_64BIT
    uint64_t crc0, crc1, crc2;      /* need to be 64 bits for crc32q */
#else
    uint32_t crc0, crc1, crc2;
#endif

    /* pre-process the crc */
    crc0 = crc ^ 0xffffffff;

    /* compute the crc for up to seven leading bytes to bring the data pointer
       to an eight-byte boundary */
    while (len && ((uintptr_t)next & 7) != 0)
    {
        crc0 = wrap_crc32_u8((uint32_t)crc0, *next);
        ++next;
        --len;
    }

#if _TARGET_64BIT
    /* compute the crc on sets of LONG_SHIFT*3 bytes, executing three independent crc
       instructions, each on LONG_SHIFT bytes -- this is optimized for the Nehalem,
       Westmere, Sandy Bridge, and Ivy Bridge architectures, which have a
       throughput of one crc per cycle, but a latency of three cycles */
    while (len >= 3 * LONG_SHIFT)
    {
        crc1 = 0;
        crc2 = 0;
        end = next + LONG_SHIFT;
        do
        {
            crc0 = wrap_crc32_u64(crc0, *(const uint64_t *)next);
            crc1 = wrap_crc32_u64(crc1, *(const uint64_t *)(next + LONG_SHIFT));
            crc2 = wrap_crc32_u64(crc2, *(const uint64_t *)(next + 2 * LONG_SHIFT));
            next += 8;
        } while (next < end);
        crc0 = shift_crc(long_shifts, (uint32_t)crc0) ^ crc1;
        crc0 = shift_crc(long_shifts, (uint32_t)crc0) ^ crc2;
        next += 2 * LONG_SHIFT;
        len -= 3 * LONG_SHIFT;
    }

    /* do the same thing, but now on SHORT_SHIFT*3 blocks for the remaining data less
       than a LONG_SHIFT*3 block */
    while (len >= 3 * SHORT_SHIFT)
    {
        crc1 = 0;
        crc2 = 0;
        end = next + SHORT_SHIFT;
        do
        {
            crc0 = wrap_crc32_u64(crc0, *(const uint64_t *)(next));
            crc1 = wrap_crc32_u64(crc1, *(const uint64_t *)(next + SHORT_SHIFT));
            crc2 = wrap_crc32_u64(crc2, *(const uint64_t *)(next + 2 * SHORT_SHIFT));
            next += 8;
        } while (next < end);
        crc0 = shift_crc(short_shifts, (uint32_t)crc0) ^ crc1;
        crc0 = shift_crc(short_shifts, (uint32_t)crc0) ^ crc2;
        next += 2 * SHORT_SHIFT;
        len -= 3 * SHORT_SHIFT;
    }

    /* compute the crc on the remaining eight-byte units less than a SHORT_SHIFT*3
    block */
    end = next + (len - (len & 7));
    while (next < end)
    {
        crc0 = wrap_crc32_u64(crc0, *(const uint64_t *)next);
        next += 8;
    }
#else
    /* compute the crc on sets of LONG_SHIFT*3 bytes, executing three independent crc
    instructions, each on LONG_SHIFT bytes -- this is optimized for the Nehalem,
    Westmere, Sandy Bridge, and Ivy Bridge architectures, which have a
    throughput of one crc per cycle, but a latency of three cycles */
    while (len >= 3 * LONG_SHIFT)
    {
        crc1 = 0;
        crc2 = 0;
        end = next + LONG_SHIFT;
        do
        {
            crc0 = wrap_crc32_u32(crc0, *(const uint32_t *)next);
            crc1 = wrap_crc32_u32(crc1, *(const uint32_t *)(next + LONG_SHIFT));
            crc2 = wrap_crc32_u32(crc2, *(const uint32_t *)(next + 2 * LONG_SHIFT));
            next += 4;
        } while (next < end);
        crc0 = shift_crc(long_shifts, (uint32_t)crc0) ^ crc1;
        crc0 = shift_crc(long_shifts, (uint32_t)crc0) ^ crc2;
        next += 2 * LONG_SHIFT;
        len -= 3 * LONG_SHIFT;
    }

    /* do the same thing, but now on SHORT_SHIFT*3 blocks for the remaining data less
    than a LONG_SHIFT*3 block */
    while (len >= 3 * SHORT_SHIFT)
    {
        crc1 = 0;
        crc2 = 0;
        end = next + SHORT_SHIFT;
        do
        {
            crc0 = wrap_crc32_u32(crc0, *(const uint32_t *)next);
            crc1 = wrap_crc32_u32(crc1, *(const uint32_t *)(next + SHORT_SHIFT));
            crc2 = wrap_crc32_u32(crc2, *(const uint32_t *)(next + 2 * SHORT_SHIFT));
            next += 4;
        } while (next < end);
        crc0 = shift_crc(short_shifts, (uint32_t)crc0) ^ crc1;
        crc0 = shift_crc(short_shifts, (uint32_t)crc0) ^ crc2;
        next += 2 * SHORT_SHIFT;
        len -= 3 * SHORT_SHIFT;
    }

    /* compute the crc on the remaining eight-byte units less than a SHORT_SHIFT*3
    block */
    end = next + (len - (len & 7));
    while (next < end)
    {
        crc0 = wrap_crc32_u32(crc0, *(const uint32_t *)next);
        next += 4;
    }
#endif
    len &= 7;

    /* compute the crc for up to seven trailing bytes */
    while (len)
    {
        unsigned char c = next[0];
        crc0 = wrap_crc32_u8((uint32_t)crc0, c);
        ++next;
        --len;
    }

    /* return a post-processed crc */
    return ((uint32_t)crc0) ^ 0xffffffff;
}


#endif // CRC32_HW_SUPPORT


// This function can be used to generate table
static void calculate_table()
{
  int i, t, k;
  uint32_t res;
  for (i = 0; i < 256; i++)
  {
    res = (uint32_t)i;
    for (t = 0; t < 16; t++)
    {
      for (k = 0; k < 8; k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
      table[t][i] = res;
    }
  }
}

// This function can be used to generate long_shifts and short_shifts
static void calculate_table_hw()
{
  int i, t, k;
  uint32_t res;

  for(i = 0; i < 256; i++)
  {
    res = (uint32_t)i;
    for (k = 0; k < 8 * (SHORT_SHIFT - 4); k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
    for(t = 0; t < 4; t++)
    {
      for (k = 0; k < 8; k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
      short_shifts[3 - t][i] = res;
    }
    for (k = 0; k < 8 * (LONG_SHIFT - 4 - SHORT_SHIFT); k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
    for(t = 0; t < 4; t++)
    {
      for (k = 0; k < 8; k++) res = (res & 1) == 1 ? POLY ^ (res >> 1) : (res >> 1);
      long_shifts[3 - t][i] = res;
    }
  }
}

static unsigned (*append_func)(unsigned, buffer, int);

unsigned crc32c_append(unsigned crc, buffer input, int length)
{
#if CRC32_HW_SUPPORT
  if (append_func == NULL)
  {
    if (crc32c_hw_available())
      append_func = crc32c_append_hw;
    else
      append_func = crc32c_append_sw;
  }

  return append_func(crc, input, length);
#else
  return crc32c_append_sw(crc, input, length);
#endif
}
