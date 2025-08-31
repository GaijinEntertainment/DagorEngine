#ifndef UNPACKED_MOA_SHAPES
#define UNPACKED_MOA_SHAPES

#define SDF_SHAPE(name, _ecs_name, val) static const int SDF_SHAPE_ ## name = val;
#include <collimatorMoaCore/shape_tags.hlsli>
#undef SDF_SHAPE

struct UnpackedCircle
{
  float2 center;
  float radius;
};

struct UnpackedRing
{
  float2 center;
  float radiusNear;
  float radiusFar;
};

struct UnpackedLine
{
  float2 begin,end;
  float halfWidth;
};

struct UnpackedTriangle
{
  float2 a,b,c;
};

struct UnpackedArc
{
  float2 center;
  float radius;
  float halfWidth;
  float4 rotation;
  float2 edgePlaneN;
  float2 edgePoint;
};

#endif