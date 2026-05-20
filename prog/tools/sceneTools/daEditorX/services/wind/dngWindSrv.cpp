// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "windSrv.h"

class DngBasedWindService : public WindBaseService
{
public:
  DngBasedWindService() : WindBaseService() {}

  void term() {}

  void update() {}

private:
  void applyWind(const WindSettings &settings, WindSettings::Ambient &currentAmbient) const override
  {
    if (!g_entity_mgr)
      return;

    ecs::EntityId windId = g_entity_mgr->getSingletonEntity(ECS_HASH("wind"));
    if (!windId)
      return;

    const float strength = settings.enabled ? settings.windStrength : 0.f;
    g_entity_mgr->set(windId, ECS_HASH("wind__strength"), strength);
    g_entity_mgr->set(windId, ECS_HASH("wind__dir"), settings.windAzimuth);
    g_entity_mgr->set(windId, ECS_HASH("wind__noiseStrength"), settings.ambient.windNoiseStrength);
    g_entity_mgr->set(windId, ECS_HASH("wind__noiseSpeed"), settings.ambient.windNoiseSpeed);
    g_entity_mgr->set(windId, ECS_HASH("wind__noiseScale"), settings.ambient.windNoiseScale);
    g_entity_mgr->set(windId, ECS_HASH("wind__noisePerpendicular"), settings.ambient.windNoisePerpendicular);
    g_entity_mgr->set(windId, ECS_HASH("wind__left_top_right_bottom"), settings.ambient.windMapBorders);
    g_entity_mgr->set(windId, ECS_HASH("wind__flowMap"), ecs::string(settings.ambient.windFlowTextureName.c_str()));
  }
};

static DngBasedWindService srv;

void *get_dng_based_wind_service() { return &srv; }
