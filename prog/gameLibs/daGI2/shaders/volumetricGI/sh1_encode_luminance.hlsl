#ifndef SH1_ENCODE_LUMINANCE_INCLUDED
#define SH1_ENCODE_LUMINANCE_INCLUDED

#include <sh1.hlsl>

#ifndef INCREASE_SH1_PRECISION
#define INCREASE_SH1_PRECISION 1
#endif

half gi_luminance(half3 col)
{
  //return dot(col, half3(0.299, 0.587, 0.114));
  return dot(col, half(1./3));
}

struct SH1LumEncoded
{
  half3 ambient;
  half4 directional;
};

void encode_sh1_luminance(SH1 sh1, out half3 ambient, out half4 directional)//ambient and directional can be negative
{
  half3 coefficient0 = half3(sh1.R.y, sh1.G.y, sh1.B.y);
  half3 coefficient1 = half3(sh1.R.z, sh1.G.z, sh1.B.z);
  half3 coefficient2 = half3(sh1.R.w, sh1.G.w, sh1.B.w);
  #if INCREASE_SH1_PRECISION
  ambient = half3(sh1.G.x, sh1.B.x, 0);//unused channel .z
  directional = half4(gi_luminance(coefficient0), gi_luminance(coefficient1), gi_luminance(coefficient2), sh1.R.x);
  #else
  ambient = half3(sh1.R.x, sh1.G.x, sh1.B.x);
  directional = half4(gi_luminance(coefficient0), gi_luminance(coefficient1), gi_luminance(coefficient2), 0);//unused channel .w
  #endif
}

SH1LumEncoded encode_sh1_luminance(SH1 sh1)
{
  SH1LumEncoded sh1E;
  encode_sh1_luminance(sh1, sh1E.ambient, sh1E.directional);
  return sh1E;
}

SH1 decode_sh1_luminance(half3 ambient, half4 directional)
{
  SH1 sh1;
  #if INCREASE_SH1_PRECISION
  ambient = half3(directional.w, ambient.xy);
  #endif
  sh1.R.x = ambient.r;
  sh1.G.x = ambient.g;
  sh1.B.x = ambient.b;
  half3 normalizedAmbient = half3(ambient.rgb / ( gi_luminance( ambient.rgb ) + 1e-5f )); // this calculation needs float precision

  sh1.R.yzw = directional.rgb * normalizedAmbient.r;
  sh1.G.yzw = directional.rgb * normalizedAmbient.g;
  sh1.B.yzw = directional.rgb * normalizedAmbient.b;
  return sh1;
}

SH1 decode_sh1_luminance(SH1LumEncoded sh1)
{
  return decode_sh1_luminance(sh1.ambient, sh1.directional);
}

half3 gi_sh1_decode_ambient(half3 ambient, half4 directional)
{
  #if INCREASE_SH1_PRECISION
  return half3(directional.w, ambient.xy);
  #endif
  return ambient;
}

#endif
