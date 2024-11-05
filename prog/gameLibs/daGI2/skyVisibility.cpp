// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_staticTab.h>
#include <util/dag_convar.h>
#include <render/spheres_consts.hlsli>
#include <shaders/dag_DynamicShaderHelper.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/string.h>
#include <shaders/dag_shaderVariableInfo.h>
#include <render/subtract_ibbox3.h>
#include "skyVisibility/dagi_sky_vis.hlsli"
#include "skyVisibility.h"
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

#define GLOBAL_VARS_LIST                    \
  VAR(dagi_sky_vis_sizef)                   \
  VAR(dagi_sky_vis_clipmap_sizei)           \
  VAR(dagi_sky_vis_clipmap_sizei_np2)       \
  VAR(dagi_sky_vis_debug_type)              \
  VAR(dagi_sky_vis_atlas_decode)            \
  VAR(dagi_sky_vis_temporal_size)           \
  VAR(dagi_sky_vis_temporal)                \
  VAR(dagi_sky_vis_use_sim_bounce)          \
  VAR(dagi_sky_vis_uav_load)                \
  VAR(dagi_sky_vis_trace_dist)              \
  VAR(dagi_sky_vis_update_lt_coord)         \
  VAR(dagi_sky_vis_spatial_update_from)     \
  VAR(dagi_sky_vis_update_sz_coord)         \
  VAR(dagi_sky_visibility_sph_samplerstate) \
  VAR(dagi_irradiance_grid_sph1_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

CONSOLE_BOOL_VAL("gi", gi_sky_vis_update_temporal, true);
CONSOLE_BOOL_VAL("gi", gi_sky_vis_update_all, false);
CONSOLE_BOOL_VAL("gi", gi_sky_vis_spatial_split, false);
CONSOLE_INT_VAL("gi", gi_sky_vis_spatial_passes, 1, 0, 2);

extern void set_irradiance_grid_params(uint32_t w, uint32_t d, uint32_t clips, int info);
extern void set_irradiance_grid_textures(TEXTUREID sph0, TEXTUREID sph1);
extern void set_irradiance_clip(uint32_t clip_no, const IPoint3 &lt, float probeSize);

static ShaderVariableInfo dagi_sky_vis_clipmap_lt_coordVarId[DAGI_MAX_SKY_VIS_CLIPS];
static ShaderVariableInfo dagi_sky_visibility_selected_probesVarId;
inline int volume(const IPoint3 &i) { return i.x * i.y * i.z; }
const uint32_t debug_flag = 0; // SBCF_USAGE_READ_BACK;

void SkyVisibility::initVars()
{

  if (dagi_sky_visibility_calc_temporal_cs)
    return;
  supportUnorderedLoad = d3d::get_texformat_usage(TEXCF_UNORDERED | TEXFMT_A16B16G16R16F) & d3d::USAGE_UNORDERED_LOAD;
  ShaderGlobal::set_int(dagi_sky_vis_uav_loadVarId, supportUnorderedLoad ? 1 : 0);
  eastl::string str;
  for (int i = 0; i < DAGI_MAX_SKY_VIS_CLIPS; ++i)
  {
    str.sprintf("dagi_sky_vis_clipmap_lt_coord_%d", i);
    dagi_sky_vis_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
  }
  dagi_sky_visibility_selected_probesVarId = get_shader_variable_id("dagi_sky_visibility_selected_probes");
#define CS(a) a.reset(new_compute_shader(#a))
  CS(dagi_sky_visibility_calc_temporal_cs);
  CS(dagi_sky_visibility_select_temporal_cs);
  CS(dagi_sky_visibility_clear_temporal_cs);
  CS(dagi_sky_visibility_create_indirect_cs);
  CS(dagi_sky_visibility_toroidal_movement_interpolate_cs);
  CS(dagi_sky_visibility_toroidal_movement_trace_cs);
  CS(dagi_sky_visibility_toroidal_movement_spatial_filter_split_cs);
  CS(dagi_sky_visibility_toroidal_movement_spatial_filter_split_apply_cs);
#undef CS
}

static void set_clip_vars(int clip_no, const IPoint3 &lt, float probeSize, bool replicateToIrradiance)
{
  if (clip_no < DAGI_MAX_SKY_VIS_CLIPS)
    ShaderGlobal::set_int4(dagi_sky_vis_clipmap_lt_coordVarId[clip_no], lt.x, lt.y, lt.z,
      bitwise_cast<uint32_t>(probeSize > 0 ? probeSize : 1e9f));
  if (replicateToIrradiance)
    set_irradiance_clip(clip_no, lt, probeSize);
}

void SkyVisibility::setClipVars(int clip_no) const
{
  auto &clip = clipmap[clip_no];
  set_clip_vars(clip_no, clip.lt, clip.probeSize, replicateToIrradiance);
#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(clipW) || !is_pow_of2(clipD))
  {
    IPoint4 l = dagi_sky_vis_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, clipW, clipD);
    }
  }
