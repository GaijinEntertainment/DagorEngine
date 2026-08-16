// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lockSbuffer.h>
#include <3d/dag_texMgr.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <shaders/dag_shStateBlockBindless.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_shaderBlock.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include <render/denoiser.h>
#include <render/smokeTracers.h>
#include <render/dynmodelRenderer.h>
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
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/numeric.h>
#include <gui/dag_visualLog.h>
#include <userSystemInfo/systemInfo.h>
#include <util/dag_convar.h>
#include <gameRes/dag_resourceNameResolve.h>

#include "bvh_context.h"
#include "bvh_debug.h"
#include "bvh_tlas_debug.h"
#include "bvh_add_instance.h"
#include "bvh_omm.h"
#include "shaders/dag_shaderVar.h"
#include "bvh_tools.h"
#include "bvh_particles.h"

CONSOLE_INT_VAL("render", bvh_riGen_budget_us, 10000, 100, 10000);
CONSOLE_BOOL_VAL("render", bvh_disable_parallel_instance_processing_finish, false);

CONSOLE_BOOL_VAL("render", bvh_disable_rigen_meta_prebuild, false);
CONSOLE_BOOL_VAL("render", bvh_disable_dynrend_meta_prebuild, false);

CONSOLE_BOOL_VAL("raytracing", bvh_mem_log, false);

static eastl::array<int, 3> per_frame_blas_model_budget = {10, 15, 30};
static eastl::array<int, 3> per_frame_compaction_budget = {20, 30, 60};

static constexpr bool showProceduralBLASBuildCount = false;

static bool per_frame_processing_enabled = true;

#if DAGOR_DBGLEVEL > 0
extern bool bvh_gpuobject_enable;
extern bool bvh_grass_enable;
extern bool bvh_particles_enable;
extern bool bvh_cables_enable;
extern bool bvh_splinegen_enable;
extern bool bvh_tracers_enable;
#endif

#if BVH_PROFILING_ENABLED
#define BVH_PROFILE TIME_PROFILE
#else
#define BVH_PROFILE(...)
#endif

namespace bvh
{

String AssetNameRef::resolve() const
{
  String name;
  if (resource && resolver && resolver(name, resource))
    return name;
  return String("?");
}

AssetNameRef make_asset_name_ref(const RenderableInstanceLodsResource *resource)
{
  return {resource, [](String &out_name, const void *res) {
            return resolve_game_resource_name(out_name, static_cast<const RenderableInstanceLodsResource *>(res));
          }};
}

AssetNameRef make_asset_name_ref(const DynamicRenderableSceneLodsResource *resource)
{
  return {resource, [](String &out_name, const void *res) {
            return resolve_game_resource_name(out_name, static_cast<const DynamicRenderableSceneLodsResource *>(res));
          }};
}

namespace terrain
{
dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2>> get_blases(ContextId context_id);

void init();
void init(ContextId context_id);
void teardown();
void teardown(ContextId context_id);
} // namespace terrain

namespace ri
{
void init(const AdditionalSettings &settings);
void teardown(bool device_reset);
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_scene_loaded(ContextId context_id);
void on_unload_scene(ContextId context_id);
void prepare_ri_extra_instances();
// instances in view_frustum in ri_gen_visibilities[1] will be discarded!
void update_ri_gen_instances(ContextId context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  const Point3 &view_position, const Point3 &light_direction, const Frustum &view_frustum, threadpool::JobPriority prio);
void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Point3 &lod_anchor_position,
  const Frustum &bvh_frustum, const Frustum &view_frustum, const Point3 &light_direction, threadpool::JobPriority prio);
void wait_ri_gen_instances_update(ContextId context_id);
void wait_ri_extra_instances_update(ContextId context_id);
void tidy_up_trees(ContextId context_id);
void wait_tidy_up_trees();
void set_dist_mul(float mul);
void override_out_of_camera_ri_dist_mul(float dist_sq_mul_ooc);
void readdRendinst(ContextId context_id, const RenderableInstanceLodsResource *resource);
} // namespace ri

namespace dyn
{
void init(const AdditionalSettings &settings);
void teardown(bool device_reset, bool zero_bvh_ids);
void init(ContextId context_id);
void teardown(ContextId context_id);
void enable_dynamic_planar_decals(bool enable);
void on_unload_scene(ContextId context_id);
void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position,
  dag::Vector<DynamicRenderableSceneInstance *> &&og_instances);
void wait_dynrend_instances();
void update_animchar_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position, dynrend::BVHIterateCallback iterate_callback);
void wait_animchar_instances();
void debug_update();
void set_up_dynrend_context_for_processing(dynrend::ContextId dynrend_context_id);
void tidy_up_skins(ContextId context_id);
void wait_tidy_up_skins();
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
void process_omm(ContextId context_id);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
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

namespace binscene
{
void init();
void teardown();
void add_meshes(ContextId context_id, BaseStreamingSceneHolder &bin_scene);
void update_instances(ContextId context_id);
void on_unload_scene(ContextId context_id);
} // namespace binscene

namespace fftwater
{
void init();
void teardown();
void create_patches(ContextId context_id, FFTWater &water);
void on_unload_scene(ContextId context_id);
} // namespace fftwater

namespace gpugrass
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void generate_instances(ContextId context_id, bool has_grass);
void make_meta(ContextId context_id, const GPUGrassBase &grass);
void process_omm(ContextId context_id);
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
} // namespace gpugrass

namespace debug
{
void init(ContextId context_id);
void teardown(ContextId context_id);
void teardown();
void render_debug_context(ContextId context_id, float min_t);
} // namespace debug

namespace dagdp
{
void init(ContextId context_id);
void teardown(ContextId context_id);
::BVHInstanceMapper *get_mapper(ContextId context_id);
::BVHInstanceMapper::InstanceBuffers get_buffers(ContextId context_id);
} // namespace dagdp

namespace splinegen
{
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void add_meshes(ContextId context_id, Sbuffer *vertex_buffer, eastl::vector<eastl::pair<uint32_t, MeshInfo>> &meshes,
  uint32_t instance_vertex_count, uint32_t &bvh_id);
void update_instances(ContextId context_id, const Point3 &view_pos);
} // namespace splinegen

namespace smoke_tracers
{
void init();
void teardown();
void init(ContextId context_id);
void teardown(ContextId context_id);
void connect(smoke_tracers_connect_callback callback);
void update_instances();
void get_instances(ContextId context_id, Sbuffer *&instances, Sbuffer *&instance_count);
void set_source_buffers(Sbuffer *tracer_buf, Sbuffer *dynamic_buf, Sbuffer *verts_buf);
} // namespace smoke_tracers

namespace particles
{
void init();
void teardown();
void ensure_capacity(int fx_max, int smoke_tracer_max);
int get_fx_capacity();
} // namespace particles

namespace lru_collision
{
void teardown(ContextId context_id);
void on_unload_scene(ContextId context_id);
void update(ContextId context_id, const Point3 &camera_pos);
const dag::Vector<NativeInstance> &get_instances(ContextId context_id, const Point3 &camera_pos);
void bind_resources(ContextId context_id);
} // namespace lru_collision

bool use_batched_skinned_vertex_processor = false;
bool is_in_lost_device_state = false;
bool bvh_use_hair = true;

elem_rules_fn elem_rules = nullptr;
screenshot_fn screenshot_function = nullptr;

dag::AtomicInteger<uint32_t> bvh_id_gen = 0;

float mip_range = 1000;
float mip_scale = 10;

float max_water_distance = 10.0f;
float water_fade_power = 3.0f;
float max_water_depth = 5.0f;
float rtr_max_water_depth = 0.2f;

float debug_min_t = 0;

bool delay_sync = false;

bool bvh_prioritize_compactions = false;
bool bvh_use_fast_tlas_build = false;

static void copyHwInstancesCpu(void *dst, const NativeInstance *src, size_t instance_count)
{
#if _TARGET_APPLE
  d3d::driver_command(Drv3dCommand::CONVERT_TLAS_INSTANCES, (void *)dst, (void *)src, (void *)instance_count);
#else
  memcpy(dst, src, sizeof(NativeInstance) * instance_count);
#endif
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
    args.getSplineDataFn = instance->getSplineDataFn;
    args.tree = instance->tree;
    args.flag = instance->flag;
    args.skin = instance->skin;
  }
  args.tree.ppPositionBindless = mesh.ppPositionBindless;
  args.tree.ppDirectionBindless = mesh.ppDirectionBindless;
  args.skin.clothWind.clothNoiseCombinedTexBindless = mesh.clothNoiseCombinedTexBindless;
  return args;
}

static bool process(ContextId context_id, Sbuffer *source_buffer, int source_buffer_offset, uint32_t source_buffer_bindless,
  UniqueOrReferencedBVHBuffer &processedBuffer, uint32_t &bindless_id, const BufferProcessor *processor,
  BufferProcessor::ProcessArgs &args, bool skipProcessing, bool &need_blas_build)
{
  bool didProcessing = false;
  if (processedBuffer.needAllocation() || !processor->isOneTimeOnly())
  {
    need_blas_build |= processor->process(context_id, source_buffer, source_buffer_offset, source_buffer_bindless, processedBuffer,
      bindless_id, args, skipProcessing);
    didProcessing = true;
  }

  HANDLE_LOST_DEVICE_STATE(processedBuffer.isAllocated(), false);

  return didProcessing;
}

static void process_instance_vertices(ContextId context_id, uint64_t object_id, const Mesh &mesh, const Context::Instance *instance,
  const Frustum *frustum, vec4f_const view_pos, vec4f_const light_direction, UniqueOrReferencedBVHBuffer &transformed_vertices,
  bool processed_vertices_recycled, bool &need_blas_build, MeshMeta &meta)
{
  if (mesh.vertexProcessor)
  {
    bool needProcessing = transformed_vertices.needAllocation();
    bool hasVertexAnimation = !mesh.vertexProcessor->isOneTimeOnly();
    if (hasVertexAnimation)
    {
      G_ASSERT(instance);
      G_ASSERT(frustum);

      if (instance->animationUpdateMode == Context::Instance::AnimationUpdateMode::FORCE_ON)
      {
        needProcessing = true;
      }
      else if (instance->animationUpdateMode == Context::Instance::AnimationUpdateMode::DO_CULLING)
      {
        mat44f tm44;
        v_mat43_transpose_to_mat44(tm44, instance->transform);
        vec4f localBounds = v_ldu(&mesh.boundingSphere.c.x);
        vec4f worldBounds = v_mat44_mul_bsph(tm44, localBounds);
        needProcessing = instance_needs_animation_broad_phase_with_distance_rate(worldBounds, *frustum, view_pos, light_direction);
      }
    }

    auto args = build_args(object_id, mesh, instance, processed_vertices_recycled);
    bool canProcess = mesh.vertexProcessor->isReady(args);

    // Only do the processing if we either has a per instance output to process into, or the
    // initial processing on the mesh is not yet done. Otherwise it would just process the same
    // mesh again and again for no reason.
    if (canProcess)
    {
      bool hadProcessedVertices = transformed_vertices.isAllocated();

      if (hadProcessedVertices && !delay_sync)
        d3d::resource_barrier(ResourceBarrierDesc(transformed_vertices.get(), RB_NONE));

      uint32_t bindlessIndex;
      if (process(context_id, mesh.geometry.getVertexBuffer(context_id), mesh.geometry.vbOffset, mesh.pvBindlessIndex,
            transformed_vertices, bindlessIndex, mesh.vertexProcessor, args, !needProcessing, need_blas_build))
      {
        meta.setTexcoordFormat(args.texcoordFormat);
        meta.startVertex = args.startVertex;
        meta.texcoordOffset = args.texcoordOffset;
        meta.normalOffset = args.normalOffset;
        meta.colorOffset = args.colorOffset;
        meta.vertexStride = args.vertexStride;
      }

      if (!hadProcessedVertices && transformed_vertices.isAllocated())
        meta.setVertexBufferIndex(bindlessIndex);
    }
  }
}

static void process_meta(ContextId context_id, MeshMeta &meta, const Mesh &mesh, bool &needBlasBuild,
  const Context::Instance &instance, const Frustum &frustum, vec4f_const cameraPos, vec4f_const lightDirection,
  UniqueOrReferencedBVHBuffer &animatedVertices, bool stationary, bool skipUpdate, const MeshMeta &baseMeta)
{
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
      meta.albedoTextureIndex = baseMeta.albedoTextureIndex;
      meta.normalTextureIndex = baseMeta.normalTextureIndex;
      meta.extraTextureIndex = baseMeta.extraTextureIndex;
    }
    meta.ahsVertexBufferIndex = baseMeta.ahsVertexBufferIndex;
    // Copy the index buffer index only, the vertex buffer is part of the instance
    meta.indexBufferIndex = baseMeta.indexBufferIndex;
    // make_mesh_opaque can clear the alpha test many frames after the instance metas exist, thus copy
    // the flag again here, as with the indices above.
    meta.materialType =
      (meta.materialType & ~MeshMeta::bvhMaterialAlphaTest) | (baseMeta.materialType & MeshMeta::bvhMaterialAlphaTest);
  }

  if ((!stationary && !skipUpdate) || !animatedVertices.get())
    process_instance_vertices(context_id, instance.objectId, mesh, &instance, &frustum, cameraPos, lightDirection, animatedVertices,
      verticesRecycled, needBlasBuild, meta);
  meta.vertexOffset = animatedVertices.getOffset();
}

static on_parallel_jobs_finished_callback on_parallel_jobs_finished_cb = nullptr;

namespace parallel_instance_processing
{
struct ParallelFinishResult
{
  bool MainUploadDone;
  bool PerInstanceDataUploadDone;
};

static constexpr int COPY_DONE_VALUE = -666;
static dag::AtomicInteger<int> jobGroupReleaseCounter;
static ParallelFinishResult jobGroupParallelFinishResult = {false, false};

static class PrebuildMetaJob : public cpujobs::IJob
{
  enum class PrebuildMetaJobState
  {
    IDLE,
    PREPARED,
    RUNNING,
  };
  ContextId contextId;
  PrebuildMetaJobState state = PrebuildMetaJobState::IDLE;
  Point3 cameraPos;
  Point3 lightDir;
  TMatrix itm;
  TMatrix4 projTm;

  void processInstance(vec4f vLightDir, Context::Instance &instance, const Frustum &frustum, vec4f_const viewPos)
    DAG_TS_REQUIRES(contextId->meshMetaAllocatorLock) DAG_TS_REQUIRES_SHARED(contextId->objectsLock)
  {
    if (contextId->halfBakedObjects.count(instance.objectId))
      return;
    auto iter = contextId->objects.find(instance.objectId);
    if (iter == contextId->objects.end())
      return;
    auto &object = iter->second;
    if (!object.hasVertexProcessor)
      return;

    const auto metaAllocId = MeshMetaAllocator::is_valid(instance.metaAllocId) ? instance.metaAllocId : object.metaAllocId;
    const auto baseMetaRegion = contextId->meshMetaAllocator.get(object.metaAllocId);
    auto metaRegion = contextId->meshMetaAllocator.get(metaAllocId);

    auto animatedVertices = UniqueOrReferencedBVHBuffer(*instance.uniqueTransformedBuffer); //-V595
    const bool needsProcessing = [&]() {
      for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
      {
        if (!mesh.vertexProcessor)
          continue;
        if (!ProcessorInstances::isVertexProcessorBatched(*mesh.vertexProcessor))
          return false;
        G_ASSERT(!mesh.vertexProcessor->isOneTimeOnly() && instance.uniqueTransformedBuffer);
        if (animatedVertices.needAllocation())
          return false;
      }
      return true;
    }();
    if (needsProcessing)
    {
      const bool stationary = instance.uniqueIsStationary;
      const bool skipUpdate = instance.animIndex > -1 ? !contextId->riGenUpdateSlots[instance.animIndex] : false;
      bool needBlasBuild = false;
      for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
      {
        if (!mesh.vertexProcessor)
          continue;
        process_meta(contextId, meta, mesh, needBlasBuild, instance, frustum, viewPos, vLightDir, animatedVertices, stationary,
          skipUpdate, baseMeta);
      }
      instance.alreadyProcessed = true;
      instance.needsBlasBuild = needBlasBuild;
    }
    else
    {
      instance.alreadyProcessed = false;
      instance.needsBlasBuild = false;
    }
  }

public:
  const char *getJobName(bool &) const override { return "PrebuildMetaJob"; }

  void doJob() override
  {
    auto vLightDir = v_ldu_p3_safe(&lightDir.x);
    auto itmRelative = itm;
    itmRelative.setcol(3, Point3::ZERO);
    auto frustumRelative = Frustum(TMatrix4(orthonormalized_inverse(itmRelative)) * projTm);
    auto frustumAbsolute = Frustum(TMatrix4(orthonormalized_inverse(itm)) * projTm);

    auto vCameraPos = v_ldu_p3_safe(&cameraPos.x);
    if (!bvh_disable_rigen_meta_prebuild)
    {
      TIME_PROFILE(rigen_meta_prepare);
      Context::BvhObjectReadLock objectsGuard(contextId->objectsLock);
      OSSpinlockScopedLock metaGuard(contextId->meshMetaAllocatorLock);
      for (auto &instances : contextId->riGenInstances)
        for (auto &instance : instances)
          processInstance(vLightDir, instance, frustumAbsolute, vCameraPos);
    }

    if (!bvh_disable_dynrend_meta_prebuild)
    {
      TIME_PROFILE(dynrend_meta_prepare);
      dyn::wait_dynrend_instances();
      dyn::wait_animchar_instances();
      Context::BvhObjectReadLock objectsGuard(contextId->objectsLock);
      OSSpinlockScopedLock metaGuard(contextId->meshMetaAllocatorLock);
      for (auto &instances : contextId->dynrendInstances)
        for (auto &instance : instances.second)
          processInstance(vLightDir, instance, frustumRelative, v_zero());
    }
  }

  void wait()
  {
    if (!delay_sync)
      return;
    TIME_PROFILE(wait_rigen_cache_build)
    threadpool::wait(this);
    if (state == PrebuildMetaJobState::RUNNING)
      state = PrebuildMetaJobState::IDLE;
  }

  void start()
  {
    if (!delay_sync)
      return;
    wait();
    G_ASSERT_RETURN(state == PrebuildMetaJobState::PREPARED, );
    state = PrebuildMetaJobState::RUNNING;
    if (bvh_disable_rigen_meta_prebuild && bvh_disable_dynrend_meta_prebuild)
      return;
    threadpool::add(this, threadpool::JobPriority::PRIO_NORMAL);
  }

