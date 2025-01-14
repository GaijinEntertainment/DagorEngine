// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "context.h"
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_info.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_convar.h>

namespace convar
{
CONSOLE_INT_VAL("dafx", force_sim_lod, -1, -1, 7);
CONSOLE_BOOL_VAL("dafx", allow_sim_lods, true);
CONSOLE_BOOL_VAL("dafx", allow_visibility_sim_lods, true);
CONSOLE_FLOAT_VAL("dafx", visibility_sim_lod_min_fps, 0);
} // namespace convar

namespace dafx
{
GenerationReferencedData<ContextId, Context, uint8_t, 0> g_ctx_list;
static Config mainCtxConfig;

void finish_async_update(Context &ctx);
void exec_command_queue(Context &ctx);
void swap_command_queue(Context &ctx);
void clear_command_queue(Context &ctx);
void clear_all_command_queues_locked(Context &ctx);
void populate_free_instances(Context &ctx);

SamplerHandles::SamplerHandles(float in_mipBias) : mipBias(in_mipBias)
{
  d3d::SamplerInfo samplerInfo;
  samplerInfo.address_mode_u = d3d::AddressMode::Clamp;
  samplerInfo.address_mode_v = d3d::AddressMode::Clamp;
  samplerInfo.mip_map_mode = d3d::MipMapMode::Linear;
  samplerInfo.mip_map_bias = mipBias;
  samplerInfo.anisotropic_max = 1.f;
  samplerBilinear = d3d::request_sampler(samplerInfo);

  samplerInfo.anisotropic_max = float(::dgs_tex_anisotropy);
  samplerAniso = d3d::request_sampler(samplerInfo);
}
void SamplerHandles::close() { mipBias = invalidMipBias; }
SamplerHandles::~SamplerHandles() { close(); }
SamplerHandles::SamplerHandles(SamplerHandles &&other) noexcept { *this = eastl::move(other); }
SamplerHandles &SamplerHandles::operator=(SamplerHandles &&other) noexcept
{
  eastl::swap(mipBias, other.mipBias);
  eastl::swap(samplerBilinear, other.samplerBilinear);
  eastl::swap(samplerAniso, other.samplerAniso);
  other.close();
  return *this;
}

static inline void start_job(const Config &cfg, cpujobs::IJob &job, bool wake = true)
{
  using namespace threadpool;
  if (cfg.use_async_thread)
    add(&job, cfg.low_prio_jobs ? PRIO_LOW : PRIO_DEFAULT, wake);
  else
    job.doJob();
}

ContextId create_context(const Config &cfg)
{
  DBG_OPT("create_context - start");

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

  ContextId cid = g_ctx_list.emplaceOne();
  Context *ctx = g_ctx_list.get(cid);

  bool v = set_config(cid, cfg);

  G_ASSERT(cfg.staging_buffer_size % DAFX_ELEM_STRIDE == 0);

  ctx->systemDataVarId = ::get_shader_variable_id("dafx_system_data", true);
  ctx->renderCallsVarId = ::get_shader_variable_id("dafx_render_calls", true);
  ctx->computeDispatchVarId = ::get_shader_variable_id("dafx_dispatch_descs", true);
  ctx->updateGpuCullingVarId = ::get_shader_variable_id("dafx_update_gpu_culling", true);

  v &= init_global_values(ctx->globalData);
  v &= init_buffer_pool(ctx->gpuBufferPool, cfg.data_buffer_size);
  v &= init_buffer_pool(ctx->cpuBufferPool, cfg.data_buffer_size);
  v &= init_culling(*ctx);

  ctx->frameBoundaryBufferManager.init(cfg.use_render_sbuffer, cfg.approx_boundary_computation);

  ctx->supportsNoOverwrite = d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers;
  debug("dafx: using %d threads", ctx->cfg.use_async_thread ? ctx->cfg.max_async_threads : 0);
  debug("dafx: sbuffer support: %d, multidraw: %d", cfg.use_render_sbuffer ? 1 : 0, cfg.multidraw_enabled ? 1 : 0);

  ctx->asyncCpuComputeJobs.reserve(ctx->cfg.max_async_threads);
  ctx->asyncCpuCullJobs.reserve(ctx->cfg.max_async_threads);

  ShaderGlobal::set_int(::get_shader_variable_id("dafx_sbuffer_supported", true), cfg.use_render_sbuffer ? 1 : 0);

  // we are using instancing helper even for non-multidraw, to keep shaders the same
  VSDTYPE desc[] = {VSD_STREAM_PER_INSTANCE_DATA(0), VSD_REG(VSDR_TEXC0, VSDT_UINT1), VSD_END};
  ctx->instancingVdecl = d3d::create_vdecl(desc);
  G_ASSERT(ctx->instancingVdecl != BAD_VDECL);

  ctx->serialBuf.init(ctx->cfg.multidraw_enabled ? ctx->cfg.max_multidraw_batch_size : 1);

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  os_spinlock_init(&ctx->queueLock);

  if (!v)
  {
    g_ctx_list.destroyReference(cid);
    return ContextId();
  }

  debug("dafx: create_context - done, rid: %d", (uint32_t)cid);
  return cid;
}

void clear_internal_workers(Context &ctx, bool force)
{
  reset_culling(ctx, force);

  for (int i = 0; i < Config::max_system_depth; ++i)
  {
    ctx.gpuEmissionWorkers[i].clear();
    ctx.gpuSimulationWorkers[i].clear();
  }
  if (force)
    ctx.allEmissionWorkers.clear();

  ctx.allRenderWorkers.clear();
  for (int i = 0, ie = ctx.cpuRenderWorkers.size(); i < ie; ++i)
    ctx.cpuRenderWorkers[i].clear();

  ctx.gpuTransferWorkers.clear();
}

void shrink_instances(ContextId cid, int keep_allocated_count)
{
  GET_CTX();
  for (int i = 0; i < ctx.instances.list.totalSize(); ++i)
  {
    InstanceId iid = ctx.instances.list.getRefByIdx(i);
    if (!iid)
      continue;

    int *psid = ctx.instances.list.get(iid);
    G_ASSERT_CONTINUE(psid);

    if (*psid == queue_instance_sid || *psid == dummy_instance_sid || ctx.instances.groups.get<INST_PARENT_SID>(*psid) == *psid)
      destroy_instance(cid, iid); // queue only root instances
  }

  flush_command_queue(cid);
  populate_free_instances(ctx);
  exec_delayed_page_destroy(ctx.gpuBufferPool);

  G_ASSERT(ctx.instances.list.empty());
  G_ASSERT(ctx.instances.groups.size() == ctx.instances.freeIndicesAvailable.size());
  G_ASSERT(ctx.cpuBufferPool.pages.empty());
  G_ASSERT(ctx.gpuBufferPool.pages.empty());

  // this is just in case something went wrong
  ctx.cpuBufferPool.pages.clear();
  ctx.gpuBufferPool.pages.clear();

  ctx.instances.groups.shrink_to_size(keep_allocated_count);
  ctx.instances.freeIndicesPending.clear();
  ctx.instances.freeIndicesAvailable.clear();
  for (int i = 0, ic = ctx.instances.groups.size(); i < ic; ++i)
    ctx.instances.freeIndicesAvailable.push_back(i);

  clear_internal_workers(ctx, true);
}

void clear_instances(ContextId cid, int keep_allocated_count)
{
  GET_CTX();
  clear_all_command_queues_locked(ctx);
  finish_async_update(ctx);
  shrink_instances(cid, keep_allocated_count);
}

void release_all_systems(ContextId cid)
{
  GET_CTX();
  clear_all_command_queues_locked(ctx);
  finish_async_update(ctx);
  shrink_instances(cid, 0);

  G_ASSERT(interlocked_acquire_load(ctx.systemsLockCounter) == 0);
  for (int i = 0; i < ctx.systems.list.totalSize(); ++i)
  {
    SystemId rid = ctx.systems.list.getRefByIdx(i);
    if (rid)
      release_system(cid, rid);
  }
  G_ASSERT(ctx.systems.list.empty());
}

void release_context(ContextId cid)
{
  debug("dafx: release_context, rid: %d", (uint32_t)cid);
  GET_CTX();

  finish_async_update(ctx);
  ctx.staging.ring.reset(0);
  d3d::delete_vdecl(ctx.instancingVdecl);

  if (!ctx.cullingStates.empty())
  {
    for (int i = 0; i < ctx.cullingStates.totalSize(); ++i)
    {
      CullingId cullId = ctx.cullingStates.getRefByIdx(i);
      if (cullId)
        destroy_culling_state(cid, cullId);
    }

    G_ASSERT(ctx.cullingStates.empty());
  }

  release_all_systems(cid);
  os_spinlock_destroy(&ctx.queueLock);

  g_ctx_list.destroyReference(cid);
}

void before_reset_device(ContextId cid)
{
  GET_CTX();
  os_spinlock_lock(&ctx.queueLock);
  clear_command_queue(ctx);
  os_spinlock_unlock(&ctx.queueLock);
  debug("dafx: before_reset_device");
}

void after_reset_device(ContextId cid)
{
  GET_CTX();

  os_spinlock_lock(&ctx.queueLock);
  clear_command_queue(ctx);
  os_spinlock_unlock(&ctx.queueLock);

  debug("dafx: after_reset_device");

  finish_async_update(ctx); // we need to finish emitters async job, before instance reset
  reset_instances_after_reset(ctx);

  ctx.samplersCache.clear();
  ctx.frameBoundaryBufferManager.afterDeviceReset();

  for (int i = 0, ie = ctx.renderBuffers.size(); i < ie; ++i)
  {
    ctx.renderBuffers[i].allocSize = 0;
    ctx.renderBuffers[i].usageSize = 0;
    ctx.renderBuffers[i].maxUsage = 0;
    ctx.renderBuffers[i].res.close();
  }

  clear_internal_workers(ctx, false);
}

void prepare_sim_lods(Context &ctx, float dt, int begin_sid, int end_sid)
{
  bool allowSimulationLods = ctx.cfg.sim_lods_enabled && convar::allow_sim_lods;
  bool allowVisibilitySimLods =
    allowSimulationLods && ctx.cfg.gen_sim_lods_for_invisible_instances && convar::allow_visibility_sim_lods;

  InstanceGroups &stream = ctx.instances.groups;

  int maxAllowedLod = Config::max_simulation_lods - 1;
  float minSimLodDtTick =
    convar::visibility_sim_lod_min_fps > 0 ? 1.f / convar::visibility_sim_lod_min_fps : ctx.cfg.min_sim_lod_dt_tick;
  for (int i = 1; i <= Config::max_simulation_lods; ++i)
  {
    int lodOfs = 1 << i;
    if (dt * lodOfs > minSimLodDtTick)
    {
      maxAllowedLod = i - 1;
      break;
    }
  }

#if DAGOR_DBGLEVEL > 0
  // we need to clear debug lods on those console command
  if (convar::allow_sim_lods.pullValueChange() || convar::allow_visibility_sim_lods.pullValueChange())
  {
    for (int i = begin_sid; i < end_sid; ++i)
      stream.get<INST_LOD>(i) = 0;
  }
#endif

  const uint32_t cpuSimMask = SYS_ENABLED | SYS_CPU_SIMULATION_REQ | SYS_ALLOW_SIMULATION_LODS;
  const uint32_t gpuSimMask = SYS_ENABLED | SYS_GPU_SIMULATION_REQ | SYS_ALLOW_SIMULATION_LODS;
  const uint32_t allSimMask = cpuSimMask | gpuSimMask;

  if (allowVisibilitySimLods)
  {
    for (int i = begin_sid; i < end_sid; ++i)
    {
      uint32_t flags = stream.get<INST_FLAGS>(i);
      uint32_t m = flags & allSimMask;
      const SimulationState &simulationState = stream.get<INST_SIMULATION_STATE>(i);
      if (simulationState.count > 0 && (m == cpuSimMask || m == gpuSimMask))
      {
        // invalid box could mean that inst is in-between part spawning, so it should be not lod-ed out
        stream.get<INST_LOD>(i) = flags & SYS_VISIBLE ? 0 : (flags & SYS_BBOX_VALID ? (uint8_t)maxAllowedLod : 0);
      }
    }
  }

#if DAGOR_DBGLEVEL > 0
  if (allowSimulationLods && convar::force_sim_lod >= 0)
  {
    for (int i = begin_sid; i < end_sid; ++i)
    {
      uint32_t flags = stream.get<INST_FLAGS>(i);
      uint32_t m = flags & allSimMask;
      if (m == cpuSimMask || m == gpuSimMask)
        stream.get<INST_LOD>(i) = (uint8_t)convar::force_sim_lod;
    }
  }
#else
  G_UNUSED(allowSimulationLods);
#endif

  stat_set(ctx.stats.genVisibilityLod, maxAllowedLod);
}

void prepare_workers(Context &ctx, float dt, bool main_pass, int begin_sid, int end_sid, bool gpu_fetch)
{
  //
  // TODO: fetch prev culling data at start so we can perform inital bbox filling in thread
  //

  TIME_PROFILE(dafx_prepare_workers);
  InstanceGroups &stream = ctx.instances.groups;

  prepare_sim_lods(ctx, dt, begin_sid, end_sid);

  for (int i = begin_sid; i < end_sid; ++i)
  {
    const uint32_t flags = stream.get<INST_FLAGS>(i);
    if (!(flags & SYS_ENABLED))
      continue;

    G_FAST_ASSERT(flags & SYS_VALID);

    const int &depth = stream.get<INST_DEPTH>(i);
    const EmissionState &emissionState = stream.get<INST_EMISSION_STATE>(i);
    const SimulationState &simulationState = stream.get<INST_SIMULATION_STATE>(i);
    const InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(i);

    if (emissionState.count > 0)
    {
      if (flags & SYS_CPU_EMISSION_REQ)
        ctx.cpuEmissionWorkers[depth].push_back(i);

      if (flags & SYS_GPU_EMISSION_REQ)
        ctx.gpuEmissionWorkers[depth].push_back(i);

      // we allow emission without shaders
      ctx.allEmissionWorkers.push_back(i);
    }

    bool doSimulation = simulationState.count > 0;
    uint8_t simLod = stream.get<INST_LOD>(i);
    uint32_t newFlags = flags & ~SYS_SKIP_SIMULATION_ON_THIS_FRAME;
    if (doSimulation && simLod > 0)
    {
      unsigned int frame = ctx.currentFrame + i; // mixing instance stream idx for better job balancing
      doSimulation = frame % (1 << simLod) == 0;
      newFlags |= (doSimulation ? 0 : SYS_SKIP_SIMULATION_ON_THIS_FRAME);
    }
    stream.get<INST_FLAGS>(i) = newFlags;

    if (doSimulation)
    {
      if (flags & SYS_CPU_SIMULATION_REQ)
        ctx.cpuSimulationWorkers[depth].push_back(i);

      if (flags & SYS_GPU_SIMULATION_REQ)
        ctx.gpuSimulationWorkers[depth].push_back(i);
    }

    if (main_pass && (flags & SYS_GPU_TRANFSER_REQ))
      ctx.gpuTransferWorkers.push_back(i);

    if (flags & SYS_RENDER_REQ)
    {
      G_FAST_ASSERT(activeState.aliveCount > 0);
      G_FAST_ASSERT(flags & SYS_RENDERABLE);
      G_FAST_ASSERT(flags & (SYS_CPU_RENDER_REQ | SYS_GPU_RENDER_REQ));
      G_UNREFERENCED(activeState);

      ctx.allRenderWorkers.push_back(i);

      if (flags & SYS_CPU_RENDER_REQ)
      {
        stream.get<INST_CULLING_ID>(i) = ctx.culling.cpuCullIdx++;
        ctx.culling.cpuWorkers.push_back(i);
      }

      if ((flags & SYS_GPU_RENDER_REQ) && gpu_fetch)
      {
        stream.get<INST_CULLING_ID>(i) = ctx.culling.gpuCullIdx++;
        ctx.culling.gpuWorkers.push_back(i);
      }
    }

    stat_inc(ctx.stats.activeInstances);

    if (flags & SYS_CPU_SIMULATION_REQ)
      stat_add(ctx.stats.cpuElemTotalSimLods[simLod], simulationState.count);
    else if (flags & SYS_GPU_SIMULATION_REQ)
      stat_add(ctx.stats.gpuElemTotalSimLods[simLod], simulationState.count);
  }

  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    stat_add(ctx.stats.cpuSimulationWorkers, ctx.cpuSimulationWorkers[i].size());
    stat_add(ctx.stats.cpuEmissionWorkers, ctx.cpuEmissionWorkers[i].size());
  }

