//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

// raytrace interface

#if _TARGET_PC || _TARGET_ANDROID || _TARGET_IOS
#define D3D_HAS_RAY_TRACING 1
// it somehow screws with some ps4 platform headers
#include <generic/dag_enumBitMask.h>

// Opaque handle type, there is no interface needed. Everything goes through the d3d functions
// as all operations require a execution context.
struct RaytraceTopAccelerationStructure;
struct RaytraceBottomAccelerationStructure;

class Sbuffer;

struct RaytraceGeometryDescription
{
  enum class Flags : uint32_t
  {
    NONE = 0x00,
    IS_OPAQUE = 0x01,
    NO_DUPLICATE_ANY_HIT_INVOCATION = 0x02
  };
  enum class Type
  {
    TRIANGLES,
    AABBS,
  };

  struct AABBsInfo
  {
    Sbuffer *buffer;
    uint32_t count;
    uint32_t stride;
    uint32_t offset;
    Flags flags;
  };
  struct TrianglesInfo
  {
    Sbuffer *vertexBuffer;
    Sbuffer *indexBuffer;
    uint32_t vertexCount;
    uint32_t vertexStride;
    uint32_t vertexOffset;
    // use VSDT_* vertex format types
    uint32_t vertexFormat;
    uint32_t indexCount;
    uint32_t indexOffset;
    Flags flags;
  };
  union AnyInfo
  {
    AABBsInfo aabbs;
    TrianglesInfo triangles;
  };

  Type type = Type::TRIANGLES;
  AnyInfo data = {};
};
DAGOR_ENABLE_ENUM_BITMASK(RaytraceGeometryDescription::Flags);

enum class RaytraceBuildFlags : uint32_t
{
  NONE = 0x00,
  // if you want to update the acceleration
  // structure then you need to set this flag
  ALLOW_UPDATE = 0x01,
  ALLOW_COMPACTION = 0x02,
  FAST_TRACE = 0x04,
  FAST_BUILD = 0x08,
  LOW_MEMORY = 0x10
};
DAGOR_ENABLE_ENUM_BITMASK(RaytraceBuildFlags);

struct RaytraceGeometryInstanceDescription
{
  enum Flags
  {
    NONE = 0x00,
    TRIANGLE_CULL_DISABLE = 0x01,
    TRIANGLE_CULL_FLIP_WINDING = 0x02,
    FORCE_OPAQUE = 0x04,
    FORCE_NO_OPAQUE = 0x08
  };
  float transform[12];
  uint32_t instanceId : 24;
  uint32_t mask : 8;
  uint32_t instanceOffset : 24;
  uint32_t flags : 8; // can not use typesafe bitflags here or compiler starts to padd
  RaytraceBottomAccelerationStructure *accelerationStructure;
#if !_TARGET_64BIT
  uint32_t paddToMatchTarget;
#endif
};

enum class RaytraceShaderType
{
  RAYGEN,
  ANY_HIT,
  CLOSEST_HIT,
  MISS,
  INTERSECTION,
  CALLABLE
};

enum class RaytraceShaderGroupType
{
  GENERAL,
  TRIANGLES_HIT,
  PROCEDURAL_HIT,
};
struct RaytraceShaderGroup
{
  RaytraceShaderGroupType type;
  // refers to either a raygen, miss or callable shader
  // only used if type is GENERAL
  uint32_t indexToGeneralShader;
  // refers to a closest hit shader
  // only used if type is TRIANGLES_HIT or PROCEDURAL_HIT
  // can be ~0 to indicate no shader is used
  uint32_t indexToClosestHitShader;
  // refers to a any hit shader
  // only used if type is TRIANGLES_HIT or PROCEDURAL_HIT
  // can be ~0 to indicate no shader is used
  uint32_t indexToAnyHitShader;
  // refers to a intersection shader
  // only used if type is PROCEDURAL_HIT
  // is required if used
  uint32_t indexToIntersecionShader;
};
#endif
