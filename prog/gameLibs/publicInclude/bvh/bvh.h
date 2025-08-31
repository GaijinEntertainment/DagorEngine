//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_driver.h>
#include <3d/dag_resPtr.h>
#include <math/dag_boundingSphere.h>
#include <bvh/bvh_heightProvider.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_rendInstRes.h>
#include <math/dag_Point4.h>
#include <util/dag_threadPool.h>

class Sbuffer;
class LandMeshManager;
class RandomGrass;
class Cables;
class TMatrix;
struct RaytraceGeometryDescription;
struct RaytraceBottomAccelerationStructure;
struct RiGenVisibility;
struct BVHConnection;
class DynamicRenderableSceneResource;
enum class RaytraceBuildFlags : uint32_t;

namespace bvh
{
struct Context;
using ContextId = Context *;
} // namespace bvh

namespace dynrend
{
enum class ContextId;
using BVHSetInstanceData = void (*)(int instance_offset);
struct BVHCamoData
{
  float condition = 1, scale = 1, rotation = 0;
  TEXTUREID burntCamo;
};
using BVHIterateOneInstanceCallback = void (*)(const DynamicRenderableSceneInstance &inst, const DynamicRenderableSceneResource &res,
  const uint8_t *path_filter, uint32_t path_filter_size, uint8_t render_mask, dag::ConstSpan<int> offsets,
  BVHSetInstanceData set_instance_data, bool animate, BVHCamoData &camo_data, void *user_data);
using BVHIterateCallback = void (*)(BVHIterateOneInstanceCallback iter, const Point3 &view_position, void *user_data);
} // namespace dynrend

struct SbufferDeleter
{
  void operator()(Sbuffer *buf)
  {
    if (buf)
      destroy_d3dres(buf);
  }
};
using UniqueBVHBuffer = eastl::unique_ptr<Sbuffer, SbufferDeleter>;

template <typename ASType>
struct UniqueAS
{
  template <typename T = ASType>
  static eastl::enable_if_t<eastl::is_same_v<T, RaytraceBottomAccelerationStructure>, UniqueAS> create(
    RaytraceGeometryDescription *desc, uint32_t desc_count, RaytraceBuildFlags flags)
  {
    G_UNUSED(desc);
    G_UNUSED(desc_count);
    G_UNUSED(flags);

    UniqueAS as;
#if D3D_HAS_RAY_TRACING
    as.as = d3d::create_raytrace_bottom_acceleration_structure(desc, desc_count, flags, as.buildScratchSize, &as.updateScratchSize);
    if (as.as)
    {
      as.gpuAddress = d3d::get_raytrace_acceleration_structure_gpu_handle(as.as).handle;
      as.asSize = d3d::get_raytrace_acceleration_structure_size(as.as);
    }
#endif
    return as;
  }

  template <typename T = ASType>
  static eastl::enable_if_t<eastl::is_same_v<T, RaytraceBottomAccelerationStructure>, UniqueAS> create(uint32_t size)
  {
    G_UNUSED(size);

    UniqueAS as;
#if D3D_HAS_RAY_TRACING
    as.as = d3d::create_raytrace_bottom_acceleration_structure(size);
    if (as.as)
    {
      as.gpuAddress = d3d::get_raytrace_acceleration_structure_gpu_handle(as.as).handle;
      as.asSize = d3d::get_raytrace_acceleration_structure_size(as.as);
    }
#endif
    return as;
  }

  template <typename T = ASType>
  static eastl::enable_if_t<eastl::is_same_v<T, RaytraceTopAccelerationStructure>, UniqueAS> create(uint32_t instance_count,
    RaytraceBuildFlags flags, const char *name)
  {
    G_UNUSED(instance_count);
    G_UNUSED(flags);

    UniqueAS as;
#if D3D_HAS_RAY_TRACING
    as.as = d3d::create_raytrace_top_acceleration_structure(instance_count, flags, as.buildScratchSize, &as.updateScratchSize);
    if (as.as)
    {
      as.gpuAddress = d3d::get_raytrace_acceleration_structure_gpu_handle(as.as).handle;
      as.asSize = d3d::get_raytrace_acceleration_structure_size(as.as);
      as.scratchBuffer.reset(d3d::create_sbuffer(1, as.buildScratchSize, SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE, 0,
        String(0, "tlas_scratch_%s", name)));
    }
#endif
    return as;
  }

