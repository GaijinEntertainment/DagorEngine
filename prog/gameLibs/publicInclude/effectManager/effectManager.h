//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <fx/dag_commonFx.h>
#include <EASTL/string.h>
#include <dag/dag_vector.h>
#include <math/dag_color.h>
#include <math/dag_Point2.h>
#include <math/dag_Point4.h>
#include <math/dag_TMatrix4.h>
#include <util/dag_threadPool.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <util/dag_generationReferencedData.h>
#include <soundSystem/varId.h>
#include <daFx/dafx.h>
#include <generic/dag_tab.h>


class AcesEffect;
class EffectManager;
class SoundExEvent;
class BaseEffectObject;
class DataBlock;
class framemem_allocator;
class TexStreamingContext;
struct SortedRenderTransItem;


class AcesEffect
{
  friend class EffectManager;

public:
  struct FxIdDummy;
  using FxId = GenerationRefId<8, FxIdDummy>;

  AcesEffect() = default;
  ~AcesEffect();
  void setFxTm(const TMatrix &tm);
  void setEmitterTm(const TMatrix &tm);
  void setFakeBrightnessBackgroundPos(const Point3 &pos);
  void setVelocity(const Point3 &vel);
  void setVelocityScaleMinMax(const Point2 &scale);
  void setSpawnRate(float rate);
  void setLightRadiusMultiplier(float multiplier);
  void setColorMult(const Color4 &colorMult);
  void setVisibility(uint32_t visibility);
  void hide(bool hidden);
  void unlock();
  void unsetEmitter();
  void stop();
  void enableActiveQuery();
  void setFxScale(float scale); // TODO: rename to setLightIntensity
  void setWindScale(float scale);
  void setGravityTm(const Matrix3 &tm);
  void setRestrictionBox(const TMatrix &box);

  bool hasSound() const;
  void pauseSound(bool pause);

  bool isAlive() const;
  bool isActive() const;
  bool isDelayed() const { return false; }

  float lifeTime() const;
  float soundEnableDistanceSq() const;
  const Point3 &getWorldPos() const;

  FxId getFxId() const { return fxId; }
  const char *getName() const;

  enum Visibility
  {
    VISIBLE = 1 << 0,
    VISIBLE_REFLECTION = 1 << 1,
  };

private:
  FxId fxId;
  int cachedFlags = 0;
  Point3 cachedPos = Point3::ZERO;
  EffectManager *mgr = nullptr;
  enum FxFlags
  {
    IS_LOCKED = 1 << 0,
    IS_ACTIVE = 1 << 1,
    IS_MARK_AS_DELETED = 1 << 2,
    ACTIVE_QUERY_ENABLED = 1 << 3,
    IS_PLAYER = 1 << 4,
  };
  EA_NON_COPYABLE(AcesEffect);
};
DAG_DECLARE_RELOCATABLE(AcesEffect);

struct EffectManagerAsyncLoad : public cpujobs::IJob
{
  EffectManager *mgr;
  const char *getJobName(bool &) const override { return "EffectManagerAsyncLoad"; }
  void doJob() override;
};

struct FxLightParams
{
  Point3 pos;
  Point3 color;
  float radius;
  float power;
};

class EffectManager
{
public:
  friend class AcesEffect;
  friend struct EffectManagerAsyncLoad;
  friend void acesfx::start_effect_client(int, const TMatrix &, const TMatrix &, bool, bool, AcesEffect **, float, FxErrorType *);
  friend void acesfx::start_effect_client(const acesfx::StartFxParams &, AcesEffect **, FxErrorType *);

  EffectManager(const char *_name, const DataBlock *blk, const char *effect_name, acesfx::PrefetchType prefetch_type);
  ~EffectManager();

  void initAsync();
  void reset();
  void shutdown();
  void force_async_load_complete();
  void updateStart(float dt);
  void updateLoading();
  void updateLights(float dt, FxLightParams *out_big_light);
  void updateActiveAndFree(int active_query_sparse_step);
  void updateDelayedLights();
  float lifeTime() const;

  AcesEffect *startEffect(const Point3 &pos, bool lock, bool is_player, FxErrorType *perr = nullptr, bool with_sound = true);
  void stopAllEffects();
  void clearAllSoundsExt();

