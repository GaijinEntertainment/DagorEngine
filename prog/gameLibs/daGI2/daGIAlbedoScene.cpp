// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_carray.h>
#include <generic/dag_staticTab.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_info.h>
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/string.h>
#include <render/subtract_ibbox3.h>
#include "daGIAlbedoScene.h"
#include "albedoScene/dagi_albedo_scene.hlsli"

#include <EASTL/bitvector.h>

CONSOLE_INT_VAL("gi", gi_albedo_scene_debug, 0, 0, 2);
CONSOLE_BOOL_VAL("gi", gi_fix_albedo_scene_alpha, true);
CONSOLE_INT_VAL("gi", gi_albedo_scene_update_from_gbuf_speed_min, 128, 64, 32 << 10);
CONSOLE_INT_VAL("gi", gi_albedo_scene_update_from_gbuf_speed_count, 8192, 0, 256 << 10);

#define GLOBAL_VARS_LIST                                 \
  VAR(dagi_albedo_atlas_reg_no)                          \
  VAR(dagi_albedo_indirection__free_indices_list_reg_no) \
  VAR(dagi_albedo_clipmap_update_old_lt)                 \
  VAR(dagi_albedo_scene_stable_update)                   \
  VAR(dagi_albedo_scene_update_count)                    \
  VAR(dagi_albedo_clipmap_update_lt_coord)               \
  VAR(dagi_albedo_clipmap_update_sz_coord)               \
  VAR(dagi_albedo_clipmap_update_box_lt)                 \
  VAR(dagi_albedo_clipmap_update_box_sz)                 \
  VAR(dagi_albedo_atlas_sizei)                           \
  VAR(dagi_albedo_update_ofs)                            \
  VAR(dagi_albedo_clipmap_sizei)                         \
  VAR(dagi_albedo_clipmap_sizei_np2)                     \
  VAR(dagi_albedo_inv_atlas_size_blocks_texels)          \
  VAR(dagi_albedo_internal_block_size_tc_border)         \
  VAR(dagi_albedo_atlas_samplerstate)

static ShaderVariableInfo dagi_albedo_clipmap_lt_coordVarId[DAGI_MAX_ALBEDO_CLIPS];
#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
#undef VAR

uint16_t DaGIAlbedoScene::block_resolution() { return DAGI_ALBEDO_INTERNAL_BLOCK_SIZE; }

float DaGIAlbedoScene::clip_block_size(float voxel0Size, uint8_t clip)
{
  return (DAGI_ALBEDO_INTERNAL_BLOCK_SIZE * voxel0Size) * (1u << clip);
}

float DaGIAlbedoScene::clip_voxel_size(float voxel0Size, uint8_t clip) { return voxel0Size * (1u << clip); }

enum
{
  ALBEDO_MOVE_THRESHOLD = 1
};
uint16_t DaGIAlbedoScene::moving_threshold_blocks() { return ALBEDO_MOVE_THRESHOLD; }

void DaGIAlbedoScene::fixup_settings(uint8_t &w, uint8_t &d, uint8_t &c)
{
  if constexpr (!DAGI_ALBEDO_ALLOW_NON_POW2)
  {
    w = get_bigger_pow2(w);
    d = get_bigger_pow2(d);
  }
  w = clamp<uint8_t>(w, 4, 64);
  d = clamp<uint8_t>(d, 4, 64);
  c = clamp<uint8_t>(c, 0, DAGI_MAX_ALBEDO_CLIPS);
}

const uint32_t debug_flag = 0; // SBCF_USAGE_READ_BACK;

void DaGIAlbedoScene::afterReset()
{
  for (int i = 0; i < DAGI_MAX_ALBEDO_CLIPS; ++i)
    ShaderGlobal::set_int4(dagi_albedo_clipmap_lt_coordVarId[i], -1000000, 1000000, 10000000, bitwise_cast<uint32_t>(1e9f));
  validHistory = false;
  clipmap.assign(clipmap.size(), Clip());
}

void DaGIAlbedoScene::close()
{
  afterReset();
  clipmap.clear();
  dagi_albedo_atlas.close();
  dagi_albedo_indirection__free_indices_list.close();
  dagi_albedo_indirect_args.close();
  clipW = clipD = 0;
  ShaderGlobal::set_int4(dagi_albedo_atlas_sizeiVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_albedo_clipmap_sizeiVarId, 0, 0, 0, 0);
  ShaderGlobal::set_int4(dagi_albedo_clipmap_sizei_np2VarId, 1, 1, 0, 0);
}

