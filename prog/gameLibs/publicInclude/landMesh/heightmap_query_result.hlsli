#ifndef HEIGHTMAP_QUERY_RESULT_HLSL_INCLUDED
#define HEIGHTMAP_QUERY_RESULT_HLSL_INCLUDED 1

struct HeightmapQueryResult
{
  // TODO: pack it more efficiently
  float3 normal;
  float hitDistNoOffset;
  float hitDistWithOffset;
  float hitDistWithOffsetDeform;

  float _padding1;
  float _padding2;
};

#endif