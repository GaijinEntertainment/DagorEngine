// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "vulkan_api.h"

// NOTE: compiled_meta_data includes vulkan.h directly, which is wrong,
// we should always go through our vlukan_api.h wrapper to get consistent
// preprocessor state
#include <drv/shadersMetaData/spirv/compiled_meta_data.h>

#include "drvCommonConsts.h"

namespace drv3d_vulkan
{

template <VkShaderStageFlagBits>
struct ShaderStageTraits;

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_COMPUTE_BIT>
{
  static const uint32_t min_register_count = 1 << 6;
  static const uint32_t max_register_count = 4096;
  static const uint32_t register_index = spirv::compute::REGISTERS_SET_INDEX;
  static const VkPipelineBindPoint pipeline_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
};

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_VERTEX_BIT>
{
  static const uint32_t min_register_count = DEF_VS_CONSTS;
  static const uint32_t max_register_count = 4096;
  static const uint32_t register_index = spirv::graphics::vertex::REGISTERS_SET_INDEX;
  static const VkPipelineBindPoint pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_FRAGMENT_BIT>
{
  static const uint32_t min_register_count = MAX_PS_CONSTS;
  static const uint32_t max_register_count = MAX_PS_CONSTS;
  static const uint32_t register_index = spirv::graphics::fragment::REGISTERS_SET_INDEX;
  static const VkPipelineBindPoint pipeline_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
};

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_GEOMETRY_BIT> : ShaderStageTraits<VK_SHADER_STAGE_VERTEX_BIT>
{
  static const uint32_t register_index = spirv::graphics::geometry::REGISTERS_SET_INDEX;
};

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT> : ShaderStageTraits<VK_SHADER_STAGE_VERTEX_BIT>
{
  static const uint32_t register_index = spirv::graphics::control::REGISTERS_SET_INDEX;
};

template <>
struct ShaderStageTraits<VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT> : ShaderStageTraits<VK_SHADER_STAGE_VERTEX_BIT>
{
  static const uint32_t register_index = spirv::graphics::evaluation::REGISTERS_SET_INDEX;
};
} // namespace drv3d_vulkan