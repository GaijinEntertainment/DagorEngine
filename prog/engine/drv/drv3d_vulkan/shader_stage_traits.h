#pragma once

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
  static const uint32_t min_register_count = MAX_VS_CONSTS_BONES;
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