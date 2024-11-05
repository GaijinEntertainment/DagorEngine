// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_staticTab.h>
#include <util/dag_convar.h>
#include <render/spheres_consts.hlsli>
#include <shaders/dag_DynamicShaderHelper.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/string.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <render/subtract_ibbox3.h>
#include "radianceGrid/dagi_rad_grid.hlsli"
#include "radianceGrid/dagi_irrad_grid.hlsli"
#include "radianceGrid.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

#define GLOBAL_VARS_LIST                      \
  VAR(dagi_rad_oct_resi)                      \
  VAR(dagi_rad_grid_clipmap_sizei)            \
  VAR(dagi_rad_grid_clipmap_sizei_np2)        \
  VAR(dagi_rad_grid_debug_type)               \
  VAR(dagi_rad_grid_temporal_size)            \
  VAR(dagi_rad_grid_temporal)                 \
  VAR(dagi_rad_grid_update_lt_coord)          \
  VAR(dagi_rad_grid_update_sz_coord)          \
  VAR(dagi_rad_grid_decode_xy)                \
  VAR(dagi_rad_grid_decode_z)                 \
  VAR(dagi_rad_grid_temporal_indirect)        \
  VAR(dagi_irrad_grid_debug_type)             \
  VAR(dagi_irrad_grid_update_lt_coord)        \
  VAR(dagi_irrad_grid_update_sz_coord)        \
  VAR(dagi_irradiance_grid_sph0)              \
  VAR(dagi_irradiance_grid_sph1)              \
  VAR(dagi_irradiance_grid_sph1_samplerstate) \
  VAR(dagi_irrad_grid_clipmap_sizei)          \
  VAR(dagi_irrad_grid_clipmap_sizei_np2)      \
  VAR(dagi_irrad_grid_atlas_decode)           \
  VAR(dagi_irrad_grid_inv_size)               \
  VAR(dagi_radiance_grid_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

CONSOLE_BOOL_VAL("gi", gi_irradiance_grid_invalidate, false);
CONSOLE_BOOL_VAL("gi", gi_radiance_grid_update_temporal, true);
CONSOLE_INT_VAL("gi", gi_irradiance_spatial_passes, 2, 0, 4);

static ShaderVariableInfo dagi_rad_grid_clipmap_lt_coordVarId[DAGI_MAX_RAD_GRID_CLIPS],
  dagi_irrad_grid_clipmap_lt_coordVarId[DAGI_MAX_IRRAD_GRID_CLIPS];

const uint32_t debug_flag = 0; // SBCF_USAGE_READ_BACK;

static void init_vars()
{
  if (dagi_rad_grid_clipmap_lt_coordVarId[0])
    return;
  eastl::string str;
  for (int i = 0; i < DAGI_MAX_RAD_GRID_CLIPS; ++i)
  {
    str.sprintf("dagi_rad_grid_clipmap_lt_coord_%d", i);
    dagi_rad_grid_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
  }
  for (int i = 0; i < DAGI_MAX_IRRAD_GRID_CLIPS; ++i)
  {
    str.sprintf("dagi_irrad_grid_clipmap_lt_coord_%d", i);
    dagi_irrad_grid_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
  }
}

void RadianceGrid::fixup_settings(uint8_t &w, uint8_t &d, uint8_t &clips, uint8_t &additional_iclips, float &irradianceProbeDetail)
{
  irradianceProbeDetail = max(1.f, irradianceProbeDetail);
  w = clamp<uint8_t>(w, 8, DAGI_MAX_RAD_GRID_RES);
  if (d)
    d = clamp<uint8_t>(d, 8, DAGI_MAX_RAD_GRID_RES);
  if constexpr (!DAGI_RAD_GRID_ALLOW_NON_POW2)
  {
    w = get_bigger_pow2(w);
    if (d)
      d = get_bigger_pow2(d);
  }
  clips = clamp<uint8_t>(clips, 1, DAGI_MAX_RAD_GRID_CLIPS);
  uint32_t iw = w * irradianceProbeDetail, id = d * irradianceProbeDetail;
  if constexpr (!DAGI_IRRAD_GRID_ALLOW_NON_POW2)
  {
    iw = get_bigger_pow2(w * irradianceProbeDetail);
    if (d)
      id = get_bigger_pow2(d * irradianceProbeDetail);
  }
  iw = clamp<uint16_t>(iw, 8, DAGI_MAX_IRRAD_GRID_RES);
  if (id)
    id = clamp<uint16_t>(id, 8, DAGI_MAX_IRRAD_GRID_RES);
  irradianceProbeDetail = float(iw) / w;
  additional_iclips = min<int16_t>(clips + additional_iclips, DAGI_MAX_IRRAD_GRID_CLIPS) - clips;
}

void RadianceGrid::initVars()
{
  if (dagi_radiance_grid_calc_temporal_cs)
    return;
  init_vars();
#define CS(a) a.reset(new_compute_shader(#a))
  CS(dagi_radiance_grid_calc_temporal_cs);
  CS(dagi_radiance_grid_select_temporal_cs);
  CS(dagi_radiance_grid_clear_temporal_cs);
  CS(dagi_radiance_grid_create_indirect_cs);
  CS(dagi_radiance_grid_toroidal_movement_cs);
  CS(dagi_radiance_grid_toroidal_movement_irradiance_cs);
  CS(dagi_radiance_grid_calc_temporal_irradiance_cs);

  CS(dagi_irradiance_grid_toroidal_movement_interpolate_cs);
  CS(dagi_irradiance_grid_toroidal_movement_trace_cs);
  CS(dagi_irradiance_grid_select_temporal_cs);
  CS(dagi_irradiance_grid_calc_temporal_cs);
  CS(dagi_irradiance_grid_spatial_filter_cs);
#undef CS

  {
    d3d::SamplerHandle sampler = d3d::request_sampler({});
    dagi_irradiance_grid_sph1_samplerstateVarId.set_sampler(sampler);
  }
}

static inline IPoint4 calc_clip_var(const IPoint3 &lt, float probeSize)
{
  return IPoint4(lt.x, lt.y, lt.z, bitwise_cast<uint32_t>(probeSize > 0 ? probeSize : 1e9f));
}

inline IPoint4 RadianceGrid::calc_clip_var(const Clip &clip) { return ::calc_clip_var(clip.lt, clip.probeSize); }

inline IPoint3 RadianceGrid::Clipmap::getNewClipLT(uint32_t clip, const Point3 &world_pos) const
{
  return ipoint3(floor(Point3::xzy(world_pos) / get_probe_size(clip) + 0.5)) - IPoint3(clipW, clipW, clipD) / 2;
}

void RadianceGrid::clearRadiancePosition()
{
  for (int i = 0; i < DAGI_MAX_RAD_GRID_CLIPS; ++i)
    ShaderGlobal::set_int4(dagi_rad_grid_clipmap_lt_coordVarId[i], calc_clip_var(Clip()));
}

void RadianceGrid::clearIrradiancePosition()
{
  for (int i = 0; i < DAGI_MAX_IRRAD_GRID_CLIPS; ++i)
    ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_lt_coordVarId[i], calc_clip_var(Clip()));
}
void RadianceGrid::clearPosition()
{
  clearRadiancePosition();
  clearIrradiancePosition();
}

