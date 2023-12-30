#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraAccess.h>
#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "riGen/riUtil.h"
#include "riGen/riGenExtraMaxHeight.h"
#include "riGen/riGrid.h"
#include "riGen/riGridDebug.h"
#include "render/extraRender.h"
#include "render/gpuObjects.h"
#include "visibility/genVisibility.h"
#include "scopedRiExtraWriteTryLock.h"

#include <ADT/linearGrid.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_drv3dReset.h>
#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_console.h>
#include <util/dag_hash.h>
#include <math/dag_mathUtils.h>


#if DAGOR_DBGLEVEL > 0
static const int LOGMESSAGE_LEVEL = LOGLEVEL_ERR;
#else
static const int LOGMESSAGE_LEVEL = LOGLEVEL_WARN;
#endif

static const DataBlock *riConfig = nullptr;
rendinst::RiExtraPoolsVec rendinst::riExtra;
uint32_t rendinst::RiExtraPoolsVec::size_interlocked() const { return interlocked_acquire_load(*(const volatile int *)&mCount); }
uint32_t rendinst::RiExtraPoolsVec::interlocked_increment_size() { return interlocked_increment(*(volatile int *)&mCount); }
static rendinst::HasRIClipmap hasRiClipmap = rendinst::HasRIClipmap::UNKNOWN;
FastNameMap rendinst::riExtraMap;

void rendinst::iterateRIExtraMap(eastl::fixed_function<sizeof(void *) * 3, void(int, const char *)> cb)
{
  iterate_names(rendinst::riExtraMap, cb);
}

void (*rendinst::shadow_invalidate_cb)(const BBox3 &box) = nullptr;
BBox3 (*rendinst::get_shadows_bbox_cb)() = nullptr;

SmartReadWriteFifoLock rendinst::ccExtra;

int rendinst::maxExtraRiCount = 0;
int rendinst::maxRenderedRiEx[countof(rendinst::render::vbExtraCtx)];
int rendinst::perDrawInstanceDataBufferType = 0; // Shaders support only: 1 - Buffer, 2 - StructuredBuffer; otherwise support is either
                                                 // flexible or undefined
int rendinst::instancePositionsBufferType = 0;
unsigned rendinst::RiExtraPool::defLodLimits = 0xF0F0F0F0;
static const float riex_session_time_0 = 0;
const float *rendinst::RiExtraPool::session_time = &riex_session_time_0;
Tab<uint16_t> rendinst::riExPoolIdxPerStage[RIEX_STAGE_COUNT];
static Tab<rendinst::invalidate_handle_cb> invalidate_handle_callbacks;
static Tab<rendinst::ri_destruction_cb> riex_destruction_callbacks;

using rendinst::regCollCb;
using rendinst::riex_handle_t;
using rendinst::unregCollCb;

static volatile int maxRiExtraHeight = 0;

static void update_max_ri_extra_height(int value)
{
  int old = interlocked_relaxed_load(maxRiExtraHeight);
  if (value > old)
    interlocked_relaxed_store(maxRiExtraHeight, value);
}

static IPoint2 to_ipoint2(Point2 p) { return IPoint2(p.x, p.y); }

static RiGrid riExtraGrid;

static void init_ri_extra_grid(const DataBlock *level_blk)
{
  if (const DataBlock *riExGrid = ::dgs_get_game_params()->getBlockByName("riExtraGrid"))
  {
    riExtraGrid.setCellSizeWithoutObjectsReposition(riExGrid->getInt("cellSize", LINEAR_GRID_DEFAULT_CELL_SIZE));
    riExtraGrid.configMaxMainExtension = riExGrid->getReal("maxMainExtension", riExtraGrid.configMaxMainExtension);
    riExtraGrid.configHalfMaxMainExtension = riExtraGrid.configMaxMainExtension / 2.f;
    riExtraGrid.configMaxSubExtension = riExGrid->getReal("maxSubExtension", riExtraGrid.configMaxSubExtension);
    riExtraGrid.configMaxSubRadius = riExGrid->getReal("maxSubRadius", riExtraGrid.configMaxSubRadius);
    riExtraGrid.configObjectsToCreateSubGrid = riExGrid->getInt("objectsToCreateSubGrid", riExtraGrid.configObjectsToCreateSubGrid);
    riExtraGrid.configMaxLeafObjects = riExGrid->getInt("maxLeafObjects", riExtraGrid.configMaxLeafObjects);
    riExtraGrid.configReserveObjectsOnGrow = riExGrid->getInt("reserveObjectsOnGrow", riExtraGrid.configReserveObjectsOnGrow);
  }

  rendinst::init_tiled_scenes(level_blk);
}

static void term_ri_extra_grids()
{
  rendinst::term_tiled_scenes();
  riExtraGrid.clear();
}

static vec4f make_pos_and_rad(mat44f_cref tm, vec4f center_and_rad)
{
  vec4f pos = v_mat44_mul_vec3p(tm, center_and_rad);
  vec4f maxScale = v_max(v_length3_est(tm.col0), v_max(v_length3_est(tm.col1), v_length3_est(tm.col2)));
  vec4f bb_ext = v_mul(v_splat_w(center_and_rad), maxScale);
  return v_perm_xyzd(pos, bb_ext);
}

void rendinst::initRIGenExtra(bool need_render, const DataBlock *level_blk)
{
  init_ri_extra_grid(level_blk);
  if (!need_render)
    return;
  render::allocateRIGenExtra(rendinst::render::vbExtraCtx[0]);
}

void rendinst::optimizeRIGenExtra()
{
  ScopedLockWrite lock(ccExtra);
  debug(riExtraGrid.isOptimized() ? "Re-optimizing RiGrid on update" : "Optimizing RiGrid");
  riExtraGrid.optimizeCells();
}

namespace rendinst
{
static void update_ri_extra_game_resources(const char *ri_res_name, int id, RenderableInstanceLodsResource *riRes,
  const DataBlock &params_block);
} // namespace rendinst

void rendinst::termRIGenExtra()
{
  for (auto rg : rgLayer) // clear delayed RI as it hold references to riExtra
    if (rg)
      rg->clearDelayedRi();
    else
      break;
  interlocked_relaxed_store(maxRiExtraHeight, 0);
  ri_extra_max_height_on_terminate();
  term_ri_extra_grids();
  for (int i = 0; i < countof(riExPoolIdxPerStage); i++)
    riExPoolIdxPerStage[i].clear();
  clear_and_shrink(riExtra);
  riExtraMap.reset(true);
  hasRiClipmap = rendinst::HasRIClipmap::UNKNOWN;
  render::termElems();
  for (auto &ve : rendinst::render::vbExtraCtx)
  {
    del_it(ve.vb);
    ve.gen = INVALID_VB_EXTRA_GEN + 1;
  }
  rendinst::gpuobjects::clear_all();
}

void rendinst::RiExtraPool::setWasNotSavedToElems()
{
  wasSavedToElems = 0;
  interlocked_increment(rendinst::render::pendingRebuildCnt);
}

rendinst::RiExtraPool::~RiExtraPool()
{
  if (clonedFromIdx != -1) // does not own resources
  {
    res = nullptr;
    collRes = nullptr;
    destroyedPhysRes = nullptr;
    return;
  }
  if (res)
  {
    res->delRef();
    res = nullptr;
  }
  if (collRes)
  {
    if (unregCollCb)
      unregCollCb(collHandle);
    release_game_resource((GameResource *)collRes);
    collRes = nullptr;
  }
  if (destroyedPhysRes)
  {
    release_game_resource((GameResource *)destroyedPhysRes);
    destroyedPhysRes = nullptr;
  }
}


void rendinst::RiExtraPool::validateLodLimits()
{
  int maxl = min<int>(res->lods.size(), MAX_LODS) - 1;

  // RiExtra does not support impostors. Need to skip impostor and transition lods.
  maxl -= maxl ? hasTransitionLod + posInst : 0;

  for (int i = 0; i < 4; i++)
  {
    int shl = i * 8;
    int llm = lodLimits >> shl;
    int min_lod = clamp(llm & 0xF, 0, maxl), max_lod = clamp((llm >> 4) & 0xF, 0, maxl);

    lodLimits &= ~(0xFF << shl);
    lodLimits |= ((clamp(min_lod, 0, 15) & 0xF) | ((clamp(max_lod, 0, 15) & 0xF) << 4)) << shl;
  }
  // debug("riExtra[%d].lodLimits=%08X", this-riExtra.data(), lodLimits);
}


const DataBlock *rendinst::registerRIGenExtraConfig(const DataBlock *persist_props)
{
  const DataBlock *prev = riConfig;
  riConfig = persist_props;
  return prev;
}
bool rendinst::isRIGenExtraObstacle(const char *nm)
{
  if (!riConfig)
    return false;
  if (!nm || !*nm)
    return false;
  const DataBlock *b = riConfig->getBlockByNameEx("riExtra")->getBlockByName(nm);
  return !b || b->getBool("isObstacle", true);
}
bool rendinst::isRIGenExtraUsedInDestr(const char *nm)
{
  if (!riConfig)
    return false;
  return riConfig->getBlockByNameEx("riExtra")->getBlockByName(nm) != nullptr;
}

static inline float riResLodRange(RenderableInstanceLodsResource *riRes, int lod, const DataBlock *ri_ovr)
{
  static const char *lod_nm[] = {"lod0", "lod1", "lod2", "lod3"};
  if (lod < riRes->getQlMinAllowedLod())
    return -1; // lod = riRes->getQlMinAllowedLod();

  if (!ri_ovr || lod > 3)
    return riRes->lods[lod].range;

  const bool shouldOnlyExtend = ri_ovr->getBool("extendOnly", false);
  const float overrideVal = ri_ovr->getReal(lod_nm[lod], riRes->lods[lod].range);
  if (shouldOnlyExtend)
    return max(overrideVal, riRes->lods[lod].range);
  return overrideVal;
}