  void prepare(ContextId ctx_id, const Point3 &view_position, const Point3 &light_direction, const TMatrix &in_itm,
    const TMatrix4 &in_projTm)
  {
    if (!delay_sync)
      return;
    wait();
    contextId = ctx_id;
    cameraPos = view_position;
    lightDir = light_direction;
    itm = in_itm;
    projTm = in_projTm;
    state = PrebuildMetaJobState::PREPARED;
  }
} prebuild_meta_job;

enum class TargetFrame
{
  Current,
  Next
};

static bool upload_main_data(ContextId context_id, TargetFrame target_frame)
{
  TIME_D3D_PROFILE(upload_main_tlas_data);

  int impostorCount = 0;
  for (auto &instances : context_id->impostorInstances)
    impostorCount += instances.size();

  int riExtraCount = 0;
  for (auto &instances : context_id->riExtraInstances)
    riExtraCount += instances.size();

  const int uploadCount = impostorCount + riExtraCount;
  if (uploadCount == 0)
    return true; // no instance, nothing to upload, all good

  auto tlasBuf = target_frame == TargetFrame::Next ? context_id->tlasUploadMain.getNextBuf() : context_id->tlasUploadMain.getBuf();

  const uint32_t HW_INSTANCE_SIZE = d3d::get_driver_desc().raytrace.topAccelerationStructureInstanceElementSize;

  if (!tlasBuf || tlasBuf->getSize() < uploadCount * HW_INSTANCE_SIZE)
    return false;

  auto upload = lock_sbuffer<uint8_t>(tlasBuf, 0, uploadCount * HW_INSTANCE_SIZE, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
  if (!upload)
    return false;

  auto cursor = upload.get();
  for (auto &instances : context_id->impostorInstances)
  {
    TIME_PROFILE(memcpy_impostors)

    copyHwInstancesCpu(cursor, instances.data(), instances.size());
    cursor += instances.size() * HW_INSTANCE_SIZE;
  }

  for (auto &instances : context_id->riExtraInstances)
  {
    TIME_PROFILE(memcpy_ri_extra)
    copyHwInstancesCpu(cursor, instances.data(), instances.size());
    cursor += instances.size() * HW_INSTANCE_SIZE;
  }

  return true;
}

static bool upload_per_instance_data(ContextId context_id, TargetFrame target_frame)
{
  TIME_D3D_PROFILE(upload_per_instance_data_heavy);

  int impostorCount = 0;
  for (auto &instances : context_id->impostorInstanceData)
    impostorCount += instances.size();

  int riExtraCount = 0;
  for (auto &instances : context_id->riExtraInstanceData)
    riExtraCount += instances.size();

  const int uploadCount = impostorCount + riExtraCount;
  if (uploadCount == 0)
    return true; // no instance, nothing to upload, all good

  auto perInstanceDataBuf =
    target_frame == TargetFrame::Next ? context_id->perInstanceData.getNextBuf() : context_id->perInstanceData.getBuf();

  if (!perInstanceDataBuf || perInstanceDataBuf->getSize() < uploadCount * sizeof(PerInstanceData))
    return false;

  auto upload =
    lock_sbuffer<uint8_t>(perInstanceDataBuf, 0, uploadCount * sizeof(PerInstanceData), VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE);
  if (!upload)
    return false;

  TIME_D3D_PROFILE(memcpy_per_instance_data);

  int offset = 0;
  for (auto &instanceData : context_id->impostorInstanceData)
  {
    memcpy(upload.get() + offset, instanceData.data(), instanceData.size() * sizeof(PerInstanceData));
    offset += instanceData.size() * sizeof(PerInstanceData);
  }
  for (auto &instanceData : context_id->riExtraInstanceData)
  {
    memcpy(upload.get() + offset, instanceData.data(), instanceData.size() * sizeof(PerInstanceData));
    offset += instanceData.size() * sizeof(PerInstanceData);
  }
  return true;
}

static void on_parallel_jobs_finished(ContextId context_id)
{
  if (bvh_disable_parallel_instance_processing_finish)
    return;

  // sanity check, they should always match:
  for (int i = 0; i < ri_gen_thread_count; i++)
    G_ASSERT(context_id->impostorInstances[i].size() == context_id->impostorInstanceData[i].size());
  for (int i = 0; i < ri_extra_thread_count; i++)
    G_ASSERT(context_id->riExtraInstances[i].size() == context_id->riExtraInstanceData[i].size());

  TIME_PROFILE(on_parallel_jobs_finished)

  parallel_instance_processing::prebuild_meta_job.start();

  if (on_parallel_jobs_finished_cb)
    on_parallel_jobs_finished_cb();

  bool mainUploadDone = upload_main_data(context_id, TargetFrame::Next);
  bool perInstanceDataUploadDone = upload_per_instance_data(context_id, TargetFrame::Next);
  jobGroupParallelFinishResult = ParallelFinishResult{mainUploadDone, perInstanceDataUploadDone};

  int prevValue = jobGroupReleaseCounter.exchange(COPY_DONE_VALUE);
  G_UNUSED(prevValue);
  G_ASSERT(prevValue == 0);
}

void start_frame(ContextId context_id)
{
  G_UNUSED(context_id);
  jobGroupReleaseCounter.store(1); // add +1, so we won't finish before adding all the jobs that can contribute
}

void finish_adding_jobs(ContextId context_id)
{
  int counter = jobGroupReleaseCounter.sub_fetch(1);
  if (counter == 0) // the processing threads finished extremely early, need to call the finish function directly
    on_parallel_jobs_finished(context_id);
}

void before_job_start(ContextId context_id)
{
  G_UNUSED(context_id);
  int counter = jobGroupReleaseCounter.add_fetch(1);
  G_UNUSED(counter);
  G_ASSERT(counter > 0);
}

void after_job_end(ContextId context_id)
{
  G_UNUSED(context_id);
  int counter = jobGroupReleaseCounter.sub_fetch(1);
  if (counter != 0)
    return;

  on_parallel_jobs_finished(context_id);
}

ParallelFinishResult is_parallel_jobs_finished(ContextId context_id)
{
  G_UNUSED(context_id);
  if (jobGroupReleaseCounter.load() != COPY_DONE_VALUE)
  {
    // for extra safety
    if (!bvh_disable_parallel_instance_processing_finish)
      logerr("[BVH] Parallel jobs not finished, this should never happen! It will cause reallocation and stuttering!");
    return ParallelFinishResult{false, false};
  }

  return jobGroupParallelFinishResult;
}
} // namespace parallel_instance_processing

void set_on_parallel_jobs_finished_cb(on_parallel_jobs_finished_callback callback) { on_parallel_jobs_finished_cb = callback; }

struct BVHLinearBufferManager
{
  struct Buffer
  {
    Buffer(size_t alignment, size_t struct_size, size_t elem_count, uint32_t flags, const char *name) : alignment(alignment)
    {
      buffer = dag::create_sbuffer(struct_size, elem_count, flags, 0, name, RESTAG_BVH);
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
    if (!elems)
    {
      offset = 0;
      return nullptr;
    }

    if (elems > elemCount)
    {
      offset = 0;
      auto buffer = d3d::create_sbuffer(structSize, elems, flags, 0, String(0, "%s_large_%u", name, largeBuffers.size()), RESTAG_BVH);
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
  eastl::vector<Context::BLASCompaction *> compactions;
  ContextId contextId;

  static void start(eastl::vector<Context::BLASCompaction *> &&queue, ContextId context_id)
  {
    auto job = new CreateCompactedBLASJob;
    context_id->createCompactedBLASJobQueue.push_back().reset(job);
    job->compactions = eastl::move(queue);
    job->contextId = context_id;
    for (auto compaction : job->compactions)
      compaction->blasCreateJob = job;
    threadpool::add(job, threadpool::PRIO_NORMAL);
  }

  const char *getJobName(bool &) const override { return "CreateCompactedBLASJob"; }
  void doJob() override
  {
    for (auto compaction : compactions)
    {
      compaction->compactedBlas = UniqueBLAS::create(compaction->compactedSizeValue);
      interlocked_release_store_ptr(compaction->blasCreateJob, static_cast<cpujobs::IJob *>(nullptr));
    }
  }
};

static struct BVHUploadMetaJob : public cpujobs::IJob
{
  ContextId contextId;

  void start(ContextId context_id)
  {
    contextId = context_id;

    int metaCount;
    {
      OSSpinlockScopedLock metaGuard(contextId->meshMetaAllocatorLock); // for safety, it should never block
      metaCount = contextId->meshMetaAllocator.size();
    }

    // a context with no mesh content ever added (collision-only) has no metas
    // and no buffer to lock; the waits are no-ops for a job that was not added
    if (!metaCount)
      return;

    if (contextId->meshMeta && contextId->meshMeta->getNumElements() < metaCount)
      contextId->meshMeta.close();

    // a failed allocation returns inside the macro as device loss, the library's
    // uniform policy for every allocation in the build
    if (!contextId->meshMeta)
      HANDLE_LOST_DEVICE_STATE(contextId->meshMeta.allocate(sizeof(MeshMeta), metaCount, SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED,
                                 "bvh_meta", contextId), );

    threadpool::add(this, threadpool::JobPriority::PRIO_HIGH);
  }
  void doJob() override
  {
    OSSpinlockScopedLock metaGuard(contextId->meshMetaAllocatorLock); // for safety, it should never block
    if (auto upload = lock_sbuffer<MeshMeta>(contextId->meshMeta.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_DISCARD))
    {
      auto dst = upload.get();
      int poolIndex = 0;
      while (auto src = contextId->meshMetaAllocator.data(poolIndex++))
      {
        memcpy(dst, src, MeshMetaAllocator::PoolSize * sizeof(MeshMeta));
        dst += MeshMetaAllocator::PoolSize;
      }
    }
  }
  const char *getJobName(bool &) const override { return "BVHUploadMetaJob"; }
  void wait()
  {
    TIME_PROFILE(wait_bvh_upload_meta_job)
    threadpool::wait(this);
  }
} bvh_upload_meta_job;

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
  auto memoryRequiredKB = gfx->getInt("bvhMinRequiredMemoryGB", 5) * 1100 * 950;        // intentionally not 1024
  auto memoryRequiredUMAKB = gfx->getInt("bvhMinRequiredMemoryGBUMA", 15) * 1100 * 950; // intentionally not 1024

  auto &drvDesc = d3d::get_driver_desc();
  if (drvDesc.caps.hasRayQuery && drvDesc.info.isUMA)
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

namespace var
{
static ShaderVariableInfo object_tess_factor("object_tess_factor", true);
}

bool is_global_object_tessellation_enabled() { return var::object_tess_factor.get_float() > 0; }

bool is_available() { return is_available_verbose() == BvhAvailabilityCode::AVAILABLE; }

BvhAvailabilityCode is_available_verbose()
{
  if (dgs_get_settings()->getBlockByNameEx("gameplay")->getBool("enableVR", false))
  {
    logonce("bvh::is_available is failed because VR is enabled.");
    return BvhAvailabilityCode::VR_ENABLED;
  }
  if (!d3d::get_driver_desc().caps.hasRayQuery)
  {
    logonce("bvh::is_available is failed because of no ray query support.");
    return BvhAvailabilityCode::NOT_SUPPORTED;
  }
  if (!d3d::get_driver_desc().caps.hasBindless)
  {
    logonce("bvh::is_available is failed because of no bindless support.");
    return BvhAvailabilityCode::NOT_SUPPORTED;
  }
  if (!denoiser::is_available())
  {
    logonce("bvh::is_available is failed because of no denoiser support.");
    return BvhAvailabilityCode::NOT_SUPPORTED;
  }
#if _TARGET_SCARLETT || _TARGET_C2
  static bool hasEnoughVRAM = true;
#elif _TARGET_PC
  if (!d3d::is_inited())
    return BvhAvailabilityCode::AVAILABLE;
  static bool hasEnoughVRAM = has_enough_vram_for_rt_initial_check();
#else
  static bool hasEnoughVRAM = false;
#endif
  if (!hasEnoughVRAM)
  {
    logonce("bvh::is_available is failed because of not enough VRAM.");
    return BvhAvailabilityCode::NOT_ENOUGH_MEMORY;
  }

  static bool isGPUValid = []() {
    if (
      const DataBlock *bvhBlacklistGPUBlk = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBlockByNameEx("bvh_blacklist_gpu"))
    {
      bool isValid = true;
      for (int i = 0; i < bvhBlacklistGPUBlk->paramCount(); i++)
      {
        isValid = isValid && strcmp(bvhBlacklistGPUBlk->getStr(i), d3d::get_device_name()) != 0;
      }
      return isValid;
    }
    return true;
  }();

  if (!isGPUValid)
  {
    logonce("bvh::is_available is failed because of gpu is not valid, check bvh gpu blacklist.");
    return BvhAvailabilityCode::BLACKLISTED_GPU;
  }
  logonce("bvh::is_available is success.");
  return BvhAvailabilityCode::AVAILABLE;
}

void init(elem_rules_fn elem_rules_init, screenshot_fn screenshot, AdditionalSettings settings)
{
  delay_sync = d3d::get_driver_code().is(d3d::vulkan) || d3d::get_driver_code().is(d3d::ps5) || d3d::get_driver_code().is(d3d::dx12);
  bvh_prioritize_compactions = settings.prioritizeCompactions;
  bvh_use_fast_tlas_build = settings.useFastTlasBuild;
  bvh_use_hair = settings.enableHair;
  set_omm_settings(settings);

  elem_rules = elem_rules_init;
  screenshot_function = screenshot;
  use_batched_skinned_vertex_processor = settings.batchedSkinning;

  scratch_buffer_manager.alignment = d3d::get_driver_desc().raytrace.accelerationStructureBuildScratchBufferOffsetAlignment;

  terrain::init();
  ri::init(settings);
  dyn::init(settings);
  gobj::init();
  grass::init();
  fx::init();
  smoke_tracers::init();
  particles::init();
  cables::init();
  binscene::init();
  fftwater::init();
  gpugrass::init();
}

static void wait_all_jobs();

void teardown(bool device_reset, bool zero_bvh_ids)
{
  wait_all_jobs();
  terrain::teardown();
  ri::teardown(device_reset);
  dyn::teardown(device_reset, zero_bvh_ids);
  gobj::teardown();
  grass::teardown();
  fx::teardown();
  smoke_tracers::teardown();
  particles::teardown();
  cables::teardown();
  binscene::teardown();
  fftwater::teardown();
  gpugrass::teardown();
  debug::teardown();
#if DAGOR_DBGLEVEL > 0
  debug::tlas_debug_teardown();
#endif
  hwInstanceCopyShader.reset();
  ProcessorInstances::teardown();
  scratch_buffer_manager.teardown();
  transform_buffer_manager.teardown();

  is_in_lost_device_state = false;
}

void enable_dynamic_planar_decals(bool enable) { dyn::enable_dynamic_planar_decals(enable); }

ContextId create_context(const char *name, Features features, Features designated_dyn_features)
{
  G_ASSERT(name && *name);
  G_ASSERTF((designated_dyn_features & ~Features::AnyDynrend) == 0,
    "BVH: create_context: designated_dyn_features (0x%x) must be a subset of Features::AnyDynrend", unsigned(designated_dyn_features));

  auto context_id = new Context();
  context_id->name = name;
  context_id->features = features;
  context_id->designatedDynFeatures = designated_dyn_features;
  context_id->ommEnabled = init_omm_context(context_id);

  logdbg("[BVH] ommEnabled is %s", context_id->ommEnabled ? "true" : "false");

  static int bvh_has_omm_supportVarId = get_shader_variable_id("bvh_has_omm_support", true);
  ShaderGlobal::set_int(bvh_has_omm_supportVarId, context_id->ommEnabled ? 1 : 0);

  terrain::init(context_id);
  ri::init(context_id);
  dyn::init(context_id);
  grass::init(context_id);
  fx::init(context_id);
  smoke_tracers::init(context_id);
  gpugrass::init(context_id);
  if (context_id->hasAny(Features::Cable))
    cables::init(context_id);
  dagdp::init(context_id);
  debug::init(context_id);
  return context_id;
}

bool has_features(ContextId context_id, uint32_t features)
{
  if (!context_id)
    return false;
  return context_id->hasAny(features);
}

static void release_camo_textures(ContextId context_id)
{
  for (auto &texture : context_id->camoTextures)
    context_id->releaseTexture(TEXTUREID(texture.first));
  context_id->camoTextures.clear();
}

static void release_process_buffers(ContextId context_id)
{
  for (auto &[alloc, _] : context_id->processBufferAllocator)
    context_id->releaseBuffer(alloc.getHeap().getBuf());
  context_id->processBufferAllocator.clear();
}

void teardown(ContextId &context_id)
{
  if (context_id == InvalidContextId)
    return;

  wait_all_jobs();
  terrain::teardown(context_id);
  ri::teardown(context_id);
  dyn::teardown(context_id);
  gobj::teardown(context_id);
  grass::teardown(context_id);
  fx::teardown(context_id);
  smoke_tracers::teardown(context_id);
  gpugrass::teardown(context_id);
  if (context_id->hasAny(Features::Cable))
    cables::teardown(context_id);
  dagdp::teardown(context_id);
  splinegen::teardown(context_id);
  lru_collision::teardown(context_id);
  debug::teardown(context_id);

  fftwater::on_unload_scene(context_id);
  release_camo_textures(context_id);
  release_process_buffers(context_id);

  {
    Context::BvhObjectWriteLock objectsGuard(context_id->objectsLock);
    for (auto &[object_id, object] : context_id->objects)
      object.teardown(context_id, object_id);
    for (auto &[object_id, object] : context_id->impostors)
      object.teardown(context_id, object_id);
  }

  context_id->releaseUnavailableTextures();

  G_ASSERT(context_id->usedTextures.empty());
  G_ASSERT(context_id->usedBuffers.empty());

  delete context_id;

  context_id = InvalidContextId;
}

static void destroy_object(ContextId context_id, uint64_t object_id) DAG_TS_REQUIRES(context_id->objectsLock)
{
  TIME_PROFILE(bvh::do_remove_object);

  context_id->cancelCompaction(object_id);

  auto iter = context_id->objects.find(object_id);
  if (iter != context_id->objects.end())
  {
    DA_PROFILE_TAG(bvh::do_remove_object, "object");
    iter->second.teardown(context_id, object_id);
    context_id->objects.erase(iter);
  }
  else
  {
    iter = context_id->impostors.find(object_id);
    if (iter != context_id->impostors.end())
    {
      DA_PROFILE_TAG(bvh::do_remove_object, "impostor");
      iter->second.teardown(context_id, object_id);
      context_id->impostors.erase(iter);
    }
  }

  context_id->halfBakedObjects.erase(object_id);
}

// Dropping a queued add must also drop the object's OMM texture-wait refs, or the wait bookkeeping
// (and the managed-texture references it holds) leaks. Returns the next iterator so loop callers keep
// the erase-in-place idiom. The release is a no-op when the object has no waits registered.
static eastl::unordered_map<uint64_t, eastl::pair<uint32_t, ObjectInfo>>::iterator erase_pending_object_add(ContextId context_id,
  eastl::unordered_map<uint64_t, eastl::pair<uint32_t, ObjectInfo>>::iterator iter)
  DAG_TS_REQUIRES(context_id->pendingObjectActionsLock)
{
  release_omm_texture_waits_for_object(context_id, iter->first);
  return context_id->pendingObjectAddActions.erase(iter);
}

static void remove_all_dyn_objects(ContextId context_id)
{
  Context::BvhObjectWriteLock objectsGuard(context_id->objectsLock);
  OSSpinlockScopedLock pendingLock(context_id->pendingObjectActionsLock);

  for (auto iter = context_id->pendingObjectAddActions.begin(); iter != context_id->pendingObjectAddActions.end();)
  {
    if (iter->second.second.type == BvhType::Dyn)
      iter = erase_pending_object_add(context_id, iter);
    else
      ++iter;
  }

  FRAMEMEM_REGION;

  dag::Vector<uint64_t, framemem_allocator> dynObjectIds;
  for (const auto &[object_id, object] : context_id->objects)
    if (object.type == BvhType::Dyn)
      dynObjectIds.push_back(object_id);

  for (uint64_t object_id : dynObjectIds)
    destroy_object(context_id, object_id);
}

void enable_dyn_models(ContextId context_id)
{
  if (context_id == InvalidContextId)
    return;

  const bool alreadyEnabled = context_id->hasAll(context_id->designatedDynFeatures);
  if (alreadyEnabled)
    return;

  wait_all_jobs();

  context_id->features = Features(context_id->features | context_id->designatedDynFeatures);
  dyn::init(context_id);
}

void disable_dyn_models(ContextId context_id)
{
  if (context_id == InvalidContextId)
    return;

  const bool alreadyDisabled = !context_id->hasAny(context_id->designatedDynFeatures);
  if (alreadyDisabled)
    return;

  wait_all_jobs();

  dyn::teardown(context_id);
  remove_all_dyn_objects(context_id);
  context_id->freeUniqueSkinBLASes.clear();
  release_camo_textures(context_id);
  context_id->features = Features(context_id->features & ~context_id->designatedDynFeatures);
}

void start_frame()
{
  ri::debug_update();
  dyn::debug_update();
  scratch_buffer_manager.startNewFrame();
  transform_buffer_manager.startNewFrame();
}

void add_instance(ContextId context_id, uint64_t object_id, mat43f_cref transform) DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  add_instance(context_id, context_id->genericInstances, object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    MeshMetaAllocator::INVALID_ALLOC_ID, false);
}

void update_instances_impl(ContextId bvh_context_id, const Point3 &view_position, const Point3 &lod_anchor_position,
  const Point3 &light_direction, const TMatrix &itm, const TMatrix4 &projTm, const Frustum &bvh_frustum, const Frustum &view_frustum,
  dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id,
  const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate,
  dag::Vector<DynamicRenderableSceneInstance *> &&og_instances, threadpool::JobPriority prio)
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
  for (auto &instances : bvh_context_id->riExtraFlagInstances)
    instances.clear();
  for (auto &instances : bvh_context_id->impostorInstances)
    instances.clear();
  for (auto &data : bvh_context_id->impostorInstanceData)
    data.clear();

