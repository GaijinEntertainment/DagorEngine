#include <rendInst/rendInstGen.h>
#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "riGen/riGenRender.h"
#include "riGen/riUtil.h"
#include "riGen/riGenExtraRender.h"
#include "riGen/riGenExtraMaxHeight.h"
#include "riGen/riGrid.h"

#include <generic/dag_sort.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <gameRes/dag_stdGameRes.h>
#include <gameRes/dag_collisionResource.h>
#include <shaders/dag_shaderResUnitedData.h>
#include <3d/dag_ringDynBuf.h>
#include <3d/dag_drv3dReset.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_adjpow2.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_convar.h>
#include <util/dag_hash.h>
#include <math/dag_vecMathCompatibility.h>
#include <math/dag_mathUtils.h>
#include <gpuObjects/gpuObjects.h>
#include <perfMon/dag_perfTimer.h>


#if DAGOR_DBGLEVEL > 0
static const int LOGMESSAGE_LEVEL = LOGLEVEL_ERR;
#else
static const int LOGMESSAGE_LEVEL = LOGLEVEL_WARN;
#endif

static const DataBlock *riConfig = NULL;
rendinst::RiExtraPoolsVec rendinst::riExtra;
uint32_t rendinst::RiExtraPoolsVec::size_interlocked() const { return interlocked_acquire_load(*(const volatile int *)&mCount); }
uint32_t rendinst::RiExtraPoolsVec::interlocked_increment_size() { return interlocked_increment(*(volatile int *)&mCount); }
static rendinst::HasRIClipmap hasRiClipmap = rendinst::HasRIClipmap::UNKNOWN;
FastNameMap rendinst::riExtraMap;

CallbackToken rendinst::meshRElemsUpdatedCbToken = {};

void (*rendinst::shadow_invalidate_cb)(const BBox3 &box) = nullptr;
BBox3 (*rendinst::get_shadows_bbox_cb)() = nullptr;

SmartReadWriteFifoLock rendinst::ccExtra;
struct ScopedRIExtraWriteLock
{
  ScopedRIExtraWriteLock() { rendinst::ccExtra.lockWrite(); }
  ~ScopedRIExtraWriteLock() { rendinst::ccExtra.unlockWrite(); }
};
struct ScopedRIExtraWriteTryLock
{
  ScopedRIExtraWriteTryLock(bool do_not_wait = false)
  {
    if (do_not_wait)
      locked = rendinst::ccExtra.tryLockWrite();
    else
    {
      rendinst::ccExtra.lockWrite();
      locked = true;
    }
  }
  ~ScopedRIExtraWriteTryLock()
  {
    if (isLocked())
      rendinst::ccExtra.unlockWrite();
  }
  const bool isLocked() { return locked; }

protected:
  bool locked = false;
};

int rendinst::maxExtraRiCount = 0;
int rendinst::maxRenderedRiEx[countof(rendinst::vbExtraCtx)];
int rendinst::perDrawInstanceDataBufferType = 0; // Shaders support only: 1 - Buffer, 2 - StructuredBuffer; otherwise support is either
                                                 // flexible or undefined
int rendinst::instancePositionsBufferType = 0;
unsigned rendinst::RiExtraPool::defLodLimits = 0xF0F0F0F0;
static const float riex_session_time_0 = 0;
const float *rendinst::RiExtraPool::session_time = &riex_session_time_0;
Tab<uint16_t> rendinst::riExPoolIdxPerStage[RIEX_STAGE_COUNT];
float rendinst::half_minimal_grid_cell_size = 32.;
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

static StaticTab<RiGrid, 8> riExtraGrid;
static const float MAX_GRID_CELL_SIZE = 8192;

static void repopulate_ri_extra_grid();

static void init_ri_extra_grids(const DataBlock *level_blk)
{
  bool use_locks = false;
  if (const DataBlock *riExGrid = ::dgs_get_game_params()->getBlockByName("riExtraGrids"))
  {
    riExtraGrid.resize(riExGrid->getInt("gridCount", 0));
    for (int i = 0; i < riExtraGrid.size(); i++)
      riExtraGrid[i].init(riExGrid->getInt(String(0, "grid%dcell", i + 1), 32 << (i * 2)), use_locks);
  }
  else
  {
    riExtraGrid.resize(3);
    riExtraGrid[0].init(64, use_locks);
    riExtraGrid[1].init(256, use_locks);
    riExtraGrid[2].init(1024, use_locks);
  }
  debug("init_ri_extra_grids(%d)", riExtraGrid.size());
  rendinst::half_minimal_grid_cell_size = riExtraGrid.size() ? 0.5f * riExtraGrid[0].getCellSize() : 32;
  for (int i = 0; i < riExtraGrid.size(); i++)
  {
    rendinst::half_minimal_grid_cell_size = min(rendinst::half_minimal_grid_cell_size, 0.5f * riExtraGrid[i].getCellSize());
    debug("  riGrid[%d] cell=%.0f %s", i, riExtraGrid[i].getCellSize(), riExtraGrid[i].hasLocks() ? "locks" : "");
  }
  repopulate_ri_extra_grid();

  rendinst::init_tiled_scenes(level_blk);
}

