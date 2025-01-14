// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "instances.h"
#include "context.h"
#include <gameRes/dag_gameResSystem.h>
#include <dag_noise/dag_uint_noise.h>

namespace dafx
{
void load_local_textures(const eastl::string &sys_name, const eastl::vector<TextureDesc> &tex_descs)
{
  G_UNUSED(sys_name);
  for (auto [tid, a] : tex_descs)
  {
    G_ASSERT(tid != BAD_TEXTUREID);
    BaseTexture *tex = acquire_managed_tex(tid);
    if (!tex)
      logerr("dafx: sys: %s, unknown texture: %d", sys_name, tid);
  }
}

void unload_local_textures(eastl::vector<TextureDesc> &tex_descs)
{
  for (auto [tid, a] : tex_descs)
  {
    release_gameres_or_texres((GameResource *)(uintptr_t) unsigned(tid));
    tid = BAD_TEXTUREID;
  }
}

uint32_t compute_instantces_to_create(const SystemTemplate &sys)
{
  uint32_t instToCreate = 1;
  for (const SystemTemplate &subSys : sys.subsystems)
    instToCreate += compute_instantces_to_create(subSys);
  return instToCreate;
}

inline bool check_quality(Context &ctx, const SystemTemplate &sys) { return (ctx.cfg.qualityMask & sys.qualityFlags) ? true : false; }

void adjust_buffer_size_by_quality(Context &ctx, const SystemTemplate &sys, int &cpu_size, int &gpu_size, bool force_dummy)
{
  // we need to reduce total cpu/gpu buffer if some of sub-fx are removed by quality settings
  force_dummy = force_dummy || !check_quality(ctx, sys);
  if (force_dummy)
  {
    cpu_size -= sys.localCpuDataSize;
    gpu_size -= sys.localGpuDataSize;
  }
  for (const SystemTemplate &subSys : sys.subsystems)
    adjust_buffer_size_by_quality(ctx, subSys, cpu_size, gpu_size, force_dummy);
};

InstanceId create_subinstance(Context &ctx, InstanceId queued_iid, const SystemTemplate &sys, int parent_sid, GpuBuffer &group_gpu_buf,
  CpuBuffer &group_cpu_buf, int gpu_parent_offset, int cpu_parent_offset, float life_time, int depth)
{
  if (!check_quality(ctx, sys) || (group_cpu_buf.size == 0 && group_gpu_buf.size == 0)) // subfx or whole chain is discarded by quality
  {
    if (queued_iid)
    {
      INST_LIST_LOCK_GUARD;
      int *psid = ctx.instances.list.get(queued_iid);
      G_ASSERT_RETURN(psid && *psid == queue_instance_sid, InstanceId());
      *psid = dummy_instance_sid;
    }

    return queued_iid;
  }

  SYS_LOCK_GUARD;
  INST_TUPLE_LOCK_GUARD;
  G_ASSERT(interlocked_acquire_load(ctx.instanceTupleLockCounter) == depth + 1);

  Instances &instances = ctx.instances;
  InstanceGroups &stream = instances.groups;
  int sid;
  if (!instances.freeIndicesAvailable.empty())
  {
    sid = instances.freeIndicesAvailable.back();
    instances.freeIndicesAvailable.pop_back();
    G_ASSERT_RETURN(!(stream.get<INST_FLAGS>(sid) & SYS_VALID), InstanceId());
  }
  else
  {
    sid = stream.size();
    stream.push_back();
  }

  parent_sid = parent_sid >= 0 ? parent_sid : sid;
  gpu_parent_offset = gpu_parent_offset >= 0 ? gpu_parent_offset : group_gpu_buf.offset;
  cpu_parent_offset = cpu_parent_offset >= 0 ? cpu_parent_offset : group_cpu_buf.offset;

  G_ASSERT(group_gpu_buf.pageId || group_cpu_buf.pageId);
  G_ASSERT(group_gpu_buf.size > 0 || group_cpu_buf.size > 0);

  GpuBuffer gpuBuffer;
  if (sys.localGpuDataSize > 0)
  {
    gpuBuffer = group_gpu_buf;
    gpuBuffer.size = sys.localGpuDataSize;
  }

  CpuBuffer cpuBuffer;
  if (sys.localCpuDataSize > 0)
  {
    cpuBuffer = group_cpu_buf;
    cpuBuffer.size = sys.localCpuDataSize;
  }

  eastl::vector<TextureDesc> texturesVs = sys.resources[STAGE_VS];
  eastl::vector<TextureDesc> texturesPs = sys.resources[STAGE_PS];
  eastl::vector<TextureDesc> texturesCs = sys.resources[STAGE_CS];

  load_local_textures(sys.name, texturesCs);
  load_local_textures(sys.name, texturesVs);
  load_local_textures(sys.name, texturesPs);

  EmitterState emitterState = sys.emitterState;

  int rng = uint32_hash(interlocked_increment(ctx.rndSeed));
  apply_emitter_randomizer(sys.emitterRandomizer, emitterState, rng);

  if (life_time > 0)
  {
    emitterState.globalLifeLimit = emitterState.globalLifeLimit > 0 ? min(emitterState.globalLifeLimit, life_time) : life_time;
    emitterState.globalLifeLimitRef = emitterState.globalLifeLimit;
  }

  InstanceState instanceState = {0, 0, emitterState.emissionLimit};
  EmissionState emissionState = {0, 0};
  SimulationState simulationState = {0, 0};

  InstanceId rid;

  if (queued_iid)
  {
    rid = queued_iid;
    int *psid = instances.list.get(rid);
    G_ASSERT_RETURN(psid && *psid == queue_instance_sid, InstanceId());
    *psid = sid;
  }
  else
  {
    rid = instances.list.emplaceOne(sid);
  }

#define SETV(a, b) stream.get<a>(sid) = b;

  SETV(INST_RID, rid);
  SETV(INST_FLAGS, sys.refFlags);
  SETV(INST_REF_FLAGS, sys.refFlags);
  SETV(INST_SYNCED_FLAGS, sys.refFlags);
  SETV(INST_DEPTH, sys.depth);
  SETV(INST_LOD, ctx.cfg.forced_sim_lod_offset);

  SETV(INST_PARENT_SID, parent_sid);
  G_ASSERT(stream.get<INST_SUBINSTANCES>(sid).empty());

  SETV(INST_VALUE_BIND_ID, sys.valueBindId);

  SETV(INST_GPU_BUFFER, gpuBuffer);
  SETV(INST_CPU_BUFFER, cpuBuffer);

  SETV(INST_GPU_PARENT_OFFSET, gpu_parent_offset);
  SETV(INST_CPU_PARENT_OFFSET, cpu_parent_offset);

  SETV(INST_RENDER_ELEM_STRIDE, sys.renderElemSize);
  SETV(INST_SIMULATION_ELEM_STRIDE, sys.simulationElemSize);

  SETV(INST_PARENT_RENDER_ELEM_STRIDE, parent_sid != sid ? stream.get<INST_RENDER_ELEM_STRIDE>(parent_sid) : 0)
  SETV(INST_PARENT_SIMULATION_ELEM_STRIDE, parent_sid != sid ? stream.get<INST_SIMULATION_ELEM_STRIDE>(parent_sid) : 0)

  SETV(INST_SERVICE_REF_DATA_ID, sys.serviceRefDataId);

  SETV(INST_RENDER_DATA_SIZE, sys.renderDataSize);
  SETV(INST_SIMULATION_DATA_SIZE, sys.simulationDataSize);
  SETV(INST_SERVICE_DATA_SIZE, sys.serviceDataSize);

  SETV(INST_PRIM_PER_PART, sys.renderPrimPerPart);

  SETV(INST_PARENT_RENDER_DATA_SIZE, parent_sid != sid ? stream.get<INST_RENDER_DATA_SIZE>(parent_sid) : 0);
  SETV(INST_PARENT_SIMULATION_DATA_SIZE, parent_sid != sid ? stream.get<INST_SIMULATION_DATA_SIZE>(parent_sid) : 0);

  SETV(INST_RENDER_DATA_TRANSFER, DataTransfer());
  SETV(INST_SIMULATION_DATA_TRANSFER, DataTransfer());
  SETV(INST_SERVICE_DATA_TRANSFER, DataTransfer());

  SETV(INST_TARGET_RENDER_BUF, (sys.refFlags & SYS_CPU_RENDER_REQ) ? BAD_D3DRESID : gpuBuffer.resId);
  SETV(INST_TARGET_RENDER_DATA_OFFSET, gpuBuffer.offset + sizeof(DataHead));
  SETV(INST_TARGET_RENDER_PARENT_OFFSET, gpu_parent_offset + sizeof(DataHead));

  SETV(INST_EMISSION_GPU_SHADER, sys.gpuEmissionShaderId);
  SETV(INST_EMISSION_CPU_SHADER, sys.cpuEmissionShaderId);

  SETV(INST_SIMULATION_GPU_SHADER, sys.gpuSimulationShaderId);
  SETV(INST_SIMULATION_CPU_SHADER, sys.cpuSimulationShaderId);

  SETV(INST_ACTIVE_STATE, instanceState);
  SETV(INST_EMITTER_STATE, emitterState);
  SETV(INST_EMISSION_STATE, emissionState);
  SETV(INST_SIMULATION_STATE, simulationState);

  SETV(INST_RENDER_TAGS, sys.renderTags);
  SETV(INST_RENDER_SHADERS, sys.renderShaders);

  SETV(INST_LOCAL_RES_CS, eastl::move(texturesCs));
  SETV(INST_LOCAL_RES_VS, eastl::move(texturesVs));
  SETV(INST_LOCAL_RES_PS, eastl::move(texturesPs));

  v_bbox3_init_empty(stream.get<INST_BBOX>(sid));
  SETV(INST_POSITION, Point4(0, 0, 0, 0));
  SETV(INST_VIEW_DIST, 0);
  SETV(INST_RENDER_SORT_DEPTH, sys.renderSortDepth);
  SETV(INST_CULLING_ID, 0xffffffff);
  SETV(INST_LAST_VALID_BBOX_FRAME, ctx.currentFrame);
  SETV(INST_VISIBILITY, 0xffffffff);
  if constexpr (INST_RENDERABLE_TRIS >= 0)
    *stream.getOpt<INST_RENDERABLE_TRIS, uint>(sid) = 0;

  RefData *renderRefData = ctx.refDatas.get(sys.renderRefDataId);
  RefData *simulationRefData = ctx.refDatas.get(sys.simulationRefDataId);
  RefData *serviceRefData = ctx.refDatas.get(sys.serviceRefDataId);

  unsigned int renderRefDataSize = renderRefData ? renderRefData->size() : 0;
  unsigned int simulationRefDataSize = simulationRefData ? simulationRefData->size() : 0;
  unsigned int serviceRefDataSize = serviceRefData ? serviceRefData->size() : 0;

  SETV(INST_RENDER_REF_DATA_SIZE, renderRefDataSize);
  SETV(INST_SIMULATION_REF_DATA_SIZE, simulationRefDataSize);
  SETV(INST_SERVICE_REF_DATA_SIZE, serviceRefDataSize);

#if DAGOR_DBGLEVEL > 0
  SETV(INST_GAMERES_ID, sys.gameResId);
#endif

  G_ASSERT((renderRefDataSize + simulationRefDataSize + serviceRefDataSize) <= cpuBuffer.size);

  if (cpuBuffer.size > 0)
  {
    CpuPage *page = ctx.cpuBufferPool.pages.get(cpuBuffer.pageId);
    G_ASSERT_RETURN(page, InstanceId());

    // head
    // keep both cpu and gpu offset at the same head,
    // so we can use same descriptor for both cpu sim
    // and render at the same time
    DataHead head;
    head.parentCpuRenOffset = sizeof(DataHead) + cpu_parent_offset;
    head.parentCpuSimOffset = head.parentCpuRenOffset + stream.get<INST_PARENT_RENDER_DATA_SIZE>(sid);
    head.dataCpuRenOffset = sizeof(DataHead) + cpuBuffer.offset;
    head.dataCpuSimOffset = head.dataCpuRenOffset + sys.renderDataSize;
    head.serviceCpuOffset = head.dataCpuSimOffset + sys.simulationDataSize;

    head.parentGpuRenOffset = sizeof(DataHead) + gpu_parent_offset;
    head.parentGpuSimOffset = head.parentGpuRenOffset + stream.get<INST_PARENT_RENDER_DATA_SIZE>(sid);
    head.dataGpuRenOffset = sizeof(DataHead) + gpuBuffer.offset;
    head.dataGpuSimOffset = head.dataGpuRenOffset + sys.renderDataSize;
    head.serviceGpuOffset = head.dataGpuSimOffset + sys.simulationDataSize;

    head.parentRenStride = stream.get<INST_PARENT_RENDER_ELEM_STRIDE>(sid);
    head.parentSimStride = stream.get<INST_PARENT_SIMULATION_ELEM_STRIDE>(sid);

    head.dataRenStride = sys.renderElemSize;
    head.dataSimStride = sys.simulationElemSize;

    head.lifeLimit = sys.emitterState.lifeLimit;
    head.lifeLimitRcp = safeinv(sys.emitterState.lifeLimit);

    head.elemLimit = sys.emitterState.emissionLimit;

    memcpy(page->res.get() + cpuBuffer.offset, &head, sizeof(DataHead));
    if (sys.refFlags & SYS_GPU_TRANFSERABLE)
    {
      G_ASSERT(sys.localGpuDataSize > 0);
      SETV(INST_HEAD_DATA_TRANSFER, DataTransfer(0, 0, sizeof(DataHead)));
      stream.get<INST_FLAGS>(sid) |= SYS_GPU_TRANFSER_REQ;
    }

    // for gpu sim, cpu page containes only actual ref data, without empty space
    bool directRefMapping = cpuBuffer.size >= gpuBuffer.size;

    if (renderRefDataSize > 0)
    {
      G_ASSERT(renderRefDataSize <= sys.renderDataSize);

      memcpy(page->res.get() + cpuBuffer.offset + sizeof(DataHead), // -V522
        renderRefData->data(),                                      // -V522
        renderRefDataSize);

      if (sys.refFlags & SYS_GPU_TRANFSERABLE)
      {
        SETV(INST_RENDER_DATA_TRANSFER, DataTransfer(sizeof(DataHead), sizeof(DataHead), renderRefDataSize));
        stream.get<INST_FLAGS>(sid) |= SYS_GPU_TRANFSER_REQ;
      }
    }

    if (simulationRefDataSize > 0)
    {
      G_ASSERT(simulationRefDataSize <= sys.simulationDataSize);

      int src = sizeof(DataHead) + (directRefMapping ? sys.renderDataSize : renderRefDataSize);
      int dst = sizeof(DataHead) + sys.renderDataSize;
      memcpy(page->res.get() + cpuBuffer.offset + src, // -V522
        simulationRefData->data(),                     // -V522
        simulationRefDataSize);

      if (sys.refFlags & SYS_GPU_TRANFSERABLE)
      {
        SETV(INST_SIMULATION_DATA_TRANSFER, DataTransfer(src, dst, simulationRefDataSize));
        stream.get<INST_FLAGS>(sid) |= SYS_GPU_TRANFSER_REQ;
      }
    }

    if (serviceRefDataSize > 0)
    {
      G_ASSERT(serviceRefDataSize <= sys.serviceDataSize);

      int src = sizeof(DataHead) +
                (directRefMapping ? sys.renderDataSize + sys.simulationDataSize : renderRefDataSize + simulationRefDataSize);
      int dst = sizeof(DataHead) + sys.renderDataSize + sys.simulationDataSize;
      memcpy(page->res.get() + cpuBuffer.offset + src, // -V522
        serviceRefData->data(),                        // -V522
        serviceRefDataSize);

      if (sys.refFlags & SYS_GPU_TRANFSERABLE)
      {
        SETV(INST_SERVICE_DATA_TRANSFER, DataTransfer(src, dst, serviceRefDataSize));
        stream.get<INST_FLAGS>(sid) |= SYS_GPU_TRANFSER_REQ;
      }
    }
  }
  else // proxy node, just a container
  {
    G_ASSERT_RETURN(!sys.subsystems.empty(), InstanceId());
  }

#undef SETV

  group_gpu_buf.offset += gpuBuffer.size;
  group_gpu_buf.size -= gpuBuffer.size;

  group_cpu_buf.offset += cpuBuffer.size;
  group_cpu_buf.size -= cpuBuffer.size;

  gpu_parent_offset = gpuBuffer.offset;
  cpu_parent_offset = cpuBuffer.offset;

  for (const SystemTemplate &subSys : sys.subsystems)
  {
    InstanceId subRid = create_subinstance(ctx, InstanceId(), subSys, sid, group_gpu_buf, //
      group_cpu_buf, gpu_parent_offset, cpu_parent_offset, emitterState.globalLifeLimit, depth + 1);

    int *subSid = instances.list.get(subRid);
    stream.get<INST_SUBINSTANCES>(sid).push_back(subSid ? *subSid : dummy_instance_sid);
  }

  return rid;
}

InstanceId create_instance(ContextId cid, SystemId sid)
{
  GET_CTX_RET(InstanceId());
  G_ASSERT_RETURN(sid && ctx.systems.list.get(sid), InstanceId());

  os_spinlock_lock(&ctx.queueLock);
  G_ASSERT(interlocked_acquire_load(ctx.instanceListLockCounter) == 0);
  InstanceId iid = ctx.instances.list.emplaceOne(queue_instance_sid);
  ctx.commandQueueNext.createInstance.push_back({sid, iid});
  DBG_OPT("create_instance q: %d", (uint32_t)iid);
  os_spinlock_unlock(&ctx.queueLock);

  return iid;
}

void create_instances_from_queue(Context &ctx)
{
  TIME_PROFILE(dafx_create_instances_from_queue);

  for (const CommandQueue::CreateInstance &cq : ctx.commandQueue.createInstance)
  {
    SystemTemplate *sys = ctx.systems.list.get(cq.sid);
    G_ASSERT_CONTINUE(sys);

    DBG_OPT("creating instance: %s", sys->name);

    GpuBuffer gpuBuf;
    CpuBuffer cpuBuf;

    int cpuDataSize = sys->totalCpuDataSize;
    int gpuDataSize = sys->totalGpuDataSize;

    adjust_buffer_size_by_quality(ctx, *sys, cpuDataSize, gpuDataSize, false);

    if (cpuDataSize > ctx.cfg.data_buffer_size || gpuDataSize > ctx.cfg.data_buffer_size)
    {
      logerr("total data size of fx: %s is exceeding buffer limit (%d | %d) > %d", sys->name.c_str(), cpuDataSize, gpuDataSize,
        ctx.cfg.data_buffer_size);
      cpuDataSize = 0;
      gpuDataSize = 0;
    }

    if (gpuDataSize > 0)
    {
      if (!create_gpu_buffer(ctx.gpuBufferPool, gpuDataSize, gpuBuf))
      {
        logerr("dafx: sys: %s, can't create gpu buffer");
        ctx.instances.list.destroyReference(cq.iid);
        continue;
      }
    }

    if (cpuDataSize > 0)
    {
      if (!create_cpu_buffer(ctx.cpuBufferPool, cpuDataSize, cpuBuf))
      {
        logerr("dafx: sys: %s, can't create cpu buffer");
        ctx.instances.list.destroyReference(cq.iid);
        continue;
      }
    }

    InstanceId iid = create_subinstance(ctx, cq.iid, *sys, -1, gpuBuf, cpuBuf, -1, -1, 0.0f, 0);
    G_ASSERT_CONTINUE(gpuBuf.size == 0);
    G_ASSERT_CONTINUE(cpuBuf.size == 0);
    G_ASSERT_CONTINUE(iid == cq.iid);

    if (!iid)
      ctx.instances.list.destroyReference(cq.iid);
  }
}

InstanceId create_instance(ContextId cid, const eastl::string &name)
{
  GET_CTX_RET(InstanceId());

  SystemNameMap::iterator it = ctx.systems.nameMap.find(name);
  if (it == ctx.systems.nameMap.end())
  {
    logerr("dafx: sys: %s is not registered", name.c_str());
    return InstanceId();
  }

  return create_instance(cid, it->second);
}

inline void destroy_subinstance(Context &ctx, int sid, int depth)
{
  if (sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;

  Instances &instances = ctx.instances;
  InstanceGroups &stream = instances.groups;

  G_ASSERT(sid >= 0 && sid < stream.size());

  InstanceId rid = stream.get<INST_RID>(sid);
  G_ASSERT_RETURN(rid, );

  stream.get<INST_RID>(sid) = InstanceId();

  uint32_t &fl = stream.get<INST_FLAGS>(sid);
  G_ASSERT(fl & SYS_VALID);
  G_UNREFERENCED(fl);

  GpuBuffer &gpuBuffer = stream.get<INST_GPU_BUFFER>(sid);
  if (gpuBuffer.size > 0)
    release_gpu_buffer(ctx.gpuBufferPool, gpuBuffer, ctx.cfg.delayed_release_gpu_buffers);

  CpuBuffer &cpuBuffer = stream.get<INST_CPU_BUFFER>(sid);
  if (cpuBuffer.size > 0)
    release_cpu_buffer(ctx.cpuBufferPool, cpuBuffer);

  unload_local_textures(stream.get<INST_LOCAL_RES_CS>(sid));
  unload_local_textures(stream.get<INST_LOCAL_RES_VS>(sid));
  unload_local_textures(stream.get<INST_LOCAL_RES_PS>(sid));

  instances.freeIndicesPending.push_back(sid);

  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    destroy_subinstance(ctx, subSid, depth + 1);

  stream.get<INST_FLAGS>(sid) = 0;
  stream.get<INST_REF_FLAGS>(sid) = 0;

  stream.get<INST_LOCAL_RES_CS>(sid).clear();
  stream.get<INST_LOCAL_RES_VS>(sid).clear();
  stream.get<INST_LOCAL_RES_PS>(sid).clear();
  stream.get<INST_SUBINSTANCES>(sid).clear();
  stream.get<INST_RENDER_SHADERS>(sid).clear();

  stream.get<INST_HEAD_DATA_TRANSFER>(sid) = DataTransfer();
  stream.get<INST_RENDER_DATA_TRANSFER>(sid) = DataTransfer();
  stream.get<INST_SIMULATION_DATA_TRANSFER>(sid) = DataTransfer();
  stream.get<INST_SERVICE_DATA_TRANSFER>(sid) = DataTransfer();

  stream.get<INST_EMISSION_CPU_SHADER>(sid) = CpuComputeShaderId();
  stream.get<INST_EMISSION_GPU_SHADER>(sid) = GpuComputeShaderId();
  stream.get<INST_SIMULATION_CPU_SHADER>(sid) = CpuComputeShaderId();
  stream.get<INST_SIMULATION_GPU_SHADER>(sid) = GpuComputeShaderId();

  InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(sid);
  EmissionState &emissionState = stream.get<INST_EMISSION_STATE>(sid);
  SimulationState &simulationState = stream.get<INST_SIMULATION_STATE>(sid);

  activeState.aliveStart = 0;
  activeState.aliveCount = 0;
  activeState.aliveLimit = 0;

  emissionState.start = 0;
  emissionState.count = 0;

  simulationState.start = 0;
  simulationState.count = 0;

  G_ASSERT(interlocked_acquire_load(ctx.instanceTupleLockCounter) == depth + 1);
  instances.list.destroyReference(rid);
}

void destroy_instance(ContextId cid, InstanceId &iid)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );
  DBG_OPT("destroy_instance q: %d", (uint32_t)iid);
  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.destroyInstance.push_back(iid);
  os_spinlock_unlock(&ctx.queueLock);
  iid = InstanceId();
}

