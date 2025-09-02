//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shaders.h>

#define SSSS_GLOBAL_VARS_LIST                \
  VAR(ssss_transmittance_thickness_bias)     \
  VAR(ssss_transmittance_thickness_min)      \
  VAR(ssss_transmittance_thickness_scale)    \
  VAR(ssss_transmittance_translucency_scale) \
  VAR(ssss_transmittance_blur_scale)         \
  VAR(ssss_transmittance_sample_count)       \
  VAR(ssss_normal_offset)                    \
  VAR(ssss_reflectance_blur_width)           \
  VAR(ssss_reflectance_blur_quality)         \
  VAR(ssss_reflectance_follow_surface)       \
  VAR(ssss_reflectance_blur_pass)            \
  VAR(use_ssss)                              \
  VAR(ssss_quality)

#define VAR(a) static int a##VarId = -1;
SSSS_GLOBAL_VARS_LIST
#undef VAR


// Values should match use_ssss shader interval
enum
{
  SSSS_OFF = 0,
  SSSS_SUN_AND_DYNAMIC_LIGHTS = 1
};

// ssss_quality interval
enum class SsssQuality
{
  OFF,
  LOW, // Reflectance blur only
  HIGH // Reflectance blur and trasmittance
};

static inline void ensure_shadervars_inited()
{
  if (ssss_transmittance_thickness_biasVarId < 0)
  {
#define VAR(a) a##VarId = ::get_shader_variable_id(#a, true);
    SSSS_GLOBAL_VARS_LIST
#undef VAR
  }
}
