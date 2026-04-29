// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shaderVariableInfo.h>
#include <drv/3d/dag_sampler.h>
#include <math/dag_Point4.h>

namespace envi_cover_vars
{
namespace var
{
static ShaderVariableInfo envi_cover("envi_cover");
static ShaderVariableInfo intensity_map_left_top_right_bottom("envi_cover_intensity_map_left_top_right_bottom");
static ShaderVariableInfo intensity_map("envi_cover_intensity_map");
static ShaderVariableInfo intensity_map_sampler("envi_cover_intensity_map_samplerstate");
static ShaderVariableInfo albedo("envi_cover_albedo");
static ShaderVariableInfo normal("envi_cover_normal");
static ShaderVariableInfo reflectance("envi_cover_reflectance");
static ShaderVariableInfo translucency("envi_cover_translucency");
static ShaderVariableInfo smoothness("envi_cover_smoothness");
static ShaderVariableInfo normal_infl("envi_cover_normal_infl");
static ShaderVariableInfo depth_smoothstep_max("envi_cover_depth_smoothstep_max");
static ShaderVariableInfo depth_pow_exponent("envi_cover_depth_pow_exponent");
static ShaderVariableInfo noise_high_frequency("envi_cover_noise_high_frequency");
static ShaderVariableInfo noise_low_frequency("envi_cover_noise_low_frequency");
static ShaderVariableInfo noise_mask_factor("envi_cover_noise_mask_factor");
static ShaderVariableInfo depth_mask_threshold("envi_cover_depth_mask_threshold");
static ShaderVariableInfo normal_mask_threshold("envi_cover_normal_mask_threshold");
static ShaderVariableInfo depth_mask_contrast("envi_cover_depth_mask_contrast");
static ShaderVariableInfo normal_mask_contrast("envi_cover_normal_mask_contrast");
static ShaderVariableInfo lowest_intensity("envi_cover_lowest_intensity");

// For NBS:
static ShaderVariableInfo thresholds("envi_cover_thresholds");
static ShaderVariableInfo normal_influenced("envi_cover_normal_influenced");
static ShaderVariableInfo intensity_map_scale_offset("envi_cover_intensity_map_scale_offset");
} // namespace var

void set_envi_cover(bool envi_cover) { ShaderGlobal::set_int(var::envi_cover, envi_cover); }

void set_params(const Point4 &envi_cover_intensity_map_left_top_right_bottom, const char *envi_cover_intensity_map,
  const Point4 &envi_cover_albedo, const Point4 &envi_cover_normal, float envi_cover_reflectance, float envi_cover_translucency,
  float envi_cover_smoothness, float envi_cover_normal_infl, float envi_cover_depth_smoothstep_max,
  float envi_cover_depth_pow_exponent, float envi_cover_noise_high_frequency, float envi_cover_noise_low_frequency,
  float envi_cover_noise_mask_factor, float envi_cover_depth_mask_threshold, float envi_cover_normal_mask_threshold,
  float envi_cover_depth_mask_contrast, float envi_cover_normal_mask_contrast, float envi_cover_lowest_intensity)
{
  ShaderGlobal::set_float4(var::intensity_map_left_top_right_bottom, envi_cover_intensity_map_left_top_right_bottom);
  ShaderGlobal::set_texture(var::intensity_map, get_managed_texture_id(envi_cover_intensity_map));
  ShaderGlobal::set_sampler(var::intensity_map_sampler, d3d::request_sampler(d3d::SamplerInfo()));
  ShaderGlobal::set_float4(var::albedo, envi_cover_albedo);
  ShaderGlobal::set_float4(var::normal, envi_cover_normal);
  ShaderGlobal::set_float(var::reflectance, envi_cover_reflectance);
  ShaderGlobal::set_float(var::translucency, envi_cover_translucency);
  ShaderGlobal::set_float(var::smoothness, envi_cover_smoothness);
  ShaderGlobal::set_float(var::normal_infl, envi_cover_normal_infl);
  ShaderGlobal::set_float(var::depth_smoothstep_max, envi_cover_depth_smoothstep_max);
  ShaderGlobal::set_float(var::depth_pow_exponent, envi_cover_depth_pow_exponent);
  ShaderGlobal::set_float(var::noise_high_frequency, envi_cover_noise_high_frequency);
  ShaderGlobal::set_float(var::noise_low_frequency, envi_cover_noise_low_frequency);
  ShaderGlobal::set_float(var::noise_mask_factor, envi_cover_noise_mask_factor);
  ShaderGlobal::set_float(var::depth_mask_threshold, envi_cover_depth_mask_threshold);
  ShaderGlobal::set_float(var::normal_mask_threshold, envi_cover_normal_mask_threshold);
  ShaderGlobal::set_float(var::depth_mask_contrast, envi_cover_depth_mask_contrast);
  ShaderGlobal::set_float(var::normal_mask_contrast, envi_cover_normal_mask_contrast);
  ShaderGlobal::set_float(var::lowest_intensity, envi_cover_lowest_intensity);

  float w = 1 - envi_cover_normal_infl;
  Point4 normal_influenced(envi_cover_normal.x / w, envi_cover_normal.y / w, envi_cover_normal.z / w, -envi_cover_normal_infl / w);
  ShaderGlobal::set_float4(var::normal_influenced, normal_influenced);

  Point4 thresholds(envi_cover_noise_mask_factor, (2 * envi_cover_depth_mask_threshold - 1) * envi_cover_depth_mask_contrast,
    (2 * envi_cover_normal_mask_threshold - 1) * envi_cover_normal_mask_contrast, envi_cover_normal_mask_contrast);

  ShaderGlobal::set_float4(var::thresholds, thresholds);

  Point4 intensity_map_scale_offset(
    1.0 / (envi_cover_intensity_map_left_top_right_bottom.z - envi_cover_intensity_map_left_top_right_bottom.x),
    1.0 / (envi_cover_intensity_map_left_top_right_bottom.w - envi_cover_intensity_map_left_top_right_bottom.y),
    -envi_cover_intensity_map_left_top_right_bottom.x /
      (envi_cover_intensity_map_left_top_right_bottom.z - envi_cover_intensity_map_left_top_right_bottom.x),
    -envi_cover_intensity_map_left_top_right_bottom.y /
      (envi_cover_intensity_map_left_top_right_bottom.w - envi_cover_intensity_map_left_top_right_bottom.y));
  ShaderGlobal::set_float4(var::intensity_map_scale_offset, intensity_map_scale_offset);
}
} // namespace envi_cover_vars