// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sampler_resource.h"
#include "globals.h"
#include "resource_manager.h"
#include "sampler_cache.h"

using namespace drv3d_vulkan;

void SamplerResource::createVulkanObject()
{
  samplerInfo = Globals::samplers.create(desc.state);
  setHandle(generalize(samplerInfo.sampler[0]));
}

void SamplerResource::destroyVulkanObject()
{
  auto &VkDevice = drv3d_vulkan::Globals::VK::dev;
  VkDevice.vkDestroySampler(VkDevice.get(), samplerInfo.sampler[0], nullptr);
  VkDevice.vkDestroySampler(VkDevice.get(), samplerInfo.sampler[1], nullptr);
  setHandle(generalize(Handle()));
}