  bvh_context_id->riGenStartIndexType = (bvh_context_id->riGenStartIndexType + 1) % Context::MaxTreeAnimIndices;
  bvh_context_id->rebuildUpdateSlots();

  binscene::update_instances(bvh_context_id);
  splinegen::update_instances(bvh_context_id, view_position);

  if (bvh_context_id->hasAny(Features::AnyDynrend))
  {
    dyn::wait_tidy_up_skins();
    if (dynrend_iterate)
      dyn::update_animchar_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position,
        dynrend_iterate);
    else
      dyn::update_dynrend_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position,
        eastl::move(og_instances));
  }

  if (bvh_context_id->hasAny(Features::AnyRI))
  {
    ri::wait_tidy_up_trees();
    parallel_instance_processing::prebuild_meta_job.prepare(bvh_context_id, view_position, light_direction, itm, projTm);
    parallel_instance_processing::start_frame(bvh_context_id);
    ri::update_ri_gen_instances(bvh_context_id, ri_gen_visibilities, view_position, light_direction, view_frustum, prio);
    ri::update_ri_extra_instances(bvh_context_id, view_position, lod_anchor_position, bvh_frustum, view_frustum, light_direction,
      prio);
    parallel_instance_processing::finish_adding_jobs(bvh_context_id);
  }
}

void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Point3 &lod_anchor_position,
  const Point3 &light_direction, const TMatrix &itm, const TMatrix4 &projTm, const Frustum &bvh_frustum, const Frustum &view_frustum,
  dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id, RiGenVisibility *ri_gen_visibility,
  dag::Vector<DynamicRenderableSceneInstance *> &&og_instances, threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, lod_anchor_position, light_direction, itm, projTm, bvh_frustum, view_frustum,
    dynrend_context_id, dynrend_no_shadow_context_id, {ri_gen_visibility}, nullptr, eastl::move(og_instances), prio);
}

// daNetGame doesn't use dynrend, but these can be used to identify contexts in BVH
static dynrend::ContextId bvh_dynrend_context = dynrend::ContextId::INVALID_BVH;
static dynrend::ContextId bvh_dynrend_no_shadow_context = dynrend::ContextId::INVALID_BVH_NO_SHADOWS;
void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Point3 &light_direction, const TMatrix &itm,
  const TMatrix4 &projTm, const Frustum &bvh_frustum, const Frustum &view_frustum,
  const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate, threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, view_position, light_direction, itm, projTm, bvh_frustum, view_frustum,
    &bvh_dynrend_context, &bvh_dynrend_no_shadow_context, ri_gen_visibilities, dynrend_iterate, {}, prio);
}

void wait_dynamic_instances_jobs()
{
  dyn::wait_dynrend_instances();
  dyn::wait_animchar_instances();
}

static __forceinline bool need_winding_flip(Mesh &mesh, const Context::Instance &instance)
{
  if (!mesh.needWindingFlip.has_value())
    mesh.needWindingFlip = need_winding_flip(instance.transform);
  return mesh.needWindingFlip.value();
}

static void process_mesh_vertices(ContextId context_id, uint64_t object_id, Mesh &mesh,
  UniqueOrReferencedBVHBuffer &transformed_vertices, bool &need_blas_build, MeshMeta &meta)
{
  if (mesh.vertexProcessor)
  {
    auto args = build_args(object_id, mesh, nullptr, false);
    bool canProcess = mesh.vertexProcessor->isReady(args);

    // Only do the processing if we either has a per instance output to process into, or the
    // initial processing on the mesh is not yet done. Otherwise it would just process the same
    // mesh again and again for no reason.
    if (canProcess && transformed_vertices.needAllocation())
    {
      bool needProcessing = transformed_vertices.needAllocation();
      bool hadProcessedVertices = transformed_vertices.isAllocated();

      uint32_t bindlessIndex;
      if (process(context_id, mesh.geometry.getVertexBuffer(context_id), mesh.geometry.vbOffset, mesh.pvBindlessIndex,
            transformed_vertices, bindlessIndex, mesh.vertexProcessor, args, !needProcessing, need_blas_build))
      {
        d3d::resource_barrier(ResourceBarrierDesc(transformed_vertices.get(), bindlessSRVBarrier));

        // Mesh should only change if this is not an animating mesh
        mesh.positionOffset = args.positionOffset;

        meta.setTexcoordFormat(args.texcoordFormat);
        meta.startVertex = args.startVertex;
        meta.texcoordOffset = args.texcoordOffset;
        meta.normalOffset = args.normalOffset;
        meta.colorOffset = args.colorOffset;
        meta.vertexStride = args.vertexStride;
      }

      if (!hadProcessedVertices && transformed_vertices.isAllocated())
        mesh.pvBindlessIndex = bindlessIndex;

      meta.setVertexBufferIndex(mesh.pvBindlessIndex);
    }
  }
}


static void process_ahs_vertices(ContextId context_id, Mesh &mesh, MeshMeta &meta)
{
  if (!context_id->ommEnabled && mesh.materialType & MeshMeta::bvhMaterialAlphaTest)
  {
    G_ASSERT(mesh.indexFormat);
    G_ASSERT(mesh.indexCount);

    uint32_t bindlessIndex;
    if (meta.materialType & MeshMeta::bvhMaterialImpostor)
    {
      int8_t texcoordOffset = meta.texcoordOffset;
      int8_t colorOffset = meta.colorOffset;
      int8_t vertexStride = meta.vertexStride;

      G_ASSERT(texcoordOffset > -1);
      G_ASSERT(colorOffset > -1);
      G_ASSERT(vertexStride > -1);

      ProcessorInstances::getAHSProcessor().process(context_id, mesh.geometry, mesh.ahsVertices, bindlessIndex, mesh.indexFormat,
        mesh.indexCount, texcoordOffset, VSDT_FLOAT2, vertexStride, colorOffset);
    }
    else
    {
      G_ASSERT(mesh.vertexStride);
      G_ASSERT(mesh.texcoordOffset != MeshInfo::invalidOffset);
      G_ASSERT(mesh.texcoordFormat);

      ProcessorInstances::getAHSProcessor().process(context_id, mesh.geometry, mesh.ahsVertices, bindlessIndex, mesh.indexFormat,
        mesh.indexCount, mesh.texcoordOffset, mesh.texcoordFormat, mesh.vertexStride, -1);
    }

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

enum class DoAddObjectResult
{
  Failed,
  NoActionNeeded,
  Succeeded,
  WaitingForTexture,
};

static DoAddObjectResult do_add_object(ContextId context_id, uint64_t object_id, const ObjectInfo &object_info)
  DAG_TS_REQUIRES(context_id->objectsLock)
{
  OmmTextureWait ommTextureWait = OmmTextureWait::Ready;
  if (context_id->ommEnabled)
  {
    ommTextureWait = should_wait_for_omm_texture(context_id, object_id, object_info);
    if (ommTextureWait == OmmTextureWait::Wait)
      return DoAddObjectResult::WaitingForTexture;
  }

  auto &objectMap = (object_info.meshes.size() && object_info.meshes[0].isImpostor) ? context_id->impostors : context_id->objects;

#if DAGOR_DBGLEVEL > 0
  for (const auto &mesh : object_info.meshes)
    G_ASSERTF_RETURN(mesh.indices, DoAddObjectResult::Failed,
      "Not having indices is commented out from the shader code for performance. If this is really needed, lets discuss it.");
#endif

  // If a mesh has a BLAS, it is either non animated or it has it's vertex/index buffers already
  // copied/processed into its own buffer. So we don't care about any of the changes.
  auto iter = objectMap.find(object_id);
  if (iter != objectMap.end())
  {
    if (iter->second.blas)
      return DoAddObjectResult::NoActionNeeded;

    iter->second.teardown(context_id, object_id);
    context_id->cancelCompaction(object_id);
    context_id->halfBakedObjects.erase(object_id);
  }

  auto &object = (iter != objectMap.end()) ? iter->second : objectMap[object_id];
  object.tag = object_info.tag;
  object.assetName = object_info.assetName;
  object.meshes = decltype(object.meshes)(object_info.meshes.size());
  object.isAnimated = object_info.isAnimated;
  object.type = object_info.type;
  object.hasVertexProcessor =
    eastl::any_of(object_info.meshes.begin(), object_info.meshes.end(), [](const auto &m) { return m.vertexProcessor; });
  {
    object.ensureMetaAllocated(context_id, object_info.meshes.size());
    LockedMetaAccess lockedMeta(*context_id, object.metaAllocId);
    auto metaRegion = lockedMeta.span();
    uint32_t geometryIndex = 0;
    for (auto [info, mesh, meta] : zip(object_info.meshes, object.meshes, metaRegion))
    {
      mesh.albedoTextureId = info.albedoTextureId;
      mesh.alphaTextureId = info.alphaTextureId;
      mesh.normalTextureId = info.normalTextureId;
      mesh.extraTextureId = info.extraTextureId;
      mesh.ppPositionTextureId = info.ppPositionTextureId;
      mesh.ppDirectionTextureId = info.ppDirectionTextureId;
      mesh.clothNoiseCombinedTexTextureId = info.clothNoiseCombinedTexTextureId;
      mesh.indexCount = info.indexCount;
      mesh.indexFormat = info.indices->getFlags() & SBCF_INDEX32 ? 4 : 2;
      mesh.vertexCount = info.vertexCount;
      mesh.vertexStride = info.vertexSize;
      mesh.positionFormat = info.positionFormat;
      mesh.positionOffset = info.positionOffset;
      mesh.processedPositionFormat =
        info.vertexProcessor ? info.vertexProcessor->getOutputPositionFormat(info.positionFormat) : info.positionFormat;
      mesh.texcoordOffset = info.texcoordOffset;
      mesh.texcoordFormat = info.texcoordFormat;
      mesh.secTexcoordOffset = info.secTexcoordOffset;
      mesh.normalOffset = info.normalOffset;
      mesh.colorOffset = info.colorOffset;
      mesh.indicesOffset = info.indicesOffset;
      mesh.weightsOffset = info.weightsOffset;
      mesh.vertexProcessor = info.vertexProcessor;
      mesh.hasSecondaryGeometry = info.vertexProcessor && info.vertexProcessor->isGeneratingSecondaryVertices();
      const uint32_t meshGeometryIndex = geometryIndex;
      geometryIndex += mesh.hasSecondaryGeometry ? 2 : 1;
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
      mesh.isCamoNet = info.isCamoNet;
      mesh.hasColorMod = info.hasColorMod;
      memcpy(mesh.impostorOffsets, info.impostorOffsets, sizeof(mesh.impostorOffsets));
      if (info.isInterior)
        mesh.materialType = MeshMeta::bvhMaterialInterior;
      else if (info.isClipmap)
        mesh.materialType = MeshMeta::bvhMaterialTerrain;
      else if (info.isRiLandclass)
        mesh.materialType = MeshMeta::bvhMaterialLandclass;
      else if (info.isMonochrome)
        mesh.materialType = MeshMeta::bvhMaterialMonochrome;
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

      if (info.hasAnimcharDecals)
        mesh.materialType |= MeshMeta::bvhMaterialAnimcharDecals;

      if (object_info.type == BvhType::Dyn)
        mesh.materialType |= MeshMeta::bvhMaterialDynrend;

      // After the materialType flags: the publication reads them. The give-up precedes any bake, thus
      // the published entry is empty and dispatches nothing, and needs no delayed-sync handling.
      if (ommTextureWait == OmmTextureWait::GaveUp)
      {
        for (Mesh::OmmSlot &slot : mesh.ommSlots)
          fail_omm_slot(slot, Mesh::OmmFailure::AlphaTextureNeverLoaded);
        if (mesh_wants_omm(context_id, mesh))
        {
          publish_omm_debug_result(mesh, mesh.ommSlots[OMM_PRIMARY_SLOT], object_id, meshGeometryIndex, OMM_PRIMARY_SLOT, false);
          if (mesh.hasSecondaryGeometry)
            publish_omm_debug_result(mesh, mesh.ommSlots[OMM_SECONDARY_SLOT], object_id, meshGeometryIndex, OMM_SECONDARY_SLOT, false);
        }
      }

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
      meta.forceNonMetal = info.forceNonMetal;
      meta.hasColorMod = info.hasColorMod;
      meta.setIndexBufferIndex(0);
      meta.setVertexBufferIndex(0);

      meta.holdAlphaTex(context_id, mesh.alphaTextureId);
      meta.holdNormalTex(context_id, mesh.normalTextureId);
      meta.holdExtraTex(context_id, mesh.extraTextureId);
      meta.holdAlbedoTex(context_id, mesh.albedoTextureId);

      if (info.albedoTextureId != BAD_TEXTUREID)
      {
        mesh.albedoTextureLevel = D3dResManagerData::getLevDesc(info.albedoTextureId.index(), TQL_thumb);
        mark_managed_tex_lfu(info.albedoTextureId, mesh.albedoTextureLevel);
      }

      if (info.ppPositionTextureId != BAD_TEXTUREID)
        context_id->holdTexture(info.ppPositionTextureId, mesh.ppPositionBindless);
      if (info.ppDirectionTextureId != BAD_TEXTUREID)
        context_id->holdTexture(info.ppDirectionTextureId, mesh.ppDirectionBindless);
      if (info.clothNoiseCombinedTexTextureId != BAD_TEXTUREID)
        context_id->holdTexture(info.clothNoiseCombinedTexTextureId, mesh.clothNoiseCombinedTexBindless);

      if (mesh.materialType & MeshMeta::bvhMaterialAlphaTest && info.alphaTextureId == BAD_TEXTUREID)
      {
        // If we need alpha testing, lets set the albedo texture to the alpha texture.
        meta.alphaTextureIndex = meta.albedoTextureIndex;
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

      if (info.isRiLandclass)
      {
        meta.materialData1 = info.landclassMapping;
        meta.materialData2 = float4(eastl::bit_cast<float>(info.riLandclassIndex), 0.0, 0.0, 0.0);
      }

      if (info.isMonochrome)
      {
        meta.materialData1 = info.colorOverride;
        meta.materialData2 = info.monochromeData;
      }

      // Always process indices/vertices to be independent from the streaming system.

      G_ASSERT(mesh.indexFormat == 2);

      int ibDwordCount = (info.indexCount + 1) / 2;
      int vbDwordCount = (mesh.vertexStride * mesh.vertexCount + 3) / 4;

      bool forceUniqueGeomBuffer = !!info.vertexProcessor; // TODO: fix the root cause that breaks these

      {
        TIME_PROFILE(geometry_buffer_alloc);
        auto dwordCount = ibDwordCount + vbDwordCount;
        auto alloc = context_id->allocateSourceGeometry(dwordCount, forceUniqueGeomBuffer);

        mesh.geometry.heapIndex = alloc.heapIx;
        mesh.geometry.bindlessIndex = alloc.bindlessId;
        mesh.geometry.bufferRegion = alloc.region;
        mesh.geometry.ibOffset = context_id->getSourceBufferOffset(alloc.heapIx, alloc.region);
        mesh.geometry.vbOffset = mesh.geometry.ibOffset + ibDwordCount * 4;

        G_ASSERT(mesh.geometry.ibOffset % 4 == 0);
        G_ASSERT(mesh.geometry.vbOffset % 4 == 0);

        HANDLE_LOST_DEVICE_STATE(mesh.geometry, DoAddObjectResult::Failed);
      }

      {
        // Indices

        ProcessorInstances::getIndexProcessor().process(info.indices, mesh.geometry, mesh.indexFormat, mesh.indexCount,
          mesh.startIndex, mesh.startVertex, context_id);

        // Subtracts mesh.startVertex from all indices

        mesh.startIndex = meta.startIndex = mesh.geometry.ibOffset / mesh.indexFormat;
        mesh.piBindlessIndex = mesh.geometry.bindlessIndex;
        meta.setIndexBit(mesh.indexFormat);
        meta.setIndexBufferIndex(mesh.piBindlessIndex);

        // Vertices

        info.vertices->copyTo(mesh.geometry.getVertexBuffer(context_id), mesh.geometry.vbOffset,
          mesh.vertexStride * (mesh.baseVertex + mesh.startVertex), mesh.vertexStride * mesh.vertexCount);
        mesh.startVertex = 0;
        mesh.baseVertex = 0;
        meta.startVertex = 0;
        meta.vertexOffset = mesh.geometry.vbOffset;
        mesh.pvBindlessIndex = mesh.geometry.bindlessIndex;

        meta.setVertexBufferIndex(mesh.pvBindlessIndex);

        // Also transitions the vertex buffer (in fact the whole heap)
        d3d::resource_barrier(ResourceBarrierDesc(mesh.geometry.getIndexBuffer(context_id), bindlessSRVBarrier));
      }

      if (!info.isImpostor)
        process_ahs_vertices(context_id, mesh, meta);
    }
  }

  if (object_info.isAnimated && object_info.meshes[0].vertexProcessor == &ProcessorInstances::getTreeVertexProcessor())
  {
    if (auto iter = context_id->stationaryTreeBuffers.find(object_id); iter == context_id->stationaryTreeBuffers.end())
    {
      ReferencedTransformData data;
      data.metaAllocId = context_id->allocateMetaRegion(1, "stationaryTree");
      context_id->stationaryTreeBuffers.insert({object_id, eastl::move(data)});
    }
  }

  if (!object_info.isAnimated)
    context_id->halfBakedObjects.insert(object_id);

  return DoAddObjectResult::Succeeded;
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

  uint32_t order = context_id->pendingObjectActionOrderCounter.add_fetch(1);

  context_id->hasPendingObjectAddActions.store(true, dag::memory_order_relaxed);
  OSSpinlockScopedLock lock(context_id->pendingObjectActionsLock);
  if (auto iter = context_id->pendingObjectAddActions.find(object_id); iter != context_id->pendingObjectAddActions.end())
  {
    iter->second.first = order;
    iter->second.second = info;
  }
  else
    context_id->pendingObjectAddActions.insert({object_id, {order, eastl::move(info)}});
}

void remove_object(ContextId context_id, uint64_t object_id)
{
  OSSpinlockScopedLock lock(context_id->pendingObjectActionsLock);
  uint32_t order = context_id->pendingObjectActionOrderCounter.add_fetch(1);
  context_id->pendingObjectRemoveActions.insert({object_id, order});
}

void before_object_action(ContextId context_id, uint64_t object_id)
{
  OSSpinlockScopedLock lock(context_id->pendingObjectActionsLock);
  uint32_t order = context_id->pendingObjectActionOrderCounter.add_fetch(1);
  context_id->pendingObjectPreChangeActions.insert({object_id, order});
}

static int calc_mesh_add_budget(ContextId context_id, BuildBudget budget)
{
  const int initialMeshBudget = per_frame_blas_model_budget[int(budget)];
  if (!bvh_prioritize_compactions)
    return initialMeshBudget;

  const int compactionBudget = per_frame_compaction_budget[int(budget)];
  int meshBudget = compactionBudget;
  for (auto iter = context_id->blasCompactions.begin(); meshBudget > 0 && iter != context_id->blasCompactions.end(); iter++)
    if (iter->has_value())
      --meshBudget;
  return min(meshBudget, initialMeshBudget);
}

static void handle_pending_mesh_actions(ContextId context_id, BuildBudget budget) DAG_TS_REQUIRES(context_id->objectsLock)
{
  TIME_PROFILE(handle_pending_mesh_actions);

  for (auto ri : context_id->pendingStaticBLASRequestActions)
    ri::readdRendinst(context_id, ri);
  context_id->pendingStaticBLASRequestActions.clear();

  OSSpinlockScopedLock lock(context_id->pendingObjectActionsLock);

  {
    DA_PROFILE_TAG(handle_pending_mesh_actions, "%u pre change in queue", context_id->pendingObjectPreChangeActions.size());

    for (auto [object_id, order] : context_id->pendingObjectPreChangeActions)
    {
      // If there is an add action pending, and that is older than the change action, then we can
      // discard the add action, as it is not valid anymore and there is a new one coming
      if (auto iter = context_id->pendingObjectAddActions.find(object_id); iter != context_id->pendingObjectAddActions.end())
        if (iter->second.first < order)
          erase_pending_object_add(context_id, iter);

      context_id->cancelCompaction(object_id);
      context_id->halfBakedObjects.erase(object_id);
    }

    context_id->pendingObjectPreChangeActions.clear();
  }

  {
    DA_PROFILE_TAG(handle_pending_mesh_actions, "%u removal in queue", context_id->pendingObjectRemoveActions.size());

    for (auto [object_id, order] : context_id->pendingObjectRemoveActions)
    {
      destroy_object(context_id, object_id);

      // Only remove the add action itself if it is older than the remove action.
      if (auto iter = context_id->pendingObjectAddActions.find(object_id); iter != context_id->pendingObjectAddActions.end())
        if (iter->second.first < order)
          erase_pending_object_add(context_id, iter);
    }

    context_id->pendingObjectRemoveActions.clear();
  }

  DA_PROFILE_TAG(handle_pending_mesh_actions, "%u additions in queue", context_id->pendingObjectAddActions.size());

  int meshBudget = calc_mesh_add_budget(context_id, budget);

  // Texture-wait rechecks scan every candidate mesh, so they are budgeted separately from real
  // adds. Once spent, known waiters are skipped for the frame instead of ending the loop, so ready
  // entries behind them are not starved.
  const int waitBudget = meshBudget;

  int counter = 0;
  int waitCounter = 0;
  for (auto iter = context_id->pendingObjectAddActions.begin();
       iter != context_id->pendingObjectAddActions.end() && counter < meshBudget && !is_in_lost_device_state;)
  {
    TIME_PROFILE(handle_pending_mesh_action);

    if (waitCounter >= waitBudget && context_id->ommTextureWaitsByObject.count(iter->first))
    {
      ++iter;
      continue;
    }

    // When the mesh is already added, but not yet built, we need to remove it as the build data is not valid anymore.

    context_id->halfBakedObjects.erase(iter->first);
    switch (do_add_object(context_id, iter->first, iter->second.second))
    {
      case DoAddObjectResult::Succeeded:
        ++counter;
        iter = erase_pending_object_add(context_id, iter);
        break;
      case DoAddObjectResult::Failed:
      case DoAddObjectResult::NoActionNeeded: iter = erase_pending_object_add(context_id, iter); break;
      case DoAddObjectResult::WaitingForTexture:
        ++waitCounter;
        ++iter;
        break;
    }
  }

  context_id->hasPendingObjectAddActions.store(!context_id->pendingObjectAddActions.empty(), dag::memory_order_relaxed);
}

static constexpr uint32_t MAX_GEOMETRIES_PER_BLAS = 32;

Object *find_half_baked_object(ContextId context_id, uint64_t object_id)
{
  auto it = context_id->objects.find(object_id);
  if (it != context_id->objects.end())
    return &it->second;
  it = context_id->impostors.find(object_id);
  if (it != context_id->impostors.end())
    return &it->second;
  return nullptr;
}

// Drop objects that can never fit in a single BLAS before any baking stage runs. descCount is fixed
// for an object's mesh set, so such an object would only be rejected at the build pass -- but by then
// it may have wasted an OMM bake whose slot then hangs unconsumed. Rejecting up front avoids that.
static void drop_oversized_half_baked_objects(ContextId context_id) DAG_TS_REQUIRES(context_id->objectsLock)
{
  for (auto iter = context_id->halfBakedObjects.begin(); iter != context_id->halfBakedObjects.end();)
  {
    const uint64_t objectId = *iter;
    Object *objectPtr = find_half_baked_object(context_id, objectId);
    if (!objectPtr)
    {
      ++iter;
      continue;
    }
    Object &object = *objectPtr;

    size_t descCount = eastl::accumulate(object.meshes.begin(), object.meshes.end(), size_t(0),
      [](size_t count, const auto &mesh) { return count + (mesh.hasSecondaryGeometry ? 2 : 1); });
    if (descCount > MAX_GEOMETRIES_PER_BLAS)
    {
      logerr("BVH object <%s> (id %llX, %u meshes) has too many geometries for BLAS build: %u, max: %u; "
             "reduce the geometry/material count of this asset to fix it",
        object.tag ? object.tag : "?", objectId, uint32_t(object.meshes.size()), uint32_t(descCount), MAX_GEOMETRIES_PER_BLAS);
      // The mesh set can never fit, so this object will never get a BLAS. Remove it entirely instead of
      // only dropping the queue entry: leaving the Object in `objects` would let add_instances pick it
      // up (it skips only ids still in halfBakedObjects) and emit a TLAS instance with a null BLAS
      // address. Advance the iterator first; destroy_object erases the now-absent key itself (a no-op).
      iter = context_id->halfBakedObjects.erase(iter);
      destroy_object(context_id, objectId);
      continue;
    }
    ++iter;
  }
}

struct OmmBuildBatch
{
  OmmBuildInfos infos;
  OmmBuildResults results;
};

struct BlasBuildBatch
{
  dag::Vector<RaytraceGeometryDescription, framemem_allocator> geomDescriptors;
  dag::Vector<raytrace::BatchedBottomAccelerationStructureBuildInfo, framemem_allocator> blasBuildInfos;
  dag::Vector<Context::BLASCompaction *, framemem_allocator> pendingCompactions;
  // Destroy these only after the builds of this frame are submitted: destroy_object frees the bake
  // results that the staged OMM array builds point into.
  dag::Vector<uint64_t, framemem_allocator> objectsToDestroy;
  int blasCount = 0;
  int triangleCount = 0;
};

static void process_half_baked_meshes(ContextId context_id, int &budget) DAG_TS_REQUIRES(context_id->objectsLock)
{
  for (uint64_t objectId : context_id->halfBakedObjects)
  {
    if (budget <= 0 || is_in_lost_device_state)
      break;

    Object *objectPtr = find_half_baked_object(context_id, objectId);
    G_ASSERT(objectPtr);
    if (!objectPtr)
      continue;
    Object &object = *objectPtr;

    bool anyNeedsProcessing = false;
    for (const Mesh &mesh : object.meshes)
      if (mesh.buildStage == Mesh::BuildStage::NeedsProcessing)
      {
        anyNeedsProcessing = true;
        break;
      }
    if (!anyNeedsProcessing)
      continue;

    TIME_PROFILE(half_baked_process);
    OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock); // for safety, it should never block
    auto metaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);
    for (int i = 0; auto &mesh : object.meshes)
    {
      auto &meta = metaRegion[i];
      i += mesh.hasSecondaryGeometry ? 2 : 1;

      if (mesh.buildStage != Mesh::BuildStage::NeedsProcessing)
        continue;

      bool needBlasBuild = false;
      UniqueBVHBuffer transformedVertices;
      UniqueOrReferencedBVHBuffer pb(transformedVertices);
      process_mesh_vertices(context_id, objectId, mesh, pb, needBlasBuild, meta);

      CHECK_LOST_DEVICE_STATE();

      G_ASSERT(!mesh.vertexProcessor || mesh.vertexProcessor->isOneTimeOnly());
      mesh.vertexProcessor = nullptr;

      if (transformedVertices)
      {
        mesh.geometry.processedVertexBuffer.swap(transformedVertices);
        mesh.geometry.vbOffset = 0;
        meta.vertexOffset = 0;
        transformedVertices.reset();
      }

      if (meta.materialType & MeshMeta::bvhMaterialImpostor)
        process_ahs_vertices(context_id, mesh, meta);

      mesh.buildStage = mesh_wants_omm(context_id, mesh) ? Mesh::BuildStage::ResolvingOmm : Mesh::BuildStage::NeedsBlasBuild;
    }
    --budget;
  }
}

static void resolve_half_baked_omms(ContextId context_id, int &budget) DAG_TS_REQUIRES(context_id->objectsLock)
{
  if (context_id->ommEnabled)
    for (uint64_t objectId : context_id->halfBakedObjects)
    {
      if (budget <= 0 || is_in_lost_device_state)
        break;

      Object *objectPtr = find_half_baked_object(context_id, objectId);
      G_ASSERT(objectPtr);
      if (!objectPtr)
        continue;
      Object &object = *objectPtr;

      bool anyResolving = false;
      for (const Mesh &mesh : object.meshes)
        if (mesh.buildStage == Mesh::BuildStage::ResolvingOmm)
        {
          anyResolving = true;
          break;
        }
      if (!anyResolving)
        continue;

      TIME_PROFILE(half_baked_omm);
      OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);
      auto metaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);
      for (int i = 0; auto &mesh : object.meshes)
      {
        const uint32_t geometryIndex = i;
        auto &meta = metaRegion[i];
        i += mesh.hasSecondaryGeometry ? 2 : 1;

        if (mesh.buildStage != Mesh::BuildStage::ResolvingOmm)
          continue;

        OmmBakeSource ommSource = make_omm_bake_source(context_id, mesh, meta);
        // resolve_half_baked_omms runs outside any delayed-sync window
        if (start_new_omm_bakes(context_id, objectId, mesh, geometryIndex, ommSource, false))
          mesh.buildStage = Mesh::BuildStage::NeedsBlasBuild;
      }

      --budget;
    }
}