void RadianceGrid::Clipmap::resetHistory(uint32_t clips)
{
  clipmap.resize(clips);
  clipmap.assign(clipmap.size(), Clip());
  temporalFrames.resize(clipmap.size());
  temporalFrames.assign(temporalFrames.size(), 0);
  temporalFrame = 0;
}

void RadianceGrid::initHistory()
{
  if (validHistory)
    return;
  d3d::set_rwbuffer(STAGE_CS, 0, dagi_radiance_grid_selected_probes.getBuf());
  dagi_radiance_grid_clear_temporal_cs->dispatchThreads(1, 1, 1);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  // clearing should not be needed is variables disallow accessing invalid values
  // d3d::clear_rwtexf(dagi_irradiance_grid_sph0.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
  // d3d::clear_rwtexf(dagi_irradiance_grid_sph1.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
  // d3d::clear_rwtexf(dagi_radiance_grid.getVolTex(), ResourceClearValue{}.asFloat, 0, 0);
  // d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  // d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  validHistory = true;
}

RadianceGrid::Clipmap::Temporal RadianceGrid::Clipmap::updateTemporal(const uint32_t threads, const uint32_t tempOffset)
{
  const uint32_t buffer_size = temporalProbesBufferSize;
  Temporal t;
  if ((temporalFrame & ((1 << clipmap.size()) - 1)) == 0)
    temporalFrame = 1;
  t.clip = __bsr((temporalFrame & ((1 << clipmap.size()) - 1)));
  t.clip = clipmap.size() - 1 - t.clip;
  for (uint32_t i = 0, ie = clipmap.size(); clipmap[t.clip].probeSize == 0 && i < ie; ++i)
    t.clip = (t.clip + 1) % clipmap.size();

  uint32_t frame = temporalFrames[t.clip]++;
  if (frame == (1 << 24))
    frame = temporalFrames[t.clip] = 0;
  const uint32_t clipSize = clipW * clipW * clipD;
  const uint32_t framesToUpdate = (clipSize + buffer_size - 1) / buffer_size;
  t.frameSparse = framesToUpdate > 3 ? get_log2i(framesToUpdate) : 1; // each time keep probes passing through twice
                                                                      // more than buffer size (to ensure we have enough
                                                                      // space)
  const uint32_t frameUse = frame / t.frameSparse;
  const uint32_t idealBatchSize = clipSize / t.frameSparse;
  t.batchSize = ((idealBatchSize + threads - 1) / threads) * threads;


  ShaderGlobal::set_int4(dagi_rad_grid_temporalVarId, temporalProbesBufferSize * t.frameSparse, frameUse, t.clip,
    (frame % t.frameSparse) * t.batchSize);
  ShaderGlobal::set_int4(dagi_rad_grid_temporal_sizeVarId, temporalProbesBufferSize, t.batchSize, clipW, clipD);

  ShaderGlobal::set_int4(dagi_rad_grid_temporal_indirectVarId, clipW * clipW * clipD, (tempOffset + t.clip) * 8 + 4,
    temporalProbesBufferSize, 0);
  ++temporalFrame;
  return t;
}

