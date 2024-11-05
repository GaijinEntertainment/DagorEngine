// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_sampler.h>
#include "driver.h"
#include "sampler.h"
#include "texture.h"

using namespace drv3d_dx11;

namespace drv3d_dx11
{

dag::RelocatableFixedVector<SamplerKey, SAMPLER_KEYS_INPLACE_COUNT> g_sampler_keys;
WinCritSec g_sampler_keys_cs;

void close_samplers()
{
  if (!resetting_device_now)
    g_sampler_keys.clear();
}

} // namespace drv3d_dx11

SamplerKey::SamplerKey(const BaseTex *tex)
{
  borderColor = tex->borderColor;
  lodBias = tex->lodBias;
  k = tex->key & BaseTextureImpl::SAMPLER_KEY_MASK;
}

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

d3d::SamplerHandle d3d::request_sampler(const d3d::SamplerInfo &sampler_info)
{
  SamplerKey samplerKey = makeKey(sampler_info);
  SamplerKey *ptr = nullptr;

  SamplerKeysAutoLock lock;
  for (auto &key : g_sampler_keys)
    if (key == samplerKey)
    {
      ptr = &key;
      break;
    }

  if (!ptr)
  {
    ptr = &g_sampler_keys.push_back(samplerKey);
    if (DAGOR_UNLIKELY(g_sampler_keys.size() == SAMPLER_KEYS_INPLACE_COUNT))
    {
      // We don't expect the number of unique samplers to be large. It is technically supported, but should not happen with real-world
      // use, unless there's some bug.
      logerr("DX11: created %" PRIu32 " unique samplers. This probably indicates a problem.", g_sampler_keys.size());
    }
  }

  return SamplerHandle(ptr - g_sampler_keys.begin() + 1);
}