void destroy_instance_from_queue(Context &ctx)
{
  for (InstanceId iid : ctx.commandQueue.destroyInstance)
  {
    G_ASSERT(interlocked_acquire_load(ctx.instanceListLockCounter) == 0);
    int *sid = ctx.instances.list.get(iid);
    G_ASSERT_CONTINUE(sid);
    DBG_OPT("destroing instance: %d", (uint32_t)iid);
    if (*sid == queue_instance_sid || *sid == dummy_instance_sid)
      ctx.instances.list.destroyReference(iid); // queued or dummy, not actually created yet
    else
      destroy_subinstance(ctx, *sid, 0);
  }
}

inline void reset_subinstance(Context &ctx, int sid)
{
  if (sid == queue_instance_sid)
    return;

  if (sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;

  InstanceGroups &stream = ctx.instances.groups;
  G_ASSERT(sid >= 0 && sid < stream.size());
  G_ASSERT_RETURN(stream.get<INST_FLAGS>(sid) & SYS_VALID, );

  InstanceState &instState = stream.get<INST_ACTIVE_STATE>(sid);
  EmissionState &emState = stream.get<INST_EMISSION_STATE>(sid);
  SimulationState &simState = stream.get<INST_SIMULATION_STATE>(sid);

  instState.aliveStart = 0;
  instState.aliveCount = 0;
  emState.start = 0;
  emState.count = 0;
  simState.start = 0;
  simState.count = 0;

  uint32_t &flags = stream.get<INST_FLAGS>(sid);
  stream.get<INST_POSITION>(sid) = Point4(0, 0, 0, 0);
  flags = stream.get<INST_REF_FLAGS>(sid);
  stream.get<INST_SYNCED_FLAGS>(sid) = flags;
  stream.get<INST_LAST_VALID_BBOX_FRAME>(sid) = ctx.currentFrame;
  stream.get<INST_VISIBILITY>(sid) = 0xffffffff;
  if constexpr (INST_RENDERABLE_TRIS >= 0)
    *stream.getOpt<INST_RENDERABLE_TRIS, uint>(sid) = 0;

  // first transfer is still pending (reset right after creating)
  if (stream.get<INST_HEAD_DATA_TRANSFER>(sid).size > 0)
  {
    flags |= SYS_GPU_TRANFSER_REQ;
  }
  else
  {
    DataTransfer &rtransf = stream.get<INST_RENDER_DATA_TRANSFER>(sid);
    DataTransfer &stransf = stream.get<INST_SIMULATION_DATA_TRANSFER>(sid);

    rtransf.size = 0;
    rtransf.srcOffset = 0xffffffff;
    rtransf.dstOffset = 0xffffffff;

    stransf.size = 0;
    stransf.srcOffset = 0xffffffff;
    stransf.dstOffset = 0xffffffff;

    RefData *serviceRefData = ctx.refDatas.get(stream.get<INST_SERVICE_REF_DATA_ID>(sid));
    if (serviceRefData)
    {
      CpuBuffer &cpuBuffer = stream.get<INST_CPU_BUFFER>(sid);
      CpuPage *page = ctx.cpuBufferPool.pages.get(cpuBuffer.pageId);
      G_FAST_ASSERT(page);

      bool directRefMapping = stream.get<INST_CPU_BUFFER>(sid).size >= stream.get<INST_GPU_BUFFER>(sid).size;
      int serviceRefDataSize = stream.get<INST_SERVICE_REF_DATA_SIZE>(sid);

      int renderDataSize = stream.get<INST_RENDER_DATA_SIZE>(sid);
      int simulationDataSize = stream.get<INST_SIMULATION_DATA_SIZE>(sid);

      int renderRefDataSize = stream.get<INST_RENDER_REF_DATA_SIZE>(sid);
      int simulationRefDataSize = stream.get<INST_SIMULATION_REF_DATA_SIZE>(sid);

      int src =
        sizeof(DataHead) + (directRefMapping ? renderDataSize + simulationDataSize : renderRefDataSize + simulationRefDataSize);
      memcpy(page->res.get() + cpuBuffer.offset + src, // -V522
        serviceRefData->data(),                        // -V522
        serviceRefDataSize);
    }
  }

  reset_emitter_state(stream.get<INST_EMITTER_STATE>(sid));

  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    reset_subinstance(ctx, subSid);
}

void reset_instance(ContextId cid, InstanceId iid)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );
  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.resetInstance.push_back(iid);
  os_spinlock_unlock(&ctx.queueLock);
}