void RadianceGrid::selectProbes(ComputeShaderElement &cs, uint32_t batchSize)
{
  DA_PROFILE_GPU;
  // from previous frame
  d3d::resource_barrier({dagi_radiance_grid_selected_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  // select new
  d3d::set_rwbuffer(STAGE_CS, 0, dagi_radiance_grid_selected_probes.getBuf());
  cs.dispatchThreads(batchSize, 1, 1);

  createIndirect();
}

void RadianceGrid::createIndirect()
{
  DA_PROFILE_GPU;
  d3d::resource_barrier({dagi_radiance_grid_selected_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::set_rwbuffer(STAGE_CS, 1, dagi_rad_grid_indirect_buffer.getBuf());
  dagi_radiance_grid_create_indirect_cs->dispatch(1, 1, 1);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  d3d::resource_barrier({dagi_rad_grid_indirect_buffer.getBuf(), RB_RO_INDIRECT_BUFFER});
}

void RadianceGrid::updateTemporal()
{
  if (!gi_radiance_grid_update_temporal)
    return;
  updateTemporalRadiance();
  updateTemporalIrradiance();
}

void RadianceGrid::updateTemporalRadiance()
{
  if (lastUpdatedRadianceClipsCount || !radiance.temporalProbesBufferSize)
    return;
  DA_PROFILE_GPU;

  auto &grid = radiance;
  auto t = grid.updateTemporal(dagi_radiance_grid_select_temporal_cs->getThreadGroupSizes()[0], 0);
  if (!grid.clipmap[t.clip].isValid())
    return;
  d3d::resource_barrier({dagi_radiance_grid_probes_age.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  selectProbes(*dagi_radiance_grid_select_temporal_cs, t.batchSize);
  if (debug_flag)
  {
    uint32_t *count;
    if (dagi_radiance_grid_selected_probes->lock(0, 4 + DAGI_MAX_RAD_GRID_CLIPS * 8, (void **)&count, VBLOCK_READONLY))
    {
      debug("count groups %d probability %d, count %f, framesSparse %d buffer size %d, clip %d", *count, count[t.clip * 2 + 1],
        count[t.clip * 2 + 2] / 32.f, t.frameSparse, grid.temporalProbesBufferSize, t.clip);
      dagi_radiance_grid_selected_probes->unlock();
    }
  }

  // d3d::resource_barrier({dagi_radiance_grid_sph.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE,0,0});
  TIME_D3D_PROFILE(rad_grid_calc_probes);
  dagi_radiance_grid_calc_temporal_cs->dispatch_indirect(dagi_rad_grid_indirect_buffer.getBuf(), 0);
  d3d::resource_barrier({dagi_radiance_grid.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});

  if (irradianceType == IrradianceType::DIRECT)
  {
    TIME_D3D_PROFILE(rd_temporal_irradiance);
    dagi_radiance_grid_calc_temporal_irradiance_cs->dispatch_indirect(dagi_rad_grid_indirect_buffer.getBuf(), 0);

    auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
    d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
    d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
  }
}

void RadianceGrid::updateTemporalIrradiance()
{
  if (irradianceType != IrradianceType::DETAILED || lastUpdatedIrradianceClipsCount || !irradiance.temporalProbesBufferSize)
    return;
  DA_PROFILE_GPU;

  auto &grid = irradiance;
  auto t = grid.updateTemporal(dagi_irradiance_grid_select_temporal_cs->getThreadGroupSizes()[0], DAGI_MAX_RAD_GRID_CLIPS);
  if (!grid.clipmap[t.clip].isValid())
    return;
  selectProbes(*dagi_irradiance_grid_select_temporal_cs, t.batchSize);

  TIME_D3D_PROFILE(irrad_grid_calc_probes);
  dagi_irradiance_grid_calc_temporal_cs->dispatch_indirect(dagi_rad_grid_indirect_buffer.getBuf(), 0);
  auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
  d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
  d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
}

RadianceGrid::~RadianceGrid()
{
  clearPosition();
  ShaderGlobal::set_int4(dagi_rad_grid_clipmap_sizeiVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_rad_grid_clipmap_sizei_np2VarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizeiVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizei_np2VarId, 0, 0, 0, 0);
}

bool RadianceGrid::updateClip(uint32_t clip_no, const IPoint3 &lt, float newProbeSize)
{
  DA_PROFILE_GPU;
  auto &grid = radiance;
  auto &clip = grid.clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 sz(grid.clipW, grid.clipW, grid.clipD);
  const IPoint3 oldLt = (clip.probeSize != newProbeSize) ? clip.lt - sz * 2 : clip.lt;
  if (oldLt == lt)
    return false;
  // debug("sky clip %d: %@->%@ %f %f", clip_no, oldLt, lt, clip.probeSize, newProbeSize);
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt = move_box_toroidal(lt, oldLt, sz, changedSpan);
  clip.lt = lt;
  clip.probeSize = newProbeSize;

#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(grid.clipW) || !is_pow_of2(grid.clipD))
  {
    IPoint4 l = dagi_rad_grid_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, grid.clipW,
        grid.clipD);
    }
  }
#endif

  ShaderGlobal::set_int4(dagi_rad_grid_clipmap_lt_coordVarId[clip_no], calc_clip_var(clip));

  for (int ui = 0; ui < changedCnt; ++ui)
  {
    ShaderGlobal::set_int4(dagi_rad_grid_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
    const IPoint3 updateSize = changed[ui].width();
    ShaderGlobal::set_int4(dagi_rad_grid_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
      bitwise_cast<uint32_t>(clip.probeSize));
    dagi_radiance_grid_toroidal_movement_cs->dispatch(updateSize.x, updateSize.y, updateSize.z);

    // as we are reading in next clip anyway, we have to add barrier between clips
    d3d::resource_barrier(
      {dagi_radiance_grid.getVolTex(), ui < changedCnt - 1 ? RB_NONE : RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({dagi_radiance_grid_probes_age.getVolTex(), RB_NONE, 0, 0});
  }

  if (irradianceType == IrradianceType::DIRECT)
  {
    ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_lt_coordVarId[clip_no], calc_clip_var(clip));
    TIME_D3D_PROFILE(rd_irradiance);
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_rad_grid_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_rad_grid_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      dagi_radiance_grid_toroidal_movement_irradiance_cs->dispatch(updateSize.x, updateSize.y, updateSize.z);

      auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
      d3d::resource_barrier(
        {dagi_irradiance_grid_sph0.getVolTex(), ui < changedCnt - 1 ? RB_NONE : RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
      d3d::resource_barrier(
        {dagi_irradiance_grid_sph1.getVolTex(), ui < changedCnt - 1 ? RB_NONE : RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
    }
  }
  return true;
}

bool RadianceGrid::updateClipIrradiance(uint32_t clip_no, const IPoint3 &lt, float newProbeSize, bool last)
{
  if (irradianceType != IrradianceType::DETAILED)
    return false;
  // fixme:remove copypaste
  DA_PROFILE_GPU;
  auto &grid = irradiance;
  auto &clip = grid.clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 sz(grid.clipW, grid.clipW, grid.clipD);
  const IPoint3 oldLt = (clip.probeSize != newProbeSize) ? clip.lt - sz * 2 : clip.lt;
  if (oldLt == lt)
    return false;
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt = move_box_toroidal(lt, oldLt, sz, changedSpan);
  clip.lt = lt;
  clip.probeSize = newProbeSize;

#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(grid.clipW) || !is_pow_of2(grid.clipD))
  {
    IPoint4 l = dagi_irrad_grid_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, grid.clipW,
        grid.clipD);
    }
  }
#endif

  uint32_t numFilterPasses = dagi_irradiance_grid_spatial_filter_cs ? gi_irradiance_spatial_passes.get() : 0;
  ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_lt_coordVarId[clip_no], calc_clip_var(clip));
  {
    TIME_D3D_PROFILE(rd_irradiance_interpolate);
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_irrad_grid_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_irrad_grid_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      dagi_irradiance_grid_toroidal_movement_interpolate_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1);

      d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_NONE, 0, 0});
      d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_NONE, 0, 0});
      d3d::resource_barrier({dagi_irradiance_grid_probes_age.getVolTex(),
        ui < changedCnt - 1 ? RB_NONE : RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    }
    d3d::resource_barrier({dagi_irradiance_grid_probes_age.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }
  {
    TIME_D3D_PROFILE(rd_irradiance_trace);
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_irrad_grid_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_irrad_grid_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      dagi_irradiance_grid_toroidal_movement_trace_cs->dispatch(updateSize.x, updateSize.y, updateSize.z);
      dagi_irradiance_grid_toroidal_movement_trace_cs->dispatch(updateSize.x, updateSize.y, updateSize.z);

      if (ui < changedCnt - 1 || (!last && !numFilterPasses))
      {
        d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_NONE, 0, 0});
        d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_NONE, 0, 0});
      }
    }
  }

  if (numFilterPasses)
  {
    TIME_D3D_PROFILE(spatial_filter);
    d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});

    for (int si = 0; si < numFilterPasses; ++si)
      for (int ui = 0; ui < changedCnt; ++ui)
      {
        ShaderGlobal::set_int4(dagi_irrad_grid_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
        const IPoint3 updateSize = changed[ui].width();
        ShaderGlobal::set_int4(dagi_irrad_grid_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
          bitwise_cast<uint32_t>(clip.probeSize));
        dagi_irradiance_grid_spatial_filter_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1);
        d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(),
          ui < changedCnt - 1 || si == numFilterPasses - 1 ? RB_NONE : RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0,
          0});
        d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(),
          ui < changedCnt - 1 || si == numFilterPasses - 1 ? RB_NONE : RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0,
          0});
      }
  }
  return true;
}

