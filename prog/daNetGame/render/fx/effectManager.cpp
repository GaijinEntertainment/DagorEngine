// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/fx/effectManager.h"
#include <stdio.h> // _snprintf
#include "render/fx/fx.h"
#include "render/fx/fxRenderTags.h"
#include <render/omniLight.h>
#include "render/renderer.h"
#include "render/skies.h"
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_lookup.h>
#include <util/dag_convar.h>
#include <util/dag_console.h>

#include <fx/dag_baseFxClasses.h>
#include <gameRes/dag_gameResources.h>
#include <gameRes/dag_gameResSystem.h>
#include <math/dag_frustum.h>
#include <math/random/dag_random.h>

#include <ioSys/dag_dataBlock.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <gameRes/dag_stdGameRes.h>
#include <supp/dag_prefetch.h>
#include <perfMon/dag_cpuFreq.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <gamePhys/props/atmosphere.h>
#include <soundSystem/soundSystem.h>
#include <soundSystem/vars.h>
#include <camera/sceneCam.h>
#include <debug/dag_debug3d.h>
#include <dafxCompound_decl.h>
#include <math/dag_check_nan.h>

#define INSIDE_RENDERER 1
#include "../world/private_worldRenderer.h"

static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }

// #include "daGUI/dag_hudPrimitives.h"
#if DAGOR_DBGLEVEL > 0
#include <debug/dag_log.h>
#include <debug/dag_textMarks.h>
#endif
#include <math/dag_adjpow2.h>
#include <daFx/dafx.h>

#include "render/fx/renderTrans.h"

// extern Point3 get_wind_speed_at_land_surface(const Point3 &pos);
#if _TARGET_PC | _TARGET_C2 | _TARGET_SCARLETT
static constexpr int MIN_INSTANCES = 16;
#else
static constexpr int MIN_INSTANCES = 8;
#endif

#define debug(...) logmessage(_MAKE4C('_FX_'), __VA_ARGS__)

#define INVISIBLE_UPDATE_DIVIDER 7 // Not proportional to NUM_THREADS.
#define FAR_UPDATE_DIVIDER       3

#define RANDOM_DISTANCE_OFFSET 0.2

#define FX_LIST_PREFETCH_AHEAD 3

bool EffectManager::lightsEnabled = true;
EffectManager::ActiveShadowVec EffectManager::activeShadowVec;
static int maxActiveShadows = EffectManager::DEFAULT_ACTIVE_SHADOWS;
static ShadowsQuality shadowQuality = ShadowsQuality::SHADOWS_LOW;
CONSOLE_BOOL_VAL("fx", debug_active_shadows, false);
static constexpr float LIGHT_UPDATE_DISTANCE_THRESHOLD = 0.03f;

void EffectManager::LightWithShadow::init(int light_id)
{
  reset();

  lightHandle = light_id;
  shadowEnabled = ShadowEnabled::NOT_INITED;
}

EffectManager::LightWithShadow::operator bool() const { return bool(lightHandle); }

int EffectManager::LightWithShadow::getLightId() const { return int(lightHandle); }

void EffectManager::LightWithShadow::reset()
{
  releaseShadow(ShadowEnabled::NOT_INITED);
  lightHandle.reset();
}

void EffectManager::LightWithShadow::releaseShadow(ShadowEnabled shadow_enabled)
{
  G_ASSERT_RETURN(shadow_enabled == ShadowEnabled::OFF || shadow_enabled == ShadowEnabled::NOT_INITED, );

  if (shadowEnabled == ShadowEnabled::ON)
  {
    ActiveShadow *record = eastl::find_if(activeShadowVec.begin(), activeShadowVec.end(),
      [this](const ActiveShadow &sh) -> bool { return sh.lightWithShadow == this; });

    G_ASSERT(record != activeShadowVec.end());
    activeShadowVec.erase_unsorted(record);
    if (debug_active_shadows)
      console::print_d("FX: active_shadow_count=%d; max=%d", activeShadowVec.size(), maxActiveShadows);
  }

  shadowEnabled = shadow_enabled;
};

EffectManager::LightWithShadow::~LightWithShadow() { reset(); }

void EffectManager::LightWithShadow::activateShadow(
  const Point3 &pos, const LightShadowParams &lightShadowParams, const eastl::string &name)
{
  // Activate N nearest to the camera effects with shadow
  if (maxActiveShadows == 0 || shadowEnabled != ShadowEnabled::NOT_INITED)
    return;

  const Point3 &viewPos = get_cam_itm().getcol(3);
  float distanceSq = (pos - viewPos).lengthSq();

  // if we already have maxActiveShadows effect, try to find more far effect to disable light
  if (activeShadowVec.size() >= maxActiveShadows)
  {
    ActiveShadow *record = eastl::find_if(activeShadowVec.begin(), activeShadowVec.end(),
      [&viewPos, distanceSq](const ActiveShadow &e) -> bool { return (e.pos - viewPos).lengthSq() > distanceSq; });

    if (record == activeShadowVec.end())
    {
      // If current effect is more far then all active shadow effects, stop and return
      if (debug_active_shadows)
        console::print_d("FX: shadow for effect '%s' pos=%@ hasn't been activated, because active_shadow_count=%d reached max=%d",
          name.c_str(), pos, activeShadowVec.size(), maxActiveShadows);
      shadowEnabled = ShadowEnabled::OFF;
      return;
    }

    // Disable light for found effect (more far, than current)
    record->lightWithShadow->releaseShadow(ShadowEnabled::OFF);
    // Now activeShadowVec.size() == maxActiveShadows - 1
  }

  // activeShadowVec.size() < maxActiveShadows, so we can activate current effect
  activeShadowVec.push_back({this, pos});
  shadowEnabled = ShadowEnabled::ON;

  if (debug_active_shadows)
    console::print_d("FX: shadow for effect '%s' pos=%@ has been activated; active_shadow_count=%d; max=%d", name.c_str(), pos,
      activeShadowVec.size(), maxActiveShadows);

  get_renderer()->updateLightShadow(lightHandle, lightShadowParams);
}

extern bool register_dafx_compound_system(
  BaseEffectObject *src, uint32_t tags, FxRenderGroup group, const DataBlock &fx_blk, int sort_depth, const eastl::string &name);

int AcesEffect::getLightId() const { return mgr->allEffects[groupIdx].lights[fxIdx].getLightId(); }

AcesEffect::~AcesEffect()
{
  if (fx)
  {
    delete fx;
    fx = nullptr;
  }
  destroySound();
}

uint16_t &AcesEffect::getFlags() { return mgr->allEffects[groupIdx].flags[fxIdx]; }

bool AcesEffect::isAlive() { return !fx || (getFlags() & AcesEffect::IS_ALIVE); }

