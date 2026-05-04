#ifndef NBS_SPHERES_INCLUDED
#define NBS_SPHERES_INCLUDED

struct NBSSphere
{
  float3 pos;
  float invRadius;
  float centerDensity;
  float edgeDensity;
  float2 padding;
};

#endif