  stat_set(ctx.stats.allRenderWorkers, ctx.allRenderWorkers.size());
  stat_set(ctx.stats.totalInstances, stream.size() - ctx.instances.freeIndicesAvailable.size());
}


void prepare_render_buffers_state(Context &ctx)
{
  TIME_PROFILE(prepare_render_buffers_state);

  // Allow overwrite of previous buffers data
  ctx.currentRenderDispatchBuffer = 0;
  ctx.currentMutltidrawBuffer = 0;
  ctx.beforeRenderUpdatedTags = 0;

  // Reset SYS_RENDER_BUF_TAKEN state
  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.allRenderWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.allRenderWorkers[ii];
    uint32_t &flags = stream.get<INST_FLAGS>(sid); // -V758

    flags &= ~SYS_RENDER_BUF_TAKEN;
  }
}

void prepare_next_emitters(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_prepare_next_emission);

  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.allEmissionWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.allEmissionWorkers[ii];

    uint32_t &flags = stream.get<INST_FLAGS>(sid);
    InstanceState &instState = stream.get<INST_ACTIVE_STATE>(sid);
    EmissionState &emissionState = stream.get<INST_EMISSION_STATE>(sid);
    SimulationState &simulationState = stream.get<INST_SIMULATION_STATE>(sid);

    emissionState.start = 0;
    emissionState.count = 0;
    simulationState.start = instState.aliveStart;
    simulationState.count = instState.aliveCount;

    // last elem was emitted
    if (!(flags & SYS_EMITTER_REQ))
      flags &= ~(SYS_CPU_EMISSION_REQ | SYS_GPU_EMISSION_REQ);

    stream.get<INST_SYNCED_FLAGS>(sid) = flags;
  }
  ctx.allEmissionWorkers.clear();

  for (int sid : ctx.culling.cpuWorkers)
  {
    if (stream.get<INST_FLAGS>(sid) & SYS_BBOX_VALID)
      stream.get<INST_LAST_VALID_BBOX_FRAME>(sid) = ctx.currentFrame;
  }
  ctx.culling.cpuWorkers.clear();
  ctx.culling.gpuWorkers.clear();

  if (ctx.culling.gpuFetchedIdx >= 0)
  {
    CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[ctx.culling.gpuFetchedIdx];
    for (int sid : feedback.frameWorkers)
    {
      if (stream.get<INST_FLAGS>(sid) & SYS_BBOX_VALID)
        stream.get<INST_LAST_VALID_BBOX_FRAME>(sid) = ctx.currentFrame;
    }
    feedback.frameWorkers.clear();
  }
}

