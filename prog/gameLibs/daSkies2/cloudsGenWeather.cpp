// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "cloudsGenWeather.h"

#include <osApiWrappers/dag_miscApi.h>
#include <drv/3d/dag_info.h>

#include <util/dag_convar.h>

CONSOLE_INT_VAL("clouds", weather_resolution, 128, 64, 768); // for 65536 meters 128 res is enough - sufficient details (minimum
                                                             // clouds size is around 512meters)

void GenWeather::init()
{
  clouds_weather_texture.close();
  clouds_weather_texture = dag::create_tex(NULL, size, size, TEXCF_RTARGET | CLOUDS_ESRAM_ONLY, 1, "clouds_weather_texture");
  ShaderGlobal::set_sampler(::get_shader_variable_id("clouds_weather_texture_samplerstate"), d3d::request_sampler({}));
  gen_weather.init("gen_weather");
  invalidate();
}

CloudsChangeFlags GenWeather::render()
{
  if (size != weather_resolution.get())
  {
    size = weather_resolution.get();
    init();
  }
  if (frameValid)
    return CLOUDS_NO_CHANGE;

  TIME_D3D_PROFILE(gen_weather);
  if (externalWeatherTexture == BAD_TEXTUREID)
  {
    SCOPE_RENDER_TARGET;
    d3d::set_render_target(clouds_weather_texture.getTex2D(), 0);
    d3d::clearview(CLEAR_DISCARD_TARGET, 0, 0, 0);
    gen_weather.render();
    d3d::resource_barrier({clouds_weather_texture.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
  }
  frameValid = true;
  return CLOUDS_INVALIDATED;
}

bool GenWeather::setExternalWeatherTexture(TEXTUREID tid)
{
  if (externalWeatherTexture != tid)
  {
    externalWeatherTexture = tid;
    invalidate();
    ShaderGlobal::set_texture(clouds_weather_texture.getVarId(), tid == BAD_TEXTUREID ? clouds_weather_texture.getTexId() : tid);
    return true;
  }
  return false;
}