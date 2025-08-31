bool calculate_envi_cover_influence(float3 worldPos, float3 normal, EnviSnowParams enviParams, out float enviInfluence)
{
  float2 depthNormalFactor;

  BRANCH
  if (getDepthNormalFactor(worldPos, enviParams, normal, depthNormalFactor))
  {
    enviInfluence = depthNormalFactor.x * depthNormalFactor.y;
    enviInfluence = sqrt(enviInfluence);
    enviInfluence *= saturate(worldPos.y * enviParams.water_level_fade_factor + enviParams.water_level_fade_factor * (envi_water_level));

    return false;
  }

  enviInfluence = 0;
  return true;
}

#ifdef NEED_ENVI_COVER_GAMMA_CORRECTION
#define ENVI_COVER_GAMMA_USE_PRECISE 0
void enviCoverSRGBToLinear(inout half3 albedo)
{
  #if ENVI_COVER_GAMMA_USE_PRECISE
    albedo = pow(albedo, 2.2f);
  #else
    albedo = pow2(albedo);
  #endif
}

void enviCoverLinearToSRGB(inout half3 albedo)
{
  #if ENVI_COVER_GAMMA_USE_PRECISE
    albedo = pow(albedo, 1 / 2.2f);
  #else
    albedo = sqrt(albedo);
  #endif
}
#else
void enviCoverSRGBToLinear(inout half3 albedo) {}
void enviCoverLinearToSRGB(inout half3 albedo) {}
#endif


void lerp_envi_cover(float3 worldPos, float3 normal, float skylight_progress_value, float enviInfluence, EnviSnowParams enviParams,
  inout half3 albedo, inout half reflectance, inout half smoothness, inout half translucency, inout uint material)
{
  enviCoverSRGBToLinear(albedo);
  albedo =lerp(albedo, enviParams.albedo.xyz, enviInfluence);
  enviCoverLinearToSRGB(albedo);
  reflectance = lerp(reflectance, enviParams.reflectance, enviInfluence);
  smoothness = lerp(smoothness, enviParams.smoothness, enviInfluence);
  translucency = lerp(translucency, enviParams.translucency, enviInfluence);
  material = material == SHADING_FOLIAGE ? SHADING_FOLIAGE : SHADING_SUBSURFACE;

  computeSnowSparkle(worldPos, world_view_pos.xyz, min(0.99, 1 - skylight_progress_value), normal, smoothness);
}

bool apply_envi_cover(float3 worldPos, float skylight_progress_value, EnviSnowParams enviParams, inout UnpackedGbuffer gbuffer)
{
  BRANCH
  if (!isEmissiveShader(gbuffer.material) && (!gbuffer.dynamic || isFoliageShader(gbuffer.material)) && !gbuffer.isLandscape)
  {
    half enviCoverInfluence;
    BRANCH
    if(!calculate_envi_cover_influence(worldPos, gbuffer.normal, enviParams, enviCoverInfluence))
    {
      gbuffer.metalness = 0;
      lerp_envi_cover(worldPos, gbuffer.normal, skylight_progress_value, enviCoverInfluence, enviParams, gbuffer.albedo, gbuffer.reflectance,
                      gbuffer.smoothness, gbuffer.translucency, gbuffer.material);

      gbuffer.sss_profile = SSS_PROFILE_NEUTRAL_TRANSLUCENT;
      gbuffer.shadow = gbuffer.material == SHADING_FOLIAGE ? gbuffer.shadow : 1;
      return true;
    }
  }
  return false;
}