int prepare_gpu_transfer(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_prepare_gpu_transfer);

  int totalSize = 0;
  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.gpuTransferWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.gpuTransferWorkers[ii];

    uint32_t &flags = stream.get<INST_FLAGS>(sid);
    if (flags & SYS_GPU_TRANFSER_PENDING || !(flags & SYS_VALID)) // alredy in the list or dead
      continue;

    G_FAST_ASSERT(flags & SYS_GPU_TRANFSER_REQ);

    const DataTransfer &htransf = stream.get<INST_HEAD_DATA_TRANSFER>(sid);
    const DataTransfer &rtransf = stream.get<INST_RENDER_DATA_TRANSFER>(sid);
    const DataTransfer &stransf = stream.get<INST_SIMULATION_DATA_TRANSFER>(sid);
    const DataTransfer &srvtransf = stream.get<INST_SERVICE_DATA_TRANSFER>(sid);

    int sz = htransf.size + rtransf.size + stransf.size + srvtransf.size;
    if (sz == 0) // dead
      continue;

    G_FAST_ASSERT(flags & SYS_VALID);

    totalSize += sz;
    flags &= ~SYS_GPU_TRANFSER_REQ;
    flags |= SYS_GPU_TRANFSER_PENDING;
  }
  return totalSize;
}

bool prepare_gpu_staging(Context &ctx, int staging_size)
{
  TIME_D3D_PROFILE(dafx_prepare_gpu_staging);

  unsigned char *ptr = start_updating_staging(ctx, staging_size);
  if (!ptr)
    return false;

  DA_PROFILE_TAG(dafx_prepare_gpu_staging, "workers: %d, size: %d", (int)ctx.gpuTransferWorkers.size(), staging_size);

  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.gpuTransferWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.gpuTransferWorkers[ii];
    if (!(stream.get<INST_FLAGS>(sid) & SYS_GPU_TRANFSER_PENDING))
      continue;

    const CpuBuffer &cpuBuffer = stream.get<INST_CPU_BUFFER>(sid);
    const DataTransfer &htransf = stream.get<INST_HEAD_DATA_TRANSFER>(sid);
    const DataTransfer &rtransf = stream.get<INST_RENDER_DATA_TRANSFER>(sid);
    const DataTransfer &stransf = stream.get<INST_SIMULATION_DATA_TRANSFER>(sid);
    const DataTransfer &srvtransf = stream.get<INST_SERVICE_DATA_TRANSFER>(sid);

#if DAGOR_DBGLEVEL > 0
    validate_cpu_buffer(cpuBuffer, ctx.cpuBufferPool);
#endif

    G_FAST_ASSERT(htransf.size == 0 || (htransf.srcOffset + htransf.size <= cpuBuffer.size));
    G_FAST_ASSERT(rtransf.size == 0 || (rtransf.srcOffset + rtransf.size <= cpuBuffer.size));
    G_FAST_ASSERT(stransf.size == 0 || (stransf.srcOffset + stransf.size <= cpuBuffer.size));
    G_FAST_ASSERT(srvtransf.size == 0 || (srvtransf.srcOffset + srvtransf.size <= cpuBuffer.size));

    if (htransf.size > 0)
    {
      memcpy(ptr, cpuBuffer.directPtr + cpuBuffer.offset + htransf.srcOffset, htransf.size);
      ptr += htransf.size;
    }

    if (rtransf.size > 0)
    {
      memcpy(ptr, cpuBuffer.directPtr + cpuBuffer.offset + rtransf.srcOffset, rtransf.size);
      ptr += rtransf.size;
    }

    if (stransf.size > 0)
    {
      memcpy(ptr, cpuBuffer.directPtr + cpuBuffer.offset + stransf.srcOffset, stransf.size);
      ptr += stransf.size;
    }

    if (srvtransf.size > 0)
    {
      memcpy(ptr, cpuBuffer.directPtr + cpuBuffer.offset + srvtransf.srcOffset, srvtransf.size);
      ptr += srvtransf.size;
    }
  }

  stop_updating_staging(ctx);
  return true;
}

