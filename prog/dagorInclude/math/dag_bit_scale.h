//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>
#include <string.h> // for memcpy

static inline uint16_t bit_upscale_4x4_to_8x8(uint64_t sourceData, int quadrantX, int quadrantY);

// Upscales data of dim*dim bits block x2 (writing to same dim x dim block)
// from is pointing to begining of quadrant we are upscaling, dim >= 16 (so from is pointing to aligned source)
static inline void bit_upscale_2x2(uint8_t *to8, const uint8_t *from, int dim);

// Upscales data of dim*dim bits block x4 (writing to same dim x dim block)
// from is pointing to begining of quadrant we are upscaling, dim >= 32 (so from is pointing to aligned source)
static inline void bit_upscale_4x4(uint8_t *to8, const uint8_t *from, int dim);

// Quadruplicates lower 2 bits of y to 8 bits
static inline uint8_t bit_quadruplicate_lower_2bits_to8(int y);

// Duplicates lower 4 bits of y to 8 bits
static inline uint8_t bit_duplicate_lower_4bits_to8(uint8_t y);

// Duplicates lower 8 bits of y to 16 bits
static inline uint16_t bit_duplicate_lower_8bits_to16(uint8_t y);

// Quadruplicates lower 4 bits of y to 16 bits
static inline uint16_t bit_quadruplicate_lower_4bits_to16(int y);

// Quadruplicates lower 8 bits of y to 32 bits
static inline uint32_t bit_quadruplicate_lower_8bits_to32(int y);

// Downsamples 16-bit line to 8 bits, selecting every other (even) bit: 0(first), 2 (third), etc
static inline uint8_t bit_downsample_line16_to8(uint16_t y);

// Downsamples 32-bit line to 16 bits, selecting every other (even) bit: 0(first), 2 (third), etc
static inline uint16_t bit_downsample_line32_to16(uint32_t y);

// Downsamples 8x8 bit block (64 bits) to 4x4 (16 bits)
static inline uint16_t bit_downsample8x8(uint64_t x);

static inline uint16_t bit_upscale_4x4_to_8x8(uint64_t sourceData, int quadrantX, int quadrantY)
{
  sourceData >>= (quadrantY << 5) + (quadrantX << 2);
  uint16_t result = sourceData & 0xF;

  sourceData >>= 8;
  result |= (sourceData & 0xF) << 4;

  sourceData >>= 8;
  result |= (sourceData & 0xF) << 8;

  sourceData >>= 8;
  result |= (sourceData & 0xF) << 12;

  return result;
}


static inline uint8_t bit_duplicate_lower_4bits_to8(uint8_t x)
{
  uint8_t y = x & 0x0F;
  uint8_t even_bits = (y & 0x01) | ((y & 0x02) << 1) | ((y & 0x04) << 2) | ((y & 0x08) << 3);
  return even_bits | (even_bits << 1);
}

static inline uint16_t bit_duplicate_lower_8bits_to16(uint8_t y)
{
  uint16_t even_bits = (y & 0x01) | ((y & 0x02) << 1) | ((y & 0x04) << 2) | ((y & 0x08) << 3) | ((y & 0x10) << 4) | ((y & 0x20) << 5) |
                       ((y & 0x40) << 6) | ((y & 0x80) << 7);
  return even_bits | (even_bits << 1);
}

static inline uint8_t bit_quadruplicate_lower_2bits_to8(int y)
{
  uint8_t ret = uint8_t(-(y & 0x01)) & 0xF;
  ret |= uint8_t(-(y & 0x02)) & 0xF0;
  return ret;
}

static inline uint16_t bit_quadruplicate_lower_4bits_to16(int y)
{
  uint16_t ret;
  ret = uint16_t(-(y & 0x01)) & 0xF;
  ret |= uint16_t(-(y & 0x02)) & 0xF0;
  ret |= uint16_t(-(y & 0x04)) & 0xF00;
  ret |= uint16_t(-(y & 0x08)) & 0xF000;
  return ret;
}

