//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// raytrace interface

#if _TARGET_PC || _TARGET_SCARLETT || _TARGET_ANDROID || _TARGET_IOS || _TARGET_C2
#define D3D_HAS_RAY_TRACING 1
// it somehow screws with some ps4 platform headers
#include <generic/dag_enumBitMask.h>

// Opaque handle type, there is no interface needed. Everything goes through the d3d functions
// as all operations require a execution context.
struct RaytraceTopAccelerationStructure;
struct RaytraceBottomAccelerationStructure;

// Simple wrapper for API functions that can take either top or bottom acceleration structures.
struct RaytraceAnyAccelerationStructure
{
  RaytraceTopAccelerationStructure *top = nullptr;
  RaytraceBottomAccelerationStructure *bottom = nullptr;

  RaytraceAnyAccelerationStructure() = default;
  RaytraceAnyAccelerationStructure(const RaytraceAnyAccelerationStructure &) = default;
  RaytraceAnyAccelerationStructure &operator=(const RaytraceAnyAccelerationStructure &) = default;
  RaytraceAnyAccelerationStructure(RaytraceTopAccelerationStructure *t) : top{t} {}
  RaytraceAnyAccelerationStructure(RaytraceBottomAccelerationStructure *b) : bottom{b} {}
  explicit operator bool() const { return (nullptr != top) || (nullptr != bottom); }
};

struct RaytraceAccelerationStructureGpuHandle
{
  uint64_t handle;
};

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
    Sbuffer *transformBuffer;
    Sbuffer *vertexBuffer;
    Sbuffer *indexBuffer;
    // In matrices (12 floats)
    uint32_t transformOffset;
    uint32_t vertexCount;
    uint32_t vertexStride;
    uint32_t vertexOffset;
    uint32_t vertexOffsetExtraBytes;
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

namespace raytrace
{
struct BottomAccelerationCreateInfo
{
  /// An array of geometry descriptions that describes all the geometry the structure should encapsulate.
  const RaytraceGeometryDescription *geometryDesc = nullptr;
  /// Array size of geometryDesc.
  uint32_t geometryDescCount = 0;
  /// Flags that influence some characteristics and valid usage of the structure.
  RaytraceBuildFlags flags = RaytraceBuildFlags::NONE;
};
/// Provides all necessary information to build or update a ray trace bottom acceleration structure.
struct BottomAccelerationStructureBuildInfo : BottomAccelerationCreateInfo
{
  /// Do an update instead of a rebuild.
  /// Only valid to use when previous rebuilds had the flag RaytraceBuildFlags::ALLOW_UPDATE set.
  bool doUpdate = false;
  /// Needs to be set when RaytraceBuildFlags::ALLOW_COMPACTION flag is set, otherwise ignored.
  /// At the end of the build process, the GPU will write the required size of a compacted
  /// variant of resulting data structure content to this buffer at the offset location.
  /// This can be used to create smaller compacted acceleration structures.
  Sbuffer *compactedSizeOutputBuffer = nullptr;
  /// Ignored when compactedSizeOutputBuffer is nullptr.
  /// compactedSizeOutputBuffer has to be large enough to store a uint64_t at that offset.
  /// Offset has to be 64 bit aligned.
  uint32_t compactedSizeOutputBufferOffsetInBytes = 0;
  /// Buffer that provides scratch space needed for the build, size requirements are
  /// derived from scratchSpaceBufferOffsetInBytes + scratchSpaceBufferSizeInBytes.
  /// @note Multiple builds can safely use the same buffer without synchronization as long as
  /// they do not overlap in used region of the buffer.
  /// @note May be nullptr when the corresponding scratch buffer size provided by
  /// d3d::create_raytrace_bottom_acceleration_structure was 0.
  /// @note A frame end is a implicit flush of every buffer. So no synchronization in
  /// between frames is needed.
  /// @warning Currently this is optional, but this behavior is deprecated until all users
  /// have updated to provide this buffer when needed.
  Sbuffer *scratchSpaceBuffer = nullptr;
  /// Offset into scratchSpaceBuffer, buffer has to be large enough to provide
  /// space for scratchSpaceBufferOffsetInBytes + scratchSpaceBufferSizeInBytes.
  uint32_t scratchSpaceBufferOffsetInBytes = 0;
  /// Usable space of scratchSpaceBuffer, this has to be at least as large as the
  /// provided build_scratch_size_in_bytes value for the build structure for a rebuild
  /// or update_scratch_size_in_bytes for updates.
  uint32_t scratchSpaceBufferSizeInBytes = 0;
};

struct BatchedBottomAccelerationStructureBuildInfo
{
  /// The BLAS for an element of a batched BLAS build.
  RaytraceBottomAccelerationStructure *as = nullptr;
  /// The descriptor for an element of a batched BLAS build.
  BottomAccelerationStructureBuildInfo basbi;
};

/// Provides all necessary information to build or update a ray trace top acceleration structure.
struct TopAccelerationStructureBuildInfo
{
  /// Do an update instead of a rebuild.
  /// Only valid to use when previous rebuilds had the flag RaytraceBuildFlags::ALLOW_UPDATE set.
  bool doUpdate = false;
  /// A buffer, filled with instance descriptors.
  Sbuffer *instanceBuffer = nullptr;
  /// The number of instance descriptors in the instanceBuffer.
  uint32_t instanceCount = 0;
  /// Buffer that provides scratch space needed for the build, size requirements are
  /// derived from scratchSpaceBufferOffsetInBytes + scratchSpaceBufferSizeInBytes.
  /// @note Multiple builds can safely use the same buffer without synchronization as long as
  /// they do not overlap in used region of the buffer.
  /// @note May be nullptr when the corresponding scratch buffer size provided by
  /// d3d::create_raytrace_top_acceleration_structure was 0.
  /// @note A frame end is a implicit flush of every buffer. So no synchronization in
  /// between frames is needed.
  /// @warning Currently this is optional, but this behavior is deprecated until all users
  /// have updated to provide this buffer when needed.
  Sbuffer *scratchSpaceBuffer = nullptr;
  /// Offset into scratchSpaceBuffer, buffer has to be large enough to provide
  /// space for scratchSpaceBufferOffsetInBytes + scratchSpaceBufferSizeInBytes.
  uint32_t scratchSpaceBufferOffsetInBytes = 0;
  /// Usable space of scratchSpaceBuffer, this has to be at least as large as the
  /// provided build_scratch_size_in_bytes value for the build structure for a rebuild
  /// or update_scratch_size_in_bytes for updates.
  uint32_t scratchSpaceBufferSizeInBytes = 0;
  /// Flags that influence some characteristics and valid usage of the structure.
  RaytraceBuildFlags flags = RaytraceBuildFlags::NONE;
};

struct BatchedTopAccelerationStructureBuildInfo
{
  /// The TLAS for an element of a batched BLAS build.
  RaytraceTopAccelerationStructure *as = nullptr;
  /// The TLAS for an element of a batched BLAS build.
  TopAccelerationStructureBuildInfo tasbi;
};

/// Checks if any VSDT_* is supported by the device as source for building bottom acceleration structures.
/// When 'DeviceDriverCapabilitiesBase::hasRayAccelerationStructure' is false, then this will always return false.
/// When 'DeviceDriverCapabilitiesBase::hasRayAccelerationStructure' is true, then this has to return true for
/// VSDT_FLOAT3, which is always required to be supported, any other format is optional and may or may not return
/// true.
bool check_vertex_format_support_for_acceleration_structure_build(uint32_t format);
} // namespace raytrace
#endif