void transfser_gpu_staging(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_transfser_gpu_staging);

  int stagingOffset = ctx.staging.ofs;
  Sbuffer *staging = ctx.staging.buffer.getBuf();
  G_ASSERT_RETURN(staging, );

  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.gpuTransferWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.gpuTransferWorkers[ii];
    uint32_t &flags = stream.get<INST_FLAGS>(sid);
    if (!(flags & SYS_GPU_TRANFSER_PENDING))
      continue;

    GpuBuffer &gpuBuffer = stream.get<INST_GPU_BUFFER>(sid);
    DataTransfer &htransf = stream.get<INST_HEAD_DATA_TRANSFER>(sid);
    DataTransfer &rtransf = stream.get<INST_RENDER_DATA_TRANSFER>(sid);
    DataTransfer &stransf = stream.get<INST_SIMULATION_DATA_TRANSFER>(sid);
    DataTransfer &srvtransf = stream.get<INST_SERVICE_DATA_TRANSFER>(sid);

#if DAGOR_DBGLEVEL > 0
    validate_gpu_buffer(gpuBuffer, ctx.gpuBufferPool);
#endif

    G_FAST_ASSERT(htransf.size == 0 || (htransf.dstOffset + htransf.size <= gpuBuffer.size));
    G_FAST_ASSERT(rtransf.size == 0 || (rtransf.dstOffset + rtransf.size <= gpuBuffer.size));
    G_FAST_ASSERT(stransf.size == 0 || (stransf.dstOffset + stransf.size <= gpuBuffer.size));
    G_FAST_ASSERT(srvtransf.size == 0 || (srvtransf.dstOffset + srvtransf.size <= gpuBuffer.size));

    stat_inc(ctx.stats.gpuTransferCount);
    stat_add(ctx.stats.gpuTransferSize, htransf.size + rtransf.size + stransf.size + srvtransf.size);

    if (htransf.size > 0)
    {
      staging->copyTo(gpuBuffer.directPtr, gpuBuffer.offset + htransf.dstOffset, stagingOffset, htransf.size);

      stagingOffset += htransf.size;
      htransf.srcOffset = 0xffffffff;
      htransf.dstOffset = 0xffffffff;
      htransf.size = 0;
    }

    if (rtransf.size > 0)
    {
      staging->copyTo(gpuBuffer.directPtr, gpuBuffer.offset + rtransf.dstOffset, stagingOffset, rtransf.size);

      stagingOffset += rtransf.size;
      rtransf.srcOffset = 0xffffffff;
      rtransf.dstOffset = 0xffffffff;
      rtransf.size = 0;
    }

    if (stransf.size > 0)
    {
      staging->copyTo(gpuBuffer.directPtr, gpuBuffer.offset + stransf.dstOffset, stagingOffset, stransf.size);

      stagingOffset += stransf.size;
      stransf.srcOffset = 0xffffffff;
      stransf.dstOffset = 0xffffffff;
      stransf.size = 0;
    }

    if (srvtransf.size > 0)
    {
      staging->copyTo(gpuBuffer.directPtr, gpuBuffer.offset + srvtransf.dstOffset, stagingOffset, srvtransf.size);

      stagingOffset += srvtransf.size;
      srvtransf.srcOffset = 0xffffffff;
      srvtransf.dstOffset = 0xffffffff;
      srvtransf.size = 0;
    }

    flags &= ~SYS_GPU_TRANFSER_PENDING;
  }

  DA_PROFILE_TAG(dafx_transfser_gpu_staging, "count: %d, size: %d", ctx.stats.gpuTransferCount, ctx.stats.gpuTransferSize);
}

void update_frame_boundary(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_update_frame_boundary);
  ctx.frameBoundaryBufferManager.update(ctx.currentFrame);
}

void updateDafxFrameBounds(ContextId cid)
{
  GET_CTX();
  update_frame_boundary(ctx);
}

void update_gpu_transfer(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_update_gpu_transfer);

  int stagingSize = prepare_gpu_transfer(ctx);
  if (stagingSize == 0)
  {
    ctx.gpuTransferWorkers.clear();
    return;
  }

  if (!prepare_gpu_staging(ctx, stagingSize))
  {
    ctx.gpuTransferWorkers.clear();
    return;
  }

  transfser_gpu_staging(ctx);

  ctx.gpuTransferWorkers.clear();
}

void update_gpu_compute_tasks(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_gpu_compute_tasks);

  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    update_gpu_emission(ctx, ctx.gpuEmissionWorkers[i]);
    stat_add(ctx.stats.gpuEmissionWorkers, ctx.gpuEmissionWorkers[i].size());
    ctx.gpuEmissionWorkers[i].clear();
  }

  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    update_gpu_simulation(ctx, ctx.gpuSimulationWorkers[i]);
    stat_add(ctx.stats.gpuSimulationWorkers, ctx.gpuSimulationWorkers[i].size());
    ctx.gpuSimulationWorkers[i].clear();
  }

  DA_PROFILE_TAG(dafx_gpu_compute_tasks, "em: %d, sim: %d", ctx.stats.gpuEmissionWorkers, ctx.stats.gpuSimulationWorkers);
}

void sort_shader_tasks(Context &ctx)
{
  TIME_PROFILE(dafx_sort_shader);
  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    sort_gpu_shader_tasks<Context, EmissionState, INST_GPU_BUFFER, INST_EMISSION_GPU_SHADER>(ctx, ctx.gpuEmissionWorkers[i]);
    sort_gpu_shader_tasks<Context, SimulationState, INST_GPU_BUFFER, INST_SIMULATION_GPU_SHADER>(ctx, ctx.gpuSimulationWorkers[i]);
  }
}

void gather_warmup_instances(Context &ctx, int sid, eastl::vector<int> &dst)
{
  const int gpuReq = SYS_GPU_EMISSION_REQ | SYS_GPU_SIMULATION_REQ;
  const uint32_t flags = ctx.instances.groups.get<INST_FLAGS>(sid);
  if ((flags & SYS_EMITTER_REQ) && !(flags & gpuReq))
    dst.push_back(sid);

  const eastl::vector<int> &subinstances = ctx.instances.groups.get<INST_SUBINSTANCES>(sid); // -V758
  for (int sub : subinstances)
    gather_warmup_instances(ctx, sub, dst);
};

