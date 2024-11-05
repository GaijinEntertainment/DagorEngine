// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "context.h"
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_variableRateShading.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <util/dag_convar.h>
#include <debug/dag_debug3d.h>
#include <3d/dag_quadIndexBuffer.h>
#include <math/random/dag_random.h>
#include <daFx/dafx_render_dispatch_desc.hlsli>
#include "frameBoundaryBufferManager.h"

namespace convar
{
CONSOLE_BOOL_VAL("dafx", enable_multidraw, true);
CONSOLE_BOOL_VAL("dafx", allow_small_multidraw, false);
} // namespace convar

namespace dafx
{
constexpr int g_prim_limit_16b = 65536 / 3; // we are using 16bit index buffer, if we ever need more, we need to swithch to 32bit one

int acquire_frame_boundary(ContextId cid, TEXTUREID texture_id, IPoint2 frame_dim)
{
  GET_CTX_RET(DAFX_INVALID_BOUNDARY_OFFSET);
  return ctx.frameBoundaryBufferManager.acquireFrameBoundary(texture_id, frame_dim);
}

void prepare_render_workers(Context &ctx, uint32_t tags_mask)
{
  TIME_D3D_PROFILE(dafx_prepare_cpu_render_workers);

  int lowestBitTag = __bsf(tags_mask);
  G_ASSERT(lowestBitTag < Config::max_render_tags);

  eastl::vector<int> &cpuRenderWorkers = ctx.cpuRenderWorkers[lowestBitTag];
  cpuRenderWorkers.clear();

  const uint32_t cpuReqFlags = SYS_ENABLED | SYS_CPU_RENDER_REQ | SYS_VISIBLE;
  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = ctx.allRenderWorkers.size(); ii < iie; ++ii)
  {
    int sid = ctx.allRenderWorkers[ii];
    const uint32_t &renderTags = stream.get<INST_RENDER_TAGS>(sid);
    if (!(renderTags & tags_mask))
      continue;

    uint32_t &flags = stream.get<INST_FLAGS>(sid); // -V758
    if ((flags & cpuReqFlags) == cpuReqFlags && !(flags & SYS_RENDER_BUF_TAKEN))
    {
      G_FAST_ASSERT(flags & SYS_VALID);
      G_FAST_ASSERT(stream.get<INST_ACTIVE_STATE>(sid).aliveCount > 0);
      cpuRenderWorkers.push_back(sid);
      flags |= SYS_RENDER_BUF_TAKEN;
    }
  }

  int totalCpuWorkers = 0;
  for (int ii = 0, iie = ctx.cpuRenderWorkers.size(); ii < iie; ++ii)
    totalCpuWorkers += ctx.cpuRenderWorkers[ii].size();
  stat_set(ctx.stats.cpuRenderWorkers, totalCpuWorkers);
}

void prepare_cpu_render_buffer(Context &ctx, uint32_t tags_mask)
{
  TIME_D3D_PROFILE(dafx_prepare_cpu_render_buffer);

  int lowestBitTag = __bsf(tags_mask);
  G_ASSERT(lowestBitTag < Config::max_render_tags);

  const eastl::vector<int> &cpuRenderWorkers = ctx.cpuRenderWorkers[lowestBitTag];
  if (cpuRenderWorkers.empty())
    return;

  int newSize = 0;
  InstanceGroups &stream = ctx.instances.groups;
  for (int ii = 0, iie = cpuRenderWorkers.size(); ii < iie; ++ii)
  {
    int sid = cpuRenderWorkers[ii];
    const uint32_t &flags = stream.get<INST_FLAGS>(sid);
    if (!(flags & SYS_VISIBLE))
      continue;

    const InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(sid);
    newSize += activeState.aliveCount * stream.get<INST_RENDER_ELEM_STRIDE>(sid);

    int parentSid = stream.get<INST_PARENT_SID>(sid);
    if (parentSid != sid)
      newSize += stream.get<INST_ACTIVE_STATE>(parentSid).aliveCount * stream.get<INST_RENDER_ELEM_STRIDE>(parentSid);
  }

  ctx.renderBuffers[lowestBitTag].usageSize = newSize;
}


