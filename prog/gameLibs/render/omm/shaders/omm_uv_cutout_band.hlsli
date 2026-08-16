// Copyright (C) Gaijin Games KFT.  All rights reserved.

// One definition shared by the bake (omm_uv_cutout.hlsli) and the viewer overlay (omm_debug.dshl): if
// the two differ, the overlay no longer explains the states below it.

#ifndef OMM_UV_CUTOUT_BAND_HLSLI
#define OMM_UV_CUTOUT_BAND_HLSLI

bool omm_uv_cutout_band_keeps(float4 lines, float2 uv)
{
  float2 band = lines.xy * uv.y + lines.zw;
  return uv.x >= band.x && uv.x <= band.y;
}

#endif