void warmup_instance_from_queue(Context &ctx)
{
  // note: slow per-instance simulation
  // but we are not expecting to warmup more that just a couple of instances per frame
  if (ctx.commandQueue.instanceWarmup.empty())
    return;

  TIME_D3D_PROFILE(dafx_warmup);

  G_ASSERT(ctx.allEmissionWorkers.empty());
  G_ASSERT(ctx.gpuTransferWorkers.empty());
  G_ASSERT(ctx.culling.cpuWorkers.empty());
  G_ASSERT(ctx.culling.gpuWorkers.empty());

  float origDt = 0;
  get_global_value(ctx, ctx.cfg.dt_global_id, &origDt, sizeof(float));

  const unsigned int simSteps = clamp((unsigned int)(ctx.cfg.warmup_sims_budged / ctx.commandQueue.instanceWarmup.size()), 1U,
    ctx.cfg.max_warmup_steps_per_instance);

  eastl::vector<int> subinstances;

  for (const CommandQueue::InstanceWarmup &cq : ctx.commandQueue.instanceWarmup)
  {
    int *psid = ctx.instances.list.get(cq.iid);
    if (!psid) // could be dead
      continue;

    subinstances.clear();
    gather_warmup_instances(ctx, *psid, subinstances);

    float ldt = cq.time / simSteps;
    set_global_value(ctx, ctx.cfg.dt_global_id, &ldt, sizeof(float));

    for (int sub : subinstances)
    {
      for (int i = 0; i < simSteps; ++i)
      {
        update_emitters(ctx, ldt, sub, sub + 1);
        prepare_workers(ctx, ldt, false, sub, sub + 1, false);
        prepare_cpu_culling(ctx, false);

        for (int d = 0; d < ctx.cfg.max_system_depth; ++d)
        {
          G_ASSERT(ctx.gpuEmissionWorkers[d].empty());
          G_ASSERT(ctx.gpuSimulationWorkers[d].empty());
          update_cpu_emission(ctx, ctx.cpuEmissionWorkers[d], 0, ctx.cpuEmissionWorkers[d].size());
          update_cpu_simulation(ctx, ctx.cpuSimulationWorkers[d], 0, ctx.cpuSimulationWorkers[d].size());
        }

        prepare_next_emitters(ctx);

        for (int d = 0; d < ctx.cfg.max_system_depth; ++d)
        {
          ctx.cpuEmissionWorkers[d].clear();
          ctx.cpuSimulationWorkers[d].clear();
        }

        ctx.culling.cpuCullIdx = 0;
        ctx.culling.gpuCullIdx = 0;
        ctx.culling.cpuWorkers.clear();
        ctx.culling.gpuWorkers.clear();
      }
    }
  }

  ctx.allRenderWorkers.clear();

  set_global_value(ctx, ctx.cfg.dt_global_id, &origDt, sizeof(float));

  ctx.commandQueue.instanceWarmup.clear();
}

static void start_next_cpu_cull_threads(ContextId cid)
{
  TIME_PROFILE(dafx_process_cpu_culling);
  GET_CTX();

  if (ctx.culling.cpuWorkers.empty())
  {
    interlocked_release_store(ctx.asyncCounter, -1);
    return;
  }

  int step = max<int>(ctx.culling.cpuWorkers.size() / ctx.cfg.max_async_threads, 1U);
  int threads = min<int>(ctx.culling.cpuWorkers.size(), ctx.cfg.max_async_threads);
  int tail = ctx.culling.cpuWorkers.size() % threads;

  interlocked_release_store(ctx.asyncCounter, threads);
  ctx.asyncCpuCullJobs.resize(threads);
  for (int i = 0; i < threads; ++i)
  {
    AsyncCpuCullJob &job = ctx.asyncCpuCullJobs[i];
    job.cid = cid;
    job.start = i * step;
    job.count = i < threads - 1 ? step : step + tail;
    job.doJob(); // Note: it's small enough to be executed synchronously
  }
}

static int find_next_non_empty_workers(const Context &ctx, int depth, bool &is_emission)
{
  for (; depth <= ctx.cfg.max_system_depth; ++depth)
  {
    if (depth != ctx.cfg.max_system_depth)
    {
      if (!(is_emission ? ctx.cpuEmissionWorkers[depth] : ctx.cpuSimulationWorkers[depth]).empty())
        break;
    }
    else if (is_emission)
    {
      depth = -1; // 0 on next iter
      is_emission = false;
    }
    else // both emission and simulation are done, start culling phase
      return -1;
  }
  return depth;
}

static void start_next_cpu_compute_threads(ContextId cid, int depth = 0, bool is_emission = true)
{
  TIME_PROFILE(dafx_start_next_cpu_compute_threads);
  GET_CTX();

  G_ASSERT(interlocked_acquire_load(ctx.asyncCounter) == 0);
  depth = find_next_non_empty_workers(ctx, depth, is_emission);
  if (depth < 0)
    return start_next_cpu_cull_threads(cid);

  eastl::vector<int> &workers = is_emission ? ctx.cpuEmissionWorkers[depth] : ctx.cpuSimulationWorkers[depth];
  G_ASSERT(!workers.empty());

  if (!ctx.cfg.use_async_thread)
  {
    interlocked_release_store(ctx.asyncCounter, 1);
    AsyncCpuComputeJob job;
    job.cid = cid;
    job.emission = is_emission;
    job.depth = depth;
    job.threadId = -1;
    job.doJob();
    return;
  }

  int threads = min<int>(workers.size(), ctx.cfg.max_async_threads);
  G_ASSERT(threads <= ctx.workersByThreads.size());

  G_STATIC_ASSERT(sizeof(EmissionState) == sizeof(SimulationState)); // they should be same, structure-wise

  // to balance thread load
  // this way all threads should have similar amount of work
  if (is_emission)
    eastl::sort(workers.begin(), workers.end(), [&](int l, int r) {
      return ctx.instances.groups.cget<INST_EMISSION_STATE>(l).count > ctx.instances.groups.cget<INST_EMISSION_STATE>(r).count;
    });
  else
    eastl::sort(workers.begin(), workers.end(), [&](int l, int r) {
      return ctx.instances.groups.cget<INST_SIMULATION_STATE>(l).count > ctx.instances.groups.cget<INST_SIMULATION_STATE>(r).count;
    });

  for (eastl::vector<int> &i : ctx.workersByThreads)
    i.clear();

  interlocked_release_store(ctx.asyncCounter, threads);

  // Ensure that previous jobs in batch are done before attempting to re-use it's instances for new batch
  for (AsyncCpuComputeJob &j : ctx.asyncCpuComputeJobs)
    threadpool::wait(&j);

  const AsyncCpuComputeJob *cdata = ctx.asyncCpuComputeJobs.data();
  G_UNUSED(cdata);
  ctx.asyncCpuComputeJobs.resize(threads);
  for (int t = 0; t < threads; ++t)
  {
    AsyncCpuComputeJob &job = ctx.asyncCpuComputeJobs[t];
    job.cid = cid;
    job.emission = is_emission;
    job.depth = depth;
    job.threadId = t;

    eastl::vector<int> &dst = ctx.workersByThreads[t];
    dst.reserve(workers.size() / threads + 1);                 // rough estimation
    for (int i = t, ie = workers.size(); i < ie; i += threads) // threads balancing
      dst.emplace_back(workers[i]);
  }

#if DAGOR_DBGLEVEL > 0
  G_ASSERT(ctx.asyncCpuComputeJobs.data() == cdata); // No realloc occured

  int check = 0;
  for (eastl::vector<int> &i : ctx.workersByThreads)
    check += i.size();
  G_ASSERT(check == workers.size());
#endif

  for (auto &j : ctx.asyncCpuComputeJobs)
    start_job(ctx.cfg, j, /*wake*/ false);
  threadpool::wake_up_all();
}

