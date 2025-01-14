// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/fx/effectEntity.h"
#include "render/fx/effectManager.h"
#include "render/fx/fx.h"
#include "render/renderer.h"
#include "render/renderEvent.h"
#include "render/renderSettings.h"
#include <gameRes/dag_gameResSystem.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <gameRes/dag_stdGameResId.h>
#include <soundSystem/banks.h>
#include <math/dag_mathUtils.h>
#include "camera/sceneCam.h"
#include <gamePhys/collision/collisionLib.h>
#include <landMesh/biomeQuery.h>
#include <ecs/render/updateStageRender.h>

ECS_REGISTER_EVENT(RecreateEffectEvent)

static void set_spawn_rate(TheEffect &effect, float effect_spawn_rate)
{
  for (ScaledAcesEffect &fx : effect.getEffects())
    fx.fx->setSpawnRate(max(effect_spawn_rate, 0.0f));
}

static void set_light_fadeout(TheEffect &effect, float light_fadeout)
{
  for (ScaledAcesEffect &fx : effect.getEffects())
    fx.fx->setLightFadeout(light_fadeout);
}

static void set_wind_scale(TheEffect &effect, float wind_scale)
{
  for (ScaledAcesEffect &fx : effect.getEffects())
    fx.fx->setWindScale(wind_scale);
}

bool TheEffect::addEffect(const char *fxName,
  const TMatrix &tm,
  float fxScale,
  float fx_inst_scale,
  const Color4 &fxColorMult,
  bool is_attached,
  bool with_sound,
  const TMatrix *restriction_box,
  FxSpawnType spawnType)
{
  int fxType = acesfx::get_type_by_name(fxName);

  FxErrorType fxErr = FX_ERR_NONE;
  AcesEffect *fx = (fxType >= 0) ? acesfx::start_effect(fxType, is_attached ? TMatrix::IDENT : tm, is_attached ? tm : TMatrix::IDENT,
                                     spawnType == FxSpawnType::Player, with_sound ? &acesfx::defSoundDesc : nullptr, &fxErr)
                                 : nullptr;
  if (fx)
  {
    fx->lock();
    fx->setColorMult(fxColorMult);
    fx->setFxScale(fxScale * fx_inst_scale * length(tm.getcol(0)));
    if (restriction_box)
      fx->setRestrictionBox(*restriction_box);
    if (numEffects < countof(effectsInline))
      effectsInline[numEffects++] = ScaledAcesEffect{fx, fx_inst_scale};
    else if (numEffects == countof(effectsInline)) // inline -> heap
    {
      ScaledAcesEffect *newEffecsHeap = (ScaledAcesEffect *)memalloc(sizeof(ScaledAcesEffect) * (numEffects + 1));
      memcpy(newEffecsHeap, effectsInline, sizeof(effectsInline));
      newEffecsHeap[numEffects++] = ScaledAcesEffect{fx, fx_inst_scale};
      effectsHeap = newEffecsHeap;
    }
    else // heap -> heap (realloc)
    {
      effectsHeap = (ScaledAcesEffect *)defaultmem->realloc(effectsHeap, sizeof(ScaledAcesEffect) * (numEffects + 1));
      effectsHeap[numEffects++] = ScaledAcesEffect{fx, fx_inst_scale};
    }
    return true;
  }
#if DAGOR_DBGLEVEL > 0
  else if (fxErr != FX_ERR_HAS_ENOUGH) // hasEnoughEffectsAtPoint is not an error
    logerr("can't %s effect with name <%s> err: %d", (fxType >= 0) ? "create" : "start", fxName, fxErr);
#endif
  return false;
}