void update_cpu_render_buffer(Context &ctx, uint32_t tags_mask)
{
  TIME_D3D_PROFILE(dafx_update_cpu_render_buffer);

  int lowestBitTag = __bsf(tags_mask);
  G_ASSERT(lowestBitTag < Config::max_render_tags);

  RenderBuffer &renderBuffer = ctx.renderBuffers[lowestBitTag];
  const eastl::vector<int> &cpuRenderWorkers = ctx.cpuRenderWorkers[lowestBitTag];
  if (renderBuffer.usageSize == 0)
    return;

  InstanceGroups &stream = ctx.instances.groups;
  unsigned char *data = start_updating_render_buffer(ctx, lowestBitTag);
  if (!data)
  {
    // reset device, force invalidate broken gpu pointer
    for (int ii = 0, iie = cpuRenderWorkers.size(); ii < iie; ++ii)
    {
      int sid = cpuRenderWorkers[ii];
      stream.get<INST_TARGET_RENDER_BUF>(sid) = BAD_D3DRESID;
      stream.get<INST_TARGET_RENDER_DATA_OFFSET>(sid) = 0;
      stream.get<INST_TARGET_RENDER_PARENT_OFFSET>(sid) = 0;
    }
    return;
  }

  int dstOffset = 0;
  for (int ii = 0, iie = cpuRenderWorkers.size(); ii < iie; ++ii)
  {
    int sid = cpuRenderWorkers[ii];
    const uint32_t &flags = stream.get<INST_FLAGS>(sid);
    if (!(flags & SYS_VISIBLE))
      continue;

    // TODO: add whole hierarchy chain, not only 1 parent
    int parentSid = stream.get<INST_PARENT_SID>(sid);
    if (parentSid != sid)
    {
      const InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(parentSid);
      const int &elemSize = stream.get<INST_RENDER_ELEM_STRIDE>(parentSid);
      const CpuBuffer &src = stream.get<INST_CPU_BUFFER>(parentSid);
      int sz = activeState.aliveCount * elemSize;
      // only copy render data
      memcpy(data, src.directPtr + src.offset + sizeof(DataHead) + activeState.aliveStart * elemSize, sz);
      stream.get<INST_TARGET_RENDER_PARENT_OFFSET>(sid) = dstOffset;
      data += sz;
      dstOffset += sz;
    }

    const InstanceState &activeState = stream.get<INST_ACTIVE_STATE>(sid);
    const CpuBuffer &src = stream.get<INST_CPU_BUFFER>(sid);
    const int &elemSize = stream.get<INST_RENDER_ELEM_STRIDE>(sid);

    bool wrapped = false;
    int sz;
    if (activeState.aliveStart + activeState.aliveCount <= activeState.aliveLimit)
    {
      sz = activeState.aliveCount;
    }
    else
    {
      wrapped = true;
      sz = activeState.aliveLimit - activeState.aliveStart;
    }

    memcpy(data, src.directPtr + src.offset + sizeof(DataHead) + activeState.aliveStart * elemSize, sz * elemSize);
    data += sz * elemSize;

    if (wrapped)
    {
      sz = activeState.aliveCount - sz;
      memcpy(data, src.directPtr + src.offset + sizeof(DataHead), sz * elemSize);
      data += sz * elemSize;
    }

    stream.get<INST_TARGET_RENDER_BUF>(sid) = renderBuffer.res.getId();
    stream.get<INST_TARGET_RENDER_DATA_OFFSET>(sid) = dstOffset;
    dstOffset += activeState.aliveCount * elemSize;
  }

  stop_updating_render_buffer(renderBuffer);
}

void before_render(ContextId cid, uint32_t tags_mask)
{
  TIME_D3D_PROFILE(dafx_before_render);
  GET_CTX();

  if (ctx.debugFlags & DEBUG_DISABLE_RENDER)
    return;

  // update_culling_state remaps workers. So we should take care of remaps to this tags_mask.
  tags_mask = inverse_remap_tags_mask(cid, tags_mask);
  tags_mask &= ~ctx.beforeRenderUpdatedTags;
  ctx.beforeRenderUpdatedTags |= tags_mask;

  if (!tags_mask)
    return;

  prepare_render_workers(ctx, tags_mask);
  prepare_cpu_render_buffer(ctx, tags_mask);

  d3d::GpuAutoLock gpuLock;
  update_cpu_render_buffer(ctx, tags_mask);
}

void before_render(ContextId cid, const eastl::vector<eastl::string> &tags_name)
{
  GET_CTX();

  uint32_t tags_mask = 0;
  for (const eastl::string &tagName : tags_name)
  {
    uint32_t tag = get_render_tag(ctx.shaders, tagName, Config::max_render_tags);
    G_ASSERT(tag < Config::max_render_tags);

    tags_mask |= 1 << tag;
  }

  before_render(cid, tags_mask);
}

