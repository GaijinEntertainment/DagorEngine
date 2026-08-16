#ifndef DACLOUDS_TONEMAP_HLSL
#define DACLOUDS_TONEMAP_HLSL  1

//perceptual-space accumulation: with exposure 0 the tonemap pair is an exact
//identity (rcp(l*0+1)==1), so the runtime clouds_taa_exposure var toggles it
#ifndef TAA_IN_HDR_SPACE
#define TAA_IN_HDR_SPACE 0
#endif
#define TAA_BRIGHTNESS_SCALE 1.
#define TAA_CLOUDS_FRAMES 16

#define TAA_TONEMAP_KNEE 0.5
#ifndef TAA_TONEMAP_KNEE
float simple_luma_tonemap(float luma, float exposure) { return rcp(luma * exposure + 1.0); }
float simple_luma_tonemap_inv(float luma, float exposure) { return rcp(max(1.0 - luma * exposure, 0.001)); }
#else
//compress only above a knee (in exposure-normalized units): dark and mid tones
//accumulate linearly - a full-range curve biases the neighborhood clamp toward dark
//neighbors and reads as a dark edge on medium-bright clouds. Exposure 0 = identity.
//the returned FACTOR must stay finite under hostile input: sun-adjacent pixels sit near
//the curve asymptote, and sharpen/variance-clip overshoot fed through an unbounded
//inverse compounds through the history into Inf/NaN. The inverse expansion is capped
//(overshoot above the representable range clamps to a large finite luma); the u clamp
//keeps only the factor finite (!(x<y) catches NaN) - callers scale their OWN color by
//it, so non-finite color is scrubbed at the source (get_clouds_ret) or by the caller

float simple_luma_tonemap(float luma, float exposure)
{
  float u = luma * exposure;
  if (!(u < 1e6))
    u = 1e6;
  float over = max(u - TAA_TONEMAP_KNEE, 0.);
  return u > 1e-6 ? (min(u, TAA_TONEMAP_KNEE) + 1. - rcp(over + 1.)) * rcp(u) : 1.;
}
float simple_luma_tonemap_inv(float luma, float exposure)
{
  float u = luma * exposure;
  if (!(u < 1e6))
    u = 1e6;
  float over = clamp(u - TAA_TONEMAP_KNEE, 0., 0.9375);//caps the expansion at 16x
  return u > 1e-6 ? (min(u, TAA_TONEMAP_KNEE) + over * rcp(1. - over)) * rcp(u) : 1.;
}
#endif

#define CLOUDS_TONEMAPPED 1
#define CLOUDS_TONEMAPPED_TO_SRGB 2
#define ALREADY_TONEMAPPED_SCENE CLOUDS_TONEMAPPED//CLOUDS_TONEMAPPED_TO_SRGB
#ifndef TONEMAPPED_SCENE_EXPOSURE
#define TONEMAPPED_SCENE_EXPOSURE clouds_taa_exposure
#endif
#endif