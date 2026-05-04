// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_cpuFreq.h>
#include <perfMon/dag_statDrv.h>
#include <util/dag_string.h>
#include <math/dag_frustum.h>
#include <math/dag_half.h>
#include <shaders/dag_shaders.h>
#include "shaders/dag_computeShaders.h"
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_render.h>
#include <3d/dag_lockTexture.h>
#include <EASTL/utility.h>
#include <util/dag_convar.h>
#include "waterRenderCommon.h"

static int water_levelVarId = -1;
static int wind_dir_xVarId = -1, wind_dir_yVarId = -1;
static int wind_speedVarId = -1;
static int shoreDampVarId = -1;
static int max_wave_heightVarId = -1;
static int shoreDistanceFieldTexVarId = -1;
static int wakeHtTexVarId = -1;
static ShaderVariableInfo shore_waves_onVarId;

void WaterRenderCommon::init()
{
  if (inited)
    return;

  water_levelVarId = get_shader_variable_id("water_level", true);
  max_wave_heightVarId = get_shader_variable_id("max_wave_height", true);

  wind_dir_xVarId = get_shader_variable_id("wind_dir_x");
  wind_dir_yVarId = get_shader_variable_id("wind_dir_y");
  wind_speedVarId = get_shader_variable_id("wind_speed");

  shoreDampVarId = get_shader_variable_id("shore_damp", true);
  shore_waves_onVarId = get_shader_variable_id("shore_waves_on", true);

  shoreDistanceFieldTexVarId = get_shader_variable_id("shore_distance_field_tex");
  wakeHtTexVarId = get_shader_variable_id("wake_ht_tex", true);

  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
  ShaderGlobal::set_sampler(get_shader_variable_id("shore_distance_field_tex_samplerstate"), d3d::request_sampler(smpInfo));

  inited = true;
}

void WaterRenderCommon::setGlobalShaderConsts(bool is_chop_water) const
{
  ShaderGlobal::set_float(water_levelVarId, waterLevel);

  ShaderGlobal::set_float(wind_dir_xVarId, windDirX);
  ShaderGlobal::set_float(wind_dir_yVarId, windDirY);
  ShaderGlobal::set_float(wind_speedVarId, windSpeed);

  float effectiveMaxWaveHeight = maxWaveHeight;
  if (is_chop_water)
  {
    // more controllable shore wave visual, can be made per-bofort value
    effectiveMaxWaveHeight = min(maxWaveHeight, 1.2f);
  }

  ShaderGlobal::set_float4(shoreDampVarId, Color4(0.0f, max(min(6.0f, effectiveMaxWaveHeight * 2.0f), 1e-2f), 0, 0));

  ShaderGlobal::set_float(max_wave_heightVarId, effectiveMaxWaveHeight);

  bool isShoreOn = shoreEnabled && effectiveMaxWaveHeight > shoreWaveThreshold;
  ShaderGlobal::set_int(shore_waves_onVarId, isShoreOn ? 1 : 0);
}

WaterRenderCommon::SavedStates WaterRenderCommon::setCachedStates(TEXTUREID distanceTex) const
{
  WaterRenderCommon::SavedStates cachedState;
  cachedState.shoreDistanceFieldTexId = ShaderGlobal::get_tex(shoreDistanceFieldTexVarId);

  if (wakeHtTexId != BAD_TEXTUREID)
  {
    G_ASSERT(VariableMap::isGlobVariablePresent(wakeHtTexVarId));
    ShaderGlobal::set_texture(wakeHtTexVarId, wakeHtTexId);
  }
  if (distanceTex != BAD_TEXTUREID)
    ShaderGlobal::set_texture(shoreDistanceFieldTexVarId, distanceTex);

  return cachedState;
}

void WaterRenderCommon::resetCachedStates(const WaterRenderCommon::SavedStates &states) const
{
  if (VariableMap::isGlobVariablePresent(wakeHtTexVarId))
    ShaderGlobal::set_texture(wakeHtTexVarId, BAD_TEXTUREID);
  ShaderGlobal::set_texture(shoreDistanceFieldTexVarId, states.shoreDistanceFieldTexId);
}

void WaterRenderCommon::setWind(float wind_dir_x, float wind_dir_y, float wind_speed)
{
  windDirX = wind_dir_x;
  windDirY = wind_dir_y;
  windSpeed = wind_speed;
}

void WaterRenderCommon::setMaxWave(float max_wave_height) { maxWaveHeight = max_wave_height; }

void WaterRenderCommon::setShoreWaveThreshold(float value) { shoreWaveThreshold = value; }

void WaterRenderCommon::shoreEnable(bool enable) { shoreEnabled = enable; }