  UniqueAS() = default;

  UniqueAS(UniqueAS &&other) :
    as(other.as),
    scratchBuffer(eastl::move(other.scratchBuffer)),
    gpuAddress(other.gpuAddress),
    asSize(other.asSize),
    buildScratchSize(other.buildScratchSize),
    updateScratchSize(other.updateScratchSize)
  {
    other.as = nullptr;
    other.gpuAddress = 0;
    other.asSize = 0;
    other.buildScratchSize = 0;
    other.updateScratchSize = 0;
  }

  UniqueAS &operator=(UniqueAS &&other)
  {
    reset();
    as = other.as;
    scratchBuffer.swap(other.scratchBuffer);
    gpuAddress = other.gpuAddress;
    asSize = other.asSize;
    buildScratchSize = other.buildScratchSize;
    updateScratchSize = other.updateScratchSize;
    other.as = nullptr;
    other.gpuAddress = 0;
    other.asSize = 0;
    other.buildScratchSize = 0;
    other.updateScratchSize = 0;
    return *this;
  }

  ~UniqueAS() { reset(); }

  ASType *get() const { return as; }
  Sbuffer *getScratchBuffer() const { return scratchBuffer.get(); }
  uint64_t getGPUAddress() const { return gpuAddress; }
  uint32_t getASSize() const { return asSize; }
  // This code should be used, but a bug in the D3D12 validation layers always expect the larger build size.
  // So play along with that now.
  // uint32_t getBuildScratchSize() const { return buildScratchSize; }
  // uint32_t getUpdateScratchSize() const { return updateScratchSize; }
  uint32_t getBuildScratchSize() const { return max(buildScratchSize, updateScratchSize); }
  uint32_t getUpdateScratchSize() const { return max(buildScratchSize, updateScratchSize); }

  void reset()
  {
#if D3D_HAS_RAY_TRACING
    if (as)
    {
      if constexpr (eastl::is_same_v<ASType, RaytraceBottomAccelerationStructure>)
        d3d::delete_raytrace_bottom_acceleration_structure(as);
      else if constexpr (eastl::is_same_v<ASType, RaytraceTopAccelerationStructure>)
        d3d::delete_raytrace_top_acceleration_structure(as);
    }
#endif
    as = nullptr;
    scratchBuffer.reset();
    gpuAddress = 0;
    asSize = 0;
    buildScratchSize = 0;
    updateScratchSize = 0;
  }

  void swap(UniqueAS &other)
  {
    eastl::swap(as, other.as);
    scratchBuffer.swap(other.scratchBuffer);
    eastl::swap(asSize, other.asSize);
    eastl::swap(gpuAddress, other.gpuAddress);
    eastl::swap(buildScratchSize, other.buildScratchSize);
    eastl::swap(updateScratchSize, other.updateScratchSize);
  }

  operator bool() const { return as != nullptr; }
  bool operator!() const { return as == nullptr; }

  UniqueAS(const UniqueAS &other) = delete;
  UniqueAS &operator=(const UniqueAS &other) = delete;

private:
  ASType *as = nullptr;
  UniqueBVHBuffer scratchBuffer;
  uint64_t gpuAddress = 0;
  uint32_t asSize = 0;
  uint32_t buildScratchSize = 0;
  uint32_t updateScratchSize = 0;
};

using UniqueBLAS = UniqueAS<RaytraceBottomAccelerationStructure>;
using UniqueTLAS = UniqueAS<RaytraceTopAccelerationStructure>;

struct UniqueBVHBufferWithOffset
{
  UniqueBVHBuffer buffer;
  uint32_t offset = 0;

  operator bool() const { return buffer != nullptr; }
  bool operator!() const { return buffer == nullptr; }

  Sbuffer *operator->() const { return buffer.get(); }
  Sbuffer *get() const { return buffer.get(); }

  void close()
  {
    buffer.reset();
    offset = 0;
  }
};

struct BVHBufferReference
{
  Sbuffer *buffer = nullptr;
  uint32_t offset = 0;
  uint32_t allocator = 0;

  operator bool() const { return buffer != nullptr; }
  bool operator!() const { return buffer == nullptr; }

