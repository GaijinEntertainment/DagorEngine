// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstExtraRender.h>

#include "riGen/riUtil.h"
#include "render/drawOrder.h"
#include "render/extraRender.h"
#include "render/genRender.h"
#include "render/extra/cachedDynVarsPolicy.h"
#include "render/extra/opaqueGlobalDynVarsPolicy.h"
#include "render/gpuObjects.h"
#include "visibility/genVisibility.h"

#include <osApiWrappers/dag_miscApi.h>
#include <math/dag_mathUtils.h>
#include <scene/dag_occlusionMap.h>
#include <util/dag_convar.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_stlqsort.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_threadPool.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_lsbVisitor.h>
#include <render/gpuVisibilityTest.h>
#include <drv/3d/dag_buffers.h>

#define USE_SHADOW_CULLING_HACK 1 // workaround for shadow occlusion culling bug // TODO: fix the root cause

float rendinst::riExtraLodDistSqMul = 1.f;
float rendinst::riExtraCullDistSqMul = 1.f;
float rendinst::render::riExtraMinSizeForReflection = 25.f;
float rendinst::render::riExtraMinSizeForDraftDepth = 25.f;
int rendinst::render::instancingTexRegNo = -1;
rendinst::render::VbExtraCtx rendinst::render::vbExtraCtx[2];
UniqueBuf rendinst::render::riExtraPerDrawData;


__forceinline scene::node_index get_riex_render_node_index(rendinst::riex_render_info_t handle) { return handle & 0x3FFFFFFF; }
__forceinline uint32_t get_riex_render_scene_index(rendinst::riex_render_info_t handle) { return handle >> 30; }

static Tab<rendinst::render::ri_extra_render_marker_cb> ri_extra_render_marker_callbacks;

struct CallRenderExtraMarkerCallbacksRAII
{
  CallRenderExtraMarkerCallbacksRAII()
  {
    for (rendinst::render::ri_extra_render_marker_cb cb : ri_extra_render_marker_callbacks)
      cb(true);
  }

  ~CallRenderExtraMarkerCallbacksRAII()
  {
    for (rendinst::render::ri_extra_render_marker_cb cb : ri_extra_render_marker_callbacks)
      cb(false);
  }
};

// If too much objects - solid angle threshold decrease, if too small - increase
namespace rendinst
{

struct RIOcclusionData
{
  static constexpr int MAX_TEMP_OCCLUDERS = 256;
  float occluder_dis_multiplier = 1.0f;
  const float occluder_min_dis_multiplier = 0.00001;
  const float occluder_max_dis_multiplier = 10000.f;

  float occluder_size_multiplier = 16.0f;
  const float occluder_min_size_multiplier = 0.0001;
  eastl::fixed_vector<mat44f, MAX_TEMP_OCCLUDERS, false> occluders;
  eastl::fixed_vector<IPoint2, MAX_TEMP_OCCLUDERS, false> occludersIdDist;
};
void destroyOcclusionData(RIOcclusionData *&data) { del_it(data); }
RIOcclusionData *createOcclusionData() { return new RIOcclusionData; }
void iterateOcclusionData(RIOcclusionData &od, const mat44f *&mat, const IPoint2 *&indices, uint32_t &cnt)
{
  mat = od.occluders.data();
  indices = od.occludersIdDist.data();
  cnt = od.occludersIdDist.size();
}
static void gatherOcclusion(mat44f_cref globtm, vec4f vpos, RIOcclusionData &od, const int maxRiToAdd, uint32_t flag);

}; // namespace rendinst


rendinst::RITiledScenes rendinst::riExTiledScenes;
float rendinst::riExTiledSceneMaxDist[4] = {0};


dag::Vector<rendinst::render::RenderElem> rendinst::render::allElems;
dag::Vector<uint32_t> rendinst::render::allElemsIndex;
int rendinst::render::pendingRebuildCnt = 0;
bool rendinst::render::relemsForGpuObjectsHasRebuilded = true;

namespace rendinst::render
{

static eastl::hash_set<eastl::string> dynamicRiExtra;
static uint32_t oldPoolsCount = 0;

MultidrawContext<rendinst::RiExPerInstanceParameters> riExMultidrawContext("ri_ex_multidraw");

bool canIncreaseRenderBuffer = true;
}; // namespace rendinst::render

void rendinst::render::termElems()
{
  allElems = {};
  allElemsIndex = {};
  oldPoolsCount = 0;
  pendingRebuildCnt = 0;
}

CallbackToken rendinst::render::meshRElemsUpdatedCbToken = {};

void rendinst::setRiExtraTiledSceneWritingThread(int64_t tid) { riExTiledScenes.setWritingThread(tid); }

void rendinst::gatherOcclusion(mat44f_cref globtm, vec4f vpos, RIOcclusionData &od, const int maxRiToAdd, uint32_t flag)
{
  od.occluders.resize(0);
  od.occludersIdDist.resize(0);
  vec4f minRadius = v_splats(od.occluder_size_multiplier);
  vec4f vpos_distscale = v_perm_xyzd(vpos, v_splats(rendinst::riExtraCullDistSqMul * od.occluder_dis_multiplier));
  int distancePassed = 0;
  for (const auto &tiled_scene : riExTiledScenes.scenes())
  {
    tiled_scene.frustumCull<true, false, false>(globtm, vpos_distscale, flag, flag, nullptr,
      [&](scene::node_index, mat44f_cref m, vec4f distSqScaled) {
        distancePassed++;
        if (od.occluders.size() >= od.MAX_TEMP_OCCLUDERS) // to speed up culling in a worst case, it will converge anyway
          return;
        vec4f rad2 = scene::get_node_bsphere_vrad(m);
        rad2 = v_mul_x(rad2, rad2);
        vec4f sphereScale = v_div_x(v_mul_x(rad2, v_splat_w(vpos_distscale)), distSqScaled);
        if (v_test_vec_x_lt(sphereScale, minRadius)) // too small
          return;
        od.occludersIdDist.emplace_back(od.occluders.size(), v_extract_xi(v_cast_vec4i(sphereScale)));
        od.occluders.push_back(m);
      });
  }

  if (distancePassed > maxRiToAdd * (4))
    od.occluder_dis_multiplier = min(od.occluder_max_dis_multiplier, od.occluder_dis_multiplier * 1.07f);
  else if (distancePassed < maxRiToAdd * (4) / 2)
    od.occluder_dis_multiplier = max(od.occluder_min_dis_multiplier, od.occluder_dis_multiplier * .97f);

  if (od.occluders.size() > maxRiToAdd)
    od.occluder_size_multiplier = min(od.occluder_size_multiplier * 1.03f, 16.f);
  else if (od.occluders.size() < maxRiToAdd / 2)
    od.occluder_size_multiplier = max(od.occluder_size_multiplier * 0.97f, od.occluder_min_size_multiplier);

  // debug("count = %d(%d) d.sc = %f, sz.sc=%f",
  //   od.occluders.size(), maxRiToAdd, od.occluder_dis_multiplier, od.occluder_size_multiplier);
  if (!od.occluders.size())
    return;

  G_ASSERT(od.occluders.size() == od.occludersIdDist.size());
  if (od.occluders.size() > maxRiToAdd)
  {
    stlsort::nth_element(od.occludersIdDist.begin(), od.occludersIdDist.begin() + maxRiToAdd, od.occludersIdDist.end(),
      [](const auto &a, const auto &b) { return a.y > b.y; });
    od.occludersIdDist.resize(maxRiToAdd);
  }
  stlsort::sort(od.occludersIdDist.begin(), od.occludersIdDist.begin() + od.occludersIdDist.size(),
    [](const auto &a, const auto &b) { return a.y > b.y; });
}

void rendinst::prepareOcclusionData(mat44f_cref gtm, vec4f vpos, RIOcclusionData &od, int max_ri_to_add, bool walls)
{
  gatherOcclusion(gtm, vpos, od, max_ri_to_add, walls ? RendinstTiledScene::IS_WALLS : RendinstTiledScene::LARGE_OCCLUDER);
}

static rendinst::RIOcclusionData simpleOcc;
void rendinst::prepareOcclusion(Occlusion *use_occlusion, class OcclusionMap *occl_map)
{
  if (!use_occlusion || !occl_map)
    return;
  TIME_PROFILE(prepareOcclusion);

  occl_map->resetDynamicOccluders();

  const int maxToAdd = use_occlusion->hasGPUFrame() ? 16 : 32; // if we don't have gpu frame, try to rasterize more data
  gatherOcclusion(use_occlusion->getCurViewProj(), use_occlusion->getCurViewPos(), simpleOcc, maxToAdd,
    RendinstTiledScene::HAS_OCCLUDER);

  for (int i = 0, ei = simpleOcc.occludersIdDist.size(); i < ei; ++i)
  {
    mat44f m = simpleOcc.occluders[simpleOcc.occludersIdDist[i].x];
    const scene::pool_index poolId = scene::get_node_pool(m);
    const auto &poolInfo = riExTiledScenes.getPools()[poolId];

    if (poolInfo.boxOccluder < 0xFFFF)
      occl_map->addOccluder(m, riExTiledScenes.getBoxOccluders()[poolInfo.boxOccluder].bmin,
        riExTiledScenes.getBoxOccluders()[poolInfo.boxOccluder].bmax);
    if (poolInfo.quadOccluder < 0xFFFF)
    {
      vec4f v0, v1, v2, v3;
      // todo: can transform 4 points at ones, using SoA
      auto v04 = riExTiledScenes.getQuadOccluders()[poolInfo.quadOccluder];
      v0 = v_mat44_mul_vec3p(m, v04.col0);
      v1 = v_mat44_mul_vec3p(m, v04.col1);
      v2 = v_mat44_mul_vec3p(m, v04.col2);
      v3 = v_mat44_mul_vec3p(m, v04.col3);
      occl_map->addOccluder(v0, v1, v2, v3);
    }
  }
}

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("rendinst", debugDumpScene, false);
#endif

void rendinst::RendinstTiledScene::debugDumpScene(const char *file_name)
{
  FullFileSaveCB cb(file_name);
  scene::save_simple_scene(cb, getBaseSimpleScene());
}

void rendinst::setRIGenExtraResDynamic(const char *ri_res_name)
{
  rendinst::render::dynamicRiExtra.insert(ri_res_name);
  int res_idx = rendinst::getRIGenExtraResIdx(ri_res_name);
  if (res_idx >= 0)
    riExtra[res_idx].tsIndex = DYNAMIC_SCENE;
}

bool rendinst::isRIGenExtraDynamic(int res_idx) { return (riExtra[res_idx].tsIndex == DYNAMIC_SCENE); }

static uint16_t get_pool_flags(rendinst::RiExtraPool &pool)
{
  uint16_t flag = 0;

  const float bboxSize = v_extract_x(v_length3_x(v_sub(pool.lbb.bmax, pool.lbb.bmin))) * pool.scaleForPrepasses;

  if (bboxSize >= rendinst::render::riExtraMinSizeForReflection) // visible in reflections
    flag |= rendinst::RendinstTiledScene::REFLECTION;
  if (bboxSize >= rendinst::render::riExtraMinSizeForDraftDepth) // rendered to draftDepth
    flag |= rendinst::RendinstTiledScene::DRAFT_DEPTH;
  if (pool.hasOccluder)
    flag |= rendinst::RendinstTiledScene::HAS_OCCLUDER;
  if (pool.largeOccluder)
    flag |= rendinst::RendinstTiledScene::LARGE_OCCLUDER;
  if (pool.useShadow)
    flag |= rendinst::RendinstTiledScene::HAVE_SHADOWS;
  if (!(pool.hideMask & 1))
    flag |= rendinst::RendinstTiledScene::VISIBLE_0;
  if (!(pool.hideMask & 4))
    flag |= rendinst::RendinstTiledScene::VISIBLE_2;
  if (!pool.collRes)
    flag |= rendinst::RendinstTiledScene::VISUAL_COLLISION;
  if (pool.largeOccluder && pool.isWalls && pool.collRes != nullptr)
    flag |= rendinst::RendinstTiledScene::IS_WALLS;
  if (pool.isRendinstClipmap)
    flag |= rendinst::RendinstTiledScene::IS_RENDINST_CLIPMAP;
  if (pool.useVsm)
    flag |= rendinst::RendinstTiledScene::VISIBLE_IN_VSM;
  if (pool.usedInLandmaskHeight)
    flag |= rendinst::RendinstTiledScene::VISIBLE_IN_LANDMASK;
  return flag;
}

