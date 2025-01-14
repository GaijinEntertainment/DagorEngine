// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <3d/dag_lockSbuffer.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <gui/dag_stdGuiRender.h>
#include <math/dag_math3d.h>
#include <math/dag_dxmath.h>
#include <math/dag_half.h>
#include <util/dag_threadPool.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <gui/dag_visualLog.h>
#include <drv/3d/dag_commands.h>

#include "bvh_context.h"
#include "bvh_debug.h"
#include "bvh_add_instance.h"

static constexpr int per_frame_blas_model_budget[3] = {10, 15, 30};

static constexpr float animation_distance_rate = 20;

static constexpr bool showProceduralBLASBuildCount = false;

static bool per_frame_processing_enabled = true;

#if DAGOR_DBGLEVEL > 0
extern bool bvh_gpuobject_enable;
extern bool bvh_grass_enable;
extern bool bvh_particles_enable;
extern bool bvh_cables_enable;
#endif

namespace bvh
{

namespace terrain
{
dag::Vector<eastl::tuple<uint64_t, uint32_t, Point2>> get_blases(ContextId context_id);

void init();
void init(ContextId context_id);
void teardown();
void teardown(ContextId context_id);
} // namespace terrain

namespace ri
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void prepare_ri_extra_instances();
void update_ri_gen_instances(ContextId context_id, RiGenVisibility *ri_gen_visibility, const Point3 &view_position);
void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &frustum);
void wait_ri_gen_instances_update(ContextId context_id);
void wait_ri_extra_instances_update(ContextId context_id);
void cut_down_trees(ContextId context_id);
void wait_cut_down_trees();
} // namespace ri

namespace dyn
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_plane_context_id, const Point3 &view_position);
void set_up_dynrend_context_for_processing(dynrend::ContextId dynrend_context_id);
} // namespace dyn

namespace gobj
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
} // namespace gobj

namespace grass
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void reload_grass(ContextId context_id, RandomGrass *grass);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
void blas_compacted(int layer_ix, int lod_ix, uint64_t new_gpu_address);
UniqueBLAS *get_blas(int layer_ix, int lod_ix);
} // namespace grass

namespace fx
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void connect(fx_connect_callback callback);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
void on_unload_scene(ContextId context_id);
} // namespace fx

namespace cables
{
void on_cables_changed(Cables *cables, ContextId context_id);
const dag::Vector<UniqueBLAS> &get_blases(ContextId context_id, int &meta_alloc_id);

void init();
void init(ContextId context_id);
void teardown();
void teardown(ContextId context_id);
} // namespace cables

namespace debug
{
void init(ContextId context_id);
void teardown(ContextId context_id);
void teardown();
void render_debug_context(ContextId context_id);
} // namespace debug

bool is_in_lost_device_state = false;

elem_rules_fn elem_rules = nullptr;

std::atomic_uint32_t bvh_id_gen = 0;

float mip_range = 1000;
float mip_scale = 10;

float max_water_distance = 10.0f;
float water_fade_power = 3.0f;

UniqueTLAS tlasEmpty;

struct BVHLinearBufferManager
{
  struct Buffer
  {
    Buffer(size_t alignment, size_t struct_size, size_t elem_count, uint32_t flags, const char *name) : alignment(alignment)
    {
      buffer = dag::create_sbuffer(struct_size, elem_count, flags, 0, name);
    }

    void startNewFrame() { cursor = 0; }

    bool hasSpace(uint32_t elems) { return cursor + elems <= buffer->getNumElements(); }

    Sbuffer *alloc(uint32_t elems, uint32_t &offset)
    {
      G_ASSERT(hasSpace(elems));
      offset = cursor;
      cursor += elems;
      if (alignment > 1)
        cursor = (cursor + alignment - 1) & ~(alignment - 1);

      return buffer.getBuf();
    }

    size_t alignment;

    UniqueBuf buffer;
    uint32_t cursor = 0;
  };

  BVHLinearBufferManager(size_t struct_size, size_t elem_count, uint32_t flags, const char *name) :
    alignment(0), structSize(struct_size), elemCount(elem_count), flags(flags), name(name)
  {}

  void startNewFrame()
  {
    for (auto buffer : largeBuffers)
      del_d3dres(buffer);
    largeBuffers.clear();
    for (auto &buffer : buffers)
      buffer.startNewFrame();
  }

  Sbuffer *alloc(uint32_t elems, uint32_t &offset)
  {
    if (elems > elemCount)
    {
      offset = 0;
      auto buffer = d3d::create_sbuffer(structSize, elems, flags, 0, String(0, "%s_large_%u", name, largeBuffers.size()));
      HANDLE_LOST_DEVICE_STATE(buffer, nullptr);
      largeBuffers.push_back(buffer);
      return buffer;
    }

    for (auto &buffer : buffers)
      if (buffer.hasSpace(elems))
        return buffer.alloc(elems, offset);

    buffers.emplace_back(alignment, structSize, elemCount, flags, String(0, "%s_%u", name, buffers.size()).data());
    HANDLE_LOST_DEVICE_STATE(buffers.back().buffer, nullptr);
    return buffers.back().alloc(elems, offset);
  }

  uint32_t getMemoryStatistics() const { return buffers.size() * elemCount * structSize; }

  void teardown()
  {
    for (auto buffer : largeBuffers)
      del_d3dres(buffer);
    largeBuffers.clear();
    buffers.clear();
  }

  size_t alignment;
  size_t structSize;
  size_t elemCount;
  uint32_t flags;
  const char *name;

  dag::Vector<Buffer> buffers;
  dag::Vector<Sbuffer *> largeBuffers;
};

static BVHLinearBufferManager scratch_buffer_manager(1, 2 * 1024 * 1024, SBCF_USAGE_ACCELLERATION_STRUCTURE_BUILD_SCRATCH_SPACE,
  "bvh_scratch_buffer");
static BVHLinearBufferManager transform_buffer_manager(sizeof(float) * 3 * 4, 200,
  SBCF_BIND_SHADER_RES | SBCF_CPU_ACCESS_WRITE | SBCF_MISC_STRUCTURED, "bvh_transform_buffer");

uint32_t get_scratch_buffers_memory_statistics() { return scratch_buffer_manager.getMemoryStatistics(); }

uint32_t get_transform_buffers_memory_statistics() { return transform_buffer_manager.getMemoryStatistics(); }

Sbuffer *alloc_scratch_buffer(uint32_t size, uint32_t &offset)
{
  uint32_t alignment = d3d::get_driver_desc().raytrace.accelerationStructureBuildScratchBufferOffsetAlignment;
  uint32_t alignedSize = ((size + alignment - 1) / alignment) * alignment;
  return scratch_buffer_manager.alloc(alignedSize, offset);
}

static eastl::unique_ptr<ComputeShaderElement> hwInstanceCopyShader;

ComputeShaderElement &get_hw_instance_copy_shader()
{
  if (!hwInstanceCopyShader)
    hwInstanceCopyShader.reset(new_compute_shader("bvh_hwinstance_copy"));

  G_ASSERT(hwInstanceCopyShader);
  return *hwInstanceCopyShader;
}

static __forceinline void realign(mat43f &mat, const Point3 &pos)
{
  mat.row0 = v_sub(mat.row0, v_make_vec4f(0, 0, 0, pos.x));
  mat.row1 = v_sub(mat.row1, v_make_vec4f(0, 0, 0, pos.y));
  mat.row2 = v_sub(mat.row2, v_make_vec4f(0, 0, 0, pos.z));
}

static __forceinline void realign(mat43f &mat, vec4f s1, vec4f s2, vec4f s3)
{
  mat.row0 = v_sub(mat.row0, s1);
  mat.row1 = v_sub(mat.row1, s2);
  mat.row2 = v_sub(mat.row2, s3);
}

bool has_enough_vram_for_rt()
{
  return (d3d::driver_command(Drv3dCommand::GET_VIDEO_MEMORY_BUDGET) >=
          dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhMinRequiredMemoryGB", 5) * 1100 * 1000); // intentionally not
                                                                                                                // 1024
}

static bool has_enough_vram_for_rt_initial_check()
{
  bool ret = has_enough_vram_for_rt();
  logdbg("bvh: VRAM check %s", ret ? "OK" : "FAILED");
  return ret;
}

bool is_available()
{
  if (dgs_get_settings()->getBlockByNameEx("gameplay")->getBool("enableVR", false))
    return false;
  if (!d3d::get_driver_desc().caps.hasRayQuery)
    return false;
  if (!d3d::get_driver_desc().caps.hasBindless)
    return false;
#if _TARGET_SCARLETT || _TARGET_C2
  static bool hasEnoughVRAM = true;
#elif _TARGET_PC
  if (!d3d::is_inited())
    return true;
  static bool hasEnoughVRAM = has_enough_vram_for_rt_initial_check();
#else
  static bool hasEnoughVRAM = false;
#endif
  return hasEnoughVRAM;
}

void init(elem_rules_fn elem_rules_init)
{
  elem_rules = elem_rules_init;

  scratch_buffer_manager.alignment = d3d::get_driver_desc().raytrace.accelerationStructureBuildScratchBufferOffsetAlignment;

  UniqueBVHBuffer emptyInstances(
    d3d::buffers::create_persistent_sr_structured(sizeof(HWInstance), 1, "empty_tlas_instances", d3d::buffers::Init::Zero));

  tlasEmpty = UniqueTLAS::create(0, RaytraceBuildFlags::NONE, "empty");

  raytrace::TopAccelerationStructureBuildInfo buildInfo = {};
  buildInfo.instanceBuffer = emptyInstances.get();
  buildInfo.scratchSpaceBufferSizeInBytes = tlasEmpty.getBuildScratchSize();
  buildInfo.scratchSpaceBuffer =
    alloc_scratch_buffer(buildInfo.scratchSpaceBufferSizeInBytes, buildInfo.scratchSpaceBufferOffsetInBytes);
  buildInfo.flags = RaytraceBuildFlags::FAST_TRACE;
  d3d::build_top_acceleration_structure(tlasEmpty.get(), buildInfo);

  terrain::init();
  ri::init();
  dyn::init();
  gobj::init();
  grass::init();
  fx::init();
  cables::init();
}

void teardown()
{
  tlasEmpty.reset();

  terrain::teardown();
  ri::teardown();
  dyn::teardown();
  gobj::teardown();
  grass::teardown();
  fx::teardown();
  cables::teardown();
  debug::teardown();
  hwInstanceCopyShader.reset();
  ProcessorInstances::teardown();
  scratch_buffer_manager.teardown();
  transform_buffer_manager.teardown();

  is_in_lost_device_state = false;
}

ContextId create_context(const char *name, Features features)
{
  G_ASSERT(name && *name);

  auto context_id = new Context();
  context_id->name = name;
  context_id->features = features;
  terrain::init(context_id);
  ri::init(context_id);
  dyn::init(context_id);
  grass::init(context_id);
  fx::init(context_id);
  if (context_id->has(Features::Cable))
    cables::init(context_id);
  debug::init(context_id);
  return context_id;
}

void teardown(ContextId &context_id)
{
  if (context_id == InvalidContextId)
    return;

  terrain::teardown(context_id);
  ri::teardown(context_id);
  dyn::teardown(context_id);
  gobj::teardown(context_id);
  grass::teardown(context_id);
  fx::teardown(context_id);
  if (context_id->has(Features::Cable))
    cables::teardown(context_id);
  debug::teardown(context_id);

  for (auto &mesh : context_id->meshes)
    mesh.second.teardown(context_id);
  for (auto &mesh : context_id->impostors)
    mesh.second.teardown(context_id);

  // TODO:: something else references "combined_paint_tex" (DNG has the same issue) so it is
  // not evicted
  G_ASSERT(context_id->usedTextures.empty());
  G_ASSERT(context_id->usedBuffers.empty());

  delete context_id;

  context_id = InvalidContextId;

  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
  ShaderGlobal::set_tlas(bvh_mainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, nullptr);
}

void start_frame()
{
  scratch_buffer_manager.startNewFrame();
  transform_buffer_manager.startNewFrame();
}

void add_instance(ContextId context_id, uint64_t mesh_id, mat43f_cref transform)
{
  add_instance(context_id, context_id->genericInstances, mesh_id, transform, nullptr, false, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, -1);
}

void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Frustum &frustum,
  dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_plane_context_id, RiGenVisibility *ri_gen_visibility)
{
  bvh_context_id->genericInstances.clear();
  for (auto &instances : bvh_context_id->dynrendInstances)
    instances.second.clear();
  for (auto &instances : bvh_context_id->riGenInstances)
    instances.clear();
  for (auto &instances : bvh_context_id->riExtraInstances)
    instances.clear();
  for (auto &data : bvh_context_id->riExtraInstanceData)
    data.clear();
  for (auto &instances : bvh_context_id->riExtraTreeInstances)
    instances.clear();
  for (auto &instances : bvh_context_id->impostorInstances)
    instances.clear();
  for (auto &data : bvh_context_id->impostorInstanceData)
    data.clear();

  if (bvh_context_id->has(Features::RIFull | Features::RIBaked))
  {
    ri::wait_cut_down_trees();
    ri::update_ri_gen_instances(bvh_context_id, ri_gen_visibility, view_position);
    ri::update_ri_extra_instances(bvh_context_id, view_position, frustum);
  }
  if (bvh_context_id->has(Features::DynrendRigidFull | Features::DynrendRigidBaked | Features::DynrendSkinnedFull))
    dyn::update_dynrend_instances(bvh_context_id, *dynrend_context_id, *dynrend_plane_context_id, view_position);
}