  Sbuffer *get() const { return buffer; }
  uint32_t size() const { return buffer ? buffer->getSize() : 0; }
};

struct UniqueOrReferencedBVHBuffer
{
  UniqueBVHBuffer *unique = nullptr;
  BVHBufferReference *referenced = nullptr;

  UniqueOrReferencedBVHBuffer() = default;
  UniqueOrReferencedBVHBuffer(UniqueBVHBuffer &unique) : unique(&unique) {}
  UniqueOrReferencedBVHBuffer(BVHBufferReference &referenced) : referenced(&referenced) {}

  bool operator!() const { return !unique && !referenced; }
  operator bool() const { return unique || referenced; }

  Sbuffer *get() const { return unique ? unique->get() : referenced ? referenced->get() : nullptr; }
  uint32_t getOffset() const { return referenced ? referenced->offset : 0; }

  bool needAllocation() const { return unique && !*unique || referenced && !*referenced; }
  bool isAllocated() const { return unique && *unique || referenced && referenced->buffer; }
};

namespace bvh
{
struct BufferProcessor;
struct TreeData;

struct MeshSkinningInfo
{
  TMatrix4 invWorldTm;
  eastl::function<void()> setTransformsFn;
  BVHBufferReference *skinningBuffer = nullptr;
  UniqueBLAS *skinningBlas = nullptr;
};

struct MeshHeliRotorInfo
{
  TMatrix4 invWorldTm;
  eastl::function<void(Point4 &, Point4 &)> getParamsFn;
  BVHBufferReference *transformedBuffer = nullptr;
  UniqueBLAS *transformedBlas = nullptr;
};

struct DeformedInfo
{
  Point2 simParams;
  TMatrix4 invWorldTm;
  eastl::function<void(float &, Point2 &)> getParamsFn;
  BVHBufferReference *transformedBuffer = nullptr;
  UniqueBLAS *transformedBlas = nullptr;
};

struct TreeData
{
  bool isPivoted;
  bool isPosInstance;
  float windBranchAmp;
  float windDetailAmp;
  float windSpeed;
  float windTime;
  Point4 windChannelStrength;
  Point4 windBlendParams;
  Texture *ppPosition;
  Texture *ppDirection;
  Point4 ppWindPerLevelAngleRotMax;
  float ppWindNoiseSpeedBase;
  float ppWindNoiseSpeedLevelMul;
  float ppWindAngleRotBase;
  float ppWindAngleRotLevelMul;
  float ppWindParentContrib;
  float ppWindMotionDampBase;
  float ppWindMotionDampLevelMul;
  float AnimWindScale;
  bool apply_tree_wind;
  E3DCOLOR color;
  uint32_t perInstanceRenderAdditionalData;
};

struct TreeInfo
{
  TMatrix4 invWorldTm;
  BVHBufferReference *transformedBuffer;
  UniqueBLAS *transformedBlas;
  bool recycled;

  TreeData data;
};

struct LeavesInfo
{
  TMatrix4 invWorldTm;
  BVHBufferReference *transformedBuffer;
  UniqueBLAS *transformedBlas;
  bool recycled;
};

struct FlagData
{
  uint32_t hashVal;
  int windType;
  union
  {
    struct
    {
      Point4 frequencyAmplitude;
      Point4 windDirection;
      float windStrength;
      float waveLength;
    } fixedWind;
    struct
    {
      Point4 flagpolePos0;
      Point4 flagpolePos1;
      float stiffness;
      float flagMovementScale;
      float bend;
      float deviation;
      float stretch;
      float flagLength;
      float swaySpeed;
      int widthType;
    } globalWind;
  };
};

struct FlagInfo
{
  TMatrix4 invWorldTm;
  BVHBufferReference *transformedBuffer;
  UniqueBLAS *transformedBlas;
  FlagData data;
};

struct MeshInfo
{
  static constexpr uint32_t invalidOffset = ~0U;

  Sbuffer *indices = 0;
  uint32_t indexCount = 0;
  uint32_t startIndex = 0;
  Sbuffer *vertices = 0;
  uint32_t vertexCount = 0;
  uint32_t vertexSize = 0;
  uint32_t baseVertex = 0;
  uint32_t startVertex = 0;
  uint32_t positionFormat = 0; // use VSDT_* vertex format types
  uint32_t positionOffset = invalidOffset;
  uint32_t normalOffset = invalidOffset;
  uint32_t colorOffset = invalidOffset;