static void repopulate_tiled_scenes();

void rendinst::init_tiled_scenes(const DataBlock *level_blk)
{
  if (!RendInstGenData::renderResRequired) // do not init on server
    return;

  const DataBlock *riExTS = nullptr;

  if (level_blk != nullptr)
    riExTS = level_blk->getBlockByName("riExtraTiledScenes");

  if (riExTS == nullptr)
    riExTS = ::dgs_get_game_params()->getBlockByName("riExtraTiledScenes");

  bool isRiExTSDefined = riExTS != nullptr; // Used for tuning some parameters
  int defaultRiexTSReserve = isRiExTSDefined ? 128 : 1024;
  if (!isRiExTSDefined)
    riExTS = &DataBlock::emptyBlock;

  using rendinst::riExTiledSceneMaxDist;
  using rendinst::riExTiledScenes;
  const int scnsCount = clamp(riExTS->getInt("scnCount", 1), 1, (int)rendinst::RITiledScenes::MAX_COUNT - 1);
  riExTiledScenes.reinit(scnsCount + rendinst::STATIC_SCENES_START);
  debug("init_ri_extra_tiled_scenes(%d)", riExTiledScenes.size());
  for (int i = rendinst::STATIC_SCENES_START; i < riExTiledScenes.scenes().size(); i++)
  {
    float tileSize = isRiExTSDefined ? riExTS->getReal(String(0, "scn%d.tileSize", i)) : 256.f;
    riExTiledScenes[i].init(tileSize, riExTS->getInt(String(0, "scn%d.objsReserve", i), defaultRiexTSReserve));
    riExTiledScenes[i].setKdtreeBuildParams(riExTS->getInt("splitGeomCnt", 8), riExTS->getInt("splitCnt", 32),
      riExTS->getReal("boxGeom", 8.f), riExTS->getReal("boxCount", 1.f));
    riExTiledSceneMaxDist[i] = riExTS->getReal(String(0, "scn%d.maxDist", i), 1e6);
    debug("  riExTiledScenes[%d] tileSize=%.0f  max_dist=%.0f", i, riExTiledScenes[i].getTileSize(), riExTiledSceneMaxDist[i]);
  }

  float dynTileSize = riExTS->getReal("dynamicTileSize", 128);
  riExTiledScenes[rendinst::DYNAMIC_SCENE].init(dynTileSize, 1024);
  riExTiledScenes[rendinst::DYNAMIC_SCENE].setKdtreeBuildParams(8, 32, 16.f, 4.f);
  riExTiledScenes[rendinst::DYNAMIC_SCENE].setKdtreeRebuildParams(32, 4.f); // minimum 64 objects per tile, 400% is threshold to
                                                                            // rebuild kdtree

  repopulate_tiled_scenes();
}

void rendinst::term_tiled_scenes()
{
  using rendinst::riExTiledScenes;
  if (riExTiledScenes.size())
  {
    debug("term_ri_extra_tiled_scenes(%d)", riExTiledScenes.size());
    riExTiledScenes.reinit(0);
  }
  rendinst::render::dynamicRiExtra.clear();
}

namespace rendinst
{

struct SetSceneUserData
{
  RendinstTiledScene &s;
  RITiledScenes &scenes;
  SetSceneUserData(RendinstTiledScene &s_, RITiledScenes &scenes_) : s(s_), scenes(scenes_) {}

  bool operator()(uint32_t cmd, scene::TiledScene &, scene::node_index ni, const char *d)
  {
    auto &tiledPools = scenes.getPools();
    auto &quadOccluders = scenes.getQuadOccluders();
    auto &boxOccluders = scenes.getBoxOccluders();
    if (cmd == RendinstTiledScene::INVALIDATE_SHADOW_DIST)
    {
      s.invalidateShadowDist();
      return true;
    }

    if (cmd == RendinstTiledScene::POOL_ADDED || cmd == RendinstTiledScene::BOX_OCCLUDER || cmd == RendinstTiledScene::QUAD_OCCLUDER)
    {
      if (tiledPools.size() <= ni)
        tiledPools.resize(ni + 1, TiledScenePoolInfo{{-1}, 0});
      if (cmd == RendinstTiledScene::POOL_ADDED)
      {
        // if (&s == &riExTiledScenes[0])//this is our assumption. however, it is safe to do same things numerous times anyway
        tiledPools[ni] = *(const TiledScenePoolInfo *)(d);
      }
      if (cmd == RendinstTiledScene::BOX_OCCLUDER && tiledPools[ni].boxOccluder == 0xFFFF)
      {
        G_FAST_ASSERT(boxOccluders.size() <= eastl::numeric_limits<uint16_t>::max());
        tiledPools[ni].boxOccluder = static_cast<uint16_t>(boxOccluders.size()); //-V1029
        boxOccluders.push_back(*(const bbox3f *)(d));
      }
      if (cmd == RendinstTiledScene::QUAD_OCCLUDER && tiledPools[ni].quadOccluder == 0xFFFF)
      {
        G_FAST_ASSERT(quadOccluders.size() <= eastl::numeric_limits<uint16_t>::max());
        tiledPools[ni].quadOccluder = static_cast<uint16_t>(quadOccluders.size()); //-V1029
        quadOccluders.push_back(*(const mat44f *)(d));
      }
      return true;
    }
    switch (cmd)
    {
      case RendinstTiledScene::ADDED:
        // It is not guaranteed that ADD and ADDED will be called without gaps (see "after write lock" comment in addRIGenExtra43)
        // and both setDistance and getNode do not check for INVALID_INDEX and will most likely crash if the node is destroyed in
        // between the commands.
        if (s.isAliveNode(ni)) // We are under lock in flushDeferredTransformUpdates, so checking is threadsafe.
        {
          s.setDistance(ni, *(uint8_t *)d);
          {
            const mat44f &node = s.getNode(ni);
            bbox3f wabb;
            vec4f sphere = scene::get_node_bsphere(node);
            wabb.bmin = v_sub(sphere, v_splat_w(sphere));
            wabb.bmax = v_add(sphere, v_splat_w(sphere));
            v_bbox3_add_box(riExTiledScenes.newlyCreatedNodesBox, wabb);
          }
        }
        else
          debug("Node was lost between DeferredCommand::ADD and RendinstTiledScene::ADDED (ni=%d)", (int)ni);
        break;
      case RendinstTiledScene::SET_UDATA: s.setUserData(ni, (const int *)d, ((const int *)d)[15]); break;
      case RendinstTiledScene::COMPARE_AND_SWAP_UDATA:
        s.casUserData(ni, (const int *)d, ((const int *)d) + ((const int *)d)[15] / 2, ((const int *)d)[15] / 2);
        break;
      case RendinstTiledScene::SET_FLAGS_IN_BOX:
      {
        bbox3f_cref box_flags = *reinterpret_cast<const bbox3f *>(d);
        uint16_t flags = uint16_t(v_extract_wi(v_cast_vec4i(box_flags.bmax)));
        // When we change only single bit we can test that this flag is actually unset. It can be faster due to kdtree structure.
        if (__popcount(flags) == 1)
          s.boxCull<true, true>(box_flags, flags, 0, [this, flags](scene::node_index ni, mat44f_cref) { s.setFlagsImm(ni, flags); });
        else
          s.boxCull<false, true>(box_flags, 0, 0, [this, flags](scene::node_index ni, mat44f_cref) { s.setFlagsImm(ni, flags); });
      }
      break;
      case RendinstTiledScene::SET_PER_INSTANCE_OFFSET:
      {
        uint32_t offset = *reinterpret_cast<const uint32_t *>(d);
        s.setPerInstanceRenderDataOffsetImm(ni, offset);
      }
      break;
    }
    return true;
  }
};

void RendinstTiledScene::onDirFromSunChanged(const Point3 &nd)
{
  if (dot(dirFromSunOnPrevDistInvalidation, nd) < 0.97)
  {
    dirFromSunOnPrevDistInvalidation = nd;
    setNodeUserData(0, INVALIDATE_SHADOW_DIST, int(0), SetSceneUserData(*this, riExTiledScenes));
  }
}

} // namespace rendinst