static __forceinline bool need_winding_flip(const Mesh &mesh, const Context::Instance &instance)
{
  return need_winding_flip(mesh, instance.transform);
}

static BufferProcessor::ProcessArgs build_args(uint64_t mesh_id, const Mesh &mesh, const Context::Instance *instance, bool recycled)
{
  BufferProcessor::ProcessArgs args{};
  args.meshId = mesh_id;
  args.indexStart = mesh.startIndex;
  args.indexCount = mesh.indexCount;
  args.indexFormat = mesh.indexFormat;
  args.baseVertex = mesh.baseVertex;
  args.startVertex = mesh.startVertex;
  args.vertexCount = mesh.vertexCount;
  args.vertexStride = mesh.vertexStride;
  args.positionFormat = mesh.positionFormat;
  args.positionOffset = mesh.positionOffset;
  args.texcoordOffset = mesh.texcoordOffset;
  args.texcoordFormat = mesh.texcoordFormat;
  args.secTexcoordOffset = mesh.secTexcoordOffset;
  args.normalOffset = mesh.normalOffset;
  args.colorOffset = mesh.colorOffset;
  args.indicesOffset = mesh.indicesOffset;
  args.weightsOffset = mesh.weightsOffset;
  args.posMul = mesh.posMul;
  args.posAdd = mesh.posAdd;
  args.texture = mesh.albedoTextureId;
  args.textureLevel = mesh.albedoTextureLevel;
  args.impostorHeightOffset = mesh.impostorHeightOffset;
  args.impostorScale = mesh.impostorScale;
  args.impostorSliceTm1 = mesh.impostorSliceTm1;
  args.impostorSliceTm2 = mesh.impostorSliceTm2;
  args.impostorSliceClippingLines1 = mesh.impostorSliceClippingLines1;
  args.impostorSliceClippingLines2 = mesh.impostorSliceClippingLines2;
  args.recycled = recycled;
  memcpy(args.impostorOffsets, mesh.impostorOffsets, sizeof(args.impostorOffsets));
  if (instance)
  {
    args.worldTm = instance->transform;
    args.invWorldTm = instance->invWorldTm;
    args.setTransformsFn = instance->setTransformsFn;
    args.getHeliParamsFn = instance->getHeliParamsFn;
    args.getDeformParamsFn = instance->getDeformParamsFn;
    args.tree = instance->tree;
    args.flag = instance->flag;
  }
  return args;
}

static bool process(ContextId context_id, Sbuffer *buffer, UniqueOrReferencedBVHBuffer &processedBuffer,
  const BufferProcessor *processor, BufferProcessor::ProcessArgs &args, bool skipProcessing, bool &need_blas_build)
{
  bool didProcessing = false;
  if (processedBuffer.needAllocation() || !processor->isOneTimeOnly())
  {
    need_blas_build |= processor->process(context_id, buffer, processedBuffer, args, skipProcessing);
    didProcessing = true;
  }

  HANDLE_LOST_DEVICE_STATE(processedBuffer.isAllocated(), false);

  return didProcessing;
}

static void process_vertices(ContextId context_id, uint64_t mesh_id, Mesh &mesh, const Context::Instance *instance,
  const Frustum *frustum, vec4f_const view_pos, vec4f_const light_direction, UniqueOrReferencedBVHBuffer &transformed_vertices,
  bool processed_vertices_recycled, bool &need_blas_build, MeshMeta &meta, MeshMeta &base_meta, bool batched)
{
  if (mesh.vertexProcessor)
  {
    auto needProcessing = transformed_vertices.needAllocation();
    auto hasVertexAnimation = !mesh.vertexProcessor->isOneTimeOnly();
    if (hasVertexAnimation)
    {
      G_ASSERT(instance);
      G_ASSERT(frustum);

      if (instance && frustum)
      {
        mat44f tm44;
        v_mat43_transpose_to_mat44(tm44, instance->transform);

        vec4f localBounds = v_ldu(&mesh.boundingSphere.c.x);
        vec4f worldBounds = v_mat44_mul_bsph(tm44, localBounds);
        bbox3f worldBoundsBox;
        worldBoundsBox.bmin = v_sub(worldBounds, v_splat_w(worldBounds));
        worldBoundsBox.bmax = v_add(worldBounds, v_splat_w(worldBounds));
        vec3f far_point = v_mul(v_add(v_add(worldBoundsBox.bmax, worldBoundsBox.bmin), light_direction), V_C_HALF);
        worldBoundsBox.bmin = v_min(worldBoundsBox.bmin, far_point);
        worldBoundsBox.bmax = v_max(worldBoundsBox.bmax, far_point);
        auto isVisible = !!frustum->testBoxB(worldBoundsBox.bmin, worldBoundsBox.bmax);
        if (isVisible)
        {
          auto distance = v_length3(v_sub(view_pos, worldBounds));
          auto rate = v_extract_x(v_mul(v_div(distance, v_splat_w(worldBounds)), V_C_HALF));

          if (rate < animation_distance_rate)
          {
            needProcessing = true;
            context_id->animatedInstanceCount++;
          }
        }
      }
    }

    auto args = build_args(mesh_id, mesh, instance, processed_vertices_recycled);

    bool canProcess = mesh.vertexProcessor->isReady(args);

    // Only do the processing if we either has a per instance output to process into, or the
    // initial processing on the mesh is not yet done. Otherwise it would just process the same
    // mesh again and again for no reason.
    if (canProcess && (transformed_vertices.needAllocation() || (instance && instance->uniqueTransformBuffer)))
    {
      bool hadProcessedVertices = transformed_vertices.isAllocated();

      if (hadProcessedVertices && batched)
        d3d::resource_barrier(ResourceBarrierDesc(transformed_vertices.get(), RB_NONE));

      if (process(context_id, mesh.processedVertices.get(), transformed_vertices, mesh.vertexProcessor, args, !needProcessing,
            need_blas_build))
      {
        if (!batched)
          d3d::resource_barrier(ResourceBarrierDesc(transformed_vertices.get(), bindlessSRVBarrier));

        // Mesh should only change if this is not an animating mesh
        if (!instance)
          mesh.positionOffset = args.positionOffset;
        mesh.processedPositionFormat = args.positionFormat;

        meta.setTexcoordFormat(args.texcoordFormat);
        meta.startVertex = args.startVertex;
        meta.texcoordNormalColorOffsetAndVertexStride = (args.texcoordOffset & 255) << 24 | (args.normalOffset & 255) << 16 |
                                                        (args.colorOffset & 255) << 8 | args.vertexStride & 255;

        base_meta.setTexcoordFormat(args.texcoordFormat);
        base_meta.startVertex = meta.startVertex;
        base_meta.texcoordNormalColorOffsetAndVertexStride = meta.texcoordNormalColorOffsetAndVertexStride;
      }

      if (!hadProcessedVertices && transformed_vertices.isAllocated())
      {
        uint32_t bindlessIndex;
        context_id->holdBuffer(transformed_vertices.get(), bindlessIndex);
        if (instance && instance->uniqueTransformBuffer)
        {
          meta.indexAndVertexBufferIndex &= 0xFFFF0000U;
          meta.indexAndVertexBufferIndex |= bindlessIndex;
        }
        else
          mesh.pvBindlessIndex = bindlessIndex;
      }

      if (!instance || !instance->uniqueTransformBuffer)
      {
        meta.indexAndVertexBufferIndex &= 0xFFFF0000U;
        meta.indexAndVertexBufferIndex |= mesh.pvBindlessIndex;
      }
    }
  }
}


