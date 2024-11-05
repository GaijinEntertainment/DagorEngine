// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/unique_ptr.h>
#include <EASTL/fixed_string.h>
#include <EASTL/optional.h>
#include <dag/dag_vector.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/dag_bounds3.h>
#include <math/dag_color.h>
#include <math/dag_TMatrix.h>
#include <vecmath/dag_vecMath.h>
#include <soundSystem/handle.h>
#include <util/dag_threadPool.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_atomic.h>
#include <render/lightShadowParams.h>

#include "fxErrorType.h"
#include "fx.h"
#include "lightHandle.h"

class EffectManager;
class BaseEffectObject;
class framemem_allocator;

class AcesEffect
{
  friend class EffectManager;

public:
  AcesEffect() :
    pos(0, 0, 0),
    visibilityMask(0),
    isPlayer(false),
    fx(NULL),
    distanceOffsetSq(0),
    heightOffset(0),
    softnessDepthInv(0),
    softnessDepth(0.f),
    scale(0),
    animplanesZbias(0),
    mgr(NULL),
    sfxHandle(sndsys::INVALID_SOUND_HANDLE),
    sfxAbandon(false),
    groupIdx(0),
    fxIdx(0),
    velocity(0, 0, 0),
    velocityScale(NAN),
    windScale(NAN),
    fxScale(1),
    spawnRate(1),
    colorMult(1, 1, 1, 1),
    lifeDuration(-1),
    lightFadeout(-1),
    fxTm(TMatrix::IDENT),
    emitterTm(TMatrix::IDENT),
    gravityTm(eastl::nullopt),
    isEmitterTmSet(false),
    isFxScaleSet(false)
  {}
  ~AcesEffect();

  bool isBurst() const;
  void setVelocity(const Point3 &vel);
  void setVelocityScale(const float &scale);
  void setWindScale(const float &scale);
  void setSpawnRate(float rate);
  void setLightFadeout(float light_fadeout);
  void setFxTm(const TMatrix &tm);
  void setFxTmNoScale(const TMatrix &tm);
  void setFxTm(const TMatrix &tm, bool keep_original_scale);
  bool getFxTm(TMatrix &tm);
  bool getEmmTm(TMatrix &tm);
  void setEmitterTm(const TMatrix &tm, bool updateWind = false);
  void setColorMult(const Color4 &colorMult);
  void setFxScale(float scale);
  float getFxScale() const;
  void unsetEmitter();
  void setLifeDuration(float life_duration);
  void setRestrictionBox(const TMatrix &box);
  const TMatrix *getRestrictionBox() const;
  void setGravityTm(const Matrix3 &tm);

  void playSound(const acesfx::SoundDesc &snd_desc);
  void destroySound();
  bool isSoundPlaying() const;

  void lock();
  void unlock();
  void kill();
  bool isAlive();
  bool queryIsActive() const;
  bool isUpdated() const { return (getFlags() & IS_FIRST_UPDATE) == 0; }

  const Point3 &getWorldPos() const { return pos; }
  float getDistanceOffsetSq() const { return distanceOffsetSq; }
  float getDefaultScale() const { return scale; }

protected:
  void reset();
  void resetLight(int seed);

  uint16_t &getFlags();
  uint16_t getFlags() const { return const_cast<AcesEffect *>(this)->getFlags(); }

  int getLightId() const;
  void setFxLightExt();

  static constexpr uint16_t INVALID_FX_ID = uint16_t(-1);

  uint32_t visibilityMask;
  uint16_t groupIdx = INVALID_FX_ID, fxIdx = INVALID_FX_ID;
  BaseEffectObject *fx;

  Point3 pos;

  bool isPlayer;
  bool isEmitterTmSet;
  bool isFxScaleSet;
  bool sfxAbandon;
  float distanceOffsetSq;
  float heightOffset;
  float softnessDepthInv;
  float softnessDepth;
  float scale;
  float animplanesZbias;
  float fxScale;
  float spawnRate;
  float lifeDuration;
  float lightFadeout;
  Color4 colorMult;
  Point3 velocity;
  float velocityScale;
  float windScale;
  TMatrix fxTm;
  TMatrix emitterTm;
  eastl::optional<Matrix3> gravityTm;
  class EffectManager *mgr;

  enum FxFlags
  {
    IS_ALIVE = 1,
    IS_VISIBLE = 2,
    IS_POSSET = 4,
    IS_LOCKED = 8,
    IS_ACTIVE = 16,
    IS_PLAYER = 32,
    IS_FIRST_UPDATE = 64,
  };

  sndsys::EventHandle sfxHandle;

  EA_NON_COPYABLE(AcesEffect);
};

struct EffectManagerAsyncLoad final : public cpujobs::IJob
{
  EffectManager *mgr;
  void doJob() override;
};

class EffectManager
{
public:
  friend class AcesEffect;
  friend struct EffectManagerAsyncLoad;

  FxRenderGroup renderGroup;
  uint8_t renderTags = 0;

  enum
  {
    NOT_INITIALIZED,
    LOAD_FAILED,
    LOADED,
    READY,
    UNLOADED
  };
  volatile int atomicInitialized = 0;

