#ifndef GI_VOXELS_25D_ENCODING_INCLUDED
#define GI_VOXELS_25D_ENCODING_INCLUDED 1

#include <dagi_voxels_consts_25d.hlsli>

#define VOXEL_25D_MAX_ALPHA_ADDRESS (VOXEL_25D_RESOLUTION_X*VOXEL_25D_RESOLUTION_Z*VOXEL_25D_RESOLUTION_Y)
#define VOXEL_25D_MAX_COLOR_ADDRESS (VOXEL_25D_MAX_ALPHA_ADDRESS + (VOXEL_25D_RESOLUTION_X*VOXEL_25D_RESOLUTION_Z*VOXEL_25D_RESOLUTION_Y)*4)
#define VOXEL_25D_MAX_EMISSION 16.f
#define VOXEL_25D_MIN_ALBEDO 0.02f
#define VOXEL_25D_MAX_ALBEDO 0.9f
#define VOXEL_25D_ALBEDO_RANGE (VOXEL_25D_MAX_ALBEDO-VOXEL_25D_MIN_ALBEDO)

//address in uints
uint getAlphaBlockAddress(uint3 dtId)
{
  return ((dtId.x>>1) + (dtId.y>>1)*(VOXEL_25D_RESOLUTION_X/2) + dtId.z*(VOXEL_25D_RESOLUTION_Z*VOXEL_25D_RESOLUTION_X/4))<<2;
}
uint getAlphaShift(uint3 dtId)  { return ((dtId.x&1) + ((dtId.y&1)<<1))<<3; }

uint extractAlpha(uint alpha, uint3 dtId) { return (alpha>>getAlphaShift(dtId))&0xFF; }

#ifdef VOXEL_25D_USE_COLOR
//address in uints
uint getColorBlockAddress(uint3 dtId)
{
  return (dtId.x + dtId.y*VOXEL_25D_RESOLUTION_X + dtId.z*VOXEL_25D_RESOLUTION_Z*VOXEL_25D_RESOLUTION_X)*4 +
         VOXEL_25D_MAX_ALPHA_ADDRESS;
}
#endif

uint getFloorBlockAddress(uint2 dtId)
{
#ifdef VOXEL_25D_USE_COLOR
  return (dtId.x + dtId.y*VOXEL_25D_RESOLUTION_X)*4 + VOXEL_25D_MAX_COLOR_ADDRESS;
#else
  return (dtId.x + dtId.y*VOXEL_25D_RESOLUTION_X)*4 + VOXEL_25D_MAX_ALPHA_ADDRESS;
#endif
}


uint encode_scene_voxels_25d_color_uint_no_alpha(float3 albedo, float3 emission)
{
  //todo: currently we under utilize albedo
  albedo = saturate(albedo*(1./VOXEL_25D_ALBEDO_RANGE) - VOXEL_25D_MIN_ALBEDO/VOXEL_25D_ALBEDO_RANGE);// Do not exceed max albedo it to avoid energy conservation
  uint hasEmission = any(emission);
  float4 value = float4(hasEmission ? max(0, emission * (1./VOXEL_25D_MAX_EMISSION)) : albedo, dot(albedo,1./3));//todo: use luminance constants
  uint4 col = uint4(255*saturate(sqrt(value))+0.5);
  return col.r|(col.g<<8)|(col.b<<16)|(((col.a&(-int(hasEmission)))|hasEmission)<<24);//lowest alpha bit is always on for emission
}

void decode_scene_voxels_25d_color_uint_no_alpha(uint ucol, out float3 albedo, out float3 emission)
{
  float4 value = float4(ucol&0xFF, (ucol>>8)&0xFF, (ucol>>16)&0xFF, (ucol>>24)&0xFF)*(1./255);
  value*=value;

  // Albedo color is not stored when emission is present, just brightness
  emission = value.a ? value.rgb*VOXEL_25D_MAX_EMISSION : 0;
  albedo = value.rgb*VOXEL_25D_ALBEDO_RANGE + VOXEL_25D_MIN_ALBEDO;
  FLATTEN
  if (value.a)
    albedo = value.a*VOXEL_25D_ALBEDO_RANGE + VOXEL_25D_MIN_ALBEDO;
}

#endif