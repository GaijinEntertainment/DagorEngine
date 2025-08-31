// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_baseDef.h>
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_sampler.h>

#include "vulkan_api.h"

namespace drv3d_vulkan
{

// borrowed from ps4 backend
enum D3DFORMAT
{
  D3DFMT_UNKNOWN = 0,

  D3DFMT_R8G8B8 = 20,
  D3DFMT_A8R8G8B8 = 21,
  D3DFMT_X8R8G8B8 = 22,
  D3DFMT_R5G6B5 = 23,
  D3DFMT_X1R5G5B5 = 24,
  D3DFMT_A1R5G5B5 = 25,
  D3DFMT_A4R4G4B4 = 26,
  D3DFMT_A8 = 28,
  D3DFMT_A8R3G3B2 = 29,
  D3DFMT_X4R4G4B4 = 30,
  D3DFMT_A2B10G10R10 = 31,
  D3DFMT_A8B8G8R8 = 32,
  D3DFMT_X8B8G8R8 = 33,
  D3DFMT_G16R16 = 34,
  D3DFMT_A2R10G10B10 = 35,
  D3DFMT_A16B16G16R16 = 36,

  D3DFMT_L8 = 50,
  D3DFMT_A8L8 = 51,
  D3DFMT_L16 = 81,

  D3DFMT_V8U8 = 60,

  D3DFMT_R16F = 111,
  D3DFMT_G16R16F = 112,
  D3DFMT_A16B16G16R16F = 113,
  D3DFMT_R32F = 114,
  D3DFMT_G32R32F = 115,
  D3DFMT_A32B32G32R32F = 116,

  D3DFMT_DXT1 = MAKE4C('D', 'X', 'T', '1'),
  D3DFMT_DXT2 = MAKE4C('D', 'X', 'T', '2'),
  D3DFMT_DXT3 = MAKE4C('D', 'X', 'T', '3'),
  D3DFMT_DXT4 = MAKE4C('D', 'X', 'T', '4'),
  D3DFMT_DXT5 = MAKE4C('D', 'X', 'T', '5'),
};

inline VkSamplerAddressMode translate_texture_address_mode_to_vulkan(int mode) { return (VkSamplerAddressMode)(mode - 1); }

inline VkFilter translate_filter_type_to_vulkan(int m)
{
  return (m == TEXFILTER_POINT || m == TEXFILTER_NONE) ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
}

inline VkSamplerMipmapMode translate_mip_filter_type_to_vulkan(int m)
{
  return (m == TEXMIPMAP_POINT || m == TEXMIPMAP_NONE) ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
}

inline VkCompareOp translate_compare_func_to_vulkan(int mode) { return (VkCompareOp)(mode - 1); }

inline uint32_t translate_compare_func_from_vulkan(VkCompareOp op) { return ((uint32_t)op) + 1; }

inline VkStencilOp translate_stencil_op_to_vulkan(uint32_t so) { return (VkStencilOp)(so - 1); }

inline VkBlendFactor translate_rgb_blend_mode_to_vulkan(uint32_t bm)
{
  static const VkBlendFactor table[] = {
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_FACTOR_ZERO,
    VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_SRC_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    VK_BLEND_FACTOR_SRC_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_FACTOR_DST_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    VK_BLEND_FACTOR_DST_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    VK_BLEND_FACTOR_ZERO, // NOT DEFINED
    VK_BLEND_FACTOR_ZERO, // NOT DEFINED
    VK_BLEND_FACTOR_CONSTANT_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
    VK_BLEND_FACTOR_SRC1_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
    VK_BLEND_FACTOR_SRC1_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
  };
  return table[bm];
}

inline VkBlendFactor translate_alpha_blend_mode_to_vulkan(uint32_t bm)
{
  static const VkBlendFactor table[] = {
    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE,
    VK_BLEND_FACTOR_SRC_ALPHA,           // VK_BLEND_FACTOR_SRC_COLOR   = 3,
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR   = 4,
    VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    VK_BLEND_FACTOR_DST_ALPHA,           // VK_BLEND_FACTOR_DST_COLOR  = 9,
    VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR  = 10,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    VK_BLEND_FACTOR_ZERO,                     // NOT DEFINED
    VK_BLEND_FACTOR_ZERO,                     // NOT DEFINED
    VK_BLEND_FACTOR_CONSTANT_ALPHA,           // VK_BLEND_FACTOR_CONSTANT_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA, // VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR
    VK_BLEND_FACTOR_SRC1_ALPHA,               // VK_BLEND_FACTOR_SRC1_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,     // VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR ,
    VK_BLEND_FACTOR_SRC1_ALPHA,
    VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA // 19
  };
  return table[bm];
}

inline VkBlendFactor translate_vulkan_blend_mode_to_no_dual_source_mode(VkBlendFactor bm)
{
  static const VkBlendFactor table[] = {
    VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    VK_BLEND_FACTOR_DST_COLOR, VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    VK_BLEND_FACTOR_DST_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, VK_BLEND_FACTOR_CONSTANT_COLOR,
    VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR, VK_BLEND_FACTOR_CONSTANT_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
    VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
    VK_BLEND_FACTOR_SRC_COLOR,           // from VK_BLEND_FACTOR_SRC1_COLOR
    VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR, // from VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR
    VK_BLEND_FACTOR_SRC_ALPHA,           // from VK_BLEND_FACTOR_SRC1_ALPHA
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, // from VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
  };
  return table[bm];
}

inline VkBlendOp translate_blend_op_to_vulkan(uint32_t bo) { return (VkBlendOp)(bo - 1); }

inline VkPrimitiveTopology translate_primitive_topology_to_vulkan(uint32_t top)
{
  if (top == PRIM_4_CONTROL_POINTS)
    return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

  return (VkPrimitiveTopology)(top - 1);
}

inline unsigned translate_primitive_topology_mask_to_vulkan(unsigned mask) { return mask >> 1u; }

} // namespace drv3d_vulkan
