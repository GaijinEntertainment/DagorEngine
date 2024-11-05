// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/fx/postFxEvent.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <util/dag_console.h>
#include "game/player.h"
#include "game/gameEvents.h"
#include "render/renderEvent.h"
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/registerAccess.h>
#include <generic/dag_fixedVectorMap.h>

// for hero_dof
#include <ecs/core/attributeEx.h>
#include <ecs/render/updateStageRender.h>
#include <gamePhys/collision/collisionLib.h>
#include <ecs/anim/anim.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

ECS_REGISTER_EVENT(ReloadPostFx)

ECS_REQUIRE(ecs::Tag tonemapper)
ECS_TRACK(white_balance, color_grading, tonemap)
ECS_REQUIRE(ecs::Object white_balance, ecs::Object color_grading, ecs::Object tonemap)
ECS_ON_EVENT(on_appear, on_disappear)
static __forceinline void tonemap_update_es(const ecs::Event &)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
    renderer->postFxTonemapperChanged();
}

static int damage_indicatorVarId = -1;

static float get_pulsation_value(float x)
{
  // 138.1 x^6 - 414.2 x^5 + 475.8 x^4 - 261 x^3 + 62.72 x^2 - 1.38 x + 0.007625
  const float scaling = 1.0 / 1.324402; // normalization
  float ret = 138.1f;
  ret = ret * x - 414.2f;
  ret = ret * x + 475.8f;
  ret = ret * x - 261.f;
  ret = ret * x + 62.72f;
  ret = ret * x - 1.38f;
  ret = ret * x + 0.007625f;
  return ret * scaling;
}