static void term_ri_extra_grids()
{
  // for (int i = 0; i < riExtraGrid.size(); i ++)
  //{
  //   debug("--- riExtra grid %d", i);
  //   riExtraGrid[i].dumpGridState();
  // }
  rendinst::term_tiled_scenes();
  if (riExtraGrid.size())
    debug("term_ri_extra_grids(%d)", riExtraGrid.size());
  riExtraGrid.resize(0);
  for (int i = 0; i < riExtraGrid.static_size; i++)
  {
    riExtraGrid.data()[i].term();
  }
}
static float select_grid_cell_size(float rad_x2)
{
  if (rad_x2 < 32)
    return 32;
  G_ASSERTF(rad_x2 < MAX_GRID_CELL_SIZE, "rad_x2=%.0f", rad_x2);
  if (rad_x2 > MAX_GRID_CELL_SIZE)
    return MAX_GRID_CELL_SIZE;
  return (float)get_bigger_pow2(int(floorf(rad_x2)));
}
static int select_grid_idx(float rad, bool can_add = true)
{
  G_ASSERTF(rad > 0, "rad=%.2f", rad);
  float rad_x2 = floorf(rad * 2);
  for (int i = 0; i < riExtraGrid.size(); i++)
    if (rad_x2 <= riExtraGrid[i].getCellSize() || riExtraGrid[i].getCellSize() >= MAX_GRID_CELL_SIZE)
      return i;
  if (riExtraGrid.size() < riExtraGrid.static_size && can_add)
  {
    static WinCritSec cc("addGridLayer");

    cc.lock();
    // re-check after lock
    for (int i = 0; i < riExtraGrid.size(); i++)
      if (rad_x2 <= riExtraGrid[i].getCellSize() || riExtraGrid[i].getCellSize() >= MAX_GRID_CELL_SIZE)
      {
        cc.unlock();
        return i;
      }

    bool use_locks = false;
    int idx = riExtraGrid.size();
    riExtraGrid.push_back().init(select_grid_cell_size(rad_x2), use_locks);
    debug("added riGrid[%d] cell=%.0f %s (for rad=%.1f)", idx, riExtraGrid[idx].getCellSize(),
      riExtraGrid[idx].hasLocks() ? "locks" : "", rad);
    cc.unlock();
    return idx;
  }
  logerr("%s for rad=%.1f, riExtraGrid.size()=%d", can_add ? "no more grids" : "can't add grid", rad, riExtraGrid.size());
  return -1;
}
static inline int reselect_grid_idx(int grid_idx, float real_rad, bool can_add)
{
  if (grid_idx >= 0 && real_rad * 2 < riExtraGrid[grid_idx].getCellSize())
    return grid_idx;
  return select_grid_idx(real_rad, can_add);
}
static vec4f make_pos_and_rad(mat44f_cref tm, vec4f center_and_rad)
{
  vec4f pos = v_mat44_mul_vec3p(tm, center_and_rad);
  vec4f maxScale = v_max(v_length3_est(tm.col0), v_max(v_length3_est(tm.col1), v_length3_est(tm.col2)));
  vec4f bb_ext = v_mul(v_splat_w(center_and_rad), maxScale);
  return v_perm_xyzd(pos, bb_ext);
}

static void repopulate_ri_extra_grid()
{
  using rendinst::riExtra;
  using rendinst::RiExtraPool;

  int repopulated_cnt = 0;
  bbox3f accum_box;
  v_bbox3_init_empty(accum_box);
  for (int res_idx = 0; res_idx < riExtra.size(); res_idx++)
  {
    RiExtraPool &pool = riExtra[res_idx];
    if (pool.gridIdx < 0)
      continue;
    for (int idx = 0; idx < pool.riXYZR.size(); idx++)
    {
      if (!pool.isValid(idx))
        continue;
      int grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), true);
      if (grid_idx >= 0)
      {
        mat44f tm44;
        v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);

        bbox3f wabb;
        v_bbox3_init(wabb, tm44, pool.collBb);

        riExtraGrid[grid_idx].insert(rendinst::make_handle(res_idx, idx), *(Point3 *)&pool.riXYZR[idx], wabb);
        repopulated_cnt++;
        v_bbox3_add_box(accum_box, wabb);
      }
    }
  }
  if (repopulated_cnt > 0)
  {
    riutil::world_version_inc(accum_box);
    logwarn("init_ri_extra_grids: re-populated %d instances", repopulated_cnt);
  }
}

void rendinst::update_per_draw_gathered_data(uint32_t id)
{
  rendinstgenrender::RiShaderConstBuffers perDrawGatheredData;
  perDrawGatheredData.setBBoxNoChange();
  perDrawGatheredData.setOpacity(0, 1);
  perDrawGatheredData.setRandomColors(rendinst::riExtra[id].poolColors);
  perDrawGatheredData.setInstancing(0, 4, 0);
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
  rendinst::perDrawData->updateData(id * sizeof(rendinstgenrender::RiShaderConstBuffers), sizeof(perDrawGatheredData),
    &perDrawGatheredData, 0);
}

void rendinst::allocateRIGenExtra(VbExtraCtx &vbctx)
{
  ScopedRIExtraWriteLock wr;
  if (!vbctx.vb)
  {
    vbctx.vb = new RingDynamicSB;
    bool useStructuredBind;
    if (instancePositionsBufferType == 1)
    {
      // The support for useStructuredBind depends on shader, and that in turn depends on the ability to coexist with exported RI
      // positions format, which is half4.
      useStructuredBind = false;
      if (d3d::get_driver_desc().issues.hasSmallSampledBuffers)
        maxExtraRiCount = min<int>(maxExtraRiCount, 65536 / RIEXTRA_VECS_COUNT); // The minimum guaranteed supported Buffer on Vulkan.
    }
    else if (instancePositionsBufferType == 2)
      useStructuredBind = true;
    else
      useStructuredBind = d3d::get_driver_desc().issues.hasSmallSampledBuffers;

    char vbName[] = "RIGz_extra0";
    G_FAST_ASSERT(&vbctx - &vbExtraCtx[0] < countof(vbExtraCtx));
    vbName[sizeof(vbName) - 2] += &vbctx - &vbExtraCtx[0]; // RIGz_extra0 -> RIGz_extra{0,1}
    vbctx.vb->init(RIEXTRA_VECS_COUNT * maxExtraRiCount, sizeof(vec4f), sizeof(vec4f),
      SBCF_BIND_SHADER_RES | (useStructuredBind ? SBCF_MISC_STRUCTURED : 0), useStructuredBind ? 0 : TEXFMT_A32B32G32R32F, vbName);
    vbctx.gen++;
  }
}

