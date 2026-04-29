// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// wrapper for vk buffer alignment handling

#include <ska_hash_map/flat_hash_map2.hpp>
#include "vulkan_api.h"

namespace drv3d_vulkan
{

class BufferProps
{
  BufferProps(const BufferProps &);
  BufferProps &operator=(const BufferProps &);

  struct Prop
  {
    VkDeviceSize align;
    uint32_t memoryTypeMask;
  };

  ska::flat_hash_map<VkBufferUsageFlags, Prop> perUsageFlags;
  Prop calculateForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage);

public:
  BufferProps() {}
  ~BufferProps() {}

  VkDeviceSize minimal = 0;
  VkDeviceSize getAlignForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage);
  uint32_t getMemoryTypeMaskForUsageAndFlags(VkBufferCreateFlags flags, VkBufferUsageFlags usage);
  uint32_t alignSize(uint32_t size) const { return (size + (minimal - 1)) & ~(minimal - 1); }

  void init();
  void shutdown();
};

} // namespace drv3d_vulkan