void AsyncPrepareJob::doJob()
{
  GET_CTX();

  warmup_instance_from_queue(ctx); // TODO: move to multiple threads
  // always update emitters before everything else
  TIME_PROFILE(dafx_prepare);
  ctx.culling.cpuCullIdx = 0;
  ctx.culling.gpuCullIdx = 0;
  update_emitters(ctx, dt, 0, ctx.instances.groups.size());
  prepare_workers(ctx, dt, true, 0, ctx.instances.groups.size(), gpu_fetch);
  sort_shader_tasks(ctx);
  prepare_cpu_culling(ctx, true);
  start_next_cpu_compute_threads(cid);
  ctx.app_was_inactive = false;
}

void AsyncCpuComputeJob::doJob()
{
  GET_CTX();
  TIME_PROFILE(dafx_cpu_compute_tasks);

  eastl::vector<int> &workers =
    threadId >= 0 ? ctx.workersByThreads[threadId] : (emission ? ctx.cpuEmissionWorkers[depth] : ctx.cpuSimulationWorkers[depth]);

  if (emission)
    sort_cpu_shader_tasks<InstanceGroups, INST_CPU_BUFFER, INST_EMISSION_CPU_SHADER>(ctx.instances.groups, workers);
  else
    sort_cpu_shader_tasks<InstanceGroups, INST_CPU_BUFFER, INST_SIMULATION_CPU_SHADER>(ctx.instances.groups, workers);

  int parts = 0;
  if (emission)
    parts = update_cpu_emission(ctx, workers, 0, workers.size());
  else
    parts = update_cpu_simulation(ctx, workers, 0, workers.size());

  DA_PROFILE_TAG(dafx_cpu_compute_tasks, "em:%d, depth:%d, cnt:%d, parts:%d", (int)emission, depth, (int)workers.size(), parts);

  // all workers of current depth are done
  if (interlocked_decrement(ctx.asyncCounter) == 0)
  {
    if (int nd = find_next_non_empty_workers(ctx, depth + 1, emission); nd >= 0)
    {
      // Do it via separate job instance in order to be able safely re-use cpu compute job instances
      bool wake = threadpool::get_current_worker_id() < 0;
      start_job(ctx.cfg, *ctx.startNextComputeBatchJob.prepare(cid, nd, emission), wake);
    }
    else
      start_next_cpu_cull_threads(cid);
  }
}

inline cpujobs::IJob *AsyncStartNextComputeBatchJob::prepare(ContextId cid_, int depth_, bool emi)
{
  cid = cid_;
  depth = depth_;
  emission = emi;
  return this;
}

void AsyncStartNextComputeBatchJob::doJob() { start_next_cpu_compute_threads(cid, depth, emission); }

void AsyncCpuCullJob::doJob()
{
  GET_CTX();
  process_cpu_culling(ctx, start, count);
  if (interlocked_decrement(ctx.asyncCounter) == 0)
    interlocked_release_store(ctx.asyncCounter, -1);
}

void start_update(ContextId cid, float dt, bool update_gpu, bool tp_wake_up)
{
#if DAFX_FLUSH_BEFORE_UPDATE
  d3d::driver_command(Drv3dCommand::D3D_FLUSH);
#endif
  TIME_D3D_PROFILE(dafx_start_update);
  G_ASSERTF(dt >= 0, "invalid dt: %.2f", dt);
  dt = max(dt, 0.f);
  GET_CTX();

  if (ctx.updateInProgress)
    return;

  Stats::Queue queue = ctx.stats.queue[0];
  memset(&ctx.stats, 0, sizeof(Stats));
  memset(&ctx.asyncStats, 0, sizeof(AsyncStats));
  ctx.stats.queue[1] = queue; // populate before actual start_update

  G_ASSERT_RETURN(!ctx.asyncPrepareJob.cid, );
  ctx.updateInProgress = true;

  if (dt == 0.f || (ctx.debugFlags & DEBUG_DISABLE_SIMULATION))
  {
    ctx.simulationIsPaused = true;
    return;
  }

  ctx.simulationIsPaused = false;
  ctx.allRenderWorkers.clear();

  ctx.asyncPrepareJob.cid = cid;
  ctx.asyncPrepareJob.dt = dt;
  ctx.asyncPrepareJob.gpu_fetch = update_gpu;

  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    ctx.cpuSimulationWorkers[i].clear();
    ctx.cpuEmissionWorkers[i].clear();
  }

  interlocked_release_store(ctx.asyncCounter, 0);

  start_job(ctx.cfg, ctx.asyncPrepareJob, tp_wake_up);
}

void finish_update(ContextId cid, bool update_gpu_fetch, bool beforeRenderFrameFrameBoundary)
{
  finish_update_cpu_only(cid);
  finish_update_gpu_only(cid, update_gpu_fetch, beforeRenderFrameFrameBoundary);
}

void finish_update_cpu_only(ContextId cid)
{
  TIME_D3D_PROFILE(dafx_finish_update_cpu);
  // always flush jobs and command queue, even if render in not active,
  // otherwise we will get queue overflow
  flush_command_queue(cid);
}

void gather_system_usage_stats(Context &ctx);

void set_was_inactive(ContextId cid)
{
  GET_CTX();
  ctx.app_was_inactive = true;
}

void finish_update_gpu_only(ContextId cid, bool update_gpu_fetch, bool beforeRenderFrameFrameBoundary)
{
  TIME_D3D_PROFILE(dafx_finish_update_gpu);
  GET_CTX();

  // cpu async must be finished at this point. if not - someone forgot to pair start with finish
  int async = interlocked_acquire_load(ctx.asyncCounter);
  G_ASSERT(async == -1);
  G_UNUSED(async);

  // always flush jobs and command queue, even if render in not active,
  // otherwise we will get queue overflow
  flush_command_queue(cid);

  if (!ctx.updateInProgress)
    return;

  ctx.updateInProgress = false;

  {
    d3d::GpuAutoLock gpuLock; // main thread
    exec_delayed_page_destroy(ctx.gpuBufferPool);

    if (!beforeRenderFrameFrameBoundary)
      update_frame_boundary(ctx);

    update_gpu_transfer(ctx);
    update_global_data(ctx);

    bool issueFeedback = false;
    if (update_gpu_fetch)
      issueFeedback = prepare_gpu_culling(ctx, true);

    ShaderGlobal::set_int(ctx.updateGpuCullingVarId, issueFeedback);

    update_gpu_compute_tasks(ctx);
    prepare_next_emitters(ctx);

    if (issueFeedback)
      issue_culling_feedback(ctx);
    if (update_gpu_fetch)
    {
      fetch_culling(ctx);
      process_gpu_culling(ctx);
    }
  }

  prepare_render_buffers_state(ctx);

  populate_free_instances(ctx);
  update_buffers_stats(ctx.stats, ctx.gpuBufferPool, ctx.cpuBufferPool, ctx.renderBuffers, ctx.staging);

  stat_add(ctx.stats.cpuElemProcessed, ctx.asyncStats.cpuElemProcessed);
  stat_add(ctx.stats.gpuElemProcessed, ctx.asyncStats.gpuElemProcessed);
  stat_add(ctx.stats.cpuDispatchCalls, ctx.asyncStats.cpuDispatchCalls);
  stat_add(ctx.stats.gpuDispatchCalls, ctx.asyncStats.gpuDispatchCalls);

  for (int i = 0; i < Config::max_simulation_lods; ++i)
  {
    stat_add(ctx.stats.cpuElemProcessedByLods[i], ctx.asyncStats.cpuElemProcessedByLods[i]);
    stat_add(ctx.stats.gpuElemProcessedByLods[i], ctx.asyncStats.gpuElemProcessedByLods[i]);
  }

  gather_system_usage_stats(ctx);
  if (!ctx.simulationIsPaused)
    ctx.currentFrame++;
}