void DaGIAlbedoScene::init(uint8_t w, uint8_t d, uint8_t c, float voxel0, float average_allocation)
{
  if (!dagi_albedo_clipmap_lt_coordVarId[0])
  {
    eastl::string str;
    for (int i = 0; i < DAGI_MAX_ALBEDO_CLIPS; ++i)
    {
      str.sprintf("dagi_albedo_clipmap_lt_coord_%d", i);
      dagi_albedo_clipmap_lt_coordVarId[i] = get_shader_variable_id(str.c_str(), false);
    }
  }
  if (!dagi_clear_albedo_freelist_cs)
  {
#define CS(a) a.reset(new_compute_shader(#a))
    CS(dagi_clear_albedo_freelist_cs);
    CS(dagi_clear_albedo_texture_cs);
    CS(dagi_albedo_scene_from_gbuf_cs);
    CS(dagi_fix_empty_alpha_cs);
    CS(dagi_albedo_toroidal_movement_cs);
    CS(dagi_albedo_after_toroidal_movement_create_indirect_cs);
    CS(dagi_albedo_allocate_after_toroidal_movement_cs);
    CS(dagi_albedo_allocate_after_toroidal_movement_pass2_cs);
    CS(dagi_albedo_fix_insufficient_cs);
#undef CS
  }
  if (c == 0)
  {
    close();
    return;
  }

  fixup_settings(w, d, c);
  if (average_allocation <= 0)
    average_allocation = 0.25 * float(w) / float(d); // fixme. we should auto-detect (based on feedback loop) exhaustion
  const uint32_t atlasShiftInBlocks = get_log2i(256 / DAGI_ALBEDO_BLOCK_SIZE);
  const uint32_t atlasInBlocksW = 1 << atlasShiftInBlocks;
  const uint32_t totalDirect = w * w * d * c;
  const float totalOptimal = ceilf(totalDirect * average_allocation);
  uint32_t atlasInBlocksD = ceilf(totalOptimal / (atlasInBlocksW * atlasInBlocksW));
  if (clipW == w && clipD == d && clipmap.size() == c && atlasW == atlasInBlocksW && atlasD == atlasInBlocksD && voxel0Size == voxel0)
    return;

  close();
  atlasW = atlasInBlocksW;
  atlasD = atlasInBlocksD;
  debug("init albedo scene, clipmap %dx%dx%dx%d (meters %fx%fx%f) atlas %dx%dx%d voxel0=%f", w, w, d, c,
    (DAGI_ALBEDO_INTERNAL_BLOCK_SIZE << (c - 1)) * w * voxel0, (DAGI_ALBEDO_INTERNAL_BLOCK_SIZE << (c - 1)) * w * voxel0,
    (DAGI_ALBEDO_INTERNAL_BLOCK_SIZE << (c - 1)) * d * voxel0, atlasInBlocksW, atlasInBlocksW, atlasInBlocksD, voxel0);
  dagi_albedo_atlas = dag::create_voltex(atlasW * DAGI_ALBEDO_BLOCK_SIZE, atlasW * DAGI_ALBEDO_BLOCK_SIZE,
    atlasD * DAGI_ALBEDO_BLOCK_SIZE, TEXCF_UNORDERED | TEXCF_SRGBREAD, 1, "dagi_albedo_atlas"); //
  ShaderGlobal::set_sampler(dagi_albedo_atlas_samplerstateVarId, d3d::request_sampler({}));
  ShaderGlobal::set_int4(dagi_albedo_atlas_sizeiVarId, atlasInBlocksW - 1, atlasShiftInBlocks, atlasShiftInBlocks * 2, atlasInBlocksD);
  ShaderGlobal::set_color4(dagi_albedo_inv_atlas_size_blocks_texelsVarId, 1.f / atlasW, 1.f / atlasD,
    1.f / (atlasW * DAGI_ALBEDO_BLOCK_SIZE), 1.f / (atlasD * DAGI_ALBEDO_BLOCK_SIZE));

  const float usefulSpaceLinear = float(DAGI_ALBEDO_INTERNAL_BLOCK_SIZE) / DAGI_ALBEDO_BLOCK_SIZE;
  ShaderGlobal::set_color4(dagi_albedo_internal_block_size_tc_borderVarId, usefulSpaceLinear / atlasW, usefulSpaceLinear / atlasD,
    DAGI_ALBEDO_BORDER / (atlasW * DAGI_ALBEDO_BLOCK_SIZE), DAGI_ALBEDO_BORDER / (atlasD * DAGI_ALBEDO_BLOCK_SIZE));

  voxel0Size = voxel0;
  clipW = w;
  clipD = d;
  const uint32_t clips = c;
  clipmap.resize(clips);

  const uint32_t totalIndirectionSize = clipW * clipW * clipD * clips;
  const uint32_t atlasOfs = (totalIndirectionSize + 3) & ~3;
  const uint32_t atlasSize = atlasW * atlasW * atlasD;
  const uint32_t freeListSize = atlasSize + 1, updatedListSize = atlasSize + 1;
  dagi_albedo_indirection__free_indices_list = dag::create_sbuffer(sizeof(uint32_t), atlasOfs + freeListSize + updatedListSize,
    debug_flag | SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_BIND_SHADER_RES, 0, "dagi_albedo_indirection__free_indices_list");

  dagi_albedo_indirect_args = dag::create_sbuffer(sizeof(uint32_t), 3, SBCF_BIND_UNORDERED | SBCF_MISC_ALLOW_RAW | SBCF_UA_INDIRECT, 0,
    "dagi_albedo_indirect_args");

  debug("dagi_albedo_indirection__free_indices_list = freeListSize(%d) + atlasOfs(%d) + updatedListSize(%d)", freeListSize, atlasOfs,
    updatedListSize);

  ShaderGlobal::set_int4(dagi_albedo_clipmap_sizeiVarId, clipW, clipD, clips, atlasOfs * 4);
  ShaderGlobal::set_int(dagi_albedo_update_ofsVarId, (atlasOfs + freeListSize) * 4);
  const int max_pos = (1 << 30) - 1;
  ShaderGlobal::set_int4(dagi_albedo_clipmap_sizei_np2VarId, clipW, clipD, is_pow_of2(clipW) ? 0 : (max_pos / clipW) * clipW,
    is_pow_of2(clipD) ? 0 : (max_pos / clipD) * clipD);
  validHistory = false;
}


