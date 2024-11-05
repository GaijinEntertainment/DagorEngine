#ifndef OCTAHEDRAL_COMMON_HLSL
#define OCTAHEDRAL_COMMON_HLSL 1

#define DAGI_USE_EQUAL_AREA_OCTAHEDRAL 1

#if !DAGI_USE_EQUAL_AREA_OCTAHEDRAL
#include <octahedral.hlsl>
//to [-1,1] tc
float2 dagi_dir_oct_encode(float3 world_dir)
{
  return octEncode(world_dir.xzy);
}
///tc -1,1
float3 dagi_dir_oct_decode(float2 tc)
{
  return octDecode(tc).xzy;
}
#include <octahedral_solid_angle.hlsl>

float dagi_octahedral_solid_angle_fast(float2 uv, float inv_res)
{
  return octahedral_solid_angle_fast(uv, inv_res);
}
float dagi_octahedral_solid_angle(float2 uv, float inv_res)
{
  return octahedral_solid_angle(uv, inv_res);
}
#else
#include <equalAreaOctahedral.hlsl>
float2 dagi_dir_oct_encode(float3 world_dir)
{
  return equalAreaOctahedralEncode(world_dir.xzy);
}
///tc -1,1
float3 dagi_dir_oct_decode(float2 tc)
{
  return equalAreaOctahedralDecode(tc).xzy;
}

float dagi_octahedral_solid_angle_fast(float2 uv, float inv_res)
{
  return 4*PI*inv_res*inv_res;
}
float dagi_octahedral_solid_angle(float2 uv, float inv_res)
{
  return 4*PI*inv_res*inv_res;
}
#endif

// maps octahedral map for correct hardware bilinear filtering (adds one pixel border)
uint2 octahedral_map_border_to_octhedral(uint2 coord_with_border, uint resolution_with_border, uint border=1)
{
  uint sourceResolution = resolution_with_border - border;
  if (coord_with_border.x < border)
  {
    coord_with_border.x = border + border - 1 - coord_with_border.x;
    coord_with_border.y = resolution_with_border - 1 - coord_with_border.y;
  }
  if (coord_with_border.x >= resolution_with_border - border)
  {
    coord_with_border.x = sourceResolution + sourceResolution - 1 - coord_with_border.x;
    coord_with_border.y = resolution_with_border - 1 - coord_with_border.y;
  }
  if (coord_with_border.y < border)
  {
    coord_with_border.y = border + border - 1 - coord_with_border.y;
    coord_with_border.x = resolution_with_border - 1 - coord_with_border.x;
  }
  if (coord_with_border.y >= resolution_with_border - border)
  {
    coord_with_border.y = sourceResolution + sourceResolution - 1 - coord_with_border.y;
    coord_with_border.x = resolution_with_border - 1 - coord_with_border.x;
  }

  return coord_with_border - border;
}

#endif