void reset_instance_from_queue(Context &ctx)
{
  for (InstanceId iid : ctx.commandQueue.resetInstance)
  {
    INST_LIST_LOCK_GUARD;
    int *sid = ctx.instances.list.get(iid);
    G_ASSERT_CONTINUE(sid);
    reset_subinstance(ctx, *sid);
  }
}

void warmup_instance(ContextId cid, InstanceId iid, float time)
{
  GET_CTX();
  G_ASSERT_RETURN(time > 0, );
  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.instanceWarmup.push_back({iid, time});
  os_spinlock_unlock(&ctx.queueLock);
}
DAGOR_NOINLINE
bool replicate_compare_specialized_ool(uint8_t *__restrict dest, const void *__restrict source, int size) // supposedly never happen
{
  if (memcmp(dest, source, size) == 0)
    return false;
  memcpy(dest, source, size);
  return true;
}

__forceinline bool compare_and_copy_64_bytes(void *__restrict dst, const void *__restrict src)
{
  vec4i src0 = v_ldui((int *)src), src1 = v_ldui((int *)src + 4), src2 = v_ldui((int *)src + 8), src3 = v_ldui((int *)src + 12);
  vec4i cmp = v_andi(v_andi(v_cmp_eqi(v_ldui((int *)dst), src0), v_cmp_eqi(v_ldui((int *)dst + 4), src1)),
    v_andi(v_cmp_eqi(v_ldui((int *)dst + 8), src2), v_cmp_eqi(v_ldui((int *)dst + 12), src3)));
  if (v_signmask(v_cast_vec4f(cmp)) == 0xF) // all dwords are equal to source
    return false;
  v_stui((int *)dst, src0);
  v_stui((int *)dst + 4, src1);
  v_stui((int *)dst + 8, src2);
  v_stui((int *)dst + 12, src3);
  return true;
}
__forceinline bool replicate_compare_specialized(uint8_t *__restrict dst, const void *__restrict src, int size)
{
  const uint32_t *__restrict source = (const uint32_t *)src;
  uint32_t *__restrict dest = (uint32_t *)dst;
  // G_ASSERT(((uintptr_t(dst)&3) == 0) && ((uintptr_t(src)&3) == 0));
  if (DAGOR_LIKELY(size == 12))
  {
    if ((dest[0] == source[0]) & (dest[1] == source[1]) & (dest[2] == source[2]))
      return false;
    dest[0] = source[0];
    dest[1] = source[1];
    dest[2] = source[2];
    return true;
  }
  if (DAGOR_LIKELY(size == 4))
  {
    if (*dest == *source)
      return false;
    *dest = *source;
    return true;
  }
  if (DAGOR_LIKELY(size == 64))
  {
    return compare_and_copy_64_bytes(dest, source);
  }
  return replicate_compare_specialized_ool(dst, src, size);
}
inline void set_instance_value(Context &ctx, int sid, int offset, const void *data, int size)
{
  INST_TUPLE_LOCK_GUARD;
  InstanceGroups &stream = ctx.instances.groups;

  CpuBuffer &cpuBuffer = stream.get<INST_CPU_BUFFER>(sid);
  unsigned char *dest = cpuBuffer.directPtr;

#if DAGOR_DBGLEVEL > 0
  validate_cpu_buffer(cpuBuffer, ctx.cpuBufferPool);
  G_ASSERT(offset >= 0 && offset + size <= cpuBuffer.size);
#endif

  int destOffset = cpuBuffer.offset + offset;
  if (!replicate_compare_specialized(dest + destOffset, data, size))
    return;

  uint32_t &flags = stream.get<INST_FLAGS>(sid);
  if (!(flags & SYS_GPU_TRANFSERABLE))
    return;

  bool isRenderData = offset < (stream.get<INST_RENDER_DATA_SIZE>(sid) + sizeof(DataHead));
  DataTransfer &tranfser = isRenderData ? stream.get<INST_RENDER_DATA_TRANSFER>(sid) : stream.get<INST_SIMULATION_DATA_TRANSFER>(sid);

  unsigned int end = offset + size;
  unsigned int curEnd = tranfser.size > 0 ? tranfser.dstOffset + tranfser.size : end;

  unsigned int newOffset = min(tranfser.dstOffset, (unsigned int)offset);
  unsigned int newSize = max(curEnd, end) - newOffset;

  tranfser.srcOffset = newOffset - newOffset % DAFX_ELEM_STRIDE;
  tranfser.dstOffset = newOffset - newOffset % DAFX_ELEM_STRIDE;
  tranfser.size = get_aligned_size(newOffset - tranfser.dstOffset + newSize);

  flags |= SYS_GPU_TRANFSER_REQ;
}

