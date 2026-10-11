// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "raytracedToroidalStaticShadow_api.h"
#include <core/rtssRasterPlan.h>
#include <render/staticShadowsCB.h>
#include <render/viewTransformData.h>
#include <3d/dag_resPtr.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_viewScissor.h>
#include <shaders/dag_computeShaders.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_shaderVar.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_adjpow2.h>
#include <vecmath/dag_vecMath.h>
#include <debug/dag_log.h>
#include <EASTL/algorithm.h>

#define GLOBAL_VARS_LIST   \
  VAR(rtss_globals)        \
  VAR(rtss_job_count)      \
  VAR(rtss_group_offset)   \
  VAR(rtss_build_augment)  \
  VAR(rtss_gather_sampler) \
  VAR(rtss_debug_mode)     \
  VAR(rtss_debug_level)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

using Settings = RaytracedToroidalStaticShadowSettings;
using GpuCounters = RaytracedToroidalStaticShadow::GpuCounters;

static constexpr int MAX_DISPATCH_GROUPS = 65535;
#define RTSS_COUNTER_AT(field, slot) (offsetof(GpuCounters, field) == (slot) * sizeof(uint32_t))
static_assert(RTSS_COUNTER_AT(lookups, RTSS_COUNTER_LOOKUPS) && RTSS_COUNTER_AT(noData, RTSS_COUNTER_NO_DATA) &&
              RTSS_COUNTER_AT(fallbackReleased, RTSS_COUNTER_NO_DATA + RTSS_TAP_RELEASED) &&
              RTSS_COUNTER_AT(fallbackBit, RTSS_COUNTER_NO_DATA + RTSS_TAP_BIT) &&
              RTSS_COUNTER_AT(fallbackDepth, RTSS_COUNTER_NO_DATA + RTSS_TAP_DEPTH) &&
              RTSS_COUNTER_AT(referencePixels, RTSS_COUNTER_REF_PIXELS) && RTSS_COUNTER_AT(referenceDiff, RTSS_COUNTER_REF_DIFF) &&
              sizeof(GpuCounters) == RTSS_COUNTERS * sizeof(uint32_t));
#undef RTSS_COUNTER_AT

template <typename T>
static bool upload_one_frame(UniqueBufWithShaderVar &buf, const dag::Vector<T> &data, int min_count, const char *name)
{
  const int count = int(data.size());
  if (!buf || buf->getNumElements() < count)
    buf = dag::buffers::create_one_frame_sr_structured(sizeof(T), eastl::max(min_count, get_bigger_pow2(count)), name, RESTAG_SHADOW);
  return buf && buf->updateData(0, data_size(data), data.data(), VBLOCK_DISCARD | VBLOCK_WRITEONLY);
}

struct RtssRasterDraw
{
  ViewTransformData transform;
  TMatrix view;
  TMatrix4 proj;
};

struct RaytracedToroidalStaticShadow::Impl
{
  Settings settings;
  eastl::unique_ptr<RtssScheduler> scheduler;
  RtssFramePlan *framePlan = nullptr;
  double timeSec = 0, sunStepSec = 0;
  BBox3 worldBox;
  bool hasWorldBox = false, slotsStale = false;
  Point3 sunDir = Point3(0, 1, 0), sunAngVel = Point3(0, 0, 0);
  shaders::UniqueOverrideStateId rasterState;

  UniqueTexWithShaderVar atlas;
  UniqueBufWithShaderVar slots, levels, jobs, groups;
  PostFxRenderer debugScreen, debugAtlas;
  UniqueBuf debugCounters;
  RingCPUBufferLock debugCountersReadback;
  GpuCounters gpuCounters = {};

  RtssRasterPlan rasterPlan;
  dag::Vector<RtssRasterDraw> rasterDraws;       // by cull group
  dag::Vector<uint8_t> rasterReady, rasterEmpty; // by plan job index
  dag::Vector<uint32_t> skipped, provisional, rasterTiles, buildGroups, sortedGroups;
  UniqueTexWithShaderVar rasterScratch;
  UniqueBufWithShaderVar rasterTilesBuf;
  IPoint2 rasterScratchSize = IPoint2(0, 0);
  ComputeShader resolveCs = ComputeShader("rtss_resolve_cs");