TheEffect::TheEffect(ecs::EntityManager &mgr, ecs::EntityId eid) : numEffects(0)
{
  if (mgr.getOr(eid, ECS_HASH("effect__is_paused"), false))
    return;

  const TMatrix *transform = mgr.getNullable<TMatrix>(eid, ECS_HASH("transform"));
  const Point3 *projectile__visualPos = mgr.getNullable<Point3>(eid, ECS_HASH("projectile__visualPos"));
  if (!transform && !projectile__visualPos)
  {
    logerr("Invalid effect %i <%s> without tm", eid.index(), mgr.getEntityTemplateName(eid));
    return;
  }
  TMatrix tm = transform ? *transform : TMatrix::IDENT;
  if (projectile__visualPos)
    tm.setcol(3, *projectile__visualPos);

  float fxScale = mgr.getOr(eid, ECS_HASH("effect__scale"), 1.0f);
  FxSpawnType spawnType = mgr.getOr(eid, ECS_HASH("effect__spawnOnPlayer"), false) ? FxSpawnType::Player : FxSpawnType::World;
  Color4 fxColorMult = Color4(mgr.getOr(eid, ECS_HASH("effect__colorMult"), E3DCOLOR(0xFFFFFFFF)));
  bool isAttachedEffect = mgr.has(eid, ECS_HASH("attachedEffect")) || mgr.has(eid, ECS_HASH("attachedNoScaleEffect"));
  const bool withSound = !mgr.has(eid, ECS_HASH("sound_control__shouldPlay")) && mgr.getOr(eid, ECS_HASH("effect__withSound"), true);
  TMatrix restrictionBox;
  const TMatrix *boxPtr = nullptr;

  if (mgr.has(eid, ECS_HASH("effect__box")))
  {
    if (mgr.getOr(eid, ECS_HASH("effect__use_box"), false))
    {
      boxPtr = &restrictionBox;
      restrictionBox = mgr.getOr(eid, ECS_HASH("effect__box"), TMatrix::ZERO);
      bool automaticBox = mgr.getOr(eid, ECS_HASH("effect__light__automatic_box"), false);
      if (lengthSq(restrictionBox.getcol(0)) <= 0.0001f)
        automaticBox = true;
      if (automaticBox)
      {
        // Note: world level might be not yet created since level isn't created either
        create_world_renderer()->getBoxAround(tm.getcol(3), restrictionBox);
      }
    }
  }
  bool hasEffectComponents = false;
  if (const char *fxName = mgr.getOr(eid, ECS_HASH("effect__name"), (const char *)nullptr))
  {
    hasEffectComponents = true;
    addEffect(fxName, tm, fxScale, 1.f, fxColorMult, isAttachedEffect, withSound, boxPtr, spawnType);
  }
  if (const ecs::Object *effectsObject = mgr.getNullable<ecs::Object>(eid, ECS_HASH("effects")))
    for (auto &effect : *effectsObject)
    {
      hasEffectComponents = true;
      addEffect(effect.first.c_str(), tm, fxScale, effect.second.get<float>(), fxColorMult, isAttachedEffect, withSound, boxPtr,
        spawnType);
    }

  const float *spawnRate = mgr.getNullable<float>(eid, ECS_HASH("effect__spawnRate"));
  if (spawnRate)
    set_spawn_rate(*this, *spawnRate);

  const float *lightFadeout = mgr.getNullable<float>(eid, ECS_HASH("effect__lightFadeout"));
  if (lightFadeout)
  {
    if (*lightFadeout < 0.0f)
      logerr("Entity %d (%s): effect__lightFadeout should be more than 0.0, current value=%f", eid, mgr.getEntityTemplateName(eid),
        *lightFadeout);

    set_light_fadeout(*this, *lightFadeout);
  }

  const float *windScale = mgr.getNullable<float>(eid, ECS_HASH("effect__windScale"));
  if (windScale)
    set_wind_scale(*this, *windScale);

  if (DAGOR_UNLIKELY(!hasEffectComponents))
    logerr("Entity %d (%s) inited an effect but had no 'effect__name' or 'effects' components", eid, mgr.getEntityTemplateName(eid));
}

TheEffect::TheEffect(AcesEffect &&effect) : numEffects(1)
{
  effectsInline[0].fx = &effect;
  effectsInline[0].scale = 1.f;
  effect.lock();
}