bool AcesEffect::queryIsActive() const
{
  bool isActive = true;
  if (fx)
    fx->getParam(HUID_ACES_IS_ACTIVE, &isActive);
  return isActive;
}

void AcesEffect::reset()
{
  pos.zero();
  velocity.zero();
  scale = 1.f;
  velocityScale = NAN;
  windScale = NAN;
  visibilityMask = 0;
  destroySound();

  if (fx)
    fx->setParam(HUID_ACES_RESET, NULL);
}

void AcesEffect::resetLight(int seed)
{
  G_ASSERT_RETURN(fx, );

  bool reset = true;
  if (*(bool *)fx->getParam(HUID_BURST, NULL))
    fx->setParam(HUID_BURST, &reset);
  fx->setParam(HUID_SEED, &seed);
  Color4 tmp(0.f, 0.f, 0.f);
  fx->setParam(HUID_LIGHT_PARAMS, &tmp);
}

bool AcesEffect::isBurst() const
{
  if (!fx)
    return false;
  bool isBurst = false;
  fx->getParam(HUID_BURST, (void *)&isBurst);
  return isBurst;
}

void AcesEffect::setVelocity(const Point3 &vel)
{
  velocity = vel;
  if (fx)
    fx->setVelocity(&velocity);
}

void AcesEffect::setVelocityScale(const float &vel_scale)
{
  velocityScale = vel_scale;
  if (fx)
    fx->setVelocityScale(&vel_scale);
}

void AcesEffect::setWindScale(const float &wind_scale)
{
  windScale = wind_scale;
  if (fx)
    fx->setWindScale(&wind_scale);
}

void AcesEffect::setFxScale(float fx_scale)
{
  fxScale = fx_scale;
  isFxScaleSet = true;
  if (fx)
    fx->setParam(_MAKE4C('SCAL'), (void *)&fxScale);
}

void AcesEffect::setRestrictionBox(const TMatrix &box)
{
  G_ASSERT_RETURN(fxIdx != INVALID_FX_ID && groupIdx != INVALID_FX_ID, );
  mgr->allEffects[groupIdx].checkAndInitRestrictionBoxes();
  mgr->allEffects[groupIdx].restrictionBoxes[fxIdx] = box;
}

const TMatrix *AcesEffect::getRestrictionBox() const
{
  G_ASSERT_RETURN(fxIdx != INVALID_FX_ID && groupIdx != INVALID_FX_ID, nullptr);
  if (!mgr->allEffects[groupIdx].restrictionBoxes)
    return nullptr;
  return &mgr->allEffects[groupIdx].restrictionBoxes[fxIdx];
}

void AcesEffect::setGravityTm(const Matrix3 &tm)
{
  gravityTm = tm;
  if (fx)
    fx->setParam(_MAKE4C('GZTM'), (void *)&tm);
}

float AcesEffect::getFxScale() const
{
  if (fx)
    fx->getParam(_MAKE4C('SCAL'), (void *)&fxScale);

  return fxScale;
}

void AcesEffect::setSpawnRate(float rate)
{
  spawnRate = rate;
  if (fx)
    fx->setSpawnRate(&rate);
}

void AcesEffect::setLightFadeout(float light_fadeout) { lightFadeout = light_fadeout; }

void AcesEffect::setColorMult(const Color4 &color_mult)
{
  colorMult = color_mult;
  if (fx)
    fx->setColor4Mult(&colorMult);
}

void AcesEffect::setLifeDuration(float life_duration)
{
  lifeDuration = life_duration;
  if (fx)
    fx->setParam(HUID_LIFE, &lifeDuration);
}

void AcesEffect::setFxTm(const TMatrix &tm, bool keep_original_scale)
{
#if DAGOR_DBGLEVEL > 0
  if (tm.getcol(3).lengthSq() < 0.0001f)
  {
    logerr("%s : zero pos", __FUNCTION__);
    debug_dump_stack();
  }
#endif

  isFxScaleSet = false;
  isEmitterTmSet = false;
  float oldFxScale = 1.f;
  if (keep_original_scale)
    oldFxScale = getFxScale();

  fxTm.setcol(0, scale * tm.getcol(0));
  fxTm.setcol(1, scale * tm.getcol(1));
  fxTm.setcol(2, scale * tm.getcol(2));
  fxTm.setcol(3, tm.getcol(3));

  pos = fxTm.getcol(3);
  // we need to keep sorting order stable for compound fx
  Point4 p = Point4::xyzV(pos, distanceOffsetSq);

  if (fx) // loading
  {
    fx->setTm(&fxTm);
    fx->setParam(_MAKE4C('PFXP'), &p);
  }

  // fx::setTm overrides fxScale
  if (keep_original_scale)
    setFxScale(oldFxScale);

  uint16_t &flags = getFlags();

  if (!(flags & IS_POSSET) && fx)
  {
    Point3 windSpeed = gamephys::atmosphere::get_wind();
    fx->setWind(&windSpeed);
    flags |= IS_POSSET;
  }

  setFxLightExt();
}

void AcesEffect::setFxTm(const TMatrix &tm) { setFxTm(tm, false); }

void AcesEffect::setFxTmNoScale(const TMatrix &tm) { setFxTm(tm, true); }

bool AcesEffect::getFxTm(TMatrix &tm)
{
  if (fx)
    fx->getParam(HUID_TM, &fxTm);

  tm = fxTm;
  return true;
}

bool AcesEffect::getEmmTm(TMatrix &tm)
{
  if (fx)
    fx->getParam(HUID_EMITTER_TM, &emitterTm);

  tm = emitterTm;
  return true;
}

void AcesEffect::setFxLightExt()
{
  int lightId = getLightId();
  if (lightId >= 0 && fx)
  {
    const Point3 *lightPos = get_renderer() ? static_cast<Point3 *>(fx->getParam(HUID_LIGHT_POS, NULL)) : nullptr;
    if (lightPos)
    {
      OmniLight light = get_renderer()->getOmniLight(lightId);
      constexpr float INVALIDATE_DISTANCE_SQ = LIGHT_UPDATE_DISTANCE_THRESHOLD * LIGHT_UPDATE_DISTANCE_THRESHOLD;
      if (lengthSq(Point3::xyz(light.pos_radius) - *lightPos) > INVALIDATE_DISTANCE_SQ)
      {
        light.setPos(*lightPos);
        get_renderer()->setLight(lightId, light, true);
      }
    }
  }
}