static BlasBuildBatch build_half_baked_blases(ContextId context_id, int &budget) DAG_TS_REQUIRES(context_id->objectsLock)
{
  BlasBuildBatch batch;
  batch.geomDescriptors.resize(MAX_GEOMETRIES_PER_BLAS * budget);
  // Each built object appends exactly one build and the loop is bounded by budget, so reserve up front
  // to avoid per-frame growth reallocations.
  batch.blasBuildInfos.reserve(budget);
  for (auto iter = context_id->halfBakedObjects.begin(); iter != context_id->halfBakedObjects.end() && budget > 0 &&
                                                         !is_in_lost_device_state &&
                                                         context_id->compactedSizeWritesInQueue < context_id->compactedSizeBufferSize;)
  {
    uint64_t objectId = *iter;
    Object *objectPtr = find_half_baked_object(context_id, objectId);
    if (!objectPtr)
    {
      // Should never happen: the id is queued but the object is gone. logerr (not just G_ASSERT, which
      // compiles out) so the dropped build is recorded in release builds too, where the object id is
      // the only lead for diagnosing the lost object.
      logerr("BVH: half-baked object id %llX is queued for BLAS build but no longer exists; dropping it", objectId);
      iter = context_id->halfBakedObjects.erase(iter);
      continue;
    }
    Object &object = *objectPtr;

    bool allReady = true;
    for (const Mesh &mesh : object.meshes)
      if (mesh.buildStage != Mesh::BuildStage::NeedsBlasBuild)
      {
        allReady = false;
        break;
      }
    if (!allReady)
    {
      ++iter;
      continue;
    }

    // Alpha-tested geometry with no OMM is invisible, thus remove the full object. A mesh that still
    // bakes does not come here: it stays in ResolvingOmm.
    if (context_id->ommEnabled)
    {
      const Mesh *missingOmm = nullptr;
      {
        // One meta for each mesh, as ensureMetaAllocated allocates them. Not the per-geometry index that
        // the descriptor loop below uses.
        OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);
        auto metaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);
        for (uint32_t meshIndex = 0; meshIndex < object.meshes.size(); ++meshIndex)
        {
          Mesh &mesh = object.meshes[meshIndex];
          if (!(mesh.materialType & MeshMeta::bvhMaterialAlphaTest) || mesh_omms_built(mesh))
            continue;
          if (mesh_should_be_opaque(mesh))
          {
            make_mesh_opaque(mesh, metaRegion[meshIndex]);
            continue;
          }
          if (mesh_should_be_skipped(mesh))
            continue;
          missingOmm = &mesh;
          break;
        }
      }
      if (missingOmm)
      {
        logerr("BVH: dropping alpha-tested object <%s> (%s, id %llX) from the BVH -- %s", object.assetName.resolve().c_str(),
          object.tag ? object.tag : "?", objectId, describe_missing_omm(context_id, *missingOmm).c_str());
        iter = context_id->halfBakedObjects.erase(iter);
        batch.objectsToDestroy.push_back(objectId);
        continue;
      }
    }

    TIME_PROFILE(half_baked_build);

    size_t descCount = eastl::accumulate(object.meshes.begin(), object.meshes.end(), size_t(0),
      [](size_t count, const auto &mesh) { return count + (mesh.hasSecondaryGeometry ? 2 : 1); });
    // Oversized objects are dropped up front by drop_oversized_half_baked_objects, before any baking
    // stage, so by the time an object reaches here the fit is guaranteed.
    G_ASSERT(descCount <= MAX_GEOMETRIES_PER_BLAS);

    RaytraceGeometryDescription *desc = &batch.geomDescriptors[MAX_GEOMETRIES_PER_BLAS * batch.blasCount];
    memset(desc, 0, sizeof(RaytraceGeometryDescription) * descCount);

    {
      OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock); // for safety, it should never block
      auto metaRegion = context_id->meshMetaAllocator.get(object.metaAllocId);
      for (int i = 0; auto &mesh : object.meshes)
      {
        auto &meta = metaRegion[i];
        const bool hasSecondaryGeometry = mesh.hasSecondaryGeometry;

        bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;
        const bool skipped = context_id->ommEnabled && mesh_should_be_skipped(mesh);

        desc[i].type = RaytraceGeometryDescription::Type::TRIANGLES;
        desc[i].data.triangles.vertexBuffer = mesh.geometry.getVertexBuffer(context_id);
        desc[i].data.triangles.indexBuffer = mesh.geometry.getIndexBuffer(context_id);
        desc[i].data.triangles.vertexCount = mesh.vertexCount;
        desc[i].data.triangles.vertexStride = meta.vertexStride;
        desc[i].data.triangles.vertexOffset = mesh.baseVertex;
        desc[i].data.triangles.vertexOffsetExtraBytes = mesh.positionOffset + mesh.geometry.vbOffset;
        desc[i].data.triangles.vertexFormat = mesh.processedPositionFormat;
        desc[i].data.triangles.indexCount = skipped ? 0 : mesh.indexCount;
        desc[i].data.triangles.indexOffset = meta.startIndex;
        desc[i].data.triangles.flags =
          (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
        if (context_id->ommEnabled && !skipped)
          set_omm_linkage(desc[i], mesh);

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
          HANDLE_LOST_DEVICE_STATE(desc[i].data.triangles.transformBuffer, batch);                                            //-V522
          desc[i].data.triangles.transformBuffer->updateDataWithLock(sizeof(m) * desc[i].data.triangles.transformOffset,      //-V522
            sizeof(m),                                                                                                        //-V522
            m, 0);
        }

        if (hasSecondaryGeometry)
        {
          desc[i + 1].type = RaytraceGeometryDescription::Type::TRIANGLES;
          desc[i + 1].data.triangles.transformBuffer = desc[i].data.triangles.transformBuffer;
          desc[i + 1].data.triangles.transformOffset = desc[i].data.triangles.transformOffset;
          desc[i + 1].data.triangles.vertexBuffer = mesh.geometry.getVertexBuffer(context_id);
          desc[i + 1].data.triangles.indexBuffer = mesh.geometry.getIndexBuffer(context_id);
          desc[i + 1].data.triangles.vertexCount = mesh.vertexCount;
          desc[i + 1].data.triangles.vertexOffset = mesh.vertexCount;
          desc[i + 1].data.triangles.vertexStride = meta.vertexStride;
          desc[i + 1].data.triangles.vertexFormat = mesh.processedPositionFormat;
          desc[i + 1].data.triangles.vertexOffsetExtraBytes = mesh.geometry.vbOffset;
          desc[i + 1].data.triangles.indexCount = skipped ? 0 : mesh.indexCount;
          desc[i + 1].data.triangles.indexOffset = meta.startIndex;
          desc[i + 1].data.triangles.flags =
            (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
          if (context_id->ommEnabled)
          {
            desc[i + 1].extraDataAvailableMask.hasOpacityMicroMapLinkage = false;
            if (!skipped)
              set_omm_linkage(desc[i + 1], mesh, OMM_SECONDARY_SLOT);
          }
        }

        batch.triangleCount += skipped ? 0 : mesh.indexCount / 3;

        i += hasSecondaryGeometry ? 2 : 1;
      }
    }

    raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
    buildInfo.flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

    Context::BLASCompaction *compaction = context_id->beginBLASCompaction(objectId, object.type);
    if (compaction)
    {
      buildInfo.flags |= RaytraceBuildFlags::ALLOW_COMPACTION;
      buildInfo.compactedSizeOutputBuffer = context_id->compactedSizeBuffer.get();
      buildInfo.compactedSizeOutputBufferOffsetInBytes = compaction->compactedSizeOffset * sizeof(uint64_t);
      batch.pendingCompactions.push_back(compaction);
      ++context_id->compactedSizeWritesInQueue;
    }

    object.blas = UniqueBLAS::create(desc, descCount, buildInfo.flags);
    HANDLE_LOST_DEVICE_STATE(object.blas, batch);
    if (object.type == BvhType::RI && object.blas)
      unitedvdata::riUnitedVdata.adjustBlasSize(object.blas.getASSize());
    else if (object.type == BvhType::Dyn && object.blas)
      unitedvdata::dmUnitedVdata.adjustBlasSize(object.blas.getASSize());

    buildInfo.geometryDesc = desc;
    buildInfo.geometryDescCount = descCount;
    buildInfo.scratchSpaceBuffer = alloc_scratch_buffer(object.blas.getBuildScratchSize(), buildInfo.scratchSpaceBufferOffsetInBytes);
    buildInfo.scratchSpaceBufferSizeInBytes = object.blas.getBuildScratchSize();

    raytrace::BatchedBottomAccelerationStructureBuildInfo batchedBuild;
    batchedBuild.as = object.blas.get();
    batchedBuild.basbi = buildInfo;
    batch.blasBuildInfos.push_back(batchedBuild);

    budget--;
    batch.blasCount++;

    iter = context_id->halfBakedObjects.erase(iter);
  }
  return batch;
}

