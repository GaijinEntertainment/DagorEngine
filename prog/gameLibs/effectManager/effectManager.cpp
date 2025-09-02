// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <effectManager/effectManager.h>

#include <generic/dag_sort.h>
#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResources.h>
#include <math/dag_frustum.h>
#include <math/random/dag_random.h>
#include <ADT/threadpoolLocalVector.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <gameRes/dag_stdGameResId.h>
#include <memory/dag_framemem.h>
#include <startup/dag_globalSettings.h>
#include <effectManager/effectManagerDetails.h>
#include <generic/dag_relocatableFixedVector.h>

#if DAGOR_DBGLEVEL > 0
#include <debug/dag_log.h>
#endif

#define FX_CMD_STAT_DBG 0

#define debug(...)             logmessage(_MAKE4C('_FX_'), __VA_ARGS__)
#define RANDOM_DISTANCE_OFFSET 0.2

namespace acesfx
{
bool enableUnlimitedTestingMode = false;
extern dafx::ContextId g_dafx_ctx;
extern bool g_async_load;
} // namespace acesfx

Point3 EffectManager::viewPos = Point3(0, 0, 0);

struct EffectManager::ActiveShadow
{
  EffectManager &mgr;
  AcesEffect::FxId fxId;

  LightEffect &getLightEffect() const
  {
    const BaseEffect *e = mgr.fxList.get(fxId);
    G_ASSERT(e && e->lightId >= 0);
    return mgr.lightList[e->lightId];
  }

  float distanceSq() const { return (getLightEffect().pos - EffectManager::viewPos).lengthSq(); }
};
static constexpr size_t DEFAULT_ACTIVE_SHADOWS = 4;
using ActiveShadowVec = dag::RelocatableFixedVector<EffectManager::ActiveShadow, DEFAULT_ACTIVE_SHADOWS>;
static ActiveShadowVec activeShadowVec;

static int maxActiveShadows = 0;
static float invMagnification = 1.0f;
static float invMagnificationSq = 1.0f;

void EffectManager::onSettingsChanged(bool shadows_enabled, int max_active_shadows)
{
  maxActiveShadows = shadows_enabled ? max_active_shadows : 0;
  onSettingsChangedExt();

  // Release all active shadows
  for (const ActiveShadow &sh : activeShadowVec)
  {
    LightEffect &le = sh.getLightEffect();
    le.shadowEnabled = ShadowEnabled::NOT_INITED;
    G_ASSERT(le.extLightId >= 0);
    le.releaseShadowExt();
  }
  activeShadowVec.clear();
}

void EffectManager::setMagnification(float magnification)
{
  invMagnification = safeinv(magnification);
  invMagnificationSq = invMagnification * invMagnification;
}

void EffectManager::activateShadow(LightEffect &le)
{
  // Activate N nearest to the camera effects with shadow
  G_ASSERT(le.shadowParams.enabled && le.extLightId >= 0);

  if (maxActiveShadows == 0 || le.shadowEnabled != ShadowEnabled::NOT_INITED)
    return;

  le.shadowEnabled = ShadowEnabled::OFF;

  // if we already have maxActiveShadows effect, try to find more far effect to disable light
  if (activeShadowVec.size() >= maxActiveShadows)
  {
    float maxDistSq = -1;
    const ActiveShadow *farShadow = nullptr;
    for (ActiveShadow &sh : activeShadowVec)
    {
      float curDistSq = sh.distanceSq();
      if (curDistSq > maxDistSq)
      {
        maxDistSq = curDistSq;
        farShadow = &sh;
      }
    }
    G_ASSERT(farShadow);

    if (maxDistSq < (le.pos - EffectManager::viewPos).lengthSq())
      // If current effect is more far then all active shadow effects, stop and return
      return;

    // Disable light for found effect (more far, than current)
    farShadow->mgr.releaseShadow(farShadow->getLightEffect(), ShadowEnabled::OFF, farShadow);
    // Now activeShadowVec.size() == maxActiveShadows - 1
  }

  // activeShadowVec.size() < maxActiveShadows, so we can activate current effect
  le.shadowEnabled = ShadowEnabled::ON;
  le.activateShadowExt();
  activeShadowVec.push_back({*this, le.fxId});
}

void EffectManager::releaseShadow(LightEffect &le, ShadowEnabled shadow_enabled, const ActiveShadow *record) const
{
  G_ASSERT(shadow_enabled == ShadowEnabled::OFF || shadow_enabled == ShadowEnabled::NOT_INITED);
  G_ASSERT(lightList.begin() <= &le && &le < lightList.end());

  bool isShadowEnabled = le.shadowEnabled == ShadowEnabled::ON;
  le.shadowEnabled = shadow_enabled;

  if (!isShadowEnabled)
    return;

  if (!record)
    record = eastl::find_if(activeShadowVec.begin(), activeShadowVec.end(),
      [this, &le](const ActiveShadow &sh) { return sh.fxId == le.fxId && &sh.mgr == this; });

  G_ASSERT(record != activeShadowVec.end());

  if (le.extLightId >= 0)
    le.releaseShadowExt();
  activeShadowVec.erase_unsorted(record);
}

void EffectManager::destroyLight(BaseEffect &fx)
{
  if (fx.lightId < 0)
    return;

  LightEffect &le = lightList[fx.lightId];
  if (le.extLightId >= 0)
  {
    destroyLightExt(le);
    le.extLightId = -1;
    releaseShadow(le);
  }

  BaseEffect *swapFx = remove_and_fix_subfx(fx.lightId, lightList, fxList);
  if (swapFx)
    swapFx->lightId = fx.lightId;
  fx.lightId = -1;
}

void EffectManager::setFxLight(BaseEffect &fx)
{
  if (!params.lightEnabled)
    return;
  bool hidden = (fx.visibility & AcesEffect::VISIBLE) == 0;

  LightEffect &le = lightList[fx.lightId];
  if (hidden == bool(le.extLightId < 0))
    return;

  if (!hidden)
  {
    if (le.extLightId < 0)
    {
      Point3 *lightPos = static_cast<Point3 *>(fx.obj->getParam(HUID_LIGHT_POS, nullptr));
      G_ASSERT(lightPos);

      Color4 *lightParams = static_cast<Color4 *>(fx.obj->getParam(HUID_LIGHT_PARAMS, NULL));
      G_ASSERT(lightParams);

      le.pos = *lightPos;
      le.params = Point4::xyzV(Point3::rgb(*lightParams) * le.intensity, lightParams->a * le.radiusMultiplier);

      le.extLightId = addLightExt(le);
    }
  }
  else if (le.extLightId >= 0)
  {
    destroyLightExt(le);
    le.extLightId = -1;
    releaseShadow(le);
  }
}

