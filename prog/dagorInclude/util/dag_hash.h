//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>

#ifdef _MSC_VER
// '*': integral constant overflow. It's ok. overflow is a part of the algorithm
// It seems that for constexpr functions warning should be disabled globaly
// with pragma(push/pop) warning will be reenabled
#pragma warning(disable : 4307)
#endif

template <int HashBits>
struct FNV1Params;

template <>
struct FNV1Params<32>
{
  typedef uint32_t HashVal;
  static constexpr uint32_t offset_basis = 2166136261U;
  static constexpr uint32_t prime = 16777619;
};

template <>
struct FNV1Params<64>
{
  typedef uint64_t HashVal;
  static constexpr uint64_t offset_basis = UINT64_C(14695981039346656037);
  static constexpr uint64_t prime = 1099511628211;
};

template <int HashBits>
using HashVal = typename FNV1Params<HashBits>::HashVal;

template <int HashBits>
constexpr HashVal<HashBits> fnv1_step(uint32_t c, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return (result * FNV1Params<HashBits>::prime) ^ c;
}

template <int HashBits>
constexpr HashVal<HashBits> fnv1a_step(uint32_t c, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return (result ^ c) * FNV1Params<HashBits>::prime;
}

template <int HashBits, typename F = decltype(fnv1_step<HashBits>)>
constexpr HashVal<HashBits> str_hash_fnv1(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis,
  F step_cb = fnv1_step<HashBits>)
{
  uint32_t c;
  while ((c = (uint8_t)*s++) != 0)
    result = step_cb(c, result);
  return result;
}

template <int HashBits, typename F = decltype(fnv1_step<HashBits>)>
constexpr HashVal<HashBits> mem_hash_fnv1(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis,
  F step_cb = fnv1_step<HashBits>)
{
  for (size_t i = 0; i < len; ++i)
    result = step_cb((uint8_t)b[i], result);
  return result;
}

constexpr HashVal<32> str_hash_fnv1(const char *s) { return str_hash_fnv1<32>(s); }
constexpr HashVal<32> mem_hash_fnv1(const char *b, size_t len) { return mem_hash_fnv1<32>(b, len); }
constexpr HashVal<32> str_hash_fnv1a(const char *s) { return str_hash_fnv1<32>(s, FNV1Params<32>::offset_basis, fnv1a_step<32>); }
constexpr HashVal<32> mem_hash_fnv1a(const char *b, size_t len)
{
  return mem_hash_fnv1<32>(b, len, FNV1Params<32>::offset_basis, fnv1a_step<32>);
}

constexpr HashVal<32> operator""_h(const char *str, size_t len) { return mem_hash_fnv1<32>(str, len); }


namespace ska
{
struct power_of_two_hash_policy;
}

template <typename T>
struct Hash
{
  typedef ska::power_of_two_hash_policy hash_policy;
  constexpr HashVal<32> operator()(const T &s) const
  {
    if constexpr (requires { s.c_str(); })
      return str_hash_fnv1a(s.c_str());
    else
      return mem_hash_fnv1a(s.data(), s.size());
  }
};

template <>
struct Hash<const char *>
{
  typedef ska::power_of_two_hash_policy hash_policy;
  constexpr HashVal<32> operator()(const char *str) const { return str_hash_fnv1a(str); }
};

template <typename T = const char *>
using HashFNV1A = Hash<T>;

inline uint32_t hash_int(uint32_t x)
{
  x ^= x >> 16;
  x *= UINT32_C(0x21f0aaad);
  x ^= x >> 15;
  x *= UINT32_C(0xd35a2d97);
  x ^= x >> 15;
  return x;
}
