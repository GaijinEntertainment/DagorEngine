// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_resUpdateBuffer.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <3d/dag_lockSbuffer.h>
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
#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/optional.h>
#include <EASTL/unordered_map.h>
#include <EASTL/numeric.h>
#include <gui/dag_visualLog.h>
#include <userSystemInfo/systemInfo.h>
#include <util/dag_convar.h>

#include "bvh_context.h"
#include "bvh_debug.h"
#include "bvh_add_instance.h"
#include "shaders/dag_shaderVar.h"
#include "bvh_tools.h"

CONSOLE_INT_VAL("render", bvh_riGen_budget_us, 10000, 100, 10000);
CONSOLE_BOOL_VAL("render", bvh_disable_parallel_instance_processing_finish, false);

static eastl::array<int, 3> per_frame_blas_model_budget = {10, 15, 30};
static eastl::array<int, 3> per_frame_compaction_budget = {20, 30, 60};

static constexpr float animation_distance_rate = 20;

static constexpr bool showProceduralBLASBuildCount = false;

static bool per_frame_processing_enabled = true;

#if DAGOR_DBGLEVEL > 0
extern bool bvh_gpuobject_enable;
extern bool bvh_grass_enable;
extern bool bvh_particles_enable;
extern bool bvh_cables_enable;
extern bool bvh_splinegen_enable;
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
dag::Vector<eastl::tuple<uint64_t, MeshMetaAllocator::AllocId, Point2>> get_blases(ContextId context_id);

void init();
void init(ContextId context_id);
void teardown();
void teardown(ContextId context_id);
} // namespace terrain

namespace ri
{
void init(int single_lod_filter_max_faces, float single_lod_filter_max_range, float _ri_lod_dist_bias);
void teardown(bool device_reset);
void init(ContextId context_id);
void teardown(ContextId context_id);
void on_scene_loaded(ContextId context_id);
void on_unload_scene(ContextId context_id);
void prepare_ri_extra_instances();
// instances in view_frustum in ri_gen_visibilities[1] will be discarded!
void update_ri_gen_instances(ContextId context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  const Point3 &view_position, const Point3 &light_direction, const Frustum &view_frustum, threadpool::JobPriority prio);
void update_ri_extra_instances(ContextId context_id, const Point3 &view_position, const Frustum &bvh_frustum,
  const Frustum &view_frustum, threadpool::JobPriority prio);
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
void init(int single_lod_filter_max_faces, float single_lod_filter_max_range, bool discard_destr_assets);
void teardown(bool device_reset, bool zero_bvh_ids);
void init(ContextId context_id);
void teardown(ContextId context_id);
void enable_dynamic_planar_decals(bool enable);
void on_unload_scene(ContextId context_id);
void update_dynrend_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position);
void wait_dynrend_instances();
void update_animchar_instances(ContextId bvh_context_id, dynrend::ContextId dynrend_context_id,
  dynrend::ContextId dynrend_no_shadow_context_id, const Point3 &view_position, dynrend::BVHIterateCallback iterate_callback);
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
void create_patches(ContextId context_id, FFTWater &fft_water);
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

bool use_batched_skinned_vertex_processor = false;
bool is_in_lost_device_state = false;

elem_rules_fn elem_rules = nullptr;
screenshot_fn screenshot_function = nullptr;

dag::AtomicInteger<uint32_t> bvh_id_gen = 0;

float mip_range = 1000;
float mip_scale = 10;

float max_water_distance = 10.0f;
float water_fade_power = 3.0f;
float max_water_depth = 5.0f;
float rtr_max_water_depth = 0.2f;

int ri_thread_count_ofset = 0;

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
  G_UNUSED(context_id);
  // remove the +1 we added in start_frame, now the counter reflects the number of jobs that can contribute to the release
  // if the jobs finish before this call, there is not much to copy anyway, and we can do it separately
  jobGroupReleaseCounter.sub_fetch(1);
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
      compaction->blasCreateJob = nullptr;
      contextId->numCompactionBlasesBeingCreated--;
      contextId->numCompactionBlasesWaitingBuild++;
    }
  }
};

static struct BVHUploadMetaJob : public cpujobs::IJob
{
  ContextId contextId;

