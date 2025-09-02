// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// table with t/u/b registers to descriptors mapping

#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <any_descriptor.h>

namespace drv3d_vulkan
{

struct DescriptorTable
{
  VkAnyDescriptorInfo arr[spirv::TOTAL_REGISTER_COUNT];

  void clear()
  {
    for (uint32_t i = 0; i < spirv::B_REGISTER_INDEX_MAX; ++i)
      arr[spirv::B_CONST_BUFFER_OFFSET + i].buffer.offset = 0;
    for (uint32_t i = 0; i < spirv::T_REGISTER_INDEX_MAX; ++i)
      arr[spirv::T_SAMPLED_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    for (uint32_t i = 0; i < spirv::U_REGISTER_INDEX_MAX; ++i)
      arr[spirv::U_IMAGE_OFFSET + i].image.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    for (uint32_t i = 0; i < spirv::TOTAL_REGISTER_COUNT; ++i)
      arr[i].clear();
  }
  VkAnyDescriptorInfo &offset(uint32_t index, uint32_t offset) { return arr[index + offset]; }

  VkAnyDescriptorInfo &BConstBuffer(uint32_t index) { return offset(index, spirv::B_CONST_BUFFER_OFFSET); }
  VkAnyDescriptorInfo &TSampledImage(uint32_t index) { return offset(index, spirv::T_SAMPLED_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &TSampledBuffer(uint32_t index) { return offset(index, spirv::T_BUFFER_SAMPLED_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &TBuffer(uint32_t index) { return offset(index, spirv::T_BUFFER_OFFSET); }
  VkAnyDescriptorInfo &TRaytraceAS(uint32_t index) { return offset(index, spirv::T_ACCELERATION_STRUCTURE_OFFSET); }
  VkAnyDescriptorInfo &TInputAttachment(uint32_t index) { return offset(index, spirv::T_INPUT_ATTACHMENT_OFFSET); }
  VkAnyDescriptorInfo &UImage(uint32_t index) { return offset(index, spirv::U_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &USampledBuffer(uint32_t index) { return offset(index, spirv::U_BUFFER_IMAGE_OFFSET); }
  VkAnyDescriptorInfo &UBuffer(uint32_t index) { return offset(index, spirv::U_BUFFER_OFFSET); }
  VkAnyDescriptorInfo &TSampler(uint32_t index) { return offset(index, spirv::S_SAMPLER_OFFSET); }
};

} // namespace drv3d_vulkan
