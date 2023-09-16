#include "riGen/riGenExtraRender.h"
#include "riGen/riGenRender.h"
#include "riGen/riUtil.h"
#include "riGen/extraRender/consoleHandler.h"
#include "riGen/extraRender/cachedDynVarsPolicy.h"
#include "riGen/extraRender/opaqueGlobalDynVarsPolicy.h"

#include <workCycle/dag_workCycle.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_mathUtils.h>
#include <scene/dag_occlusionMap.h>
#include <util/dag_convar.h>
#include <ioSys/dag_fileIo.h>
#include <util/dag_stlqsort.h>
#include <EASTL/sort.h>
#include <EASTL/hash_set.h>
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <EASTL/fixed_vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <shaders/dag_renderStateId.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_dynVariantsCache.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_threadPool.h>
#include <util/dag_console.h>
#include <vecmath/dag_vecMath.h>
#include <vecmath/dag_vecMath.h>
#include <render/gpuVisibilityTest.h>
#include <gpuObjects/gpuObjects.h>

// for console comand
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>

float rendinst::riExtraLodDistSqMul = 1.f;
float rendinst::riExtraCullDistSqMul = 1.f;
float rendinstgenrender::riExtraMinSizeForReflection = 25.f;
float rendinstgenrender::riExtraMinSizeForDraftDepth = 25.f;
rendinst::VbExtraCtx rendinst::vbExtraCtx[2];
UniqueBufHolder rendinst::perDrawData;

const uint32_t ADDITIONAL_DATA_IDX = rendinst::RIEXTRA_VECS_COUNT - 1; // It is always last vector


__forceinline rendinst::riex_render_info_t make_riex_render_info_handle(uint32_t scene, scene::node_index ni)
{
  G_ASSERT(scene <= 3 && ni <= 0x3FFFFFFF);
  return (uint32_t(scene) << uint32_t(30)) | uint32_t(ni);
}

__forceinline scene::node_index get_riex_render_node_index(rendinst::riex_render_info_t handle) { return handle & 0x3FFFFFFF; }
__forceinline uint32_t get_riex_render_scene_index(rendinst::riex_render_info_t handle) { return handle >> 30; }

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

bool isRiGenVisibilityLodsLoaded(const RiGenVisibility *visibility)
{
  const RiGenExtraVisibility &v = visibility->riex;
  dag::ConstSpan<uint16_t> riResOrder = v.riexPoolOrder;
  for (int k = 0, ke = riResOrder.size(); k < ke; ++k)
  {
    auto ri_idx = riResOrder[k] & RI_RES_ORDER_COUNT_MASK;
    RenderableInstanceLodsResource *res = rendinst::riExtra[ri_idx].res;
    int bestLod = res->getQlBestLod();
    if (bestLod > v.forcedExtraLod)
    {
      res->updateReqLod(min<int>(v.forcedExtraLod, res->lods.size() - 1));
      return false;
    }
  }
  return true;
}

}; // namespace rendinst


rendinst::RITiledScenes rendinst::riExTiledScenes;
float rendinst::riExTiledSceneMaxDist[4] = {0};


namespace rendinstgenrender
{
extern Point3_vec4 dir_from_sun; // fixme it should be initialized to most glancing angle
static eastl::hash_set<eastl::string> dynamicRiExtra;
}; // namespace rendinstgenrender


void rendinst::setDirFromSun(const Point3 &d)
{
  // todo: if direction is too diferent we should schedule recalculation of distances
  rendinstgenrender::dir_from_sun = d;
  if (!riExTiledScenes.size())
    return;
  for (auto &scene : riExTiledScenes.scenes(STATIC_SCENES_START, riExTiledScenes.size() - STATIC_SCENES_START))
    scene.setDirFromSun(d);
}

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
    tiled_scene.frustumCull<true, false, false>(globtm, vpos_distscale, flag, flag, NULL,
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

static constexpr int MAX_OPTIMIZATION_INSTANCES = 3;
#define MIN_OPTIMIZATION_DIST 90.f

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("rendinst", debugDumpScene, false);
CONSOLE_INT_VAL("rendinst", parallel_for, 7, 0, 8);
#endif

void rendinst::RendinstTiledScene::debugDumpScene(const char *file_name)
{
  FullFileSaveCB cb(file_name);
  scene::save_simple_scene(cb, getBaseSimpleScene());
}

void rendinst::setRIGenExtraResDynamic(const char *ri_res_name)
{
  rendinstgenrender::dynamicRiExtra.insert(ri_res_name);
  int res_idx = rendinst::getRIGenExtraResIdx(ri_res_name);
  if (res_idx >= 0)
    riExtra[res_idx].tsIndex = DYNAMIC_SCENE;
}

bool rendinst::isRIGenExtraDynamic(int res_idx) { return (riExtra[res_idx].tsIndex == DYNAMIC_SCENE); }

namespace rendinstgen
{
extern bool custom_trace_ray_earth(const Point3 &src, const Point3 &dir, real &dist);
};

static uint16_t get_pool_flags(rendinst::RiExtraPool &pool)
{
  uint16_t flag = 0;

  const float bboxSize = v_extract_x(v_length3_x(v_sub(pool.lbb.bmax, pool.lbb.bmin))) * pool.scaleForPrepasses;

  if (bboxSize >= rendinstgenrender::riExtraMinSizeForReflection) // visible in reflections
    flag |= rendinst::RendinstTiledScene::REFLECTION;
  if (bboxSize >= rendinstgenrender::riExtraMinSizeForDraftDepth) // rendered to draftDepth
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
  rendinstgenrender::dynamicRiExtra.clear();
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
        s.setDistance(ni, *(uint8_t *)d);
        {
          const mat44f &node = s.getNode(ni);
          bbox3f wabb;
          vec4f sphere = scene::get_node_bsphere(node);
          wabb.bmin = v_sub(sphere, v_splat_w(sphere));
          wabb.bmax = v_add(sphere, v_splat_w(sphere));
          v_bbox3_add_box(riExTiledScenes.newlyCreatedNodesBox, wabb);
        }
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
    }
    return true;
  }
};