static void do_add_mesh(ContextId context_id, uint64_t mesh_id, const MeshInfo &info)
{
  auto &meshMap = info.isImpostor ? context_id->impostors : context_id->meshes;

  // If a mesh has a BLAS, it is either non animated or it has it's vertex/index buffers already
  // copied/processed into its own buffer. So we don't care about any of the changes.
  auto iter = meshMap.find(mesh_id);
  if (iter != meshMap.end())
  {
    if (iter->second.blas)
      return;

    iter->second.teardown(context_id);
    context_id->removeFromCompaction(mesh_id);
    context_id->halfBakedMeshes.erase(mesh_id);
  }

  auto &mesh = (iter != meshMap.end()) ? iter->second : meshMap[mesh_id];
  mesh.albedoTextureId = info.albedoTextureId;
  mesh.alphaTextureId = info.alphaTextureId;
  mesh.normalTextureId = info.normalTextureId;
  mesh.extraTextureId = info.extraTextureId;
  mesh.indexCount = info.indexCount;
  mesh.indexFormat = info.indices ? (info.indices->getFlags() & SBCF_INDEX32 ? 4 : 2) : 0;
  mesh.vertexCount = info.vertexCount;
  mesh.vertexStride = info.vertexSize;
  mesh.positionFormat = info.positionFormat;
  mesh.positionOffset = info.positionOffset;
  mesh.processedPositionFormat = info.positionFormat;
  mesh.texcoordOffset = info.texcoordOffset;
  mesh.texcoordFormat = info.texcoordFormat;
  mesh.secTexcoordOffset = info.secTexcoordOffset;
  mesh.normalOffset = info.normalOffset;
  mesh.colorOffset = info.colorOffset;
  mesh.indicesOffset = info.indicesOffset;
  mesh.weightsOffset = info.weightsOffset;
  mesh.vertexProcessor = info.vertexProcessor;
  mesh.startIndex = info.startIndex;
  mesh.baseVertex = info.baseVertex;
  mesh.startVertex = info.startVertex;
  mesh.posMul = info.posMul;
  mesh.posAdd = info.posAdd;
  mesh.enableCulling = info.enableCulling;
  mesh.boundingSphere = info.boundingSphere;
  mesh.impostorHeightOffset = info.impostorHeightOffset;
  mesh.impostorScale = info.impostorScale;
  mesh.impostorSliceTm1 = info.impostorSliceTm1;
  mesh.impostorSliceTm2 = info.impostorSliceTm2;
  mesh.impostorSliceClippingLines1 = info.impostorSliceClippingLines1;
  mesh.impostorSliceClippingLines2 = info.impostorSliceClippingLines2;
  mesh.isHeliRotor = info.isHeliRotor;
  memcpy(mesh.impostorOffsets, info.impostorOffsets, sizeof(mesh.impostorOffsets));
  if (info.isInterior)
    mesh.materialType = MeshMeta::bvhMaterialInterior;
  else if (info.isClipmap)
    mesh.materialType = MeshMeta::bvhMaterialTerrain;
  else
    mesh.materialType = MeshMeta::bvhMaterialRendinst;

  if (info.hasInstanceColor)
    mesh.materialType |= MeshMeta::bvhInstanceColor;

  if (info.isImpostor)
    mesh.materialType |= MeshMeta::bvhMaterialImpostor;

  if (info.alphaTest)
    mesh.materialType |= MeshMeta::bvhMaterialAlphaTest;

  if (info.painted)
    mesh.materialType |= MeshMeta::bvhMaterialPainted;

  if (info.useAtlas)
    mesh.materialType |= MeshMeta::bvhMaterialAtlas;

  if (info.isCamo)
    mesh.materialType |= MeshMeta::bvhMaterialCamo;

  if (info.isLayered)
    mesh.materialType |= MeshMeta::bvhMaterialLayered;

  if (info.isEmissive)
    mesh.materialType |= MeshMeta::bvhMaterialEmissive;

  auto &meta = mesh.createAndGetMeta(context_id);

  meta.markInitialized();

  meta.materialType = mesh.materialType;
  meta.setIndexBitAndTexcoordFormat(mesh.indexFormat, info.texcoordFormat);
  meta.texcoordNormalColorOffsetAndVertexStride =
    (info.texcoordOffset & 255) << 24 | (info.normalOffset & 255) << 16 | (info.colorOffset & 255) << 8 | info.vertexSize & 255;
  meta.startIndex = info.startIndex;
  meta.startVertex = info.baseVertex;
  meta.texcoordScale = info.texcoordScale;
  meta.indexAndVertexBufferIndex = 0;

  context_id->holdTexture(mesh.alphaTextureId, meta.alphaTextureAndSamplerIndex);
  context_id->holdTexture(mesh.normalTextureId, meta.normalTextureAndSamplerIndex);
  context_id->holdTexture(info.extraTextureId, meta.extraTextureAndSamplerIndex);
  auto albedoTexture = context_id->holdTexture(mesh.albedoTextureId, meta.albedoTextureAndSamplerIndex);

  if (info.albedoTextureId != BAD_TEXTUREID)
  {
    TextureInfo ti;
    albedoTexture->getinfo(ti);

    mesh.albedoTextureLevel = D3dResManagerData::getLevDesc(info.albedoTextureId.index(), TQL_thumb);
    mark_managed_tex_lfu(info.albedoTextureId, mesh.albedoTextureLevel);
  }

  if (info.painted)
  {
    meta.materialData1 = info.paintData;
    meta.materialData2 = info.colorOverride;
  }

  if (info.useAtlas)
  {
    meta.atlasTileSize = uint32_t(float_to_half(info.atlasTileU)) | uint32_t(float_to_half(info.atlasTileV)) << 16;
    meta.atlasFirstLastTile = info.atlasFirstTile | (info.atlasLastTile - info.atlasFirstTile + 1) << 16;
  }

  if (info.isCamo)
    meta.atlasTileSize = info.secTexcoordOffset;

  if (info.isLayered)
  {
    meta.layerData[0] = uint32_t(float_to_half(info.maskGammaStart)) | uint32_t(float_to_half(info.maskGammaEnd)) << 16;
    meta.layerData[1] = uint32_t(float_to_half(info.maskTileU)) | uint32_t(float_to_half(info.maskTileV)) << 16;
    meta.layerData[2] = uint32_t(float_to_half(info.detail1TileU)) | uint32_t(float_to_half(info.detail1TileV)) << 16;
    meta.layerData[3] = uint32_t(float_to_half(info.detail2TileU)) | uint32_t(float_to_half(info.detail2TileV)) << 16;

    // Mask texture is extraTextureAndSamplerIndex
    // tile1diffuse is alphaTextureAndSamplerIndex
    // tile2diffuse is normalTextureAndSamplerIndex

    meta.texcoordScale = info.secTexcoordOffset;
  }

  if (info.indices)
  {
    // Always process indices to be independent from the streaming system.

    G_ASSERT(mesh.indexFormat == 2);

    static int counter = 0;
    String name(32, "bvh_indices_%u", ++counter);
    auto dwordCount = (info.indexCount + 1) / 2;
    mesh.processedIndices.buffer.reset(d3d::buffers::create_persistent_sr_byte_address(dwordCount, name.data()));
    mesh.processedIndices.offset = 0;

    HANDLE_LOST_DEVICE_STATE(mesh.processedIndices.buffer, );

    ProcessorInstances::getIndexProcessor().process(info.indices, mesh.processedIndices, mesh.indexFormat, mesh.indexCount,
      mesh.startIndex, mesh.startVertex);
    G_ASSERT(mesh.processedIndices);

    mesh.startIndex = meta.startIndex = 0;
    meta.setIndexBit(mesh.indexFormat);

    uint32_t bindlessIndex;
    context_id->holdBuffer(mesh.processedIndices.get(), bindlessIndex);
    mesh.piBindlessIndex = bindlessIndex;

    meta.indexAndVertexBufferIndex |= bindlessIndex << 16;

    d3d::resource_barrier(ResourceBarrierDesc(mesh.processedIndices.get(), bindlessSRVBarrier));
  }

  {
    // Always copy the vertices to be independent from the streaming system.

    static int nameGen = 0;

    int bufferSize = mesh.vertexStride * mesh.vertexCount;
    int dwordCount = (bufferSize + 3) / 4;
    mesh.processedVertices.buffer.reset(
      d3d::buffers::create_persistent_sr_byte_address(dwordCount, String(20, "bvh_vb_%d", ++nameGen)));
    mesh.processedVertices.offset = 0;

    HANDLE_LOST_DEVICE_STATE(mesh.processedVertices.buffer, );

    info.vertices->copyTo(mesh.processedVertices.get(), 0, mesh.vertexStride * (mesh.baseVertex + mesh.startVertex),
      mesh.vertexStride * mesh.vertexCount);
    mesh.startVertex = 0;
    mesh.baseVertex = 0;
    meta.startVertex = 0;

    uint32_t bindlessIndex;
    context_id->holdBuffer(mesh.processedVertices.get(), bindlessIndex);
    mesh.pvBindlessIndex = bindlessIndex;

    meta.indexAndVertexBufferIndex |= bindlessIndex;

    d3d::resource_barrier(ResourceBarrierDesc(mesh.processedVertices.get(), bindlessSRVBarrier));
  }

  if (!info.vertexProcessor || info.vertexProcessor->isOneTimeOnly())
    context_id->halfBakedMeshes.insert(mesh_id);
}

void add_mesh(ContextId context_id, uint64_t mesh_id, const MeshInfo &info)
{
  G_ASSERT(info.vertices);

  context_id->hasPendingMeshActions.store(true, std::memory_order_relaxed);
  OSSpinlockScopedLock lock(context_id->pendingMeshActionsLock);
  if (auto iter = context_id->pendingMeshActions.find(mesh_id); iter != context_id->pendingMeshActions.end())
    iter->second = info;
  else
    context_id->pendingMeshActions.insert({mesh_id, eastl::move(info)});
}

static void do_remove_mesh(ContextId context_id, uint64_t mesh_id)
{
  context_id->removeFromCompaction(mesh_id);

  auto iter = context_id->meshes.find(mesh_id);
  if (iter != context_id->meshes.end())
  {
    iter->second.teardown(context_id);
    context_id->meshes.erase(iter);
  }
  else
  {
    iter = context_id->impostors.find(mesh_id);
    if (iter != context_id->impostors.end())
    {
      iter->second.teardown(context_id);
      context_id->impostors.erase(iter);
    }
  }
}

void remove_mesh(ContextId context_id, uint64_t mesh_id)
{
  do_remove_mesh(context_id, mesh_id);
  {
    OSSpinlockScopedLock lock(context_id->pendingMeshActionsLock);
    context_id->pendingMeshActions.erase(mesh_id);
    context_id->hasPendingMeshActions.store(!context_id->pendingMeshActions.empty(), std::memory_order_relaxed);
  }
  context_id->halfBakedMeshes.erase(mesh_id);
}

static void handle_pending_mesh_actions(ContextId context_id)
{
  OSSpinlockScopedLock lock(context_id->pendingMeshActionsLock);

  int counter = 0;
  for (auto iter = context_id->pendingMeshActions.begin();
       iter != context_id->pendingMeshActions.end() && counter < 100 && !is_in_lost_device_state; counter++)
  {
    // When the mesh is already added, but not yet built, we need to remove it as the build data is not valid anymore.
    context_id->halfBakedMeshes.erase(iter->first);
    do_add_mesh(context_id, iter->first, iter->second);
    iter = context_id->pendingMeshActions.erase(iter);
  }

  context_id->hasPendingMeshActions.store(!context_id->pendingMeshActions.empty(), std::memory_order_relaxed);
}

template <typename Callback>
static void patch_per_instance_data(Context::HWInstanceMap &instances, dag::Vector<Point4, framemem_allocator> &output,
  const dag::Vector<Point4> &input, Callback &&callback)
{
  if (input.empty())
    return;

  int preSize = output.size();

  for (auto &instance : instances)
    callback(instance, preSize);

  output.resize(preSize + input.size());
  memcpy(output.data() + preSize, input.data(), input.size() * sizeof(Point4));
}