void EffectManager::updateDelayedLights()
{
  OSSpinlockScopedLock lock(&mgrSLock);

  for (LightEffect &le : lightList)
  {
    if (le.extLightId < 0) // hidden
      continue;

    updateLightExt(le);

    if (le.shadowParams.enabled)
      activateShadow(le);
  }
}

AcesEffect::~AcesEffect()
{
  // These instances are assumed to be deleted from within EffectManager::{startEffect,update,reset) with mgrSLock already locked
  EffectManager::BaseEffect *e = mgr->fxList.get(fxId);
  G_ASSERT_RETURN(e, );
  mgr->destroyEffect(fxId, *e);
}
void AcesEffect::setFxTm(const TMatrix &tm) { mgr->setFxTmBuff(fxId, tm, false); }
void AcesEffect::setEmitterTm(const TMatrix &tm) { mgr->setFxTmBuff(fxId, tm, true); }
void AcesEffect::setVelocity(const Point3 &vel) { mgr->setFxVelocityBuff(fxId, vel); }
void AcesEffect::setVelocityScaleMinMax(const Point2 &scale) { mgr->setFxVelocityScaleMinMaxBuff(fxId, scale); }
void AcesEffect::setSpawnRate(float rate) { mgr->setFxSpawnRateBuff(fxId, rate); }
void AcesEffect::setFakeBrightnessBackgroundPos(const Point3 &backgroundPos)
{
  mgr->setFakeBrightnessBackgroundPosBuff(fxId, backgroundPos);
}
void AcesEffect::setLightRadiusMultiplier(float multiplier) { mgr->setFxLightRadiusMultiplierBuff(fxId, multiplier); }
void AcesEffect::setFxScale(float scale) { mgr->setFxLightIntensityBuff(fxId, scale); }
void AcesEffect::setRestrictionBox(const TMatrix &box) { mgr->setFxLightBoxBuff(fxId, box); }
void AcesEffect::setWindScale(float scale) { mgr->setFxWindScaleBuff(fxId, scale); }
void AcesEffect::setColorMult(const Color4 &colorMult) { mgr->setFxColorMultBuff(fxId, colorMult); }
void AcesEffect::setVisibility(uint32_t visibility) { mgr->setFxVisibilityBuff(fxId, visibility, false); }
void AcesEffect::hide(bool hidden) { mgr->setFxVisibilityBuff(fxId, hidden ? 0 : 0xffffffff, false); }
void AcesEffect::unlock() { mgr->unlockFxBuff(fxId); }
bool AcesEffect::isAlive() const { return cachedFlags & IS_LOCKED; }
void AcesEffect::unsetEmitter() { mgr->unsetFxEmitterBuff(fxId); }
void AcesEffect::stop() { mgr->stopFxBuff(fxId); }
void AcesEffect::pauseSound(bool pause) { mgr->pauseFxSoundBuff(fxId, pause); }
bool AcesEffect::hasSound() const { return (*mgr->params.sfxPath) && (*mgr->params.sfxEvent); }
void AcesEffect::enableActiveQuery()
{
  mgr->enableActiveQueryBuff(fxId);
  cachedFlags |= ACTIVE_QUERY_ENABLED; // set now it to avoid error in isActive
}
bool AcesEffect::isActive() const
{
#if DAGOR_DBGLEVEL > 0
  if ((cachedFlags & ACTIVE_QUERY_ENABLED) == 0)
    LOGERR_ONCE("isActive() called without enableActiveQuery() flag for fx:'%s'", getName());
#endif
  return cachedFlags & IS_ACTIVE;
}
float AcesEffect::soundEnableDistanceSq() const { return mgr->params.soundEnableDistanceSq; }
const Point3 &AcesEffect::getWorldPos() const { return cachedPos; }
const char *AcesEffect::getName() const { return mgr->params.name.c_str(); }

// It's just read from Manager's instance (which is fixed in memory, same as *this). No need for lock.
float AcesEffect::lifeTime() const { return mgr->lifeTime(); }
float EffectManager::lifeTime() const { return params.lifeTime; }

enum BuffCmdType
{
  FX_CMD_VELOCITY,
  FX_CMD_VELOCITY_SCALE_MIN_MAX,
  FX_CMD_HIDE,
  FX_CMD_EMM_TM,
  FX_CMD_SPAWN_RATE,
  FX_CMD_COLOR_MULT,
  FX_CMD_STOP,
  FX_CMD_LIGHT_RADIUS,
  FX_CMD_LIGHT_INTENSITY,
  FX_CMD_LIGHT_BOX,
  FX_CMD_WIND_SCALE,
  FX_CMD_GRAVITY_TM,
  FX_CMD_UNLOCK,
  FX_CMD_PAUSE_SOUND,
  FX_CMD_TM,
  FX_CMD_UNSET_EMITTER,
  FX_CMD_ENABLE_ACTIVE_QUERY,
  FX_CMD_FAKE_BRIGHTNESS_BACKGROUND_POS
};
struct BuffCommand
{
  static inline volatile bool immediateMode = true;
  union
  {
    EffectManager *mgr;
    mutable size_t imgr;
  };
  AcesEffect::FxId fxId;
  BuffCmdType type;
  union
  {
    TMatrix tm;
    Matrix3 m3;
    Point2 p2;
    Point3 p3;
    Color4 c4;
    struct
    {
      float f;
      uint32_t u;
      bool b;
    };
  };
};
static ThreadpoolLocalVector<BuffCommand> effectCommandQueue;

#if FX_CMD_STAT_DBG
#include <osApiWrappers/dag_stackHlp.h>
struct DbgFxCmdStat
{
  using CallStack = stackhelp::CallStackCaptureStore<12>;
  struct Stat
  {
    CallStack stack;
    volatile int count = 0;
  };
  OSReadWriteLock rwLock;
  eastl::vector_map<uint64_t, Stat> stat;
  DAGOR_NOINLINE void capture()
  {
    CallStack stack;
    stack.capture(2);
    const uint64_t hash = wyhash(stack.store, sizeof(void *) * stack.storeSize, 42);
    rwLock.lockRead();
    if (const auto it = stat.find(hash); DAGOR_LIKELY(it != stat.end()))
    {
      interlocked_increment(it->second.count);
      rwLock.unlockRead();
      return;
    }
    rwLock.unlockRead();
    rwLock.lockWrite();
    Stat &s = stat[hash];
    interlocked_increment(s.count);
    s.stack = stack;
    rwLock.unlockWrite();
  }
  void flushAndPrint()
  {
    rwLock.lockWrite();
    debug("printing fx cmd stats:");
    char stackStrBuf[4096];
    int total = 0;
    for (auto &[_, stat] : stat)
    {
      stackhlp_get_call_stack(stackStrBuf, 4096, stat.stack.store, stat.stack.storeSize);
      debug("  calls: %i\n%s", stat.count, stackStrBuf);
      total += stat.count;
      stat.count = 0;
    }
    stat.clear();
    debug("done printing fx cmd stats, total calls: %i", total);
    rwLock.unlockWrite();
  }
};
static DbgFxCmdStat dbg_fx_cmd_stat;
#endif

