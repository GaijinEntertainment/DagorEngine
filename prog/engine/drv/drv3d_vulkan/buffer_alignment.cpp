// Copyright (C) Gaijin Games KFT.  All rights reserved.

// wrapper for vk buffer alignment handling
#include "buffer_alignment.h"
#include "driver.h"
#include "globals.h"
#include "physical_device_set.h"
#include "device_memory.h"
#include "buffer_resource.h"

using namespace drv3d_vulkan;

VkDeviceSize BufferAlignment::getForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage)
{
  G_ASSERTF(flags == 0, "vulkan: non zero buffer flags are not supported");
  G_ASSERTF(perUsageFlags.find(usage) != perUsageFlags.end(), "vulkan: unknown buffer usage flags %s", formatBufferUsageFlags(usage));
  G_UNUSED(flags);
  return perUsageFlags[usage];
}

VkDeviceSize BufferAlignment::calculateForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage)
{
  G_ASSERTF(flags == 0, "vulkan: non zero buffer flags are not supported");
  G_UNUSED(flags);
  VulkanBufferHandle tmpBuf;
  VkBufferCreateInfo bci;
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.pNext = NULL;
  bci.flags = 0;
  bci.size = 0x1000;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  bci.queueFamilyIndexCount = 0;
  bci.pQueueFamilyIndices = NULL;
  const VkResult resCode = VULKAN_CHECK_RESULT(Globals::VK::dev.vkCreateBuffer(Globals::VK::dev.get(), &bci, NULL, ptr(tmpBuf)));
  if (resCode == VK_SUCCESS)
  {
    MemoryRequirementInfo ret = get_memory_requirements(Globals::VK::dev, tmpBuf);
    Globals::VK::dev.vkDestroyBuffer(Globals::VK::dev.get(), tmpBuf, nullptr);
    return ret.requirements.alignment;
  }
  return 0;
}

void BufferAlignment::init()
{
  minimal = max<VkDeviceSize>(1, max(max(max(max(Globals::VK::phy.properties.limits.minStorageBufferOffsetAlignment,
                                               (VkDeviceSize)Globals::VK::phy.properties.limits.minMemoryMapAlignment),
                                           Globals::VK::phy.properties.limits.minTexelBufferOffsetAlignment),
                                       Globals::VK::phy.properties.limits.minUniformBufferOffsetAlignment),
                                   Globals::VK::phy.properties.limits.nonCoherentAtomSize));

#define ALIGMENT_CALC(x)                                                \
  {                                                                     \
    VkBufferUsageFlags buflags = Buffer::getUsage(Globals::VK::dev, x); \
    perUsageFlags[buflags] = calculateForUsageAndFlags(0, buflags);     \
  }

  ALIGMENT_CALC(DeviceMemoryClass::DEVICE_RESIDENT_BUFFER);
  ALIGMENT_CALC(DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER);
  ALIGMENT_CALC(DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER);
#if !_TARGET_C3
  ALIGMENT_CALC(DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER);
#endif
  ALIGMENT_CALC(DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER);

#undef ALIGMENT_CALC
}

void BufferAlignment::shutdown() { perUsageFlags.clear(); }
