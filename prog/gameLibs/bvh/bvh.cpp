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
#include <shaders/dag_shaderBlock.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <render/denoiser.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <perfMon/dag_statDrv.h>
#include <generic/dag_enumerate.h>
#include <generic/dag_zip.h>
#include <gui/dag_stdGuiRender.h>
#include <math/dag_math3d.h>
#include <math/dag_dxmath.h>
#include <math/dag_half.h>
#include <util/dag_threadPool.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/numeric.h>
#include <gui/dag_visualLog.h>
#include <userSystemInfo/systemInfo.h>

#include "bvh_context.h"
#include "bvh_debug.h"
#include "bvh_add_instance.h"
#include "shaders/dag_shaderVar.h"

static constexpr int per_frame_blas_model_budget[3] = {10, 15, 30};
static constexpr int per_frame_compaction_budget[3] = {20, 30, 60};

static constexpr float animation_distance_rate = 20;

static constexpr bool showProceduralBLASBuildCount = false;

static bool per_frame_processing_enabled = true;

#if DAGOR_DBGLEVEL > 0
extern bool bvh_gpuobject_enable;
extern bool bvh_grass_enable;
extern bool bvh_particles_enable;
extern bool bvh_cables_enable;
#endif

#if BVH_PROFILING_ENABLED
#define BVH_PROFILE TIME_PROFILE
#else
#define BVH_PROFILE(...)
#endif
namespace bvh
{

namespace terrain
{
dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2, bool>> get_blases(ContextId context_id);

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
void on_scene_loaded(ContextId context_id);
void on_unload_scene(ContextId context_id);
void prepare_ri_extra_instances();
// instances in view_frustum in ri_gen_visibilities[1] will be discarded!
void update_ri_gen_instances(ContextId context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  const Point3 &view_position, const Frustum &view_frustum, threadpool::JobPriority prio);
void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &bvh_frustum,
  const Frustum &view_frustum, threadpool::JobPriority prio);
void wait_ri_gen_instances_update(ContextId context_id);
void wait_ri_extra_instances_update(ContextId context_id, bool do_work);
void cut_down_trees(ContextId context_id);
void wait_cut_down_trees();
void set_dist_mul(float mul);
void override_out_of_camera_ri_dist_mul(float dist_sq_mul_ooc);
} // namespace ri

namespace dyn
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position);
void wait_dynrend_instances();
void update_animchar_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position, dynrend::BVHIterateCallback iterate_callback);
void set_up_dynrend_context_for_processing(dynrend::ContextId dynrend_context_id);
void purge_skin_buffers(ContextId context_id);
void wait_purge_skin_buffers();
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
void render_debug_context(ContextId context_id, float min_t);
} // namespace debug

bool is_in_lost_device_state = false;

elem_rules_fn elem_rules = nullptr;

dag::AtomicInteger<uint32_t> bvh_id_gen = 0;

float mip_range = 1000;
float mip_scale = 10;

float max_water_distance = 10.0f;
float water_fade_power = 3.0f;
float max_water_depth = 5.0f;
float rtr_max_water_depth = 0.2f;

int ri_thread_count_ofset = 0;

float debug_min_t = 0;

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

struct CreateCompactedBLASJob : public cpujobs::IJob
{
  Context::BLASCompaction *compaction = nullptr;

  static void start(Context::BLASCompaction *compaction)
  {
    auto job = new CreateCompactedBLASJob;
    job->compaction = compaction;
    compaction->blasCreateJob = job;
    threadpool::add(job, threadpool::PRIO_LOW, false);
  }

  const char *getJobName(bool &) const override { return "CreateCompactedBLASJob"; }
  void doJob() override { compaction->compactedBlas = UniqueBLAS::create(compaction->compactedSizeValue); }
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
  if (d3d::is_stub_driver())
    return true;

  auto gfx = dgs_get_settings()->getBlockByNameEx("graphics");
  auto memoryRequiredKB = gfx->getInt("bvhMinRequiredMemoryGB", 5) * 1100 * 1000;        // intentionally not 1024
  auto memoryRequiredUMAKB = gfx->getInt("bvhMinRequiredMemoryGBUMA", 15) * 1100 * 1000; // intentionally not 1024

  auto &desc = d3d::get_driver_desc();
  if (desc.caps.hasRayQuery && desc.info.isUMA)
  {
    // For UMA GPUs we check the system memory instead of the VRAM.

    logdbg("BVH memory check for UMA GPU...");

    int sysMemKB;
    if (systeminfo::get_physical_memory(sysMemKB))
    {
      sysMemKB *= 1024;
      bool enough = sysMemKB > memoryRequiredUMAKB;
      logdbg("BVH memory check found %dKB of system memory which is%s enough.", sysMemKB, enough ? "" : " not");
      return enough;
    }
    else
      logerr("BVH memory check failed to get the amount of system memory!");
  }

  return (d3d::driver_command(Drv3dCommand::GET_VIDEO_MEMORY_BUDGET) >= memoryRequiredKB);
}

static bool has_enough_vram_for_rt_initial_check()
{
  bool ret = has_enough_vram_for_rt();
  logdbg("bvh: VRAM check %s", ret ? "OK" : "FAILED");
  return ret;
}

inline bool logonce(const char *msg)
{
  static bool logged = false;
  if (!logged)
  {
    logdbg(msg);
    logged = true;
  }
  return false;
}

bool is_available()
{
  if (dgs_get_settings()->getBlockByNameEx("gameplay")->getBool("enableVR", false))
  {
    logonce("bvh::is_available is failed because VR is enabled.");
    return false;
  }
  if (!d3d::get_driver_desc().caps.hasRayQuery)
  {
    logonce("bvh::is_available is failed because of no ray query support.");
    return false;
  }
  if (!d3d::get_driver_desc().caps.hasBindless)
  {
    logonce("bvh::is_available is failed because of no bindless support.");
    return false;
  }
  if (!denoiser::is_available())
  {
    logonce("bvh::is_available is failed because of no denoiser support.");
    return false;
  }
#if _TARGET_SCARLETT || _TARGET_C2
  static bool hasEnoughVRAM = true;
#elif _TARGET_PC
  if (!d3d::is_inited())
    return true;
  static bool hasEnoughVRAM = has_enough_vram_for_rt_initial_check();
#else
  static bool hasEnoughVRAM = false;
#endif
  if (hasEnoughVRAM)
    logonce("bvh::is_available is success.");
  else
    logonce("bvh::is_available is failed because of not enough VRAM.");

  return hasEnoughVRAM;
}

void init(elem_rules_fn elem_rules_init)
{
  elem_rules = elem_rules_init;

  scratch_buffer_manager.alignment = d3d::get_driver_desc().raytrace.accelerationStructureBuildScratchBufferOffsetAlignment;

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

static void unbind_tlases()
{
#if !_TARGET_C2
  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
  ShaderGlobal::set_tlas(bvh_mainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, nullptr);
#endif

  static int bvh_main_validVarId = get_shader_variable_id("bvh_main_valid");
  static int bvh_terrain_validVarId = get_shader_variable_id("bvh_terrain_valid");
  static int bvh_particles_validVarId = get_shader_variable_id("bvh_particles_valid");
  ShaderGlobal::set_int(bvh_main_validVarId, 0);
  ShaderGlobal::set_int(bvh_terrain_validVarId, 0);
  ShaderGlobal::set_int(bvh_particles_validVarId, 0);
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

  for (auto &mesh : context_id->objects)
    mesh.second.teardown(context_id);
  for (auto &mesh : context_id->impostors)
    mesh.second.teardown(context_id);

  G_ASSERT(context_id->usedTextures.empty());
  G_ASSERT(context_id->usedBuffers.empty());

  delete context_id;

  context_id = InvalidContextId;

  unbind_tlases();
}

void start_frame()
{
  scratch_buffer_manager.startNewFrame();
  transform_buffer_manager.startNewFrame();
}

void add_instance(ContextId context_id, uint64_t object_id, mat43f_cref transform)
{
  add_instance(context_id, context_id->genericInstances, object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    MeshMetaAllocator::INVALID_ALLOC_ID);
}

void update_instances_impl(ContextId bvh_context_id, const Point3 &view_position, const Frustum &bvh_frustum,
  const Frustum &view_frustum, dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id,
  const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate, threadpool::JobPriority prio)
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

  if (!dynrend_iterate)
    ri_thread_count_ofset = -1;
  else
    ri_thread_count_ofset = 0;

  if (bvh_context_id->has(Features::DynrendRigidFull | Features::DynrendRigidBaked | Features::DynrendSkinnedFull))
    if (!dynrend_iterate)
      dyn::update_dynrend_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position);

  if (bvh_context_id->has(Features::RIFull | Features::RIBaked))
  {
    ri::wait_cut_down_trees();
    ri::update_ri_gen_instances(bvh_context_id, ri_gen_visibilities, view_position, view_frustum, prio);
    ri::update_ri_extra_instances(bvh_context_id, view_position, bvh_frustum, view_frustum, prio);
  }

  if (bvh_context_id->has(Features::DynrendRigidFull | Features::DynrendRigidBaked | Features::DynrendSkinnedFull))
  {
    dyn::wait_purge_skin_buffers();
    if (dynrend_iterate)
      dyn::update_animchar_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position,
        dynrend_iterate);
  }
}

void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Frustum &bvh_frustum, const Frustum &view_frustum,
  dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id, RiGenVisibility *ri_gen_visibility,
  threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, bvh_frustum, view_frustum, dynrend_context_id, dynrend_no_shadow_context_id,
    {ri_gen_visibility}, nullptr, prio);
}