enum
{
  STATE_DATA_SRC = 1 << 0,
  STATE_DISP_SRC = 1 << 1,
  STATE_TEXTURES = 1 << 2,
  STATE_SAMPLERS = 1 << 3,
  STATE_SHADER = 1 << 4,
  STATE_VRS = 1 << 5,
  STATE_PRIM_PER_ELEM = 1 << 6,
  STATE_FLUSH_SHADER = 1 << 7,
};

struct DrawState
{
  const eastl::vector<TextureDesc> *resources;
  ShaderElement *shader;
  D3DRESID dataSource;
  int dispatchBufferIdx;
  int primPerElem;
  int vrs;
  uint32_t changes;
};

struct DrawCall
{
  int stateId;
  int instanceCount;
};

struct MultiDrawCall
{
  int stateId;
  int bufId;
  int bufOffset;
  int drawCallCount;
  int totalPrimCount;
};

struct DrawQueue
{
  dag::Vector<DrawState, framemem_allocator> states;
  dag::Vector<DrawCall, framemem_allocator> drawCalls;
  dag::Vector<MultiDrawCall, framemem_allocator> multidrawCalls;
  dag::Vector<RenderDispatchDesc, framemem_allocator> dispatches; // 1 dispatch desc for each instance
};

bool prepare_render_queue(Context &ctx, CullingState *cull, uint32_t tag, DrawQueue &queue)
{
  TIME_PROFILE(prepare_render_queue);

  const eastl::vector<int> &workers = cull->workers[tag];
  if (workers.empty())
    return false;

  InstanceGroups &stream = ctx.instances.groups;
  const uint32_t validationFlags = SYS_ENABLED | SYS_VALID | SYS_VISIBLE | SYS_RENDERABLE | SYS_RENDER_REQ;
  const bool vrsEnabled = ctx.cfg.vrs_enabled && cull->shadingRates[tag] > 0;
  // if cull was remapped and merged with another tag, we need to get 'original' render shader from 'original' tag
  // and since we can have multiple remaps, we keep all them here as fallback render tags
  dag::Vector<int, framemem_allocator> renderShadersRemap;
  const auto &remapTags = vrsEnabled ? cull->vrsRemapTags : cull->remapTags;
  for (int i = 0; i < remapTags.size(); ++i)
  {
    if (remapTags[i] == tag && i != tag)
    {
      renderShadersRemap.reserve(remapTags.size());
      renderShadersRemap.push_back(i);
    }
  }

  int lastVrs = 1; // 1 = no custom VRS, default
  int lastPrimPerElem = -1;
  int lastDispatchBufferId = -1;
  ShaderElement *lastShader = nullptr;
  D3DRESID lastDataSource = BAD_D3DRESID;
  eastl::array<TEXTUREID, Config::max_res_slots> lastResources;
  eastl::array<uint8_t, Config::max_res_slots> lastSamplers; // invalid=0, aniso=1, linear=2
  lastResources.fill(BAD_TEXTUREID);
  lastSamplers.fill(0);

  queue.states.reserve(workers.size());     // TODO: very pessimistic, probably should be 2/3
  queue.dispatches.reserve(workers.size()); // exact
  queue.drawCalls.reserve(workers.size());  // exact

  for (int ii = 0, iie = workers.size(); ii < iie; ++ii)
  {
    int sid = workers[ii];

    const uint32_t flags = stream.get<INST_FLAGS>(sid);
    // it is possible that instance has died or reseted between update and render
    if ((flags & validationFlags) != validationFlags) // -V547
      continue;

    G_FAST_ASSERT(!(flags & SYS_CPU_RENDER_REQ) || (flags & SYS_RENDER_BUF_TAKEN));
    const InstanceState &state = stream.get<INST_ACTIVE_STATE>(sid); // -V779
    if (state.aliveCount >= 0xffff)                                  // we are packing it to 16b
    {
      logerr("fx aliveCount is over the limit (65536)");
      continue;
    }

    const eastl::vector<TextureDesc> *resList = &stream.get<INST_LOCAL_RES_PS>(sid);

    // tex + samplers
    // TODO: hash it to avoid loops?
    bool resChange = false;
    bool smpChange = false;
    bool texLoaded = true;
    for (int s = 0, se = resList->size(); s < se; ++s)
    {
      const TextureDesc &t = resList->at(s);
      G_FAST_ASSERT(t.texId != BAD_TEXTUREID);
      if (lastResources[s] != t.texId)
      {
        lastResources[s] = t.texId;
        resChange = true;
      }

      int sampler = t.anisotropic ? 1 : 2;
      if (lastSamplers[s] != sampler)
      {
        lastSamplers[s] = sampler;
        smpChange = true;
      }

      mark_managed_tex_lfu(t.texId);
      if (!check_managed_texture_loaded(t.texId))
        texLoaded = false;
    }
    if (!texLoaded) // no need to render fx with stub textures, pretty rare
    {
      lastResources.fill(BAD_TEXTUREID);
      lastSamplers.fill(0);
      continue;
    }

    // data source (render buffer in case of CPU sim render, instace buffer for GPU sim render)
    D3DRESID dataSource = stream.get<INST_TARGET_RENDER_BUF>(sid);
    if (!dataSource) // render buffer was not allocated, skipping (reset device, OOM), rare
      continue;

    bool dataSourceChange = lastDataSource != dataSource;
    lastDataSource = dataSource;

    // render shader
    const eastl::vector<RenderShaderId> &shadersByTags = stream.get<INST_RENDER_SHADERS>(sid);
    G_FAST_ASSERT(!shadersByTags.empty());
    RenderShaderId renderShaderId = shadersByTags[tag];
    if (!renderShaderId) // remap?
    {
      for (int ri = 0; ri < renderShadersRemap.size(); ++ri)
      {
        renderShaderId = shadersByTags[renderShadersRemap[ri]];
        if (renderShaderId)
          break;
      }
    }

    RenderShaderPtr *shader = ctx.shaders.renderShaders.get(renderShaderId);
    G_FAST_ASSERT(shader);
    bool shaderChange = lastShader != shader->get()->shader;
    lastShader = shader->get()->shader;

    // vrs
    // extract shading rate by associating render tag with culling predefines for those tags
    // TODO: hash it to avoid loops?
    int vrs = 1;
    if (vrsEnabled)
    {
      uint32_t tags = stream.get<INST_RENDER_TAGS>(sid);
      for (int t = __bsf_unsafe(tags), te = __bsr_unsafe(tags); t <= te; ++t)
      {
        int rate = cull->shadingRates[t];
        if (rate > 0 && (1 << t) & tags)
        {
          vrs = rate;
          break;
        }
      }
    }
    bool vrsChange = lastVrs != vrs;
    lastVrs = vrs;

    // prim per elem (2 for quads, 4 for ribbons). Required mostly for multidraw
    int primPerElem = stream.get<INST_PRIM_PER_PART>(sid);
    bool primPerElemChange = lastPrimPerElem != primPerElem;
    lastPrimPerElem = primPerElem;

    // render dispatch
    int dispatchBufferId = ctx.currentRenderDispatchBuffer + queue.dispatches.size() / DAFX_RENDER_GROUP_SIZE;
    bool dispatchBufferIdChange = lastDispatchBufferId != dispatchBufferId;
    lastDispatchBufferId = dispatchBufferId;

    int start = (flags & SYS_CPU_RENDER_REQ) ? 0 : state.aliveStart;
    int dataRenStride = stream.get<INST_RENDER_ELEM_STRIDE>(sid) / DAFX_ELEM_STRIDE;

    RenderDispatchDesc &desc = queue.dispatches.push_back_noinit();
    desc.dataRenOffset = stream.get<INST_TARGET_RENDER_DATA_OFFSET>(sid) / DAFX_ELEM_STRIDE;
    desc.parentRenOffset = stream.get<INST_TARGET_RENDER_PARENT_OFFSET>(sid) / DAFX_ELEM_STRIDE;
    desc.startAndLimit = start | (state.aliveLimit << 16);
    desc.dataRenStrideAndInstanceCount = dataRenStride | (state.aliveCount << 16);

    // state
    // we are not adding duplicated states (map search is quite heavy, so we check only sequential states)
    uint32_t reqShaderFlushChanges = 0;
    reqShaderFlushChanges |= dataSourceChange ? STATE_DATA_SRC : 0;
    reqShaderFlushChanges |= dispatchBufferIdChange ? STATE_DISP_SRC : 0;
    reqShaderFlushChanges |= shaderChange ? STATE_SHADER : 0;

    uint32_t changes = reqShaderFlushChanges;
    changes |= resChange ? STATE_TEXTURES : 0;
    changes |= smpChange ? STATE_SAMPLERS : 0;
    changes |= vrsChange ? STATE_VRS : 0;
    changes |= primPerElemChange ? STATE_PRIM_PER_ELEM : 0;
    changes |= reqShaderFlushChanges != 0 ? STATE_FLUSH_SHADER : 0;
    if (changes != 0)
    {
      DrawState &state = queue.states.push_back_noinit();
      state.resources = resList;
      state.shader = shader->get()->shader;
      state.dataSource = dataSource;
      state.dispatchBufferIdx = dispatchBufferId;
      state.primPerElem = primPerElem;
      state.vrs = vrs;
      state.changes = changes;
    }
    queue.drawCalls.push_back({(int)queue.states.size() - 1, state.aliveCount});

    if constexpr (INST_RENDERABLE_TRIS >= 0)
      stat_add(ctx.stats.renderedTriangles, *stream.getOpt<INST_RENDERABLE_TRIS, uint>(sid) * 2);
  }

  return queue.drawCalls.size() > 0;
}

