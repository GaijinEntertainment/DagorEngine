//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <cstdint>
#include <cstring>

#include <debug/dag_assert.h>

/*
When size of data used for memcpy is known in compile time and less than 128 bytes, compiler will generate a series of move instruction
using registers like:
movups  xmm0, xmmword ptr [rsi + 112]
movups  xmmword ptr [rdi + 112], xmm0
movups  xmm0, xmmword ptr [rsi + 96]
movups  xmmword ptr [rdi + 96], xmm0
movups  xmm0, xmmword ptr [rsi + 80]
movups  xmmword ptr [rdi + 80], xmm0
movups  xmm0, xmmword ptr [rsi + 64]
movups  xmmword ptr [rdi + 64], xmm0
movups  xmm0, xmmword ptr [rsi]
movups  xmm1, xmmword ptr [rsi + 16]
movups  xmm2, xmmword ptr [rsi + 32]
movups  xmm3, xmmword ptr [rsi + 48]
movups  xmmword ptr [rdi + 48], xmm3
movups  xmmword ptr [rdi + 32], xmm2
movups  xmmword ptr [rdi + 16], xmm1
movups  xmmword ptr [rdi], xmm0

So, we can split the whole memory into blocks and iterate over blocks to copy them without memcpy call.

According to xbox profiling utils and microprofiler it is 15% faster than memcpy call for the whole memory.
The performance was tested on synthetic benchmarks, RI matrix copy and occlusion readback.
*/

template <size_t BLOCK_BYTES>
inline void block_memcopy(uint8_t *dst, const uint8_t *src, size_t size)
{
  G_FAST_ASSERT(size % BLOCK_BYTES == 0);
  static_assert(BLOCK_BYTES % 16 == 0, "Block should be representable as several vec4f.");
  static_assert(BLOCK_BYTES <= 128, "If the block size is bigger than 128 bytes it will just call memcpy function");
  for (const uint8_t *srcEnd = src + size; src < srcEnd; src += BLOCK_BYTES, dst += BLOCK_BYTES)
    memcpy(dst, src, BLOCK_BYTES);
}
