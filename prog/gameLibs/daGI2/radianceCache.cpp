// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_carray.h>
#include <util/dag_convar.h>
#include <render/spheres_consts.hlsli>
#include <EASTL/bitvector.h>
#include <EASTL/string.h>
#include <util/dag_bitwise_cast.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/subtract_ibbox3.h>
#include <perfMon/dag_statDrv.h>
#include "radianceCache.h"
#include "radianceCache/radianace_cache_indirect.hlsli"
#include "radianceCache/radianace_cache_consts.hlsli"
#include <shaders/dag_shaderVariableInfo.h>
#include <3d/dag_ringCPUQueryLock.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

#define GLOBAL_VARS_LIST                              \
  VAR(radiance_cache_atlas_sizei)                     \
  VAR(radiance_cache_clipmap_sizei)                   \
  VAR(radiance_cache_clipmap_sizei_np2)               \
  VAR(radiance_cache_clipmap_update_clip_to_lt_coord) \
  VAR(radiance_cache_current_frame)                   \
  VAR(radiance_cache_temporal_recalc_params)          \
  VAR(radiance_cache_temporal_recalc_params2)         \
  VAR(radiance_cache_irradiance_frame)                \
  VAR(radiance_cache_clipmap_update_lt_coord)         \
  VAR(radiance_cache_clipmap_update_sz_coord)

static ShaderVariableInfo radiance_cache_clipmap_lt_coordVarId[MAX_RADIANCE_CACHE_CLIPS];
static ShaderVariableInfo radiance_cache_clipmap_lt_prev_coordVarId[MAX_RADIANCE_CACHE_CLIPS];
#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

void RadianceCache::initVars()
{
  if (calc_radiance_cache_new_cs)
    return;
  eastl::string str;
  for (int i = 0; i < MAX_RADIANCE_CACHE_CLIPS; ++i)
  {
    str.sprintf("radiance_cache_clipmap_lt_coord_%d", i);
    radiance_cache_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
    str.sprintf("radiance_cache_clipmap_lt_prev_coord_%d", i);
    radiance_cache_clipmap_lt_prev_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
  }
#define CS(a) a.reset(new_compute_shader(#a))
  CS(rd_calc_irradiance_cache_new_cs);
  CS(rd_calc_irradiance_cache_temporal_cs);

  CS(radiance_cache_create_dispatch_indirect_cs);

  CS(radiance_cache_free_unused_after_movement_cs);
  CS(clear_radiance_cache_clipmap_indirection_cs);

  CS(clear_radiance_cache_positions_cs);
  CS(radiance_cache_clear_new_cs);
  CS(mark_used_radiance_cache_probes_cs);
  CS(free_unused_radiance_cache_probes_cs);
  CS(allocate_radiance_cache_probes_cs);

  CS(calc_radiance_cache_new_cs);
  CS(calc_temporal_radiance_cache_cs);
  CS(select_temporal_radiance_cache_cs);
  if (supportUnorderedLoad)
    CS(filter_temporal_radiance_cache_cs);

  CS(radiance_cache_toroidal_movement_cs);
#undef CS
}

const uint32_t debug_flag = 0; // SBCF_USAGE_READ_BACK;

static inline void calc_atlas(int probes_cnt, int &probes_w, int &probes_h)
{
  probes_w = (max<int>(8, sqrtf(probes_cnt)) + 3) & ~3;
  probes_h = (probes_cnt + probes_w - 1) / probes_w;
}

void RadianceCache::initAtlas(int probes_min, int probes_max)
{
  // atlas
  if (!get_fake_radiance_cache_res_cs)
    get_fake_radiance_cache_res_cs.reset(new_compute_shader("get_fake_radiance_cache_res_cs"));
  const int probe_oct_size = get_fake_radiance_cache_res_cs ? get_fake_radiance_cache_res_cs->getThreadGroupSizes()[0] : 8;
  if (minAtlasSizeInProbes == probes_min && maxAtlasSizeInProbes == probes_max && probe_oct_size == probeSize)
    return;
  minAtlasSizeInProbes = probes_min;
  maxAtlasSizeInProbes = probes_max;

  if (minAtlasSizeInProbes != maxAtlasSizeInProbes && !readBackData)
    initReadBack();
  int w, h;
  calc_atlas(minAtlasSizeInProbes, w, h);
  if ((minAtlasSizeInProbes == maxAtlasSizeInProbes && (atlasSizeInProbesH * atlasSizeInProbesW != w * h)) ||
      probe_oct_size != probeSize)
    initAtlasInternal(w, h, probe_oct_size);
}