  void createResources();
  // with lookup false the shaders find no level, so they read no slot that render() did not write
  void setGlobals(bool lookup);
  // the scheduler dropped its plan: render() must not build it
  void dropPlan()
  {
    framePlan = nullptr;
    rasterDraws.clear();
  }
  void planRaster();
  RtssRaster raster(const RtssJob &job) const { return settings.recipes[job.levelSlot >> 16].raster; }
  bool renderRaster(const RtssFramePlan &plan, IStaticShadowsCB &cb, IRtssTraceCB *trace_cb);
  bool traceBatch(const RtssFramePlan &plan, IRtssTraceCB *cb, bool augment);
};

void RaytracedToroidalStaticShadow::Impl::createResources()
{
  const RtssSchedulerSettings &s = scheduler->getSettings();
  const unsigned usage = d3d::get_texformat_usage(TEXFMT_L16, D3DResourceType::ARRTEX);
  if (!(usage & d3d::USAGE_UNORDERED))
    logerr("rtss: the driver has no UAV support for L16 array textures");
  ShaderGlobal::set_sampler(rtss_gather_samplerVarId, d3d::request_sampler({})); // a gather reads mip 0 and no filter
  const int side = s.windowPages * s.slotTexels, slotCount = s.levels * s.windowPages * s.windowPages;
  atlas = dag::create_array_tex(side, side, s.levels, TEXFMT_L16 | TEXCF_UNORDERED, 1, "rtss_atlas", RESTAG_SHADOW);
  slots = dag::buffers::create_persistent_sr_byte_address(slotCount * sizeof(RtssSlot) / 4, "rtss_slots", d3d::buffers::Init::Zero,
    RESTAG_SHADOW);
  levels = dag::buffers::create_persistent_sr_byte_address(s.levels * sizeof(RtssLevel) / 4, "rtss_levels", d3d::buffers::Init::Zero,
    RESTAG_SHADOW);
  shaders::OverrideState zClamp; // constraint: no depth bias, the lookup applies it
  zClamp.set(shaders::OverrideState::Z_CLAMP_ENABLED | shaders::OverrideState::Z_FUNC);
  zClamp.zFunc = CMPF_LESSEQUAL;
  zClamp.colorWr = 0;
  rasterState = shaders::overrides::create(zClamp);
  setGlobals(false);
}

void RaytracedToroidalStaticShadow::Impl::setGlobals(bool lookup)
{
  IPoint4 paramsI;
  Point4 globals[5]; // constraint: rtss_common.hlsli order: rtss_params_i, rtss_params, rtss_origin, rtss_bias, rtss_atlas_texel
  scheduler->globals(paramsI, globals[1], globals[2]);
  globals[0] = Point4(lookup ? paramsI.x : 0, paramsI.y, paramsI.z, paramsI.w);
  globals[3] = Point4(settings.biasTexels, settings.normalBiasTexels, scheduler->centerY(), float(scheduler->texel(0)));
  globals[4] = Point4(1.f / float(paramsI.y * paramsI.z), 0, 0, 0);
  ShaderGlobal::set_float4_array(rtss_globalsVarId, globals, countof(globals));
}

RaytracedToroidalStaticShadow::RaytracedToroidalStaticShadow(const Settings &settings) : impl(eastl::make_unique<Impl>())
{
  setSettings(settings);
}

RaytracedToroidalStaticShadow::~RaytracedToroidalStaticShadow() { impl->setGlobals(false); }

void RaytracedToroidalStaticShadow::setSettings(const Settings &settings)
{
  Impl &d = *impl;
  if (!d.scheduler || !(static_cast<const RtssSchedulerLayout &>(settings) == d.settings))
  {
    d.settings = settings;
    d.scheduler = eastl::make_unique<RtssScheduler>(settings);
    d.dropPlan();
    if (d.hasWorldBox)
      d.scheduler->setWorldBox(d.worldBox);
    d.scheduler->setSun(d.sunDir, d.sunAngVel);
    d.createResources();
    return;
  }
  d.settings = settings;
  d.scheduler->setBudget(settings);
  if (d.scheduler->setRecipes(settings))
    d.dropPlan();
}

const Settings &RaytracedToroidalStaticShadow::getSettings() const { return impl->settings; }

void RaytracedToroidalStaticShadow::setWorldBox(const BBox3 &casters_box)
{
  if (!impl->hasWorldBox || !(casters_box == impl->worldBox))
    impl->dropPlan();
  impl->worldBox = casters_box;
  impl->hasWorldBox = true;
  impl->scheduler->setWorldBox(casters_box);
}

