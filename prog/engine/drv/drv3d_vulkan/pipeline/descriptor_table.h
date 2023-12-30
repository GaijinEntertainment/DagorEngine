#pragma once
#include <spirv/compiled_meta_data.h>
#include <debug/dag_assert.h>
#include <vk_wrapped_handles.h>

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

  VkAnyDescriptorInfo &operator=(const ReducedImageInfo &i)
  {
    image.imageLayout = i.layout;
    image.imageView = i.view;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(VulkanImageViewHandle i)
  {
    image.imageView = i;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(VulkanSamplerHandle i)
  {
    G_ASSERTF(type == TYPE_IMG, "vulkan: setting sampler for non image descriptor");
    image.sampler = i;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(VkImageLayout i)
  {
    G_ASSERTF(type == TYPE_IMG, "vulkan: setting layout for non image descriptor");
    image.imageLayout = i;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(const VkDescriptorImageInfo &i)
  {
    image = i;
    type = TYPE_IMG;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(const VkDescriptorBufferInfo &i)
  {
    buffer = i;
    type = TYPE_BUF;
    return *this;
  };

  VkAnyDescriptorInfo &operator=(VulkanBufferViewHandle i)
  {
    texelBuffer = i;
    type = TYPE_BUF_VIEW;
    return *this;
  };

#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  VkAnyDescriptorInfo &operator=(VkAccelerationStructureKHR as)
  {
    raytraceAccelerationStructure = as;
    type = TYPE_AS;
    return *this;
  }
#endif

  void clear() { type = TYPE_NULL; }
};

struct ResourceDummyRef
{
  VkAnyDescriptorInfo descriptor;
  void *resource;
};

typedef ResourceDummyRef ResourceDummySet[spirv::MISSING_IS_FATAL_INDEX + 1];

struct DescriptorTable
{
  VkAnyDescriptorInfo arr[spirv::TOTAL_REGISTER_COUNT];

  void clear()
  {
    for (uint32_t i = 0; i < spirv::B_REGISTER_INDEX_MAX; ++i)
      arr[spirv::B_CONST_BUFFER_OFFSET + i].buffer.offset = 0;
    for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
      arr[spirv::T_SAMPLED_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
      arr[spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    for (uint32_t i = 0; i < spirv::U_REGISTER_INDEX_MAX; ++i)
      arr[spirv::U_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    for (uint32_t i = 0; i < spirv::TOTAL_REGISTER_COUNT; ++i)
      arr[i].clear();
  }
  VkAnyDescriptorInfo &offset(uint32_t index, uint32_t offset) { return arr[index + offset]; }

  VkAnyDescriptorInfo &BConstBuffer(uint32_t index) { return offset(index, spirv::B_CONST_BUFFER_OFFSET); }
  VkAnyDescriptorInfo &TSampledImage(uint32_t index) { return offset(index, spirv::T_SAMPLED_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &TSampledCompareImage(uint32_t index) { return offset(index, spirv::T_SAMPLED_IMAGE_WITH_COMPARE_OFFSET); }
  VkAnyDescriptorInfo &TSampledBuffer(uint32_t index) { return offset(index, spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &TBuffer(uint32_t index) { return offset(index, spirv::T_BUFFER_OFFSET); }
  VkAnyDescriptorInfo &TRaytraceAS(uint32_t index) { return offset(index, spirv::T_ACCELERATION_STRUCTURE_OFFSET); }
  VkAnyDescriptorInfo &TInputAttachment(uint32_t index) { return offset(index, spirv::T_INPUT_ATTACHMENT_OFFSET); }
  VkAnyDescriptorInfo &UImage(uint32_t index) { return offset(index, spirv::U_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &USampledBuffer(uint32_t index) { return offset(index, spirv::U_BUFFER_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &UBuffer(uint32_t index) { return offset(index, spirv::U_BUFFER_OFFSET); }
};

} // namespace drv3d_vulkan
