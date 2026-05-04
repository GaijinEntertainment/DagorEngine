//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <supp/dag_define_KRNLIMP.h>

namespace dagor_random
{

#define DAGOR_RAND_MAX 0x7FFFFFFF // INT_MAX

__forceinline unsigned mutate_seed(int &seed, unsigned nr = 1)
{
  unsigned s = (unsigned)seed + 0x9E3779B9u; // Weyl sequence (golden ratio)
  seed = (int)s;
  unsigned long long t = (unsigned long long)(s ^ (s >> 16)) * 0x21f0aaadu; // mum hash round 1
  unsigned z = (unsigned)(t >> 32) ^ (unsigned)t;
  if (nr >= 2) // mum hash round 2
  {
    t = (unsigned long long)(z ^ (z >> 15)) * 0x735a2d97u;
    z = (unsigned)(t >> 32) ^ (unsigned)t;
  }
  return z;
}

__forceinline int _rnd(int &seed) { return mutate_seed(seed) >> 1; } // Returns [0, DAGOR_RAND_MAX]
__forceinline float _frnd(int &s)                                    // Returns [0, 1.0)
{
  return __builtin_bit_cast(float, /*1.0*/ 0x3f800000 | (mutate_seed(s) >> 9)) - 1.f;
}
__forceinline float _srnd(int &s) { return _frnd(s) * 2.f - 1.f; } // Returns (-1.0, 1.0)
__forceinline void _rnd_svec(int &seed, float &x, float &y, float &z) { x = _srnd(seed), y = _srnd(seed), z = _srnd(seed); }
__forceinline float _rnd_float(int &seed, float a, float b) { return a + (b - a) * _frnd(seed); }                 // Returns [a, b)
__forceinline int _rnd_bound(int &seed, unsigned n) { return ((unsigned long long)mutate_seed(seed) * n) >> 32; } // Returns [0, n)
__forceinline int _rnd_range(int &seed, int a, int b) { return a + _rnd_bound(seed, b - a); }                     // Returns [a, b)
__forceinline int _rnd_int(int &seed, int a, int b) { return _rnd_range(seed, a, b + 1); }                        // Returns [a, b]

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
  unsigned x = (a >> 16) & 0x7FFF, t = x >> 7;
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
