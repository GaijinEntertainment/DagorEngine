#ifndef DAGDP_VOLUME_HLSLI_INCLUDED
#define DAGDP_VOLUME_HLSLI_INCLUDED

#define DAGDP_PREFIX_SUM_GROUP_SIZE 256

struct VolumeVariantGpuData
{
  float density;
  float minTriangleArea2;
  float2 distBasedScale;

  float3 distBasedCenter;
  float distBasedInvRange;

  uint flags;
  float sampleRange;
  uint _padding[2];
};

#define VOLUME_TYPE_BOX 0
#define VOLUME_TYPE_CYLINDER 1
#define VOLUME_TYPE_ELLIPSOID 2
#define VOLUME_TYPE_FULL 3
#define VOLUME_TYPE_COUNT 4

struct VolumeGpuData
{
  float4 itmRow0;
  float4 itmRow1;
  float4 itmRow2;

  float3 axis;
  uint volumeType;

  uint variantIndex;
  uint _padding[3];
};

#define MAX_VB_INDICES 16
#define MAX_DISPATCH_MESHES 512
#define MESH_PLACE_GROUP_SIZE 64

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