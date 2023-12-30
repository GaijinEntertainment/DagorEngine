#include "sampler_resource.h"

#include "device.h"

using namespace drv3d_vulkan;

void SamplerResource::createVulkanObject()
{
  samplerInfo = drv3d_vulkan::get_device().createSampler(desc.state);
  setHandle(generalize(samplerInfo.sampler[0]));
}

void SamplerResource::destroyVulkanObject()
{
  auto &VkDevice = drv3d_vulkan::get_device().getVkDevice();
  VkDevice.vkDestroySampler(VkDevice.get(), samplerInfo.sampler[0], nullptr);
  VkDevice.vkDestroySampler(VkDevice.get(), samplerInfo.sampler[1], nullptr);
}