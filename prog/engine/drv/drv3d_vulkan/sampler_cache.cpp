// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler_cache.h"
#include "vulkan_device.h"
#include "driver_config.h"
#include "physical_device_set.h"
#include "resource_manager.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

static constexpr size_t SAMPLERS_MAX_EXPECTED_COUNT = 128;

void SamplerCache::init()
{
  defaultSamplerState = SamplerState::make_default();
  defaultSampler = Globals::Mem::res.alloc<SamplerResource>({SamplerDescription(defaultSamplerState)});
}

void SamplerCache::shutdown()
{
  defaultSampler = nullptr;
  // actual sampler resources will be cleaned up in resource manager on shutdown
  samplerResources.clear();
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
  sci.compareEnable = state.isCompare ? VK_TRUE : VK_FALSE;
  VULKAN_EXIT_ON_FAIL(Globals::VK::dev.vkCreateSampler(Globals::VK::dev.get(), &sci, VKALLOC(sampler), ptr(info.handle)));

  if (Globals::cfg.debugLevel > 0)
  {
    Globals::Dbg::naming.setSamplerName(info.handle, String(64, "SPL %016llX", state.wrapper.value));
  }

  return info;
}

SamplerResource *SamplerCache::getResource(SamplerState state)
{
  if (state == defaultSamplerState)
    return defaultSampler;

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