float DaGIAlbedoScene::getInternalBlockSizeClip(uint32_t clip) const { return voxel0Size * (DAGI_ALBEDO_INTERNAL_BLOCK_SIZE << clip); }

bool DaGIAlbedoScene::updateClip(const dagi_albedo_rasterize_cb &cb, uint32_t clip_no, const IPoint3 &lt, float newBlockSize)
{
  DA_PROFILE_GPU;
  auto &clip = clipmap[clip_no];

  carray<IBBox3, 6> changed;
  const IPoint3 sz(clipW, clipW, clipD);
  const float oldSz = clip.internalBlockSize;
  const IPoint3 oldLt = (clip.internalBlockSize != newBlockSize) ? clip.lt - sz * 2 : clip.lt;
  if (oldLt == lt)
    return false;
  dag::Span<IBBox3> changedSpan(changed.data(), changed.size());
  const int changedCnt = move_box_toroidal(lt, oldLt, sz, changedSpan);
  clip.lt = lt;
  clip.internalBlockSize = newBlockSize;

  setClipVars(clip_no);
  IBBox3 allBox;
  for (int ui = 0; ui < changedCnt; ++ui)
    allBox += changed[ui];

  const IPoint3 allSz = allBox.width();
  ShaderGlobal::set_int4(dagi_albedo_clipmap_update_box_ltVarId, allBox[0].x, allBox[0].y, allBox[0].z, clip_no);
  ShaderGlobal::set_int4(dagi_albedo_clipmap_update_box_szVarId, allSz.x, allSz.y, allSz.z, bitwise_cast<uint32_t>(newBlockSize));
  ShaderGlobal::set_int4(dagi_albedo_clipmap_update_old_ltVarId, oldLt.x, oldLt.y, oldLt.z, bitwise_cast<uint32_t>(oldSz));
  // d3d::resource_barrier(
  //   {dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  // if there were insufficient blocks allocated, fix counter before freeing anything
  dagi_albedo_fix_insufficient_cs->dispatch(1, 1, 1);
  for (int ui = 0; ui < changedCnt; ++ui)
  {
    ShaderGlobal::set_int4(dagi_albedo_clipmap_update_lt_coordVarId, changed[ui][0].x, changed[ui][0].y, changed[ui][0].z, 0);
    const IPoint3 updateSize = changed[ui].width();
    ShaderGlobal::set_int4(dagi_albedo_clipmap_update_sz_coordVarId, updateSize.x, updateSize.y, updateSize.z,
      updateSize.x * updateSize.y);
    // debug("update albedo clip %@ %@, %@ %@", allBox, clip_no, lt, updateSize);
    dagi_albedo_toroidal_movement_cs->dispatchThreads(updateSize.x * updateSize.y * updateSize.z, 1, 1); // basically clear cache and
    if (ui < changedCnt - 1)
      d3d::resource_barrier({dagi_albedo_indirection__free_indices_list.getBuf(), RB_NONE});
  }
  {
    TIME_D3D_PROFILE(albedo_create_indirect)
    d3d::set_rwbuffer(STAGE_CS, 0, dagi_albedo_indirect_args.getBuf());
    dagi_albedo_after_toroidal_movement_create_indirect_cs->dispatchThreads(1, 1, 1);
    d3d::set_rwbuffer(STAGE_CS, 0, nullptr);
    d3d::resource_barrier({dagi_albedo_indirect_args.getBuf(), RB_RO_INDIRECT_BUFFER});
  }
  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_NONE, 0, 0});
  // we have to flush free list
  {
    TIME_D3D_PROFILE(albedo_add_changed_blocks)
    d3d::resource_barrier(
      {dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    dagi_albedo_allocate_after_toroidal_movement_cs->dispatch_indirect(dagi_albedo_indirect_args.getBuf(), 0);
    d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_NONE, 0, 0});
  }

  {
    TIME_D3D_PROFILE(free_changed_blocks)
    // fix again, as we free now
    dagi_albedo_fix_insufficient_cs->dispatch(1, 1, 1);
    d3d::resource_barrier(
      {dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
    dagi_albedo_allocate_after_toroidal_movement_pass2_cs->dispatch_indirect(dagi_albedo_indirect_args.getBuf(), 0);
  }

  // we have to flush UAV, as we call rasterize after that
  const auto stageAll = RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX;

  d3d::resource_barrier({dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | stageAll});
  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | stageAll, 0, 0});
  validateClipmap("toroidal");
  BBox3 worldBox{Point3::xzy(allBox[0]) * newBlockSize, Point3::xzy(allBox[1] + IPoint3(1, 1, 1)) * newBlockSize};
  uintptr_t handle = 0; // fixme

  if (cb(worldBox, newBlockSize / DAGI_ALBEDO_INTERNAL_BLOCK_SIZE, handle) == UpdateAlbedoStatus::RENDERED)
  {
    d3d::resource_barrier({dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_PIXEL | stageAll});
    d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_PIXEL | stageAll, 0, 0});
  }
  ShaderGlobal::set_int4(dagi_albedo_clipmap_update_box_ltVarId, -10000, 10000, -10000, 256);
  validateClipmap("render");
  return true;
}

