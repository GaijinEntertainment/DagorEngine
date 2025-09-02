struct EnviSnowParams
{
  float lowest_intensity;
  float depth_smoothstep_max;
  float depth_pow_exponent;
  float noise_high_frequency;
  float noise_low_frequency;

  #ifdef USE_ENVI_MATERIAL_BASED
    float water_level_fade_factor;
    float reflectance;
    float smoothness;
    float translucency;
    float3 albedo;
  #endif
};

bool getDepthNormalFactor(float3 worldPos, EnviSnowParams enviParams, float3 normal, out float2 depthNormalFactor)
{

  #if IS_SNOW_COVER_COMPATIBILITY
    float depthDist = 1;
  #else
    float vignetteEffect = 0;
    float2 lerpFactor;
    float4 depthSamples = gather_depth_above(worldPos.xyz, vignetteEffect, lerpFactor);
    depthSamples = clamp(depthSamples, worldPos.y - envi_cover_y_clamp_from_pos, worldPos.y + envi_cover_y_clamp_from_pos);
    float finalDepth = lerp_gatered_depth_above(depthSamples, lerpFactor);
    float depthDist = worldPos.y + 1 - finalDepth;
    depthDist = lerp(depthDist, 1.0, vignetteEffect);
  #endif

  depthNormalFactor = saturate(float2(
    depthDist,
    dot(envi_cover_normal_influenced.xyz, normal) + envi_cover_normal_influenced.w
  ));

  #if USE_NOISE_128_TEX_HASH
    float noiseSnowInfluenceHi = noise_128_tex_hash.SampleLevel(noise_linear_sampler_state, worldPos.xz * enviParams.noise_high_frequency, 0).x;
    float noiseSnowInfluenceLo = noise_128_tex_hash.SampleLevel(noise_linear_sampler_state, worldPos.xz * enviParams.noise_low_frequency, 0).x;
    float noiseInfl = saturate(noiseSnowInfluenceHi * noiseSnowInfluenceLo) * envi_cover_thresholds.x - envi_cover_thresholds.x + 1;
  #else
    float noiseInfl =  1 - envi_cover_thresholds.x;
  #endif

  #if ENVI_COVER_INTENSITY_MAP
    float2 tc = worldPos.xz * envi_cover_intensity_map_scale_offset.xy + envi_cover_intensity_map_scale_offset.zw;
    // do manually clamp because the shared sampler can be with the different addressing
    tc = saturate(tc);
    // sample an intensity with the shared or own sampler
    float intensity = max(envi_cover_intensity_map.SampleLevel(intensity_map_linear_sampler_state, tc, 0).r, enviParams.lowest_intensity);
  #else
    float intensity = enviParams.lowest_intensity;
  #endif

  // equals to: (2 * envi_cover_normal_mask_threshold * intensity - 1) * envi_cover_normal_mask_contrast
  float normalThreshold = envi_cover_thresholds.z * intensity + envi_cover_thresholds.w * intensity - envi_cover_thresholds.w;

  depthNormalFactor.x = smoothstep(0, enviParams.depth_smoothstep_max, saturate(pow(depthNormalFactor.x, enviParams.depth_pow_exponent) - noiseInfl));
  depthNormalFactor.y = depthNormalFactor.y * 2 * noiseInfl - noiseInfl;
  depthNormalFactor = saturate(depthNormalFactor * float2(envi_cover_depth_mask_contrast, envi_cover_thresholds.w) + float2(envi_cover_thresholds.y, normalThreshold));

  return depthNormalFactor.x > 1e-3f && depthNormalFactor.y > 1e-3f;
}