void rendinst::initRIGenExtra(bool need_render, const DataBlock *level_blk)
{
  init_ri_extra_grids(level_blk);
  if (!need_render)
    return;
  allocateRIGenExtra(vbExtraCtx[0]);
}

namespace rendinst
{
static void termElems();
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
  termElems();
  for (auto &ve : vbExtraCtx)
  {
    del_it(ve.vb);
    ve.gen = INVALID_VB_EXTRA_GEN + 1;
  }
  if (gpu_objects_mgr)
    gpu_objects_mgr->clearObjects();
}

void rendinst::RiExtraPool::setWasNotSavedToElems()
{
  wasSavedToElems = 0;
  interlocked_increment(rendinst::pendingRebuildCnt);
}

rendinst::RiExtraPool::~RiExtraPool()
{
  if (clonedFromIdx != -1) // does not own resources
  {
    res = NULL;
    collRes = NULL;
    destroyedPhysRes = NULL;
    return;
  }
  if (res)
  {
    res->delRef();
    res = NULL;
  }
  if (collRes)
  {
    if (unregCollCb)
      unregCollCb(collHandle);
    release_game_resource((GameResource *)collRes);
    collRes = NULL;
  }
  if (destroyedPhysRes)
  {
    release_game_resource((GameResource *)destroyedPhysRes);
    destroyedPhysRes = NULL;
  }
}


void rendinst::RiExtraPool::validateLodLimits()
{
  int maxl = min<int>(res->lods.size(), MAX_LODS) - 1;

  if (isPosInst() && maxl)
    maxl--; //== skip last LOD for posInst since imposters not supported for riExtra

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
  return riConfig->getBlockByNameEx("riExtra")->getBlockByName(nm) != NULL;
}

