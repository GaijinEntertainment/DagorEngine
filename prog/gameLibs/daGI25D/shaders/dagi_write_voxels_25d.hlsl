#ifndef SCENE_25D_VOXELS_WRITE_INCLUDED
#define SCENE_25D_VOXELS_WRITE_INCLUDED 1
#include "dagi_voxels_25d_encoding.hlsl"

void writeScene25DVoxelData(uint3 coord, uint alpha, float3 color,
                            float emissionStrength, float4 emissionColor)
{
  emissionColor.rgb *= lerp(1, color.rgb, emissionColor.a);
  float3 emission = emissionStrength * emissionColor.rgb;
#ifdef VOXEL_25D_USE_COLOR
  storeBuffer(voxels25d, getColorBlockAddress(coord), encode_scene_voxels_25d_color_uint_no_alpha(color, emission));
#endif
  voxels25d.InterlockedOr(getAlphaBlockAddress(coord), alpha<<getAlphaShift(coord));
}

#endif