// daNetGame doesn't use dynrend, but these can be used to identify contexts in BVH
static dynrend::ContextId bvh_dynrend_context = dynrend::ContextId{-1};
static dynrend::ContextId bvh_dynrend_no_shadow_context = dynrend::ContextId{-2};
void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Frustum &bvh_frustum, const Frustum &view_frustum,
  const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate, threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, bvh_frustum, view_frustum, &bvh_dynrend_context, &bvh_dynrend_no_shadow_context,
    ri_gen_visibilities, dynrend_iterate, prio);
}

static BufferProcessor::ProcessArgs build_args(uint64_t object_id, const Mesh &mesh, const Context::Instance *instance, bool recycled)
{
  BufferProcessor::ProcessArgs args{};
  args.objectId = object_id;
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

static void process_vertices(ContextId context_id, uint64_t object_id, Mesh &mesh, const Context::Instance *instance,
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

      if (instance && instance->animationUpdateMode == Context::Instance::AnimationUpdateMode::FORCE_ON)
      {
        needProcessing = true;
        context_id->animatedInstanceCount++;
      }
      else if (instance && frustum && instance->animationUpdateMode == Context::Instance::AnimationUpdateMode::DO_CULLING)
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

    auto args = build_args(object_id, mesh, instance, processed_vertices_recycled);

    bool canProcess = mesh.vertexProcessor->isReady(args);

    // Only do the processing if we either has a per instance output to process into, or the
    // initial processing on the mesh is not yet done. Otherwise it would just process the same
    // mesh again and again for no reason.
    if (canProcess && (transformed_vertices.needAllocation() || (instance && instance->uniqueTransformedBuffer)))
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
        meta.texcoordOffset = args.texcoordOffset;
        meta.normalOffset = args.normalOffset;
        meta.colorOffset = args.colorOffset;
        meta.vertexStride = args.vertexStride;

        base_meta.setTexcoordFormat(args.texcoordFormat);
        base_meta.startVertex = meta.startVertex;
        base_meta.texcoordOffset = meta.texcoordOffset;
        base_meta.normalOffset = meta.normalOffset;
        base_meta.colorOffset = meta.colorOffset;
        base_meta.vertexStride = meta.vertexStride;
      }

      if (!hadProcessedVertices && transformed_vertices.isAllocated())
      {
        uint32_t bindlessIndex;
        context_id->holdBuffer(transformed_vertices.get(), bindlessIndex);
        if (instance && instance->uniqueTransformedBuffer)
        {
          meta.setVertexBufferIndex(bindlessIndex);
        }
        else
          mesh.pvBindlessIndex = bindlessIndex;
      }

      if (!instance || !instance->uniqueTransformedBuffer)
      {
        meta.setVertexBufferIndex(mesh.pvBindlessIndex);
      }
    }
  }
}

static void process_ahs_vertices(ContextId context_id, Mesh &mesh, MeshMeta &meta)
{
  if (mesh.materialType & MeshMeta::bvhMaterialAlphaTest)
  {
    G_ASSERT(mesh.indexFormat);
    G_ASSERT(mesh.indexCount);

    if (meta.materialType & MeshMeta::bvhMaterialImpostor)
    {
      int8_t texcoordOffset = meta.texcoordOffset;
      int8_t colorOffset = meta.colorOffset;
      int8_t vertexStride = meta.vertexStride;

      G_ASSERT(texcoordOffset > -1);
      G_ASSERT(colorOffset > -1);
      G_ASSERT(vertexStride > -1);

      ProcessorInstances::getAHSProcessor().process(mesh.processedIndices.get(), mesh.processedVertices.get(), mesh.ahsVertices,
        mesh.indexFormat, mesh.indexCount, texcoordOffset, VSDT_FLOAT2, vertexStride, colorOffset);
    }
    else
    {
      G_ASSERT(mesh.vertexStride);
      G_ASSERT(mesh.texcoordOffset != MeshInfo::invalidOffset);
      G_ASSERT(mesh.texcoordFormat);

      ProcessorInstances::getAHSProcessor().process(mesh.processedIndices.get(), mesh.processedVertices.get(), mesh.ahsVertices,
        mesh.indexFormat, mesh.indexCount, mesh.texcoordOffset, mesh.texcoordFormat, mesh.vertexStride, -1);
    }

    uint32_t bindlessIndex;
    context_id->holdBuffer(mesh.ahsVertices.get(), bindlessIndex);

    meta.indexCount = mesh.indexCount;
    meta.setAhsVertexBufferIndex(bindlessIndex);
    if (bindlessIndex > BVH_BINDLESS_BUFFER_MAX)
      logerr("BVH vertex buffer bindless index out of range: %u", bindlessIndex);
    if (mesh.indexCount > 0xFFFFU)
      logerr("BVH vertex buffer index count out of range: %u", mesh.indexCount);

    // If the alpha comes from the alpha texture, we set a bit on the uppermost bit, signaling that the shader
    // should use the R channel from the alpha texture. Otherwise it will use the A channel.
    if (mesh.alphaTextureId != BAD_TEXTUREID)
      meta.materialType |= MeshMeta::bvhMaterialAlphaInRed;
  }
}

static int do_add_object(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info)
{
  auto &objectMap = (object_info.meshes.size() && object_info.meshes[0].isImpostor) ? context_id->impostors : context_id->objects;

#if DAGOR_DBGLEVEL > 0
  for (const auto &mesh : object_info.meshes)
    G_ASSERTF_RETURN(mesh.indices, 0,
      "Not having indices is commented out from the shader code for performance. If this is really needed, lets discuss it.");
#endif

  // If a mesh has a BLAS, it is either non animated or it has it's vertex/index buffers already
  // copied/processed into its own buffer. So we don't care about any of the changes.
  auto iter = objectMap.find(object_id);
  if (iter != objectMap.end())
  {
    if (iter->second.blas)
      return 0;

    iter->second.teardown(context_id);
    context_id->cancelCompaction(object_id);
    context_id->halfBakedObjects.erase(object_id);
  }

  auto &object = (iter != objectMap.end()) ? iter->second : objectMap[object_id];
  object.meshes = decltype(object.meshes)(object_info.meshes.size());
  object.isAnimated = object_info.isAnimated;
  auto metaRegion = object.createAndGetMeta(context_id, object_info.meshes.size());
  for (auto [info, mesh, meta] : zip(object_info.meshes, object.meshes, metaRegion))
  {
    mesh.albedoTextureId = info.albedoTextureId;
    mesh.alphaTextureId = info.alphaTextureId;
    mesh.normalTextureId = info.normalTextureId;
    mesh.extraTextureId = info.extraTextureId;
    mesh.indexCount = info.indexCount;
    mesh.indexFormat = info.indices->getFlags() & SBCF_INDEX32 ? 4 : 2;
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
    {
      mesh.materialType |= MeshMeta::bvhMaterialPainted;
      if (info.paintData.y >= 1.0001f)
        mesh.isPaintedHeightLocked = true;
    }

    if (info.useAtlas)
      mesh.materialType |= MeshMeta::bvhMaterialAtlas;

    if (info.isCamo)
      mesh.materialType |= MeshMeta::bvhMaterialCamo;

    if (info.isMFD)
      mesh.materialType |= MeshMeta::bvhMaterialMFD;

    if (info.isLayered)
      mesh.materialType |= MeshMeta::bvhMaterialLayered;

    if (info.isEmissive)
      mesh.materialType |= MeshMeta::bvhMaterialEmissive;

    if (info.isPerlinLayered)
      mesh.materialType |= MeshMeta::bvhMaterialPerlinLayered;

    if (info.isEye)
      mesh.materialType |= MeshMeta::bvhMaterialEye;

    meta.markInitialized();

    meta.materialType = mesh.materialType;
    meta.setIndexBitAndTexcoordFormat(mesh.indexFormat, info.texcoordFormat);
    meta.texcoordOffset = info.texcoordOffset;
    meta.normalOffset = info.normalOffset;
    meta.colorOffset = info.colorOffset;
    meta.vertexStride = info.vertexSize;
    meta.startIndex = info.startIndex;
    meta.startVertex = info.baseVertex;
    meta.texcoordScale = info.texcoordScale;
    meta.setIndexBufferIndex(0);
    meta.setVertexBufferIndex(0);

    meta.holdAlphaTex(context_id, mesh.alphaTextureId);
    meta.holdNormalTex(context_id, mesh.normalTextureId);
    meta.holdExtraTex(context_id, mesh.extraTextureId);
    auto albedoTexture = meta.holdAlbedoTex(context_id, mesh.albedoTextureId);

    if (info.albedoTextureId != BAD_TEXTUREID)
    {
      TextureInfo ti;
      albedoTexture->getinfo(ti);

      mesh.albedoTextureLevel = D3dResManagerData::getLevDesc(info.albedoTextureId.index(), TQL_thumb);
      mark_managed_tex_lfu(info.albedoTextureId, mesh.albedoTextureLevel);
    }

    if (mesh.materialType & MeshMeta::bvhMaterialAlphaTest && info.alphaTextureId == BAD_TEXTUREID)
    {
      // If we need alpha testing, lets set the albedo texture to the alpha texture.
      meta.alphaTextureIndex = meta.albedoTextureIndex;
      meta.alphaSamplerIndex = meta.albedoAndNormalSamplerIndex;
    }

    if (info.painted || info.isEmissive)
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

    if (info.isPerlinLayered)
    {
      meta.layerData[0] = info.detailsData1;
      meta.layerData[1] = info.detailsData2;
      meta.layerData[2] = info.detailsData3;
      meta.layerData[3] = info.detailsData4;

      meta.materialData1 = info.paintData;
      meta.materialData2 = info.colorOverride;

      meta.atlasTileSize = uint32_t(info.atlasTileU);
      meta.atlasFirstLastTile = uint32_t(info.atlasTileV);

      // Mask texture is extraTextureAndSamplerIndex
      // tile1diffuse is alphaTextureAndSamplerIndex
      // tile2diffuse is normalTextureAndSamplerIndex
    }

    {
      // Always process indices to be independent from the streaming system.

      G_ASSERT(mesh.indexFormat == 2);

      {
        TIME_PROFILE(index_buffer_alloc)
        static int counter = 0;
        eastl::string name(eastl::string::CtorSprintf{}, "bvh_indices_%u", ++counter);
        auto dwordCount = (info.indexCount + 1) / 2;
        mesh.processedIndices.buffer.reset(d3d::buffers::create_persistent_sr_byte_address(dwordCount, name.c_str()));
      };
      mesh.processedIndices.offset = 0;

      HANDLE_LOST_DEVICE_STATE(mesh.processedIndices.buffer, 1);

      ProcessorInstances::getIndexProcessor().process(info.indices, mesh.processedIndices, mesh.indexFormat, mesh.indexCount,
        mesh.startIndex, mesh.startVertex);
      G_ASSERT(mesh.processedIndices);

      mesh.startIndex = meta.startIndex = 0;
      meta.setIndexBit(mesh.indexFormat);

      uint32_t bindlessIndex;
      context_id->holdBuffer(mesh.processedIndices.get(), bindlessIndex);
      mesh.piBindlessIndex = bindlessIndex;

      meta.setIndexBufferIndex(bindlessIndex);

      d3d::resource_barrier(ResourceBarrierDesc(mesh.processedIndices.get(), bindlessSRVBarrier));
    }

    {
      // Always copy the vertices to be independent from the streaming system.

      {
        TIME_PROFILE(vertex_buffer_alloc)
        static int nameGen = 0;
        int bufferSize = mesh.vertexStride * mesh.vertexCount;
        int dwordCount = (bufferSize + 3) / 4;
        eastl::string name(eastl::string::CtorSprintf{}, "bvh_vb_%d", nameGen++);
        mesh.processedVertices.buffer.reset(d3d::buffers::create_persistent_sr_byte_address(dwordCount, name.c_str()));
      }
      mesh.processedVertices.offset = 0;

      HANDLE_LOST_DEVICE_STATE(mesh.processedVertices.buffer, 1);

      info.vertices->copyTo(mesh.processedVertices.get(), 0, mesh.vertexStride * (mesh.baseVertex + mesh.startVertex),
        mesh.vertexStride * mesh.vertexCount);
      mesh.startVertex = 0;
      mesh.baseVertex = 0;
      meta.startVertex = 0;

      uint32_t bindlessIndex;
      context_id->holdBuffer(mesh.processedVertices.get(), bindlessIndex);
      mesh.pvBindlessIndex = bindlessIndex;

      meta.setVertexBufferIndex(bindlessIndex);

      d3d::resource_barrier(ResourceBarrierDesc(mesh.processedVertices.get(), bindlessSRVBarrier));
    }

    if (!info.isImpostor)
      process_ahs_vertices(context_id, mesh, meta);
  }

  if (!object_info.isAnimated)
    context_id->halfBakedObjects.insert(object_id);

  return 1;
}