constexpr int max_pos = (1 << 30) - 1;
void set_irradiance_grid_textures(TEXTUREID sph0, TEXTUREID sph1)
{
  ShaderGlobal::set_texture(dagi_irradiance_grid_sph0VarId, sph0);
  ShaderGlobal::set_texture(dagi_irradiance_grid_sph1VarId, sph1);
}

void set_irradiance_grid_params(uint32_t w, uint32_t d, uint32_t clips, int grid_info)
{
  if (!clips)
  {
    ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizeiVarId, 0, 0, 0, 0);
    ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizei_np2VarId, 0, 0, 0, 0);
    ShaderGlobal::set_color4(dagi_irrad_grid_atlas_decodeVarId, 0, 0, 0, grid_info);
    ShaderGlobal::set_color4(dagi_irrad_grid_inv_sizeVarId, 0, 0, 0, 0);
    return;
  }
  init_vars();
  const float fullAtlasDepth = float((d + 2) * clips);
  const float clipInAtlasPart = float(d) / fullAtlasDepth;
  const float clipWithBorderInAtlasPart = float(d + 2) / fullAtlasDepth;

  ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizeiVarId, w, d, clips, d + 2);
  ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_sizei_np2VarId, w, d, is_pow_of2(w) ? 0 : (max_pos / w) * w,
    is_pow_of2(d) ? 0 : (max_pos / d) * d);
  ShaderGlobal::set_color4(dagi_irrad_grid_atlas_decodeVarId, clipInAtlasPart, clipWithBorderInAtlasPart, 1. / fullAtlasDepth,
    grid_info);
  ShaderGlobal::set_color4(dagi_irrad_grid_inv_sizeVarId, 1.f / w, 1. / d, 1.f / fullAtlasDepth, 1. / (d + 2));
}