static const DataBlock &getRiParamsBlockByName(const char *ri_res_name)
{
  if (!riConfig)
    return DataBlock::emptyBlock;

  const DataBlock *b = riConfig->getBlockByNameEx("riExtra")->getBlockByNameEx(ri_res_name);
  for (int j = 0, nid = riConfig->getNameId("dmg"); !b && j < riConfig->blockCount(); j++)
    if (riConfig->getBlock(j)->getBlockNameId() == nid)
      if (strcmp(ri_res_name, riConfig->getBlock(j)->getStr("name", "")) == 0)
        b = riConfig->getBlock(j);

  if (b == nullptr) // For PVS only; getBlockByNameEx never returns nullptr; b != nullptr always
    return DataBlock::emptyBlock;

  return *b;
}

static void push_res_idx_to_stage(rendinst::LayerFlags res_layers, int res_idx)
{
  rendinst::ScopedRIExtraWriteLock wr;
  if (res_layers & rendinst::LayerFlag::Opaque)
    rendinst::riExPoolIdxPerStage[get_layer_index(rendinst::LayerFlag::Opaque)].push_back(res_idx);
  if (res_layers & rendinst::LayerFlag::Transparent)
    rendinst::riExPoolIdxPerStage[get_layer_index(rendinst::LayerFlag::Transparent)].push_back(res_idx);
  if (res_layers & rendinst::LayerFlag::Decals)
    rendinst::riExPoolIdxPerStage[get_layer_index(rendinst::LayerFlag::Decals)].push_back(res_idx);
  if (res_layers & rendinst::LayerFlag::Distortion)
    rendinst::riExPoolIdxPerStage[get_layer_index(rendinst::LayerFlag::Distortion)].push_back(res_idx);
}

static void erase_res_idx_from_stage(rendinst::LayerFlags res_layers, int res_idx)
{
  rendinst::ScopedRIExtraWriteLock wr;
  for (int i = 0; i < countof(rendinst::riExPoolIdxPerStage); i++)
  {
    if (!(res_layers & static_cast<rendinst::LayerFlag>(1 << i)))
      continue;
    uint16_t *it = eastl::find(rendinst::riExPoolIdxPerStage[i].begin(), rendinst::riExPoolIdxPerStage[i].end(), res_idx);
    G_ASSERT(it != rendinst::riExPoolIdxPerStage[i].end());
    erase_items_fast(rendinst::riExPoolIdxPerStage[i], it - rendinst::riExPoolIdxPerStage[i].begin(), 1);
  }
}

static void init_relems_for_new_pool(int new_res_idx)
{
  rendinst::render::on_ri_mesh_relems_updated_pool(new_res_idx);

  if (RendInstGenData::renderResRequired)
    rendinst::render::update_per_draw_gathered_data(new_res_idx);
}

int rendinst::addRIGenExtraResIdx(const char *ri_res_name, int ri_pool_ref, int ri_pool_ref_layer, AddRIFlags ri_flags)
{
  bool useShadow = bool(ri_flags & AddRIFlag::UseShadow);
  bool immortal = bool(ri_flags & AddRIFlag::Immortal);
  float combinedRendinstHeight = 0.0f;
  int id = riExtraMap.getNameId(ri_res_name);
  float ttl = 15.f;
  const DataBlock &paramsBlock = getRiParamsBlockByName(ri_res_name);

  if (!immortal)
  {
    immortal = paramsBlock.getBool("immortal", false);
    combinedRendinstHeight = paramsBlock.getReal("combinedRendinstHeight", 0.0f);
  }

  if (id >= 0)
  {
    if (useShadow)
      riExtra[id].useShadow = useShadow;
    if (immortal)
      riExtra[id].immortal = immortal;
    if (ri_pool_ref >= 0 && riExtra[id].riPoolRef < 0)
    {
      riExtra[id].riPoolRef = ri_pool_ref;
      riExtra[id].riPoolRefLayer = ri_pool_ref_layer;
      debug("updated riPoolRef=%d (@%d) for %s", ri_pool_ref, ri_pool_ref_layer, ri_res_name);
    }
    riExtra[id].rendinstHeight = combinedRendinstHeight;
    return id;
  }
  if (riExtra.size() >= riex_max_type())
  {
    logerr_ctx("RI pool overflow (%d) for %s", riExtra.size(), ri_res_name);
    return -1;
  }

  RenderableInstanceLodsResource *riRes =
    (RenderableInstanceLodsResource *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(ri_res_name), RendInstGameResClassId);

  if (!riRes)
  {
    logerr("failed to resolve riExtra <%s>", ri_res_name);
    riRes = rendinst::get_stub_res();
  }
  if (!riRes)
    return -1;

  if (!rendinst::render::vbExtraCtx[0].vb && RendInstGenData::renderResRequired && maxExtraRiCount)
  {
    debug("initRIGenExtra: due to addRIGenExtraResIdx()");
    rendinst::initRIGenExtra();
  }

  id = riExtraMap.addNameId(ri_res_name);
  G_ASSERT(id == riExtra.size());

  if (riExtra.size() == 0)
    hasRiClipmap = rendinst::HasRIClipmap::NO;

  if (id >= riExtra.capacity())
  {
    ScopedRIExtraWriteLock wr; // To consider: hold lock to whole duration of this function?
    riExtra.reserve(riExtra.capacity() ? (riExtra.capacity() + riExtra.capacity() / 2) : 192);
  }
  // custom atomic push_back() in order to avoid to read partially constructed pool data
  new (riExtra.data() + id, _NEW_INPLACE) RiExtraPool;
  riExtra.interlocked_increment_size();

  // Update riExtra[id] fields not related with game resources (riRes and collision)

  riExtra[id].poolColors[0] = riExtra[id].poolColors[1] = 0x80808080;
  if (ri_pool_ref >= 0 && ri_pool_ref_layer >= 0 && getRgLayer(ri_pool_ref_layer))
  {
    RendInstGenData::RtData *rtData = getRgLayer(ri_pool_ref_layer)->rtData;
    if (rtData)
    {
      riExtra[id].poolColors[0] = rtData->riColPair[ri_pool_ref * 2 + 0];
      riExtra[id].poolColors[1] = rtData->riColPair[ri_pool_ref * 2 + 1];
    }
  }

  // todo: make it project dependent and data driven
  riExtra[id].isWalls = strstr(ri_res_name, "walls") != 0 || strstr(ri_res_name, "house") != 0 || strstr(ri_res_name, "building") != 0;
  riExtra[id].useVsm = getRiParamsBlockByName(ri_res_name).getBool("useVsm", false);

  riExtra[id].isDynamicRendinst = bool(ri_flags & AddRIFlag::Dynamic);

  riExtra[id].riPoolRef = ri_pool_ref;
  riExtra[id].riPoolRefLayer = ri_pool_ref_layer;
  riExtra[id].useShadow = useShadow;
  riExtra[id].immortal = immortal;
  riExtra[id].ttl = ttl;
  riExtra[id].rendinstHeight = combinedRendinstHeight;
  riExtra[id].killsNearEffects = false;

  if (const DataBlock *b = riConfig && !immortal ? riConfig->getBlockByNameEx("riExtra")->getBlockByName(ri_res_name) : nullptr)
  {
    riExtra[id].initialHP = b->getReal("hp", 300);
    float regenTime = b->getReal("hpFullRegenTime", 180);
    riExtra[id].regenHpRate = (riExtra[id].initialHP > 0 && regenTime > 0) ? riExtra[id].initialHP / regenTime : 0;
    riExtra[id].destroyedColl = b->getBool("nextColl", true);
    riExtra[id].damageThreshold = b->getReal("damageThreshold", b->getReal("impulseThreshold", 0));
    riExtra[id].rendinstHeight = b->getReal("combinedRendinstHeight", 0.0f);
    riExtra[id].killsNearEffects = b->getBool("killsNearEffects", false);
#if RI_VERBOSE_OUTPUT
    debug("riExtra hp=%.1f damageThres=%.3f regenRate=%.3f", riExtra[id].initialHP, riExtra[id].damageThreshold,
      riExtra[id].regenHpRate);
#endif
    const char *next_res = b->getStr("nextRes", nullptr);
    if (next_res && *next_res)
    {
      // At first we should add a new instance to riExtra and only then
      // address riExtra by id in order to assign instance to destroyedRiIdx field,
      // otherwise a memory corruption occurs
      int destrIdx = addRIGenExtraResIdx(next_res, -1, -1, useShadow ? AddRIFlag::UseShadow : AddRIFlag{});
      riExtra[id].destroyedRiIdx = destrIdx;
      if (destrIdx >= 0)
      {
        riExtra[id].destrDepth = riExtra[destrIdx].destrDepth + 1;
        riExtra[destrIdx].parentForDestroyedRiIdx = id;
      }
    }
    else if (next_res)
      logerr("empty nextRes for riExtra=%s", ri_res_name);

    next_res = b->getStr("physRes", nullptr);
    if (next_res && *next_res)
    {
      riExtra[id].destroyedPhysRes =
        (DynamicPhysObjectData *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(next_res), PhysObjGameResClassId);
      riExtra[id].isDestroyedPhysResExist = riExtra[id].destroyedPhysRes != nullptr;
    }
    else if (next_res)
      logerr("empty physRes for riExtra=%s", ri_res_name);
  }
  else if (riConfig && !immortal)
  {
    // try to find older damage settings
    int nid = riConfig->getNameId("dmg");
    const DataBlock *debrisBlk = nullptr;
    for (int i = 0, ie = riConfig->blockCount(); i < ie; i++)
    {
      if (riConfig->getBlock(i)->getBlockNameId() == nid)
        if (strcmp(ri_res_name, riConfig->getBlock(i)->getStr("name", "")) == 0)
        {
          const DataBlock *blk = riConfig->getBlock(i);
          riExtra[id].initialHP = blk->getReal("destructionImpulse", 0) / riConfig->getReal("hitPointsToDestrImpulseMult", 3000);
          if (blk->paramExists("debris"))
            debrisBlk = blk;
          if (riExtra[id].initialHP <= 0 || riExtra[id].initialHP < riConfig->getReal("minRendInstHP", 30))
          {
            riExtra[id].initialHP = -1;
            break;
          }
          float regenTime = blk->getReal("hpFullRegenTime", 180);
          riExtra[id].regenHpRate = (riExtra[id].initialHP > 0 && regenTime > 0) ? riExtra[id].initialHP / regenTime : 0;
          riExtra[id].damageThreshold = riExtra[id].initialHP / 1000;
#if RI_VERBOSE_OUTPUT
          debug("riGen   hp=%.1f damageThres=%.3f regenRate=%.3f", riExtra[id].initialHP, riExtra[id].damageThreshold,
            riExtra[id].regenHpRate);
#endif
          break;
        }
    }
    if (riExtra[id].destroyedRiIdx < 0)
    {
      if (!debrisBlk && (ri_flags & AddRIFlag::ForceDebris))
        debrisBlk = riConfig->getBlockByName("default_building");
      if (debrisBlk)
      {
        const char *debrisRes = debrisBlk->getStr("debris", nullptr);
        if (debrisRes && *debrisRes)
        {
          int destrIdx =
            addRIGenExtraResIdx(debrisRes, -1, -1, (useShadow ? AddRIFlag::UseShadow : AddRIFlag{}) | AddRIFlag::Immortal);
          riExtra[id].destroyedRiIdx = destrIdx;
          riExtra[id].destroyedColl = 1;
          if (destrIdx >= 0)
          {
            riExtra[id].destrDepth = riExtra[destrIdx].destrDepth + 1;
            riExtra[destrIdx].parentForDestroyedRiIdx = id;
          }
        }
        riExtra[id].scaleDebris = debrisBlk->getBool("scaleDebris", false);
      }
    }
  }

  update_ri_extra_game_resources(ri_res_name, id, riRes, paramsBlock);

  return id;
}

