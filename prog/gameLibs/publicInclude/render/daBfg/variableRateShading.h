//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <render/daBfg/virtualResourceSemiRequest.h>

#include <3d/dag_drv3dConsts.h>
#include <EASTL/optional.h>


namespace dabfg
{

using VrsRateTextureRequest = VirtualResourceSemiRequest<detail::ResourceRequestPolicy::Readonly>;

/**
 * \brief Describes the settings with which variable rate shading should
 * be enabled for a particular node.
 * \details See \inlinerst :cpp:func:`d3d::set_variable_rate_shading` \endrst
 * and \inlinerst :cpp:func:`d3d::set_variable_rate_shading_texture` \endrst
 * for further explanation of what these parameters do.
 */
struct VrsRequirements
{
  uint32_t rateX = 1; ///< horizontal rate
  uint32_t rateY = 1; ///< vertical rate
  /**
   * FG-managed rate texture, as in `registry.read("your_rate_tex_name")`.
   * \note eastl::nullopt means "no VRS, please"
   */
  eastl::optional<VrsRateTextureRequest> rateTexture;
  /// Vertex combiner. See VariableRateShadingCombiner for details.
  VariableRateShadingCombiner vertexCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
  /// Pixel combiner. See VariableRateShadingCombiner for details.
  VariableRateShadingCombiner pixelCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
};

} // namespace dabfg