/*static*/
void TheEffect::requestResources(const char *, const ecs::ResourceRequestCb &res_cb)
{
  auto addFxRes = [&](const eastl::string &fx_name) {
    if (!fx_name.empty())
      if (const char *fxResName = (acesfx::get_type_by_name(fx_name.c_str()) >= 0) ? acesfx::get_fxres_name(fx_name.c_str()) : nullptr)
        res_cb(fxResName, EffectGameResClassId);
  };

  eastl::string emptystr;
  addFxRes(res_cb.getOr(ECS_HASH("effect__name"), emptystr));

  ecs::Object emptyobj;
  for (auto &effect : res_cb.getOr(ECS_HASH("effects"), emptyobj))
    res_cb(effect.first.c_str(), EffectGameResClassId);
}

TheEffect::TheEffect(TheEffect &&other)
{
  memcpy(this, &other, sizeof(*this));
  other.numEffects = 0;
}

TheEffect &TheEffect::operator=(TheEffect &&other)
{
  if (this != &other)
  {
    this->~TheEffect();
    memcpy(this, &other, sizeof(*this));
    other.numEffects = 0;
  }
  return *this;
}

TheEffect::~TheEffect() { reset(); }

void TheEffect::reset()
{
  for (auto &fx : getEffects())
    acesfx::stop_effect(fx.fx);
  if (numEffects > countof(effectsInline))
    memfree(effectsHeap, defaultmem);
  numEffects = 0;
}

ECS_REGISTER_RELOCATABLE_TYPE(TheEffect, nullptr);

ECS_AUTO_REGISTER_COMPONENT(TheEffect, "effect", nullptr, 0);

ECS_NO_ORDER
__forceinline void effect_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &transform ECS_REQUIRE_NOT(ecs::Tag staticEffect) ECS_REQUIRE_NOT(ecs::Tag attachedEffect)
    ECS_REQUIRE_NOT(ecs::Tag attachedNoScaleEffect) ECS_REQUIRE_NOT(TMatrix effect_animation__transform))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setEmitterTm(transform);
}

ECS_NO_ORDER
void effect_debug_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &transform ECS_REQUIRE(ecs::Tag staticEffect) ECS_REQUIRE(ecs::Tag daeditor__selected) ECS_REQUIRE_NOT(
    ecs::Tag attachedEffect) ECS_REQUIRE_NOT(ecs::Tag attachedNoScaleEffect) ECS_REQUIRE_NOT(TMatrix effect_animation__transform))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setEmitterTm(transform);
}

ECS_NO_ORDER
__forceinline void effect_attached_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &transform ECS_REQUIRE(ecs::Tag attachedEffect) ECS_REQUIRE_NOT(TMatrix effect_animation__transform))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setFxTm(transform);
}

ECS_AFTER(effect_attached_es)
__forceinline void effect_attached_no_scale_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &transform ECS_REQUIRE(ecs::Tag attachedNoScaleEffect) ECS_REQUIRE_NOT(TMatrix effect_animation__transform))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setFxTmNoScale(transform);
}

ECS_AFTER(effect_attached_es)
__forceinline void effect_anim_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &effect_animation__transform ECS_REQUIRE_NOT(ecs::Tag attachedEffect) ECS_REQUIRE_NOT(ecs::Tag attachedNoScaleEffect))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setEmitterTm(effect_animation__transform);
}

ECS_AFTER(effect_attached_es)
__forceinline void effect_attached_anim_es(
  const ecs::UpdateStageInfoAct &, TheEffect &effect, const TMatrix &effect_animation__transform ECS_REQUIRE(ecs::Tag attachedEffect))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setFxTm(effect_animation__transform);
}

ECS_AFTER(effect_attached_es)
__forceinline void effect_attached_no_scale_anim_es(const ecs::UpdateStageInfoAct &,
  TheEffect &effect,
  const TMatrix &effect_animation__transform ECS_REQUIRE(ecs::Tag attachedNoScaleEffect))
{
  for (auto &fx : effect.getEffects())
    fx.fx->setFxTmNoScale(effect_animation__transform);
}

ECS_AFTER(update_projectile_es)
__forceinline void effect_projectile_waiter_es(const ecs::UpdateStageInfoAct &,
  ecs::EntityId eid,
  const ecs::string &effect__templateAfterSuccessWait,
  const Point3 &projectile__visualPos)
{
  if (projectile__visualPos.lengthSq() > 0.0)
  {
    g_entity_mgr->reCreateEntityFromAsync(eid, effect__templateAfterSuccessWait.c_str());
  }
}

