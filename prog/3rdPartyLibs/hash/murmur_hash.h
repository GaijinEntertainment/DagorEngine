#pragma once
/**
 * `murmurhash.h' - murmurhash
 *
 * copyright (c) 2014 joseph werle <joseph.werle@gmail.com>
 */

#include <stdlib.h>
#include <stdint.h>

#if (defined( __cplusplus) && (__cplusplus >= 201703L)) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define FALLTHROUGH [[fallthrough]]
#else
#define FALLTHROUGH
#endif

#if defined(_MSC_VER)
__forceinline uint32_t murmur_rotl32 ( uint32_t x, int8_t r ) { return _rotl(x,r);}
#else
__forceinline uint32_t murmur_rotl32 ( uint32_t x, int8_t r ) { return (x << r) | (x >> (32 - r)); }
#endif // !defined(_MSC_VER)
__forceinline uint32_t murmur_fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

inline uint32_t
murmurhash (const char *key, uint32_t len, uint32_t seed)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

  for(int i = -nblocks; i; i++)
  {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = murmur_rotl32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = murmur_rotl32(h1,13);
    h1 = h1*5+0xe6546b64;
  }
  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

  uint32_t k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16; FALLTHROUGH;
  case 2: k1 ^= tail[1] << 8; FALLTHROUGH;
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = murmur_rotl32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  return murmur_fmix32(h1);
}

inline uint32_t
murmurhash_aligned (const uint32_t *keys, uint32_t keys_count, uint32_t seed)
{
  const uint32_t l = keys_count;
  const int nblocks = l;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(keys + nblocks);

  for(int i = -nblocks; i; i++)
  {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = murmur_rotl32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = murmur_rotl32(h1,13);
    h1 = h1*5+0xe6546b64;
  }
  //----------
  // finalization

  h1 ^= l;

  return murmur_fmix32(h1);
}

#undef FALLTHROUGH