void RendinstTiledScene::setDirFromSun(const Point3 &nd)
{
  if (dot(dirFromSunForShadows, nd) < 0.97)
  {
    dirFromSunForShadows = nd;
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
  const bool isDynamic = name && rendinstgenrender::dynamicRiExtra.find_as(name) != rendinstgenrender::dynamicRiExtra.end();
  if (riExTiledScenes.size())
  {
    TiledScenePoolInfo info = {
      pool.distSqLOD[0], pool.distSqLOD[1], pool.distSqLOD[2], pool.distSqLOD[3], pool.lodLimits, 0xFFFF, 0xFFFF, isDynamic};
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
    if (pool.hasImpostor()) // todo: or we know it is dynamic object, like door/window
      pool.tsIndex = DYNAMIC_SCENE;
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

  oscene.destroy(ni);
  scene::node_index nni = nscene.allocate(tm44, pool_idx, get_pool_flags(pool));
  nscene.setNodeUserData(nni, RendinstTiledScene::ADDED,
    (originalSceneId == DYNAMIC_SCENE) ? nscene.LIGHTDIST_DYNAMIC : nscene.LIGHTDIST_INVALID,
    SetSceneUserData(nscene, riExTiledScenes));
  if (add_data_dwords)
    nscene.setNodeUserDataEx(nni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data, SetSceneUserData(nscene, riExTiledScenes));

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

    oscene.destroy(ni);
    auto &nscene = riExTiledScenes[DYNAMIC_SCENE];
    scene::node_index nni = nscene.allocate(tm44, pool_idx, get_pool_flags(pool));
    nscene.setNodeUserData(nni, RendinstTiledScene::ADDED, RendinstTiledScene::LIGHTDIST_DYNAMIC,
      SetSceneUserData(nscene, riExTiledScenes));
    if (add_data_dwords)
      nscene.setNodeUserDataEx(nni, RendinstTiledScene::SET_UDATA, add_data_dwords, add_data,
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
    G_ASSERTF(start == 0, " implement this command with start offset!");
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
    G_ASSERTF(start == 0, " implement this command with start offset!");
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
  riExTiledScenes[nodeId >> 30].destroy(nodeId & 0x3FFFFFFF);
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
    rendinst::add_ri_pool_to_tiled_scenes(pool, res_idx, NULL, sqrtf(pool.distSqLOD[rendinst::RiExtraPool::MAX_LODS - 1]));
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
        init_instance_user_data_for_tiled_scene(pool, ni, sphere, 0, NULL);
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

template <int N>
inline int find_lod(const float *__restrict lod_dists, float dist)
{
  dist *= rendinstgenrender::riExtraLodsShiftDistMul;
  for (int lod = 0; lod <= N - 1; lod++)
    if (lod_dists[lod] < dist)
      return lod;
  return N;
}

template <>
inline int find_lod<4>(const float *__restrict lod_dists, float dist)
{
  dist *= rendinstgenrender::riExtraLodsShiftDistMul;
  if (dist < lod_dists[1])
    return (dist < lod_dists[0] ? 0 : 1);
  return (dist < lod_dists[2] ? 2 : 3);
}

int rendinst::getRIGenExtraPreferrableLod(int res_idx, float squared_distance)
{
  if (res_idx < 0 || res_idx >= rendinst::riExtra.size())
    return -1;
  squared_distance *= rendinst::riExtraCullDistSqMul;
  const rendinst::RiExtraPool &riPool = rendinst::riExtra[res_idx];
  int lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, squared_distance);
  const int llm = riPool.lodLimits >> ((rendinst::ri_game_render_mode + 1) * 8);
  const int min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
  lod = clamp(lod, min_lod, max_lod);
  return lod;
}

static inline void swap_data(SmallTab<vec4f> &data, uint32_t i0, uint32_t i1, const uint32_t vecs_count)
{
  vec4f temp[32];
  G_ASSERT(vecs_count < countof(temp));
  i0 *= vecs_count;
  i1 *= vecs_count;
  const uint32_t dataSize = vecs_count * sizeof(vec4f);
  memcpy(temp, data.data() + i1, dataSize);
  memcpy(data.data() + i1, data.data() + i0, dataSize);
  memcpy(data.data() + i0, temp, dataSize);
}

template <typename T>
static inline T *append_data(SmallTab<T> &data, const uint32_t vecs_count)
{
  const size_t cSize = data.size();
  data.resize(cSize + vecs_count);
  return data.data() + cSize;
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

namespace rendinst
{
static constexpr int MAX_CULL_JOBS = 8;

struct CullJobSharedState
{
  mat44f globtm = {};
  vec4f vpos_distscale = {};
  Occlusion *use_occlusion = nullptr;
  RiGenExtraVisibility *v = nullptr;
  dag::ConstSpan<RendinstTiledScene> scenes;
  scene::TiledSceneCullContext *sceneContexts = nullptr;
  const eastl::vector<TiledScenePoolInfo> *poolInfo = nullptr;
  std::atomic<uint32_t> *riexDataCnt = nullptr;
  std::atomic<uint32_t> *riexLargeCnt = nullptr;
  SmallTab<Point2, framemem_allocator> *perPoolMinDist = nullptr;
  SmallTab<RiGenExtraVisibility::UVec2, framemem_allocator> *perPoolBestId = nullptr;
};

class CullJob final : public cpujobs::IJob
{
  CullJobSharedState *parent = 0;
  int jobIdx = 0;

public:
  void set(int job_idx, CullJobSharedState *parent_)
  {
    jobIdx = job_idx;
    parent = parent_;
  }

  static inline void perform_job(int job_idx, CullJobSharedState *info)
  {
    TIME_PROFILE(parallel_ri_cull_job);

    dag::ConstSpan<RendinstTiledScene> scenes = info->scenes;
    mat44f globtm = info->globtm;
    vec4f vpos_distscale = info->vpos_distscale;
    auto use_occlusion = info->use_occlusion;

    auto &v = *info->v;
    G_ASSERTF(v.forcedExtraLod < 0, "CullJob is expected to not have forcedExtraLod enabled.");
    auto &poolInfo = *info->poolInfo;
    const bool sortLarge = use_occlusion && check_occluders;
    std::atomic<uint32_t> *riexDataCnt = info->riexDataCnt;
    std::atomic<uint32_t> *riexLargeCnt = info->riexLargeCnt;

    for (int tiled_scene_idx = 0; tiled_scene_idx < scenes.size(); tiled_scene_idx++)
    {
      const auto &tiled_scene = scenes[tiled_scene_idx];
      scene::TiledSceneCullContext &ctx = info->sceneContexts[tiled_scene_idx];
      while (!ctx.tilesPassDone.load() || ctx.nextIdxToProcess.load(std::memory_order_relaxed) < ctx.tilesCount)
      {
        const int next_idx = ctx.nextIdxToProcess.fetch_add(1);
        spin_wait([&] { return next_idx >= ctx.tilesCount && !ctx.tilesPassDone.load(); });
        if (next_idx >= ctx.tilesCount)
          break;

        int tile_idx = ctx.tilesPtr[next_idx];
        // debug("  cull %d:[%d]=%d", tiled_scene_idx, next_idx, tile_idx);
        auto cb = [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
          static constexpr int LARGE_LOD_CNT = RiGenExtraVisibility::LARGE_LOD_CNT;
          G_UNUSED(ni);
          const scene::pool_index poolId = scene::get_node_pool(m);
          const auto &riPool = poolInfo[poolId];
          float dist = v_extract_x(distSqScaled);
          unsigned lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, dist);
          const unsigned llm = riPool.lodLimits >> ((ri_game_render_mode + 1) * 8);
          const unsigned min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
          if (lod > max_lod)
            return;
          lod = clamp(lod, min_lod, max_lod);
          int cnt_idx = poolId * rendinst::RiExtraPool::MAX_LODS + lod;
          int new_sz = riexDataCnt[cnt_idx].fetch_add(RIEXTRA_VECS_COUNT) + RIEXTRA_VECS_COUNT;
          if (new_sz > v.riexData[lod].data()[poolId].size())
          {
            if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
              riexLargeCnt[poolId * LARGE_LOD_CNT + lod]++;
            return;
          }

          vec4f *addData = v.riexData[lod].data()[poolId].data() + (new_sz - RIEXTRA_VECS_COUNT);
          const int32_t *userData = tiled_scene.getUserData(ni);
          const uint32_t curDistSq = interlocked_relaxed_load(*(const uint32_t *)(v.minSqDistances[lod].data() + poolId));
          const uint32_t distI = bitwise_cast<uint32_t>(dist); // since squared distance is positive, it is fine to compare in ints
                                                               // (bitwise representation of floats will still be in same order)
          if (distI < curDistSq)
          {
            // while we could make CAS min (in loop), or parallel sum (different arrays per thread), it is not important distance, and
            // used only for texture lod selection so, even a bit randomly it should work with relaxed load/store
            interlocked_relaxed_store(*(uint32_t *)(v.minSqDistances[lod].data() + poolId), distI);
          }
          if (userData)
            eastl::copy_n(userData, tiled_scene.getUserDataWordCount(), (uint32_t *)(addData + ADDITIONAL_DATA_IDX));
          v_mat44_transpose_to_mat43(*(mat43f *)addData, m);
          uint32_t perDataBufferOffset = poolId * (sizeof(rendinstgenrender::RiShaderConstBuffers) / sizeof(vec4f)) + 1;
          vec4i extraData = v_make_vec4i(0, perDataBufferOffset, (poolId << 1) | uint32_t(riPool.isDynamic), 0);
          addData[ADDITIONAL_DATA_IDX] = v_perm_ayzw(v_cast_vec4f(extraData), addData[ADDITIONAL_DATA_IDX]);

          const uint32_t id = new_sz / RIEXTRA_VECS_COUNT - 1;
          vec4f rad = scene::get_node_bsphere_vrad(m);
          rad = v_div_x(distSqScaled, v_mul_x(rad, rad));
          float sdist = v_extract_x(rad);
          if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
          {
            cnt_idx = poolId * LARGE_LOD_CNT + lod;
            new_sz = riexLargeCnt[cnt_idx].fetch_add(1) + 1;
            if (new_sz > v.riexLarge[lod].data()[poolId].size())
              return;
            // this is almost as fast as using dist2 and is technically more correct.
            // however, since large occluders are usually not scaled, their radius is constant, and
            //  v_dot3_x(sphere, sort_dir) isn't that much different from projected dist
            // vec4f sphere = scene::get_node_bsphere(m);
            // float sdist = v_extract_x(v_sub(v_dot3_x(sphere, sort_dir), v_splat_w(sphere)));
            v.riexLarge[lod].data()[poolId][new_sz - 1] = {bitwise_cast<int, float>(sdist), id};
          }

          auto &minDist = info->perPoolMinDist->data()[poolId + poolInfo.size() * job_idx];
          auto &bestId = info->perPoolBestId->data()[poolId + poolInfo.size() * job_idx];
          if (sdist < minDist.x)
          {
            minDist.y = minDist.x;
            bestId.y = bestId.x;
            minDist.x = sdist;
            bestId.x = (id) | (lod << 28);
          }
          else if (sdist < minDist.y)
          {
            minDist.y = sdist;
            bestId.y = (id) | (lod << 28);
          }
        };

        if (ctx.test_flags)
          tiled_scene.frustumCullOneTile<true, true, true>(ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
        else
          tiled_scene.frustumCullOneTile<false, true, true>(ctx, globtm, vpos_distscale, use_occlusion, tile_idx, cb);
      }
    }
  }
  virtual void doJob() override
  {
    if (!parent)
      return;
    perform_job(jobIdx, parent);
    parent = nullptr;
  }
};

struct CullJobRing
{
  StaticTab<CullJob, MAX_CULL_JOBS> jobs;
  CullJobSharedState info;
  uint32_t queue_pos = 0;
  threadpool::JobPriority prio = threadpool::PRIO_HIGH;

  void start(int num_jobs, const CullJobSharedState &jinfo)
  {
    G_FAST_ASSERT(num_jobs > 0);
    num_jobs = min(num_jobs - 1, (int)MAX_CULL_JOBS);
    info = jinfo;
    prio = is_main_thread() ? threadpool::PRIO_HIGH : threadpool::PRIO_NORMAL;
    jobs.resize(num_jobs);
    for (int i = 0; i < jobs.size(); ++i)
    {
      jobs[i].set(i + 1, &info);
      threadpool::add(&jobs[i], prio, queue_pos, threadpool::AddFlags::None);
    }
    threadpool::wake_up_all();
  }
  void finishWork()
  {
    CullJob::perform_job(0, &info);
    if (jobs.empty()) // could be if 1 worker
      return;

    // Warning: scenes are read locked at this point, so any job that want to grab write lock for it will deadlock
    barrier_active_wait_for_job(&jobs.back(), prio, queue_pos);

    DA_PROFILE_WAIT("wait_ri_cull_jobs");
    for (auto &j : jobs)
      threadpool::wait(&j);
  }
};
} // namespace rendinst

static void sortByPoolSizeOrder(RiGenExtraVisibility &v, int start_lod)
{
  // todo: use predefined order based on pool bbox size, so we render first biggest
  for (unsigned pool = 0; pool < v.riexData[0].size(); ++pool)
  {
    int lod = start_lod;
    for (; lod >= 0; --lod)
      if (v.riexData[lod][pool].size())
        break;
    if (lod >= 0)
      v.riexPoolOrder.push_back(pool);
  }
}

void rendinst::gatherRIGenExtraToTestForShadows(eastl::vector<GpuVisibilityTestManager::TestObjectData> &out_bboxes,
  mat44f_cref globtm_cull, float static_shadow_texel_size, uint32_t usr_data)
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
      RendinstTiledScene::NEEDS_CHECK_IN_SHADOW, NULL, [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
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

void rendinst::invalidateRIGenExtraShadowsVisibility()
{
  vec4f boxHalfSize = v_make_vec4f(1e6, 1e6, 1e6, 0);
  bbox3f box;
  box.bmin = v_neg(boxHalfSize);
  box.bmax = boxHalfSize;
  invalidateRIGenExtraShadowsVisibilityBox(box);
}

void rendinst::invalidateRIGenExtraShadowsVisibilityBox(bbox3f_cref box)
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

static eastl::pair<int, int> scene_range_from_visiblity_rendering_flags(rendinst::VisibilityRenderingFlags flags)
{
  eastl::pair<int, int> result{0, rendinst::riExTiledScenes.size()};

  // scene array structure is as follows;
  // [dynamic scene, static scene 1, static scene 2, ... static scene n]

  if (!(flags & rendinst::VisibilityRenderingFlag::Dynamic))
    result.first = rendinst::STATIC_SCENES_START;
  if (!(flags & rendinst::VisibilityRenderingFlag::Static))
    result.second = rendinst::STATIC_SCENES_START;

  return result;
}

template <bool use_external_filter>
bool rendinst::prepareExtraVisibilityInternal(mat44f_cref globtm_cull, const Point3 &camera_pos, RiGenVisibility &vbase,
  bool render_for_shadow, Occlusion *use_occlusion, rendinst::RiExtraCullIntention cullIntention, bool for_visual_collision,
  bool filter_rendinst_clipmap, bool for_vsm, const rendinst::VisibilityExternalFilter &external_filter)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount || RendInstGenData::isLoading)
    return false;
  TIME_PROFILE(riextra_visibility);
  const VisibilityRenderingFlags rendering = vbase.rendering;
  RiGenExtraVisibility &v = vbase.riex;
  if (ri_game_render_mode == 0)
    use_occlusion = NULL;
  mat44f globtm = globtm_cull;
#if DAGOR_DBGLEVEL > 0
  if (!render_for_shadow && use_occlusion)
    globtm = use_occlusion->getCurViewProj(); // allow "frustum stop" (add_occlusion console command)
#endif
  v.vbExtraGeneration = INVALID_VB_EXTRA_GEN;

  Point3_vec4 vpos = camera_pos;
  float distSqMul = rendinst::riExtraCullDistSqMul;
  vec4f vpos_distscale = v_perm_xyzd((vec4f &)vpos, v_splats(v.forcedExtraLod >= 0 ? -1.f : distSqMul));

  const auto &poolInfo = riExTiledScenes.getPools();

  for (int lod = 0; lod < rendinst::RiExtraPool::MAX_LODS; ++lod)
  {
    clear_and_resize(v.riexData[lod], poolInfo.size());
    clear_and_resize(v.minSqDistances[lod], poolInfo.size());
    memset(v.minSqDistances[lod].data(), 0x7f, data_size(v.minSqDistances[lod])); // ~FLT_MAX
    for (auto &vv : v.riexData[lod])
      vv.resize(0);
  }

  v.riexPoolOrder.resize(0);
  v.riexInstCount = 0;
  if (!riExTiledScenes.size())
    return false;
  uint32_t additionalData = riExTiledScenes[0].getUserDataWordCount(); // in dwords
  for (const auto &tiled_scene : riExTiledScenes.scenes())
  {
    G_ASSERTF(!additionalData || !tiled_scene.getUserDataWordCount() || additionalData == tiled_scene.getUserDataWordCount(),
      " %d == %d", additionalData, tiled_scene.getUserDataWordCount());
    if (!additionalData)
      additionalData = tiled_scene.getUserDataWordCount();
  }

  // can be made invisibleFlag, if testFlags = RendinstTiledScene::VISIBLE_0, equalFlags = ~RendinstTiledScene::VISIBLE_0;
  uint32_t visibleFlag = ri_game_render_mode == 0   ? RendinstTiledScene::VISIBLE_0
                         : ri_game_render_mode == 2 ? RendinstTiledScene::VISIBLE_2
                                                    : 0;
  const auto [firstScene, lastScene] = scene_range_from_visiblity_rendering_flags(rendering);
  const int sceneCount = lastScene - firstScene;

  const bool sortLarge = !render_for_shadow && use_occlusion && check_occluders;
  static constexpr int LARGE_LOD_CNT = RiGenExtraVisibility::LARGE_LOD_CNT;

  if (sortLarge)
  {
    for (int lod = 0; lod < LARGE_LOD_CNT; ++lod)
      clear_and_resize(v.riexLarge[lod], poolInfo.size());
  }

  int newVisCnt = 0;

#define LAMBDA_BODY(forced_extra_lod_less_then_zero)                                                                      \
  G_UNUSED(ni);                                                                                                           \
  if (render_for_shadow && scene::check_node_flags(m, RendinstTiledScene::CHECKED_IN_SHADOWS) &&                          \
      !scene::check_node_flags(m, RendinstTiledScene::VISIBLE_IN_SHADOWS))                                                \
    return;                                                                                                               \
  if (filter_rendinst_clipmap && !scene::check_node_flags(m, RendinstTiledScene::IS_RENDINST_CLIPMAP))                    \
    return;                                                                                                               \
  if (use_external_filter)                                                                                                \
  {                                                                                                                       \
    vec4f bboxmin, bboxmax;                                                                                               \
    vec4f sphere = scene::get_node_bsphere(m);                                                                            \
    vec4f rad = v_splat_w(sphere);                                                                                        \
    bboxmin = v_sub(sphere, rad);                                                                                         \
    bboxmax = v_add(sphere, rad);                                                                                         \
    if (!external_filter(bboxmin, bboxmax))                                                                               \
      return;                                                                                                             \
  }                                                                                                                       \
  const scene::pool_index poolId = scene::get_node_pool(m);                                                               \
  const auto &riPool = poolInfo[poolId];                                                                                  \
  const unsigned llm = riPool.lodLimits >> ((ri_game_render_mode + 1) * 8);                                               \
  const unsigned min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;                                                         \
  unsigned lod = 0;                                                                                                       \
  float dist = v_extract_x(distSqScaled);                                                                                 \
  if (forced_extra_lod_less_then_zero)                                                                                    \
  {                                                                                                                       \
    lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, v_extract_x(distSqScaled));                         \
    if (lod > max_lod)                                                                                                    \
      return;                                                                                                             \
  }                                                                                                                       \
  else                                                                                                                    \
    lod = forcedExtraLod;                                                                                                 \
  lod = clamp(lod, min_lod, max_lod);                                                                                     \
  vec4f *addData = append_data(v.riexData[lod].data()[poolId], RIEXTRA_VECS_COUNT);                                       \
  v.minSqDistances[lod].data()[poolId] = min(v.minSqDistances[lod].data()[poolId], dist);                                 \
  const int32_t *userData = tiled_scene.getUserData(ni);                                                                  \
  if (userData)                                                                                                           \
    eastl::copy_n(userData, tiled_scene.getUserDataWordCount(), (uint32_t *)(addData + ADDITIONAL_DATA_IDX));             \
  v_mat44_transpose_to_mat43(*(mat43f *)addData, m);                                                                      \
  uint32_t perDataBufferOffset = poolId * (sizeof(rendinstgenrender::RiShaderConstBuffers) / sizeof(vec4f)) + 1;          \
  addData[ADDITIONAL_DATA_IDX] = v_perm_xaxa(addData[ADDITIONAL_DATA_IDX], v_cast_vec4f(v_splatsi(perDataBufferOffset))); \
  newVisCnt++;


#define LAMBDA(forced_extra_lod_less_then_zero) \
  [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) { LAMBDA_BODY(forced_extra_lod_less_then_zero) }

  SmallTab<Point2, framemem_allocator> perPoolMinDist;
  SmallTab<RiGenExtraVisibility::UVec2, framemem_allocator> perPoolBestId;

  if (!render_for_shadow && use_occlusion && check_occluders) // occlusion
  {
    G_ASSERT(v.forcedExtraLod < 0);
    const int forcedExtraLod = -1;
    int eff_num_tp_workers = threadpool::get_num_workers();
    eff_num_tp_workers = (eff_num_tp_workers > 1) ? eff_num_tp_workers : 0; // special case for 1 threadpool worker that can only serve
                                                                            // low prio jobs
    const int max_avail_threads = min(eff_num_tp_workers + (is_main_thread() ? 1 : 0), rendinst::MAX_CULL_JOBS + 1);
#if DAGOR_DBGLEVEL > 0
    if (parallel_for.get() > max_avail_threads)
      parallel_for.set(max_avail_threads);
    const int threads = parallel_for.get();
#else
    const int threads = max_avail_threads > 1 ? max_avail_threads : 0;
#endif

    perPoolMinDist.resize(poolInfo.size() * max(threads, 1));
    memset(perPoolMinDist.data(), 0x7f, data_size(perPoolMinDist)); // ~FLT_MAX
    perPoolBestId.resize(poolInfo.size() * max(threads, 1));
    memset(perPoolBestId.data(), 0xff, data_size(perPoolBestId));

    if (threads)
    {
      dag::ConstSpan<RendinstTiledScene> cscenes = riExTiledScenes.cscenes(firstScene, sceneCount);
      scene::TiledSceneCullContext *sceneContexts =
        (scene::TiledSceneCullContext *)alloca(sizeof(scene::TiledSceneCullContext) * cscenes.size());
      std::atomic<uint32_t> *riexDataCnt =
        (std::atomic<uint32_t> *)alloca(poolInfo.size() * rendinst::RiExtraPool::MAX_LODS * sizeof(std::atomic<uint32_t>));
      std::atomic<uint32_t> *riexLargeCnt =
        sortLarge ? (std::atomic<uint32_t> *)alloca(poolInfo.size() * LARGE_LOD_CNT * sizeof(std::atomic<uint32_t>)) : nullptr;
      for (int tiled_scene_idx = 0; tiled_scene_idx < cscenes.size(); tiled_scene_idx++)
        new (sceneContexts + tiled_scene_idx, _NEW_INPLACE) scene::TiledSceneCullContext;

      memset(riexDataCnt, 0, poolInfo.size() * rendinst::RiExtraPool::MAX_LODS * sizeof(riexDataCnt[0]));
      if (riexLargeCnt)
        memset(riexLargeCnt, 0, poolInfo.size() * LARGE_LOD_CNT * sizeof(riexDataCnt[0]));

      CullJobSharedState cull_sd;
      cull_sd.globtm = globtm;
      cull_sd.vpos_distscale = vpos_distscale;
      cull_sd.use_occlusion = use_occlusion;
      cull_sd.v = &v;
      cull_sd.scenes.set(cscenes.data(), cscenes.size());
      cull_sd.sceneContexts = sceneContexts;
      cull_sd.poolInfo = &poolInfo;
      cull_sd.riexDataCnt = riexDataCnt;
      cull_sd.riexLargeCnt = riexLargeCnt;
      cull_sd.perPoolMinDist = &perPoolMinDist;
      cull_sd.perPoolBestId = &perPoolBestId;

      // we should lock for reading before processing
      for (int tiled_scene_idx = 0; tiled_scene_idx < cscenes.size(); tiled_scene_idx++)
        sceneContexts[tiled_scene_idx].needToUnlock = cscenes[tiled_scene_idx].lockForRead();

      CullJobRing ring;
      ring.start(threads, cull_sd);

      for (int lod = 0; lod < rendinst::RiExtraPool::MAX_LODS; ++lod)
        for (auto &vv : v.riexData[lod])
          vv.resize(vv.capacity());
      if (sortLarge)
        for (int lod = 0; lod < LARGE_LOD_CNT; ++lod)
          for (auto &vv : v.riexLarge[lod])
            vv.resize(vv.capacity());
      for (int tiled_scene_idx = 0; tiled_scene_idx < cscenes.size(); tiled_scene_idx++)
      {
        const auto &tiled_scene = cscenes[tiled_scene_idx];
        if (visibleFlag)
          tiled_scene.frustumCullTilesPass<true, true, true>(globtm, vpos_distscale, visibleFlag, visibleFlag, use_occlusion,
            sceneContexts[tiled_scene_idx]);
        else
          tiled_scene.frustumCullTilesPass<false, true, true>(globtm, vpos_distscale, 0, 0, use_occlusion,
            sceneContexts[tiled_scene_idx]);
      }

      for (int tries = 2; tries > 0; tries--)
      {
        ring.finishWork();

        bool had_overflow = false;
        newVisCnt = 0;
        for (int lod = 0; lod < rendinst::RiExtraPool::MAX_LODS; ++lod)
          for (int poolId = 0; poolId < poolInfo.size(); ++poolId)
          {
            int sz = riexDataCnt[poolId * rendinst::RiExtraPool::MAX_LODS + lod];
            newVisCnt += sz;
            auto &vData = v.riexData[lod].data()[poolId];
            if (sz <= vData.size())
              vData.resize(sz);
            else
            {
              sz = (sz + 127) & ~127;
              vData.resize(0);
              vData.set_capacity(
                min(max(sz, (int)vData.capacity() * 2), (int)RIEXTRA_VECS_COUNT * riExtra[poolId].getEntitiesCount()));
              vData.resize(vData.capacity());
              had_overflow = true;
            }
            if (!riexLargeCnt || lod >= LARGE_LOD_CNT)
              continue;

            sz = riexLargeCnt[poolId * LARGE_LOD_CNT + lod];
            auto &vLarge = v.riexLarge[lod].data()[poolId];
            if (sz <= vLarge.size())
              vLarge.resize(sz);
            else
            {
              sz = (sz + 127) & ~127;
              vLarge.resize(0);
              vLarge.set_capacity(
                min(max(sz, (int)vLarge.capacity() * 2), (int)RIEXTRA_VECS_COUNT * riExtra[poolId].getEntitiesCount()));
              vLarge.resize(vLarge.capacity());
              had_overflow = true;
            }
          }

        if (!had_overflow)
          break;
        G_ASSERT(tries > 1);

        // compute once more
        memset(perPoolMinDist.data(), 0x7f, data_size(perPoolMinDist)); // ~FLT_MAX
        memset(perPoolBestId.data(), 0xff, data_size(perPoolBestId));
        memset(riexDataCnt, 0, poolInfo.size() * rendinst::RiExtraPool::MAX_LODS * sizeof(int)); //-V780
        if (riexLargeCnt)
          memset(riexLargeCnt, 0, poolInfo.size() * LARGE_LOD_CNT * sizeof(int)); //-V780

        for (int tiled_scene_idx = 0; tiled_scene_idx < cscenes.size(); tiled_scene_idx++)
          sceneContexts[tiled_scene_idx].nextIdxToProcess = 0;
        ring.start(threads, cull_sd);
      }

      for (int tiled_scene_idx = 0; tiled_scene_idx < cscenes.size(); tiled_scene_idx++)
      {
        if (sceneContexts[tiled_scene_idx].needToUnlock)
        {
          cscenes[tiled_scene_idx].unlockAfterRead();
          sceneContexts[tiled_scene_idx].needToUnlock = 0;
        }
        sceneContexts[tiled_scene_idx].~TiledSceneCullContext();
      }
      newVisCnt /= RIEXTRA_VECS_COUNT;

      // choose best of the bests
      if (threads > 1) //-V547
        for (int poolId = 0; poolId < poolInfo.size(); poolId++)
        {
          auto *__restrict minDist = perPoolMinDist.data() + poolId;
          auto *__restrict bestId = perPoolBestId.data() + poolId;
          for (int i = 1; i < threads; i++)
          {
            float sdist = (minDist + poolInfo.size() * i)->x;
            if (sdist < minDist->x)
            {
              minDist->y = minDist->x;
              bestId->y = bestId->x;
              minDist->x = sdist;
              bestId->x = (bestId + poolInfo.size() * i)->x;
            }
            else if (sdist < minDist->y)
            {
              minDist->y = sdist;
              bestId->y = (bestId + poolInfo.size() * i)->x;
            }
            else
              continue;

            sdist = (minDist + poolInfo.size() * i)->y;
            if (sdist < minDist->x)
            {
              minDist->y = minDist->x;
              bestId->y = bestId->x;
              minDist->x = sdist;
              bestId->x = (bestId + poolInfo.size() * i)->y;
            }
            else if (sdist < minDist->y)
            {
              minDist->y = sdist;
              bestId->y = (bestId + poolInfo.size() * i)->y;
            }
          }
        }
    }
    else
      for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
      {
        tiled_scene.frustumCull<false, true, true>(globtm, vpos_distscale, 0, 0, use_occlusion,
          [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
            LAMBDA_BODY(forcedExtraLod < 0)
            const uint32_t id = v.riexData[lod].data()[poolId].size() / RIEXTRA_VECS_COUNT - 1;
            vec4f rad = scene::get_node_bsphere_vrad(m);
            rad = v_div_x(distSqScaled, v_mul_x(rad, rad));
            float sdist = v_extract_x(rad);
            if (sortLarge && lod < LARGE_LOD_CNT && (scene::check_node_flags(m, RendinstTiledScene::LARGE_OCCLUDER)))
            {
              // this is almost as fast as using dist2 and is technically more correct.
              // however, since large occluders are usually not scaled, their radius is constant, and
              //  v_dot3_x(sphere, sort_dir) isn't that much different from projected dist
              // vec4f sphere = scene::get_node_bsphere(m);
              // float sdist = v_extract_x(v_sub(v_dot3_x(sphere, sort_dir), v_splat_w(sphere)));
              v.riexLarge[lod].data()[poolId].push_back({bitwise_cast<int, float>(sdist), id});
            }

            if (sdist < perPoolMinDist.data()[poolId].x)
            {
              perPoolMinDist.data()[poolId].y = perPoolMinDist.data()[poolId].x;
              perPoolBestId.data()[poolId].y = perPoolBestId.data()[poolId].x;
              perPoolMinDist.data()[poolId].x = sdist;
              perPoolBestId.data()[poolId].x = (id) | (lod << 28);
            }
            else if (sdist < perPoolMinDist.data()[poolId].y)
            {
              perPoolMinDist.data()[poolId].y = sdist;
              perPoolBestId.data()[poolId].y = (id) | (lod << 28);
            }
          });
        // store
      }
  }
  else if (render_for_shadow && use_occlusion && check_occluders) // shadow occlusion
  {
    G_ASSERT(v.forcedExtraLod < 0); // can't be forced lod in main csm
    const int forcedExtraLod = -1;
    const uint32_t useFlags = RendinstTiledScene::HAVE_SHADOWS | visibleFlag;
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
    {
      // we intentionally do not use use_flags template arg here, as virtually all nodes have shadows
      tiled_scene.frustumCull<false, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL,
        [&](scene::node_index ni, mat44f_cref m, vec4f distSqScaled) {
          uint8_t instLightDist = tiled_scene.getDistanceMT(ni);
          if (instLightDist < tiled_scene.LIGHTDIST_TOOBIG)
          {
            vec4f sphere = scene::get_node_bsphere(m);
            if (instLightDist < tiled_scene.LIGHTDIST_DYNAMIC)
            {
              vec4f rad = v_splat_w(sphere);
              vec3f top_point = v_add(sphere, v_and(v_cast_vec4f(V_CI_MASK0100), rad));
              if (instLightDist == tiled_scene.LIGHTDIST_INVALID)
              {
                instLightDist = tiled_scene.LIGHTDIST_TOOBIG;
                Point3_vec4 topPos;
                v_st(&topPos.x, top_point);
                float dist = 128.f;
                if (rendinstgen::custom_trace_ray_earth(topPos, rendinstgenrender::dir_from_sun, dist)) // fixme: currently
                                                                                                        // dir_from_sun is not set
                                                                                                        // until first update impostors
                  instLightDist = int(ceilf(dist)) + 1;
                tiled_scene.setDistanceMT(ni, instLightDist);
              }

              vec3f lightDist =
                v_mul(v_cvt_vec4f(v_splatsi(instLightDist)), reinterpret_cast<vec4f &>(rendinstgenrender::dir_from_sun));
              vec3f far_point = v_add(top_point, lightDist);
              bbox3f worldBox;
              worldBox.bmin = v_min(far_point, v_sub(sphere, rad));
              worldBox.bmax = v_max(far_point, v_add(sphere, rad));
              if (!use_occlusion->isVisibleBox(worldBox.bmin, worldBox.bmax)) // may be we should also use isOccludedBox here?
                return;
            }
            else // dynamic object
            {
              // replace with bounding sphere
              if (use_occlusion->isOccludedSphere(sphere, v_splat_w(v_add(sphere, sphere)))) //
                return;
            }
          }
          if (!scene::check_node_flags(m, RendinstTiledScene::HAVE_SHADOWS)) // we still have to check flag, but we assume it will
                                                                             // happen very rare that it fails, so check it last
            return;
          LAMBDA_BODY(forcedExtraLod < 0);
        });
    }
  }
  else if (cullIntention != RiExtraCullIntention::MAIN)
  {
    uint32_t useFlags = visibleFlag;
    bool depthOrReflectino = cullIntention == RiExtraCullIntention::DRAFT_DEPTH || cullIntention == RiExtraCullIntention::REFLECTIONS;
    G_ASSERT(v.forcedExtraLod < 0 || !depthOrReflectino);
    const int forcedExtraLod = depthOrReflectino ? -1 : v.forcedExtraLod; // can't be forced lod for depth/reflections
    if (cullIntention == RiExtraCullIntention::DRAFT_DEPTH)
    {
      useFlags |= RendinstTiledScene::DRAFT_DEPTH;
    }
    else if (cullIntention == RiExtraCullIntention::REFLECTIONS)
    {
      useFlags |= RendinstTiledScene::REFLECTION;
    }
    else if (cullIntention == RiExtraCullIntention::LANDMASK)
    {
      useFlags |= RendinstTiledScene::VISIBLE_IN_LANDMASK;
    }
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
    {
      tiled_scene.frustumCull<true, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL, LAMBDA(forcedExtraLod < 0));
    }
  }
  else if (for_visual_collision) // phydetails
  {
    const int forcedExtraLod = v.forcedExtraLod;
    const uint32_t useFlags = RendinstTiledScene::VISUAL_COLLISION | visibleFlag;
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
    {
      tiled_scene.frustumCull<true, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL, LAMBDA(forcedExtraLod < 0));
    }
  }
  else if (for_vsm) // phydetails
  {
    const int forcedExtraLod = v.forcedExtraLod;
    const uint32_t useFlags = RendinstTiledScene::VISIBLE_IN_VSM | visibleFlag;
    for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
    {
      tiled_scene.frustumCull<true, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL, LAMBDA(forcedExtraLod < 0));
    }
  }
  else
  {
    const int forcedExtraLod = v.forcedExtraLod;
    const uint32_t useFlags = visibleFlag;
    //
    if (useFlags == 0)
    {
      if (forcedExtraLod >= 0) // we just hope that compiler will optimize code inside lambda with it. Although it is possible that it
                               // won't, than can copy-paste lambda code
        for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
          tiled_scene.frustumCull<false, true, false>(globtm, vpos_distscale, 0, 0, NULL, LAMBDA(false));
      else
        for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
          tiled_scene.frustumCull<false, true, false>(globtm, vpos_distscale, 0, 0, NULL, LAMBDA(true));
    }
    else
    {
      if (forcedExtraLod >= 0) // we just hope that compiler will optimize code inside lambda with it. Although it is possible that it
                               // won't, than can copy-paste lambda code
        for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
          tiled_scene.frustumCull<true, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL, LAMBDA(false));
      else
        for (const auto &tiled_scene : riExTiledScenes.cscenes(firstScene, sceneCount))
          tiled_scene.frustumCull<true, true, false>(globtm, vpos_distscale, useFlags, useFlags, NULL, LAMBDA(true));
    }
  }
  v.riexInstCount = newVisCnt;
  // todo: if not rendering to main, use predefined pool order based on pool bbox size (for shadows and such), from big to small
  // todo: replace reflection hardcodes & guesses (minimum_size>0, and setting flag 1 if pool size > 25) with explicit logic
  // todo: auto detect params for rendinst scenes count and params based on profile guided distances
  if (!v.riexInstCount)
    return true;

  {
    TIME_PROFILE(sortPool);
    if (sortLarge)
    {
      uint16_t minPool, maxPool;
      for (minPool = 0; minPool < poolInfo.size(); ++minPool)
        if (perPoolMinDist[minPool].x < bitwise_cast<float, int>(0x7f7f7f7f))
          break;
      for (maxPool = poolInfo.size() - 1; maxPool > minPool; --maxPool)
        if (perPoolMinDist[maxPool].x < bitwise_cast<float, int>(0x7f7f7f7f))
          break;

      v.riexPoolOrder.reserve(maxPool - minPool + 1);
      // cost of sort is about 0.02msec. however it speeds up rendering sometimes by 10% of GPU time
      // it can be used in shadows as well, but based on sun dir distance
      {
        SmallTab<RiGenExtraVisibility::Order, framemem_allocator> distAndPool;
        distAndPool.reserve(maxPool - minPool + 1);
        for (unsigned i = minPool; i <= maxPool; ++i)
          if (perPoolMinDist[i].x < bitwise_cast<float, int>(0x7f7f7f7f))
            distAndPool.push_back({bitwise_cast<int, float>(perPoolMinDist[i].x), i});
        stlsort::sort_branchless(distAndPool.begin(), distAndPool.end());
        v.riexPoolOrder.resize(distAndPool.size());
        for (int i = 0; i < distAndPool.size(); ++i)
          v.riexPoolOrder[i] = distAndPool[i].id;
      }
    }
    else
    {
      sortByPoolSizeOrder(v, rendinst::RiExtraPool::MAX_LODS - 1);
    }
  }

  if (sortLarge)
  {
    TIME_PROFILE(sortLarge);
    const float max_dist_to_sort = 500.f * 500.f * distSqMul;
    static int min_optimization_dist2i = bitwise_cast<int, float>(MIN_OPTIMIZATION_DIST * MIN_OPTIMIZATION_DIST * distSqMul);
    for (int lod = 0; lod < LARGE_LOD_CNT; ++lod)
      for (auto &poolAndCnt : v.riexPoolOrder)
      {
        auto poolId = poolAndCnt & RI_RES_ORDER_COUNT_MASK;
        auto &data = v.riexData[lod][poolId];
        if (!data.size())
          continue;
        auto &ind = v.riexLarge[lod][poolId];
        if (ind.size() && (perPoolMinDist[poolId].x < max_dist_to_sort || ind.size() < 8))
        {
          stlsort::sort_branchless(ind.begin(), ind.end());
          int id = 0;
          clear_and_resize(v.largeTempData, ind.size() * RIEXTRA_VECS_COUNT);
          for (auto i : ind)
          {
            memcpy(v.largeTempData.data() + (id++) * RIEXTRA_VECS_COUNT, data.data() + i.id * RIEXTRA_VECS_COUNT,
              RIEXTRA_VECS_COUNT * sizeof(vec4f));
          }
          memcpy(data.data(), v.largeTempData.data(), ind.size() * RIEXTRA_VECS_COUNT * sizeof(vec4f));
          if (lod == 0)
          {
            uint32_t instances = 0;
            for (int j = 0, je = min<int>(ind.size(), MAX_OPTIMIZATION_INSTANCES); j < je; ++j)
              if (ind[j].d < min_optimization_dist2i)
                instances++;
            G_ASSERT(instances <= MAX_OPTIMIZATION_INSTANCES);
            poolAndCnt |= instances << RI_RES_ORDER_COUNT_SHIFT;
          }
          G_STATIC_ASSERT(MAX_OPTIMIZATION_INSTANCES <= (1 << 2) - 1); // cause we just allocated 2 bits
        }
        else if (data.size() > RIEXTRA_VECS_COUNT)
        {
          auto best = perPoolBestId[poolId];
          if ((best.x >> 28) == lod)
          {
            swap_data(data, best.x & ((1 << 28) - 1), 0, RIEXTRA_VECS_COUNT);
            // eastl::swap(matrices[best.x&((1<<28)-1)], matrices[0]);
            if ((best.y >> 28) == lod)
              swap_data(data, 0 == (best.y & ((1 << 28) - 1)) ? (best.x & ((1 << 28) - 1)) : (best.y & ((1 << 28) - 1)), 1,
                RIEXTRA_VECS_COUNT);
            // eastl::swap(matrices[ 0 == (best.y&((1<<28)-1)) ? (best.x&((1<<28)-1)) : (best.y&((1<<28)-1))], matrices[1]);
          }
          else if ((best.y >> 28) == lod)
            swap_data(data, best.y & ((1 << 28) - 1), 0, RIEXTRA_VECS_COUNT);
        }
        ind.resize(0);
      }
    // sort matrices
  }

  return true;
}