ECS_AFTER(effect_projectile_waiter_es)
__forceinline void effect_projectile_es(const ecs::UpdateStageInfoAct &, TheEffect &effect, const Point3 &projectile__visualPos)
{
  for (auto &fx : effect.getEffects())
  {
    TMatrix tm = TMatrix::IDENT;
    tm.setcol(3, tm.getcol(3) + projectile__visualPos);
    fx.fx->setEmitterTm(tm);
  }
}

ECS_TRACK(effect__scale, transform)
static inline void effect_scale_es_event_handler(const ecs::Event &, TheEffect &effect, const TMatrix &transform, float effect__scale)
{
  for (auto &fx : effect.getEffects())
    fx.fx->setFxScale(effect__scale * fx.scale * length(transform.getcol(0)));
}

ECS_TRACK(effect__velocity)
static inline void effect_velocity_es_event_handler(const ecs::Event &, TheEffect &effect, const Point3 &effect__velocity)
{
  for (auto &fx : effect.getEffects())
    fx.fx->setVelocity(effect__velocity);
}

ECS_TRACK(effect__distance_scale)
static inline void effect_velocity_scale_es_event_handler(const ecs::Event &, TheEffect &effect, const float &effect__distance_scale)
{
  for (auto &fx : effect.getEffects())
    fx.fx->setVelocityScale(effect__distance_scale);
}

ECS_TRACK(effect__windScale)
static inline void effect_wind_scale_es_event_handler(const ecs::Event &, TheEffect &effect, const float &effect__windScale)
{
  for (auto &fx : effect.getEffects())
    fx.fx->setWindScale(effect__windScale);
}

ECS_TRACK(effect__colorMult)
static inline void effect_colorMult_es_event_handler(const ecs::Event &, TheEffect &effect, const E3DCOLOR effect__colorMult)
{
  for (auto &fx : effect.getEffects())
    fx.fx->setColorMult(Color4(effect__colorMult));
}

ECS_TRACK(sound_control__shouldPlay)
static inline void effect_sound_enabled_es_event_handler(const ecs::Event &, TheEffect &effect, bool sound_control__shouldPlay)
{
  if (sound_control__shouldPlay)
    for (auto &fx : effect.getEffects())
      fx.fx->playSound({});
  else
    for (auto &fx : effect.getEffects())
      fx.fx->destroySound();
}

ECS_TRACK(effect__spawnRate)
static void effect_spawnrate_es_event_handler(const ecs::Event &, TheEffect &effect, float effect__spawnRate)
{
  set_spawn_rate(effect, effect__spawnRate);
}

ECS_ON_EVENT(UpdateEffectRestrictionBoxes)
ECS_TRACK(effect__box, transform, effect__use_box, effect__automatic_box)
static inline void effect_box_es_event_handler(const ecs::Event &,
  TheEffect &effect,
  const TMatrix &transform,
  const TMatrix &effect__box,
  bool effect__use_box,
  bool effect__automatic_box)
{
  TMatrix restrictionBox;
  if (!effect__use_box)
    restrictionBox.zero();
  else if (effect__automatic_box)
  {
    if (!get_world_renderer()->getBoxAround(transform.getcol(3), restrictionBox))
      restrictionBox.zero();
  }
  else
    restrictionBox = effect__box;
  for (auto &fx : effect.getEffects())
    fx.fx->setRestrictionBox(restrictionBox);
}