template <typename F>
static inline bool push_fx_cmd(EffectManager *mgr, AcesEffect::FxId fx_id, BuffCmdType cmdt, const F &pushf)
{
  if (interlocked_relaxed_load(BuffCommand::immediateMode))
    return false;

#if FX_CMD_STAT_DBG
  dbg_fx_cmd_stat.capture();
#endif

  auto &cmd = effectCommandQueue.push_back_noinit();
  cmd.mgr = mgr;
  cmd.fxId = fx_id;
  cmd.type = cmdt;
  pushf(cmd);

  return true;
}

namespace acesfx
{
extern void wait_fx_managers_update_job_done();
void set_immediate_mode(bool v, bool flush_cmds)
{
  wait_fx_managers_update_job_done();
  if (flush_cmds)
    EffectManager::updateCmdBuff();
  interlocked_relaxed_store(BuffCommand::immediateMode, v);
}
} // namespace acesfx

static void register_base_fx_res(BaseEffectObject *base_fx)
{
  bool forceRecreate = false;
  base_fx->setParam(_MAKE4C('PFXR'), &forceRecreate); // register dafx system
}

void EffectManager::validateDeleted(const BaseEffect &fx)
{
#if DAGOR_DBGLEVEL > 0
  // G_ASSERT(!(fx.flags & AcesEffect::IS_MARK_AS_DELETED));
  if (fx.flags & AcesEffect::IS_MARK_AS_DELETED)
    logerr("fx: %s is used after marked for deletion", params.name.c_str());
#else
  G_UNUSED(fx);
#endif
}

void EffectManager::updateCachedFlags(const BaseEffect &fx)
{
  if (fx.extFx != nullptr)
    // keep ACTIVE_QUERY_ENABLED flag always set to avoid error in isActive and because it is set once
    fx.extFx->cachedFlags = fx.flags | (fx.extFx->cachedFlags & AcesEffect::ACTIVE_QUERY_ENABLED);
}

void EffectManager::destroyEffect(AcesEffect::FxId fx_id, BaseEffect &fx)
{
#if DAGOR_DBGLEVEL > 0
  if (fx.flags & AcesEffect::IS_LOCKED)
  {
    logerr("fx (%s/%s) is still locked during its own destruction", params.name, params.resName);
    debug_dump_stack();
  }
#endif
  destroyFxSoundExt(fx.soundId, true, true);
  destroyLight(fx);
  del_it(fx.obj);
  fxList.destroyReference(fx_id);
}

void EffectManager::setFxTm(BaseEffect &fx, const TMatrix &tm, bool is_emitter_tm)
{
#if DAGOR_DBGLEVEL > 0
  if (check_nan(tm.getcol(0).lengthSq()) || check_nan(tm.getcol(1).lengthSq()) || check_nan(tm.getcol(2).lengthSq()) ||
      check_nan(tm.getcol(3).lengthSq()))
    logerr("fx : NaN in fx tm (%s/%s)", params.name, params.resName);

  if (tm.getcol(3).lengthSq() < 0.0001f)
  {
    logerr("%s : zero pos for fx: %s/%s", __FUNCTION__, params.name, params.resName);
    debug_dump_stack();
  }
#endif
  validateDeleted(fx);

  fx.tm = tm;
  if (fx.extFx != nullptr)
    fx.extFx->cachedPos = fx.tm.getcol(3);
  if (fx.pendingId >= 0)
  {
    PendingData &pd = pendingList[fx.pendingId];
    pd.isEmitterTm = is_emitter_tm;
    return;
  }
  G_ASSERT(fx.obj);

  Point4 pos = Point4::xyzV(fx.tm.getcol(3), fx.distanceOffsetSq);
  fx.obj->setParam(_MAKE4C('PFXP'), &pos);

  if (is_emitter_tm)
    fx.obj->setEmitterTm(&tm);
  else
    fx.obj->setTm(&tm);

  setFxTmSoundExt(fx);
}

void EffectManager::setFxVisibility(BaseEffect &fx, uint32_t visibility, bool force)
{
  validateDeleted(fx);
  if (visibility == fx.visibility && !force)
    return;

  fx.visibility = visibility;
  updateCachedFlags(fx);
  if (fx.pendingId >= 0 && !force)
    return;

  G_ASSERT(fx.obj);
  fx.obj->setParam(_MAKE4C('PFXV'), fx.visibility ? &fx.visibility : nullptr);
  if (fx.lightId >= 0)
    setFxLight(fx);
}

void EffectManager::setFxFakeBrightnessBackgroundPos(BaseEffect &fx, const Point3 &backgroundPos)
{
  validateDeleted(fx);
  if (fx.pendingId < 0)
    fx.obj->setFakeBrightnessBackgroundPos(&backgroundPos);
  else
    pendingList[fx.pendingId].fakeBrightnessBackgroundPos = backgroundPos;
}

void EffectManager::setFxVelocity(BaseEffect &fx, const Point3 &vel)
{
  validateDeleted(fx);
  if (fx.pendingId < 0)
    fx.obj->setVelocity(&vel);
  else
  {
    pendingList[fx.pendingId].velocity = vel;
    pendingList[fx.pendingId].velocitySet = true;
  }

  setFxVelocitySoundExt(fx, vel);
}

void EffectManager::setFxVelocityScaleMinMax(BaseEffect &fx, const Point2 &value)
{
  validateDeleted(fx);
  if (fx.pendingId < 0)
    fx.obj->setVelocityScaleMinMax(&value);
  else
  {
    pendingList[fx.pendingId].velocityScaleMinMax = value;
    pendingList[fx.pendingId].velocityScaleMinMaxSet = true;
  }
}

void EffectManager::setFxSpawnRate(BaseEffect &fx, float value)
{
  validateDeleted(fx);
  if (fx.pendingId < 0)
    fx.obj->setSpawnRate(&value);
  else
    pendingList[fx.pendingId].spawnRate = value;
}

