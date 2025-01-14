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
#include <debug/dag_assert.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_shaderLibraryObject.h>
#include <drv/3d/dag_samplerHandle.h>

class BaseTexture;

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

/// Shader group describing a ray gen group
struct RayGenShaderGroupMember
{
  /// Reference of a ray gen shader in a shader library.
  /// Has to be a valid reference.
  ShaderInLibraryReference rayGen;
};
/// Shader group describing a miss group
struct MissShaderGroupMember
{
  /// Reference of a miss shader in a shader library.
  /// Has to be a valid reference.
  ShaderInLibraryReference miss;
};
/// Shader group describing a callable group
struct CallableShaderGroupMember
{
  /// Reference of a callable shader
  /// Has to be a valid reference.
  ShaderInLibraryReference callable;
};
/// Shader group describing a triangle hit group
/// At least one of 'anyHit' and 'closestHit' members has to be a valid reference.
struct TriangleShaderGroupMember
{
  /// Reference of a any hit shader
  /// Can be a valid or a null reference.
  ShaderInLibraryReference anyHit;
  /// Reference of a closest git shader
  /// Can be a valid or a null reference.
  ShaderInLibraryReference closestHit;
};
/// Shader group describing a procedural hit group
struct ProceduralShaderGroupMember : TriangleShaderGroupMember
{
  /// Reference of a intersection shader
  /// Has to be a valid reference.
  ShaderInLibraryReference intersection;
};
/// Describes which kind of shader group a shader belongs to.
enum class ShaderGroup
{
  /// Undefined, indicates that ShaderGroupMember is uninitialized.
  Undefined,
  /// Shader group is of RayGenShaderGroupMember type
  RayGen,
  /// Shader group is of MissShaderGroupMember type
  Miss,
  /// Shader group is of CallableShaderGroupMember type
  Callable,
  /// Shader group is of TriangleShaderGroupMember type
  Triangle,
  /// Shader group is of ProceduralShaderGroupMember type
  Procedural
};
/// Defines a shader group of a specific type
struct ShaderGroupMember
{
  /// Indicates to which group this member belongs and which member variable is valid to use.
  ShaderGroup group = ShaderGroup::Undefined;
  union
  {
    /// Valid when 'type' is of ShaderGroupType::RayGen
    /// Specifies a ray gen group
    RayGenShaderGroupMember rayGen;
    /// Valid when 'type' is of ShaderGroupType::Miss
    /// Specifies a miss group
    MissShaderGroupMember miss;
    /// Valid when 'type' is of ShaderGroupType::Callable
    /// Specifies a callable group
    CallableShaderGroupMember callable;
    /// Valid when 'type' is of ShaderGroupType::Triangle
    /// Specifies a triangle group
    TriangleShaderGroupMember triangle;
    /// Valid when 'type' is of ShaderGroupType::Procedural
    /// Specifies a procedural group
    ProceduralShaderGroupMember procedural;
  };

  /// Constructs empty state
  ShaderGroupMember() = default;
  /// Default copy constructor
  ShaderGroupMember(const ShaderGroupMember &) = default;
  /// Constructs this with a state that represents a ray gen group
  ShaderGroupMember(const RayGenShaderGroupMember &rg) : group{ShaderGroup::RayGen}, rayGen{rg} {}
  /// Constructs this with a state that represents a miss group
  ShaderGroupMember(const MissShaderGroupMember &m) : group{ShaderGroup::Miss}, miss{m} {}
  /// Constructs this with a state that represents a callable group
  ShaderGroupMember(const CallableShaderGroupMember &c) : group{ShaderGroup::Callable}, callable{c} {}
  /// Constructs this with a state that represents a triangle group
  ShaderGroupMember(const TriangleShaderGroupMember &tr) : group{ShaderGroup::Triangle}, triangle{tr} {}
  /// Constructs this with a state that represents a procedural group
  ShaderGroupMember(const ProceduralShaderGroupMember &pr) : group{ShaderGroup::Procedural}, procedural{pr} {}

  /// Dispatching helper, will switch over the group member and invokes clb with the corresponding union member.
  /// @param clb Callback that may receive the rayGen, miss, callable, triangle or procedural member as input.
  template <typename T>
  void visit(T clb)
  {
    switch (group)
    {
      case ShaderGroup::Undefined: break;
      case ShaderGroup::RayGen: clb(rayGen); break;
      case ShaderGroup::Miss: clb(miss); break;
      case ShaderGroup::Callable: clb(callable); break;
      case ShaderGroup::Triangle: clb(triangle); break;
      case ShaderGroup::Procedural: clb(procedural); break;
    }
  }
};