ECS_TAG(render)
ECS_BEFORE(effect_es)
ECS_AFTER(camera_animator_update_es)
static inline void bound_camera_effect_es(const ecs::UpdateStageInfoAct &info,
  Point3 &effect__offset,
  Point3 &camera_prev_pos,
  Point3 &camera_prev_vel,
  TMatrix &transform,
  const float effect__smooth_coef,
  const float effect__bias_coef)
{
  TMatrix camTm = get_cam_itm();
  Point3 velocityVector = clamp(((camTm.getcol(3) - camera_prev_pos) * safeinv(info.dt)), -100.0f, 100.0f);
  camera_prev_pos = camTm.getcol(3);
  velocityVector = effect__smooth_coef * velocityVector + (1 - effect__smooth_coef) * camera_prev_vel;
  if (velocityVector.y >= -3.0f && velocityVector.y <= 3.0f)
    velocityVector.y = 0; // for cases when changes of velocity on very big we are do not move effect on y axis
  camera_prev_vel = velocityVector;
  Point3 res_offset = (velocityVector * effect__bias_coef) + effect__offset;
  TMatrix resTm = transform;
  resTm.setcol(3, (camTm.getcol(3) + res_offset));
  transform = resTm;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void biome_query_init_es(
  const ecs::Event &, TMatrix transform, int &biome_query__state, int &biome_query__id, float biome_query__radius)
{
  biome_query__state = int(GpuReadbackResultState::IN_PROGRESS);
  biome_query__id = biome_query::query(transform.getcol(3), biome_query__radius);
}

ECS_TAG(render)
ECS_TRACK(biome_query__state)
static inline void create_fx_based_on_biome_es(const ecs::Event &,
  ecs::EntityId eid,
  TMatrix transform,
  int biome_query__state,
  int biome_query__groupId,
  Point4 biome_query__color,
  const ecs::string &hit_fx_name,
  const ecs::string &biome_query__desiredBiomeName = "")
{
  if (biome_query__state == int(GpuReadbackResultState::IN_PROGRESS))
  {
    return;
  }
  else if (biome_query__state == int(GpuReadbackResultState::SUCCEEDED))
  {
    if (!biome_query__desiredBiomeName.empty() &&
        biome_query__groupId != biome_query::get_biome_group_id(biome_query__desiredBiomeName.c_str()))
    {
      g_entity_mgr->destroyEntity(eid);
    }
    else
    {
      ecs::ComponentsInitializer attrs;
      attrs[ECS_HASH("transform")] = transform;
      attrs[ECS_HASH("paint_color")] = biome_query__color;
      g_entity_mgr->reCreateEntityFromAsync(eid, hit_fx_name.c_str(), eastl::move(attrs));
    }
  }
  else
  {
    g_entity_mgr->destroyEntity(eid);
  }
}

ECS_TAG(render)
ECS_NO_ORDER
static inline void biome_query_update_es(const ecs::UpdateStageInfoAct &,
  int &biome_query__state,
  int biome_query__id,
  Point4 &biome_query__color,
  int &biome_query__groupId,
  int &biome_query__liveFrames)
{
  if (biome_query__state != int(GpuReadbackResultState::IN_PROGRESS))
    return;

  BiomeQueryResult biomeQueryResult;
  GpuReadbackResultState biomeQueryResultState = biome_query::get_query_result(biome_query__id, biomeQueryResult);
  if (::is_gpu_readback_query_successful(biomeQueryResultState))
  {
    biome_query__color =
      Point4(biomeQueryResult.averageBiomeColor.x, biomeQueryResult.averageBiomeColor.y, biomeQueryResult.averageBiomeColor.z, 1.0f);
    biome_query__groupId = biomeQueryResult.mostFrequentBiomeGroupIndex;
  }

  biome_query__state = int(biomeQueryResultState);
  biome_query__liveFrames -= 1;
  if (biome_query__liveFrames <= 0)
    biome_query__state = int(GpuReadbackResultState::FAILED);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void paint_hit_fx_es(
  const ecs::Event &, TheEffect &effect, Point4 paint_color, bool paint_from_biome = false, bool usePaintColor = false)
{
  if (!(paint_from_biome || usePaintColor))
    return;

  for (auto &fx : effect.getEffects())
    fx.fx->setColorMult(Color4::xyzw(paint_color));
}

ECS_TAG(render)
ECS_ON_EVENT(ecs::EventEntityManagerBeforeClear)
static inline void destroy_effects_es(const ecs::Event &, TheEffect &effect) { effect.reset(); }

ECS_TAG(dev, render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag autodeleteEffectEntity, float effect__emit_range_limit)
static inline void validate_emit_range_limit_es(const ecs::Event &, ecs::EntityId eid)
{
  logerr("effect__emit_range_limit doesn't work for autodelete effects; "
         "entities will be destroyed right after pause and cannot be resumed: eid=%d, template='%s'\n"
         "It looks like, that you try to use range_pauseable_effect with temporary effect; "
         "it's bad idea, because we cannot rewind effect to actual time on resume; "
         "range_pauseable_effect makes sense only for permanent environment effects\n"
         "Set spawn_range_limit in effect game resource (in av) instead of effect__emit_range_limit "
         "for entities temporary effects with 'autodeleteEffectEntity' tag",
    static_cast<ecs::entity_id_t>(eid), g_entity_mgr->getEntityTemplateName(eid));
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static inline void pause_effects_set_sq_es(const ecs::Event &,
  ecs::EntityId eid,
  ecs::EntityManager &manager,
  float effect__emit_range_limit,
  float effect__emit_range_gap,
  float &effect__emit_range_limit_start_sq,
  float &effect__emit_range_limit_stop_sq)
{
  G_UNUSED(eid);
  G_UNUSED(manager);

  if (effect__emit_range_limit <= 0.0)
  {
#if DAGOR_DBGLEVEL > 0
    logerr("Expected positive effect__emit_range_limit for eid=%d, template='%s'\n"
           "Please set positive distance value for effect__emit_range_limit or remove it from template",
      static_cast<ecs::entity_id_t>(eid), manager.getEntityTemplateName(eid));
#endif
    return;
  }

  effect__emit_range_limit_start_sq = sqr(effect__emit_range_limit * (1 - effect__emit_range_gap));
  effect__emit_range_limit_stop_sq = sqr(effect__emit_range_limit * (1 + effect__emit_range_gap));
}

template <typename Callable>
static void pause_effects_ecs_query(Callable c);

ECS_TAG(render)
ECS_AFTER(after_camera_sync)
static inline void pause_effects_es(const ecs::UpdateStageInfoAct &, ecs::EntityManager &manager)
{
  const TMatrix &camItm = get_cam_itm();
  const Point3 &cameraPos = camItm.getcol(3);

  pause_effects_ecs_query([&cameraPos, &manager](ecs::EntityId eid, const TMatrix &transform, float effect__emit_range_limit_start_sq,
                            float effect__emit_range_limit_stop_sq, TheEffect &effect, bool &effect__is_paused) {
    if (effect__emit_range_limit_start_sq <= 0)
      return;

    const Point3 &effectPos = transform.getcol(3);
    float distanceSq = lengthSq(effectPos - cameraPos);

    if (!effect__is_paused)
    {
      if (distanceSq > effect__emit_range_limit_stop_sq)
      {
        // Pause effect
        effect__is_paused = true;
        effect.reset();
      }
    }
    else
    {
      if (distanceSq < effect__emit_range_limit_start_sq)
      {
        // Restart effect
        effect__is_paused = false; // This line should be before TheEffect() constructor,
                                   // because TheEffect() checks component effect__is_paused of eid
        effect = TheEffect(manager, eid);
      }
    }
  });
}

ECS_TAG(render)
ECS_ON_EVENT(RecreateEffectEvent)
static inline void recreate_effect_es(const ecs::Event &, ecs::EntityManager &manager, ecs::EntityId eid, TheEffect &effect)
{
  effect = TheEffect(manager, eid);
}

ECS_TAG(render)
ECS_BEFORE(effect_es)
static inline void alwasy_in_viewport_effects_es(const ecs::UpdateStageInfoAct &,
  TMatrix &transform ECS_REQUIRE(ecs::Tag alwaysInViewportEffect))
{
  const TMatrix &camItm = get_cam_itm();
  Point3 dirOffsetedPos = camItm.getcol(3) + camItm.getcol(2);
  transform.setcol(3, dirOffsetedPos);
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__effectsShadows, render_settings__effects__max_active_shadows, render_settings__shadowsQuality)
ECS_REQUIRE(const ecs::string &render_settings__shadowsQuality)
ECS_AFTER(init_shadows_es)
static void effect_manager_settings_es(
  const ecs::Event &, bool render_settings__effectsShadows, int render_settings__effects__max_active_shadows)
{
  EffectManager::onSettingsChanged(render_settings__effectsShadows, render_settings__effects__max_active_shadows);
}