void rendinst::add_ri_pool_to_tiled_scenes(rendinst::RiExtraPool &pool, int pool_idx, const char *name, float max_lod_dist)
{
  bool boxOccluder = false, quadOccluder = false;
  bbox3f occlusionBox;
  mat44f occlusionQuad;
  v_bbox3_init_empty(occlusionBox);
  occlusionQuad.col0 = occlusionQuad.col1 = occlusionQuad.col2 = occlusionQuad.col3 = v_zero();

  if (!name && !pool.hasOccluder) // maintain the same hasOccluder on repopulate
    boxOccluder = quadOccluder = false;
  else if (name && strstr(name, "town_building_ruin_e_dp_01")) //== WTF?
  {
    boxOccluder = quadOccluder = false;
  }
  else if (pool.res->getOccluderBox() && !pool.res->getOccluderBox()->isempty())
  {
    const BBox3 &box = *pool.res->getOccluderBox(); //-V522
    occlusionBox = v_ldu_bbox3(box);
    boxOccluder = !pool.hasImpostor() && box.width().y > 3;
  }
  else if (const Point3 *pt = pool.res->getOccluderQuadPts())
  {
    quadOccluder = true;
    vec3f v0, v1, v2, v3;
    v0 = v_ldu(&pt[0].x);
    v1 = v_ldu(&pt[1].x);
    v2 = v_make_vec4f(pt[3].x, pt[3].y, pt[3].z, 1.0f);
    v3 = v_ldu(&pt[2].x);
    occlusionQuad.col0 = v0;
    occlusionQuad.col1 = v1;
    occlusionQuad.col2 = v2;
    occlusionQuad.col3 = v3;

    vec3f n0 = v_cross3(v_sub(v1, v0), v_sub(v2, v0));
    vec3f n1 = v_cross3(v_sub(v1, v2), v_sub(v3, v2));
    if (v_test_vec_x_lt_0(v_dot3_x(n0, n1)))
    {
      G_ASSERTF(0, "rendinst <%s> has incorrect quadOccluder. It should be 4-corners convex", name);
      quadOccluder = false;
    }
    if (v_test_vec_x_gt(v_abs(v_plane_dist_x(v_norm3(v_make_plane(v0, v1, v2)), v3)), v_splats(0.01f)))
    {
      G_ASSERTF(0, "rendinst <%s> has incorrect quadOccluder. It should be flat", name);
      quadOccluder = false;
    }
  }
  pool.hasOccluder = quadOccluder || boxOccluder ? 1 : 0;

  for (auto &s : riExTiledScenes.scenes())
  {
    s.setPoolBBox(pool_idx, pool.lbb);
    bbox3f box = pool.lbb;
    vec3f boxCenter = v_bbox3_center(box);
    vec3f sphVerticalCenter = v_and(boxCenter, v_cast_vec4f(V_CI_MASK0100));
    vec3f maxRad = v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 0)));
    maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 4))));
    maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 1))));
    maxRad = v_max(maxRad, v_length3_sq_x(v_sub(sphVerticalCenter, v_bbox3_point(box, 7))));
    // half of bbox points are intentionally omitted, since we choose sphere center located at y of bbox3 center, i.e. radius is
    // symetrical around maxRad = v_splat_x(v_sqrt_x(maxRad)); float bsr = riRes->bsphRad;
    s.setPoolDistanceSqScale(pool_idx, safediv(max_lod_dist * max_lod_dist, v_extract_x(maxRad)));
    // s.setPoolDistanceSqScale(pool_idx, max_lod_dist*max_lod_dist);//safediv(max_lod_dist*max_lod_dist, bsr*bsr));
  }
  const bool isDynamic = name && rendinst::render::dynamicRiExtra.find_as(name) != rendinst::render::dynamicRiExtra.end();
  if (riExTiledScenes.size())
  {
    TiledScenePoolInfo info = {pool.distSqLOD[0], pool.distSqLOD[1], pool.distSqLOD[2], pool.distSqLOD[3], pool.lodLimits,
      static_cast<uint32_t>(pool_idx), 0xFFFF, 0xFFFF, isDynamic};
    auto &s = riExTiledScenes[0]; // this HAS to be first scene, to avoid racing condition
    s.setNodeUserData(pool_idx, RendinstTiledScene::POOL_ADDED, info, SetSceneUserData(s, riExTiledScenes));
    if (boxOccluder)
      s.setNodeUserData(pool_idx, RendinstTiledScene::BOX_OCCLUDER, occlusionBox, SetSceneUserData(s, riExTiledScenes));
    if (quadOccluder)
      s.setNodeUserData(pool_idx, RendinstTiledScene::QUAD_OCCLUDER, occlusionQuad, SetSceneUserData(s, riExTiledScenes));
  }
  if (!name || pool.tsIndex != DYNAMIC_SCENE) // maintain the same tsIndex on repopulate
    pool.tsIndex = -1;

  if (!(pool.hideMask & 2)) // mask&2 - completely invisible, never rendered
  {
    if (isDynamic)
      pool.tsIndex = DYNAMIC_SCENE;

    if (pool.tsIndex < 0)
    {
      pool.tsIndex = riExTiledScenes.size() - 1;
      for (int i = STATIC_SCENES_START; i < riExTiledScenes.size(); i++)
        if (max_lod_dist < riExTiledSceneMaxDist[i] && pool.bsphRad() * 2 <= riExTiledScenes[i].getTileSize())
        {
          pool.tsIndex = i;
          break;
        }
    }
  }
}
scene::node_index rendinst::alloc_instance_for_tiled_scene(rendinst::RiExtraPool &pool, int pool_idx, int inst_idx, mat44f &tm44,
  vec4f &out_sphere)
{
  if (!riExTiledScenes.size())
    return scene::INVALID_NODE;

  if (inst_idx >= pool.tsNodeIdx.size())
    pool.tsNodeIdx.resize(inst_idx + 1, scene::INVALID_NODE);
  scene::node_index ni = riExTiledScenes[pool.tsIndex].allocate(tm44, pool_idx, get_pool_flags(pool));
  pool.tsNodeIdx[inst_idx] = (pool.tsIndex << 30) | ni;
  out_sphere = pool.riXYZR[inst_idx]; // this is actually not same sphere that used in rendering! fixme:
  return ni;
}

void rendinst::init_instance_user_data_for_tiled_scene(rendinst::RiExtraPool &pool, scene::node_index ni, vec4f sphere,
  int add_data_dwords, const int32_t *add_data)
{
  auto &scene = riExTiledScenes[pool.tsIndex];
  const bool isDynamicScene = pool.tsIndex == DYNAMIC_SCENE;
  uint8_t instLightDist = isDynamicScene ? scene.LIGHTDIST_DYNAMIC : scene.LIGHTDIST_INVALID;
  if (v_extract_w(sphere) > 64.f)
    instLightDist = scene.LIGHTDIST_TOOBIG;
  scene.setNodeUserData(ni, RendinstTiledScene::ADDED, instLightDist, SetSceneUserData(scene, riExTiledScenes));
  if (add_data_dwords)
    scene.setNodeUserDataEx(ni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data, SetSceneUserData(scene, riExTiledScenes));
}

void rendinst::move_instance_to_original_scene(rendinst::RiExtraPool &pool, int pool_idx, scene::node_index &nodeId)
{
  const uint32_t originalSceneId = pool.tsIndex;
  const uint32_t sceneId = nodeId >> 30;
  if (originalSceneId == -1 || originalSceneId == sceneId)
    return;

  scene::node_index ni = nodeId & 0x3FFFFFFF;

  auto &oscene = riExTiledScenes[sceneId];
  auto &nscene = riExTiledScenes[originalSceneId];

  int32_t add_data[16];
  int add_data_dwords = oscene.getUserDataWordCount();
  if (add_data_dwords)
    memcpy(add_data, oscene.getUserData(ni), 4 * add_data_dwords); //-V575

  mat44f tm44 = oscene.getNode(ni);
  uint32_t perInstanceRenderDataOffset = 0;
  if (oscene.getNodeFlags(ni) & RendinstTiledScene::HAS_PER_INSTANCE_RENDER_DATA)
  {
    perInstanceRenderDataOffset = oscene.getPerInstanceRenderDataOffset(ni);
    oscene.setNodeUserData(ni, RendinstTiledScene::SET_PER_INSTANCE_OFFSET, uint32_t(0), SetSceneUserData(oscene, riExTiledScenes));
  }

  oscene.destroy(ni);
  scene::node_index nni = nscene.allocate(tm44, pool_idx, get_pool_flags(pool));
  nscene.setNodeUserData(nni, RendinstTiledScene::ADDED,
    (originalSceneId == DYNAMIC_SCENE) ? nscene.LIGHTDIST_DYNAMIC : nscene.LIGHTDIST_INVALID,
    SetSceneUserData(nscene, riExTiledScenes));
  if (add_data_dwords)
    nscene.setNodeUserDataEx(nni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data, SetSceneUserData(nscene, riExTiledScenes));

  if (perInstanceRenderDataOffset)
    nscene.setNodeUserData(nni, RendinstTiledScene::SET_PER_INSTANCE_OFFSET, perInstanceRenderDataOffset,
      SetSceneUserData(nscene, riExTiledScenes));

  nodeId = (originalSceneId << 30) | nni;
}

void rendinst::move_instance_in_tiled_scene(rendinst::RiExtraPool &pool, int pool_idx, scene::node_index &nodeId, mat44f &tm44,
  bool do_not_wait)
{
  const uint32_t sceneId = nodeId >> 30;
  scene::node_index ni = nodeId & 0x3FFFFFFF;
  auto &oscene = riExTiledScenes[sceneId];

  if (sceneId != DYNAMIC_SCENE && oscene.isAliveNode(ni)) // was not dynamic
  {
    int32_t add_data[16];
    int add_data_dwords = oscene.getUserDataWordCount();
    if (add_data_dwords)
      if (const auto *user_data = oscene.getUserData(ni))
        memcpy(add_data, user_data, 4 * add_data_dwords);

    uint32_t perInstanceRenderDataOffset = 0;
    if (oscene.getNodeFlags(ni) & RendinstTiledScene::HAS_PER_INSTANCE_RENDER_DATA)
    {
      perInstanceRenderDataOffset = oscene.getPerInstanceRenderDataOffset(ni);
      oscene.setNodeUserData(ni, RendinstTiledScene::SET_PER_INSTANCE_OFFSET, uint32_t(0), SetSceneUserData(oscene, riExTiledScenes));
    }

    oscene.destroy(ni);
    auto &nscene = riExTiledScenes[DYNAMIC_SCENE];
    scene::node_index nni = nscene.allocate(tm44, pool_idx, get_pool_flags(pool));
    nscene.setNodeUserData(nni, RendinstTiledScene::ADDED, RendinstTiledScene::LIGHTDIST_DYNAMIC,
      SetSceneUserData(nscene, riExTiledScenes));
    if (add_data_dwords)
      nscene.setNodeUserDataEx(nni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data,
        SetSceneUserData(nscene, riExTiledScenes));

    if (perInstanceRenderDataOffset)
      nscene.setNodeUserData(nni, RendinstTiledScene::SET_PER_INSTANCE_OFFSET, perInstanceRenderDataOffset,
        SetSceneUserData(nscene, riExTiledScenes));

    nodeId = (DYNAMIC_SCENE << 30) | nni;
  }
  else
  {
    oscene.setTransform(ni, tm44, do_not_wait);
    // riExTiledScenes[sceneId].setNodeUserData(ni, RendinstTiledScene::MOVED, 0);//it is already dynamic
  }
}

void rendinst::set_instance_user_data(scene::node_index nodeId, int add_data_dwords, const int32_t *add_data)
{
  const uint32_t sceneId = nodeId >> 30;
  const scene::node_index ni = nodeId & 0x3FFFFFFF;
  dag::Span<RendinstTiledScene> scenes = riExTiledScenes.scenes();
  auto &scene = scenes[sceneId];
  if (scene.isAliveNode(ni))
    scene.setNodeUserDataEx(ni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data, SetSceneUserData(scene, riExTiledScenes));
  else
    scene.setNodeUserDataDeferred(ni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data);
}

void rendinst::set_user_data(rendinst::riex_render_info_t id, uint32_t start, const uint32_t *add_data, uint32_t count)
{
  const uint32_t sceneId = get_riex_render_scene_index(id);
  scene::node_index ni = get_riex_render_node_index(id);
  auto &oscene = riExTiledScenes[sceneId];
  if (start != 0)
  {
    G_ASSERT_FAIL(" implement this command with start offset!");
    return;
  }
  if (oscene.getUserDataWordCount() < start + count)
  {
    logerr("additional data count %d, set %d dwords at %d", oscene.getUserDataWordCount(), count, start);
    return;
  }

  oscene.setNodeUserDataEx(ni, RendinstTiledScene::SET_UDATA, count, (const int32_t *)add_data,
    SetSceneUserData(oscene, riExTiledScenes));
}

void rendinst::set_cas_user_data(rendinst::riex_render_info_t id, uint32_t start, const uint32_t *was_data, const uint32_t *new_data,
  uint32_t count)
{
  const uint32_t sceneId = get_riex_render_scene_index(id);
  scene::node_index ni = get_riex_render_node_index(id);
  auto &oscene = riExTiledScenes[sceneId];
  if (start != 0)
  {
    G_ASSERT_FAIL(" implement this command with start offset!");
    return;
  }
  if (oscene.getUserDataWordCount() < start + count || count > 7) // 7 is (16-1)/2
  {
    logerr("additional data count %d, set %d dwords at %d", oscene.getUserDataWordCount(), count, start);
    return;
  }
  G_ASSERT(count * 2 * 4 + 4 <= sizeof(mat44f));
  int32_t data[16];
  memcpy(data, was_data, count * 4);
  memcpy(data + count, new_data, count * 4);
  oscene.setNodeUserDataEx(ni, RendinstTiledScene::COMPARE_AND_SWAP_UDATA, count * 2, data, SetSceneUserData(oscene, riExTiledScenes));
}