bool rendinst::prepareRIGenExtraVisibility(mat44f_cref globtm_cull, const Point3 &camera_pos, RiGenVisibility &vbase,
  bool render_for_shadow, Occlusion *use_occlusion, RiExtraCullIntention cullIntention, bool for_visual_collision,
  bool filter_rendinst_clipmap, bool for_vsm, const rendinst::VisibilityExternalFilter &external_filter)
{
  if (!external_filter)
    return prepareExtraVisibilityInternal(globtm_cull, camera_pos, vbase, render_for_shadow, use_occlusion, cullIntention,
      for_visual_collision, filter_rendinst_clipmap, for_vsm);
  else
    return prepareExtraVisibilityInternal<true>(globtm_cull, camera_pos, vbase, render_for_shadow, use_occlusion, cullIntention,
      for_visual_collision, filter_rendinst_clipmap, for_vsm, external_filter);
}

bool rendinst::prepareRIGenExtraVisibilityBox(bbox3f_cref box_cull, int forced_lod, float min_size, float min_dist,
  RiGenVisibility &vbase, bbox3f *result_box)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount || RendInstGenData::isLoading)
    return false;
  TIME_PROFILE(riextra_visibility_box);
  const VisibilityRenderingFlags rendering = vbase.rendering;
  RiGenExtraVisibility &v = vbase.riex;
  v.vbExtraGeneration = INVALID_VB_EXTRA_GEN;

  const auto &poolInfo = riExTiledScenes.getPools();

  for (int lod = 0; lod < rendinst::RiExtraPool::MAX_LODS; ++lod)
  {
    clear_and_resize(v.riexData[lod], poolInfo.size());
    clear_and_resize(v.minSqDistances[lod], poolInfo.size());
    memset(v.minSqDistances[lod].data(), 0x7f, data_size(v.minSqDistances[lod])); // ~FLT_MAX
    for (auto &vv : v.riexData[lod])
      vv.resize(0);
  }
  forced_lod = clamp(forced_lod, 0, rendinst::RiExtraPool::MAX_LODS - 1);

  v.riexPoolOrder.resize(0);
  if (!riExTiledScenes.size())
  {
    v.riexInstCount = 0;
    return false;
  }
  uint32_t additionalData = riExTiledScenes[0].getUserDataWordCount(); // in dwords
  for (const auto &tiled_scene : riExTiledScenes.scenes())
  {
    G_ASSERTF(!additionalData || !tiled_scene.getUserDataWordCount() || additionalData == tiled_scene.getUserDataWordCount(),
      " %d == %d", additionalData, tiled_scene.getUserDataWordCount());
    if (!additionalData)
      additionalData = tiled_scene.getUserDataWordCount();
  }

  // can be made invisibleFlag, if testFlags = RendinstTiledScene::VISIBLE_0, equalFlags = ~RendinstTiledScene::VISIBLE_0;
  // uint32_t visibleFlag = ri_game_render_mode == 0 ? RendinstTiledScene::VISIBLE_0 : 0;//todo:? support flags?
  const auto [firstScene, lastScene] = scene_range_from_visiblity_rendering_flags(rendering);

  int newVisCnt = 0;
  vec4f min_size_v = v_splats(min_size);
  int maxLodUsed = forced_lod;
  if (result_box)
    v_bbox3_init_empty(*result_box);

  for (int scnI = firstScene; scnI < lastScene; ++scnI)
  {
    if (riExTiledSceneMaxDist[scnI] <= min_dist) // skip it anyway, all it's data will be of smaller size
      continue;
    float min_dist_sq = min_dist * min_dist;
    const auto &tiled_scene = riExTiledScenes[scnI];
    tiled_scene.boxCull<false, true>(box_cull, 0, 0, [&](scene::node_index ni, mat44f_cref m) {
      if (v_test_vec_x_lt(scene::get_node_bsphere_vrad(m), min_size_v))
        return;
      const scene::pool_index poolId = scene::get_node_pool(m);
      const auto &riPool = poolInfo[poolId];
      if (riPool.distSqLOD[rendinst::RiExtraPool::MAX_LODS - 1] < min_dist_sq)
        return;
      const unsigned llm = riPool.lodLimits >> ((ri_game_render_mode + 1) * 8);
      const unsigned min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
      int lod = clamp((unsigned)forced_lod, min_lod, max_lod);
      maxLodUsed = max(lod, maxLodUsed);
      vec4f *addData = append_data(v.riexData[lod].data()[poolId], RIEXTRA_VECS_COUNT);
      const int32_t *userData = tiled_scene.getUserData(ni);
      if (userData)
        eastl::copy_n(userData, tiled_scene.getUserDataWordCount(), (uint32_t *)(addData + ADDITIONAL_DATA_IDX));
      v_mat44_transpose_to_mat43(*(mat43f *)addData, m);
      if (result_box)
        v_bbox3_add_box(*result_box, tiled_scene.calcNodeBox(m));
      uint32_t perDataBufferOffset = poolId * (sizeof(rendinstgenrender::RiShaderConstBuffers) / sizeof(vec4f)) + 1;
      addData[ADDITIONAL_DATA_IDX] = v_perm_xaxa(addData[ADDITIONAL_DATA_IDX], v_cast_vec4f(v_splatsi(perDataBufferOffset)));
      newVisCnt++;
    });
  }
  v.riexInstCount = newVisCnt;
  // todo: if not rendering to main, use predefined pool order based on pool bbox size (for shadows and such), from big to small
  // todo: replace reflection hardcodes & guesses (minimum_size>0, and setting flag 1 if pool size > 25) with explicit logic
  // todo: auto detect params for rendinst scenes count and params based on profile guided distances
  if (!v.riexInstCount)
    return true;

  sortByPoolSizeOrder(v, maxLodUsed);

  return true;
}

