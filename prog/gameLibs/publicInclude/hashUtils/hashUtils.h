//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_localConv.h>

#include <EASTL/array.h>
#include <util/dag_strUtil.h>

#define OPENSSL_SUPPRESS_DEPRECATED
#include <openssl/sha.h>
#undef OPENSSL_SUPPRESS_DEPRECATED

#include <hash/BLAKE3/blake3.h>

template <typename Hasher>
struct GenericHash
{
  enum class StringMode
  {
    TO_LOWER = 1
  };

  static constexpr size_t Length = Hasher::Length;
  static constexpr size_t LengthStr = Length * 2 + 1;

  using HashType = GenericHash<Hasher>;
  using Value = typename Hasher::Value;
  using ValueStr = eastl::array<char, LengthStr>;

  ValueStr valueStr;

  GenericHash()
  {
    valueStr[0] = 0;
    valueStr[LengthStr - 1] = 0;
  }

  GenericHash(const HashType &) = default;
  GenericHash(HashType &&) = default;

  GenericHash &operator=(const HashType &) = default;
  GenericHash &operator=(HashType &&) = default;

  GenericHash(const Value &value) { data_to_str_hex_buf(valueStr.data(), valueStr.size(), value.data(), value.size()); }

  GenericHash(const char *value) : GenericHash()
  {
    if (value && *value)
    {
      const size_t len = ::strlen(value);
      G_ASSERT(len == LengthStr - 1);
      if (len == LengthStr - 1)
        ::memcpy(valueStr.data(), value, len);
    }
  }

  GenericHash(const char *value, StringMode mode) : GenericHash(value)
  {
    if (value && *value)
    {
      if (mode == StringMode::TO_LOWER)
        dd_strlwr(valueStr.data());
    }
  }

  char *data() { return valueStr.data(); }

  const char *c_str() const { return valueStr.data(); }

  size_t length() const { return LengthStr - 1; }

  explicit operator bool() const { return valueStr[0] != 0; }

  bool operator==(const HashType &rhs) const { return eastl::equal(valueStr.cbegin(), valueStr.cend(), rhs.valueStr.cbegin()); }

  bool operator!=(const HashType &rhs) const { return !(operator==(rhs)); }

  static HashType hash(const void *data, int size)
  {
    Hasher hasher;
    hasher.update((const uint8_t *)data, size);
    return hasher.finalize();
  }
};

template <size_t N>
struct GenericHasher
{
  static constexpr size_t Length = N;

  using Value = eastl::array<uint8_t, Length>;
};

struct SHA1Hasher : GenericHasher<SHA_DIGEST_LENGTH>
{
  SHA_CTX ctx;

  SHA1Hasher() { SHA1_Init(&ctx); }

  void update(const uint8_t *data, size_t size) { SHA1_Update(&ctx, data, size); }

  Value finalize()
  {
    Value value;
    SHA1_Final(value.data(), &ctx);
    return value;
  }
};

template <size_t N>
struct Blake3Hasher : GenericHasher<N>
{
  using Value = typename GenericHasher<N>::Value;

  blake3_hasher ctx;

  Blake3Hasher() { blake3_hasher_init(&ctx); }

  void update(const uint8_t *data, size_t size) { blake3_hasher_update(&ctx, data, size); }

  Value finalize()
  {
    Value value;
    blake3_hasher_finalize(&ctx, value.data(), value.size());
    return value;
  }
};

using SHA1Hash = GenericHash<SHA1Hasher>;
using Blake3Hash = GenericHash<Blake3Hasher<BLAKE3_OUT_LEN>>;
using Blake3ShortHash = GenericHash<Blake3Hasher<SHA_DIGEST_LENGTH>>;
