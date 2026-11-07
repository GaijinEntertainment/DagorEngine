// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/debug_gbuffer_tex_constants.hlsli>
#include <debug/dag_gbufferColorLegends.h>

// The game owns the pass; debug_texture_analysis.dshl lives only in the debug shaderdump.
namespace render::debug_texture_analysis
{
inline constexpr float COLOR_TO_BYTE = 255.0f;

inline E3DCOLOR legend_color(float r, float g, float b)
{
  return E3DCOLOR_MAKE((int)(r * COLOR_TO_BYTE), (int)(g * COLOR_TO_BYTE), (int)(b * COLOR_TO_BYTE), 255);
}

static const Tab<eastl::pair<const char *, E3DCOLOR>> worldTexelDensityLegend = {
  {"2X+ Under", legend_color(DBG_DENSITY_COLOR_FAR_UNDER_R, DBG_DENSITY_COLOR_FAR_UNDER_G, DBG_DENSITY_COLOR_FAR_UNDER_B)},
  {"1X Under", legend_color(DBG_DENSITY_COLOR_NEAR_UNDER_R, DBG_DENSITY_COLOR_NEAR_UNDER_G, DBG_DENSITY_COLOR_NEAR_UNDER_B)},
  {"Good", legend_color(DBG_DENSITY_COLOR_ON_TARGET_R, DBG_DENSITY_COLOR_ON_TARGET_G, DBG_DENSITY_COLOR_ON_TARGET_B)},
  {"1X Over", legend_color(DBG_DENSITY_COLOR_NEAR_OVER_R, DBG_DENSITY_COLOR_NEAR_OVER_G, DBG_DENSITY_COLOR_NEAR_OVER_B)},
  {"2X+ Over", legend_color(DBG_DENSITY_COLOR_FAR_OVER_R, DBG_DENSITY_COLOR_FAR_OVER_G, DBG_DENSITY_COLOR_FAR_OVER_B)},
  {"Worst Under (red/grey checker)",
    legend_color(DBG_DENSITY_COLOR_FAR_UNDER_R, DBG_DENSITY_COLOR_FAR_UNDER_G, DBG_DENSITY_COLOR_FAR_UNDER_B)},
  {"Worst Over (green/grey checker)",
    legend_color(DBG_DENSITY_COLOR_FAR_OVER_R, DBG_DENSITY_COLOR_FAR_OVER_G, DBG_DENSITY_COLOR_FAR_OVER_B)},
};
} // namespace render::debug_texture_analysis