/// Pipeline type, wraps a pipeline object and shader handle table.
/// Drivers that implement pipelines should implement a derived type of this, that provides
/// a appropriate constructor and access to the internals when needed.
/// To adapt input pipelines this can be achieved by a "copy" constructor that uses the copy constructor
/// of this type to initialize the derived object.
class Pipeline
{
protected:
  /// Object handle for the driver.
  void *driverObject = nullptr;
  /// Shader handle table of all shaders of that pipeline, in the order they where defined during creation and expansion.
  const uint8_t *shaderHandleTable = nullptr;
  /// Size in bytes of a single shader handle in shaderHandleTable.
  uint32_t shaderHandleSize = 0;
  /// Number of shaders of the pipeline and in shaderHandleTable.
  uint32_t shaderCount = 0;

  /// Drivers implementing this should create a derived type to gain access to this constructor.
  Pipeline(void *obj, const uint8_t *sht, uint32_t shs, uint32_t sc) :
    driverObject{obj}, shaderHandleTable{sht}, shaderHandleSize{shs}, shaderCount{sc}
  {}

public:
  constexpr Pipeline() = default;

  Pipeline(const Pipeline &) = default;
  Pipeline &operator=(const Pipeline &) = default;

  Pipeline(Pipeline &&) = default;
  Pipeline &operator=(Pipeline &&) = default;

  /// Returns the number of shaders the pipeline object makes usable.
  uint32_t getShaderCount() const { return shaderCount; }
  /// Returns a shader handle pointer with a size returned by getShaderHandleSize.
  dag::ConstSpan<const uint8_t> getShaderHandle(uint32_t index) const
  {
    G_ASSERT_RETURN(index < shaderCount, {});
    return make_span_const(&shaderHandleTable[index * shaderHandleSize], shaderHandleSize);
  }
  /// Helper to copy a shader handle at the target memory location.
  void writeShaderHandleTo(uint32_t index, dag::Span<uint8_t> target) const
  {
    G_ASSERT_RETURN(index < shaderCount, );
    G_ASSERT_RETURN(target.size() >= shaderHandleSize, );
    memcpy(target.data(), &shaderHandleTable[index * shaderHandleSize], shaderHandleSize);
  }
  /// Retrieves the size of a handle returned by getShaderHandle
  uint32_t getShaderHandleSize() const { return shaderHandleSize; }

  friend bool operator==(const Pipeline &l, const Pipeline &r) { return l.driverObject == r.driverObject; }
  friend bool operator!=(const Pipeline &l, const Pipeline &r) { return !(l == r); }
};

/// Represents a "null pointer" to a pipeline object.
inline constexpr Pipeline InvalidPipeline{};

/// Describes a expansion of a existing pipeline
struct PipelineExpandInfo
{
  /// Debug name of the pipeline
  const char *name = nullptr;
  /// A set of shader group members that should be appended to the pipeline, in the order they appear in the array.
  dag::ConstSpan<const ShaderGroupMember> groupMemebers;
  /// Indicates that the pipeline may be expanded over time with calls to expand_pipeline.
  /// Calls to expand_pipeline without this flag are invalid.
  /// This flag is not inherited by the expanded pipeline and has to be set on expansion when the expanded pipeline should be capable
  /// to be further expanded.
  bool expandable = false;
  /// Pipelines with this flag will always ignore triangle geometry during traversal.
  /// Requires the hasRayTraceSkipPrimitiveType feature to be available, otherwise pipeline create / expand will fail.
  bool alwaysIgnoreTriangleGeometry = false;
  /// Pipelines with this flag will always ignore procedural geometry during traversal.
  /// Requires the hasRayTraceSkipPrimitiveType feature to be available, otherwise pipeline create / expand will fail.
  bool alwaysIgnoreProceduralGeometry = false;
};

