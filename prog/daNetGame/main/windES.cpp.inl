// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <util/dag_console.h>
#include <gamePhys/props/atmosphere.h>
#include <render/wind/ambientWind.h>
#include <render/fx/fx.h>
ECS_DECLARE_RELOCATABLE_TYPE(AmbientWind);
ECS_REGISTER_RELOCATABLE_TYPE(AmbientWind, nullptr);

ECS_NO_ORDER
ECS_TAG(render)
static inline void wind_update_es(const ecs::UpdateStageInfoAct &, AmbientWind &ambient_wind, float wind__dir = 0)
{
  ambient_wind.update();
  Point2 windDir = Point2(cosf(DegToRad(wind__dir)), sinf(DegToRad(wind__dir)));
  acesfx::setWindParams(ambient_wind.getWindStrength(), windDir);
}

ECS_NO_ORDER
ECS_ON_EVENT(on_appear)
ECS_TRACK(wind__strength,
  wind__noiseStrength,
  wind__noiseSpeed,
  wind__noiseScale,
  wind__noisePerpendicular,
  wind__dir,
  wind__left_top_right_bottom,
  wind__flowMap)
static inline void wind_es_event_handler(const ecs::Event &,
  AmbientWind &ambient_wind,
  float wind__strength,
  float wind__noiseStrength,
  float wind__noiseSpeed,
  float wind__noiseScale,
  float wind__noisePerpendicular,
  float wind__dir = 0,
  const Point4 &wind__left_top_right_bottom = Point4(-2048, -2048, 2048, 2048),
  const ecs::string &wind__flowMap = "")
{
  Point2 windDir = Point2(cosf(DegToRad(wind__dir)), sinf(DegToRad(wind__dir)));
  Point3 windDir3 = Point3(windDir.x, 0.f, windDir.y);


  gamephys::atmosphere::set_wind(windDir3, wind__strength);
  const char *textureName = wind__flowMap.c_str();
  ambient_wind.setWindParameters(wind__left_top_right_bottom, Point2(cosf(DegToRad(wind__dir)), sinf(DegToRad(wind__dir))),
    wind__strength, wind__noiseStrength, wind__noiseSpeed, wind__noiseScale, wind__noisePerpendicular);
  ambient_wind.setWindTextures(textureName);
}

ECS_REQUIRE(float wind__strength)
ECS_ON_EVENT(on_disappear)
static inline void destroy_wind_managed_resources_es_event_handler(const ecs::Event &, AmbientWind &ambient_wind)
{
  ambient_wind.close();
}

static bool change_wind_parameters_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("wind", "dir", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float dir;
    if (argc > 1)
    {
      dir = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__dir"), dir);
    }
    else
      dir = g_entity_mgr->getOr(windId, ECS_HASH("wind__dir"), 0.f);
    console::print_d("Wind dir for level is set to %.3f", dir);
  }
  CONSOLE_CHECK_NAME("wind", "strength", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float strength;
    if (argc > 1)
    {
      strength = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__strength"), strength);
    }
    else
      strength = g_entity_mgr->getOr(windId, ECS_HASH("wind__strength"), 0.f);
    console::print_d("Wind strength for level is set to %.3f", strength);
  }
  CONSOLE_CHECK_NAME("wind", "noise_strength", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float noiseStrength;
    if (argc > 1)
    {
      noiseStrength = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__noiseStrength"), noiseStrength);
    }
    else
      noiseStrength = g_entity_mgr->getOr(windId, ECS_HASH("wind__noiseStrength"), 0.f);
    console::print_d("Wind noise strength for level is set to %.3f", noiseStrength);
  }
  CONSOLE_CHECK_NAME("wind", "noise_speed", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float noiseSpeed;
    if (argc > 1)
    {
      noiseSpeed = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__noiseSpeed"), noiseSpeed);
    }
    else
      noiseSpeed = g_entity_mgr->getOr(windId, ECS_HASH("wind__noiseSpeed"), 0.f);
    console::print_d("Wind noise speed for level is set to %.3f", noiseSpeed);
  }
  CONSOLE_CHECK_NAME("wind", "noise_scale", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float noiseScale;
    if (argc > 1)
    {
      noiseScale = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__noiseScale"), noiseScale);
    }
    else
      noiseScale = g_entity_mgr->getOr(windId, ECS_HASH("wind__noiseScale"), 0.f);
    console::print_d("Wind noise scale for level is set to %.3f", noiseScale);
  }
  CONSOLE_CHECK_NAME("wind", "noise_perpendicular", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    float noisePerpendicular;
    if (argc > 1)
    {
      noisePerpendicular = atof(argv[1]);
      g_entity_mgr->set(windId, ECS_HASH("wind__noisePerpendicular"), noisePerpendicular);
    }
    else
      noisePerpendicular = g_entity_mgr->getOr(windId, ECS_HASH("wind__noisePerpendicular"), 0.f);
    console::print_d("Wind noise perpendicular for level is set to %.3f", noisePerpendicular);
  }
  CONSOLE_CHECK_NAME("wind", "flowmap_tex", 1, 2)
  {
    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    if (argc > 1)
    {
      bool disableTex = stricmp("-1", argv[1]) == 0;
      g_entity_mgr->setOptional(windId, ECS_HASH("wind__flowMap"), disableTex ? "" : ecs::string(argv[1]));
      if (g_entity_mgr->has(windId, ECS_HASH("wind__flowMap")))
        console::print_d("Wind flowmap texture is set to %s", disableTex ? "NULL" : argv[1]);
      else
        console::print_d("Could not find wind__flowMap component");
    }
    else
    {
      const char *texName = g_entity_mgr->getOr(windId, ECS_HASH("wind__flowMap"), (const char *)nullptr);
      console::print_d("Wind flowmap texture is %s", texName ? texName : "NULL, using direction");
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(change_wind_parameters_console_handler);
