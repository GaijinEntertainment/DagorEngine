// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_hash.h>
#include <debug/dag_assert.h>

template <class TStcodeStorage>
inline uint64_t calc_stcode_hash(const TStcodeStorage &stcode)
{
  HashVal<64> hash = FNV1Params<64>::offset_basis;
  for (size_t i = 0; i < stcode.size(); ++i)
  {
    const auto &codePass = stcode[i];
    G_ASSERT(codePass.size() <= (uint32_t)(-1));
    hash = fnv1a_step<64>((uint32_t)(-1), hash); // delimiter
    hash = fnv1a_step<64>((uint32_t)codePass.size(), hash);
    hash = mem_hash_fnv1<64>((const char *)codePass.data(), codePass.size() * sizeof(codePass[0]), hash);
  }
  return hash;
}