  // Only needed for skinned meshes
  uint32_t indicesOffset = invalidOffset; // only needed for skinned meshes
  uint32_t weightsOffset = invalidOffset; // only needed for skinned meshes
  // ~Only needed for skinned meshes

  // Only needed for meshes with textures
  uint32_t texcoordFormat = 0; // use VSDT_* vertex format types
  uint32_t texcoordOffset = invalidOffset;
  uint32_t secTexcoordOffset = invalidOffset;
  TEXTUREID albedoTextureId = BAD_TEXTUREID;
  TEXTUREID alphaTextureId = BAD_TEXTUREID;
  TEXTUREID normalTextureId = BAD_TEXTUREID;
  TEXTUREID extraTextureId = BAD_TEXTUREID;
  bool alphaTest = false;
  // ~Only needed for meshes with textures

  const BufferProcessor *vertexProcessor = nullptr;

  Point4 posMul;
  Point4 posAdd;

  BSphere3 boundingSphere;

  float impostorHeightOffset = 0;
  Point4 impostorScale;
  Point4 impostorSliceTm1;
  Point4 impostorSliceTm2;
  Point4 impostorSliceClippingLines1;
  Point4 impostorSliceClippingLines2;
  Point4 impostorOffsets[4];

  bool isImpostor = false;
  bool isInterior = false;
  bool isClipmap = false;
  bool hasInstanceColor = false;
  bool isCamo = false;
  bool isMFD = false;
  bool isHeliRotor = false;
  bool isEmissive = false;

  bool painted = false;
  Point4 paintData;
  Point4 colorOverride = Point4(0.5, 0.5, 0.5, 0);
  uint32_t detailsData1 = 0;
  uint32_t detailsData2 = 0;
  uint32_t detailsData3 = 0;
  uint32_t detailsData4 = 0;


  bool useAtlas = false;
  float atlasTileU = 1.0;
  float atlasTileV = 1.0;
  uint32_t atlasFirstTile = 0;
  uint32_t atlasLastTile = 0;

  bool isLayered = false;
  bool isPerlinLayered = false;
  bool isEye = false;
  float maskGammaStart = 0.5;
  float maskGammaEnd = 2;
  float maskTileU = 1;
  float maskTileV = 1;
  float detail1TileU = 1;
  float detail1TileV = 1;
  float detail2TileU = 1;
  float detail2TileV = 1;

  float texcoordScale = 1.f;
};

struct ObjectInfo
{
  dag::Vector<MeshInfo> meshes;
  bool isAnimated = false;
};

struct Context;
using ContextId = Context *;
static inline constexpr ContextId InvalidContextId = nullptr;

static constexpr uint32_t bvhGroupTerrain = 1 << 0;
static constexpr uint32_t bvhGroupRiGen = 1 << 1;
static constexpr uint32_t bvhGroupRiExtra = 1 << 2;
static constexpr uint32_t bvhGroupDynrend = 1 << 3;
static constexpr uint32_t bvhGroupGrass = 1 << 4;
static constexpr uint32_t bvhGroupImpostor = 1 << 5;
static constexpr uint32_t bvhGroupNoShadow = 1 << 6;

enum Features
{
  Terrain = 1 << 0,            // Terrain is enabled.
  RIFull = 1 << 1,             // RI is enabled with the original mesh.
  RIBaked = 1 << 2,            // RI is enabled with the diffues texture baked into the vertices. Only last LOD is used.
  DynrendRigidFull = 1 << 3,   // Dynrend rigid parts are enabled with the original mesh.
  DynrendRigidBaked = 1 << 4,  // Dynrend rigid parts are enabled with the diffues texture baked into the vertices. Only last LOD is
                               // used.
  DynrendSkinnedFull = 1 << 5, // Dynrend skinned parts are enabled with the original mesh.
  GpuObjects = 1 << 6,         // GpuObjects are enabled. Only works if and of the RI is enabled.
  Grass = 1 << 7,              // Grass is enabled.
  Fx = 1 << 8,                 // Particle effects are enabled.
  Cable = 1 << 9,              // Cable rendering is enabled

