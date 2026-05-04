#ifndef SPLINE_GEN_BUFFER_HLSLI
#define SPLINE_GEN_BUFFER_HLSLI

#define SPLINE_GEN_COMPUTE_DIM 256
#define SPLINE_GEN_ATTACHMENT_COMPUTE_X 32
#define SPLINE_GEN_ATTACHMENT_COMPUTE_Y 8

struct SplineGenSpline
{
  float3 pos;
  float radius;
  float3 tangent;
  float emissiveIntensity;
  float3 bitangent;

  // Shape data
  float distToClosest;
  int closestShapeOfs;
  int closestShapeSegmentNr;
  int secondClosestShapeOfs;
  int secondClosestShapeSegmentNr;
};

struct SplineGenVertex
{
  float3 worldPos;
  uint normal;
  float2 texcoord;
  uint tangent;
  uint bitangent;
};

typedef uint BatchId;
//declare size, because float3x4 is not a valid c++ type
#define ATTACHMENT_DATA_SIZE 48
struct SplineGenInstance
{
  float2 tcMul;
  float displacementLod;
  uint flags;

  float3 bbox_lim0;
  float objSizeMul;

  float3 bbox_lim1;
  float objStripeMul;

  float displacementStrength;
  float inverseMaxRadius;
  uint batchIdsStart;
  uint objCount;

  float4 emissiveColor;

  float4 uvOffsets;
  float4 uvScales;
  float2 glassParams;
  float uvInterpolationValue;

  float cylinderStartOffset;

  float2 additionalThicknessBounds;
  float surfaceOpaqueness;
  uint padding0;
};

struct SplineGenShapeData
{
  // xy - position of point, zw - UV coordinates
  // (U as in Useless; that's only really here for padding)
  float4 firstPos;
  float4 lastPos;
};

//for flags
#define PREV_SB_VALID (0x1u)
#define PREV_ATTACHMENT_DATA_VALID (0x2u)

#define INVALID_INSTANCE_ID 0xFFFFFFFF
typedef uint InstanceId;

#endif