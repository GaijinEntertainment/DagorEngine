#include "context.h"
#include <3d/dag_drv3dCmd.h>
#include <osApiWrappers/dag_miscApi.h>

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

ContextId create_context(const Config &cfg)
{
  DBG_OPT("create_context - start");

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

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

  ctx->supportsNoOverwrite = d3d::get_driver_desc().caps.hasNoOverwriteOnShaderResourceBuffers;
  debug("dafx: using %d threads", ctx->cfg.use_async_thread ? ctx->cfg.max_async_threads : 0);

  ctx->asyncCpuComputeJobs.reserve(ctx->cfg.max_async_threads);
  ctx->asyncCpuCullJobs.reserve(ctx->cfg.max_async_threads);

  ShaderGlobal::set_int(::get_shader_variable_id("dafx_sbuffer_supported", true), cfg.use_render_sbuffer ? 1 : 0);

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);

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
    InstanceId iid = ctx.instances.list.createReferenceFromIdx(i);
    if (!iid)
      continue;

    int *psid = ctx.instances.list.get(iid);
    G_ASSERT_CONTINUE(psid);

    if (*psid == queue_instance_sid || *psid == dummy_instance_sid || ctx.instances.groups.get<INST_PARENT_SID>(*psid) == *psid)
      destroy_instance(cid, iid); // queue only root instances
  }

  flush_command_queue(cid);
  populate_free_instances(ctx);
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

  for (int i = 0; i < ctx.systems.list.totalSize(); ++i)
  {
    SystemId rid = ctx.systems.list.createReferenceFromIdx(i);
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

  if (!ctx.cullingStates.empty())
  {
    for (int i = 0; i < ctx.cullingStates.totalSize(); ++i)
    {
      CullingId cullId = ctx.cullingStates.createReferenceFromIdx(i);
      if (cullId)
        destroy_culling_state(cid, cullId);
    }

    G_ASSERT(ctx.cullingStates.empty());
  }

  release_all_systems(cid);
  os_spinlock_destroy(&ctx.queueLock);

  g_ctx_list.destroyReference(cid);
}