void process_meshes(ContextId context_id, BuildBudget budget)
{
  if (!per_frame_processing_enabled)
  {
    logdbg("[BVH] Device is in reset mode.");
    return;
  }
  TIME_D3D_PROFILE(bvh_process_meshes);

  CHECK_LOST_DEVICE_STATE();

  FRAMEMEM_REGION;

  parallel_instance_processing::prebuild_meta_job.wait();

  Context::BvhObjectWriteLock objectsGuard(context_id->objectsLock); // for safety, it should never block

  handle_pending_mesh_actions(context_id, budget);

  const bool isCompactionCheap = is_blas_compaction_cheap();

  // Each baking step gets its own per-frame model budget so an early step (mesh preprocessing) can not
  // consume the whole budget and starve later steps (BLAS build) -- otherwise a frame could preprocess
  // a pile of meshes and build no BLASes at all. Budget per step is the pre-fine-tune value.
  int blasModelBudget = per_frame_blas_model_budget[int(budget)];

  auto getBlas = [&](Context::BLASCompaction &c) DAG_TS_REQUIRES(context_id->objectsLock) -> UniqueBLAS * {
    auto mesh = context_id->objects.find(c.objectId);
    if (mesh == context_id->objects.end())
    {
      mesh = context_id->impostors.find(c.objectId);
      if (mesh == context_id->impostors.end())
        return nullptr;
    }

    return &mesh->second.blas;
  };

  // Drop objects that can never fit a BLAS before any baking stage, so they never waste an OMM bake.
  drop_oversized_half_baked_objects(context_id);

  int processModelBudget = per_frame_blas_model_budget[int(budget)];
  process_half_baked_meshes(context_id, processModelBudget);
  // OMM baking MUST happen before BLAS building but AFTER mesh processing,
  // and furthermore, OMM baking can take multiple frames due to a readback,
  // so we are forced to use a state machine here, potentially leaving some meshes
  // in a "half-baked" state for a few frames until the OMM baking is complete.
  OmmBuildBatch ommBatch;
  consume_ready_omm_bakes(context_id, ommBatch.infos, ommBatch.results);
  int ommModelBudget = per_frame_blas_model_budget[int(budget)];
  resolve_half_baked_omms(context_id, ommModelBudget);
  BlasBuildBatch blasBatch = build_half_baked_blases(context_id, blasModelBudget);

  // Limit heavy compactions by the minimum of all other steps' remaining budget
  // to avoid frames where one step consumed its entire budget being swarmed with
  // compactions.
  int compactionsBudget = eastl::min({blasModelBudget, processModelBudget, ommModelBudget});

  if (!is_in_lost_device_state)
  {
    TIME_PROFILE(compaction_update);

    if (!ommBatch.infos.empty())
    {
      TIME_D3D_PROFILE(bvh_build_blas_with_omm);
      d3d::raytrace::build_acceleration_structure({
        .opacityMicroMapTriangleArrayBuilds = ommBatch.infos,
        .bottomBuilds = blasBatch.blasBuildInfos,
        .flushAfterOpacityMicroMapTriangleArrayBuilds = true,
        .flushAfterBottomBuild = true,
      });
    }
    else
    {
      d3d::raytrace::build_acceleration_structure({
        .bottomBuilds = blasBatch.blasBuildInfos,
        .flushAfterBottomBuild = true,
      });
    }
    release_omm_bake_build_inputs(ommBatch.results);

    // The two grass paths bake and build here, in the same GPU-dispatch context as the mesh builds above.
    // Their geometry is static, thus this runs one time.
    grass::process_omm(context_id);
    gpugrass::process_omm(context_id);

    for (auto compaction : blasBatch.pendingCompactions)
      if (compaction->stage == Context::BLASCompaction::Stage::Created)
        compaction->stage = Context::BLASCompaction::Stage::SizeQueried;

    if (context_id->compactedSizeQueryRunning)
    {
      if (d3d::get_event_query_status(context_id->compactedSizeQuery.get(), false))
      {
        context_id->compactedSizeQueryRunning = false;
#if _TARGET_C2

#else
        constexpr bool bufferReadback = true;
#endif
        void *values = nullptr;
        // on PS5 compactedSizeBuffer is not used and readback is not issued. Compacted size fetching happens in handling
        // BLASCompaction::Stage::SizeReceived case
        bool sizesValid = !bufferReadback;
        if (context_id->compactedSizeBufferReadback && context_id->compactedSizeBufferReadback->lock(0, 0, &values, VBLOCK_READONLY))
        {
          G_ASSERT(
            context_id->compactedSizeBufferReadback->getSize() == context_id->compactedSizeBufferValues.size() * sizeof(uint64_t));
          memcpy(context_id->compactedSizeBufferValues.data(), values, context_id->compactedSizeBuffer->getSize());
          context_id->compactedSizeBufferReadback->unlock();
          sizesValid = true;
        }

        uint32_t count = 0;
        uint32_t cancelledCount = 0;
        for (auto &compaction : context_id->blasCompactions)
          if (compaction.has_value())
          {
            if (compaction->stage == Context::BLASCompaction::Stage::SizeBeingRead)
            {
              // If for whatever reason, acquiring the size fails, drop the compaction. The
              // model will be used uncompacted.
              if (!sizesValid)
              {
                ++cancelledCount;
                context_id->blasCompactionsAccel.erase(compaction->objectId);
                compaction.reset();
                continue;
              }
              ++count;
              compaction->stage = Context::BLASCompaction::Stage::SizeReceived;
              if constexpr (bufferReadback)
                compaction->compactedSizeValue = context_id->compactedSizeBufferValues[compaction->compactedSizeOffset];
            }
          }

        if (cancelledCount > 0)
          logerr("[BVH] Compacted size readback failed, %u compactions cancelled", cancelledCount);

        if constexpr (bufferReadback)
        {
          G_ASSERTF(count <= context_id->compactedSizeBufferSize,
            "bvh: too much compactions was requested increase compactedSizeBufferSize above %u", count);
        }
      }
    }

    if (!blasBatch.pendingCompactions.empty() || context_id->compactedSizeWritesInQueue)
    {
      if (!context_id->compactedSizeQueryRunning)
      {
        if (context_id->compactedSizeBuffer && context_id->compactedSizeBufferReadback)
        {
          context_id->compactedSizeBuffer.get()->copyTo(context_id->compactedSizeBufferReadback.get());
          if (context_id->compactedSizeBufferReadback->lock(0, 0, static_cast<void **>(nullptr), VBLOCK_READONLY))
            context_id->compactedSizeBufferReadback->unlock();
        }

        d3d::issue_event_query(context_id->compactedSizeQuery.get());
        context_id->compactedSizeQueryRunning = true;
        context_id->compactedSizeWritesInQueue = 0;

        for (auto &compaction : context_id->blasCompactions)
          if (compaction.has_value())
          {
            if (compaction->stage == Context::BLASCompaction::Stage::SizeQueried)
              compaction->stage = Context::BLASCompaction::Stage::SizeBeingRead;
          }
      }
    }
  }

  blasBatch.blasBuildInfos.clear();
  ommBatch.infos.clear();
  ommBatch.results.clear();
  blasBatch.geomDescriptors.clear();

  // Safe only after the OMM array builds are submitted and their inputs released. objectsGuard is still
  // held, thus nothing sees an object between its removal from halfBakedObjects and its destruction.
  for (uint64_t objectId : blasBatch.objectsToDestroy)
    destroy_object(context_id, objectId);

  {
    TIME_PROFILE(blas_compactions);
    // Remove the empty compactions from the front
    for (auto iter = context_id->blasCompactions.begin(); iter != context_id->blasCompactions.end() && !iter->has_value();)
      iter = context_id->blasCompactions.erase(iter);

    eastl::vector<Context::BLASCompaction *> creationQueue;

    int activeCompactions = 0;
    int maxActiveCompactions = per_frame_compaction_budget[int(budget)];
    for (auto iter = context_id->blasCompactions.begin(); iter != context_id->blasCompactions.end() && !is_in_lost_device_state;)
    {
      // Inactive compactions are skipped. They will be removed eventually when they get to the front.
      if (!iter->has_value())
      {
        ++iter;
        continue;
      }

      auto cancelCompaction = [](ContextId context_id, Context::CompQueue::iterator &iter) {
        context_id->cancelCompaction(iter->value().objectId);
        ++iter;
      };

      auto &compaction = iter->value();

      switch (compaction.stage)
      {
        case Context::BLASCompaction::Stage::MovedFrom:
        case Context::BLASCompaction::Stage::Created: G_ASSERT(false); break;
        case Context::BLASCompaction::Stage::SizeQueried: break;
        case Context::BLASCompaction::Stage::SizeBeingRead: break;
        case Context::BLASCompaction::Stage::SizeReceived:
        {
#if _TARGET_C2





#else
#if DAGOR_DBGLEVEL > 0
          // In stub driver, data read from a locked buffer has no meaning, so the size has to be estimated instead
          if (DAGOR_UNLIKELY(d3d::is_stub_driver()))
          {
            // Emulation of dx12 beh -- compacted size was observed to be around 0.25 of initial
            compaction.compactedSizeValue = d3d::get_raytrace_acceleration_structure_size(getBlas(compaction)->get()) / 4;
          }
#endif
          // If the compaction size can't be right, drop the compaction of the object.
          if (UniqueBLAS *blas = getBlas(compaction);
              !blas || !compaction.compactedSizeValue || compaction.compactedSizeValue > blas->getASSize())
          {
            logerr("[BVH] Compacted size %u of BLAS %llx is invalid (original size: %u), cancelling compaction",
              compaction.compactedSizeValue, compaction.objectId, blas ? blas->getASSize() : 0u);
            cancelCompaction(context_id, iter);
            continue;
          }
#endif
          if (!compaction.compactedSizeValue) //-V547
          {
            cancelCompaction(context_id, iter);
            continue;
          }
          else if (context_id->numCompactionBlasesInFlight < maxActiveCompactions * 2)
          {
            creationQueue.push_back(&compaction);
            compaction.stage = Context::BLASCompaction::Stage::WaitingGPUTime;
            context_id->numCompactionBlasesInFlight++;
          }
          break;
        }
        case Context::BLASCompaction::Stage::WaitingGPUTime:
        {
          if (interlocked_acquire_load_ptr(compaction.blasCreateJob))
            break;

          if (!compaction.compactedBlas)
          {
            logwarn("[BVH] Failed to allocate compacted BLAS for %llx, cancelling compaction.", compaction.objectId);
            cancelCompaction(context_id, iter);
            continue;
          }

          if (activeCompactions >= maxActiveCompactions)
            break;

          if (isCompactionCheap || compactionsBudget > 0)
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

            ++activeCompactions;
            context_id->numCompactionBlasesInFlight--;

            compaction.stage = Context::BLASCompaction::Stage::WaitingCompaction;

            // Compacting is a heavy operation.
            if (!isCompactionCheap)
              compactionsBudget -= 1;
          }
          break;
        }
        case Context::BLASCompaction::Stage::WaitingCompaction:
        {
          if (d3d::get_event_query_status(compaction.query.get(), false))
          {
            if (auto blas = getBlas(compaction))
            {
              if (compaction.type == BvhType::RI)
              {
                unitedvdata::riUnitedVdata.adjustBlasSize(-int64_t(blas->getASSize()));
                unitedvdata::riUnitedVdata.adjustBlasSize(compaction.compactedBlas.getASSize());
              }
              else if (compaction.type == BvhType::Dyn)
              {
                unitedvdata::dmUnitedVdata.adjustBlasSize(-int64_t(blas->getASSize()));
                unitedvdata::dmUnitedVdata.adjustBlasSize(compaction.compactedBlas.getASSize());
              }
              blas->swap(compaction.compactedBlas);
            }

            cancelCompaction(context_id, iter);
            continue;
          }
          break;
        }
      }

      ++iter;
    }

    eastl::erase_if(context_id->createCompactedBLASJobQueue, [](const auto &job) { return interlocked_acquire_load(job->done); });

    if (!creationQueue.empty())
      CreateCompactedBLASJob::start(eastl::move(creationQueue), context_id);

    DA_PROFILE_TAG(blas_compactions, "%d of %d active compactions were processed, %d in flight", activeCompactions,
      (int)context_id->blasCompactionsAccel.size(), context_id->numCompactionBlasesInFlight);

    // This is frequently needed for debugging.
    // logdbg("%d of %d active compactions were processed - in flight: %d", activeCompactions,
    //  (int)context_id->blasCompactionsAccel.size(), context_id->numCompactionBlasesInFlight);
  }

  auto regGameTex = [&](int var_id, bvh::Context::BindlessTexHolder &holder, uint32_t *size = nullptr) {
    TEXTUREID texId = ShaderGlobal::get_tex(var_id);
    if (holder.texId != texId)
    {
      holder.close(context_id);
      holder.texId = texId;
      if (auto texture = context_id->holdTexture(texId, holder.bindlessTexture); texture && size)
      {
        TextureInfo info;
        texture->getinfo(info);
        *size = info.w;
      }
    }
  };

  static int paint_details_texVarId = get_shader_variable_id("paint_details_tex", true);
  static int grass_land_color_maskVarId = get_shader_variable_id("grass_land_color_mask", true);
  static int dynamic_mfd_texVarId = get_shader_variable_id("dynamic_mfd_tex", true);
  static int cache_tex0VarId = get_shader_variable_id("cache_tex0", true);
  static int indirection_texVarId = get_shader_variable_id("indirection_tex", true);
  static int cache_tex1VarId = get_shader_variable_id("cache_tex1", true);
  static int cache_tex2VarId = get_shader_variable_id("cache_tex2", true);
  static int last_clip_texVarId = get_shader_variable_id("last_clip_tex", true);
  static int dynamic_decals_atlasVarId = get_shader_variable_id("dynamic_decals_atlas", true);

  {
    TIME_PROFILE(regGameTex);
    regGameTex(dynamic_mfd_texVarId, context_id->dynamic_mfd_texBindless);
    regGameTex(paint_details_texVarId, context_id->paint_details_texBindless, &context_id->paintTexSize);
    regGameTex(grass_land_color_maskVarId, context_id->grass_land_color_maskBindless);
    regGameTex(cache_tex0VarId, context_id->cache_tex0Bindless);
    regGameTex(indirection_texVarId, context_id->indirection_texBindless);
    regGameTex(cache_tex1VarId, context_id->cache_tex1Bindless);
    regGameTex(cache_tex2VarId, context_id->cache_tex2Bindless);
    regGameTex(last_clip_texVarId, context_id->last_clip_texBindless);
    regGameTex(dynamic_decals_atlasVarId, context_id->dynamic_decals_atlasBindless);
  }

  {
    TIME_PROFILE(camo_texture_cleanup);
    for (auto iter = context_id->camoTextures.begin(); iter != context_id->camoTextures.end();)
      if (get_managed_res_refcount(TEXTUREID(iter->first)) < 2)
      {
        context_id->releaseTexture(TEXTUREID(iter->first));
        iter = context_id->camoTextures.erase(iter);
      }
      else
        ++iter;
  }

  static bool showBVHBuildEvents = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("showBVHBuildEvents", false);

  if (showBVHBuildEvents && blasBatch.triangleCount > 0)
    visuallog::logmsg(String(0, "The BVH build %d triangles for %d BLASes.", blasBatch.triangleCount, blasBatch.blasCount));
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

    TIME_D3D_PROFILE(copy_hwinstances);
    DA_PROFILE_TAG(copy_hwinstances, "%d instances", bufferSize);

    ShaderGlobal::set_int(bvh_hwinstance_copy_start_instanceVarId, startInstance);
    ShaderGlobal::set_int(bvh_hwinstance_copy_instance_slotsVarId, bufferSize);
    ShaderGlobal::set_int(bvh_hwinstance_copy_modeVarId, 0);

    d3d::set_buffer(STAGE_CS, source_const_no, instances);
    d3d::set_buffer(STAGE_CS, instance_count_const_no, instanceCount);
    d3d::set_rwbuffer(STAGE_CS, output_uav_no, uploadBuffer);

    get_hw_instance_copy_shader().dispatchThreads(bufferSize, 1, 1);
  }
}

static struct BVHFallbackUploadHeavyDataJob : public cpujobs::IJob
{
private:
  ContextId contextId;
  bool uploadMainData;
  bool uploadPerInstanceData;
  bool mainUploadDone = true;
  bool perInstanceDataUploadDone = true;

public:
  void start(ContextId context_id, bool upload_main_data, bool upload_per_instance_data)
  {
    G_ASSERT(upload_main_data || upload_per_instance_data);
    contextId = context_id;
    uploadMainData = upload_main_data;
    uploadPerInstanceData = upload_per_instance_data;
    mainUploadDone = true;
    perInstanceDataUploadDone = true;
    threadpool::add(this, threadpool::JobPriority::PRIO_HIGH, true);
  }
  void doJob() override
  {
    if (uploadMainData)
      mainUploadDone = parallel_instance_processing::upload_main_data(contextId, parallel_instance_processing::TargetFrame::Current);
    if (uploadPerInstanceData)
      perInstanceDataUploadDone =
        parallel_instance_processing::upload_per_instance_data(contextId, parallel_instance_processing::TargetFrame::Current);
  }
  const char *getJobName(bool &) const override { return "BVHFallbackUploadHeavyDataJob"; }
  // Results are valid after wait()
  bool isMainUploadDone() const { return mainUploadDone; }
  bool isPerInstanceDataUploadDone() const { return perInstanceDataUploadDone; }
  void wait()
  {
    TIME_PROFILE(wait_bvh_fallback_upload_heavy_data_job)
    threadpool::wait(this);
  }
} bvh_fallback_upload_heavy_data_job;

static bool is_tree_instance(const Context::Instance &instance)
{
  // so far this is only true for trees
  return instance.hasInstanceColor;
}

// get_reason is a function and not a string because making the reason resolves an asset name and formats
// strings: too costly for each frame that reports nothing.
template <typename F>
static void report_withheld_dynamic_omm(bool &already_logged, const Object &object, uint64_t object_id, F get_reason)
{
  if (already_logged)
    return;
  already_logged = true;
  logerr("BVH: withholding alpha-tested dynamic object <%s> (%s, id %llX) from the BVH -- %s", object.assetName.resolve().c_str(),
    object.tag ? object.tag : "?", object_id, get_reason().c_str());
}

