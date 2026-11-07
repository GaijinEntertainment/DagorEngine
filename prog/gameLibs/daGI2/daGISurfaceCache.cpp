// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_statDrv.h>
#include <drv/3d/dag_rwResource.h>
#include <3d/dag_resourceTags.h>
#include <3d/dag_lockSbuffer.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <util/dag_finally.h>
#include <math/dag_adjpow2.h>
#include <daSDF/worldSDF.h>
#include "daGISurfaceCache.h"
#include "shaders/voxelLitScene/dagi_surface_cache_consts.hlsli"

CONSOLE_BOOL_VAL("gi", gi_surface_cache_stats, false);
CONSOLE_INT_VAL("gi", gi_surface_cache_debug, 0, 0, 3); // 1 cells, 2 voxels, 3 cell else voxels; green unknown

#define GLOBAL_VARS_LIST              \
  VAR(dagi_surface_cache_params)      \
  VAR(dagi_surface_cache_cell)        \
  VAR(dagi_surface_cache_center)      \
  VAR(dagi_surface_cache_voxels_res)  \
  VAR(dagi_surface_cache_feed_pixels) \
  VAR(dagi_surface_cache_prev_center) \
  VAR(dagi_surface_cache_debug_type)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

DaGISurfaceCache::~DaGISurfaceCache() { ShaderGlobal::set_int4(dagi_surface_cache_paramsVarId, 0, 0, 0, 0); }

// negative entries: count failed claims
void DaGISurfaceCache::setParams()
{
  ShaderGlobal::set_int4(dagi_surface_cache_paramsVarId, statsFramesLeft ? -int(entries) : int(entries), frame, levels, refreshAge);
}

void DaGISurfaceCache::barrier(ResourceBarrier rb)
{
  if (dagi_surface_cache)
    d3d::resource_barrier({dagi_surface_cache.getBuf(), rb});
}

