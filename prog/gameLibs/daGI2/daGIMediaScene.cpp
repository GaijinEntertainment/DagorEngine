// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_carray.h>
#include <EASTL/string.h>
#include <generic/dag_staticTab.h>
#include <util/dag_convar.h>
#include <render/subtract_ibbox3.h>
#include <perfMon/dag_statDrv.h>
#include "daGIMediaScene.h"
#include "mediaScene/dagi_media_scene.hlsli"

CONSOLE_INT_VAL("gi", gi_media_scene_debug, 0, 0, 2);
CONSOLE_INT_VAL("gi", gi_media_scene_update_from_gbuf_speed_min, 64, 64, 32 << 10);
CONSOLE_INT_VAL("gi", gi_media_scene_update_from_gbuf_speed_count, 2048, 0, 256 << 10);
CONSOLE_BOOL_VAL("gi", gi_media_scene_update_from_gbuf, true);

#define GLOBAL_VARS_LIST                  \
  VAR(dagi_media_scene_update_count)      \
  VAR(dagi_media_scene_clipmap_sizei)     \
  VAR(dagi_media_scene_clipmap_sizei_np2) \
  VAR(dagi_media_scene_update_sz_coord)   \
  VAR(dagi_media_scene_update_lt_coord)   \
  VAR(dagi_media_scene_atlas_decode)      \
  VAR(dagi_media_scene_samplerstate)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

static ShaderVariableInfo dagi_media_scene_clipmap_lt_coordVarId[DAGI_MAX_MEDIA_SCENE_CLIPS];

void DaGIMediaScene::setClipVars(int clip_no) const
{
  auto &clip = clipmap[clip_no];
  ShaderGlobal::set_int4(dagi_media_scene_clipmap_lt_coordVarId[clip_no], clip.lt.x, clip.lt.y, clip.lt.z,
    bitwise_cast<uint32_t>(clip.voxelSize));
#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(clipW) || !is_pow_of2(clipD))
  {
    IPoint4 l = dagi_media_scene_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, clipW, clipD);
    }
  }
#endif
}

void DaGIMediaScene::initVars()
{
  if (dagi_media_scene_reset_cs)
    return;
  eastl::string str;
  for (int i = 0; i < DAGI_MAX_MEDIA_SCENE_CLIPS; ++i)
  {
    str.sprintf("dagi_media_scene_clipmap_lt_coord_%d", i);
    dagi_media_scene_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
  }
#define CS(a) a.reset(new_compute_shader(#a))
  CS(dagi_media_scene_reset_cs);
  CS(dagi_media_scene_from_gbuf_cs);
  CS(dagi_media_toroidal_movement_cs);
}

void DaGIMediaScene::init(uint32_t w, uint32_t d, uint32_t media_scene_clips, float voxel0)
{
  media_scene_clips = max<int>(media_scene_clips, 1);
  if (w == clipW && d == clipD && clipmap.size() == media_scene_clips && voxel0 == voxelSize0)
    return;
  initVars();
  debug("daGI: init media scene res %dx%dx%d, clips %d, voxel %f", w, w, d, media_scene_clips, voxel0);
  clipW = w;
  clipD = d;
  voxelSize0 = voxel0;
  clipmap.resize(media_scene_clips);
  fullAtlasResD = (clipD + 2) * clipmap.size();
  dagi_media_scene.close();
  // TEXFMT_R11G11B10F
  dagi_media_scene = dag::create_voltex(clipW, clipW, fullAtlasResD, TEXCF_UNORDERED | TEXFMT_A16B16G16R16F, 1, "dagi_media_scene");
  dagi_media_scene->disableSampler();
  dagi_media_scene_samplerstateVarId.set_sampler(d3d::request_sampler({}));
  ShaderGlobal::set_int4(dagi_media_scene_clipmap_sizeiVarId, clipW, clipD, clipmap.size(), clipD + 2);

  constexpr int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(dagi_media_scene_clipmap_sizei_np2VarId, clipW, clipD, is_pow_of2(clipW) ? 0 : (max_pos / clipW) * clipW,
    is_pow_of2(clipD) ? 0 : (max_pos / clipD) * clipD);

  const float clipInAtlasPart = float(clipD) / fullAtlasResD;
  const float clipWithBorderInAtlasPart = float(clipD + 2) / fullAtlasResD;

  ShaderGlobal::set_color4(dagi_media_scene_atlas_decodeVarId, clipInAtlasPart, clipWithBorderInAtlasPart, 1.f / fullAtlasResD, 0);
  afterReset();
}