void EffectManager::setFxLightRadiusMultiplier(BaseEffect &fx, float multiplier)
{
  if (fx.pendingId < 0)
  {
    // NOTE: if we're in the pending list we cannot check params.lightEnabled, because it might
    // not have been set yet by initAsync(). If we are pending we just always set this multiplier
    // on the pending list and potentially discard it later when we call this method again
    // while not pending.
    if (params.lightEnabled)
      lightList[fx.lightId].radiusMultiplier = multiplier;
  }
  else
    pendingList[fx.pendingId].lightRadiusMultiplier = multiplier;
}

void EffectManager::setFxLightIntensity(BaseEffect &fx, float intensity)
{
  if (fx.pendingId < 0)
  {
    if (params.lightEnabled)
      lightList[fx.lightId].intensity = intensity;
  }
  else
    pendingList[fx.pendingId].lightIntensity = intensity;
}

void EffectManager::setFxLightBox(BaseEffect &fx, const TMatrix &box)
{
  if (fx.pendingId < 0)
  {
    if (params.lightEnabled)
      lightList[fx.lightId].box = box;
  }
  else
    pendingList[fx.pendingId].lightBox = box;
}

void EffectManager::setFxWindScale(BaseEffect &fx, float scale)
{
  if (fx.pendingId < 0)
    fx.obj->setWindScale(&scale);
  else
    pendingList[fx.pendingId].windScale = scale;
}

void EffectManager::setFxColorMult(BaseEffect &fx, const Color4 &value)
{
  validateDeleted(fx);
  if (fx.pendingId < 0)
    fx.obj->setColor4Mult(&value);
  else
    pendingList[fx.pendingId].colorMult = value;
}

void EffectManager::stopFx(BaseEffect &fx)
{
  validateDeleted(fx);
  unsetFxEmitter(fx);
  fx.flags &= ~AcesEffect::IS_LOCKED;
  fx.flags |= AcesEffect::IS_MARK_AS_DELETED;
  updateCachedFlags(fx);
}

void EffectManager::unsetFxEmitter(BaseEffect &fx)
{
  validateDeleted(fx);

  if (fx.pendingId < 0)
  {
    setFxSpawnRate(fx, 0.f);
    destroyFxSoundExt(fx.soundId, true, false);

    bool lightFXUnsetEmitter = true;
    fx.obj->setParam(_MAKE4C('LFXU'), &lightFXUnsetEmitter);
  }
  else
  {
    pendingList[fx.pendingId].unsetEmitter = true;
  }
}

int EffectManager::getAliveFxCount() const { return fxList.totalSize() - fxList.freeIndicesSize(); }

EffectManager::EffectManager(const char *_name, const DataBlock *_blk, const char *effect_name, acesfx::PrefetchType prefetch_type)
{
  G_ASSERT(_name && *_name);
  G_ASSERT(!_blk || _blk->isValid());
#if DAGOR_DBGLEVEL > 0
  G_ASSERTF(params.name.empty(), "Effect manager '%s/%s' is already initialized", _name, params.name.c_str());
#endif

  params.sfxPath[0] = params.sfxEvent[0] = '\0';
  params.name = _name;
  params.resName = effect_name;
  blk = _blk ? *_blk : DataBlock();
  asyncLoad.mgr = this;

  G_ASSERT_LOG(effect_name && *effect_name, "Effect name is not specified for '%s'", _name);
  if (!(effect_name && *effect_name))
  {
    interlocked_release_store(atomicInitialized, EffectManager::LOAD_FAILED);
    return;
  }

  params.distanceOffset = blk.getReal("sortDistanceOffset", 0.f);
  params.lifeTime = blk.getReal("lifeTime", 0.f);
  params.soundEnableDistanceSq = blk.getReal("soundEnableDistance", -1.f);
  if (params.soundEnableDistanceSq > 0)
    params.soundEnableDistanceSq *= params.soundEnableDistanceSq;
  params.maxNumInstances = -1;
  params.spawnRangeLimit = 0;

  loadSoundParamsExt();

  effectBase = nullptr;

  debug("fx: loading: %s", _name);
  bool allowAsync = acesfx::g_async_load || prefetch_type == acesfx::PrefetchType::FORCE_ASYNC;
  if (allowAsync && prefetch_type != acesfx::PrefetchType::FORCE_SYNC)
  {
    cpujobs::add_job(ecs::get_common_loading_job_mgr(), &asyncLoad, /*prepend*/ false, /*need_release*/ false);
  }
  else
  {
    asyncLoad.doJob();
    register_base_fx_res(effectBase);
    interlocked_release_store(atomicInitialized, READY); // sync prefetch guarantee that fx is ready and we dont need to wait for the
                                                         // first update
  }
}

EffectManager::~EffectManager() { shutdown(); }

void EffectManagerAsyncLoad::doJob()
{
  mgr->initAsync();
  interlocked_release_store(mgr->atomicInitialized, mgr->effectBase ? EffectManager::LOADED : EffectManager::LOAD_FAILED);
}

void EffectManager::initAsync()
{
  effectBase = (BaseEffectObject *)get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(params.resName.c_str()), EffectGameResClassId);
  if (!effectBase)
  {
    logerr("Effect resource '%s' referenced in '%s' not found", params.resName.c_str(), params.name.c_str());
    params.maxNumInstances = 0;
    return;
  }

  Point2 onePointParams(0, 0);
  if (!effectBase->getParam(_MAKE4C('FXLX'), &onePointParams))
    onePointParams = {0, 0};

  if (onePointParams.x <= 0 || onePointParams.y <= 0)
  {
    onePointParams = {1, 1};
    logerr("wrong one point number/radius for '%s'", params.name.c_str());
  }

  params.onePointNumber = onePointParams.x;
  params.onePointRadius = onePointParams.y;

  effectBase->getParam(_MAKE4C('FXLR'), &params.spawnRangeLimit);
  params.spawnRangeLimit *= params.spawnRangeLimit;
  effectBase->getParam(_MAKE4C('FXLM'), &params.maxNumInstances);
  if (params.maxNumInstances <= 0)
  {
    logerr("Wrong numInstances number for effect manager '%s'", params.name.c_str());
    params.maxNumInstances = 1;
  }

  initLightEnabledExt();
}

