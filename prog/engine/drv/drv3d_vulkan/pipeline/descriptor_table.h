// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// table with t/u/b registers to descriptors mapping

#include <drv/shadersMetaData/spirv/compiled_meta_data.h>
#include <any_descriptor.h>

namespace drv3d_vulkan
{

struct DescriptorTable
{
  VkAnyDescriptorInfo arr[spirv::DESCRIPTOR_TABLE_SIZE];

  void clear()
  {
    for (VkAnyDescriptorInfo &i : arr)
      i.clear();
  }
  VkAnyDescriptorInfo &TInputAttachment(uint32_t index) { return arr[spirv::T_INPUT_ATTACHMENT_DESCRIPTOR_TABLE_OFFSET + index]; }
};

} // namespace drv3d_vulkan