void process_meshes(ContextId context_id, BuildBudget budget)
{
  if (!per_frame_processing_enabled)
  {
    logdbg("[BVH] Device is in reset mode.");
    return;
  }
  TIME_D3D_PROFILE_NAME(bvh_process_meshes, String(128, "bvh_process_meshes for %s", context_id->name.data()));

  CHECK_LOST_DEVICE_STATE();

  handle_pending_mesh_actions(context_id);

  int blasModelBudget = per_frame_blas_model_budget[int(budget)];
  int triangleCount = 0;
  int blasCount = 0;

  auto getBlas = [&](Context::BLASCompaction &c) -> UniqueBLAS * {
    auto mesh = context_id->meshes.find(c.meshId);
    if (mesh == context_id->meshes.end())
    {
      mesh = context_id->impostors.find(c.meshId);
      if (mesh == context_id->impostors.end())
        return nullptr;
    }

    return &mesh->second.blas;
  };

  for (auto iter = context_id->halfBakedMeshes.begin();
       iter != context_id->halfBakedMeshes.end() && blasModelBudget > 0 && !is_in_lost_device_state;)
  {
    bool hasMesh = false;

    auto meshIter = context_id->meshes.find(*iter);
    if (meshIter != context_id->meshes.end())
      hasMesh = true;
    else
    {
      meshIter = context_id->impostors.find(*iter);
      if (meshIter != context_id->impostors.end())
        hasMesh = true;
    }

    G_ASSERT(hasMesh);

    auto &mesh = meshIter->second;
    auto &meta = context_id->meshMetaAllocator.get(mesh.metaAllocId);

    bool needBlasBuild = false;
    UniqueBVHBuffer transformedVertices;
    UniqueOrReferencedBVHBuffer pb(transformedVertices);
    process_vertices(context_id, *iter, mesh, nullptr, nullptr, V_C_ONE, V_C_ONE, pb, false, needBlasBuild, meta, meta, false);

    CHECK_LOST_DEVICE_STATE();

    if (transformedVertices)
    {
      context_id->releaseBuffer(mesh.processedVertices.get());
      mesh.processedVertices.buffer.swap(transformedVertices);
      mesh.processedVertices.offset = 0;
      transformedVertices.reset();
    }

    bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;

    RaytraceGeometryDescription desc[2];
    memset(&desc, 0, sizeof(desc));
    desc[0].type = RaytraceGeometryDescription::Type::TRIANGLES;
    desc[0].data.triangles.vertexBuffer = mesh.processedVertices.get();
    desc[0].data.triangles.indexBuffer = mesh.processedIndices.get();
    desc[0].data.triangles.vertexCount = mesh.vertexCount;
    desc[0].data.triangles.vertexStride = meta.texcoordNormalColorOffsetAndVertexStride & 255;
    desc[0].data.triangles.vertexOffset = mesh.baseVertex;
    desc[0].data.triangles.vertexOffsetExtraBytes = mesh.positionOffset;
    desc[0].data.triangles.vertexFormat = mesh.processedPositionFormat;
    desc[0].data.triangles.indexCount = mesh.indexCount;
    desc[0].data.triangles.indexOffset = meta.startIndex;
    desc[0].data.triangles.flags =
      (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;

    if (mesh.posMul != Point4::ONE || mesh.posAdd != Point4::ZERO)
    {
      float m[12];
      m[0] = mesh.posMul.x;
      m[1] = 0;
      m[2] = 0;
      m[3] = mesh.posAdd.x;

      m[4] = 0;
      m[5] = mesh.posMul.y;
      m[6] = 0;
      m[7] = mesh.posAdd.y;

      m[8] = 0;
      m[9] = 0;
      m[10] = mesh.posMul.z;
      m[11] = mesh.posAdd.z;

      desc[0].data.triangles.transformBuffer = transform_buffer_manager.alloc(1, desc[0].data.triangles.transformOffset);
      HANDLE_LOST_DEVICE_STATE(desc[0].data.triangles.transformBuffer, );
      desc[0].data.triangles.transformBuffer->updateDataWithLock(sizeof(m) * desc[0].data.triangles.transformOffset, sizeof(m), //-V522
        m, 0);
    }

    int descCount = (mesh.vertexProcessor && mesh.vertexProcessor->isGeneratingSecondaryVertices()) ? 2 : 1;
    if (descCount == 2)
    {
      desc[1].type = RaytraceGeometryDescription::Type::TRIANGLES;
      desc[1].data.triangles.transformBuffer = desc[0].data.triangles.transformBuffer;
      desc[1].data.triangles.transformOffset = desc[0].data.triangles.transformOffset;
      desc[1].data.triangles.vertexBuffer = mesh.processedVertices.get();
      desc[1].data.triangles.indexBuffer = mesh.processedIndices.get();
      desc[1].data.triangles.vertexCount = mesh.vertexCount;
      desc[1].data.triangles.vertexOffset = mesh.vertexCount;
      desc[1].data.triangles.vertexStride = meta.texcoordNormalColorOffsetAndVertexStride & 255;
      desc[1].data.triangles.vertexFormat = mesh.processedPositionFormat;
      desc[1].data.triangles.indexCount = mesh.indexCount;
      desc[1].data.triangles.indexOffset = meta.startIndex;
      desc[1].data.triangles.flags =
        (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
    }

    raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
    buildInfo.flags = RaytraceBuildFlags::FAST_TRACE;
    Context::BLASCompaction *compaction = context_id->beginBLASCompaction(meshIter->first);
    if (compaction)
    {
      buildInfo.flags |= RaytraceBuildFlags::ALLOW_COMPACTION;
      buildInfo.compactedSizeOutputBuffer = compaction->compactedSize;
    }

    mesh.blas = UniqueBLAS::create(desc, descCount, buildInfo.flags);
    HANDLE_LOST_DEVICE_STATE(mesh.blas, );

    buildInfo.geometryDesc = desc;
    buildInfo.geometryDescCount = descCount;
    buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(mesh.blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
    buildInfo.scratchSpaceBufferSizeInBytes = mesh.blas.getBuildScratchSize();

    d3d::build_bottom_acceleration_structure(mesh.blas.get(), buildInfo);

    if (compaction)
      compaction->beginSizeQuery();

    blasModelBudget--;
    triangleCount += mesh.indexCount / 3;
    blasCount++;

    iter = context_id->halfBakedMeshes.erase(iter);
  }

  for (auto iter = context_id->blasCompactions.begin(); iter != context_id->blasCompactions.end() && !is_in_lost_device_state;)
  {
    auto &compaction = *iter;
    switch (compaction.stage)
    {
      case Context::BLASCompaction::Stage::Prepared: G_ASSERT(false); break;
      case Context::BLASCompaction::Stage::WaitingSize:
        if (d3d::get_event_query_status(compaction.query.get(), false))
        {
          compaction.compactedSizeValue = 0;
          if (auto bufferData = lock_sbuffer<const uint64_t>(compaction.compactedSize, 0, 0, VBLOCK_READONLY))
            compaction.compactedSizeValue = bufferData[0];

          del_d3dres(compaction.compactedSize);

          if (!compaction.compactedSizeValue)
          {
            iter = context_id->blasCompactions.erase(iter);
            break;
          }
          else
            compaction.stage = Context::BLASCompaction::Stage::WaitingGPUTime;
        }

        if (compaction.stage != Context::BLASCompaction::Stage::WaitingGPUTime)
          break;

        // Fall through!!!

      case Context::BLASCompaction::Stage::WaitingGPUTime:
        if (blasModelBudget > 0)
        {
          G_ASSERT(compaction.compactedSizeValue);

          auto blas = getBlas(compaction);

          compaction.compactedBlas = UniqueBLAS::create(compaction.compactedSizeValue);
          HANDLE_LOST_DEVICE_STATE(compaction.compactedBlas, );
          d3d::copy_raytrace_acceleration_structure(compaction.compactedBlas.get(), blas->get(), true);

          d3d::issue_event_query(compaction.query.get());

          compaction.stage = Context::BLASCompaction::Stage::WaitingCompaction;

          // Compacting is a heavy operation.
          blasModelBudget -= 10;
        }
        break;
      case Context::BLASCompaction::Stage::WaitingCompaction:
        if (d3d::get_event_query_status(compaction.query.get(), false))
        {
          if (auto blas = getBlas(compaction))
            blas->swap(compaction.compactedBlas);

          iter = context_id->blasCompactions.erase(iter);
          continue;
        }
        break;
    }

    ++iter;
  }

  auto regGameTex = [&](int var_id, int sampler_id, TEXTUREID &tex_id, d3d::SamplerHandle &sampler, uint32_t &bindless_id,
                      uint32_t *size) {
    TEXTUREID texId = ShaderGlobal::get_tex(var_id);
    auto samplerId = ShaderGlobal::get_sampler(sampler_id);
    if (tex_id != texId)
    {
      if (tex_id != BAD_TEXTUREID)
        context_id->releaseTexure(tex_id);
      tex_id = texId;
      sampler = samplerId;
      if (auto texture = context_id->holdTexture(texId, bindless_id, samplerId); texture && size)
      {
        TextureInfo info;
        texture->getinfo(info);
        *size = info.w;
      }
    }
    else if (tex_id != BAD_TEXTUREID && sampler != samplerId)
    {
      // To update the sampler, we re-register the texture with the new sampler
      // then release it to make the reference counter correct.

      sampler = samplerId;
      context_id->holdTexture(tex_id, bindless_id, samplerId);
      context_id->releaseTexure(tex_id);
    }
  };

  static int paint_details_texVarId = get_shader_variable_id("paint_details_tex", true);
  static int paint_details_tex_samplerstateVarId = get_shader_variable_id("paint_details_tex_samplerstate", true);
  static int grass_land_color_maskVarId = get_shader_variable_id("grass_land_color_mask", true);
  static int grass_land_color_mask_samplerstateVarId = get_shader_variable_id("grass_land_color_mask_samplerstate", true);
  regGameTex(paint_details_texVarId, paint_details_tex_samplerstateVarId, context_id->registeredPaintTexId,
    context_id->registeredPaintTexSampler, context_id->paintTexBindlessIds, &context_id->paintTexSize);
  regGameTex(grass_land_color_maskVarId, grass_land_color_mask_samplerstateVarId, context_id->registeredLandColorTexId,
    context_id->registeredLandColorTexSampler, context_id->landColorTexBindlessIds, nullptr);

  for (auto iter = context_id->camoTextures.begin(); iter != context_id->camoTextures.end();)
    if (get_managed_res_refcount(TEXTUREID(iter->first)) < 2)
    {
      context_id->releaseTexure(TEXTUREID(iter->first));
      iter = context_id->camoTextures.erase(iter);
    }
    else
      ++iter;

  static bool showBVHBuildEvents = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("showBVHBuildEvents", false);

  if (showBVHBuildEvents && triangleCount > 0)
    visuallog::logmsg(String(0, "The BVH build %d triangles for %d BLASes.", triangleCount, blasCount));
}

void build(ContextId context_id, const TMatrix &itm, const TMatrix4 &projTm, const Point3 &camera_pos, const Point3 &light_direction)
{
  if (!per_frame_processing_enabled)
  {
    logdbg("[BVH] Device is in reset mode.");
    return;
  }
  CHECK_LOST_DEVICE_STATE();

  TIME_D3D_PROFILE_NAME(bvh_build, String(128, "bvh_build for %s", context_id->name.data()));

  ri::wait_ri_gen_instances_update(context_id);
  ri::wait_ri_extra_instances_update(context_id);

  context_id->processDeathrow();

  Sbuffer *grassInstances = nullptr;
  Sbuffer *grassInstanceCount = nullptr;
  grass::get_instances(context_id, grassInstances, grassInstanceCount);

  Sbuffer *gobjInstances = nullptr;
  Sbuffer *gobjInstanceCount = nullptr;
  gobj::get_instances(context_id, gobjInstances, gobjInstanceCount);

  Sbuffer *fxInstances = nullptr;
  Sbuffer *fxInstanceCount = nullptr;
  fx::get_instances(context_id, fxInstances, fxInstanceCount);

  int cablesMetaAllocId = -1;
  auto cableBlases = &cables::get_blases(context_id, cablesMetaAllocId);

#if DAGOR_DBGLEVEL > 0
  if (!bvh_grass_enable)
  {
    grassInstances = nullptr;
    grassInstanceCount = nullptr;
  }
  if (!bvh_gpuobject_enable)
  {
    gobjInstances = nullptr;
    gobjInstanceCount = nullptr;
  }
  if (!bvh_particles_enable)
  {
    fxInstances = nullptr;
    fxInstanceCount = nullptr;
  }
  if (!bvh_cables_enable)
  {
    cableBlases = nullptr;
    cablesMetaAllocId = -1;
  }
#endif

  // The buffer size is used to hold instances for the grass and gobj. There is no indirect AS build
  // so the unused instances are zeroed out in the buffer, and ignored during build.
  int grassBufferSize = grassInstances ? grassInstances->getNumElements() : 0;
  int gobjBufferSize = gobjInstances ? gobjInstances->getNumElements() : 0;
  int fxBufferSize = fxInstances ? fxInstances->getNumElements() : 0;

  int impostorCount = 0;
  for (auto &instances : context_id->impostorInstances)
    impostorCount += instances.size();

  int riGenCount = 0;
  for (auto &instances : context_id->riGenInstances)
    riGenCount += instances.size();

  int riExtraCount = 0;
  for (auto &instances : context_id->riExtraInstances)
    riExtraCount += instances.size();

  int riExtraTreeCount = 0;
  for (auto &instances : context_id->riExtraTreeInstances)
    riExtraTreeCount += instances.size();

  int dynrendCount = 0;
  for (auto &instances : context_id->dynrendInstances)
    dynrendCount += instances.second.size();

  auto terrainBlases = terrain::get_blases(context_id);

  auto instanceCount = context_id->genericInstances.size() + riGenCount + riExtraCount + riExtraTreeCount + dynrendCount +
                       terrainBlases.size() + (cableBlases ? cableBlases->size() : 0) + impostorCount; //-V547

  // Mark them invalid for now. They will be marked valid later as they are getting uploaded.
  context_id->tlasMainValid = false;
  context_id->tlasTerrainValid = false;
  context_id->tlasParticlesValid = false;

  if (!instanceCount && !grassBufferSize && !gobjBufferSize && !fxBufferSize)
    return;

  G_ASSERT(sizeof(HWInstance) == d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize);

  static_assert(sizeof(HWInstance) == 64, "HWInstance size must be 64 bytes. If this changes, adjust the allocation size below.");

  auto round_up = [](uint32_t value, uint32_t alignment) { return (value + alignment - 1) & ~(alignment - 1); };

  auto uploadSizeMain = max(round_up(instanceCount + grassBufferSize + gobjBufferSize, 1024 << 3), 1U << 17);
  auto uploadSizeTerrain = round_up(terrainBlases.size() + 1, 64);
  auto uploadSizeParticles = max(round_up(fxBufferSize, 1024), 1U << 13);

  if (context_id->tlasUploadMain && uploadSizeMain > context_id->tlasUploadMain->getNumElements())
    context_id->tlasUploadMain.close();
  if (context_id->tlasUploadTerrain && uploadSizeTerrain > context_id->tlasUploadTerrain->getNumElements())
    context_id->tlasUploadTerrain.close();
  if (context_id->tlasUploadParticles && uploadSizeParticles > context_id->tlasUploadParticles->getNumElements())
    context_id->tlasUploadParticles.close();

  bool reallocateMainTlas = !context_id->tlasUploadMain;
  bool reallocateTerrainTlas = !context_id->tlasUploadTerrain;
  bool reallocateParticlesTlas = !context_id->tlasUploadParticles;

  if (!context_id->tlasUploadMain)
  {
    context_id->tlasUploadMain =
      dag::buffers::create_ua_sr_structured(sizeof(HWInstance), uploadSizeMain, ccn(context_id, "bvh_tlas_upload_main"));
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadMain, );
    logdbg("tlasUploadMain resized to %u", uploadSizeMain);
  }
  if (!context_id->tlasUploadTerrain)
  {
    context_id->tlasUploadTerrain =
      dag::buffers::create_ua_sr_structured(sizeof(HWInstance), uploadSizeTerrain, ccn(context_id, "bvh_tlas_upload_terrain"));
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadTerrain, );
    logdbg("tlasUploadTerrain resized to %u", uploadSizeTerrain);
  }
  if (!context_id->tlasUploadParticles)
  {
    context_id->tlasUploadParticles =
      dag::buffers::create_ua_sr_structured(sizeof(HWInstance), uploadSizeParticles, ccn(context_id, "bvh_tlas_upload_particles"));
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadParticles, );
    logdbg("tlasUploadParticles resized to %u", uploadSizeParticles);
  }

  context_id->animatedInstanceCount = 0;

  dag::Vector<HWInstance, framemem_allocator> instanceDescs;
  instanceDescs.reserve(instanceCount);

  dag::Vector<Point4, framemem_allocator> perInstanceData;
  perInstanceData.reserve(riGenCount + riExtraTreeCount + dynrendCount + impostorCount + 1);
  perInstanceData.emplace_back(0, 0, 0, 0);

  dag::Vector<::raytrace::BatchedBottomAccelerationStructureBuildInfo, framemem_allocator> blasUpdates;
  dag::Vector<RaytraceGeometryDescription, framemem_allocator> updateGeoms;
  blasUpdates.reserve(512);
  updateGeoms.reserve(512);

  auto itmRelative = itm;
  itmRelative.setcol(3, Point3::ZERO);
  auto frustumRelative = Frustum(TMatrix4(orthonormalized_inverse(itmRelative)) * projTm);
  auto frustumAbsolute = Frustum(TMatrix4(orthonormalized_inverse(itm)) * projTm);

  auto addInstances = [&](const Context::InstanceMap &instances, dag::Vector<HWInstance, framemem_allocator> &hwInstances,
                        uint32_t group_mask, bool is_camera_relative, const char *name) {
    CHECK_LOST_DEVICE_STATE();

    TIME_PROFILE_NAME(addInstance, name);

    const float maxLightDistForBvhShadow = 20.0f;
    auto lightDirection = v_mul(v_ldu_p3_safe(&light_direction.x), v_splats(maxLightDistForBvhShadow * 2));
    auto cameraPos = v_ldu_p3_safe(&camera_pos.x);

    for (auto &instance : instances)
    {
      CHECK_LOST_DEVICE_STATE();

      if (context_id->halfBakedMeshes.count(instance.meshId))
        continue;

      auto iter = context_id->meshes.find(instance.meshId);
      if (iter == context_id->meshes.end())
        continue;

      auto &mesh = iter->second;

      int metaIndex = instance.metaAllocId < 0 ? mesh.metaAllocId : instance.metaAllocId;
      auto &blas = instance.uniqueBlas ? *instance.uniqueBlas : mesh.blas;

      if (mesh.vertexProcessor)
      {
        G_ASSERT(!mesh.vertexProcessor->isOneTimeOnly() && instance.uniqueTransformBuffer);

        bool needBlasBuild = false;

        auto animatedVertices = UniqueOrReferencedBVHBuffer(*instance.uniqueTransformBuffer); //-V595
        auto verticesRecycled = instance.uniqueIsRecycled;

        MeshMeta &meta = context_id->meshMetaAllocator.get(metaIndex);
        MeshMeta &baseMeta = context_id->meshMetaAllocator.get(mesh.metaAllocId);

        if (instance.metaAllocId > -1 && !meta.isInitialized())
        {
          // The meta is specific to an instance and not yet initialized.
          meta = baseMeta;
          meta.markInitialized();
        }

        process_vertices(context_id, instance.meshId, mesh, &instance, is_camera_relative ? &frustumRelative : &frustumAbsolute,
          is_camera_relative ? v_zero() : cameraPos, lightDirection, animatedVertices, verticesRecycled, needBlasBuild, meta, baseMeta,
          true);

        CHECK_LOST_DEVICE_STATE();

        meta.vertexOffset = animatedVertices.getOffset();

        // If the BLAS is not unique, then only build it when it has not been built at all.
        if (needBlasBuild && (!blas || instance.uniqueBlas))
        {
          bool isAnimated = mesh.vertexProcessor && !mesh.vertexProcessor->isOneTimeOnly();

          RaytraceBuildFlags flags =
            isAnimated ? RaytraceBuildFlags::FAST_BUILD | RaytraceBuildFlags::ALLOW_UPDATE : RaytraceBuildFlags::FAST_TRACE;
          bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;

          auto &update = blasUpdates.emplace_back();
          auto &geom = updateGeoms.emplace_back();

          geom.type = RaytraceGeometryDescription::Type::TRIANGLES;
          geom.data.triangles.vertexBuffer = animatedVertices.get();
          geom.data.triangles.vertexOffsetExtraBytes = animatedVertices.getOffset();
          geom.data.triangles.indexBuffer = mesh.processedIndices.get();
          geom.data.triangles.vertexCount = mesh.vertexCount;
          geom.data.triangles.vertexStride = meta.texcoordNormalColorOffsetAndVertexStride & 255;
          geom.data.triangles.vertexFormat = mesh.processedPositionFormat;
          geom.data.triangles.indexCount = mesh.indexCount;
          geom.data.triangles.indexOffset = meta.startIndex;
          geom.data.triangles.flags =
            (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;

          bool isNew = false;

          if (!blas)
          {
            blas = UniqueBLAS::create(&geom, 1, flags);
            isNew = true;

            HANDLE_LOST_DEVICE_STATE(blas, );

            if (isAnimated && mesh.blas && mesh.blas != blas)
            {
              d3d::copy_raytrace_acceleration_structure(blas.get(), mesh.blas.get());
              isNew = false;
            }
          }

          update.as = blas.get();
          update.basbi.geometryDescCount = 1;
          update.basbi.flags = flags;
          update.basbi.doUpdate = isAnimated && !isNew;
          update.basbi.scratchSpaceBufferSizeInBytes =
            update.basbi.doUpdate ? blas.getUpdateScratchSize() : blas.getBuildScratchSize();
          update.basbi.scratchSpaceBuffer =
            alloc_scratch_buffer(update.basbi.scratchSpaceBufferSizeInBytes, update.basbi.scratchSpaceBufferOffsetInBytes);

          HANDLE_LOST_DEVICE_STATE(update.basbi.scratchSpaceBuffer, );

          // After building the first instance, a copy of it is made into the mesh structure.
          // This copy is used when new instances needs to be made, and with the new instance,
          // there is no need to build the BLAS topology, only an update, which is about 50
          // times faster.
          // If the template is being created, we build immediately.
          if (!mesh.blas)
          {
            // inside delay/continue sync we must use only that work that can be batched together
            // otherwise we can't properly generate barriers, so simply make another batch when we need to build BLAS
            const bool isVulkan = d3d::get_driver_code().is(d3d::vulkan);
            if (isVulkan)
              d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);

            G_ASSERT(update.basbi.scratchSpaceBufferSizeInBytes > 0);

            update.basbi.geometryDesc = &geom;

            d3d::resource_barrier(ResourceBarrierDesc(geom.data.triangles.vertexBuffer, bindlessSRVBarrier));
            d3d::build_bottom_acceleration_structure(update.as, update.basbi);
            d3d::resource_barrier(ResourceBarrierDesc(geom.data.triangles.vertexBuffer, bindlessUAVBarrier));

            if ((flags & RaytraceBuildFlags::ALLOW_UPDATE) != RaytraceBuildFlags::NONE)
            {
              // build_bottom_acceleration_structure is not flushing the BLAS when it is
              // updateable, so we flush it here, before cloning it.
              d3d::resource_barrier(ResourceBarrierDesc(update.as));
            }

            mesh.blas = UniqueBLAS::create(&geom, 1, update.basbi.flags);
            d3d::copy_raytrace_acceleration_structure(mesh.blas.get(), update.as);

            blasUpdates.pop_back();
            updateGeoms.pop_back();

            if (isVulkan)
              d3d::driver_command(Drv3dCommand::DELAY_SYNC);
          }
        }
      }

      bool flipWinding = need_winding_flip(mesh, instance);

      int perInstanceDataIndex = 0;
      if (instance.perInstanceData.has_value())
      {
        perInstanceDataIndex = perInstanceData.size();
        perInstanceData.emplace_back(instance.perInstanceData.value());
      }

      auto &desc = hwInstances.emplace_back();
      desc.transform = instance.transform;
      desc.instanceID = metaIndex;
      desc.instanceMask = instance.noShadow ? bvhGroupNoShadow : group_mask;
      desc.instanceContributionToHitGroupIndex = perInstanceDataIndex;
      desc.flags = mesh.enableCulling ? (flipWinding ? RaytraceGeometryInstanceDescription::TRIANGLE_CULL_FLIP_WINDING
                                                     : RaytraceGeometryInstanceDescription::NONE)
                                      : RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
      desc.blasGpuAddress = blas.getGPUAddress();

      if (!is_camera_relative)
        realign(desc.transform, camera_pos);
    }
  };

  {
    TIME_D3D_PROFILE(add_and_animate_instances);

    int bufferCount = 0;
    for (auto &alloc : context_id->processBufferAllocators)
      bufferCount += alloc.second.pools.size();

    dag::Vector<Sbuffer *, framemem_allocator> rbBuffers;
    rbBuffers.reserve(bufferCount);

    for (auto &alloc : context_id->processBufferAllocators)
      for (auto &pool : alloc.second.pools)
        rbBuffers.push_back(pool.buffer.get());

    // vulkan don't allow barrier mixing used to batch prepare  dx12 barriers
    const bool isVulkan = d3d::get_driver_code().is(d3d::vulkan);
    if (!rbBuffers.empty() && !isVulkan)
    {
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessUAVBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size()));
    }
    else
      d3d::driver_command(Drv3dCommand::DELAY_SYNC);

    addInstances(context_id->genericInstances, instanceDescs, bvhGroupRiExtra, false, "generic");
    CHECK_LOST_DEVICE_STATE();

    for (auto &instances : context_id->riGenInstances)
    {
      addInstances(instances, instanceDescs, bvhGroupRiGen, false, "riGen");
      CHECK_LOST_DEVICE_STATE();
    }
    for (auto &instances : context_id->riExtraTreeInstances)
    {
      addInstances(instances, instanceDescs, bvhGroupRiGen, false, "riExtraTree");
      CHECK_LOST_DEVICE_STATE();
    }
    for (auto &instances : context_id->dynrendInstances)
    {
      dyn::set_up_dynrend_context_for_processing(instances.first);
      addInstances(instances.second, instanceDescs, bvhGroupDynrend, true, "dynrend");
      CHECK_LOST_DEVICE_STATE();
    }

    CHECK_LOST_DEVICE_STATE();

    if (!rbBuffers.empty() && !isVulkan)
    {
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessSRVBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size()));
    }
    else
      d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
  }

  {
    TIME_D3D_PROFILE_NAME(procedural_blas_builds, String(64, "procedural_blas_builds: %d", blasUpdates.size()));
    if (showProceduralBLASBuildCount)
      visuallog::logmsg(String(64, "Procedural BLAS builds: %d", blasUpdates.size()));

    for (auto [index, update] : enumerate(blasUpdates))
      update.basbi.geometryDesc = &updateGeoms[index];

    d3d::build_bottom_acceleration_structures(blasUpdates.data(), blasUpdates.size());
  }

  int terrainDescIndex = instanceDescs.size();

  {
    TIME_D3D_PROFILE(terrain);

    for (auto [blasIx, blas] : enumerate(terrainBlases))
    {
      auto &origin = eastl::get<2>(blas);

      auto &desc = instanceDescs.emplace_back();
      desc.transform.row0 = v_make_vec4f(1, 0, 0, origin.x - camera_pos.x);
      desc.transform.row1 = v_make_vec4f(0, 1, 0, 0 - camera_pos.y);
      desc.transform.row2 = v_make_vec4f(0, 0, 1, origin.y - camera_pos.z);

      desc.instanceID = eastl::get<1>(blas);
      desc.instanceMask = bvhGroupTerrain;
      desc.instanceContributionToHitGroupIndex = 0;
      desc.flags = RaytraceGeometryInstanceDescription::NONE;
      desc.blasGpuAddress = eastl::get<0>(blas);
    }
  }