void DaGIAlbedoScene::validateClipmap(const char *n)
{
  if (!debug_flag)
    return;
  uint32_t *fData; // -V779 unreachable code
  int fCnt = 0;
  const uint32_t atlasSize = atlasW * atlasW * atlasD, indirectionSize = clipW * clipW * clipD * clipmap.size(),
                 atlasOfs = (indirectionSize + 3) & ~3;
  if (dagi_albedo_indirection__free_indices_list->lock(0, (atlasSize + atlasOfs + 1) * 4, (void **)&fData, VBLOCK_READONLY))
  {
    eastl::bitvector<> freeCache(atlasSize);
    fCnt = fData[atlasOfs];
    debug("%s: empty blocks %d", n, fCnt);
    if (fCnt < 0 || fCnt > freeCache.size())
      logerr("%s: invalid empty blocks count %d/%d", n, fCnt, freeCache.size());
    for (int i = 0, maxB = atlasSize, ie = fCnt; i < ie; ++i)
    {
      const int id = fData[atlasOfs + i + 1];
      if (id < 0 || id >= freeCache.size())
        logerr("%s: invalid empty block %d %d/%d", n, id, i, freeCache.size());
      else if (freeCache[id])
        logerr("%s: empty block %d already exist %d/%d", n, id, i, freeCache.size());
      else
      {
        if (id != maxB - i - 1)
          debug("%s: free pos %d, block %d", n, i, id);
        freeCache.set(id, true);
      }
    }
    dag::Vector<uint32_t> usedCache(atlasSize, ~0u);
    for (int i = 0, ie = indirectionSize; i < ie; ++i)
    {
      const uint32_t id = fData[i];
      if (id == ~0u)
        continue;
      else if (id >= usedCache.size())
        logerr("%s: clipmap invalid empty block %d/%d", n, id, usedCache.size());
      else if (freeCache[id])
        logerr("%s: clipmap block %dx%dx%d:%d is empty %d/%d", n, i % clipW, (i / clipW) % clipW, (i / (clipW * clipW)) % clipD,
          i / (clipW * clipW * clipD), id, usedCache.size());
      else if (usedCache[id] != ~0u)
        logerr("%s: clipmap block %d is double used %d and %d", n, id, i, usedCache[id]);
      else
        usedCache[id] = i;
    }
    dagi_albedo_indirection__free_indices_list->unlock();
  }
}