#endif
}

inline IPoint3 SkyVisibility::getNewClipLT(uint32_t clip, const Point3 &world_pos) const
{
  return ipoint3(floor(Point3::xzy(world_pos) / get_probe_size(clip) + 0.5)) - IPoint3(clipW, clipW, clipD) / 2;
}

static void clear_clipmap(bool replicateToIrradiance)
{
  for (int i = 0; i < DAGI_MAX_SKY_VIS_CLIPS; ++i)
    set_clip_vars(i, IPoint3(-100000, 100000, +100000), 0, replicateToIrradiance);
}

SkyVisibility::~SkyVisibility()
{
  clear_clipmap(replicateToIrradiance);
  ShaderGlobal::set_int4(dagi_sky_vis_clipmap_sizeiVarId, 0, 0, 0, 0);
  ShaderGlobal::set_color4(dagi_sky_vis_sizefVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_sky_vis_clipmap_sizei_np2VarId, 0, 0, 0, 0);
  if (replicateToIrradiance)
  {
    set_irradiance_grid_params(0, 0, 0, 2);
    set_irradiance_grid_textures(BAD_TEXTUREID, BAD_TEXTUREID);
  }
}

void SkyVisibility::setClipmapVars()
{
  clear_clipmap(replicateToIrradiance);
  for (int i = 0, ie = clipmap.size(); i < ie; ++i)
    setClipVars(i);
}

void SkyVisibility::initHistory()
{
  if (validHistory)
    return;
  clipmap.assign(clipmap.size(), Clip());
  setClipmapVars();
  temporalFrame = 0;
  dagi_sky_visibility_clear_temporal_cs->dispatchThreads(1, 1, 1);
  validHistory = true;
}

