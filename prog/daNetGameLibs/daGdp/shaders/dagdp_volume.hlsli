#ifndef DAGDP_VOLUME_HLSLI_INCLUDED
#define DAGDP_VOLUME_HLSLI_INCLUDED

#define DAGDP_PREFIX_SUM_GROUP_SIZE 256

struct VolumeVariantGpuData
{
  float density;
  float minTriangleArea2;
  uint _padding[2];
};

#define VOLUME_TYPE_BOX 0
#define VOLUME_TYPE_CYLINDER 1
#define VOLUME_TYPE_ELLIPSOID 2
#define VOLUME_TYPE_COUNT 3

struct VolumeGpuData
{
  float4 itmRow0;
  float4 itmRow1;
  float4 itmRow2;

  uint volumeType;
  uint variantIndex;
  uint _padding[2];
};

// Reference: gpu_objects_const.hlsli
struct MeshIntersection
{
  int startIndex;
  int numFaces;
  int baseVertex;
  int stride;

  float4 meshTmRow0;
  float4 meshTmRow1;
  float4 meshTmRow2;

  uint areasIndex;
  uint vbIndex;
  uint volumeIndex;
  uint uniqueId;
};

#endif // DAGDP_VOLUME_HLSLI_INCLUDED