namespace rendinst
{
static eastl::vector_map<uint32_t, float> riExtraBBoxScaleForPrepasses;

static void update_ri_extra_game_resources(const char *ri_res_name, int id, RenderableInstanceLodsResource *riRes,
  const DataBlock &params_block)
{
  G_ASSERT_RETURN(riRes, );

  CollisionResource *collRes = nullptr;
  String coll_name(128, "%s" RI_COLLISION_RES_SUFFIX, ri_res_name);
  collRes = get_resource_type_id(coll_name) == CollisionGameResClassId
              ? (CollisionResource *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(coll_name), CollisionGameResClassId)
              : nullptr;
  if (collRes && collRes->getAllNodes().empty())
  {
    release_game_resource((GameResource *)collRes);
    collRes = nullptr;
  }
  if (collRes)
    collRes->collapseAndOptimize(USE_GRID_FOR_RI);

  // Update riExtra[id] fields which depends on game resources (riRes and collision)

  riExtra[id].res = riRes;
  riExtra[id].collRes = collRes;
  if (collRes && regCollCb)
  {
    const bool isCollidable = params_block.getBool("isCollidable", true);
    if (isCollidable)
      riExtra[id].collHandle = regCollCb(collRes, coll_name.str());
  }

  riExtra[id].bsphXYZR = v_perm_xyzd(v_ldu(&riRes->bsphCenter.x), v_splat4(&riRes->bsphRad));
  if (collRes)
    riExtra[id].bsphXYZR = collRes->getBoundingSphereXYZR();

  riExtra[id].isTree = riExtra[id].posInst = riRes->hasImpostor();
  riExtra[id].lbb.bmin = v_and(v_ldu(&riRes->bbox[0].x), v_cast_vec4f(V_CI_MASK1110));
  riExtra[id].lbb.bmax = v_and(v_ldu(&riRes->bbox[1].x), v_cast_vec4f(V_CI_MASK1110));
  riExtra[id].collBb = collRes ? collRes->vFullBBox : riExtra[id].lbb;
  if (
    is_ri_extra_for_inst_count_only(ri_res_name) || get_skip_nearest_lods(ri_res_name, riRes->hasImpostor(), riRes->lods.size()) == 0)
    riExtra[id].lodLimits = 0xF0F0F0F0;
  v_bbox3_init_empty(riExtra[id].fullWabb);
  Point3 box_wd = riRes->bbox.width();
  if (riRes->bbox.isempty())
  {
    logerr("%s: empty RI bbox, collRes=%p", ri_res_name, collRes);
    if (collRes)
      riExtra[id].lbb = riExtra[id].collBb;
    else
      riExtra[id].lbb.bmin = riExtra[id].collBb.bmin = riExtra[id].lbb.bmax = riExtra[id].collBb.bmax = v_zero();
    box_wd.set(0, 0, 0);
  }
  riExtra[id].largeOccluder = !riRes->hasImpostor() && box_wd.x * box_wd.y * box_wd.z > 30;
  riExtra[id].hasOccluder = 0;
  riExtra[id].usedInLandmaskHeight = 0;
  riExtra[id].hideMask = rendinst::getResHideMask(ri_res_name, &riRes->bbox);
  riExtra[id].validateLodLimits();

  if (RendInstGenData::renderResRequired)
  {
    const Point3 &sphereCenter = riRes->bsphCenter;
    riExtra[id].sphereRadius = riRes->bsphRad + sqrtf(sphereCenter.x * sphereCenter.x + sphereCenter.z * sphereCenter.z);
    riExtra[id].sphCenterY = sphereCenter.y;
  }
  else
    riExtra[id].sphereRadius = riExtra[id].sphCenterY = 0;

  const DataBlock *ri_ovr = rendinst::ri_lod_ranges_ovr.getBlockByName(ri_res_name);
  int lod_cnt = riRes->lods.size();
  float max_lod_dist = lod_cnt ? riResLodRange(riRes, lod_cnt - 1, ri_ovr) : (RendInstGenData::renderResRequired ? 0.0f : 1e5f);
  // RiExtra does not support impostors. Need to skip impostor and transition lods.
  if (riExtra[id].isPosInst() && lod_cnt > 1)
  {
    Tab<ShaderMaterial *> mats;
    riRes->lods[lod_cnt - 2].scene->gatherUsedMat(mats); // transition is penultimate lod
    for (size_t j = 0; j < mats.size(); ++j)
    {
      if (riRes->hasImpostorVars(mats[j]))
        riExtra[id].hasTransitionLod = true;
    }
    lod_cnt -= 1 + riExtra[id].hasTransitionLod;
  }

  if (lod_cnt > rendinst::RiExtraPool::MAX_LODS)
    logerr("riExtra[%d] %s, have %d lods but MAX_LODS are %d, please reduce number of lods", id, ri_res_name, lod_cnt,
      rendinst::RiExtraPool::MAX_LODS);

  if (RendInstGenData::renderResRequired)
  {
    static int atest_variable_id = VariableMap::getVariableId("atest");
    static int is_colored_variable_id = VariableMap::getVariableId("is_colored");
    static int is_rendinst_clipmap_variable_id = VariableMap::getVariableId("is_rendinst_clipmap");
    static int enable_hmap_blend_variable_id = VariableMap::getVariableId("enable_hmap_blend");
    static int material_pn_triangulation_variable_id = VariableMap::getVariableId("material_pn_triangulation");
    static int draw_grass_variable_id = VariableMap::getVariableId("draw_grass");

    static int ri_landclass_indexVarId = VariableMap::getVariableId("ri_landclass_index");
    bool isPoolRendinstLandclass = false;

    LayerFlags layers = {};
    riExtra[id].hasColoredShaders = riExtra[id].isPosInst();
    riExtra[id].isRendinstClipmap = 0;
    riExtra[id].isPaintFxOnHit = params_block.getBool("isPaintFxOnHit", true);
    for (unsigned int lodNo = 0; lodNo < min<int>(lod_cnt, rendinst::RiExtraPool::MAX_LODS); lodNo++)
    {
      ShaderMesh *m = riRes->lods[lodNo].scene->getMesh()->getMesh()->getMesh();
      uint32_t mask = 0;
      uint32_t cmask = 0;
      uint32_t tessellationMask = 0;
      G_STATIC_ASSERT(sizeof(mask) == sizeof(riExtra[id].elemMask[0].atest));
      G_STATIC_ASSERT(sizeof(tessellationMask) == sizeof(riExtra[id].elemMask[0].tessellation));
      G_STATIC_ASSERT(sizeof(riExtra[id].elemMask[0].atest) == sizeof(riExtra[id].elemMask[0].cullN));
      dag::Span<ShaderMesh::RElem> elems = m->getElems(m->STG_opaque, m->STG_decal);
      G_ASSERTF(elems.size() <= sizeof(mask) * 8, "elems.size()=%d mask=%d bits", elems.size(), sizeof(mask) * 8);
      if (m->getElems(m->STG_opaque, m->STG_imm_decal).size())
        layers |= rendinst::LayerFlag::Opaque;
      if (m->getElems(m->STG_trans).size())
        layers |= rendinst::LayerFlag::Transparent;
      if (m->getElems(m->STG_decal).size())
        layers |= rendinst::LayerFlag::Decals;
      if (m->getElems(m->STG_distortion).size())
        layers |= rendinst::LayerFlag::Distortion;

      for (unsigned int elemNo = 0; elemNo < elems.size(); elemNo++)
        if (elems[elemNo].vertexData)
        {
          int atest = 0;
          const char *className = elems[elemNo].mat->getShaderClassName();
          className = className ? className : "";
          bool isTreeShader = strstr(className, "facing_leaves") || strstr(className, "rendinst_tree");

          // Not atest but must use separate shader to apply the wind.
          if (isTreeShader || (elems[elemNo].mat->getIntVariable(atest_variable_id, atest) && atest > 0))
            mask |= 1 << elemNo;

          riExtra[id].isTree |= isTreeShader;

          if ((elems[elemNo].mat->get_flags() & SHFLG_2SIDED) && !(elems[elemNo].mat->get_flags() & SHFLG_REAL2SIDED))
            cmask |= 1 << elemNo;
          riExtra[id].usingClipmap |= (strstr(className, "rendinst_layered_hmap_vertex_blend") != nullptr);
          riExtra[id].hasDynamicDisplacement |= strstr(className, "rendinst_flag_colored") != nullptr ||
                                                strstr(className, "rendinst_flag_layered") != nullptr ||
                                                strstr(className, "rendinst_anomaly") != nullptr;
          riExtra[id].patchesHeightmap |= strstr(className, "rendinst_heightmap_patch") != nullptr;
          int isColoredShader = 0;
          riExtra[id].hasColoredShaders |=
            elems[elemNo].mat->getIntVariable(is_colored_variable_id, isColoredShader) && isColoredShader;
          int isRendinstClipmap = 0;
          int isHeightmapBlended = 0;

          riExtra[id].isRendinstClipmap |=
            elems[elemNo].mat->getIntVariable(is_rendinst_clipmap_variable_id, isRendinstClipmap) && isRendinstClipmap;

          riExtra[id].isRendinstClipmap |=
            elems[elemNo].mat->getIntVariable(enable_hmap_blend_variable_id, isHeightmapBlended) && isHeightmapBlended;

          int isElemRiLandclass = 0;
          isPoolRendinstLandclass |= elems[elemNo].mat->getIntVariable(ri_landclass_indexVarId, isElemRiLandclass);

          int isTessellated = 0;
          if (elems[elemNo].mat->getIntVariable(material_pn_triangulation_variable_id, isTessellated) && isTessellated)
            tessellationMask |= 1 << elemNo;

          int drawGrass = 0;
          riExtra[id].usedInLandmaskHeight |= elems[elemNo].mat->getIntVariable(draw_grass_variable_id, drawGrass) && drawGrass;
          if (riExtra[id].isRendinstClipmap)
          {
            hasRiClipmap = rendinst::HasRIClipmap::YES;
            riExtra[id].immortal = true;
          }

          if (isPoolRendinstLandclass)
          {
            int uniqueRendinstLandclassId = id; // pool index is used as unique ri landclass index
            elems[elemNo].mat->set_int_param(ri_landclass_indexVarId, uniqueRendinstLandclassId);
          }
        }
      riExtra[id].elemMask[lodNo].atest = mask;
      riExtra[id].elemMask[lodNo].cullN = cmask;
      riExtra[id].elemMask[lodNo].tessellation = tessellationMask;
    }

    riExtra[id].riLandclassCachedData.clear();
    if (isPoolRendinstLandclass)
    {
      RiExtraPool::RiLandclassCachedData &riLandclassCachedData = riExtra[id].riLandclassCachedData.emplace_back();
      riLandclassCachedData.index = id;

      static int texcoord_scale_offsetVarId = VariableMap::getVariableId("texcoord_scale_offset");

      ShaderMesh *m = riExtra[id].res->lods[0].scene->getMesh()->getMesh()->getMesh(); // lod0 should have mapping

      bool hasMapping = false;
      for (ShaderMesh::RElem &elem : m->getElems(m->STG_opaque, m->STG_decal))
      {
        Color4 mappingVal;
        if (elem.mat->getColor4Variable(texcoord_scale_offsetVarId, mappingVal))
        {
          riLandclassCachedData.mapping = Point4(mappingVal.r, mappingVal.g, mappingVal.b, mappingVal.a);
          hasMapping = true;
          break;
        }
      }
      if (!hasMapping)
      {
        logerr("RI landclass has no mapping data!");
        riExtra[id].riLandclassCachedData.clear();
      }
    }

    ri_extra_max_height_on_ri_resource_added(riExtra[id]);

    riExtra[id].layers = layers;
    push_res_idx_to_stage(layers, id);
  }

  riExtra[id].qlPrevBestLod = riRes->getQlBestLod();

  riExtra[id].distSqLOD[0] = lod_cnt > 1 ? riResLodRange(riRes, 0, ri_ovr) : max_lod_dist;
#if RI_VERBOSE_OUTPUT
  debug("riExtra[%d] %s, %d lods, max_dist=%.1f, rad=%.3f "
        "atest_m=%04X, %04X, %04X, %04X cull_m=%04X, %04X, %04X, %04X%s %.0f hm=%02X LL=%04X",
    id, ri_res_name, lod_cnt, max_lod_dist, riExtra[id].bsphRad(), riExtra[id].elemMask[0].atest, riExtra[id].elemMask[1].atest,
    riExtra[id].elemMask[2].atest, riExtra[id].elemMask[3].atest, riExtra[id].elemMask[0].cullN, riExtra[id].elemMask[1].cullN,
    riExtra[id].elemMask[2].cullN, riExtra[id].elemMask[3].cullN, riExtra[id].largeOccluder ? " LARGE" : "",
    box_wd.x * box_wd.y * box_wd.z, riExtra[id].hideMask, riExtra[id].lodLimits);
#endif
  if (lod_cnt > 0)
  {
    for (int l = 1; l < rendinst::RiExtraPool::MAX_LODS; l++)
      riExtra[id].distSqLOD[l] =
        l < lod_cnt ? (l < lod_cnt - 1 ? riResLodRange(riRes, l, ri_ovr) : max_lod_dist) : riExtra[id].distSqLOD[lod_cnt - 1] - 0.1f;
    for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
      riExtra[id].distSqLOD[l] *= riExtra[id].distSqLOD[l];
  }
  else
    for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
      riExtra[id].distSqLOD[l] = max_lod_dist * max_lod_dist;

  rendinst::add_ri_pool_to_tiled_scenes(riExtra[id], id, ri_res_name, max_lod_dist);
  // this is hacky way to prevent constant invalidating of cache due to moving rendinsts, but still preserve validity of cache.
  // todo: We need to correctly maintain cache for static and separate cache for dynamic, which is invalidate only if it is in
  // someone's cache
  if (riExtra[id].tsIndex == DYNAMIC_SCENE)
  {
    riExtra[id].collBb = collRes ? collRes->vFullBBox : riExtra[id].lbb;
    vec4f wd = v_sub(riExtra[id].collBb.bmax, riExtra[id].collBb.bmin);
    vec4f center = v_bbox3_center(riExtra[id].collBb);
    riExtra[id].collBb.bmin = v_sub(center, wd);
    riExtra[id].collBb.bmax = v_add(center, wd);
    riExtra[id].bsphXYZR = v_perm_xyzd(riExtra[id].bsphXYZR, v_add(riExtra[id].bsphXYZR, riExtra[id].bsphXYZR));
  }
  if (!rendinst::riExtraBBoxScaleForPrepasses.empty())
  {
    auto scaleFound = riExtraBBoxScaleForPrepasses.find(str_hash_fnv1(ri_res_name));
    if (scaleFound != riExtraBBoxScaleForPrepasses.end())
    {
      riExtra[id].scaleForPrepasses = scaleFound->second;
    }
  }
  init_relems_for_new_pool(id);
}

static void unload_ri_extra_game_resources(int res_idx)
{
  RiExtraPool &riExtraPool = riExtra[res_idx];
  if (riExtraPool.res)
  {
    riExtraPool.res->delRef();
    riExtraPool.res = nullptr;
  }
  if (riExtraPool.collRes)
  {
    if (unregCollCb)
      unregCollCb(riExtraPool.collHandle);
    release_game_resource((GameResource *)riExtraPool.collRes);
    riExtraPool.collRes = nullptr;
  }
  erase_res_idx_from_stage(riExtraPool.layers, res_idx);
}
} // namespace rendinst

