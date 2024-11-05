// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <dasModules/dasModulesCommon.h>
#include <dasModules/dasDataBlock.h>
#include <dasModules/aotDagorMath.h>
#include <render/skies.h>
#include <daSkies2/daSkiesToBlk.h>
#include <main/level.h>

MAKE_TYPE_FACTORY(DngSkies, DngSkies);
MAKE_TYPE_FACTORY(StrataClouds, DaSkies::StrataClouds);

namespace bind_dascript
{
inline void load_daSkies(DngSkies &skies, const DataBlock &pt2blk, int seed, const DataBlock &levelblk)
{
  // Upcasting DngSkies to DaSkies
  skies_utils::load_daSkies(skies, pt2blk, seed, levelblk);
}

inline float daskies_getCloudsStartAltSettings(const DngSkies &skies) { return skies.getCloudsStartAltSettings(); }

inline float daskies_getCloudsTopAltSettings(const DngSkies &skies) { return skies.getCloudsTopAltSettings(); }

inline double daskies_getStarsJulianDay(const DngSkies &skies) { return skies.getStarsJulianDay(); }

inline void daskies_setStarsJulianDay(DngSkies &skies, double julianDay) { skies.setStarsJulianDay(julianDay); }

inline bool daskies_currentGroundSunSkyColor(const DngSkies &skies,
  float &sun_cos,
  float &moon_cos,
  Color3 &sun_color,
  Color3 &sky_color,
  Color3 &moon_color,
  Color3 &moon_sky_color)
{
  return skies.currentGroundSunSkyColor(sun_cos, moon_cos, sun_color, sky_color, moon_color, moon_sky_color);
}

// if result of this function is needed to be stored, be sure to
// copy the string, as the pointer may become stale later
inline const char *das_get_current_weather_preset_name()
{
  ecs::EntityId levelEid = get_current_level_eid();
  return (g_entity_mgr->get<ecs::string>(levelEid, ECS_HASH("level__weather"))).c_str();
}
} // namespace bind_dascript