static bool lock_vbextra(RiGenExtraVisibility &v, vec4f *&vbPtr, rendinst::VbExtraCtx &vbctx)
{
  using namespace rendinst;

  const int cnt = v.riexInstCount;
  if (vbctx.gen == v.vbExtraGeneration || !cnt) // buffer already filled or no instances to render
    return cnt != 0;

  TIME_PROFILE_DEV(lock_vbextra);

  bool canIncreaseBuffer = !(instancePositionsBufferType == 1 && d3d::get_driver_desc().issues.hasSmallSampledBuffers); // Maximum
                                                                                                                        // TBuffer
                                                                                                                        // size is
                                                                                                                        // 64K on
                                                                                                                        // old
                                                                                                                        // Android
                                                                                                                        // devices.
  int vi = &vbctx - &vbExtraCtx[0];
  maxRenderedRiEx[vi] = max(maxRenderedRiEx[vi], cnt);
  if (cnt > maxExtraRiCount)
  {
    logwarn("vbExtra[%d] recreated to requirement of render more = than %d (%d*2)", vi, maxExtraRiCount, cnt);
    if (!canIncreaseBuffer)
    {
      logerr("Too many RIEx to render, and buffer resize is not possible");
      return false;
    }
    maxExtraRiCount = cnt * 2;
    del_it(vbctx.vb);
    rendinst::allocateRIGenExtra(vbctx);
    debug_dump_stack();
  }
  uint32_t dataSize = cnt * RIEXTRA_VECS_COUNT;
  if (vbctx.vb->getPos() + dataSize > vbctx.vb->bufSize())
  {
    if (vbctx.lastSwitchFrameNo && vbctx.lastSwitchFrameNo >= dagor_frame_no() - 1)
    {
      logwarn("vbExtra[%d] switched more than once during frame #%d, vbExtraLastSwitchFrameNo = %d (%d -> %d)", vi, dagor_frame_no(),
        vbctx.lastSwitchFrameNo, maxExtraRiCount, 2 * maxExtraRiCount);
      if (canIncreaseBuffer)
        maxExtraRiCount *= 2;
      // probably intented to avoid accessing GPU owned data, so recreate it regardless of size growth
      del_it(vbctx.vb);
      rendinst::allocateRIGenExtra(vbctx);
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
  rendinst::VbExtraCtx &vbctx)
{
  using namespace rendinst;

  if (vbctx.gen == v.vbExtraGeneration) // buffer already filled
    return true;

  TIME_D3D_PROFILE(ri_extra_fill_buffer);

  const int cnt = v.riexInstCount;
  if (!vbPtr && !lock_vbextra(v, vbPtr, vbctx))
    return false;

  int start_ofs = vbctx.vb->getPos(), cur_ofs = start_ofs;
  v.riExLodNotEmpty = 0;
  for (int l = 0; l < RiExtraPool::MAX_LODS; l++)
  {
    int lod_ofs = cur_ofs;
    auto &riExDataLod = v.riexData[l];
    v.vbOffsets[l].assign(riExTiledScenes.getPools().size(), IPoint2(0, 0));
    for (auto poolAndCnt : v.riexPoolOrder)
    {
      auto poolI = poolAndCnt & RI_RES_ORDER_COUNT_MASK;
      if (uint32_t poolCnt = (uint32_t)v.riexData[l][poolI].size() / RIEXTRA_VECS_COUNT)
      {
        v.vbOffsets[l][poolI] = IPoint2(cur_ofs, poolCnt);
        cur_ofs += poolCnt * RIEXTRA_VECS_COUNT;
        const vec4f *data = riExDataLod[poolI].data();
        for (int m = 0; m < poolCnt; ++m, vbPtr += RIEXTRA_VECS_COUNT, data += RIEXTRA_VECS_COUNT)
          memcpy(vbPtr, data, RIEXTRA_VECS_COUNT * sizeof(vec4f));
      }
    }
    if (lod_ofs != cur_ofs)
      v.riExLodNotEmpty |= 1 << l;
  }
  vbctx.vb->unlockData(cnt * RIEXTRA_VECS_COUNT);
  G_ASSERT(cur_ofs - start_ofs == v.riexInstCount * RIEXTRA_VECS_COUNT);
  G_UNUSED(start_ofs);
  v.vbexctx = &vbctx;
  G_FAST_ASSERT(vbctx.gen != INVALID_VB_EXTRA_GEN);
  interlocked_release_store(v.vbExtraGeneration, vbctx.gen); // mark buffer as filled
  return true;
}

static struct RenderRiExtraJob final : public cpujobs::IJob
{
  rendinst::RiExtraRenderer riExRenderer;
  RiGenVisibility *vbase = nullptr;
  GlobalVariableStates gvars;
  TexStreamingContext texContext = TexStreamingContext(0);

  void prepare(RiGenVisibility &v, int frame_stblk, bool enable, TexStreamingContext texCtx)
  {
    TIME_PROFILE(prepareToStartAsyncRIGenExtraOpaqueRender);
    G_ASSERT(is_main_thread());
    threadpool::wait(this); // It should not be running at this point

    if (EASTL_UNLIKELY(!enable))
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

      rendinstgenrender::setCoordType(rendinstgenrender::COORD_TYPE_TM);
      ShaderBlockSetter ssb(frame_stblk, ShaderGlobal::LAYER_FRAME);
      SCENE_LAYER_GUARD(rendinstgenrender::rendinstSceneBlockId);
      ShaderGlobal::set_int(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(rendinst::RenderPass::Normal));

      G_ASSERT(gvars.empty());
      gvars.set_allocator(framemem_ptr());
      copy_current_global_variables_states(gvars);
      riExRenderer.globalVarsState = &gvars;

      ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_MIXED);
    }

    rendinst::rebuildAllElems();
    rendinst::allocateRIGenExtra(rendinst::vbExtraCtx[1]); // To consider: create with SBCF_FRAMEMEM (can't be more 256K at time of
                                                           // writing)
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
      if (EASTL_UNLIKELY(!lock_vbextra(vbase->riex, vbPtr, rendinst::vbExtraCtx[1]))) // lock failed (or no instances to render)
      {
        vbase->riex.riexInstCount = 0;
        G_FAST_ASSERT(rendinst::vbExtraCtx[1].gen != INVALID_VB_EXTRA_GEN);
        interlocked_release_store(vbase->riex.vbExtraGeneration, rendinst::vbExtraCtx[1].gen); // mark buffer as filled
        interlocked_release_store_ptr(vbase, (RiGenVisibility *)nullptr);
        return;
      }
      G_VERIFY(fill_vbextra(vbase->riex, vbPtr, rendinst::vbExtraCtx[1]));
    }
    dag::ConstSpan<uint16_t> riResOrder = vbase->riex.riexPoolOrder;
    riExRenderer.init(riResOrder.size(), rendinst::LAYER_OPAQUE, rendinst::RenderPass::Normal);
    riExRenderer.addObjectsToRender(vbase->riex, riResOrder, texContext);
    riExRenderer.sortMeshesByMaterial();
  }

  rendinst::RiExtraRenderer *wait(const RiGenVisibility *v)
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
void rendinst::prepareToStartAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, int frame_stblk, TexStreamingContext texCtx,
  bool enable)
{
  return render_ri_extra_job.prepare(vis, frame_stblk, enable, texCtx);
}
void rendinst::startPreparedAsyncRIGenExtraOpaqueRender(RiGenVisibility &vis, bool wake)
{
  return render_ri_extra_job.start(vis, wake);
}
void rendinst::waitAsyncRIGenExtraOpaqueRenderVbFill(const RiGenVisibility *vis)
{
  if (interlocked_acquire_load_ptr(render_ri_extra_job.vbase) == vis)
    spin_wait([&] { return interlocked_acquire_load(vis->riex.vbExtraGeneration) == INVALID_VB_EXTRA_GEN; });
}
rendinst::RiExtraRenderer *rendinst::waitAsyncRIGenExtraOpaqueRender(const RiGenVisibility *vis)
{
  return render_ri_extra_job.wait(vis);
}

