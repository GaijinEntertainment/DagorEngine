// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/beamTracer/beamTracerClient.h"
#include <util/dag_convar.h>
#include <perfMon/dag_statDrv.h>
#include "beamTracers.h"

#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>

CONSOLE_FLOAT_VAL_MINMAX("render", beam_tracer_exp_framerate, 60.f, 1.f, 120.f);
namespace render::beam_tracer
{
struct Context
{
  BeamTracerManager beam_tracers;
};

eastl::unique_ptr<Context> g_ctx;

void init()
{
  if (g_ctx)
    return;

  g_ctx.reset(new Context());
}

void close() { g_ctx.reset(); }

bool is_init() { return g_ctx != nullptr; }

void after_device_reset()
{
  if (!g_ctx)
    return;

  g_ctx->beam_tracers.afterDeviceReset();
}

int create_beam_tracer(const Point3 &start_pos, const Point3 &normalized_dir, float smoke_radius, const Color4 &smoke_color,
  const Color3 &head_color, float luminosity, float burn_time, float time_to_live, float fade_dist, float begin_fade_time,
  float end_fade_time, float scroll_speed, bool is_ray, float is_hero_laser)
{
  return g_ctx ? g_ctx->beam_tracers.createTracer(start_pos, normalized_dir, smoke_radius, smoke_color, head_color, luminosity,
                   burn_time, time_to_live, fade_dist, begin_fade_time, end_fade_time, scroll_speed, is_ray, is_hero_laser)
               : -1;
}

int update_beam_tracer_pos(unsigned id, const Point3 &pos, const Point3 &start_pos)
{
  return g_ctx ? g_ctx->beam_tracers.updateTracerPos(id, pos, start_pos) : -1;
}

void before_render(const Frustum &culling_frustum, float pixel_scale)
{
  if (!g_ctx)
    return;
  TIME_D3D_PROFILE(prepareBeamTracers);
  float exposure_time = 1 / beam_tracer_exp_framerate; // technically, exposure_time is either fixed (for fixed frame rate), or real
  // rendering frame time. however, due to framerate instability, we'd better use one
  // fixed 60fps
  g_ctx->beam_tracers.beforeRender(culling_frustum, exposure_time, pixel_scale);
}

void render_beam_tracers()
{
  if (!g_ctx)
    return;
  TIME_D3D_PROFILE(beamTracers);
  g_ctx->beam_tracers.renderTrans();
}

void update_beam_tracers(float dt)
{
  if (!g_ctx)
    return;

  g_ctx->beam_tracers.update(dt);
}

void remove_beam_tracer(int id)
{
  if (!g_ctx)
    return;
  g_ctx->beam_tracers.removeTracer(id);
}

void update_tac_laser_settings(const TacLaserRenderSettings &settings)
{
  if (!g_ctx)
    return;
  g_ctx->beam_tracers.updateTacLaserRenderSettings(settings);
}

void leave_beam_tracer(unsigned id)
{
  if (!g_ctx)
    return;

  g_ctx->beam_tracers.leaveTracer(id);
}
} // namespace render::beam_tracer

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_BEFORE(tac_laser_update_render_settings_es)
ECS_REQUIRE(float tac_laser_render__min_intensity_angle)
static void tac_laser_init_es(const ecs::Event &) { render::beam_tracer::init(); }

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(float tac_laser_render__min_intensity_angle)
static void tac_laser_close_es(const ecs::Event &) { render::beam_tracer::close(); }

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_TRACK(*)
static void tac_laser_update_render_settings_es(const ecs::Event &, const float tac_laser_render__min_intensity_angle,
  const float tac_laser_render__min_intensity_mul, const float tac_laser_render__max_intensity_angle,
  const float tac_laser_render__max_intensity_mul, const float tac_laser_render__fog_intensity_mul,
  const float tac_laser_render__fade_dist)
{
  TacLaserRenderSettings s;
  s.minIntensityCos = cosf(clamp(tac_laser_render__min_intensity_angle, 0.0f, 90.0f) * DEG_TO_RAD);
  s.minIntensityMul = tac_laser_render__min_intensity_mul;
  s.maxIntensityCos = cosf(clamp(tac_laser_render__max_intensity_angle, 0.0f, 90.0f) * DEG_TO_RAD);
  s.maxIntensityMul = tac_laser_render__max_intensity_mul;
  s.fogIntensityMul = tac_laser_render__fog_intensity_mul;
  s.fadeDistance = tac_laser_render__fade_dist;

  render::beam_tracer::update_tac_laser_settings(s);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag beam_tracer_projectile)
static void beam_tracer_projectile_init_es(const ecs::Event &) { render::beam_tracer::init(); }

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag beam_tracer_projectile)
static void beam_tracer_projectile_close_es(const ecs::Event &) { render::beam_tracer::close(); }