static inline float riResLodRange(RenderableInstanceLodsResource *riRes, int lod, const DataBlock *ri_ovr)
{
  static const char *lod_nm[] = {"lod0", "lod1", "lod2", "lod3"};
  if (lod < riRes->getQlMinAllowedLod())
    return -1; // lod = riRes->getQlMinAllowedLod();

  if (!ri_ovr || lod > 3)
    return riRes->lods[lod].range;
  return ri_ovr->getReal(lod_nm[lod], riRes->lods[lod].range);
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

static void push_res_idx_to_stage(uint32_t res_layers, int res_idx)
{
  ScopedRIExtraWriteLock wr;
  if (res_layers & rendinst::LAYER_OPAQUE)
    rendinst::riExPoolIdxPerStage[get_const_log2(rendinst::LAYER_OPAQUE)].push_back(res_idx);
  if (res_layers & rendinst::LAYER_TRANSPARENT)
    rendinst::riExPoolIdxPerStage[get_const_log2(rendinst::LAYER_TRANSPARENT)].push_back(res_idx);
  if (res_layers & rendinst::LAYER_DECALS)
    rendinst::riExPoolIdxPerStage[get_const_log2(rendinst::LAYER_DECALS)].push_back(res_idx);
  if (res_layers & rendinst::LAYER_DISTORTION)
    rendinst::riExPoolIdxPerStage[get_const_log2(rendinst::LAYER_DISTORTION)].push_back(res_idx);
}

static void erase_res_idx_from_stage(uint32_t res_layers, int res_idx)
{
  ScopedRIExtraWriteLock wr;
  for (int i = 0; i < countof(rendinst::riExPoolIdxPerStage); i++)
  {
    if (!(res_layers & (1 << i)))
      continue;
    uint16_t *it = eastl::find(rendinst::riExPoolIdxPerStage[i].begin(), rendinst::riExPoolIdxPerStage[i].end(), res_idx);
    G_ASSERT(it != rendinst::riExPoolIdxPerStage[i].end());
    erase_items_fast(rendinst::riExPoolIdxPerStage[i], it - rendinst::riExPoolIdxPerStage[i].begin(), 1);
  }
}

static void init_relems_for_new_pool(int new_res_idx)
{
  rendinst::on_ri_mesh_relems_updated_pool(new_res_idx);

  if (RendInstGenData::renderResRequired)
    rendinst::update_per_draw_gathered_data(new_res_idx);
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

  if (!vbExtraCtx[0].vb && RendInstGenData::renderResRequired && maxExtraRiCount)
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

  if (const DataBlock *b = riConfig && !immortal ? riConfig->getBlockByNameEx("riExtra")->getBlockByName(ri_res_name) : NULL)
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
    const char *next_res = b->getStr("nextRes", NULL);
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

    next_res = b->getStr("physRes", NULL);
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
    const DataBlock *debrisBlk = NULL;
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
        const char *debrisRes = debrisBlk->getStr("debris", NULL);
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

  CollisionResource *collRes = NULL;
  String coll_name(128, "%s" RI_COLLISION_RES_SUFFIX, ri_res_name);
  collRes = get_resource_type_id(coll_name) == CollisionGameResClassId
              ? (CollisionResource *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(coll_name), CollisionGameResClassId)
              : NULL;
  if (collRes && collRes->getAllNodes().empty())
  {
    release_game_resource((GameResource *)collRes);
    collRes = NULL;
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
  riExtra[id].gridIdx = collRes ? select_grid_idx(riExtra[id].bsphRad()) : -1;
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
  if (riExtra[id].isPosInst() && lod_cnt)
    lod_cnt--; //== skip last LOD for posInst since imposters not supported for riExtra

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

    uint32_t layers = 0;
    riExtra[id].hasColoredShaders = riExtra[id].isPosInst();
    riExtra[id].isRendinstClipmap = 0;
    riExtra[id].isTessellated = 0;
    riExtra[id].isPaintFxOnHit = params_block.getBool("isPaintFxOnHit", true);
    for (unsigned int lodNo = 0; lodNo < lod_cnt; lodNo++)
    {
      ShaderMesh *m = riRes->lods[lodNo].scene->getMesh()->getMesh()->getMesh();
      uint32_t mask = 0;
      uint32_t cmask = 0;
      G_STATIC_ASSERT(sizeof(mask) == sizeof(riExtra[id].elemMask[0].atest));
      G_STATIC_ASSERT(sizeof(riExtra[id].elemMask[0].atest) == sizeof(riExtra[id].elemMask[0].cullN));
      dag::Span<ShaderMesh::RElem> elems = m->getElems(m->STG_opaque, m->STG_decal);
      G_ASSERTF(elems.size() <= sizeof(mask) * 8, "elems.size()=%d mask=%d bits", elems.size(), sizeof(mask) * 8);
      if (m->getElems(m->STG_opaque, m->STG_imm_decal).size())
        layers |= rendinst::LAYER_OPAQUE;
      if (m->getElems(m->STG_trans).size())
        layers |= rendinst::LAYER_TRANSPARENT;
      if (m->getElems(m->STG_decal).size())
        layers |= rendinst::LAYER_DECALS;
      if (m->getElems(m->STG_distortion).size())
        layers |= rendinst::LAYER_DISTORTION;

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
          riExtra[id].usingClipmap |= (strstr(className, "rendinst_layered_hmap_vertex_blend") != NULL);
          riExtra[id].hasDynamicDisplacement |= strstr(className, "rendinst_flag_colored") != NULL ||
                                                strstr(className, "rendinst_flag_layered") != NULL ||
                                                strstr(className, "rendinst_anomaly") != NULL;
          riExtra[id].patchesHeightmap |= strstr(className, "rendinst_heightmap_patch") != NULL;
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
          riExtra[id].isTessellated |=
            elems[elemNo].mat->getIntVariable(material_pn_triangulation_variable_id, isTessellated) && isTessellated;
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
    riExtra[id].gridIdx = collRes ? select_grid_idx(riExtra[id].bsphRad()) : -1;
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
    riExtraPool.res = NULL;
  }
  if (riExtraPool.collRes)
  {
    if (unregCollCb)
      unregCollCb(riExtraPool.collHandle);
    release_game_resource((GameResource *)riExtraPool.collRes);
    riExtraPool.collRes = NULL;
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
    return NULL;
  return riExtra[res_idx].res;
}
CollisionResource *rendinst::getRIGenExtraCollRes(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return NULL;
  return riExtra[res_idx].collRes;
}
const bbox3f *rendinst::getRIGenExtraCollBb(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return NULL;
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

namespace rendinst
{
dag::Vector<RenderElem> allElems;
dag::Vector<uint32_t> allElemsIndex;
uint32_t oldPoolsCount = 0;
int pendingRebuildCnt = 0;
bool relemsForGpuObjectsHasRebuilded = true;
static void termElems()
{
  allElems = {};
  allElemsIndex = {};
  oldPoolsCount = 0;
  pendingRebuildCnt = 0;
}
} // namespace rendinst

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

  dag::Vector<BBox3> invalidBoxes = rendinst::getRIGenExtraInstancesWorldBboxesByGrid(poolI, gridOrigin, gridSize, gridCells);

  // shadowsInvalidateGatheredBBoxes has n^2 complexity, so fallback to less resolution if we gathered too many boxes.
  if (invalidBoxes.size() > MAX_BOXES_FOR_INVALIDATION)
  {
    invalidBoxes = rendinst::getRIGenExtraInstancesWorldBboxesByGrid(poolI, gridOrigin, gridSize,
      IPoint2(MAX_BOXES_FOR_INVALIDATION_SQRT, MAX_BOXES_FOR_INVALIDATION_SQRT));
  }

  for (const BBox3 &box : invalidBoxes)
    rendinst::shadow_invalidate_cb(box);
}

void rendinst::on_ri_mesh_relems_updated_pool(uint32_t poolI)
{
  riExtra[poolI].setWasNotSavedToElems();

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

void rendinst::updateShaderElems(uint32_t poolI)
{
  auto &riPool = riExtra[poolI];
  for (int l = 0; l < riPool.res->lods.size(); ++l)
  {
    RenderableInstanceResource *rendInstRes = riPool.res->lods[l].scene;
    ShaderMesh *mesh = rendInstRes->getMesh()->getMesh()->getMesh();
    mesh->updateShaderElems();
  }
}

void rendinst::on_ri_mesh_relems_updated(const RenderableInstanceLodsResource *r)
{
  ScopedRIExtraReadLock wr;
  for (int poolI = 0, poolE = riExtra.size_interlocked(); poolI < poolE; ++poolI) // fixme: linear search inside 2k array! use Hashmap
    if (riExtra[poolI].res == r)
    {
      on_ri_mesh_relems_updated_pool(poolI);
      // break; More than one pool can have the same res. This happens in case of clonned ri
    }
}

void rendinst::rebuildAllElemsInternal()
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
  for (int r = toRebuild, i = 0, e = newPoolsCount; r && i != e; ++i) // todo: probably we should use SOA of bools for wasSavedToElems.
                                                                      // better locality.
  {
    if (!rendinst::riExtra[i].wasSavedToElems)
    {
      toUpdate.push_back(i);
      riExtra[i].wasSavedToElems = 1;
      --r;
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
            int drawOrder = 0;
            elem.mat->getIntVariable(drawOrderVarId, drawOrder);
            const int vbIdx = elem.vertexData->getVbIdx();
            G_ASSERT(vbIdx <= eastl::numeric_limits<uint8_t>::max());
            allElems.emplace_back(RenderElem{(ShaderElement *)elem.e, (short)elem.vertexData->getStride(), (uint8_t)vbIdx,
              uint8_t(sign(drawOrder) + 1), elem.si, elem.numf, elem.baseVertex});
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

  rendinst::relemsForGpuObjectsHasRebuilded = false;
}

void rendinst::reinitOnShadersReload()
{
  int riExtraCount = rendinst::riExtra.size();
  if (!riExtraCount)
    return;
  for (int i = 0; i < riExtraCount; ++i)
  {
    updateShaderElems(i);
    riExtra[i].wasSavedToElems = 0;
  }
  interlocked_release_store(pendingRebuildCnt, riExtraCount);
  rebuildAllElemsInternal();
}

riex_handle_t rendinst::addRIGenExtra43(int res_idx, bool /*m*/, mat43f_cref tm, bool has_collision, int orig_cell, int orig_offset,
  int add_data_dwords, const int32_t *add_data)
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
    int grid_idx = -1;
    if (has_collision && pool.gridIdx >= 0)
    {
      grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), true);
      if (grid_idx >= 0)
        riExtraGrid[grid_idx].insert(h, *(Point3 *)&pool.riXYZR[idx], wabb);
      riutil::world_version_inc(wabb);

      update_max_ri_extra_height(static_cast<int>(v_extract_y(wabb.bmax) - v_extract_y(wabb.bmin)) + 1);
    }
    if (grid_idx < 0)
      pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_neg(pool.riXYZR[idx]));

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
  int add_data_dwords, const int32_t *add_data)
{
  if (res_idx < 0 || res_idx > riExtra.size())
    return RIEX_HANDLE_NULL;
  mat43f m43;
  v_mat44_transpose_to_mat43(m43, tm);
  return addRIGenExtra43(res_idx, movable, m43, has_collision, orig_cell, orig_offset, add_data_dwords, add_data);
}

const mat43f &rendinst::getRIGenExtra43(riex_handle_t id)
{
  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  G_ASSERT(res_idx < riExtra.size());
  RiExtraPool &pool = riExtra[res_idx];
  if (idx >= pool.riTm.size())
  {
    logerr("%s idx out of range: idx=%d, count=%d (res_idx=%d)", __FUNCTION__, idx, pool.riTm.size(), res_idx);
    static mat43f m43;
    m43.row0 = m43.row1 = m43.row2 = v_zero();
    return m43;
  }
  return pool.riTm[idx];
}
namespace rendinst
{
void getRIGenExtra44NoLock(riex_handle_t id, mat44f &out_tm) { v_mat43_transpose_to_mat44(out_tm, getRIGenExtra43(id)); }
void getRIGenExtra44(riex_handle_t id, mat44f &out_tm)
{
  ScopedRIExtraReadLock rd;
  getRIGenExtra44NoLock(id, out_tm);
}
} // namespace rendinst

void rendinst::getRIExtraCollInfo(riex_handle_t id, CollisionResource **out_collision, BSphere3 *out_bsphere)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT(resIdx < riExtra.size());

  if (out_collision)
    *out_collision = riExtra[resIdx].collRes;

  if (out_bsphere)
  {
    const RiExtraPool &pool = riExtra[resIdx];
    *out_bsphere = BSphere3(pool.res->bsphCenter, pool.res->bsphRad);
  }
}

