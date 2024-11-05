// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// wrapper for vk buffer alignment handling

#include <ska_hash_map/flat_hash_map2.hpp>
#include "vulkan_api.h"

namespace drv3d_vulkan
{

class BufferAlignment
{
  BufferAlignment(const BufferAlignment &);
  BufferAlignment &operator=(const BufferAlignment &);

  ska::flat_hash_map<VkBufferUsageFlags, VkDeviceSize> perUsageFlags;
  VkDeviceSize calculateForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage);

public:
  BufferAlignment() {}
  ~BufferAlignment() {}

  VkDeviceSize minimal = 0;
  VkDeviceSize getForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage);
  uint32_t alignSize(uint32_t size) const { return (size + (minimal - 1)) & ~(minimal - 1); }

  void init();
  void shutdown();
};

} // namespace drv3d_vulkan
