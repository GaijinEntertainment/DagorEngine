// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/daFrameGraph/registry.h>
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

inline bool can_use_motion_vrs()
{
  // NOTE: we can also use it on other platforms, but need to verify that
  // everything works and looks as expected.
  const Driver3dDesc &drv = d3d::get_driver_desc();
  return d3d::get_driver_code().is(d3d::dx12 && d3d::windows) && drv.caps.hasVariableRateShadingTexture &&
         // NOTE: the following is guaranteed by dx12 spec but lets have it checked
         // here explicitly for when we decide to use it on other platforms too.
         drv.variableRateTextureTileSizeX == drv.variableRateTextureTileSizeY &&
         (drv.variableRateTextureTileSizeX == 8 || drv.variableRateTextureTileSizeX == 16 || drv.variableRateTextureTileSizeX == 32);
}
