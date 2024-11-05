// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "vk_format_utils.h"
#include "globals.h"
#include "vulkan_device.h"
#include "driver.h"
#include "physical_device_set.h"

using namespace drv3d_vulkan;

bool FormatUtil::support(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
  VkSampleCountFlags samples)
{
  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(Globals::VK::dev.getInstance().vkGetPhysicalDeviceImageFormatProperties(
    Globals::VK::phy.device, format, type, tiling, usage, flags, &properties));
  if (VULKAN_FAIL(result))
    return false;
  // needed ?
  if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
    return properties.maxArrayLayers >= 6;
  if (!(samples & properties.sampleCounts))
    return false;
  return true;
}

VkFormatFeatureFlags FormatUtil::features(VkFormat format)
{
  VkFormatProperties props;
  VULKAN_LOG_CALL(Globals::VK::dev.getInstance().vkGetPhysicalDeviceFormatProperties(Globals::VK::phy.device, format, &props));
  return props.optimalTilingFeatures;
}

VkFormatFeatureFlags FormatUtil::buffer_features(VkFormat format)
{
  VkFormatProperties props;
  VULKAN_LOG_CALL(Globals::VK::dev.getInstance().vkGetPhysicalDeviceFormatProperties(Globals::VK::phy.device, format, &props));
  return props.bufferFeatures;
}

VkSampleCountFlags FormatUtil::samples(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage)
{
  VkImageFormatProperties properties;
  VkResult result = VULKAN_CHECK_RESULT(Globals::VK::dev.getInstance().vkGetPhysicalDeviceImageFormatProperties(
    Globals::VK::phy.device, format, type, tiling, usage, 0, &properties));

  return VULKAN_OK(result) ? properties.sampleCounts : 0;
}
