//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_stdint.h>
#include <util/dag_hash.h>
#include <hash/wyhash.h>
#include <osApiWrappers/dag_localConv.h>

template <bool ignore_case, typename hash_t = uint32_t>
class DefaultOAHasher
{
public:
  static hash_t hash_data(const char *s, size_t len)
  {
    hash_t ret;
    if (ignore_case)
    {
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      ret = FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::offset_basis;
      for (size_t i = 0; i < len; ++i)
      {
        hash_t c = to_lower_lut[(uint8_t)s[i]];
        ret = (ret * FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::prime) ^ c;
      }
    }
    else
    {
      ret = (hash_t)wyhash(s, len, 1);
    }
    return ret ? ret : (hash_t(1) << (sizeof(hash_t) * 8 - 1));
  }
};

template <bool ignore_case, typename hash_t = uint32_t>
class FNV1OAHasher
{
public:
  static hash_t hash_data(const char *s, size_t len)
  {
    hash_t ret;
    if (ignore_case)
    {
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      ret = FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::offset_basis;
      for (size_t i = 0; i < len; ++i)
      {
        hash_t c = to_lower_lut[(uint8_t)s[i]];
        ret = (ret * FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::prime) ^ c;
      }
    }
    else
    {
      ret = FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::offset_basis;
      for (size_t i = 0; i < len; ++i)
      {
        hash_t c = (uint8_t)s[i];
        ret = (ret * FNV1Params < sizeof(hash_t) == 8 ? 64 : 32 > ::prime) ^ c;
      }
    }
    return ret ? ret : 0x80000000;
  }
};
