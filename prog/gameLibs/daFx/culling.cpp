// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "culling.h"
#include "context.h"
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_query.h>
#include <util/dag_stlqsort.h>
#include <math/dag_frustum.h>
#include <memory/dag_framemem.h>
#include <generic/dag_smallTab.h>
#include <scene/dag_occlusion.h>
#include <util/dag_convar.h>

namespace convar
{
CONSOLE_FLOAT_VAL("dafx", screen_area_discard_threshold, 0.f);
CONSOLE_FLOAT_VAL("dafx", screen_area_discard_threshold_mul, 1.f);
CONSOLE_BOOL_VAL("dafx", allow_screen_area_discard, true);
} // namespace convar

namespace dafx
{
CullingId create_culling_state(ContextId cid, const eastl::vector<CullingDesc> &descs)
{
  GET_CTX_RET(CullingId());
  G_ASSERT_RETURN(!descs.empty(), CullingId());

  CullingState cull;
  cull.shadingRates.fill(0);
  cull.remapTags.fill(-1);
  cull.vrsRemapTags.fill(-1);
  cull.renderTags.reserve(descs.size());
  cull.discardThreshold.fill(0);

  DBG_OPT("create_culling_state: elems: %d", descs.size());
  for (const CullingDesc &i : descs)
  {
    uint32_t tag = register_render_tag(ctx.shaders, i.tag, Config::max_render_tags);
    G_ASSERT_RETURN(tag < Config::max_render_tags, CullingId());

    cull.renderTagsMask |= 1 << tag;
    cull.renderTags.push_back(tag);
    cull.sortings[tag] = i.sortingType;
    cull.shadingRates[tag] = i.vrsRate;
    cull.discardThreshold[tag] = i.screenAreaDiscardThreshold;
    cull.visibilityMask = i.visibilityMask;
    G_ASSERT(i.vrsRate <= 2); // basic VRS supports only 1xN - 2xN rates
    DBG_OPT("  render_tag: %s, sorting: %d", i.tag, (int)i.sortingType);
  }

  for (uint32_t tag : cull.renderTags)
  {
    cull.remapTags[tag] = tag;
    cull.vrsRemapTags[tag] = tag;
  }

  int vrsFirstTag = -1;
  for (int i = 0; i < cull.shadingRates.size(); ++i)
  {
    if (cull.shadingRates[i] == 0)
      continue;

    if (vrsFirstTag < 0)
    {
      vrsFirstTag = i;
      continue;
    }

    cull.vrsRemapTags[i] = vrsFirstTag;
  }

  G_ASSERT_RETURN(cull.renderTagsMask, CullingId());

  return ctx.cullingStates.emplaceOne(eastl::move(cull));
}

CullingId create_proxy_culling_state(ContextId cid)
{
  GET_CTX_RET(CullingId());

  CullingState cull;
  cull.isProxy = true;
  return ctx.cullingStates.emplaceOne(eastl::move(cull));
}

void destroy_culling_state(ContextId cid, CullingId cull_id)
{
  GET_CTX();
  ctx.cullingStates.destroyReference(cull_id);
}

void clear_culling_state(ContextId cid, CullingId cull_id)
{
  GET_CTX();

  CullingState *cull = ctx.cullingStates.get(cull_id);
  G_ASSERT_RETURN(cull, );

  for (int i = 0; i < cull->workers.size(); ++i)
    cull->workers[i].clear();
}

uint32_t get_culling_state_visiblity_mask(ContextId cid, CullingId cullid)
{
  GET_CTX_RET(0);

  CullingState *cull = ctx.cullingStates.get(cullid);
  G_ASSERT_RETURN(cull, 0);

  return cull->visibilityMask;
}

void remap_culling_state_tag(ContextId cid, CullingId cullid, const eastl::string &from, const eastl::string &to)
{
  GET_CTX();

  CullingState *cull = ctx.cullingStates.get(cullid);
  G_ASSERT_RETURN(cull, );

  RenderTagMap::iterator it = ctx.shaders.renderTagMap.find(from);
  G_ASSERT_RETURN(it != ctx.shaders.renderTagMap.end(), );
  int fromTag = it->second;
  G_ASSERT_RETURN(cull->renderTagsMask & (1 << fromTag), );

  it = ctx.shaders.renderTagMap.find(to);
  int toTag = it != ctx.shaders.renderTagMap.end() ? it->second : -1;
  G_ASSERT_RETURN(toTag == -1 || (cull->renderTagsMask & (1 << toTag)), );

  cull->remapTags[fromTag] = toTag;
}

uint32_t inverse_remap_tags_mask(ContextId cid, uint32_t tags_mask)
{
  GET_CTX_RET(tags_mask);

  uint32_t result = tags_mask;
  for (int i = 0; i < ctx.cullingStates.totalSize(); ++i)
  {
    const CullingState *cull = ctx.cullingStates.cgetByIdx(i);
    if (!cull || cull->isProxy)
      continue;

    for (int t = 0, te = cull->renderTags.size(); t < te; ++t)
    {
      int tag = cull->renderTags[t];
      int remap = cull->remapTags[tag];

      if (tag != remap && remap >= 0 && (tags_mask & (1 << remap)))
        result |= 1 << tag;
    }
  }

  return result;
}

bool init_culling(Context &ctx)
{
  ComputeShaderElement *res = new_compute_shader(ctx.cfg.culling_discard_shader, true);
  if (!res)
  {
    logerr("dafx: can't create cull_disard shader:%s", ctx.cfg.culling_discard_shader);
    return false;
  }
  ctx.culling.discardShader.reset(res);
  return true;
}

void reset_culling(Context &ctx, bool clear_cpu)
{
  for (int i = 0; i < Config::max_culling_feedbacks; ++i)
  {
    CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[i];
    feedback.allocCount = 0;
    feedback.readbackIssued = false;
    feedback.queryIssued = false;
    feedback.gpuRes.close();
    feedback.eventQuery.reset(nullptr);
    feedback.frameWorkers.clear();
  }
  ctx.culling.gpuWorkers.clear();

  if (clear_cpu)
    ctx.culling.cpuWorkers.clear();
}

void prepare_cpu_culling(Context &ctx, bool exec_clear)
{
  TIME_D3D_PROFILE(dafx_prepare_cpu_culling);

  if (ctx.culling.cpuWorkers.size() > ctx.culling.cpuResSize)
  {
    ctx.culling.cpuResSize = ctx.culling.cpuWorkers.size();
    create_cpu_res(ctx.culling.cpuRes, DAFX_CULLING_STRIDE, ctx.culling.cpuResSize, "dafx_culling");
  }

  unsigned char *ptr = ctx.culling.cpuRes.get();

  const uint32_t discardData[] = {DAFX_CULLING_DISCARD_MIN, DAFX_CULLING_DISCARD_MIN, DAFX_CULLING_DISCARD_MIN, 0,
    DAFX_CULLING_DISCARD_MAX, DAFX_CULLING_DISCARD_MAX, DAFX_CULLING_DISCARD_MAX, 0};

  if (exec_clear)
    for (int i = 0, ie = ctx.culling.cpuWorkers.size(); i < ie; ++i)
      memcpy(ptr + i * DAFX_CULLING_STRIDE, discardData, DAFX_CULLING_STRIDE);

  stat_set(ctx.stats.cpuCullElems, ctx.culling.cpuWorkers.size());
}

bool prepare_gpu_culling(Context &ctx, bool exec_clear)
{
  TIME_D3D_PROFILE(dafx_prepare_gpu_culling);

  if (ctx.culling.gpuWorkers.empty())
    return false;

  ctx.culling.gpuFeedbackIdx = (ctx.culling.gpuFeedbackIdx + 1) % Config::max_culling_feedbacks;
  CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[ctx.culling.gpuFeedbackIdx];
  if (feedback.readbackIssued)
  {
    feedback.readbackIssued = false;
    if (feedback.queryIssued)
    {
      feedback.eventQuery.reset(nullptr);
      feedback.queryIssued = false;
    }
  }

  eastl::swap(feedback.frameWorkers, ctx.culling.gpuWorkers); // same as move
  ctx.culling.gpuWorkers.clear();

  // copy flags from this frame to the history
  InstanceGroups &stream = ctx.instances.groups;
  feedback.frameFlags.resize(feedback.frameWorkers.size());
  for (int i = 0, ie = feedback.frameWorkers.size(); i < ie; ++i)
    feedback.frameFlags[i] = stream.get<INST_FLAGS>(feedback.frameWorkers[i]);

  if (feedback.frameWorkers.size() > feedback.allocCount)
  {
    feedback.gpuRes.close();
    feedback.allocCount = feedback.frameWorkers.size() * 2;

    DBG_OPT("resizing culling feedback buf, frame: %d, alloc: %d", ctx.culling.gpuFeedbackIdx, feedback.allocCount);

    eastl::string name;
    name.append_sprintf("dafx_culling_rb_%d", ctx.culling.gpuFeedbackIdx);

    // metal supports atomics only for 1 components values
    if (!create_gpu_rb_res(feedback.gpuRes, sizeof(int), feedback.allocCount * 2 * 4, name.c_str())) // 2 lines, 4 components each
    {
      logerr("dafx: can't create culling buffer");
      feedback.allocCount = 0;
      feedback.frameWorkers.clear();
      return false;
    }

    feedback.eventQuery.reset(nullptr);
    feedback.queryIssued = false;
  }

  if (!feedback.eventQuery)
    feedback.eventQuery.reset(d3d::create_event_query());

  if (exec_clear)
  {
    uint count = feedback.frameWorkers.size();

    if (!d3d::set_immediate_const(STAGE_CS, &count, 1))
    {
      feedback.allocCount = 0;
      return false;
    }

    if (!d3d::set_rwbuffer(STAGE_CS, DAFX_CULLING_DATA_UAV_SLOT, feedback.gpuRes.getBuf()))
    {
      feedback.allocCount = 0;
      return false;
    }

    ctx.culling.discardShader->dispatch((feedback.frameWorkers.size() - 1) / DAFX_DEFAULT_WARP + 1, 1, 1);
    d3d::resource_barrier({feedback.gpuRes.getBuf(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, DAFX_CULLING_DATA_UAV_SLOT, nullptr);
  }

  stat_set(ctx.stats.gpuCullElems, feedback.frameWorkers.size());
  return true;
}

void issue_culling_feedback(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_issue_culling_feedback);

  CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[ctx.culling.gpuFeedbackIdx];
  if (feedback.frameWorkers.empty())
    return;

  G_ASSERT(!feedback.queryIssued && !feedback.readbackIssued);
  G_ASSERT_RETURN(feedback.allocCount > 0 && feedback.gpuRes.getBuf() && feedback.eventQuery, );

  Sbuffer *buf = feedback.gpuRes.getBuf();
  if (buf->lock(0, 0, (void **)nullptr, VBLOCK_READONLY))
    buf->unlock();

  feedback.readbackIssued = true;
  feedback.queryIssued = d3d::issue_event_query(feedback.eventQuery.get());

  if (!feedback.queryIssued)
    feedback.frameWorkers.clear(); // to skip actual fetch
}

void fetch_culling(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_fetch_culling);

  bool fetched = false;
  ctx.culling.gpuFetchedIdx = -1;

  for (int i = 1; i < Config::max_culling_feedbacks; ++i)
  {
    // wrapped prev idx
    int prevIdx = (ctx.culling.gpuFeedbackIdx + Config::max_culling_feedbacks - i) % Config::max_culling_feedbacks;
    CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[prevIdx];

    if (feedback.frameWorkers.empty() || !feedback.queryIssued)
      continue;

    bool force = (i == Config::max_culling_feedbacks - 1);
    if (force)
      DBG_OPT("dafx: culling query is not responding, flushing");

    if (fetched)
      feedback.frameWorkers.clear(); // no need to keep older culling

    bool status = d3d::get_event_query_status(feedback.eventQuery.get(), force);
    feedback.queryIssued = !status;

    if (!fetched && status)
    {
      fetched = true;
      ctx.culling.gpuFetchedIdx = prevIdx;
    }
  }
}

__forceinline bool pull_culling_data(const unsigned char *__restrict data, int cull_id, uint32_t &flags, bbox3f &bbox,
  uint *active_tris, bool overwrite_box)
{
  vec4f cullingScale = v_make_vec4f(DAFX_CULLING_SCALE, DAFX_CULLING_SCALE, DAFX_CULLING_SCALE, 0);

  int32_t *__restrict val = (int32_t *)(data + cull_id * DAFX_CULLING_STRIDE);
  if (active_tris)
    *active_tris = val[3];

  bbox3f nb;
  nb.bmin = v_cvti_vec4f(v_ldui(val)); // cull data is aligned as uint4, so this is fine
  nb.bmax = v_cvti_vec4f(v_ldui(val + 4));

  nb.bmin = v_mul(nb.bmin, cullingScale);
  nb.bmax = v_mul(nb.bmax, cullingScale);

  if (overwrite_box) // normal scenario
  {
    bbox = nb;
  }
  else // simulation was skipped, so culling data only from emission phase is valid. preserve previous box
  {
    if (!v_bbox3_is_empty(nb)) // emission was issued this frame, add it to previous box
      v_bbox3_add_box(bbox, nb);
  }

  bool r = !v_bbox3_is_empty(bbox);

  flags &= ~SYS_VISIBLE;
  flags |= SYS_CULL_FETCHED;

#if DAFX_DEBUG_CULLING_FETCH
  G_FAST_ASSERT(val[3] == cull_id && val[7] == cull_id);
#endif

  return r;
}

void process_cpu_culling(Context &ctx, int start, int count)
{
  G_ASSERT(ctx.culling.cpuWorkers.size() >= start + count);

  const uint32_t cpuValidationFlags = SYS_ENABLED | SYS_VALID | SYS_RENDERABLE | SYS_RENDER_REQ | SYS_CPU_RENDER_REQ;
  InstanceGroups &stream = ctx.instances.groups;

  const unsigned char *__restrict cpuData = ctx.culling.cpuRes.get();
  G_ASSERT_RETURN(cpuData || ctx.culling.cpuWorkers.empty(), );
  for (int i = start, ie = start + count; i < ie; ++i)
  {
    int sid = ctx.culling.cpuWorkers[i];
    int cullId = i;

    uint32_t &flags = stream.get<INST_FLAGS>(sid);
    G_UNUSED(cpuValidationFlags);
    G_FAST_ASSERT((flags & cpuValidationFlags) == cpuValidationFlags);

    bool validBox = pull_culling_data(cpuData, cullId, flags, stream.get<INST_BBOX>(sid),
      stream.getOpt<INST_RENDERABLE_TRIS, uint>(sid), !(flags & SYS_SKIP_SIMULATION_ON_THIS_FRAME));
    flags &= ~SYS_BBOX_VALID;
    flags |= validBox ? SYS_BBOX_VALID : 0;
  }
}

void process_gpu_culling(Context &ctx)
{
  TIME_D3D_PROFILE(dafx_process_gpu_culling);

  if (ctx.culling.gpuFetchedIdx < 0)
    return;

  CullingGpuFeedback &feedback = ctx.culling.gpuFeedbacks[ctx.culling.gpuFetchedIdx];
  if (feedback.frameWorkers.empty())
    return;

  G_ASSERT(feedback.eventQuery && d3d::get_event_query_status(feedback.eventQuery.get(), false));

  Sbuffer *gpuBuf = feedback.gpuRes.getBuf();
  G_ASSERT_RETURN(gpuBuf, );

  unsigned char *__restrict gpuData = nullptr;
  if (!gpuBuf->lock(0, 0, (void **)&gpuData, VBLOCK_READONLY))
    return;

  if (!gpuData)
    return;

  const uint32_t gpuValidationFlags = SYS_ENABLED | SYS_VALID | SYS_RENDERABLE | SYS_RENDER_REQ | SYS_GPU_RENDER_REQ;
  InstanceGroups &stream = ctx.instances.groups;

  for (int i = 0, ie = feedback.frameWorkers.size(); i < ie; ++i)
  {
    int sid = feedback.frameWorkers[i];
    int cullId = i;

    const uint32_t historyFlags = feedback.frameFlags[i];
    uint32_t &flags = stream.get<INST_FLAGS>(sid);
    if ((flags & gpuValidationFlags) != gpuValidationFlags)
      continue; // it can be already dead, because we have 1..5 frames lag

    bool validBox = pull_culling_data(gpuData, cullId, flags, stream.get<INST_BBOX>(sid),
      stream.getOpt<INST_RENDERABLE_TRIS, uint>(sid), !(historyFlags & SYS_SKIP_SIMULATION_ON_THIS_FRAME));
    if (validBox)
    {
      alignas(vec4f) Point4 emitterPos = stream.get<INST_POSITION>(sid);
      v_bbox3_add_pt(stream.get<INST_BBOX>(sid), bitwise_cast<vec4f>(emitterPos)); // compensates for feedback latency.
    }
    flags &= ~SYS_BBOX_VALID;
    flags |= validBox ? SYS_BBOX_VALID : 0;
  }

  gpuBuf->unlock();
}

void sort_culling_states(Context &ctx, CullingState &cull, const Point3 &view_pos)
{
  TIME_D3D_PROFILE(dafx_sort_culling_states);

  // ok, our distances are positive, so we can use integers to sort them

  const vec4f viewPos = v_ldu(&view_pos.x);

  auto sortingIt = cull.sortings.cbegin();
  for (auto &worker : cull.workers)
  {
    SortingType sortingType = *sortingIt++;
    if (sortingType == SortingType::BACK_TO_FRONT || sortingType == SortingType::FRONT_TO_BACK)
    {
      for (auto sid : worker)
      {
        if (sid < 0)
          continue;

        vec4f v = v_ldu(&ctx.instances.groups.get<INST_POSITION>(sid).x);
        reinterpret_cast<int &>(ctx.instances.groups.get<INST_VIEW_DIST>(sid)) =
          v_extract_xi(v_cast_vec4i(v_add_x(v_length3_sq_x(v_sub(v, viewPos)), v_splat_w(v))));
      }
    }

    if (sortingType == SortingType::BACK_TO_FRONT)
    {
      stlsort::sort(worker.begin(), worker.end(), [&](const int &sid0, const int &sid1) {
        int v0 = reinterpret_cast<int &>(ctx.instances.groups.get<INST_VIEW_DIST>(sid0));
        int v1 = reinterpret_cast<int &>(ctx.instances.groups.get<INST_VIEW_DIST>(sid1));
        return v0 != v1
                 ? v0 > v1
                 : ctx.instances.groups.get<INST_RENDER_SORT_DEPTH>(sid0) < ctx.instances.groups.get<INST_RENDER_SORT_DEPTH>(sid1);
      });
    }
    else if (sortingType == SortingType::FRONT_TO_BACK)
    {
      stlsort::sort(worker.begin(), worker.end(), [&](const int &sid0, const int &sid1) {
        int v0 = reinterpret_cast<int &>(ctx.instances.groups.get<INST_VIEW_DIST>(sid0));
        int v1 = reinterpret_cast<int &>(ctx.instances.groups.get<INST_VIEW_DIST>(sid1));
        return v0 != v1
                 ? v0 < v1
                 : ctx.instances.groups.get<INST_RENDER_SORT_DEPTH>(sid0) < ctx.instances.groups.get<INST_RENDER_SORT_DEPTH>(sid1);
      });
    }
    else if (sortingType == SortingType::BY_SHADER)
    {
      // TODO (depends on tags)
    }
  }
}

inline void populate_cull_workers(int sid, CullingState *cull, uint32_t &flags, uint32_t render_tags)
{
  flags |= SYS_VISIBLE;
  for (uint32_t tag : cull->renderTags)
  {
    if (render_tags & (1 << tag))
      cull->workers[tag].push_back(sid);
  }
}

void update_culling_state(Context &ctx, CullingState *cull, const Frustum &frustum, const mat44f *glob_tm, const Point3 &view_pos,
  Occlusion *(*occlusion_sync_wait_f)())
{
  TIME_D3D_PROFILE(dafx_update_culling_state);

  for (int i = 0; i < cull->workers.size(); ++i)
    cull->workers[i].clear();

  if (check_nan(view_pos.x) || check_nan(view_pos.y) || check_nan(view_pos.z))
  {
    logerr("dafx: NaN in view pos");
    return;
  }

  uint32_t targetRenderTagsMask = cull->renderTagsMask;
  uint32_t validationFlags = SYS_VALID | SYS_ENABLED | SYS_RENDERABLE | SYS_RENDER_REQ;

  dag::Vector<int, framemem_allocator> visibleWorkers;
  bool doTest = !(ctx.debugFlags & DEBUG_DISABLE_CULLING);
  bool useOcclusion = (occlusion_sync_wait_f != nullptr) && doTest && !(ctx.debugFlags & DEBUG_DISABLE_OCCLUSION);

  mat44f globTm;
  vec4f minW = v_splat_x(v_set_x(1e-7));
  vec4f maxR = v_splat_x(v_set_x(eastl::numeric_limits<float>::max()));
  eastl::array<vec4f, Config::max_render_tags> minR;
  bool allowDiscard = glob_tm && convar::allow_screen_area_discard && ctx.cfg.screen_area_cull_discard;
  float maxDiscardThreshold = 0;
  if (allowDiscard)
  {
    globTm = *glob_tm;
    for (int i = 0; i < cull->discardThreshold.size(); ++i)
    {
      float v = convar::screen_area_discard_threshold <= 0 ? cull->discardThreshold[i] : convar::screen_area_discard_threshold;
      v *= convar::screen_area_discard_threshold_mul;
      maxDiscardThreshold = max(maxDiscardThreshold, v);
      minR[i] = v_make_vec4f(v, 0, 0, 0);
    }
  }

  InstanceGroups &stream = ctx.instances.groups;
  for (int sid : ctx.allRenderWorkers)
  {
    uint32_t &flags = stream.get<INST_FLAGS>(sid);

    if ((flags & validationFlags) != validationFlags)
      continue;

    if ((stream.get<INST_VISIBILITY>(sid) & cull->visibilityMask) == 0)
      continue;

    if (stream.get<INST_ACTIVE_STATE>(sid).aliveCount == 0)
      continue;

    const uint32_t renderTags = stream.get<INST_RENDER_TAGS>(sid);
    const uint32_t activeTags = renderTags & targetRenderTagsMask;
    if (!activeTags)
      continue;

    const bbox3f &box = stream.get<INST_BBOX>(sid);
    bool test = doTest && (flags & SYS_CULL_FETCHED); // we need to show not-culled-yet instances
    if (test && (v_bbox3_is_empty(box) || !frustum.testBoxB(box.bmin, box.bmax)))
      continue;

    if (test && maxDiscardThreshold > 0 && !(flags & SYS_SKIP_SCREEN_PROJ_CULL_DISCARD))
    {
      vec4f bmin = v_mat44_mul_vec3p(globTm, box.bmin);
      vec4f bmax = v_mat44_mul_vec3p(globTm, box.bmax);

      vec4f bminW = v_max(v_splat_w(bmin), minW);
      vec4f bmaxW = v_max(v_splat_w(bmax), minW);

      bmin = v_div(bmin, bminW);
      bmax = v_div(bmax, bmaxW);

      vec4f rad = v_hmax3(v_abs(v_sub(bmax, bmin)));
      vec4f r = maxR;
      for (int t = __bsf_unsafe(activeTags), te = __bsr_unsafe(activeTags); t <= te; ++t)
      {
        uint32_t tag = 1 << t;
        if (tag & activeTags) // we dont want to include non-active tags, since they minR will be 0
          r = v_min(r, minR[t]);
      }

      vec4f vcmp = v_splat_x(v_cmp_gt(rad, r));
#if _TARGET_SIMD_SSE >= 4
      if (v_test_all_bits_zeros(vcmp))
        continue;
#else
      // SSE2 is producing false negative results in WT, problem is probably with ether _mm_cmpneq_ps or _mm_movemask_ps
      // so we fallback to compare results directly
      const uint32_t nullMask = 0;
      DECLSPEC_ALIGN(16) Point4 p4;
      v_st(&p4.x, vcmp);
      if (memcmp(&p4.x, &nullMask, sizeof(float)) == 0)
        continue;
#endif
    }

    visibleWorkers.push_back(sid);
  }

  // we try to invoke occlision as late as possible, so it can have more time to finish async job
  Occlusion *occlusion = useOcclusion ? occlusion_sync_wait_f() : nullptr;
  if (occlusion)
  {
    for (int sid : visibleWorkers)
    {
      const bbox3f &box = stream.get<INST_BBOX>(sid);
      uint32_t &flags = stream.get<INST_FLAGS>(sid);
      if (!(flags & SYS_CULL_FETCHED) || occlusion->isVisibleBox(box.bmin, box.bmax))
        populate_cull_workers(sid, cull, flags, stream.cget<INST_RENDER_TAGS>(sid));
    }
  }
  else
  {
    for (int sid : visibleWorkers)
      populate_cull_workers(sid, cull, stream.get<INST_FLAGS>(sid), stream.cget<INST_RENDER_TAGS>(sid));
  }

  auto &remapTags = ctx.cfg.vrs_enabled ? cull->vrsRemapTags : cull->remapTags;
  for (int t = 0, te = cull->renderTags.size(); t < te; ++t)
  {
    int tag = cull->renderTags[t];
    int remap = remapTags[tag];
    if (tag != remap)
    {
      if (remap >= 0)
        cull->workers[remap].insert(cull->workers[remap].end(), cull->workers[tag].begin(), cull->workers[tag].end());

      cull->workers[tag].clear();
    }
  }

  if (!(ctx.debugFlags & DEBUG_DISABLE_SORTING))
    sort_culling_states(ctx, *cull, view_pos);

  G_UNREFERENCED(validationFlags);
}

void update_culling_state(ContextId cid, CullingId cull_id, const Frustum &frustum, const mat44f &glob_tm, const Point3 &view_pos,
  Occlusion *(*occlusion_sync_wait_f)())
{
  GET_CTX();
  CullingState *cull = ctx.cullingStates.get(cull_id);
  update_culling_state(ctx, cull, frustum, &glob_tm, view_pos, occlusion_sync_wait_f);
}


void update_culling_state(ContextId cid, CullingId cull_id, const Frustum &frustum, const Point3 &view_pos,
  Occlusion *(*occlusion_sync_wait_f)())
{
  GET_CTX();
  CullingState *cull = ctx.cullingStates.get(cull_id);
  update_culling_state(ctx, cull, frustum, nullptr, view_pos, occlusion_sync_wait_f);
}

int fetch_culling_state_visible_count(ContextId cid, CullingId cull_id, const eastl::string &tag_name)
{
  GET_CTX_RET(0);

  CullingState *cull = ctx.cullingStates.get(cull_id);
  G_ASSERT_RETURN(cull, 0);

  uint32_t tag = get_render_tag(ctx.shaders, tag_name, Config::max_render_tags);

  if (tag >= Config::max_render_tags)
    return 0;

  return cull->workers[tag].size();
}

template <class UnaryPredicate>
void partition_workers_if(ContextId cid, CullingId cull_id_from, CullingId cull_id_to, const eastl::vector<eastl::string> &tags,
  UnaryPredicate p)
{
  TIME_D3D_PROFILE(dafx_partition);
  GET_CTX();

  CullingState *cull_from = ctx.cullingStates.get(cull_id_from);
  G_ASSERT_RETURN(cull_from, );

  CullingState *cull_to = ctx.cullingStates.get(cull_id_to);
  G_ASSERT_RETURN(cull_to && cull_to->isProxy, );

  uint32_t tags_mask = 0;
  for (const auto &tag_name : tags)
  {
    uint32_t tag = get_render_tag(ctx.shaders, tag_name, Config::max_render_tags);
    G_ASSERT_RETURN(tag < Config::max_render_tags, );

    tags_mask |= 1 << tag;
  }

  cull_to->sortings = cull_from->sortings;

  // copy tags
  cull_to->remapTags = cull_from->remapTags;
  cull_to->renderTagsMask = cull_from->renderTagsMask & tags_mask;
  cull_to->renderTags.clear();
  for (int tag : cull_from->renderTags)
    if (cull_to->renderTagsMask & (1 << tag))
      cull_to->renderTags.push_back(tag);

  // clear_culling_state
  for (int i = 0; i < cull_to->workers.size(); ++i)
    cull_to->workers[i].clear();

  // partition
  for (int t = 0, te = cull_to->renderTags.size(); t < te; ++t)
  {
    int tag = cull_to->renderTags[t];

    eastl::copy_if(cull_from->workers[tag].begin(), cull_from->workers[tag].end(), eastl::back_inserter(cull_to->workers[tag]), p);
    eastl::erase_if(cull_from->workers[tag], p);
  }
}

template <bool insideSphere>
void partition_workers_if_x_sphere(ContextId cid, CullingId cull_id_from, CullingId cull_id_to,
  const eastl::vector<eastl::string> &tags, Point4 sphere)
{
  GET_CTX();

  const vec4f v_center = v_ldu(&sphere.x);
  const vec4f v_radius_sq = v_set_x(sphere.w * sphere.w);
  partition_workers_if(cid, cull_id_from, cull_id_to, tags, [&](int sid) -> bool {
    vec4f v_pos = v_ldu(&ctx.instances.groups.get<INST_POSITION>(sid).x);
    vec4f v_distance_sq = v_length3_sq_x(v_sub(v_pos, v_center));

    if constexpr (insideSphere)
      return v_test_vec_x_le(v_distance_sq, v_radius_sq);
    else
      return v_test_vec_x_ge(v_distance_sq, v_radius_sq);
  });
}

void partition_workers_if_outside_sphere(ContextId cid, CullingId cull_id_from, CullingId cull_id_to,
  const eastl::vector<eastl::string> &tags, Point4 sphere)
{
  partition_workers_if_x_sphere<false>(cid, cull_id_from, cull_id_to, tags, sphere);
}

void partition_workers_if_inside_sphere(ContextId cid, CullingId cull_id_from, CullingId cull_id_to,
  const eastl::vector<eastl::string> &tags, Point4 sphere)
{
  partition_workers_if_x_sphere<true>(cid, cull_id_from, cull_id_to, tags, sphere);
}

} // namespace dafx
