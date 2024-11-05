#ifndef PLANAR_DECALS_PARAMS_HLSLI
#define PLANAR_DECALS_PARAMS_HLSLI

// We define it for readability, but it can't be changed actually
// (as long as we provide decalWidths through float4).
#define MAX_PLANAR_DECALS 4

struct PlanarDecalTransformAndUV
{
  // We have 2 transforms for the same decal, cause it can be drawn from 2 sides of a unit.
  float4 tmRow0[2];
  float4 tmRow1[2];
  float4 tmRow2[2];
  float4 atlasUvOriginScale;
};

struct PlanarDecalsParamsSet
{
  float4 decalWidths; // One component per decal.
  PlanarDecalTransformAndUV tmAndUv[MAX_PLANAR_DECALS];
};

#endif // PLANAR_DECALS_PARAMS_HLSLI