const CollisionResource *rendinst::getDestroyedRIExtraCollInfo(riex_handle_t handle)
{
  const auto resIdx = handle_to_ri_type(handle);
  G_ASSERT_RETURN(resIdx < riex_max_type(), nullptr);

  if (riExtra[resIdx].destroyedRiIdx < 0)
    return nullptr;

  return riExtra[riExtra[resIdx].destroyedRiIdx].collRes;
}


float rendinst::getInitialHP(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT(resIdx < riExtra.size());
  return riExtra[resIdx].initialHP;
}

float rendinst::getHP(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT(resIdx < riExtra.size());
  return riExtra[resIdx].getHp(handle_to_ri_inst(id));
}

bool rendinst::isPaintFxOnHit(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT(resIdx < riExtra.size());
  return riExtra[resIdx].isPaintFxOnHit;
}

bool rendinst::isKillsNearEffects(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT(resIdx < riExtra.size());
  return riExtra[resIdx].killsNearEffects;
}

void rendinst::setRIGenExtraImmortal(uint32_t pool, bool value)
{
  if (pool >= riExtra.size())
  {
    logerr("failed to set 'immortal' property of invalid pool '%ull'", pool);
    return;
  }
  riExtra[pool].immortal = value;
}

bool rendinst::isRIGenExtraImmortal(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].immortal;

  logerr("failed to get 'immortal' property of invalid pool '%ull'", pool);
  return true;
}