  ForRendering = Terrain | RIFull | DynrendRigidFull | DynrendSkinnedFull | GpuObjects | Grass | Fx | Cable,
  ForGI = Terrain | RIBaked | GpuObjects,
};


struct ChannelParser : public ShaderChannelsEnumCB
{
  int flags = 0;
  uint32_t positionFormat = -1;
  uint32_t positionOffset = 0;
  uint32_t texcoordFormat = -1;
  uint32_t texcoordOffset = 0;
  uint32_t secTexcoordFormat = -1;
  uint32_t secTexcoordOffset = 0;
  uint32_t thirdTexcoordFormat = -1;
  uint32_t thirdTexcoordOffset = 0;
  uint32_t normalFormat = -1;
  uint32_t normalOffset = 0;
  uint32_t colorFormat = -1;
  uint32_t colorOffset = 0;
  uint32_t indicesFormat = -1;
  uint32_t indicesOffset = 0;
  uint32_t weightsFormat = -1;
  uint32_t weightsOffset = 0;

  void enum_shader_channel(int usage, int usage_index, int type, int vb_usage, int vb_usage_index, ChannelModifier mod,
    int stream) override;
};

bool has_enough_vram_for_rt();

void set_enable(bool enable);

bool is_available();

using elem_rules_fn = void (*)(const ShaderMesh::RElem &, MeshInfo &, const ChannelParser &,
  const RenderableInstanceLodsResource::ImpostorParams *, const RenderableInstanceLodsResource::ImpostorTextures *);

void init(elem_rules_fn elem_rules = nullptr);

void teardown();

ContextId create_context(const char *name, Features features = Features::ForRendering);

void teardown(ContextId &context_id);

void start_frame();

void add_terrain(ContextId context_id, HeightProvider *height_provider);

void remove_terrain(ContextId context_id);

void update_terrain(ContextId context_id, const Point2 &location);

void set_for_gpu_objects(ContextId context_id);

void prepare_ri_extra_instances();

void set_ri_dist_mul(float mul);

void override_out_of_camera_ri_dist_mul(float dist_sq_mul_ooc);

void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Frustum &bvh_frustum, const Frustum &view_frustum,
  dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id, RiGenVisibility *ri_gen_visibility,
  threadpool::JobPriority prio);
void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Frustum &bvh_frustum, const Frustum &view_frustum,
  const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate,
  threadpool::JobPriority prio);

// The upper 32 bits of the object_id are reserved for RI
void add_object(ContextId context_id, uint64_t object_id, const ObjectInfo &info);

void add_instance(ContextId context_id, uint64_t object_id, mat43f_cref transform);

void remove_object(ContextId context_id, uint64_t object_id);

enum class BuildBudget
{
  Low,
  Medium,
  High,
};

void build(ContextId context_id, const TMatrix &itm, const TMatrix4 &projTm, const Point3 &camera_pos, const Point3 &light_direction);

void process_meshes(ContextId context_id, BuildBudget budget = BuildBudget::High);

void bind_resources(ContextId context_id, int render_width);

// ps5 specific functions. Currently ps5 doesn't support tlas shader vars
void bind_tlas_stage(ContextId context_id, ShaderStage stage);
void unbind_tlas_stage(ShaderStage stage);

void on_load_scene(ContextId context_id);

void on_scene_loaded(ContextId context_id);

void on_before_unload_scene(ContextId context_id);

void on_before_settings_changed(ContextId context_id);

void on_unload_scene(ContextId context_id);

void reload_grass(ContextId context_id, RandomGrass *grass);

using fx_connect_callback = void (*)(BVHConnection *);
void connect_fx(ContextId context_id, fx_connect_callback callback);

void on_cables_changed(Cables *cables, ContextId context_id);

using atmosphere_callback = void (*)(const Point3 &view_pos, const Point3 &view_dir, float d, Color3 &insc, Color3 &loss);
void start_async_atmosphere_update(ContextId context_id, const Point3 &view_pos, atmosphere_callback callback);

void finalize_async_atmosphere_update(ContextId context_id);

bool is_building(ContextId context_id);

void set_grass_range(ContextId context_id, float range);

void set_debug_view_min_t(float min_t);

void enable_per_frame_processing(bool enable);
} // namespace bvh