static int get_subinstance_id(Context &ctx, dafx::InstanceId iid, int subIdx)
{
  InstanceGroups &stream = ctx.instances.groups;

  INST_TUPLE_LOCK_GUARD;
  int *psid = ctx.instances.list.get(iid);
  G_ASSERT_RETURN(psid, dummy_instance_sid);

  int sid = *psid;
  if (sid == dummy_instance_sid)
    return dummy_instance_sid;
  G_FAST_ASSERT(sid >= 0 && sid < stream.size());
  if (subIdx >= 0)
  {
    const eastl::vector<int> &subinstances = ctx.instances.groups.get<INST_SUBINSTANCES>(sid);
    G_FAST_ASSERT(subIdx < subinstances.size());
    sid = subinstances[subIdx];
    if (sid == dummy_instance_sid)
      return dummy_instance_sid;
    G_FAST_ASSERT(sid >= 0 && sid < stream.size());
  }

  const uint32_t &flags = stream.get<INST_FLAGS>(sid);
  G_ASSERT(flags & SYS_VALID);
  G_UNREFERENCED(flags);
  return sid;
}

void set_instance_value_from_queue(Context &ctx)
{
  TIME_PROFILE(dafx_set_instance_value_from_queue);
  InstanceGroups &stream = ctx.instances.groups;

  for (const CommandQueue::InstanceValue &cq : ctx.commandQueue.instanceValue)
  {
    int sid = get_subinstance_id(ctx, cq.iid, -1);
    if (sid == dummy_instance_sid)
      continue;

    ValueBind *bind = get_local_value_bind(ctx.binds.localValues, stream.get<INST_VALUE_BIND_ID>(sid), cq.name);
    G_ASSERT(bind || cq.isOpt);
    if (!bind)
      return;

    G_ASSERT(bind->size == cq.size);
    set_instance_value(ctx, sid, bind->offset + sizeof(DataHead), ctx.commandQueue.instanceValueData.data() + cq.srcOffset, cq.size);
  }

  for (const CommandQueue::InstanceValueDirect &cq : ctx.commandQueue.instanceValueDirect)
  {
    int sid = get_subinstance_id(ctx, cq.iid, cq.subIdx);
    if (sid == dummy_instance_sid)
      continue;

    set_instance_value(ctx, sid, cq.dstOffset + sizeof(DataHead), ctx.commandQueue.instanceValueData.data() + cq.srcOffset, cq.size);
  }

  for (const CommandQueue::InstanceValueFromSystemScaled &cq : ctx.commandQueue.instanceValueFromSystemScaled)
  {
    int sid = get_subinstance_id(ctx, cq.iid, cq.subIdx);
    if (sid == dummy_instance_sid)
      continue;

    dafx::SystemTemplate *psys = ctx.systems.list.get(cq.sysid);
    G_ASSERT_CONTINUE(psys);
    if (cq.subIdx >= 0)
    {
      G_FAST_ASSERT(cq.subIdx < psys->subsystems.size());
      psys = &psys->subsystems[cq.subIdx];
    }

    int renderDataSize = stream.get<INST_RENDER_DATA_SIZE>(sid);
    bool isRenderData = cq.dstOffset < renderDataSize;
    const RefData *systemRef = ctx.refDatas.get(isRenderData ? psys->renderRefDataId : psys->simulationRefDataId);
    const unsigned char *systemData = systemRef->data() + (isRenderData ? cq.dstOffset : cq.dstOffset - renderDataSize);

    if (cq.vecOffset != -1)
    {
      G_ASSERT_CONTINUE(cq.size % sizeof(float) == 0);

      unsigned char *tempData = ctx.commandQueue.instanceValueData.data() + cq.vecOffset;
      for (int ofs = 0; ofs < cq.size; ofs += sizeof(float))
      {
        float scale;
        memcpy(&scale, tempData + ofs, sizeof(float));
        float sysValue;
        memcpy(&sysValue, systemData + ofs, sizeof(float));

        float finalValue = scale * sysValue;
        memcpy(tempData + ofs, &finalValue, sizeof(float));
      }
      set_instance_value(ctx, sid, cq.dstOffset + sizeof(DataHead), tempData, cq.size);
    }
    else
    {
      set_instance_value(ctx, sid, cq.dstOffset + sizeof(DataHead), systemData, cq.size);
    }
  }
}

