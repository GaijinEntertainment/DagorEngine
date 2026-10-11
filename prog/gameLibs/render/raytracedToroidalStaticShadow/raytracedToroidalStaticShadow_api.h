// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <vecmath/dag_vecMathDecl.h>
#include <shaders/dag_computeShaders.h>
#include <EASTL/unique_ptr.h>
#include <render/viewTransformData.h>
#include "core/rtssScheduler.h"

class IStaticShadowsCB;

// the groups of one trace batch of render(), sorted by level
struct RtssTraceBatch
{
  uint32_t levelGroups[RTSS_MAX_LEVELS + 1]; // the groups of level k are [levelGroups[k], levelGroups[k + 1])
  BaseTexture *atlas;                        // the UAV that the tracer writes
  // dispatches cs over the groups of the levels of level_mask; true with no groups, false for an invalid cs or a failed dispatch
  bool dispatch(const ComputeShader &cs, uint32_t level_mask) const;
};

// render() calls traceJobs for each trace batch with groups, with the vars of rtss_trace_common.dshl set but rtss_group_offset,
// which dispatch() sets for each run; a tracer that dispatches its own cs sets it and gives atlas RB_NONE, as dispatch() does
class IRtssTraceCB
{
public:
  virtual bool traceJobs(const RtssTraceBatch &batch) = 0; // true only when the groups of all levels were dispatched
};

// one compute shader for the levels of levelMask; with a partial mask it is a part of a tracer whose parts cover all levels,
// and its traceJobs answers for its levels only. A project lists its tracer and passes rtss_trace_swrt_cs or rtss_trace_hwrt_cs
class RtssComputeTracer : public IRtssTraceCB
{
public:
  explicit RtssComputeTracer(const char *cs_name, uint32_t level_mask = ~0u) : levelMask(level_mask), cs(cs_name, true) {}
  bool traceJobs(const RtssTraceBatch &batch) override { return batch.dispatch(cs, levelMask); }
  uint32_t levelMask;

private:
  ComputeShader cs;
};

// recipes: how each level builds a page; the tracer callback of render() traces the None and Augment levels
struct RaytracedToroidalStaticShadowSettings : RtssSchedulerSettings
{
  float biasTexels = 1.f;       // depth bias, texels of the level
  float normalBiasTexels = 2.f; // normal offset, texels at the receiver distance, times 1 - NoL
  float sunJumpDeg = 1.f;       // a larger sun step is a jump, as a time skip, with no sun rate

  constexpr RaytracedToroidalStaticShadowSettings() :
    RtssSchedulerSettings{{.levels = 5, .windowPages = 8, .slotTexels = 128, .borderTexels = 3, .texel0 = 0.08f, .maxAnisotropy = 2.f},
      {.updatePagesPerFrame = 3, .validityBurstPagesPerFrame = 32, .batchFrames = 4}}
  {}
};

class RaytracedToroidalStaticShadow
{
public:
  explicit RaytracedToroidalStaticShadow(const RaytracedToroidalStaticShadowSettings &settings);
  ~RaytracedToroidalStaticShadow(); // a lookup after it finds no level and reads lit

  // budgets, costs and raster sizes apply from the next update(), biases from the next render(); a layout change rebuilds,
  // a raster change of a level invalidates that level; a project must invalidate a level whose tracer it changes:
  // invalidate(), or invalidateBoxes() with the world box and the bit of that level
  void setSettings(const RaytracedToroidalStaticShadowSettings &settings);
  const RaytracedToroidalStaticShadowSettings &getSettings() const; // as given; the scheduler clamps its own copy

  void setWorldBox(const BBox3 &casters_box); // a changed box invalidates all slots
  // a changed sun moves at its step over the update time since the last change, as a sun can change in steps of many frames
  void setSun(const Point3 &dir_to_sun);

  void invalidate();
  void invalidateBoxes(const BBox3 *boxes, uint32_t cnt, uint32_t level_mask = ~0u);

  // camera_pos becomes rtss_origin of the lookup, as world_view_pos; camera_velocity in m/s, above maxCameraSpeed a teleport; dt in s
  void update(const DPoint3 &camera_pos, const Point3 &camera_velocity, float dt);
  float getDistance(uint32_t level = ~0u) const; // the reach of the level from the camera; the coarsest by default and past the last
  // from update() to render(): the boxes that render() gives raster_cb, region r as its region argument, to cull them ahead
  int getRasterRegionCount() const;
  const ViewTransformData &getRasterRegion(int region) const;
  // debug: the slot table (rtss_common.hlsli) of the GPU after the next render(), entry (level * W + y) * W + x; a new layout frees it
  dag::ConstSpan<RtssSlot> getSlotTable() const;
  // builds the jobs that update() planned, writes the tables, sets the shader vars; a raster level needs raster_cb, which
  // draws a box of its pages in each call, and a traced level trace_cb; the pages of an Augment box that raster_cb reports
  // NothingRendered are traced alone. raster_cb gets the level as cascade, the index of the box in the frame as region
  // and the plain globtm of the box as culling_view_proj (not the classic twice-zfar one); raster_cb draws with the depth clamp
  // override state of the library, so it sets none of its own, and the caller of render() has none set
  void render(IStaticShadowsCB *raster_cb, IRtssTraceCB *trace_cb);

  // the level, clamped to the coarsest, has data over box if it covers it; DIRTY data counts, a page in a depth rebuild does not;
  // false while it sweeps fast or is not live. A lookup with the coarsest level ready reads no lit gap over box
  bool isReady(const BBox3 &box, uint32_t level = ~0u) const;
  void afterDeviceReset();

  // pixels of a debugRenderScreen pass that a later pass reads back, up to 3 frames later, all with the sun above the horizon only;
  // a fallback reads a coarser level than the selected one; reference: the lit pixels that mode 5 compared, and those that differ
  struct GpuCounters
  {
    uint32_t lookups, noData, fallbackReleased, fallbackBit, fallbackDepth, referencePixels, referenceDiff;
  };
  struct Stats : RtssSchedulerStats
  {
    int rasterJobs, rasterSkipped, rasterProvisional, rasterEmpty; // rasterJobs of the last update(), the others of the last render()
    float sunRateDegPerSec;                                        // of the last setSun()
    GpuCounters gpu;
  };
  void getStats(Stats &out) const;
  // from the gbuffer depth; 1: visibility, 2: level, 3: slot bit, 4: fallback, 5: the difference to the project reference
  void debugRenderScreen(int mode);
  void debugDrawAtlas(int level); // slot states and depth

private:
  struct Impl;
  eastl::unique_ptr<Impl> impl;
};