#if DAGOR_DBGLEVEL > 0
  if (cableBlases)
#endif
  {
    TIME_D3D_PROFILE(cables);

    for (auto [blasIx, blas] : enumerate(*cableBlases))
    {
      auto &desc = instanceDescs.emplace_back();
      desc.transform.row0 = v_make_vec4f(1, 0, 0, -camera_pos.x);
      desc.transform.row1 = v_make_vec4f(0, 1, 0, -camera_pos.y);
      desc.transform.row2 = v_make_vec4f(0, 0, 1, -camera_pos.z);

      desc.instanceID = cablesMetaAllocId;
      desc.instanceMask = bvhGroupRiExtra;
      desc.instanceContributionToHitGroupIndex = 0;
      desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
      desc.blasGpuAddress = blas.getGPUAddress();
    }
  }

  {
    TIME_PROFILE(patch_per_instance_data);

    for (auto [bucket, instances] : enumerate(context_id->riExtraInstances))
      patch_per_instance_data(instances, perInstanceData, context_id->riExtraInstanceData[bucket], [](HWInstance &instance, int base) {
        if (instance.instanceContributionToHitGroupIndex == 0xFFFFFFU)
          instance.instanceContributionToHitGroupIndex = 0;
        else
          instance.instanceContributionToHitGroupIndex += base;
      });

    for (auto [bucket, instances] : enumerate(context_id->impostorInstances))
      patch_per_instance_data(instances, perInstanceData, context_id->impostorInstanceData[bucket],
        [](HWInstance &instance, int base) { instance.instanceContributionToHitGroupIndex += base; });
  }

  if (!perInstanceData.empty())
  {
    TIME_D3D_PROFILE(upload_per_instance_data);

    if (context_id->perInstanceData && context_id->perInstanceData->getNumElements() < perInstanceData.size())
      context_id->perInstanceData.close();

    if (!context_id->perInstanceData)
    {
      context_id->perInstanceData = dag::buffers::create_persistent_sr_structured(sizeof(Point4), perInstanceData.size(),
        ccn(context_id, "bvh_per_instance_data"));
      HANDLE_LOST_DEVICE_STATE(context_id->perInstanceData, );
    }

    context_id->perInstanceData->updateDataWithLock(0, perInstanceData.size() * sizeof(Point4), perInstanceData.data(),
      VBLOCK_DISCARD);
  }

  {
    TIME_D3D_PROFILE(upload_meta);

    int metaCount = context_id->meshMetaAllocator.size();

    if (context_id->meshMeta && context_id->meshMeta->getNumElements() < metaCount)
      context_id->meshMeta.close();

    if (!context_id->meshMeta)
    {
      context_id->meshMeta = dag::buffers::create_persistent_sr_structured(sizeof(MeshMeta), metaCount, ccn(context_id, "bvh_meta"));
      HANDLE_LOST_DEVICE_STATE(context_id->meshMeta, );
    }

    if (auto upload = lock_sbuffer<MeshMeta>(context_id->meshMeta.getBuf(), 0, 0, VBLOCK_DISCARD | VBLOCK_WRITEONLY))
      memcpy(upload.get(), context_id->meshMetaAllocator.data(), metaCount * sizeof(MeshMeta));
  }

  auto copyHwInstances = [](Sbuffer *instanceCount, Sbuffer *instances, Sbuffer *uploadBuffer, int bufferSize, int startInstance) {
    if (instanceCount)
    {
      static int bvh_hwinstance_copy_start_instanceVarId = get_shader_variable_id("bvh_hwinstance_copy_start_instance");
      static int bvh_hwinstance_copy_instance_slotsVarId = get_shader_variable_id("bvh_hwinstance_copy_instance_slots");
      static int bvh_hwinstance_copy_modeVarId = get_shader_variable_id("bvh_hwinstance_copy_mode");

      static int source_const_no = ShaderGlobal::get_slot_by_name("bvh_hwinstance_copy_source_const_no");
      static int instance_count_const_no = ShaderGlobal::get_slot_by_name("bvh_hwinstance_copy_instance_count_const_no");
      static int output_uav_no = ShaderGlobal::get_slot_by_name("bvh_hwinstance_copy_output_uav_no");

      TIME_D3D_PROFILE_NAME(copy_hwinstances, String(64, "copy hw instance: %d", bufferSize));

      ShaderGlobal::set_int(bvh_hwinstance_copy_start_instanceVarId, startInstance);
      ShaderGlobal::set_int(bvh_hwinstance_copy_instance_slotsVarId, bufferSize);
      ShaderGlobal::set_int(bvh_hwinstance_copy_modeVarId, 0);

      d3d::set_buffer(STAGE_CS, source_const_no, instances);
      d3d::set_buffer(STAGE_CS, instance_count_const_no, instanceCount);
      d3d::set_rwbuffer(STAGE_CS, output_uav_no, uploadBuffer);

      get_hw_instance_copy_shader().dispatchThreads(bufferSize, 1, 1);
    }
  };

  int cpuInstanceCount = 0;

  {
    TIME_D3D_PROFILE(copy_to_tlas_main_upload);

    int uploadCount = instanceDescs.size();
    for (auto &instances : context_id->impostorInstances)
      uploadCount += instances.size();
    for (auto &instances : context_id->riExtraInstances)
      uploadCount += instances.size();

    {
      auto upload = lock_sbuffer<HWInstance>(context_id->tlasUploadMain.getBuf(), 0, uploadCount, VBLOCK_WRITEONLY);
      HANDLE_LOST_DEVICE_STATE(upload, );

      TIME_PROFILE(memcpy);
      auto cursor = upload.get();
      memcpy(cursor, instanceDescs.data(), instanceDescs.size() * sizeof(HWInstance));
      cursor += instanceDescs.size();
      for (auto &instances : context_id->impostorInstances)
      {
        memcpy(cursor, instances.data(), instances.size() * sizeof(HWInstance));
        cursor += instances.size();
      }
      for (auto &instances : context_id->riExtraInstances)
      {
        memcpy(cursor, instances.data(), instances.size() * sizeof(HWInstance));
        cursor += instances.size();
      }

      cpuInstanceCount = cursor - upload.get();
    }

    G_ASSERT(cpuInstanceCount <= instanceCount);

    copyHwInstances(grassInstanceCount, grassInstances, context_id->tlasUploadMain.getBuf(), grassBufferSize, cpuInstanceCount);
    cpuInstanceCount += grassBufferSize;

    copyHwInstances(gobjInstanceCount, gobjInstances, context_id->tlasUploadMain.getBuf(), gobjBufferSize, cpuInstanceCount);
    cpuInstanceCount += gobjBufferSize;
  }

  {
    TIME_D3D_PROFILE(copy_to_tlas_terrain_upload);
    if (auto upload = lock_sbuffer<HWInstance>(context_id->tlasUploadTerrain.getBuf(), 0, 0, VBLOCK_WRITEONLY))
      memcpy(upload.get(), instanceDescs.data() + terrainDescIndex, terrainBlases.size() * sizeof(HWInstance));
  }

  {
    TIME_D3D_PROFILE(copy_to_tlas_particles_upload);
    copyHwInstances(fxInstanceCount, fxInstances, context_id->tlasUploadParticles.getBuf(), fxBufferSize, 0);
  }

  G_ASSERT(cpuInstanceCount <= uploadSizeMain);
  G_ASSERT(cpuInstanceCount <= context_id->tlasUploadMain->getNumElements());

  if (reallocateMainTlas && context_id->tlasMain)
    context_id->tlasMain.reset();
  if (reallocateTerrainTlas && context_id->tlasTerrain)
    context_id->tlasTerrain.reset();
  if (reallocateParticlesTlas && context_id->tlasParticles)
    context_id->tlasParticles.reset();

  if (!context_id->tlasMain && context_id->tlasUploadMain)
  {
    context_id->tlasMain = UniqueTLAS::create(context_id->tlasUploadMain->getNumElements(), RaytraceBuildFlags::FAST_TRACE, "main");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasMain, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasMain.getScratchBuffer(), );

    logdbg("Main TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.", context_id->tlasUploadMain->getNumElements(),
      context_id->tlasMain.getASSize() >> 10, context_id->tlasMain.getScratchBuffer()->getNumElements() >> 10);
  }

  if (!context_id->tlasTerrain && context_id->tlasUploadTerrain)
  {
    context_id->tlasTerrain =
      UniqueTLAS::create(context_id->tlasUploadTerrain->getNumElements(), RaytraceBuildFlags::FAST_TRACE, "terrain");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasTerrain, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasTerrain.getScratchBuffer(), );

    logdbg("Terrain TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.",
      context_id->tlasUploadTerrain->getNumElements(), context_id->tlasTerrain.getASSize() >> 10,
      context_id->tlasTerrain.getScratchBuffer()->getNumElements() >> 10);
  }

  if (!context_id->tlasParticles && context_id->tlasUploadParticles)
  {
    context_id->tlasParticles =
      UniqueTLAS::create(context_id->tlasUploadParticles->getNumElements(), RaytraceBuildFlags::FAST_TRACE, "particle");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasParticles, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasParticles.getScratchBuffer(), );

    logdbg("Particle TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.",
      context_id->tlasUploadParticles->getNumElements(), context_id->tlasParticles.getASSize() >> 10,
      context_id->tlasParticles.getScratchBuffer()->getNumElements() >> 10);
  }

  TIME_D3D_PROFILE(build_tlas);

  context_id->tlasMainValid = context_id->tlasMain && cpuInstanceCount;
  context_id->tlasTerrainValid = context_id->tlasTerrain && terrainBlases.size();
  context_id->tlasParticlesValid = context_id->tlasParticles && fxBufferSize;

  raytrace::BatchedTopAccelerationStructureBuildInfo tlasUpdate[3];
  int tlasCount = 0;

  auto fillTlas = [&](const UniqueTLAS &tlas, const UniqueBuf &instances, uint32_t instance_count) {
    auto &tasbi = tlasUpdate[tlasCount].tasbi;

    G_ASSERT(tlas.get());
    G_ASSERT(tlas.getScratchBuffer());

    tlasUpdate[tlasCount].as = tlas.get();
    tasbi.doUpdate = false;
    tasbi.instanceBuffer = instances.getBuf();
    tasbi.instanceCount = instance_count;
    tasbi.scratchSpaceBufferSizeInBytes = tlas.getBuildScratchSize();
    tasbi.scratchSpaceBufferOffsetInBytes = 0;
    tasbi.scratchSpaceBuffer = tlas.getScratchBuffer();
    tasbi.flags = RaytraceBuildFlags::FAST_TRACE;
    ++tlasCount;

    HANDLE_LOST_DEVICE_STATE(tasbi.scratchSpaceBuffer, );
  };

  if (context_id->tlasMainValid)
  {
    fillTlas(context_id->tlasMain, context_id->tlasUploadMain, cpuInstanceCount);
    CHECK_LOST_DEVICE_STATE();
  }

  if (context_id->tlasTerrainValid)
  {
    fillTlas(context_id->tlasTerrain, context_id->tlasUploadTerrain, terrainBlases.size());
    CHECK_LOST_DEVICE_STATE();
  }

  if (context_id->tlasParticlesValid)
  {
    fillTlas(context_id->tlasParticles, context_id->tlasUploadParticles, fxBufferSize);
    CHECK_LOST_DEVICE_STATE();
  }

  d3d::build_top_acceleration_structures(tlasUpdate, tlasCount);

  context_id->markChangedTextures();
  context_id->bindlessTextureAllocator.getHeap().updateBindings();
  context_id->bindlessBufferAllocator.getHeap().updateBindings();

  static int bvh_originVarId = get_shader_variable_id("bvh_origin");
  ShaderGlobal::set_color4(bvh_originVarId, camera_pos);

  debug::render_debug_context(context_id);

  ri::cut_down_trees(context_id);

  static bool dumpStats = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhDumpMemoryStats", false);
  if (dumpStats)
  {
    auto mb = [](auto v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), decltype(v)(1))); };

    auto stats = bvh::get_memory_statistics(context_id);

    logdbg("BVH memory statistics");
    logdbg("TLAS: %d MB - Upload: %d MB", mb(stats.tlasSize), mb(stats.tlasUploadSize));
    logdbg("Scratch space: %d MB", mb(stats.scratchBuffersSize));
    logdbg("Transform buffers: %d MB", mb(stats.transformBuffersSize));
    logdbg("Mesh count %d, Mesh VB: %d MB, Mesh VB copy: %d MB, Mesh IB: %d MB", stats.meshCount, mb(stats.meshVBSize),
      mb(stats.meshVBCopySize), mb(stats.meshIBSize));
    logdbg("Mesh meta: %d MB, Mesh BLAS: %d MB", mb(stats.meshMetaSize), mb(stats.meshBlasSize));
    logdbg("Skin BLAS: %d MB, Skin VB: %d MB, Skin count: %d", mb(stats.skinBLASSize), mb(stats.skinVBSize), stats.skinCount);
    logdbg("Tree BLAS: %d MB, Tree VB: %d MB, Tree count: %d", mb(stats.treeBLASSize), mb(stats.treeVBSize), stats.treeCount);
    logdbg("Tree cache BLAS: %d MB, Tree cache count: %d", mb(stats.treeCacheBLASSize), stats.treeCacheCount);
    logdbg("Dynamic VB allocator: %d MB", mb(stats.dynamicVBAllocatorSize));
    logdbg("Terrain BLAS: %d MB, Terrain VB: %d MB", mb(stats.terrainBlasSize), mb(stats.terrainVBSize));
    logdbg("Grass BLAS: %d MB, Grass VB: %d MB, Grass IB: %d MB, Grass meta: %d MB, Grass query: %d MB", mb(stats.grassBlasSize),
      mb(stats.grassVBSize), mb(stats.grassIBSize), mb(stats.grassMetaSize), mb(stats.grassQuerySize));
    logdbg("Cable BLAS: %d MB, Cable VB: %d MB, Cable IB: %d MB", mb(stats.cableBLASSize), mb(stats.cableVBSize),
      mb(stats.cableIBSize));
    logdbg("GPU object meta: %d MB, GPU object query: %d MB", mb(stats.gobjMetaSize), mb(stats.gobjQuerySize));
    logdbg("Per instance data: %d MB", mb(stats.perInstanceDataSize));
    logdbg("Compaction data: %d MB - Peak: %d MB", mb(stats.compactionSize), mb(stats.peakCompactionSize));
    logdbg("Atmosphere texture: %d MB", mb(stats.atmosphereTextureSize));
    logdbg("BLAS count: %d", stats.blasCount);
    logdbg("Death row buffer count: %d, buffers size: %d MB", stats.deathRowBufferCount, mb(stats.deathRowBufferSize));
    logdbg("IndexProcessor buffer count: %d, buffers size: %d MB", stats.indexProcessorBufferCount,
      mb(stats.indexProcessorBufferSize));
    logdbg("-------------------------");
    logdbg("Total: %d MB", mb(stats.totalMemory));
    logdbg("-------------------------");
    logdbg("Denoiser - Common: %d MB - AO: %d MB - Reflection: %d MB - Shared: %d MB", stats.demoiserCommonSizeMB,
      stats.demoiserAoSizeMB, stats.demoiserReflectionSizeMB, stats.demoiserTransientSizeMB);
    logdbg("-------------------------");
  }
}