  void killEffectsInSphere(const BSphere3 &bsph);

  int getAliveFxCount() const;
  const char *getName() const { return params.name.c_str(); }

  bool hasLockedEffects();
  static void updateCmdBuff();
  static void clearCmdBuff();

  static Point3 viewPos;

  static void onSettingsChanged(bool shadows_enabled, int max_active_shadows);
  static void onSettingsChangedExt();
  static void setMagnification(float magnification);

  void getFxMatrices(dag::Vector<TMatrix, framemem_allocator> &matrices);
  void getDebugInfo(dag::Vector<Point3, framemem_allocator> &positions, dag::Vector<int, framemem_allocator> &elems,
    dag::Vector<eastl::string_view, framemem_allocator> &names);
  void getResDebugInfo(dag::Vector<Point3, framemem_allocator> &positions, dag::Vector<eastl::string, framemem_allocator> &names,
    dag::Vector<int, framemem_allocator> &out_elems);
  int getStats(eastl::string &out);
  static void renderSortedRenderTransListExt(Tab<SortedRenderTransItem> &sorted_render_transList, TexStreamingContext &main_tex_ctx,
    bool &inside_render_trans_list_processing, bool do_clear = true);

  struct LightEffect;
  struct ActiveShadow;

private:
  enum class ShadowEnabled
  {
    NOT_INITED,
    ON,
    OFF
  };

  struct EffectParams
  {
    float distanceOffset = 0;
    float lifeTime = 0.f;
    float soundEnableDistanceSq = -1.0f;
    int onePointNumber = 0;
    float onePointRadius = 0;
    float spawnRangeLimit = 0;
    int maxNumInstances = 0;
    bool lightEnabled = false;
    char sfxPath[48] = {0};
    char sfxEvent[24] = {0};
    sndsys::VarId intenseParamId = sndsys::VarId(sndsys::INVALID_VAR_ID);
    eastl::string name;
    eastl::string resName;
    bool pauseWhenFarAway = false;
    float soundMaxDistanceSq = -1.0f;
  } params;

  struct SoundEffect;
  struct PendingData;

  struct BaseEffect
  {
    int flags = 0;
    int soundId = -1;
    int lightId = -1;
    int pendingId = -1;
    float distanceOffsetSq = 0; // TODO: move this to dafx or dafxCompound
    AcesEffect *extFx = nullptr;
    BaseEffectObject *obj = nullptr;
    TMatrix tm = TMatrix::IDENT;
    uint32_t visibility = 0xffffffff;
    dafx::InstanceId iid;
  };

  // NOTE: requires atomicInitialized >= LOADED
  bool hasEnoughEffectsAtPoint(bool is_player, const Point3 &pos);

  void destroyEffect(AcesEffect::FxId fx_id, BaseEffect &fx);
  void destroyFxSoundExt(int id, bool allow_fadeout, bool full_destroy);
  void activateShadow(LightEffect &le);
  void releaseShadow(LightEffect &le, ShadowEnabled shadow_enabled = ShadowEnabled::NOT_INITED,
    const ActiveShadow *record = nullptr) const;
  static int addLightExt(LightEffect &le);
  static void destroyLightExt(LightEffect &le);
  static void updateLightExt(LightEffect &le);
  void destroyLight(BaseEffect &fx);
  void initLightEnabledExt();
  void startEffectSoundExt(AcesEffect::FxId fx_id, BaseEffect &fx, bool with_sound, const Point3 &pos);
  void setFxTm(BaseEffect &fx, const TMatrix &tm, bool is_emitter_tm);
  void setFxTmSoundExt(BaseEffect &fx);
  void setFxSoundParamsExt(BaseEffect &fx, float delay, float intensity);
  void setFxVisibility(BaseEffect &fx, uint32_t visibility, bool force);
  void setFxFakeBrightnessBackgroundPos(BaseEffect &fx, const Point3 &backgroundPos);
  void setFxVelocity(BaseEffect &fx, const Point3 &vel);
  void setFxVelocitySoundExt(BaseEffect &fx, const Point3 &vel);
  void setFxVelocityScaleMinMax(BaseEffect &fx, const Point2 &scale);
  void setFxSpawnRate(BaseEffect &fx, float value);
  void setFxLightRadiusMultiplier(BaseEffect &fx, float multiplier);
  void setFxLightIntensity(BaseEffect &fx, float intensity);
  void setFxLightFadeout(BaseEffect &fx, float fadeout);
  void setFxLightBox(BaseEffect &fx, const TMatrix &box);
  void setFxWindScale(BaseEffect &fx, float scale);
  void setFxGravityTm(BaseEffect &fx, const Matrix3 &tm);
  void setFxColorMult(BaseEffect &fx, const Color4 &value);
  void unsetFxEmitter(BaseEffect &fx);
  void stopFx(BaseEffect &fx);
  void setFxLight(BaseEffect &fx);
  void loadSoundParamsExt();
  void tryPlayFxSoundExt(SoundEffect &se);
  void pauseFxSoundExt(BaseEffect &fx, bool paused);
  void createFxRes(AcesEffect::FxId fx_id, const Point3 &pos);
  void updateFxSoundsExt(float dt);

