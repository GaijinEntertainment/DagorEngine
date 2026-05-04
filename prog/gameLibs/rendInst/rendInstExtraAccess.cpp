// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/rendInstExtraAccess.h>

#include "riGen/riGenData.h"
#include "riGen/riGenExtra.h"
#include "render/extraRender.h"
#include "scopedRiExtraWriteTryLock.h"


// TODO: this was copy-pasted from rendInstGenExtra and is inconsistent
// accross the library, should probably be unified somehow.
#if DAGOR_DBGLEVEL > 0
static const int LOGMESSAGE_LEVEL = LOGLEVEL_ERR;
#else
static const int LOGMESSAGE_LEVEL = LOGLEVEL_WARN;
#endif

float rendinst::get_riextra_destr_time_to_live(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, -1.0f);
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrTimeToLive;
}
float rendinst::get_riextra_destr_default_time_to_live(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, -1.0f);
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrDefaultTimeToLive;
}
float rendinst::get_riextra_destr_time_to_kinematic(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, -1.0f);
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrTimeToKinematic;
}
float rendinst::get_riextra_destr_time_to_sink_underground(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, -1.0f);
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrTimeToSinkUnderground;
}

Point3 rendinst::get_riextra_destr_disintegration_params(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, Point3(-1.0f, 0, 1));
  return Point3(rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrTimeToStartDisintegration,
    rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrDisintegrationDuration,
    rendinst::riExtra[rendinst::handle_to_ri_type(handle)].destrDisintegrationScale);
}

bool rendinst::get_riextra_immortality(rendinst::riex_handle_t handle)
{
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].immortal;
}

bool rendinst::is_riextra_rendinst_clipmap(rendinst::riex_handle_t handle)
{
  G_ASSERT_RETURN(handle != RIEX_HANDLE_NULL, false);
  return rendinst::riExtra[rendinst::handle_to_ri_type(handle)].isRendinstClipmap;
}

uint32_t rendinst::get_riextra_instance_seed(rendinst::riex_handle_t handle)
{
  uint32_t instSeed = 0;
  if (const uint32_t *userData = rendinst::get_user_data(handle))
    instSeed = userData[0]; // correct while instSeed lying at the start of data
  return instSeed;
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

const mat43f &rendinst::getRIGenExtra43(riex_handle_t id)
{
  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);
  if (res_idx < riExtra.size())
  {
    RiExtraPool &pool = riExtra[res_idx];
    if (idx < pool.riTm.size())
      return pool.riTm[idx];
    else
      logerr("%s idx out of range: idx=%d, count=%d (res_idx=%d)", __FUNCTION__, idx, pool.riTm.size(), res_idx);
  }
  else
    logerr("%s res_idx out of range: idx=%d, count=%d (res_idx=%d)", __FUNCTION__, idx, riExtra.size(), res_idx);

  static mat43f m43;
  m43.row0 = m43.row1 = m43.row2 = v_zero();
  return m43;
}

const mat43f &rendinst::getRIGenExtra43(riex_handle_t id, uint32_t &seed)
{
  seed = get_riextra_instance_seed(id);
  return getRIGenExtra43(id);
}

namespace rendinst
{
void getRIGenExtra44NoLock(riex_handle_t id, mat44f &out_tm) { v_mat43_transpose_to_mat44(out_tm, getRIGenExtra43(id)); }
} // namespace rendinst

void rendinst::getRIGenExtra44(riex_handle_t id, mat44f &out_tm)
{
  ScopedRIExtraReadLock rd;
  getRIGenExtra44NoLock(id, out_tm);
}

uint32_t rendinst::getRIGenExtraPoolCount() { return riExtra.size(); }

dag::ConstSpan<mat43f> rendinst::getAllRIGenExtra43FromPool(uint32_t pool)
{
  return pool < riExtra.size() ? riExtra[pool].riTm : dag::ConstSpan<mat43f>();
}

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
float rendinst::getDamageThreshold(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT_RETURN(resIdx < riExtra.size(), 0.f);
  return riExtra[resIdx].damageThreshold;
}
bool rendinst::isInvincible(riex_handle_t id)
{
  uint32_t resIdx = handle_to_ri_type(id);
  G_ASSERT_RETURN(resIdx < riExtra.size(), false);
  return riExtra[resIdx].isInvincible(handle_to_ri_inst(id));
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
    logerr("failed to set 'immortal' property of invalid pool '%u'", pool);
    return;
  }
  riExtra[pool].immortal = value;
}

bool rendinst::isRIGenExtraImmortal(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].immortal;

  logerr("failed to get 'immortal' property of invalid pool '%u'", pool);
  return true;
}

bool rendinst::isRIGenExtraWalls(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].isWalls;

  logerr("failed to get 'isWalls' property of invalid pool '%u'", pool);
  return true;
}

