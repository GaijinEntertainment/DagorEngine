#ifndef TEX2D_BICUBIC
#define TEX2D_BICUBIC  1

#include <tex2d_bicubic_weights.hlsl>

float4 tex2D_bicubic_sharpen(Texture2D tex, SamplerState tex_samplerstate, float2 uv, float2 res, float2 invRes, float sharpness)
{
  BicubicSharpenWeights weights;
  compute_bicubic_sharpen_weights(uv, res, invRes, sharpness, weights);
  return (tex2D(tex, weights.uv0)*weights.w0 + tex2D(tex, weights.uv1)*weights.w1 + tex2D(tex, weights.uv2)*weights.w2 + tex2D(tex, weights.uv3)*weights.w3 + tex2D(tex, weights.uv4)*weights.w4) * rcp(weights.weightsSum);
}

float4 tex2D_bicubic(Texture2D tex, SamplerState tex_samplerstate, float2 uv, float2 res)
{
  uv = uv * res + 0.5;
  float2 iuv = floor( uv );
  float2 fuv = frac( uv );

  float g0x = g0(fuv.x);
  float g1x = g1(fuv.x);
  float h0x = h0(fuv.x);
  float h1x = h1(fuv.x);
  float h0y = h0(fuv.y);
  float h1y = h1(fuv.y);

  float2 p0 = (float2(iuv.x + h0x, iuv.y + h0y) - 0.5) / res;
  float2 p1 = (float2(iuv.x + h1x, iuv.y + h0y) - 0.5) / res;
  float2 p2 = (float2(iuv.x + h0x, iuv.y + h1y) - 0.5) / res;
  float2 p3 = (float2(iuv.x + h1x, iuv.y + h1y) - 0.5) / res;

  return g0(fuv.y) * (g0x * tex2D(tex, p0)  + g1x * tex2D(tex, p1)) + g1(fuv.y) * (g0x * tex2D(tex, p2) + g1x * tex2D(tex, p3));
}

float4 tex2D_bicubic_lod(Texture2D tex, SamplerState tex_samplerstate, float2 uv, float2 res, float lod)
{
  uv = uv * res + 0.5;
  float2 iuv = floor( uv );
  float2 fuv = frac( uv );

  float g0x = g0(fuv.x);
  float g1x = g1(fuv.x);
  float h0x = h0(fuv.x);
  float h1x = h1(fuv.x);
  float h0y = h0(fuv.y);
  float h1y = h1(fuv.y);

  float2 p0 = (float2(iuv.x + h0x, iuv.y + h0y) - 0.5) / res;
  float2 p1 = (float2(iuv.x + h1x, iuv.y + h0y) - 0.5) / res;
  float2 p2 = (float2(iuv.x + h0x, iuv.y + h1y) - 0.5) / res;
  float2 p3 = (float2(iuv.x + h1x, iuv.y + h1y) - 0.5) / res;

  return g0(fuv.y) * (g0x * tex2Dlod(tex, float4(p0,0,lod))  + g1x * tex2Dlod(tex, float4(p1,0,lod))) + g1(fuv.y) * (g0x * tex2Dlod(tex, float4(p2,0,lod)) + g1x * tex2Dlod(tex, float4(p3,0,lod)));
}

#endif