  EffectManager() :
    farPlane(5000.f),
    playerFadeDistScale(1.f),
    nonPlayerFadeDistScale(1.f),
    onePointNumber(0),
    haze(false),
    scaleRange(Point2(1.f, 1.f)),
    lightColor(0.f, 0.f, 0.f, 0.f),
    renderGroup(FX_RENDER_GROUP_LOWRES)
  {
#if DAGOR_DBGLEVEL > 0
    name[0] = 0;
#endif
  }

  void init(const char *_name, const DataBlock *blk, bool ground_effect, const char *fx_name);
  void shutdown();
  static void onSettingsChanged(bool shadows_enabled, int max_active_shadows);

  void update(float dt);

  // These flags can be combined with | operator
  enum class DebugMode : int
  {
    DEFAULT = 0,
    ONE_POINT_RADIUS = 1 << 0,
    DETAILED = 1 << 1,
    SUBFX_INFO = 1 << 2
  };
  void drawDebug(DebugMode debug_mode);
  void getDebugInfo(dag::Vector<Point3, framemem_allocator> &positions, dag::Vector<eastl::string_view, framemem_allocator> &names);

  AcesEffect *startEffect(const Point3 &pos, bool is_player, FxErrorType *perr = nullptr);
  void freeEffect(AcesEffect *eff, bool kill);
  bool canStartEffect(bool is_player);
  bool hasEnoughEffectsAtPoint(bool is_player, const Point3 &pos);
  bool shouldGive(AcesEffect *eff);

  void killEffectsInSphere(const BSphere3 &bsph);

#if DAGOR_DBGLEVEL > 0
  const char *getName() const { return name.c_str(); }
#endif

  void reset();
  bool hasVisibleEffects();
  bool hasHaze() { return interlocked_acquire_load(atomicInitialized) == READY && haze; }

  static void enableLights(bool v);
  ~EffectManager() { shutdown(); }

  float lifeTime() const { return lifeTimeParam; }

  void playSoundOneshot(const acesfx::SoundDesc &snd_desc, const Point3 &pos) const;
  void getStats(eastl::string &o);

  static constexpr int DEFAULT_ACTIVE_SHADOWS = 4;

protected:
  DataBlock blk;
  bool groundEffect;
  bool needRestrictionBox = false;
  eastl::string name;
  eastl::string resName;
  BaseEffectObject *effectBase;

  class LightWithShadow
  {
  public:
    enum class ShadowEnabled
    {
      NOT_INITED,
      ON,
      OFF
    };

    void init(int light_id);
    void reset();
    ~LightWithShadow();
    void activateShadow(const Point3 &pos, const LightShadowParams &lightShadowParams, const eastl::string &name);
    void releaseShadow(ShadowEnabled shadow_enabled);

    int getLightId() const;
    explicit operator bool() const;

  private:
    LightHandle lightHandle;
    ShadowEnabled shadowEnabled = ShadowEnabled::NOT_INITED;
  };

  struct ActiveShadow
  {
    LightWithShadow *lightWithShadow;
    Point3 pos;
  };
  using ActiveShadowVec = dag::RelocatableFixedVector<ActiveShadow, DEFAULT_ACTIVE_SHADOWS>;
  static ActiveShadowVec activeShadowVec;

  struct EffectGroup
  {
    // Note: all arrays/vectors of size 'count'
    uint16_t count = 0, alive = 0, forPlayer = 0, framesNotAlive = 0;
    eastl::unique_ptr<AcesEffect[]> effects;
    eastl::unique_ptr<uint16_t[]> flags;
    eastl::unique_ptr<float[]> currentLifeTimes;
    eastl::unique_ptr<LightWithShadow[]> lights;
    eastl::unique_ptr<TMatrix[]> restrictionBoxes; // optional
    EffectGroup(uint16_t cnt);
    void reset();
    void checkAndInitRestrictionBoxes();
  };
  void update(float dt, EffectGroup &group, uint16_t &max_dead);
  void render(EffectGroup &group);
  void initAsync();
  struct EffectParams
  {
    float distanceOffset = 0, heightOffset = -1, softnessDepthInv = 0.5f, animplanesZbias = 0.0005f, cloudBlend = 0.3f;
    float spawnRangeLimit = 0.0;
    Point2 alphaHeightRange = {-10000, -10000};
    bool isTrail = false, onMoving = false, groundEffect = false, syncSubFx1ToSubFx0 = false;
  } params;
  int maxNumInstances = 0, maxPlayerCount = 0;
  int instancesCount = 0, playerCount = 0;

  void createNewGroup(uint16_t cnt);
  void createFxRes(AcesEffect &eff);
  void prepFxRes(AcesEffect &eff);
  dag::Vector<EffectGroup> allEffects;
  float farPlane;
  bool haze;
  Point2 scaleRange;
  float playerFadeDistScale;
  float nonPlayerFadeDistScale;
  int onePointNumber;
  float onePointRadiusSq = 0.f;
  float lifeTimeParam;
  EffectManagerAsyncLoad asyncLoad;
  bool maxInstancesLimitWarning = true;
  bool hasOnePointNumberInFxBlk = false;

  struct SFX
  {
    eastl::fixed_string<char, 64> path;
    bool abandon = false;
  } sfx;

  Point4 lightColor;
  LightShadowParams lightShadowParams;
  int aliveFxCount = 0;

  static bool lightsEnabled;
};
