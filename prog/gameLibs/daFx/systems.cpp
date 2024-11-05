// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "systems.h"
#include "context.h"
#include "shaders.h"

namespace dafx
{
bool register_subsystem(Context &ctx, SystemTemplate &sys, const SystemDesc &desc, const eastl::string &name, int depth,
  SystemId sys_id)
{
  DBG_OPT("register_subsystem, name:%s ", name);
  G_ASSERT_RETURN(sys.name.empty(), false);
  G_ASSERT_RETURN(depth < Config::max_system_depth, false);
  G_ASSERT_RETURN(desc.serviceDataSize >= desc.serviceData.size(), false);

  if (desc.renderDescs.empty() && desc.subsystems.empty())
  {
    logerr("dafx: sys: %s, empty render allowed only for instances with subsystems", name.c_str());
    return false;
  }

  bool elemAligned = (desc.renderElemSize % DAFX_ELEM_STRIDE == 0) && (desc.simulationElemSize % DAFX_ELEM_STRIDE == 0);

  if (!elemAligned)
  {
    logerr("dafx: sys: %s, elemSize must be alligned by %d", name.c_str(), DAFX_ELEM_STRIDE);
    return false;
  }

  if (desc.renderElemSize == 0 && desc.simulationElemSize == 0 && desc.subsystems.empty())
  {
    logerr("dafx: sys: %s, both elemSizes are 0", name.c_str());
    return false;
  }

  if (desc.renderElemSize == 0 && !desc.renderDescs.empty())
  {
    logerr("dafx: sys: %d, non-empty render with empty render elems", name);
    return false;
  }

  if (!desc.qualityFlags)
  {
    logerr("dafx: sys: %d, quality is not set", name);
    return false;
  }

  bool isProxyNode =
    desc.emitterData.type == dafx::EmitterType::FIXED && desc.emitterData.fixedData.count == 0 && !desc.subsystems.empty();
  unsigned int emLimit = get_emitter_limit(desc.emitterData, false);
  G_ASSERT_RETURN(emLimit > 0 || isProxyNode, false);

  emLimit = isProxyNode ? 0 : max((int)(round(emLimit * max(ctx.cfg.emission_factor, desc.emitterData.minEmissionFactor))), 1);

  G_ASSERT_RETURN(emLimit < ctx.cfg.emission_limit && emLimit < ctx.cfg.emission_max_limit, false);

  sys.depth = depth;
  sys.refFlags = SYS_VALID | SYS_ENABLED | (isProxyNode ? 0 : SYS_EMITTER_REQ);
  if (desc.specialFlags & SystemDesc::FLAG_SKIP_SCREEN_PROJ_CULL_DISCARD)
    sys.refFlags |= SYS_SKIP_SCREEN_PROJ_CULL_DISCARD;
  if (!(desc.specialFlags & SystemDesc::FLAG_DISABLE_SIM_LODS))
    sys.refFlags |= SYS_ALLOW_SIMULATION_LODS;
  sys.qualityFlags = desc.qualityFlags;

  create_emitter_state(sys.emitterState, desc.emitterData, emLimit, ctx.cfg.emission_factor);
  create_emitter_randomizer(sys.emitterRandomizer, desc.emitterData, max(ctx.cfg.emission_factor, desc.emitterData.minEmissionFactor));

  sys.gpuEmissionShaderId = GpuComputeShaderId();
  sys.gpuSimulationShaderId = GpuComputeShaderId();

  sys.cpuEmissionShaderId = CpuComputeShaderId();
  sys.cpuSimulationShaderId = CpuComputeShaderId();

  sys.renderRefDataId = RefDataId();
  sys.simulationRefDataId = RefDataId();
  sys.valueBindId = ValueBindId();

  sys.renderElemSize = desc.renderElemSize;
  sys.simulationElemSize = desc.simulationElemSize;

  sys.renderDataSize = get_aligned_size(emLimit * desc.renderElemSize);
  sys.simulationDataSize = get_aligned_size(emLimit * desc.simulationElemSize);
  sys.serviceDataSize = get_aligned_size(desc.serviceDataSize);

  G_ASSERT_RETURN(desc.renderPrimPerPart > 0, false);
  sys.renderPrimPerPart = desc.renderPrimPerPart;

  sys.renderTags = 0;
  G_ASSERT(sys.renderShaders.empty());

  sys.gameResId = desc.gameResId;
  sys.resources[STAGE_CS] = desc.texturesCs;
  sys.resources[STAGE_VS] = desc.texturesVs;
  sys.resources[STAGE_PS] = desc.texturesPs;

  G_ASSERT(desc.texturesCs.size() <= desc.maxTextureSlotsAllocated);
  G_ASSERT(desc.texturesVs.size() <= desc.maxTextureSlotsAllocated);
  G_ASSERT(desc.texturesPs.size() <= desc.maxTextureSlotsAllocated);
  int maxTextureSlotsAllocated = interlocked_relaxed_load(ctx.maxTextureSlotsAllocated);
  interlocked_relaxed_store(ctx.maxTextureSlotsAllocated, max(maxTextureSlotsAllocated, desc.maxTextureSlotsAllocated));

  G_ASSERT(sys.subsystems.empty());

  if (desc.emissionData.type == EmissionType::SHADER)
  {
    sys.cpuEmissionShaderId = register_cpu_compute_shader_opt(ctx.shaders, desc.emissionData.shader);
    if (sys.cpuEmissionShaderId)
    {
      sys.refFlags |= SYS_CPU_EMISSION_REQ;
    }
    else
    {
      sys.refFlags |= SYS_GPU_EMISSION_REQ;
      sys.gpuEmissionShaderId = register_gpu_compute_shader(ctx.shaders, desc.emissionData.shader, name);
      if (!sys.gpuEmissionShaderId)
        return false;
    }
  }
  else if (desc.emissionData.type == EmissionType::REF_DATA)
  {
    if (desc.emissionData.renderRefData.size() != emLimit * desc.renderElemSize ||
        desc.emissionData.simulationRefData.size() != emLimit * desc.simulationElemSize)
    {
      logerr("dafx: sys: %s, emission refData size doesn't match with emission limit (%d-%d) (%d-%d)", name.c_str(),
        desc.emissionData.renderRefData.size(), emLimit * desc.renderElemSize, desc.emissionData.simulationRefData.size(),
        emLimit * desc.simulationElemSize);
      return false;
    }

    if (!desc.emissionData.renderRefData.empty())
    {
      eastl::vector<unsigned char> refData = desc.emissionData.renderRefData;
      while (refData.size() % DAFX_ELEM_STRIDE != 0)
        refData.push_back(0);
      sys.renderRefDataId = ctx.refDatas.emplaceOne(eastl::move(refData));
    }

    if (!desc.emissionData.simulationRefData.empty())
    {
      eastl::vector<unsigned char> refData = desc.emissionData.simulationRefData;
      while (refData.size() % DAFX_ELEM_STRIDE != 0)
        refData.push_back(0);
      sys.simulationRefDataId = ctx.refDatas.emplaceOne(eastl::move(refData));
    }
  }
  else if (desc.emissionData.type != EmissionType::NONE)
  {
    G_ASSERT_RETURN(false, false);
  }

  if (desc.simulationData.type == SimulationType::SHADER)
  {
    sys.cpuSimulationShaderId = register_cpu_compute_shader_opt(ctx.shaders, desc.simulationData.shader);
    if (sys.cpuSimulationShaderId)
    {
      sys.refFlags |= SYS_CPU_SIMULATION_REQ;
    }
    else
    {
      sys.refFlags |= SYS_GPU_SIMULATION_REQ;
      sys.gpuSimulationShaderId = register_gpu_compute_shader(ctx.shaders, desc.simulationData.shader, name);
      if (!sys.gpuSimulationShaderId)
        return false;
    }
  }
  else if (desc.simulationData.type != SimulationType::NONE)
  {
    G_ASSERT_RETURN(false, false);
  }

  if (sys.cpuSimulationShaderId && sys.gpuEmissionShaderId)
  {
    logerr("dafx: sys: %s, cpu sim with gpu emission is not supported", name.c_str());
    return false;
  }

  if (desc.serviceDataSize > 0 && !desc.serviceData.empty())
  {
    eastl::vector<unsigned char> refData = desc.serviceData;
    while (refData.size() % DAFX_ELEM_STRIDE != 0)
      refData.push_back(0);

    sys.serviceRefDataId = ctx.refDatas.emplaceOne(eastl::move(refData));
  }

  if (!desc.renderDescs.empty())
  {
    sys.refFlags |= SYS_RENDERABLE;

    if ((sys.renderRefDataId || sys.cpuEmissionShaderId || sys.cpuSimulationShaderId) && !sys.gpuSimulationShaderId)
      sys.refFlags |= SYS_CPU_RENDER_REQ;
    else if (sys.gpuEmissionShaderId || sys.gpuSimulationShaderId)
      sys.refFlags |= SYS_GPU_RENDER_REQ;
  }

  unsigned int localDataSize = sizeof(DataHead) + sys.renderDataSize + sys.simulationDataSize + sys.serviceDataSize;
  if (isProxyNode)
    localDataSize = 0;

  sys.localCpuDataSize = 0;
  if (sys.cpuEmissionShaderId || sys.cpuSimulationShaderId)
  {
    sys.localCpuDataSize = localDataSize;
  }
  else if (!isProxyNode)
  {
    sys.localCpuDataSize += sizeof(DataHead);
    sys.localCpuDataSize += sys.renderRefDataId ? sys.renderDataSize : 0;
    sys.localCpuDataSize += sys.simulationRefDataId ? sys.simulationDataSize : 0;
    sys.localCpuDataSize += sys.serviceRefDataId ? desc.serviceData.size() : 0;
  }

  sys.localGpuDataSize = 0;
  if (sys.gpuEmissionShaderId || sys.gpuSimulationShaderId)
  {
    sys.localGpuDataSize = localDataSize;
    sys.refFlags |= SYS_GPU_TRANFSERABLE; // we need to transfer at least a DataHead
  }

  eastl::vector<RenderShaderId> renderShaders;
  renderShaders.resize(ctx.cfg.max_render_tags);
  for (const RenderDesc &rdesc : desc.renderDescs)
  {
    uint32_t tag = register_render_tag(ctx.shaders, rdesc.tag, ctx.cfg.max_render_tags);
    G_ASSERT_RETURN(tag < ctx.cfg.max_render_tags, false);
    sys.renderTags |= 1 << tag;

    RenderShaderId shaderId = register_render_shader(ctx.shaders, rdesc.shader, name, ctx.instancingVdecl);
    if (!shaderId)
      return false;

    G_ASSERT_RETURN(!renderShaders[tag], false);

    renderShaders[tag] = shaderId;
  }
  sys.renderShaders = eastl::move(renderShaders); // shrink to fit
  sys.renderSortDepth = desc.renderSortDepth;

  if (!desc.localValuesDesc.empty())
  {
    sys.valueBindId = create_local_value_binds(ctx.binds, name, desc.localValuesDesc, sys.renderDataSize + sys.simulationDataSize);
    G_ASSERT_RETURN(sys.valueBindId, false);
  }

  {
    OSSpinlockScopedLock lock{GlobalData::bindSpinLock};
    if (!register_global_value_binds(ctx.binds, name, desc.reqGlobalValues))
      return false;
  }

  sys.totalGpuDataSize = 0;
  sys.totalCpuDataSize = 0;
  for (const SystemDesc &subDesc : desc.subsystems)
  {
    SystemTemplate &subSys = sys.subsystems.push_back();
    if (!register_subsystem(ctx, subSys, subDesc, name, sys.depth + 1, sys_id))
      return false;

    if (subSys.localGpuDataSize > 0)
    {
      // parent->child cpu-gpu tranfser req
      if (sys.localCpuDataSize > 0)
        sys.refFlags |= SYS_GPU_TRANFSERABLE;

      // children req gpu parent data
      if (subSys.gpuEmissionShaderId || subSys.gpuSimulationShaderId)
        sys.localGpuDataSize = localDataSize;
    }

    sys.totalGpuDataSize += subSys.totalGpuDataSize;
    sys.totalCpuDataSize += subSys.totalCpuDataSize;
  }

  sys.totalCpuDataSize += sys.localCpuDataSize;
  sys.totalGpuDataSize += sys.localGpuDataSize;

  DBG_OPT("renderElemSize: %d", sys.renderElemSize);
  DBG_OPT("simulationElemSize: %d", sys.simulationElemSize);

  DBG_OPT("renderDataSize: %d", sys.renderDataSize);
  DBG_OPT("simulationDataSize: %d", sys.simulationDataSize);

  DBG_OPT("localGpuDataSize: %d", sys.localGpuDataSize);
  DBG_OPT("localCpuDataSize: %d", sys.localCpuDataSize);

  DBG_OPT("totalGpuDataSize: %d", sys.totalGpuDataSize);
  DBG_OPT("totalCpuDataSize: %d", sys.totalCpuDataSize);

  return true;
}

SystemId get_system_by_name(ContextId cid, const eastl::string &name)
{
  GET_CTX_RET(SystemId());
  SystemNameMap::iterator sysNameIt = ctx.systems.nameMap.find(name);
  return sysNameIt != ctx.systems.nameMap.end() ? sysNameIt->second : SystemId();
}


SystemId register_system(ContextId cid, const SystemDesc &desc, const eastl::string &name)
{
  GET_CTX_RET(SystemId());
  SYS_LOCK_GUARD;
  if (name.empty())
  {
    logerr("dafx: unnamed system");
    return SystemId();
  }

  SystemNameMap::iterator sysNameIt = ctx.systems.nameMap.find(name);
  if (sysNameIt != ctx.systems.nameMap.end())
  {
    logerr("dafx: sys: %s, system already registered", name.c_str());
    return SystemId();
  }

  G_ASSERT(interlocked_acquire_load(ctx.systemsLockCounter) == 1);
  SystemId rid = ctx.systems.list.emplaceOne();
  SystemTemplate *sys = ctx.systems.list.get(rid);
  G_ASSERT_RETURN(sys, SystemId());

  if (!register_subsystem(ctx, *sys, desc, name, 0, rid))
  {
    ctx.systems.list.destroyReference(rid);
    return SystemId();
  }

  sys->name = name;
  ctx.systems.nameMap.insert(name).first->second = rid;

  DBG_OPT("register_system: %s, rid:%d, gpuSize:%d, cpuSize: %d", name.c_str(), (uint32_t)rid, sys->totalGpuDataSize,
    sys->totalCpuDataSize);
  return rid;
}

void release_subsystem(Context &ctx, SystemTemplate &sys)
{
  G_ASSERT(sys.refFlags & SYS_VALID);

  release_local_value_binds(ctx.binds, sys.valueBindId);

  if (sys.renderRefDataId)
    ctx.refDatas.destroyReference(sys.renderRefDataId);

  if (sys.simulationRefDataId)
    ctx.refDatas.destroyReference(sys.simulationRefDataId);

  sys.refFlags = 0;
  sys.name.clear();

  sys.renderShaders.clear();
  for (int i = 0; i < sys.resources.size(); ++i)
    sys.resources[i].clear();

  for (SystemTemplate &subSys : sys.subsystems)
    release_subsystem(ctx, subSys);

  sys.subsystems.clear();
}

void release_system(ContextId cid, SystemId rid)
{
  GET_CTX();
  SYS_LOCK_GUARD;
  G_ASSERT(interlocked_acquire_load(ctx.systemsLockCounter) == 1);
  SystemTemplate *sys = ctx.systems.list.get(rid);
  G_ASSERT_RETURN(sys, );

  DBG_OPT("release_system: %s, rid:%d", sys->name, (uint32_t)rid);

  SystemNameMap::iterator sysNameIt = ctx.systems.nameMap.find(sys->name);
  G_ASSERT(sysNameIt != ctx.systems.nameMap.end());

  release_subsystem(ctx, *sys);
  ctx.systems.list.destroyReference(rid);
  ctx.systems.nameMap.erase(sysNameIt);
}

bool get_system_name(ContextId cid, SystemId rid, eastl::string &out_name)
{
  GET_CTX_RET(false);
  SYS_LOCK_GUARD;
  SystemTemplate *sys = ctx.systems.list.get(rid);
  G_ASSERT_RETURN(sys, false);

  out_name = sys->name;

  return true;
}

int get_system_value_offset(ContextId cid, SystemId rid, eastl::string &name, int ref_size)
{
  GET_CTX_RET(-1);
  SYS_LOCK_GUARD;
  SystemTemplate *sys = ctx.systems.list.get(rid);
  G_ASSERT_RETURN(sys, -1);

  ValueBind *bind = get_local_value_bind(ctx.binds.localValues, sys->valueBindId, name);
  if (!bind)
    return -1;

  G_ASSERT_RETURN(bind->size == ref_size, -1);
  return bind->offset;
}

bool prefetch_system_textures(SystemTemplate &sys)
{
  bool fetched = true;
  for (int stage = 0; stage < sys.resources.size(); ++stage)
  {
    const eastl::vector<TextureDesc> &list = sys.resources[stage];
    for (auto [tid, a] : list)
    {
      if (tid != BAD_TEXTUREID)
        fetched &= prefetch_managed_texture(tid);
    }
  }

  for (SystemTemplate &sub : sys.subsystems)
    fetched &= prefetch_system_textures(sub);

  return fetched;
}

bool prefetch_system_textures(ContextId cid, SystemId sys_id)
{
  GET_CTX_RET(false);
  SYS_LOCK_GUARD;
  SystemTemplate *sys = ctx.systems.list.get(sys_id);
  G_ASSERT_RETURN(sys, false);
  return prefetch_system_textures(*sys);
}
} // namespace dafx