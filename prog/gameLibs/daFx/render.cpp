#include "context.h"
#include <3d/dag_drv3dCmd.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <debug/dag_debug3d.h>
#include <3d/dag_quadIndexBuffer.h>
#include <math/random/dag_random.h>
#include <daFx/dafx_render_dispatch_desc.hlsli>
#include "frameBoundaryBufferManager.h"


namespace dafx
{
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

  d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

  update_cpu_render_buffer(ctx, tags_mask);

  d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);
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

void set_rendering_resolution(ContextId cid, const IPoint2 &resolution)
{
  GET_CTX();
  ctx.renderingResolution = resolution;
}

bool render(ContextId cid, CullingId cull_id, const eastl::string &tag_name)
{
  TIME_D3D_PROFILE(dafx_render);
  GET_CTX_RET(false);

  ctx.frameBoundaryBufferManager.prepareRender();

  if (ctx.debugFlags & DEBUG_DISABLE_RENDER)
    return false;

  CullingState *cull = ctx.cullingStates.get(cull_id);
  G_ASSERT_RETURN(cull, false);

  uint32_t tag = get_render_tag(ctx.shaders, tag_name, Config::max_render_tags);
  if (tag >= Config::max_render_tags)
    return false;

  bool vrsEnabled = ctx.cfg.vrs_enabled && cull->shadingRates[tag] > 0;
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

  eastl::vector<int> &workers = cull->workers[tag];
  if (workers.empty())
    return false;

  G_ASSERT_RETURN(ctx.systemDataVarId >= 0, false);
  G_ASSERT_RETURN(ctx.screenPosToTcVarId >= 0, false);

  InstanceGroups &stream = ctx.instances.groups;
  const uint32_t validationFlags = SYS_ENABLED | SYS_VALID | SYS_VISIBLE | SYS_RENDERABLE | SYS_RENDER_REQ;

  enum
  {
    DISP_COUNTS,
    DISP_SOURCE,
    DISP_SHADER,
    DISP_BINDS,
    DISP_STRUCT,
    DISP_PRIMS,
    DISP_VRS,
    DISP_CUSTOM_DEPTH,
  };

  eastl::tuple_vector_alloc<framemem_allocator,
    int,                              // counts
    D3DRESID,                         // data source
    RenderShaderId,                   // shader
    const eastl::vector<TEXTUREID> *, // binded resources
    RenderDispatchDesc,               // dispatch
    int,                              // prims
    int,                              // vrs (opt)
    bool                              // custom depth
    >
    dispatches;

  dispatches.reserve(workers.size());

  // merge all call descs to 1 buffer
  {
    TIME_PROFILE(merge);
    for (int ii = 0, iie = workers.size(); ii < iie; ++ii)
    {
      int sid = workers[ii];

      const uint32_t &flags = stream.get<INST_FLAGS>(sid);
      // it is possible that instance has died or reseted between update and render
      if ((flags & validationFlags) != validationFlags)
        continue;

      const eastl::vector<TEXTUREID> *resList = &stream.get<INST_LOCAL_RES_PS>(sid);
      ;
      bool texLoaded = true;
      for (TEXTUREID tid : *resList)
      {
        mark_managed_tex_lfu(tid);
        if (!check_managed_texture_loaded(tid))
        {
          texLoaded = false;
          break;
        }
        G_ASSERT(D3dResManagerData::getBaseTex(tid));
      }

      if (!texLoaded) // no need to render fx with stub textures
        continue;

      G_FAST_ASSERT(!(flags & SYS_CPU_RENDER_REQ) || (flags & SYS_RENDER_BUF_TAKEN));

      D3DRESID dispSource = stream.get<INST_TARGET_RENDER_BUF>(sid);
      if (!dispSource) // render buffer was not allocated, skipping (reset device, OOM)
        continue;

      const InstanceState &state = stream.get<INST_ACTIVE_STATE>(sid);

      auto dst = dispatches.push_back();

      eastl::get<DISP_COUNTS>(dst) = state.aliveCount;
      G_FAST_ASSERT(eastl::get<0>(dst));

      eastl::get<DISP_SOURCE>(dst) = dispSource;

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

      G_FAST_ASSERT(renderShaderId);
      uint32_t tags = stream.get<INST_RENDER_TAGS>(sid);
      int vrs = 1;
      // extract shading rate by associating render tag with culling predefines for those tags
      if (vrsEnabled)
      {
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
      eastl::get<DISP_VRS>(dst) = vrs;
      eastl::get<DISP_SHADER>(dst) = renderShaderId;

      eastl::get<DISP_BINDS>(dst) = resList;
      eastl::get<DISP_PRIMS>(dst) = stream.get<INST_PRIM_PER_PART>(sid);
      eastl::get<DISP_CUSTOM_DEPTH>(dst) = ctx.customDepth && (flags & SYS_CUSTOM_DEPTH);

      RenderDispatchDesc &desc = eastl::get<DISP_STRUCT>(dst);
      desc.dataRenOffset = stream.get<INST_TARGET_RENDER_DATA_OFFSET>(sid) / DAFX_ELEM_STRIDE;
      desc.parentRenOffset = stream.get<INST_TARGET_RENDER_PARENT_OFFSET>(sid) / DAFX_ELEM_STRIDE;

      int start = (flags & SYS_CPU_RENDER_REQ) ? 0 : state.aliveStart;
      int dataRenStride = stream.get<INST_RENDER_ELEM_STRIDE>(sid) / DAFX_ELEM_STRIDE;
      int parentRenStride = stream.get<INST_PARENT_RENDER_ELEM_STRIDE>(sid) / DAFX_ELEM_STRIDE;

      desc.startAndLimit = start | (state.aliveLimit << 16);
      desc.dataAndParentRenStride = dataRenStride | (parentRenStride << 16);

      // TODO: add bounds check/asserts!

      stat_add(ctx.stats.renderedTriangles, stream.get<INST_RENDERABLE_TRIS>(sid) * 2);
    }
  }

  if (dispatches.empty())
    return 0;

  // transfer dispatches to gpu, cbuffer max size = DAFX_RENDER_GROUP_SIZE
  G_STATIC_ASSERT(DAFX_RENDER_GROUP_SIZE * sizeof(RenderDispatchDesc) == 65536); // constant buffer are alocated for at least 65k!
  int firstDispatchBufferIdx = ctx.currentRenderDispatchBuffer;
  {
    TIME_D3D_PROFILE(gather_dispatches);
    for (int i = 0, ie = (dispatches.size() - 1) / DAFX_RENDER_GROUP_SIZE + 1; i < ie; ++i)
    {
      int bufIdx = firstDispatchBufferIdx + i;
      GpuResourcePtr *buf = bufIdx < ctx.renderDispatchBuffers.size() ? &ctx.renderDispatchBuffers[bufIdx] : nullptr;
      if (!buf)
      {
        eastl::string name;
        name.append_sprintf("dafx_render_dispatch_buffer_%d", bufIdx);
        buf = &ctx.renderDispatchBuffers.push_back();
        if (!create_gpu_cb_res(*buf, sizeof(RenderDispatchDesc), DAFX_RENDER_GROUP_SIZE, name.c_str()))
          return 0;
      }

      int ofs = i * DAFX_RENDER_GROUP_SIZE;
      int sz = min(DAFX_RENDER_GROUP_SIZE, (int)dispatches.size() - ofs);
      if (!update_gpu_cb_buffer(buf->getBuf(), dispatches.get<RenderDispatchDesc>() + ofs, sz * sizeof(RenderDispatchDesc)))
        return 0;

      ctx.currentRenderDispatchBuffer++;
    }
  }

  ctx.globalData.gpuBuf.setVar();

  d3d::setvdecl(BAD_VDECL);
  d3d::setvsrc(0, NULL, 0);
  index_buffer::use_quads_16bit();

  int drawCalls = 0;
  int totalPrims = 0;
  int curDispatchBuffer = -1;
  RenderShaderId curShaderId;
  D3DRESID curBufId = BAD_D3DRESID;
  int curVrsRate = 1;

  eastl::array<TEXTUREID, Config::max_res_slots> curResList;
  curResList.fill(BAD_TEXTUREID);

  Driver3dRenderTarget defaultRt;
  IPoint2 renderingResolution = ctx.renderingResolution;
  if (renderingResolution.x <= 0 || renderingResolution.y <= 0)
  {
    d3d::get_render_target(defaultRt);
    d3d::get_render_target_size(renderingResolution.x, renderingResolution.y, defaultRt.getColor(0).tex);
  }
  ShaderGlobal::set_color4(ctx.screenPosToTcVarId,
    Point4(1.f / renderingResolution.x, 1.f / renderingResolution.y, HALF_TEXEL_OFSF / renderingResolution.x,
      HALF_TEXEL_OFSF / renderingResolution.y));

  bool customDepth = false;
  {
    TIME_D3D_PROFILE(render);
    for (int i = 0, ie = dispatches.size(); i < ie; ++i)
    {
      bool setStates = false;
      int newDispatchBuffer = firstDispatchBufferIdx + i / DAFX_RENDER_GROUP_SIZE;
      D3DRESID nextBufId = dispatches.get<DISP_SOURCE>()[i];
      RenderShaderId nextShaderId = dispatches.get<DISP_SHADER>()[i];
      bool nextCustomDepth = dispatches.get<DISP_CUSTOM_DEPTH>()[i];
      int nextVrsRate = dispatches.get<DISP_VRS>()[i];

      if (nextVrsRate != curVrsRate)
      {
        curVrsRate = nextVrsRate;
        d3d::set_variable_rate_shading(curVrsRate, curVrsRate);
      }

      if (customDepth != nextCustomDepth)
      {
        if (nextCustomDepth)
          d3d::set_depth(ctx.customDepth, false);
        else
          d3d::set_render_target(defaultRt);
        customDepth = nextCustomDepth;
        setStates = true;
      }

      if (newDispatchBuffer != curDispatchBuffer)
      {
        setStates = true;
        curDispatchBuffer = newDispatchBuffer;
        d3d::resource_barrier({ctx.renderDispatchBuffers[curDispatchBuffer].getBuf(), RB_RO_SRV | RB_STAGE_VERTEX});
        ShaderGlobal::set_buffer(ctx.renderCallsVarId, ctx.renderDispatchBuffers[curDispatchBuffer].getId());
        stat_inc(ctx.stats.renderSwitchDispatches);
      }

      if (curBufId != nextBufId)
      {
        setStates = true;
        curBufId = nextBufId;
        if (curBufId != BAD_D3DRESID)
        {
          Sbuffer *buf = acquire_managed_buf(curBufId);
          d3d::resource_barrier({buf, RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_PIXEL});
          release_managed_buf(curBufId);
        }
        ShaderGlobal::set_buffer(ctx.systemDataVarId, curBufId);
        stat_inc(ctx.stats.renderSwitchBuffers);
      }

      if (curShaderId != nextShaderId || setStates)
      {
        curShaderId = nextShaderId;
        RenderShaderPtr *sh = ctx.shaders.renderShaders.get(curShaderId);
        G_FAST_ASSERT(sh);

        sh->get()->shader->setStates(0, true);
        stat_inc(ctx.stats.renderSwitchShaders);
      }

      const eastl::vector<TEXTUREID> &resList = *dispatches.get<DISP_BINDS>()[i];
      for (int sl = 0, sle = resList.size(); sl < sle; ++sl)
      {
        TEXTUREID id = resList[sl];
        G_FAST_ASSERT(id != BAD_TEXTUREID);
        if (id != curResList[sl])
        {
          curResList[sl] = id;
          d3d::set_tex(STAGE_PS, ctx.cfg.texture_start_slot + sl, D3dResManagerData::getBaseTex(id), true);
          stat_inc(ctx.stats.renderSwitchTextures);
        }
      }

      int instances = dispatches.get<DISP_COUNTS>()[i];
      int prims = dispatches.get<DISP_PRIMS>()[i];

      uint32_t params[] = {(uint32_t)i % DAFX_RENDER_GROUP_SIZE, (uint32_t)instances};
      d3d::set_immediate_const(STAGE_VS, params, 2);
      d3d::set_immediate_const(STAGE_PS, params, 2);

      d3d::drawind_instanced(PRIM_TRILIST, 0, prims, 0, instances);
      drawCalls++;
      totalPrims += prims * instances;
    }
  }

  d3d::set_immediate_const(STAGE_VS, nullptr, 0);

  ShaderGlobal::set_buffer(ctx.systemDataVarId, BAD_D3DRESID);
  ShaderGlobal::set_buffer(ctx.renderCallsVarId, BAD_D3DRESID);

  if (ctx.renderingResolution.x <= 0 || ctx.renderingResolution.y <= 0)
  {
    d3d::set_render_target(defaultRt);
  }
  stat_add(ctx.stats.renderDrawCalls, drawCalls);
  stat_add(ctx.stats.visibleTriangles, totalPrims);

  if (vrsEnabled)
    d3d::set_variable_rate_shading(1, 1);

  return drawCalls > 0;
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