void EffectManager::createFxRes(AcesEffect::FxId fx_id, const Point3 &pos)
{
  BaseEffectObject *obj = (BaseEffectObject *)effectBase->clone();
  G_ASSERT_RETURN(obj, );

  BaseEffect *fx = fxList.get(fx_id);
  G_ASSERT_RETURN(fx, );

  // delayed cmd buffer is executed before fxmgr::update, so instance could be stopped before creation
  bool markAsDeleted = fx->flags & AcesEffect::IS_MARK_AS_DELETED;
  fx->flags &= ~AcesEffect::IS_MARK_AS_DELETED;
  fx->obj = obj;
  if (params.lightEnabled)
  {
    fx->lightId = lightList.size();
    lightList.emplace_back(fx_id, fx->obj);
    fx->obj->getParam(_MAKE4C('FXSH'), &lightList[fx->lightId].shadowParams);
    setFxLight(*fx);
  }

  BaseEffectEnabled dafxe = {true, length(viewPos - pos) * invMagnification};
  fx->obj->setParam(_MAKE4C('PFXE'), &dafxe);
  fx->obj->getParam(_MAKE4C('FXID'), &fx->iid);

  if (fx->pendingId >= 0)
  {
    int pendId = fx->pendingId;
    PendingData &pe = pendingList[pendId];
    fx->pendingId = -1;

    if (pe.velocitySet)
      setFxVelocity(*fx, pe.velocity);
    if (pe.velocityScaleMinMaxSet)
      setFxVelocityScaleMinMax(*fx, pe.velocityScaleMinMax);

    setFxSpawnRate(*fx, markAsDeleted ? 0.f : pe.spawnRate);
    setFxLightRadiusMultiplier(*fx, pe.lightRadiusMultiplier);
    setFxLightIntensity(*fx, pe.lightIntensity);
    setFxLightBox(*fx, pe.lightBox);
    if (pe.windScale >= 0)
      setFxWindScale(*fx, pe.windScale);
    setFxGravityTm(*fx, pe.gravityTm);
    setFxColorMult(*fx, pe.colorMult);

    setFxTm(*fx, fx->tm, pe.isEmitterTm);
    setFxVisibility(*fx, fx->visibility, true);

    if (pe.unsetEmitter)
      unsetFxEmitter(*fx);

    if (warmupDt > 0)
      fx->obj->setParam(_MAKE4C('PFXG'), &warmupDt);

    BaseEffect *swapFx = remove_and_fix_subfx(pendId, pendingList, fxList);
    if (swapFx)
      swapFx->pendingId = pendId;
  }

  fx->flags |= markAsDeleted ? AcesEffect::IS_MARK_AS_DELETED : 0;
  updateCachedFlags(*fx);
}

void EffectManager::reset()
{
  OSSpinlockScopedLock lock(&mgrSLock);

  dag::Vector<AcesEffect::FxId, framemem_allocator> killList;
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (!fx)
      continue;

    if (fx->flags & AcesEffect::IS_LOCKED)
      logerr("fx (%s/%s) is still locked during fx reset!", params.name, params.resName);
    else
      killList.push_back(fxList.getRefByIdx(i));
  }

  for (AcesEffect::FxId id : killList)
  {
    BaseEffect *fx = fxList.get(id);
    G_ASSERT_CONTINUE(fx);
    del_it(fx->extFx);
  }
}

AcesEffect *EffectManager::startEffect(const Point3 &pos, bool lock, bool is_player, FxErrorType *perr, bool with_sound)
{
  int asyncState = interlocked_acquire_load(atomicInitialized);
  if (asyncState == LOAD_FAILED)
  {
    logerr("trying to create failed-to-load fx : %s, %s", params.name.c_str(), params.resName.c_str());
    if (perr)
      *perr = FxErrorType::FX_ERR_INVALID_TYPE;
    return nullptr;
  }

  if (asyncState == READY && hasEnoughEffectsAtPoint(is_player, pos))
  {
    if (perr)
      *perr = FX_ERR_HAS_ENOUGH;
    return nullptr;
  }

  int currentCount = fxList.totalSize() - fxList.freeIndicesSize();
  if (asyncState == READY && currentCount > params.maxNumInstances)
  {
    // find 1 non-visibile, non-locked fx
    for (int i = 0; i < fxList.totalSize(); ++i)
    {
      BaseEffect *fx = fxList.getByIdx(i);
      if (!fx || (fx->flags & AcesEffect::IS_LOCKED))
        continue;

      bool visible = false;
      if (!fx->obj->getParam(_MAKE4C('PFXC'), &visible) || visible)
        continue;

      del_it(fx->extFx);
      currentCount--;
      break;
    }

    if (!is_player && currentCount > params.maxNumInstances)
    {
      logerr("fx are over the limit : %s/%s %d/%d", params.name.c_str(), params.resName.c_str(), currentCount, params.maxNumInstances);
      if (perr)
        *perr = FxErrorType::FX_ERR_HAS_ENOUGH;
      return nullptr;
    }
  }

  AcesEffect *extFx = new AcesEffect();
  AcesEffect::FxId fxId = fxList.emplaceOne();
  BaseEffect *fx = fxList.get(fxId);
  extFx->fxId = fxId;
  extFx->mgr = this;
  G_ASSERT(fx->flags == 0);
  fx->flags |= lock ? AcesEffect::IS_LOCKED : 0;
  fx->flags |= is_player ? AcesEffect::IS_PLAYER : 0;
  fx->flags |= AcesEffect::IS_ACTIVE;
  int rndSeed = (size_t(this) >> 4) | (uint32_t)fxId;
  fx->distanceOffsetSq = params.distanceOffset + RANDOM_DISTANCE_OFFSET * _frnd(rndSeed);
  fx->distanceOffsetSq *= fx->distanceOffsetSq;
  fx->extFx = extFx;
  updateCachedFlags(*fx);

  startEffectSoundExt(fxId, *fx, with_sound, pos);

  if (asyncState == READY)
  {
    createFxRes(fxId, pos);
  }
  else
  {
    fx->pendingId = pendingList.size();
    pendingList.emplace_back(fxId);
  }

  return extFx;
}


void EffectManager::updateStart(float dt)
{
  OSSpinlockScopedLock lock(&mgrSLock);

  updateStarted = false;

#if DAGOR_DBGLEVEL > 0
  if (soundList.size() > getAliveFxCount() || lightList.size() > getAliveFxCount() || pendingList.size() > getAliveFxCount())
    logerr("fx: mismatched descriptor lists: fx: %d, sounds: %d, lights: %d, pending: %d", getAliveFxCount(), soundList.size(),
      lightList.size(), pendingList.size());
#endif

  if (fxList.empty())
    return;

  updateFxSoundsExt(dt);

  int asyncState = interlocked_acquire_load(atomicInitialized);
  if (asyncState < LOADED)
  {
    warmupDt += asyncState == NOT_INITIALIZED ? dt : 0;
    updateStarted = false;
  }
  else
  {
    updateStarted = true;
  }
}


