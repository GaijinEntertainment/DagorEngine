#ifndef PSEUDO_RANDOM_INCLUDED
#define PSEUDO_RANDOM_INCLUDED 1
//repeat each 128x128 pixels

float pseudo_random_128x128(float2 xy)
{
  float2 pos = frac(xy / 128.0f) * 128.0f + float2(-64.340622f, -72.465622f);
  return frac(dot(pos.xyx * pos.xyy, float3(20.390625f, 60.703125f, 2.4281209f)));
}
#endif