void RadianceCache::initAtlasInternal(int probes_w, int probes_h, int probe_oct_size)
{
  if (probe_oct_size == probeSize && atlasSizeInProbesW == probes_w && atlasSizeInProbesH == probes_h)
    return;
  static uint32_t texfmt = TEXFMT_R11G11B10F; // TEXFMT_A16B16G16R16F;
  supportUnorderedLoad = d3d::get_texformat_usage(TEXCF_UNORDERED | texfmt) & d3d::USAGE_UNORDERED_LOAD;

  initVars();
  probeSize = probe_oct_size;
  atlasSizeInProbesW = probes_w;
  atlasSizeInProbesH = probes_h;
  debug("init radiance cache %dx%d with probe of %d^2", atlasSizeInProbesW, atlasSizeInProbesH, probe_oct_size);
  atlasSizeInTexelsW = atlasSizeInProbesW * probeSize;
  atlasSizeInTexelsH = atlasSizeInProbesH * probeSize;
  current_radiance_cache.close();
  current_radiance_cache_hit_distance.close();
  current_radiance_cache =
    dag::create_tex(NULL, atlasSizeInTexelsW, atlasSizeInTexelsH, TEXCF_UNORDERED | texfmt, 1, "current_radiance_cache");
  current_radiance_cache_hit_distance = dag::create_tex(NULL, atlasSizeInTexelsW, atlasSizeInTexelsH, TEXCF_UNORDERED | TEXFMT_R16F, 1,
    "current_radiance_cache_hit_distance");


  radiance_cache_positions.close();
  radiance_cache_positions = dag::create_sbuffer(sizeof(uint32_t), atlasSizeInProbesW * atlasSizeInProbesH,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "radiance_cache_positions");

  radiance_cache_selected_temporal_probes.close();
  // fixme: this actually can be smaller than maximum size. we only need 1 + atlasSizeInProbesW*atlasSizeInProbesH/rd_update_in_frames
  temporalProbesPerFrameCount = atlasSizeInProbesW * atlasSizeInProbesH;
  radiance_cache_selected_temporal_probes = dag::create_sbuffer(sizeof(uint32_t), 3 + temporalProbesPerFrameCount,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES | SBCF_UA_INDIRECT, 0,
    "radiance_cache_selected_temporal_probes");

  radiance_cache_age.close();
  radiance_cache_age = dag::create_sbuffer(sizeof(uint32_t), atlasSizeInProbesW * atlasSizeInProbesH * 2,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "radiance_cache_age");
  free_radiance_cache_indices_list.close();
  free_radiance_cache_indices_list = dag::create_sbuffer(sizeof(uint32_t), atlasSizeInProbesW * atlasSizeInProbesH + 1,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "free_radiance_cache_indices_list");
  // not more than total cache size
  new_radiance_cache_probes_needed.close();
  new_radiance_cache_probes_needed = dag::create_sbuffer(sizeof(uint32_t), atlasSizeInProbesW * atlasSizeInProbesH + 1,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "new_radiance_cache_probes_needed");
  radiance_cache_indirect_buffer.close();
  radiance_cache_indirect_buffer = dag::create_sbuffer(sizeof(uint32_t), RADIANCE_CACHE_INDIRECT_BUFFER_SIZE,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_UA_INDIRECT, 0, "radiance_cache_indirect_buffer");
  validHistory = false;
  frame = 0;
  ShaderGlobal::set_int4(radiance_cache_atlas_sizeiVarId, atlasSizeInProbesW, atlasSizeInProbesH, probeSize,
    atlasSizeInProbesW * atlasSizeInProbesH);
  if (readBackData)
    initReadBack();
}

RadianceCache::~RadianceCache() = default;
RadianceCache::RadianceCache() = default;

void RadianceCache::closeReadBack()
{
  if (minAtlasSizeInProbes == maxAtlasSizeInProbes)
    resetReadBack();
}
void RadianceCache::resetReadBack()
{
  readBackData.reset();
  exhaustedCacheFramesCnt = freeCacheFramesCnt = 0;
  latestFreeAmount = ~0u;
}

