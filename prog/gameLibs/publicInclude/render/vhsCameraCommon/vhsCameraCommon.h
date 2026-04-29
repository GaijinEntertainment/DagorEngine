//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ecs/render/shaderVar.h>

void vhs_camera_set_shader_params(float vhs_camera__resolutionDownscale, float vhs_camera__saturationMultiplier,
  float vhs_camera__noiseIntensity, float vhs_camera__dynamicRangeMin, float vhs_camera__dynamicRangeMax,
  float vhs_camera__dynamicRangeGamma, float vhs_camera__scanlineHeight, float vhs_camera__noiseUvScale,
  float vhs_camera__chromaticAberrationIntensity, float vhs_camera__chromaticAberrationStart, float vhs_camera__noiseSaturation,
  ShaderVar vhs_camera__downscale, ShaderVar vhs_camera__saturation, ShaderVar vhs_camera__noise_strength,
  ShaderVar vhs_camera__noise_saturation, ShaderVar vhs_camera__dynamic_range_min, ShaderVar vhs_camera__dynamic_range_max,
  ShaderVar vhs_camera__dynamic_range_gamma, ShaderVar vhs_camera__scanline_height, ShaderVar vhs_camera__noise_uv_scale,
  ShaderVar vhs_camera__chromatic_aberration_intensity, ShaderVar vhs_camera__chromatic_aberration_start);