inline void set_instance_value(ContextId cid, InstanceId iid, const eastl::string &name, const void *data, int size, bool is_opt)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );

  CommandQueue::InstanceValue iv;
  iv.iid = iid;
  iv.isOpt = is_opt;
  iv.size = size;
  iv.name = name;

  os_spinlock_lock(&ctx.queueLock);
  iv.srcOffset = ctx.commandQueueNext.instanceValueData.size();

  ctx.commandQueueNext.instanceValueData.resize(iv.srcOffset + size);
  memcpy(ctx.commandQueueNext.instanceValueData.data() + iv.srcOffset, data, size);

  ctx.commandQueueNext.instanceValue.emplace_back(iv);
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_value(ContextId cid, InstanceId iid, const eastl::string &name, const void *data, int size)
{
  set_instance_value(cid, iid, name, data, size, false);
}

void set_instance_value_opt(ContextId cid, InstanceId iid, const eastl::string &name, const void *data, int size)
{
  set_instance_value(cid, iid, name, data, size, true);
}

eastl::string get_instance_info(ContextId cid, InstanceId iid)
{
  GET_CTX_RET("");
  INST_TUPLE_LOCK_GUARD;
  int *pid = ctx.instances.list.get(iid);
  G_ASSERT_RETURN(pid, "");
  int sid = *pid;
  InstanceGroups &stream = ctx.instances.groups;
  eastl::string result;
  result.append_sprintf("alive instances: %d, shaders: {", stream.get<INST_ACTIVE_STATE>(sid).aliveCount);
  const eastl::vector<RenderShaderId> &shadersByTags = stream.get<INST_RENDER_SHADERS>(sid); // -V758
  bool firstShader = true;
  for (int i = 0, sz = shadersByTags.size(); i < sz; i++)
  {
    RenderShaderId renderShaderId = shadersByTags[i];
    if (renderShaderId)
    {
      result.append_sprintf("%s %s : %s", firstShader ? "" : " | ", get_render_tag_by_id(ctx.shaders, i).c_str(),
        ctx.shaders.renderShaders.get(renderShaderId)->get()->shader->getShaderClassName());
      firstShader = false;
    }
  }
  result.append_sprintf(" }");
  return result;
}