static void add_instances(ContextId context_id, const Context::InstanceMap &instances, dag::Vector<NativeInstance> &outInstances,
  uint32_t group_mask, bool is_camera_relative, const char *name, const Point3 &light_direction, const Point3 &camera_pos,
  eastl::unordered_set<void *, eastl::hash<void *>, eastl::equal_to<void *>, framemem_allocator> &allBlasUpdatesAs,
  const Frustum &frustumAbsolute, const Frustum &frustumRelative, const BufferProcessor *assumed_buffer_processor,
  OmmBuildInfos &ommBuildInfos, OmmBuildResults &ommBuildResults) DAG_TS_REQUIRES(context_id->meshMetaAllocatorLock)
  DAG_TS_REQUIRES_SHARED(context_id->objectsLock)
{
  CHECK_LOST_DEVICE_STATE();

  TIME_PROFILE_NAME(addInstance, name);
  DA_PROFILE_TAG(addInstance, "Instance count: %d", instances.size());

  const auto lightDirection = v_ldu_p3_safe(&light_direction.x);
  auto &perInstanceData = context_id->perInstanceDataCpu;
  auto &blasUpdates = context_id->blasUpdates;
  auto &updateGeoms = context_id->updateGeoms;

  const auto &frustum = is_camera_relative ? &frustumRelative : &frustumAbsolute;
  const auto cameraPos = is_camera_relative ? v_zero() : v_ldu_p3_safe(&camera_pos.x);

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

    if (object.hasVertexProcessor)
    {
      const bool stationary = instance.uniqueIsStationary;
      bool needBlasBuild = instance.needsBlasBuild;

      if (context_id->ommEnabled)
      {
        bool withholdInstance = false;
        // The OMM slot is shared per mesh across this object's instances, so this re-polls once per
        // instance. That is cheap: after the bake completes each poll is just a state check, and while
        // baking it is a light readback-readiness probe, so a per-frame dedup is not worth the state.
        for (uint32_t meshIndex = 0; meshIndex < object.meshes.size(); ++meshIndex)
        {
          Mesh &mesh = object.meshes[meshIndex];
          auto &meta = metaRegion[meshIndex];
          if (!mesh.vertexProcessor)
            continue;
          if (!(mesh.materialType & MeshMeta::bvhMaterialAlphaTest))
            continue;
          if (!mesh_wants_omm(context_id, mesh))
          {
            withholdInstance = true;
            report_withheld_dynamic_omm(mesh.ommSlots[OMM_PRIMARY_SLOT].failureLogged, object, instance.objectId,
              [] { return String(omm_failure_text(Mesh::OmmFailure::NoAlphaSource)); });
            break;
          }
          // Do not fail the slot: it stays correct for each instance that the mesh-shared OMM covers.
          // TODO: bake for each (mesh, override texture) pair into a small cache, in place of the one
          // shared slot of the mesh. Only the cache and its eviction are missing: the BLAS of the
          // instance already links an OMM at its first build.
          if (!instance_can_use_mesh_omm(mesh, meta, baseMetaRegion[meshIndex]))
          {
            withholdInstance = true;
            report_withheld_dynamic_omm(mesh.ommSlots[OMM_PRIMARY_SLOT].alphaSourceOverrideLogged, object, instance.objectId,
              [] { return String(omm_failure_text(Mesh::OmmFailure::InstanceAlphaSourceOverride)); });
            break;
          }

          OmmBakeSource ommSource = make_omm_bake_source(context_id, mesh);
          // add_instances runs inside build's delayed-sync window when delay_sync is set
          consume_mesh_omm_bakes(context_id, instance.objectId, mesh, meshIndex, ommBuildInfos, ommBuildResults, delay_sync);
          if (!start_new_omm_bakes(context_id, instance.objectId, mesh, meshIndex, ommSource, delay_sync))
          {
            withholdInstance = true; // the bake continues, or no bake slot is free: this is temporary
            break;
          }
          // start_new_omm_bakes completed the bake work, thus a slot that is not Built here has failed.
          if (!mesh_omms_built(mesh))
          {
            if (mesh_should_be_opaque(mesh))
            {
              make_mesh_opaque(mesh, baseMetaRegion[meshIndex]);
              continue;
            }
            if (mesh_should_be_skipped(mesh))
              continue;
            withholdInstance = true;
            report_withheld_dynamic_omm(mesh.ommSlots[OMM_PRIMARY_SLOT].failureLogged, object, instance.objectId,
              [&] { return describe_missing_omm(context_id, mesh); });
            break;
          }
        }

        if (withholdInstance)
          continue;
      }

      auto &geoms = updateGeoms.emplace_back();

      if (!instance.alreadyProcessed) // prebuilt on a worker thread
      {
        const bool skipUpdate = instance.animIndex > -1 ? !context_id->riGenUpdateSlots[instance.animIndex] : false;
        needBlasBuild = false;
        for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
        {
          if (!mesh.vertexProcessor)
            continue;
          G_ASSERT(!mesh.vertexProcessor->isOneTimeOnly() && instance.uniqueTransformedBuffer);

          auto animatedVertices = UniqueOrReferencedBVHBuffer(*instance.uniqueTransformedBuffer); //-V595
          process_meta(context_id, meta, mesh, needBlasBuild, instance, *frustum, cameraPos, lightDirection, animatedVertices,
            stationary, skipUpdate, baseMeta);
        }
      }

      for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
      {
        if (!mesh.vertexProcessor)
          continue;
        G_ASSERT(!mesh.vertexProcessor->isOneTimeOnly() && instance.uniqueTransformedBuffer);

        auto animatedVertices = UniqueOrReferencedBVHBuffer(*instance.uniqueTransformedBuffer); //-V595

#if DAGOR_DBGLEVEL > 0
        static bool check_alpha = true;
        if (check_alpha && (meta.materialType & MeshMeta::bvhMaterialAlphaTest) &&
            meta.ahsVertexBufferIndex == BVH_BINDLESS_BUFFER_MAX && !context_id->ommEnabled)
        {
          if (screenshot_function)
          {
            screenshot_function();
            logerr("There is a bad alpha tested model in the BVH. A screenshot is made. Please report with the screenshot!");
          }
          else
            logerr("There is a bad alpha tested model in the BVH. Please make a screenshot and report with it!");

          check_alpha = false;
        }
#endif

        CHECK_LOST_DEVICE_STATE();

        bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;
        const bool skipped = context_id->ommEnabled && mesh_should_be_skipped(mesh);
        auto &geom = *(RaytraceGeometryDescription *)geoms.push_back_uninitialized();

        geom.type = RaytraceGeometryDescription::Type::TRIANGLES;
        geom.data.triangles.transformBuffer = nullptr;
        geom.data.triangles.vertexBuffer = animatedVertices.get();
        geom.data.triangles.indexBuffer = mesh.geometry.getIndexBuffer(context_id);
        geom.data.triangles.transformOffset = 0;
        geom.data.triangles.vertexCount = mesh.vertexCount;
        geom.data.triangles.vertexStride = meta.vertexStride;
        geom.data.triangles.vertexOffset = 0;
        geom.data.triangles.vertexOffsetExtraBytes = animatedVertices.getOffset();
        geom.data.triangles.vertexFormat = mesh.processedPositionFormat;
        geom.data.triangles.indexCount = skipped ? 0 : mesh.indexCount;
        geom.data.triangles.indexOffset = meta.startIndex;
        // geom is not zero initialized (push_back_uninitialized): the new field
        // must be written, or the width resolution reads indeterminate memory
        geom.data.triangles.indexFormat = RaytraceGeometryDescription::IndexFormat::UseBuffer;
        geom.data.triangles.flags =
          (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
        geom.extraDataAvailableMask.hasOpacityMicroMapLinkage = false;
        if (context_id->ommEnabled && !skipped && instance_can_use_mesh_omm(mesh, meta, baseMeta))
          set_omm_linkage(geom, mesh);
      }

      // If the BLAS is not unique, then only build it when it has not been built at all.
      if (needBlasBuild && (!blas || instance.uniqueBlas))
      {
        RaytraceBuildFlags flags =
          (object.isAnimated && !stationary)
            ? RaytraceBuildFlags::FAST_BUILD | RaytraceBuildFlags::ALLOW_UPDATE | RaytraceBuildFlags::LOW_MEMORY
            : RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;
        bool isNew = false;

        if (!blas)
        {
          blas = UniqueBLAS::create(geoms.data(), geoms.size(), flags);
          isNew = true;

          HANDLE_LOST_DEVICE_STATE(blas, );
          // this is how we distinguish tree which are later deleted via stationaryTreeBuffers
          if (object.type == BvhType::RI && is_tree_instance(instance) && blas)
            unitedvdata::riUnitedVdata.adjustBlasSize(blas.getASSize());
          else if (object.type == BvhType::Dyn && is_tree_instance(instance) && blas)
            unitedvdata::dmUnitedVdata.adjustBlasSize(blas.getASSize());

          if (object.blas && object.blas != blas && !stationary)
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
        update.basbi.scratchSpaceBufferSizeInBytes = update.basbi.doUpdate ? blas.getUpdateScratchSize() : blas.getBuildScratchSize();
        update.basbi.scratchSpaceBuffer =
          alloc_scratch_buffer(update.basbi.scratchSpaceBufferSizeInBytes, update.basbi.scratchSpaceBufferOffsetInBytes);

        if (update.basbi.scratchSpaceBufferSizeInBytes)
          HANDLE_LOST_DEVICE_STATE(update.basbi.scratchSpaceBuffer, );

        // After building the first instance, a copy of it is made into the mesh structure.
        // This copy is used when new instances needs to be made, and with the new instance,
        // there is no need to build the BLAS topology, only an update, which is about 50
        // times faster.
        // If the template is being created, we build immediately.
        if (!object.blas)
        {
          if (assumed_buffer_processor)
            assumed_buffer_processor->end(true);
          // inside delay/continue sync we must use only that work that can be batched together
          // otherwise we can't properly generate barriers, so simply make another batch when we need to build BLAS
          if (delay_sync)
            d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);

          if (context_id->ommEnabled)
            build_pending_omm_arrays(ommBuildInfos, ommBuildResults);

#if _TARGET_C2

#else
          G_ASSERT(update.basbi.scratchSpaceBufferSizeInBytes > 0);
#endif

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

          if (object.type == BvhType::RI && object.blas)
            unitedvdata::riUnitedVdata.adjustBlasSize(object.blas.getASSize());
          else if (object.type == BvhType::Dyn && object.blas)
            unitedvdata::dmUnitedVdata.adjustBlasSize(object.blas.getASSize());

          allBlasUpdatesAs.erase(blasUpdates.back().as);
          blasUpdates.pop_back();
          updateGeoms.pop_back();

          if (delay_sync)
            d3d::driver_command(Drv3dCommand::DELAY_SYNC);

          if (assumed_buffer_processor)
            assumed_buffer_processor->begin();
        }
      }
      else
      {
        updateGeoms.pop_back();
      }
    }

    bool flipWinding = need_winding_flip(object.meshes[0], instance);

    perInstanceData.emplace_back(instance.perInstanceData.value_or(PerInstanceData::ZERO));

    HWInstance desc;
    desc.transform = instance.transform;
    desc.instanceID = MeshMetaAllocator::decode(metaAllocId);
    desc.instanceMask = instance.noShadow ? bvhGroupNoShadow : object.meshes[0].isCamoNet ? bvhGroupCamoNet : group_mask;
    desc.instanceContributionToHitGroupIndex = 0;
    desc.flags = instance.forceEnableBackfaceCulling ? (flipWinding ? RaytraceGeometryInstanceDescription::TRIANGLE_CULL_FLIP_WINDING
                                                                    : RaytraceGeometryInstanceDescription::NONE)
                                                     : RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
    desc.blasGpuAddress = blas.getGPUAddress();

    if (!is_camera_relative)
      realign(desc.transform, camera_pos);

    outInstances.push_back(convert_instance(desc));
  }
}


