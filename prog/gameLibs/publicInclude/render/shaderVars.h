//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

struct Color3;

namespace shadervars
{
enum
{
  RENDER_TYPE_COMBINED = 0,
  RENDER_TYPE_DEPTH_W = 1,
  RENDER_TYPE_DEPTH_ZW = 2,
  RENDER_TYPE_CLASSIC = 3,
};
extern int render_type_glob_varId;

extern int fog_color_and_density_glob_varId; //<common fog

// range based fog
extern int sun_fog_color_and_density_glob_varId, sky_fog_color_and_density_glob_varId, sun_fog_azimuth_glob_varId;

// layered fog
extern int layered_fog_sun_color_glob_varId, layered_fog_sky_color_glob_varId, layered_fog_density_glob_varId,
  layered_fog_min_height_glob_varId, layered_fog_max_height_glob_varId;

void init(bool mandatory_shader_blocks = false);

void set_layered_fog(float sun_fog_azimuth, float dens, float min_height, float max_height, const Color3 &sky_color,
  const Color3 &sun_color);
void set_progammable_fog(float sun_fog_azimuth, float sky_dens, float sun_dens, const Color3 &sky_color, const Color3 &sun_color);
}; // namespace shadervars