void set_irradiance_clip(uint32_t clip_no, const IPoint3 &lt, float probeSize)
{
  if (clip_no < DAGI_MAX_IRRAD_GRID_CLIPS)
    ShaderGlobal::set_int4(dagi_irrad_grid_clipmap_lt_coordVarId[clip_no], calc_clip_var(lt, probeSize));
}

void RadianceGrid::createIrradiance()
{
  dagi_irradiance_grid_sph0.close();
  dagi_irradiance_grid_sph1.close();
  dagi_irradiance_grid_probes_age.close();
  const uint32_t w = irradiance.clipW, d = irradiance.clipD, clips = irradiance.clipmap.size();

  debug("create RadianceGrid %s irradiance resolution %dx%dx%d : %d probe %f (dist %f)",
    irradianceType == IrradianceType::DIRECT ? "direct" : "detailed", w, w, d, clips, irradiance.probeSize0,
    (w << (clips - 1)) * 0.5f * irradiance.probeSize0);
  dagi_irradiance_grid_sph0 =
    dag::create_voltex(w, w, (d + 2) * clips, TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "dagi_irradiance_grid_sph0");
  dagi_irradiance_grid_sph1 =
    dag::create_voltex(w, w, (d + 2) * clips, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "dagi_irradiance_grid_sph1");
  // todo: there is enough empty bits in dagi_irradiance_grid_sph0.b (10bit float!) to implement age (or something else) there
  if (irradianceType != IrradianceType::DIRECT)
    dagi_irradiance_grid_probes_age =
      dag::create_voltex(w, w, d * clips, TEXCF_UNORDERED | TEXFMT_R8, 1, "dagi_irradiance_grid_probes_age");
  set_irradiance_grid_params(w, d, clips, irradianceType == IrradianceType::DIRECT ? 1 : 0);
}

