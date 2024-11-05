// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// variant-like wrapper for vulkan descriptor info

#include <vulkan/vulkan.h>
#include <vk_wrapped_handles.h>
#include <debug/dag_assert.h>

namespace drv3d_vulkan
{

struct VkAnyDescriptorInfo
{
  union
  {
    VkDescriptorImageInfo image;
    VkDescriptorBufferInfo buffer;
    VkBufferView texelBuffer;
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
    VkAccelerationStructureKHR raytraceAccelerationStructure;
#endif
  };

  enum
  {
    TYPE_NULL = 0,
    TYPE_IMG = 1,
    TYPE_BUF = 2,
    TYPE_BUF_VIEW = 3,
    TYPE_AS = 4
  };
  uint8_t type : 3;

  struct ReducedImageInfo
  {
    VkImageView view;
    VkImageLayout layout;
  };

  VkAnyDescriptorInfo &operator+=(const ReducedImageInfo &i)
  {
    image.imageLayout = i.layout;
    image.imageView = i.view;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(VulkanImageViewHandle i)
  {
    image.imageView = i;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(VulkanSamplerHandle i)
  {
    G_ASSERTF(type == TYPE_IMG, "vulkan: setting sampler for non image descriptor");
    image.sampler = i;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(VkImageLayout i)
  {
    G_ASSERTF(type == TYPE_IMG, "vulkan: setting layout for non image descriptor");
    image.imageLayout = i;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(const VkDescriptorImageInfo &i)
  {
    image = i;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(const VkDescriptorBufferInfo &i)
  {
    buffer = i;
    type = TYPE_BUF;
    return *this;
  };

  VkAnyDescriptorInfo &operator+=(VulkanBufferViewHandle i)
  {
    texelBuffer = i;
    type = TYPE_BUF_VIEW;
    return *this;
  };

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  VkAnyDescriptorInfo &operator+=(VkAccelerationStructureKHR as)
  {
    raytraceAccelerationStructure = as;
    type = TYPE_AS;
    return *this;
  }
#endif

  void clear() { type = TYPE_NULL; }
};

} // namespace drv3d_vulkan