bool update_dispatch_buffer(Context &ctx, DrawQueue &queue)
{
  TIME_D3D_PROFILE(update_dispatch_buffer);
  G_STATIC_ASSERT(DAFX_RENDER_GROUP_SIZE * sizeof(RenderDispatchDesc) >= 65536); // constant buffer are alocated for at least 65k
  int firstDispatchBufferIdx = ctx.currentRenderDispatchBuffer;
  for (int i = 0, ie = (queue.dispatches.size() - 1) / DAFX_RENDER_GROUP_SIZE + 1; i < ie; ++i)
  {
    int bufIdx = firstDispatchBufferIdx + i;
    GpuResourcePtr *buf = bufIdx < ctx.renderDispatchBuffers.size() ? &ctx.renderDispatchBuffers[bufIdx] : nullptr;
    if (!buf)
    {
      eastl::string name;
      name.append_sprintf("dafx_render_dispatch_buffer_%d", bufIdx);
      buf = &ctx.renderDispatchBuffers.push_back();
      if (!create_gpu_cb_res(*buf, sizeof(RenderDispatchDesc), DAFX_RENDER_GROUP_SIZE, name.c_str()))
        return false;
    }

    int ofs = i * DAFX_RENDER_GROUP_SIZE;
    int sz = min(DAFX_RENDER_GROUP_SIZE, (int)queue.dispatches.size() - ofs);
    if (!update_gpu_cb_buffer(buf->getBuf(), queue.dispatches.data() + ofs, sz * sizeof(RenderDispatchDesc)))
      return false;

    ctx.currentRenderDispatchBuffer++;
  }
  return true;
}