void RadianceGrid::initClipmap(uint32_t clipW, uint32_t clipD, uint32_t clips_, uint32_t oct_res, float probe0)
{
  if constexpr (!DAGI_RAD_GRID_ALLOW_NON_POW2)
  {
    clipW = get_bigger_pow2(clipW);
    clipD = get_bigger_pow2(clipD);
  }
  clipW = clamp<uint16_t>(clipW, 8, DAGI_MAX_RAD_GRID_RES);
  clipD = clamp<uint16_t>(clipD, 8, DAGI_MAX_RAD_GRID_RES);
  clips_ = clamp<uint16_t>(clips_, 1, DAGI_MAX_RAD_GRID_CLIPS);
  clearRadiancePosition();
  radiance.resetHistory(clips_);
  radiance.probeSize0 = probe0;
  radiance.clipW = clipW;
  radiance.clipD = clipD;
  radiance.resetHistory(clips_);
  octRes = oct_res;

  debug("create RadianceGrid resolution %dx%dx%d * %d^2 : %d probe %f (dist %f)", clipW, clipW, clipD, clips_, octRes,
    radiance.probeSize0, (clipW << (clips_ - 1)) * 0.5f * radiance.probeSize0);

  dagi_radiance_grid.close();
  const uint32_t texFmt = TEXFMT_R11G11B10F; // TEXFMT_A16B16G16R16F
  dagi_radiance_grid =
    dag::create_voltex(clipW * octRes, clipW * octRes, clipD * clips_, TEXCF_UNORDERED | texFmt, 1, "dagi_radiance_grid");
  ShaderGlobal::set_sampler(dagi_radiance_grid_samplerstateVarId, d3d::request_sampler({}));

  const uint32_t fullDepth = clipD * clips_;
  ShaderGlobal::set_int4(dagi_rad_grid_clipmap_sizeiVarId, clipW, clipD, clips_, fullDepth);
  ShaderGlobal::set_int4(dagi_rad_grid_clipmap_sizei_np2VarId, clipW, clipD, is_pow_of2(clipW) ? 0 : (max_pos / clipW) * clipW,
    is_pow_of2(clipD) ? 0 : (max_pos / clipD) * clipD);

  ShaderGlobal::set_color4(dagi_rad_grid_decode_xyVarId, 0.5f / clipW, 0.5f / (octRes * clipW), (1.f - 0.5f / octRes) / clipW, 0);
  ShaderGlobal::set_color4(dagi_rad_grid_decode_zVarId, 1.f / clipD, 1.f / clips_, 0.5f / fullDepth, 1.f / clipW);
  ShaderGlobal::set_int(dagi_rad_oct_resiVarId, octRes);

  dagi_radiance_grid_probes_age.close();
  dagi_radiance_grid_probes_age =
    dag::create_voltex(clipW, clipW, clipD * clips_, TEXCF_UNORDERED | TEXFMT_R8, 1, "dagi_radiance_grid_probes_age");
}