bool rendinst::isRIGenExtraWalls(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].isWalls;

  logerr("failed to get 'isWalls' property of invalid pool '%ull'", pool);
  return true;
}

float rendinst::getRIGenExtraBsphRad(uint32_t pool)
{
  if (pool < riExtra.size())
    return abs(riExtra[pool].bsphRad()); // radius is negative sometimes

  logerr("failed to get 'bsphRad' property of invalid pool '%ull'", pool);
  return 0.0f;
}

Point3 rendinst::getRIGenExtraBsphPos(uint32_t pool)
{
  if (pool < riExtra.size())
  {
    vec4f xyzr = riExtra[pool].bsphXYZR;
    return Point3(v_extract_x(xyzr), v_extract_y(xyzr), v_extract_z(xyzr));
  }

  logerr("failed to get 'bsphPos' property of invalid pool '%ull'", pool);
  return Point3(0.0f, 0.0f, 0.0f);
}

Point4 rendinst::getRIGenExtraBSphereByTM(uint32_t pool, const TMatrix &tm)
{
  if (pool < riExtra.size())
  {
    const vec4f xyzr = riExtra[pool].bsphXYZR;
    const float radius = abs(v_extract_w(xyzr)); // radius is negative sometimes
    const Point3 center = Point3(v_extract_x(xyzr), v_extract_y(xyzr), v_extract_z(xyzr));
    const float maxScale = max(tm.getcol(0).length(), max(tm.getcol(1).length(), tm.getcol(2).length()));
    const Point3 pos = tm * center;
    return Point4(pos.x, pos.y, pos.z, radius * maxScale);
  }

  logerr("failed to get 'bsphereByTM' of invalid pool '%ull'", pool);
  return Point4(0.0f, 0.0f, 0.0f, 0.0f);
}

int rendinst::getRIGenExtraParentForDestroyedRiIdx(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].parentForDestroyedRiIdx;

  logerr("failed to get 'parentForDestroyedRiIdx' property of invalid pool '%ull'", pool);
  return -1;
}

bool rendinst::isRIGenExtraDestroyedPhysResExist(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].isDestroyedPhysResExist;

  logerr("failed to get 'isDestroyedPhysResExist' property of invalid pool '%ull'", pool);
  return false;
}

int rendinst::getRIGenExtraDestroyedRiIdx(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].destroyedRiIdx;

  logerr("failed to get 'destroyedRiIdx' property of invalid pool '%ull'", pool);
  return -1;
}

vec4f rendinst::getRIGenExtraBSphere(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  uint32_t instIdx = handle_to_ri_inst(id);
  G_ASSERT(resIdx < riExtra.size() && instIdx < riExtra[resIdx].riXYZR.size());
  return riExtra[resIdx].riXYZR[instIdx];
}

void rendinst::setRiGenExtraHp(riex_handle_t id, float hp)
{
  uint32_t resIdx = handle_to_ri_type(id);
  uint32_t instIdx = handle_to_ri_inst(id);
  G_ASSERT(resIdx < riExtra.size() && instIdx < riExtra[resIdx].riHP.size());
  riExtra[resIdx].setHp(instIdx, hp);
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
  if (moved && pool.gridIdx >= 0 && pool.isInGrid(idx))
  {
    mat44f tm44_old;
    bbox3f wabb0;

    v_mat43_transpose_to_mat44(tm44_old, pool.riTm[idx]);
    v_bbox3_init(wabb0, tm44_old, pool.collBb);
    Point3_vec4 p3_old;
    v_st(&p3_old.x, pool.riXYZR[idx]);

    int old_grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), false);
    pool.riXYZR[idx] = bsphere;
    int new_grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), true);
    if (old_grid_idx == new_grid_idx && new_grid_idx >= 0)
    {
      riExtraGrid[old_grid_idx].update(id, p3_old, *(Point3 *)&pool.riXYZR[idx], wabb0, wabb1);
      update_max_ri_extra_height(static_cast<int>(v_extract_y(wabb1.bmax) - v_extract_y(wabb1.bmin)) + 1);
    }
    else
    {
      if (old_grid_idx >= 0)
        riExtraGrid[old_grid_idx].remove(id, p3_old, wabb0);
      if (new_grid_idx >= 0)
      {
        riExtraGrid[new_grid_idx].insert(id, *(Point3 *)&pool.riXYZR[idx], wabb1);
        update_max_ri_extra_height(static_cast<int>(v_extract_y(wabb1.bmax) - v_extract_y(wabb1.bmin)) + 1);
      }
      else
        pool.riXYZR[idx] = v_perm_xyzd(bsphere, v_neg(bsphere));
    }

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
    pool.riXYZR[idx] = v_perm_xyzd(bsphere, v_neg(bsphere));
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

  if (pool.gridIdx >= 0 && pool.isInGrid(idx))
  {
    mat44f tm44;
    bbox3f wabb;
    v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);
    v_bbox3_init(wabb, tm44, pool.collBb);

    int grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), false);
    if (grid_idx >= 0)
    {
      riExtraGrid[grid_idx].remove(id, *(Point3 *)&pool.riXYZR[idx], wabb);
      pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_neg(pool.riXYZR[idx]));
    }

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
  if (pool.isValid(idx) && pool.gridIdx >= 0 && pool.isInGrid(idx))
  {
    mat44f tm44;
    bbox3f wabb;
    v_mat43_transpose_to_mat44(tm44, pool.riTm[idx]);
    v_bbox3_init(wabb, tm44, pool.collBb);

    int grid_idx = reselect_grid_idx(pool.gridIdx, pool.getObjRad(idx), false);
    if (grid_idx >= 0)
    {
      riExtraGrid[grid_idx].remove(id, *(Point3 *)&pool.riXYZR[idx], wabb);
      pool.riXYZR[idx] = v_perm_xyzd(pool.riXYZR[idx], v_neg(pool.riXYZR[idx]));
    }

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
    for (int gi = riExtraGrid.size() - 1; gi >= 0; gi--)
    {
      if (!riExtraGrid[gi].hasObjects())
        continue;

      riExtraGrid[gi].iterateBox(v_ldu_bbox3(box), [&](const rendinst::riex_handle_t *h, int cnt) {
        for (; cnt > 0; cnt--, h++)
        {
          vec4f bsph = rendinst::riExtra[rendinst::handle_to_ri_type(*h)].riXYZR[rendinst::handle_to_ri_inst(*h)];
          if (v_bbox3_test_sph_intersect(v_ldu_bbox3(box), bsph, v_sqr_x(v_splat_w(bsph))))
            out_handles.push_back(*h);
        }
        return false;
      });
    }
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
  for (int gi = riExtraGrid.size() - 1; gi >= 0; gi--)
  {
    if (!riExtraGrid[gi].hasObjects())
      continue;

    riExtraGrid[gi].iterateBox(box, [&](const rendinst::riex_handle_t *h, int cnt) {
      for (; cnt > 0; cnt--, h++)
      {
        vec4f bsph = rendinst::riExtra[rendinst::handle_to_ri_type(*h)].riXYZR[rendinst::handle_to_ri_inst(*h)];
        if (v_bbox3_test_sph_intersect(box, bsph, v_sqr_x(v_splat_w(bsph))))
        {
          if (v_extract_w(bsph) >= min_bsph_rad)
            out_handles.push_back(*h);
        }
      }
      return false;
    });
  }
}

