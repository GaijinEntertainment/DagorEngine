// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler.h"

#include <drv/3d/dag_sampler.h>
#include "driver.h"
#include "drv_log_defs.h"
#include "validation.h"

using namespace drv3d_dx11;

namespace drv3d_dx11
{

dag::RelocatableFixedVector<SamplerKey, SAMPLER_KEYS_INPLACE_COUNT> g_sampler_keys;
OSSpinlock g_sampler_keys_mtx;

void close_samplers()
{
  if (resetting_device_now)
    return;
  SamplerKeysAutoLock l;
  g_sampler_keys.clear();
}

} // namespace drv3d_dx11
static SamplerKey makeKey(const d3d::SamplerInfo &sampler_info)
{
  SamplerKey key;
  memset(&key, 0, sizeof(key));

  key.borderColor = sampler_info.border_color;
  key.lodBias = sampler_info.mip_map_bias;

  key.addrU = uint32_t(sampler_info.address_mode_u);
  key.addrV = uint32_t(sampler_info.address_mode_v);
  key.addrW = uint32_t(sampler_info.address_mode_w);

  key.anisotropyLevel = uint32_t(sampler_info.anisotropic_max);
  key.texFilter = uint32_t(sampler_info.filter_mode);
  key.mipFilter = uint32_t(sampler_info.mip_map_mode);

  return key;
}

NO_UBSAN d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  SamplerKey samplerKey = makeKey(sampler_info);

  SamplerKeysAutoLock lock;
  auto toHandle = [](SamplerKey *k) {
    return SamplerHandle(eastl::distance(g_sampler_keys.begin(), k) + (uint64_t(INVALID_SAMPLER_HANDLE) == 0));
  };

  for (auto &key : g_sampler_keys)
    if (key == samplerKey)
      return toHandle(&key);

  auto nk = &g_sampler_keys.push_back(samplerKey);
  if (DAGOR_UNLIKELY(g_sampler_keys.size() == SAMPLER_KEYS_INPLACE_COUNT))
  {
    // We don't expect the number of unique samplers to be large. It is technically supported, but should not happen with real-world
    // use, unless there's some bug.
    D3D_ERROR("DX11: created %u unique samplers. This likely indicates a problem.", g_sampler_keys.size());
  }
  return toHandle(nk);
}