// TODO: add generic version with update_dispatch_buffer
bool update_multidraw_buffer(Context &ctx, dag::ConstSpan<DrawIndexedIndirectArgs> data)
{
  TIME_D3D_PROFILE(update_multidraw_buffer)
  G_STATIC_ASSERT(sizeof(DrawIndexedIndirectArgs) == DRAW_INDEXED_INDIRECT_BUFFER_SIZE);
  int firstBufIdx = ctx.currentMutltidrawBuffer;
  uint32_t flags = VBLOCK_WRITEONLY;
  flags |= d3d::get_driver_code().is(d3d::vulkan) ? VBLOCK_DISCARD : 0;
  for (int i = 0, ie = (data.size() - 1) / ctx.cfg.multidraw_buffer_size + 1; i < ie; ++i)
  {
    int bufIdx = firstBufIdx + i;
    UniqueBuf *buf = bufIdx < ctx.multidrawBufers.size() ? &ctx.multidrawBufers[bufIdx] : nullptr;
    if (!buf)
    {
      eastl::string name;
      name.append_sprintf("dafx_multidraw_buffer_%d", bufIdx);
      buf = &ctx.multidrawBufers.push_back();
      *buf = dag::create_sbuffer(INDIRECT_BUFFER_ELEMENT_SIZE, ctx.cfg.multidraw_buffer_size * DRAW_INDEXED_INDIRECT_NUM_ARGS,
        SBCF_INDIRECT, 0, name.c_str());
      if (!(*buf))
        return false;
    }

    int ofs = i * ctx.cfg.multidraw_buffer_size;
    int sz = min((int)ctx.cfg.multidraw_buffer_size, (int)data.size() - ofs);
    if (!buf->getBuf()->updateData(0, sz * sizeof(DrawIndexedIndirectArgs), data.data() + ofs, flags))
      return false;

    ctx.currentMutltidrawBuffer++;
  }
  return true;
}