void rendinst::updateRiExtraBBoxScalesForPrepasses(const DataBlock &blk)
{
  rendinst::riExtraBBoxScaleForPrepasses.clear();
  auto iterateBBoxScales = [&blk](int paramIdx, uint32_t nameId, uint32_t paramType) {
    G_UNUSED(paramIdx);
    G_UNUSED(paramType);
    const char *name = blk.getName(nameId);
    float val = blk.getRealByNameId(nameId, 1.0f);
    if (val != 1.0f)
    {
      rendinst::riExtraBBoxScaleForPrepasses[str_hash_fnv1(name)] = val;
    }
  };
  dblk::iterate_params(blk, iterateBBoxScales);
}

int rendinst::cloneRIGenExtraResIdx(const char *source_res_name, const char *dst_res_name)
{
  int sourceId = riExtraMap.getNameId(source_res_name);
  if (sourceId < 0)
  {
    logerr("Cannot clone from %s into %s because the original does not exist (yet?)", source_res_name, dst_res_name);
    return -1;
  }
  int newId = riExtraMap.addNameId(dst_res_name);
  G_ASSERT(newId == riExtra.size());
  riExtra.push_back(); // Warning: not thread safe
  riExtra[newId] = riExtra[sourceId];
  riExtra[newId].clonedFromIdx = sourceId;

  push_res_idx_to_stage(riExtra[newId].layers, newId);

  const DataBlock *ri_ovr = rendinst::ri_lod_ranges_ovr.getBlockByName(source_res_name);
  int lod_cnt = riExtra[newId].res->lods.size();
  float max_lod_dist =
    lod_cnt ? riResLodRange(riExtra[newId].res, lod_cnt - 1, ri_ovr) : (RendInstGenData::renderResRequired ? 0.0f : 1e5f);
  rendinst::add_ri_pool_to_tiled_scenes(riExtra[newId], newId, dst_res_name, max_lod_dist);

  init_relems_for_new_pool(newId);

  int sourcePoolRef = riExtra[sourceId].riPoolRef;
  if (RendInstGenData *rgl = (sourcePoolRef >= 0) ? rendinst::getRgLayer(riExtra[sourceId].riPoolRefLayer) : nullptr)
  {
    const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[sourcePoolRef];
    riExtra[newId].riPoolRef = rgl->rtData->riProperties.size();
    rgl->rtData->riProperties.push_back(RendInstGenData::RendinstProperties(riProp));
    rgl->rtData->riDestr.push_back(RendInstGenData::DestrProps(rgl->rtData->riDestr[sourcePoolRef]));
  }
  rendinst::addRiGenExtraDebris(newId, 0);
  return newId;
}