  void start(ContextId context_id)
  {
    contextId = context_id;

    int metaCount = contextId->meshMetaAllocator.size();

    if (contextId->meshMeta && contextId->meshMeta->getNumElements() < metaCount)
      contextId->meshMeta.close();

    if (!contextId->meshMeta)
      HANDLE_LOST_DEVICE_STATE(contextId->meshMeta.allocate(sizeof(MeshMeta), metaCount, SBCF_BIND_SHADER_RES | SBCF_MISC_STRUCTURED,
                                 "bvh_meta", contextId), );

    threadpool::add(this, threadpool::JobPriority::PRIO_HIGH);
  }
  void doJob() override
  {
    if (auto upload = lock_sbuffer<MeshMeta>(contextId->meshMeta.getBuf(), 0, 0, VBLOCK_WRITEONLY | VBLOCK_NOOVERWRITE))
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
  bvh_use_fast_tlas_build = settings.use_fast_tlas_build;

  elem_rules = elem_rules_init;
  screenshot_function = screenshot;
  use_batched_skinned_vertex_processor = settings.batchedSkinning;

  scratch_buffer_manager.alignment = d3d::get_driver_desc().raytrace.accelerationStructureBuildScratchBufferOffsetAlignment;

  terrain::init();
  ri::init(settings.singleLodFilterMaxFaces, settings.singleLodFilterMaxRange, settings.riLodDistBias);
  dyn::init(settings.singleLodFilterMaxFaces, settings.singleLodFilterMaxRange, settings.discardDestrAssets);
  gobj::init();
  grass::init();
  fx::init();
  cables::init();
  binscene::init();
  fftwater::init();
  gpugrass::init();
}

void teardown(bool device_reset, bool zero_bvh_ids)
{
  terrain::teardown();
  ri::teardown(device_reset);
  dyn::teardown(device_reset, zero_bvh_ids);
  gobj::teardown();
  grass::teardown();
  fx::teardown();
  cables::teardown();
  binscene::teardown();
  fftwater::teardown();
  gpugrass::teardown();
  debug::teardown();
  hwInstanceCopyShader.reset();
  ProcessorInstances::teardown();
  scratch_buffer_manager.teardown();
  transform_buffer_manager.teardown();

  is_in_lost_device_state = false;
}

void enable_dynamic_planar_decals(bool enable) { dyn::enable_dynamic_planar_decals(enable); }

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
  gpugrass::init(context_id);
  if (context_id->has(Features::Cable))
    cables::init(context_id);
  dagdp::init(context_id);
  debug::init(context_id);
  return context_id;
}

bool has_features(ContextId context_id, uint32_t features)
{
  if (!context_id)
    return false;
  return context_id->has(features);
}

static void unbind_tlases()
{
  // The shadervars are optional, since this function might get called after switching to compatibility binary
#if !_TARGET_C2
  static int bvh_mainVarId = get_shader_variable_id("bvh_main", true);
  static int bvh_terrainVarId = get_shader_variable_id("bvh_terrain", true);
  static int bvh_particlesVarId = get_shader_variable_id("bvh_particles", true);
  ShaderGlobal::set_tlas(bvh_mainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_terrainVarId, nullptr);
  ShaderGlobal::set_tlas(bvh_particlesVarId, nullptr);
#endif

  static int bvh_main_validVarId = get_shader_variable_id("bvh_main_valid", true);
  static int bvh_terrain_validVarId = get_shader_variable_id("bvh_terrain_valid", true);
  static int bvh_particles_validVarId = get_shader_variable_id("bvh_particles_valid", true);
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
  gpugrass::teardown(context_id);
  if (context_id->has(Features::Cable))
    cables::teardown(context_id);
  dagdp::teardown(context_id);
  splinegen::teardown(context_id);
  debug::teardown(context_id);

  for (auto &[object_id, object] : context_id->objects)
    object.teardown(context_id, object_id);
  for (auto &[object_id, object] : context_id->impostors)
    object.teardown(context_id, object_id);

  G_ASSERT(context_id->usedTextures.empty());
  G_ASSERT(context_id->usedBuffers.empty());

  delete context_id;

  context_id = InvalidContextId;

  unbind_tlases();
}

void start_frame()
{
  ri::debug_update();
  dyn::debug_update();
  scratch_buffer_manager.startNewFrame();
  transform_buffer_manager.startNewFrame();
}

void add_instance(ContextId context_id, uint64_t object_id, mat43f_cref transform)
{
  add_instance(context_id, context_id->genericInstances, object_id, transform, nullptr, false,
    Context::Instance::AnimationUpdateMode::DO_CULLING, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    MeshMetaAllocator::INVALID_ALLOC_ID);
}

void update_instances_impl(ContextId bvh_context_id, const Point3 &view_position, const Point3 &light_direction,
  const Frustum &bvh_frustum, const Frustum &view_frustum, dynrend::ContextId *dynrend_context_id,
  dynrend::ContextId *dynrend_no_shadow_context_id, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities,
  dynrend::BVHIterateCallback dynrend_iterate, threadpool::JobPriority prio)
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

  binscene::update_instances(bvh_context_id);
  splinegen::update_instances(bvh_context_id, view_position);

  if (!dynrend_iterate)
    ri_thread_count_ofset = -1;
  else
    ri_thread_count_ofset = 0;

  if (bvh_context_id->has(Features::AnyDynrend))
    if (!dynrend_iterate)
      dyn::update_dynrend_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position);

  if (bvh_context_id->has(Features::AnyRI))
  {
    ri::wait_tidy_up_trees();
    parallel_instance_processing::start_frame(bvh_context_id);
    ri::update_ri_gen_instances(bvh_context_id, ri_gen_visibilities, view_position, light_direction, view_frustum, prio);
    ri::update_ri_extra_instances(bvh_context_id, view_position, bvh_frustum, view_frustum, prio);
    parallel_instance_processing::finish_adding_jobs(bvh_context_id);
  }

  if (bvh_context_id->has(Features::DynrendSkinnedFull))
  {
    dyn::wait_tidy_up_skins();
    if (dynrend_iterate)
      dyn::update_animchar_instances(bvh_context_id, *dynrend_context_id, *dynrend_no_shadow_context_id, view_position,
        dynrend_iterate);
  }
}

