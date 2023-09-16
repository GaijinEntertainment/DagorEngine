#ifndef BIOME_QUERY_RESULT_HLSL_INCLUDED
#define BIOME_QUERY_RESULT_HLSL_INCLUDED 1

struct BiomeQueryResult
{
  int mostFrequentBiomeGroupIndex;
  float mostFrequentBiomeGroupWeight;
  int secondMostFrequentBiomeGroupIndex;
  float secondMostFrequentBiomeGroupWeight;
  float4 averageBiomeColor;
};

#endif