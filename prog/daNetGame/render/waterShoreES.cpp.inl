// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <fftWater/fftWater.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/unique_ptr.h>

#include "net/time.h"
#include "net/dedicated.h"
#include "main/level.h"
#include "main/water.h"
#include "render/renderer.h"
#include <gamePhys/collision/collisionLib.h>
#include <gamePhys/phys/rendinstFloating.h>
#include <daECS/core/updateStage.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

ECS_REQUIRE(const FFTWater &water)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_TRACK(shore__texture_size, shore__enabled, shore__hmap_size, shore__rivers_width, shore__significant_wave_threshold, )
static void water_shore_setup_es_event_handler(const ecs::Event &,
  int shore__texture_size,
  bool shore__enabled,
  float shore__hmap_size,
  float shore__rivers_width,
  float shore__significant_wave_threshold)
{
  if (IRenderWorld *wr = get_world_renderer())
  {
    wr->getShore()->setupShore(shore__enabled, shore__texture_size, shore__hmap_size, shore__rivers_width,
      shore__significant_wave_threshold);
  }
}

ECS_REQUIRE(const FFTWater &water)
ECS_ON_EVENT(on_appear, EventLevelLoaded)
ECS_TRACK(shore__wave_height_to_amplitude,
  shore__amplitude_to_length,
  shore__parallelism_to_wind,
  shore__width_k,
  shore__waves_dist,
  shore__waves_depth_min,
  shore__waves_depth_fade_interval,
  shore__wave_gspeed,
  shore__rivers_wave_multiplier)
static void water_shore_surf_setup_es_event_handler(const ecs::Event &,
  float shore__wave_height_to_amplitude,
  float shore__amplitude_to_length,
  float shore__parallelism_to_wind,
  float shore__width_k,
  const Point4 &shore__waves_dist,
  float shore__waves_depth_min,
  float shore__waves_depth_fade_interval,
  float shore__wave_gspeed,
  float shore__rivers_wave_multiplier)
{
  if (IRenderWorld *wr = get_world_renderer())
    wr->getShore()->setupShoreSurf(shore__wave_height_to_amplitude, shore__amplitude_to_length, shore__parallelism_to_wind,
      shore__width_k, shore__waves_dist, shore__waves_depth_min, shore__waves_depth_fade_interval, shore__wave_gspeed,
      shore__rivers_wave_multiplier);
}
