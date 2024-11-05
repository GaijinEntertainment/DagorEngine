// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler_cache.h"
#include "vulkan_device.h"
#include "driver_config.h"
#include "physical_device_set.h"
#include "resource_manager.h"

using namespace drv3d_vulkan;

static constexpr size_t SAMPLERS_MAX_EXPECTED_COUNT = 128;

void SamplerCache::shutdown()
{
  for (int i = 0; i < samplers.size(); ++i)
  {
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySampler(Globals::VK::dev.get(), samplers[i]->sampler[0], NULL));
    VULKAN_LOG_CALL(Globals::VK::dev.vkDestroySampler(Globals::VK::dev.get(), samplers[i]->sampler[1], NULL));
  }
  clear_all_ptr_items_and_shrink(samplers);

  // actual sampler resources will be cleaned up in resource manager on shutdown
  samplerResources.clear();
}

SamplerInfo *SamplerCache::get(SamplerState state)
{
  for (int i = 0; i < samplers.size(); ++i)
    if (samplers[i]->state == state)
      return samplers[i];

  SamplerInfo *info = new SamplerInfo;
  *info = create(state);

  samplers.push_back(info);
  return info;
}

SamplerInfo SamplerCache::create(SamplerState state)
{
  VkSamplerCreateInfo sci;
  sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sci.pNext = NULL;
  sci.flags = 0;
  sci.magFilter = sci.minFilter = state.getFilter();
  sci.addressModeU = state.getU();
  sci.addressModeV = state.getV();
  sci.addressModeW = state.getW();
  if (Globals::cfg.has.anisotropicSampling)
  {
    sci.maxAnisotropy = clamp(state.getAniso(), 1.f, Globals::VK::phy.properties.limits.maxSamplerAnisotropy);
  }
  else
  {
    sci.maxAnisotropy = 1.f;
  }
  sci.anisotropyEnable = sci.maxAnisotropy > 1.f;
  sci.mipLodBias = state.getBias();
  sci.compareOp = state.getCompareOp();
  sci.minLod = -1000.f;
  sci.maxLod = 1000.f;
  sci.borderColor = state.getBorder();
  sci.unnormalizedCoordinates = VK_FALSE;

  SamplerInfo info = {};
  info.state = state;

  sci.mipmapMode = state.getMip();
  sci.compareEnable = VK_FALSE;
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSampler(Globals::VK::dev.get(), &sci, NULL, ptr(info.sampler[0])));

  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  sci.compareEnable = VK_TRUE;
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSampler(Globals::VK::dev.get(), &sci, NULL, ptr(info.sampler[1])));

  return info;
}

SamplerResource *SamplerCache::getResource(SamplerState state)
{
  WinAutoLock lock(samplerResourcesMutex);

  for (const SamplerResourcesDescPtrPair &i : samplerResources)
  {
    if (i.first == state)
      return i.second;
  }

  WinAutoLock memLock(Globals::Mem::mutex);

  SamplerResource *ret = Globals::Mem::res.alloc<SamplerResource>({SamplerDescription(state)});
  const size_t size = Globals::Mem::res.sizeAllocated<SamplerResource>();
  if (DAGOR_UNLIKELY(size == SAMPLERS_MAX_EXPECTED_COUNT))
    D3D_ERROR("vulkan: created %zu unique samplers. This probably indicates a problem.", size);

  samplerResources.push_back(eastl::make_pair(state, ret));
  return ret;
}