vec4f rendinst::getRIGenExtraPoolBSphere(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].bsphXYZR;

  logerr("failed to get 'bsphXYZR' property of invalid pool '%u'", pool);
  return v_zero();
}

float rendinst::getMaxLodDist(uint32_t pool)
{
  if (pool < riExtra.size())
  {
    const RiExtraPool &riexPool = riExtra[pool];

    const int llm = riexPool.lodLimits >> ((rendinst::ri_game_render_mode + 1) * 8);
    const int max_lod = (llm >> 4) & 0xF;
    return riexPool.distSqLOD[max_lod];
  }

  logerr("failed to get 'maxLodDist' property of invalid pool '%u'", pool);
  return 0;
}

float rendinst::getRIGenExtraBsphRad(uint32_t pool)
{
  if (pool < riExtra.size())
    return abs(riExtra[pool].bsphRad()); // radius is negative sometimes

  logerr("failed to get 'bsphRad' property of invalid pool '%u'", pool);
  return 0.0f;
}

Point3 rendinst::getRIGenExtraBsphPos(uint32_t pool)
{
  if (pool < riExtra.size())
  {
    vec4f xyzr = riExtra[pool].bsphXYZR;
    return Point3(v_extract_x(xyzr), v_extract_y(xyzr), v_extract_z(xyzr));
  }

  logerr("failed to get 'bsphPos' property of invalid pool '%u'", pool);
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

  logerr("failed to get 'bsphereByTM' of invalid pool '%u'", pool);
  return Point4(0.0f, 0.0f, 0.0f, 0.0f);
}

int rendinst::getRIGenExtraParentForDestroyedRiIdx(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].parentForDestroyedRiIdx;

  logerr("failed to get 'parentForDestroyedRiIdx' property of invalid pool '%u'", pool);
  return -1;
}

bool rendinst::isRIGenExtraDestroyedPhysResExist(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].isDestroyedPhysResExist;

  logerr("failed to get 'isDestroyedPhysResExist' property of invalid pool '%u'", pool);
  return false;
}