bool DaGIMediaScene::updateClip(uint32_t clip_no, const IPoint3 &lt, float voxelSize)
{
  DA_PROFILE_GPU;
  auto &clip = clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 sz(clipW, clipW, clipD);
  const IPoint3 oldLt = (clip.voxelSize != voxelSize) ? clip.lt - sz * 2 : clip.lt;
  if (oldLt == lt)
    return false;
  // debug("sky clip %d: %@->%@ %f %f", clip_no, oldLt, lt, clip.probeSize, newProbeSize);
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt = move_box_toroidal(lt, oldLt, sz, changedSpan);
  clip.lt = lt;
  clip.voxelSize = voxelSize;

  setClipVars(clip_no);

  for (int ui = 0; ui < changedCnt; ++ui)
  {
    ShaderGlobal::set_int4(dagi_media_scene_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, clip_no);
    const IPoint3 updateSize = changed[ui].width();
    ShaderGlobal::set_int4(dagi_media_scene_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
      bitwise_cast<uint32_t>(clip.voxelSize));
    dagi_media_toroidal_movement_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1);

    d3d::resource_barrier(
      {dagi_media_scene.getVolTex(), ui < changedCnt - 1 ? RB_NONE : RB_FLUSH_UAV | RB_STAGE_COMPUTE | RB_SOURCE_STAGE_COMPUTE, 0, 0});
  }
  return true;
}

void DaGIMediaScene::updatePos(const Point3 &world_pos, bool update_all)
{
  initHistory();

  StaticTab<eastl::pair<int, Clip>, DAGI_MAX_MEDIA_SCENE_CLIPS> updatedClipmaps;
  for (int i = clipmap.size() - 1; i >= 0; --i)
  {
    auto &clip = clipmap[i];
    const IPoint3 lt = getNewClipLT(i, world_pos);
    const IPoint3 move = lt - clip.lt, absMove = abs(move);
    const float voxelSize = get_voxel_size(i);
    const int maxMove = max(max(absMove.x, absMove.y), absMove.z);
    enum
    {
      MOVE_THRESHOLD = 1
    };
    if (maxMove > MOVE_THRESHOLD || clip.voxelSize != voxelSize)
    {
      IPoint3 useMove{0, 0, 0};
      const Point3 relMove = div(point3(absMove), Point3(clipW, clipW, clipD));
      if (clip.voxelSize != voxelSize ||                      // everything has changed
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
      updatedClipmaps.emplace_back(i, Clip{clip.lt + useMove, voxelSize});
      if (!update_all)
        break;
    }
  }

  DA_PROFILE_GPU;
  for (auto &clip : updatedClipmaps)
    updateClip(clip.first, clip.second.lt, clip.second.voxelSize);

  auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;
  d3d::resource_barrier({dagi_media_scene.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
}

void DaGIMediaScene::afterReset()
{
  for (int i = 0, ie = clipmap.size(); i < ie; ++i)
  {
    clipmap[i] = Clip();
    setClipVars(i);
  }
  cleared = false;
}

void DaGIMediaScene::initHistory()
{
  if (cleared)
    return;
  afterReset();
  if (dagi_media_scene_reset_cs)
  {
    DA_PROFILE_GPU;
    dagi_media_scene_reset_cs->dispatchThreads(clipW, clipW, fullAtlasResD);
    d3d::resource_barrier({dagi_media_scene.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  }
  cleared = true;
}

void DaGIMediaScene::rbNone() { d3d::resource_barrier({dagi_media_scene.getVolTex(), RB_NONE, 0, 0}); }

void DaGIMediaScene::rbFinish()
{
  d3d::resource_barrier(
    {dagi_media_scene.getVolTex(), RB_RO_SRV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX, 0, 0});
}


void DaGIMediaScene::updateFromGbuf()
{
  if (!dagi_media_scene_from_gbuf_cs || !gi_media_scene_update_from_gbuf || temporalSpeed <= 0)
    return;
  DA_PROFILE_GPU;
  const uint32_t pixelCount = gi_media_scene_update_from_gbuf_speed_min + gi_media_scene_update_from_gbuf_speed_count * temporalSpeed;
  ShaderGlobal::set_int4(dagi_media_scene_update_countVarId, pixelCount, gbuf_update_frame++, 0, 0);
  dagi_media_scene_from_gbuf_cs->dispatchThreads(pixelCount, 1, 1);

  // fixme: this barrier is not needed here. Instead, we need to call it _before update (from gbuf or on move)
  d3d::resource_barrier({dagi_media_scene.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
}

void DaGIMediaScene::debugRender()
{
  if (!gi_media_scene_debug)
    return;
  if (!dagi_media_scene_debug.getElem())
    dagi_media_scene_debug.init("dagi_media_scene_debug");
  if (!dagi_media_scene_trace_debug.getElem())
    dagi_media_scene_trace_debug.init("dagi_media_scene_trace_debug");

  DA_PROFILE_GPU;
  if (gi_media_scene_debug == 2)
    dagi_media_scene_trace_debug.render();
  else if (gi_media_scene_debug == 1)
    dagi_media_scene_debug.render();
}