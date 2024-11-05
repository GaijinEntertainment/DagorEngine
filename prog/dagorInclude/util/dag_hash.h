//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <stdint.h>

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
  static constexpr uint64_t offset_basis = 14695981039346656037LU;
  static constexpr uint64_t prime = 1099511628211;
};

template <int HashBits>
using HashVal = typename FNV1Params<HashBits>::HashVal;

template <int HashBits>
static inline HashVal<HashBits> fnv1_step(uint32_t c, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return (result * FNV1Params<HashBits>::prime) ^ c;
}
template <int HashBits>
static inline HashVal<HashBits> fnv1a_step(uint32_t c, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return (result ^ c) * FNV1Params<HashBits>::prime;
}

#if (defined(_MSC_VER) && _MSC_VER > 1900) || __cplusplus >= 201402L // CPP14_CONSTEXPR
template <int HashBits>
constexpr HashVal<HashBits> str_hash_fnv1(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  HashVal<HashBits> c = 0;
  while ((c = *s++) != 0)
    result = (result * FNV1Params<HashBits>::prime) ^ c;
  return result;
}

template <int HashBits>
constexpr HashVal<HashBits> mem_hash_fnv1(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  for (size_t i = 0; i < len; ++i)
  {
    HashVal<HashBits> c = b[i];
    result = (result * FNV1Params<HashBits>::prime) ^ c;
  }
  return result;
}

#else
template <int HashBits>
constexpr HashVal<HashBits> mem_hash_fnv1_recursive(const char *b, size_t len,
  HashVal<HashBits> h = FNV1Params<HashBits>::offset_basis)
{
  return len == 0 ? h : mem_hash_fnv1_recursive<HashBits>(b + 1, len - 1, (h * FNV1Params<HashBits>::prime) ^ HashVal<HashBits>(*b));
}

template <int HashBits>
constexpr HashVal<HashBits> str_hash_fnv1_recursive(const char *s, HashVal<HashBits> h = FNV1Params<HashBits>::offset_basis)
{
  return *s == 0 ? h : str_hash_fnv1_recursive<HashBits>(s + 1, (h * FNV1Params<HashBits>::prime) ^ HashVal<HashBits>(*s));
}

template <int HashBits>
constexpr HashVal<HashBits> str_hash_fnv1(const char *str, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return str_hash_fnv1_recursive<HashBits>(str, result);
}

template <int HashBits>
constexpr HashVal<HashBits> mem_hash_fnv1(const char *bytes, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return mem_hash_fnv1_recursive<HashBits>(bytes, len, result);
}

#endif // CPP14_CONSTEXPR

// by default use 32-bit FNV1
constexpr HashVal<32> str_hash_fnv1(const char *s) { return str_hash_fnv1<32>(s); }

constexpr HashVal<32> mem_hash_fnv1(const char *b, size_t len) { return mem_hash_fnv1<32>(b, len); }

inline uint32_t hash_int(uint32_t x)
{
  x ^= x >> 16;
  x *= UINT32_C(0x21f0aaad);
  x ^= x >> 15;
  x *= UINT32_C(0xd35a2d97);
  x ^= x >> 15;
  return x;
}

constexpr HashVal<32> operator"" _h(const char *str, size_t len) { return mem_hash_fnv1<32>(str, len); }

template <class T>
struct Hash;

// Hash functor for using in std/eastl containers. Still constexpr for const char* and may
// calculate hash for strings in compile-time. obj["mykey"] <- here hash will be
// calculated in compile-time then compiling with optimization enabled
template <>
struct Hash<const char *>
{
  constexpr HashVal<32> operator()(const char *str) const { return str_hash_fnv1(str); }
};

typedef Hash<const char *> HashFNV1;