int get_instance_elem_count(Context &ctx, int sid)
{
  if (sid == queue_instance_sid) // queued
    return 0;

  if (sid == dummy_instance_sid)
    return 0;

  INST_TUPLE_LOCK_GUARD;
  InstanceGroups &stream = ctx.instances.groups;
  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);

  int count = stream.get<INST_PARENT_SID>(sid) == sid ? 0 : stream.get<INST_ACTIVE_STATE>(sid).aliveCount;
  for (int subSid : subinstances)
    count += get_instance_elem_count(ctx, subSid);

  return count;
}

int get_instance_elem_count(ContextId cid, InstanceId iid)
{
  GET_CTX_RET(0);

  INST_TUPLE_LOCK_GUARD;
  int *sid = ctx.instances.list.get(iid);
  G_ASSERT_RETURN(sid, 0);
  return get_instance_elem_count(ctx, *sid);
}

#if DAGOR_DBGLEVEL > 0
void gather_instance_stats(Context &ctx, int sid, eastl::vector<eastl::string> &names, eastl::vector<int> &elems,
  eastl::vector<int> &lods)
{
  if (sid == queue_instance_sid || sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;
  InstanceGroups &stream = ctx.instances.groups;

  int gameResId = stream.cget<INST_GAMERES_ID>(sid);
  if (gameResId >= 0 && stream.cget<INST_SIMULATION_REF_DATA_SIZE>(sid) == 0)
  {
    String resName;
    get_game_resource_name(gameResId, resName);
    eastl::string tmp;
    if (stream.cget<INST_RENDER_ELEM_STRIDE>(sid) == 0)
      tmp.append_sprintf("[%s]", resName.data());
    else
      tmp = resName.data();
    names.push_back(tmp);
    elems.push_back(get_instance_elem_count(ctx, sid));
    lods.push_back(stream.get<INST_LOD>(sid));
  }

  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    gather_instance_stats(ctx, subSid, names, elems, lods);
}

void gather_instance_stats(ContextId cid, InstanceId iid, eastl::vector<eastl::string> &names, eastl::vector<int> &elems,
  eastl::vector<int> &lods)
{
  GET_CTX();

  INST_TUPLE_LOCK_GUARD;
  int *sid = ctx.instances.list.get(iid);
  G_ASSERT_RETURN(sid, );
  gather_instance_stats(ctx, *sid, names, elems, lods);
}
#else
void gather_instance_stats(ContextId, InstanceId, eastl::vector<eastl::string> &, eastl::vector<int> &, eastl::vector<int> &) {}
#endif

bool get_instance_value(ContextId cid, InstanceId iid, int offset, void *out_data, int size)
{
  GET_CTX_RET(false);
  G_ASSERT_RETURN(out_data && offset >= 0 && size > 0, false);

  INST_LIST_LOCK_GUARD;
  int *pid = ctx.instances.list.get(iid);
  G_ASSERT_RETURN(pid, false);

  INST_TUPLE_LOCK_GUARD;
  int sid = *pid;
  InstanceGroups &stream = ctx.instances.groups;
  G_ASSERT_RETURN(sid >= 0 && sid < stream.size(), false);
  G_ASSERT_RETURN(stream.get<INST_FLAGS>(sid) & SYS_VALID, false);

  CpuBuffer &cpuBuffer = stream.get<INST_CPU_BUFFER>(sid);
  G_ASSERT_RETURN(cpuBuffer.size >= offset + size, false);

  int destOffset = cpuBuffer.offset + sizeof(DataHead) + offset;
  memcpy(out_data, cpuBuffer.directPtr + destOffset, size);
  return true;
}

void set_instance_value(Context &ctx, InstanceId iid, int sub_idx, int offset, const void *data, int size)
{
  G_FAST_ASSERT(offset >= 0);
  G_ASSERT_RETURN(iid, );

  os_spinlock_lock(&ctx.queueLock);
  INST_TUPLE_LOCK_GUARD;
  CommandQueue::InstanceValueDirect iv;
  iv.iid = iid;
  iv.subIdx = sub_idx;
  iv.size = size;
  iv.dstOffset = offset;
  iv.srcOffset = ctx.commandQueueNext.instanceValueData.size();

  ctx.commandQueueNext.instanceValueData.resize(iv.srcOffset + size);
  memcpy(ctx.commandQueueNext.instanceValueData.data() + iv.srcOffset, data, size);
  ctx.commandQueueNext.instanceValueDirect.emplace_back(iv);
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_value(ContextId cid, InstanceId iid, int offset, const void *data, int size)
{
  GET_CTX();
  set_instance_value(ctx, iid, -1, offset, data, size);
}

void set_subinstance_value(ContextId cid, InstanceId iid, int sub_idx, int offset, const void *data, int size)
{
  GET_CTX();
  set_instance_value(ctx, iid, sub_idx, offset, data, size);
}

void set_instance_value_from_system(Context &ctx, InstanceId iid, int sub_idx, SystemId sid, int offset, int size,
  dag::Span<const float> scale_vec)
{
  G_FAST_ASSERT(offset >= 0);
  G_ASSERT_RETURN(iid, );

  CommandQueue::InstanceValueFromSystemScaled iv;
  iv.iid = iid;
  iv.subIdx = sub_idx;
  iv.sysid = sid;
  iv.size = size;
  iv.dstOffset = offset;
  iv.vecOffset = -1;

  os_spinlock_lock(&ctx.queueLock);
  if (scale_vec.size())
  {
    G_ASSERT(size == scale_vec.size() * sizeof(float));
    iv.vecOffset = ctx.commandQueueNext.instanceValueData.size();

    ctx.commandQueueNext.instanceValueData.resize(iv.vecOffset + size);
    memcpy(ctx.commandQueueNext.instanceValueData.data() + iv.vecOffset, scale_vec.data(), size);
  }
  ctx.commandQueueNext.instanceValueFromSystemScaled.emplace_back(iv);
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_value_from_system(ContextId cid, InstanceId iid, SystemId sid, int offset, int size,
  dag::Span<const float> scale_vec)
{
  GET_CTX();
  set_instance_value_from_system(ctx, iid, -1, sid, offset, size, scale_vec);
}

void set_subinstance_value_from_system(ContextId cid, InstanceId iid, int sub_idx, SystemId sid, int offset, int size,
  dag::Span<const float> scale_vec)
{
  GET_CTX();
  set_instance_value_from_system(ctx, iid, sub_idx, sid, offset, size, scale_vec);
}

void set_instance_status(Context &ctx, int sid, bool enabled)
{
  if (sid == dummy_instance_sid)
    return;
  INST_TUPLE_LOCK_GUARD;

  InstanceGroups &stream = ctx.instances.groups;

  uint32_t &flag = stream.get<INST_FLAGS>(sid);
  G_ASSERT(flag & SYS_VALID);
  if (bool(flag & SYS_ENABLED) == enabled)
    return;

  flag &= ~SYS_ENABLED;
  flag |= enabled ? SYS_ENABLED : 0;

  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    set_instance_status(ctx, subSid, enabled);
}

void set_instance_status(ContextId cid, InstanceId iid, bool enabled)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );
  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.instanceStatus.push_back({iid, enabled});
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_status_from_queue(Context &ctx)
{
  TIME_PROFILE(dafx_set_instance_status_from_queue);
  for (const CommandQueue::InstanceStatus &cq : ctx.commandQueue.instanceStatus)
  {
    int *sid = ctx.instances.list.get(cq.iid);
    G_ASSERT_CONTINUE(sid);
    set_instance_status(ctx, *sid, cq.enabled);
  }
}

bool get_instance_status(ContextId cid, InstanceId rid)
{
  GET_CTX_RET(false);
  INST_TUPLE_LOCK_GUARD;
  int *sid = ctx.instances.list.get(rid);
  G_ASSERT_RETURN(sid, false);

  if (*sid == queue_instance_sid)
    return true; // queued

  if (*sid == dummy_instance_sid)
    return false;

  uint32_t &flags = ctx.instances.groups.get<INST_SYNCED_FLAGS>(*sid);
  G_FAST_ASSERT(flags & SYS_VALID);

  return flags & SYS_ENABLED;
}

void set_instance_visibility(ContextId cid, InstanceId iid, uint32_t visibility)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );

  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.instanceVisibility.push_back({iid, visibility});
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_visibility(Context &ctx, int sid, uint32_t visibility)
{
  if (sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;

  InstanceGroups &stream = ctx.instances.groups;
  stream.get<INST_VISIBILITY>(sid) = visibility;

  eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid); // -V758
  for (int subSid : subinstances)
    set_instance_visibility(ctx, subSid, visibility);
}

void set_instance_visibility_from_queue(Context &ctx)
{
  TIME_PROFILE(dafx_set_insance_visibility_from_queue);
  for (const CommandQueue::InstanceVisibility &cq : ctx.commandQueue.instanceVisibility)
  {
    INST_LIST_LOCK_GUARD;
    int *sid = ctx.instances.list.get(cq.iid);
    G_ASSERT_CONTINUE(sid);
    set_instance_visibility(ctx, *sid, cq.visibility);
  }
}

void set_instance_pos(Context &ctx, int sid, const Point4 &pos)
{
  if (sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;
  ctx.instances.groups.get<INST_POSITION>(sid) = pos;
  ctx.instances.groups.get<INST_FLAGS>(sid) |= SYS_POS_VALID;

  eastl::vector<int> &subinstances = ctx.instances.groups.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    set_instance_pos(ctx, subSid, pos);
}

void set_instance_pos(ContextId cid, InstanceId iid, const Point4 &pos)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );

  if (check_nan(pos.x) || check_nan(pos.y) || check_nan(pos.z) || check_nan(pos.w))
  {
    logerr("dafx: NaN in instance pos");
    return;
  }

  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.instancePos.push_back({iid, pos});
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_pos_from_queue(Context &ctx)
{
  TIME_PROFILE(dafx_set_instance_pos_from_queue);
  for (const CommandQueue::InstancePos &cq : ctx.commandQueue.instancePos)
  {
    INST_LIST_LOCK_GUARD;
    int *sid = ctx.instances.list.get(cq.iid);
    G_ASSERT_CONTINUE(sid);
    set_instance_pos(ctx, *sid, cq.pos);
  }
}

inline void set_instance_emission_rate(Context &ctx, int sid, float v)
{
  if (sid == dummy_instance_sid)
    return;

  INST_TUPLE_LOCK_GUARD;
  set_emitter_emission_rate(ctx.instances.groups.get<INST_EMITTER_STATE>(sid), v);
  eastl::vector<int> &subinstances = ctx.instances.groups.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    set_instance_emission_rate(ctx, subSid, v);
}

void set_instance_emission_rate(ContextId cid, InstanceId iid, float v)
{
  GET_CTX();
  G_ASSERT_RETURN(iid, );
  G_ASSERTF_AND_DO(v >= 0.f, v = 0.f, "emission_rate should be >=0, emission_rate=%f", v);
  os_spinlock_lock(&ctx.queueLock);
  ctx.commandQueueNext.instanceEmissionRate.push_back({iid, v});
  os_spinlock_unlock(&ctx.queueLock);
}

void set_instance_emission_rate_from_queue(Context &ctx)
{
  for (const CommandQueue::InstanceEmissionRate &cq : ctx.commandQueue.instanceEmissionRate)
  {
    INST_LIST_LOCK_GUARD;
    int *sid = ctx.instances.list.get(cq.iid);
    G_ASSERT_CONTINUE(sid);
    set_instance_emission_rate(ctx, *sid, cq.rate);
  }
}

bool is_instance_renderable_active(Context &ctx, int sid)
{
  if (sid == queue_instance_sid) // queued
    return true;

  if (sid == dummy_instance_sid)
    return false;

  INST_TUPLE_LOCK_GUARD;

  InstanceGroups &stream = ctx.instances.groups;
  const uint32_t &flags = stream.get<INST_SYNCED_FLAGS>(sid);

  if (!(flags & SYS_ENABLED))
    return false;

  G_FAST_ASSERT(flags & SYS_VALID);
  if (flags & SYS_RENDERABLE)
  {
    const EmitterState &emitterState = stream.get<INST_EMITTER_STATE>(sid);
    if (emitterState.totalTickRate > 0 && emitterState.spawnTick < 0 || // delayed, empty for now, but will be populated soon
        emitterState.globalLifeLimit > 0)                               // explicit lifetime is not over yet
      return true;

    unsigned int lastFrame = stream.get<INST_LAST_VALID_BBOX_FRAME>(sid);
    if ((ctx.currentFrame - lastFrame) < Config::max_inactive_frames)
      return true;
  }

  const eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid);
  for (int subSid : subinstances)
    if (is_instance_renderable_active(ctx, subSid))
      return true;

  return false;
}

