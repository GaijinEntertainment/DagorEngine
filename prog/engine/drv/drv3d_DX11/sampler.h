// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_relocatableFixedVector.h>
#include <osApiWrappers/dag_critSec.h>
#include <drv/3d/dag_sampler.h>
#include "keyMapPools.h"

namespace drv3d_dx11
{

class BaseTex;

struct SamplerKey
{
  E3DCOLOR borderColor;
  float lodBias;
  union
  {
    uint32_t k;
    struct
    {
      uint32_t anisotropyLevel : 5;
      uint32_t addrU : 3;
      uint32_t addrV : 3;
      uint32_t addrW : 3;
      uint32_t texFilter : 3;
      uint32_t mipFilter : 3;
    };
  };

  SamplerKey() = default;
  SamplerKey(const BaseTex *tex);
  bool operator==(const SamplerKey &other) { return k == other.k && lodBias == other.lodBias && borderColor == other.borderColor; }
  uint32_t getHash() const { return hash32shiftmult(k); }
};

static constexpr size_t SAMPLER_KEYS_INPLACE_COUNT = 128;
extern dag::RelocatableFixedVector<SamplerKey, SAMPLER_KEYS_INPLACE_COUNT> g_sampler_keys;
extern WinCritSec g_sampler_keys_cs;

struct SamplerKeysAutoLock
{
  SamplerKeysAutoLock() { g_sampler_keys_cs.lock(); }
  ~SamplerKeysAutoLock() { g_sampler_keys_cs.unlock(); }
};

} // namespace drv3d_dx11