/// Describes creation data for a new pipeline
struct PipelineCreateInfo : PipelineExpandInfo
{
  /// The pipeline should be prepared to add any ray gen shaders from the set of libraries.
  /// Having a pipeline prepared for any library increases the likelihood that a expand will work and does not require a recreate.
  dag::ConstSpan<const ShaderLibrary> rayGenForwardRefernces;
  /// The pipeline should be prepared to add any miss shaders from the set of libraries.
  /// Having a pipeline prepared for any library increases the likelihood that a expand will work and does not require a recreate.
  dag::ConstSpan<const ShaderLibrary> missForwardReferences;
  /// The pipeline should be prepared to add any hit shaders from the set of libraries.
  /// Having a pipeline prepared for any library increases the likelihood that a expand will work and does not require a recreate.
  dag::ConstSpan<const ShaderLibrary> hitForwardRefrences;
  /// The pipeline should be prepared to add any callable shaders from the set of libraries.
  /// Having a pipeline prepared for any library increases the likelihood that a expand will work and does not require a recreate.
  dag::ConstSpan<const ShaderLibrary> callableForwardReferences;
  /// Max number of recursion depth, has to be less or equal to Driver3dDesc::raytraceMaxRecursion.
  /// With 0 the driver will try to guess on basis of some meta data of all used shaders.
  /// A min value of 1 means that the RayGen shader is the only shader that can shoot rays.
  uint32_t maxRecursionDepth = 0;
  /// Max payload size, in bytes, that is transmitted from hit and miss shaders to ray cast invocations.
  /// With 0 the driver will try to guess on basis of some meta data of all used shaders.
  uint32_t maxPayloadSize = 0;
  /// Max size of attributes, usually this is 2 * size of float, for build-in triangle intersection,
  /// but for procedural intersection this can be anything.
  /// With 0 and no procedural hit groups, the driver will assume 2 * size of float, otherwise it
  /// will guess on basis of some meta data of all used procedural hit groups and triangle hit
  /// groups.
  uint32_t maxAttributeSize = 0;
};

/// Creates a new pipeline object with the given properties defined by the 'pci' parameter.
/// May return InvalidPipeline on an error.
/// In case of a device reset, the pipeline becomes unusable and has to be destroyed and recreated.
Pipeline create_pipeline(const PipelineCreateInfo &pci);
/// Tries to expand a existing pipeline object.
/// This will return a new pipeline object, the old one is can still be used normally (and concurrently) and is kept as is and may be
/// destroyed at any time with destroy_pipeline should the new object replace the old one.
/// May return InvalidPipeline on an error or when an expansion was not be possible.
/// This is to be cheaper than 'create_pipeline' with the same existing and added shaders, when the driver
/// supports pipeline expansion natively. In any case it should not perform worse than 'create_pipeline'.
/// Native support for pipeline expansion is not required.
/// In some cases, driver may not be able to expand the pipeline, even when native expansion is supported,
/// this may be because of various cases, like pipeline layout / root signature not being able to supply all the resource slots needed
/// by the expanded pipeline.
Pipeline expand_pipeline(const Pipeline &pipeline, const PipelineExpandInfo &pei);
/// Destroys a pipeline object, pending ray dispatches will be guaranteed to be executed before the object
/// is actually destroyed. This function will not block to make this guarantee.
void destroy_pipeline(Pipeline &p);

/// Properties for any shader group a shader binding table
struct ShaderBindingTableGroupInfo
{
  /// Minimum of resources references the shader record has to be able to store.
  /// As this is a minimum, the driver may use more for total size calculation should
  /// shaders in that shader group need more space.
  /// In most cases, leaving this as 0 is fine.
  uint32_t minResourceReferences = 0;
  /// Minimum number of bytes of constant data the shader record has to be able to store.
  /// As this is a minimum, the driver may use more for total size calculation should
  /// shaders in that shader group need more space.
  /// In most cases, leaving this as 0 is fine.
  uint32_t minConstantDataSizeInBytes = 0;
};

/// Describes a shader binding table layout.
struct ShaderBindingTableDefinition
{
  /// Properties of the ray gen group
  ShaderBindingTableGroupInfo rayGenGroup;
  /// Properties of the miss group
  ShaderBindingTableGroupInfo missGroup;
  /// Properties of the hit group
  ShaderBindingTableGroupInfo hitGroup;
  /// Properties of the callable group
  ShaderBindingTableGroupInfo callableGroup;
  /// Create flags the buffer should have.
  /// Note that some usages are not working (well), like formatted srv / uav uses, use instead byte address or structured.
  /// Note be mindful of the create flags, as this table is used during the execution dispatch command to kick off all sorts of shaders
  /// on each ray cast and shader call will read from the table.
  uint32_t bufferCreateFlags = 0;
};

/// Properties of a shader group in a shader binding table.
struct ShaderBindingTableGroupProperties
{
  /// Total size of paired data of a entry of that shader group.
  /// This includes resource references and constant data.
  uint32_t dataSizeInBytes = 0;
  /// Total size (data plus shader handle) of an entry aligned to the platforms requirement size.
  /// This alignment has to be a value that results in valid values for arrays of the shader group.
  /// In case of the ray gen group, there are only single values and this means an array of those
  /// would be an array of groups not an a group of an array of shader records.
  uint32_t entryAlignedSizeInBytes = 0;
};