void rendinst::addRiGenExtraDebris(uint32_t res_idx, int layer)
{
  RendInstGenData *rgl = getRgLayer(layer);
  if (!rgl)
    return;

  G_ASSERT(riConfig);
  G_ASSERT(res_idx < riExtra.size());
  if (riExtra[res_idx].riPoolRef >= 0)
    return;

  RendInstGenData::RtData *rtData = rgl->rtData;
  G_ASSERT(rtData);

  rtData->addDebrisForRiExtraRange(*riConfig, res_idx, 1);
  if (riExtra[res_idx].destroyedRiIdx >= 0)
    addRiGenExtraDebris(riExtra[res_idx].destroyedRiIdx, layer);
}

int rendinst::getRIGenExtraResIdx(const char *ri_res_name) { return riExtraMap.getNameId(ri_res_name); }

void rendinst::reloadRIExtraResources(const char *ri_res_name)
{
  int resIdx = getRIGenExtraResIdx(ri_res_name);
  if (resIdx < 0)
    return;
  unload_ri_extra_game_resources(resIdx);
  RenderableInstanceLodsResource *riRes =
    (RenderableInstanceLodsResource *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(ri_res_name), RendInstGameResClassId);

  if (!riRes)
  {
    logerr("failed to resolve riExtra <%s>", ri_res_name);
    riRes = rendinst::get_stub_res();
  }
  if (!riRes)
    return;
  update_ri_extra_game_resources(ri_res_name, resIdx, riRes, getRiParamsBlockByName(ri_res_name));
}

RenderableInstanceLodsResource *rendinst::getRIGenExtraRes(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return nullptr;
  return riExtra[res_idx].res;
}
CollisionResource *rendinst::getRIGenExtraCollRes(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return nullptr;
  return riExtra[res_idx].collRes;
}
const bbox3f *rendinst::getRIGenExtraCollBb(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return nullptr;
  return &riExtra[res_idx].collBb;
}

dag::Vector<BBox3> rendinst::getRIGenExtraInstancesWorldBboxesByGrid(int res_idx, Point2 grid_origin, Point2 grid_size,
  IPoint2 grid_cells)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return dag::Vector<BBox3>{};

  TIME_PROFILE(gatherRiExtraBboxesInGrid);

  Point2 cellsPerMeter = div(Point2(grid_cells), grid_size);
  Point2 cellSize(1.0 / cellsPerMeter.x, 1.0 / cellsPerMeter.y);
  IPoint2 cellMin(0, 0);
  IPoint2 cellMax(grid_cells - IPoint2(1, 1));

  dag::Vector<BBox3> gridBoxes(grid_cells.x * grid_cells.y);
  for (const mat43f &riTm : rendinst::riExtra[res_idx].riTm)
  {
    mat44f riTm44f;
    v_mat43_transpose_to_mat44(riTm44f, riTm);

    bbox3f vBox;
    v_bbox3_init_empty(vBox);
    v_bbox3_add_transformed_box(vBox, riTm44f, rendinst::riExtra[res_idx].lbb);

    BBox3 box;
    v_stu_bbox3(box, vBox);
    BBox2 boxXZ(Point2(box.lim[0].x, box.lim[0].z), Point2(box.lim[1].x, box.lim[1].z));

    IPoint2 cellFrom = max(cellMin, min(cellMax, to_ipoint2(mul(boxXZ.lim[0] - grid_origin, cellsPerMeter))));
    IPoint2 cellTo = max(cellMin, min(cellMax, to_ipoint2(mul(boxXZ.lim[1] - grid_origin, cellsPerMeter))));

    for (int x = cellFrom.x; x <= cellTo.x; ++x)
    {
      for (int y = cellFrom.y; y <= cellTo.y; ++y)
      {
        gridBoxes[x * grid_cells.y + y] += box;
      }
    }
  }

  for (int x = 0; x < grid_cells.x; ++x)
  {
    for (int y = 0; y < grid_cells.y; ++y)
    {
      BBox3 cellLimitBox;
      cellLimitBox.lim[0].x = x * cellSize.x + grid_origin.x;
      cellLimitBox.lim[0].y = MIN_REAL / 4;
      cellLimitBox.lim[0].z = y * cellSize.y + grid_origin.y;
      cellLimitBox.lim[1].x = (x + 1) * cellSize.x + grid_origin.x;
      cellLimitBox.lim[1].y = MAX_REAL / 4;
      cellLimitBox.lim[1].z = (y + 1) * cellSize.y + grid_origin.y;

      int id = x * grid_cells.y + y;
      gridBoxes[id] = gridBoxes[id].getIntersection(cellLimitBox);
    }
  }

  auto newEnd = eastl::remove_if(gridBoxes.begin(), gridBoxes.end(), [](const BBox3 &box) {
    return box.lim[0].x > box.lim[1].x; // We either have emptiness for all dimensions or for none of them, so can check only one dim.
  });
  gridBoxes.resize(newEnd - gridBoxes.begin());

  return gridBoxes;
}

bbox3f rendinst::getRIGenExtraOverallInstancesWorldBbox(int res_idx)
{
  bbox3f result;
  v_bbox3_init_empty(result);
  if (res_idx >= 0 && res_idx < riExtra.size())
  {
    for (const mat43f &riTm : rendinst::riExtra[res_idx].riTm)
    {
      mat44f riTm44f;
      v_mat43_transpose_to_mat44(riTm44f, riTm);
      v_bbox3_add_transformed_box(result, riTm44f, rendinst::riExtra[res_idx].lbb);
    }
  }
  return result;
}

riex_handle_t rendinst::addRIGenExtra43(int res_idx, bool /*m*/, mat43f_cref tm, bool has_collision, int orig_cell, int orig_offset,
  int add_data_dwords, const int32_t *add_data, bool on_loading)
{
  if (res_idx < 0 || res_idx > riExtra.size())
    return RIEX_HANDLE_NULL;

  RiExtraPool &pool = riExtra[res_idx];
  scene::node_index ni = scene::INVALID_NODE;
  riex_handle_t h = RIEX_HANDLE_NULL;
  vec4f sphere = v_zero();
  { // scoped lock
    ScopedRIExtraWriteLock wr;
    int idx = -1;
    if (pool.uuIdx.size())
    {
      idx = pool.uuIdx.back();
      pool.uuIdx.pop_back();
      // debug_ctx("pool.uuIdx=%d idx=%d tm=%d col=%d", pool.uuIdx.size(), idx, pool.riTm.size(), pool.riColPair.size());
      if (idx >= pool.riTm.size())
      {
        logerr_ctx("Invalid index in RI pool.uuIdx = %d/%d", idx, pool.riTm.size());
        idx = -1;
      }
    }

    if (idx == -1)
    {
      if (pool.riTm.size() >= riex_max_inst())
      {
        logerr_ctx("RI overflow (%d) for %s", pool.riTm.size(), riExtraMap.getName(res_idx));
        return RIEX_HANDLE_NULL;
      }
      idx = append_items(pool.riTm, 1);
      // debug_ctx("idx=%d tm=%d col=%d cf=%d", idx, pool.riTm.size(), pool.riColPair.size(), pool.riCollFlag.size());
      append_items(pool.riHP, 1);
      append_items(pool.riUniqueData, 1);
      append_items(pool.riXYZR, 1);
    }

    pool.riTm[idx] = tm;
    pool.riHP[idx].init(pool.initialHP);
    pool.riUniqueData[idx].cellId = orig_cell;
    pool.riUniqueData[idx].offset = orig_offset;
    h = make_handle(res_idx, idx);

    mat44f tm44;
    v_mat43_transpose_to_mat44(tm44, tm);
    bbox3f wabb;
    v_bbox3_init(wabb, tm44, pool.collBb);
    v_bbox3_add_box(pool.fullWabb, wabb);

    pool.riXYZR[idx] = make_pos_and_rad(tm44, pool.bsphXYZR);
    if (has_collision && pool.collRes)
    {
      riExtraGrid.insert(h, pool.riXYZR[idx], wabb, on_loading);
      riutil::world_version_inc(wabb);

      update_max_ri_extra_height(static_cast<int>(v_extract_y(wabb.bmax) - v_extract_y(wabb.bmin)) + 1);
    }
    else
      pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_or(pool.riXYZR[idx], V_CI_SIGN_MASK));

    G_ASSERTF(!(pool.hideMask & 2) || (pool.hideMask & 1), "pool should be either invisible always, or in mode0 (planes) only");
    ni = (pool.tsIndex >= 0) ? alloc_instance_for_tiled_scene(pool, res_idx, idx, tm44, sphere) : scene::INVALID_NODE;

    ri_extra_max_height_on_ri_added_or_moved(pool, v_extract_y(wabb.bmax));
  }
  // after write lock
  if (ni != scene::INVALID_NODE)
    init_instance_user_data_for_tiled_scene(pool, ni, sphere, add_data_dwords, add_data);

  return h;
}
riex_handle_t rendinst::addRIGenExtra44(int res_idx, bool movable, mat44f_cref tm, bool has_collision, int orig_cell, int orig_offset,
  int add_data_dwords, const int32_t *add_data, bool on_loading)
{
  if (res_idx < 0 || res_idx > riExtra.size())
    return RIEX_HANDLE_NULL;
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm);
  return addRIGenExtra43(res_idx, movable, m43, has_collision, orig_cell, orig_offset, add_data_dwords, add_data, on_loading);
}

void rendinst::setCASRIGenExtraData(riex_render_info_t id, uint32_t start, const uint32_t *wasdata, const uint32_t *newdata,
  uint32_t cnt)
{
  set_cas_user_data(id, start, wasdata, newdata, cnt);
}
void rendinst::setRIGenExtraData(rendinst::riex_render_info_t id, uint32_t start, const uint32_t *udata, uint32_t cnt)
{
  set_user_data(id, start, udata, cnt);
}
const mat44f &rendinst::getRIGenExtraTiledSceneNode(rendinst::riex_render_info_t id) { return get_tiled_scene_node(id); }

void rendinst::moveToOriginalScene(riex_handle_t id)
{
  bool do_not_wait = false;
  ScopedRIExtraWriteTryLock wr(do_not_wait);
  if (!wr.isLocked())
    return;

  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT_RETURN(res_idx < riExtra.size(), );
  RiExtraPool &pool = riExtra[res_idx];

  if (idx < pool.tsNodeIdx.size() && pool.tsNodeIdx[idx] != scene::INVALID_NODE)
    move_instance_to_original_scene(pool, res_idx, pool.tsNodeIdx[idx]);
}