void RaytracedToroidalStaticShadow::setSun(const Point3 &dir_to_sun)
{
  Impl &d = *impl;
  const Point3 dir = normalize(dir_to_sun), axis = d.sunDir % dir;
  const float angle = atan2f(length(axis), d.sunDir * dir), stepSec = float(d.timeSec - d.sunStepSec);
  d.sunAngVel =
    angle > 0 && stepSec > 0 && angle <= DegToRad(d.settings.sunJumpDeg) ? normalize(axis) * (angle / stepSec) : Point3(0, 0, 0);
  if (angle > 0)
    d.sunStepSec = d.timeSec;
  d.sunDir = dir;
  d.scheduler->setSun(dir, d.sunAngVel);
}

void RaytracedToroidalStaticShadow::invalidate()
{
  impl->dropPlan();
  impl->scheduler->invalidate();
}

void RaytracedToroidalStaticShadow::invalidateBoxes(const BBox3 *boxes, uint32_t cnt, uint32_t level_mask)
{
  impl->dropPlan();
  impl->scheduler->invalidateBoxes(boxes, cnt, level_mask);
}

void RaytracedToroidalStaticShadow::update(const DPoint3 &camera_pos, const Point3 &camera_velocity, float dt)
{
  Impl &d = *impl;
  d.timeSec += eastl::max(dt, 0.f);
  d.framePlan = &d.scheduler->plan(camera_pos, camera_velocity, d.timeSec);
  d.planRaster();
}

float RaytracedToroidalStaticShadow::getDistance(uint32_t level) const
{
  return float(impl->scheduler->extent(int(eastl::min(level, uint32_t(impl->scheduler->getSettings().levels - 1)))));
}

void RaytracedToroidalStaticShadow::Impl::planRaster()
{
  rtss_raster_plan(make_span(framePlan->jobs), rtss_raster_levels(*scheduler), *scheduler, rasterPlan);
  rasterDraws.resize(rasterPlan.cullGroups.size());
  for (int g = 0; g < (int)rasterPlan.cullGroups.size(); ++g)
  {
    const RtssRasterGroup &group = rasterPlan.cullGroups[g];
    RtssRasterDraw &rd = rasterDraws[g];
    const RtssRasterRect rect = rtss_raster_group_rect(*scheduler, make_span_const(framePlan->jobs), rasterPlan, group);
    rtss_raster_matrices(*scheduler, group.level, group.bit, rect, group.transposed, rd.view, rd.proj);
    const TMatrix4 globtm = TMatrix4(rd.view) * rd.proj;
    mat44f tm;
    v_mat44_make_from_44cu(tm, &globtm._11);
    rd.transform = {tm, group.origin.x, group.origin.y, group.size.x, group.size.y, 0.f, 1.f, {.cascade = group.level}};
  }
}

dag::ConstSpan<RtssSlot> RaytracedToroidalStaticShadow::getSlotTable() const { return impl->scheduler->slotTable(); }
int RaytracedToroidalStaticShadow::getRasterRegionCount() const { return impl->rasterDraws.size(); }
const ViewTransformData &RaytracedToroidalStaticShadow::getRasterRegion(int region) const
{
  return impl->rasterDraws[region].transform;
}

bool RtssTraceBatch::dispatch(const ComputeShader &cs, uint32_t level_mask) const
{
  bool built = true;
  for (int k = 0; k < RTSS_MAX_LEVELS && built; ++k)
  {
    if (!(level_mask & (1u << k)))
      continue;
    int end = k + 1;
    while (end < RTSS_MAX_LEVELS && (level_mask & (1u << end)))
      ++end;
    if (levelGroups[k] < levelGroups[end] && !cs)
    {
      LOGERR_ONCE("rtss: a tracer has groups and no compute shader");
      return false;
    }
    for (uint32_t first = levelGroups[k]; first < levelGroups[end] && built; first += MAX_DISPATCH_GROUPS)
    {
      ShaderGlobal::set_int(rtss_group_offsetVarId, first);
      d3d::resource_barrier({atlas, RB_NONE, 0, 0}); // the dispatches of a frame write disjoint slots, so they may overlap
      built &= cs.dispatchGroups(eastl::min<uint32_t>(levelGroups[end] - first, MAX_DISPATCH_GROUPS), 1, 1);
    }
    k = end;
  }
  return built;
}

