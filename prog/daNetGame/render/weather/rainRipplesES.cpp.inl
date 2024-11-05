// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/coreEvents.h>
#include <ecs/core/entityManager.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/render/shaderVar.h>
#include <ecs/render/postfx_renderer.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <drv/3d/dag_renderStates.h>

// we use this event localy to reduce amount of code which init/terminate rain ripples and track settings
ECS_BROADCAST_EVENT_TYPE(OnRainToggle, bool /*enabled*/)
ECS_REGISTER_EVENT(OnRainToggle)

ECS_ON_EVENT(on_appear)
ECS_REQUIRE(ecs::Tag rain_tag)
static void rain_ripples_appear_es(const ecs::Event &, ecs::EntityManager &manager) { manager.broadcastEvent(OnRainToggle(true)); }

ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag rain_tag)
static void rain_ripples_disappear_es(const ecs::Event &, ecs::EntityManager &manager) { manager.broadcastEvent(OnRainToggle(false)); }

ECS_TRACK(*)
ECS_ON_EVENT(OnRainToggle)
static void rain_ripples_toggle_es(const ecs::Event &event,
  const ecs::string &render_settings__fftWaterQuality,
  bool render_settings__fullDeferred,
  bool render_settings__rain_ripples_enabled,
  bool render_settings__bare_minimum,
  ecs::EntityManager &manager)
{
  bool rainEnabled = false;
  if (event.is<OnRainToggle>())
    rainEnabled = static_cast<const OnRainToggle &>(event).get<0>();
  else
    rainEnabled = manager.getSingletonEntity(ECS_HASH("rain_ripples")) != ecs::INVALID_ENTITY_ID;

  bool useRipplePostEffect = rainEnabled && render_settings__rain_ripples_enabled && !render_settings__bare_minimum;
  bool useRippleOnWater = useRipplePostEffect && render_settings__fullDeferred &&
                          (render_settings__fftWaterQuality == "high" || render_settings__fftWaterQuality == "ultra");

  if (!useRipplePostEffect)
    manager.destroyEntity(manager.getSingletonEntity(ECS_HASH("rain_ripples")));
  else
    manager.getOrCreateSingletonEntity(ECS_HASH("rain_ripples"));

  ShaderGlobal::set_int(get_shader_glob_var_id("water_rain_ripples_enabled", true), useRippleOnWater);
}


ECS_ON_EVENT(OnRenderDecals)
static void render_rain_ripples_es(const ecs::Event &, ShaderVar zn_zfar, const PostFxRenderer &rain_ripples_shader)
{
  TIME_D3D_PROFILE(rain_ripples);
  Color4 zn_zfar_value = ShaderGlobal::get_color4(zn_zfar);
  d3d::set_depth_bounds(0, 50.f / zn_zfar_value.g);
  rain_ripples_shader.render();
  d3d::set_depth_bounds(0, 1.0f);
}

// it' rain entity, not rain ripples
ECS_ON_EVENT(on_appear)
static void set_rain_ripples_size_es(const ecs::Event &, float rain_ripples__size)
{
  ShaderGlobal::set_real(get_shader_glob_var_id("rain_ripples_max_radius", true), rain_ripples__size);
  ShaderGlobal::set_real(get_shader_glob_var_id("droplets_scale", true), rain_ripples__size * 20.f);
}