void build(ContextId context_id, const TMatrix &itm, const TMatrix4 &projTm, const Point3 &camera_pos, const Point3 &light_direction)
{
  if (!per_frame_processing_enabled)
  {
    logdbg("[BVH] Device is in reset mode.");
    return;
  }
  CHECK_LOST_DEVICE_STATE();

  FRAMEMEM_REGION;

  TIME_D3D_PROFILE(bvh_build);

  if (context_id->hasAny(Features::AnyDynrend))
  {
    dyn::wait_dynrend_instances();
    dyn::wait_animchar_instances();
  }
  if (context_id->hasAny(Features::AnyRI))
  {
    ri::wait_ri_gen_instances_update(context_id);
    ri::wait_ri_extra_instances_update(context_id);
  }

  const parallel_instance_processing::ParallelFinishResult parallelFinishResult =
    parallel_instance_processing::is_parallel_jobs_finished(context_id);

  lru_collision::update(context_id, camera_pos);
  CHECK_LOST_DEVICE_STATE();

  Sbuffer *grassInstances = nullptr;
  Sbuffer *grassInstanceCount = nullptr;
  grass::get_instances(context_id, grassInstances, grassInstanceCount);

  Sbuffer *gobjInstances = nullptr;
  Sbuffer *gobjInstanceCount = nullptr;
  gobj::get_instances(context_id, gobjInstances, gobjInstanceCount);

  Sbuffer *fxInstances = nullptr;
  Sbuffer *fxInstanceCount = nullptr;
  fx::get_instances(context_id, fxInstances, fxInstanceCount);

  Sbuffer *smokeTracerInstances = nullptr;
  Sbuffer *smokeTracerInstanceCount = nullptr;
  smoke_tracers::get_instances(context_id, smokeTracerInstances, smokeTracerInstanceCount);

  auto dgdpBuffers = dagdp::get_buffers(context_id);
  Sbuffer *dagdpInstances = dgdpBuffers.instances;
  Sbuffer *dagdpInstancesCount = dgdpBuffers.instanceCount;

  Sbuffer *gpuGrassInstances = nullptr;
  Sbuffer *gpuGrassInstanceCount = nullptr;
  gpugrass::get_instances(context_id, gpuGrassInstances, gpuGrassInstanceCount);

  int cablesMetaAllocId = -1;
  auto cableBlases = &cables::get_blases(context_id, cablesMetaAllocId);

#if DAGOR_DBGLEVEL > 0
  if (!bvh_grass_enable)
  {
    grassInstances = nullptr;
    grassInstanceCount = nullptr;
    gpuGrassInstances = nullptr;
    gpuGrassInstanceCount = nullptr;
  }
  if (!bvh_gpuobject_enable)
  {
    gobjInstances = nullptr;
    gobjInstanceCount = nullptr;
    dagdpInstances = nullptr;
    dagdpInstancesCount = nullptr;
  }
  if (!bvh_particles_enable)
  {
    fxInstances = nullptr;
    fxInstanceCount = nullptr;
  }
  if (!bvh_tracers_enable)
  {
    smokeTracerInstances = nullptr;
    smokeTracerInstanceCount = nullptr;
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
  int smokeTracerBufferSize = smokeTracerInstances ? smokeTracerInstances->getNumElements() : 0;
  int dagdpBufferSize = dagdpInstances ? dagdpInstances->getNumElements() : 0;
  int gpuGrassBufferSize = gpuGrassInstances ? gpuGrassInstances->getNumElements() : 0;

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

  int riExtraFlagCount = 0;
  for (auto &instances : context_id->riExtraFlagInstances)
    riExtraFlagCount += instances.size();

  int dynrendCount = 0;
  for (auto &instances : context_id->dynrendInstances)
    dynrendCount += instances.second.size();

  int waterCount = 0;
  for (auto &patch : context_id->water_patches)
    waterCount += patch.instances.size();

  int splinegenCount = context_id->splineGenInstances.size();

  int cableCount = cableBlases ? cableBlases->size() : 0; //-V547

  auto terrainBlases = terrain::get_blases(context_id);
  int terrainCount = terrainBlases.size();

  auto &lruCollisionInstances = lru_collision::get_instances(context_id, camera_pos);
  const int lruCollisionCount = lruCollisionInstances.size();

  const int precalcedInstanceCount = impostorCount + riExtraCount; // these are calculated right after the parallel jobs
  const int nonGpuZeroDataInstanceCount = terrainCount + cableCount + waterCount;
  const int dynamicDataInstanceCount =
    context_id->genericInstances.size() + riGenCount + riExtraTreeCount + riExtraFlagCount + dynrendCount + splinegenCount;

  const int instanceCount = dynamicDataInstanceCount + nonGpuZeroDataInstanceCount + precalcedInstanceCount;

  const int gpuInstanceCount = grassBufferSize + gobjBufferSize + dagdpBufferSize + gpuGrassBufferSize;
  const int allInstancesCount = instanceCount + gpuInstanceCount;

  // Mark them invalid for now. They will be marked valid later as they are getting uploaded.
  context_id->tlasMainValid = false;
  context_id->tlasTerrainValid = false;
  context_id->tlasParticlesValid = false;
  context_id->tlasLruCollisionValid = false;

  // Discard OMM bakes whose objects went inactive. Must run before the no-instance early return below:
  // a dynamic bake started in add_instances lives only in objectsWithBakingOmm, so if its last instance
  // disappears and build() keeps returning early, the bake and its buffers would never be freed.
  {
    Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
    discard_inactive_omm_bakes(context_id);
  }

  if (!instanceCount && !grassBufferSize && !gobjBufferSize && !fxBufferSize && !smokeTracerBufferSize && !dagdpBufferSize &&
      !gpuGrassInstances && !lruCollisionCount)
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

  const int particleBufferSize = fxBufferSize + smokeTracerBufferSize;

  const int fxParticleRegion = min(fxBufferSize, particles::get_fx_capacity());
  const int builtParticleCount = fxParticleRegion + smokeTracerBufferSize;

  auto uploadSizeMain = max(round_up(allInstancesCount, 1024 << 3), 1U << 17);
  auto uploadSizeTerrain = round_up(terrainBlases.size() + 1, 64);
  auto uploadSizeParticles = max(round_up(particleBufferSize, 1024), 1U << 13);
  auto uploadSizeLruCollision = round_up(lruCollisionCount + 1, 64);
  auto uploadSizePerInstanceData = max(round_up(allInstancesCount, 1024), 1U << 13);

  if (context_id->tlasUploadMain && uploadSizeMain > context_id->tlasUploadMain->getNumElements())
    context_id->tlasUploadMain.close();
  if (context_id->tlasUploadTerrain && uploadSizeTerrain > context_id->tlasUploadTerrain->getNumElements())
    context_id->tlasUploadTerrain.close();
  if (context_id->tlasUploadParticles && uploadSizeParticles > context_id->tlasUploadParticles->getNumElements())
    context_id->tlasUploadParticles.close();
  if (context_id->tlasUploadLruCollision && uploadSizeLruCollision > context_id->tlasUploadLruCollision->getNumElements())
    context_id->tlasUploadLruCollision.close();
  if (context_id->perInstanceData && uploadSizePerInstanceData > context_id->perInstanceData->getNumElements())
    context_id->perInstanceData.close();

  if (!bvh_disable_parallel_instance_processing_finish)
  {
    if (!parallelFinishResult.MainUploadDone)
      context_id->tlasUploadMain.close();
    if (!parallelFinishResult.PerInstanceDataUploadDone)
      context_id->perInstanceData.close();
  }

  bool reallocateMainTlas = !context_id->tlasUploadMain;
  bool reallocateTerrainTlas = !context_id->tlasUploadTerrain;
  bool reallocateParticlesTlas = !context_id->tlasUploadParticles;
  bool reallocateLruCollisionTlas = context_id->lruCollision && !context_id->tlasUploadLruCollision;
  bool reallocatePerInstanceData = !context_id->perInstanceData;

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
    context_id->tlasUploadParticles = dag::buffers::create_ua_sr_structured(HW_INSTANCE_SIZE, uploadSizeParticles,
      ccn(context_id, "bvh_tlas_upload_particles"), d3d::buffers::Init::No, RESTAG_BVH);
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadParticles, );
    logdbg("tlasUploadParticles resized to %u", uploadSizeParticles);
  }

  if (reallocateLruCollisionTlas)
  {
    TIME_PROFILE(allocate_lru_collision_tlas_upload);
    HANDLE_LOST_DEVICE_STATE(context_id->tlasUploadLruCollision.allocate(HW_INSTANCE_SIZE, uploadSizeLruCollision,
                               SBCF_UA_SR_STRUCTURED, "bvh_tlas_upload_lru_collision", context_id), );
  }

  if (reallocatePerInstanceData)
  {
    HANDLE_LOST_DEVICE_STATE(context_id->perInstanceData.allocate(sizeof(PerInstanceData), uploadSizePerInstanceData,
                               SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED, "bvh_per_instance_data", context_id), );
  }

  const bool needFallbackMain = !parallelFinishResult.MainUploadDone || reallocateMainTlas;
  const bool needFallbackPerInstance = !parallelFinishResult.PerInstanceDataUploadDone || reallocatePerInstanceData;
  if (needFallbackMain || needFallbackPerInstance)
    bvh_fallback_upload_heavy_data_job.start(context_id, needFallbackMain, needFallbackPerInstance);

  auto &instanceDescs = context_id->instanceDescsCpu;

  auto &dynamicPerInstanceData = context_id->perInstanceDataCpu;
  dynamicPerInstanceData.clear();
  dynamicPerInstanceData.reserve(dynamicDataInstanceCount);

  instanceDescs.clear();
  instanceDescs.reserve(instanceCount);

  auto itmRelative = itm;
  itmRelative.setcol(3, Point3::ZERO);
  auto frustumRelative = Frustum(TMatrix4(orthonormalized_inverse(itmRelative)) * projTm);
  auto frustumAbsolute = Frustum(TMatrix4(orthonormalized_inverse(itm)) * projTm);

  eastl::unordered_set<void *, eastl::hash<void *>, eastl::equal_to<void *>, framemem_allocator> allBlasUpdatesAs;
  OmmBuildInfos ommBuildInfos;
  OmmBuildResults ommBuildResults;

  parallel_instance_processing::prebuild_meta_job.wait();

  {
    TIME_D3D_PROFILE(add_and_animate_instances);

    // for safety, they should never block
    Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
    OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);

    context_id->blasUpdates.clear();
    context_id->updateGeoms.clear();

    int bufferCount = context_id->processBufferAllocator.size();

    DA_PROFILE_TAG(add_and_animate_instances, "bufferCount: %d", bufferCount);

    dag::Vector<Sbuffer *, framemem_allocator> rbBuffers;
    rbBuffers.reserve(bufferCount);

    for (auto &alloc : context_id->processBufferAllocator)
      rbBuffers.push_back(alloc.first.getHeap().getBuf());

    if (!rbBuffers.empty())
    {
      TIME_PROFILE(rbBuffers_barrier);
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessUAVComputeBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size())); // -V512
    }
    if (delay_sync)
      d3d::driver_command(Drv3dCommand::DELAY_SYNC);

    if (!context_id->genericInstances.empty())
    {
      add_instances(context_id, context_id->genericInstances, instanceDescs, bvhGroupRi, false, "generic", light_direction, camera_pos,
        allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr, ommBuildInfos, ommBuildResults);
      CHECK_LOST_DEVICE_STATE();
    }

    {
      context_id->riGenStartIndexType = (context_id->riGenStartIndexType + 1) % Context::MaxTreeAnimIndices;

      ProcessorInstances::getTreeVertexProcessor().begin();

      auto startRef = profile_ref_ticks();
      for (auto &instances : context_id->riGenInstances)
      {
        add_instances(context_id, instances, instanceDescs, bvhGroupRi, false, "riGen", light_direction, camera_pos, allBlasUpdatesAs,
          frustumAbsolute, frustumRelative, &ProcessorInstances::getTreeVertexProcessor(), ommBuildInfos, ommBuildResults);
        CHECK_LOST_DEVICE_STATE();
      }
      auto riGenTimeUs = profile_time_usec(startRef);
      context_id->lastRiGenProcessTimeUs = riGenTimeUs;

      int toleranceRange = (5 * bvh_riGen_budget_us) / 100; // tolerance is 5% up and down
      if (riGenTimeUs > bvh_riGen_budget_us + toleranceRange)
        context_id->riGenIndexTypePerFrame = max(context_id->riGenIndexTypePerFrame - 1, 1);
      else if (riGenTimeUs < bvh_riGen_budget_us - toleranceRange)
        context_id->riGenIndexTypePerFrame = min(context_id->riGenIndexTypePerFrame + 1, Context::MaxTreeAnimIndices);

      for (auto &instances : context_id->riExtraTreeInstances)
      {
        add_instances(context_id, instances, instanceDescs, bvhGroupRi, false, "riExtraTree", light_direction, camera_pos,
          allBlasUpdatesAs, frustumAbsolute, frustumRelative, &ProcessorInstances::getTreeVertexProcessor(), ommBuildInfos,
          ommBuildResults);
        CHECK_LOST_DEVICE_STATE();
      }
      ProcessorInstances::getTreeVertexProcessor().end(false);
    }

    for (auto &instances : context_id->riExtraFlagInstances)
    {
      add_instances(context_id, instances, instanceDescs, bvhGroupRi, false, "riExtraFlag", light_direction, camera_pos,
        allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr, ommBuildInfos, ommBuildResults);
      CHECK_LOST_DEVICE_STATE();
    }

    if (use_batched_skinned_vertex_processor)
      ProcessorInstances::getSkinnedVertexProcessorBatched().begin();
    for (auto &instances : context_id->dynrendInstances)
    {
      if (dynrend::is_valid_context(instances.first))
        dyn::set_up_dynrend_context_for_processing(instances.first);
      add_instances(context_id, instances.second, instanceDescs, bvhGroupDynrend, true, "dynrend", light_direction, camera_pos,
        allBlasUpdatesAs, frustumAbsolute, frustumRelative,
        use_batched_skinned_vertex_processor ? &ProcessorInstances::getSkinnedVertexProcessorBatched() : nullptr, ommBuildInfos,
        ommBuildResults);
      CHECK_LOST_DEVICE_STATE();
    }
    if (use_batched_skinned_vertex_processor)
      ProcessorInstances::getSkinnedVertexProcessorBatched().end(false);

    if (!context_id->splineGenInstances.empty())
    {
      add_instances(context_id, context_id->splineGenInstances, instanceDescs, bvhGroupDynrend, true, "splinegen", light_direction,
        camera_pos, allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr, ommBuildInfos, ommBuildResults);
      CHECK_LOST_DEVICE_STATE();
    }

    CHECK_LOST_DEVICE_STATE();

    if (delay_sync)
      d3d::driver_command(Drv3dCommand::CONTINUE_SYNC);
    if (!rbBuffers.empty())
    {
      dag::Vector<ResourceBarrier, framemem_allocator> rb(rbBuffers.size(), bindlessSRVBarrier);
      d3d::resource_barrier(ResourceBarrierDesc(rbBuffers.data(), rb.data(), rbBuffers.size())); // -V512
    }
  }

  bvh_upload_meta_job.start(context_id);

  int terrainDescIndex = instanceDescs.size();

  {
    TIME_D3D_PROFILE(procedural_blas_builds);
    DA_PROFILE_TAG(procedural_blas_builds, "blas count: %d", context_id->blasUpdates.size());
    if (showProceduralBLASBuildCount)
      visuallog::logmsg(String(64, "Procedural BLAS builds: %d", context_id->blasUpdates.size()));

    for (auto [geoms, update] : zip(context_id->updateGeoms, context_id->blasUpdates))
    {
      update.basbi.geometryDesc = geoms.data();
      update.basbi.geometryDescCount = geoms.size();
    }

    if (!ommBuildInfos.empty())
    {
      TIME_D3D_PROFILE(bvh_build_procedural_blas_with_omm);
      d3d::raytrace::build_acceleration_structure({
        .opacityMicroMapTriangleArrayBuilds = ommBuildInfos,
        .bottomBuilds = context_id->blasUpdates,
        .flushAfterOpacityMicroMapTriangleArrayBuilds = true,
        .flushAfterBottomBuild = true,
      });
      ommBuildInfos.clear();
      release_omm_bake_build_inputs(ommBuildResults);
    }
    else
    {
      d3d::build_bottom_acceleration_structures(context_id->blasUpdates.data(), context_id->blasUpdates.size());
    }
  }

  if (terrainCount > 0) //-v1051
  {
    TIME_D3D_PROFILE(terrain);

    for (auto [blasIx, blas] : enumerate(terrainBlases))
    {
      auto &origin = eastl::get<2>(blas);

      HWInstance desc;
      desc.transform.row0 = v_make_vec4f(1, 0, 0, origin.x - camera_pos.x);
      desc.transform.row1 = v_make_vec4f(0, 1, 0, 0 - camera_pos.y);
      desc.transform.row2 = v_make_vec4f(0, 0, 1, origin.y - camera_pos.z);

      desc.instanceID = MeshMetaAllocator::decode(eastl::get<1>(blas));
      desc.instanceMask = bvhGroupTerrain;
      desc.instanceContributionToHitGroupIndex = 0;
      desc.flags = RaytraceGeometryInstanceDescription::NONE;
      desc.blasGpuAddress = eastl::get<0>(blas);

      instanceDescs.push_back(convert_instance(desc));
    }
  }

  if (cableBlases) //-V547
  {
    TIME_D3D_PROFILE(cables);

    for (auto [blasIx, blas] : enumerate(*cableBlases))
    {
      HWInstance desc;
      desc.transform.row0 = v_make_vec4f(1, 0, 0, -camera_pos.x);
      desc.transform.row1 = v_make_vec4f(0, 1, 0, -camera_pos.y);
      desc.transform.row2 = v_make_vec4f(0, 0, 1, -camera_pos.z);

      desc.instanceID = cablesMetaAllocId;
      desc.instanceMask = bvhGroupRi;
      desc.instanceContributionToHitGroupIndex = 0;
      desc.flags = RaytraceGeometryInstanceDescription::TRIANGLE_CULL_DISABLE;
      desc.blasGpuAddress = blas.getGPUAddress();
      instanceDescs.push_back(convert_instance(desc));
    }
  }

  if (waterCount > 0)
  {
    TIME_D3D_PROFILE(water);

    for (auto &patch : context_id->water_patches)
    {
      for (auto &instance : patch.instances)
      {
        HWInstance desc;
        desc.transform.row0 = v_make_vec4f(instance.scale.x, 0, 0, instance.position.x - camera_pos.x);
        desc.transform.row1 = v_make_vec4f(0, 1, 0, 0 - camera_pos.y);
        desc.transform.row2 = v_make_vec4f(0, 0, instance.scale.y, instance.position.y - camera_pos.z);

        desc.instanceID = MeshMetaAllocator::decode(patch.metaAllocId);
        desc.instanceMask = bvhGroupWater;
        desc.instanceContributionToHitGroupIndex = 0;
        desc.flags = 0;
        desc.blasGpuAddress = patch.blas.getGPUAddress();
        instanceDescs.push_back(convert_instance(desc));
      }
    }
  }

  bvh_fallback_upload_heavy_data_job.wait();

  // A failed fallback upload leaves the precalced region of the instance or per instance
  // data buffer stale or uninitialized while the TLAS build still counts those instances.
  // Retry synchronously, and if that fails too, do not mark the main TLAS valid this frame.
  bool mainInstanceDataValid = true;
  if (needFallbackMain && !bvh_fallback_upload_heavy_data_job.isMainUploadDone())
  {
    mainInstanceDataValid =
      parallel_instance_processing::upload_main_data(context_id, parallel_instance_processing::TargetFrame::Current);
    if (!mainInstanceDataValid)
      logerr("[BVH] Fallback instance upload failed, skipping the main TLAS for this frame");
  }
  if (needFallbackPerInstance && !bvh_fallback_upload_heavy_data_job.isPerInstanceDataUploadDone())
  {
    if (!parallel_instance_processing::upload_per_instance_data(context_id, parallel_instance_processing::TargetFrame::Current))
    {
      mainInstanceDataValid = false;
      logerr("[BVH] Fallback per instance data upload failed, skipping the main TLAS for this frame");
    }
  }

  int cpuInstanceCount = 0;

  {
    TIME_D3D_PROFILE(copy_to_tlas_main_upload);

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
      cpuInstanceCount += instanceDescs.size();
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

    {
      TIME_D3D_PROFILE(memcpy_dagdp)
      copyHwInstances(dagdpInstancesCount, dagdpInstances, context_id->tlasUploadMain.getBuf(), dagdpBufferSize, cpuInstanceCount);
      cpuInstanceCount += dagdpBufferSize;
    }

    {
      TIME_D3D_PROFILE(memcpy_gpu_grass)
      copyHwInstances(gpuGrassInstanceCount, gpuGrassInstances, context_id->tlasUploadMain.getBuf(), gpuGrassBufferSize,
        cpuInstanceCount);
      cpuInstanceCount += gpuGrassBufferSize;
    }
  }

  const int zeroDataInstanceCount = nonGpuZeroDataInstanceCount + gpuInstanceCount;
  const int dynamicDataUploadCount = dynamicPerInstanceData.size() + zeroDataInstanceCount;
  G_ASSERT(precalcedInstanceCount + dynamicDataUploadCount <= allInstancesCount);

  if (dynamicDataUploadCount > 0)
  {
    TIME_D3D_PROFILE(upload_per_instance_data);

    G_ASSERT(context_id->perInstanceData && context_id->perInstanceData->getNumElements() >= dynamicDataUploadCount);

    if (auto upload = lock_sbuffer<uint8_t>(context_id->perInstanceData.getBuf(), precalcedInstanceCount * sizeof(PerInstanceData),
          dynamicDataUploadCount * sizeof(PerInstanceData), VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
    {
      TIME_D3D_PROFILE(upload_per_instance_data_etc);
      int offset = 0;
      memcpy(upload.get() + offset, dynamicPerInstanceData.data(), dynamicPerInstanceData.size() * sizeof(PerInstanceData));

      offset += dynamicPerInstanceData.size() * sizeof(PerInstanceData);
      memset(upload.get() + offset, 0, zeroDataInstanceCount * sizeof(PerInstanceData));
    }
  }

  {
    TIME_D3D_PROFILE(copy_to_tlas_terrain_upload);
    if (auto upload = lock_sbuffer<uint8_t>(context_id->tlasUploadTerrain.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
      copyHwInstancesCpu(upload.get(), instanceDescs.data() + terrainDescIndex, terrainBlases.size());
  }

  {
    TIME_D3D_PROFILE(copy_to_tlas_particles_upload);
    copyHwInstances(fxInstanceCount, fxInstances, context_id->tlasUploadParticles.getBuf(), fxParticleRegion, 0);
    copyHwInstances(smokeTracerInstanceCount, smokeTracerInstances, context_id->tlasUploadParticles.getBuf(), smokeTracerBufferSize,
      fxParticleRegion);
  }

  if (lruCollisionCount)
  {
    TIME_D3D_PROFILE(copy_to_tlas_lru_collision_upload);
    if (auto upload = lock_sbuffer<uint8_t>(context_id->tlasUploadLruCollision.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
      copyHwInstancesCpu(upload.get(), lruCollisionInstances.data(), lruCollisionCount);
  }

  G_ASSERT(cpuInstanceCount <= uploadSizeMain);
  G_ASSERT(cpuInstanceCount <= context_id->tlasUploadMain->getNumElements());

  if (reallocateMainTlas && context_id->tlasMain)
    context_id->tlasMain.reset();
  if (reallocateTerrainTlas && context_id->tlasTerrain)
    context_id->tlasTerrain.reset();
  if (reallocateParticlesTlas && context_id->tlasParticles)
    context_id->tlasParticles.reset();
  if (reallocateLruCollisionTlas && context_id->tlasLruCollision)
    context_id->tlasLruCollision.reset();

  RaytraceBuildFlags tlasBuildFlags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;
  if (bvh_use_fast_tlas_build)
    tlasBuildFlags |= RaytraceBuildFlags::FAST_BUILD;

  if (!context_id->tlasMain && context_id->tlasUploadMain)
  {
    context_id->tlasMain = UniqueTLAS::create(context_id->tlasUploadMain->getNumElements(), tlasBuildFlags, "main");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasMain, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasMain.getScratchBuffer(), );

    logdbg("Main TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.", context_id->tlasUploadMain->getNumElements(),
      context_id->tlasMain.getASSize() >> 10, context_id->tlasMain.getScratchBuffer()->getNumElements() >> 10);
  }

  if (!context_id->tlasTerrain && context_id->tlasUploadTerrain)
  {
    context_id->tlasTerrain = UniqueTLAS::create(context_id->tlasUploadTerrain->getNumElements(), tlasBuildFlags, "terrain");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasTerrain, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasTerrain.getScratchBuffer(), );

    logdbg("Terrain TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.",
      context_id->tlasUploadTerrain->getNumElements(), context_id->tlasTerrain.getASSize() >> 10,
      context_id->tlasTerrain.getScratchBuffer()->getNumElements() >> 10);
  }

  if (!context_id->tlasParticles && context_id->tlasUploadParticles)
  {
    context_id->tlasParticles = UniqueTLAS::create(context_id->tlasUploadParticles->getNumElements(), tlasBuildFlags, "particle");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasParticles, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasParticles.getScratchBuffer(), );

    logdbg("Particle TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.",
      context_id->tlasUploadParticles->getNumElements(), context_id->tlasParticles.getASSize() >> 10,
      context_id->tlasParticles.getScratchBuffer()->getNumElements() >> 10);
  }

  if (!context_id->tlasLruCollision && context_id->tlasUploadLruCollision)
  {
    context_id->tlasLruCollision =
      UniqueTLAS::create(context_id->tlasUploadLruCollision->getNumElements(), tlasBuildFlags, "lru_collision");
    HANDLE_LOST_DEVICE_STATE(context_id->tlasLruCollision, );
    HANDLE_LOST_DEVICE_STATE(context_id->tlasLruCollision.getScratchBuffer(), );

    logdbg("Lru collision TLAS creation for %u instances. AS size: %ukb, Scratch size: %ukb.",
      context_id->tlasUploadLruCollision->getNumElements(), context_id->tlasLruCollision.getASSize() >> 10,
      context_id->tlasLruCollision.getScratchBuffer()->getNumElements() >> 10);
  }

  {
    TIME_D3D_PROFILE(build_tlas);

    context_id->tlasMainValid = context_id->tlasMain && cpuInstanceCount && mainInstanceDataValid;
    context_id->tlasTerrainValid = context_id->tlasTerrain && terrainBlases.size();
    context_id->tlasParticlesValid = context_id->tlasParticles && particleBufferSize;
    context_id->tlasLruCollisionValid = context_id->tlasLruCollision && lruCollisionCount;

#if DAGOR_DBGLEVEL > 0
    const debug::TlasSizes debugTlasSizes{.mainInstanceCount = uint32_t(cpuInstanceCount),
      .impostorCount = uint32_t(impostorCount),
      .riExtraCount = uint32_t(riExtraCount),
      .cpuCount = uint32_t(instanceDescs.size()),
      .grassCount = uint32_t(grassBufferSize),
      .gpuObjectCount = uint32_t(gobjBufferSize),
      .dagdpCount = uint32_t(dagdpBufferSize),
      .gpuGrassCount = uint32_t(gpuGrassBufferSize),
      .terrainCount = uint32_t(terrainBlases.size()),
      .fxCount = uint32_t(fxParticleRegion),
      .smokeTracerCount = uint32_t(smokeTracerBufferSize),
      .grassCounter = grassInstanceCount,
      .gpuObjectCounter = gobjInstanceCount,
      .dagdpCounter = dagdpInstancesCount,
      .gpuGrassCounter = gpuGrassInstanceCount,
      .fxCounter = fxInstanceCount,
      .smokeTracerCounter = smokeTracerInstanceCount};
#endif

    raytrace::BatchedTopAccelerationStructureBuildInfo tlasUpdate[4];
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
      tasbi.flags = tlasBuildFlags;
      ++tlasCount;

      HANDLE_LOST_DEVICE_STATE(tasbi.scratchSpaceBuffer, );
    };

    if (context_id->tlasMainValid)
    {
      DA_PROFILE_TAG(build_tlas, "main: %d instances", cpuInstanceCount);
      fillTlas(context_id->tlasMain, context_id->tlasUploadMain.getBuf(), cpuInstanceCount);
      CHECK_LOST_DEVICE_STATE();
    }

    if (context_id->tlasTerrainValid)
    {
      DA_PROFILE_TAG(build_tlas, "terrain: %d instances", (int)terrainBlases.size());
      fillTlas(context_id->tlasTerrain, context_id->tlasUploadTerrain.getBuf(), terrainBlases.size());
      CHECK_LOST_DEVICE_STATE();
    }

    if (context_id->tlasParticlesValid)
    {
      DA_PROFILE_TAG(build_tlas, "particles: %d instances", builtParticleCount);
      fillTlas(context_id->tlasParticles, context_id->tlasUploadParticles.getBuf(), builtParticleCount);
      CHECK_LOST_DEVICE_STATE();
    }

    if (context_id->tlasLruCollisionValid)
    {
      DA_PROFILE_TAG(build_tlas, "lru collision: %d instances", lruCollisionCount);
      fillTlas(context_id->tlasLruCollision, context_id->tlasUploadLruCollision.getBuf(), lruCollisionCount);
      CHECK_LOST_DEVICE_STATE();
    }

#if DAGOR_DBGLEVEL > 0
    // Before the build, so a bad descriptor is reported rather than consumed.
    debug::validate_tlas_instances(context_id, debugTlasSizes);
#endif

    d3d::build_top_acceleration_structures(tlasUpdate, tlasCount);

#if DAGOR_DBGLEVEL > 0
    debug::probe_tlas(context_id, debugTlasSizes, camera_pos);
#endif
  }

  {
    TIME_PROFILE(markChangedTextures)
    context_id->markChangedTextures();
  }

  static int bvh_originVarId = get_shader_variable_id("bvh_origin");
  ShaderGlobal::set_float4(bvh_originVarId, camera_pos);

  {
    static int bvh_impostor_start_offsetVarId = get_shader_variable_id("bvh_impostor_start_offset");
    static int bvh_impostor_end_offsetVarId = get_shader_variable_id("bvh_impostor_end_offset");

    static constexpr int impostorStartOffset = 0;
    ShaderGlobal::set_int(bvh_impostor_start_offsetVarId, impostorStartOffset);
    ShaderGlobal::set_int(bvh_impostor_end_offsetVarId, impostorStartOffset + impostorCount);
  }

  bvh_upload_meta_job.wait();

  debug::render_debug_context(context_id, debug_min_t);

  ri::tidy_up_trees(context_id);
  dyn::tidy_up_skins(context_id);

  context_id->processDeathrow();

  static bool dumpRtStatsSetting = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhDumpMemoryStats", false);
  if (dumpRtStatsSetting || bvh_mem_log)
  {
    auto mb = [](int64_t v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), (int64_t)1)); };
    auto overhead = bvh::get_rt_memory_overhead(context_id);
    logdbg("BVH RT memory overhead (real, RT-only)");
    overhead.forEachCategory([](const eastl::string &) {},
      [&](const RtMemoryOverhead::Item &it) {
        if (it.note.empty())
          logdbg("    %s / %s: %d MB", it.category.c_str(), it.sub.c_str(), mb(it.bytes));
        else
          logdbg("    %s / %s: %d MB  [%s]", it.category.c_str(), it.sub.c_str(), mb(it.bytes), it.note.c_str());
      },
      [&](const eastl::string &cat, int64_t sum) { logdbg("  = %s: %d MB", cat.c_str(), mb(sum)); });
    logdbg("-------------------------");
    logdbg("RT overhead total: %d MB", mb(overhead.total));
    logdbg("BLAS total: %d MB  (x%d)", mb(overhead.blasTotalBytes), overhead.blasCount);
    logdbg("Last-LOD BLAS (streaming floor): %d MB  (x%d)", mb(overhead.lastLodBlasBytes), overhead.lastLodBlasCount);
    logdbg("-------------------------");
  }
}

