// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/type_traits.h>
#include <daSkies2/daSkiesToBlk.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <ioSys/dag_dataBlockUtils.h>
#include <util/dag_hash.h>
#include <util/dag_pull_console_proc.h>
#include "daSkiesBlkGetSet.h"

PULL_CONSOLE_PROC(convert_weather_blk_to_entity_console_handler)

namespace skies_utils
{
void get_rand_value(float &v, const DataBlock &blk, const char *name, float def, uint32_t seed, uint32_t at)
{
  v = get_point2lerp_or_real_noise(blk, name, def, seed, at);
}

static inline Point2 replace_float(Point2 pt, int a, float v)
{
  pt[a] = v;
  return pt;
}
inline void set_rand_value(DataBlock &blk, const char *name, float def, int part)
{
  blk.setPoint2(name, replace_float(blk.getPoint2(name, Point2(def, def)), part, def));
}

#define _PARAM_RAND(tp, name, def_value) \
  get_rand_value(result.name, blk, #name, def_value, seed, eastl::integral_constant<uint32_t, str_hash_fnv1(#name)>::value);
#define _PARAM_RAND_CLAMP(tp, name, def_value, min, max) \
  _PARAM_RAND(tp, name, def_value) result.name = clamp<decltype(result.name)>(result.name, min, max);
#define _PARAM(tp, name, def_value) get_value(result.name, blk, #name, def_value);
#define READ(tp, a)                      \
  tp result;                             \
  uint32_t position = str_hash_fnv1(#a); \
  G_UNUSED(seed);                        \
  G_UNUSED(position);                    \
  a;                                     \
  return result;

SkyAtmosphereParams read_sky_params(const DataBlock &blk, int seed) { READ(SkyAtmosphereParams, SKY_PARAMS) }

void read_ground_params(SkyAtmosphereParams &result, const DataBlock &blk, int seed)
{
  uint32_t position = str_hash_fnv1("GROUND_PARAMS");
  G_UNUSED(seed);
  G_UNUSED(position);
  GROUND_PARAMS
}

DaSkies::StrataClouds read_strata_clouds(const DataBlock &blk, int seed){
  READ(DaSkies::StrataClouds, STRATA_CLOUDS_PARAMS)} DaSkies::CloudsWeatherGen read_weather_gen(const DataBlock &blk, int seed){
  READ(DaSkies::CloudsWeatherGen, CLOUDS_WEATHER_GEN_PARAMS)} DaSkies::CloudsSettingsParams
  read_clouds_settings(const DataBlock &blk, int seed){READ(DaSkies::CloudsSettingsParams, CLOUDS_SETTINGS_PARAMS)} DaSkies::CloudsForm
  read_clouds_form(const DataBlock &blk, int seed){READ(DaSkies::CloudsForm, CLOUDS_FORM_PARAMS)} DaSkies::CloudsRendering
  read_clouds_rendering(const DataBlock &blk, int seed)
{
  READ(DaSkies::CloudsRendering, CLOUDS_RENDERING_PARAMS)
}

#undef _PARAM
#undef _PARAM_RAND
#undef _PARAM_RAND_CLAMP
#undef READ

#define _PARAM_RAND(tp, name, def_value)                   set_rand_value(blk, #name, result.name, part);
#define _PARAM_RAND_CLAMP(type, name, def_value, min, max) _PARAM_RAND(type, name, def_value)
#define _PARAM(tp, name, def_value)                        set_value(blk, #name, result.name);
void write_ground_params(const SkyAtmosphereParams &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  GROUND_PARAMS
}
void write_sky_params(const SkyAtmosphereParams &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  SKY_PARAMS
}
void write_strata_clouds(const DaSkies::StrataClouds &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  STRATA_CLOUDS_PARAMS
}

void write_weather_gen(const DaSkies::CloudsWeatherGen &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  CLOUDS_WEATHER_GEN_PARAMS
}
void write_clouds_settings(const DaSkies::CloudsSettingsParams &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  CLOUDS_SETTINGS_PARAMS
}
void write_clouds_form(const DaSkies::CloudsForm &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  CLOUDS_FORM_PARAMS
}
void write_clouds_rendering(const DaSkies::CloudsRendering &result, DataBlock &blk, int part)
{
  G_UNUSED(part);
  CLOUDS_RENDERING_PARAMS
}

#undef _PARAM
#undef _PARAM_RAND
#undef _PARAM_RAND_CLAMP

void load_daSkies(DaSkies &skies, const DataBlock &pt2blk, int seed, const DataBlock &levelblk)
{
  debug("load skies with %d seed", seed);
  SkyAtmosphereParams atmo = read_sky_params(*pt2blk.getBlockByNameEx("sky"), seed);
  read_ground_params(atmo, *levelblk.getBlockByNameEx("ground"), seed);
  skies.setSkyParams(atmo);
  skies.setWeatherGen(read_weather_gen(*pt2blk.getBlockByNameEx("clouds_weather_gen"), seed));
  skies.setCloudsGameSettingsParams(read_clouds_settings(*levelblk.getBlockByNameEx("clouds_settings"), seed));
  skies.setCloudsForm(read_clouds_form(*pt2blk.getBlockByNameEx("clouds_form"), seed));
  skies.setCloudsRendering(read_clouds_rendering(*pt2blk.getBlockByNameEx("clouds_rendering"), seed));

  skies.setStrataClouds(read_strata_clouds(*pt2blk.getBlockByNameEx("strata_clouds"), seed));
}

void save_daSkies(const DaSkies &skies, DataBlock &blk, bool save_min, DataBlock &levelblk)
{
  const int part = save_min ? 0 : 1;
  write_sky_params(skies.getSkyParams(), *blk.addBlock("sky"), part);
  write_ground_params(skies.getSkyParams(), *levelblk.addBlock("ground"), part);
  write_strata_clouds(skies.getStrataClouds(), *blk.addBlock("strata_clouds"), part);

  write_weather_gen(skies.getWeatherGen(), *blk.addBlock("clouds_weather_gen"), part);
  write_clouds_settings(skies.getCloudsGameSettingsParams(), *levelblk.addBlock("clouds_settings"), part);
  write_clouds_form(skies.getCloudsForm(), *blk.addBlock("clouds_form"), part);
  write_clouds_rendering(skies.getCloudsRendering(), *blk.addBlock("clouds_rendering"), part);
}

}; // namespace skies_utils