void RadianceGrid::initClipmapIrradiance(uint32_t clipW, uint32_t clipD, uint32_t clips_, float probe0)
{
  if constexpr (!DAGI_IRRAD_GRID_ALLOW_NON_POW2)
  {
    clipW = get_bigger_pow2(clipW);
    clipD = get_bigger_pow2(clipD);
  }
  clipW = clamp<uint16_t>(clipW, 8, DAGI_MAX_IRRAD_GRID_RES);
  clipD = clamp<uint16_t>(clipD, 8, DAGI_MAX_IRRAD_GRID_RES);
  clips_ = clamp<uint16_t>(clips_, 1, DAGI_MAX_IRRAD_GRID_CLIPS);
  clearIrradiancePosition();

  irradiance.clipW = clipW;
  irradiance.clipD = clipD;
  irradiance.probeSize0 = probe0;
  irradianceType = clipW == radiance.clipW && clipD == radiance.clipD && clips_ == radiance.clipmap.size() ? IrradianceType::DIRECT
                                                                                                           : IrradianceType::DETAILED;
  irradiance.resetHistory(clips_);

  createIrradiance();
}

void RadianceGrid::initTemporal(uint32_t frames_to_update_clip)
{
  if (frames_to_update_clip == radiance.totalFramesPerClipToUpdate)
    return;
  validHistory = false;
  const uint32_t clipProbes = radiance.clipD * radiance.clipW * radiance.clipW;
  radiance.totalFramesPerClipToUpdate = frames_to_update_clip;
  radiance.temporalProbesBufferSize =
    radiance.totalFramesPerClipToUpdate ? max(clipProbes / radiance.totalFramesPerClipToUpdate, 1u) : 0;

  dagi_radiance_grid_selected_probes.close();
  dagi_rad_grid_indirect_buffer.close();
  if (radiance.temporalProbesBufferSize)
  {
    const uint32_t tsz = 0; // if !typedUAV (octRes * octRes + 1) / 2;
    selectedProbesDwords = (tsz + 1) * radiance.temporalProbesBufferSize;
    dagi_radiance_grid_selected_probes =
      dag::create_sbuffer(sizeof(uint32_t), selectedProbesDwords + (DAGI_MAX_RAD_GRID_CLIPS + DAGI_MAX_IRRAD_GRID_CLIPS) * 2 + 1,
        SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES | debug_flag, 0, "dagi_radiance_grid_selected_probes");
    dagi_rad_grid_indirect_buffer = dag::create_sbuffer(sizeof(uint32_t), 3,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_UA_INDIRECT | debug_flag, 0, "dagi_rad_grid_indirect_buffer");
  }
  else
    selectedProbesDwords = 0;
  irradiance.temporalProbesBufferSize = selectedProbesDwords / 4;
}

void RadianceGrid::init(uint8_t w_, uint8_t d_, uint8_t clips_, float probe0, uint8_t additional_iclips, float irradiance_detail,
  uint32_t frames_to_update_clip)
{
  const uint32_t oct_res = DAGI_RAD_GRID_OCT_RES; // fixme

  uint16_t iw_ = w_ * irradiance_detail, id_ = d_ * irradiance_detail, iclips_ = clips_ + additional_iclips;
  float iprobe0 = ((w_ << (clips_ - 1)) * probe0) / (iw_ << (iclips_ - 1));
  if (iprobe0 > probe0) // we better use same same radiance then. As it is almost free.
  {
    iprobe0 = probe0;
    iw_ = w_;
    id_ = d_;
    iclips_ = clips_;
  }
  const bool sameClipmap = radiance.clipW == w_ && radiance.clipD == d_ && radiance.clipmap.size() == clips_ && oct_res == octRes &&
                           radiance.probeSize0 == probe0;
  const bool sameIrradiance =
    irradiance.clipW == iw_ && irradiance.clipD == id_ && irradiance.clipmap.size() == iclips_ && irradiance.probeSize0 == iprobe0;
  if (sameClipmap && sameIrradiance && frames_to_update_clip == radiance.totalFramesPerClipToUpdate)
    return;
  initVars();

  // sameIrradiance should not be checked. Instead if irradiance is not same, and sameClipmap == true && DIRECT, we need to recalc
  // irradiance
  if (!sameClipmap || !sameIrradiance)
    initClipmap(w_, d_, clips_, oct_res, probe0);

  if (!sameIrradiance)
    initClipmapIrradiance(iw_, id_, iclips_, iprobe0);

  initTemporal(frames_to_update_clip);
}

