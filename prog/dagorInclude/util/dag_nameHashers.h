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
    constexpr int hbits = sizeof(hash_t) * 8;
    hash_t ret;
    if constexpr (ignore_case)
    {
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      ret = FNV1Params<hbits>::offset_basis;
      for (size_t i = 0; i < len; ++i)
        ret = fnv1_step<hbits>(to_lower_lut[(uint8_t)s[i]], ret);
    }
    else
    {
      ret = (hash_t)wyhash(s, len, 1);
    }
    return ret ? ret : (hash_t(1) << (hbits - 1));
  }
};

template <bool ignore_case, typename hash_t = uint32_t>
class FNV1AOAHasher
{
public:
  static hash_t hash_data(const char *s, size_t len)
  {
    constexpr int hbits = sizeof(hash_t) * 8;
    hash_t ret;
    if constexpr (ignore_case)
    {
      const unsigned char *to_lower_lut = dd_local_cmp_lwtab;
      ret = FNV1Params<hbits>::offset_basis;
      for (size_t i = 0; i < len; ++i)
        ret = fnv1a_step<hbits>(to_lower_lut[(uint8_t)s[i]], ret);
    }
    else
      ret = mem_hash_fnv1a(s, len);
    return ret ? ret : (hash_t(1) << (hbits - 1));
  }
};