void RadianceCache::initReadBack()
{
  closeReadBack();
  readBackData.reset(new RingCPUBufferLock);
  readBackData->init(sizeof(uint32_t), 1, 6, "free_radiance_cache_counter", SBCF_UA_STRUCTURED_READBACK, 0, false);
}

void RadianceCache::adjustAtlasSize()
{
  constexpr uint32_t maxFramesTotolerateExhaustedCache = 16, maxFramesTotolerateAlmostFreeCache = 1024;
  latestFreeAmount = ~0u;
  const uint32_t currentTotalCacheProbes = atlasSizeInProbesH * atlasSizeInProbesW;
  uint32_t freeAmount = 0;
  if (readBackCounter(freeAmount))
  {
    // debug("read freeAmount = %d", freeAmount);
    latestFreeAmount = freeAmount;
    if (freeAmount <= currentTotalCacheProbes) // sanity
    {
      const bool isExhausted = (freeAmount == 0) && currentTotalCacheProbes < maxAtlasSizeInProbes;
      const bool isAlmostFree = freeAmount > currentTotalCacheProbes * 3 / 4 && currentTotalCacheProbes > minAtlasSizeInProbes;
      if (isExhausted)
      {
        if (++exhaustedCacheFramesCnt >= maxFramesTotolerateExhaustedCache)
        {
          int w, h;
          calc_atlas(min<int>(maxAtlasSizeInProbes, (currentTotalCacheProbes + 1) * 3 / 2), w, h);
          initAtlasInternal(w, h, probeSize);
          exhaustedCacheFramesCnt = 0;
        }
      }
      else
        exhaustedCacheFramesCnt = 0;
      if (isAlmostFree)
      {
        if (++freeCacheFramesCnt >= maxFramesTotolerateAlmostFreeCache)
        {
          // debug("we need %d/%d cache only for %d frames, freeing", currentTotalCacheProbes - freeAmount, currentTotalCacheProbes,
          // freeCacheFramesCnt);
          int w, h;
          calc_atlas(max<int>(minAtlasSizeInProbes, (currentTotalCacheProbes - freeAmount) * 4 / 3), w, h);
          initAtlasInternal(w, h, probeSize);
          freeCacheFramesCnt = 0;
        }
      }
      else
        freeCacheFramesCnt = 0;
    }
  }
}

bool RadianceCache::readBackCounter(uint32_t &counter)
{
  if (!readBackData)
    return false;
  uint32_t cFrame;
  int stride;
  bool ret = false;
  if (auto counterData = readBackData->lock(stride, cFrame, true, 256))
  {
    counter = *(uint32_t *)counterData;
    ret = true;
    readBackData->unlock();
  }
  if (auto target = (Sbuffer *)readBackData->getNewTarget(cFrame))
  {
    free_radiance_cache_indices_list.getBuf()->copyTo(target, 0, 0, 4);
    readBackData->startCPUCopy();
  }
  return ret;
}

void RadianceCache::initIrradiance()
{
  radiance_cache_irradiance.close();
  radiance_cache_irradiance = dag::create_tex(NULL, atlasSizeInProbesW * probeSize, atlasSizeInProbesH * probeSize,
    TEXCF_UNORDERED | TEXFMT_R11G11B10F, 1, "radiance_cache_irradiance");
}


void RadianceCache::fixup_clipmap_settings(uint16_t &xz_res, uint16_t &y_res, uint8_t &clips)
{
  if constexpr (!RADIANCE_CACHE_SUPPORTS_NON_POW2_CLIPMAP)
  {
    xz_res = get_bigger_pow2(xz_res);
    y_res = get_bigger_pow2(y_res);
  }
  xz_res = clamp<uint16_t>(xz_res, 16, 256);
  y_res = clamp<uint16_t>(y_res, 16, 256);
  clips = clamp<uint8_t>(clips, 1, MAX_RADIANCE_CACHE_CLIPS);
}

