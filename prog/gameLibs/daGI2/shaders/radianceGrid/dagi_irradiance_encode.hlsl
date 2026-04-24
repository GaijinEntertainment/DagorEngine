#ifndef DAGI_IRRAD_ENCODE_INCLUDED
#define DAGI_IRRAD_ENCODE_INCLUDED

#include <sh1.hlsl>

struct SH1Encoded
{
  float4 s0; // u8
  float4 s1; // f16
};

SH1Encoded lerp(SH1Encoded a, SH1Encoded b, half v)
{
  a.s0 = lerp(a.s0, b.s0, v);
  a.s1 = lerp(a.s1, b.s1, v);
  return a;
}

SH1 lerp(SH1 a, SH1 b, half v)
{
  a.R = lerp(a.R, b.R, v);
  a.G = lerp(a.G, b.G, v);
  a.B = lerp(a.B, b.B, v);
  return a;
}

float3 dagi_irrad_encode_color_lrg(float3 rgb)
{
  rgb = max(rgb, float3(0, 0, 0)) + 2e-4;
  float s = rgb.r + rgb.g + rgb.b;
  return float3(s, rgb.rg * rcp(s));
}

float3 dagi_irrad_decode_color_lrg(float3 lrg)
{
  return float3(lrg.yz, 1 - lrg.y - lrg.z) * lrg.x;
}

SH1Encoded dagi_irrad_encode_sh1(SH1 sh1)
{
  half3 amb = dagi_irrad_encode_color_lrg(float3(sh1.R.x, sh1.G.x, sh1.B.x));
  half3 up = dagi_irrad_encode_color_lrg(float3(sh1.R.x + sh1.R.y, sh1.G.x + sh1.G.y, sh1.B.x + sh1.B.y));
  half3 dir = (sh1.R.yzw + sh1.G.yzw + sh1.B.yzw) / 3.0;

  SH1Encoded enc;
  enc.s0 = half4(amb.yz, up.yz);
  enc.s1 = half4(amb.x, dir);
  return enc;
}

SH1 dagi_irrad_decode_sh1(SH1Encoded enc)
{
  half3 ambNorm = dagi_irrad_decode_color_lrg(float3(1.0, enc.s0.xy));
  half3 amb = ambNorm * enc.s1.x;
  half3 dir = enc.s1.yzw;
  half2 upTint = enc.s0.zw;

  // Next goes a solution to following system of equations:
  // # to keep same luminance on top
  // Ry + Gy + By = dir.x * 3;
  // # keep tint on top
  // (Ry + amb.r) / (Ry + amb.r + Gy + amb.g + By + amb.b) = upTint.r
  // (Gy + amb.g) / (Ry + amb.r + Gy + amb.g + By + amb.b) = upTint.g

  float d_1 = dir.x * 3.0;
  float r_0 = amb.r;
  float g_0 = amb.g;
  float b_0 = amb.b;
  float r_1 = upTint.r;
  float g_1 = upTint.g;
  float Ry = r_1 * (b_0 + d_1 + g_0) + r_0 * (r_1 - 1);
  float Gy = g_1 * (b_0 + d_1 + r_0) + g_0 * (g_1 - 1);
  float By = -b_0 * g_1 - b_0 * r_1 - d_1 * (g_1 + r_1 - 1) - g_1 * r_0 - g_0 * (g_1 + r_1 - 1) + r_0 - r_0 * r_1;

  SH1 sh1;
  sh1.R = float4(amb.r, float3(Ry, dir.yz * ambNorm.r));
  sh1.G = float4(amb.g, float3(Gy, dir.yz * ambNorm.g));
  sh1.B = float4(amb.b, float3(By, dir.yz * ambNorm.b));

#ifdef DAGI_IRRADIANCE_SH1_MIN_THRESHOLD_AMBIENT_MUL
  // Recompute constant and directional terms so that sampled irradiance is always
  // greater than amb * DAGI_IRRADIANCE_SH1_MIN_THRESHOLD_AMBIENT_MUL.
  float3 maxDirTerm = max(float3(length(sh1.R.yzw), length(sh1.G.yzw), length(sh1.B.yzw)), 1e-6);
  float3 minDirTerm = max(-amb * DAGI_IRRADIANCE_SH1_MIN_THRESHOLD_AMBIENT_MUL, -maxDirTerm);
  float3 avgDirTerm = (maxDirTerm + minDirTerm) * 0.5;
  float3 halfDirTerm = (maxDirTerm - minDirTerm) * 0.5;
  sh1.R.x += avgDirTerm.x;
  sh1.G.x += avgDirTerm.y;
  sh1.B.x += avgDirTerm.z;
  sh1.R.yzw = sh1.R.yzw * halfDirTerm.x / maxDirTerm.x;
  sh1.G.yzw = sh1.G.yzw * halfDirTerm.y / maxDirTerm.y;
  sh1.B.yzw = sh1.B.yzw * halfDirTerm.z / maxDirTerm.z;
#endif

  return sh1;
}

uint3 dagi_irrad_pack_encoded_sh1(SH1Encoded enc)
{
  uint3 p;
  uint4 con = uint4(saturate(enc.s0) * 255);
  p.x = con.x | (con.y << 8u) | (con.z << 16u) | (con.w << 24u);
  uint4 lin = f32tof16(enc.s1);
  p.y = lin.x | (lin.y << 16u);
  p.z = lin.z | (lin.w << 16u);
  return p;
}

SH1Encoded dagi_irrad_unpack_encoded_sh1(uint3 p)
{
  SH1Encoded enc;
  uint4 con;
  con.x = p.x & 0xFFu;
  con.y = (p.x >> 8u) & 0xFFu;
  con.z = (p.x >> 16u) & 0xFFu;
  con.w = p.x >> 24u;
  enc.s0 = float4(con) / 255.0;
  uint4 lin;
  lin.x = p.y & 0xFFFFu;
  lin.y = p.y >> 16u;
  lin.z = p.z & 0xFFFFu;
  lin.w = p.z >> 16u;
  enc.s1 = f16tof32(lin);
  return enc;
}

#endif