bool rendinst::moveRIGenExtra43(riex_handle_t id, mat43f_cref tm, bool moved, bool do_not_wait)
{
  ScopedRIExtraWriteTryLock wr(do_not_wait);
  if (!wr.isLocked())
    return false;

  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT_RETURN(res_idx < riExtra.size(), true);
  RiExtraPool &pool = riExtra[res_idx];
  if (!pool.isValid(idx))
  {
    logmessage(LOGMESSAGE_LEVEL, "moveRIGenExtra43 idx out of range or dead: idx=%d, count=%d (res_idx=%d)", idx, pool.riTm.size(),
      res_idx);
    return true;
  }

  mat44f tm44;
  v_mat43_transpose_to_mat44(tm44, tm);
  if (idx < pool.tsNodeIdx.size() && pool.tsNodeIdx[idx] != scene::INVALID_NODE)
    move_instance_in_tiled_scene(pool, res_idx, pool.tsNodeIdx[idx], tm44, do_not_wait);

  bbox3f wabb1;
  v_bbox3_init(wabb1, tm44, pool.collBb);
  vec4f bsphere = make_pos_and_rad(tm44, pool.bsphXYZR);
  if (moved && pool.collRes && pool.isInGrid(idx))
  {
    mat44f tm44_old;
    bbox3f wabb0;

    v_mat43_transpose_to_mat44(tm44_old, pool.riTm[idx]);
    v_bbox3_init(wabb0, tm44_old, pool.collBb);
    vec4f oldWbsph = pool.riXYZR[idx];

    pool.riXYZR[idx] = bsphere;
    riExtraGrid.update(id, oldWbsph, pool.riXYZR[idx], wabb1);
    update_max_ri_extra_height(static_cast<int>(v_extract_y(wabb1.bmax) - v_extract_y(wabb1.bmin)) + 1);

    v_bbox3_add_box(pool.fullWabb, wabb1);
    // this is hacky way to prevent constant invalidating of cache due to moving rendinsts.
    // todo: We need to correctly maintain cache for static and separate cache for dynamic, which is invalidate only if it is in
    // someone's cache
    if (pool.tsIndex != DYNAMIC_SCENE)
    {
      v_bbox3_add_box(wabb0, wabb1);
      riutil::world_version_inc(wabb0);
    }

    ri_extra_max_height_on_ri_added_or_moved(pool, v_extract_y(wabb1.bmax));
  }
  else
  {
    pool.riXYZR[idx] = v_perm_xyzd(bsphere, v_or(bsphere, V_CI_SIGN_MASK));
    v_bbox3_add_box(pool.fullWabb, wabb1);
  }
  pool.riTm[idx] = tm;

  return true;
}

bool rendinst::moveRIGenExtra44(riex_handle_t id, mat44f_cref tm, bool moved, bool do_not_wait)
{
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm);
  return moveRIGenExtra43(id, m43, moved, do_not_wait);
}

bool rendinst::delRIGenExtra(riex_handle_t id)
{
  if (id == RIEX_HANDLE_NULL)
    return false;

  ScopedRIExtraWriteLock wr;
  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT_RETURN(res_idx < riExtra.size(), false);
  RiExtraPool &pool = riExtra[res_idx];
  if (!pool.isValid(idx))
    return false;
  if (pool.isRendinstClipmap && hasRiClipmap == rendinst::HasRIClipmap::YES)
    hasRiClipmap = rendinst::HasRIClipmap::UNKNOWN;
  G_ASSERT(find_value_idx(pool.uuIdx, idx) == -1);

  for (invalidate_handle_cb cb : invalidate_handle_callbacks)
    cb(id);

  if (idx < pool.tsNodeIdx.size() && pool.tsNodeIdx[idx] != scene::INVALID_NODE)
    rendinst::remove_instance_from_tiled_scene(pool.tsNodeIdx[idx]);

  if (pool.collRes && pool.isInGrid(idx))
  {
    mat44f tm44;
    bbox3f wabb;
    v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);
    v_bbox3_init(wabb, tm44, pool.collBb);

    riExtraGrid.erase(id, pool.riXYZR[idx]);
    pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_or(pool.riXYZR[idx], V_CI_SIGN_MASK));

    riutil::world_version_inc(wabb);
  }

  if (idx + 1 == pool.riTm.size())
  {
    pool.riTm.pop_back();
    pool.riHP.pop_back();
    pool.riUniqueData.pop_back();
    pool.riXYZR.pop_back();
    if (idx + 1 == pool.tsNodeIdx.size())
      pool.tsNodeIdx.pop_back();
  }
  else
  {
    pool.riTm[idx].row0 = pool.riTm[idx].row1 = pool.riTm[idx].row2 = v_zero();
    pool.riHP[idx].init(0);
    pool.riUniqueData[idx].cellId = pool.riUniqueData[idx].offset = -1;
    pool.riXYZR[idx] = v_perm_xyzd(v_zero(), v_splats(-1.f));
    pool.uuIdx.push_back(idx);
    if (idx < pool.tsNodeIdx.size())
      pool.tsNodeIdx[idx] = scene::INVALID_NODE;
  }
  if (pool.riTm.size() == pool.uuIdx.size())
  {
    pool.riTm.clear();
    pool.riHP.clear();
    pool.riUniqueData.clear();
    pool.riXYZR.clear();
    pool.uuIdx.clear();
    if (pool.tsNodeIdx.size())
      pool.tsNodeIdx.resize(0);
    v_bbox3_init_empty(pool.fullWabb);
  }
  return true;
}

void rendinst::delRIGenExtraFromCell(riex_handle_t id, int cell_Id, int offset)
{
  uint32_t resIdx = handle_to_ri_type(id);
  if (resIdx >= riExtra.size())
    return;

  RiExtraPool &pool = riExtra[resIdx];

  if (pool.destroyedRiIdx >= 0 && pool.destroyedRiIdx < riExtra.size())
  {
    RiExtraPool &destrPool = riExtra[pool.destroyedRiIdx];
    for (int instNo = 0; instNo < destrPool.riUniqueData.size(); ++instNo)
      if (destrPool.riUniqueData[instNo].cellId == cell_Id && destrPool.riUniqueData[instNo].offset == offset)
      {
        delRIGenExtra(make_handle(pool.destroyedRiIdx, instNo));
        break;
      }
  }

  uint32_t idx = handle_to_ri_inst(id);
  if (idx < pool.riUniqueData.size() && pool.riUniqueData[idx].cellId == cell_Id && pool.riUniqueData[idx].offset == offset)
  {
    delRIGenExtra(id);
  }
}

void rendinst::delRIGenExtraFromCell(const RendInstDesc &desc)
{
  if (!desc.isRiExtra())
    return;

  delRIGenExtraFromCell(make_handle(desc.pool, desc.pool), -(desc.cellIdx + 1), desc.offs);
}

bool rendinst::removeRIGenExtraFromGrid(riex_handle_t id)
{
  ScopedRIExtraWriteLock wr;
  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT_RETURN(res_idx < riExtra.size(), false);
  RiExtraPool &pool = riExtra[res_idx];
  if (pool.isValid(idx) && pool.collRes && pool.isInGrid(idx))
  {
    mat44f tm44;
    bbox3f wabb;
    v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);
    v_bbox3_init(wabb, tm44, pool.collBb);
    riExtraGrid.erase(id, pool.riXYZR[idx]);
    pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_or(pool.riXYZR[idx], V_CI_SIGN_MASK));
    riutil::world_version_inc(wabb);
    return true;
  }
  return false;
}

void rendinst::setGameClockForRIGenExtraDamage(const float *session_time)
{
  rendinst::RiExtraPool::session_time = session_time ? session_time : &riex_session_time_0;
}

bool rendinst::applyDamageRIGenExtra(riex_handle_t id, float dmg_pts, float *absorbed_dmg_impulse, bool ignore_damage_threshold)
{
  if (id == RIEX_HANDLE_NULL || dmg_pts < 0)
    return false;

  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT(res_idx < riExtra.size());
  RiExtraPool &pool = riExtra[res_idx];
  if (pool.immortal)
    return false;

  if (idx >= pool.riTm.size())
  {
    logerr("applyDamageRIGenExtra idx out of range: idx=%d, count=%d (res_idx=%d)", idx, pool.riTm.size(), res_idx);
    return false;
  }

  if (!ignore_damage_threshold && dmg_pts < pool.damageThreshold)
  {
    if (absorbed_dmg_impulse)
      *absorbed_dmg_impulse = dmg_pts;
    return false;
  }

  if (pool.riHP[idx].isInvincible())
    return false;

  float hp = pool.getHp(idx);
  /*
  debug("riex.dmg %d[%d]: hp=%.1f at t=%.1f, effHp=%.1f, dmg=%.2f thres=%.2f (%d %p)",
    res_idx, idx, pool.riHP[idx].healthPoints, pool.riHP[idx].lastDmgTime, hp, dmg_pts, pool.damageThreshold,
    pool.destroyedRiIdx, pool.destroyedPhysRes);
  */
  if (absorbed_dmg_impulse)
    *absorbed_dmg_impulse = min(hp, dmg_pts);
  if (hp > dmg_pts)
  {
    pool.riHP[idx].applyDamageNoClamp(dmg_pts, *pool.session_time);
    // debug("  -> hp=%.1f at t=%.1f", pool.riHP[idx].healthPoints, pool.riHP[idx].lastDmgTime);
  }
  else if (hp)
  {
    pool.riHP[idx].init(0);
    return true;
  }
  else if (pool.initialHP)
    return true; // we had initialHP, but our hp is zero now, return that we actually died previously.
  return false;
}
bool rendinst::damageRIGenExtra(riex_handle_t &id, float dmg_pts, mat44f *out_destr_tm /*=nullptr*/,
  float *absorbed_dmg_impulse /*=nullptr*/, int *out_destroyed_riex_offset /*=nullptr*/, DestrOptionFlags destroy_flags)
{
  if (id == RIEX_HANDLE_NULL)
    return false;
  if (rendinst::applyDamageRIGenExtra(id, dmg_pts, absorbed_dmg_impulse) || destroy_flags & DestrOptionFlag::ForceDestroy)
  {
    uint32_t res_idx = handle_to_ri_type(id);
    uint32_t idx = handle_to_ri_inst(id);
    G_ASSERT(res_idx < riExtra.size());
    RiExtraPool &pool = riExtra[res_idx];
    if (pool.destroyedRiIdx < 0 && !(destroy_flags & DestrOptionFlag::ForceDestroy))
      return false;

    if (idx >= pool.riTm.size())
    {
      logerr("damageRIGenExtra idx out of range: idx=%d, count=%d (res_idx=%d)", idx, pool.riTm.size(), res_idx);
      return false;
    }

    mat43f m43 = rendinst::getRIGenExtra43(id);
    if (out_destr_tm)
      v_mat43_transpose_to_mat44(*out_destr_tm, m43);

    if (pool.destroyedRiIdx < 0 || !(destroy_flags & DestrOptionFlag::AddDestroyedRi))
      delRIGenExtra(id);
    else
    {
      RiExtraPool::ElemUniqueData origUD = pool.riUniqueData[idx];
      delRIGenExtra(id);
      if (pool.scaleDebris)
      {
        vec4f riSize = v_sub(pool.lbb.bmax, pool.lbb.bmin);
        vec4f debrisSize = v_sub(riExtra[pool.destroyedRiIdx].lbb.bmax, riExtra[pool.destroyedRiIdx].lbb.bmin);
        vec4f dif = v_div(riSize, debrisSize);
        vec4f scale = v_perm_xyzd(v_max(v_splat_x(dif), v_splat_z(dif)), V_C_ONE);
        m43.row0 = v_mul(m43.row0, scale);
        m43.row1 = v_mul(m43.row1, scale);
        m43.row2 = v_mul(m43.row2, scale);
      }
      int nextOffs = origUD.offset + 16;
      id = rendinst::addRIGenExtra43(pool.destroyedRiIdx, false, m43, pool.destroyedColl, origUD.cellId, nextOffs);
      if (out_destroyed_riex_offset)
        *out_destroyed_riex_offset = nextOffs;
    }
    return true;
  }
  return false;
}

