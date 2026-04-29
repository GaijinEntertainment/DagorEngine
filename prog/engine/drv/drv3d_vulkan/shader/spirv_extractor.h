// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_consts.h>
#include <smolv.h>
#include <spirv.hpp>
#include <EASTL/optional.h>

#include "vulkan_device.h"
#include "shader_module.h"

namespace drv3d_vulkan
{

struct ShaderProgramData
{
  uint32_t offset;
  uint32_t size;
};

namespace spirv_extractor
{
#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
ShaderDebugInfo getDebugInfo(const Tab<spirv::ChunkHeader> &chunks, const dag::ConstSpan<uint8_t> &chunk_data,
  uint32_t extension_bits);
dag::ConstSpan<char> getName(const Tab<spirv::ChunkHeader> &chunks, const dag::ConstSpan<uint8_t> &chunk_data,
  uint32_t extension_bits);
#endif

ShaderModuleBlob getBlob(const ShaderModuleHeader &header, const ShaderSource &source, const struct ShaderProgramData &bytecode,
  const Tab<spirv::ChunkHeader> &chunks, dag::ConstSpan<uint8_t> chunk_data, uint32_t extension_bits);

eastl::optional<ShaderModuleHeader> getHeader(VkShaderStageFlags stage, const Tab<spirv::ChunkHeader> &chunks,
  dag::ConstSpan<uint8_t> chunk_data, uint32_t extension_bits);
} // namespace spirv_extractor
} // namespace drv3d_vulkan