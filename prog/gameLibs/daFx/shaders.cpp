#include "shaders.h"
#include "context.h"
#include <dag_noise/dag_uint_noise.h>
#include <generic/dag_relocatableFixedVector.h>
#include <memory/dag_framemem.h>

namespace dafx
{
RenderShaderId register_render_shader(Shaders &dst, const eastl::string &shader_name, const eastl::string &sys_name)
{
  G_UNREFERENCED(sys_name);

  RenderShaderMap::iterator it = dst.renderShadersMap.find(shader_name);
  if (it != dst.renderShadersMap.end())
    return it->second;

  DynamicShaderHelper *res = new DynamicShaderHelper();
  if (!res->init(shader_name.c_str(), nullptr, 0, nullptr, true))
  {
    logerr("dafx: sys: %s, render shader %s not found", sys_name.c_str(), shader_name.c_str());
    del_it(res);
    return RenderShaderId();
  }

  RenderShaderId rid = dst.renderShaders.emplaceOne(RenderShaderPtr(res));
  dst.renderShadersMap.insert(shader_name).first->second = rid;
  DBG_OPT("register_render_shader, name:%s, id:%d", shader_name.c_str(), (uint32_t)rid);
  return rid;
}

GpuComputeShaderId register_gpu_compute_shader(Shaders &dst, const eastl::string &shader_name, const eastl::string &sys_name)
{
  G_UNREFERENCED(sys_name);

  GpuComputeShaderMap::iterator it = dst.gpuComputeShadersMap.find(shader_name);
  if (it != dst.gpuComputeShadersMap.end())
    return it->second;

  ComputeShaderElement *res = new_compute_shader(shader_name.c_str(), true);
  if (!res)
  {
    logerr("dafx: sys: %s, compute shader %s not found", sys_name.c_str(), shader_name.c_str());
    return GpuComputeShaderId();
  }

  GpuComputeShaderId rid = dst.gpuComputeShaders.emplaceOne(GpuComputeShaderPtr(res));
  dst.gpuComputeShadersMap.insert(shader_name).first->second = rid;
  DBG_OPT("register_gpu_compute_shader, name:%s, id:%d", shader_name.c_str(), (uint32_t)rid);
  return rid;
}

CpuComputeShaderId register_cpu_compute_shader_opt(Shaders &dst, const eastl::string &shader_name)
{
  CpuComputeShaderMap::iterator it = dst.cpuComputeShadersMap.find(shader_name);
  return it != dst.cpuComputeShadersMap.end() ? it->second : CpuComputeShaderId();
}

uint32_t register_render_tag(Shaders &dst, const eastl::string &tag, uint32_t max_tags)
{
  G_ASSERT_RETURN(dst.renderTagMap.size() < max_tags, max_tags);

  RenderTagMap::iterator it = dst.renderTagMap.find(tag);
  if (it != dst.renderTagMap.end())
    return it->second;

  int sz = dst.renderTagMap.size();
  dst.renderTagMap.insert(tag).first->second = sz;
  dst.renderTagInverseMap.insert(sz).first->second = tag;

  DBG_OPT("register_render_tag, tag:%s, v:%d(%d)", tag, sz, 1 << sz);
  return sz;
}

void register_cpu_override_shader(ContextId cid, const eastl::string &shader_name, cpu_exec_cb_t cb)
{
  GET_CTX();
  G_ASSERT_RETURN(!shader_name.empty(), );
  G_ASSERT_RETURN(cb != nullptr, );

  Shaders &dst = ctx.shaders;
  CpuComputeShaderMap::iterator it = dst.cpuComputeShadersMap.find(shader_name);
  G_ASSERT_RETURN(it == dst.cpuComputeShadersMap.end(), );

  CpuComputeShaderId rid = dst.cpuComputeShaders.emplaceOne(cb);
  dst.cpuComputeShadersMap.insert(shader_name).first->second = rid;

  DBG_OPT("register_cpu_override_shader: %s", shader_name);
}

//
// cpu shaders
//

template <typename T_state, int T_shader>
int update_cpu_tasks(Context &ctx, const eastl::vector<int> &workers, int start, int count)
{
  if (workers.empty())
    return 0;

  G_ASSERT(workers.size() >= start + count);

  dag::RelocatableFixedVector<CpuPageId, 32, true, framemem_allocator> pages;
  dag::RelocatableFixedVector<CpuComputeShaderId, 32, true, framemem_allocator> shaders;
  dag::RelocatableFixedVector<DispatchDesc, 32, true, framemem_allocator> dispatches;

  pages.reserve(count);
  shaders.reserve(count);
  dispatches.reserve(count);

  int cpuElemProcessed = 0;
  InstanceGroups &stream = ctx.instances.groups;

  for (int ii = start, ie = start + count; ii < ie; ++ii)
  {
    int sid = workers[ii];
    G_FAST_ASSERT(sid < stream.size());
    const T_state &state = stream.get<T_state>(sid);
    if (state.count == 0) // was destroyed already
      continue;

    const InstanceState &inst = stream.get<INST_ACTIVE_STATE>(sid); // -V758
    CpuBuffer &buf = stream.get<INST_CPU_BUFFER>(sid);

    pages.push_back(buf.pageId);
    shaders.push_back(stream.get<T_shader>(sid));
    DispatchDesc &ddesc = dispatches.push_back();

    int rnd = interlocked_increment(ctx.rndSeed);
    ddesc.headOffset = buf.offset;
    ddesc.startAndCount = state.start | (state.count << 16);
    ddesc.aliveStartAndCount = inst.aliveStart | (inst.aliveCount << 16);
    ddesc.rndSeedAndCullingId = (uint32_hash(rnd + sid + ddesc.startAndCount) % 0xff) | (stream.get<INST_CULLING_ID>(sid) << 8);
    cpuElemProcessed += state.count;
  }

  interlocked_add(ctx.asyncStats.cpuElemProcessed, cpuElemProcessed);

  if (dispatches.empty())
    return 0;

  CpuExecData params;
  params.globalValues = ctx.globalData.cpuRes.get();
  G_FAST_ASSERT(params.globalValues);

  params.culling = ctx.culling.cpuRes.get();
  G_FAST_ASSERT(params.culling);

  int cpuDispatchCalls = 0;
  int lastDispatchId = 0;
  CpuPageId curPageId = pages[0];
  CpuComputeShaderId curShaderId = shaders[0];

  for (int i = 1, ie = dispatches.size() + i; i < ie; ++i)
  {
    bool lastBatch = (i == ie - 1);

    CpuPageId newPageId = lastBatch ? curPageId : pages[i];
    CpuComputeShaderId newShaderId = lastBatch ? curShaderId : shaders[i];

    if (newPageId == curPageId && curShaderId == newShaderId && !lastBatch)
      continue;

    CpuPage *pagePtr = ctx.cpuBufferPool.pages.get(curPageId);
    G_FAST_ASSERT(pagePtr && pagePtr->res.get());

    CpuComputeShaderPtr *shaderPtr = ctx.shaders.cpuComputeShaders.get(curShaderId);
    G_FAST_ASSERT(shaderPtr);

    params.buffer = pagePtr->res.get();
    params.dispatches = dispatches.data() + lastDispatchId;

    params.dispatchesCount = i - lastDispatchId;
    (*shaderPtr)(params);

    lastDispatchId = i;
    curPageId = newPageId;
    curShaderId = newShaderId;

    cpuDispatchCalls++;
  }

  interlocked_add(ctx.asyncStats.cpuDispatchCalls, cpuDispatchCalls);
  return cpuElemProcessed;
}

int update_cpu_emission(Context &ctx, const eastl::vector<int> &workers, int start, int count)
{
  return update_cpu_tasks<EmissionState, INST_EMISSION_CPU_SHADER>(ctx, workers, start, count);
}

int update_cpu_simulation(Context &ctx, const eastl::vector<int> &workers, int start, int count)
{
  return update_cpu_tasks<SimulationState, INST_SIMULATION_CPU_SHADER>(ctx, workers, start, count);
}

//
// gpu shaders
//

template <typename T_state, int T_shader>
void update_gpu_tasks(Context &ctx, const eastl::vector<int> &workers)
{
  if (workers.empty())
    return;

  FRAMEMEM_REGION;

  dag::RelocatableFixedVector<int, 8, true, framemem_allocator> groups;
  dag::RelocatableFixedVector<GpuPageId, 8, true, framemem_allocator> pages;
  dag::RelocatableFixedVector<GpuComputeShaderId, 8, true, framemem_allocator> shaders;
  dag::RelocatableFixedVector<DispatchDesc, 8, true, framemem_allocator> dispatches;

  groups.reserve(workers.size());
  pages.reserve(workers.size());
  shaders.reserve(workers.size());
  dispatches.reserve(workers.size());

  int gpuElemProcessed = 0;
  InstanceGroups &stream = ctx.instances.groups;

  // prep all dispatches
  for (int ii = 0, ie = workers.size(); ii < ie; ++ii)
  {
    int sid = workers[ii];

    const T_state &state = stream.get<T_state>(sid);
    if (state.count == 0) // was destroyed already
      continue;

    G_FAST_ASSERT(stream.get<INST_FLAGS>(sid) & SYS_VALID);

    const InstanceState &inst = stream.get<INST_ACTIVE_STATE>(sid); // -V758
    GpuBuffer &buf = stream.get<INST_GPU_BUFFER>(sid);

    pages.push_back(buf.pageId);
    groups.push_back(((state.count - 1) / DAFX_DEFAULT_WARP + 1) * DAFX_DEFAULT_WARP);
    shaders.push_back(stream.get<T_shader>(sid));
    DispatchDesc &ddesc = dispatches.push_back();

    int rnd = interlocked_increment(ctx.rndSeed);
    ddesc.headOffset = buf.offset;
    ddesc.startAndCount = state.start | (state.count << 16);
    ddesc.aliveStartAndCount = inst.aliveStart | (inst.aliveCount << 16);
    ddesc.rndSeedAndCullingId = (uint32_hash(rnd + sid + ddesc.startAndCount) % 0xff) | (stream.get<INST_CULLING_ID>(sid) << 8);
    gpuElemProcessed += state.count;
  }

  interlocked_add(ctx.asyncStats.gpuElemProcessed, gpuElemProcessed);

  if (dispatches.empty())
    return;

  // transfer dispatches to gpu, cbuffer max size = DAFX_BUCKET_GROUP_SIZE
  for (int i = 0, ie = (dispatches.size() - 1) / DAFX_BUCKET_GROUP_SIZE + 1; i < ie; ++i)
  {
    GpuResourcePtr *buf = i < ctx.computeDispatchBuffers.size() ? &ctx.computeDispatchBuffers[i] : nullptr;
    if (!buf)
    {
      eastl::string name;
      name.append_sprintf("dafx_simulation_dispatch_buffer_%d", i);
      buf = &ctx.computeDispatchBuffers.push_back();
      if (!create_gpu_cb_res(*buf, sizeof(DispatchDesc), DAFX_BUCKET_GROUP_SIZE, name.c_str()))
        return;
    }

    int ofs = i * DAFX_BUCKET_GROUP_SIZE;
    int sz = min(DAFX_BUCKET_GROUP_SIZE, (int)dispatches.size() - ofs);
    if (!update_gpu_cb_buffer(buf->getBuf(), dispatches.data() + ofs, sz * sizeof(DispatchDesc)))
      return;
  }

  ctx.globalData.gpuBuf.setVar();
  d3d::resource_barrier({ctx.culling.gpuFeedbacks[ctx.culling.gpuFeedbackIdx].gpuRes.getBuf(), RB_NONE});
  if (!d3d::set_rwbuffer(STAGE_CS, DAFX_CULLING_DATA_UAV_SLOT, ctx.culling.gpuFeedbacks[ctx.culling.gpuFeedbackIdx].gpuRes.getBuf()))
    return;

  // cache
  bool setStates = true;
  int gpuDispatchCalls = 0;
  int groupSize = 1;
  int lastDispatchId = 0;
  int curDispatchBuffer = -1;
  uint curBucketSize = 0;
  GpuPage *curPagePtr = nullptr;
  GpuPageId curPageId = GpuPageId();
  GpuComputeShaderId curShaderId = GpuComputeShaderId();
  ComputeShaderElement *curShaderPtr = nullptr;
  GpuPage *firstPagePtr = ctx.gpuBufferPool.pages.get(pages[0]);
  G_FAST_ASSERT(firstPagePtr && firstPagePtr->res.getBuf());
  d3d::resource_barrier({firstPagePtr->res.getBuf(), RB_NONE});

  // dispatch by buckets and groups
  for (int i = 0, ie = dispatches.size(); i < ie; ++i)
  {
    int newDispatchBuffer = i / DAFX_BUCKET_GROUP_SIZE;
    int newBucketSize = groups[i];
    GpuPageId newPageId = pages[i];
    GpuComputeShaderId newShaderId = shaders[i];

    bool pageChanged = newPageId != curPageId;
    bool shaderChanged = newShaderId != curShaderId;
    bool bucketChanged = newBucketSize > curBucketSize;
    bool dispatchBufferChanged = newDispatchBuffer != curDispatchBuffer;

    G_FAST_ASSERT(newBucketSize >= curBucketSize || pageChanged || shaderChanged);

    if (!pageChanged && !shaderChanged && !bucketChanged && !dispatchBufferChanged)
    {
      groupSize++;
      continue;
    }

    if (i != 0)
    {
      G_FAST_ASSERT(((groupSize * curBucketSize) % DAFX_DEFAULT_WARP) == 0);
      G_FAST_ASSERT(((groupSize * curBucketSize - 1) / curBucketSize + lastDispatchId) < groups.size());

      if (setStates)
      {
        setStates = false;
        curShaderPtr->setStates();
      }

      uint32_t params[] = {(uint32_t)curBucketSize, (uint32_t)lastDispatchId};
      if (!d3d::set_immediate_const(STAGE_CS, params, 2))
        return;
      curShaderPtr->dispatch((groupSize * curBucketSize) / DAFX_DEFAULT_WARP, 1, 1, GpuPipeline::GRAPHICS, false);
      d3d::resource_barrier({ctx.culling.gpuFeedbacks[ctx.culling.gpuFeedbackIdx].gpuRes.getBuf(), RB_NONE});
      d3d::resource_barrier({curPagePtr->res.getBuf(), RB_NONE});
      gpuDispatchCalls++;
    }

    groupSize = 1;
    lastDispatchId = i % DAFX_BUCKET_GROUP_SIZE;

    if (dispatchBufferChanged)
    {
      setStates = true;
      lastDispatchId = 0;
      curDispatchBuffer = newDispatchBuffer;
      d3d::resource_barrier({ctx.computeDispatchBuffers[curDispatchBuffer].getBuf(), RB_RO_SRV | RB_STAGE_COMPUTE});
      if (!ShaderGlobal::set_buffer(ctx.computeDispatchVarId, ctx.computeDispatchBuffers[curDispatchBuffer].getId()))
        return;
    }

    if (pageChanged)
    {
      // no need for set_states, rw buffer set directly to the regs
      curPageId = newPageId;
      curPagePtr = ctx.gpuBufferPool.pages.get(newPageId);
      G_FAST_ASSERT(curPagePtr && curPagePtr->res.getBuf());

      if (!d3d::set_rwbuffer(STAGE_CS, DAFX_SYSTEM_DATA_UAV_SLOT, curPagePtr->res.getBuf()))
        return;
      d3d::resource_barrier({curPagePtr->res.getBuf(), RB_NONE});
    }

    if (shaderChanged)
    {
      setStates = true;
      curShaderId = newShaderId;
      GpuComputeShaderPtr *ptr = ctx.shaders.gpuComputeShaders.get(newShaderId);
      G_FAST_ASSERT(ptr);
      curShaderPtr = ptr->get();
    }

    // allow bucket shrink after dispatch, so there will be less empty threads
    curBucketSize = newBucketSize;
  }

  // last batch
  G_FAST_ASSERT(((groupSize * curBucketSize) % DAFX_DEFAULT_WARP) == 0);
  G_FAST_ASSERT(((groupSize * curBucketSize - 1) / curBucketSize + lastDispatchId) < groups.size());

  uint32_t params[] = {(uint32_t)curBucketSize, (uint32_t)lastDispatchId};
  if (!d3d::set_immediate_const(STAGE_CS, params, 2))
    return;

  curShaderPtr->dispatch((groupSize * curBucketSize) / DAFX_DEFAULT_WARP, 1, 1, GpuPipeline::GRAPHICS, true);
  gpuDispatchCalls++;

  d3d::set_immediate_const(STAGE_CS, nullptr, 0);
  d3d::set_rwbuffer(STAGE_CS, DAFX_SYSTEM_DATA_UAV_SLOT, nullptr);
  d3d::set_rwbuffer(STAGE_CS, DAFX_CULLING_DATA_UAV_SLOT, nullptr);
  interlocked_add(ctx.asyncStats.gpuDispatchCalls, gpuDispatchCalls);
}

void update_gpu_emission(Context &ctx, const eastl::vector<int> &workers)
{
  TIME_D3D_PROFILE(dafx_update_gpu_emission);
  update_gpu_tasks<EmissionState, INST_EMISSION_GPU_SHADER>(ctx, workers);
}

void update_gpu_simulation(Context &ctx, const eastl::vector<int> &workers)
{
  TIME_D3D_PROFILE(dafx_update_gpu_simulation);
  update_gpu_tasks<SimulationState, INST_SIMULATION_GPU_SHADER>(ctx, workers);
}
} // namespace dafx