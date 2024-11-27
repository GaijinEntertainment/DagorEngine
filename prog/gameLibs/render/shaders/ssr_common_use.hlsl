#ifndef SSR_COMMON_USE_INCLUDED
#define SSR_COMMON_USE_INCLUDED 1
  // below CALC_SSR_ROUGHNESS_THRESHOLD there is no SSR calculation
  #define CALC_SSR_ROUGHNESS_THRESHOLD 0.7
  #define CALC_SSR_SMOOTHNESS_THRESHOLD (1-0.7)
  // between CALC_SSR_ROUGHNESS_THRESHOLD and MAX_LERP_SSR_ROUGHNESS_THRESHOLD there will be blending with rough specular
  #define MAX_LERP_SSR_ROUGHNESS_THRESHOLD 0.4
  half get_roughness_fade(half linear_roughness)
  {
    const half a = 1./(MAX_LERP_SSR_ROUGHNESS_THRESHOLD-CALC_SSR_ROUGHNESS_THRESHOLD);
    const half b = -CALC_SSR_ROUGHNESS_THRESHOLD/(MAX_LERP_SSR_ROUGHNESS_THRESHOLD-CALC_SSR_ROUGHNESS_THRESHOLD);
    return saturate(linear_roughness * a + b);
  }

  // for calculations of virtual point
  #define HIGH_ROUGHNESS_THRESHOLD_END 0.5
  #define HIGH_ROUGHNESS_THRESHOLD_START 0.1
  // 0 - should be treated perform mirro-like for virtual point reflection
  // 1 - should be treated rough-like for virtual point reflection
  half get_is_rough_surface_param(half linear_roughness)
  {
    return saturate(linear_roughness*(1./HIGH_ROUGHNESS_THRESHOLD_END) - HIGH_ROUGHNESS_THRESHOLD_START/(HIGH_ROUGHNESS_THRESHOLD_END-HIGH_ROUGHNESS_THRESHOLD_START));
  }

#endif