const mat44f &rendinst::get_tiled_scene_node(rendinst::riex_render_info_t handle)
{
  uint32_t sceneIdx = get_riex_render_scene_index(handle);
  scene::node_index nodeIdx = get_riex_render_node_index(handle);
  return riExTiledScenes[sceneIdx].getNode(nodeIdx);
}

void rendinst::remove_instance_from_tiled_scene(scene::node_index nodeId)
{
  RendinstTiledScene &scene = riExTiledScenes[nodeId >> 30];
  scene::node_index ni = nodeId & 0x3FFFFFFF;

  scene.setNodeUserData(ni, RendinstTiledScene::SET_PER_INSTANCE_OFFSET, uint32_t(0), SetSceneUserData(scene, riExTiledScenes));
  scene.destroy(ni);
}

static void repopulate_tiled_scenes()
{
  using rendinst::riExtra;
  using rendinst::RiExtraPool;

  int repopulated_cnt = 0;
  bbox3f accum_box;
  v_bbox3_init_empty(accum_box);
  for (int res_idx = 0; res_idx < riExtra.size(); res_idx++)
  {
    RiExtraPool &pool = riExtra[res_idx];
    rendinst::add_ri_pool_to_tiled_scenes(pool, res_idx, nullptr, sqrtf(pool.distSqLOD[rendinst::RiExtraPool::MAX_LODS - 1]));
    for (int idx = 0; idx < pool.tsNodeIdx.size(); idx++)
    {
      if (pool.tsNodeIdx[idx] == scene::INVALID_NODE)
        continue;
      vec4f sphere = v_zero();
      mat44f tm44;
      v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);
      scene::node_index ni =
        (pool.tsIndex >= 0) ? alloc_instance_for_tiled_scene(pool, res_idx, idx, tm44, sphere) : scene::INVALID_NODE;
      if (ni != scene::INVALID_NODE)
        init_instance_user_data_for_tiled_scene(pool, ni, sphere, 0, nullptr);
      repopulated_cnt++;
      bbox3f wabb;
      v_bbox3_init(wabb, tm44, pool.lbb);
      v_bbox3_add_box(accum_box, wabb);
    }
  }
  if (repopulated_cnt > 0)
  {
    riutil::world_version_inc(accum_box);
    logwarn("init_ri_extra_tiled_scenes: re-populated %d instances", repopulated_cnt);
  }
}

void rendinst::applyTiledScenesUpdateForRIGenExtra(int max_quota_usec, int max_maintenance_quota_usec)
{
  TIME_PROFILE(rendinst_tiled_scenes_update);
  int64_t reft = ref_time_ticks();
#if DAGOR_DBGLEVEL > 0
  if (debugDumpScene.get())
  {
    for (int i = 0; i < riExTiledScenes.size(); ++i)
      riExTiledScenes[i].debugDumpScene(String(128, "riextra_scene_%d.bin", i));
    debugDumpScene.set(false);
  }
#endif
  // visual studio compiler (2015) fails on capturing template class instance members
  // so we keep explicit arrays

  //
  for (auto &s : riExTiledScenes.scenes())
  {
    s.flushDeferredTransformUpdates<char>(SetSceneUserData(s, riExTiledScenes));
    if (get_time_usec(reft) >= max_quota_usec)
      return;
  }
  max_quota_usec -= get_time_usec(reft);
  if (max_quota_usec <= 0)
    return;
  if (max_maintenance_quota_usec > max_quota_usec)
    max_maintenance_quota_usec = max_quota_usec;
  for (auto &s : riExTiledScenes.scenes())
  {
    s.shrinkDistance();
    if (!s.doMaintenance(reft, max_maintenance_quota_usec))
      break;
  }
  // debug("total apply and main time = %dus", get_time_usec(reft));
}

void rendinst::render::allocateRIGenExtra(rendinst::render::VbExtraCtx &vbctx)
{
  ScopedRIExtraWriteLock wr;
  if (!vbctx.vb)
  {
    // WT cannot use structured buffers becase of outdated DX10 Intel GPU.
    bool riUseStructuredBuffer =
      ShaderGlobal::get_interval_current_value(::get_shader_variable_id("small_sampled_buffers", true)) == 1;

    // Many mobile devices may hang and report device_lost if structured buffer is larger than 64K. To simplify limits testing we need
    // unified behviour across platforms, and for that the limit is controlled with the flag, and the driver issue is checked
    // separately.
    rendinst::render::canIncreaseRenderBuffer =
      ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("riexCanIncreaseRenderBuffer", true);
    if (d3d::get_driver_desc().issues.hasSmallSampledBuffers &&
        (RIEXTRA_VECS_COUNT * maxExtraRiCount > 65536 || rendinst::render::canIncreaseRenderBuffer))
      logerr("RIEx limits error: driver reports hasSmallSampledBuffers, rendinstExtraMaxCnt=%d, canIncreaseRenderBuffer=%d",
        maxExtraRiCount, rendinst::render::canIncreaseRenderBuffer);

    vbctx.vb = new RingDynamicSB;
    char vbName[] = "RIGz_extra0";
    G_FAST_ASSERT(&vbctx - &rendinst::render::vbExtraCtx[0] < countof(rendinst::render::vbExtraCtx));
    vbName[sizeof(vbName) - 2] += &vbctx - &rendinst::render::vbExtraCtx[0]; // RIGz_extra0 -> RIGz_extra{0,1}
    vbctx.vb->init(RIEXTRA_VECS_COUNT * maxExtraRiCount, sizeof(vec4f), sizeof(vec4f),
      SBCF_BIND_SHADER_RES | (riUseStructuredBuffer ? SBCF_MISC_STRUCTURED : 0), riUseStructuredBuffer ? 0 : TEXFMT_A32B32G32R32F,
      vbName);
    vbctx.gen++;
  }
}
void rendinst::render::updateShaderElems(uint32_t poolI)
{
  auto &riPool = riExtra[poolI];
  for (int l = 0; l < riPool.res->lods.size(); ++l)
  {
    RenderableInstanceResource *rendInstRes = riPool.res->lods[l].scene;
    ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
    mesh->updateShaderElems();
  }
}

void rendinst::render::on_ri_mesh_relems_updated(const RenderableInstanceLodsResource *r, bool)
{
  ScopedRIExtraReadLock wr;
  for (int poolI = 0, poolE = riExtra.size_interlocked(); poolI < poolE; ++poolI) // fixme: linear search inside 2k array! use Hashmap
    if (riExtra[poolI].res == r)
    {
      on_ri_mesh_relems_updated_pool(poolI);
      // break; More than one pool can have the same res. This happens in case of clonned ri
    }
}

void rendinst::render::rebuildAllElemsInternal()
{
  if (!RendInstGenData::renderResRequired)
    return;

  TIME_PROFILE(ri_rebuild_elems);

  // int64_t reft = profile_ref_ticks();
  static int drawOrderVarId = -2;
  if (drawOrderVarId == -2)
    drawOrderVarId = VariableMap::getVariableId("draw_order");

  ScopedRIExtraWriteLock wr;
  const int toRebuild = interlocked_acquire_load(pendingRebuildCnt);
  int newPoolsCount = rendinst::riExtra.size_interlocked();

  decltype(allElems) oldAllElems;
  decltype(allElemsIndex) oldAllElemsIndex;
  static constexpr int MAX_LODS = rendinst::RiExtraPool::MAX_LODS;
  int allElemsCnt = newPoolsCount * MAX_LODS;
  oldAllElems.reserve(toRebuild * MAX_LODS * 2 + allElems.size());
  oldAllElemsIndex.resize(allElemsCnt * ShaderMesh::STG_COUNT + 1);

  oldAllElems.swap(allElems);
  oldAllElemsIndex.swap(allElemsIndex);

  dag::Vector<uint16_t, framemem_allocator> toUpdate;
  toUpdate.reserve(toRebuild);
  {
    int j = 0, r = toRebuild;
    for (auto v : riExtraPoolWasNotSavedToElems.get_container())
    {
      auto procNonZeroValue = [&](auto v) {
        for (auto bi : LsbVisitor{v})
        {
          int i = j + (int)bi;
          if (i < newPoolsCount)
          {
            toUpdate.push_back(i);
            riExtraPoolWasNotSavedToElems[i] = false;
            if (--r)
              continue;
          }
          return false;
        }
        return true;
      };
      if (v && !procNonZeroValue(v))
        break;
      j += riExtraPoolWasNotSavedToElems.kBitCount;
      if (j >= newPoolsCount)
        break;
    }
  }
  int aei = 0;
  for (int l = 0; l < MAX_LODS; l++)
  {
    auto copyPools = [&](int fromOld, int toOld) {
      if (fromOld >= toOld)
        return;
      const uint32_t fromIn = (oldPoolsCount * l + fromOld) * ShaderMesh::STG_COUNT;
      const uint32_t toIn = (oldPoolsCount * l + toOld) * ShaderMesh::STG_COUNT;
      const int indicesOffset = allElems.size() - oldAllElemsIndex[fromIn];
      allElems.insert(allElems.end(), oldAllElems.data() + oldAllElemsIndex[fromIn], oldAllElems.data() + oldAllElemsIndex[toIn]);
      for (int i = fromIn; i < toIn; ++i, ++aei)
        allElemsIndex[aei] = oldAllElemsIndex[i] + indicesOffset;
    };
    int firstToCopyPool = 0;
    for (auto poolI : toUpdate)
    {
      auto &riPool = rendinst::riExtra[poolI];
      copyPools(firstToCopyPool, poolI);
      firstToCopyPool = poolI + 1;
      if (l >= riPool.res->lods.size())
      {
        for (unsigned int stage = 0; stage < ShaderMesh::STG_COUNT; stage++, aei++)
          allElemsIndex[aei] = allElems.size();
      }
      else
      {
        RenderableInstanceResource *rendInstRes = riPool.res->lods[l].scene;
        ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();

        for (unsigned int stage = 0; stage < ShaderMesh::STG_COUNT; stage++, aei++)
        {
          allElemsIndex[aei] = allElems.size();
          dag::Span<ShaderMesh::RElem> elems = mesh->getElems(stage);
          for (unsigned int elemNo = 0, en = elems.size(); elemNo < en; elemNo++)
          {
            ShaderMesh::RElem &elem = elems[elemNo];
            const PackedDrawOrder drawOrder{elem, stage, drawOrderVarId};
            const int vbIdx = elem.vertexData->getVbIdx();
            G_ASSERT(vbIdx <= eastl::numeric_limits<uint8_t>::max());
            allElems.emplace_back(RenderElem{(ShaderElement *)elem.e, (short)elem.vertexData->getStride(), (uint8_t)vbIdx, drawOrder,
              elem.getPrimitive(), elem.si, elem.sv, elem.numv, elem.numf, elem.baseVertex});
          }
        }
      }
    }
    copyPools(toUpdate.back() + 1, oldPoolsCount);
  }

  G_ASSERTF(aei == allElemsIndex.size() - 1, "aei %d %d", aei, allElemsIndex.size());
  allElemsIndex[aei] = allElems.size();
  // debug("rendinst::rebuildAllElems: %d for %d usec", pendingRebuildCnt, profile_time_usec(reft));
  G_VERIFY(interlocked_add(pendingRebuildCnt, -toRebuild) >= 0);
  oldPoolsCount = newPoolsCount;

  relemsForGpuObjectsHasRebuilded = false;
}

void rendinst::render::reinitOnShadersReload()
{
  int riExtraCount = rendinst::riExtra.size();
  if (!riExtraCount)
    return;
  for (int i = 0; i < riExtraCount; ++i)
  {
    updateShaderElems(i);
    riExtraPoolWasNotSavedToElems[i] = true;
  }
  interlocked_release_store(pendingRebuildCnt, riExtraCount);
  rebuildAllElemsInternal();
}