void AcesEffect::setEmitterTm(const TMatrix &tm, bool updateWind)
{
#if DAGOR_DBGLEVEL > 0
  if (tm.getcol(3).lengthSq() < 0.0001f)
  {
    logerr("%s : zero pos", __FUNCTION__);
    debug_dump_stack();
  }
#endif

  isEmitterTmSet = true;
  emitterTm = tm;
  pos = emitterTm.getcol(3);
  // we need to keep sorting order stable for compound fx
  Point4 p = Point4::xyzV(pos, distanceOffsetSq);

  if (fx)
  {
    fx->setEmitterTm(&emitterTm);
    fx->setParam(_MAKE4C('PFXP'), &p);
  }

  uint16_t &flags = getFlags();
  if ((updateWind || !(flags & IS_POSSET)) && fx)
  {
    Point3 windSpeed = gamephys::atmosphere::get_wind();
    fx->setWind(&windSpeed);
    flags |= IS_POSSET;
  }

  setFxLightExt();

  if (sfxHandle)
    sndsys::set_3d_attr(sfxHandle, pos);
}

void AcesEffect::unsetEmitter()
{
  setSpawnRate(0.f);
  destroySound();

  if (getLightId() >= 0 && fx && lightFadeout >= 0)
    fx->setParam(_MAKE4C('LFXF'), &lightFadeout);
}
void AcesEffect::unlock()
{
  uint16_t &flags = getFlags();
  flags &= ~IS_LOCKED;
  mgr->freeEffect(this, false);
}
void AcesEffect::kill()
{
  uint16_t &flags = getFlags();
  flags &= ~IS_LOCKED;
  mgr->freeEffect(this, true);
}

void AcesEffect::lock()
{
  uint16_t &flags = getFlags();
  flags |= IS_LOCKED;
}

void AcesEffect::destroySound()
{
  if (!sfxHandle)
    return;
  if (sfxAbandon)
    sndsys::abandon(sfxHandle);
  else
    sndsys::release(sfxHandle);
}
void AcesEffect::playSound(const acesfx::SoundDesc &snd_desc)
{
  destroySound();
  if (!sndsys::is_inited() || mgr->sfx.path.empty())
    return;
  sfxHandle = sndsys::init_event(mgr->sfx.path.c_str(), pos);
  if (!sfxHandle)
    return;
  sfxAbandon = mgr->sfx.abandon;
  sndsys::start(sfxHandle);
  if (snd_desc.parameter && *snd_desc.parameter)
    sndsys::set_var(sfxHandle, snd_desc.parameter, snd_desc.value);
  if (sndsys::is_one_shot(sfxHandle))
    sndsys::abandon(sfxHandle);
}

bool AcesEffect::isSoundPlaying() const { return sfxHandle && sndsys::is_playing(sfxHandle); }

EffectManager::EffectGroup::EffectGroup(uint16_t cnt) :
  count(cnt),
  effects(eastl::make_unique<AcesEffect[]>(cnt)),
  flags(eastl::make_unique<uint16_t[]>(cnt)),
  currentLifeTimes(eastl::make_unique<float[]>(cnt)),
  lights(eastl::make_unique<LightWithShadow[]>(cnt))
{}

inline void EffectManager::EffectGroup::checkAndInitRestrictionBoxes()
{
  if (!restrictionBoxes)
  {
    restrictionBoxes = eastl::make_unique<TMatrix[]>(count);
    memset(restrictionBoxes.get(), 0, sizeof(TMatrix) * count);
  }
}

void EffectManager::createFxRes(AcesEffect &eff)
{
  bool forceRecreate = false;
  effectBase->setParam(_MAKE4C('PFXR'), &forceRecreate);
  bool canCreateInstance = true;
  effectBase->setParam(_MAKE4C('PFXA'), &canCreateInstance);
  eff.fx = (BaseEffectObject *)effectBase->clone();
  effectBase->setParam(_MAKE4C('PFXA'), nullptr);
  G_ASSERT(eff.fx);

  prepFxRes(eff);

  if (params.isTrail)
    eff.fx->setParam(HUID_ACES_DISTRIBUTE_MODE, &params.isTrail);
  eff.fx->setParam(_MAKE4C('ONMV'), &params.onMoving);
  eff.fx->setParam(HUID_ACES_ALPHA_HEIGHT_RANGE, &params.alphaHeightRange);
  eff.fx->setParam(_MAKE4C('DIST'), &farPlane);
  eff.fx->setParam(_MAKE4C('GRND'), &params.groundEffect);
  eff.fx->setParam(_MAKE4C('CLBL'), &params.cloudBlend);
  eff.fx->setParam(_MAKE4C('PFXD'), &eff.softnessDepth);

  if (params.syncSubFx1ToSubFx0)
    eff.fx->setParam(_MAKE4C('SYNC'), NULL);

  // push pending data
  if (eff.getFlags() & AcesEffect::IS_ALIVE)
  {
    eff.setVelocity(eff.velocity);
    eff.setSpawnRate(eff.spawnRate);
    if (eff.gravityTm)
      eff.setGravityTm(*eff.gravityTm);
    eff.setColorMult(eff.colorMult);
    if (eff.lifeDuration >= 0)
      eff.setLifeDuration(eff.lifeDuration);
    if (!check_nan(eff.velocityScale))
      eff.setVelocityScale(eff.velocityScale);
    if (!check_nan(eff.windScale))
      eff.setWindScale(eff.windScale);

    // trying to immitate fx params calling order
    bool fxTmOk = eff.fxTm != TMatrix::IDENT;
    bool emTmOk = eff.emitterTm != TMatrix::IDENT;
    bool isFxScaleSet = eff.isFxScaleSet;

    if (!isFxScaleSet)
      eff.setFxScale(eff.fxScale);

    if (eff.isEmitterTmSet)
    {
      if (fxTmOk)
        eff.setFxTm(eff.fxTm);
      if (emTmOk)
        eff.setEmitterTm(eff.emitterTm);
    }
    else
    {
      if (emTmOk)
        eff.setEmitterTm(eff.emitterTm);
      if (fxTmOk)
        eff.setFxTm(eff.fxTm);
    }

    if (isFxScaleSet)
      eff.setFxScale(eff.fxScale);
  }
}

void EffectManager::prepFxRes(AcesEffect &eff)
{
  G_ASSERT_RETURN(eff.fx, );

  bool dafxe = true;
  eff.fx->setParam(_MAKE4C('PFXE'), &dafxe);

  bool show = eff.getFlags() & AcesEffect::IS_ALIVE;
  eff.fx->setParam(_MAKE4C('PFXF'), &show);

  eff.fx->setParam(_MAKE4C('FADE'), eff.isPlayer ? &playerFadeDistScale : &nonPlayerFadeDistScale);

  const Color4 *lightParams = static_cast<const Color4 *>(eff.fx->getParam(HUID_LIGHT_PARAMS, NULL));
  if (lightParams && lightsEnabled && get_renderer())
  {
    int newOmniLight = -1;
    const TMatrix *box = eff.getRestrictionBox();
    OmniLight light = OmniLight{Point3(0.f, 0.f, 0.f), Color3(lightParams->r, lightParams->g, lightParams->b), lightParams->a, 0, box};
    newOmniLight = ((WorldRenderer *)get_world_renderer())->addOmniLight(eastl::move(light), ~OmniLightsManager::GI_LIGHT_MASK);

    if (newOmniLight >= 0)
    {
      Color4 tmp(0.f, 0.f, 0.f);
      eff.fx->setParam(HUID_LIGHT_PARAMS, &tmp);
      allEffects[eff.groupIdx].lights[eff.fxIdx].init(newOmniLight);
    }
  }
}

