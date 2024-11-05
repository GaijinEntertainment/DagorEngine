// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daBfg/registry.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_variableRateShading.h>


// TODO: replace with codegen
enum class VRSRATE : int
{
  RATE_1x1 = 0,
  RATE_1x2 = 1,
  RATE_2x1 = 4,
  RATE_2x2 = 5,
  RATE_2x4 = 6,
  RATE_4x2 = 9,
  RATE_4x4 = 10
};

inline constexpr auto VRS_RATE_TEXTURE_NAME = "vrs_mask_tex";

inline void use_default_vrs(dabfg::VirtualPassRequest &pass, dabfg::StateRequest &state)
{
  if (!d3d::get_driver_desc().caps.hasVariableRateShadingTexture)
    return;

  pass = eastl::move(pass).vrsRate(VRS_RATE_TEXTURE_NAME);
  state = eastl::move(state).vrs({/* rateX */ 1,
    /* rateY */ 1,
    /* vertexCombiner */ VariableRateShadingCombiner::VRS_PASSTHROUGH,
    /* pixelCombiner */ VariableRateShadingCombiner::VRS_OVERRIDE});
};