bool DaGISurfaceCache::init(const DaGISettings::SurfaceCache &s, const WorldSDF &sdf, uint32_t sdf_w, uint32_t sdf_h)
{
  const float sdfVoxel0 = sdf.getVoxelSize(0), voxelCell = s.voxelScale * sdfVoxel0;
  const uint32_t clips = sdf.getClipsCount();
  ShaderGlobal::set_float4(dagi_surface_cache_cellVarId, s.cellSize, voxelCell, 1.f / (0.5f * sdfVoxel0 * sdf_w),
    1.f / (0.5f * sdfVoxel0 * sdf_h));
  // a voxel is a coarse cell; the box around the view holds the clip box wherever it lags within its move threshold,
  // so a voxel that the clear filled stays in the box while the clip holds it
  const uint32_t voxelsW = 2 * uint32_t(ceilf((0.5f * sdf_w + sdf.getTexelMoveThresholdXZ()) / s.voxelScale)) + 1,
                 voxelsH = 2 * uint32_t(ceilf((0.5f * sdf_h + sdf.getTexelMoveThresholdAlt()) / s.voxelScale)) + 1;
  // a voxel's byte offset is a uint: voxels that would not fit stay off
  const uint32_t firstVoxelClip = min<uint32_t>(s.voxelsFirstClip, clips);
  const uint32_t voxelsFirst = uint64_t(voxelsW) * voxelsW * voxelsH * (clips - firstVoxelClip) < (1u << 28) ? firstVoxelClip : clips;
  const uint32_t n = s.entries, voxelsCount = voxelsW * voxelsW * voxelsH * (clips - voxelsFirst);
  // the remean dither fills at most half the list
  const uint32_t remeanFrames = max<uint32_t>(s.voxelRemeanFrames,
    get_bigger_pow2((voxelsCount + DAGI_SC_VOXELS_LIST_CAPACITY / 2 - 1) / (DAGI_SC_VOXELS_LIST_CAPACITY / 2)));
  ShaderGlobal::set_int4(dagi_surface_cache_voxels_resVarId, voxelsW, voxelsH, voxelsFirst, remeanFrames);
  const IPoint3 voxelsRes(voxelsW, voxelsH, voxelsFirst);
  if (n != entries || clips != levels || s.cellSize != cell0 || voxelCell != voxelCell0 || voxelsRes != voxelsRes0)
    cleared = false;
  cell0 = s.cellSize;
  voxelCell0 = voxelCell;
  voxelsRes0 = voxelsRes;
  levels = clips;
  if (n != entries || voxelsCount != voxels)
  {
    entries = n;
    voxels = voxelsCount;
    dagi_surface_cache.close();
    // the table of 4-uint entries, an entry that counts the claims that found no slot old enough to evict,
    // the voxels, their marks, the voxels to remean
    dagi_surface_cache = dag::create_sbuffer(sizeof(uint32_t),
      (entries + 1) * 4 + voxels + (voxels + 31) / 32 + min<uint32_t>(voxels, DAGI_SC_VOXELS_LIST_CAPACITY), SBCF_UA_SR_BYTE_ADDRESS,
      0, "dagi_surface_cache", RESTAG_DAGI2);
  }
  if (!dagi_surface_cache_requests)
    dagi_surface_cache_requests = dag::create_sbuffer(sizeof(uint32_t), DAGI_SC_REQUESTS_SIZE / sizeof(uint32_t),
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "dagi_surface_cache_requests", RESTAG_DAGI2);
  const bool clear = !cleared && dagi_surface_cache;
  if (clear)
  {
    d3d::zero_rwbufi(dagi_surface_cache.getBuf());
    barrier(RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL);
    d3d::zero_rwbufi(dagi_surface_cache_requests.getBuf());
    d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    cleared = true;
    frame = statsFails = 0;
    statsFramesLeft = statsFramesLeft ? DAGI_SC_EVICT_AGE : 0;
  }
  setParams();
  if (!from_gbuf_cs)
    from_gbuf_cs.reset(new_compute_shader("dagi_surface_cache_from_gbuf_cs"));
  if (!relight_cs)
    relight_cs.reset(new_compute_shader("dagi_surface_cache_relight_cs"));
  if (!voxels_cs)
    voxels_cs.reset(new_compute_shader("dagi_surface_cache_voxels_cs"));
  if (!voxels_update_cs)
    voxels_update_cs.reset(new_compute_shader("dagi_surface_cache_voxels_update_cs"));
  return clear;
}

void DaGISurfaceCache::beginSdfUpdate(const Point3 &center, bool store_light)
{
  ShaderGlobal::set_float4(dagi_surface_cache_centerVarId, center.x, center.y, center.z, store_light ? 1 : 0);
  barrier(RB_RW_UAV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL);
  meanVoxels();
  prevCenter = center;
  voxelsMeaned = true;
}

void DaGISurfaceCache::updateVoxels()
{
  if (!voxelsMeaned)
  {
    barrier(RB_RW_UAV | RB_STAGE_COMPUTE);
    meanVoxels();
    endSdfUpdate();
  }
  voxelsMeaned = false;
}

void DaGISurfaceCache::meanVoxels()
{
  if (!voxels || !voxels_cs || !voxels_update_cs)
    return;
  TIME_D3D_PROFILE(sc_voxels);
  ShaderGlobal::set_float4(dagi_surface_cache_prev_centerVarId, prevCenter);
  const uint32_t words = (voxels + 31) / 32;
  voxels_cs->dispatchThreads(min<uint32_t>(words, DAGI_SC_VOXELS_ROW), (words + DAGI_SC_VOXELS_ROW - 1) / DAGI_SC_VOXELS_ROW, 1);
  barrier(RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE);
  d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  TIME_D3D_PROFILE(sc_voxels_mean);
  voxels_update_cs->dispatchThreads(min<uint32_t>(voxels, DAGI_SC_VOXELS_LIST_CAPACITY), 1, 1);
  barrier(RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL);
}

void DaGISurfaceCache::endSdfUpdate() { barrier(RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL); }