void rendinst::gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const BBox3 &box, bool read_lock)
{
  if (read_lock)
    rendinst::ccExtra.lockRead();
  {
    TIME_PROFILE_DEV(gather_riex_collidable_box);
    rigrid_find_in_box_by_bounding(riExtraGrid, v_ldu_bbox3(box), [&](RiGridObject object) {
      out_handles.push_back(object.handle);
      return false;
    });
  }
  if (read_lock)
    rendinst::ccExtra.unlockRead();

  if (!out_handles.empty())
    eastl::sort(out_handles.begin(), out_handles.end());
}

void rendinst::gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const TMatrix &tm, const BBox3 &box, bool read_lock)
{
  if (read_lock)
    rendinst::ccExtra.lockRead();
  {
    TIME_PROFILE_DEV(gather_riex_collidable_transformed_box);
    rigrid_find_in_transformed_box_by_bounding(riExtraGrid, tm, box, [&](RiGridObject object) {
      out_handles.push_back(object.handle);
      return false;
    });
  }
  if (read_lock)
    rendinst::ccExtra.unlockRead();

  if (!out_handles.empty())
    eastl::sort(out_handles.begin(), out_handles.end());
}


void rendinst::gatherRIGenExtraCollidableMin(riex_collidable_t &out_handles, bbox3f_cref box, float min_bsph_rad)
{
  ScopedRIExtraReadLock rd;
  TIME_PROFILE_DEV(gather_riex_collidable_box_min);

  rigrid_find_in_box_by_bounding(riExtraGrid, box, [&](RiGridObject object) {
    if (v_extract_w(object.getWBSph()) >= min_bsph_rad)
      out_handles.push_back(object.handle);
    return false;
  });
}

void rendinst::gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const Point3 &from, const Point3 &dir, float len,
  bool read_lock)
{
  if (read_lock)
    rendinst::ccExtra.lockRead();
  {
    TIME_PROFILE_DEV(gather_riex_collidable_ray);
    rigrid_find_ray_intersections(riExtraGrid, from, dir, len, [&](RiGridObject object) {
      out_handles.push_back(object.handle);
      return false;
    });
  }
  if (read_lock)
    rendinst::ccExtra.unlockRead();
  if (!out_handles.empty())
    eastl::sort(out_handles.begin(), out_handles.end());
}

void rendinst::addRiExtraRefs(DataBlock *b, const DataBlock *riConf, const char *name)
{
  if (!riConf && name && riConfig)
    riConf = riConfig->getBlockByNameEx("riExtra")->getBlockByName(name);
  if (!riConf)
    return;

  const char *next_res = riConf->getStr("nextRes", nullptr);
  if (next_res && *next_res)
  {
    b->setStr("refRendInst", next_res);
    b->setStr("refColl", String(128, "%s" RI_COLLISION_RES_SUFFIX, next_res));
  }
  next_res = riConf->getStr("physRes", nullptr);
  if (next_res && *next_res)
    b->setStr("refPhysObj", next_res);
  next_res = riConf->getStr("apexRes", nullptr);
  if (next_res && *next_res)
    b->setStr("refApex", next_res);
}
void rendinst::prepareRiExtraRefs(const DataBlock &_riConf)
{
  const DataBlock &riConf = *_riConf.getBlockByNameEx("riExtra");
  for (int i = 0; i < riConf.blockCount(); i++)
    if (DataBlock *b = gameres_rendinst_desc.getBlockByName(riConf.getBlock(i)->getBlockName()))
      addRiExtraRefs(b, riConf.getBlock(i), nullptr);
}

bool rayHitRiExtraInstance(const Point3 &from, const Point3 &dir, float len, rendinst::riex_handle_t handle,
  rendinst::RendInstDesc &ri_desc, const MaterialRayStrat &strategy)
{
  uint32_t res_idx = rendinst::handle_to_ri_type(handle);
  const rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (RendInstGenData *rgl = (pool.riPoolRef >= 0) ? rendinst::getRgLayer(pool.riPoolRefLayer) : nullptr)
  {
    const RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[pool.riPoolRef];
    if (strategy.shouldIgnoreRendinst(/*isPos*/ false, riProp.immortal, riProp.matId))
      return false;
  }
  int idx = rendinst::handle_to_ri_inst(handle);
  if (idx >= pool.riTm.size())
  {
    logerr("rayHitRiExtraInstance idx out of range: idx=%d, count=%d (res_idx=%d)", idx, pool.riTm.size(), res_idx);
    return false;
  }

  mat44f tm;
  v_mat43_transpose_to_mat44(tm, pool.riTm[idx]);

  CollisionResource *collRes = rendinst::riExtra[res_idx].collRes;

  int outMatId = PHYSMAT_INVALID;
  if (collRes->rayHit(tm, from, dir, len, strategy.rayMatId, outMatId))
  {
    if (strategy.shouldIgnoreRendinst(/*isPos*/ false, /* is_immortal */ false, outMatId))
      return false;
    ri_desc.setRiExtra();
    ri_desc.idx = idx;
    ri_desc.pool = res_idx;
    ri_desc.offs = 0;
    return true;
  }
  return false;
}

bool rendinst::rayHitRIGenExtraCollidable(const Point3 &from, const Point3 &dir, float len, rendinst::RendInstDesc &ri_desc,
  const MaterialRayStrat &strategy, float min_r)
{
  ri_desc.reset();
  ScopedRIExtraReadLock rd;
  RiGridObject ret = rigrid_find_ray_intersections(riExtraGrid, from, dir, len, [&](RiGridObject object) {
    if (v_extract_w(object.getWBSph()) < min_r)
      return false;
    return rayHitRiExtraInstance(from, dir, len, object.handle, ri_desc, strategy);
  });
  return ret != RIEX_HANDLE_NULL;
}

void rendinst::reapplyLodRanges()
{
  for (int id = 0; id < riExtra.size(); id++)
  {
    const DataBlock *ri_ovr = rendinst::ri_lod_ranges_ovr.getBlockByName(riExtraMap.getName(id));
    RenderableInstanceLodsResource *riRes = riExtra[id].res;

    int lod_cnt = riRes->lods.size();
    float max_lod_dist = lod_cnt ? riResLodRange(riRes, lod_cnt - 1, ri_ovr) : (RendInstGenData::renderResRequired ? 0.0f : 1e5f);

    // RiExtra does not support impostors. Need to skip impostor and transition lods.
    lod_cnt -= lod_cnt ? riExtra[id].hasTransitionLod + riExtra[id].posInst : 0;

    riExtra[id].distSqLOD[0] = lod_cnt > 1 ? riResLodRange(riRes, 0, ri_ovr) : max_lod_dist;
    if (lod_cnt > 0)
    {
      for (int l = 1; l < rendinst::RiExtraPool::MAX_LODS; l++)
        riExtra[id].distSqLOD[l] =
          l < lod_cnt ? (l < lod_cnt - 1 ? riResLodRange(riRes, l, ri_ovr) : max_lod_dist) : riExtra[id].distSqLOD[lod_cnt - 1] - 0.1f;
      for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
        riExtra[id].distSqLOD[l] *= riExtra[id].distSqLOD[l];
    }
    else
      for (int l = 0; l < rendinst::RiExtraPool::MAX_LODS; l++)
        riExtra[id].distSqLOD[l] = max_lod_dist * max_lod_dist;
  }
}