void set_rigen_cpu_budget(int budget_us) { bvh_riGen_budget_us = budget_us; }
void set_tree_anim_max_distance(float distance) { ri::set_tree_anim_max_distance(distance); }


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

void bind_gbuffer_textures(ContextId context_id, Texture *gbuffer_albedo, Texture *gbuffer_normal, Texture *gbuffer_material,
  Texture *gbuffer_motion, Texture *gbuffer_depth)
{
  static int bvh_gbuffer_albedo_indexVarId = get_shader_variable_id("bvh_gbuffer_albedo_index");
  static int bvh_gbuffer_normal_indexVarId = get_shader_variable_id("bvh_gbuffer_normal_index");
  static int bvh_gbuffer_material_indexVarId = get_shader_variable_id("bvh_gbuffer_material_index");
  static int bvh_gbuffer_motion_indexVarId = get_shader_variable_id("bvh_gbuffer_motion_index");
  static int bvh_gbuffer_depth_indexVarId = get_shader_variable_id("bvh_gbuffer_depth_index");

  enum
  {
    bvhGbufferAlbedo = 0,
    bvhGbufferNormal = 1,
    bvhGbufferMaterial = 2,
    bvhGbufferMotion = 3,
    bvhGbufferDepth = 4,
  };

  if (context_id->gbufferBindlessRange < 0)
    context_id->gbufferBindlessRange = d3d::allocate_bindless_resource_range(D3DResourceType::TEX, 5);

  d3d::update_bindless_resources_to_null(D3DResourceType::TEX, context_id->gbufferBindlessRange, 5);

  auto registerGbufferTexture = [br = context_id->gbufferBindlessRange](Texture *t, int i, int shadervar) {
    d3d::resource_barrier({t, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    d3d::update_bindless_resource(D3DResourceType::TEX, br + i, t);
    ShaderGlobal::set_int(shadervar, br + i);
  };

  if (gbuffer_albedo)
    registerGbufferTexture(gbuffer_albedo, bvhGbufferAlbedo, bvh_gbuffer_albedo_indexVarId);
  if (gbuffer_normal)
    registerGbufferTexture(gbuffer_normal, bvhGbufferNormal, bvh_gbuffer_normal_indexVarId);
  if (gbuffer_material)
    registerGbufferTexture(gbuffer_material, bvhGbufferMaterial, bvh_gbuffer_material_indexVarId);
  if (gbuffer_motion)
    registerGbufferTexture(gbuffer_motion, bvhGbufferMotion, bvh_gbuffer_motion_indexVarId);
  if (gbuffer_depth)
    registerGbufferTexture(gbuffer_depth, bvhGbufferDepth, bvh_gbuffer_depth_indexVarId);
}

void bind_fom_textures(ContextId context_id, Texture *fom_sin, Texture *fom_cos, const d3d::SamplerHandle *fom_sampler)
{
  if (!fom_sin || !fom_cos || !fom_sampler)
    return;

  static int fom_shadows_bindless_slotsVarId = get_shader_variable_id("fom_shadows_bindless_slots");

  if (context_id->fomShadowsBindlessRange < 0)
    context_id->fomShadowsBindlessRange = d3d::allocate_bindless_resource_range(D3DResourceType::TEX, 2);

  d3d::resource_barrier({fom_sin, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  d3d::update_bindless_resource(D3DResourceType::TEX, context_id->fomShadowsBindlessRange + 0, fom_sin);
  d3d::resource_barrier({fom_cos, RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  d3d::update_bindless_resource(D3DResourceType::TEX, context_id->fomShadowsBindlessRange + 1, fom_cos);

  ShaderGlobal::set_int4(fom_shadows_bindless_slotsVarId, context_id->fomShadowsBindlessRange, context_id->fomShadowsBindlessRange + 1,
    d3d::register_bindless_sampler(*fom_sampler), 0);
}

void bind_resources(ContextId context_id, int render_width)
{
  G_ASSERT(context_id);
  static int bvh_meta_countVarId = get_shader_variable_id("bvh_meta_count");
  static int bvh_metaVarId = get_shader_variable_id("bvh_meta");
  static int bvh_per_instance_dataVarId = get_shader_variable_id("bvh_per_instance_data");
  static int bvh_atmosphere_textureVarId = get_shader_variable_id("bvh_atmosphere_texture");
  static int bvh_atmosphere_texture_distanceVarId = get_shader_variable_id("bvh_atmosphere_texture_distance");
  static int bvh_paint_details_tex_slotVarId = get_shader_variable_id("bvh_paint_details_tex_slot");
  static int bvh_land_color_tex_slotVarId = get_shader_variable_id("bvh_land_color_tex_slot", true);
  static int bvh_dynamic_mfd_color_tex_slotVarId = get_shader_variable_id("bvh_dynamic_mfd_color_tex_slot", true);
  static int bvh_dynamic_decals_atlas_tex_slotVarId = get_shader_variable_id("bvh_dynamic_decals_atlas_tex_slot", true);
  static int bvh_dynamic_decals_atlas_buf_slotVarId = get_shader_variable_id("bvh_dynamic_decals_atlas_buf_slot", true);
  static int bvh_initial_nodes_buf_slotVarId = get_shader_variable_id("bvh_initial_nodes_buf_slot", true);

  static int cache_tex0_tex_slotVarId = get_shader_variable_id("cache_tex0_tex_slot", true);
  static int indirection_tex_tex_slotVarId = get_shader_variable_id("indirection_tex_tex_slot", true);
  static int cache_tex1_tex_slotVarId = get_shader_variable_id("cache_tex1_tex_slot", true);
  static int cache_tex2_tex_slotVarId = get_shader_variable_id("cache_tex2_tex_slot", true);
  static int last_clip_tex_tex_slotVarId = get_shader_variable_id("last_clip_tex_tex_slot", true);

  static int bvh_mip_rangeVarId = get_shader_variable_id("bvh_mip_range");
  static int bvh_mip_scaleVarId = get_shader_variable_id("bvh_mip_scale");
  static int bvh_mip_biasVarId = get_shader_variable_id("bvh_mip_bias");
#if !_TARGET_C2
  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
  static int bvh_lru_collisionVarId = get_shader_variable_id("bvh_lru_collision", true);
#else




#endif
  static int bvh_main_validVarId = get_shader_variable_id("bvh_main_valid");
  static int bvh_terrain_validVarId = get_shader_variable_id("bvh_terrain_valid");
  static int bvh_particles_validVarId = get_shader_variable_id("bvh_particles_valid");
  static int bvh_lru_collision_validVarId = get_shader_variable_id("bvh_lru_collision_valid", true);
  static int bvh_max_water_distanceVarId = get_shader_variable_id("bvh_max_water_distance");
  static int bvh_water_fade_powerVarId = get_shader_variable_id("bvh_water_fade_power", true);
  static int bvh_max_water_depthVarId = get_shader_variable_id("bvh_max_water_depth", true);
  static int rtr_max_water_depthVarId = get_shader_variable_id("rtr_max_water_depth", true);

#if !_TARGET_C2
  ShaderGlobal::set_tlas(bvh_mainVarId, context_id->tlasMainValid ? context_id->tlasMain.get() : nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, context_id->tlasTerrainValid ? context_id->tlasTerrain.get() : nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, context_id->tlasParticlesValid ? context_id->tlasParticles.get() : nullptr);
  ShaderGlobal::set_tlas(bvh_lru_collisionVarId, context_id->tlasLruCollisionValid ? context_id->tlasLruCollision.get() : nullptr);
#else







#endif

  ShaderGlobal::set_int(bvh_main_validVarId, context_id->tlasMainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_terrain_validVarId, context_id->tlasTerrainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_particles_validVarId, context_id->tlasParticlesValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_lru_collision_validVarId, context_id->tlasLruCollisionValid ? 1 : 0);
  lru_collision::bind_resources(context_id);

  {
    OSSpinlockScopedLock metaGuard(context_id->meshMetaAllocatorLock);
    ShaderGlobal::set_int(bvh_meta_countVarId, context_id->meshMetaAllocator.size());
  }
  ShaderGlobal::set_buffer(bvh_metaVarId, context_id->meshMeta.getBufId());
  ShaderGlobal::set_buffer(bvh_per_instance_dataVarId, context_id->perInstanceData.getBufId());

  ShaderGlobal::set_int(bvh_paint_details_tex_slotVarId, context_id->paint_details_texBindless.bindlessTexture);

  ShaderGlobal::set_int(bvh_land_color_tex_slotVarId, context_id->grass_land_color_maskBindless.bindlessTexture);

  ShaderGlobal::set_int(bvh_dynamic_mfd_color_tex_slotVarId, context_id->dynamic_mfd_texBindless.bindlessTexture);

  ShaderGlobal::set_int(cache_tex0_tex_slotVarId, context_id->cache_tex0Bindless.bindlessTexture);
  ShaderGlobal::set_int(indirection_tex_tex_slotVarId, context_id->indirection_texBindless.bindlessTexture);
  ShaderGlobal::set_int(cache_tex1_tex_slotVarId, context_id->cache_tex1Bindless.bindlessTexture);
  ShaderGlobal::set_int(cache_tex2_tex_slotVarId, context_id->cache_tex2Bindless.bindlessTexture);
  ShaderGlobal::set_int(last_clip_tex_tex_slotVarId, context_id->last_clip_texBindless.bindlessTexture);
  ShaderGlobal::set_int(bvh_dynamic_decals_atlas_tex_slotVarId, context_id->dynamic_decals_atlasBindless.bindlessTexture);
  ShaderGlobal::set_int(bvh_dynamic_decals_atlas_buf_slotVarId, context_id->decalDataHolderBindlessSlot);
  ShaderGlobal::set_int(bvh_initial_nodes_buf_slotVarId, context_id->initialNodesHolderBindlessSlot);

  ShaderGlobal::set_texture(bvh_atmosphere_textureVarId, context_id->atmosphereTexture);
  ShaderGlobal::set_float(bvh_atmosphere_texture_distanceVarId, Context::atmMaxDistance);

  ShaderGlobal::set_float(bvh_mip_rangeVarId, mip_range);
  ShaderGlobal::set_float(bvh_mip_scaleVarId, mip_scale);
  ShaderGlobal::set_float(bvh_mip_biasVarId, max(log2f(3840.0f / render_width), 0.0f));
  ShaderGlobal::set_float(bvh_max_water_distanceVarId, max_water_distance);
  ShaderGlobal::set_float(bvh_water_fade_powerVarId, water_fade_power);
  ShaderGlobal::set_float(bvh_max_water_depthVarId, max_water_depth);
  ShaderGlobal::set_float(rtr_max_water_depthVarId, rtr_max_water_depth);
}

void unbind_resources()
{
#if !_TARGET_C2
  static int bvh_mainVarId = get_shader_variable_id("bvh_main");
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain");
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles");
  static int bvh_lru_collisionVarId = get_shader_variable_id("bvh_lru_collision", true);
  ShaderGlobal::set_tlas(bvh_mainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_lru_collisionVarId, nullptr);
#else








#endif

  static int bvh_main_validVarId = get_shader_variable_id("bvh_main_valid", true);
  static int bvh_terrain_validVarId = get_shader_variable_id("bvh_terrain_valid", true);
  static int bvh_particles_validVarId = get_shader_variable_id("bvh_particles_valid", true);
  static int bvh_lru_collision_validVarId = get_shader_variable_id("bvh_lru_collision_valid", true);
  ShaderGlobal::set_int(bvh_main_validVarId, 0);
  ShaderGlobal::set_int(bvh_terrain_validVarId, 0);
  ShaderGlobal::set_int(bvh_particles_validVarId, 0);
  ShaderGlobal::set_int(bvh_lru_collision_validVarId, 0);

  static int bvh_lru_collision_face_normalsVarId = get_shader_variable_id("bvh_lru_collision_face_normals", true);
  static int bvh_lru_collision_normals_indexVarId = get_shader_variable_id("bvh_lru_collision_normals_index", true);
  ShaderGlobal::set_buffer(bvh_lru_collision_face_normalsVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(bvh_lru_collision_normals_indexVarId, BAD_D3DRESID);
}

void set_for_gpu_objects(ContextId context_id) { gobj::init(context_id); }

void add_bin_scene(ContextId context_id, BaseStreamingSceneHolder &bin_scene) { binscene::add_meshes(context_id, bin_scene); }

void add_water(ContextId context_id, FFTWater &water) { fftwater::create_patches(context_id, water); }

void prepare_ri_extra_instances() { ri::prepare_ri_extra_instances(); }

void on_before_unload_scene(ContextId context_id) { context_id->releaseAllBindlessTexHolders(); }

void on_before_settings_changed(ContextId context_id) { context_id->releaseAllBindlessTexHolders(); }

void on_load_scene(ContextId context_id) { context_id->clearDeathrow(); }

void on_scene_loaded(ContextId context_id)
{
  context_id->clearDeathrow();
  ri::on_scene_loaded(context_id);
}

static void wait_all_jobs()
{
  bvh_upload_meta_job.wait();
  bvh_fallback_upload_heavy_data_job.wait();
  threadpool::wait(&bvh_update_atmosphere_job);
  parallel_instance_processing::prebuild_meta_job.wait();
}

void on_unload_scene(ContextId context_id)
{
  wait_all_jobs();
  dyn::on_unload_scene(context_id);
  ri::on_unload_scene(context_id);
  grass::on_unload_scene(context_id);
  gobj::on_unload_scene(context_id);
  fx::on_unload_scene(context_id);
  binscene::on_unload_scene(context_id);
  splinegen::on_unload_scene(context_id);
  fftwater::on_unload_scene(context_id);
  gpugrass::on_unload_scene(context_id);
  lru_collision::on_unload_scene(context_id);
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
  release_camo_textures(context_id);
  release_process_buffers(context_id);

  context_id->atmosphereDirty = true;

  context_id->instanceDescsCpu.clear();
  context_id->instanceDescsCpu.shrink_to_fit();
  context_id->perInstanceDataCpu.clear();
  context_id->perInstanceDataCpu.shrink_to_fit();
  context_id->blasUpdates.clear();
  context_id->blasUpdates.shrink_to_fit();
  context_id->updateGeoms.clear();
  context_id->updateGeoms.shrink_to_fit();

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
  ATTRIB(fourthTexcoord, vb_, SCUSAGE_TC, 3);
  ATTRIB(color, vb_, SCUSAGE_VCOL, 0);
  ATTRIB(normal, vb_, SCUSAGE_NORM, 0);
  ATTRIB(indices, , SCUSAGE_EXTRA, 0);
  ATTRIB(weights, , SCUSAGE_EXTRA, 1);

  if (texcoordFormat == VSDT_SHORT2)
    texcoordFormat = BufferProcessor::bvhAttributeShort2TC;
  if (secTexcoordFormat == VSDT_SHORT2)
    secTexcoordFormat = BufferProcessor::bvhAttributeShort2TC;
  if (thirdTexcoordFormat == VSDT_SHORT2)
    thirdTexcoordFormat = BufferProcessor::bvhAttributeShort2TC;
  if (fourthTexcoordFormat == VSDT_SHORT2)
    fourthTexcoordFormat = BufferProcessor::bvhAttributeShort2TC;
#undef ATTRIB
}

void connect_fx(ContextId context_id, fx_connect_callback callback)
{
  if (context_id->hasAny(Features::Fx))
    fx::connect(callback);
}

void on_cables_changed(Cables *cables, ContextId context_id) { cables::on_cables_changed(cables, context_id); }

bool is_building(ContextId context_id)
{
  Context::BvhObjectReadLock objectsGuard(context_id->objectsLock);
  // objectsWithBakingOmm covers instances whose TLAS entry add_instances withholds while their OMM
  // bakes -- they are not in halfBakedObjects, so without this a consumer could treat the BVH as ready
  // with those instances missing.
  return !context_id->halfBakedObjects.empty() || context_id->hasPendingObjectAddActions.load(dag::memory_order_relaxed) ||
         !context_id->objectsWithBakingOmm.empty();
}

void set_grass_range(ContextId context_id, float range) { context_id->grassRange = range; }

void set_grass_fraction_to_keep(ContextId context_id, float fraction) { context_id->grassFraction = fraction; }

void start_async_atmosphere_update(ContextId context_id, const Point3 &view_pos, atmosphere_callback callback)
{
  if (!context_id->atmosphereTexture)
  {
    context_id->atmosphereTexture =
      dag::create_array_tex(Context::atmTexWidth, Context::atmTexHeight, 2, TEXCF_DYNAMIC, 1, "bvh_atmosphere_tex", RESTAG_BVH);
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

void connect_dagdp(ContextId context_id, dagdp_connect_callback callback) { callback(dagdp::get_mapper(context_id)); }

void gpu_grass_make_meta(ContextId context_id, const GPUGrassBase &grass) { gpugrass::make_meta(context_id, grass); }

void generate_gpu_grass_instances(ContextId context_id, bool has_grass) { gpugrass::generate_instances(context_id, has_grass); }

void gather_splinegen_instances(ContextId context_id, Sbuffer *vertex_buffer, eastl::vector<eastl::pair<uint32_t, MeshInfo>> &meshes,
  uint32_t instance_vertex_count, uint32_t &bvh_id)
{
#if DAGOR_DBGLEVEL > 0
  if (!bvh_splinegen_enable)
    return;
#endif

  splinegen::add_meshes(context_id, vertex_buffer, meshes, instance_vertex_count, bvh_id);
}

void remove_spline_gen_instances(ContextId context_id) { splinegen::teardown(context_id); }

void connect_smoke_tracers(ContextId context_id, smoke_tracers_connect_callback callback)
{
  if (context_id->hasAny(Features::SmokeTracers))
    smoke_tracers::connect(callback);
}

void update_smoke_tracer_instances(SmokeTracerManager *mgr)
{
  if (mgr)
    smoke_tracers::set_source_buffers(mgr->getTracerBuffer(), mgr->getDynamicBuffer(), mgr->getVertsBuffer());
  smoke_tracers::update_instances();
}

void ensure_particle_buffer_capacity(int fx_max, int smoke_tracer_max) { particles::ensure_capacity(fx_max, smoke_tracer_max); }

} // namespace bvh

size_t BVHInstanceMapper::getHWInstanceSize() { return sizeof(bvh::HWInstance); }

Sbuffer *BVHGeometryBufferWithOffset::getIndexBuffer(bvh::ContextId context_id) const
{
  return context_id->sourceGeometryAllocators[heapIndex].first->getHeap().getBuf();
}

Sbuffer *BVHGeometryBufferWithOffset::getVertexBuffer(bvh::ContextId context_id) const
{
  return processedVertexBuffer ? processedVertexBuffer.get() : getIndexBuffer(context_id);
}

void BVHGeometryBufferWithOffset::close(bvh::ContextId context_id)
{
  context_id->freeSourceGeometry(heapIndex, bufferRegion);
  processedVertexBuffer.reset();
  vbOffset = 0;
  ibOffset = 0;
}
