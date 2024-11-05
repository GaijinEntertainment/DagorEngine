#ifndef RADIANCE_CACHE_COMMON_MATH_INCLUDED
#define RADIANCE_CACHE_COMMON_MATH_INCLUDED 1

#include "radiance_cache_consts.hlsl"
#define RADIANCE_CACHE_MAX_ENCODED_DIST 32.

float3 radiance_cache_encode_texture_radiance(float3 v) {return v*v;}
float3 radiance_cache_decode_texture_radiance(float3 v) {return sqrt(v);}
//float3 radiance_cache_encode_texture_radiance(float3 v) {return v;}
//float3 radiance_cache_decode_texture_radiance(float3 v) {return v;}
#include <octahedral_common.hlsl>
//to [-1,1] tc
float2 radiance_cache_dir_encode(float3 world_dir)
{
  return dagi_dir_oct_encode(world_dir);
}
///tc -1,1
float3 radiance_cache_dir_decode(float2 tc)
{
  return dagi_dir_oct_decode(tc);
}
uint encode_temporal_age_and_frame(uint age, uint frame)
{
  return (age<<RADIANCE_CACHE_TEMPORAL_AGE_SHIFT) | (frame&RADIANCE_CACHE_FRAME_UPDATED_MASK);
}

void decode_temporal_age_and_frame(uint encoded, out uint age, out uint frame)
{
  age = encoded>>RADIANCE_CACHE_TEMPORAL_AGE_SHIFT;
  frame = encoded&RADIANCE_CACHE_FRAME_UPDATED_MASK;
}
#endif