void DaGIAlbedoScene::setClipVars(int clip_no) const
{
  auto &clip = clipmap[clip_no];
  ShaderGlobal::set_int4(dagi_albedo_clipmap_lt_coordVarId[clip_no], clip.lt.x, clip.lt.y, clip.lt.z,
    bitwise_cast<uint32_t>(clip.internalBlockSize));
#if DAGOR_DBGLEVEL > 0
  if (!is_pow_of2(clipW) || !is_pow_of2(clipD))
  {
    IPoint4 l = dagi_albedo_clipmap_sizei_np2VarId.get_int4();
    if (min(min(l.z + abs(clip.lt.x), l.z + abs(clip.lt.y)), l.w + abs(clip.lt.y)) < 0 || l.z < -clip.lt.x || l.z < -clip.lt.y ||
        l.w < -clip.lt.z)
    {
      LOGERR_ONCE("position %@ is too far from center, due to non-pow2 of clip size %dx%d. See magic_np2.txt", clip.lt, clipW, clipD);
    }
  }
#endif
}

void DaGIAlbedoScene::initHistory()
{
  if (validHistory)
    return;
  DA_PROFILE_GPU;

  setUAV(STAGE_CS);
  d3d::resource_barrier(
    {dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});
  const uint32_t indirectionSize = clipW * clipW * clipD * clipmap.size();
  dagi_clear_albedo_freelist_cs->dispatchThreads(max<uint32_t>(atlasW * atlasW * atlasD, (indirectionSize + 3) / 4), 1, 1);
  d3d::resource_barrier(
    {dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE});

  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  dagi_clear_albedo_texture_cs->dispatch(atlasW, atlasW, atlasD);
  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  resetUAV(STAGE_CS);

  validHistory = true;
}

inline IPoint3 DaGIAlbedoScene::getNewClipLT(uint32_t clip, const Point3 &world_pos) const
{
  return ipoint3(floor(Point3::xzy(world_pos) / getInternalBlockSizeClip(clip) + 0.5)) - IPoint3(clipW, clipW, clipD) / 2;
}