void EffectManager::updateLoading()
{
  if (!updateStarted)
    return;

  OSSpinlockScopedLock lock(&mgrSLock);

  int asyncState = interlocked_acquire_load(atomicInitialized);
  if (asyncState != LOADED)
  {
    updateStarted &= (asyncState & READY) ? true : false;
    return;
  }

  register_base_fx_res(effectBase);

  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx)
    {
      const Point3 &pos = fx->tm.getcol(3);
      if (fx->flags & AcesEffect::IS_LOCKED || !hasEnoughEffectsAtPoint(fx->flags & AcesEffect::IS_PLAYER, pos))
      {
        createFxRes(fxList.getRefByIdx(i), pos);
      }
      else // we only can query hasEnoughEffectsAtPoint after async job is done. so it we overshoot - destroy fx that are over the
           // limit
      {
        G_ASSERT(fx->pendingId >= 0);
        BaseEffect *swapFx = remove_and_fix_subfx(fx->pendingId, pendingList, fxList);
        if (swapFx)
          swapFx->pendingId = fx->pendingId;
        del_it(fx->extFx);
      }
    }
  }

  G_ASSERT(pendingList.empty());
  pendingList.shrink_to_fit();

  warmupDt = 0;
  interlocked_release_store(atomicInitialized, READY);
}

void EffectManager::updateLights(float dt, FxLightParams *out_big_light)
{
  if (!updateStarted)
    return;

  OSSpinlockScopedLock lock(&mgrSLock);

  FxLightParams biggestLight = {{0, 0, 0}, {0, 0, 0}, 0, 0};

  for (LightEffect &le : lightList)
  {
    le.obj->update(dt);
    if (le.extLightId < 0) // hidden
      continue;
    Point3 *lightPos = static_cast<Point3 *>(le.obj->getParam(HUID_LIGHT_POS, nullptr));
    Color4 *lightParams = static_cast<Color4 *>(le.obj->getParam(HUID_LIGHT_PARAMS, NULL));
    G_ASSERT(lightPos && lightParams);

    le.pos = *lightPos;
    le.params = Point4::xyzV(Point3::rgb(*lightParams) * le.intensity, lightParams->a * le.radiusMultiplier);

    bool isCloudLight = *(bool *)le.obj->getParam(_MAKE4C('LFXC'), &isCloudLight);
    if (out_big_light && isCloudLight && biggestLight.radius < lightParams->a)
    {
      biggestLight.pos = *lightPos;
      biggestLight.color = Point3(lightParams->r, lightParams->g, lightParams->b);
      biggestLight.radius = lightParams->a;
      biggestLight.power = *(float *)le.obj->getParam(_MAKE4C('LFXS'), &biggestLight.power);
    }
  }
  if (out_big_light)
    *out_big_light = biggestLight;
}

void EffectManager::updateActiveAndFree(int active_query_sparse_step)
{
  if (!updateStarted)
    return;

  OSSpinlockScopedLock lock(&mgrSLock);

  dag::Vector<BaseEffect *, framemem_allocator> killList;
  int frameNo = ::dagor_frame_no();
  dag::Vector<BaseEffect *, framemem_allocator> fxs;
  dag::Vector<dafx::InstanceId, framemem_allocator> iids;
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (!fx)
      continue;

    int flags = fx->flags;
    bool isLocked = flags & AcesEffect::IS_LOCKED;
    bool activeQuery = flags & AcesEffect::ACTIVE_QUERY_ENABLED;

    if (!activeQuery && ((frameNo + i) % active_query_sparse_step) != 0)
      continue;

    if (!isLocked || activeQuery)
    {
      fxs.push_back(fx);
      iids.push_back(fx->iid);
    }
  }

  dag::ConstSpan<bool> results = dafx::query_instances_renderable_active(acesfx::g_dafx_ctx, make_span_const(iids));
  G_ASSERT(results.size() == fxs.size());
  for (int i = 0; i < fxs.size(); ++i)
  {
    BaseEffect *fx = fxs[i];
    int flags = fx->flags;
    bool isLocked = flags & AcesEffect::IS_LOCKED;
    bool isActive = results[i];
    bool activeQuery = flags & AcesEffect::ACTIVE_QUERY_ENABLED;
    if (activeQuery)
    {
      flags &= ~AcesEffect::IS_ACTIVE;
      flags |= isActive ? AcesEffect::IS_ACTIVE : 0;
      fx->flags = flags;
      updateCachedFlags(*fx);
    }

    if (!isActive && !isLocked)
      killList.push_back(fx);
  }

  for (BaseEffect *fx : killList)
  {
    G_ASSERT_CONTINUE(fx);
    del_it(fx->extFx);
  }
}


void EffectManager::force_async_load_complete()
{
  if (DAGOR_UNLIKELY(interlocked_acquire_load(atomicInitialized) == NOT_INITIALIZED))
    while (interlocked_acquire_load(atomicInitialized) == NOT_INITIALIZED)
      sleep_msec(1);
}

void EffectManager::shutdown()
{
  while (cpujobs::is_job_running(ecs::get_common_loading_job_mgr(), &asyncLoad)) // job itself can still be running, even if atomic is
                                                                                 // done
    sleep_msec(1);
  interlocked_release_store(atomicInitialized, NOT_INITIALIZED);

  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx)
      del_it(fx->extFx);
  }

  pendingList.clear();
  lightList.clear();
  soundList.clear();
  fxList.clear();

  if (effectBase)
  {
    release_game_resource((GameResource *)effectBase);
    effectBase = nullptr;
  }
}

void EffectManager::getFxMatrices(dag::Vector<TMatrix, framemem_allocator> &matrices)
{
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx)
      matrices.push_back(fx->tm);
  }
}

void EffectManager::getDebugInfo(dag::Vector<Point3, framemem_allocator> &positions, dag::Vector<int, framemem_allocator> &elems,
  dag::Vector<eastl::string_view, framemem_allocator> &names)
{
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx)
    {
      int count = 0;
      if (fx->obj)
        fx->obj->getParam(_MAKE4C('FXIC'), &count);

      elems.push_back(count);
      positions.push_back(fx->tm.getcol(3));
      names.push_back(params.name);
    }
  }
}

void EffectManager::getResDebugInfo(dag::Vector<Point3, framemem_allocator> &positions,
  dag::Vector<eastl::string, framemem_allocator> &out_names, dag::Vector<int, framemem_allocator> &out_elems)
{
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (!fx || !fx->obj)
      continue;

    dafx::InstanceId iid;
    fx->obj->getParam(_MAKE4C('FXID'), &iid);

    eastl::vector<eastl::string> names;
    eastl::vector<int> elems;
    eastl::vector<int> lods;

    dafx::gather_instance_stats(acesfx::g_dafx_ctx, iid, names, elems, lods);
    for (int i = 0; i < names.size(); ++i)
    {
      eastl::string tmp;
      int l = lods[i];
      if (l > 0)
        tmp.append_sprintf("%s__lod:%d", names[i].c_str(), l);
      else
        tmp = names[i];

      out_names.push_back(tmp);
      out_elems.push_back(elems[i]);
      positions.push_back(fx->tm.getcol(3));
    }
  }
}

