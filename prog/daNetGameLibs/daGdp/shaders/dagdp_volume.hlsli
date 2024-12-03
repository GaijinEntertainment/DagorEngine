#ifndef DAGDP_VOLUME_HLSLI_INCLUDED
#define DAGDP_VOLUME_HLSLI_INCLUDED

#define DAGDP_PREFIX_SUM_GROUP_SIZE 256

// Reference: gpu_objects_const.hlsli
struct MeshIntersection
{
  float4 bboxItmRow0;
  float4 bboxItmRow1;
  float4 bboxItmRow2;

  int startIndex;
  int numFaces;
  int baseVertex;
  int stride;

  float4 tmRow0;
  float4 tmRow1;
  float4 tmRow2;

  uint areasIndex;
  uint vbIndex;
  uint variantIndex;
  uint uniqueId;
};

#endif // DAGDP_VOLUME_HLSLI_INCLUDED