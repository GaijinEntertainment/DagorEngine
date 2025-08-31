// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler_resource.h"
#include "globals.h"
#include "resource_manager.h"
#include "sampler_cache.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

void SamplerResource::createVulkanObject()
{
  samplerInfo = Globals::samplers.create(desc.state);
  setHandle(generalize(samplerInfo.handle));
}

void SamplerResource::destroyVulkanObject()
{
  auto &VkDevice = drv3d_vulkan::Globals::VK::dev;
  VkDevice.vkDestroySampler(VkDevice.get(), samplerInfo.handle, VKALLOC(sampler));
  setHandle(generalize(Handle()));
}
