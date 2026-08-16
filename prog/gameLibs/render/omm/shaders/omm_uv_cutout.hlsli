// Copyright (C) Gaijin Games KFT.  All rights reserved.

#ifndef OMM_UV_CUTOUT_HLSLI
#define OMM_UV_CUTOUT_HLSLI

#include "omm_uv_cutout_band.hlsli"

float omm_uv_cutout_keeps_texel(float2 texel_center)
{
  return omm_uv_cutout_band_keeps(omm_uv_cutout_lines, texel_center * g_GlobalConstants.InvTexSize) ? 1.0 : 0.0;
}

float4 omm_uv_cutout_gather_mask(float2 uv)
{
  if (!omm_uv_cutout_enabled)
    return float4(1.0, 1.0, 1.0, 1.0);

  float2 base = floor(uv * g_GlobalConstants.TexSize - 0.5);
  return float4(omm_uv_cutout_keeps_texel(base + float2(0.5, 1.5)), omm_uv_cutout_keeps_texel(base + float2(1.5, 1.5)),
    omm_uv_cutout_keeps_texel(base + float2(1.5, 0.5)), omm_uv_cutout_keeps_texel(base + float2(0.5, 0.5)));
}

float omm_uv_cutout_sample_mask(float2 uv)
{
  return omm_uv_cutout_enabled && !omm_uv_cutout_band_keeps(omm_uv_cutout_lines, uv) ? 0.0 : 1.0;
}

#define OMM_ALPHA_SAMPLE_LEVEL(uv) \
  (omm_alpha_tex.SampleLevel(OMM_GLOBAL_SAMPLER(g_GlobalConstants.SamplerIndex), uv, 0) * omm_uv_cutout_sample_mask(uv))

#define OMM_ALPHA_GATHER_R(uv) \
  (omm_alpha_tex.GatherRed(OMM_GLOBAL_SAMPLER(g_GlobalConstants.SamplerIndex), uv, 0) * omm_uv_cutout_gather_mask(uv))
#define OMM_ALPHA_GATHER_G(uv) \
  (omm_alpha_tex.GatherGreen(OMM_GLOBAL_SAMPLER(g_GlobalConstants.SamplerIndex), uv, 0) * omm_uv_cutout_gather_mask(uv))
#define OMM_ALPHA_GATHER_B(uv) \
  (omm_alpha_tex.GatherBlue(OMM_GLOBAL_SAMPLER(g_GlobalConstants.SamplerIndex), uv, 0) * omm_uv_cutout_gather_mask(uv))
#define OMM_ALPHA_GATHER_A(uv) \
  (omm_alpha_tex.GatherAlpha(OMM_GLOBAL_SAMPLER(g_GlobalConstants.SamplerIndex), uv, 0) * omm_uv_cutout_gather_mask(uv))

#endif