void RadianceCache::initClipmap(uint16_t w, uint16_t d, uint8_t clips, float probe_size_0)
{
  fixup_clipmap_settings(w, d, clips);
  if (clipmap.size() == clips && clipW == w && clipD == d && probeSize0 == probe_size_0)
    return;
  initVars();
  clipW = w;
  clipD = d;
  probeSize0 = probe_size_0;
  clipmap.clear();
  clipmap.resize(clips);
  validHistory = false;
  frame = 0;
  used_radiance_cache_mask.close();
  radiance_cache_indirection_clipmap.close();
  radiance_cache_indirection_clipmap =
    dag::buffers::create_ua_sr_structured(sizeof(uint32_t), (w * w * d * clips + 3) & ~3, "radiance_cache_indirection_clipmap");
  used_radiance_cache_mask = dag::create_sbuffer(sizeof(uint32_t), ((w * w * d * clips + 31) / 32 + 3) & ~3,
    SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "used_radiance_cache_mask");
  initIrradiance();
  debug("radiance cache indirection res=%dx%dx%d clips=%d probe0 = %f dist %f", w, d, w, clips, getProbeSizeClip(0),
    0.5f * w * getProbeSizeClip(clips - 1));

  ShaderGlobal::set_int4(radiance_cache_clipmap_sizeiVarId, clipW, clipD, clipmap.size(), clipW * clipW * clipD * clipmap.size());
  constexpr int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(radiance_cache_clipmap_sizei_np2VarId, clipW, clipD, is_pow_of2(clipW) ? 0 : (max_pos / clipW) * clipW,
    is_pow_of2(clipD) ? 0 : (max_pos / clipD) * clipD);
}

enum
{
  THRESHOLD = 1
};

bool RadianceCache::updateClip(uint32_t clip_no)
{
  DA_PROFILE_GPU;
  auto &clip = clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 oldLt = clip.prevLt;
  const IPoint3 sz(clipW, clipW, clipD);
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt =
    move_box_toroidal(clip.lt, (clip.probeSize != clip.prevProbeSize) ? clip.lt - sz * 2 : oldLt, sz, changedSpan);

  ShaderGlobal::set_int4(radiance_cache_clipmap_update_clip_to_lt_coordVarId, clip.lt.x, clip.lt.y, clip.lt.z, clip_no);
  d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_indirection_clipmap.getBuf());
  for (int ui = 0; ui < changedCnt; ++ui)
  {
    ShaderGlobal::set_int4(radiance_cache_clipmap_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, 0);
    const IPoint3 updateSize = changed[ui].width();
    ShaderGlobal::set_int4(radiance_cache_clipmap_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
      updateSize.x * updateSize.y);
    radiance_cache_toroidal_movement_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1); // basically clear cache
                                                                                                            // and indirection
    d3d::resource_barrier({free_radiance_cache_indices_list.getBuf(), RB_NONE});
    d3d::resource_barrier({radiance_cache_age.getBuf(), RB_NONE});
    d3d::resource_barrier({radiance_cache_indirection_clipmap.getBuf(), RB_NONE});
    d3d::resource_barrier({radiance_cache_positions.getBuf(), RB_NONE});
  }

  return true;
}

void RadianceCache::set_clip_vars(int clip_no) const
{
  auto &clip = clipmap[clip_no];
  ShaderGlobal::set_int4(radiance_cache_clipmap_lt_coordVarId[clip_no], clip.lt.x, clip.lt.y, clip.lt.z,
    bitwise_cast<uint32_t>(clip.probeSize));
  ShaderGlobal::set_int4(radiance_cache_clipmap_lt_prev_coordVarId[clip_no], clip.prevLt.x, clip.prevLt.y, clip.prevLt.z,
    bitwise_cast<uint32_t>(clip.prevProbeSize));
#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(clipW) || !is_pow_of2(clipD))
  {
    IPoint4 l = radiance_cache_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, clipW, clipD);
    }
  }
#endif
}

