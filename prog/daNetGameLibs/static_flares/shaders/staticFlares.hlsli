#ifndef STATIC_FLARES_HLSL
#define STATIC_FLARES_HLSL 1

struct StaticFlareInstance
{
  uint4 dirs_dots_start_coef;
  float2 xz;
  uint y_size;
  uint color;

#ifdef __cplusplus
  bool operator==(const StaticFlareInstance&other) const
  {
    return this == &other;
  }
#endif
};

#endif