// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <hash/BLAKE3/blake3.h>
#include <cstring>

struct CryptoHash
{
  uint8_t data[32];
  bool operator==(const CryptoHash &a) const { return memcmp(data, a.data, sizeof(data)) == 0; }
};
struct CryptoHasher
{
  blake3_hasher hasher;
  CryptoHasher() { blake3_hasher_init(&hasher); }
  explicit CryptoHasher(const CryptoHash &key) { blake3_hasher_init_keyed(&hasher, key.data); }
  void update(const void *data, size_t len) { blake3_hasher_update(&hasher, data, len); }
  template <class T>
  void update(const T &d)
  {
    blake3_hasher_update(&hasher, &d, sizeof(T));
  }
  void update(const char *s) { blake3_hasher_update(&hasher, s, strlen(s)); }
  CryptoHash hash() const
  {
    CryptoHash ret;
    blake3_hasher_finalize(&hasher, ret.data, sizeof(ret.data));
    return ret;
  }
};

static inline unsigned int get_hash(const char *string)
{
  if (!string)
    return 0;

  unsigned int hash = 0;
  for (const char *pos = string; *pos; pos++)
    hash = ((hash << 2) ^ *pos) ^ (hash >> 30);

  return hash ^ (hash >> 16);
}