static struct BVHUpdateAtmosphereJob : public cpujobs::IJob
{
  ContextId contextId;
  atmosphere_callback callback;
  Point3 viewPos;
  int lineBegin;
  int lineEnd;

  void doJob() override
  {
    TIME_PROFILE(BVHUpdateAtmosphereJob);

    auto iValues = contextId->atmData.inscatterValues;
    auto lValues = contextId->atmData.lossValues;

    for (int y = lineBegin; y < lineEnd; ++y)
    {
      int tci = y * Context::atmTexWidth;
      float distance = float(y * Context::atmDistanceSteps);
      for (int x = 0; x < Context::atmTexWidth; x++, tci++)
      {
        float angle = XMConvertToRadians(x * Context::atmDegreesPerSample);
        Point3 dir = Point3(sinf(angle), 0, cosf(angle));
        Color3 insc, loss;
        callback(viewPos, dir, distance, insc, loss);
        insc.clamp1();
        loss.clamp1();
        iValues[tci] = e3dcolor(insc);
        lValues[tci] = e3dcolor(loss);
      }
    }
  }
} bvh_update_atmosphere_job;

static void upload_atmosphere(ContextId context_id)
{
  if (!context_id->atmosphereTexture)
    return;

  if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(context_id->atmosphereTexture.getArrayTex(), 0, 0))
  {
    auto pitch = d3d::get_update_buffer_pitch(rub) / sizeof(uint32_t); // In pixels
    auto texels = (uint32_t *)d3d::get_update_buffer_addr_for_write(rub);
    for (int y = 0; y < Context::atmTexHeight; y++, texels += pitch)
      memcpy(texels, &context_id->atmData.inscatterValues[y * Context::atmTexWidth], Context::atmTexWidth * sizeof(uint32_t));
    d3d::update_texture_and_release_update_buffer(rub);
  }
  if (d3d::ResUpdateBuffer *rub = d3d::allocate_update_buffer_for_tex(context_id->atmosphereTexture.getArrayTex(), 0, 1))
  {
    auto pitch = d3d::get_update_buffer_pitch(rub) / sizeof(uint32_t); // In pixels
    auto texels = (uint32_t *)d3d::get_update_buffer_addr_for_write(rub);
    for (int y = 0; y < Context::atmTexHeight; y++, texels += pitch)
      memcpy(texels, &context_id->atmData.lossValues[y * Context::atmTexWidth], Context::atmTexWidth * sizeof(uint32_t));
    d3d::update_texture_and_release_update_buffer(rub);
  }
}