void EffectManager::createNewGroup(uint16_t cnt)
{
  G_ASSERT(cnt != 0);

  EffectGroup &grp = allEffects.emplace_back(cnt);
  instancesCount += cnt;

  bool fxReady = interlocked_acquire_load(atomicInitialized) == READY;
  if (fxReady && needRestrictionBox)
    grp.checkAndInitRestrictionBoxes();

  for (int i = 0; i < cnt; i++)
  {
    AcesEffect &eff = grp.effects[i];

    eff.fx = nullptr;
    grp.flags[i] = 0;
    eff.visibilityMask = 0;
    grp.currentLifeTimes[i] = lifeTimeParam;
    eff.groupIdx = allEffects.size() - 1;
    eff.distanceOffsetSq = params.distanceOffset + RANDOM_DISTANCE_OFFSET * gfrnd();
    eff.distanceOffsetSq *= eff.distanceOffsetSq;
    eff.softnessDepth = safeinv(params.softnessDepthInv);
    eff.softnessDepthInv = params.softnessDepthInv;
    eff.animplanesZbias = params.animplanesZbias;
    eff.heightOffset = params.heightOffset;
    eff.scale = 1.f;
    eff.fxIdx = i;
    eff.mgr = this;

    if (fxReady)
      createFxRes(eff);
  }
}

void EffectManagerAsyncLoad::doJob()
{
  mgr->initAsync();

  interlocked_release_store(mgr->atomicInitialized, mgr->effectBase ? EffectManager::LOADED : EffectManager::LOAD_FAILED);
}

void EffectManager::init(const char *_name, const DataBlock *_blk, bool ground_effect, const char *effect_name)
{
  G_ASSERT(_name && *_name);
  G_ASSERT(!_blk || _blk->isValid());
  G_ASSERTF(!name[0], "Effect manager '%s' is already initialized", _name);

  G_ASSERT_RETURN(interlocked_acquire_load(atomicInitialized) == NOT_INITIALIZED, );

  const int defaultNumInstances = 100;
  const int defaultNumReserved = 10;

  name = _name;
  resName = effect_name;
  blk = _blk ? *_blk : DataBlock();
  groundEffect = ground_effect;
  asyncLoad.mgr = this;

  // need to be ready immediately
  effectBase = nullptr;
  instancesCount = playerCount = 0;
  maxNumInstances = _blk ? blk.getInt("numInstances", -1) : defaultNumInstances;
  auto initFailed = [this]() {
    maxNumInstances = maxPlayerCount = 0;
    interlocked_release_store(atomicInitialized, EffectManager::LOAD_FAILED);
  };
  if (maxNumInstances <= 0)
  {
    logerr("Wrong numInstances number for effect manager '%s'", name.c_str());
    return initFailed();
  }

  maxPlayerCount = _blk ? blk.getInt("numReserved", -1) : defaultNumReserved;
  if ((unsigned)maxPlayerCount > (unsigned)maxNumInstances)
  {
    logerr("Wrong numReserved number for effect manager '%s'", name.c_str());
    maxPlayerCount = maxNumInstances;
  }

  if (!(effect_name && *effect_name))
  {
    logerr("Effect name is not specified for '%s'", name.c_str());
    return initFailed();
  }

  params.distanceOffset = blk.getReal("sortDistanceOffset", 0.f);
  params.heightOffset = blk.getReal("heightOffset", -1.f);
  params.softnessDepthInv = safeinv(blk.getReal("softnessDepth", 0.5f));
  params.animplanesZbias = blk.getReal("animplanesZbias", 0.0005f);
  scaleRange.x = blk.getReal("minScale", 1.f);
  scaleRange.y = blk.getReal("maxScale", 1.f);
  if (scaleRange.y >= scaleRange.x)
    scaleRange.y = scaleRange.y - scaleRange.x;
  else
  {
    logerr("Wrong scale value for fx '%s'", name.c_str());
    scaleRange = Point2(1.f, 0.f);
  }

  lifeTimeParam = blk.getReal("lifeTime", 0);
  hasOnePointNumberInFxBlk = blk.findParam("onePointNumber") >= 0;
  onePointNumber = blk.getInt("onePointNumber", 0);           // TODO: drop after Flowps2 dafx effect is dropped from Enl/Cr
  onePointRadiusSq = sqr(blk.getReal("onePointRadius", 5.f)); // TODO: drop after Flowps2 dafx effect is dropped from Enl/Cr
  sfx.path = blk.getStr("sfx", "");
  sfx.abandon = blk.getBool("sfxAbandon", false);

  debug("fx: loading: %s", _name);

#if DAGOR_DBGLEVEL > 0
  if (blk.getReal("preDelayTime", -1.f) > 0.f)
    logerr("<%s>: support for fx feature '%s' was dropped", _name, "preDelayTime");
  if (blk.getReal("fadeTimeout", -1.f) > 0.f)
    logerr("<%s>: support for fx feature '%s' was dropped", _name, "fadeTimeout");
#endif

  WinCritSec &gameres_main_cs = get_gameres_main_cs();
  for (int i = 0; i < 1000; ++i)
    if (gameres_main_cs.tryLock())
    {
      if (is_game_resource_loaded_nolock(GAMERES_HANDLE_FROM_STRING(resName.c_str()), EffectGameResClassId))
        effectBase = (BaseEffectObject *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), EffectGameResClassId);
      gameres_main_cs.unlock();
      if (effectBase)
      {
        asyncLoad.doJob();
        return;
      }
      else
        break;
    }
    else
      cpu_yield();
  cpujobs::add_job(ecs::get_common_loading_job_mgr(), &asyncLoad, /*prepend*/ false, /*need_release*/ false);
}