void rendinst::gatherRIGenExtraCollidable(riex_collidable_t &out_handles, const Point3 &from, const Point3 &dir, float len,
  bool read_lock)
{
  if (read_lock)
    rendinst::ccExtra.lockRead();
  {
    TIME_PROFILE_DEV(gather_riex_collidable_ray);
    for (int gi = riExtraGrid.size() - 1; gi >= 0; gi--)
    {
      if (!riExtraGrid[gi].hasObjects())
        continue;

      riExtraGrid[gi].iterateRay(from, dir, len, [&](const rendinst::riex_handle_t *h, int cnt) {
        for (; cnt > 0; cnt--, h++)
        {
          vec4f xyzr = rendinst::riExtra[rendinst::handle_to_ri_type(*h)].riXYZR[rendinst::handle_to_ri_inst(*h)];
          vec4f r = v_splat_w(xyzr);
          if (v_test_ray_sphere_intersection(v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), xyzr, v_mul_x(r, r)))
            out_handles.push_back(*h);
        }
        return false;
      });
    }
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

  const char *next_res = riConf->getStr("nextRes", NULL);
  if (next_res && *next_res)
  {
    b->setStr("refRendInst", next_res);
    b->setStr("refColl", String(128, "%s" RI_COLLISION_RES_SUFFIX, next_res));
  }
  next_res = riConf->getStr("physRes", NULL);
  if (next_res && *next_res)
    b->setStr("refPhysObj", next_res);
  next_res = riConf->getStr("apexRes", NULL);
  if (next_res && *next_res)
    b->setStr("refApex", next_res);
}
void rendinst::prepareRiExtraRefs(const DataBlock &_riConf)
{
  const DataBlock &riConf = *_riConf.getBlockByNameEx("riExtra");
  for (int i = 0; i < riConf.blockCount(); i++)
    if (DataBlock *b = gameres_rendinst_desc.getBlockByName(riConf.getBlock(i)->getBlockName()))
      addRiExtraRefs(b, riConf.getBlock(i), NULL);
}

bool rayHitRiExtraInstance(vec4f from, vec4f dir, float len, rendinst::riex_handle_t handle, rendinst::RendInstDesc &ri_desc,
  const MaterialRayStrat &strategy)
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

  int matId = -1;
  if (collRes->rayHit(tm, from, dir, len, matId))
  {
    if (strategy.shouldIgnoreRendinst(/*isPos*/ false, /* is_immortal */ false, matId))
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
  vec4f p0 = v_ld(&from.x);
  vec4f ndir = v_ld(&dir.x);
  bbox3f box;

  box.bmin = box.bmax = p0;
  v_bbox3_add_pt(box, v_madd(ndir, v_splat_w(p0), p0));

  ScopedRIExtraReadLock rd;
  for (int gi = riExtraGrid.size() - 1; gi >= 0; gi--)
  {
    if (!riExtraGrid[gi].hasObjects())
      continue;

    bool ret = riExtraGrid[gi].iterateBox(box, [&](const riex_handle_t *h, int cnt) {
      for (; cnt > 0; cnt--, h++)
      {
        rendinst::RiExtraPool &riPool = riExtra[rendinst::handle_to_ri_type(*h)];
        int inst = rendinst::handle_to_ri_inst(*h);
        if (!riPool.isValid(inst) || riPool.getObjRad(inst) < min_r)
          continue;
        vec4f xyzr = riPool.riXYZR[inst];
        vec4f r = v_splat_w(xyzr);
        if (!v_test_ray_sphere_intersection(v_ldu(&from.x), v_ldu(&dir.x), v_splats(len), xyzr, v_mul_x(r, r)))
          continue;
        if (rayHitRiExtraInstance(v_ldu(&from.x), v_ldu(&dir.x), len, *h, ri_desc, strategy))
          return true;
      }
      return false;
    });
    if (ret)
      return true;
  }
  return false;
}

