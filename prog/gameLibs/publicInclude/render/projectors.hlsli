#ifndef PROJECTORS_HLSL
#define PROJECTORS_HLSL

#define MAX_PROJECTORS 128

struct Projector
{
  float4 pos_angle;
  uint2 dir_length;
  uint2 color_sourceWidth;
};

struct ProjectorsData
{
  Projector data[MAX_PROJECTORS];
};

#endif