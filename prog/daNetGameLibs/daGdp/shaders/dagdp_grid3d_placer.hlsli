#ifndef DAGDP_GRID3D_PLACER_HLSLI_INCLUDED
#define DAGDP_GRID3D_PLACER_HLSLI_INCLUDED

#define GRID3D_TILE_SIZE 8
#define GRID3D_GROUP_SIZE (GRID3D_TILE_SIZE * GRID3D_TILE_SIZE * GRID3D_TILE_SIZE)

struct Grid3dTile
{
  int3 pos;
  uint volumeRange;
  uint placerViewport;
  uint _pad1;
  uint _pad2;
  uint _pad3;
};

struct Grid3dPlacerGpuData
{
  uint prngSeed;
  float tilePosDelta;
  float gridJitter;
  float _pad;
};

struct Grid3dViewportGpuData
{
  float4 frustumPlanes[6];

  float3 worldPos;
  float maxDistance;
};

#endif // DAGDP_GRID3D_PLACER_HLSLI_INCLUDED
