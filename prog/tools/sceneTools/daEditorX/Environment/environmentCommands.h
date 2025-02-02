// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <oldEditor/de_cm.h>

// commands
enum
{
  CM_TOOL = CM_PLUGIN_BASE,
  CM_IMPORT,
  CM_SHOW_PANEL,
  CM_ERASE_ENVI,
  CM_RESET_GIZMO,
  CM_PID_NAME_GRP,
  CM_PID_NAME,
  CM_PID_GEOMETRY_GRP,
  CM_PID_GEOMETRY_POS,
  CM_PID_GEOMETRY_RADIUS,
  CM_PID_GEOMETRY_HEIGHT,
  CM_PID_FOG_GRP,
  CM_PID_FOG_USE,
  CM_PID_FOG_EXP2,
  CM_PID_USE_SUN_FOG,
  CM_PID_SUN_FOG_DENSITY,
  CM_PID_SKY_FOG_DENSITY,
  CM_PID_SUN_FOG_COLOR,
  CM_PID_SKY_FOG_COLOR,
  CM_PID_LFOG_COL1,
  CM_PID_LFOG_COL2,
  CM_PID_LFOG_DENSITY,
  CM_PID_LFOG_MIN_HEIGHT,
  CM_PID_LFOG_MAX_HEIGHT,
  CM_PID_SCENE_GRP,
  CM_PID_SCENE_ZNEAR,
  CM_PID_SCENE_ZFAR,
  CM_PID_SCENE_ROTY,
  CM_PID_SCENE_EXPORT,
  CM_PID_ZNZF_SCALE,
  CM_PID_SUN_GRP,
  CM_PID_SUN_GRP_AZIMUTH,
  CM_PID_SUN_GRP_ZENITH,
  CM_PID_SUN2_GRP,
  CM_PID_SUN2_GRP_AZIMUTH,
  CM_PID_SUN2_GRP_ZENITH,
  CM_PID_SKY_GRP,
  CM_PID_SKY_HDR_MULTIPLIER,
  // CM_PID_DEFAULTS,
  CM_PID_SUN_AZIMUTH,
  CM_PID_GET_AZIMUTH_FROM_LIGHT,

  CM_PID_PREVIEW_FOG,

  CM_SHOW_POSTFX_PANEL,
  CM_SHOW_SHGV_PANEL,
  CM_HDR_VIEW_SETTINGS,
  CM_HTTP_GAME_SERVER,

  CM_COLOR_CORRECTION,

  CM_PID_ENVIRONMENTS,
  CM_PID_WEATHER_PRESETS_LIST,
  CM_PID_WEATHER_TYPES_LIST,
  CM_PID_ENVIRONMENTS_LIST,
  CM_PID_ENV_FORCE_WEATHER,
  CM_PID_ENV_FORCE_GEO_DATE,
  CM_PID_ENV_FORCE_TIME,
  CM_PID_ENV_FORCE_SEED,
  CM_PID_ENV_LATITUDE,
  CM_PID_ENV_YEAR,
  CM_PID_ENV_MONTH,
  CM_PID_ENV_DAY,
  CM_PID_ENV_TIME_OF_DAY,
  CM_PID_ENV_RND_SEED,

  PID_DIRECT_LIGHTING,
  PID_SYNC_DIR_LIGHT_DIR,
  PID_SYNC_DIR_LIGHT_COLOR,
  PID_DIR_SUN1_COLOR,
  PID_DIR_SUN1_COLOR_MUL,
  PID_DIR_SUN2_COLOR,
  PID_DIR_SUN2_COLOR_MUL,
  PID_DIR_SKY_COLOR,
  PID_DIR_SKY_COLOR_MUL,
  PID_DIR_SUN1_DIR,
  PID_DIR_SUN2_DIR,
};
