#pragma once

#include <3d/dag_drv3dConsts.h>
#include <3d/dag_sampler.h>

#include "driver.h"

/**
* @file
* @brief This file contains functions used to translate various flags between DX12 and dagor formats.
*/
namespace drv3d_dx12
{

/**
 * @brief Translates a Dagor Engine stencil comparison function value to a corresponding DirectX12 value.
 * 
 * @param [in] cmp  The Dagor Engine stencil comparison function value.
 * @return          The DirectX12 stencil comparison function value.
 */
inline D3D12_COMPARISON_FUNC translate_compare_func_to_dx12(int cmp) { return static_cast<D3D12_COMPARISON_FUNC>(cmp); }

/**
 * @brief Translates a Dagor Engine stencil operation value to a corresponding DirectX12 value.
 * 
 * @param [in] so   The Dagor Engine stencil operation value.
 * @return          The DirectX12 stencil operation value
 */
inline D3D12_STENCIL_OP translate_stencil_op_to_dx12(int so) { return static_cast<D3D12_STENCIL_OP>(so); }

/**
 * @brief Translates a Dagor Engine alpha blend mode value to a corresponding DirectX12 value.
 * 
 * @param [in] b    The Dagor Engine alpha blend mode value.
 * @return          The DirectX12 alpha blend mode value.
 */
inline D3D12_BLEND translate_alpha_blend_mode_to_dx12(int b) { return static_cast<D3D12_BLEND>(b); }

/**
 * @brief Translates a Dagor Engine RGB blend mode value to a corresponding DirectX12 value.
 * 
 * @param [in] b    The Dagor Engine RGB blend mode value.
 * @return          The DirectX12 RGB blend mode value.
 */
inline D3D12_BLEND translate_rgb_blend_mode_to_dx12(int b) { return static_cast<D3D12_BLEND>(b); }

/**
 * @brief Translates a Dagor Engine blending operation value to a corresponding DirectX12 value.
 * 
 * @param [in] bo   The Dagor Engine blending operation value.
 * @return          The DirectX12 blending operation value.
 */
inline D3D12_BLEND_OP translate_blend_op_to_dx12(int bo) { return static_cast<D3D12_BLEND_OP>(bo); }

/**
 * @brief Translates a Dagor Engine texture addressing mode value to a corresponding DirectX12 value.
 * 
 * @param [in] mode The Dagor Engine texture addressing mode value.
 * @return          The DirectX12 texture addressing mode value
 */
inline D3D12_TEXTURE_ADDRESS_MODE translate_texture_address_mode_to_dx12(int mode)
{
  return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(mode);
}

/**
 * @brief Translates a DirectX12 texture addressing value to a corresponding Dagor Engine value.
 * 
 * @param [in] mode The DirectX12 texture address mode value.
 * @return          The Dagor Engine texture addressing mode value.
 */
inline int translate_texture_address_mode_to_engine(D3D12_TEXTURE_ADDRESS_MODE mode) { return static_cast<int>(mode); }

/**
 * @brief Translates a Dagor Engine texture filtering type value to a corresponding DirectX12 value.
 * 
 * @param [in] ft   The Dagor Engine texture filtering type value.
 * @return          The DirectX12 texture filtering type value.
 */
inline D3D12_FILTER_TYPE translate_filter_type_to_dx12(int ft)
{
  return (ft == TEXFILTER_POINT || ft == TEXFILTER_NONE) ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
}

/**
 * @brief Translates a Dagor Engine mipmap filtering type value to a corresponding DirectX12 value.
 * 
 * @param [in] ft   The Dagor Engine mipmap filtering type value.
 * @return          The DirectX12 mipmap filtering type value.
 */
inline D3D12_FILTER_TYPE translate_mip_filter_type_to_dx12(int ft)
{
  return (ft == TEXMIPMAP_POINT || ft == TEXMIPMAP_NONE) ? D3D12_FILTER_TYPE_POINT : D3D12_FILTER_TYPE_LINEAR;
}

/**
 * @brief Translates a Dagor Engine primitive topology type value to a corresponding DirectX12 value.
 * 
 * @param [in] value    The Dagor Engine primitive topology type value.
 * @return              The DirectX12 primitive topology type value.
 */
inline D3D12_PRIMITIVE_TOPOLOGY translate_primitive_topology_to_dx12(int value)
{
  // G_ASSERTF(value < PRIM_TRIFAN, "primitive topology was %u", value);
#if _TARGET_XBOX
  if (value == PRIM_QUADLIST)
    return D3D_PRIMITIVE_TOPOLOGY_QUADLIST;
#endif
  return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(value);
}

#if !_TARGET_XBOXONE

/**
 * @brief Translates a Dagor Engine variable rate shading combiner value to a corresponding DirectX12 value.
 * 
 * @param [in] combiner The Dagor Engine variable rate shading combiner value.
 * @return              The DirectX12 variable rate shading combiner value.
 */
inline D3D12_SHADING_RATE_COMBINER map_shading_rate_combiner_to_dx12(VariableRateShadingCombiner combiner)
{
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_PASSTHROUGH == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_PASSTHROUGH));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_OVERRIDE == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_OVERRIDE));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_MIN == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_MIN));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_MAX == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_MAX));
  G_STATIC_ASSERT(D3D12_SHADING_RATE_COMBINER_SUM == static_cast<uint32_t>(VariableRateShadingCombiner::VRS_SUM));
  return static_cast<D3D12_SHADING_RATE_COMBINER>(combiner);
}

/**
 * @brief Constructs a shading rate value from a coarse pixel size value.
 * 
 * @param [in] x, y The coarse pixel dimensions.
 * @return          The DirectX12 shading rate constant value.
 * 
 * The result value is calculated using following formula: (\bx / 2) << 2 | (\b y / 2).
 * Valid values for \b x and \b y are 1, 2 or 4.
 */
inline D3D12_SHADING_RATE make_shading_rate_from_int_values(unsigned x, unsigned y)
{
  G_ASSERTF_RETURN(x <= 4 && y <= 4, D3D12_SHADING_RATE_1X1, "Variable Shading Rate can not exceed 4");
  G_ASSERTF_RETURN(x != 3 && y != 3, D3D12_SHADING_RATE_1X1, "Variable Shading Rate can not be 3");
  G_ASSERTF_RETURN(abs(int(x / 2) - int(y / 2)) < 2, D3D12_SHADING_RATE_1X1,
    "Variable Shading Rate invalid combination of x=%u and y=%u shading rates", x, y);
  G_STATIC_ASSERT(D3D12_SHADING_RATE_X_AXIS_SHIFT == 2);
  G_STATIC_ASSERT(D3D12_SHADING_RATE_VALID_MASK == 3);
  // simple formula (x-rate / 2) << 2 | (y-rage / 2)
  // valid range for x and y are 1, 2 and 4
  return static_cast<D3D12_SHADING_RATE>(((x >> 1) << D3D12_SHADING_RATE_X_AXIS_SHIFT) | (y >> 1));
}
#endif

} // namespace drv3d_dx12
