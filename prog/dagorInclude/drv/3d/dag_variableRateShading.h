//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class BaseTexture;

#include <drv/3d/dag_consts.h>

namespace d3d
{
/**
 * \brief Sets variable rate shading setup for next draw calls.
 *
 * \note Rates (\p rate_x, \p rate_y) of 1 by 4 or 4 by 1 are invalid.
 *
 * \note Depth/Stencil values are always computed at full rate and so
 * shaders that modify depth value output may interfere with the
 * \p pixel_combiner.
 *
 * \param rate_x,rate_y Constant rates for the next draw calls, those
 *   are supported by all VRS capable devices. Valid values
 *   for both are 1, 2 and with the corresponding feature cap 4.
 * \param vertex_combiner The mode in which the constant rate of
 *   \p rate_x and \p rate_y is combined with a possible vertex/geometry
 *   shader rate output. For shader outputs see SV_ShadingRate, note
 *   that the provoking vertex or the per primitive value is used.
 * \param pixel_combiner The mode in which the result of 'vertex_combiner'
 *   is combined with the rate of the sampling rate texture set by
 *   \ref set_variable_rate_shading_texture.
 */
void set_variable_rate_shading(unsigned rate_x, unsigned rate_y,
  VariableRateShadingCombiner vertex_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH,
  VariableRateShadingCombiner pixel_combiner = VariableRateShadingCombiner::VRS_PASSTHROUGH);

/**
 * \brief Sets the variable rate shading texture for the next draw calls.
 * \param rate_texture The texture to use as a shading rate source.
 * \details Note that when you start to modify the used texture, you
 * should reset the used shading rate texture to null to ensure that on
 * next use as a shading rate source, the texture is in a state the device
 * can use.
 * \note It is invalid to call this when DeviceDriverCapabilities::hasVariableRateShadingTexture
 * feature is not supported.
 */
void set_variable_rate_shading_texture(BaseTexture *rate_texture = nullptr);

} // namespace d3d

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
namespace d3d
{
inline void set_variable_rate_shading(unsigned rate_x, unsigned rate_y, VariableRateShadingCombiner vertex_combiner,
  VariableRateShadingCombiner pixel_combiner)
{
  d3di.set_variable_rate_shading(rate_x, rate_y, vertex_combiner, pixel_combiner);
}

inline void set_variable_rate_shading_texture(BaseTexture *rate_texture) { d3di.set_variable_rate_shading_texture(rate_texture); }
} // namespace d3d
#endif