  void setFxTmBuff(AcesEffect::FxId fx_id, const TMatrix &tm, bool is_emitter_tm);
  void setFxVisibilityBuff(AcesEffect::FxId fx_id, uint32_t visibility, bool force);
  void unlockFxBuff(AcesEffect::FxId fx_id);
  void setFakeBrightnessBackgroundPosBuff(AcesEffect::FxId fx_id, const Point3 &value);
  void setFxVelocityBuff(AcesEffect::FxId fx_idx, const Point3 &vel);
  void setFxVelocityScaleMinMaxBuff(AcesEffect::FxId fx_id, const Point2 &scale);
  void setFxSpawnRateBuff(AcesEffect::FxId fx_id, float value);
  void setFxLightRadiusMultiplierBuff(AcesEffect::FxId fx_id, float multiplier);
  void setFxLightIntensityBuff(AcesEffect::FxId fx_id, float intensity);
  void setFxLightFadeoutBuff(AcesEffect::FxId fx_id, float fadeout);
  void setFxLightBoxBuff(AcesEffect::FxId fx_id, const TMatrix &box);
  void setFxWindScaleBuff(AcesEffect::FxId fx_id, float scale);
  void setFxGravityTmBuff(AcesEffect::FxId fx_id, const Matrix3 &tm);
  void setFxColorMultBuff(AcesEffect::FxId fx_id, const Color4 &value);
  void unsetFxEmitterBuff(AcesEffect::FxId fx_id);
  void stopFxBuff(AcesEffect::FxId fx_id);
  void setFxLightExtBuff(AcesEffect::FxId fx_id);
  void pauseFxSoundBuff(AcesEffect::FxId fx_id, bool paused);
  void enableActiveQueryBuff(AcesEffect::FxId fx_id);
  void validateDeleted(const BaseEffect &fx);
  static void updateCachedFlags(const BaseEffect &fx);

  bool loaded() const;
  bool preCheckCanSpawnAtPoint(bool is_player, const Point3 &pos) const;

  template <typename Tx, typename Ty>
  BaseEffect *remove_and_fix_subfx(int id, Tx &list, Ty &fx_list)
  {
    int last = list.size() - 1;
    if (id == last)
    {
      list.pop_back();
      return nullptr;
    }
    list[id] = list[last];
    BaseEffect *swapFx = fx_list.get(list[id].fxId);
    // G_ASSERT_RETURN(swapFx, nullptr); // temporary workaround to fix crash in EffectManager::remove_and_fix_subfx() on battle finish
    list.pop_back();
    return swapFx;
  }

  OSSpinlock mgrSLock;
  enum
  {
    NOT_INITIALIZED,
    LOAD_FAILED,
    LOADED,
    READY
  };
  volatile int atomicInitialized = 0;
  float warmupDt = 0.f;
  bool updateStarted = false;
  BaseEffectObject *effectBase = nullptr;
  EffectManagerAsyncLoad asyncLoad;
  DataBlock blk;
  GenerationReferencedData<AcesEffect::FxId, BaseEffect> fxList;
  dag::Vector<SoundEffect> soundList;
  dag::Vector<LightEffect> lightList;
  dag::Vector<PendingData> pendingList;
};
