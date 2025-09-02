// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <ecs/weather/skiesSettings.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daScattering.h>
#include "render/skies.h"
#include <render/renderEvent.h>
#include <dag_noise/dag_uint_noise.h>
#include <math/random/dag_random.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"
#include "render/world/worldRendererQueries.h"


ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
ECS_BEFORE(clouds_form_es, clouds_settings_es)
static void force_panoramic_sky_es_event_handler(const ecs::Event &, bool clouds_settings__force_panorama)
{
  G_UNUSED(clouds_settings__force_panorama);
  if (!get_daskies())
    return;
  if (auto worldRenderer = static_cast<WorldRenderer *>(get_world_renderer()))
  {
    worldRenderer->setupSkyPanoramaAndReflectionFromSetting(false);
  }
}

ECS_REQUIRE(ecs::Tag skies_settings_tag)
ECS_TRACK(*)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void sky_enable_black_sky_rendering_es(const ecs::Event &, bool sky_disable_sky_influence = false)
{
  DngSkies *skies = get_daskies();
  if (!skies)
    return;
  skies->enableBlackSkyRendering(sky_disable_sky_influence);
}

ECS_TRACK(sky_coord_frame__altitude_offset)
ECS_ON_EVENT(on_appear, EventSkiesLoaded)
static __forceinline void sky_settings_altitude_ofs_es_event_handler(const ecs::Event &, float sky_coord_frame__altitude_offset)
{
  DngSkies *skies = get_daskies();
  if (!skies)
    return;
  skies->setAltitudeOffset(sky_coord_frame__altitude_offset);
  g_entity_mgr->broadcastEventImmediate(CmdSkiesInvalidate());
}

static __forceinline void invalidate_skies_es(const CmdSkiesInvalidate &)
{
  DngSkies *skies = get_daskies();
  if (!skies)
    return;
  skies->invalidatePanorama(true);
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (renderer)
    renderer->invalidateCubeReloadOnly();
}