bool prepare_multidraw(Context &ctx, DrawQueue &queue)
{
  int currentStateId = -1;
  int currentBufOfs = 0; // in elements, not bytes
  int prevBufOfs = 0;
  int currentBufId = ctx.currentMutltidrawBuffer;
  int lastDrawCallCount = 0;
  int lastElemsInBatch = 0;
  int lastPrimPerElem = 0;
  dag::Vector<DrawIndexedIndirectArgs, framemem_allocator> args;

  const bool allowSmallMultidraw = convar::allow_small_multidraw;

  queue.multidrawCalls.reserve(queue.drawCalls.size());
  {
    TIME_PROFILE(prepare_multidraw)
    for (const DrawCall &call : queue.drawCalls)
    {
      if (currentBufOfs == ctx.cfg.multidraw_buffer_size)
      {
        currentBufOfs = 0;
        currentBufId++;
      }

      if (call.stateId != currentStateId || currentBufOfs == 0 ||
          (lastElemsInBatch + call.instanceCount) * lastPrimPerElem >= g_prim_limit_16b)
      {
        if (!allowSmallMultidraw && lastDrawCallCount == 1)
        {
          args.pop_back();
          currentBufOfs = prevBufOfs;
        }

        currentStateId = call.stateId;
        MultiDrawCall &c = queue.multidrawCalls.push_back();
        c.stateId = call.stateId;
        c.bufOffset = currentBufOfs * sizeof(DrawIndexedIndirectArgs);
        c.bufId = currentBufId;
        c.drawCallCount = 0;
        c.totalPrimCount = 0;
        lastElemsInBatch = 0;
      }

      MultiDrawCall &mdCall = queue.multidrawCalls.back();
      const DrawState &state = queue.states[currentStateId];
      int totalPrimCount = call.instanceCount * state.primPerElem;

      DrawIndexedIndirectArgs &v = args.push_back_noinit();
      v.indexCountPerInstance = totalPrimCount * 3;
      v.instanceCount = 1;
      v.startIndexLocation = 0;
      v.baseVertexLocation = 0;
      v.startInstanceLocation = mdCall.drawCallCount;

      mdCall.totalPrimCount += totalPrimCount;
      lastDrawCallCount = ++mdCall.drawCallCount;
      prevBufOfs = currentBufOfs++;

      lastPrimPerElem = state.primPerElem;
      lastElemsInBatch += call.instanceCount; // we can ignore primPerElem, since it change will force new MD call anyway

      if (lastDrawCallCount >= ctx.cfg.max_multidraw_batch_size)
        currentStateId = -1;
    }
  }

  return update_multidraw_buffer(ctx, args);
}