void rendinst::render::update_per_draw_gathered_data(uint32_t id)
{
  rendinst::render::RiShaderConstBuffers perDrawGatheredData;
  perDrawGatheredData.setBBoxNoChange();
  perDrawGatheredData.setOpacity(0, 1);
  perDrawGatheredData.setCrossDissolveRange(0);
  perDrawGatheredData.setRandomColors(rendinst::riExtra[id].poolColors);
  perDrawGatheredData.setInstancing(0, 4, 0, 0);
  perDrawGatheredData.setBoundingSphere(0, 0, rendinst::riExtra[id].sphereRadius, rendinst::riExtra[id].sphereRadius,
    rendinst::riExtra[id].sphCenterY);
  vec4f bbox = v_sub(rendinst::riExtra[id].lbb.bmax, rendinst::riExtra[id].lbb.bmin);
  Point4 bboxData;
  v_stu(&bboxData, bbox);
  perDrawGatheredData.setBoundingBox(bbox);
  perDrawGatheredData.setRadiusFade(rendinst::riExtra[id].radiusFade, rendinst::riExtra[id].radiusFadeDrown);
  float rendinstHeight = rendinst::riExtra[id].rendinstHeight == 0.0f ? bboxData.y : rendinst::riExtra[id].rendinstHeight;
  bbox = v_add(rendinst::riExtra[id].lbb.bmax, rendinst::riExtra[id].lbb.bmin);
  v_stu(&bboxData, bbox);
  rendinstHeight = rendinst::riExtra[id].hasImpostor() ? rendinst::riExtra[id].sphereRadius : rendinstHeight;
  perDrawGatheredData.setInteractionParams(rendinst::riExtra[id].hardness, rendinstHeight, bboxData.x * 0.5, bboxData.z * 0.5);
  if ((id + 1) * sizeof(rendinst::render::RiShaderConstBuffers) >
      rendinst::render::riExtraPerDrawData->getElementSize() * rendinst::render::riExtraPerDrawData->getNumElements())
    logerr("perDrawData buffer is not large enough for %d models (riMaxModelsPerLevel)", id);
  if (rendinst::riExtra[id].hasPLOD())
    perDrawGatheredData.setPLODRadius(rendinst::riExtra[id].plodRadius);
  rendinst::render::riExtraPerDrawData->updateData(id * sizeof(rendinst::render::RiShaderConstBuffers), sizeof(perDrawGatheredData),
    &perDrawGatheredData, 0);
}

static IPoint2 to_ipoint2(Point2 p) { return IPoint2(p.x, p.y); }

static void invalidate_riextra_shadows_by_grid(uint32_t poolI)
{
  BBox3 shadowsBbox = rendinst::get_shadows_bbox_cb();
  if (shadowsBbox.isempty())
    return;
  Point2 gridOrigin = Point2(shadowsBbox.lim[0].x, shadowsBbox.lim[0].z);
  Point2 gridSize = Point2(shadowsBbox.lim[1].x, shadowsBbox.lim[1].z) - gridOrigin;

  // Below are empirical numbers: assume that static shadows are less than 4km long; 64 is taken from riExTiledScenes[0] cell size.
  // Can be parametrized if needed.
  constexpr int MAX_CELLS_PER_DIM = 64;
  constexpr float MIN_CELL_SIZE = 64.0f;
  constexpr int MAX_BOXES_FOR_INVALIDATION_SQRT = 10;
  constexpr int MAX_BOXES_FOR_INVALIDATION = MAX_BOXES_FOR_INVALIDATION_SQRT * MAX_BOXES_FOR_INVALIDATION_SQRT;

  IPoint2 gridCells = max(min(to_ipoint2(gridSize / MIN_CELL_SIZE), IPoint2(MAX_CELLS_PER_DIM, MAX_CELLS_PER_DIM)), IPoint2(1, 1));

  auto invalidBoxes = rendinst::getRIGenExtraInstancesWorldBboxesByGrid(poolI, gridOrigin, gridSize, gridCells);

  // shadowsInvalidateGatheredBBoxes has n^2 complexity, so fallback to less resolution if we gathered too many boxes.
  if (invalidBoxes.size() > MAX_BOXES_FOR_INVALIDATION)
  {
    clear_and_shrink(invalidBoxes);
    invalidBoxes = rendinst::getRIGenExtraInstancesWorldBboxesByGrid(poolI, gridOrigin, gridSize,
      IPoint2(MAX_BOXES_FOR_INVALIDATION_SQRT, MAX_BOXES_FOR_INVALIDATION_SQRT));
  }

  for (const BBox3 &box : invalidBoxes)
    rendinst::shadow_invalidate_cb(box); // TODO: pass span instead of one by one
}

void rendinst::render::on_ri_mesh_relems_updated_pool(uint32_t poolI)
{
  riExtra[poolI].setWasNotSavedToElems();
  if (rendinst::on_vsm_invalidate && rendinst::riExtra[poolI].useVsm)
  {
    rendinst::on_vsm_invalidate();
  }

  // static shadows are rendered with lod1
  if (rendinst::shadow_invalidate_cb && riExtra[poolI].qlPrevBestLod > 1 && riExtra[poolI].res->getQlBestLod() <= 1)
  {
    if (rendinst::get_shadows_bbox_cb)
    {
      invalidate_riextra_shadows_by_grid(poolI);
    }
    else
    {
      BBox3 box;
      v_stu_bbox3(box, rendinst::getRIGenExtraOverallInstancesWorldBbox(poolI));
      if (!box.isempty())
        rendinst::shadow_invalidate_cb(box);
    }
  }

  riExtra[poolI].qlPrevBestLod = riExtra[poolI].res->getQlBestLod();
}

void rendinst::gatherRIGenExtraRenderable(Tab<riex_render_info_t> &out_handles, const int *sorted_res_idx, int count, bbox3f_cref box,
  bool fast, SceneSelection s)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount || RendInstGenData::isLoading)
    return;
  const int firstScene = s == SceneSelection::OnlyStatic ? STATIC_SCENES_START : 0;
  const int sceneCount = s == SceneSelection::OnlyDynamic ? STATIC_SCENES_START : riExTiledScenes.size() - firstScene;
  auto lambda = [&](scene::node_index ni, mat44f_cref m, int scenei) {
    if (sorted_res_idx != nullptr && !eastl::binary_search(sorted_res_idx, sorted_res_idx + count, mat44_to_ri_type(m)))
      return;
    out_handles.push_back(make_riex_render_info_handle(scenei, ni));
  };
  if (fast)
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<false, false>(box, 0, 0,
        [&](scene::node_index ni, mat44f_cref m) { lambda(ni, m, riExTiledScenes.at(tiled_scene)); });
  else
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<false, true>(box, 0, 0,
        [&](scene::node_index ni, mat44f_cref m) { lambda(ni, m, riExTiledScenes.at(tiled_scene)); });
}

void rendinst::gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, bbox3f_cref box, bool fast, rendinst::SceneSelection s)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount || RendInstGenData::isLoading)
    return;
  const int firstScene = s == SceneSelection::OnlyStatic ? STATIC_SCENES_START : 0;
  const int sceneCount = s == SceneSelection::OnlyDynamic ? STATIC_SCENES_START : riExTiledScenes.size() - firstScene;
  if (fast)
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<false, false>(box, 0, 0, [&](scene::node_index, mat44f_cref m) { out_handles.push_back(m); });
  else
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<false, true>(box, 0, 0, [&](scene::node_index, mat44f_cref m) { out_handles.push_back(m); });
}

void rendinst::gatherRIGenExtraRenderable(Tab<mat44f> &out_handles, const BBox3 &box, bool fast, rendinst::SceneSelection s)
{
  bbox3f vbox = v_ldu_bbox3(box);
  return gatherRIGenExtraRenderable(out_handles, vbox, fast, s);
}

void rendinst::gatherRIGenExtraRenderableNotCollidable(riex_pos_and_ri_tab_t &out, bbox3f_cref box, bool fast, SceneSelection s)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount || RendInstGenData::isLoading)
    return;
  const int firstScene = s == SceneSelection::OnlyStatic ? STATIC_SCENES_START : 0;
  const int sceneCount = s == SceneSelection::OnlyDynamic ? STATIC_SCENES_START : riExTiledScenes.size() - firstScene;
  auto lambda = [&](scene::node_index ni, mat44f_cref m, int scenei) {
    pos_and_render_info_t pi;
    v_stu_p3(&pi.pos.x, m.col3);
    pi.info = make_riex_render_info_handle(scenei, ni);
    out.emplace_back(pi);
  };
  if (fast)
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<true, false>(box, rendinst::RendinstTiledScene::VISUAL_COLLISION,
        rendinst::RendinstTiledScene::VISUAL_COLLISION,
        [&](scene::node_index ni, mat44f_cref m) { lambda(ni, m, riExTiledScenes.at(tiled_scene)); });
  else
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      tiled_scene.boxCull<true, true>(box, rendinst::RendinstTiledScene::VISUAL_COLLISION,
        rendinst::RendinstTiledScene::VISUAL_COLLISION,
        [&](scene::node_index ni, mat44f_cref m) { lambda(ni, m, riExTiledScenes.at(tiled_scene)); });
}

void rendinst::gatherRIGenExtraToTestForShadows(Tab<bbox3f> &out_bboxes, mat44f_cref globtm_cull, float static_shadow_texel_size,
  uint32_t usr_data)
{
  TIME_PROFILE(riextra_shadows_visibility_gather);

  bool hasConservativeRasterization = d3d::get_driver_desc().caps.hasConservativeRassterization;
  vec4f bboxExpandSize = hasConservativeRasterization ? v_splats(0.0f) : v_splats(static_shadow_texel_size * 0.5f);
  dag::Vector<scene::node_index, framemem_allocator> nodesToUnsetFlags;

  Frustum cullingFrustum(globtm_cull);
  // Don't process dynamic scene as it's incorrect to rely on single occlusion test -
  // they should be retested every time they move.
  // Don't process the last scene as it contains big objects that almost always won't be occluded.
  for (int sceneId = STATIC_SCENES_START; sceneId < (int)riExTiledScenes.size() - 1; ++sceneId)
  {
    nodesToUnsetFlags.clear();
    auto &scene = riExTiledScenes[sceneId];
    scene.frustumCull<true, true, false>(globtm_cull, v_splats(0.0f), RendinstTiledScene::NEEDS_CHECK_IN_SHADOW,
      RendinstTiledScene::NEEDS_CHECK_IN_SHADOW, nullptr, [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
        G_UNUSED(distSqScaled);
        // We can't unset flags directly here because we are under read lock. Tiled scene modification
        // will acquire lock for deferred command buffer, but in writing thread we acquire them in reverse
        // order so deadlock is possible.
        nodesToUnsetFlags.push_back(ni);
        bbox3f bbox = scene.calcNodeBox(m);
        vec4f bboxCenter2 = v_add(bbox.bmax, bbox.bmin);
        vec4f bboxSize = v_sub(bbox.bmax, bbox.bmin);
        // Test only fully inside bboxes.
        if (cullingFrustum.testBoxExtent(bboxCenter2, bboxSize) == Frustum::INSIDE)
        {
#if USE_SHADOW_CULLING_HACK
          bbox = scene.calcNodeBoxFromSphere(m);
#endif
          riex_render_info_t id = make_riex_render_info_handle(sceneId, ni);
          bbox.bmin = v_perm_xyzd(v_sub(bbox.bmin, bboxExpandSize), v_cast_vec4f(v_splatsi(id)));
          bbox.bmax = v_perm_xyzd(v_add(bbox.bmax, bboxExpandSize), v_cast_vec4f(v_splatsi(usr_data)));
          out_bboxes.push_back(bbox);
        }
      });
    // Have to reset VISIBLE_IN_SHADOWS too, cause after occlusion test it will be set through | operation.
    // All these flags are also resetted for boxes on edges - practically we just ignore them, cause
    // we can't rely on test results of partially visible boxes.
    for (scene::node_index nodeIdx : nodesToUnsetFlags)
      scene.unsetFlags(nodeIdx,
        RendinstTiledScene::VISIBLE_IN_SHADOWS | RendinstTiledScene::CHECKED_IN_SHADOWS | RendinstTiledScene::NEEDS_CHECK_IN_SHADOW);
  }
}

