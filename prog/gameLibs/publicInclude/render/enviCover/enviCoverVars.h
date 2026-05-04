//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point4;

namespace envi_cover_vars
{
void set_envi_cover(bool envi_cover);
void set_params(const Point4 &envi_cover_intensity_map_left_top_right_bottom, const char *envi_cover_intensity_map,
  const Point4 &envi_cover_albedo, const Point4 &envi_cover_normal, float envi_cover_reflectance, float envi_cover_translucency,
  float envi_cover_smoothness, float envi_cover_normal_infl, float envi_cover_depth_smoothstep_max,
  float envi_cover_depth_pow_exponent, float envi_cover_noise_high_frequency, float envi_cover_noise_low_frequency,
  float envi_cover_noise_mask_factor, float envi_cover_depth_mask_threshold, float envi_cover_normal_mask_threshold,
  float envi_cover_depth_mask_contrast, float envi_cover_normal_mask_contrast, float envi_cover_lowest_intensity);
} // namespace envi_cover_vars