int rendinst::getRIGenExtraDestroyedRiIdx(uint32_t pool)
{
  if (pool < riExtra.size())
    return riExtra[pool].destroyedRiIdx;

  logerr("failed to get 'destroyedRiIdx' property of invalid pool '%u'", pool);
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

const char *rendinst::getRIGenExtraName(uint32_t res_idx)
{
  ScopedRIExtraReadLock rd;
  return riExtraMap.getName(res_idx);
}

int rendinst::getRIGenExtraPreferableLodRawDistanceUnsafe(uint32_t res_idx, float squared_distance, bool &over_max_lod,
  int &last_lod_arg)
{
  const rendinst::RiExtraPool &riPool = rendinst::riExtra[res_idx];
  int lod = find_lod<rendinst::RiExtraPool::MAX_LODS>(riPool.distSqLOD, squared_distance);
  const int llm = riPool.lodLimits >> ((rendinst::ri_game_render_mode + 1) * 8);
  const int min_lod = llm & 0xF, max_lod = (llm >> 4) & 0xF;
  over_max_lod = riPool.distSqLOD[max_lod] <= squared_distance;
  last_lod_arg = max_lod;
  lod = clamp(lod, min_lod, max_lod);
  return lod;
}

int rendinst::getRIGenExtraPreferableLodRawDistance(uint32_t res_idx, float squared_distance, bool &over_max_lod, int &last_lod_arg)
{
  if (res_idx >= rendinst::riExtra.size())
    return -1;
  return getRIGenExtraPreferableLodRawDistanceUnsafe(res_idx, squared_distance, over_max_lod, last_lod_arg);
}

int rendinst::getRIGenExtraPreferableLod(uint32_t res_idx, float squared_distance, bool &over_max_lod, int &last_lod)
{
  squared_distance *= rendinst::riExtraCullDistSqMul;
  return getRIGenExtraPreferableLodRawDistance(res_idx, squared_distance, over_max_lod, last_lod);
}

int rendinst::getRIGenExtraPreferableLod(uint32_t res_idx, float squared_distance)
{
  bool over_max_lod;
  int max_lod;
  squared_distance *= rendinst::riExtraCullDistSqMul;
  return getRIGenExtraPreferableLodRawDistance(res_idx, squared_distance, over_max_lod, max_lod);
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

bbox3f rendinst::riex_get_lbb(int res_idx)
{
  if (res_idx < 0 || res_idx >= riExtra.size())
    return bbox3f();
  return riExtra[res_idx].lbb;
}

float rendinst::riex_get_bsph_rad(int res_idx)
{
  if (res_idx >= riExtra.size())
    return 0.f;
  return riExtra[res_idx].sphereRadius;
}

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

vec4f rendinst::getNodeBsphere(riex_handle_t handle, float &pool_rad) { return rendinst::get_node_bsphere(handle, pool_rad); }

void rendinst::prefetchNode(riex_handle_t handle)
{
  uint32_t res_idx = handle_to_ri_type(handle);
  uint32_t idx = handle_to_ri_inst(handle);
  if (DAGOR_UNLIKELY(res_idx >= riExtra.size()))
    return;

  auto &pool = riExtra.data()[res_idx];
  RiExtraPool::NodeIdxAndTs tsNodeIdx = pool.getNodeIdx(idx);
  if (DAGOR_UNLIKELY(tsNodeIdx.nodeIdx == scene::INVALID_NODE))
    return;

  riExTiledScenes[tsNodeIdx.tsIndex].prefetchNode(tsNodeIdx.nodeIdx);
  G_UNUSED(pool);

  PREFETCH_DATA(0, &pool.lodLimits);
  PREFETCH_DATA(0, &pool.distSqLOD);
}

bool rendinst::isNodeVisible(riex_handle_t id, uint32_t res_idx, const mat44f *&mat_out, uint32_t &hash)
{
#if DAGOR_DBGLEVEL > 0
  if (DAGOR_UNLIKELY(res_idx != handle_to_ri_type(id)))
  {
    LOGERR_ONCE("res_idx mismatch: %u (arg) vs %u (from id)", res_idx, handle_to_ri_type(id));
    res_idx = handle_to_ri_type(id); // original behavior
  }
#endif

  uint32_t idx = handle_to_ri_inst(id);
  const RiExtraPool &pool = riExtra[res_idx];

  RiExtraPool::NodeIdxAndTs tsNodeIdx = pool.getNodeIdx(idx);
  if (tsNodeIdx.nodeIdx == scene::INVALID_NODE)
    return false;

  RendinstTiledScene &scene = riExTiledScenes[tsNodeIdx.tsIndex];
  if (!scene.isAliveNodeFast(tsNodeIdx.nodeIdx))
    return false;

  const mat44f &m = scene.getAliveNode(tsNodeIdx.nodeIdx);
  const uint32_t flags = scene::get_node_flags(m);
  const uint32_t visibleFlag = rendinst::ri_game_render_mode == 0   ? rendinst::RendinstTiledScene::VISIBLE_0
                               : rendinst::ri_game_render_mode == 2 ? rendinst::RendinstTiledScene::VISIBLE_2
                                                                    : 0;
  if ((flags & visibleFlag) != visibleFlag)
    return false;

  mat_out = &m;

  uint32_t *userDataPtr = (uint32_t *)scene.getUserData(tsNodeIdx.nodeIdx);
  hash = userDataPtr ? userDataPtr[0] : 0;

  return true;
}

bool rendinst::isNodeAlive(riex_handle_t id, vec4f &bsphere, float &pool_rad)
{
  uint32_t res_idx = handle_to_ri_type(id);
  uint32_t idx = handle_to_ri_inst(id);

  const RiExtraPool &pool = riExtra[res_idx];

  RiExtraPool::NodeIdxAndTs tsNodeIdx = pool.getNodeIdx(idx);
  if (tsNodeIdx.nodeIdx == scene::INVALID_NODE)
    return false;

  RendinstTiledScene &scene = riExTiledScenes[tsNodeIdx.tsIndex];
  if (!scene.isAliveNodeFast(tsNodeIdx.nodeIdx))
    return false;

  const mat44f &m = scene.getAliveNode(tsNodeIdx.nodeIdx);
  bsphere = scene::get_node_bsphere(m);
  pool_rad = scene.getPoolSphereRad(res_idx);

  return true;
}

float rendinst::getCullDistSqMul() { return riExtraCullDistSqMul; }

namespace rendinst
{
struct AllRendinstExtraScenesLockImpl
{
private:
  bool needUnlock[rendinst::RITiledScenes::MAX_COUNT];

public:
  AllRendinstExtraScenesLockImpl()
  {
    for (int i = 0; i < rendinst::RITiledScenes::MAX_COUNT; i++)
      needUnlock[i] = rendinst::riExTiledScenes[i].lockForRead();
  }
  ~AllRendinstExtraScenesLockImpl()
  {
    for (int i = rendinst::RITiledScenes::MAX_COUNT - 1; i >= 0; i--)
      if (needUnlock[i])
        rendinst::riExTiledScenes[i].unlockAfterRead();
  }
};

AllRendinstExtraScenesLock::AllRendinstExtraScenesLock() { ptr = new AllRendinstExtraScenesLockImpl(); }
AllRendinstExtraScenesLock::~AllRendinstExtraScenesLock() { delete ptr; }
} // namespace rendinst