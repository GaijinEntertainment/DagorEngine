#ifndef OCTAHEDRAL_SOLID_ANGLE_HLSL
#define OCTAHEDRAL_SOLID_ANGLE_HLSL

#include <fast_shader_trig.hlsl>
// Computes the solid angle of a spherical triangle with vertices A, B, C as unit length vectors
// https://en.wikipedia.org/wiki/Spherical_trigonometry#Area_and_spherical_excess
//tan (e/2) = sinC/(cosC + coTan(0.5*a)*coTan(0.5*b))
// cotan(0.5a) = sqrt((1+cosa)/(1-cosa))
// coTan(0.5*a)*coTan(0.5*b) = sqrt((1+cosb)*(1+cosa)/((1-cosa)*((1-cosb)))
float compute_triangle_solid_angle(float3 A, float3 B, float3 C)
{
  float cosAB = dot(A, B);
  float cosBC = dot(B, C);
  float cosCA = dot(C, A);
  float sinAB = 1.0f - cosAB * cosAB;
  float sinBC = 1.0f - cosBC * cosBC;
  float cosC = cosCA - cosAB * cosBC;
  float sinC = sqrt(sinAB * sinBC - cosC * cosC);
  float norm = (1.0f - cosAB) * (1.0f - cosBC);
  return 2.0f * atan2(sinC, sqrt((sinAB * sinBC * (1.0f + cosBC) * (1.0f + cosAB)) / norm) + cosC);
}

float compute_triangle_solid_angle_half_tan(float3 A, float3 B, float3 C)
{
  float cosAB = dot(A, B);
  float cosBC = dot(B, C);
  float cosCA = dot(C, A);
  float sinAB = 1.0f - cosAB * cosAB;
  float sinBC = 1.0f - cosBC * cosBC;
  float cosC = cosCA - cosAB * cosBC;
  float sinC = sqrt(saturate(sinAB * sinBC - cosC * cosC));
  float norm = (1.0f - cosAB) * (1.0f - cosBC);
  return sinC / max(1e-11, sqrt((sinAB * sinBC * (1.0f + cosBC) * (1.0f + cosAB)) / norm) + cosC);
}

float3 dagi_dir_oct_decode_uv(float2 uv) {return dagi_dir_oct_decode(uv*2-1);}

// uv should be centered on the octahedral texel, in the range [.5f, .5f + resolution - 1]/resolution
// uses approximation, and requires solidangle of each triangle to be less than 1 radian
// works almost same speed as LUT, and probably a bit more precise
float octahedral_solid_angle_fast(float2 uv, float inv_res)
{
  float3 direction10 = dagi_dir_oct_decode_uv(uv + float2(.5f, -.5f) * inv_res);
  float3 direction01 = dagi_dir_oct_decode_uv(uv + float2(-.5f, .5f) * inv_res);

  float solidAngle0Tan = compute_triangle_solid_angle_half_tan(
      dagi_dir_oct_decode_uv(uv + float2(-.5f, -.5f) * inv_res),
      direction10,
      direction01);

  float solidAngle1Tan = compute_triangle_solid_angle_half_tan(
      dagi_dir_oct_decode_uv(uv + float2(.5f, .5f) * inv_res),
      direction01,
      direction10);
  return 2*atanFast4((solidAngle0Tan+solidAngle1Tan) / (1-solidAngle0Tan*solidAngle1Tan));
  //this is more accurate, but significantly slower
  //return 2*atan2( solidAngle0Tan+solidAngle1Tan, 1-solidAngle0Tan*solidAngle1Tan);
}

// uv should be centered on the octahedral texel, in the range [.5f, .5f + resolution - 1]/resolution
float octahedral_solid_angle(float2 uv, float inv_res)
{
  float3 direction10 = dagi_dir_oct_decode_uv(uv + float2(.5f, -.5f) * inv_res);
  float3 direction01 = dagi_dir_oct_decode_uv(uv + float2(-.5f, .5f) * inv_res);

  float solidAngle0 = compute_triangle_solid_angle(
      dagi_dir_oct_decode_uv(uv + float2(-.5f, -.5f) * inv_res),
      direction10,
      direction01);

  float solidAngle1 = compute_triangle_solid_angle(
      dagi_dir_oct_decode_uv(uv + float2(.5f, .5f) * inv_res),
      direction01,
      direction10);

  return solidAngle0 + solidAngle1;
}

#endif