inline bool get_sid_safe(Context &ctx, InstanceId rid, int &sid)
{
  os_spinlock_lock(&ctx.queueLock);
  int *sidp = nullptr;
  {
    INST_LIST_LOCK_GUARD; // need to be out of scope before os_spinlock_unlock
    sidp = ctx.instances.list.get(rid);
    if (DAGOR_LIKELY(sidp))
      sid = *sidp;
  }
  os_spinlock_unlock(&ctx.queueLock);
  return sidp != nullptr;
}

bool is_instance_renderable_active(ContextId cid, InstanceId rid)
{
  GET_CTX_RET(false);
  int sid;
  G_ASSERT_RETURN(get_sid_safe(ctx, rid, sid), false);
  return is_instance_renderable_active(ctx, sid);
}

dag::ConstSpan<bool> query_instances_renderable_active(ContextId cid, dag::ConstSpan<InstanceId> iids)
{
  GET_CTX_RET(make_span_const<bool>(nullptr, 0));

  ctx.activeQueryContainer.resize(iids.size());
  dag::Vector<int, framemem_allocator> sids;
  sids.resize(iids.size());
  os_spinlock_lock(&ctx.queueLock);
  {
    INST_LIST_LOCK_GUARD;
    int *psid = nullptr;
    for (int i = 0; i < iids.size(); ++i)
    {
      psid = ctx.instances.list.get(iids[i]);
      sids[i] = psid ? *psid : dummy_instance_sid;
    }
  }
  os_spinlock_unlock(&ctx.queueLock);

  for (int i = 0; i < sids.size(); ++i)
    ctx.activeQueryContainer[i] = is_instance_renderable_active(ctx, sids[i]);

  return make_span_const(ctx.activeQueryContainer);
}