void EffectManager::initAsync()
{
  renderTags = 1 << ERT_TAG_NORMAL;
  for (int pid = blk.findParam("tag"); pid >= 0; pid = blk.findParam("tag", pid))
    renderTags |= 1 << lup(blk.getStr(pid), render_tags, countof(render_tags), 0);
  for (int pid = blk.findParam("ntag"); pid >= 0; pid = blk.findParam("ntag", pid))
    renderTags &= ~(1 << lup(blk.getStr(pid), render_tags, countof(render_tags), 0));

  renderGroup = FX_RENDER_GROUP_LOWRES;
  const char *group = blk.getStr("group", "lowres");
  if (stricmp(group, "haze") == 0 || stricmp(name.c_str(), "haze") == 0)
    renderGroup = FX_RENDER_GROUP_HAZE;
  else if (stricmp(group, "water") == 0 || stricmp(name.c_str(), "water_foam") == 0)
    renderGroup = FX_RENDER_GROUP_WATER;
  else if (stricmp(group, "water_proj") == 0)
    renderGroup = FX_RENDER_GROUP_WATER_PROJ;

  params.alphaHeightRange = blk.getPoint2("alphaHeightRange", Point2(-10000, -10000));
  params.isTrail = blk.getBool("isTrail", false);
  params.onMoving = blk.getBool("onMoving", false);
  params.groundEffect = groundEffect;
  farPlane = blk.getReal("farPlane", 5000.f);
  playerFadeDistScale = blk.getReal("playerFadeDistScale", 1.f);
  nonPlayerFadeDistScale = blk.getReal("nonPlayerFadeDistScale", 1.f);
  params.syncSubFx1ToSubFx0 = blk.getBool("syncSubFx1ToSubFx0", false);
  params.cloudBlend = blk.getReal("cloudBlend", 0.3f);

  if (!effectBase)
  {
    effectBase = (BaseEffectObject *)get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), EffectGameResClassId);
    if (DAGOR_UNLIKELY(!effectBase))
    {
      logerr("Effect resource '%s' referenced in '%s' not found", resName.c_str(), name.c_str());
      maxNumInstances = maxPlayerCount = 0;
      return;
    }
  }
  needRestrictionBox = lightsEnabled && effectBase->getParam(HUID_LIGHT_PARAMS, nullptr) != nullptr;

  effectBase->setParam(_MAKE4C('PFXA'), nullptr);
  effectBase->getParam(_MAKE4C('DSTR'), &haze);
  effectBase->getParam(_MAKE4C('FXLM'), &maxNumInstances);
  effectBase->getParam(_MAKE4C('FXLP'), &maxPlayerCount);
  params.spawnRangeLimit = 0;
  effectBase->getParam(_MAKE4C('FXLR'), &params.spawnRangeLimit);
  params.spawnRangeLimit *= params.spawnRangeLimit;

  lightShadowParams = LightShadowParams(); // reset, if there is no shadowParams in effectBase
  LightfxShadowParams shadowParams;
  shadowParams.enabled = false;
  if (effectBase->getParam(_MAKE4C('FXSH'), &shadowParams) && shadowParams.enabled)
  {
    lightShadowParams = shadowParams;
  }

  Point2 onePointParams(0, 0);
  if (effectBase->getParam(_MAKE4C('FXLX'), &onePointParams) &&
      (/* Was set in av, default values of onePointParams in AV is (3, 5)*/ onePointParams.x != 3 || onePointParams.y != 5 ||
        /* Wasn't set in fx.blk */ !hasOnePointNumberInFxBlk))
  {
    // Override values from fx.blk with values from av
    onePointNumber = onePointParams.x;
    onePointRadiusSq = sqr(onePointParams.y);

#if DAGOR_DBGLEVEL > 0
    if (onePointNumber * onePointRadiusSq <= 0)
      logerr("wrong one point number/radius for effect manager '%s'", name.c_str());

    if (hasOnePointNumberInFxBlk)
      logerr("override one point number/radius for effect manager '%s' "
             "from fx.blk with values from game resource\n"
             "please, remove these parameters from fx.blk\n"
             "fx.blk is outdated",
        name.c_str());
#endif
  }

  if (maxNumInstances <= 0)
  {
    logerr("Wrong numInstances number for effect manager '%s'", name.c_str());
    maxNumInstances = 1;
  }

  if (maxPlayerCount > maxNumInstances)
  {
    logerr("Wrong numReserved number for effect manager '%s'", name.c_str());
    maxPlayerCount = 1;
  }
}

void EffectManager::shutdown()
{
  if (DAGOR_UNLIKELY(interlocked_acquire_load(atomicInitialized) == NOT_INITIALIZED)) // dont leave manager in half-inited state
    while (interlocked_acquire_load(atomicInitialized) == NOT_INITIALIZED)
      sleep_msec(1);
  interlocked_release_store(atomicInitialized, UNLOADED);

  // job itself can still be running, even if atomic is done
  while (cpujobs::is_job_running(ecs::get_common_loading_job_mgr(), &asyncLoad))
    sleep_msec(1);

  decltype(allEffects)().swap(allEffects);
  instancesCount = 0;

  if (effectBase)
  {
    release_game_resource((GameResource *)effectBase);
    effectBase = nullptr;
  }
}

void EffectManager::onSettingsChanged(bool shadows_enabled, int max_active_shadows)
{
  maxActiveShadows = shadows_enabled ? max_active_shadows : 0;
  shadowQuality = get_renderer()->getSettingsShadowsQuality();

  // Have to copy activeShadowVec, because LightWithShadow::releaseShadow() is modifying activeShadowVec
  ActiveShadowVec activeShadowCopy = activeShadowVec;
  for (const ActiveShadow &activeShadow : activeShadowCopy)
    activeShadow.lightWithShadow->releaseShadow(LightWithShadow::ShadowEnabled::NOT_INITED);
}