void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Point3 &light_direction, const Frustum &bvh_frustum,
  const Frustum &view_frustum, dynrend::ContextId *dynrend_context_id, dynrend::ContextId *dynrend_no_shadow_context_id,
  RiGenVisibility *ri_gen_visibility, threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, light_direction, bvh_frustum, view_frustum, dynrend_context_id,
    dynrend_no_shadow_context_id, {ri_gen_visibility}, nullptr, prio);
}

// daNetGame doesn't use dynrend, but these can be used to identify contexts in BVH
static dynrend::ContextId bvh_dynrend_context = dynrend::ContextId{-1};
static dynrend::ContextId bvh_dynrend_no_shadow_context = dynrend::ContextId{-2};
void update_instances(ContextId bvh_context_id, const Point3 &view_position, const Point3 &light_direction, const Frustum &bvh_frustum,
  const Frustum &view_frustum, const dag::Vector<RiGenVisibility *> &ri_gen_visibilities, dynrend::BVHIterateCallback dynrend_iterate,
  threadpool::JobPriority prio)
{
  update_instances_impl(bvh_context_id, view_position, light_direction, bvh_frustum, view_frustum, &bvh_dynrend_context,
    &bvh_dynrend_no_shadow_context, ri_gen_visibilities, dynrend_iterate, prio);
}

static __forceinline bool need_winding_flip(Mesh &mesh, const Context::Instance &instance)
{
  if (!mesh.needWindingFlip.has_value())
    mesh.needWindingFlip = need_winding_flip(instance.transform);
  return mesh.needWindingFlip.value();
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
  }
  args.tree.ppPositionBindless = mesh.ppPositionBindless;
  args.tree.ppDirectionBindless = mesh.ppDirectionBindless;
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
        mesh.processedPositionFormat = args.positionFormat;

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

static void process_instance_vertices(ContextId context_id, uint64_t object_id, const Mesh &mesh, const Context::Instance *instance,
  const Frustum *frustum, vec4f_const view_pos, vec4f_const light_direction, UniqueOrReferencedBVHBuffer &transformed_vertices,
  bool processed_vertices_recycled, bool &need_blas_build, MeshMeta &meta, uint32_t &processed_position_format)
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
            needProcessing = true;
        }
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
        processed_position_format = args.positionFormat;
      }

      if (!hadProcessedVertices && transformed_vertices.isAllocated())
        meta.setVertexBufferIndex(bindlessIndex);
    }
  }
}