bool is_instance_renderable_visible(Context &ctx, int sid)
{
  if (sid == queue_instance_sid) // queued
    return true;

  if (sid == dummy_instance_sid)
    return false;

  INST_TUPLE_LOCK_GUARD;
  InstanceGroups &stream = ctx.instances.groups;
  const uint32_t &flags = stream.get<INST_SYNCED_FLAGS>(sid); // -V758
  G_FAST_ASSERT(flags & SYS_VALID);

  if (flags & SYS_VISIBLE)
    return true;

  const eastl::vector<int> &subinstances = stream.get<INST_SUBINSTANCES>(sid); // -V758
  for (int subSid : subinstances)
    if (is_instance_renderable_visible(ctx, subSid))
      return true;

  return false;
}

bool is_instance_renderable_visible(ContextId cid, InstanceId iid)
{
  GET_CTX_RET(false);
  int sid;
  G_ASSERT_RETURN(get_sid_safe(ctx, iid, sid), false);
  return is_instance_renderable_visible(ctx, sid);
}

bool get_subinstances(ContextId cid, InstanceId iid, eastl::vector<InstanceId> &out)
{
  GET_CTX_RET(false);

  INST_LIST_LOCK_GUARD;
  int *sid = ctx.instances.list.get(iid);
  if (!sid || *sid >= ctx.instances.groups.size()) // still queued
    return false;

  out.clear();

  const eastl::vector<int> &subinstances = ctx.instances.groups.get<INST_SUBINSTANCES>(*sid);
  for (int subSid : subinstances)
    out.push_back(ctx.instances.groups.get<INST_RID>(subSid));

  return true;
}

void reset_instances_after_reset(Context &ctx)
{
  INST_TUPLE_LOCK_GUARD;
  InstanceGroups &stream = ctx.instances.groups;
  for (int i = 0, ie = stream.size(); i < ie; ++i)
  {
    uint32_t &flags = stream.get<INST_FLAGS>(i);
    if (!(flags & SYS_VALID))
      continue;

    stream.get<INST_LAST_VALID_BBOX_FRAME>(i) = ctx.currentFrame;

    if (flags & SYS_GPU_TRANFSERABLE)
    {
      bool directRefMapping = stream.get<INST_CPU_BUFFER>(i).size >= stream.get<INST_GPU_BUFFER>(i).size;

      int renderDataSize = stream.get<INST_RENDER_DATA_SIZE>(i);
      int simulationDataSize = stream.get<INST_SIMULATION_DATA_SIZE>(i);

      int renderRefDataSize = stream.get<INST_RENDER_REF_DATA_SIZE>(i);
      int simulationRefDataSize = stream.get<INST_SIMULATION_REF_DATA_SIZE>(i);
      int serviceRefDataSize = stream.get<INST_SERVICE_REF_DATA_SIZE>(i);

      stream.get<INST_HEAD_DATA_TRANSFER>(i) = DataTransfer(0, 0, sizeof(DataHead));
      stream.get<INST_RENDER_DATA_TRANSFER>(i) = DataTransfer(sizeof(DataHead), sizeof(DataHead), renderRefDataSize);

      int src = sizeof(DataHead) + (directRefMapping ? renderDataSize : renderRefDataSize);
      int dst = sizeof(DataHead) + renderDataSize;

      stream.get<INST_SIMULATION_DATA_TRANSFER>(i) = DataTransfer(src, dst, simulationRefDataSize);

      src = sizeof(DataHead) + (directRefMapping ? renderDataSize + simulationDataSize : renderRefDataSize + simulationRefDataSize);
      dst = sizeof(DataHead) + renderDataSize + simulationDataSize;

      stream.get<INST_SERVICE_DATA_TRANSFER>(i) = DataTransfer(src, dst, serviceRefDataSize);

      flags |= SYS_GPU_TRANFSER_REQ;
    }

    // only gpu sim requires hard reset
    if ((flags & (SYS_GPU_EMISSION_REQ | SYS_GPU_SIMULATION_REQ)))
    {
      flags &= ~(SYS_RENDER_REQ | SYS_VISIBLE);

      EmissionState &emState = stream.get<INST_EMISSION_STATE>(i);
      emState.start = 0;
      emState.count = 0;

      SimulationState &simState = stream.get<INST_SIMULATION_STATE>(i);
      simState.start = 0;
      simState.count = 0;

      EmitterState &emitterState = stream.get<INST_EMITTER_STATE>(i);
      emitterState.spawnTick = emitterState.spawnTickRef;
      emitterState.shrinkTick = emitterState.shrinkTickRef;

      InstanceState &instState = stream.get<INST_ACTIVE_STATE>(i);
      instState.aliveStart = 0;
      instState.aliveCount = 0;
    }
  }
}
} // namespace dafx