#ifndef HEUGHTMAP_QUERY_HLSL_INCLUDED
#define HEUGHTMAP_QUERY_HLSL_INCLUDED 1

#include "landMesh/heightmap_query_result.hlsli"

#define HEIGHTMAP_QUERY_WARP_SIZE 64

#define MAX_HEIGHTMAP_QUERIES_PER_DISPATCH 256
#define MAX_HEIGHTMAP_QUERIES_PER_FRAME MAX_HEIGHTMAP_QUERIES_PER_DISPATCH



struct HeightmapQueryInput
{
  float3 worldPos;
  int riLandclassIndex;
};

#endif