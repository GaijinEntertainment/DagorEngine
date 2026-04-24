//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

struct Color4;
struct Color3;
struct Frustum;
struct TacLaserRenderSettings;

class Point3;

namespace render::beam_tracer
{
constexpr int MIN_HEIGHT_RESOLUTION_FOR_LASER_BEAM = 720;
void init();
void close();
bool is_init();

int create_beam_tracer(const Point3 &start_pos, const Point3 &normalized_dir, float smoke_radius, const Color4 &smoke_color,
  const Color3 &head_color, float luminosity, float burn_time, float time_to_live, float fade_dist, float begin_fade_time,
  float end_fade_time, float scroll_speed, bool is_ray, float is_hero_laser = 0.f);
int update_beam_tracer_pos(unsigned id, const Point3 &pos, const Point3 &start_pos);

void after_device_reset();
void before_render(const Frustum &culling_frustum, float pixel_scale);
void render_beam_tracers();
void update_beam_tracers(float dt);
void remove_beam_tracer(int id);
void update_tac_laser_settings(const TacLaserRenderSettings &settings);
void leave_beam_tracer(unsigned id);
} // namespace render::beam_tracer