void rendinst::getRendinstLandclassData(TempRiLandclassVec &ri_landclasses)
{
  ri_landclasses.clear();

  for (int res_idx = 0, res_idx_end = riExtra.size(); res_idx < res_idx_end; res_idx++)
  {
    RiExtraPool &pool = riExtra[res_idx];

    if (pool.riLandclassCachedData.size() == 0)
      continue;

    if (!pool.res->lods.size())
      continue;

    int firstValidTmId = -1;
    for (int i = 0; i < pool.riTm.size(); i++)
      if (pool.isValid(i))
      {
        if (firstValidTmId != -1)
        {
          firstValidTmId = -1;
          break;
        }
        firstValidTmId = i;
      }

    if (firstValidTmId == -1) // only for unique RI landclasses
      continue;

    RendinstLandclassData landclassData;
    {
      mat44f riTm44, riInvTm44;
      v_mat43_transpose_to_mat44(riTm44, pool.riTm[firstValidTmId]);
      v_mat44_orthonormal_inverse43_to44(riInvTm44, riTm44);
      alignas(16) TMatrix riTm;
      v_mat_43ca_from_mat44(riTm.m[0], riInvTm44);

      landclassData.matrixInv = riTm;
      landclassData.bbox = pool.collBb;
      landclassData.radius = v_extract_w(pool.bsphXYZR);
    }

    RiExtraPool::RiLandclassCachedData &riLandclassCachedData = pool.riLandclassCachedData[0];
    landclassData.index = riLandclassCachedData.index;
    landclassData.mapping = riLandclassCachedData.mapping;

    ri_landclasses.push_back(landclassData);
  }
}

float rendinst::getMaxRendinstHeight() { return static_cast<float>(interlocked_relaxed_load(maxRiExtraHeight)); }

float rendinst::getMaxRendinstHeight(int variableId) { return ri_extra_max_height_get_max_height(variableId); }

bool rendinst::RendInstDesc::doesReflectedInstanceMatches(const rendinst::RendInstDesc &rhs) const
{
  if (!isRiExtra())
    return *this == rhs;
  // do a match for unique data here
  bool hasUniqueData = idx < rendinst::riExtra[pool].riUniqueData.size();
  bool cellMatches = hasUniqueData && -(rendinst::riExtra[pool].riUniqueData[idx].cellId + 1) == rhs.cellIdx;
  bool offsMatches = hasUniqueData && rendinst::riExtra[pool].riUniqueData[idx].offset == rhs.offs;
  return pool == rhs.pool && ((cellMatches && offsMatches) || (idx == rhs.idx && layer == rhs.layer));
}

bool rendinst::RendInstDesc::isDynamicRiExtra() const { return isRiExtra() && rendinst::riExtra[pool].isDynamicRendinst; }

rendinst::riex_handle_t rendinst::RendInstDesc::getRiExtraHandle() const
{
  return (pool >= 0 && cellIdx < 0) ? make_handle(pool, idx) : RIEX_HANDLE_NULL;
}
uint32_t rendinst::getRiGenExtraResCount() { return riExtra.size(); }
int rendinst::getRiGenExtraInstances(Tab<rendinst::riex_handle_t> &out_handles, uint32_t res_idx)
{
  out_handles.clear();
  ScopedRIExtraReadLock rd;
  G_ASSERT_RETURN(res_idx < riExtra.size(), 0);
  RiExtraPool &pool = riExtra[res_idx];
  if (int cnt = pool.riTm.size() - pool.uuIdx.size())
  {
    out_handles.reserve(cnt);
    for (int i = 0; i < pool.riTm.size(); i++)
      if (pool.isValid(i))
      {
        out_handles.push_back(make_handle(res_idx, i));
        if (out_handles.size() == cnt)
          break;
      }
  }
  return out_handles.size();
}
int rendinst::getRiGenExtraInstances(Tab<riex_handle_t> &out_handles, uint32_t res_idx, const bbox3f &box)
{
  BBox3 bb;
  float radius = abs(getRIGenExtraBsphRad(res_idx));
  const Point3 delta(radius, radius, radius);
  v_stu_bbox3(bb, box);
  bb.lim[0] -= delta;
  bb.lim[1] += delta;

  out_handles.clear();
  ScopedRIExtraReadLock rd;
  G_ASSERT_RETURN(res_idx < riExtra.size(), 0);
  RiExtraPool &pool = riExtra[res_idx];
  if (int cnt = pool.riTm.size() - pool.uuIdx.size())
  {
    out_handles.reserve(cnt);
    for (int i = 0; i < pool.riTm.size(); i++)
      if (pool.isValid(i))
      {
        const auto &ri_handle = make_handle(res_idx, i);
        vec4f bsph = rendinst::getRIGenExtraBSphere(ri_handle);
        const float *bsph_data = (const float *)&bsph;
        const Point3 pos(bsph_data[0], bsph_data[1], bsph_data[2]);

        if (pos.x >= bb.lim[0].x && pos.x <= bb.lim[1].x && pos.y >= bb.lim[0].y && pos.y <= bb.lim[1].y && pos.z >= bb.lim[0].z &&
            pos.z <= bb.lim[1].z)
        {
          out_handles.push_back(ri_handle);
          if (out_handles.size() == cnt)
            break;
        }
      }
  }
  return out_handles.size();
}

rendinst::RendInstDesc::RendInstDesc(rendinst::riex_handle_t handle) :
  cellIdx(-1), idx(rendinst::handle_to_ri_inst(handle)), pool(rendinst::handle_to_ri_type(handle)), offs(0), layer(0)
{}

bool rendinst::isRiGenExtraValid(riex_handle_t id)
{
  const rendinst::RiExtraPool *pool = safe_at(riExtra, handle_to_ri_type(id));
  return pool && pool->isValid(handle_to_ri_inst(id));
}

void rendinst::registerRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb)
{
  if (find_value_idx(invalidate_handle_callbacks, cb) == -1)
    invalidate_handle_callbacks.push_back(cb);
}

bool rendinst::unregisterRIGenExtraInvalidateHandleCb(invalidate_handle_cb cb)
{
  return erase_item_by_value(invalidate_handle_callbacks, cb);
}

void rendinst::registerRiExtraDestructionCb(ri_destruction_cb cb)
{
  if (find_value_idx(riex_destruction_callbacks, cb) == -1)
    riex_destruction_callbacks.push_back(cb);
}

bool rendinst::unregisterRiExtraDestructionCb(ri_destruction_cb cb) { return erase_item_by_value(riex_destruction_callbacks, cb); }

void rendinst::onRiExtraDestruction(riex_handle_t id, bool is_dynamic, int32_t user_data)
{
  for (auto cb : riex_destruction_callbacks)
    cb(id, is_dynamic, user_data);
}

dag::ConstSpan<mat43f> rendinst::riex_get_instance_matrices(int res_idx)
{
  return res_idx < riExtra.size() ? make_span_const(riExtra[res_idx].riTm) : dag::ConstSpan<mat43f>();
}

bool rendinst::riex_is_instance_valid(const mat43f &ri_tm)
{
  uint64_t *p = (uint64_t *)&ri_tm;
  return p[0] || p[1];
}

void rendinst::riex_lock_write(const char *) { rendinst::ccExtra.lockWrite(); }
void rendinst::riex_unlock_write() { rendinst::ccExtra.unlockWrite(); }
void rendinst::riex_lock_read() { rendinst::ccExtra.lockRead(); }
void rendinst::riex_unlock_read() { rendinst::ccExtra.unlockRead(); }

const uint32_t *rendinst::get_user_data(rendinst::riex_handle_t handle)
{
  uint32_t resIdx = handle_to_ri_type(handle);
  RiExtraPool &pool = riExtra[resIdx];
  uint32_t idx = handle_to_ri_inst(handle);
  scene::node_index nodeIdx = pool.getNodeIdx(idx);
  if (nodeIdx == scene::INVALID_NODE)
    return nullptr;
  return (uint32_t *)riExTiledScenes[pool.tsIndex].getUserData(nodeIdx);
}

void rendinst::hideRIGenExtraNotCollidable(const BBox3 &_box, bool hide_immortals)
{
  bbox3f box = v_ldu_bbox3(_box);
  riex_pos_and_ri_tab_t posAndInfo;
  rendinst::gatherRIGenExtraRenderableNotCollidable(posAndInfo, box, true, rendinst::SceneSelection::All);
  for (const pos_and_render_info_t &pi : posAndInfo)
  {
    rendinst::riex_render_info_t h = pi.info;
    int s = h >> 30;
    int n = h & 0x3FFFFFFF;
    if (!hide_immortals)
    {
      int poolId = riExTiledScenes[s].getNodePool(n);
      if (poolId < rendinst::riExtra.size() && rendinst::riExtra[poolId].immortal)
        continue;
    }

    mat44f m;
    m.col0 = v_zero();
    m.col1 = v_zero();
    m.col2 = v_zero();
    m.col3 = v_perm_xyzd(v_ldu(&pi.pos.x), V_C_ONE);
    riExTiledScenes[s].setTransform(n, m);
  }
}

rendinst::HasRIClipmap rendinst::hasRIClipmapPools()
{
  if (hasRiClipmap == rendinst::HasRIClipmap::UNKNOWN)
  {
    for (const auto &ri : riExtra)
      if (ri.isRendinstClipmap)
        return hasRiClipmap = rendinst::HasRIClipmap::YES;
    return hasRiClipmap = rendinst::HasRIClipmap::NO;
  }
  return hasRiClipmap;
}

bool rendinst::hasRIExtraOnLayers(const RiGenVisibility *visibility, LayerFlags layer_flags)
{
  if (!visibility->riex.riexInstCount)
    return false;

  for (int layer = 0; layer < RIEX_STAGE_COUNT; ++layer)
  {
    if (!(layer_flags & static_cast<LayerFlag>(1 << layer)))
      continue;

    if (riExPoolIdxPerStage[layer].size() > 0)
      return true;
  }

  return false;
}

static void riPerDrawData_afterDeviceReset(bool /*full_reset*/)
{
  if (!RendInstGenData::renderResRequired)
    return;
  for (uint32_t i = 0; i < rendinst::riExtra.size(); ++i)
    rendinst::render::update_per_draw_gathered_data(i);
}

REGISTER_D3D_AFTER_RESET_FUNC(riPerDrawData_afterDeviceReset);

void rendinst::rigrid_debug_pos(const Point3 &pos, const Point3 &camera_pos) { rigrid_debug_pos(riExtraGrid, pos, camera_pos); }

static bool rigrid_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("rigrid", "stats", 1, 1) { riExtraGrid.printStats(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(rigrid_console_handler);