static void set_post_fx_settings(const ecs::Object &post_fx)
{
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
    renderer->setPostFx(post_fx);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static __forceinline void post_fx_appear_es(
  const ecs::Event &, ecs::Object &generic_post_fx, ecs::Object &post_fx, ecs::EntityManager &manager)
{
  // post_fx = generic_post_fx + post_fx
  ecs::Object updatedPostFx = eastl::move(generic_post_fx);
  for (auto &i : post_fx)
    updatedPostFx.insert(i.first.c_str()) = eastl::move(i.second);
  post_fx = eastl::move(updatedPostFx);

  manager.broadcastEventImmediate(ReloadPostFx());
}


template <typename Callable>
inline void gather_override_post_fx_ecs_query(Callable c);


ECS_TAG(render)
ECS_TRACK(post_fx)
ECS_ON_EVENT(ReloadPostFx, BeforeLoadLevel) // we should init post_fx on WR creation if these objects created before
static __forceinline void post_fx_reload_es(const ecs::Event &, const ecs::Object &post_fx)
{
  damage_indicatorVarId = ::get_shader_variable_id("damage_indicator", true);

  ecs::Object updatedPostFx;                       // Container for post_fx + overrides
  const ecs::Object *finalPostFx = &updatedPostFx; // Pointer to container; or to post_fx, if no overrides

  { // Framemem scope
    using OverrideDeclaration = eastl::pair<ecs::EntityId, const ecs::Object *>;
    dag::FixedVectorMap<int, OverrideDeclaration, 4, true, framemem_allocator> overrides;

    gather_override_post_fx_ecs_query(
      [&overrides](ECS_REQUIRE(eastl::true_type override_post_fx__enabled) ecs::EntityId eid, const ecs::EntityManager &manager,
        const ecs::Object &override_post_fx__params, int override_post_fx__priority) {
        if (override_post_fx__priority < 0)
        {
          logerr("override_post_fx__priority=%d is unset for %d<%s>, value should be >= 0", override_post_fx__priority, eid,
            manager.getEntityTemplateName(eid));
        }

        while (overrides.find(override_post_fx__priority) != overrides.end())
        {
          const ecs::EntityId prevEid = overrides[override_post_fx__priority].first;
          logerr("Duplicate override_post_fx__priority=%d in %d<%s> and %d<%s>", override_post_fx__priority, eid,
            manager.getEntityTemplateName(eid), prevEid, manager.getEntityTemplateName(prevEid));
          override_post_fx__priority += 1;
        }
        overrides[override_post_fx__priority] = {eid, &override_post_fx__params};
      });

    if (overrides.empty())
      finalPostFx = &post_fx; // Use post_fx without copy
    else
    {
      // Have to copy and update post_fx
      updatedPostFx = post_fx;
      for (const auto &over : overrides)
        for (const ecs::Object::value_type &i : *over.second.second)
          updatedPostFx.insert(i.first.c_str()) = i.second;
    }
  }

  set_post_fx_settings(*finalPostFx);
}


ECS_TAG(render)
ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(*)
static __forceinline void override_post_fx_es(const ecs::Event &,
  ecs::EntityManager &manager,
  const ecs::Object &override_post_fx__params,
  int override_post_fx__priority,
  bool override_post_fx__enabled)
{
  G_UNREFERENCED(override_post_fx__params);
  G_UNREFERENCED(override_post_fx__priority);
  G_UNREFERENCED(override_post_fx__enabled);
  manager.broadcastEventImmediate(ReloadPostFx());
}


// it would actually better to be as additional template around player, so we can 'mutate' it as dof controller

template <typename Callable>
inline void animchar_ecs_query(Callable c);

ECS_ON_EVENT(on_appear)
static __forceinline void hero_dof_es(const ecs::Event &, ecs::EntityId &dof_target_id, const TMatrix &transform)
{
  float currentDist = MAX_REAL;
  Point3 pos = transform.getcol(3);
  animchar_ecs_query([&dof_target_id, &currentDist, pos](ecs::EntityId eid,
                       const TMatrix &transform ECS_REQUIRE(const AnimV20::AnimcharBaseComponent &animchar)) {
    float cDist = lengthSq(transform.getcol(3) - pos);
    if (cDist >= currentDist)
      return;
    dof_target_id = eid;
    currentDist = cDist;
  });
}

static __forceinline void hero_dof_es(const UpdateStageInfoBeforeRender &info,
  float fStop,
  float sensorHeight_mm,
  ecs::EntityId dof_target_id,
  const TMatrix &transform,
  ecs::EntityManager &manager)
{
  float focusDistance;
  Point3 target = transform.getcol(3);
  if (auto trans = manager.getNullable<TMatrix>(dof_target_id, ECS_HASH("transform")))
    target = trans->getcol(3);

  focusDistance = length(target - info.camPos);
  ecs::Object postfx;
  postfx.addMember(ECS_HASH("dof__focusDistance"), focusDistance);
  postfx.addMember(ECS_HASH("dof__fStop"), fStop);
  postfx.addMember(ECS_HASH("dof__sensorHeight_mm"), sensorHeight_mm);
  postfx.addMember(ECS_HASH("dof__focalLength_mm"), -1);
  postfx.addMember(ECS_HASH("dof__on"), true);
  if (WorldRenderer *renderer = (WorldRenderer *)get_world_renderer())
    renderer->setPostFx(postfx);
}

static int get_damage_indicator_stage(float hp, const Point3 &damage_indicator__thresholds)
{
  if (hp < damage_indicator__thresholds.z)
    return 3;
  if (hp < damage_indicator__thresholds.y)
    return 2;
  if (hp < damage_indicator__thresholds.x)
    return 1;
  return 0;
}

template <typename Callable>
static void get_hero_ecs_query(ecs::EntityId, Callable);

ECS_TAG(render)
ECS_ON_EVENT(on_appear, EventHeroChanged)
static void post_fx_blood_es(const ecs::Event &,
  const Point3 &damage_indicator__thresholds,
  float &damage_indicator__phase,
  int &damage_indicator__stage,
  float &damage_indicator__startTime,
  float &damage_indicator__prevLife,
  Point3 &damage_indicator__pulseState)
{
  // These values will cause the effect to start at the last, "infinite" pulsation mode
  // simulating that the damage had been received a long time ago

  damage_indicator__pulseState = Point3(0, 0, 0);
  damage_indicator__prevLife = 0;
  damage_indicator__startTime = 0;
  damage_indicator__stage = 0;
  damage_indicator__phase = 0;


  get_hero_ecs_query(game::get_controlled_hero(), [&](float hitpoints__maxHp = 1.f, float hitpoints__hp = 1.f) {
    float health = safediv(hitpoints__hp, hitpoints__maxHp); // relative health
    if (health > 0)
    {
      damage_indicator__stage = get_damage_indicator_stage(health, damage_indicator__thresholds);
      damage_indicator__prevLife = health;
    }
  });
  // shader var is initialized with the proper value
  // if the var id is not initialized yet, then no need to reset it
  if (damage_indicatorVarId != -1)
    ShaderGlobal::set_real(damage_indicatorVarId, 0);
}

ECS_TAG(render)
ECS_REQUIRE(float damage_indicator__phase)
ECS_ON_EVENT(on_disappear)
static void post_fx_disappear_es(const ecs::Event &) { ShaderGlobal::set_real(damage_indicatorVarId, 0); }

ECS_TAG(render)
ECS_NO_ORDER
static void post_fx_es(const ecs::UpdateStageInfoAct &info,
  ecs::EntityManager &manager,
  const Point3 &damage_indicator__thresholds,
  float damage_indicator__blockDuration,
  float &damage_indicator__phase,
  int &damage_indicator__stage,
  float &damage_indicator__startTime,
  float &damage_indicator__prevLife,
  Point3 &damage_indicator__pulseState,
  const Point4 &damage_indicator__lightPulsationFreq,
  const Point4 &damage_indicator__lightIntensities,
  const Point4 &damage_indicator__lightIntensitySaturations,
  const Point4 &damage_indicator__mediumPulsationFreq,
  const Point4 &damage_indicator__mediumIntensities,
  const Point4 &damage_indicator__mediumIntensitySaturations,
  const Point4 &damage_indicator__severePulsationFreq,
  const Point4 &damage_indicator__severeIntensities,
  const Point4 &damage_indicator__severeIntensitySaturations)
{
  ecs::EntityId hero = game::get_controlled_hero();
  const float maxHp = manager.getOr(hero, ECS_HASH("hitpoints__maxHp"), 1.0f);
  const float hp = manager.getOr(hero, ECS_HASH("hitpoints__hp"), 1.0f);
  float health = safediv(hp, maxHp); // relative health
  if (health > 0)
  {
    int stage = get_damage_indicator_stage(health, damage_indicator__thresholds);
    bool instantChange = false;
    if ((health < damage_indicator__prevLife && stage > 0) || damage_indicator__prevLife <= 0)
    {
      damage_indicator__startTime = info.curTime;
      instantChange = true;
      if (stage > damage_indicator__stage)
        damage_indicator__phase = 0;
    }
    damage_indicator__prevLife = health;
    damage_indicator__stage = stage;
    Point3 pulseState = Point3(0, 0, 0);
    if (damage_indicator__stage > 0)
    {
      float progress = info.curTime - damage_indicator__startTime;
      int block = min(int(progress / damage_indicator__blockDuration), 3);

      Point4 pulsationFreq = Point4(0, 0, 0, 0);
      Point4 intensities = Point4(0, 0, 0, 0);
      Point4 intensitySaturations = Point4(0, 0, 0, 0);

      if (damage_indicator__stage == 1)
      {
        pulsationFreq = damage_indicator__lightPulsationFreq;
        intensities = damage_indicator__lightIntensities;
        intensitySaturations = damage_indicator__lightIntensitySaturations;
      }
      else if (damage_indicator__stage == 2)
      {
        pulsationFreq = damage_indicator__mediumPulsationFreq;
        intensities = damage_indicator__mediumIntensities;
        intensitySaturations = damage_indicator__mediumIntensitySaturations;
      }
      else if (damage_indicator__stage == 3)
      {
        pulsationFreq = damage_indicator__severePulsationFreq;
        intensities = damage_indicator__severeIntensities;
        intensitySaturations = damage_indicator__severeIntensitySaturations;
      }

      pulseState[0] = pulsationFreq[block];
      pulseState[1] = intensities[block];
      pulseState[2] = intensitySaturations[block];
    }
    bool hadEffect = damage_indicator__pulseState[1] > 0;
    const float transitionSpeed = 1;
    float transition = transitionSpeed * info.dt;
    for (uint32_t i = 0; i < 3; ++i)
    {
      float diff = pulseState[i] - damage_indicator__pulseState[i];
      if (instantChange || abs(diff) <= transition)
        damage_indicator__pulseState[i] = pulseState[i];
      else
        damage_indicator__pulseState[i] += transition * sign(diff);
    }
    if (damage_indicator__pulseState[1] > 0)
    {
      damage_indicator__phase += info.dt * damage_indicator__pulseState[0];
      damage_indicator__phase -= floorf(damage_indicator__phase);

      float damageIndicatorValue = get_pulsation_value(damage_indicator__phase);
      float saturation = damage_indicator__pulseState[2];
      float intensity = damage_indicator__pulseState[1];
      damageIndicatorValue = (damageIndicatorValue * (1.f - saturation) + saturation) * intensity;
      ShaderGlobal::set_real(damage_indicatorVarId, max(0.f, damageIndicatorValue));
    }
    else if (hadEffect)
    {
      damage_indicator__startTime = 0;
      damage_indicator__stage = 0;
      damage_indicator__phase = 0;
      ShaderGlobal::set_real(damage_indicatorVarId, 0);
    }
  }
  else if (damage_indicator__pulseState[0] > 0)
  {
    damage_indicator__pulseState = Point3(0, 0, 0);
    damage_indicator__prevLife = 0;
    damage_indicator__startTime = 0;
    damage_indicator__stage = 0;
    damage_indicator__phase = 0;

    ShaderGlobal::set_real(damage_indicatorVarId, 0);
  }
}

template <typename Callable>
static inline void postfx_console_ecs_query(Callable c);

static bool postfx_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("postfx", "start_smoke", 1, 1)
  {
    ShaderGlobal::set_real(get_shader_variable_id("smoke_blackout_effect_time_start", true), get_shader_global_time_phase(0, 0));
    ShaderGlobal::set_real(get_shader_variable_id("smoke_blackout_effect_time_end", true), get_shader_global_time_phase(0, 0) + 10.0);
    ShaderGlobal::set_int(get_shader_variable_id("smoke_blackout_active", true), 1);
  }

  return found;
}

REGISTER_CONSOLE_HANDLER(postfx_console_handler);
