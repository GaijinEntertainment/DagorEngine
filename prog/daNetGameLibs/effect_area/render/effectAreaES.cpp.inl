// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "effectArea.h"
#include "render/fx/fx.h"
#include <render/fx/effectEntity.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <math/random/dag_random.h>

#define SCALE_MIN 0.5
#define SCALE_MAX 1.0

static bool perform_bernouly_test(float threshold)
{
  float randomNum = rnd_float(0.0, 1.0);
  return randomNum < threshold;
}

static Point3 generate_random_point_in_box()
{
  float x = rnd_float(-0.5, 0.5);
  float y = rnd_float(-0.5, 0.5);
  float z = rnd_float(-0.5, 0.5);
  return Point3(x, y, z);
}

static Point3 generate_random_point_in_sphere()
{
  float u = rnd_float(0.0, 1.0);
  float v = rnd_float(0.0, 1.0);
  float theta = u * 2.0 * PI;
  float phi = acos(2.0 * v - 1.0);
  float r = cbrt(rnd_float(0.0, 1.0));
  float x = r * sin(phi) * cos(theta);
  float y = r * sin(phi) * sin(theta);
  float z = r * cos(phi);
  return Point3(x, y, z);
}

EffectArea::EffectArea(const ecs::EntityManager &mgr, ecs::EntityId eid)
{
  if (mgr.has(eid, ECS_HASH("box_zone")))
    getRandomPoint = generate_random_point_in_box;
  else if (mgr.getNullable<float>(eid, ECS_HASH("sphere_zone__radius")))
    getRandomPoint = generate_random_point_in_sphere;
  fxType = acesfx::get_type_by_name(mgr.getOr(eid, ECS_HASH("effect_area__effectName"), (const char *)nullptr));
}

void EffectArea::initPause(float cur_time, float pause_length_max) { timeOfUnpause = cur_time + rnd_float(0.0, pause_length_max); }

ECS_DECLARE_RELOCATABLE_TYPE(EffectArea);
ECS_REGISTER_RELOCATABLE_TYPE(EffectArea, nullptr);
ECS_AUTO_REGISTER_COMPONENT(EffectArea, "effect_area", nullptr, 0);

ECS_TAG(render)
ECS_BEFORE(before_net_phys_sync)
ECS_REQUIRE(eastl::true_type effect_area__isPaused)
void update_effect_area_on_pause_es(const ecs::UpdateStageInfoAct &act, EffectArea &effect_area, bool &effect_area__isPaused)
{
  if (effect_area.timeOfUnpause < act.curTime)
  {
    effect_area__isPaused = false;
    effect_area.timeOfUnpause = 0.0;
  }
}

ECS_TAG(render)
ECS_BEFORE(before_net_phys_sync)
ECS_REQUIRE(eastl::false_type effect_area__isPaused)
void update_effect_area_es(const ecs::UpdateStageInfoAct &act,
  EffectArea &effect_area,
  float effect_area__frequency,
  float effect_area__pauseChance,
  float effect_area__pauseLengthMax,
  bool &effect_area__isPaused,
  const TMatrix &transform)
{
  if (perform_bernouly_test(effect_area__frequency))
  {
    G_ASSERT_RETURN(effect_area.getRandomPoint, );
    Point3 pos = effect_area.getRandomPoint();
    float scale = rnd_float(SCALE_MIN, SCALE_MAX);
    acesfx::start_effect_pos(effect_area.fxType, transform * pos, false, scale);
  }
  else
  {
    if (perform_bernouly_test(effect_area__pauseChance))
    {
      effect_area.initPause(act.curTime, effect_area__pauseLengthMax);
      effect_area__isPaused = true;
    }
  }
}

ECS_TRACK(effect_area__effectName)
static void effect_area_effect_name_change_es_event_handler(
  const ecs::Event &, const ecs::string &effect_area__effectName, EffectArea &effect_area)
{
  effect_area.fxType = acesfx::get_type_by_name(effect_area__effectName.c_str());
}