void EffectManager::update(float dt, EffectGroup &group, uint16_t &max_frames_dead)
{
  G_ASSERT(group.count);

  for (int i = 0, ie = group.count; i < ie; ++i)
  {
    uint16_t &flags = group.flags[i];
    if ((flags & (AcesEffect::IS_ALIVE | AcesEffect::IS_ACTIVE)) == 0)
      group.lights[i].reset();
    if (!(flags & AcesEffect::IS_ALIVE))
      continue;
    AcesEffect *__restrict eff = &group.effects[i];
    if (!(flags & AcesEffect::IS_LOCKED))
    {
      if (eff->queryIsActive())
        flags |= AcesEffect::IS_ACTIVE;
      else if (flags & AcesEffect::IS_ACTIVE)
      {
        if (flags & AcesEffect::IS_ALIVE)
        {
          G_ASSERT(group.alive > 0);
          group.alive--;
        }
        flags &= ~(AcesEffect::IS_ALIVE | AcesEffect::IS_ACTIVE | AcesEffect::IS_VISIBLE);
        eff->fx->setParam(_MAKE4C('PFXE'), nullptr);
        continue;
      }
    }

    flags &= ~AcesEffect::IS_FIRST_UPDATE;

    LightWithShadow &lightWithShadow = group.lights[i];
    if (lightWithShadow)
    {
      eff->fx->update(dt); // TODO: port lightfx to dafx, so we can get rid of this updates

      const Color4 *lightParams = static_cast<Color4 *>(eff->fx->getParam(HUID_LIGHT_PARAMS, NULL));
      OmniLight light;
      if (lightParams)
      {
        light = get_renderer()->getOmniLight(lightWithShadow.getLightId());
        light.setColor(Color3::rgb(*lightParams) * eff->getFxScale());
        light.setRadius(lightParams->a);
        const TMatrix *box = eff->getRestrictionBox();
        if (box && lengthSq(box->getcol(0)) > 0)
          light.setBox(*box);
        else
          light.setDefaultBox();
      }
      else
        light.setZero();
      // assumes all fx lights are highly dynamic
      get_renderer()->setLightWithMask(lightWithShadow.getLightId(), light, ~OmniLightsManager::GI_LIGHT_MASK, true);

      const Point3 *lightPos = static_cast<const Point3 *>(eff->fx->getParam(HUID_LIGHT_POS, NULL));

      if (lightParams && lightPos)
      {
        Point4 lightPosAtt = Point4::xyzV(*lightPos, lightParams->a * eff->getFxScale());
        Point4 buf[2];

        memcpy(&buf[0], &lightPosAtt, sizeof(Point4));
        memcpy(&buf[1], lightParams, sizeof(Color4));

        eff->fx->setParam(_MAKE4C('PFXL'), buf);
      }

      if (lightShadowParams.isEnabled && lightPos)
      {
        LightShadowParams lsp = lightShadowParams;
        if (shadowQuality < ShadowsQuality::SHADOWS_MEDIUM && !lsp.isDynamic)
          lsp.supportsDynamicObjects = false;
        if (shadowQuality < ShadowsQuality::SHADOWS_HIGH)
          lsp.supportsGpuObjects = false;
        lightWithShadow.activateShadow(*lightPos, lsp, resName);
      }
    }
    if (lifeTimeParam > 0.f)
    {
      group.currentLifeTimes[i] -= dt;
      if (group.currentLifeTimes[i] <= 0)
      {
        eff->unsetEmitter();
        group.currentLifeTimes[i] = lifeTimeParam;
      }
    }
  }
  if (!group.alive)
  {
    group.framesNotAlive++;
    max_frames_dead = max(max_frames_dead, group.framesNotAlive);
  }
  aliveFxCount += group.alive;
}

void EffectManager::update(float dt)
{
#if DAGOR_DBGLEVEL > 0
  G_ASSERT(name[0]);
#endif

  int atomicInitializedLocal = interlocked_acquire_load(atomicInitialized);
  if (atomicInitializedLocal <= LOAD_FAILED)
    return;

  if (atomicInitializedLocal == LOADED)
  {
    register_dafx_compound_system(effectBase, renderTags, renderGroup, blk, 0, name);

    for (auto groupIt = allEffects.begin(); groupIt != allEffects.end(); groupIt++)
    {
      EffectGroup &group = *groupIt;
      for (int i = 0, ie = group.count; i < ie; ++i)
        createFxRes(group.effects[i]);
    }

    interlocked_release_store(atomicInitialized, READY);
    blk = DataBlock();

    dafx::SystemId sys;
    if (effectBase->getParam(_MAKE4C('PFXT'), &sys) && sys)
      dafx::prefetch_system_textures(acesfx::get_dafx_context(), sys);
  }

  aliveFxCount = 0;
  if (!allEffects.size())
    return;
  uint16_t maxFramesDead = 0;
  allEffects.front().framesNotAlive = 0; // never destroy it
  for (auto &group : allEffects)
    update(dt, group, maxFramesDead);
#if _TARGET_PC | _TARGET_C2 | _TARGET_SCARLETT
  enum
  {
    MAX_FRAMES_TO_SHRINK = 4096
  }; // about a minute to shrink group
#else
  enum
  {
    MAX_FRAMES_TO_SHRINK = 2048
  }; // about a minute to shrink group
#endif
  if (maxFramesDead > MAX_FRAMES_TO_SHRINK)
  {
    for (auto groupIt = allEffects.begin(); groupIt != allEffects.end();)
      if (groupIt->framesNotAlive > MAX_FRAMES_TO_SHRINK)
      {
        G_ASSERT(instancesCount >= groupIt->count);
        instancesCount -= groupIt->count;
        allEffects.erase_unsorted(groupIt);
      }
      else
        groupIt++;
    for (int groupIdx = 0; groupIdx < allEffects.size(); ++groupIdx)
      for (int i = 0, ie = allEffects[groupIdx].count; i < ie; ++i)
        allEffects[groupIdx].effects[i].groupIdx = groupIdx;
  }
}

#if DAGOR_DBGLEVEL > 0
static void drawDebugInstanceHierarchy(
  const Point3 &pos, dafx::InstanceId iid, dafx::SystemDesc *desc, int &margin, eastl::string prefix)
{
  String resourceName;
  get_game_resource_name(desc->gameResId, resourceName);
  eastl::string description;
  description.append_sprintf("%s %s : ", prefix.c_str(), resourceName.c_str());
  description += dafx::get_instance_info(acesfx::get_dafx_context(), iid);
  eastl::vector<dafx::InstanceId> subinstances;
  dafx::get_subinstances(acesfx::get_dafx_context(), iid, subinstances);
  add_debug_text_mark(pos, description.c_str(), -1, margin++, E3DCOLOR_MAKE(0, 0, 0x40, 0xff));
  if (desc->subsystems.size() == subinstances.size())
    for (int i = 0; i < subinstances.size(); i++)
      drawDebugInstanceHierarchy(pos, subinstances[i], &desc->subsystems[i], margin, prefix + "*");
};
#endif