static void process_ahs_vertices(ContextId context_id, Mesh &mesh, MeshMeta &meta)
{
  if (mesh.materialType & MeshMeta::bvhMaterialAlphaTest)
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

    iter->second.teardown(context_id, object_id);
    context_id->cancelCompaction(object_id);
    context_id->halfBakedObjects.erase(object_id);
  }

  auto &object = (iter != objectMap.end()) ? iter->second : objectMap[object_id];
  object.tag = object_info.tag;
  object.meshes = decltype(object.meshes)(object_info.meshes.size());
  object.isAnimated = object_info.isAnimated;
  object.hasVertexProcessor =
    eastl::any_of(object_info.meshes.begin(), object_info.meshes.end(), [](const auto &m) { return m.vertexProcessor; });
  auto metaRegion = object.createAndGetMeta(context_id, object_info.meshes.size());
  for (auto [info, mesh, meta] : zip(object_info.meshes, object.meshes, metaRegion))
  {
    mesh.albedoTextureId = info.albedoTextureId;
    mesh.alphaTextureId = info.alphaTextureId;
    mesh.normalTextureId = info.normalTextureId;
    mesh.extraTextureId = info.extraTextureId;
    mesh.ppPositionTextureId = info.ppPositionTextureId;
    mesh.ppDirectionTextureId = info.ppDirectionTextureId;
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

      HANDLE_LOST_DEVICE_STATE(mesh.geometry, 1);
    }

    {
      // Indices

      ProcessorInstances::getIndexProcessor().process(info.indices, mesh.geometry, mesh.indexFormat, mesh.indexCount, mesh.startIndex,
        mesh.startVertex, context_id);

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

  if (object_info.isAnimated && object_info.meshes[0].vertexProcessor == &ProcessorInstances::getTreeVertexProcessor())
  {
    if (auto iter = context_id->stationaryTreeBuffers.find(object_id); iter == context_id->stationaryTreeBuffers.end())
    {
      ReferencedTransformData data;
      data.metaAllocId = context_id->allocateMetaRegion(1);
      context_id->stationaryTreeBuffers.insert({object_id, eastl::move(data)});
    }
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

static void handle_pending_mesh_actions(ContextId context_id, BuildBudget budget)
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
          context_id->pendingObjectAddActions.erase(iter);

      context_id->cancelCompaction(object_id);
      context_id->halfBakedObjects.erase(object_id);
    }

    context_id->pendingObjectPreChangeActions.clear();
  }

  {
    DA_PROFILE_TAG(handle_pending_mesh_actions, "%u removal in queue", context_id->pendingObjectRemoveActions.size());

    for (auto [object_id, order] : context_id->pendingObjectRemoveActions)
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

      // Only remove the add action itself if it is older than the remove action.
      if (auto iter = context_id->pendingObjectAddActions.find(object_id); iter != context_id->pendingObjectAddActions.end())
        if (iter->second.first < order)
          context_id->pendingObjectAddActions.erase(iter);
    }

    context_id->pendingObjectRemoveActions.clear();
  }

  DA_PROFILE_TAG(handle_pending_mesh_actions, "%u additions in queue", context_id->pendingObjectAddActions.size());

  int meshBudget = calc_mesh_add_budget(context_id, budget);

  int counter = 0;
  for (auto iter = context_id->pendingObjectAddActions.begin();
       iter != context_id->pendingObjectAddActions.end() && counter < meshBudget && !is_in_lost_device_state;)
  {
    TIME_PROFILE(handle_pending_mesh_action);

    // When the mesh is already added, but not yet built, we need to remove it as the build data is not valid anymore.

    context_id->halfBakedObjects.erase(iter->first);
    counter += do_add_object(context_id, iter->first, iter->second.second);
    iter = context_id->pendingObjectAddActions.erase(iter);
  }

  context_id->hasPendingObjectAddActions.store(!context_id->pendingObjectAddActions.empty(), dag::memory_order_relaxed);
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

  handle_pending_mesh_actions(context_id, budget);

  const bool isCompactionCheap = is_blas_compaction_cheap();

  int blasModelBudget = per_frame_blas_model_budget[int(budget)];
  int triangleCount = 0;
  int blasCount = 0;

  dag::Vector<RaytraceGeometryDescription, framemem_allocator> geomDescriptors;
  dag::Vector<raytrace::BatchedBottomAccelerationStructureBuildInfo, framemem_allocator> blasBuildInfos;

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

  for (auto iter = context_id->halfBakedObjects.begin(); iter != context_id->halfBakedObjects.end() && blasModelBudget > 0 &&
                                                         !is_in_lost_device_state &&
                                                         context_id->compactedSizeWritesInQueue < context_id->compactedSizeBufferSize;)
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
      process_mesh_vertices(context_id, objectId, mesh, pb, needBlasBuild, meta);

      CHECK_LOST_DEVICE_STATE();

      G_ASSERT(!mesh.vertexProcessor || mesh.vertexProcessor->isOneTimeOnly());
      const bool hasSecondaryGeometry = mesh.vertexProcessor && mesh.vertexProcessor->isGeneratingSecondaryVertices();
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

      bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;

      desc[i].type = RaytraceGeometryDescription::Type::TRIANGLES;
      desc[i].data.triangles.vertexBuffer = mesh.geometry.getVertexBuffer(context_id);
      desc[i].data.triangles.indexBuffer = mesh.geometry.getIndexBuffer(context_id);
      desc[i].data.triangles.vertexCount = mesh.vertexCount;
      desc[i].data.triangles.vertexStride = meta.vertexStride;
      desc[i].data.triangles.vertexOffset = mesh.baseVertex;
      desc[i].data.triangles.vertexOffsetExtraBytes = mesh.positionOffset + mesh.geometry.vbOffset;
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
        desc[i + 1].data.triangles.vertexBuffer = mesh.geometry.getVertexBuffer(context_id);
        desc[i + 1].data.triangles.indexBuffer = mesh.geometry.getIndexBuffer(context_id);
        desc[i + 1].data.triangles.vertexCount = mesh.vertexCount;
        desc[i + 1].data.triangles.vertexOffset = mesh.vertexCount;
        desc[i + 1].data.triangles.vertexStride = meta.vertexStride;
        desc[i + 1].data.triangles.vertexFormat = mesh.processedPositionFormat;
        desc[i + 1].data.triangles.vertexOffsetExtraBytes = mesh.geometry.vbOffset;
        desc[i + 1].data.triangles.indexCount = mesh.indexCount;
        desc[i + 1].data.triangles.indexOffset = meta.startIndex;
        desc[i + 1].data.triangles.flags =
          (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
      }

      triangleCount += mesh.indexCount / 3;

      i += hasSecondaryGeometry ? 2 : 1;
    }

    raytrace::BottomAccelerationStructureBuildInfo buildInfo{};
    buildInfo.flags = RaytraceBuildFlags::FAST_TRACE | RaytraceBuildFlags::LOW_MEMORY;

    Context::BLASCompaction *compaction = context_id->beginBLASCompaction(objectId);
    if (compaction)
    {
      buildInfo.flags |= RaytraceBuildFlags::ALLOW_COMPACTION;
      buildInfo.compactedSizeOutputBuffer = context_id->compactedSizeBuffer.get();
      buildInfo.compactedSizeOutputBufferOffsetInBytes = compaction->compactedSizeOffset * sizeof(uint64_t);
      pendingCompactions.push_back(compaction);
      ++context_id->compactedSizeWritesInQueue;
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
    TIME_PROFILE(compaction_update);

    d3d::raytrace::build_acceleration_structure({
      .bottomBuilds = blasBuildInfos,
      .flushAfterBottomBuild = true,
    });

    for (auto compaction : pendingCompactions)
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
        if (context_id->compactedSizeBufferReadback && context_id->compactedSizeBufferReadback->lock(0, 0, &values, VBLOCK_READONLY))
        {
          G_ASSERT(
            context_id->compactedSizeBufferReadback->getSize() == context_id->compactedSizeBufferValues.size() * sizeof(uint64_t));
          memcpy(context_id->compactedSizeBufferValues.data(), values, context_id->compactedSizeBuffer->getSize());
          context_id->compactedSizeBufferReadback->unlock();
        }

        uint32_t count = 0;
        for (auto &compaction : context_id->blasCompactions)
          if (compaction.has_value())
          {
            if (compaction->stage == Context::BLASCompaction::Stage::SizeBeingRead)
            {
              ++count;
              compaction->stage = Context::BLASCompaction::Stage::SizeReceived;
              if constexpr (bufferReadback)
                compaction->compactedSizeValue = context_id->compactedSizeBufferValues[compaction->compactedSizeOffset];
            }
          }

        if constexpr (bufferReadback)
        {
          G_ASSERTF(count <= context_id->compactedSizeBufferSize,
            "bvh: too much compactions was requested increase compactedSizeBufferSize above %u", count);
        }
      }
    }

    if (!pendingCompactions.empty() || context_id->compactedSizeWritesInQueue)
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

  blasBuildInfos.clear();
  geomDescriptors.clear();

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
        context_id->blasCompactionsAccel.erase(iter->value().objectId);
        iter->reset();
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
#endif
          if (!compaction.compactedSizeValue)
          {
            cancelCompaction(context_id, iter);
            continue;
          }
          else if (
            context_id->numCompactionBlasesBeingCreated + context_id->numCompactionBlasesWaitingBuild < maxActiveCompactions * 2)
          {
            creationQueue.push_back(&compaction);
            compaction.stage = Context::BLASCompaction::Stage::WaitingGPUTime;
            context_id->numCompactionBlasesBeingCreated++;
          }
          break;
        }
        case Context::BLASCompaction::Stage::WaitingGPUTime:
        {
          if (!compaction.compactedBlas)
            break;

          if (activeCompactions >= maxActiveCompactions)
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

            ++activeCompactions;
            context_id->numCompactionBlasesWaitingBuild--;

            compaction.stage = Context::BLASCompaction::Stage::WaitingCompaction;

            // Compacting is a heavy operation.
            if (!isCompactionCheap)
              blasModelBudget -= 1;
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

    eastl::erase_if(context_id->createCompactedBLASJobQueue, [](const auto &job) { return interlocked_acquire_load(job->done); });

    if (!creationQueue.empty())
      CreateCompactedBLASJob::start(eastl::move(creationQueue), context_id);

    DA_PROFILE_TAG(blas_compactions, "%d of %d active compactions were processed", activeCompactions,
      (int)context_id->blasCompactionsAccel.size());

    // This is frequently needed for debugging.
    // logdbg("%d of %d active compactions were processed - creating: %d, waiting: %d", activeCompactions,
    //  (int)context_id->blasCompactionsAccel.size(), context_id->numCompactionBlasesBeingCreated.load(),
    //  context_id->numCompactionBlasesWaitingBuild.load());
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

  if (showBVHBuildEvents && triangleCount > 0)
    visuallog::logmsg(String(0, "The BVH build %d triangles for %d BLASes.", triangleCount, blasCount));
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

public:
  void start(ContextId context_id, bool upload_main_data, bool upload_per_instance_data)
  {
    G_ASSERT(upload_main_data || upload_per_instance_data);
    contextId = context_id;
    uploadMainData = upload_main_data;
    uploadPerInstanceData = upload_per_instance_data;
    threadpool::add(this, threadpool::JobPriority::PRIO_HIGH, true);
  }
  void doJob() override
  {
    if (uploadMainData)
    {
      bool mainUploadDone =
        parallel_instance_processing::upload_main_data(contextId, parallel_instance_processing::TargetFrame::Current);
      G_ASSERT(mainUploadDone);
      G_UNUSED(mainUploadDone);
    }
    if (uploadPerInstanceData)
    {
      bool perInstanceDataUploadDone =
        parallel_instance_processing::upload_per_instance_data(contextId, parallel_instance_processing::TargetFrame::Current);
      G_ASSERT(perInstanceDataUploadDone);
      G_UNUSED(perInstanceDataUploadDone);
    }
  }
  const char *getJobName(bool &) const override { return "BVHFallbackUploadHeavyDataJob"; }
  void wait()
  {
    TIME_PROFILE(wait_bvh_fallback_upload_heavy_data_job)
    threadpool::wait(this);
  }
} bvh_fallback_upload_heavy_data_job;