void set_custom_depth(ContextId cid, BaseTexture *tex)
{
  GET_CTX();
  ctx.customDepth = tex;
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

void prepare_workers(Context &ctx, bool main_pass, int begin_sid, int end_sid, bool gpu_fetch)
{
  //
  // TODO: fetch prev culling data at start so we can perform inital bbox filling in thread
  //
  TIME_PROFILE(dafx_prepare_workers);
  InstanceGroups &stream = ctx.instances.groups;
  for (int i = begin_sid; i < end_sid; ++i)
  {
    const uint32_t &flags = stream.get<INST_FLAGS>(i);
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

    if (simulationState.count > 0)
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
    set_global_value(ctx, ctx.cfg.dt_global_id, sizeof(ctx.cfg.dt_global_id) - 1, &ldt, sizeof(float));

    for (int sub : subinstances)
    {
      for (int i = 0; i < simSteps; ++i)
      {
        update_emitters(ctx, ldt, sub, sub + 1);
        prepare_workers(ctx, false, sub, sub + 1, false);
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

  set_global_value(ctx, ctx.cfg.dt_global_id, sizeof(ctx.cfg.dt_global_id) - 1, &origDt, sizeof(float));

  ctx.commandQueue.instanceWarmup.clear();
}

void start_next_cpu_cull_threads(ContextId cid)
{
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

    if (ctx.cfg.use_async_thread)
      threadpool::add(&job, threadpool::PRIO_DEFAULT, /*wake*/ false);
    else
      job.doJob();
  }
  if (ctx.cfg.use_async_thread)
    threadpool::wake_up_all();
}

void start_next_cpu_compute_threads(ContextId cid, int depth, bool is_emission)
{
  TIME_PROFILE(start_next_cpu_compute_threads);
  GET_CTX();

  G_ASSERT(interlocked_acquire_load(ctx.asyncCounter) == 0);

  if (depth == ctx.cfg.max_system_depth)
  {
    if (is_emission)
    {
      depth = 0;
      is_emission = false;
    }
    else // both emission and simulation are done, start culling phase
    {
      start_next_cpu_cull_threads(cid);
      return;
    }
  }

  eastl::vector<int> &workers = is_emission ? ctx.cpuEmissionWorkers[depth] : ctx.cpuSimulationWorkers[depth];
  if (workers.empty())
  {
    start_next_cpu_compute_threads(cid, depth + 1, is_emission);
    return;
  }

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
    dst.reserve(workers.size() / threads + 1); // rough estimation
    for (int i = t, ie = workers.size(); i < ie; i += threads)
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
    threadpool::add(&j, threadpool::PRIO_DEFAULT, /*wake*/ false);
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
  prepare_workers(ctx, true, 0, ctx.instances.groups.size(), gpu_fetch);
  sort_shader_tasks(ctx);
  prepare_cpu_culling(ctx, true);
  start_next_cpu_compute_threads(cid, 0, true);
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

  DA_PROFILE_TAG(dafx_cpu_compute_tasks, "em:%d, count:%d, parts:%d", emission ? 1 : 0, (int)workers.size(), parts);

  // all workers of current depth are done
  if (interlocked_decrement(ctx.asyncCounter) == 0)
  {
    // Do it via separate job instance in order to be able safely re-use cpu compute job instances
    if (ctx.cfg.use_async_thread)
    {
      bool wake = threadpool::get_current_worker_id() < 0;
      threadpool::add(ctx.startNextComputeBatchJob.prepare(cid, depth + 1, emission), threadpool::PRIO_DEFAULT, wake);
    }
    else
      start_next_cpu_compute_threads(cid, depth + 1, emission);
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

void start_update(ContextId cid, float dt, bool update_gpu)
{
#if DAFX_FLUSH_BEFORE_UPDATE
  d3d::driver_command(DRV3D_COMMAND_D3D_FLUSH, NULL, NULL, NULL);
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
    return;

  ctx.allRenderWorkers.clear();

  ctx.currentFrame++;

  ctx.asyncPrepareJob.cid = cid;
  ctx.asyncPrepareJob.dt = dt;
  ctx.asyncPrepareJob.gpu_fetch = update_gpu;

  for (int i = 0; i < ctx.cfg.max_system_depth; ++i)
  {
    ctx.cpuSimulationWorkers[i].clear();
    ctx.cpuEmissionWorkers[i].clear();
  }

  interlocked_release_store(ctx.asyncCounter, 0);

  if (ctx.cfg.use_async_thread)
    // Save on wake up if this routine itself is called from threadpool - assume that it will be executed in current worker
    threadpool::add(&ctx.asyncPrepareJob, threadpool::PRIO_DEFAULT, /*wake*/ threadpool::get_current_worker_id() < 0);
  else
    ctx.asyncPrepareJob.doJob();
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

  // main thread
  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

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

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);

  prepare_render_buffers_state(ctx);

  populate_free_instances(ctx);
  update_buffers_stats(ctx.stats, ctx.gpuBufferPool, ctx.cpuBufferPool, ctx.renderBuffers, ctx.staging);

  stat_add(ctx.stats.cpuElemProcessed, ctx.asyncStats.cpuElemProcessed);
  stat_add(ctx.stats.gpuElemProcessed, ctx.asyncStats.gpuElemProcessed);
  stat_add(ctx.stats.cpuDispatchCalls, ctx.asyncStats.cpuDispatchCalls);
  stat_add(ctx.stats.gpuDispatchCalls, ctx.asyncStats.gpuDispatchCalls);
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
  ctx.stats.queue[0].setInstanceValue += ctx.commandQueue.instanceValue.size() + ctx.commandQueue.instanceValueDirect.size();
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
  ADDV(gpuElemProcessed);

  ADDV(renderDrawCalls);
  ADDV(renderSwitchBuffers);
  ADDV(renderSwitchShaders);
  ADDV(renderSwitchTextures);
  ADDV(renderSwitchDispatches);

  ADDV(cpuCullElems);
  ADDV(gpuCullElems);
  ADDV(visibleTriangles);
  ADDV(renderedTriangles);

  ADDV(gpuTransferCount);
  ADDV(gpuTransferSize);

  ADDV(totalInstances);
  ADDV(activeInstances);

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

#undef ADDV
}
} // namespace dafx