int EffectManager::getStats(eastl::string &out)
{
  int locked = 0;
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx && (fx->flags & AcesEffect::IS_LOCKED))
      locked++;
  }

  out.sprintf("%s (%s), total: %d, max: %d, locked: %d, lights: %d, sounds: %d", params.name.c_str(), params.resName.c_str(),
    fxList.totalSize() - fxList.freeIndicesSize(), params.maxNumInstances, locked, lightList.size(), soundList.size());

  return fxList.totalSize() - fxList.freeIndicesSize();
}

bool EffectManager::loaded() const { return interlocked_acquire_load(atomicInitialized) >= LOADED; }

bool EffectManager::preCheckCanSpawnAtPoint(bool is_player, const Point3 &pos) const
{
#if DAGOR_DBGLEVEL > 0
  int asyncState = interlocked_relaxed_load(atomicInitialized);
  if (asyncState < LOADED)
    logerr("Wrong asyncState=%d for hasEnoughEffectsAtPoint()", asyncState);

  if (acesfx::enableUnlimitedTestingMode)
    return true;
#endif

  if (!is_player && params.spawnRangeLimit > 0 && lengthSq(viewPos - pos) * invMagnificationSq > params.spawnRangeLimit)
    return false;
  return true;
}

bool EffectManager::hasEnoughEffectsAtPoint(bool is_player, const Point3 &pos)
{
#if DAGOR_DBGLEVEL > 0
  int asyncState = interlocked_relaxed_load(atomicInitialized);
  if (asyncState < LOADED)
    logerr("Wrong asyncState=%d for hasEnoughEffectsAtPoint()", asyncState);

  if (acesfx::enableUnlimitedTestingMode)
    return false;
#endif

  if (!preCheckCanSpawnAtPoint(is_player, pos))
    return true;

  if (params.onePointNumber == 0)
    return false;

  int aliveFxCount = getAliveFxCount() - pendingList.size(); // We subtract pendingList size for correct use during
                                                             // EffectManager::updateLoading.
  G_ASSERT_RETURN(aliveFxCount >= 0, false);
  if (aliveFxCount >= params.maxNumInstances)
    return true;
  if (aliveFxCount < params.onePointNumber)
    return false;

  unsigned int numAtPoint = 0;
  float r2 = params.onePointRadius * params.onePointRadius;
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (!fx || fx->pendingId >= 0)
      continue;
    Point3 testPos = fx->tm.getcol(3);
    if (lengthSq(testPos - pos) < r2)
    {
      numAtPoint++;
      if (numAtPoint >= params.onePointNumber)
        return true;
    }
  }

  return false;
}

void EffectManager::killEffectsInSphere(const BSphere3 &bsph)
{
  OSSpinlockScopedLock lock(&mgrSLock);
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (!fx)
      continue;

    if (!(fx->flags & AcesEffect::IS_MARK_AS_DELETED) && (bsph & fx->tm.getcol(3)))
      unsetFxEmitter(*fx);
  }
}

void EffectManager::stopAllEffects()
{
  OSSpinlockScopedLock lock(&mgrSLock);
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx && !(fx->flags & AcesEffect::IS_MARK_AS_DELETED))
      unsetFxEmitter(*fx);
  }
}

bool EffectManager::hasLockedEffects()
{
  OSSpinlockScopedLock lock(&mgrSLock);
  for (int i = 0; i < fxList.totalSize(); ++i)
  {
    BaseEffect *fx = fxList.getByIdx(i);
    if (fx && (fx->flags & AcesEffect::IS_LOCKED))
      return true;
  }

  return false;
}

void EffectManager::setFxTmBuff(AcesEffect::FxId fx_id, const TMatrix &tm, bool is_emitter_tm)
{
  if (push_fx_cmd(this, fx_id, is_emitter_tm ? FX_CMD_EMM_TM : FX_CMD_TM, [&](BuffCommand &cmd) { cmd.tm = tm; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxTm(*e, tm, is_emitter_tm);
  mgrSLock.unlock();
}

void EffectManager::setFxVisibilityBuff(AcesEffect::FxId fx_id, uint32_t visibility, bool force)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_HIDE, [&](BuffCommand &cmd) {
        cmd.u = visibility;
        cmd.b = force;
      }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxVisibility(*e, visibility, force);
  mgrSLock.unlock();
}

void EffectManager::unlockFxBuff(AcesEffect::FxId fx_id)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_UNLOCK, [&](BuffCommand &) {}))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
  {
    e->flags &= ~AcesEffect::IS_LOCKED;
    updateCachedFlags(*e);
  }
  mgrSLock.unlock();
}

void EffectManager::setFakeBrightnessBackgroundPosBuff(AcesEffect::FxId fx_id, const Point3 &value)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_FAKE_BRIGHTNESS_BACKGROUND_POS, [&](BuffCommand &cmd) { cmd.p3 = value; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxFakeBrightnessBackgroundPos(*e, value);
  mgrSLock.unlock();
}

void EffectManager::setFxVelocityBuff(AcesEffect::FxId fx_id, const Point3 &vel)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_VELOCITY, [&](BuffCommand &cmd) { cmd.p3 = vel; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxVelocity(*e, vel);
  mgrSLock.unlock();
}

void EffectManager::setFxVelocityScaleMinMaxBuff(AcesEffect::FxId fx_id, const Point2 &value)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_VELOCITY_SCALE_MIN_MAX, [&](BuffCommand &cmd) { cmd.p2 = value; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxVelocityScaleMinMax(*e, value);
  mgrSLock.unlock();
}

void EffectManager::setFxSpawnRateBuff(AcesEffect::FxId fx_id, float value)
{
  // TODO: uncomment, when all values for setSpawnRate will be valid
  // if (value < 0.f || value > 1.f)
  //   LOGERR_ONCE("invalid fx:setSpawnRate %.8f (%s/%s)", value, params.name, params.resName);
  // value = clamp(value, 0.f, 1.f);

  if (push_fx_cmd(this, fx_id, FX_CMD_SPAWN_RATE, [&](BuffCommand &cmd) { cmd.f = value; }))
    return;
  else
    mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxSpawnRate(*e, value);
  mgrSLock.unlock();
}

void EffectManager::setFxLightRadiusMultiplierBuff(AcesEffect::FxId fx_id, float multiplier)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_LIGHT_RADIUS, [&](BuffCommand &cmd) { cmd.f = multiplier; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxLightRadiusMultiplier(*e, multiplier);
  mgrSLock.unlock();
}