int RadianceCache::validateCache(const char *step)
{
  if (!debug_flag)
    return 0;
  int freeCnt = 0; // -V779 unreachable code
  eastl::bitvector<> freeCache;
  freeCache.resize(atlasSizeInProbesW * atlasSizeInProbesH);
  uint32_t *fData;
  if (free_radiance_cache_indices_list->lock(0, (atlasSizeInProbesW * atlasSizeInProbesH + 1) * 4, (void **)&fData, VBLOCK_READONLY))
  {
    freeCnt = *fData;
    uint32_t *cacheData;
    auto decodePosition = [&](uint32_t p) {
      return IPoint4(p % clipW, (p / clipW) % clipW, (p / (clipW * clipW)) % clipD, p / (clipW * clipW * clipD));
    };
    if (radiance_cache_positions->lock(0, atlasSizeInProbesW * atlasSizeInProbesH * 4, (void **)&cacheData, VBLOCK_READONLY))
    {
      for (int j = 0; j < freeCnt; ++j)
      {
        const uint32_t freePos = fData[j + 1];
        if (freePos >= atlasSizeInProbesW * atlasSizeInProbesH)
        {
          logerr("%s: invalid free %d at %d", step, freePos, j + 1);
        }
        else
        {
          if (freePos >= freeCache.size())
            logerr("%s: free index %d at %d is not valid", step, freePos, j + 1);
          else
          {
            if (cacheData[freePos] != ~0u)
              logerr("%s: free index %d at %d is not free %@", step, freePos, j + 1, decodePosition(cacheData[freePos]));
            freeCache.set(freePos, true);
          }
        }
      }
      for (int i = 0; i < freeCache.size(); ++i)
        if (!freeCache[i])
          if (cacheData[i] == ~0u)
            logerr("%s: occupied index %d is free %@", step, i, decodePosition(cacheData[i]));
      radiance_cache_positions->unlock();
    }
    free_radiance_cache_indices_list->unlock();
  }
  return freeCnt;
}