/// The driver calculated properties of the shader binding table to be used with create_sbuffer to create a buffer that can be used as
/// shader binding table.
/// Typical use is:
/// auto props = get_shader_binding_table_buffer_properties(myBindingTableProperties, myPipeline);
/// auto buf = d3d::create_sbuffer(props.createStructSize, props.calculateElementCount({ray_gen_count, miss_count, hit_count,
/// call_count}),
///   props.createFlags, props.createFormat, "my shader binding table buffer");
struct ShaderBindingTableBufferProperties
{
  /// Structure size that has to be used with create_sbuffer.
  /// Using a different value may result in a device reset.
  /// Only 0 on error, otherwise larger than 0.
  uint32_t createStructSize = 0;
  /// Create flags required to pass to create_sbuffer.
  /// Using different flags may result in device reset or the buffer being not usable as a shader binding table.
  uint32_t createFlags = 0;
  /// Create format required to pass to create_sbuffer.
  /// Using a different format may result in device reset or the buffer being not usable as a shader binding table.
  uint32_t createFormat = 0;
  /// Offsets of each group has to be aligned to this value. Note that this may be (and will be on DX12) different than
  /// ShaderBindingTableGroupProperties::entryAlignedSize. Not properly aligning the offsets may result in a device reset.
  uint32_t groupOffsetAlignmentSizeInBytes = 0;
  /// Properties of the ray gen shader groups.
  ShaderBindingTableGroupProperties rayGenGroup;
  /// Properties of the miss shader groups.
  ShaderBindingTableGroupProperties missGroup;
  /// Properties of the hit shader groups.
  ShaderBindingTableGroupProperties hitGroup;
  /// Properties of the callable groups.
  ShaderBindingTableGroupProperties callableGroup;

  /// Counts of shader groups to calculate the size of a buffer used as a shader binding table.
  struct GroupCounts
  {
    /// Number of ray gen shader groups. Usually 1.
    uint32_t rayGenGroupCount = 0;
    /// Number of miss shader groups. Usually 1.
    uint32_t missGroupCount = 0;
    /// Number of hit shader groups.
    uint32_t hitGroupCount = 0;
    /// Number of callable groups.
    uint32_t callableGroupCount = 0;
  };

  /// Calculates the number of elements input for sbuffer create.
  uint32_t calculateElementCount(const GroupCounts &counts) const
  {
    uint32_t groupAlignAdd = groupOffsetAlignmentSizeInBytes - 1;
    uint32_t groupAlignMask = ~groupAlignAdd;
    uint32_t rayGenGroupBytes = rayGenGroup.entryAlignedSizeInBytes * counts.rayGenGroupCount;
    uint32_t missGroupBytes = missGroup.entryAlignedSizeInBytes * counts.missGroupCount;
    uint32_t hitGroupBytes = hitGroup.entryAlignedSizeInBytes * counts.hitGroupCount;
    uint32_t callableGroupBytes = callableGroup.entryAlignedSizeInBytes * counts.callableGroupCount;
    // each group has to be aligned to groupOffsetAlignmentSizeInBytes, technically we could save some bytes on the last group a the
    // end.
    // Note rayGenGroupBytes is already aligned because we can't have packed arrays of ray gen entries
    uint32_t totalBytes = rayGenGroupBytes + ((missGroupBytes + groupAlignAdd) & groupAlignMask) +
                          ((hitGroupBytes + groupAlignAdd) & groupAlignMask) + ((callableGroupBytes + groupAlignAdd) & groupAlignMask);
    // convert to number of elements and with rounding up to next full element
    return (totalBytes + createStructSize - 1) / createStructSize;
  }
};

/// Calculates the properties of a buffer to be use as a shader binding table with the given properties.
/// Returns default initialized data structure on error and on success values to create a sbuffer for shader binding table use.
ShaderBindingTableBufferProperties get_shader_binding_table_buffer_properties(const ShaderBindingTableDefinition &sbtd,
  const Pipeline &pipeline);