inline void apply_drawcall_states(Context &ctx, const DrawQueue &queue, int state_id, d3d::SamplerHandle samplers[],
  int &current_state_id, int &current_prim_per_elem, TextureCallback tc)
{
  if (current_state_id == state_id)
    return;

  current_state_id = state_id;
  const DrawState &state = queue.states[current_state_id];
  const uint32_t changes = state.changes;
  if (changes & STATE_VRS)
    d3d::set_variable_rate_shading(state.vrs, state.vrs);

  if (changes & STATE_DISP_SRC)
  {
    d3d::resource_barrier({ctx.renderDispatchBuffers[state.dispatchBufferIdx].getBuf(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_PIXEL});
    ShaderGlobal::set_buffer(ctx.renderCallsVarId, ctx.renderDispatchBuffers[state.dispatchBufferIdx].getId());
    stat_inc(ctx.stats.renderSwitchDispatches);
  }

  if (changes & STATE_DATA_SRC)
  {
    if (state.dataSource != BAD_D3DRESID)
    {
      Sbuffer *buf = acquire_managed_buf(state.dataSource);
      G_FAST_ASSERT(buf);
      d3d::resource_barrier({buf, RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_PIXEL});
      release_managed_buf(state.dataSource);
    }

    ShaderGlobal::set_buffer(ctx.systemDataVarId, state.dataSource);
    stat_inc(ctx.stats.renderSwitchBuffers);
  }

  if (changes & STATE_FLUSH_SHADER)
  {
    state.shader->setStates(0, true);
    stat_inc(ctx.stats.renderSwitchShaders);
  }

  if (changes & STATE_TEXTURES)
  {
    for (int s = 0, se = state.resources->size(); s < se; ++s)
    {
      const TextureDesc &t = state.resources->at(s);
      BaseTexture *tex = D3dResManagerData::getBaseTex(t.texId);
      G_FAST_ASSERT(tex);
      d3d::set_tex(STAGE_PS, ctx.cfg.texture_start_slot + s, tex, false);
      stat_inc(ctx.stats.renderSwitchTextures);
    }

    if (tc && !state.resources->empty())
      tc(state.resources->begin()->texId);
  }

  if (changes & STATE_SAMPLERS)
  {
    for (int s = 0, se = state.resources->size(); s < se; ++s)
    {
      const TextureDesc &t = state.resources->at(s);
      d3d::set_sampler(STAGE_PS, ctx.cfg.texture_start_slot + s, t.anisotropic ? samplers[1] : samplers[0]);
    }
  }

  current_prim_per_elem = state.primPerElem;
  stat_inc(ctx.stats.renderSwitchRenderState);
}

bool render(ContextId cid, CullingId cull_id, const eastl::string &tag_name, float mip_bias, TextureCallback tc)
{
  TIME_D3D_PROFILE(dafx_render);
  GET_CTX_RET(false);

  ctx.frameBoundaryBufferManager.prepareRender();

  if (ctx.debugFlags & DEBUG_DISABLE_RENDER)
    return false;

  CullingState *cull = ctx.cullingStates.get(cull_id);
  G_ASSERT_RETURN(cull, false);
  G_ASSERT_RETURN(ctx.systemDataVarId >= 0, false);

  uint32_t tag = get_render_tag(ctx.shaders, tag_name, Config::max_render_tags);
  if (tag >= Config::max_render_tags)
    return false;

  const bool multidraw = ctx.cfg.multidraw_enabled && convar::enable_multidraw;

  DrawQueue queue;
  if (!prepare_render_queue(ctx, cull, tag, queue))
    return 0;

  if (!update_dispatch_buffer(ctx, queue))
    return 0;

  if (multidraw && !prepare_multidraw(ctx, queue))
    return 0;

  ctx.globalData.gpuBuf.setVar();

  d3d::setvdecl(BAD_VDECL);
  index_buffer::use_quads_16bit();
  // we still need serial buf for keeping shader logic the same
  // in case if multidraw is off - it contain only 1 value, which is 0
  d3d::setvsrc_ex(0, ctx.serialBuf.get().get(), 0, sizeof(uint32_t));

  // Get texture sampler (critical section)
  auto samplerHandlesIt = eastl::find_if(ctx.samplersCache.begin(), ctx.samplersCache.end(),
    [mip_bias](const SamplerHandles &sampler) { return abs(sampler.mipBias - mip_bias) < FLT_EPSILON; });
  if (samplerHandlesIt == ctx.samplersCache.end())
    samplerHandlesIt = &ctx.samplersCache.emplace_back(mip_bias);

  d3d::SamplerHandle samplerBilinear = samplerHandlesIt->samplerBilinear;
  d3d::SamplerHandle samplerAniso = samplerHandlesIt->samplerAniso;
  d3d::SamplerHandle samplers[] = {samplerBilinear, samplerAniso};

  int maxTextureSlotsAllocated = interlocked_relaxed_load(ctx.maxTextureSlotsAllocated);
  for (int i = 0; i < maxTextureSlotsAllocated; ++i) // to ensure there is no "bad" state still bound
    d3d::set_sampler(STAGE_PS, ctx.cfg.texture_start_slot + i, samplerBilinear);

  int totalDrawCalls = 0;
  int totalPrims = 0;
  int currentStateId = -1;
  int currentPrimPerElem = 0;

  if (multidraw)
  {
    TIME_D3D_PROFILE(exec_multidraw_calls);
#if _TARGET_C1 | _TARGET_C2 // workaround: flush caches for indirect args

#endif
    int dispatchId = 0;
    for (int i = 0, ie = queue.multidrawCalls.size(); i < ie; ++i)
    {
      const MultiDrawCall &call = queue.multidrawCalls[i];
      apply_drawcall_states(ctx, queue, call.stateId, samplers, currentStateId, currentPrimPerElem, tc);

      uint32_t params[] = {(uint32_t)dispatchId % DAFX_RENDER_GROUP_SIZE, (uint32_t)currentPrimPerElem * 2};
      d3d::set_immediate_const(STAGE_VS, params, countof(params));
      d3d::set_immediate_const(STAGE_PS, params, countof(params));

      int totalPrimCount = call.totalPrimCount;
      if (totalPrimCount > g_prim_limit_16b)
      {
        logerr("dafx: above index buffer limit (md): %d", totalPrimCount);
        totalPrimCount = g_prim_limit_16b;
      }

      int drawCallCount = call.drawCallCount;
      if (drawCallCount > 1) // no multridraw on 1 draw call instances
      {
        d3d::multi_draw_indexed_indirect(PRIM_TRILIST, ctx.multidrawBufers[call.bufId].getBuf(), drawCallCount,
          sizeof(DrawIndexedIndirectArgs), call.bufOffset);
        dispatchId += drawCallCount;
      }
      else
      {
        d3d::drawind(PRIM_TRILIST, 0, totalPrimCount, 0);
        dispatchId++;
      }

      totalDrawCalls++;
      totalPrims += call.totalPrimCount;
    }
  }
  else
  {
    TIME_D3D_PROFILE(exec_draw_calls);
    for (int i = 0, ie = queue.drawCalls.size(); i < ie; ++i)
    {
      const DrawCall &call = queue.drawCalls[i];
      apply_drawcall_states(ctx, queue, call.stateId, samplers, currentStateId, currentPrimPerElem, tc);

      int prims = currentPrimPerElem * call.instanceCount;
      if (prims > g_prim_limit_16b)
      {
        logerr("dafx: above index buffer limit (single): %d", prims);
        prims = g_prim_limit_16b;
      }

      uint32_t params[] = {(uint32_t)i % DAFX_RENDER_GROUP_SIZE, (uint32_t)currentPrimPerElem * 2};
      d3d::set_immediate_const(STAGE_VS, params, countof(params));
      d3d::set_immediate_const(STAGE_PS, params, countof(params));
      d3d::drawind(PRIM_TRILIST, 0, prims, 0);

      totalDrawCalls++;
      totalPrims += currentPrimPerElem * call.instanceCount;
    }
  }

  d3d::set_immediate_const(STAGE_VS, nullptr, 0);
  d3d::set_immediate_const(STAGE_PS, nullptr, 0);

  ShaderGlobal::set_buffer(ctx.systemDataVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(ctx.renderCallsVarId, BAD_D3DRESID);

  stat_add(ctx.stats.renderDrawCalls, totalDrawCalls);
  stat_add(ctx.stats.visibleTriangles, totalPrims);

#if DAGOR_DBGLEVEL > 0
  ctx.stats.drawCallsByRenderTags[tag] += totalDrawCalls;
#endif

  if (ctx.cfg.vrs_enabled)
    d3d::set_variable_rate_shading(1, 1);

  return totalDrawCalls > 0;
}

void reset_samplers_cache(ContextId cid)
{
  GET_CTX();
  ctx.samplersCache.clear();
}

void render_debug_opt(ContextId cid)
{
  GET_CTX();

  if (!(ctx.debugFlags & DEBUG_SHOW_BBOXES))
    return;

  begin_draw_cached_debug_lines(false, false);
  set_cached_debug_lines_wtm(TMatrix::IDENT);

  InstanceGroups &stream = ctx.instances.groups;
  for (int i = 0; i < stream.size(); ++i)
  {
    const uint32_t &flags = stream.get<INST_FLAGS>(i);

    if (!(flags & SYS_RENDERABLE))
      continue;

    G_FAST_ASSERT(flags & SYS_VALID);

    E3DCOLOR c;
    bbox3f box_vec4 = stream.get<INST_BBOX>(i);
    Point4 bmin, bmax;
    v_stu(&bmin.x, box_vec4.bmin);
    v_stu(&bmax.x, box_vec4.bmax);
    BBox3 box;
    box[0] = Point3::xyz(bmin);
    box[1] = Point3::xyz(bmax);
    Point4 pos = stream.get<INST_POSITION>(i);

    bool bboxEmpty = box.isempty();
    if (bboxEmpty)
      box.makecube(Point3(pos.x, pos.y, pos.z), 0.5);

    unsigned int lastFrame = stream.get<INST_LAST_VALID_BBOX_FRAME>(i);
    bool isActive = !(flags & SYS_CULL_FETCHED) || (ctx.currentFrame - lastFrame) < Config::max_inactive_frames;
    bool isStopped = stream.get<INST_EMITTER_STATE>(i).totalTickRate == 0;
    bool isAlive = stream.get<INST_ACTIVE_STATE>(i).aliveCount > 0;

    if (!(flags & SYS_ENABLED))
    {
      c = E3DCOLOR(255, 0, 0, 127);
    }
    else if (!(flags & SYS_VISIBLE))
    {
      c = E3DCOLOR(127, 0, 0, 127);
    }
    else if (!isActive)
    {
      c = E3DCOLOR(0, 255, 255, 127);
    }
    else if (!isAlive)
    {
      c = E3DCOLOR(255, 0, 255, 127);
    }
    else if (isStopped)
    {
      c = E3DCOLOR(255, 255, 255, 127);
    }
    else if (!(flags & SYS_BBOX_VALID) || bboxEmpty)
    {
      c = E3DCOLOR(255, 255, 0, 127);
    }
    else
    {
      int rndSeed = i;
      float c0 = _frnd(rndSeed) * 0.5 + 0.5;
      float c1 = _frnd(rndSeed) * 0.5 + 0.5;
      float c2 = _frnd(rndSeed) * 0.5 + 0.5;
      c = E3DCOLOR(c0 * 127, c1 * 127, c2 * 127, 127);
    }

    draw_cached_debug_box(box, c);
    draw_cached_debug_sphere(Point3::xyz(pos), 0.25, c);
  }
  end_draw_cached_debug_lines();
}
} // namespace dafx
