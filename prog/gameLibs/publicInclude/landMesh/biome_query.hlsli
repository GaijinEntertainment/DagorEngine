#ifndef BIOME_QUERY_HLSL_INCLUDED
#define BIOME_QUERY_HLSL_INCLUDED 1

#include "landMesh/biome_query_result.hlsli"

#define BIOME_QUERY_WARP_SIZE 16
#define MAX_BIOMES 256
#define MAX_BIOME_GROUPS 256 // to support identical biome -> group mapping for cases when no biome groups are defined
#define MAX_BIOME_QUERIES_PER_FRAME 100
#define MAX_BIOME_QUERIES_PER_DISPATCH 10

#define SQRT_PI 1.7724538509055160273f

// in order to roughly map samples to not be more dense than texels
#define MIN_SAMPLE_SCALE_RADIUS_TEXEL_SPACE (BIOME_QUERY_WARP_SIZE / SQRT_PI)

struct BiomeQueryInput
{
  float3 worldPos;
  float radius;
};

#endif