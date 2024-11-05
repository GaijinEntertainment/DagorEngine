//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

namespace dagor_random
{

#define DAGOR_RAND_MAX 32767

__forceinline int _rnd(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  return int((a >> 16) & DAGOR_RAND_MAX);
}

__forceinline float _frnd(int &s) { return float(_rnd(s)) / float(DAGOR_RAND_MAX + 1); }

__forceinline float _srnd(int &s) { return float(_rnd(s) * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1); }

__forceinline void _rnd_ivec(int &seed, int &x, int &y, int &z)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  z = int((a >> 16) & DAGOR_RAND_MAX);
  y = int((b >> 16) & DAGOR_RAND_MAX);
  x = int((c >> 16) & DAGOR_RAND_MAX);
  seed = (int)c;
}

__forceinline void _rnd_fvec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  x = float(ix) / float(DAGOR_RAND_MAX + 1);
  y = float(iy) / float(DAGOR_RAND_MAX + 1);
  z = float(iz) / float(DAGOR_RAND_MAX + 1);
}

__forceinline void _rnd_svec(int &seed, float &x, float &y, float &z)
{
  int ix, iy, iz;
  _rnd_ivec(seed, ix, iy, iz);
  x = float(ix * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  y = float(iy * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  z = float(iz * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
}

__forceinline void _skip_rnd_ivec4(int &seed)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = (unsigned)a * 0x41C64E6D + 0x3039;
  c = (unsigned)b * 0x41C64E6D + 0x3039;
  d = (unsigned)c * 0x41C64E6D + 0x3039;
  seed = (int)d;
}

__forceinline void _rnd_ivec4(int &seed, int &x, int &y, int &z, int &w)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039, b, c, d;
  b = a * 0x41C64E6D + 0x3039;
  c = b * 0x41C64E6D + 0x3039;
  d = c * 0x41C64E6D + 0x3039;
  w = int((a >> 16) & DAGOR_RAND_MAX);
  z = int((b >> 16) & DAGOR_RAND_MAX);
  y = int((c >> 16) & DAGOR_RAND_MAX);
  x = int((d >> 16) & DAGOR_RAND_MAX);
  seed = (int)d;
}

__forceinline void _rnd_fvec4(int &seed, float &x, float &y, float &z, float &w)
{
  int ix, iy, iz, iw;
  _rnd_ivec4(seed, ix, iy, iz, iw);
  x = float(ix) / float(DAGOR_RAND_MAX + 1);
  y = float(iy) / float(DAGOR_RAND_MAX + 1);
  z = float(iz) / float(DAGOR_RAND_MAX + 1);
  w = float(iw) / float(DAGOR_RAND_MAX + 1);
}

__forceinline void _rnd_svec4(int &seed, float &x, float &y, float &z, float &w)
{
  int ix, iy, iz, iw;
  _rnd_ivec4(seed, ix, iy, iz, iw);
  x = float(ix * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  y = float(iy * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  z = float(iz * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
  w = float(iw * 2 - (DAGOR_RAND_MAX + 1)) / float(DAGOR_RAND_MAX + 1);
}

__forceinline float _rnd_float(int &seed, float a, float b) { return a + (b - a) * _frnd(seed); }
__forceinline int _rnd_int(int &seed, int a, int b) { return a + (b - a + 1) * _rnd(seed) / (DAGOR_RAND_MAX + 1); }

//
// Gaussian random number
//
struct LineIntTbl
{
  float k_div_128, b;
};
extern KRNLIMP LineIntTbl _gauss_table[3][256];

__forceinline float _gauss_rnd(int &seed, int n = 0)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  unsigned x = (a >> 16) & DAGOR_RAND_MAX, t = x >> 7;
  return _gauss_table[n][t].b + _gauss_table[n][t].k_div_128 * float(x & 0x7F);
}

__forceinline float _gauss_rnd_fast(int &seed, int n = 0)
{
  unsigned int a = ((unsigned)seed) * 0x41C64E6D + 0x3039;
  seed = (int)a;
  unsigned t = (a >> 23) & 0xFF;
  return _gauss_table[n][t].b;
}

extern KRNLIMP int g_rnd_seed;


__forceinline void set_rnd_seed(int rnd_seed) { g_rnd_seed = rnd_seed; }
__forceinline int get_rnd_seed() { return g_rnd_seed; }

__forceinline int grnd() { return _rnd(g_rnd_seed); }
__forceinline float gfrnd() { return _frnd(g_rnd_seed); }
__forceinline float gsrnd() { return _srnd(g_rnd_seed); }

__forceinline float rnd_float(float a, float b) { return _rnd_float(g_rnd_seed, a, b); }
__forceinline int rnd_int(int a, int b) { return _rnd_int(g_rnd_seed, a, b); }
__forceinline void rnd_svec(float &x, float &y, float &z) { _rnd_svec(g_rnd_seed, x, y, z); }
__forceinline float gauss_rnd(int n = 0) { return _gauss_rnd(g_rnd_seed, n); }

} // namespace dagor_random
using namespace dagor_random;
#include <supp/dag_undef_KRNLIMP.h>