void EffectManager::setFxLightIntensityBuff(AcesEffect::FxId fx_id, float intensity)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_LIGHT_INTENSITY, [&](BuffCommand &cmd) { cmd.f = intensity; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxLightIntensity(*e, intensity);
  mgrSLock.unlock();
}

void EffectManager::setFxLightBoxBuff(AcesEffect::FxId fx_id, const TMatrix &box)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_LIGHT_BOX, [&](BuffCommand &cmd) { cmd.tm = box; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxLightBox(*e, box);
  mgrSLock.unlock();
}

void EffectManager::setFxWindScaleBuff(AcesEffect::FxId fx_id, float scale)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_WIND_SCALE, [&](BuffCommand &cmd) { cmd.f = scale; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxWindScale(*e, scale);
  mgrSLock.unlock();
}

void EffectManager::setFxGravityTm(BaseEffect &fx, const Matrix3 &tm)
{
  if (fx.pendingId < 0)
    fx.obj->setParam(_MAKE4C('GZTM'), (void *)&tm);
  else
    pendingList[fx.pendingId].gravityTm = tm;
}

void EffectManager::setFxGravityTmBuff(AcesEffect::FxId fx_id, const Matrix3 &tm)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_GRAVITY_TM, [&](BuffCommand &cmd) { cmd.m3 = tm; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    setFxGravityTm(*e, tm);
  mgrSLock.unlock();
}

void AcesEffect::setGravityTm(const Matrix3 &tm) { mgr->setFxGravityTmBuff(fxId, tm); }

void EffectManager::setFxColorMultBuff(AcesEffect::FxId fx_id, const Color4 &value)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_COLOR_MULT, [&](BuffCommand &cmd) { cmd.c4 = value; }))
    return;
  mgrSLock.lock();
  EffectManager::BaseEffect *e = fxList.get(fx_id);
  if (e)
    setFxColorMult(*e, value);
  mgrSLock.unlock();
}

void EffectManager::unsetFxEmitterBuff(AcesEffect::FxId fx_id)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_UNSET_EMITTER, [&](BuffCommand &) {}))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    unsetFxEmitter(*e);
  mgrSLock.unlock();
}

void EffectManager::stopFxBuff(AcesEffect::FxId fx_id)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_STOP, [&](BuffCommand &) {}))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    stopFx(*e);
  mgrSLock.unlock();
}

void EffectManager::pauseFxSoundBuff(AcesEffect::FxId fx_id, bool paused)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_PAUSE_SOUND, [&](BuffCommand &cmd) { cmd.b = paused; }))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
    pauseFxSoundExt(*e, paused);
  mgrSLock.unlock();
}

void EffectManager::enableActiveQueryBuff(AcesEffect::FxId fx_id)
{
  if (push_fx_cmd(this, fx_id, FX_CMD_ENABLE_ACTIVE_QUERY, [&](BuffCommand &) {}))
    return;
  mgrSLock.lock();
  if (EffectManager::BaseEffect *e = fxList.get(fx_id))
  {
    e->flags |= AcesEffect::ACTIVE_QUERY_ENABLED;
    updateCachedFlags(*e);
  }
  mgrSLock.unlock();
}

void EffectManager::updateCmdBuff()
{
  TIME_PROFILE(updateCmdBuff)
  for (auto &cmd : effectCommandQueue)
  {
    G_FAST_ASSERT(cmd.mgr);
    EffectManager::BaseEffect *e = cmd.mgr->fxList.get(cmd.fxId);
    if (!e || (e->flags & AcesEffect::IS_MARK_AS_DELETED))
      continue;
    switch (cmd.type)
    {
      case FX_CMD_TM: cmd.mgr->setFxTm(*e, cmd.tm, false); break;
      case FX_CMD_EMM_TM: cmd.mgr->setFxTm(*e, cmd.tm, true); break;
      case FX_CMD_HIDE: cmd.mgr->setFxVisibility(*e, cmd.u, cmd.b); break;
      case FX_CMD_UNLOCK:
        e->flags &= ~AcesEffect::IS_LOCKED;
        updateCachedFlags(*e);
        break;
      case FX_CMD_FAKE_BRIGHTNESS_BACKGROUND_POS: cmd.mgr->setFxFakeBrightnessBackgroundPos(*e, cmd.p3); break;
      case FX_CMD_VELOCITY: cmd.mgr->setFxVelocity(*e, cmd.p3); break;
      case FX_CMD_VELOCITY_SCALE_MIN_MAX: cmd.mgr->setFxVelocityScaleMinMax(*e, cmd.p2); break;
      case FX_CMD_SPAWN_RATE: cmd.mgr->setFxSpawnRate(*e, cmd.f); break;
      case FX_CMD_LIGHT_RADIUS: cmd.mgr->setFxLightRadiusMultiplier(*e, cmd.f); break;
      case FX_CMD_LIGHT_INTENSITY: cmd.mgr->setFxLightIntensity(*e, cmd.f); break;
      case FX_CMD_LIGHT_BOX: cmd.mgr->setFxLightBox(*e, cmd.tm); break;
      case FX_CMD_WIND_SCALE: cmd.mgr->setFxWindScale(*e, cmd.f); break;
      case FX_CMD_GRAVITY_TM: cmd.mgr->setFxGravityTm(*e, cmd.m3); break;
      case FX_CMD_COLOR_MULT: cmd.mgr->setFxColorMult(*e, cmd.c4); break;
      case FX_CMD_STOP:
      {
        OSSpinlockScopedLock lock(&cmd.mgr->mgrSLock);
        cmd.mgr->stopFx(*e);
      }
      break;
      case FX_CMD_UNSET_EMITTER:
      {
        OSSpinlockScopedLock lock(&cmd.mgr->mgrSLock);
        cmd.mgr->unsetFxEmitter(*e);
      }
      break;
      case FX_CMD_PAUSE_SOUND: cmd.mgr->pauseFxSoundExt(*e, cmd.b); break;
      case FX_CMD_ENABLE_ACTIVE_QUERY:
        e->flags |= AcesEffect::ACTIVE_QUERY_ENABLED;
        updateCachedFlags(*e);
        break;
      default: G_FAST_ASSERT(0); break;
    }
#if DAGOR_DBGLEVEL > 0 && _TARGET_PC
    cmd.imgr |= size_t(1) << (sizeof(size_t) * CHAR_BIT - 1); // to catch double cmd runs
#endif
  }
  // To consider: may be upper limit capacity here?
  effectCommandQueue.clear();

#if FX_CMD_STAT_DBG
  dbg_fx_cmd_stat.flushAndPrint();
#endif
}

void EffectManager::clearCmdBuff() { effectCommandQueue.clear_and_shrink(); }