/// Resource binding table, order is not important.
/// Slots that are undefined by this table that are used by the pipeline of the dispatch will be using the corresponding null resource.
/// Definitions by this table with null for resource references will be ignored and do not contribute to possible slot usage clashes.
/// 'constantBufferReads' use the 'b' register space.
/// 'bufferReads', 'textureReads' and 'accelerationStructureReads' use the 't' register space.
/// 'bufferWrites' and 'textureWrites' use the 'u' register space.
/// Behavior is undefined when slot usages clash. A usage clash happens when the same register slot of the same register space is used
/// by multiple resource usage definitions, for example if 'bufferReads' has a entry with a 'slot' value and 'textureReads' has a entry
/// with a 'slot' with the same value.
struct ResourceBindingTable
{
  /// Defines a constant buffer read on a b register slot
  struct ConstBufferRead
  {
    /// Buffer to read from
    Sbuffer *buffer = nullptr;
    /// b register slot
    uint32_t slot = 0;
    /// Offset to read from buffer
    uint32_t offsetInBytes = 0;
    /// Size of the constant data range
    uint32_t sizeInBytes = 0;
  };
  /// Defines a buffer read on a t register slot
  struct BufferRead
  {
    /// Buffer to read from
    Sbuffer *buffer = nullptr;
    /// t register slot
    uint32_t slot = 0;
  };
  /// Defines a texture read on a t register slot
  struct TextureRead
  {
    /// Texture to read from
    BaseTexture *texture = nullptr;
    /// t register slot
    uint32_t slot = 0;
  };
  /// Defines a acceleration structure read on a t register slot
  struct AccelerationStructureRead
  {
    /// Acceleration structure to read from
    RaytraceTopAccelerationStructure *structure = nullptr;
    /// t register slot
    uint32_t slot = 0;
  };
  /// Defines a buffer write on a u register slot
  struct BufferWrite
  {
    /// Buffer to write to
    Sbuffer *buffer = nullptr;
    /// u register slot
    uint32_t slot = 0;
  };
  /// Defines a texture write on a u register slot
  struct TextureWrite
  {
    /// Texture to write to
    BaseTexture *texture = nullptr;
    /// u register slot
    uint32_t slot = 0;
    /// Mip map level to write to
    uint32_t mipIndex = 0;
    /// Array layer to write to
    uint32_t arrayIndex = 0;
    /// Access data as an uint32
    bool viewAsUI32 = false;
  };
  /// Defines a sampler read on a s register slot
  struct SamplerRead
  {
    /// Sampler to read
    d3d::SamplerHandle sampler = d3d::INVALID_SAMPLER_HANDLE;
    /// s register slot
    uint32_t slot = 0;
  };
  /// Constant data that is supposed to be pushed into immediate registers, this data is usually written directly to
  /// to the command buffer of the device and update some constant registers of the device.
  dag::ConstSpan<const uint32_t> immediateConstants;
  /// Binding set of constant buffers.
  dag::ConstSpan<ConstBufferRead> constantBufferReads;
  /// Binding set for read only buffers (eg SRV).
  dag::ConstSpan<BufferRead> bufferReads;
  /// Binding set for read only textures (eg SRV).
  dag::ConstSpan<TextureRead> textureReads;
  /// Binding set for acceleration structures.
  dag::ConstSpan<AccelerationStructureRead> accelerationStructureReads;
  /// Binding set for read write buffers (eg UAV).
  dag::ConstSpan<BufferWrite> bufferWrites;
  /// Binding set for read write textures (eg UAV).
  dag::ConstSpan<TextureWrite> textureWrites;
  /// Binding set for samplers.
  dag::ConstSpan<SamplerRead> samples;
};

/// Shader binding table entry info for groups with one entry.
struct ShaderBindingTableOffset
{
  /// Buffer that hold the binding table entry / entries.
  Sbuffer *bindingTableBuffer = nullptr;
  /// Offset in the buffer to the first entry.
  uint32_t offsetInBytes = 0;
  /// Size of each entry, this includes handle size plush shader record size.
  uint32_t sizeInBytes = 0;
};

/// Extends ShaderBindingTableOffset by a stride value for groups with more than one entry.
struct ShaderBindingTableSubRange : ShaderBindingTableOffset
{
  /// Stride from entry to entry.
  uint32_t strideInBytes = 0;
};

/// Shader binding table set, it contains all the binding table data for all shader groups.
struct ShaderBindingTableSet
{
  /// ray gen group binding table data.
  ShaderBindingTableOffset rayGenGroup;
  /// miss group binding table data.
  ShaderBindingTableSubRange missGroup;
  /// hit group binding table data.
  ShaderBindingTableSubRange hitGroup;
  /// callable group binding table data.
  ShaderBindingTableSubRange callableGroup;
};