void EffectManager::drawDebug(DebugMode debug_mode)
{
  G_UNREFERENCED(debug_mode);
#if DAGOR_DBGLEVEL > 0
  bool one_point_radius = (int(debug_mode) & int(DebugMode::ONE_POINT_RADIUS)) != 0;
  bool detailed = (int(debug_mode) & int(DebugMode::DETAILED)) != 0;
  bool subfx = (int(debug_mode) & int(DebugMode::SUBFX_INFO)) != 0;
  float onePointRadius = 0.0f;
  E3DCOLOR onePointRadiusColor = E3DCOLOR_MAKE(0, 0, 0, 0);

  if (one_point_radius)
  {
    onePointRadius = sqrt(onePointRadiusSq);
    auto seed = static_cast<int>(reinterpret_cast<size_t>(this));
    int r = _rnd_int(seed, 128, 255);
    int g = _rnd_int(seed, 128, 255);
    int b = _rnd_int(seed, 128, 255);
    onePointRadiusColor = E3DCOLOR_MAKE(r, g, b, 255);
  }

  for (const auto &group : allEffects)
  {
    for (int i = 0, ie = group.count; i < ie; ++i)
    {
      if ((group.effects[i].fx) && (group.flags[i] & (AcesEffect::IS_ALIVE))) // might be not loaded yet
      {
        if (debug_mode != DebugMode::DEFAULT)
          add_debug_text_mark(group.effects[i].getWorldPos(), name.c_str(), -1, 0, E3DCOLOR_MAKE(0, 0, 0x40, 0xff));

        if (one_point_radius)
        {
          eastl::string text{
            eastl::string::CtorSprintf(), "onePointNumber = %d, onePointRadius = %.2f", onePointNumber, onePointRadius};
          add_debug_text_mark(group.effects[i].getWorldPos(), text.c_str(), -1, 1, onePointRadiusColor);
          if (onePointRadiusSq > 0.0f && onePointNumber > 0)
            draw_cached_debug_xz_circle(group.effects[i].getWorldPos(), onePointRadius, onePointRadiusColor);
        }

        if (!detailed && !subfx)
          continue;

        int margin = one_point_radius ? 2 : 1;
        eastl::vector<dafx::InstanceId> iids;
        group.effects[i].fx->setParam(_MAKE4C('PFXI'), &iids);
        if (detailed)
        {
          for (int j = 0; j < iids.size(); ++j)
          {
            eastl::string text;
            text.append_sprintf("%d | iid:%d", j, (uint32_t)iids[j]);
            add_debug_text_mark(group.effects[i].getWorldPos(), text.c_str(), -1, margin++, E3DCOLOR_MAKE(0, 0, 0x40, 0xff));
          }
        }

        if (!subfx)
          continue;

        eastl::vector<dafx::SystemDesc *> descriptions;
        effectBase->getParam(_MAKE4C('FXLD'), &descriptions);
        eastl::string effectManagerTags = "EffectManager tags: ";
        for (int j = 0; j < ERT_TAG_COUNT; j++)
          if (renderTags & (1 << j))
            effectManagerTags += eastl::string(render_tags[j]);
        Point3 pos = group.effects[i].getWorldPos();
        add_debug_text_mark(pos, effectManagerTags.c_str(), -1, margin++, E3DCOLOR_MAKE(0, 0, 0x40, 0xff));
        if (iids.size() == descriptions.size())
        {
          for (int j = 0; j < iids.size(); ++j)
            drawDebugInstanceHierarchy(pos, iids[j], descriptions[j], margin, "");
        }
      }
    }
  }
#endif
}

void EffectManager::EffectGroup::reset()
{
  forPlayer = alive = 0;
  for (int i = 0, ie = count; i < ie; ++i)
  {
    flags[i] = 0;
    effects[i].reset();
    lights[i].reset();
  }
  if (restrictionBoxes)
    memset(restrictionBoxes.get(), 0, sizeof(TMatrix) * count);
}

void EffectManager::reset()
{
  playerCount = 0;
  for (auto &group : allEffects)
    group.reset();
}

bool EffectManager::hasEnoughEffectsAtPoint(bool is_player, const Point3 &pos)
{
  int asyncState = interlocked_acquire_load(atomicInitialized);
  if (asyncState != READY)
    return asyncState == LOAD_FAILED ? true : false;

  if (onePointNumber == 0)
    return false;

  if (aliveFxCount >= maxNumInstances)
  {
    if (!is_player || playerCount >= maxPlayerCount)
      return true;
  }

  unsigned int numAtPoint = 0;
  for (const auto &group : allEffects)
  {
    if (!group.alive)
      continue;
    for (int i = 0, ie = group.count; i < ie; ++i)
    {
      if (!(group.flags[i] & AcesEffect::IS_ALIVE))
        continue;

      Point3 testPos = group.effects[i].getWorldPos();
      if (lengthSq(testPos - pos) < onePointRadiusSq)
      {
        numAtPoint++;
        if (numAtPoint >= onePointNumber)
          return true;
      }
    }
  }

  return false;
}

void EffectManager::killEffectsInSphere(const BSphere3 &bsph)
{
  for (auto &group : allEffects)
  {
    if (!group.alive)
      continue;
    for (int i = 0, ie = group.count; i < ie; ++i)
    {
      if (!(group.flags[i] & AcesEffect::IS_ALIVE))
        continue;

      if (bsph & group.effects[i].getWorldPos())
      {
        group.effects[i].setColorMult(Color4(0.f, 0.f, 0.f, 0.f));
        group.effects[i].setSpawnRate(0.f);
      }
    }
  }
}