bool RadianceCache::updateClipMapPosition(const Point3 &world_pos, bool updateAll)
{
  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE}); // from previous
                                                                                                                   // frame
  if (!validHistory)
  {
    TIME_D3D_PROFILE(clear_history);
    d3d::resource_barrier({free_radiance_cache_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_positions.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 1, free_radiance_cache_indices_list.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 2, radiance_cache_age.getBuf());
    clear_radiance_cache_positions_cs->dispatchThreads(atlasSizeInProbesW * atlasSizeInProbesH + 1, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 2, nullptr);

    d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_indirection_clipmap.getBuf());
    clear_radiance_cache_clipmap_indirection_cs->dispatchThreads((clipW * clipW * clipD * clipmap.size() + 3) / 4, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);

    d3d::resource_barrier({free_radiance_cache_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_positions.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_indirection_clipmap.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

    validateCache("after clear history");
  }
  d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_indirection_clipmap.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 1, radiance_cache_positions.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 2, free_radiance_cache_indices_list.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 3, radiance_cache_age.getBuf());

  StaticTab<int, MAX_RADIANCE_CACHE_CLIPS> updatedClipmaps;
  for (int i = clipmap.size() - 1; i >= 0; --i)
  {
    auto &clip = clipmap[i];
    const IPoint3 lt = getNewClipLT(i, world_pos);
    const IPoint3 move = lt - clip.lt, absMove = abs(move);
    const float clipProbeSize = getProbeSizeClip(i);
    if (absMove.x > THRESHOLD || absMove.y > THRESHOLD || absMove.z > THRESHOLD || clip.probeSize != clipProbeSize)
    {
      clip.prevProbeSize = clip.probeSize; // fixme: temp variable
      clip.prevLt = clip.lt;               // fixme: temp variable
      clip.lt = lt;
      clip.probeSize = clipProbeSize;
      updatedClipmaps.push_back(i);
      set_clip_vars(i);
      if (!updateAll)
        break;
    }
  }

  if (!updatedClipmaps.empty() && validHistory) // this is faster unless movement is one clip, one slice
  {
    TIME_D3D_PROFILE(radiance_cache_update_clipmap);
    if (updatedClipmaps.size() > 1)
    {
      TIME_D3D_PROFILE(radiance_cache_update_anew);
      // clear_radiance_cache_clipmap_indirection_cs->dispatchThreads((clipW*clipW*clipD*clipmap.size()+3)/4,1,1);
      radiance_cache_free_unused_after_movement_cs->dispatchThreads(atlasSizeInProbesW * atlasSizeInProbesH, 1, 1);
    }
    else
    {
      TIME_D3D_PROFILE(radiance_cache_update_toroidal);
      for (auto clip_no : updatedClipmaps)
        updateClip(clip_no);
    }
    // d3d::resource_barrier({free_radiance_cache_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_indirection_clipmap.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({radiance_cache_positions.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  }
  for (auto clip_no : updatedClipmaps)
  {
    clipmap[clip_no].prevLt = clipmap[clip_no].lt; // fixme: this is by nature temporal variable, it is only used inside
                                                   // updateClipMapPosition
    clipmap[clip_no].prevProbeSize = clipmap[clip_no].probeSize;
  }
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
  validHistory = true;
  return !updatedClipmaps.empty();
}

void RadianceCache::start_marking()
{
  if (!validHistory)
    return;
  adjustAtlasSize();
  G_ASSERT_RETURN(state == MarkProcessState::Finished, );
  ShaderGlobal::set_int(radiance_cache_current_frameVarId, frame);
  const int freeInitial = validateCache("before mark");
  if (debug_flag)
    debug("we now have %d free cache probes", freeInitial);
  DA_PROFILE_GPU;
  {
    TIME_D3D_PROFILE(clear_new)
    d3d::set_rwbuffer(STAGE_CS, 0, new_radiance_cache_probes_needed.getBuf()); // clear counter
    d3d::set_rwbuffer(STAGE_CS, 1, used_radiance_cache_mask.getBuf());
    radiance_cache_clear_new_cs->dispatchThreads(((clipW * clipW * clipD * clipmap.size() + 31) / 32 + 3) / 4, 1, 1);
    d3d::resource_barrier({new_radiance_cache_probes_needed.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::resource_barrier({used_radiance_cache_mask.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  }
  state = MarkProcessState::Started;
}

void RadianceCache::mark_new_screen_probes(int spw, int sph) { mark_new_probes(*mark_used_radiance_cache_probes_cs, spw * sph, 1, 1); }

void RadianceCache::mark_new_probes(ComputeShaderElement &cs, int tw, int th, int tz)
{
  if (!validHistory)
    return;
  DA_PROFILE_GPU;
  G_ASSERT_RETURN(state == MarkProcessState::Started || state == MarkProcessState::Marked, );
  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE}); // from previous
                                                                                                                   // frame
  d3d::set_rwbuffer(STAGE_CS, 0, new_radiance_cache_probes_needed.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 1, used_radiance_cache_mask.getBuf());
  d3d::set_rwbuffer(STAGE_CS, 2, radiance_cache_age.getBuf());
  cs.dispatchThreads(tw, th, tz);
  d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
  d3d::set_rwbuffer(STAGE_CS, 2, nullptr);

  state = MarkProcessState::Marked;
}

void RadianceCache::finish_marking()
{
  if (!validHistory)
    return;
  G_ASSERT_RETURN(state == MarkProcessState::Marked, );
  DA_PROFILE_GPU;
  if (debug_flag)
  {
    int *nData;
    if (new_radiance_cache_probes_needed->lock(0, atlasSizeInProbesW * atlasSizeInProbesH * 4, (void **)&nData, VBLOCK_READONLY))
    {
      debug("total new probes needed %d", *nData);
      if (0)
        for (int i = 0, e = *nData; i < e; ++i)
        {
          uint32_t probe = nData[i + 1];
          debug("%d: %dx%dx%d clip %d", i, probe % clipW, (probe / clipW) % clipW, (probe / (clipW * clipW)) % clipD,
            probe / (clipW * clipW * clipD));
        }
      new_radiance_cache_probes_needed->unlock();
    }
  }
  d3d::resource_barrier({new_radiance_cache_probes_needed.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  {
    TIME_D3D_PROFILE(create_indirect)
    d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_indirect_buffer.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 1, radiance_cache_selected_temporal_probes.getBuf());
    radiance_cache_create_dispatch_indirect_cs->dispatch(1, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
    d3d::resource_barrier({radiance_cache_indirect_buffer.getBuf(), RB_RO_INDIRECT_BUFFER});
  }

  {
    TIME_D3D_PROFILE(free_unused_radiance_cache_probes)
    d3d::resource_barrier({used_radiance_cache_mask.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, free_radiance_cache_indices_list.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 1, radiance_cache_positions.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 2, radiance_cache_indirection_clipmap.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 3, radiance_cache_age.getBuf());
    free_unused_radiance_cache_probes_cs->dispatch_indirect(radiance_cache_indirect_buffer.getBuf(),
      RADIANCE_CACHE_FREE_UNUSED_INDIRECT_OFFSET * 4);
    // free_unused_radiance_cache_probes_cs->dispatchThreads(atlasSizeInProbesW*atlasSizeInProbesH, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
  }
  d3d::resource_barrier({radiance_cache_indirection_clipmap.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({free_radiance_cache_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({radiance_cache_positions.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({new_radiance_cache_probes_needed.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  const int freeAfterFree = validateCache("after free");
  if (debug_flag)
    debug("we now have free %d", freeAfterFree);
  {
    TIME_D3D_PROFILE(allocate_new_probes)
    d3d::set_rwbuffer(STAGE_CS, 0, free_radiance_cache_indices_list.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 1, radiance_cache_positions.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 2, radiance_cache_indirection_clipmap.getBuf());
    d3d::set_rwbuffer(STAGE_CS, 3, radiance_cache_age.getBuf());
    // dispatch indirect, not more than new_radiance_cache_probes_needed[0]
    allocate_radiance_cache_probes_cs->dispatch_indirect(radiance_cache_indirect_buffer.getBuf(),
      RADIANCE_CACHE_ALLOCATE_NEW_INDIRECT_OFFSET * 4);
    // allocate_radiance_cache_probes_cs->dispatchThreads(atlasSizeInProbesW*atlasSizeInProbesH, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 1, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
    d3d::set_rwbuffer(STAGE_CS, 3, nullptr);
  }

  const int freeAfterAllocate = validateCache("after allocate");
  if (debug_flag)
  {
    debug("we have allocated %d, %d total free", freeAfterFree - freeAfterAllocate, freeAfterAllocate);
  }

  d3d::resource_barrier({radiance_cache_positions.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({radiance_cache_indirection_clipmap.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  // fixme:dispatch indirect on refill mask from cache if there were nto enough space

  state = MarkProcessState::Finished;
}

void RadianceCache::setTemporalParams(uint32_t update_all_in_frames, uint32_t frames_to_keep_probe, uint32_t old_probes_update_freq)
{
  updateInFrames = clamp<uint32_t>(update_all_in_frames, 1, 256);
  probeFramesKeep = clamp<uint32_t>(frames_to_keep_probe, 1, 8);
  updateOldProbeFreq = clamp<uint32_t>(old_probes_update_freq, 1, 16);
}

void RadianceCache::calcAllProbes()
{
  DA_PROFILE_GPU;

  ShaderGlobal::set_int4(radiance_cache_temporal_recalc_paramsVarId, updateInFrames, probeFramesKeep, updateOldProbeFreq,
    temporalProbesPerFrameCount);
  ShaderGlobal::set_int4(radiance_cache_temporal_recalc_params2VarId, frame / updateInFrames, frame % updateInFrames,
    (frame / updateInFrames) / updateOldProbeFreq, (frame / updateInFrames) % updateOldProbeFreq);

  {
    TIME_D3D_PROFILE(select_temporal_probes)
    d3d::resource_barrier({radiance_cache_indirect_buffer.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    d3d::set_rwbuffer(STAGE_CS, 0, radiance_cache_selected_temporal_probes.getBuf());
    select_temporal_radiance_cache_cs->dispatchThreads(atlasSizeInProbesW * atlasSizeInProbesH, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
  }

  {
    TIME_D3D_PROFILE(new_probes)
    d3d::set_rwtex(STAGE_CS, 0, current_radiance_cache.getTex2D(), 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, current_radiance_cache_hit_distance.getTex2D(), 0, 0);
    calc_radiance_cache_new_cs->dispatch_indirect(radiance_cache_indirect_buffer.getBuf(),
      RADIANCE_TRACE_CALC_NEW_PROBES_INDIRECT_OFFSET * 4);
    // d3d::resource_barrier({current_radiance_cache.getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    // d3d::resource_barrier({current_radiance_cache_hit_distance.getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE |
    // RB_STAGE_COMPUTE, 0, 0});
    d3d::resource_barrier({current_radiance_cache.getTex2D(), RB_NONE, 0, 0});
    d3d::resource_barrier({current_radiance_cache_hit_distance.getTex2D(), RB_NONE, 0, 0});
  }

  // temporal recalc of probes
  {
    TIME_D3D_PROFILE(temporal_probes)
    d3d::resource_barrier(
      {radiance_cache_selected_temporal_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

    d3d::set_rwbuffer(STAGE_CS, 2, radiance_cache_age.getBuf());
    calc_temporal_radiance_cache_cs->dispatch_indirect(radiance_cache_selected_temporal_probes.getBuf(), 0);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::set_rwtex(STAGE_CS, 1, nullptr, 0, 0);
    d3d::set_rwbuffer(STAGE_CS, 2, nullptr);
    d3d::resource_barrier({current_radiance_cache.getTex2D(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
    // d3d::resource_barrier({current_radiance_cache.getTex2D(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE |
    // RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
    d3d::resource_barrier({current_radiance_cache_hit_distance.getTex2D(),
      RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }
  if (supportUnorderedLoad && filterWithDataRace && filter_temporal_radiance_cache_cs)
  {
    TIME_D3D_PROFILE(filter_temporal_probes)
    d3d::set_rwtex(STAGE_CS, 0, current_radiance_cache.getTex2D(), 0, 0);
    d3d::resource_barrier(
      {radiance_cache_selected_temporal_probes.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    filter_temporal_radiance_cache_cs->dispatch_indirect(radiance_cache_selected_temporal_probes.getBuf(), 0);
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({current_radiance_cache.getTex2D(),
      RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }
  calcIrradiance();

  constexpr uint32_t LATEST_AGE_FRAME = 0xFFFFFF; // fixme: same in shader
  if (++frame == LATEST_AGE_FRAME)
  {
    // fixme: wrap around current age, as it became invalid
    // this only happens once per 36 hours
    frame = 128;
  }
}

void RadianceCache::calcIrradiance()
{
  // fixme: history
  DA_PROFILE_GPU;
  {
    TIME_D3D_PROFILE(irradiance_cache);
    ShaderGlobal::set_int4(radiance_cache_irradiance_frameVarId, frame, frame & 31, frame != 0, 0);
    d3d::set_rwtex(STAGE_CS, 0, radiance_cache_irradiance.getTex2D(), 0, 0);
    // rd_calc_irradiance_cache_cs->dispatch(atlasSizeInProbesW, atlasSizeInProbesH, 1);
    {
      TIME_D3D_PROFILE(irradiance_new);
      rd_calc_irradiance_cache_new_cs->dispatch_indirect(radiance_cache_indirect_buffer.getBuf(),
        RADIANCE_TRACE_CALC_NEW_PROBES_INDIRECT_OFFSET * 4);
      d3d::resource_barrier({radiance_cache_irradiance.getTex2D(), RB_NONE, 0, 0});
    }
    {
      TIME_D3D_PROFILE(irradiance_temporal);
      rd_calc_irradiance_cache_temporal_cs->dispatch_indirect(radiance_cache_selected_temporal_probes.getBuf(), 0);
    }
    d3d::set_rwtex(STAGE_CS, 0, nullptr, 0, 0);
    d3d::resource_barrier({radiance_cache_irradiance.getTex2D(),
      RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
  }
  // temporal supersample
}

void RadianceCache::initDebug()
{
  drawDebugAllProbes.init("radiance_cache_probes_draw_debug", NULL, 0, "radiance_cache_probes_draw_debug");
  drawDebugIndirection.init("radiance_cache_indirection_draw_debug", NULL, 0, "radiance_cache_indirection_draw_debug");
}

void RadianceCache::drawDebug(int debug_type)
{
  if (!drawDebugAllProbes.shader)
    initDebug();
  if (!drawDebugAllProbes.shader)
    return;
  DA_PROFILE_GPU;
  const bool drawCache = (debug_type == 0);
  (drawCache ? drawDebugAllProbes.shader : drawDebugIndirection.shader)->setStates(0, true);
  d3d::setvsrc(0, 0, 0);

  d3d::resource_barrier({radiance_cache_age.getBuf(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE}); // only for debug
  d3d::draw_instanced(PRIM_TRILIST, 0, LOW_SPHERES_INDICES_TO_DRAW,
    drawCache ? atlasSizeInProbesW * atlasSizeInProbesH : clipmap.size() * clipW * clipW * clipD);
}