void rendinst::render::invalidateRIGenExtraShadowsVisibility()
{
  vec4f boxHalfSize = v_make_vec4f(1e6, 1e6, 1e6, 0);
  bbox3f box;
  box.bmin = v_neg(boxHalfSize);
  box.bmax = boxHalfSize;
  invalidateRIGenExtraShadowsVisibilityBox(box);
}

void rendinst::render::invalidateRIGenExtraShadowsVisibilityBox(bbox3f_cref box)
{
  for (int sceneId = STATIC_SCENES_START; sceneId < (int)riExTiledScenes.size() - 1; ++sceneId)
  {
    auto &scene = riExTiledScenes[sceneId];
    bbox3f box_flags;
    box_flags.bmin = box.bmin;
    box_flags.bmax = v_perm_xyzd(box.bmax, v_cast_vec4f(v_splatsi(RendinstTiledScene::NEEDS_CHECK_IN_SHADOW)));
    scene.setNodeUserData(0, RendinstTiledScene::SET_FLAGS_IN_BOX, box_flags, SetSceneUserData(scene, riExTiledScenes));
  }
}

void rendinst::setRIGenExtraNodeShadowsVisibility(riex_render_info_t id, bool visible)
{
  scene::node_index nodeId = get_riex_render_node_index(id);
  uint32_t sceneId = get_riex_render_scene_index(id);
  auto &scene = riExTiledScenes[sceneId];

  const uint16_t visFlags = (visible ? RendinstTiledScene::VISIBLE_IN_SHADOWS : 0) | RendinstTiledScene::CHECKED_IN_SHADOWS;
  scene.setFlags(nodeId, visFlags);
}

static bool lock_vbextra(RiGenExtraVisibility &v, vec4f *&vbPtr, rendinst::render::VbExtraCtx &vbctx)
{
  const int cnt = v.riexInstCount + v.sortedTransparentElems.size();
  if (vbctx.gen == v.vbExtraGeneration || !cnt) // buffer already filled or no instances to render
    return cnt != 0;

  TIME_PROFILE_DEV(lock_vbextra);

  int vi = &vbctx - &rendinst::render::vbExtraCtx[0];
  rendinst::maxRenderedRiEx[vi] = max(rendinst::maxRenderedRiEx[vi], cnt);
  if (cnt > rendinst::maxExtraRiCount)
  {
    logwarn("vbExtra[%d] recreated to requirement of render more = than %d (%d*2)", vi, rendinst::maxExtraRiCount, cnt);
    if (!rendinst::render::canIncreaseRenderBuffer)
    {
      logerr("Too many RIEx to render, and buffer resize is not possible");
      return false;
    }
    rendinst::maxExtraRiCount = cnt * 2;
    del_it(vbctx.vb);
    rendinst::render::allocateRIGenExtra(vbctx);
    debug_dump_stack();
  }
  uint32_t dataSize = cnt * rendinst::RIEXTRA_VECS_COUNT;
  if (vbctx.vb->getPos() + dataSize > vbctx.vb->bufSize())
  {
    if (vbctx.lastSwitchFrameNo && vbctx.lastSwitchFrameNo >= dagor_frame_no() - 1)
    {
      logwarn("vbExtra[%d] switched more than once during frame #%d, vbExtraLastSwitchFrameNo = %d (%d -> %d)", vi, dagor_frame_no(),
        vbctx.lastSwitchFrameNo, rendinst::maxExtraRiCount, 2 * rendinst::maxExtraRiCount);
      if (rendinst::render::canIncreaseRenderBuffer)
        rendinst::maxExtraRiCount *= 2;
      // probably intented to avoid accessing GPU owned data, so recreate it regardless of size growth
      del_it(vbctx.vb);
      rendinst::render::allocateRIGenExtra(vbctx);
    }
    vbctx.vb->resetPos();
    vbctx.vb->resetCounters();
    vbctx.lastSwitchFrameNo = dagor_frame_no();
    vbctx.gen++;
  }

  vbPtr = (vec4f *)vbctx.vb->lockData(dataSize);
  if (!vbPtr)
  {
    if (!d3d::is_in_device_reset_now())
      logerr("vbExtra[%d]->lockData(%d) failed, bufsize=%d", vi, dataSize, vbctx.vb->bufSize());
    return false;
  }
  return true;
}

static bool fill_vbextra(RiGenExtraVisibility &v,
  vec4f *vbPtr, // if not null assume that was already locked
  rendinst::render::VbExtraCtx &vbctx)
{
  if (vbctx.gen == v.vbExtraGeneration) // buffer already filled
    return true;

  TIME_D3D_PROFILE(ri_extra_fill_buffer);

  const int cnt = v.riexInstCount + v.sortedTransparentElems.size();
  if (!vbPtr && !lock_vbextra(v, vbPtr, vbctx))
    return false;

  int start_ofs = vbctx.vb->getPos(), cur_ofs = start_ofs;
  v.riExLodNotEmpty = 0;
  for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
  {
    int lod_ofs = cur_ofs;
    auto &riExDataLod = v.riexData[l];
    v.vbOffsets[l].assign(rendinst::riExTiledScenes.getPools().size(), IPoint2(0, 0));
    for (auto poolAndCnt : v.riexPoolOrder)
    {
      auto poolI = poolAndCnt & rendinst::render::RI_RES_ORDER_COUNT_MASK;
      if (uint32_t poolCnt = (uint32_t)v.riexData[l][poolI].size() / rendinst::RIEXTRA_VECS_COUNT)
      {
        v.vbOffsets[l][poolI] = IPoint2(cur_ofs, poolCnt);
        cur_ofs += poolCnt * rendinst::RIEXTRA_VECS_COUNT;
        const vec4f *data = riExDataLod[poolI].data();
        for (int m = 0; m < poolCnt; ++m, vbPtr += rendinst::RIEXTRA_VECS_COUNT, data += rendinst::RIEXTRA_VECS_COUNT)
          memcpy(vbPtr, data, rendinst::RIEXTRA_VECS_COUNT * sizeof(vec4f));
      }
    }
    if (lod_ofs != cur_ofs)
      v.riExLodNotEmpty |= 1 << l;
  }

  if (!v.sortedTransparentElems.empty())
  {
    for (RiGenExtraVisibility::PerInstanceElem &elem : v.sortedTransparentElems)
    {
      const vec4f *data = v.riexData[elem.lod][elem.poolId].data() + rendinst::RIEXTRA_VECS_COUNT * elem.instanceId;
      elem.vbOffset = cur_ofs;
      memcpy(vbPtr, data, rendinst::RIEXTRA_VECS_COUNT * sizeof(vec4f));
      vbPtr += rendinst::RIEXTRA_VECS_COUNT;
      cur_ofs += rendinst::RIEXTRA_VECS_COUNT;
    }
  }

  vbctx.vb->unlockData(cnt * rendinst::RIEXTRA_VECS_COUNT);
  G_ASSERT(cur_ofs - start_ofs == cnt * rendinst::RIEXTRA_VECS_COUNT);
  G_UNUSED(start_ofs);
  v.vbexctx = &vbctx;
  G_FAST_ASSERT(vbctx.gen != INVALID_VB_EXTRA_GEN);
  interlocked_release_store(v.vbExtraGeneration, vbctx.gen); // mark buffer as filled
  return true;
}

static struct RenderRiExtraJob final : public cpujobs::IJob
{
  rendinst::render::RiExtraRenderer riExRenderer;
  RiGenVisibility *vbase = nullptr;
  GlobalVariableStates gvars;
  TexStreamingContext texContext = TexStreamingContext(0);

  void prepare(RiGenVisibility &v, int frame_stblk, bool enable, TexStreamingContext texCtx)
  {
    TIME_PROFILE(prepareToStartAsyncRIGenExtraOpaqueRender);
    G_ASSERT(is_main_thread());
    threadpool::wait(this); // It should not be running at this point

    if (DAGOR_UNLIKELY(!enable))
    {
      vbase = nullptr;
      v.riex.vbexctx = nullptr;
      gvars.clear();
      return;
    }

    vbase = &v;
    texContext = texCtx;

    {
      ShaderGlobal::set_real_fast(globalTranspVarId, 1.f);
      ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);

      rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);
      ShaderBlockSetter ssb(frame_stblk, ShaderGlobal::LAYER_FRAME);
      SCENE_LAYER_GUARD(rendinst::render::rendinstSceneBlockId);
      ShaderGlobal::set_int(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::Normal));

      G_ASSERT(gvars.empty());
      gvars.set_allocator(framemem_ptr());
      copy_current_global_variables_states(gvars);
      riExRenderer.globalVarsState = &gvars;

      ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_MIXED);
    }

    rendinst::render::rebuildAllElems();
    // To consider: create with SBCF_FRAMEMEM (can't be more 256K at time of writing)
    rendinst::render::allocateRIGenExtra(rendinst::render::vbExtraCtx[1]);
  }

  void start(RiGenVisibility &v, bool wake)
  {
    if (!vbase)
      return;
    G_FAST_ASSERT(&v == vbase);
    G_UNUSED(v);
    threadpool::add(this, threadpool::PRIO_DEFAULT, wake); // Note: intentionally not immediate doJob() call since current (caller's)
                                                           // job can be waited on
  }

  void doJob() override
  {
    TIME_PROFILE(prepare_render_riex_opaque);
    vec4f *vbPtr = nullptr;
    {
      if (DAGOR_UNLIKELY(!lock_vbextra(vbase->riex, vbPtr, rendinst::render::vbExtraCtx[1]))) // lock failed (or no instances to
                                                                                              // render)
      {
        vbase->riex.riexInstCount = 0;
        G_FAST_ASSERT(rendinst::render::vbExtraCtx[1].gen != INVALID_VB_EXTRA_GEN);
        interlocked_release_store(vbase->riex.vbExtraGeneration, rendinst::render::vbExtraCtx[1].gen); // mark buffer as filled
        interlocked_release_store_ptr(vbase, (RiGenVisibility *)nullptr);
        return;
      }
      G_VERIFY(fill_vbextra(vbase->riex, vbPtr, rendinst::render::vbExtraCtx[1]));
    }
    dag::ConstSpan<uint16_t> riResOrder = vbase->riex.riexPoolOrder;
    riExRenderer.init(riResOrder.size(), rendinst::LayerFlag::Opaque, rendinst::RenderPass::Normal);
    riExRenderer.addObjectsToRender(vbase->riex, riResOrder, texContext);
    riExRenderer.sortMeshesByMaterial();
  }

  rendinst::render::RiExtraRenderer *wait(const RiGenVisibility *v)
  {
    RiGenVisibility *vl = interlocked_acquire_load_ptr(vbase);
    G_FAST_ASSERT(!v || !vl || v == vl);
    G_UNUSED(v);
    threadpool::wait(this);
    G_ASSERT(is_main_thread());
    gvars.clear();                       // Free framemem allocated data
    return vl ? &riExRenderer : nullptr; // zero vbase means that either job wasn't started, there is nothing to render or vb lock
                                         // failed
  }
} render_ri_extra_job;
void rendinst::render::prepareToStartAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, int frame_stblk, TexStreamingContext texCtx,
  bool enable)
{
  return render_ri_extra_job.prepare(vis, frame_stblk, enable, texCtx);
}
void rendinst::render::startPreparedAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, bool wake)
{
  return render_ri_extra_job.start(vis, wake);
}
void rendinst::render::waitAsyncRIGenExtraOpaqueRenderVbFill(const RiGenVisibility *vis)
{
  if (interlocked_acquire_load_ptr(render_ri_extra_job.vbase) == vis)
    spin_wait([&] { return interlocked_acquire_load(vis->riex.vbExtraGeneration) == INVALID_VB_EXTRA_GEN; });
}
rendinst::render::RiExtraRenderer *rendinst::render::waitAsyncRIGenExtraOpaqueRender(const RiGenVisibility *vis)
{
  return render_ri_extra_job.wait(vis);
}