void rendinst::reapplyLodRanges()
{
  for (int id = 0; id < riExtra.size(); id++)
  {
    const DataBlock *ri_ovr = rendinst::ri_lod_ranges_ovr.getBlockByName(riExtraMap.getName(id));
    RenderableInstanceLodsResource *riRes = riExtra[id].res;

    int lod_cnt = riRes->lods.size();
    float max_lod_dist = lod_cnt ? riResLodRange(riRes, lod_cnt - 1, ri_ovr) : (RendInstGenData::renderResRequired ? 0.0f : 1e5f);
    if (riExtra[id].isPosInst() && lod_cnt)
      lod_cnt--; //== skip last LOD for posInst since imposters not supported for riExtra

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

bbox3f rendinst::riex_get_lbb(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return bbox3f();
  return riExtra[res_idx].lbb;
}

float rendinst::ries_get_bsph_rad(int res_idx)
{
  if (res_idx >= riExtra.size())
    return 0.f;
  return riExtra[res_idx].sphereRadius;
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

uint32_t rendinst::get_riextra_instance_seed(rendinst::riex_handle_t handle)
{
  uint32_t instSeed = 0;
  if (const uint32_t *userData = rendinst::get_user_data(handle))
    instSeed = userData[0]; // correct while instSeed lying at the start of data
  return instSeed;
}

float rendinst::get_riextra_ttl(rendinst::riex_handle_t handle) { return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].ttl; }

bool rendinst::get_riextra_immortality(rendinst::riex_handle_t handle)
{
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].immortal;
}

void rendinst::set_riextra_instance_seed(rendinst::riex_handle_t handle, int32_t data)
{
  ScopedRIExtraWriteTryLock wr;
  if (!wr.isLocked())
    return;

  uint32_t res_idx = handle_to_ri_type(handle);
  uint32_t idx = handle_to_ri_inst(handle);
  G_ASSERT_RETURN(res_idx < riExtra.size(), );
  RiExtraPool &pool = riExtra[res_idx];
  if (!pool.isValid(idx))
  {
    logmessage(LOGMESSAGE_LEVEL, "set_riextra_instance_seed idx out of range or dead: idx=%d, count=%d (res_idx=%d)", idx,
      pool.riTm.size(), res_idx);
    return;
  }

  scene::node_index nodeId = pool.tsNodeIdx[idx];

  if (nodeId == scene::INVALID_NODE)
    return;

  rendinst::set_instance_user_data(nodeId, 1, &data);
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

static void riPerDrawData_afterDeviceReset(bool /*full_reset*/)
{
  if (!RendInstGenData::renderResRequired)
    return;
  for (uint32_t i = 0; i < rendinst::riExtra.size(); ++i)
    rendinst::update_per_draw_gathered_data(i);
}

REGISTER_D3D_AFTER_RESET_FUNC(riPerDrawData_afterDeviceReset);

void rendinst::setRiGenResMatId(uint32_t res_idx, int matId)
{
  G_ASSERT_RETURN(res_idx < riExtra.size(), );
  const rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (RendInstGenData *rgl = (pool.riPoolRef >= 0) ? rendinst::getRgLayer(pool.riPoolRefLayer) : nullptr)
  {
    RendInstGenData::RendinstProperties &riProp = rgl->rtData->riProperties[pool.riPoolRef];
    riProp.matId = matId;
  }
}

void rendinst::setRiGenResHp(uint32_t res_idx, float hp)
{
  G_ASSERT_RETURN(res_idx < riExtra.size(), );
  rendinst::riExtra[res_idx].initialHP = hp;
}

void rendinst::setRiGenResDestructionImpulse(uint32_t res_idx, float impulse)
{
  G_ASSERT_RETURN(res_idx < riExtra.size(), );
  const rendinst::RiExtraPool &pool = rendinst::riExtra[res_idx];
  if (RendInstGenData *rgl = (pool.riPoolRef >= 0) ? rendinst::getRgLayer(pool.riPoolRefLayer) : nullptr)
  {
    RendInstGenData::DestrProps &riDestr = rgl->rtData->riDestr[pool.riPoolRef];
    riDestr.destructionImpulse = impulse;
  }
}

bool rendinst::isRIExtraGenPosInst(uint32_t res_idx)
{
  G_ASSERT_RETURN(res_idx < riExtra.size(), false);
  return rendinst::riExtra[res_idx].posInst;
}

bool rendinst::updateRiExtraReqLod(uint32_t res_idx, unsigned lod)
{
  G_ASSERT_RETURN(res_idx < riExtra.size(), false);
  RenderableInstanceLodsResource *riRes = rendinst::riExtra[res_idx].res;
  if (!riRes)
    return false;

  riRes->updateReqLod(lod);
  return true;
}

rendinst::RendInstDesc rendinst::get_restorable_desc(const RendInstDesc &ri_desc)
{
  const auto &rd = rendinst::riExtra[ri_desc.pool].riUniqueData[ri_desc.idx];

  if (rd.cellId < 0)
    return {};

  RendInstDesc result = ri_desc;
  result.cellIdx = -(rd.cellId + 1);
  result.offs = rd.offset;
  return result;
}

int rendinst::find_restorable_data_index(const RendInstDesc &desc)
{
  const auto &ud = riExtra[desc.pool].riUniqueData;
  for (int r = 0; r < ud.size(); ++r)
    if (ud[r].cellId == -(desc.cellIdx + 1) && ud[r].offset == desc.offs)
      return r;

  return -1;
}