void populate_free_instances(Context &ctx)
{
  // we cant reuse instance (create inplace of deleted ones), because prepare_workers are
  // saving workers iid in start_update, so those need to be always available or deleted,
  // never reused one, or we call work from deleted instance to a freshly created one
  // (resulting in all sorts of bugs)
  ctx.instances.freeIndicesAvailable.insert(ctx.instances.freeIndicesAvailable.end(), ctx.instances.freeIndicesPending.begin(),
    ctx.instances.freeIndicesPending.end());
  ctx.instances.freeIndicesPending.clear();
}

void swap_command_queue(Context &ctx)
{
#define SW(a) eastl::swap(ctx.commandQueue.a, ctx.commandQueueNext.a)

  SW(createInstance);
  SW(destroyInstance);
  SW(resetInstance);

  SW(instancePos);
  SW(instanceStatus);
  SW(instanceVisibility);
  SW(instanceEmissionRate);
  SW(instanceValue);
  SW(instanceValueDirect);
  SW(instanceValueFromSystemScaled);

  SW(instanceValueData);
  SW(instanceWarmup);

#undef SW
}

void exec_command_queue(Context &ctx)
{
  ctx.stats.queue[0].createInstance += ctx.commandQueue.createInstance.size();
  ctx.stats.queue[0].destroyInstance += ctx.commandQueue.destroyInstance.size();
  ctx.stats.queue[0].resetInstance += ctx.commandQueue.resetInstance.size();
  ctx.stats.queue[0].setInstancePos += ctx.commandQueue.instancePos.size();
  ctx.stats.queue[0].setInstanceStatus += ctx.commandQueue.instanceStatus.size();
  ctx.stats.queue[0].setInstanceEmissionRate += ctx.commandQueue.instanceEmissionRate.size();
  ctx.stats.queue[0].setInstanceValue += ctx.commandQueue.instanceValue.size() + ctx.commandQueue.instanceValueDirect.size() +
                                         ctx.commandQueue.instanceValueFromSystemScaled.size();
  ctx.stats.queue[0].setInstanceValueTotalSize += ctx.commandQueue.instanceValueData.size();

  create_instances_from_queue(ctx);
  reset_instance_from_queue(ctx);
  set_instance_pos_from_queue(ctx);
  set_instance_emission_rate_from_queue(ctx);
  set_instance_value_from_queue(ctx);
  set_instance_status_from_queue(ctx);
  set_instance_visibility_from_queue(ctx);
  destroy_instance_from_queue(ctx);

  clear_command_queue(ctx);
}

void clear_command_queue(Context &ctx)
{
  ctx.commandQueue.createInstance.clear();
  ctx.commandQueue.destroyInstance.clear();
  ctx.commandQueue.resetInstance.clear();
  ctx.commandQueue.instanceValue.clear();
  ctx.commandQueue.instanceValueDirect.clear();
  ctx.commandQueue.instanceValueFromSystemScaled.clear();
  ctx.commandQueue.instanceStatus.clear();
  ctx.commandQueue.instanceVisibility.clear();
  ctx.commandQueue.instancePos.clear();
  ctx.commandQueue.instanceEmissionRate.clear();
  ctx.commandQueue.instanceValueData.clear();
  ctx.commandQueue.instanceWarmup.clear();
}

void clear_all_command_queues_locked(Context &ctx)
{
  os_spinlock_lock(&ctx.queueLock);
  clear_command_queue(ctx);
  swap_command_queue(ctx);
  clear_command_queue(ctx);
  os_spinlock_unlock(&ctx.queueLock);
}

bool update_finished(ContextId cid)
{
  GET_CTX_RET(false);
  return !ctx.updateInProgress;
}

void flush_command_queue(ContextId cid)
{
  TIME_PROFILE(dafx_flush_command_queue);
  GET_CTX();
  finish_async_update(ctx);
  os_spinlock_lock(&ctx.queueLock);
  swap_command_queue(ctx);
  exec_command_queue(ctx);
  // FIXME: spinlock should be before exec_command_queue, to reduce waiting time for act from thread.
  // However if flush_command_queue is called from multiple threads (which we should avoid),
  // we need it like this, to avoid 2 command queue simultaneous excecution (data-race)
  // Another reason for data race is creating_instance also requieres spinlock, due to the change
  // in the ctx.instances. We can avoid it, but for that we need to stop direct push to
  // ctx.instances and have another approach (i.e. indirection id)
  os_spinlock_unlock(&ctx.queueLock);
}

void finish_async_update(Context &ctx)
{
  if (!ctx.asyncPrepareJob.cid)
    return;

  G_ASSERT(ctx.updateInProgress);
  TIME_PROFILE(dafx_async_wait);

  threadpool::wait(&ctx.asyncPrepareJob);
  spin_wait([&ctx] { return interlocked_acquire_load(ctx.asyncCounter) >= 0; });

  for (int i = 0; i < ctx.asyncCpuComputeJobs.size(); ++i)
    threadpool::wait(&ctx.asyncCpuComputeJobs[i]);

  for (int i = 0; i < ctx.asyncCpuCullJobs.size(); ++i)
    threadpool::wait(&ctx.asyncCpuCullJobs[i]);

  ctx.asyncPrepareJob.cid = ContextId();
}

const Config &get_config(ContextId cid)
{
  const Context *pctx = g_ctx_list.get(cid);
  // Allow to call this method after shutdown without assert (could happen e.g. during level restart when it cleared up)
  if (pctx || cid.index() == 0)
    return pctx ? pctx->cfg : mainCtxConfig;
  static Config defCfg;
  GET_CTX_RET(defCfg);
  return ctx.cfg;
}

bool set_config(ContextId cid, const Config &cfg)
{
  GET_CTX_RET(false);

  finish_async_update(ctx);

  G_ASSERT_RETURN(cfg.min_emission_factor > 0.f, false);
  G_ASSERT(cfg.emission_factor <= 1.f);
  G_ASSERT(cfg.emission_limit <= cfg.emission_max_limit);

  ctx.cfg = cfg;
  ctx.cfg.emission_limit = min(cfg.emission_limit, (unsigned int)cfg.emission_max_limit);
  ctx.cfg.emission_factor = clamp(ctx.cfg.emission_factor, ctx.cfg.min_emission_factor, 1.f);

  if (ctx.cfg.max_async_threads == 0)
  {
    ctx.cfg.max_async_threads = threadpool::get_num_workers();
    if (!ctx.cfg.max_async_threads || !threadpool::get_queue_size(threadpool::PRIO_DEFAULT))
      ctx.cfg.use_async_thread = false;
    ctx.cfg.max_async_threads = max(ctx.cfg.max_async_threads, 1U);
  }

  ctx.cfg.vrs_enabled &= d3d::get_driver_desc().caps.hasVariableRateShading ? 1 : 0;
  ctx.cfg.multidraw_enabled &= d3d::get_driver_desc().caps.hasWellSupportedIndirect;
  ctx.cfg.multidraw_enabled &= ctx.cfg.use_render_sbuffer; // no need to even try on old hw

  ctx.workersByThreads.resize(ctx.cfg.max_async_threads);

  if (cid.index() == 0)
    mainCtxConfig = ctx.cfg;

  return true;
}