using RiExtraRendererWithDynVarsCache = rendinst::render::RiExtraRendererT<framemem_allocator, rendinst::render::CachedDynVarsPolicy>;

void rendinst::render::renderRIGenExtra(const RiGenVisibility &vbase, RenderPass render_pass,
  OptimizeDepthPass optimization_depth_pass, OptimizeDepthPrepass optimization_depth_prepass,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, LayerFlag layer, uint32_t instance_multiply,
  TexStreamingContext texCtx, AtestStage atest_stage, const RiExtraRenderer *riex_renderer)
{
  TIME_D3D_PROFILE(render_ri_extra);

  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  const RiGenExtraVisibility &v = vbase.riex;
  if (!v.riexInstCount)
    return;

  rendinst::render::VbExtraCtx &vbexctx = v.vbexctx ? *v.vbexctx : rendinst::render::vbExtraCtx[0];
  if (!fill_vbextra(const_cast<RiGenExtraVisibility &>(v), nullptr, vbexctx))
    return;

  CallRenderExtraMarkerCallbacksRAII callRenderExtraMarkerCallbacks;

  rendinst::render::rebuildAllElems();

  ccExtra.lockRead(); //?!
  RingDynamicSB *vb = vbexctx.vb;
  rendinst::render::startRenderInstancing();
  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);

  const int blockToSet =
    (layer == LayerFlag::Transparent) ? rendinst::render::rendinstSceneTransBlockId : rendinst::render::rendinstSceneBlockId;
  const bool needToSetBlock = rendinst::render::setBlock(blockToSet, rendinst::render::riExtraPerDrawData);

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, vb->getRenderBuf());

  dag::ConstSpan<uint16_t> riResOrder = v.riexPoolOrder;
  if (layer == LayerFlag::Transparent || layer == LayerFlag::Decals || layer == LayerFlag::Distortion)
    riResOrder = riExPoolIdxPerStage[get_layer_index(layer)];

  const auto doRender = [&riResOrder](const auto &any_renderer) {
    any_renderer.renderSortedMeshesPacked(riResOrder);
    any_renderer.template renderSortedMeshes<RiExtraRenderer::NO_GPU_INSTANCING>(riResOrder);
  };

  if (riex_renderer)
    doRender(*riex_renderer);
  else
  {
    FRAMEMEM_REGION;
    RiExtraRendererWithDynVarsCache riexr(riResOrder.size(), layer, render_pass, optimization_depth_prepass, optimization_depth_pass,
      ignore_optimization_instances_limits, instance_multiply);

    if (layer & LayerFlag::Opaque && atest_stage == AtestStage::NoAtest)
      riexr.setNewStages(ShaderMesh::STG_opaque, ShaderMesh::STG_opaque);
    else if (layer & LayerFlag::Opaque && atest_stage == AtestStage::Atest)
      riexr.setNewStages(ShaderMesh::STG_atest, ShaderMesh::STG_atest);
    else if (layer & LayerFlag::Opaque && atest_stage == AtestStage::AtestAndImmDecal)
      riexr.setNewStages(ShaderMesh::STG_atest, ShaderMesh::STG_imm_decal);
    else if (layer & LayerFlag::Opaque && atest_stage == AtestStage::NoImmDecal)
      riexr.setNewStages(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

    riexr.addObjectsToRender(v, riResOrder, texCtx, optimization_depth_prepass, ignore_optimization_instances_limits);
    riexr.sortMeshesByMaterial();

    doRender(riexr);
  }

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  rendinst::render::endRenderInstancing();

  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_MIXED);
  ccExtra.unlockRead();
}

void rendinst::render::renderRIGenExtraSortedTransparentInstanceElems(const RiGenVisibility &vbase, const TexStreamingContext &tex_ctx,
  bool draw_partitioned_elems)
{
  TIME_D3D_PROFILE(render_ri_extra_per_instance_sorted);

  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  const RiGenExtraVisibility &v = vbase.riex;

  G_ASSERT(v.sortedTransparentElems.size() >= v.partitionedElemsCount);
  if (draw_partitioned_elems && v.partitionedElemsCount == 0 ||
      !draw_partitioned_elems && (v.sortedTransparentElems.size() - v.partitionedElemsCount) == 0)
    return;

  VbExtraCtx &vbexctx = v.vbexctx ? *v.vbexctx : vbExtraCtx[0];
  if (!fill_vbextra(const_cast<RiGenExtraVisibility &>(v), nullptr, vbexctx))
    return;

  ccExtra.lockRead();
  RingDynamicSB *vb = vbexctx.vb;

  rendinst::render::startRenderInstancing();
  int prevRenderType = ShaderGlobal::get_int(globalRendinstRenderTypeVarId);
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);

  const int blockToSet = rendinst::render::rendinstSceneTransBlockId;
  const bool needToSetBlock = rendinst::render::setBlock(blockToSet, rendinst::render::riExtraPerDrawData);

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, vb->getRenderBuf());

  dag::ConstSpan<uint16_t> riResOrder = riExPoolIdxPerStage[get_layer_index(rendinst::LayerFlag::Transparent)];

  {
    RiExtraRendererWithDynVarsCache riexr(riResOrder.size(), rendinst::LayerFlag::Transparent, rendinst::RenderPass::Normal,
      rendinst::OptimizeDepthPrepass::No, rendinst::OptimizeDepthPass::No, rendinst::IgnoreOptimizationLimits::Yes, 1);

    const auto &elems = v.sortedTransparentElems;
    for (int i = draw_partitioned_elems ? 0 : v.partitionedElemsCount,
             n = draw_partitioned_elems ? v.partitionedElemsCount : elems.size();
         i < n; i++)
    {
      const RiGenExtraVisibility::PerInstanceElem &elem = elems[i];

      int count = 1;
      while (i + 1 < n && elems[i + 1].poolId == elem.poolId && elems[i + 1].lod == elem.lod)
      {
        i++;
        count++;
      }

      int optimizationInstances = 0;
      bool optimization_depth_prepass = false;
      bool ignore_optimization_instances_limits = true;
      IPoint2 ofsAndCount(elem.vbOffset, count);
      float dist2 = bitwise_cast<float>(elem.partitionFlagDist2 & (uint32_t(-1) >> 1));
      riexr.addObjectToRender(elem.poolId, optimizationInstances, optimization_depth_prepass, ignore_optimization_instances_limits,
        ofsAndCount, elem.lod, elem.poolOrder, tex_ctx, dist2, 0.0f);
    }
    if (debug_mesh::is_enabled())
      riexr.coalesceDrawcalls<true>(false);
    else
      riexr.coalesceDrawcalls<false>(false);
    riexr.renderSortedMeshesPacked(riResOrder);
    riexr.renderSortedMeshes<RiExtraRenderer::NO_GPU_INSTANCING>(riResOrder);
  }

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);

  rendinst::render::endRenderInstancing();

  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, prevRenderType);
  ccExtra.unlockRead();
}

void rendinst::render::renderSortedTransparentRiExtraInstances(const RiGenVisibility &v, const TexStreamingContext &tex_ctx,
  bool draw_partitioned_elems)
{
  renderRIGenExtraSortedTransparentInstanceElems(v, tex_ctx, draw_partitioned_elems);
}

void rendinst::render::ensureElemsRebuiltRIGenExtra(bool gpu_instancing)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  rendinst::render::rebuildAllElems();

  if (gpu_instancing && !rendinst::render::relemsForGpuObjectsHasRebuilded)
  {
    rendinst::gpuobjects::rebuild_gpu_instancing_relem_params();
    rendinst::render::relemsForGpuObjectsHasRebuilded = true;
  }
}

void rendinst::render::renderRIGenExtraFromBuffer(Sbuffer *buffer, dag::ConstSpan<IPoint2> offsets_and_count,
  dag::ConstSpan<uint16_t> ri_indices, dag::ConstSpan<uint32_t> lod_offsets, RenderPass render_pass,
  OptimizeDepthPass optimization_depth_pass, OptimizeDepthPrepass optimization_depth_prepass,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, LayerFlag layer, ShaderElement *shader_override,
  uint32_t instance_multiply, bool gpu_instancing, Sbuffer *indirect_buffer, Sbuffer *ofs_buffer)
{
  TIME_D3D_PROFILE(render_ri_extra_from_buffer);

  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  if (offsets_and_count.empty())
    return;
  G_ASSERT(lod_offsets.size() <= RiExtraPool::MAX_LODS);

  // rebuild will happen in the next before draw
  if (gpu_instancing && !relemsForGpuObjectsHasRebuilded)
    return;

  rendinst::render::startRenderInstancing();
  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);
  ShaderGlobal::set_real_fast(useRiGpuInstancingVarId, gpu_instancing ? 1.0f : 0.0f);
  rendinst::render::setCoordType(rendinst::render::COORD_TYPE_TM);

  const int blockToSet =
    (layer == LayerFlag::Transparent) ? rendinst::render::rendinstSceneTransBlockId : rendinst::render::rendinstSceneBlockId;
  const bool needToSetBlock = rendinst::render::setBlock(blockToSet, riExtraPerDrawData);

  ShaderGlobal::set_int(rendinst::render::rendinstRenderPassVarId, eastl::to_underlying(render_pass));

  d3d::set_buffer(STAGE_VS, rendinst::render::instancingTexRegNo, buffer);
  if (gpu_instancing)
    d3d::set_buffer(STAGE_VS, rendinst::render::GPU_INSTANCING_OFSBUFFER_TEXREG, ofs_buffer);

  FRAMEMEM_REGION;
  RiExtraRendererWithDynVarsCache renderer(ri_indices.size(), layer, render_pass, optimization_depth_prepass, optimization_depth_pass,
    ignore_optimization_instances_limits, instance_multiply);
  if (shader_override)
    renderer.setNewStages(ShaderMesh::STG_opaque, ShaderMesh::STG_imm_decal);

  int numLods = min<int>(lod_offsets.size(), RiExtraPool::MAX_LODS);
  for (int lod = 0; lod < numLods; ++lod)
    for (int i = lod_offsets[lod], e = (lod == numLods - 1) ? ri_indices.size() : lod_offsets[lod + 1]; i < e; ++i)
    {
      if (offsets_and_count[i].y == 0)
        continue;
      renderer.addObjectToRender(ri_indices[i], 0, optimization_depth_prepass == OptimizeDepthPrepass::Yes,
        ignore_optimization_instances_limits == IgnoreOptimizationLimits::Yes, offsets_and_count[i], lod, i,
        TexStreamingContext(FLT_MAX), 0.0f, 0.0f, shader_override, gpu_instancing);
    }

  renderer.sortMeshesByMaterial();
  if (gpu_instancing) // TODO: Implement packed rendering for this branch.
    renderer.renderSortedMeshes<RiExtraRenderer::USE_GPU_INSTANCING>(ri_indices, indirect_buffer);
  else
  {
    renderer.renderSortedMeshesPacked(ri_indices);
    renderer.renderSortedMeshes<RiExtraRenderer::NO_GPU_INSTANCING>(ri_indices);
  }

  ShaderGlobal::set_real_fast(useRiGpuInstancingVarId, 0.0f);
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_MIXED);

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);


  rendinst::render::endRenderInstancing();
}

