//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point2.h>

namespace objgenerator
{

__forceinline int rnd(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  return int((a >> 16) & 0x7FFF);
}

__forceinline float frnd(int &s) { return rnd(s) / 32768.f; }

__forceinline float srnd(int &s) { return (rnd(s) * 2 - 32768) / 32768.f; }

__forceinline void rnd_ivec(int &seed, int &x, int &y, int &z)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  z = int((a >> 16) & 0x7FFF);
  y = int((b >> 16) & 0x7FFF);
  x = int((c >> 16) & 0x7FFF);
  seed = (int)c;
}

__forceinline void rnd_fvec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  rnd_ivec(seed, ix, iy, iz);
  x = float(ix) / 32768.f;
  y = float(iy) / 32768.f;
  z = float(iz) / 32768.f;
}

__forceinline void rnd_svec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  rnd_ivec(seed, ix, iy, iz);
  x = float(ix * 2 - 32768) / 32768.f;
  y = float(iy * 2 - 32768) / 32768.f;
  z = float(iz * 2 - 32768) / 32768.f;
}

static __forceinline void rnd_ivec2_mbit(int mask, int &seed, int &x, int &y)
{
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & mask;
  y = signed(b >> 16) & mask;
  seed = (int)b;
}

static __forceinline void rnd_ivec2(int &seed, int &x, int &y)
{
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  x = signed(a >> 16) & 0x7FFF;
  y = signed(b >> 16) & 0x7FFF;
  seed = (int)b;
}

static __forceinline void rnd_fvec2(int &seed, float &x, float &y)
{
  int ix, iy;
  rnd_ivec2(seed, ix, iy);
  y = iy / 32768.f;
  x = ix / 32768.f;
}

__forceinline void rnd_ivec4(int &seed, int &x, int &y, int &z, int &w)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  d = c * 0x41C64E6D + 0x3039;
  w = int((a >> 16) & 0x7FFF);
  z = int((b >> 16) & 0x7FFF);
  y = int((c >> 16) & 0x7FFF);
  x = int((d >> 16) & 0x7FFF);
  seed = (int)d;
}

__forceinline void rnd_fvec4(int &seed, float &x, float &y, float &z, float &w)
{
  int ix, iy, iz, iw;
  rnd_ivec4(seed, ix, iy, iz, iw);
  x = float(ix) / 32768.f;
  y = float(iy) / 32768.f;
  z = float(iz) / 32768.f;
  w = float(iw) / 32768.f;
}

__forceinline void skip_rnd_ivec4(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  c = (unsigned)b * 0x41C64E6D + 0x3039;
  d = (unsigned)c * 0x41C64E6D + 0x3039;
  seed = (int)d;
}

static __forceinline void skip6_rnd(int &seed)
{
  unsigned int a = (unsigned)seed * 0x41C64E6D + 0x3039, b, c, d, e, f;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  c = (unsigned)b * 0x41C64E6D + 0x3039;
  d = (unsigned)c * 0x41C64E6D + 0x3039;
  e = (unsigned)d * 0x41C64E6D + 0x3039;
  f = (unsigned)e * 0x41C64E6D + 0x3039;
  seed = (int)f;
}

__forceinline float getRandom(int &s, const Point2 &r) { return r.x + r.y * srnd(s); }

} // namespace objgenerator