AcesEffect *EffectManager::startEffect(const Point3 &pos, bool is_player, FxErrorType *perr)
{
  bool resReady = interlocked_acquire_load(atomicInitialized) == READY;
  if (resReady && !is_player && params.spawnRangeLimit > 0)
  {
    const Point3 &viewPos = get_cam_itm().getcol(3);
    float lenSq = lengthSq(viewPos - pos);

    if (lenSq > params.spawnRangeLimit)
    {
      if (perr)
        *perr = FxErrorType::FX_ERR_HAS_ENOUGH;
      return nullptr;
    }
  }

  EffectGroup *stealGroup = nullptr;
  int stealEffect = -1;
  if (aliveFxCount < instancesCount) // there is not used effect
  {
    for (auto &group : allEffects)
    {
      if (group.alive >= group.count)
        continue;
      for (int i = 0, ie = group.count; i < ie; ++i)
      {
        if (!(group.flags[i] & AcesEffect::IS_ALIVE))
        {
          stealGroup = &group;
          stealEffect = i;
          goto found;
        }
      }
    }
  found:;
  }
  else
  {
    int instancesLeft = maxNumInstances - instancesCount;
    if (instancesLeft > 0)
    {
      createNewGroup(min(instancesLeft, max(MIN_INSTANCES, instancesCount / 2))); // x1.5
      stealGroup = &allEffects.back();
      stealEffect = 0;
    }
  }
  if (stealEffect < 0)
  {
    for (auto &group : allEffects)
    {
      for (int i = 0, ie = group.count; i < ie; ++i)
        if (!(group.flags[i] & AcesEffect::IS_VISIBLE) && !(group.flags[i] & AcesEffect::IS_LOCKED))
        {
          stealGroup = &group;
          stealEffect = i;
          goto found2;
        }
    }
  found2:;
  }
  if (stealEffect < 0 && is_player && playerCount < maxPlayerCount)
  {
    for (auto &group : allEffects)
    {
      if (group.forPlayer >= group.count)
        continue;
      for (int i = 0, ie = group.count; i < ie; ++i)
        if (!(group.flags[i] & (AcesEffect::IS_LOCKED | AcesEffect::IS_PLAYER)))
        {
          stealGroup = &group;
          stealEffect = i;
          goto found3;
        }
    }
  found3:;
  }

  if (stealEffect < 0)
  {
#if DAGOR_DBGLEVEL > 0
    if (maxInstancesLimitWarning)
    {
      maxInstancesLimitWarning = false;
      logerr("fx: %s instances are over limit: %d", name, maxNumInstances);
    }
#endif
    if (perr)
      *perr = FX_ERR_HAS_ENOUGH;
    return NULL;
  }

  uint16_t &flags = stealGroup->flags[stealEffect];
  flags &= ~AcesEffect::IS_POSSET;

  if (flags & AcesEffect::IS_PLAYER)
  {
    flags &= ~AcesEffect::IS_PLAYER;
    G_ASSERT(stealGroup->forPlayer > 0);
    if (stealGroup->forPlayer > 0)
      stealGroup->forPlayer--;
  }

  if (resReady)
    stealGroup->effects[stealEffect].reset();

  stealGroup->effects[stealEffect].isPlayer = is_player;
  if (is_player)
  {
    flags |= AcesEffect::IS_PLAYER;
    G_ASSERTF(stealGroup->forPlayer < stealGroup->count, "stealGroup->forPlayer = %d (%d) stealGroup->count = %d(%d)",
      stealGroup->forPlayer, playerCount, stealGroup->count, instancesCount);
    if (stealGroup->forPlayer < stealGroup->count)
      stealGroup->forPlayer++;
    playerCount++;
  }

  if (!(flags & AcesEffect::IS_ALIVE))
  {
    stealGroup->alive++;
    aliveFxCount++;
  }
  flags |= AcesEffect::IS_ALIVE | AcesEffect::IS_FIRST_UPDATE; // Allow full update to spawn first particles and to get visibility.
  flags |= AcesEffect::IS_VISIBLE;

  stealGroup->effects[stealEffect].scale = scaleRange.x + gfrnd() * scaleRange.y;

  stealGroup->lights[stealEffect].reset();

  if (resReady)
    prepFxRes(stealGroup->effects[stealEffect]);

  return &stealGroup->effects[stealEffect];
}

void EffectManager::freeEffect(AcesEffect *eff, bool kill)
{
  if (kill || !eff->queryIsActive())
  {
    uint16_t &flags = eff->getFlags();
    if (flags & AcesEffect::IS_ALIVE)
    {
      G_ASSERT(allEffects[eff->groupIdx].alive > 0);
      allEffects[eff->groupIdx].alive--;
    }
    flags &= ~(AcesEffect::IS_ALIVE | AcesEffect::IS_ACTIVE | AcesEffect::IS_VISIBLE);
    if (eff->fx)
      eff->fx->setParam(_MAKE4C('PFXE'), nullptr);
  }
}

/*bool EffectManager::shouldGive(AcesEffect *eff)
{
  if (!(eff->flags & AcesEffect::IS_ALIVE))
    return false;

  int numLockedEffects = 0;
  for (unsigned int effectNo = 0; effectNo < effects.size(); effectNo++)
  {
    AcesEffect &testFx = effects[effectNo];

    if (testFx.flags & AcesEffect::IS_ALIVE
      && (testFx.flags & AcesEffect::IS_VISIBLE
      || testFx.flags & AcesEffect::IS_LOCKED))
    {
      numLockedEffects++;
      if (testFx.startedAt < eff->startedAt)
        return false;     // There are older effects to give.
    }
  }

  return effects.size() - numLockedEffects <= 2;
}

bool EffectManager::canStartEffect(bool is_player)
{
  if (isFull[is_player ? 0 : 1])
    return true;

  int effectToSteal = -1;
  for (int i = 0; i < effects.size(); i++)
  {
    AcesEffect &eff = effects[i];
    if (!(eff.flags & AcesEffect::IS_PLAYER))
    {
      if (!(eff.flags & AcesEffect::IS_ALIVE))
      {
        effectToSteal = i;
        break;
      }
      else if (!(eff.flags & AcesEffect::IS_VISIBLE) && !(eff.flags & AcesEffect::IS_LOCKED))
        effectToSteal = i;
    }
  }

  if (effectToSteal < 0 && is_player)
  {
    for (int i = 0; i < effects.size(); i++)
    {
      AcesEffect &eff = effects[i];
      if (eff.flags & AcesEffect::IS_PLAYER)
      {
        if (!(eff.flags & AcesEffect::IS_ALIVE))
        {
          effectToSteal = i;
          break;
        }
        else if (!(eff.flags & AcesEffect::IS_VISIBLE) && !(eff.flags & AcesEffect::IS_LOCKED))
          effectToSteal = i;
      }
      else
        break;
    }
  }

  isFull[is_player ? 0 : 1] = !(effectToSteal >= 0 && effectToSteal < effects.size());
  return !isFull[is_player ? 0 : 1];
}*/

bool EffectManager::hasVisibleEffects()
{
  for (auto &group : allEffects)
  {
    if (!group.alive)
      continue;
    for (int i = 0; i < group.count; ++i)
      if ((group.flags[i] & (AcesEffect::IS_ALIVE | AcesEffect::IS_VISIBLE)) == (AcesEffect::IS_ALIVE | AcesEffect::IS_VISIBLE))
        return true;
  }
  return false;
}

void EffectManager::playSoundOneshot(const acesfx::SoundDesc &snd_desc, const Point3 &pos) const
{
  if (sfx.path.empty())
    return;
  sndsys::EventHandle handle = sndsys::init_event(sfx.path.c_str(), nullptr, sndsys::IEF_ONESHOT, &pos);
  if (!handle)
    return;
  sndsys::start(handle);
  if (snd_desc.parameter && *snd_desc.parameter)
    sndsys::set_var(handle, snd_desc.parameter, snd_desc.value);
  sndsys::abandon(handle);
}

void EffectManager::getDebugInfo(dag::Vector<Point3, framemem_allocator> &positions,
  dag::Vector<eastl::string_view, framemem_allocator> &names)
{
  for (const auto &group : allEffects)
  {
    for (int i = 0, ie = group.count; i < ie; ++i)
    {
      if ((group.effects[i].fx) && (group.flags[i] & (AcesEffect::IS_ALIVE)))
      {
        positions.push_back(group.effects[i].getWorldPos());
        names.push_back(name.c_str());
      }
    }
  }
}


void EffectManager::getStats(eastl::string &o)
{
  o.append_sprintf("fx: %s, instances: %d, alive: %d, limit: %d", name.c_str(), instancesCount, aliveFxCount, maxNumInstances);
}