bool RaytracedToroidalStaticShadow::Impl::traceBatch(const RtssFramePlan &plan, IRtssTraceCB *cb, bool augment)
{
  if (buildGroups.empty())
    return true;
  TIME_D3D_PROFILE(rtss_trace);
  auto level = [&](uint32_t entry) { return plan.jobs[entry >> 16].levelSlot >> 16; };
  RtssTraceBatch batch = {{}, atlas.getArrayTex()};
  for (uint32_t entry : buildGroups)
    ++batch.levelGroups[level(entry) + 1];
  for (int k = 0; k < RTSS_MAX_LEVELS; ++k)
    batch.levelGroups[k + 1] += batch.levelGroups[k];
  uint32_t next[RTSS_MAX_LEVELS];
  eastl::copy_n(batch.levelGroups, RTSS_MAX_LEVELS, next);
  sortedGroups.resize(buildGroups.size());
  for (uint32_t entry : buildGroups)
    sortedGroups[next[level(entry)]++] = entry;
  if (!upload_one_frame(groups, sortedGroups, 16384, "rtss_groups"))
    return false;
  ShaderGlobal::set_int(rtss_build_augmentVarId, augment ? 1 : 0);
  return cb->traceJobs(batch);
}

bool RaytracedToroidalStaticShadow::Impl::renderRaster(const RtssFramePlan &plan, IStaticShadowsCB &cb, IRtssTraceCB *trace_cb)
{
  TIME_D3D_PROFILE(rtss_raster);
  const int S = scheduler->getSettings().slotTexels;
  const IPoint2 size = rasterPlan.size;
  rasterReady.resize(plan.jobs.size());
  for (int g = 0; g < (int)rasterDraws.size(); ++g)
  {
    const RtssRasterGroup &group = rasterPlan.cullGroups[g];
    const bool ready = cb.readyToRenderRegionWithFinalQuality(rasterDraws[g].transform, g);
    for (int r = group.first; r < group.first + group.count; ++r)
      rasterReady[rasterPlan.order[r]] = ready;
  }
  rtss_raster_skips(rasterPlan, make_span_const(plan.jobClass), make_span_const(rasterReady), skipped, provisional);
  auto isSkipped = [&](int r) { return eastl::binary_search(skipped.begin(), skipped.end(), rasterPlan.order[r]); };

  if (size != rasterScratchSize)
  {
    // a group has the depth range of its slots, so D16 keeps all that the L16 atlas keeps
    const uint32_t fmt = d3d::get_driver_desc().issues.hasRenderPassClearDataRace ? TEXFMT_DEPTH32_S8 : TEXFMT_DEPTH16;
    rasterScratch = dag::create_tex(nullptr, size.x, size.y, fmt | TEXCF_RTARGET, 1, "rtss_raster_scratch", RESTAG_SHADOW);
    rasterScratchSize = rasterScratch.getTex2D() ? size : IPoint2(0, 0);
  }
  if (!rasterScratch.getTex2D())
    return false; // render() resets, and the next plan tries the scratch again
  const float reach = scheduler->getSettings().maxCasterReach;
  const float minHt = hasWorldBox ? worldBox[0].y : float(scheduler->cameraPos().y) - reach;
  const float maxHt = hasWorldBox ? worldBox[1].y : float(scheduler->cameraPos().y) + reach;

  SCOPE_RENDER_TARGET;
  SCOPE_VIEW_PROJ_MATRIX;
  shaders::overrides::set(rasterState.get());
  bool built = true;
  for (int b = 0, g = 0; built && b < rasterPlan.batches; ++b)
  {
    d3d::set_render_target({rasterScratch.getTex2D(), 0, 0}, DepthAccess::RW, {});
    d3d::setview(0, 0, size.x, size.y, 0, 1);
    d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
    rasterTiles.clear();
    buildGroups.clear();
    int key = -1;
    for (; g < (int)rasterDraws.size() && rasterPlan.cullGroups[g].batch == b; ++g)
    {
      const RtssRasterGroup &group = rasterPlan.cullGroups[g];
      const RtssRasterDraw &rd = rasterDraws[g];
      int live = 0;
      for (int r = group.first; r < group.first + group.count; ++r)
        live += !isSkipped(r);
      if (!live)
        continue;
      // constraint: the scheduler gives the whole-slot jobs of one level and bit one depth range
      const RtssJob &first = plan.jobs[rasterPlan.order[group.first]];
      if (key != group.level * 2 + group.bit)
      {
        if (key >= 0)
          cb.endRenderStaticShadow();
        key = group.level * 2 + group.bit;
        d3d::settm(TM_VIEW, rd.view);
        cb.startRenderStaticShadow(rd.view, Point2(first.zStart, first.zEnd), minHt, maxHt);
      }
      d3d::settm(TM_PROJ, &rd.proj);
      d3d::setview(rd.transform.x, rd.transform.y, rd.transform.w, rd.transform.h, 0, 1);
      // an empty depth range has a zero depth scale, which would draw every caster at depth 0
      const IStaticShadowsCB::RasterizedResult drawn = first.zEnd > first.zStart
                                                         ? cb.renderStaticShadowDepth(rd.transform.globtm, rd.transform, g)
                                                         : IStaticShadowsCB::RasterizedResult::NothingRendered;
      for (int r = group.first; r < group.first + group.count; ++r)
      {
        const uint32_t jobIndex = rasterPlan.order[r];
        const RtssJob &job = plan.jobs[jobIndex];
        if (isSkipped(r))
          continue;
        if (raster(job) == RtssRaster::All)
          rasterTiles.push_back(jobIndex);
        else if (drawn == IStaticShadowsCB::RasterizedResult::NothingRendered)
          rasterEmpty[jobIndex] = 1;
        else
          for (uint32_t i = 0; i < (rtss_rect_area(job.rect) + RTSS_BUILD_GROUP - 1) / RTSS_BUILD_GROUP; ++i)
            buildGroups.push_back((jobIndex << 16) | i);
      }
    }
    if (key >= 0)
      cb.endRenderStaticShadow();
    d3d::resource_barrier({rasterScratch.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
    if (!rasterTiles.empty())
      built &=
        upload_one_frame(rasterTilesBuf, rasterTiles, 64, "rtss_raster_tiles") && resolveCs.dispatchThreads(S, S, rasterTiles.size());
    built &= traceBatch(plan, trace_cb, true);
    d3d::resource_barrier({rasterScratch.getTex2D(), RB_RW_RENDER_TARGET | RB_STAGE_PIXEL, 0, 0});
  }
  shaders::overrides::reset();
  return built;
}

void RaytracedToroidalStaticShadow::render(IStaticShadowsCB *raster_cb, IRtssTraceCB *trace_cb)
{
  Impl &d = *impl;
  const bool raster = !d.rasterDraws.empty();
  if (!d.framePlan || !d.atlas.getArrayTex() || !d.slots || !d.levels || (raster && !d.resolveCs))
    return;
  const RtssFramePlan &plan = *d.framePlan;
  const bool traced =
    eastl::any_of(plan.jobs.begin(), plan.jobs.end(), [&](const RtssJob &j) { return d.raster(j) != RtssRaster::All; });
  if ((raster && !raster_cb) || (traced && !trace_cb))
  {
    LOGERR_ONCE("rtss: a raster level needs the raster_cb of render(), a traced level its trace_cb");
    return;
  }
  d.skipped.clear();
  d.provisional.clear();
  d.rasterEmpty.assign(plan.jobs.size(), 0);

  const dag::ConstSpan<RtssLevel> levelTable = d.scheduler->levelTable();
  bool built = d.levels->updateData(0, data_size(levelTable), levelTable.data(), VBLOCK_WRITEONLY);
  d.setGlobals(true);

  if (built && !plan.jobs.empty())
  {
    built = upload_one_frame(d.jobs, plan.jobs, 1024, "rtss_jobs");
    ShaderGlobal::set_int(rtss_job_countVarId, int(plan.jobs.size()));
    if (raster && built)
      built = d.renderRaster(plan, *raster_cb, trace_cb); // -V522 a raster plan returns above with no raster_cb
    d.buildGroups.clear();
    for (uint32_t entry : plan.groups)
    {
      const uint32_t job = entry >> 16;
      const RtssRaster r = d.raster(plan.jobs[job]);
      if (r == RtssRaster::None || (r == RtssRaster::Augment && d.rasterEmpty[job]))
        d.buildGroups.push_back(entry);
    }
    built = built && d.traceBatch(plan, trace_cb, false);
    d3d::resource_barrier({d.atlas.getArrayTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }
  d.framePlan = nullptr;
  if (built)
    d.scheduler->commit(make_span_const(d.skipped), make_span_const(d.provisional));
  else
  {
    LOGERR_ONCE("rtss: a build upload or dispatch failed (a missing shader variant?), the rtss is reset");
    d.scheduler->reset(); // the dispatches that ran wrote texels under the slots of the old table
  }
  if (!built || d.slotsStale || d.scheduler->slotTableChanged())
  {
    const dag::ConstSpan<RtssSlot> slotTable = d.scheduler->slotTable();
    d.slotsStale = !d.slots->updateData(0, data_size(slotTable), slotTable.data(), VBLOCK_WRITEONLY);
  }
  if (d.slotsStale)
    d.setGlobals(false);
}

bool RaytracedToroidalStaticShadow::isReady(const BBox3 &box, uint32_t level) const
{
  const RtssScheduler &s = *impl->scheduler;
  return s.isReady(box, 1u << eastl::min(level, uint32_t(s.getSettings().levels - 1)));
}

void RaytracedToroidalStaticShadow::afterDeviceReset()
{
  impl->dropPlan();
  impl->scheduler->reset();
  impl->debugCountersReadback.reset();
  impl->createResources();
}

void RaytracedToroidalStaticShadow::getStats(Stats &out) const
{
  impl->scheduler->getStats(out);
  out.rasterJobs = impl->rasterDraws.empty() ? 0 : (int)impl->rasterPlan.order.size();
  out.rasterSkipped = (int)impl->skipped.size();
  out.rasterProvisional = (int)impl->provisional.size();
  out.rasterEmpty = (int)eastl::count(impl->rasterEmpty.begin(), impl->rasterEmpty.end(), 1);
  out.sunRateDegPerSec = RadToDeg(length(impl->sunAngVel));
  out.gpu = impl->gpuCounters;
}

void RaytracedToroidalStaticShadow::debugRenderScreen(int mode)
{
  Impl &d = *impl;
  if (!d.debugScreen.getElem())
    d.debugScreen.init("rtss_debug_screen");
  if (!d.debugCounters)
  {
    d.debugCounters = dag::buffers::create_ua_structured(sizeof(uint32_t), RTSS_COUNTERS, "rtss_debug_counters", RESTAG_SHADOW);
    d.debugCountersReadback.init(sizeof(uint32_t), RTSS_COUNTERS, 3, "rtss_debug_counters_readback", SBCF_UA_STRUCTURED_READBACK, 0,
      false);
  }
  if (!d.debugCounters || !d.debugScreen.getElem())
    return;
  int stride;
  uint32_t frame;
  if (const uint8_t *data = d.debugCountersReadback.lock(stride, frame, true))
  {
    memcpy(&d.gpuCounters, data, sizeof(d.gpuCounters));
    d.debugCountersReadback.unlock();
  }

  static const int countersUavNo = ShaderGlobal::get_slot_by_name("rtss_debug_counters_uav_no");
  ShaderGlobal::set_int(rtss_debug_modeVarId, mode);
  d3d::zero_rwbufi(d.debugCounters.getBuf());
  d3d::resource_barrier({d.debugCounters.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_PIXEL});
  d3d::set_rwbuffer(STAGE_PS, countersUavNo, d.debugCounters.getBuf());
  d.debugScreen.render();
  d3d::set_rwbuffer(STAGE_PS, countersUavNo, nullptr);
  if (Sbuffer *target = (Sbuffer *)d.debugCountersReadback.getNewTarget(frame))
  {
    d.debugCounters->copyTo(target, 0, 0, sizeof(GpuCounters));
    d.debugCountersReadback.startCPUCopy();
  }
}

void RaytracedToroidalStaticShadow::debugDrawAtlas(int level)
{
  if (!impl->debugAtlas.getElem())
    impl->debugAtlas.init("rtss_debug_atlas");
  int x, y, w, h;
  float minZ, maxZ;
  d3d::getview(x, y, w, h, minZ, maxZ);
  const int margin = 16, side = eastl::min(w, h) * 2 / 3;
  ShaderGlobal::set_int(rtss_debug_levelVarId, level);
  SCOPE_VIEWPORT;
  d3d::setview(x + w - side - margin, y + margin, side, side, 0, 1);
  impl->debugAtlas.render();
}