/// A ray dispatch group definition.
struct RayDispatchGroup
{
  /// Virtual GPU address to the beginning of the shader binding table of that group.
  uint64_t virtualGpuAddress;
  /// Total size of the group in the shader binding table.
  uint64_t sizeInBytes;
};

/// A ray dispatch group definition, for groups with multiple entries.
struct RayDispatchGroupTable : RayDispatchGroup
{
  /// Stride in bytes of a entry of the group in the shader binding table.
  uint64_t strideInBytes;
};

/// Table of shader groups for ray dispatches.
struct RayDispatchGroupTableSet
{
  /// Properties of the ray gen shader group.
  RayDispatchGroup rayGen;
  /// Properties of the miss shader group.
  RayDispatchGroupTable miss;
  /// Properties of the hit shader group.
  RayDispatchGroupTable hit;
  /// Properties of the callable shader group.
  RayDispatchGroupTable callable;
};

/// Describes the memory layout of a indirect dispatch command entry.
struct RayDispatchIndirectArguments
{
  /// Dispatch table set that should be used by the indirect command.
  RayDispatchGroupTableSet dispatchTableSet;
  /// Dispatch grid width.
  uint32_t width;
  /// Dispatch grid height.
  uint32_t height;
  /// Dispatch grid depth.
  uint32_t depth;
};

/// Parameters of a ray dispatch command.
struct RayDispatchParameters
{
  /// Shader binding table set.
  ShaderBindingTableSet shaderBindingTableSet;
  /// Dispatch grid width.
  uint32_t width;
  /// Dispatch grid height.
  uint32_t height;
  /// Dispatch grid depth.
  uint32_t depth;
};

/// Common parameters for a indirect dispatch command, with and without a count buffer.
struct RayDispatchIndirectBaseParameters
{
  /// Buffer storing indirect values of the type 'RayDispatchIndirectArguments'.
  Sbuffer *indirectBuffer;
  /// Offset into 'directBuffer', value has to be aligned to <TODO>.
  uint32_t indirectByteOffset;
  /// Stride of each entry in 'indirectBuffer', value has to be aligned to <TODO>.
  uint32_t indirectByteStride;
};

/// Parameters for a indirect dispatch command, with predefined count.
struct RayDispatchIndirectParameters : RayDispatchIndirectBaseParameters
{
  /// Number of indirect dispatches to execute.
  uint32_t count;
};

/// Parameters for a indirect dispatch command, with a count fetched from a buffer, with a upper limit.
struct RayDispatchIndirectCountParameters : RayDispatchIndirectBaseParameters
{
  /// Buffer that holds the counter of indirect invocations
  Sbuffer *countBuffer;
  /// Offset into 'countBuffer', has to be aligned to <TODO>
  uint32_t countByteOffset;
  /// The value read from 'countBuffer' can not exceed this value.
  uint32_t maxCount;
};

/// Dispatches the ray gen shader rederenced by the 'shaderBindingTableSet' member of 'rdp' with the grid parameters of 'rdp'.
/// All resource usages are defined by 'rbt' any resource slot undefined will use a null resource.
/// All shaders that may be invoked directly or indirectly by this dispatch have to be part of 'pipeline'.
void dispatch(const ResourceBindingTable &rbt, const Pipeline &pipeline, const RayDispatchParameters &rdp,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
/// Indirect dispatch of the ray gen shader referenced by each entry in the indirect buffer, the indirect invocation will be executed
/// the number of times the 'count' member of 'rdip' declares.
/// All resource usages are defined by 'rbt' any resource slot undefined will use a null resource.
/// All shaders that may be invoked directly or indirectly by this dispatch have to be part of 'pipeline'.
void dispatch_indirect(const ResourceBindingTable &rbt, const Pipeline &pipeline, const RayDispatchIndirectParameters &rdip,
  GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
/// Indirect dispatch of the ray gen shader referenced by each entry in the indirect buffer, the indirect invocation will be executed
/// the number of times the value at the count buffer is at the time of the invocation on the execution device, however this value can
/// not exceed the value of 'maxCount' member of 'rdip' declares.
/// All resource usages are defined by 'rbt' any resource slot undefined will use a null resource.
/// All shaders that may be invoked directly or indirectly by this dispatch have to be part of 'pipeline'.
void dispatch_indirect_count(const ResourceBindingTable &rbt, const Pipeline &pipeline,
  const RayDispatchIndirectCountParameters &rdicp, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);
} // namespace raytrace
#endif
