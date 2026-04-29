#ifndef DAGDP_BVH_MAPPING
#define DAGDP_BVH_MAPPING 1

struct DagdpBvhMapping
{
  uint2 blas;
  uint metaIndex;
  uint isFoliage;
  float3 wind_channel_strength;
  uint padding;
};

#endif