template <class Cb>
inline void RadianceGrid::Clipmap::updatePos(const Point3 &world_pos, bool update_all, Cb cb) const
{
  for (int i = clipmap.size() - 1; i >= 0; --i)
  {
    auto &clip = clipmap[i];
    const IPoint3 lt = getNewClipLT(i, world_pos);
    const IPoint3 move = lt - clip.lt, absMove = abs(move);
    const float probeSize = get_probe_size(i);
    const int maxMove = max(max(absMove.x, absMove.y), absMove.z);
    if (maxMove > DAGI_IRRAD_GRID_MOVE_THRESHOLD || clip.probeSize != probeSize)
    {
      IPoint3 useMove{0, 0, 0};
      const Point3 relMove = div(point3(absMove), Point3(clipW, clipW, clipD));
      if (clip.probeSize != probeSize ||                      // everything has changed
          max(max(relMove.x, relMove.y), relMove.z) >= 1.f || // we moved one axis in more than full direction
          relMove.x + relMove.y + relMove.z >= 1) // trigger full update anyway, as we have moved enough. This decrease number of
                                                  // spiked frames after teleport
        useMove = move;
      else if (absMove.x == maxMove)
        useMove.x = move.x;
      else if (absMove.y == maxMove)
        useMove.y = move.y;
      else
        useMove.z = move.z;
      cb(i, RadianceGrid::Clip{clip.lt + useMove, probeSize});
      if (!update_all)
        break;
    }
  }
}

void RadianceGrid::afterReset()
{
  radiance.resetHistory(radiance.clipmap.size());
  irradiance.resetHistory(irradiance.clipmap.size());
  clearPosition();
  validHistory = false;
}

void RadianceGrid::updatePosIrradiance(const Point3 &world_pos, bool update_all)
{
  lastUpdatedIrradianceClipsCount = 0;
  if (irradianceType != IrradianceType::DETAILED)
    return;
  if (gi_irradiance_grid_invalidate)
  {
    update_all = true;
    irradiance.resetHistory(irradiance.clipmap.size());
  }
  DA_PROFILE_GPU;
  StaticTab<eastl::pair<int, Clip>, DAGI_MAX_IRRAD_GRID_CLIPS> updatedClipmaps;
  irradiance.updatePos(world_pos, update_all, [&updatedClipmaps](int i, const Clip &c) { updatedClipmaps.emplace_back(i, c); });

  d3d::resource_barrier(
    {dagi_irradiance_grid_probes_age.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  for (auto &clip : updatedClipmaps)
    updateClipIrradiance(clip.first, clip.second.lt, clip.second.probeSize, &clip == &updatedClipmaps.back());
  lastUpdatedIrradianceClipsCount = updatedClipmaps.size();
  if (lastUpdatedIrradianceClipsCount)
  {
    auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
    d3d::resource_barrier({dagi_irradiance_grid_sph0.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
    d3d::resource_barrier({dagi_irradiance_grid_sph1.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
  }
}

void RadianceGrid::updatePos(const Point3 &world_pos, bool update_all)
{
  DA_PROFILE_GPU;
  StaticTab<eastl::pair<int, Clip>, DAGI_MAX_RAD_GRID_CLIPS> updatedClipmaps;
  initHistory();
  radiance.updatePos(world_pos, update_all, [&updatedClipmaps](int i, const Clip &c) { updatedClipmaps.emplace_back(i, c); });
  if (!updatedClipmaps.empty())
  {
    d3d::resource_barrier(
      {dagi_radiance_grid_probes_age.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    for (auto &clip : updatedClipmaps)
      updateClip(clip.first, clip.second.lt, clip.second.probeSize);
    d3d::resource_barrier({dagi_radiance_grid.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }
  lastUpdatedRadianceClipsCount = updatedClipmaps.size();
  updatePosIrradiance(world_pos, update_all);
}

void RadianceGrid::drawDebug(int debug_type)
{
  if (!drawDebugAllProbes.shader)
    drawDebugAllProbes.init("dagi_rad_grid_draw_debug", NULL, 0, "dagi_rad_grid_draw_debug");
  if (!drawDebugAllProbes.shader)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_rad_grid_debug_typeVarId, debug_type);
  drawDebugAllProbes.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, radiance.clipW * radiance.clipW * radiance.clipD);
}

void RadianceGrid::drawDebugIrradiance(int debug_type)
{
  if (!drawDebugAllProbesIrradiance.shader)
    drawDebugAllProbesIrradiance.init("dagi_irrad_grid_draw_debug", NULL, 0, "dagi_irrad_grid_draw_debug");
  if (!drawDebugAllProbesIrradiance.shader)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_irrad_grid_debug_typeVarId, debug_type);
  drawDebugAllProbesIrradiance.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  auto &grid = (irradianceType == IrradianceType::DIRECT) ? radiance : irradiance;
  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, grid.clipW * grid.clipW * grid.clipD);
}
