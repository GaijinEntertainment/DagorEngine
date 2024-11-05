// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// vk format information wrapper

#include "vulkan_api.h"

namespace drv3d_vulkan
{

struct FormatUtil
{
  bool support(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags,
    VkSampleCountFlags samples);
  VkFormatFeatureFlags features(VkFormat format);
  VkFormatFeatureFlags buffer_features(VkFormat format);
  VkSampleCountFlags samples(VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage);
};

} // namespace drv3d_vulkan
