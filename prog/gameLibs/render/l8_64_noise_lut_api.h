// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <dag_noise/dag_uint_noise.h>

// The content of the 64x64 L8 noise LUT (render/noiseTex.cpp): one home for the GPU
// fill and its CPU twins. No 3d types, so header-only consumers stay d3d-free.
static constexpr int L8_64_NOISE_W = 64;
inline uint8_t l8_64_noise_lut(int x, int y)
{
  return uint_noise1D((y & (L8_64_NOISE_W - 1)) * L8_64_NOISE_W + (x & (L8_64_NOISE_W - 1)), 0x11231231) & 0xFF;
}