void DaGISurfaceCache::update(uint32_t gbuf_pixels, uint32_t refresh_age)
{
  if (!from_gbuf_cs || !dagi_surface_cache)
    return;
  DA_PROFILE_GPU;
  ++frame;
  refreshAge = refresh_age;
  if (gi_surface_cache_stats && !statsFramesLeft)
  {
    gi_surface_cache_stats.set(false);
    statsFramesLeft = DAGI_SC_EVICT_AGE;
  }
  setParams();
  if (gbuf_pixels)
  {
    TIME_D3D_PROFILE(sc_feed);
    barrier(RB_RW_UAV | RB_STAGE_COMPUTE);
    ShaderGlobal::set_int(dagi_surface_cache_feed_pixelsVarId, gbuf_pixels);
    from_gbuf_cs->dispatchThreads(gbuf_pixels, 1, 1);
    barrier(RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL);
  }
  if (statsFramesLeft && !--statsFramesLeft)
    logStats();
}

void DaGISurfaceCache::relight(uint32_t request_threads)
{
  if (!request_threads || !relight_cs || !dagi_surface_cache)
    return;
  TIME_D3D_PROFILE(sc_relight);
  d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  barrier(RB_RW_UAV | RB_STAGE_COMPUTE);
  relight_cs->dispatchThreads(min<uint32_t>(request_threads, DAGI_SC_REQUESTS_CAPACITY), 1, 1);
  barrier(RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL);
  // the patch reads the miss list through the probe traces' UAV
  d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
}

void DaGISurfaceCache::clearRequests()
{
  if (!dagi_surface_cache_requests)
    return;
  TIME_D3D_PROFILE(sc_clear);
  d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_RW_UAV | RB_STAGE_COMPUTE});
  d3d::zero_rwbufi(dagi_surface_cache_requests.getBuf());
  d3d::resource_barrier({dagi_surface_cache_requests.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
}

// a synchronous readback of the whole table, for gi.gi_surface_cache_stats only
void DaGISurfaceCache::logStats()
{
  const uint32_t size = (entries + 1) * sizeof(uint32_t) * 4;
  UniqueBuf readback = dag::create_sbuffer(sizeof(uint32_t), size / sizeof(uint32_t), SBCF_USAGE_READ_BACK, 0,
    "dagi_surface_cache_readback", RESTAG_DAGI2);
  d3d::resource_barrier({dagi_surface_cache.getBuf(), RB_RO_COPY_SOURCE | RB_SOURCE_STAGE_COMPUTE});
  FINALLY([&] { barrier(RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL); });
  if (!readback || !dagi_surface_cache.getBuf()->copyTo(readback.getBuf(), 0, 0, size))
    return;
  auto e = lock_sbuffer<const uint32_t>(readback.getBuf(), 0, size / sizeof(uint32_t), VBLOCK_READONLY);
  if (!e)
    return;
  uint32_t empty = 0, recent = 0;
  for (uint32_t i = 0; i < entries; ++i)
    if (!e[i * 4])
      ++empty;
    else if (frame - e[i * 4 + 1] < DAGI_SC_EVICT_AGE)
      ++recent;
  const uint32_t fails = e[entries * 4];
  console::print_d("daSharc: %u entries, %.1f%% empty, %.1f%% stored in the last %d updates (not evictable); "
                   "%u claims in the last %d updates found no slot old enough to evict",
    entries, 100.f * empty / entries, 100.f * recent / entries, DAGI_SC_EVICT_AGE, fails - statsFails, DAGI_SC_EVICT_AGE);
  statsFails = fails;
}

void DaGISurfaceCache::debugRender()
{
  if (!gi_surface_cache_debug)
    return;
  if (!dagi_world_scene_surface_cache_debug.getElem())
    dagi_world_scene_surface_cache_debug.init("dagi_world_scene_surface_cache_debug");
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_surface_cache_debug_typeVarId, gi_surface_cache_debug);
  dagi_world_scene_surface_cache_debug.render();
}
