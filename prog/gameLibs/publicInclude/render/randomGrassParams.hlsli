#ifndef RANDOM_GRASS_PARAMS_HLSL_INCLUDED
#define RANDOM_GRASS_PARAMS_HLSL_INCLUDED

struct GrassLayersVS
{
  float4 grassLodRange;
  uint mask0_31;
  uint mask32_63;
  uint padding;
  float grassInstanceSize;
  float4 grassFade;
  float4 grassScales;
};

struct GrassLayersColor
{
  float4 rStart;
  float4 rEnd;

  float4 gStart;
  float4 gEnd;

  float4 bStart;
  float4 bEnd;
};
#endif