void bind_resources(ContextId context_id, int render_width)
{
  G_ASSERT(context_id);
  static int bvh_textures_range_startVarId = get_shader_variable_id("bvh_textures_range_start");
  static int bvh_buffers_range_startVarId = get_shader_variable_id("bvh_buffers_range_start");
  static int bvh_metaVarId = get_shader_variable_id("bvh_meta");
  static int bvh_per_instance_dataVarId = get_shader_variable_id("bvh_per_instance_data");
  static int bvh_atmosphere_textureVarId = get_shader_variable_id("bvh_atmosphere_texture");
  static int bvh_atmosphere_texture_distanceVarId = get_shader_variable_id("bvh_atmosphere_texture_distance");
  static int bvh_paint_details_tex_slotVarId = get_shader_variable_id("bvh_paint_details_tex_slot");
  static int bvh_paint_details_smp_slotVarId = get_shader_variable_id("bvh_paint_details_smp_slot");
  static int bvh_land_color_tex_slotVarId = get_shader_variable_id("bvh_land_color_tex_slot", true);
  static int bvh_land_color_smp_slotVarId = get_shader_variable_id("bvh_land_color_smp_slot", true);
  static int bvh_mip_rangeVarId = get_shader_variable_id("bvh_mip_range");
  static int bvh_mip_scaleVarId = get_shader_variable_id("bvh_mip_scale");
  static int bvh_mip_biasVarId = get_shader_variable_id("bvh_mip_bias");
  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
  static int bvh_max_water_distanceVarId = get_shader_variable_id("bvh_max_water_distance");
  static int bvh_water_fade_powerVarId = get_shader_variable_id("bvh_water_fade_power");

  ShaderGlobal::set_tlas(bvh_mainVarId, (context_id->tlasMainValid ? context_id->tlasMain : tlasEmpty).get());
  ShaderGlobal::set_tlas(bvh_terrainVarId, (context_id->tlasTerrainValid ? context_id->tlasTerrain : tlasEmpty).get());
  ShaderGlobal::set_tlas(bvh_particlesVarId, (context_id->tlasParticlesValid ? context_id->tlasParticles : tlasEmpty).get());

  ShaderGlobal::set_int(bvh_textures_range_startVarId, context_id->bindlessTextureAllocator.getHeap().location());
  ShaderGlobal::set_int(bvh_buffers_range_startVarId, context_id->bindlessBufferAllocator.getHeap().location());
  ShaderGlobal::set_buffer(bvh_metaVarId, context_id->meshMeta);
  ShaderGlobal::set_buffer(bvh_per_instance_dataVarId, context_id->perInstanceData);

  ShaderGlobal::set_int(bvh_paint_details_tex_slotVarId, context_id->paintTexBindlessIds >> 16);
  ShaderGlobal::set_int(bvh_paint_details_smp_slotVarId, context_id->paintTexBindlessIds & 0xFFFF);

  ShaderGlobal::set_int(bvh_land_color_tex_slotVarId, context_id->landColorTexBindlessIds >> 16);
  ShaderGlobal::set_int(bvh_land_color_smp_slotVarId, context_id->landColorTexBindlessIds & 0xFFFF);

  ShaderGlobal::set_texture(bvh_atmosphere_textureVarId, context_id->atmosphereTexture);
  ShaderGlobal::set_real(bvh_atmosphere_texture_distanceVarId, Context::atmMaxDistance);

  ShaderGlobal::set_real(bvh_mip_rangeVarId, mip_range);
  ShaderGlobal::set_real(bvh_mip_scaleVarId, mip_scale);
  ShaderGlobal::set_real(bvh_mip_biasVarId, max(log2f(3840.0f / render_width), 0.0f));
  ShaderGlobal::set_real(bvh_max_water_distanceVarId, max_water_distance);
  ShaderGlobal::set_real(bvh_water_fade_powerVarId, water_fade_power);
}

