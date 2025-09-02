#ifndef INVALIDATE_HMAP_DISPLACEMENT_HLSLI_INCLUDED
#define INVALIDATE_HMAP_DISPLACEMENT_HLSLI_INCLUDED

#define HMAP_DISPLACEMENT_INVALIDATORS_MAX_COUNT 16

struct HmapDisplacementInvalidator
{
  float3 worldPos;
  float innerRadius;
  float outerRadius;
  uint eid; // it's here for simplicity's sake; it's not needed on GPU, but we don't lose anything thanks to alignment
  uint2 padding;

#ifdef __cplusplus
  bool operator==(const HmapDisplacementInvalidator &other) const
  {
    return this == &other;
  }
#endif
};

#endif // INVALIDATE_HMAP_DISPLACEMENT_HLSLI_INCLUDED