bool rendinst::hasRIExtraOnLayers(const RiGenVisibility *visibility, uint32_t layer_flags)
{
  if (!visibility->riex.riexInstCount)
    return false;

  for (int layer = 0; layer < RIEX_STAGE_COUNT; ++layer)
  {
    if ((layer_flags & (1 << layer)) == 0)
      continue;

    if (riExPoolIdxPerStage[layer].size() > 0)
      return true;
  }

  return false;
}

using RiExtraRendererWithDynVarsCache = rendinst::RiExtraRendererT<framemem_allocator, rendinst::CachedDynVarsPolicy>;

void rendinst::renderRIGenExtra(const RiGenVisibility &vbase, RenderPass render_pass, OptimizeDepthPass optimization_depth_pass,
  OptimizeDepthPrepass optimization_depth_prepass, IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t layer,
  uint32_t instance_multiply, TexStreamingContext texCtx, AtestStage atest_stage, const RiExtraRenderer *riex_renderer)
{
  TIME_D3D_PROFILE(render_ri_extra);

  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  const RiGenExtraVisibility &v = vbase.riex;
  if (!v.riexInstCount)
    return;

  VbExtraCtx &vbexctx = v.vbexctx ? *v.vbexctx : vbExtraCtx[0];
  if (!fill_vbextra(const_cast<RiGenExtraVisibility &>(v), nullptr, vbexctx))
    return;

  rendinst::rebuildAllElems();

  ccExtra.lockRead(); //?!
  RingDynamicSB *vb = vbexctx.vb;
  rendinstgenrender::startRenderInstancing();
  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);
  rendinstgenrender::setCoordType(rendinstgenrender::COORD_TYPE_TM);

  const int blockToSet =
    (layer == LAYER_TRANSPARENT) ? rendinstgenrender::rendinstSceneTransBlockId : rendinstgenrender::rendinstSceneBlockId;
  const bool needToSetBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE) == -1;
  if (needToSetBlock)
    ShaderGlobal::setBlock(blockToSet, ShaderGlobal::LAYER_SCENE);

  d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, vb->getRenderBuf());

  dag::ConstSpan<uint16_t> riResOrder = v.riexPoolOrder;
  if (layer == LAYER_TRANSPARENT || layer == LAYER_DECALS || layer == LAYER_DISTORTION)
    riResOrder = riExPoolIdxPerStage[get_const_log2(layer)];

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

    if (layer & LAYER_OPAQUE && atest_stage == AtestStage::NoAtest)
      riexr.setNewStages(ShaderMesh::STG_opaque, ShaderMesh::STG_opaque);
    else if (layer & LAYER_OPAQUE && atest_stage == AtestStage::Atest)
      riexr.setNewStages(ShaderMesh::STG_atest, ShaderMesh::STG_atest);
    else if (layer & LAYER_OPAQUE && atest_stage == AtestStage::AtestAndImmDecal)
      riexr.setNewStages(ShaderMesh::STG_atest, ShaderMesh::STG_imm_decal);

    riexr.addObjectsToRender(v, riResOrder, texCtx, optimization_depth_prepass, ignore_optimization_instances_limits);
    riexr.sortMeshesByMaterial();

    doRender(riexr);
  }

  if (needToSetBlock)
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  rendinstgenrender::endRenderInstancing();

  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_MIXED);
  ccExtra.unlockRead();
}