void set_for_gpu_objects(ContextId context_id) { gobj::init(context_id); }

void set_ri_extra_range(ContextId context_id, float range) { context_id->riExtraRange = range; }

void prepare_ri_extra_instances() { ri::prepare_ri_extra_instances(); }

void on_before_unload_scene(ContextId context_id)
{
  context_id->releasePaintTex();
  context_id->releaseLandColorTex();
}

void on_load_scene(ContextId context_id) { context_id->clearDeathrow(); }

void on_scene_loaded(ContextId context_id) { context_id->clearDeathrow(); }

void on_unload_scene(ContextId context_id)
{
  dyn::on_unload_scene(context_id);
  ri::on_unload_scene(context_id);
  grass::on_unload_scene(context_id);
  gobj::on_unload_scene(context_id);
  fx::on_unload_scene(context_id);
  for (auto &instances : context_id->impostorInstances)
  {
    instances.clear();
    instances.shrink_to_fit();
  }
  for (auto &instances : context_id->riExtraInstances)
  {
    instances.clear();
    instances.shrink_to_fit();
  }
  for (auto &data : context_id->riExtraInstanceData)
  {
    data.clear();
    data.shrink_to_fit();
  }
  for (auto &data : context_id->impostorInstanceData)
  {
    data.clear();
    data.shrink_to_fit();
  }
  for (auto &texture : context_id->camoTextures)
    context_id->releaseTexure(TEXTUREID(texture.first));
  context_id->camoTextures.clear();

  context_id->atmosphereDirty = true;

  for (auto &alloc : context_id->processBufferAllocators)
    alloc.second.reset();

  context_id->clearDeathrow();
}

void reload_grass(ContextId context_id, RandomGrass *grass) { grass::reload_grass(context_id, grass); }

void ChannelParser::enum_shader_channel(int usage, int usage_index, int type, int vb_usage, int vb_usage_index, ChannelModifier mod,
  int stream)
{
  G_UNUSED(usage);
  G_UNUSED(usage_index);
  G_UNUSED(mod);
  G_UNUSED(stream);

#define ATTRIB(name, prefix, exp_usage, exp_usage_index)                      \
  if (name##Format == -1)                                                     \
  {                                                                           \
    if (prefix##usage == exp_usage && prefix##usage_index == exp_usage_index) \
      name##Format = type;                                                    \
    else                                                                      \
    {                                                                         \
      unsigned channelSize;                                                   \
      channel_size(type, channelSize);                                        \
      name##Offset += channelSize;                                            \
    }                                                                         \
  }

  ATTRIB(position, vb_, SCUSAGE_POS, 0);
  ATTRIB(texcoord, vb_, SCUSAGE_TC, 0);
  ATTRIB(secTexcoord, vb_, SCUSAGE_TC, 1);
  ATTRIB(thirdTexcoord, vb_, SCUSAGE_TC, 2);
  ATTRIB(color, vb_, SCUSAGE_VCOL, 0);
  ATTRIB(normal, vb_, SCUSAGE_NORM, 0);
  ATTRIB(indices, , SCUSAGE_EXTRA, 0);
  ATTRIB(weights, , SCUSAGE_EXTRA, 1);

  if (texcoordFormat == VSDT_SHORT2)
    texcoordFormat = BufferProcessor::bvhAttributeShort2TC;
#undef ATTRIB
}

void connect_fx(ContextId context_id, fx_connect_callback callback)
{
  if (context_id->has(Features::Fx))
    fx::connect(callback);
}

void on_cables_changed(Cables *cables, ContextId context_id) { cables::on_cables_changed(cables, context_id); }

bool is_building(ContextId context_id)
{
  return !context_id->halfBakedMeshes.empty() || context_id->hasPendingMeshActions.load(std::memory_order_relaxed);
}

void set_grass_range(ContextId context_id, float range) { context_id->grassRange = range; }

void start_async_atmosphere_update(ContextId context_id, const Point3 &view_pos, atmosphere_callback callback)
{
  if (!context_id->atmosphereTexture)
  {
    context_id->atmosphereTexture =
      dag::create_array_tex(Context::atmTexWidth, Context::atmTexHeight, 2, TEXCF_DYNAMIC, 1, "bvh_atmosphere_tex");
    HANDLE_LOST_DEVICE_STATE(context_id->atmosphereTexture, );
    context_id->atmosphereTexture->texaddru(TEXADDR_WRAP);
    context_id->atmosphereTexture->texaddrv(TEXADDR_CLAMP);
    context_id->atmosphereDirty = true;
  }

  if (context_id->atmosphereDirty)
    context_id->atmosphereCursor = Context::atmTexHeight - 1; // So it will be uploaded in the next frame

  bvh_update_atmosphere_job.contextId = context_id;
  bvh_update_atmosphere_job.callback = callback;
  bvh_update_atmosphere_job.viewPos.x = view_pos.x;
  bvh_update_atmosphere_job.viewPos.y = 1;
  bvh_update_atmosphere_job.viewPos.z = view_pos.z;
  bvh_update_atmosphere_job.lineBegin = context_id->atmosphereDirty ? 0 : context_id->atmosphereCursor;
  bvh_update_atmosphere_job.lineEnd = context_id->atmosphereDirty ? Context::atmTexHeight : context_id->atmosphereCursor + 1;
  threadpool::add(&bvh_update_atmosphere_job, threadpool::PRIO_NORMAL);

  context_id->atmosphereDirty = false;
  context_id->atmosphereCursor = (context_id->atmosphereCursor + 1) % Context::atmTexHeight;
}

void finalize_async_atmosphere_update(ContextId context_id)
{
  threadpool::wait(&bvh_update_atmosphere_job);

  // The cursor being zero means that during the previous frame, the last line was updated. So we need to upload.
  if (context_id->atmosphereCursor == 0)
  {
    upload_atmosphere(context_id);
  }
}
void enable_per_frame_processing(bool enable) { per_frame_processing_enabled = enable; }

} // namespace bvh