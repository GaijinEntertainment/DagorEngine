#ifndef DAGDP_VOLUME_TERRAIN_HLSLI_INCLUDED
#define DAGDP_VOLUME_TERRAIN_HLSLI_INCLUDED

struct VolumeTerrainTile
{
  uint2 intPosXZ;
  float instancePosDelta;
  uint volumeIndex;
};

#endif // DAGDP_VOLUME_TERRAIN_HLSLI_INCLUDED