void set_debug_flags(ContextId cid, uint32_t flags)
{
  GET_CTX();
  ctx.debugFlags = flags;
}

uint32_t get_debug_flags(ContextId cid)
{
  GET_CTX_RET(0);
  return ctx.debugFlags;
}

void clear_stats(ContextId cid)
{
  GET_CTX();
  memset(&ctx.stats, 0, sizeof(Stats));
}

void get_stats(ContextId cid, Stats &out_s)
{
  GET_CTX();
  out_s = ctx.stats;
}

void format_lod_list(eastl::string &out_s, eastl::array<int, Config::max_simulation_lods> &lods,
  eastl::array<int, Config::max_simulation_lods> &padding)
{
  constexpr int frameHistory = 200; // to prevert stats flickering on rows count change

  for (int i = 0; i < lods.size(); ++i)
  {
    int c = lods[i];
    if (padding[i] > 0)
      out_s.append_sprintf("  lod_%d: %4d\n", i, c);
    padding[i] = c > 0 ? frameHistory : max(padding[i] - 1, 0);
  }
}

void get_stats_as_string(ContextId cid, eastl::string &out_s)
{
  out_s = eastl::string();
  GET_CTX();

  const Stats &s = ctx.stats;
#define ADDV(v) out_s.append_sprintf("%s: %4d\n", #v, s.v);

  ADDV(updateEmitters);

  ADDV(cpuEmissionWorkers);
  ADDV(gpuEmissionWorkers);
  ADDV(cpuSimulationWorkers);
  ADDV(gpuSimulationWorkers);

  ADDV(cpuDispatchCalls);
  ADDV(gpuDispatchCalls);

  ADDV(allRenderWorkers);
  ADDV(cpuRenderWorkers);

  ADDV(cpuElemProcessed);

  static eastl::array<int, Config::max_simulation_lods> cpuElemProcessedPadding = {};
  format_lod_list(out_s, ctx.stats.cpuElemProcessedByLods, cpuElemProcessedPadding);

  ADDV(gpuElemProcessed);
  static eastl::array<int, Config::max_simulation_lods> gpuElemProcessedPadding = {};
  format_lod_list(out_s, ctx.stats.gpuElemProcessedByLods, gpuElemProcessedPadding);

  ADDV(renderDrawCalls);
  ADDV(renderSwitchBuffers);
  ADDV(renderSwitchShaders);
  ADDV(renderSwitchTextures);
  ADDV(renderSwitchDispatches);
  ADDV(renderSwitchRenderState);

  ADDV(cpuCullElems);
  ADDV(gpuCullElems);
  ADDV(visibleTriangles);
  ADDV(renderedTriangles);

  ADDV(gpuTransferCount);
  ADDV(gpuTransferSize);

  ADDV(genVisibilityLod);

  ADDV(totalInstances);
  ADDV(activeInstances);

  out_s.append_sprintf("CPU sim elems total by lods:\n");
  static eastl::array<int, Config::max_simulation_lods> cpuElemTotalSimLodsPadding = {};
  format_lod_list(out_s, ctx.stats.cpuElemTotalSimLods, cpuElemTotalSimLodsPadding);

  out_s.append_sprintf("GPU sim elems total by lods:\n");
  static eastl::array<int, Config::max_simulation_lods> gpuElemTotalSimLodsPadding = {};
  format_lod_list(out_s, ctx.stats.gpuElemTotalSimLods, gpuElemTotalSimLodsPadding);

  out_s.append_sprintf("queue create: %3d / destroy: %3d / reset: %3d\n", s.queue[1].createInstance, s.queue[1].destroyInstance,
    s.queue[1].resetInstance);

  out_s.append_sprintf("queue pos: %3d / status: %3d / em_rate: %3d / val: %3d / val_total_size: %4d \n", s.queue[1].setInstancePos,
    s.queue[1].setInstanceStatus, s.queue[1].setInstanceEmissionRate, s.queue[1].setInstanceValue,
    s.queue[1].setInstanceValueTotalSize);

  out_s.append_sprintf("stagingBuffer: %3dKb\n", s.stagingSize / 1024);

  out_s.append_sprintf("total renderBuffers alloc: %4dKb / usage: %4dKb\n", s.totalRenderBuffersAllocSize / 1024,
    s.totalRenderBuffersUsageSize / 1024);

  out_s.append_sprintf("cpuBuffers pages: %4d / alloc: %4dKb / usage: %4dKb / garbage: %4dKb\n", s.cpuBufferPageCount,
    s.cpuBufferAllocSize / 1024, s.cpuBufferUsageSize / 1024, s.cpuBufferGarbageSize / 1024);

  out_s.append_sprintf("gpuBuffers pages: %4d / alloc: %4dKb / usage: %4dKb / garbage: %4dKb\n", s.gpuBufferPageCount,
    s.gpuBufferAllocSize / 1024, s.gpuBufferUsageSize / 1024, s.gpuBufferGarbageSize / 1024);

  out_s.append_sprintf("instances tuple: count: %4d / size: %4dKb\n", ctx.instances.groups.size(),
    ctx.instances.groups.size() * sizeof(InstanceStream::value_tuple) / 1024);

  for (int i = 0; i < ctx.stats.drawCallsByRenderTags.size(); ++i)
  {
    eastl::string tagName = get_render_tag_by_id(ctx.shaders, i);
    if (!tagName.empty() && ctx.stats.drawCallsByRenderTags[i] > 0)
      out_s.append_sprintf("tag: '%s' draw calls: %d\n", tagName.c_str(), ctx.stats.drawCallsByRenderTags[i]);
  }

#undef ADDV
}

#if DAGOR_DBGLEVEL > 0
void gather_system_usage_stats(Context &ctx)
{
  if (!(ctx.debugFlags & DEBUG_ENABLE_SYSTEM_USAGE_STATS))
    return;

  eastl::vector_map<int, eastl::pair<int, int>> map;
  InstanceGroups &stream = ctx.instances.groups;
  for (int i = 0; i < stream.size(); ++i)
  {
    int flags = stream.cget<INST_FLAGS>(i);
    if ((flags & SYS_ENABLED) && (flags & (SYS_CPU_SIMULATION_REQ | SYS_GPU_SIMULATION_REQ)))
    {
      const InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(i);
      auto it = map.insert(eastl::pair{stream.cget<INST_GAMERES_ID>(i), eastl::pair{1, activeState.aliveCount}});
      G_ASSERT(it.first != map.end());
      if (!it.second)
      {
        it.first->second.first++;
        it.first->second.second += activeState.aliveCount;
      }
    }
  }

  ctx.systemUsageStats.clear();
  for (auto it : map)
    ctx.systemUsageStats.push_back({it.first, it.second.first, it.second.second});
}
#else
void gather_system_usage_stats(Context &) {}
#endif

dag::ConstSpan<SystemUsageStat> get_system_usage_stats(ContextId cid)
{
  GET_CTX_RET(make_span<SystemUsageStat>(nullptr, 0));
  return make_span(ctx.systemUsageStats);
}
} // namespace dafx
