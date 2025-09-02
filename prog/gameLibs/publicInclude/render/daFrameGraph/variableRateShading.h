//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_variableRateShading.h>


namespace dafg
{

/**
 * \brief Describes the settings with which variable rate shading should
 * be enabled for a particular node.
 * \details See \inlinerst :cpp:func:`d3d::set_variable_rate_shading` \endrst
 * for further explanation of what these parameters do.
 * \note The rate texture is specified through a render pass request, as
 * it must be kept in the tile cache on TBR/TBDR GPUs.
 * See dafg::VirtualPassRequest::vrsRate.
 */
struct VrsSettings
{
  uint32_t rateX = 1; ///< horizontal rate
  uint32_t rateY = 1; ///< vertical rate

  /// Vertex combiner. See VariableRateShadingCombiner for details.
  VariableRateShadingCombiner vertexCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
  /// Pixel combiner. See VariableRateShadingCombiner for details.
  VariableRateShadingCombiner pixelCombiner = VariableRateShadingCombiner::VRS_PASSTHROUGH;
};

} // namespace dafg