static inline uint32_t bit_quadruplicate_lower_8bits_to32(int y)
{
  uint32_t ret;
  ret = (-(y & 0x01)) & 0xF;
  ret |= (-(y & 0x02)) & 0xF0;
  ret |= (-(y & 0x04)) & 0xF00;
  ret |= (-(y & 0x08)) & 0xF000;
  ret |= (-(y & 0x10)) & 0xF0000;
  ret |= (-(y & 0x20)) & 0xF00000;
  ret |= (-(y & 0x40)) & 0xF000000;
  ret |= (-(y & 0x80)) & 0xF0000000;
  return ret;
}

static inline uint8_t bit_downsample_line16_to8(uint16_t x)
{
  return (x & 1) | (x & (1 << 2)) >> 1 | (x & (1 << 4)) >> 2 | (x & (1 << 6)) >> 3 | (x & (1 << 8)) >> 4 | (x & (1 << 10)) >> 5 |
         (x & (1 << 12)) >> 6 | (x & (1 << 14)) >> 7;
}

static inline uint16_t bit_downsample_line32_to16(uint32_t x)
{
  return (x & 1) | (x & (1 << 2)) >> 1 | (x & (1 << 4)) >> 2 | (x & (1 << 6)) >> 3 | (x & (1 << 8)) >> 4 | (x & (1 << 10)) >> 5 |
         (x & (1 << 12)) >> 6 | (x & (1 << 14)) >> 7 | (x & (1 << 16)) >> 8 | (x & (1 << 18)) >> 9 | (x & (1 << 20)) >> 10 |
         (x & (1 << 22)) >> 11 | (x & (1 << 24)) >> 12 | (x & (1 << 26)) >> 13 | (x & (1 << 28)) >> 14 | (x & (1 << 30)) >> 15;
}

static inline uint16_t bit_downsample8x8(uint64_t x)
{
  return (x & 1ULL) | (x & (1ULL << 2ULL)) >> 1ULL | (x & (1ULL << 4ULL)) >> 2ULL | (x & (1ULL << 6ULL)) >> 3ULL |
         (x & (1ULL << 16ULL)) >> 12ULL | (x & (1ULL << 18ULL)) >> 13ULL | (x & (1ULL << 20ULL)) >> 14ULL |
         (x & (1ULL << 22ULL)) >> 15ULL | (x & (1ULL << 32ULL)) >> 24ULL | (x & (1ULL << 34ULL)) >> 25ULL |
         (x & (1ULL << 36ULL)) >> 26ULL | (x & (1ULL << 38ULL)) >> 27ULL | (x & (1ULL << 48ULL)) >> 36ULL |
         (x & (1ULL << 50ULL)) >> 37ULL | (x & (1ULL << 52ULL)) >> 38ULL | (x & (1ULL << 54ULL)) >> 39ULL;
}

static inline void bit_upscale_2x2(uint8_t *to8, const uint8_t *from, int dim)
{
  uint16_t *to = (uint16_t *)to8;
  for (int j = 0, je = dim >> 1; j < je; ++j)
  {
    auto row = to;
    for (int i = 0, ie = (dim >> 4); i < ie; ++i, to++, from++) // shift >>4, as we write 16 bit
      to[0] = bit_duplicate_lower_8bits_to16(from[0]);
    memcpy(to, row, dim >> 3);       // copy row to second line
    to += dim >> 4;                  // add one line, shift by >> 4, as 'to' is 16bit (so >>3 is bytes, >>4 is words)
    from += (dim >> 3) - (dim >> 4); // add rest of stride
  }
}

static inline void bit_upscale_4x4(uint8_t *to8, const uint8_t *from, int dim)
{
  uint32_t *to = (uint32_t *)to8;
  for (int i = 0, ie = dim >> 2; i < ie; ++i)
  {
    auto row = to;
    for (int j = 0, je = dim >> 5; j != je; ++j, to++, from++) // shift >>5, as we write 32 bit, dword
      to[0] = bit_quadruplicate_lower_8bits_to32(from[0]);
    memcpy(to, row, dim >> 3); // copy row to second line
    to += dim >> 5;            // add one line, shift by >> 5, as 'to' is 32bit (so >>3 is bytes, >>5 is dwords)
    memcpy(to, row, dim >> 2); // copy two rows to 3rd + 4th line
    to += dim >> 4;            // add two lines, shift by >> 4, as 'to' is 32bit (so >>3 is bytes, >>5 is dwords) * 2

    from += (dim >> 3) - (dim >> 5); // add rest of stride
  }
}
