#ifndef SPLINE_GEN_BUFFER_HLSLI
#define SPLINE_GEN_BUFFER_HLSLI

#define SPLINE_GEN_COMPUTE_DIM 256
#define SPLINE_GEN_ATTACHMENT_COMPUTE_X 32
#define SPLINE_GEN_ATTACHMENT_COMPUTE_Y 8

struct SplineGenVertex
{
  float3 worldPos;
  uint normal;
  float2 texcoord;
  uint tangent;
  uint bitangent;
};

struct SplineGenSpline
{
  float3 pos;
  uint padding1;
  float3 tangent;
  uint padding2;
  float3 bitangent;
  uint padding3;
};

typedef uint BatchId;
//declare size, because float3x4 is not a valid c++ type
#define ATTACHMENT_DATA_SIZE 48
struct SplineGenInstance
{
  float3 radius;
  float displacementStrength;
  float2 tcMul;
  float displacementLod;
  uint flags;
  float3 bbox_lim0;
  float objSizeMul;
  float3 bbox_lim1;
  float objStripeMul;
  uint2 padding1;
  uint batchIdsStart;
  uint objCount;
};
//for flags
#define PREV_VB_VALID (0x1u)
#define PREV_ATTACHMENT_DATA_VALID (0x2u)

#define INVALID_INSTANCE_ID 0xFFFFFFFF
typedef uint InstanceId;

#endif