void rendinst::render::setRIGenExtraDiffuseTexture(uint16_t ri_idx, int tex_var_id)
{
  rendinst::RiExtraPool &riPool = rendinst::riExtra[ri_idx];
  RenderableInstanceResource *rendInstRes = riPool.res->lods[0].scene;
  ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
  dag::Span<ShaderMesh::RElem> elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_decal);
  ShaderGlobal::set_texture(tex_var_id, elems[0].mat->get_texture(0));
}

void rendinst::gatherRIGenExtraShadowInvisibleBboxes(Tab<bbox3f> &out_bboxes, bbox3f_cref gather_box)
{
  for (auto &scene : riExTiledScenes.scenes())
  {
    scene.boxCull<true, true>(gather_box, RendinstTiledScene::CHECKED_IN_SHADOWS, RendinstTiledScene::CHECKED_IN_SHADOWS,
      [&](scene::node_index ni, mat44f_cref m) {
        // For some reason using test_flags and equal_flags to get tested invisible
        // boxes produces wrong results.
        // Number of objects received with test_flags = CHECKED_IN_SHADOWS | VISIBLE_IN_SHADOWS, equal_flags = CHECKED_IN_SHADOWS |
        // VISIBLE_IN_SHADOWS plus number of objects received with test_flags = CHECKED_IN_SHADOWS | VISIBLE_IN_SHADOWS, equal_flags =
        // CHECKED_IN_SHADOWS not equal to number of objects received with test_flags = CHECKED_IN_SHADOWS, equal_flags =
        // CHECKED_IN_SHADOWS.
        if (!(scene.getNodeFlags(ni) & RendinstTiledScene::VISIBLE_IN_SHADOWS))
          out_bboxes.push_back(scene.calcNodeBox(m));
      });
  }
}

bool rendinst::gatherRIGenExtraBboxes(const RiGenVisibility *main_visibility, mat44f_cref volume_box,
  eastl::function<void(mat44f_cref, const BBox3 &, const char *)> callback)
{
  const RiGenExtraVisibility &extraVisibility = main_visibility->riex;
  if (!extraVisibility.riexInstCount)
    return false;
  bool updated = false;
  mat44f to_volume_tm;
  v_mat44_inverse43(to_volume_tm, volume_box);

  for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
  {
    for (auto poolAndCnt : extraVisibility.riexPoolOrder)
    {
      auto poolI = poolAndCnt & render::RI_RES_ORDER_COUNT_MASK;
      const vec4f *data = extraVisibility.riexData[l][poolI].data();
      const uint32_t poolCnt = (uint32_t)extraVisibility.riexData[l][poolI].size() / RIEXTRA_VECS_COUNT;
      const rendinst::RiExtraPool &riPool = rendinst::riExtra[poolI];
      BBox3 box;
      v_stu_bbox3(box, riPool.lbb);
      vec4f minMax[2] = {riPool.lbb.bmin, riPool.lbb.bmax};
      const char *riName = rendinst::riExtraMap.getName(poolI);
      for (uint32_t i = 0, n = poolCnt; i < n; ++i)
      {
        mat44f tm44 = *(const mat44f *)(data + i * RIEXTRA_VECS_COUNT);
        v_mat44_transpose(tm44, tm44);
        bool inside = true;
        for (uint32_t j = 0; j < 2; j++) // check only max and min
        {
          vec4f pos = v_mat44_mul_vec3p(tm44, minMax[j]);
          pos = v_mat44_mul_vec3p(to_volume_tm, pos);
          pos = v_cmp_gt(v_abs(pos), V_C_HALF);
          inside &= v_signmask(pos) == 0;
        }
        if (inside)
        {
          callback(tm44, box, riName);
          updated = true;
        }
      }
    }
  }
  return updated;
}

void rendinst::render::collectPixelsHistogramRIGenExtra(const RiGenVisibility *main_visibility, vec4f camera_fov,
  float histogram_scale, eastl::vector<eastl::vector_map<eastl::string_view, int>> &histogram)
{
  const RiGenExtraVisibility &extraVisibility = main_visibility->riex;
  if (!extraVisibility.riexInstCount)
    return;
  float tg = tan(v_extract_w(camera_fov) * 0.5f);
  size_t binCount = histogram.size();

  for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
  {
    for (auto poolAndCnt : extraVisibility.riexPoolOrder)
    {
      auto poolI = poolAndCnt & RI_RES_ORDER_COUNT_MASK;
      const vec4f *data = extraVisibility.riexData[l][poolI].data();
      const uint32_t poolCnt = (uint32_t)extraVisibility.riexData[l][poolI].size() / RIEXTRA_VECS_COUNT;
      const rendinst::RiExtraPool &riPool = rendinst::riExtra[poolI];
      float radius = v_extract_x(v_bbox3_outer_rad(riPool.lbb));
      const char *riName = rendinst::riExtraMap.getName(poolI);
      for (uint32_t i = 0, n = poolCnt; i < n; ++i)
      {
        mat44f tm44 = *(const mat44f *)(data + i * RIEXTRA_VECS_COUNT);
        v_mat44_transpose(tm44, tm44);
        float distance = v_extract_x(v_length3(v_sub(tm44.col3, camera_fov)));

        float partOfScreen = radius / (tg * distance);

        int bin = histogram_scale * partOfScreen * binCount;
        if (bin >= binCount)
          bin = binCount - 1;
        histogram[bin][eastl::string_view(riName)]++;
      }
    }
  }
}

void rendinst::render::validateFarLodsWithHeavyShaders(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale)
{
  const RiGenExtraVisibility &extraVisibility = main_visibility->riex;
  if (!extraVisibility.riexInstCount)
    return;

  float tg = tan(v_extract_w(camera_fov) * 0.5f);

  for (uint32_t lod = 2; lod < rendinst::RiExtraPool::MAX_LODS; lod++)
  {
    for (uint16_t poolAndCnt : extraVisibility.riexPoolOrder)
    {
      uint32_t poolI = poolAndCnt & RI_RES_ORDER_COUNT_MASK;
      const uint32_t poolCnt = (uint32_t)extraVisibility.riexData[lod][poolI].size() / RIEXTRA_VECS_COUNT;
      if (poolCnt == 0)
        continue;

      const vec4f *data = extraVisibility.riexData[lod][poolI].data();
      const rendinst::RiExtraPool &riPool = rendinst::riExtra[poolI];
      float radius = v_extract_x(v_bbox3_outer_rad(riPool.lbb));
      uint32_t smallObjectsCount = 0;
      for (uint32_t i = 0, n = poolCnt; i < n; ++i)
      {
        mat44f tm44 = *(const mat44f *)(data + i * RIEXTRA_VECS_COUNT);
        v_mat44_transpose(tm44, tm44);
        float distance = v_extract_x(v_length3(v_sub(tm44.col3, camera_fov)));

        float partOfScreen = radius / (tg * distance);

        if (histogram_scale * partOfScreen < 1.0)
          smallObjectsCount++;
      }
      if (smallObjectsCount == 0)
        continue;

      const uint32_t HEAVY_SHADERS_COUNT = 3;
      // TODO: Move this list to some blk
      const eastl::array<const char *, HEAVY_SHADERS_COUNT> HEAVY_SHADERS = {
        "rendinst_perlin_layered", "rendinst_mask_layered", "rendinst_vcolor_layered"};
      eastl::array<uint32_t, HEAVY_SHADERS_COUNT> hasShader = {0, 0, 0};

      const uint32_t startStage = ShaderMesh::STG_opaque;
      uint32_t startEIOfs = (lod * rendinst::riExtra.size() + poolI) * ShaderMesh::STG_COUNT + startStage;
      for (uint32_t EI = rendinst::render::allElemsIndex[startEIOfs], endEI = rendinst::render::allElemsIndex[startEIOfs + 1];
           EI < endEI; ++EI)
      {
        auto &elem = rendinst::render::allElems[EI];
        if (!elem.shader)
          continue;

        for (uint32_t i = 0; i < HEAVY_SHADERS_COUNT; ++i)
        {
          if (strcmp(static_cast<ShaderElement *>(elem.shader)->getShaderClassName(), HEAVY_SHADERS[i]) == 0)
            hasShader[i]++;
        }
      }
      for (uint32_t i = 0; i < HEAVY_SHADERS_COUNT; ++i)
      {
        if (hasShader[i])
          debug("HEAVY SHADERS: We render %d instances of %s with LOD %d and shader %s (%d heavy drawcalls)", smallObjectsCount,
            rendinst::riExtraMap.getName(poolI), lod, HEAVY_SHADERS[i], hasShader[i]);
      }
    }
  }
}

void rendinst::render::registerRIGenExtraRenderMarkerCb(ri_extra_render_marker_cb cb)
{
  if (find_value_idx(ri_extra_render_marker_callbacks, cb) == -1)
    ri_extra_render_marker_callbacks.push_back(cb);
}

bool rendinst::render::unregisterRIGenExtraRenderMarkerCb(ri_extra_render_marker_cb cb)
{
  return erase_item_by_value(ri_extra_render_marker_callbacks, cb);
}


void rendinst::render::write_ri_extra_per_instance_data(vec4f *dest_buffer, const RendinstTiledScene &tiled_scene,
  scene::pool_index pool_id, scene::node_index ni, mat44f_cref m, bool is_dynamic)
{
  // dest_buffer is a 4x4 mx. The first 3 rows are the matrix, the 4th row is the additional data
  // 4th row: x - 1 bit is_dynamic, 31 bits perDataBufferOffset
  // y - perInstanceDataOffset
  // zw - scene node user data (up to 2 words), first word is usually custom seed or hash set by editor
  const int32_t *userData = tiled_scene.getUserData(ni);
  if (userData)
  {
    int userDataSize = min(2, tiled_scene.getUserDataWordCount());
    eastl::copy_n(userData, userDataSize, (uint32_t *)(dest_buffer + ADDITIONAL_DATA_IDX));
  }
  v_mat44_transpose_to_mat43(*(mat43f *)dest_buffer, m);

  uint32_t perInstanceDataOffset = 0;
  if (scene::check_node_flags(m, RendinstTiledScene::HAS_PER_INSTANCE_RENDER_DATA))
    perInstanceDataOffset = tiled_scene.getPerInstanceRenderDataOffset(ni);

  uint32_t perDataBufferOffset = pool_id * (sizeof(rendinst::render::RiShaderConstBuffers) / sizeof(vec4f)) + 1;

  G_ASSERT(!(perDataBufferOffset & (1 << 31)));
  perDataBufferOffset |= (uint32_t)is_dynamic << 31;
  vec4i extraData = v_make_vec4i(perDataBufferOffset, perInstanceDataOffset, 0, 0);
  dest_buffer[ADDITIONAL_DATA_IDX] = v_perm_xyab(v_cast_vec4f(extraData), dest_buffer[ADDITIONAL_DATA_IDX]);
}