static void add_instances(ContextId context_id, const Context::InstanceMap &instances, dag::Vector<NativeInstance> &outInstances,
  uint32_t group_mask, bool is_camera_relative, const char *name, const Point3 &light_direction, const Point3 &camera_pos,
  eastl::unordered_set<void *, eastl::hash<void *>, eastl::equal_to<void *>, framemem_allocator> &allBlasUpdatesAs,
  const Frustum &frustumAbsolute, const Frustum &frustumRelative, const BufferProcessor *assumed_buffer_processor)
{
  CHECK_LOST_DEVICE_STATE();

  TIME_PROFILE_NAME(addInstance, name);
  DA_PROFILE_TAG(addInstance, "Instance count: %d", instances.size());

  const float maxLightDistForBvhShadow = 20.0f;
  const auto lightDirection = v_mul(v_ldu_p3_safe(&light_direction.x), v_splats(maxLightDistForBvhShadow * 2));
  auto &perInstanceData = context_id->perInstanceDataCpu;
  auto &blasUpdates = context_id->blasUpdates;
  auto &updateGeoms = context_id->updateGeoms;

  const auto &frustum = is_camera_relative ? &frustumRelative : &frustumAbsolute;
  const auto cameraPos = is_camera_relative ? v_zero() : v_ldu_p3_safe(&camera_pos.x);

  eastl::vector_set<int, eastl::less<int>, framemem_allocator> updateIndices;
  {
    // Calculate the skip pattern. Update indices hold the indices to be updated.
    updateIndices.set_capacity(context_id->riGenIndexTypePerFrame);

    float step = float(Context::MaxTreeAnimIndices) / context_id->riGenIndexTypePerFrame;
    for (int ix = 0; ix < context_id->riGenIndexTypePerFrame; ++ix)
    {
      int ui = (context_id->riGenStartIndexType + int(step * ix + 0.5)) % Context::MaxTreeAnimIndices;
      updateIndices.insert(ui);
    }
  }

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
    auto stationary = instance.uniqueIsStationary;
    auto skipUpdate = false;
    if (instance.animIndex > -1)
      skipUpdate = updateIndices.count(instance.animIndex) == 0;

    if (object.hasVertexProcessor)
    {
      bool needBlasBuild = false;

      auto &geoms = updateGeoms.emplace_back();

      for (auto [mesh, meta, baseMeta] : zip(object.meshes, metaRegion, baseMetaRegion))
      {
        if (!mesh.vertexProcessor)
          continue;
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
            meta.albedoTextureIndex = baseMeta.albedoTextureIndex;
            meta.normalTextureIndex = baseMeta.normalTextureIndex;
            meta.extraTextureIndex = baseMeta.extraTextureIndex;
          }
          meta.ahsVertexBufferIndex = baseMeta.ahsVertexBufferIndex;
          // Copy the index buffer index only, the vertex buffer is part of the instance
          meta.indexBufferIndex = baseMeta.indexBufferIndex;
        }

#if DAGOR_DBGLEVEL > 0
        static bool check_alpha = true;
        if (
          check_alpha && (meta.materialType & MeshMeta::bvhMaterialAlphaTest) && meta.ahsVertexBufferIndex == BVH_BINDLESS_BUFFER_MAX)
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

        uint32_t processedPositionFormat = mesh.processedPositionFormat;
        if ((!stationary && !skipUpdate) || !animatedVertices.get())
          process_instance_vertices(context_id, instance.objectId, mesh, &instance, frustum, cameraPos, lightDirection,
            animatedVertices, verticesRecycled, needBlasBuild, meta, processedPositionFormat);

        CHECK_LOST_DEVICE_STATE();

        meta.vertexOffset = animatedVertices.getOffset();
        bool hasAlphaTest = mesh.materialType & MeshMeta::bvhMaterialAlphaTest;
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
        geom.data.triangles.vertexFormat = processedPositionFormat;
        geom.data.triangles.indexCount = mesh.indexCount;
        geom.data.triangles.indexOffset = meta.startIndex;
        geom.data.triangles.flags =
          (hasAlphaTest) ? RaytraceGeometryDescription::Flags::NONE : RaytraceGeometryDescription::Flags::IS_OPAQUE;
        geom.extraDataAvailableMask.hasOpacityMicroMapLinkage = false;
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
    desc.instanceMask = instance.noShadow ? bvhGroupNoShadow : group_mask;
    desc.instanceContributionToHitGroupIndex = 0;
    desc.flags = flipWinding ? RaytraceGeometryInstanceDescription::TRIANGLE_CULL_FLIP_WINDING
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

  if (context_id->has(Features::AnyDynrend))
    dyn::wait_dynrend_instances();
  if (context_id->has(Features::AnyRI))
  {
    ri::wait_ri_gen_instances_update(context_id);
    ri::wait_ri_extra_instances_update(context_id);
  }

  const parallel_instance_processing::ParallelFinishResult parallelFinishResult =
    parallel_instance_processing::is_parallel_jobs_finished(context_id);

  Sbuffer *grassInstances = nullptr;
  Sbuffer *grassInstanceCount = nullptr;
  grass::get_instances(context_id, grassInstances, grassInstanceCount);

  Sbuffer *gobjInstances = nullptr;
  Sbuffer *gobjInstanceCount = nullptr;
  gobj::get_instances(context_id, gobjInstances, gobjInstanceCount);

  Sbuffer *fxInstances = nullptr;
  Sbuffer *fxInstanceCount = nullptr;
  fx::get_instances(context_id, fxInstances, fxInstanceCount);

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

  if (!instanceCount && !grassBufferSize && !gobjBufferSize && !fxBufferSize && !dagdpBufferSize && !gpuGrassInstances)
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

  auto uploadSizeMain = max(round_up(allInstancesCount, 1024 << 3), 1U << 17);
  auto uploadSizeTerrain = round_up(terrainBlases.size() + 1, 64);
  auto uploadSizeParticles = max(round_up(fxBufferSize, 1024), 1U << 13);
  auto uploadSizePerInstanceData = max(round_up(allInstancesCount, 1024), 1U << 13);

  if (context_id->tlasUploadMain && uploadSizeMain > context_id->tlasUploadMain->getNumElements())
    context_id->tlasUploadMain.close();
  if (context_id->tlasUploadTerrain && uploadSizeTerrain > context_id->tlasUploadTerrain->getNumElements())
    context_id->tlasUploadTerrain.close();
  if (context_id->tlasUploadParticles && uploadSizeParticles > context_id->tlasUploadParticles->getNumElements())
    context_id->tlasUploadParticles.close();
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

  {
    TIME_D3D_PROFILE(add_and_animate_instances);

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
        allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr);
      CHECK_LOST_DEVICE_STATE();
    }

    {
      context_id->riGenStartIndexType = (context_id->riGenStartIndexType + 1) % Context::MaxTreeAnimIndices;

      ProcessorInstances::getTreeVertexProcessor().begin();

      auto startRef = profile_ref_ticks();
      for (auto &instances : context_id->riGenInstances)
      {
        add_instances(context_id, instances, instanceDescs, bvhGroupRi, false, "riGen", light_direction, camera_pos, allBlasUpdatesAs,
          frustumAbsolute, frustumRelative, &ProcessorInstances::getTreeVertexProcessor());
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
          allBlasUpdatesAs, frustumAbsolute, frustumRelative, &ProcessorInstances::getTreeVertexProcessor());
        CHECK_LOST_DEVICE_STATE();
      }
      ProcessorInstances::getTreeVertexProcessor().end(false);
    }

    for (auto &instances : context_id->riExtraFlagInstances)
    {
      add_instances(context_id, instances, instanceDescs, bvhGroupRi, false, "riExtraFlag", light_direction, camera_pos,
        allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr);
      CHECK_LOST_DEVICE_STATE();
    }

    if (use_batched_skinned_vertex_processor)
      ProcessorInstances::getSkinnedVertexProcessorBatched().begin();
    for (auto &instances : context_id->dynrendInstances)
    {
      if (instances.first >= dynrend::ContextId{0})
        dyn::set_up_dynrend_context_for_processing(instances.first);
      add_instances(context_id, instances.second, instanceDescs, bvhGroupDynrend, true, "dynrend", light_direction, camera_pos,
        allBlasUpdatesAs, frustumAbsolute, frustumRelative,
        use_batched_skinned_vertex_processor ? &ProcessorInstances::getSkinnedVertexProcessorBatched() : nullptr);
      CHECK_LOST_DEVICE_STATE();
    }
    if (use_batched_skinned_vertex_processor)
      ProcessorInstances::getSkinnedVertexProcessorBatched().end(false);

    if (!context_id->splineGenInstances.empty())
    {
      add_instances(context_id, context_id->splineGenInstances, instanceDescs, bvhGroupDynrend, true, "splinegen", light_direction,
        camera_pos, allBlasUpdatesAs, frustumAbsolute, frustumRelative, nullptr);
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

    d3d::build_bottom_acceleration_structures(context_id->blasUpdates.data(), context_id->blasUpdates.size());
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
      tasbi.flags = tlasBuildFlags;
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

  static bool dumpStats = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhDumpMemoryStats", false);
  if (dumpStats)
  {
    auto mb = [](auto v) { return int(v == 0 ? 0 : eastl::max((v + 1024 * 1024 - 1) / (1024 * 1024), decltype(v)(1))); };

    auto stats = bvh::get_memory_statistics(context_id);

    logdbg("BVH memory statistics");
    logdbg("TLAS: %d MB - Upload: %d MB", mb(stats.tlasSize), mb(stats.tlasUploadSize));
    logdbg("Scratch space: %d MB", mb(stats.scratchBuffersSize));
    logdbg("Transform buffers: %d MB", mb(stats.transformBuffersSize));
    for (auto &[tag, bucket] : stats.meshStats)
      logdbg("* %s - Mesh count %d, Mesh VB: %d MB, Mesh BLAS: %d MB", tag ? tag : "untagged", bucket.meshCount, mb(bucket.meshVBSize),
        mb(bucket.meshBlasSize));
    logdbg("Mesh meta: %d MB", mb(stats.meshMetaSize));
    logdbg("Skin BLAS: %d MB, Skin VB: %d MB, Skin count: %d", mb(stats.skinBLASSize), mb(stats.skinVBSize), stats.skinCount);
    logdbg("Skin cache BLAS: %d MB, Skin cache count: %d", mb(stats.skinCacheBLASSize), stats.skinCacheCount);
    logdbg("RiGen Tree BLAS: %d MB, VB: %d MB, count: %d", mb(stats.treeBLASSize), mb(stats.treeVBSize), stats.treeCount);
    logdbg("Stat tree BLAS: %d MB, Stat tree VB: %d MB, Stat tree count: %d", mb(stats.stationaryTreeBLASSize),
      mb(stats.stationaryTreeVBSize), stats.stationaryTreeCount);
    logdbg("RiGen Tree cache BLAS: %d MB, count: %d", mb(stats.treeCacheBLASSize), stats.treeCacheCount);
    logdbg("RiEx Tree BLAS: %d MB,  VB: %d MB, count: %d", mb(stats.treeRiExBLASSize), mb(stats.treeRiExVBSize), stats.treeRiExCount);
    logdbg("RiEx Tree cache BLAS: %d MB, cache count: %d", mb(stats.treeRiExCacheBLASSize), stats.treeRiExCacheCount);
    logdbg("Flag BLAS: %d MB, VB: %d MB, count: %d", mb(stats.flagBLASSize), mb(stats.flagVBSize), stats.flagCount);
    logdbg("Dynamic VB allocator: %d MB, free %d MB", mb(stats.dynamicVBAllocatorSize), mb(stats.dynamicVBAllocatorFreeSize));
    logdbg("Terrain BLAS: %d MB, Terrain VB: %d MB", mb(stats.terrainBlasSize), mb(stats.terrainVBSize));
    logdbg("Grass BLAS: %d MB, Grass VB: %d MB, Grass IB: %d MB, Grass meta: %d MB, Grass query: %d MB", mb(stats.grassBlasSize),
      mb(stats.grassVBSize), mb(stats.grassIBSize), mb(stats.grassMetaSize), mb(stats.grassQuerySize));
    logdbg("Cable BLAS: %d MB, Cable VB: %d MB, Cable IB: %d MB", mb(stats.cableBLASSize), mb(stats.cableVBSize),
      mb(stats.cableIBSize));
    logdbg("bin scene instances: %d", stats.binSceneCount);
    logdbg("Water instances: %d Water BLAS: %d MB, Water VB: %d MB, Water IB: %d MB", stats.waterCount, mb(stats.waterBLASSize),
      mb(stats.waterVBSize), mb(stats.waterIBSize));
    logdbg("GPUGrass instances: %d GPUGrass Memory: %d MB GPUGrass Textures Memory: %d MB", stats.gpuGrassCount,
      mb(stats.gpuGrassMemory), mb(stats.gpuGrassTexturesMemory));
    logdbg("GPU object meta: %d MB, GPU object query: %d MB", mb(stats.gobjMetaSize), mb(stats.gobjQuerySize));
    logdbg("Per instance data: %d MB", mb(stats.perInstanceDataSize));
    logdbg("Compaction data: %d MB - Peak: %d MB - Count: %d - SizeBufferSize: %d", mb(stats.compactionSize),
      mb(stats.peakCompactionSize), stats.compactionCount, stats.compactionSizeBufferSize);
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

void set_rigen_cpu_budget(int budget_us) { bvh_riGen_budget_us = budget_us; }


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
  static int bvh_gbuffer_range_startVarId = get_shader_variable_id("bvh_gbuffer_range_start");

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

  auto registerGbufferTexture = [br = context_id->gbufferBindlessRange](Texture *t, int i) {
    d3d::resource_barrier({t, RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    d3d::update_bindless_resource(D3DResourceType::TEX, br + i, t);
  };

  if (gbuffer_albedo)
    registerGbufferTexture(gbuffer_albedo, bvhGbufferAlbedo);
  if (gbuffer_normal)
    registerGbufferTexture(gbuffer_normal, bvhGbufferNormal);
  if (gbuffer_material)
    registerGbufferTexture(gbuffer_material, bvhGbufferMaterial);
  if (gbuffer_motion)
    registerGbufferTexture(gbuffer_motion, bvhGbufferMotion);
  if (gbuffer_depth)
    registerGbufferTexture(gbuffer_depth, bvhGbufferDepth);

  ShaderGlobal::set_int(bvh_gbuffer_range_startVarId, context_id->gbufferBindlessRange);
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
#else



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
#else





#endif

  ShaderGlobal::set_int(bvh_main_validVarId, context_id->tlasMainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_terrain_validVarId, context_id->tlasTerrainValid ? 1 : 0);
  ShaderGlobal::set_int(bvh_particles_validVarId, context_id->tlasParticlesValid ? 1 : 0);

  ShaderGlobal::set_int(bvh_meta_countVarId, context_id->meshMetaAllocator.size());
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

void on_unload_scene(ContextId context_id)
{
  dyn::on_unload_scene(context_id);
  ri::on_unload_scene(context_id);
  grass::on_unload_scene(context_id);
  gobj::on_unload_scene(context_id);
  fx::on_unload_scene(context_id);
  binscene::on_unload_scene(context_id);
  splinegen::on_unload_scene(context_id);
  fftwater::on_unload_scene(context_id);
  gpugrass::on_unload_scene(context_id);
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
    context_id->releaseTexture(TEXTUREID(texture.first));
  context_id->camoTextures.clear();

  for (auto &[alloc, _] : context_id->processBufferAllocator)
    context_id->releaseBuffer(alloc.getHeap().getBuf());

  context_id->atmosphereDirty = true;

  context_id->instanceDescsCpu.clear();
  context_id->instanceDescsCpu.shrink_to_fit();
  context_id->perInstanceDataCpu.clear();
  context_id->perInstanceDataCpu.shrink_to_fit();
  context_id->processBufferAllocator.clear();
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
  return !context_id->halfBakedObjects.empty() || context_id->hasPendingObjectAddActions.load(dag::memory_order_relaxed);
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