void rendinst::ensureElemsRebuiltRIGenExtra(bool gpu_instancing)
{
  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  rendinst::rebuildAllElems();

  if (gpu_instancing && !rendinst::relemsForGpuObjectsHasRebuilded)
  {
    gpu_objects_mgr->rebuildGpuInstancingRelemParams();
    rendinst::relemsForGpuObjectsHasRebuilded = true;
  }
}

void rendinst::renderRIGenExtraFromBuffer(Sbuffer *buffer, dag::ConstSpan<IPoint2> offsets_and_count,
  dag::ConstSpan<uint16_t> ri_indices, dag::ConstSpan<uint32_t> lod_offsets, RenderPass render_pass,
  OptimizeDepthPass optimization_depth_pass, OptimizeDepthPrepass optimization_depth_prepass,
  IgnoreOptimizationLimits ignore_optimization_instances_limits, uint32_t layer, ShaderElement *shader_override,
  uint32_t instance_multiply, bool gpu_instancing, Sbuffer *indirect_buffer, Sbuffer *ofs_buffer)
{
  TIME_D3D_PROFILE(render_ri_extra_from_buffer);

  if (!RendInstGenData::renderResRequired || !maxExtraRiCount)
    return;

  if (offsets_and_count.empty())
    return;
  G_ASSERT(lod_offsets.size() <= RiExtraPool::MAX_LODS);

  // rebuild will happen in the next before draw
  if (gpu_instancing && !rendinst::relemsForGpuObjectsHasRebuilded)
    return;

  rendinstgenrender::startRenderInstancing();
  ShaderGlobal::set_real_fast(globalTranspVarId, 1.f); // Force not-transparent branch.
  ShaderGlobal::set_int(globalRendinstRenderTypeVarId, RENDINST_RENDER_TYPE_RIEX_ONLY);
  ShaderGlobal::set_real_fast(useRiGpuInstancingVarId, gpu_instancing ? 1.0f : 0.0f);
  rendinstgenrender::setCoordType(rendinstgenrender::COORD_TYPE_TM);

  const int blockToSet =
    (layer == LAYER_TRANSPARENT) ? rendinstgenrender::rendinstSceneTransBlockId : rendinstgenrender::rendinstSceneBlockId;
  const bool needToSetBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE) == -1;
  if (needToSetBlock)
    ShaderGlobal::setBlock(blockToSet, ShaderGlobal::LAYER_SCENE);

  ShaderGlobal::set_int(rendinstgenrender::rendinstRenderPassVarId, eastl::to_underlying(render_pass));

  d3d::set_buffer(STAGE_VS, rendinstgenrender::INSTANCING_TEXREG, buffer);
  if (gpu_instancing)
    d3d::set_buffer(STAGE_VS, rendinstgenrender::GPU_INSTANCING_OFSBUFFER_TEXREG, ofs_buffer);

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
        TexStreamingContext(FLT_MAX), 0.0f, shader_override, gpu_instancing);
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


  rendinstgenrender::endRenderInstancing();
}

void rendinst::setRIGenExtraDiffuseTexture(uint16_t ri_idx, int tex_var_id)
{
  rendinst::RiExtraPool &riPool = rendinst::riExtra[ri_idx];
  RenderableInstanceResource *rendInstRes = riPool.res->lods[0].scene;
  ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
  dag::Span<ShaderMesh::RElem> elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_decal);
  ShaderGlobal::set_texture(tex_var_id, elems[0].mat->get_texture(0));
}

void rendinst::gatherRIGenExtraShadowInvisibleBboxes(eastl::vector<bbox3f> &out_bboxes, bbox3f_cref gather_box)
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
      auto poolI = poolAndCnt & RI_RES_ORDER_COUNT_MASK;
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

void rendinst::collectPixelsHistogramRIGenExtra(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale,
  eastl::vector<eastl::vector_map<eastl::string_view, int>> &histogram)
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

void rendinst::validateFarLodsWithHeavyShaders(const RiGenVisibility *main_visibility, vec4f camera_fov, float histogram_scale)
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
      for (uint32_t EI = rendinst::allElemsIndex[startEIOfs], endEI = rendinst::allElemsIndex[startEIOfs + 1]; EI < endEI; ++EI)
      {
        auto &elem = rendinst::allElems[EI];
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
