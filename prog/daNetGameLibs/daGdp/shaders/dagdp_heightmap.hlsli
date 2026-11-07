#ifndef DAGDP_HEIGHTMAP_HLSLI_INCLUDED
#define DAGDP_HEIGHTMAP_HLSLI_INCLUDED

#define TILE_INSTANCE_COUNT    64
#define TILE_INSTANCE_COUNT_1D 8
#define TILE_COUNT_PER_GROUP 32
#define TILE_POSITION_BYTE_SIZE 16

struct DagdpViewportGpuData
{
  int2 baseTileIntPosXZ;
  float2 _pad0;
  float3 worldPos;
  float maxDistance;
  float4 frustumPlanes[6];
};

#endif // DAGDP_HEIGHTMAP_HLSLI_INCLUDED