void SkyVisibility::updateTemporal(bool update_clipmap)
{
  // todo: temporally re-calc SPH from sky (better from pre-averaged octahedral)
  if (update_clipmap && temporalProbesBufferSize)
  {
    DA_PROFILE_GPU;
    if ((temporalFrame & ((1 << clipmap.size()) - 1)) == 0)
      temporalFrame = 1;
    uint32_t clip = __bsr((temporalFrame & ((1 << clipmap.size()) - 1)));
    clip = clipmap.size() - 1 - clip;
    for (uint32_t i = 0, ie = clipmap.size(); clipmap[clip].probeSize == 0 && i < ie; ++i)
      clip = (clip + 1) % clipmap.size();

    const uint32_t frame = temporalFrames[clip]++;
    if (frame == 1 << 24)
      temporalFrames[clip] = 0;
    const uint32_t clipSize = clipW * clipW * clipD;
    const uint32_t framesToUpdate = (clipSize + temporalProbesBufferSize - 1) / temporalProbesBufferSize;
    const uint32_t frameSparse = framesToUpdate > 3 ? get_log2i(framesToUpdate) : 1; // each time keep probes passing through twice
                                                                                     // more than buffer size (to ensure we have enough
                                                                                     // space)
    const uint32_t frameUse = frame / frameSparse;
    const uint32_t idealBatchSize = clipSize / frameSparse;
    const uint32_t threads = dagi_sky_visibility_select_temporal_cs->getThreadGroupSizes()[0];
    const uint32_t batchSize = ((idealBatchSize + threads - 1) / threads) * threads;
    ShaderGlobal::set_int(dagi_sky_vis_use_sim_bounceVarId, clip < simulatedBounceCascadesCount ? 1 : 0);
    ShaderGlobal::set_int4(dagi_sky_vis_temporalVarId, temporalProbesBufferSize * frameSparse, frameUse, clip,
      (frame % frameSparse) * batchSize);
    ShaderGlobal::set_int4(dagi_sky_vis_temporal_sizeVarId, temporalProbesBufferSize, batchSize, 0, 0);
    setTraceDist(clip);
    {
      TIME_D3D_PROFILE(sky_vis_select_probes);

      // flush from previous frame
      d3d::resource_barrier({dagi_sky_visibility_probabilities.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
      d3d::resource_barrier({dagi_sky_visibility_selected_probes, RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

      // select new
      dagi_sky_visibility_select_temporal_cs->dispatchThreads(batchSize, 1, 1);
      d3d::resource_barrier({dagi_sky_visibility_selected_probes, RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

      d3d::resource_barrier({dagi_sky_visibility_probabilities.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    }
    {
      TIME_D3D_PROFILE(sky_vis_create_indirect);
      dagi_sky_visibility_create_indirect_cs->dispatch(1, 1, 1);
    }
#if DAGOR_DBGLEVEL > 0
    if (dagi_sky_visibility_probabilities->getFlags() & SBCF_USAGE_READ_BACK)
    {
      uint32_t *count;
      if (dagi_sky_visibility_probabilities->lock(0, 4 + DAGI_MAX_SKY_VIS_CLIPS * 8, (void **)&count, VBLOCK_READONLY))
      {
        debug("count groups %d probability %d, count %f, framesSparse %d buffer size %d, clip %d", *count, count[clip * 2 + 1],
          count[clip * 2 + 2] / 32.f, frameSparse, temporalProbesBufferSize, clip);
        dagi_sky_visibility_probabilities->unlock();
      }
    }
#endif

    // d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE,0,0});
    TIME_D3D_PROFILE(sky_vis_calc_probes);
    dagi_sky_visibility_calc_temporal_cs->dispatch_indirect(dagi_sky_vis_indirect_buffer.getBuf(), 0);
    ++temporalFrame;
    d3d::resource_barrier({dagi_sky_visibility_age.getVolTex(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }
}

void SkyVisibility::setTraceDist(uint32_t clip_no)
{
  float intersectTraceDist = maxTraceDist, airTraceDist = maxTraceDist;
  if (supportUnorderedLoad && clip_no < clipmap.size() - 1)
  {
    intersectTraceDist = clipmap[clip_no + 1].probeSize * intersectTraceDistProbes;
    intersectTraceDist = min(intersectTraceDist, 0.25f * get_probe_size(clip_no + 1) * clipW);
    intersectTraceDist = min(intersectTraceDist, maxTraceDist * 1.f / (clipmap.size() - clip_no));
    airTraceDist = intersectTraceDist * airTraceDistScale;
  }
  if (!supportUnorderedLoad)
  {
    maxTraceDist *= 0.5;
    airTraceDist = intersectTraceDist * airTraceDistScale;
  }

  ShaderGlobal::set_color4(dagi_sky_vis_trace_distVarId, intersectTraceDist, airTraceDist, 0, 0);
}

uint32_t SkyVisibility::getRequiredTemporalBufferSize() const { return selectedProbesBufferSize; }

void SkyVisibility::setTemporalBuffer(const ManagedBuf &buf)
{
  ShaderGlobal::set_buffer(dagi_sky_visibility_selected_probesVarId, buf);
  dagi_sky_visibility_selected_probes = buf.getBuf();
}

bool SkyVisibility::updateClip(uint32_t clip_no, const IPoint3 &lt, float newProbeSize, bool updateLast)
{
  DA_PROFILE_GPU;
  auto &clip = clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 sz(clipW, clipW, clipD);
  const IPoint3 oldLt = (clip.probeSize != newProbeSize) ? clip.lt - sz * 2 : clip.lt;
  if (oldLt == lt)
    return false;
  ShaderGlobal::set_int(dagi_sky_vis_use_sim_bounceVarId, clip_no < simulatedBounceCascadesCount ? 1 : 0);
  // debug("sky clip %d: %@->%@ %f %f", clip_no, oldLt, lt, clip.probeSize, newProbeSize);
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt = move_box_toroidal(lt, oldLt, sz, changedSpan);
  clip.lt = lt;
  clip.probeSize = newProbeSize;

  setClipVars(clip_no);
  setTraceDist(clip_no);
  {
    TIME_D3D_PROFILE(skyvis_interpolate);
    // todo:implement split interpolate!
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_sky_vis_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_sky_vis_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      dagi_sky_visibility_toroidal_movement_interpolate_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1);
      d3d::resource_barrier({dagi_sky_visibility_age.getVolTex(),
        ui < changedCnt - 1 ? RB_NONE : RB_RO_SRV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
      d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(), RB_NONE, 0, 0});
    }
  }
  {
    TIME_D3D_PROFILE(skyvis_trace);
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_sky_vis_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_sky_vis_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      dagi_sky_visibility_toroidal_movement_trace_cs->dispatch(updateSize.x, updateSize.y, updateSize.z);

      d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(),
        ui < changedCnt - 1 ? RB_NONE : RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
    }
  }
  TIME_D3D_PROFILE(sky_vis_spatial_filter);
  uint32_t maxProbesInRegion = 0;
  for (auto &r : changed)
    maxProbesInRegion = max<uint32_t>(volume(r.width()), maxProbesInRegion);

  uint32_t numFilterPasses = gi_sky_vis_spatial_passes.get();
  G_ASSERT(supportUnorderedLoad || dagi_sky_visibility_toroidal_movement_spatial_filter_split_cs->getThreadGroupSizes()[0] ==
                                     dagi_sky_visibility_toroidal_movement_spatial_filter_split_apply_cs->getThreadGroupSizes()[0]);
  for (int si = 0; si < numFilterPasses; ++si)
  {
    for (int ui = 0; ui < changedCnt; ++ui)
    {
      ShaderGlobal::set_int4(dagi_sky_vis_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
      const IPoint3 updateSize = changed[ui].width();
      ShaderGlobal::set_int4(dagi_sky_vis_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
        bitwise_cast<uint32_t>(clip.probeSize));
      const uint32_t totalSize = updateSize.x * updateSize.y * updateSize.z;

      const uint32_t threads = dagi_sky_visibility_toroidal_movement_spatial_filter_split_cs->getThreadGroupSizes()[0];
      const uint32_t groups = (selectedProbesBufferSize / 2) / threads;
      G_ASSERT(groups != 0);
      for (uint32_t updated = 0, probesInBatch = groups * threads; updated < totalSize; updated += probesInBatch)
      {
        ShaderGlobal::set_int(dagi_sky_vis_spatial_update_fromVarId, updated);
        dagi_sky_visibility_toroidal_movement_spatial_filter_split_cs->dispatch(groups, 1, 1);
        d3d::resource_barrier({dagi_sky_visibility_selected_probes, RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
        dagi_sky_visibility_toroidal_movement_spatial_filter_split_apply_cs->dispatch(groups, 1, 1);

        if (updated + probesInBatch < totalSize)
          d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(), RB_NONE, 0, 0});
        else if (updateLast && ui == changedCnt - 1 && si == numFilterPasses - 1)
        {
          // do nothing, barrier is called in a caller
        }
        else
          d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(),
            ui == changedCnt - 1 ? RB_NONE : RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
      }
    }
  }
  return true;
}

void SkyVisibility::initClipmap(uint32_t w_, uint32_t d_, uint32_t clips_)
{
  clipW = w_;
  clipD = d_;
  debug("create SkyVisibility resolution %dx%dx%d : %d probe %f (dist %f)", clipW, clipW, clipD, clips_, probeSize0,
    (clipW << (clips_ - 1)) * 0.5f * probeSize0);

  dagi_sky_visibility_sph.close();
  dagi_sky_visibility_sph =
    dag::create_voltex(clipW, clipW, (clipD + 2) * clips_, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "dagi_sky_visibility_sph");
  dagi_sky_visibility_sph->disableSampler();
  dagi_sky_visibility_sph_samplerstateVarId.set_sampler(d3d::request_sampler({}));
  dagi_irradiance_grid_sph1_samplerstateVarId.set_sampler(d3d::request_sampler({}));
  dagi_sky_visibility_age.close();
  dagi_sky_visibility_age =
    dag::create_voltex(clipW, clipW, clipD * clips_, TEXCF_UNORDERED | TEXFMT_R8, 1, "dagi_sky_visibility_age");
  ShaderGlobal::set_int4(dagi_sky_vis_clipmap_sizeiVarId, clipW, clipD, clips_, clipD + 2);
  ShaderGlobal::set_color4(dagi_sky_vis_sizefVarId, clipW, clipD, 1.f / clipW, 1.f / clipD);
  constexpr int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(dagi_sky_vis_clipmap_sizei_np2VarId, clipW, clipD, is_pow_of2(clipW) ? 0 : (max_pos / clipW) * clipW,
    is_pow_of2(clipD) ? 0 : (max_pos / clipD) * clipD);
  if (replicateToIrradiance)
  {
    set_irradiance_grid_params(clipW, clipD, clips_, 2);
    set_irradiance_grid_textures(BAD_TEXTUREID, dagi_sky_visibility_sph.getTexId());
  }
}

void SkyVisibility::initTemporal(uint32_t frames_to_update_clip)
{
  G_ASSERT(frames_to_update_clip != ~0u);
  if (frames_to_update_clip == totalFramesPerClipToUpdate)
    return;
  const uint32_t clipProbes = clipD * clipW * clipW;
  totalFramesPerClipToUpdate = frames_to_update_clip;
  temporalProbesBufferSize = totalFramesPerClipToUpdate ? max(clipProbes / totalFramesPerClipToUpdate, 1u) : 0;

  const uint32_t clipSlice = clipW * clipW;
  selectedProbesBufferSize =
    max<uint32_t>(temporalProbesBufferSize * 3, ((clipSlice * (supportUnorderedLoad ? 2 : 4) + 63) & ~63) * 2);
  if (selectedProbesBufferSize)
  {
    if (!supportUnorderedLoad)
      debug("SkyVis: unordered load is not supported. filtering with multiplass");
    dagi_sky_visibility_probabilities = dag::create_sbuffer(sizeof(uint32_t), DAGI_MAX_SKY_VIS_CLIPS * 2 + 1,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | debug_flag, 0, "dagi_sky_visibility_probabilities");
    debug("SkyVis: selectedProbesBufferSize: %d", selectedProbesBufferSize);
    debug("SkyVis: dagi_sky_visibility_probabilities buffer size: %d", DAGI_MAX_SKY_VIS_CLIPS * 2 + 1);
  }
  dagi_sky_vis_indirect_buffer.close();
  if (temporalProbesBufferSize)
  {
    dagi_sky_vis_indirect_buffer = dag::create_sbuffer(sizeof(uint32_t), 3,
      SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_UA_INDIRECT | debug_flag, 0, "dagi_sky_vis_indirect_buffer");
  }
}

void SkyVisibility::setReplicateToIrradiance(bool on)
{
  if (on == replicateToIrradiance)
    return;

  if (on && !replicateToIrradiance)
  {
    set_irradiance_grid_params(clipW, clipD, clipmap.size(), 2);
    set_irradiance_grid_textures(BAD_TEXTUREID, dagi_sky_visibility_sph.getTexId());
    setClipmapVars();
  }
  replicateToIrradiance = on;
}

void SkyVisibility::resetClipmap(uint32_t clips)
{
  initVars();
  validHistory = false;
  clipmap.clear();
  clipmap.resize(clips);
  temporalFrames.clear();
  temporalFrames.resize(clips);
  for (int i = 0; i < DAGI_MAX_SKY_VIS_CLIPS; ++i)
    ShaderGlobal::set_int4(dagi_sky_vis_clipmap_lt_coordVarId[i], -100000, 10000, 100000, bitwise_cast<uint32_t>(1e9f));
}

void SkyVisibility::init(uint32_t w_, uint32_t d_, uint32_t clips_, float probe0, uint32_t frames_to_update_clip, bool replicateTo)
{
  replicateToIrradiance = replicateTo;
  const bool sameClipmap = clipW == w_ && clipD == d_ && clipmap.size() == clips_;
  if (sameClipmap && probe0 == probeSize0)
    return initTemporal(frames_to_update_clip);
  resetClipmap(clips_);
  probeSize0 = probe0;
  if (!sameClipmap)
    initClipmap(w_, d_, clips_);
  initTemporal(frames_to_update_clip);

  const float fullAtlasDepth = float((clipD + 2) * clipmap.size());
  const float clipInAtlasPart = float(clipD) / fullAtlasDepth;
  const float clipWithBorderInAtlasPart = float(clipD + 2) / fullAtlasDepth;

  ShaderGlobal::set_color4(dagi_sky_vis_atlas_decodeVarId, clipInAtlasPart, clipWithBorderInAtlasPart, 1. / fullAtlasDepth, 0);
}

void SkyVisibility::updatePos(const Point3 &world_pos, bool update_all)
{
  if (gi_sky_vis_update_all)
  {
    update_all = true;
    validHistory = false;
  }
  initHistory();

  StaticTab<eastl::pair<int, Clip>, DAGI_MAX_SKY_VIS_CLIPS> updatedClipmaps;
  for (int i = clipmap.size() - 1; i >= 0; --i)
  {
    auto &clip = clipmap[i];
    const IPoint3 lt = getNewClipLT(i, world_pos);
    const IPoint3 move = lt - clip.lt, absMove = abs(move);
    const float probeSize = get_probe_size(i);
    const int maxMove = max(max(absMove.x, absMove.y), absMove.z);
    if (maxMove > DAGI_SKY_VIS_MOVE_THRESHOLD || clip.probeSize != probeSize)
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
      updatedClipmaps.emplace_back(i, Clip{clip.lt + useMove, probeSize});
      if (!update_all)
        break;
    }
  }

  DA_PROFILE_GPU;
  for (auto &clip : updatedClipmaps)
    updateClip(clip.first, clip.second.lt, clip.second.probeSize, &clip == &updatedClipmaps.back());

  if (gi_sky_vis_update_temporal)
    updateTemporal(updatedClipmaps.empty());

  auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
  d3d::resource_barrier({dagi_sky_visibility_sph.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
}

void SkyVisibility::drawDebug(int debug_type)
{
  if (!drawDebugAllProbes.shader)
    drawDebugAllProbes.init("dagi_sky_vis_draw_debug", NULL, 0, "dagi_sky_vis_draw_debug");
  if (!drawDebugAllProbes.shader)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_sky_vis_debug_typeVarId, debug_type);
  drawDebugAllProbes.shader->setStates(0, true);
  d3d::setvsrc(0, 0, 0);
  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW, clipW * clipW * clipD);
}