bool DaGIAlbedoScene::update(const Point3 &world_pos, bool update_all, const dagi_albedo_rasterize_cb &cb)
{
  if (!clipmap.size())
    return false;
  initHistory();

  StaticTab<eastl::pair<int, Clip>, DAGI_MAX_ALBEDO_CLIPS> updatedClipmaps;
  for (int i = clipmap.size() - 1; i >= 0; --i)
  {
    auto &clip = clipmap[i];
    const IPoint3 lt = getNewClipLT(i, world_pos);
    const IPoint3 move = lt - clip.lt, absMove = abs(move);
    const float internalBlockSize = getInternalBlockSizeClip(i);
    const int maxMove = max(max(absMove.x, absMove.y), absMove.z);
    if (maxMove > ALBEDO_MOVE_THRESHOLD || clip.internalBlockSize != internalBlockSize)
    {
      IPoint3 useMove{0, 0, 0};
      const Point3 relMove = div(point3(absMove), Point3(clipW, clipW, clipD));
      if (clip.internalBlockSize != internalBlockSize ||      // everything has changed
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
      updatedClipmaps.emplace_back(i, Clip{clip.lt + useMove, internalBlockSize});
      if (!update_all)
        break;
    }
  }

  if (updatedClipmaps.empty())
    return false;
  DA_PROFILE_GPU;
  setUAV(STAGE_PS);
  setUAV(STAGE_CS);
  for (auto &clip : updatedClipmaps)
    updateClip(cb, clip.first, clip.second.lt, clip.second.internalBlockSize);
  resetUAV(STAGE_PS);
  resetUAV(STAGE_CS);
  return true;
}

void DaGIAlbedoScene::fixBlocks()
{
  if (!gi_fix_albedo_scene_alpha || !dagi_fix_empty_alpha_cs)
    return;
  DA_PROFILE_GPU;
  const uint32_t totalUpdatePixels = (atlasW * atlasW * atlasD) << (3 * DAGI_ALBEDO_BLOCK_SHIFT);
  const uint32_t sz = dagi_fix_empty_alpha_cs->getThreadGroupSizes()[0];
  const uint32_t updateGroups = (totalUpdatePixels / 32 + sz - 1) / sz; // in update 32 frames
  ShaderGlobal::set_int4(dagi_albedo_scene_update_countVarId, updateGroups * sz, fix_update_frame++, 0, 0);
  setUAV(STAGE_CS);
  dagi_fix_empty_alpha_cs->dispatch(updateGroups, 1, 1);
  resetUAV(STAGE_CS);
  // we don't need this barrier at all, since we flush after move and use globallycoherent in temporal
  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
}

void DaGIAlbedoScene::setUAV(uint32_t stage)
{
  // all consoles have typed uav load for rgba8, known in compile time
  const bool hasTypedUAVLoad = d3d::get_driver_code().is(d3d::xboxOne) || d3d::get_driver_code().is(d3d::scarlett) ||
                               d3d::get_driver_code().is(d3d::ps4) || d3d::get_driver_code().is(d3d::ps5);
  d3d::set_rwtex(stage, dagi_albedo_atlas_reg_noVarId.get_int(), dagi_albedo_atlas.getVolTex(), 0, 0, !hasTypedUAVLoad);
  d3d::set_rwbuffer(stage, dagi_albedo_indirection__free_indices_list_reg_noVarId.get_int(),
    dagi_albedo_indirection__free_indices_list.getBuf());
}

void DaGIAlbedoScene::resetUAV(uint32_t stage)
{
  d3d::set_rwtex(stage, dagi_albedo_atlas_reg_noVarId.get_int(), nullptr, 0, 0, false);
  d3d::set_rwbuffer(stage, dagi_albedo_indirection__free_indices_list_reg_noVarId.get_int(), nullptr);
}

void DaGIAlbedoScene::updateFromGbuf()
{
  if (!dagi_albedo_scene_from_gbuf_cs)
    return;
  DA_PROFILE_GPU;
  ShaderGlobal::set_int(dagi_albedo_scene_stable_updateVarId, temporalStable ? 1 : 0);

  float useTemporalSpeed = (temporalStable ? 0.5f : 1.f) * temporalSpeed; // decrease amount
  const uint32_t pixelCount =
    gi_albedo_scene_update_from_gbuf_speed_min + gi_albedo_scene_update_from_gbuf_speed_count * useTemporalSpeed;
  ShaderGlobal::set_int4(dagi_albedo_scene_update_countVarId, pixelCount, gbuf_update_frame++, 0, 0);
  setUAV(STAGE_CS);
  dagi_albedo_scene_from_gbuf_cs->dispatchThreads(pixelCount, 1, 1);
  resetUAV(STAGE_CS);

  d3d::resource_barrier({dagi_albedo_atlas.getVolTex(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE | RB_STAGE_COMPUTE, 0, 0});
  // we don't need this barrier at all, since we use globallycoherent
  // d3d::resource_barrier({dagi_albedo_indirection__free_indices_list.getBuf(), RB_FLUSH_UAV | RB_SOURCE_STAGE_COMPUTE |
  // RB_STAGE_COMPUTE | RB_STAGE_PIXEL | RB_STAGE_VERTEX});
}

void DaGIAlbedoScene::debugRender()
{
  if (!clipmap.size())
    return;
  if (gi_albedo_scene_debug != 1)
    return;
  DA_PROFILE_GPU;

  if (!dagi_world_scene_albedo_scene_debug.getElem())
    dagi_world_scene_albedo_scene_debug.init("dagi_world_scene_albedo_scene_debug");
  dagi_world_scene_albedo_scene_debug.render();
}

void DaGIAlbedoScene::debugRenderScreen()
{
  if (!clipmap.size())
    return;
  if (gi_albedo_scene_debug != 2)
    return;

  DA_PROFILE_GPU;
  if (!dagi_albedo_scene_debug.getElem())
    dagi_albedo_scene_debug.init("dagi_albedo_scene_debug");
  dagi_albedo_scene_debug.render();
}
