//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// Math for Game Programmers: Noise-Based RNG
// author: Squirrel Eiserloh
__forceinline unsigned int uint32_hash(unsigned int val)
{
  constexpr unsigned int BIT_NOISE1 = 0x68E31DA4;
  constexpr unsigned int BIT_NOISE2 = 0xB5297A4D;
  constexpr unsigned int BIT_NOISE3 = 0x1B56C4E9;
  unsigned int mangled = val;
  mangled *= BIT_NOISE1;
  mangled ^= (mangled>>8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled<<8);
  mangled *= BIT_NOISE3;
  mangled ^= (mangled>>8);
  return mangled;
}

__forceinline unsigned int uint_noise1D(int position, unsigned int seed)
{
  constexpr unsigned int BIT_NOISE1 = 0x68E31DA4;
  constexpr unsigned int BIT_NOISE2 = 0xB5297A4D;
  constexpr unsigned int BIT_NOISE3 = 0x1B56C4E9;
  unsigned int mangled = position;
  mangled *= BIT_NOISE1;
  mangled += seed;
  mangled ^= (mangled>>8);
  mangled += BIT_NOISE2;
  mangled ^= (mangled<<8);
  mangled *= BIT_NOISE3;
  mangled ^= (mangled>>8);
  return mangled;
}

__forceinline unsigned int uint_noise2D(int posX, int posY, unsigned int seed)
{
  constexpr int PRIME_NUMBER = 198491317;
  return uint_noise1D(posX + (PRIME_NUMBER*posY), seed);
}

__forceinline unsigned int uint_noise3D(int posX, int posY, int posZ, unsigned int seed)
{
  constexpr int PRIME_NUMBER1 = 198491317;
  constexpr int PRIME_NUMBER2 = 6542989;
  return uint_noise1D(posX + (PRIME_NUMBER1*posY) + (PRIME_NUMBER2*posZ), seed);
}