void add_object(ContextId context_id, uint64_t object_id, const ObjectInfo &info)
{
#if DAGOR_DBGLEVEL > 0
  for (const auto mesh : info.meshes)
  {
    G_ASSERT(mesh.vertices);
    G_ASSERT(!mesh.indices || mesh.indexCount % 3 == 0);
  }
#endif

  context_id->hasPendingMeshAddActions.store(true, dag::memory_order_relaxed);
  OSSpinlockScopedLock lock(context_id->pendingMeshAddActionsLock);
  if (auto iter = context_id->pendingObjectAddActions.find(object_id); iter != context_id->pendingObjectAddActions.end())
    iter->second = info;
  else
    context_id->pendingObjectAddActions.insert({object_id, eastl::move(info)});
}

void remove_object(ContextId context_id, uint64_t object_id)
{
  OSSpinlockScopedLock lock(context_id->pendingMeshRemoveActionsLock);
  context_id->pendingObjectRemoveActions.insert(object_id);
}

static void handle_pending_mesh_actions(ContextId context_id, BuildBudget budget)
{
  TIME_PROFILE(handle_pending_mesh_actions);

  OSSpinlockScopedLock lock(context_id->pendingMeshAddActionsLock);

  {
    OSSpinlockScopedLock removeLock(context_id->pendingMeshRemoveActionsLock);

    DA_PROFILE_TAG(handle_pending_mesh_actions, String(32, "%u removal in queue", context_id->pendingObjectRemoveActions.size()));

    for (auto object_id : context_id->pendingObjectRemoveActions)
    {
      TIME_PROFILE(bvh::do_remove_object);

      context_id->cancelCompaction(object_id);

      auto iter = context_id->objects.find(object_id);
      if (iter != context_id->objects.end())
      {
        DA_PROFILE_TAG(bvh::do_remove_object, "object");
        iter->second.teardown(context_id);
        context_id->objects.erase(iter);
      }
      else
      {
        iter = context_id->impostors.find(object_id);
        if (iter != context_id->impostors.end())
        {
          DA_PROFILE_TAG(bvh::do_remove_object, "impostor");
          iter->second.teardown(context_id);
          context_id->impostors.erase(iter);
        }
      }

      context_id->halfBakedObjects.erase(object_id);
      context_id->pendingObjectAddActions.erase(object_id);
    }

    context_id->pendingObjectRemoveActions.clear();
  }

  DA_PROFILE_TAG(handle_pending_mesh_actions, String(32, "%u additions in queue", context_id->pendingObjectAddActions.size()));

  int meshBudget = per_frame_blas_model_budget[int(budget)];

  int counter = 0;
  for (auto iter = context_id->pendingObjectAddActions.begin();
       iter != context_id->pendingObjectAddActions.end() && counter < meshBudget && !is_in_lost_device_state;)
  {
    TIME_PROFILE(handle_pending_mesh_action);

    // When the mesh is already added, but not yet built, we need to remove it as the build data is not valid anymore.

    context_id->halfBakedObjects.erase(iter->first);
    counter += do_add_object(context_id, iter->first, iter->second);
    iter = context_id->pendingObjectAddActions.erase(iter);
  }

  context_id->hasPendingMeshAddActions.store(!context_id->pendingObjectAddActions.empty(), dag::memory_order_relaxed);
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

  handle_pending_mesh_actions(context_id, budget);

  const bool isCompactionCheap = is_blas_compaction_cheap();

  int blasModelBudget = per_frame_blas_model_budget[int(budget)];
  int triangleCount = 0;
  int blasCount = 0;

  dag::Vector<RaytraceGeometryDescription> geomDescriptors;
  dag::Vector<raytrace::BatchedBottomAccelerationStructureBuildInfo> blasBuildInfos;

  const uint32_t MAX_GEOMETRIES_PER_BLAS = 32;
  geomDescriptors.resize(MAX_GEOMETRIES_PER_BLAS * blasModelBudget);
  blasBuildInfos.reserve(blasModelBudget);

  auto getBlas = [&](Context::BLASCompaction &c) -> UniqueBLAS * {
    auto mesh = context_id->objects.find(c.objectId);
    if (mesh == context_id->objects.end())
    {
      mesh = context_id->impostors.find(c.objectId);
      if (mesh == context_id->impostors.end())
        return nullptr;
    }

    return &mesh->second.blas;
  };

  dag::Vector<Context::BLASCompaction *, framemem_allocator> pendingCompactions;

  for (auto iter = context_id->halfBakedObjects.begin();
       iter != context_id->halfBakedObjects.end() && blasModelBudget > 0 && !is_in_lost_device_state;)
  {
    TIME_PROFILE(half_baked_mesh);

    uint64_t objectId = *iter;
    bool hasObject = false;
    auto objectIter = context_id->objects.find(objectId);
    if (objectIter != context_id->objects.end())
      hasObject = true;
    else
    {
      objectIter = context_id->impostors.find(objectId);
      if (objectIter != context_id->impostors.end())
        hasObject = true;
    }

    G_ASSERT(hasObject);

    Object &object = objectIter->second;
    size_t descCount = eastl::accumulate(object.meshes.begin(), object.meshes.end(), 0, [](size_t count, const auto &mesh) {
      return count + (mesh.vertexProcessor && mesh.vertexProcessor->isGeneratingSecondaryVertices() ? 2 : 1);
    });

    RaytraceGeometryDescription *desc = &geomDescriptors[MAX_GEOMETRIES_PER_BLAS * blasCount];
    memset(desc, 0, sizeof(RaytraceGeometryDescription) * descCount);
    auto metaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);
    for (int i = 0; auto &mesh : object.meshes)
    {
      auto &meta = metaRegion[i];

      bool needBlasBuild = false;
      UniqueBVHBuffer transformedVertices;
      UniqueOrReferencedBVHBuffer pb(transformedVertices);
      process_vertices(context_id, objectId, mesh, nullptr, nullptr, V_C_ONE, V_C_ONE, pb, false, needBlasBuild, meta, meta, false);

      CHECK_LOST_DEVICE_STATE();

      G_ASSERT(!mesh.vertexProcessor || mesh.vertexProcessor->isOneTimeOnly());
      const bool hasSecondaryGeometry = mesh.vertexProcessor && mesh.vertexProcessor->isGeneratingSecondaryVertices();
      mesh.vertexProcessor = nullptr;

      if (transformedVertices)
      {
        context_id->releaseBuffer(mesh.processedVertices.get());
        mesh.processedVertices.buffer.swap(transformedVertices);
        mesh.processedVertices.offset = 0;
        transformedVertices.reset();
      }

      if (meta.materialType & MeshMeta::bvhMaterialImpostor)
        process_ahs_vertices(context_id, mesh, meta);

      bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;

      desc[i].type = RaytraceGeometryDescription::Type::TRIANGLES;
      desc[i].data.triangles.vertexBuffer = mesh.processedVertices.get();
      desc[i].data.triangles.indexBuffer = mesh.processedIndices.get();
      desc[i].data.triangles.vertexCount = mesh.vertexCount;
      desc[i].data.triangles.vertexStride = meta.vertexStride;
      desc[i].data.triangles.vertexOffset = mesh.baseVertex;
      desc[i].data.triangles.vertexOffsetExtraBytes = mesh.positionOffset;
      desc[i].data.triangles.vertexFormat = mesh.processedPositionFormat;
      desc[i].data.triangles.indexCount = mesh.indexCount;
      desc[i].data.triangles.indexOffset = meta.startIndex;
      desc[i].data.triangles.flags =
        (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;

      if (mesh.posMul != Point4::ONE || mesh.posAdd != Point4::ZERO)
      {
        float m[12];
#if _TARGET_APPLE
        m[0] = mesh.posMul.x;
        m[3] = 0;
        m[6] = 0;
        m[9] = mesh.posAdd.x;

        m[1] = 0;
        m[4] = mesh.posMul.y;
        m[7] = 0;
        m[10] = mesh.posAdd.y;

        m[2] = 0;
        m[5] = 0;
        m[8] = mesh.posMul.z;
        m[11] = mesh.posAdd.z;
#else
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
#endif

        desc[i].data.triangles.transformBuffer = transform_buffer_manager.alloc(1, desc[i].data.triangles.transformOffset); //-V522
        HANDLE_LOST_DEVICE_STATE(desc[i].data.triangles.transformBuffer, );                                                 //-V522
        desc[i].data.triangles.transformBuffer->updateDataWithLock(sizeof(m) * desc[i].data.triangles.transformOffset,      //-V522
          sizeof(m),                                                                                                        //-V522
          m, 0);
      }

      if (hasSecondaryGeometry)
      {
        desc[i + 1].type = RaytraceGeometryDescription::Type::TRIANGLES;
        desc[i + 1].data.triangles.transformBuffer = desc[i].data.triangles.transformBuffer;
        desc[i + 1].data.triangles.transformOffset = desc[i].data.triangles.transformOffset;
        desc[i + 1].data.triangles.vertexBuffer = mesh.processedVertices.get();
        desc[i + 1].data.triangles.indexBuffer = mesh.processedIndices.get();
        desc[i + 1].data.triangles.vertexCount = mesh.vertexCount;
        desc[i + 1].data.triangles.vertexOffset = mesh.vertexCount;
        desc[i + 1].data.triangles.vertexStride = meta.vertexStride;
        desc[i + 1].data.triangles.vertexFormat = mesh.processedPositionFormat;
        desc[i + 1].data.triangles.indexCount = mesh.indexCount;
        desc[i + 1].data.triangles.indexOffset = meta.startIndex;
        desc[i + 1].data.triangles.flags =
          (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
      }

      triangleCount += mesh.indexCount / 3;

      i += hasSecondaryGeometry ? 2 : 1;
    }

    raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
    buildInfo.flags = RaytraceBuildFlags::FAST_TRACE;

    Context::BLASCompaction *compaction = context_id->beginBLASCompaction(objectId);
    if (compaction)
    {
      buildInfo.flags |= RaytraceBuildFlags::ALLOW_COMPACTION;
      buildInfo.compactedSizeOutputBuffer = compaction->compactedSize.get();
      pendingCompactions.push_back(compaction);
    }

    object.blas = UniqueBLAS::create(desc, descCount, buildInfo.flags);
    HANDLE_LOST_DEVICE_STATE(object.blas, );

    buildInfo.geometryDesc = desc;
    buildInfo.geometryDescCount = descCount;
    buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(object.blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
    buildInfo.scratchSpaceBufferSizeInBytes = object.blas.getBuildScratchSize();

    raytrace::BatchedBottomAccelerationStructureBuildInfo batchedBuild;
    batchedBuild.as = object.blas.get();
    batchedBuild.basbi = buildInfo;
    blasBuildInfos.push_back(batchedBuild);

    blasModelBudget--;
    blasCount++;

    iter = context_id->halfBakedObjects.erase(iter);
  }

  if (!is_in_lost_device_state)
  {
    d3d::build_bottom_acceleration_structures(blasBuildInfos.data(), blasBuildInfos.size());

    for (auto compaction : pendingCompactions)
      compaction->beginSizeQuery();
  }

  blasBuildInfos.clear();
  geomDescriptors.clear();

  {
    TIME_PROFILE(blas_compactions);
    // Remove the empty compactions from the front
    for (auto iter = context_id->blasCompactions.begin(); iter != context_id->blasCompactions.end() && !iter->has_value();)
      iter = context_id->blasCompactions.erase(iter);

    int activeCompactions = 0;
    int maxActiveCompactions = per_frame_compaction_budget[int(budget)];
    for (auto iter = context_id->blasCompactions.begin();
         iter != context_id->blasCompactions.end() && !is_in_lost_device_state && activeCompactions < maxActiveCompactions;)
    {
      // Inactive compactions are skipped. They will be removed eventually when they get to the front.
      if (!iter->has_value())
      {
        ++iter;
        continue;
      }

      auto cancelCompaction = [](ContextId context_id, Context::CompQueue::iterator &iter) {
        context_id->blasCompactionsAccel.erase(iter->value().objectId);
        iter->reset();
        ++iter;
      };

      auto &compaction = iter->value();

      ++activeCompactions;

      switch (compaction.stage)
      {
        case Context::BLASCompaction::Stage::MovedFrom:
        case Context::BLASCompaction::Stage::Prepared: G_ASSERT(false); break;
        case Context::BLASCompaction::Stage::WaitingSize:
        {
          if (d3d::get_event_query_status(compaction.query.get(), false))
          {
            compaction.compactedSizeValue = 0;
#if _TARGET_C2




#else
            if (auto bufferData = lock_sbuffer<const uint64_t>(compaction.compactedSize.get(), 0, 0, VBLOCK_READONLY))
            {
              compaction.compactedSizeValue = bufferData[0];
            }
#if DAGOR_DBGLEVEL > 0
            // In stub driver, data read from a locked buffer has no meaning, so the size has to be estimated instead
            if (DAGOR_UNLIKELY(d3d::is_stub_driver()))
            {
              // Emulation of dx12 beh -- compacted size was observed to be around 0.25 of initial
              compaction.compactedSizeValue = d3d::get_raytrace_acceleration_structure_size(getBlas(compaction)->get()) / 4;
            }
#endif
#endif
            context_id->compactedSizeBufferCache.push_back();
            context_id->compactedSizeBufferCache.back().swap(compaction.compactedSize);

            if (!compaction.compactedSizeValue)
            {
              cancelCompaction(context_id, iter);
              continue;
            }
            else
            {
              CreateCompactedBLASJob::start(&compaction);
              compaction.stage = Context::BLASCompaction::Stage::WaitingGPUTime;
            }
          }

          break;
        }
        case Context::BLASCompaction::Stage::WaitingGPUTime:
        {
          if (!compaction.compactedBlas)
            break;

          if (isCompactionCheap || blasModelBudget > 0)
          {
            auto blas = getBlas(compaction);
            if (!blas)
            {
              logwarn("[BVH] BLAS compaction %llx is invalid.", compaction.objectId);
              cancelCompaction(context_id, iter);
              continue;
            }

            d3d::copy_raytrace_acceleration_structure(compaction.compactedBlas.get(), blas->get(), true);

            d3d::issue_event_query(compaction.query.get());

            compaction.stage = Context::BLASCompaction::Stage::WaitingCompaction;

            // Compacting is a heavy operation.
            if (!isCompactionCheap)
              blasModelBudget -= 10;
          }
          break;
        }
        case Context::BLASCompaction::Stage::WaitingCompaction:
        {
          if (d3d::get_event_query_status(compaction.query.get(), false))
          {
            if (auto blas = getBlas(compaction))
              blas->swap(compaction.compactedBlas);

            cancelCompaction(context_id, iter);
            continue;
          }
          break;
        }
      }

      ++iter;
    }

    DA_PROFILE_TAG(blas_compactions, "%d of %d active compactions were processed", activeCompactions,
      (int)context_id->blasCompactionsAccel.size());
  }

  auto regGameTex = [&](int var_id, int sampler_id, bvh::Context::BindlessTexHolder &holder, uint32_t *size) {
    TEXTUREID texId = ShaderGlobal::get_tex(var_id);
    auto samplerId = ShaderGlobal::get_sampler(sampler_id);
    if (holder.texId != texId)
    {
      holder.close(context_id);
      holder.texId = texId;
      holder.texSampler = samplerId;
      if (auto texture = context_id->holdTexture(texId, holder.bindlessTexture, holder.bindlessSampler, samplerId); texture && size)
      {
        TextureInfo info;
        texture->getinfo(info);
        *size = info.w;
      }
    }
    else if (holder.texId != BAD_TEXTUREID && holder.texSampler != samplerId)
    {
      // To update the sampler, we re-register the texture with the new sampler
      // then release it to make the reference counter correct.

      holder.texSampler = samplerId;
      context_id->holdTexture(holder.texId, holder.bindlessTexture, holder.bindlessSampler, samplerId);
      context_id->releaseTexure(holder.texId);
    }
  };

  static int paint_details_texVarId = get_shader_variable_id("paint_details_tex", true);
  static int paint_details_tex_samplerstateVarId = get_shader_variable_id("paint_details_tex_samplerstate", true);
  static int grass_land_color_maskVarId = get_shader_variable_id("grass_land_color_mask", true);
  static int grass_land_color_mask_samplerstateVarId = get_shader_variable_id("grass_land_color_mask_samplerstate", true);
  static int dynamic_mfd_texVarId = get_shader_variable_id("dynamic_mfd_tex", true);
  static int dynamic_mfd_tex_samplerstateVarId = get_shader_variable_id("dynamic_mfd_tex_samplerstate", true);
  static int cache_tex0VarId = get_shader_variable_id("cache_tex0", true);
  static int cache_tex0_samplerstateVarId = get_shader_variable_id("cache_tex0_samplerstate", true);
  static int indirection_texVarId = get_shader_variable_id("indirection_tex", true);
  static int cache_tex1VarId = get_shader_variable_id("cache_tex1", true);
  static int cache_tex2VarId = get_shader_variable_id("cache_tex2", true);
  static int last_clip_texVarId = get_shader_variable_id("last_clip_tex", true);
  static int last_clip_tex_samplerstateVarId = get_shader_variable_id("last_clip_tex_samplerstate", true);

  regGameTex(dynamic_mfd_texVarId, dynamic_mfd_tex_samplerstateVarId, context_id->dynamic_mfd_texBindless, nullptr);
  regGameTex(paint_details_texVarId, paint_details_tex_samplerstateVarId, context_id->paint_details_texBindless,
    &context_id->paintTexSize);
  regGameTex(grass_land_color_maskVarId, grass_land_color_mask_samplerstateVarId, context_id->grass_land_color_maskBindless, nullptr);
  regGameTex(cache_tex0VarId, cache_tex0_samplerstateVarId, context_id->cache_tex0Bindless, nullptr);
  regGameTex(indirection_texVarId, -1, context_id->indirection_texBindless, nullptr);
  regGameTex(cache_tex1VarId, -1, context_id->cache_tex1Bindless, nullptr);
  regGameTex(cache_tex2VarId, -1, context_id->cache_tex2Bindless, nullptr);
  regGameTex(last_clip_texVarId, last_clip_tex_samplerstateVarId, context_id->last_clip_texBindless, nullptr);

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

static void copyHwInstancesCpu(void *dst, const HWInstance *src, size_t instance_count)
{
#if _TARGET_C2 || _TARGET_APPLE
  d3d::driver_command(Drv3dCommand::CONVERT_TLAS_INSTANCES, (void *)dst, (void *)src, (void *)instance_count);
#else
  memcpy(dst, src, sizeof(HWInstance) * instance_count);
#endif
}

static void copyHwInstances(Sbuffer *instanceCount, Sbuffer *instances, Sbuffer *uploadBuffer, int bufferSize, int startInstance)
{
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
}

static struct BVHCopyToTLASJob : public cpujobs::IJob
{
  int uploadCount;
  ContextId contextId;
  void start(ContextId context_id, int impostorCount, int riExtraCount)
  {
    contextId = context_id;
    static int bvh_impostor_start_offsetVarId = get_shader_variable_id("bvh_impostor_start_offset");
    static int bvh_impostor_end_offsetVarId = get_shader_variable_id("bvh_impostor_end_offset");

    static constexpr int impostorStartOffset = 0;
    ShaderGlobal::set_int(bvh_impostor_start_offsetVarId, impostorStartOffset);
    ShaderGlobal::set_int(bvh_impostor_end_offsetVarId, impostorStartOffset + impostorCount);

    uploadCount = impostorCount + riExtraCount;
    if (uploadCount > 0)
      threadpool::add(this, threadpool::JobPriority::PRIO_HIGH, true);
  }
  void doJob() override
  {
    const uint32_t HW_INSTANCE_SIZE = d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize;
    auto upload = lock_sbuffer<uint8_t>(contextId->tlasUploadMain.getBuf(), 0, uploadCount * HW_INSTANCE_SIZE,
      VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
    HANDLE_LOST_DEVICE_STATE(upload, );

    auto cursor = upload.get();
    for (auto &instances : contextId->impostorInstances)
    {
      TIME_PROFILE(memcpy_impostors)
      copyHwInstancesCpu(cursor, instances.data(), instances.size());
      cursor += instances.size() * HW_INSTANCE_SIZE;
    }

    for (auto &instances : contextId->riExtraInstances)
    {
      TIME_PROFILE(memcpy_ri_extra)
      copyHwInstancesCpu(cursor, instances.data(), instances.size());
      cursor += instances.size() * HW_INSTANCE_SIZE;
    }
  }
  const char *getJobName(bool &) const override { return "BVHCopyToTLASJob"; }
  void wait()
  {
    TIME_PROFILE(wait_bvh_copy_to_tlas_job)
    threadpool::wait(this);
  }
} bvh_copy_to_tlas_job;

void build(ContextId context_id, const TMatrix &itm, const TMatrix4 &projTm, const Point3 &camera_pos, const Point3 &light_direction)
{
  if (!per_frame_processing_enabled)
  {
    logdbg("[BVH] Device is in reset mode.");
    return;
  }
  CHECK_LOST_DEVICE_STATE();

  FRAMEMEM_REGION;

  TIME_D3D_PROFILE_NAME(bvh_build, String(128, "bvh_build for %s", context_id->name.data()));

  if (context_id->has(Features::DynrendRigidFull | Features::DynrendRigidBaked | Features::DynrendSkinnedFull))
    dyn::wait_dynrend_instances();
  if (context_id->has(Features::RIFull | Features::RIBaked))
  {
    ri::wait_ri_gen_instances_update(context_id);
    ri::wait_ri_extra_instances_update(context_id, true);
  }

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

  Context::RingBuffers::step();

  // PS5 and Metal has different instance size, but PS5 bvh_hwinstance_copy version supports transform from HWInstance to it
#if !_TARGET_C2 && !_TARGET_APPLE
  G_ASSERTF(sizeof(HWInstance) == d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize,
    "HW raytracing instance size mismatch, expected %d, but got %d", sizeof(HWInstance),
    d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize);
#endif

  const uint32_t HW_INSTANCE_SIZE = d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize;

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

  if (reallocateMainTlas)
  {
    TIME_PROFILE(allocate_main_tlas_upload);
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadMain.allocate(HW_INSTANCE_SIZE, uploadSizeMain, SBCF_UA_SR_STRUCTURED,
                               "bvh_tlas_upload_main", context_id), );
  }
  if (reallocateTerrainTlas)
  {
    TIME_PROFILE(allocate_terrain_tlas_upload);
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadTerrain.allocate(HW_INSTANCE_SIZE, uploadSizeTerrain, SBCF_UA_SR_STRUCTURED,
                               "bvh_tlas_upload_terrain", context_id), );
  }

  if (!context_id->tlasUploadParticles)
  {
    TIME_PROFILE(allocate_particle_buffer);
    context_id->tlasUploadParticles =
      dag::buffers::create_ua_sr_structured(HW_INSTANCE_SIZE, uploadSizeParticles, ccn(context_id, "bvh_tlas_upload_particles"));
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadParticles, );
    logdbg("tlasUploadParticles resized to %u", uploadSizeParticles);
  }

  bvh_copy_to_tlas_job.start(context_id, impostorCount, riExtraCount);

  context_id->animatedInstanceCount = 0;

  auto &instanceDescs = context_id->instanceDescsCpu;
  auto &perInstanceData = context_id->perInstanceDataCpu;

  instanceDescs.clear();
  instanceDescs.reserve(instanceCount);

  perInstanceData.clear();
  if (use_icthgi_for_per_instance_data)
  {
    // This is expected to store camo only, the rest can be packed into InstanceContributionToHitGroupIndex
    perInstanceData.reserve(dynrendCount + 1);
    perInstanceData.push_back(UPoint2{0, 0});
  }
  else
  {
    TIME_PROFILE(patch_per_instance_data)
    perInstanceData.reserve(instanceCount);

    for (auto &instanceData : context_id->impostorInstanceData)
    {
      auto preSize = perInstanceData.size();
      perInstanceData.resize(preSize + instanceData.size());
      memcpy(perInstanceData.data() + preSize, instanceData.data(), instanceData.size() * sizeof(UPoint2));
    }
    for (auto &instanceData : context_id->riExtraInstanceData)
    {
      auto preSize = perInstanceData.size();
      perInstanceData.resize(preSize + instanceData.size());
      memcpy(perInstanceData.data() + preSize, instanceData.data(), instanceData.size() * sizeof(UPoint2));
    }
  }

  dag::Vector<::raytrace::BatchedBottomAccelerationStructureBuildInfo, framemem_allocator> blasUpdates;
  constexpr size_t MAX_GEOMS = 32;
  dag::Vector<eastl::fixed_vector<RaytraceGeometryDescription, MAX_GEOMS>, framemem_allocator> updateGeoms;
  eastl::unordered_set<void *, eastl::hash<void *>, eastl::equal_to<void *>, framemem_allocator> allBlasUpdatesAs;
  {
    TIME_PROFILE(allocate_update);
    blasUpdates.reserve(512);
    updateGeoms.reserve(512);
  }

  auto itmRelative = itm;
  itmRelative.setcol(3, Point3::ZERO);
  auto frustumRelative = Frustum(TMatrix4(orthonormalized_inverse(itmRelative)) * projTm);
  auto frustumAbsolute = Frustum(TMatrix4(orthonormalized_inverse(itm)) * projTm);

  auto addInstances = [&](const Context::InstanceMap &instances, dag::Vector<HWInstance> &hwInstances, uint32_t group_mask,
                        bool is_camera_relative, const char *name) {
    CHECK_LOST_DEVICE_STATE();

    TIME_PROFILE_NAME(addInstance, name);

    const float maxLightDistForBvhShadow = 20.0f;
    auto lightDirection = v_mul(v_ldu_p3_safe(&light_direction.x), v_splats(maxLightDistForBvhShadow * 2));
    auto cameraPos = v_ldu_p3_safe(&camera_pos.x);

    for (auto &instance : instances)
    {
      BVH_PROFILE(add_one_instance);
      CHECK_LOST_DEVICE_STATE();

      if (context_id->halfBakedObjects.count(instance.objectId))
        continue;

      auto iter = context_id->objects.find(instance.objectId);
      if (iter == context_id->objects.end())
        continue;

      auto &object = iter->second;
      auto &blas = instance.uniqueBlas ? *instance.uniqueBlas : object.blas;
      auto metaAllocId = MeshMetaAllocator::is_valid(instance.metaAllocId) ? instance.metaAllocId : object.metaAllocId;
      auto metaRegion = context_id->meshMetaAllocator.get(metaAllocId);
      auto baseMetaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);

      if (eastl::any_of(object.meshes.begin(), object.meshes.end(), [](const auto &m) { return m.vertexProcessor; }))
      {
        bool needBlasBuild = false;

        auto &geoms = updateGeoms.emplace_back();

        for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
        {
          G_ASSERT(!mesh.vertexProcessor->isOneTimeOnly() && instance.uniqueTransformedBuffer);

          auto animatedVertices = UniqueOrReferencedBVHBuffer(*instance.uniqueTransformedBuffer); //-V595
          auto verticesRecycled = instance.uniqueIsRecycled;

          if (instance.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID && !meta.isInitialized())
          {
            // The meta is specific to an instance and not yet initialized.
            meta = baseMeta;
            meta.markInitialized();
          }

          if (instance.metaAllocId != MeshMetaAllocator::INVALID_ALLOC_ID)
          {
            // It is possible that the mesh for the instance was unloaded and loaded again. When that
            // happens, the mesh has new buffers and textures, so we need to make sure the meta has
            // the correct indices.
            if ((meta.materialType & MeshMeta::bvhMaterialUseInstanceTextures) == 0)
            {
              meta.alphaTextureIndex = baseMeta.alphaTextureIndex;
              meta.alphaSamplerIndex = baseMeta.alphaSamplerIndex;
              meta.albedoTextureIndex = baseMeta.albedoTextureIndex;
              meta.albedoAndNormalSamplerIndex = baseMeta.albedoAndNormalSamplerIndex;
              meta.normalTextureIndex = baseMeta.normalTextureIndex;
              meta.extraTextureIndex = baseMeta.extraTextureIndex;
              meta.extraSamplerIndex = baseMeta.extraSamplerIndex;
            }
            meta.ahsVertexBufferIndex = baseMeta.ahsVertexBufferIndex;
            // Copy the index buffer index only, the vertex buffer is part of the instance
            meta.indexBufferIndex = baseMeta.indexBufferIndex;
          }

          if (meta.materialType & MeshMeta::bvhMaterialAlphaTest)
            G_ASSERT(meta.ahsVertexBufferIndex != BVH_BINDLESS_BUFFER_MAX);

          process_vertices(context_id, instance.objectId, mesh, &instance, is_camera_relative ? &frustumRelative : &frustumAbsolute,
            is_camera_relative ? v_zero() : cameraPos, lightDirection, animatedVertices, verticesRecycled, needBlasBuild, meta,
            baseMeta, true);

          CHECK_LOST_DEVICE_STATE();

          meta.vertexOffset = animatedVertices.getOffset();
          bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;
          auto &geom = geoms.emplace_back();

          geom.type = RaytraceGeometryDescription::Type::TRIANGLES;
          geom.data.triangles.vertexBuffer = animatedVertices.get();
          geom.data.triangles.vertexOffsetExtraBytes = animatedVertices.getOffset();
          geom.data.triangles.indexBuffer = mesh.processedIndices.get();
          geom.data.triangles.vertexCount = mesh.vertexCount;
          geom.data.triangles.vertexStride = meta.vertexStride;
          geom.data.triangles.vertexFormat = mesh.processedPositionFormat;
          geom.data.triangles.indexCount = mesh.indexCount;
          geom.data.triangles.indexOffset = meta.startIndex;
          geom.data.triangles.flags =
            (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
        }

        // If the BLAS is not unique, then only build it when it has not been built at all.
        if (needBlasBuild && (!blas || instance.uniqueBlas))
        {

          RaytraceBuildFlags flags =
            object.isAnimated ? RaytraceBuildFlags::FAST_BUILD | RaytraceBuildFlags::ALLOW_UPDATE : RaytraceBuildFlags::FAST_TRACE;
          bool isNew = false;

          if (!blas)
          {
            blas = UniqueBLAS::create(geoms.data(), geoms.size(), flags);
            isNew = true;

            HANDLE_LOST_DEVICE_STATE(blas, );

            if (object.blas && object.blas != blas)
            {
              d3d::copy_raytrace_acceleration_structure(blas.get(), object.blas.get());
              isNew = false;
            }
          }

          if (!allBlasUpdatesAs.insert(blas.get()).second)
          {
            logerr("[BVH] Duplicate AS instance found! This should not happen. Report it with a screenshot!");
            updateGeoms.pop_back();
            continue;
          }

          auto &update = blasUpdates.emplace_back();
          update.as = blas.get();
          update.basbi.flags = flags;
          update.basbi.geometryDesc = geoms.data();
          update.basbi.geometryDescCount = geoms.size();
          update.basbi.doUpdate = object.isAnimated && !isNew;
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
          if (!object.blas)
          {
            // inside delay/continue sync we must use only that work that can be batched together
            // otherwise we can't properly generate barriers, so simply make another batch when we need to build BLAS
            const bool delaySync = d3d::get_driver_code().is(d3d::vulkan) || d3d::get_driver_code().is(d3d::ps5);
            if (delaySync)
              d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);

            G_ASSERT(update.basbi.scratchSpaceBufferSizeInBytes > 0);

            for (auto &geom : geoms)
              d3d::resource_barrier(ResourceBarrierDesc(geom.data.triangles.vertexBuffer, bindlessSRVBarrier));

            d3d::build_bottom_acceleration_structure(update.as, update.basbi);

            for (auto &geom : geoms)
              d3d::resource_barrier(ResourceBarrierDesc(geom.data.triangles.vertexBuffer, bindlessUAVBarrier));

            // build_bottom_acceleration_structure is not flushing the BLAS,
            // so we flush it here, before cloning it.
            d3d::resource_barrier(ResourceBarrierDesc(update.as));

            object.blas = UniqueBLAS::create(geoms.data(), geoms.size(), update.basbi.flags);
            d3d::copy_raytrace_acceleration_structure(object.blas.get(), update.as);

            allBlasUpdatesAs.erase(blasUpdates.back().as);
            blasUpdates.pop_back();
            updateGeoms.pop_back();

            if (delaySync)
              d3d::driver_command(Drv3dCommand::DELAY_SYNC);
          }
        }
        else
        {
          updateGeoms.pop_back();
        }
      }

      bool hasPerInstanceData = instance.perInstanceData.has_value();
      bool isPerInstanceDataColor = instance.hasInstanceColor;
      bool isPerInstanceDataAlbedo = hasPerInstanceData && instance.perInstanceData.value().x == 0xA1B3D0U;
      uint32_t perInstanceDataIndex = 0;
      if (!use_icthgi_for_per_instance_data || !isPerInstanceDataColor || !isPerInstanceDataAlbedo)
      {
        perInstanceDataIndex = perInstanceData.size();
        perInstanceData.emplace_back(instance.perInstanceData.value_or(UPoint2{0, 0}));
      }

      auto &desc = hwInstances.emplace_back();
      desc.transform = instance.transform;
      desc.instanceID = MeshMetaAllocator::decode(metaAllocId);
      desc.instanceMask = instance.noShadow ? bvhGroupNoShadow : group_mask;
      desc.instanceContributionToHitGroupIndex = [&]() -> uint32_t {
        if (hasPerInstanceData)
        {
          if (use_icthgi_for_per_instance_data)
          {
            if (isPerInstanceDataColor)
              return ICTHGI_USE_AS_777_ISTANCE_COLOR | pack_color8_to_color777(instance.perInstanceData.value().x);
            else if (isPerInstanceDataAlbedo)
              return ICTHGI_REPLACE_ALBEDO_TEXTURE | (((instance.perInstanceData.value().y >> 16) & 0xFFFFu) << 3);
            else
              return ICTHGI_LOAD_PER_INSTANCE_DATA_WITH_ICTHGI_INDEX | (perInstanceDataIndex << 3);
          }
          else
            return ICTHGI_LOAD_PER_INSTANCE_DATA_WITH_INSTANCE_INDEX;
        }
        else
          return ICTHGI_NO_PER_INSTANCE_DATA;
      }();
      desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
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
    // ps5 driver doesn't support user barriers and currently only DELAY_SYNC command can be used to allow parallel execution of
    // dispatches
    const bool delaySync = d3d::get_driver_code().is(d3d::vulkan) || d3d::get_driver_code().is(d3d::ps5);
    if (!rbBuffers.empty() && !delaySync)
    {
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessUAVBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size())); // -V512
    }
    else
      d3d::driver_command(Drv3dCommand::DELAY_SYNC);

    addInstances(context_id->genericInstances, instanceDescs, bvhGroupRiExtra, false, "generic");
    CHECK_LOST_DEVICE_STATE();

    {
      ProcessorInstances::getTreeVertexProcessor().begin();

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

      ProcessorInstances::getTreeVertexProcessor().end();
    }
    for (auto &instances : context_id->dynrendInstances)
    {
      if (instances.first >= dynrend::ContextId{0})
        dyn::set_up_dynrend_context_for_processing(instances.first);
      addInstances(instances.second, instanceDescs, bvhGroupDynrend, true, "dynrend");
      CHECK_LOST_DEVICE_STATE();
    }

    CHECK_LOST_DEVICE_STATE();

    if (!rbBuffers.empty() && !delaySync)
    {
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessSRVBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size())); // -V512
    }
    else
      d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
  }

  {
    TIME_PROFILE(UploadMeta)
    int metaCount = context_id->meshMetaAllocator.size();

    if (context_id->meshMeta && context_id->meshMeta->getNumElements() < metaCount)
      context_id->meshMeta.close();

    if (!context_id->meshMeta)
      HANDLE_LOST_DEVICE_STATE(context_id->meshMeta.allocate(sizeof(MeshMeta), metaCount, SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED,
                                 "bvh_meta", context_id), );

    if (auto upload = lock_sbuffer<MeshMeta>(context_id->meshMeta.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
    {
      auto dst = upload.get();
      int poolIndex = 0;
      while (auto src = context_id->meshMetaAllocator.data(poolIndex++))
      {
        memcpy(dst, src, MeshMetaAllocator::PoolSize * sizeof(MeshMeta));
        dst += MeshMetaAllocator::PoolSize;
      }
    }
  }

  {
    TIME_D3D_PROFILE_NAME(procedural_blas_builds, String(64, "procedural_blas_builds: %d", blasUpdates.size()));
    if (showProceduralBLASBuildCount)
      visuallog::logmsg(String(64, "Procedural BLAS builds: %d", blasUpdates.size()));

    for (auto [geoms, update] : zip(updateGeoms, blasUpdates))
    {
      update.basbi.geometryDesc = geoms.data();
      update.basbi.geometryDescCount = geoms.size();
    }

    d3d::build_bottom_acceleration_structures(blasUpdates.data(), blasUpdates.size());
  }

  int terrainDescIndex = instanceDescs.size();

  {
    TIME_D3D_PROFILE(terrain);

    for (auto [blasIx, blas] : enumerate(terrainBlases))
    {
      bool hasHole = eastl::get<3>(blas);
      auto &origin = eastl::get<2>(blas);

      auto &desc = instanceDescs.emplace_back();
      desc.transform.row0 = v_make_vec4f(1, 0, 0, origin.x - camera_pos.x);
      desc.transform.row1 = v_make_vec4f(0, 1, 0, 0 - camera_pos.y);
      desc.transform.row2 = v_make_vec4f(0, 0, 1, origin.y - camera_pos.z);

      desc.instanceID = MeshMetaAllocator::decode(eastl::get<1>(blas));
      desc.instanceMask = bvhGroupTerrain;
      desc.instanceContributionToHitGroupIndex = ICTHGI_NO_PER_INSTANCE_DATA;
      desc.flags = hasHole ? RaytraceGeometryInstanceDescription::FORCE_NO_OPAQUE : RaytraceGeometryInstanceDescription::NONE;
      desc.blasGpuAddress = eastl::get<0>(blas);

      if (!use_icthgi_for_per_instance_data)
        perInstanceData.emplace_back(UPoint2{0, 0});
    }
  }

  if (cableBlases) //-V547
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
      desc.instanceContributionToHitGroupIndex = ICTHGI_NO_PER_INSTANCE_DATA;
      desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
      desc.blasGpuAddress = blas.getGPUAddress();

      if (!use_icthgi_for_per_instance_data)
        perInstanceData.emplace_back(UPoint2{0, 0});
    }
  }

  int cpuInstanceCount = 0;

  {
    TIME_D3D_PROFILE(copy_to_tlas_main_upload);

    bvh_copy_to_tlas_job.wait();

    cpuInstanceCount += impostorCount + riExtraCount;

    int uploadCount = instanceDescs.size();
    {
      auto upload = lock_sbuffer<uint8_t>(context_id->tlasUploadMain.getBuf(), cpuInstanceCount * HW_INSTANCE_SIZE,
        uploadCount * HW_INSTANCE_SIZE, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
      HANDLE_LOST_DEVICE_STATE(upload, );

      auto cursor = upload.get();
      {
        TIME_PROFILE(memcpy_instances)
        copyHwInstancesCpu(cursor, instanceDescs.data(), instanceDescs.size());
        cursor += instanceDescs.size() * HW_INSTANCE_SIZE;
      }
      cpuInstanceCount += (cursor - upload.get()) / HW_INSTANCE_SIZE;
    }

    G_ASSERT(cpuInstanceCount <= instanceCount);

    {
      TIME_D3D_PROFILE(memcpy_grass)
      copyHwInstances(grassInstanceCount, grassInstances, context_id->tlasUploadMain.getBuf(), grassBufferSize, cpuInstanceCount);
      cpuInstanceCount += grassBufferSize;
    }

    {
      TIME_D3D_PROFILE(memcpy_gpu_objects)
      copyHwInstances(gobjInstanceCount, gobjInstances, context_id->tlasUploadMain.getBuf(), gobjBufferSize, cpuInstanceCount);
      cpuInstanceCount += gobjBufferSize;
    }
  }

  if (!use_icthgi_for_per_instance_data)
  {
    TIME_PROFILE(patch_per_instance_data);
    if ((grassBufferSize + gobjBufferSize) > 0)
      perInstanceData.resize(perInstanceData.size() + grassBufferSize + gobjBufferSize, {0, 0});
  }

  if (!perInstanceData.empty())
  {
    TIME_D3D_PROFILE(upload_per_instance_data);

    if (context_id->perInstanceData && context_id->perInstanceData->getNumElements() < perInstanceData.size())
      context_id->perInstanceData.close();

    if (!context_id->perInstanceData)
      HANDLE_LOST_DEVICE_STATE(context_id->perInstanceData.allocate(sizeof(UPoint2), perInstanceData.size(),
                                 SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, "bvh_per_instance_data", context_id), );

    context_id->perInstanceData->updateDataWithLock(0, perInstanceData.size() * sizeof(UPoint2), perInstanceData.data(),
      VBLOCK_NOOVERWRITE);
  }

  {
    TIME_D3D_PROFILE(copy_to_tlas_terrain_upload);
    if (auto upload = lock_sbuffer<uint8_t>(context_id->tlasUploadTerrain.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
      copyHwInstancesCpu(upload.get(), instanceDescs.data() + terrainDescIndex, terrainBlases.size());
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

  {
    TIME_D3D_PROFILE(build_tlas);

    context_id->tlasMainValid = context_id->tlasMain && cpuInstanceCount;
    context_id->tlasTerrainValid = context_id->tlasTerrain && terrainBlases.size();
    context_id->tlasParticlesValid = context_id->tlasParticles && fxBufferSize;

    raytrace::BatchedTopAccelerationStructureBuildInfo tlasUpdate[3];
    int tlasCount = 0;

    auto fillTlas = [&](const UniqueTLAS &tlas, Sbuffer *instances, uint32_t instance_count) {
      auto &tasbi = tlasUpdate[tlasCount].tasbi;

      G_ASSERT(tlas.get());
      G_ASSERT(tlas.getScratchBuffer());

      tlasUpdate[tlasCount].as = tlas.get();
      tasbi.doUpdate = false;
      tasbi.instanceBuffer = instances;
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
      fillTlas(context_id->tlasMain, context_id->tlasUploadMain.getBuf(), cpuInstanceCount);
      CHECK_LOST_DEVICE_STATE();
    }

    if (context_id->tlasTerrainValid)
    {
      fillTlas(context_id->tlasTerrain, context_id->tlasUploadTerrain.getBuf(), terrainBlases.size());
      CHECK_LOST_DEVICE_STATE();
    }

    if (context_id->tlasParticlesValid)
    {
      fillTlas(context_id->tlasParticles, context_id->tlasUploadParticles.getBuf(), fxBufferSize);
      CHECK_LOST_DEVICE_STATE();
    }

    d3d::build_top_acceleration_structures(tlasUpdate, tlasCount);
  }

  {
    TIME_PROFILE(update_bindings)
    context_id->markChangedTextures();
    context_id->bindlessTextureAllocator.getHeap().updateBindings();
    context_id->bindlessCubeTextureAllocator.getHeap().updateBindings();
    context_id->bindlessBufferAllocator.getHeap().updateBindings();
  }

  static int bvh_originVarId = get_shader_variable_id("bvh_origin");
  ShaderGlobal::set_color4(bvh_originVarId, camera_pos);

  debug::render_debug_context(context_id, debug_min_t);

  ri::cut_down_trees(context_id);
  dyn::purge_skin_buffers(context_id);

  context_id->processDeathrow();

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
  const char *getJobName(bool &) const override { return "BVHUpdateAtmosphereJob"; }
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
  static int bvh_cube_textures_range_startVarId = get_shader_variable_id("bvh_cube_textures_range_start");
  static int bvh_buffers_range_startVarId = get_shader_variable_id("bvh_buffers_range_start");
  static int bvh_meta_countVarId = get_shader_variable_id("bvh_meta_count");
  static int bvh_metaVarId = get_shader_variable_id("bvh_meta");
  static int bvh_per_instance_dataVarId = get_shader_variable_id("bvh_per_instance_data");
  static int bvh_atmosphere_textureVarId = get_shader_variable_id("bvh_atmosphere_texture");
  static int bvh_atmosphere_texture_distanceVarId = get_shader_variable_id("bvh_atmosphere_texture_distance");
  static int bvh_paint_details_tex_slotVarId = get_shader_variable_id("bvh_paint_details_tex_slot");
  static int bvh_paint_details_smp_slotVarId = get_shader_variable_id("bvh_paint_details_smp_slot");
  static int bvh_land_color_tex_slotVarId = get_shader_variable_id("bvh_land_color_tex_slot", true);
  static int bvh_land_color_smp_slotVarId = get_shader_variable_id("bvh_land_color_smp_slot", true);
  static int bvh_dynamic_mfd_color_tex_slotVarId = get_shader_variable_id("bvh_dynamic_mfd_color_tex_slot", true);
  static int bvh_dynamic_mfd_color_smp_slotVarId = get_shader_variable_id("bvh_dynamic_mfd_color_smp_slot", true);

  static int cache_tex0_tex_slotVarId = get_shader_variable_id("cache_tex0_tex_slot", true);
  static int cache_tex0_smp_slotVarId = get_shader_variable_id("cache_tex0_smp_slot", true);
  static int indirection_tex_tex_slotVarId = get_shader_variable_id("indirection_tex_tex_slot", true);
  static int cache_tex1_tex_slotVarId = get_shader_variable_id("cache_tex1_tex_slot", true);
  static int cache_tex2_tex_slotVarId = get_shader_variable_id("cache_tex2_tex_slot", true);
  static int last_clip_tex_tex_slotVarId = get_shader_variable_id("last_clip_tex_tex_slot", true);
  static int last_clip_tex_smp_slotVarId = get_shader_variable_id("last_clip_tex_smp_slot", true);

  static int bvh_mip_rangeVarId = get_shader_variable_id("bvh_mip_range");
  static int bvh_mip_scaleVarId = get_shader_variable_id("bvh_mip_scale");
  static int bvh_mip_biasVarId = get_shader_variable_id("bvh_mip_bias");
#if !_TARGET_C2
  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
#endif
  static int bvh_main_validVarId = get_shader_variable_id("bvh_main_valid");
  static int bvh_terrain_validVarId = get_shader_variable_id("bvh_terrain_valid");
  static int bvh_particles_validVarId = get_shader_variable_id("bvh_particles_valid");
  static int bvh_max_water_distanceVarId = get_shader_variable_id("bvh_max_water_distance");
  static int bvh_water_fade_powerVarId = get_shader_variable_id("bvh_water_fade_power", true);
  static int bvh_max_water_depthVarId = get_shader_variable_id("bvh_max_water_depth", true);
  static int rtr_max_water_depthVarId = get_shader_variable_id("rtr_max_water_depth", true);

#if !_TARGET_C2
  ShaderGlobal::set_tlas(bvh_mainVarId, context_id->tlasMainValid ? context_id->tlasMain.get() : nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, context_id->tlasTerrainValid ? context_id->tlasTerrain.get() : nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, context_id->tlasParticlesValid ? context_id->tlasParticles.get() : nullptr);
#endif

  ShaderGlobal::set_int(bvh_main_validVarId, context_id->tlasMainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_terrain_validVarId, context_id->tlasTerrainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_particles_validVarId, context_id->tlasParticlesValid ? 1 : 0);

  ShaderGlobal::set_int(bvh_textures_range_startVarId, context_id->bindlessTextureAllocator.getHeap().location());
  ShaderGlobal::set_int(bvh_cube_textures_range_startVarId, context_id->bindlessCubeTextureAllocator.getHeap().location());
  ShaderGlobal::set_int(bvh_buffers_range_startVarId, context_id->bindlessBufferAllocator.getHeap().location());
  ShaderGlobal::set_int(bvh_meta_countVarId, context_id->meshMetaAllocator.size());
  ShaderGlobal::set_buffer(bvh_metaVarId, context_id->meshMeta.getBufId());
  ShaderGlobal::set_buffer(bvh_per_instance_dataVarId, context_id->perInstanceData.getBufId());

  ShaderGlobal::set_int(bvh_paint_details_tex_slotVarId, context_id->paint_details_texBindless.bindlessTexture);
  ShaderGlobal::set_int(bvh_paint_details_smp_slotVarId, context_id->paint_details_texBindless.bindlessSampler);

  ShaderGlobal::set_int(bvh_land_color_tex_slotVarId, context_id->grass_land_color_maskBindless.bindlessTexture);
  ShaderGlobal::set_int(bvh_land_color_smp_slotVarId, context_id->grass_land_color_maskBindless.bindlessSampler);

  ShaderGlobal::set_int(bvh_dynamic_mfd_color_tex_slotVarId, context_id->dynamic_mfd_texBindless.bindlessTexture);
  ShaderGlobal::set_int(bvh_dynamic_mfd_color_smp_slotVarId, context_id->dynamic_mfd_texBindless.bindlessSampler);

  ShaderGlobal::set_int(cache_tex0_tex_slotVarId, context_id->cache_tex0Bindless.bindlessTexture);
  ShaderGlobal::set_int(cache_tex0_smp_slotVarId, context_id->cache_tex0Bindless.bindlessSampler);
  ShaderGlobal::set_int(indirection_tex_tex_slotVarId, context_id->indirection_texBindless.bindlessTexture);
  ShaderGlobal::set_int(cache_tex1_tex_slotVarId, context_id->cache_tex1Bindless.bindlessTexture);
  ShaderGlobal::set_int(cache_tex2_tex_slotVarId, context_id->cache_tex2Bindless.bindlessTexture);
  ShaderGlobal::set_int(last_clip_tex_tex_slotVarId, context_id->last_clip_texBindless.bindlessTexture);
  ShaderGlobal::set_int(last_clip_tex_smp_slotVarId, context_id->last_clip_texBindless.bindlessSampler);


  ShaderGlobal::set_texture(bvh_atmosphere_textureVarId, context_id->atmosphereTexture);
  ShaderGlobal::set_real(bvh_atmosphere_texture_distanceVarId, Context::atmMaxDistance);

  ShaderGlobal::set_real(bvh_mip_rangeVarId, mip_range);
  ShaderGlobal::set_real(bvh_mip_scaleVarId, mip_scale);
  ShaderGlobal::set_real(bvh_mip_biasVarId, max(log2f(3840.0f / render_width), 0.0f));
  ShaderGlobal::set_real(bvh_max_water_distanceVarId, max_water_distance);
  ShaderGlobal::set_real(bvh_water_fade_powerVarId, water_fade_power);
  ShaderGlobal::set_real(bvh_max_water_depthVarId, max_water_depth);
  ShaderGlobal::set_real(rtr_max_water_depthVarId, rtr_max_water_depth);
}

#if !_TARGET_C2
void bind_tlas_stage(ContextId, ShaderStage) {}
void unbind_tlas_stage(ShaderStage) {}
#else
























#endif


void set_for_gpu_objects(ContextId context_id) { gobj::init(context_id); }

void prepare_ri_extra_instances() { ri::prepare_ri_extra_instances(); }

void on_before_unload_scene(ContextId context_id) { context_id->releaseAllBindlessTexHolders(); }

void on_before_settings_changed(ContextId context_id) { context_id->releaseAllBindlessTexHolders(); }

void on_load_scene(ContextId context_id) { context_id->clearDeathrow(); }

void on_scene_loaded(ContextId context_id)
{
  context_id->clearDeathrow();
  ri::on_scene_loaded(context_id);
}

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

  context_id->instanceDescsCpu.clear();
  context_id->instanceDescsCpu.shrink_to_fit();
  context_id->perInstanceDataCpu.clear();
  context_id->perInstanceDataCpu.shrink_to_fit();

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
  return !context_id->halfBakedObjects.empty() || context_id->hasPendingMeshAddActions.load(dag::memory_order_relaxed);
}

void set_grass_range(ContextId context_id, float range) { context_id->grassRange = range; }

void start_async_atmosphere_update(ContextId context_id, const Point3 &view_pos, atmosphere_callback callback)
{
  if (!context_id->atmosphereTexture)
  {
    context_id->atmosphereTexture =
      dag::create_array_tex(Context::atmTexWidth, Context::atmTexHeight, 2, TEXCF_DYNAMIC, 1, "bvh_atmosphere_tex");
    HANDLE_LOST_DEVICE_STATE(context_id->atmosphereTexture, );
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = d3d::AddressMode::Wrap;
      smpInfo.address_mode_v = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(get_shader_variable_id("bvh_atmosphere_texture_samplerstate"), d3d::request_sampler(smpInfo));
    }
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

void set_ri_dist_mul(float mul) { ri::set_dist_mul(mul); }

void override_out_of_camera_ri_dist_mul(float dist_sq_mul_ooc) { ri::override_out_of_camera_ri_dist_mul(dist_sq_mul_ooc); }

void set_debug_view_min_t(float min_t) { debug_min_t = min_t; }

} // namespace bvh
